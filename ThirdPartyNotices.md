# Third-party notices

The project's own code is licensed under MIT; see LICENSE. Third-party
components retain their original licenses and are not relicensed by LICENSE.

## Microsoft WebView2 SDK 1.0.4258.31

Source: Microsoft.Web.WebView2 1.0.4258.31, official NuGet package.
https://www.nuget.org/packages/Microsoft.Web.WebView2/1.0.4258.31

The native host builds against `build/native/include/WebView2.h` and distributes
the package's x64 `build/native/x64/WebView2Loader.dll` unchanged. The package
manifest identifies `LICENSE.txt` as its license. That file contains Microsoft's
copyright and BSD 3-Clause style redistribution terms, not MIT. Preserve the
complete license and disclaimer with both source and binary distributions, and
do not use Microsoft's name to endorse the application.

The original package license, notices and manifest are retained in
`third_party/webview2/LICENSE.txt`, `NOTICE.txt`, and
`Microsoft.Web.WebView2.nuspec`. The package-wide NOTICE is preserved verbatim;
its presence does not imply that every SDK tool or dependency is shipped here.

Microsoft documents shipping the architecture-matching loader with the app:
https://learn.microsoft.com/microsoft-edge/webview2/concepts/distribution

The WebView2 Runtime is a separate dependency. This project does not bundle its
installer or a Fixed Version Runtime, or claim to license it under these terms.

## REFramework, RhyLive and Lua

REFramework and RhyLive notices remain in `vendor/REFramework_LICENSE` and
`vendor/RhyLive_LICENSE`, and are copied into the binary package. Lua's original
copyright/license is extracted from the supplied Lua 5.4 source by `Build.ps1`
and shipped as `Lua_LICENSE.txt`. These components are not relicensed by the
project's MIT license.
