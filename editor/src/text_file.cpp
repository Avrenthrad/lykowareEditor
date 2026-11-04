#include "text_file.hpp"
#include "json.hpp"
#include <chrono>
#include <fstream>
#include <stdexcept>
#include <system_error>

namespace lyko {
namespace {
void validate_path(const std::filesystem::path& path) {
    if (path.empty() || path.native().find(std::filesystem::path::value_type{}) != std::filesystem::path::string_type::npos) {
        throw std::invalid_argument("File path must be nonempty and contain no null characters");
    }
}
}
std::filesystem::path path_from_utf8(const std::string& text) {
    if (text.empty() || text.find('\0') != std::string::npos || !json_detail::valid_utf8(text)) {
        throw std::invalid_argument("File path must be nonempty UTF-8 without null characters");
    }
    std::u8string encoded; encoded.reserve(text.size());
    for (unsigned char c : text) { encoded.push_back(static_cast<char8_t>(c)); }
    return std::filesystem::path(encoded);
}
std::string read_text_file(const std::filesystem::path& path) {
    validate_path(path);
    std::ifstream input(path, std::ios::binary);
    if (!input) { throw std::runtime_error("Cannot open text file"); }
    std::string data; char c;
    while (input.get(c)) {
        if (data.size() == 1024 * 1024) { throw std::invalid_argument("File exceeds 1 MiB"); }
        data += c;
    }
    if (!input.eof()) { throw std::runtime_error("Cannot read complete text file"); }
    return data;
}
void publish_text_file(const std::filesystem::path& path, const std::string& text) {
    validate_path(path);
    if (path.filename().empty()) { throw std::invalid_argument("Path must name a file"); }
    if (text.size() > 1024 * 1024) { throw std::invalid_argument("File exceeds 1 MiB"); }
    const auto parent = path.has_parent_path() ? path.parent_path() : std::filesystem::path(".");
    std::filesystem::path staging;
    for (unsigned attempt = 0; attempt != 100; ++attempt) {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        auto candidate = parent / (".lyko-save-" + std::to_string(stamp) + "-" + std::to_string(attempt));
        std::error_code error;
        if (std::filesystem::create_directory(candidate, error)) { staging = std::move(candidate); break; }
        if (error && error != std::errc::file_exists) { throw std::runtime_error("Cannot create file staging directory"); }
    }
    if (staging.empty()) { throw std::runtime_error("Cannot reserve file staging directory"); }
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove_all(path, error); }
    } cleanup{staging};
    const auto temporary = staging / "content.txt";
    std::ofstream output(temporary, std::ios::binary);
    if (!output) { throw std::runtime_error("Cannot create temporary file"); }
    output << text; output.close();
    if (!output) { throw std::runtime_error("Cannot write complete file"); }
    std::error_code error;
    // Hard-link publication is atomic and fails if the target already exists.
    // Unsupported filesystems fail safely rather than copying a partial file.
    std::filesystem::create_hard_link(temporary, path, error);
    if (error) { throw std::runtime_error("Cannot publish file; destination must be new and filesystem must support hard links"); }
}
}
