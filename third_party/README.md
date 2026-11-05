# Vendored desktop dependencies

| Library | Version / upstream commit | Licence | Contents |
| --- | --- | --- | --- |
| Dear ImGui | v1.92.5-docking / 3912b3d9a9c1b3f17431aebafd86d2f40ee6e59c | MIT; imgui/LICENSE.txt | Unmodified core and SDL3/SDLRenderer3 backend subset |
| SDL3 | release-3.2.28 / 7f3ae3d57459e59943a4ecfefc8f6277ec6bf540 | zlib; sdl3/LICENSE.txt | Complete upstream tracked source archive |
| Roboto Regular | font name version 2.001047 (2015) / 38062f4b4a0be4346d07a928408da21602545e9e | Apache-2.0; roboto/LICENSE.txt | Unmodified src/hinted/Roboto-Regular.ttf from googlefonts/roboto |

ImGui's bundled, upstream-modified `stb_rect_pack` 1.01, `stb_textedit` 1.14 and
`stb_truetype` 1.26 are included under their MIT option; licence notices are in
the corresponding headers. The built-in ProggyClean font is MIT, with its notice
in imgui_draw.cpp. The desktop now uses the embedded Roboto font listed above.
Icons are original vector drawing code in editor/src/desktop/icons.hpp; no third-party
icon library or icon font is used.

Sources were fetched from https://github.com/ocornut/imgui and
https://github.com/libsdl-org/SDL at the commits above. No submodules or runtime
network downloads are used. The SDL archive is produced by `git archive
--format=tar.gz --prefix=SDL3/ HEAD` at the pinned commit. SHA-256:
`48b4cba694feacb6d4792a23bdb20e297a6d570fdbabd42b4601afc00bd2c818`.
The CMake wrapper verifies this checksum before offline extraction into the build
directory. Licence texts are included beside both dependencies and within SDL's
archive. Platform system libraries remain system prerequisites, not vendored app
libraries. The desktop uses SDL's software renderer; there is no engine/RHI link.

Roboto source: https://github.com/googlefonts/roboto at the pinned commit above.
Copyright 2015 Google Inc. All Rights Reserved. Its embedded name records specify
Apache License, Version 2.0; the upstream licence is included unchanged. This
specific historical font file is used; no differently licensed newer font is
substituted. SHA-256:
`56a45233d29f11b4dfb86d248e921939d115778f87325e7ae8cc108383d6664d`.
The optional desktop CMake wrapper verifies the checksum and embeds the bytes in
the executable. There is no runtime font download or source-directory dependency.
The embedded font and bundled licence notices must accompany any future distribution.
