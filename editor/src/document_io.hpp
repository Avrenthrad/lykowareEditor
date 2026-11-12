#pragma once
#include "command_bus.hpp"
#include <filesystem>
namespace lyko {
struct ObservedText {
  std::string path, text, sha256;
};
ObservedText read_observed_text(const std::filesystem::path &path);
std::string serialize_document(const std::string &kind,
                               const Document &document);
} // namespace lyko
