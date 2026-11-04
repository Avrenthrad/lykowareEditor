#pragma once
#include <filesystem>
#include <string>

namespace lyko {
std::filesystem::path path_from_utf8(const std::string& text);
std::string read_text_file(const std::filesystem::path& path);
// Atomic publication of a complete NEW file. Existing destinations are preserved.
void publish_text_file(const std::filesystem::path& path, const std::string& text);
}
