#include "text_file.hpp"
#include "content_hash.hpp"
#include "json.hpp"
#include <chrono>
#include <fstream>
#include <stdexcept>
#include <system_error>

namespace lyko {
namespace {
void validate_path(const std::filesystem::path &path) {
  if (path.empty() || path.native().find(std::filesystem::path::value_type{}) !=
                          std::filesystem::path::string_type::npos) {
    throw std::invalid_argument(
        "File path must be nonempty and contain no null characters");
  }
}
} // namespace
std::filesystem::path path_from_utf8(const std::string &text) {
  if (text.empty() || text.find('\0') != std::string::npos ||
      !json_detail::valid_utf8(text)) {
    throw std::invalid_argument(
        "File path must be nonempty UTF-8 without null characters");
  }
  std::u8string encoded;
  encoded.reserve(text.size());
  for (unsigned char c : text) {
    encoded.push_back(static_cast<char8_t>(c));
  }
  return std::filesystem::path(encoded);
}
std::string read_text_file(const std::filesystem::path &path) {
  validate_path(path);
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("Cannot open text file");
  }
  std::string data;
  char c;
  while (input.get(c)) {
    if (data.size() == 1024 * 1024) {
      throw std::invalid_argument("File exceeds 1 MiB");
    }
    data += c;
  }
  if (!input.eof()) {
    throw std::runtime_error("Cannot read complete text file");
  }
  return data;
}
void publish_text_file(const std::filesystem::path &path,
                       const std::string &text) {
  validate_path(path);
  if (path.filename().empty()) {
    throw std::invalid_argument("Path must name a file");
  }
  if (text.size() > 1024 * 1024) {
    throw std::invalid_argument("File exceeds 1 MiB");
  }
  const auto parent =
      path.has_parent_path() ? path.parent_path() : std::filesystem::path(".");
  std::filesystem::path staging;
  for (unsigned attempt = 0; attempt != 100; ++attempt) {
    const auto stamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    auto candidate = parent / (".lyko-save-" + std::to_string(stamp) + "-" +
                               std::to_string(attempt));
    std::error_code error;
    if (std::filesystem::create_directory(candidate, error)) {
      staging = std::move(candidate);
      break;
    }
    if (error && error != std::errc::file_exists) {
      throw std::runtime_error("Cannot create file staging directory");
    }
  }
  if (staging.empty()) {
    throw std::runtime_error("Cannot reserve file staging directory");
  }
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
      std::error_code error;
      std::filesystem::remove_all(path, error);
    }
  } cleanup{staging};
  const auto temporary = staging / "content.txt";
  std::ofstream output(temporary, std::ios::binary);
  if (!output) {
    throw std::runtime_error("Cannot create temporary file");
  }
  output << text;
  output.close();
  if (!output) {
    throw std::runtime_error("Cannot write complete file");
  }
  std::error_code error;
  // Hard-link publication is atomic and fails if the target already exists.
  // Unsupported filesystems fail safely rather than copying a partial file.
  std::filesystem::create_hard_link(temporary, path, error);
  if (error) {
    throw std::runtime_error("Cannot publish file; destination must be new and "
                             "filesystem must support hard links");
  }
}
std::filesystem::path
replace_text_file_checked(const std::filesystem::path &path,
                          const std::string &text,
                          const std::string &expected_sha256) {
  validate_path(path);
  if (text.size() > 1024 * 1024) {
    throw std::invalid_argument("File exceeds 1 MiB");
  }
  if (!valid_sha256(expected_sha256)) {
    throw CheckedWriteError("invalid_fingerprint", "sha256",
                            "Use 64 lowercase hexadecimal SHA-256 characters.");
  }
  std::filesystem::path destination;
  try {
    destination = std::filesystem::canonical(path);
    if (!std::filesystem::is_regular_file(destination)) {
      throw CheckedWriteError("external_change_conflict", "path",
                              "Destination must remain a regular file.");
    }
  } catch (const std::filesystem::filesystem_error &) {
    throw CheckedWriteError("external_change_conflict", "path",
                            "Destination is missing or cannot be resolved.");
  }
  const auto parent = destination.parent_path();
  const auto lock = parent / ".lyko-write-lock";
  std::error_code error;
  if (!std::filesystem::create_directory(lock, error)) {
    throw CheckedWriteError("save_busy", "path",
                            "Cannot reserve the write lock; another writer or "
                            "a leftover lock may exist.");
  }
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
      std::error_code ignored;
      std::filesystem::remove_all(path, ignored);
    }
  } lock_cleanup{lock};
  std::filesystem::permissions(lock, std::filesystem::perms::owner_all);
  const auto permissions = std::filesystem::status(destination).permissions();
  const auto previous = read_text_file(destination);
  if (content_sha256(previous) != expected_sha256) {
    throw CheckedWriteError(
        "external_change_conflict", "path",
        "File bytes changed since the expected observation. Inspect/reload or "
        "explicitly provide the reviewed current digest.");
  }
  const auto backups = parent / ".lyko-backups";
  const auto status = std::filesystem::symlink_status(backups, error);
  if (error && error != std::errc::no_such_file_or_directory) {
    throw std::runtime_error("Cannot inspect backup directory.");
  }
  if (std::filesystem::exists(status) &&
      !std::filesystem::is_directory(status)) {
    throw std::runtime_error(
        "Backup location must be a real directory, not a symlink or file.");
  }
  if (std::filesystem::create_directory(backups)) {
    std::filesystem::permissions(backups, std::filesystem::perms::owner_all);
  }
  const auto backup = backups / (expected_sha256 + ".txt");
  if (std::filesystem::exists(backup)) {
    if (!std::filesystem::is_regular_file(
            std::filesystem::symlink_status(backup)) ||
        content_sha256(read_text_file(backup)) != expected_sha256) {
      throw std::runtime_error(
          "Existing content-addressed backup does not match.");
    }
  } else {
    const auto staged_backup = lock / "backup.txt";
    {
      std::ofstream output(staged_backup, std::ios::binary);
      if (!output) {
        throw std::runtime_error("Cannot create backup.");
      }
      output << previous;
      output.close();
      if (!output) {
        throw std::runtime_error("Cannot finish backup.");
      }
    }
    std::filesystem::permissions(staged_backup, permissions);
    std::filesystem::create_hard_link(staged_backup, backup);
  }
  const auto temporary = lock / "replacement.txt";
  {
    std::ofstream output(temporary, std::ios::binary);
    if (!output) {
      throw std::runtime_error("Cannot create replacement file.");
    }
    output << text;
    output.close();
    if (!output) {
      throw std::runtime_error("Cannot finish replacement file.");
    }
  }
  std::filesystem::permissions(temporary, permissions);
  if (content_sha256(read_text_file(destination)) != expected_sha256) {
    throw CheckedWriteError("external_change_conflict", "path",
                            "File changed during save preparation.");
  }
  // Same-filesystem rename publishes the complete replacement; advisory lock
  // coordinates editor writers.
  std::filesystem::rename(temporary, destination);
  return backup;
}

} // namespace lyko
