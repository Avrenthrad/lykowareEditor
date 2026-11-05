# Vendored desktop dependencies

| Library | Version / upstream commit | Licence | Contents |
| --- | --- | --- | --- |
| Dear ImGui | v1.92.5-docking / 3912b3d9a9c1b3f17431aebafd86d2f40ee6e59c | MIT; imgui/LICENSE.txt | Unmodified core and SDL3/SDLRenderer3 backend subset |
| SDL3 | release-3.2.28 / 7f3ae3d57459e59943a4ecfefc8f6277ec6bf540 | zlib; sdl3/LICENSE.txt | Complete upstream tracked source archive |

ImGui's bundled, upstream-modified `stb_rect_pack` 1.01, `stb_textedit` 1.14 and
`stb_truetype` 1.26 are included under their MIT option; licence notices are in
the corresponding headers. The built-in ProggyClean font is MIT, with its notice
in imgui_draw.cpp. No separate font or icon library has been added.

Sources were fetched from https://github.com/ocornut/imgui and
https://github.com/libsdl-org/SDL at the commits above. No submodules or runtime
network downloads are used. The SDL archive is produced by `git archive
--format=tar.gz --prefix=SDL3/ HEAD` at the pinned commit. SHA-256:
`48b4cba694feacb6d4792a23bdb20e297a6d570fdbabd42b4601afc00bd2c818`.
The CMake wrapper verifies this checksum before offline extraction into the build
directory. Licence texts are included beside both dependencies and within SDL's
archive. Platform system libraries remain system prerequisites, not vendored app
libraries. The desktop uses SDL's software renderer; there is no engine/RHI link.
