#!/usr/bin/env bash
# Rootless Debian 13 cloud setup. Network operations require the approved network
# execution context; do not mistake a default sandbox denial for a broken proxy.
set -euo pipefail
lyko_tools=/workspace/.lyko-tools
lyko_system=/workspace/.lyko-system
lyko_repo=/workspace/lykowareEditor
if [[ ! -x "$lyko_tools/cmake/data/bin/cmake" || ! -x "$lyko_tools/bin/ninja" ]]; then
  python3 -m pip install --no-input --target "$lyko_tools" cmake==3.31.10 ninja==1.13.0
fi
if [[ ! -x "$lyko_system/root/usr/bin/clang-tidy-19" || ! -f "$lyko_system/root/usr/include/X11/Xlib.h" || ! -x "$lyko_system/root/usr/bin/Xvfb" ]]; then
  mkdir -p "$lyko_system"/{lists/partial,archives/partial,empty,root}
  cat > "$lyko_system/apt.conf" <<'APT'
Dir::Etc::Parts "/workspace/.lyko-system/empty";
Dir::Etc::main "/workspace/.lyko-system/empty.conf";
Dir::Etc::sourcelist "/workspace/.lyko-system/sources.list";
Dir::Etc::sourceparts "/workspace/.lyko-system/empty";
Dir::State::lists "/workspace/.lyko-system/lists";
Dir::Cache::archives "/workspace/.lyko-system/archives";
Acquire::https::Proxy "http://proxy:8080";
APT::Sandbox::User "agent";
APT
  cat > "$lyko_system/sources.list" <<'APT'
deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] https://deb.debian.org/debian trixie main
APT
  export APT_CONFIG="$lyko_system/apt.conf"
  /usr/bin/apt-get update
  lyko_packages=()
  for lyko_package in libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxfixes-dev libxrender-dev libxss-dev libxtst-dev libxkbcommon-dev libx11-6 libx11-data libxext6 libxrandr2 libxcursor1 libxi6 libxfixes3 libxrender1 libxss1 libxtst6 libxkbcommon0 libxcb1 libxau6 libxdmcp6 libxau-dev libxdmcp-dev libxcb1-dev x11proto-dev xorg-sgml-doctools xtrans-dev xvfb xauth xserver-common x11-xkb-utils xkb-data libpixman-1-0 libfontenc1 libxfont2 libunwind8 libbsd0 libmd0 libgl1 libglx0 libglvnd0 libdrm2 libgcrypt20 libgpg-error0 clang-tidy-19 clang-format-19 libclang-cpp19 libllvm19 libclang-common-19-dev libedit2 libffi8 libxml2 libz3-4 libzstd1 zlib1g; do
    lyko_packages+=("$lyko_package/trixie")
  done
  (cd "$lyko_system/archives" && /usr/bin/apt-get download "${lyko_packages[@]}")
  for lyko_deb in "$lyko_system"/archives/*.deb; do dpkg-deb -x "$lyko_deb" "$lyko_system/root"; done
fi
cat > "$lyko_tools/activate.sh" <<'ACTIVATE'
export PATH=/workspace/.lyko-tools/cmake/data/bin:/workspace/.lyko-tools/bin:/workspace/.lyko-system/root/usr/bin:$PATH
export LD_LIBRARY_PATH=/workspace/.lyko-system/root/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
export PKG_CONFIG_SYSROOT_DIR=/workspace/.lyko-system/root
export PKG_CONFIG_LIBDIR=/workspace/.lyko-system/root/usr/lib/x86_64-linux-gnu/pkgconfig:/workspace/.lyko-system/root/usr/share/pkgconfig
ACTIVATE
# CMake's generic tool lookup uses this stable local alias.
ln -sfn /workspace/.lyko-system/root/usr/bin/clang-tidy-19 "$lyko_tools/bin/clang-tidy"
source "$lyko_tools/activate.sh"
cd "$lyko_repo/editor"
cmake --preset dev
cmake --build --preset dev --parallel 4
ctest --preset dev
cmake --preset desktop -DCMAKE_PREFIX_PATH="$lyko_system/root/usr" -DCMAKE_INCLUDE_PATH="$lyko_system/root/usr/include" -DCMAKE_LIBRARY_PATH="$lyko_system/root/usr/lib/x86_64-linux-gnu"
cmake --build --preset desktop --parallel 4
ctest --preset desktop
