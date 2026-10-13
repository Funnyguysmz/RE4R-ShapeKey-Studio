param([Parameter(Mandatory=$true)][string]$GameDirectory,[switch]$UpdateShapeKeyRuntime)
$ErrorActionPreference = 'Stop'
$gameRoot = [IO.Path]::GetFullPath($GameDirectory).TrimEnd('\')
if (!(Test-Path -LiteralPath (Join-Path $gameRoot 're4.exe') -PathType Leaf)) { throw 'GameDirectory must contain re4.exe.' }
if (Get-Process -Name re4 -ErrorAction SilentlyContinue) { throw 'Close RE4 before installing the bridge.' }
$packageRoot = $PSScriptRoot
$planned = @()
foreach($nativeName in @('RE4R_ShapeKeySystem_native_leon.json','RE4R_ShapeKeySystem_native_leon_mouth9.json')){
    $nativeRelative='reframework\data\'+$nativeName
    $nativeTarget=Join-Path $gameRoot $nativeRelative
    $nativeSource=Join-Path $packageRoot $nativeRelative
    if($nativeName -eq 'RE4R_ShapeKeySystem_native_leon_mouth9.json' -and !(Test-Path -LiteralPath $nativeSource)){continue}
    if(!(Test-Path -LiteralPath $nativeTarget)){
        $nativeText=[IO.File]::ReadAllText($nativeSource,[Text.Encoding]::UTF8)
        $nativeConfig=$nativeText | ConvertFrom-Json
        if($nativeConfig.format -ne 1 -or !$nativeConfig.joints -or !$nativeConfig.rules){throw 'Invalid bundled native conversion table. No files changed.'}
        $planned+=@{relative=$nativeRelative;bytes=[IO.File]::ReadAllBytes($nativeSource)}
    } else {
        Write-Output "Preserving existing native conversion table: $nativeTarget"
    }
}
foreach ($scriptSpec in @(
    @{name='RE4R_ShapeKeySystem.lua';adapter='runtime_adapter.lua';marker='RE4R_DESKTOP_BRIDGE';required=@('key_by_id','local function still_valid','local function zero_written','local function save_settings')},
    @{name='RhyLiveFace.lua';adapter='receiver_adapter.lua';marker='RE4R_RHYLIVE_DESKTOP';required=@('local native=','local settings=','local function safe_poll_receiver','local function mark_dirty','local function flush_settings')}
)) {
    $relative = 'reframework\autorun\'+$scriptSpec.name
    $target = Join-Path $gameRoot $relative
    $replaceShape=$UpdateShapeKeyRuntime -and $scriptSpec.name -eq 'RE4R_ShapeKeySystem.lua'
    if($replaceShape){Write-Output 'Updating ShapeKey runtime by explicit request; the previous script will be backed up. Profiles and mapping files are preserved.'}
    $basePath = if((Test-Path -LiteralPath $target) -and !$replaceShape){$target}else{Join-Path $packageRoot $relative}
    $text = [IO.File]::ReadAllText($basePath)
    if($scriptSpec.name -eq 'RE4R_ShapeKeySystem.lua' -and !$replaceShape -and (Test-Path -LiteralPath $target) -and (Test-Path -LiteralPath (Join-Path $packageRoot 'reframework\data\RE4R_ShapeKeySystem_native_leon_mouth9.json')) -and !$text.Contains('native_map_file')){
        Write-Output 'Existing ShapeKey runtime preserved. The optional 9-input model needs the newer runtime; choose -UpdateShapeKeyRuntime only if you want to back up and replace that script.'
    }
    $marker = $scriptSpec.marker
    $text = [regex]::Replace($text,"(?s)\r?\n?-- ${marker}_BEGIN.*?-- ${marker}_END\r?\n?",'')
    foreach ($probe in $scriptSpec.required) {if(!$text.Contains($probe)){throw "Unsupported runtime $($scriptSpec.name): $probe missing. No files changed."}}
    $text += "`n"+[IO.File]::ReadAllText((Join-Path $packageRoot $scriptSpec.adapter))
    $planned += @{relative=$relative;bytes=[Text.UTF8Encoding]::new($false).GetBytes($text)}
}
# Always update our bridge; preserve an already installed receiver DLL and helper modules.
$planned += @{relative='reframework\plugins\ShapeKeyBridge.dll';bytes=[IO.File]::ReadAllBytes((Join-Path $packageRoot 'reframework\plugins\ShapeKeyBridge.dll'))}
foreach($relative in @('reframework\plugins\RhyLiveFace.dll','reframework\autorun\RhyLiveFace\face_math.lua','reframework\autorun\RhyLiveFace\default_profile.lua','reframework\autorun\RhyLiveFace\localization.lua')) {
    if(!(Test-Path -LiteralPath (Join-Path $gameRoot $relative))) {$planned += @{relative=$relative;bytes=[IO.File]::ReadAllBytes((Join-Path $packageRoot $relative))}}
}
foreach($entry in $planned) {
    $target=[IO.Path]::GetFullPath((Join-Path $gameRoot $entry.relative))
    if(!$target.StartsWith($gameRoot+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Invalid target path.'}
}
$backup=Join-Path $packageRoot ('backups\'+(Get-Date -Format 'yyyyMMdd_HHmmss_fff'))
New-Item -ItemType Directory -Path $backup -Force | Out-Null
$manifest=@()
foreach($entry in $planned){
    $target=Join-Path $gameRoot $entry.relative
    $exists=Test-Path -LiteralPath $target
    if($exists){$backupPath=Join-Path $backup $entry.relative;New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($backupPath)) -Force | Out-Null;Copy-Item -LiteralPath $target -Destination $backupPath}
    $manifest+=@{target=$target;relative=$entry.relative;existed=$exists;original_sha256=$(if($exists){(Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash}else{$null})}
}
$manifest | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $backup 'manifest.json') -Encoding UTF8
try {
    foreach($entry in $planned){$target=Join-Path $gameRoot $entry.relative;New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($target)) -Force | Out-Null;[IO.File]::WriteAllBytes($target,$entry.bytes)}
} catch {
    foreach($entry in $manifest){if($entry.existed){Copy-Item -LiteralPath (Join-Path $backup $entry.relative) -Destination $entry.target -Force}elseif(Test-Path -LiteralPath $entry.target){Remove-Item -LiteralPath $entry.target}}
    throw
}
Write-Output "Installed desktop and RhyLive bridge. Backup: $backup"
Write-Output 'Start RE4, then launch RE4R_ShapeKeyStudio.exe. No desktop IP address or port is needed.'
