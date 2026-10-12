param([string]$Package)
$ErrorActionPreference='Stop'
# Isolated fixture: never call this installer against a real game directory.
function Get-Process {param($Name,$ErrorAction) return $null}
$fixture=Join-Path (Split-Path $Package -Parent) ('tests\installer-fixture-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path "$fixture\reframework\autorun" -Force | Out-Null
[IO.File]::WriteAllText("$fixture\re4.exe",'fixture')
$shape=[IO.File]::ReadAllText("$Package\reframework\autorun\RE4R_ShapeKeySystem.lua")
$shape=[regex]::Replace($shape,'(?s)\r?\n?-- RE4R_DESKTOP_BRIDGE_BEGIN.*?-- RE4R_DESKTOP_BRIDGE_END\r?\n?','')+"`n-- OTHER_AGENT_EYE_FIX_SENTINEL`n"
$receiver=[IO.File]::ReadAllText("$Package\reframework\autorun\RhyLiveFace.lua")
$receiver=[regex]::Replace($receiver,'(?s)\r?\n?-- RE4R_RHYLIVE_DESKTOP_BEGIN.*?-- RE4R_RHYLIVE_DESKTOP_END\r?\n?','')+"`n-- PERSONAL_RECEIVER_SENTINEL`n"
[IO.File]::WriteAllText("$fixture\reframework\autorun\RE4R_ShapeKeySystem.lua",$shape)
[IO.File]::WriteAllText("$fixture\reframework\autorun\RhyLiveFace.lua",$receiver)
& "$Package\Install.ps1" -GameDirectory $fixture
$native="$fixture\reframework\data\RE4R_ShapeKeySystem_native_leon.json"
if(!(Test-Path -LiteralPath $native) -or (Get-FileHash -LiteralPath $native).Hash -ne (Get-FileHash -LiteralPath "$Package\reframework\data\RE4R_ShapeKeySystem_native_leon.json").Hash){throw 'Clean installation did not install the native conversion table into reframework/data'}
$custom=[IO.File]::ReadAllText($native).TrimEnd()+"`n "
[IO.File]::WriteAllText($native,$custom,[Text.UTF8Encoding]::new($false))
$customHash=(Get-FileHash -LiteralPath $native).Hash
& "$Package\Install.ps1" -GameDirectory $fixture
if((Get-FileHash -LiteralPath $native).Hash -ne $customHash){throw 'Existing user native conversion table was overwritten'}
Write-Output 'NATIVE_TABLE_CLEAN_INSTALL_AND_USER_CONFIG_PRESERVATION_PASS'
foreach($spec in @(@('RE4R_ShapeKeySystem.lua','RE4R_DESKTOP_BRIDGE_BEGIN','OTHER_AGENT_EYE_FIX_SENTINEL'),@('RhyLiveFace.lua','RE4R_RHYLIVE_DESKTOP_BEGIN','PERSONAL_RECEIVER_SENTINEL'))){
    $text=[IO.File]::ReadAllText("$fixture\reframework\autorun\$($spec[0])")
    if([regex]::Matches($text,$spec[1]).Count -ne 1 -or !$text.Contains($spec[2])){throw 'Installer did not preserve existing script or duplicated adapter'}
}
if(!(Test-Path -LiteralPath "$fixture\reframework\plugins\RhyLiveFace.dll")){throw 'Receiver DLL missing'}
$before=(Get-FileHash -LiteralPath "$fixture\reframework\autorun\RE4R_ShapeKeySystem.lua").Hash
[IO.File]::WriteAllText("$fixture\reframework\autorun\RhyLiveFace.lua",'UNSUPPORTED')
$failed=$false
try{& "$Package\Install.ps1" -GameDirectory $fixture}catch{$failed=$true}
if(!$failed -or $before -ne (Get-FileHash -LiteralPath "$fixture\reframework\autorun\RE4R_ShapeKeySystem.lua").Hash){throw 'Invalid runtime did not fail before mutation'}
Write-Output 'INSTALLER_FIXTURE_PRESERVATION_REPEAT_INSTALL_AND_PREFLIGHT_PASS'
$rollbackFixture=$fixture+'-rollback'
New-Item -ItemType Directory -Path $rollbackFixture -Force | Out-Null
[IO.File]::WriteAllText("$rollbackFixture\re4.exe",'fixture')
function New-Item {
    param($Path,$ItemType,[switch]$Force)
    if($Path -eq "$rollbackFixture\reframework\autorun"){throw 'Injected failure after native table write'}
    Microsoft.PowerShell.Management\New-Item @PSBoundParameters
}
$failed=$false
try {& "$Package\Install.ps1" -GameDirectory $rollbackFixture} catch {$failed=$true}
finally {Remove-Item Function:\New-Item}
if(!$failed -or (Test-Path -LiteralPath "$rollbackFixture\reframework\data\RE4R_ShapeKeySystem_native_leon.json")){throw 'Failed installation left a newly installed native conversion table'}
Write-Output 'NATIVE_TABLE_INSTALL_FAILURE_ROLLBACK_PASS'
