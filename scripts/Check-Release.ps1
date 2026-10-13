param([Parameter(Mandatory=$true)][string]$ZipPath,[switch]$RequireMouth9)
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
        $allowed='^(RE4R_ShapeKeyStudio\.exe|WebView2Loader\.dll|Install\.ps1|VERSION\.txt|LICENSE|ThirdPartyNotices\.md|README\.md|Lua_LICENSE\.txt|REFramework_LICENSE|RhyLive_LICENSE|runtime_adapter\.lua|receiver_adapter\.lua|Studio\.ico|ui/(index\.html|app\.(js|css)|demo\.js|Studio_icon\.png)|third_party/webview2/(LICENSE\.txt|NOTICE\.txt|Microsoft\.Web\.WebView2\.nuspec)|docs/(TOOLCHAIN|VALIDATION)\.md|examples/mouth9/(README\.md|profile_mouth9_synthetic\.json)|reframework/data/RE4R_ShapeKeySystem_native_leon(_mouth9)?\.json|reframework/plugins/(ShapeKeyBridge|RhyLiveFace)\.dll|reframework/autorun/(RE4R_ShapeKeySystem|RhyLiveFace)\.lua|reframework/autorun/RhyLiveFace/(default_profile|face_math|localization)\.lua)$'
        if($name -notmatch $allowed){throw "Entry outside release allowlist: $name"}
        $entries[$name]=$entry
    }
    foreach($required in @('RE4R_ShapeKeyStudio.exe','WebView2Loader.dll','Install.ps1','VERSION.txt','LICENSE','ThirdPartyNotices.md','third_party/webview2/LICENSE.txt','third_party/webview2/NOTICE.txt','Lua_LICENSE.txt','REFramework_LICENSE','RhyLive_LICENSE','ui/index.html','ui/app.js','ui/app.css','ui/demo.js','reframework/data/RE4R_ShapeKeySystem_native_leon.json','reframework/plugins/ShapeKeyBridge.dll','reframework/plugins/RhyLiveFace.dll')){
        if(!$entries.ContainsKey($required)){throw "Missing release file: $required"}
    }
    function Read-ZipText([string]$Name){
        $reader=[IO.StreamReader]::new($entries[$Name].Open(),[Text.Encoding]::UTF8)
        try{return $reader.ReadToEnd()}finally{$reader.Dispose()}
    }
    if((Read-ZipText 'VERSION.txt').Trim() -ne '0.4.2-beta.1'){throw 'Wrong release version'}
    $native=(Read-ZipText 'reframework/data/RE4R_ShapeKeySystem_native_leon.json') | ConvertFrom-Json
    if($native.format -ne 1 -or !$native.joints){throw 'Invalid native conversion table'}
    $ui=Read-ZipText 'ui/app.js'
    if(!$ui.Contains("version: '0.4.2-beta.1'") -or !$ui.Contains('cap_smooth') -or !$ui.Contains('topmost')){throw 'UI does not include required 0.4 features'}
    $runtime=Read-ZipText 'reframework/autorun/RE4R_ShapeKeySystem.lua'
    foreach($api in @('get_cap_smooth','set_cap_smooth','mesh_status','allocate_weights')){
        if(!$runtime.Contains($api)){throw "Missing runtime API: $api"}
    }
    if(!(Read-ZipText 'LICENSE').Contains('Copyright (c) 2026 funnyguyz')){throw 'Incorrect project copyright'}
    $optional='reframework/data/RE4R_ShapeKeySystem_native_leon_mouth9.json'
    if($RequireMouth9 -and !$runtime.Contains("local VERSION = '0.4.5-rc.1'")){throw 'Unexpected System release version'}
    if($RequireMouth9 -and !$entries.ContainsKey($optional)){throw 'Candidate ZIP is missing the optional mouth9 map'}
    if($RequireMouth9 -and !$entries.ContainsKey('examples/mouth9/README.md')){throw 'Missing optional mapping instructions'}
    if($entries.ContainsKey($optional)){
        $mapping=(Read-ZipText $optional) | ConvertFrom-Json
        if($mapping.format -ne 1 -or !$mapping.joints -or !$mapping.rules){throw 'Invalid optional mouth9 map'}
        if(!$runtime.Contains('native_map_file') -or !$runtime.Contains('native_tap_status')){throw 'Old runtime cannot be presented as the mouth9 candidate'}
    }
    $license=Read-ZipText 'third_party/webview2/LICENSE.txt'
    if(!$license.Contains('Copyright (C) Microsoft Corporation.') -or !$license.Contains('Redistributions in binary form')){throw 'Incorrect WebView2 SDK license'}
    Write-Output 'RELEASE_CONTENT_PREFLIGHT_PASS; owner approval, final ZIP provenance and live game validation are separate.'
} finally {$zip.Dispose()}
