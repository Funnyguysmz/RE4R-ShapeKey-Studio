param([string]$Zig='zig',[Parameter(Mandatory=$true)][string]$LuaSourceDirectory)
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$apiHeader=Join-Path $root 'vendor\REFramework\API.h'
if(!(Test-Path -LiteralPath $apiHeader -PathType Leaf)){throw "Missing bundled REFramework header: $apiHeader. Restore vendor\REFramework\API.h from the source checkout."}
$lua=[IO.Path]::GetFullPath($LuaSourceDirectory)
if(!(Test-Path -LiteralPath "$lua\lapi.c")){throw 'LuaSourceDirectory must contain Lua 5.4 lapi.c and lua.h'}
$out=Join-Path $root 'build'
$package=Join-Path $out 'package'
New-Item -ItemType Directory -Path "$package\reframework\plugins","$package\reframework\autorun\RhyLiveFace","$out\tests" -Force | Out-Null
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $root 'build\cache\global'
$env:ZIG_LOCAL_CACHE_DIR=Join-Path $root 'build\cache\local'
$luaSources=@(Get-ChildItem -LiteralPath $lua -Filter '*.c' | Where-Object Name -NotIn @('lua.c','luac.c','onelua.c','ltests.c') | ForEach-Object FullName)
$common=@('cc','-target','x86_64-windows-gnu','-O2','-I',"$root\vendor\REFramework",'-I',$lua)
function Invoke-Zig {param([string[]]$Arguments) & $Zig @Arguments; if($LASTEXITCODE){throw "Zig command failed ($LASTEXITCODE)"}}
Push-Location $root
try{Invoke-Zig -Arguments @('rc','/fo','build\studio.res','src\studio.rc')}finally{Pop-Location}
Invoke-Zig ($common+@("$root\src\studio.c","$out\studio.res",'-municode','-Wl,--subsystem,windows','-lcomctl32','-lcomdlg32','-lgdiplus','-lshell32','-lgdi32','-luser32','-lole32','-liphlpapi','-lws2_32','-o',"$package\RE4R_ShapeKeyStudio.exe"))
Invoke-Zig ($common+@('-shared',"$root\src\bridge.c")+$luaSources+@('-o',"$package\reframework\plugins\ShapeKeyBridge.dll"))
Invoke-Zig ($common+@('-shared',"$root\src\receiver.c")+$luaSources+@('-lws2_32','-o',"$package\reframework\plugins\RhyLiveFace.dll"))
Invoke-Zig ($common+@('-DSTUDIO_TEST_PIPE',"$root\tests\bridge_test.c","$root\src\bridge.c")+$luaSources+@('-o',"$out\tests\bridge_test.exe"))
$runner=if(Test-Path -LiteralPath "$lua\lua.c"){"$lua\lua.c"}else{"$root\tests\lua_runner.c"}
Invoke-Zig ($common+$luaSources+@($runner,'-o',"$out\lua54.exe"))
foreach($spec in @(@('RE4R_ShapeKeySystem.lua','runtime_adapter.lua'),@('RhyLiveFace.lua','receiver_adapter.lua'))){
    $text=[IO.File]::ReadAllText("$root\runtime\$($spec[0])")+"`n"+[IO.File]::ReadAllText("$root\src\$($spec[1])")
    [IO.File]::WriteAllText("$package\reframework\autorun\$($spec[0])",$text,[Text.UTF8Encoding]::new($false))
    Copy-Item -LiteralPath "$root\src\$($spec[1])" -Destination $package -Force
}
foreach($module in @('face_math.lua','default_profile.lua','localization.lua')){Copy-Item -LiteralPath "$root\runtime\RhyLiveFace\$module" -Destination "$package\reframework\autorun\RhyLiveFace\$module" -Force}
foreach($asset in @('Studio_icon.png','Studio.ico','demo_profile.tsv')){Copy-Item -LiteralPath "$root\assets\$asset" -Destination $package -Force}
New-Item -ItemType Directory -Path "$package\reframework\data" -Force | Out-Null
Copy-Item -LiteralPath "$root\runtime\data\RE4R_ShapeKeySystem_native_leon.json" -Destination "$package\reframework\data" -Force
$optionalNative=Join-Path $root 'runtime\data\RE4R_ShapeKeySystem_native_leon_mouth9.json'
if(Test-Path -LiteralPath $optionalNative){Copy-Item -LiteralPath $optionalNative -Destination "$package\reframework\data" -Force}
Copy-Item -LiteralPath "$root\Install.ps1" -Destination $package -Force
Copy-Item -LiteralPath "$root\README.md" -Destination $package -Force
foreach($license in @('REFramework_LICENSE','RhyLive_LICENSE')){Copy-Item -LiteralPath "$root\vendor\$license" -Destination $package -Force}
$luaHeader=[IO.File]::ReadAllText("$lua\lua.h")
$license=[regex]::Match($luaHeader,'(?s)Copyright \(C\) [^\r\n]+Lua.org, PUC-Rio\..*?SOFTWARE\.').Value
if($license.Length -lt 500){throw 'Lua copyright notice not found; do not distribute the package.'}
[IO.File]::WriteAllText("$package\Lua_LICENSE.txt",$license,[Text.UTF8Encoding]::new($false))
Write-Output "BUILD_PASS $package"
