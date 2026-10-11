param([string]$OutputZip)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$package=Join-Path $root 'build\v04\package'
if(!$OutputZip){$OutputZip=Join-Path $root 'build\release\RE4R_ShapeKeyStudio_0.4.0.zip'}
$OutputZip=[IO.Path]::GetFullPath($OutputZip)
if(Test-Path -LiteralPath $OutputZip){throw 'Output ZIP already exists; preserve it before creating another package.'}
New-Item -ItemType Directory -Path (Split-Path $OutputZip -Parent) -Force | Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem
Add-Type -AssemblyName System.IO.Compression
$files=@('RE4R_ShapeKeyStudio.exe','WebView2Loader.dll','Install.ps1','VERSION.txt','LICENSE','ThirdPartyNotices.md','README.md','Lua_LICENSE.txt','REFramework_LICENSE','RhyLive_LICENSE','runtime_adapter.lua','receiver_adapter.lua','Studio.ico',
    'ui/index.html','ui/app.js','ui/app.css','ui/demo.js','ui/Studio_icon.png',
    'third_party/webview2/LICENSE.txt','third_party/webview2/NOTICE.txt','third_party/webview2/Microsoft.Web.WebView2.nuspec',
    'reframework/plugins/ShapeKeyBridge.dll','reframework/plugins/RhyLiveFace.dll',
    'reframework/autorun/RE4R_ShapeKeySystem.lua','reframework/autorun/RhyLiveFace.lua',
    'reframework/autorun/RhyLiveFace/default_profile.lua','reframework/autorun/RhyLiveFace/face_math.lua','reframework/autorun/RhyLiveFace/localization.lua')
foreach($file in $files){if(!(Test-Path -LiteralPath (Join-Path $package $file) -PathType Leaf)){throw "Missing build file: $file"}}
$zip=[IO.Compression.ZipFile]::Open($OutputZip,[IO.Compression.ZipArchiveMode]::Create)
try {
    foreach($file in $files){
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip,(Join-Path $package $file),$file,[IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
} finally {$zip.Dispose()}
& "$PSScriptRoot\Check-Release.ps1" -ZipPath $OutputZip
Write-Output "PACKAGE_PASS $OutputZip"
