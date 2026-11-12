#include "content_hash.hpp"
#include "picosha2.h"
namespace lyko {
std::string content_sha256(std::string_view bytes) {
  const char *first = bytes.empty() ? "" : bytes.data();
  return picosha2::hash256_hex_string(first, first + bytes.size());
}
} // namespace lyko
