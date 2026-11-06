#pragma once
#include <string>
#include <string_view>
namespace lyko {
std::string content_sha256(std::string_view bytes);
inline bool valid_sha256(std::string_view text) {
  if (text.size() != 64) {
    return false;
  }
  for (char c : text) {
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
      return false;
    }
  }
  return true;
}
} // namespace lyko
