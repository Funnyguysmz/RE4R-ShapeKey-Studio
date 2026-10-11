param([string]$Package)
$ErrorActionPreference='Stop'
# Isolated fixture: never call this installer against a real game directory.
function Get-Process {param($Name,$ErrorAction) return $null}
$fixture=Join-Path (Split-Path $Package -Parent) 'tests\installer-fixture'
New-Item -ItemType Directory -Path "$fixture\reframework\autorun" -Force | Out-Null
[IO.File]::WriteAllText("$fixture\re4.exe",'fixture')
$shape=[IO.File]::ReadAllText("$Package\reframework\autorun\RE4R_ShapeKeySystem.lua")
$shape=[regex]::Replace($shape,'(?s)\r?\n?-- RE4R_DESKTOP_BRIDGE_BEGIN.*?-- RE4R_DESKTOP_BRIDGE_END\r?\n?','')+"`n-- OTHER_AGENT_EYE_FIX_SENTINEL`n"
$receiver=[IO.File]::ReadAllText("$Package\reframework\autorun\RhyLiveFace.lua")
$receiver=[regex]::Replace($receiver,'(?s)\r?\n?-- RE4R_RHYLIVE_DESKTOP_BEGIN.*?-- RE4R_RHYLIVE_DESKTOP_END\r?\n?','')+"`n-- PERSONAL_RECEIVER_SENTINEL`n"
[IO.File]::WriteAllText("$fixture\reframework\autorun\RE4R_ShapeKeySystem.lua",$shape)
[IO.File]::WriteAllText("$fixture\reframework\autorun\RhyLiveFace.lua",$receiver)
& "$Package\Install.ps1" -GameDirectory $fixture
& "$Package\Install.ps1" -GameDirectory $fixture
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
