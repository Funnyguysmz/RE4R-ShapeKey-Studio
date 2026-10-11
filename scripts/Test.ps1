$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
foreach($case in @(@('adapter_test.lua','runtime_adapter.lua'),@('receiver_test.lua','receiver_adapter.lua'))){
    & "$root\build\lua54.exe" "$root\tests\$($case[0])" "$root\src\$($case[1])"
    if($LASTEXITCODE){throw 'Lua interface test failed'}
}
& "$root\build\tests\bridge_test.exe"
if($LASTEXITCODE){throw 'Native pipe/client test failed'}
$child=Start-Process -FilePath "$root\build\package\RE4R_ShapeKeyStudio.exe" -ArgumentList '--setup-smoke-test' -WindowStyle Hidden -PassThru -Wait
if($child.ExitCode){throw 'GUI setup and editing smoke test failed'}
& "$root\tests\install_test.ps1" -Package "$root\build\package"
Write-Output 'OFFLINE_TESTS_PASS; actual RE4 behavior still requires game testing.'
