param([ValidateSet('0.3','0.4')][string]$UiVersion='0.4')
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
if($UiVersion -eq '0.4'){
    $package=Join-Path $root 'build\v04\package'
    foreach($name in @('RE4R_ShapeKeyStudio.exe','WebView2Loader.dll','ui\app.js','ui\demo.js','VERSION.txt','ThirdPartyNotices.md','third_party\webview2\LICENSE.txt')){
        if(!(Test-Path -LiteralPath (Join-Path $package $name))){throw "Missing 0.4 build file: $name"}
    }
    if([IO.File]::ReadAllText("$package\VERSION.txt").Trim() -ne '0.4.2-beta.1'){throw 'Unexpected Studio version'}
    foreach($arguments in @('--selftest --demo','--selftest')){
        $child=Start-Process -FilePath "$package\RE4R_ShapeKeyStudio.exe" -ArgumentList $arguments -WindowStyle Hidden -PassThru
        if(!$child.WaitForExit(30000)){$child.Kill();throw 'WebView2 selftest timed out'}
        if($child.ExitCode){throw 'WebView2 UI ready selftest failed'}
    }
    & "$root\tests\install_test.ps1" -Package $package
    foreach($spec in @(@('system_test_panel.lua','system_fixture.lua'),@('system_test_mouth9.lua','system_fixture_mouth9_synthetic.lua'))){
        & "$root\build\lua54.exe" "$root\tests\$($spec[0])" "$package\reframework\autorun\RE4R_ShapeKeySystem.lua" "$root\tests\$($spec[1])"
        if($LASTEXITCODE){throw 'Packaged System regression failed'}
    }
    Write-Output 'WEBVIEW2_V04_DEMO_AND_HOST_READY_PASS'
}
Write-Output 'OFFLINE_TESTS_PASS; actual RE4 behavior still requires game testing.'
