# Embed the unmodified Apache-2.0 Roboto font; no runtime/source-tree lookup.
set(LYKO_FONT "${CMAKE_CURRENT_LIST_DIR}/../../third_party/roboto/Roboto-Regular.ttf")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${LYKO_FONT}")
file(SHA256 "${LYKO_FONT}" LYKO_FONT_HASH)
if(NOT LYKO_FONT_HASH STREQUAL "56a45233d29f11b4dfb86d248e921939d115778f87325e7ae8cc108383d6664d")
  message(FATAL_ERROR "Vendored Roboto checksum mismatch")
endif()
file(READ "${LYKO_FONT}" LYKO_FONT_HEX HEX)
# Keep generated source lines short for Windows compilers as well as GCC/Clang.
string(REGEX REPLACE "(................................)" "\\1\n" LYKO_FONT_HEX "${LYKO_FONT_HEX}")
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," LYKO_FONT_BYTES "${LYKO_FONT_HEX}")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/roboto_regular.hpp"
  CONTENT "// Generated from pinned, unmodified Roboto Regular. Apache-2.0.\n#pragma once\nnamespace lyko { inline unsigned char roboto_regular[] = {@LYKO_FONT_BYTES@}; }\n" @ONLY)
unset(LYKO_FONT_HEX)
unset(LYKO_FONT_BYTES)
