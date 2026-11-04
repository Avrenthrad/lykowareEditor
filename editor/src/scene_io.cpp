#include "scene_io.hpp"
#include <chrono>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <system_error>

namespace lyko {
namespace {
using O = Json::Object;
using A = Json::Array;
void exact_keys(const Json& json, std::initializer_list<std::string> names) {
    if (json.as<O>().size() != names.size()) { throw std::invalid_argument("Unexpected scene fields"); }
    for (const auto& name : names) { (void)json.at(name); }
}
}
Json scene_json(const Document& document) {
    A entities;
    for (const auto& [id, name] : document.entities) { entities.emplace_back(O{{"id", id}, {"name", name}}); }
    return O{{"schema_version", std::uint64_t{1}}, {"next_id", document.next_id}, {"entities", std::move(entities)}};
}
Document parse_scene(const Json& json) {
    exact_keys(json, {"schema_version", "next_id", "entities"});
    if (json.at("schema_version").as<std::uint64_t>() != 1) { throw std::invalid_argument("Unsupported scene schema version"); }
    Document document;
    document.next_id = json.at("next_id").as<std::uint64_t>();
    if (document.next_id == 0) { throw std::invalid_argument("next_id must be positive"); }
    for (const auto& entity : json.at("entities").as<A>()) {
        exact_keys(entity, {"id", "name"});
        auto id = entity.at("id").as<std::uint64_t>();
        auto name = entity.at("name").as<std::string>();
        if (id == 0 || id >= document.next_id || name.empty() || name.size() > 128) {
            throw std::invalid_argument("Invalid entity identifier or name");
        }
        if (!document.entities.emplace(id, std::move(name)).second) { throw std::invalid_argument("Duplicate entity identifier"); }
    }
    return document;
}
Document load_scene(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) { throw std::runtime_error("Cannot open scene file"); }
    std::string data; char c;
    while (input.get(c)) {
        if (data.size() == 1024 * 1024) { throw std::invalid_argument("Scene exceeds 1 MiB"); }
        data += c;
    }
    if (!input.eof()) { throw std::runtime_error("Cannot read complete scene file"); }
    return parse_scene(Json::parse(data));
}
void save_scene(const std::filesystem::path& path, const Document& document) {
    if (path.filename().empty()) { throw std::invalid_argument("Scene path must name a file"); }
    const auto json = scene_json(document);
    std::string text = "{\n  \"schema_version\": 1,\n  \"next_id\": " + std::to_string(document.next_id) + ",\n  \"entities\": [";
    bool first = true;
    for (const auto& entity : json.at("entities").as<A>()) {
        if (!first) { text += ','; }
        text += "\n    " + entity.dump(); first = false;
    }
    text += "\n  ]\n}\n";
    if (text.size() > 1024 * 1024) { throw std::invalid_argument("Scene exceeds 1 MiB"); }
    // Round-trip validation happens before any filesystem mutation.
    (void)parse_scene(Json::parse(text));
    const auto parent = path.has_parent_path() ? path.parent_path() : std::filesystem::path(".");
    std::filesystem::path staging;
    for (unsigned attempt = 0; attempt != 100; ++attempt) {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        auto candidate = parent / (".lyko-save-" + std::to_string(stamp) + "-" + std::to_string(attempt));
        std::error_code error;
        if (std::filesystem::create_directory(candidate, error)) { staging = std::move(candidate); break; }
        if (error && error != std::errc::file_exists) { throw std::runtime_error("Cannot create scene staging directory"); }
    }
    if (staging.empty()) { throw std::runtime_error("Cannot reserve scene staging directory"); }
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove_all(path, error); }
    } cleanup{staging};
    const auto temporary = staging / "scene.json";
    std::ofstream output(temporary, std::ios::binary);
    if (!output) { throw std::runtime_error("Cannot create temporary scene"); }
    output << text; output.close();
    if (!output) { throw std::runtime_error("Cannot write complete scene"); }
    std::error_code error;
    // Hard-link publication is atomic and fails if the target already exists.
    // Unsupported filesystems fail safely rather than copying a partial file.
    std::filesystem::create_hard_link(temporary, path, error);
    if (error) { throw std::runtime_error("Cannot publish scene; destination must be new and filesystem must support hard links"); }
}
}
