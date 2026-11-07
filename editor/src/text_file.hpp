#pragma once
#include <filesystem>
#include <stdexcept>
#include <string>

namespace lyko {
class CheckedWriteError : public std::runtime_error {
public:
  std::string code, field;
  CheckedWriteError(std::string c, std::string f, std::string message)
      : std::runtime_error(std::move(message)), code(std::move(c)),
        field(std::move(f)) {}
};
std::filesystem::path
replace_text_file_checked(const std::filesystem::path &path,
                          const std::string &text,
                          const std::string &expected_sha256);
std::filesystem::path path_from_utf8(const std::string &text);
std::string read_text_file(const std::filesystem::path &path);
// Atomic publication of a complete NEW file. Existing destinations are
// preserved.
void publish_text_file(const std::filesystem::path &path,
                       const std::string &text);
} // namespace lyko
