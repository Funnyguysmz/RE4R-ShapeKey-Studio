param(
    [string]$Zig='zig',
    [Parameter(Mandatory=$true)][string]$LuaSourceDirectory,
    [Parameter(Mandatory=$true)][string]$WebView2Directory
)
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$sdk=[IO.Path]::GetFullPath($WebView2Directory)
[xml]$manifest=[IO.File]::ReadAllText((Join-Path $sdk 'Microsoft.Web.WebView2.nuspec'))
if($manifest.package.metadata.id -ne 'Microsoft.Web.WebView2' -or $manifest.package.metadata.version -ne '1.0.4258.31'){
    throw 'Use the official Microsoft.Web.WebView2 SDK 1.0.4258.31 package.'
}
$native=Join-Path $sdk 'build\native'
foreach($file in @('include\WebView2.h','x64\WebView2Loader.dll')){
    if(!(Test-Path -LiteralPath (Join-Path $native $file) -PathType Leaf)){throw "Missing WebView2 SDK file: $file"}
}
# Keep the native 0.3 fallback in build/package; build all runtime binaries from source.
& "$root\Build.ps1" -Zig $Zig -LuaSourceDirectory $LuaSourceDirectory
$out=Join-Path $root 'build\v04\package'
New-Item -ItemType Directory -Path "$out\ui","$out\third_party\webview2" -Force | Out-Null
Push-Location $root
try {
    & $Zig rc /fo 'build\v04\studio.res' 'src\studio.rc'
    if($LASTEXITCODE){throw 'Icon resource build failed'}
} finally {Pop-Location}
& $Zig cc -target x86_64-windows-gnu -O2 -I "$native\include" "$root\src\host_webview.c" "$root\build\v04\studio.res" -municode '-Wl,--subsystem,windows' -lole32 -lshell32 -lcomdlg32 -luser32 -lgdi32 -ldwmapi -liphlpapi -lws2_32 -ladvapi32 -o "$out\RE4R_ShapeKeyStudio.exe"
if($LASTEXITCODE){throw 'WebView2 host build failed'}
Copy-Item -LiteralPath "$native\x64\WebView2Loader.dll" -Destination $out -Force
Copy-Item -Path "$root\src\ui\*" -Destination "$out\ui" -Recurse -Force
Copy-Item -LiteralPath "$root\assets\Studio_icon.png" -Destination "$out\ui" -Force
# Public demo uses channel metadata only; no model textures or geometry.
$demo=foreach($line in [IO.File]::ReadAllLines("$root\assets\demo_profile.tsv",[Text.Encoding]::UTF8)){
    $f=$line.Split([char]9)
    if($f.Length -ge 9 -and $f[0] -eq 'K'){
        [ordered]@{id=$f[1];label=$f[2];region=$f[3];group=$f[8];mesh=[int]$f[7];manual=[double]::Parse($f[4],[Globalization.CultureInfo]::InvariantCulture)}
    }
}
$json=@($demo) | ConvertTo-Json -Compress
[IO.File]::WriteAllText("$out\ui\demo.js","window.DEMO_KEYS = $json;`n",[Text.UTF8Encoding]::new($false))
foreach($name in @('Install.ps1','runtime_adapter.lua','receiver_adapter.lua','Studio.ico','Lua_LICENSE.txt','REFramework_LICENSE','RhyLive_LICENSE','README.md')){
    Copy-Item -LiteralPath "$root\build\package\$name" -Destination $out -Recurse -Force
}
Get-ChildItem -LiteralPath "$root\build\package\reframework" -File -Recurse | Where-Object Extension -In @('.dll','.lua','.json') | ForEach-Object {
    $relative=$_.FullName.Substring(("$root\build\package").Length+1)
    $target=Join-Path $out $relative
    New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force | Out-Null
    Copy-Item -LiteralPath $_.FullName -Destination $target -Force
}
# Preserve compiler symbols outside the package, including from earlier builds.
$symbols=Join-Path $root 'build\v04\symbols'
New-Item -ItemType Directory -Path $symbols -Force | Out-Null
Get-ChildItem -LiteralPath $out -File -Recurse | Where-Object Extension -In @('.pdb','.lib') | ForEach-Object {
    $target=Join-Path $symbols ($_.Name+'.'+[guid]::NewGuid().ToString('N'))
    Move-Item -LiteralPath $_.FullName -Destination $target
}
foreach($name in @('LICENSE.txt','NOTICE.txt','Microsoft.Web.WebView2.nuspec')){
    Copy-Item -LiteralPath "$root\third_party\webview2\$name" -Destination "$out\third_party\webview2" -Force
}
Copy-Item -LiteralPath "$root\ThirdPartyNotices.md" -Destination $out -Force
Copy-Item -LiteralPath "$root\LICENSE" -Destination $out -Force
[IO.File]::WriteAllText("$out\VERSION.txt","0.4.1`n",[Text.UTF8Encoding]::new($false))
Write-Output "BUILD_V04_PASS $out"
