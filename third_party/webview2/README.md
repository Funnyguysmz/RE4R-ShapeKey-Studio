# WebView2 SDK dependency

Required SDK: **Microsoft.Web.WebView2 1.0.4258.31** from Microsoft's
[official NuGet package](https://www.nuget.org/packages/Microsoft.Web.WebView2/1.0.4258.31).
Download that version's `.nupkg`, extract it to a directory of your choice, and
pass that directory (containing `Microsoft.Web.WebView2.nuspec` and `build/`) to
`-WebView2Directory`. The build checks the manifest version. No SDK download or
Runtime installation is performed automatically.

From the repository root, with Zig and Lua 5.4 C sources available:

```powershell
.\build_v04.ps1 -Zig 'C:\tools\zig\zig.exe' -LuaSourceDirectory 'C:\tools\lua-5.4\src' -WebView2Directory 'C:\tools\webview2-1.0.4258.31'
.\scripts\Test.ps1 -UiVersion '0.4'
```

The paths above are examples; supply your own dependency paths. The public build
does not depend on the author's drives, game installation or model projects.
The 0.4 output is `build/v04/package`; the retained native 0.3 fallback is
`build/package`, built by `Build.ps1` and tested with `-UiVersion '0.3'`.

Tests load the WebView2 interface in demo and host modes and verify the ready
handshake. They do not prove live game operation or capture quality. A separately
installed Microsoft WebView2 Runtime is needed to run these UI tests.

The complete SDK license, notices and manifest are preserved alongside this file.
The build copies them into the application package. SDK tools and binaries are
not checked into this repository. See `../../ThirdPartyNotices.md` for the loader
redistribution notice. The WebView2 Runtime has its own distribution terms.
