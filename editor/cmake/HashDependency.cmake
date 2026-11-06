# Offline, MIT-licensed PicoSHA2 v1.0.1; only the wrapper sees the vendor header.
set(LYKO_HASH_DIR "${CMAKE_CURRENT_LIST_DIR}/../../third_party/picosha2")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${LYKO_HASH_DIR}/picosha2.h")
file(SHA256 "${LYKO_HASH_DIR}/picosha2.h" LYKO_HASH_HEADER_SHA)
if(NOT LYKO_HASH_HEADER_SHA STREQUAL "b13c180161ffac8d0adc81e033e493c409457c4d1258ab9781ac80579ba3bdd8")
  message(FATAL_ERROR "Vendored PicoSHA2 checksum mismatch")
endif()
add_library(lyko_hash_dependency INTERFACE)
target_include_directories(lyko_hash_dependency SYSTEM INTERFACE "${LYKO_HASH_DIR}")
