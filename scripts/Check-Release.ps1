param([Parameter(Mandatory=$true)][string]$ZipPath)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip=[IO.Compression.ZipFile]::OpenRead([IO.Path]::GetFullPath($ZipPath))
try {
    $entries=@{}
    foreach($entry in $zip.Entries){
        $name=$entry.FullName.Replace('\','/')
        if($name.EndsWith('/')){continue}
        if($name.StartsWith('/') -or $name.Contains('../') -or $entries.ContainsKey($name)){throw "Unsafe or duplicate ZIP entry: $name"}
        if($name -match '(?i)(^|/)(backups|cache|logs|natives|models?)(/|$)|\.(ini|log|pdb|lib|blend|fbx|pmx|mesh|tex)(\.|$)|font|Shinano_preview|running_old'){
            throw "Excluded personal, model, game or build artifact: $name"
        }
        $allowed='^(RE4R_ShapeKeyStudio\.exe|WebView2Loader\.dll|Install\.ps1|VERSION\.txt|LICENSE|ThirdPartyNotices\.md|README\.md|Lua_LICENSE\.txt|REFramework_LICENSE|RhyLive_LICENSE|runtime_adapter\.lua|receiver_adapter\.lua|Studio\.ico|ui/(index\.html|app\.(js|css)|demo\.js|Studio_icon\.png)|third_party/webview2/(LICENSE\.txt|NOTICE\.txt|Microsoft\.Web\.WebView2\.nuspec)|reframework/plugins/(ShapeKeyBridge|RhyLiveFace)\.dll|reframework/autorun/(RE4R_ShapeKeySystem|RhyLiveFace)\.lua|reframework/autorun/RhyLiveFace/(default_profile|face_math|localization)\.lua)$'
        if($name -notmatch $allowed){throw "Entry outside release allowlist: $name"}
        $entries[$name]=$entry
    }
    foreach($required in @('RE4R_ShapeKeyStudio.exe','WebView2Loader.dll','Install.ps1','VERSION.txt','LICENSE','ThirdPartyNotices.md','third_party/webview2/LICENSE.txt','third_party/webview2/NOTICE.txt','Lua_LICENSE.txt','REFramework_LICENSE','RhyLive_LICENSE','ui/index.html','ui/app.js','ui/app.css','ui/demo.js','reframework/plugins/ShapeKeyBridge.dll','reframework/plugins/RhyLiveFace.dll')){
        if(!$entries.ContainsKey($required)){throw "Missing release file: $required"}
    }
    function Read-ZipText([string]$Name){
        $reader=[IO.StreamReader]::new($entries[$Name].Open(),[Text.Encoding]::UTF8)
        try{return $reader.ReadToEnd()}finally{$reader.Dispose()}
    }
    if((Read-ZipText 'VERSION.txt').Trim() -ne '0.4.0'){throw 'Wrong release version'}
    $ui=Read-ZipText 'ui/app.js'
    if(!$ui.Contains("version: '0.4.0'") -or !$ui.Contains('cap_smooth') -or !$ui.Contains('topmost')){throw 'UI does not include required 0.4 features'}
    $runtime=Read-ZipText 'reframework/autorun/RE4R_ShapeKeySystem.lua'
    foreach($api in @('get_cap_smooth','set_cap_smooth','mesh_status','allocate_weights')){
        if(!$runtime.Contains($api)){throw "Missing runtime API: $api"}
    }
    if(!(Read-ZipText 'LICENSE').Contains('Copyright (c) 2026 funnyguyz')){throw 'Incorrect project copyright'}
    $license=Read-ZipText 'third_party/webview2/LICENSE.txt'
    if(!$license.Contains('Copyright (C) Microsoft Corporation.') -or !$license.Contains('Redistributions in binary form')){throw 'Incorrect WebView2 SDK license'}
    Write-Output 'RELEASE_CONTENT_PREFLIGHT_PASS; owner approval, final ZIP provenance and live game validation are separate.'
} finally {$zip.Dispose()}
