#include "scene_io.hpp"
#include "text_file.hpp"
#include <stdexcept>

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
    if (auto issue = hierarchy_error(document)) { throw std::invalid_argument(*issue); }
    A entities;
    for (const auto& [id, name] : document.entities) {
        O entity{{"id", id}, {"name", name}};
        if (!document.parents.empty()) { entity.emplace("parent", document.parents.contains(id) ? document.parents.at(id) : EntityId{0}); }
        entities.emplace_back(std::move(entity));
    }
    return O{{"schema_version", std::uint64_t{document.parents.empty() ? 1U : 2U}}, {"next_id", document.next_id}, {"entities", std::move(entities)}};
}
Document parse_scene(const Json& json) {
    exact_keys(json, {"schema_version", "next_id", "entities"});
    const auto version = json.at("schema_version").as<std::uint64_t>();
    if (version != 1 && version != 2) { throw std::invalid_argument("Unsupported scene schema version"); }
    Document document;
    document.next_id = json.at("next_id").as<std::uint64_t>();
    if (document.next_id == 0) { throw std::invalid_argument("next_id must be positive"); }
    for (const auto& entity : json.at("entities").as<A>()) {
        if (version == 1) { exact_keys(entity, {"id", "name"}); }
        else { exact_keys(entity, {"id", "name", "parent"}); }
        auto id = entity.at("id").as<std::uint64_t>();
        auto name = entity.at("name").as<std::string>();
        if (id == 0 || id >= document.next_id || name.empty() || name.size() > 128) {
            throw std::invalid_argument("Invalid entity identifier or name");
        }
        if (version == 2) { const auto parent = entity.at("parent").as<std::uint64_t>(); if (parent) { document.parents.emplace(id, parent); } }
        if (!document.entities.emplace(id, std::move(name)).second) { throw std::invalid_argument("Duplicate entity identifier"); }
    }
    if (auto issue = hierarchy_error(document)) { throw std::invalid_argument(*issue); }
    return document;
}
Document load_scene(const std::filesystem::path& path) {
    return parse_scene(Json::parse(read_text_file(path)));
}

void save_scene(const std::filesystem::path& path, const Document& document) {
    if (path.filename().empty()) { throw std::invalid_argument("Scene path must name a file"); }
    const auto json = scene_json(document);
    std::string text = "{\n  \"schema_version\": " + json.at("schema_version").dump() + ",\n  \"next_id\": " + std::to_string(document.next_id) + ",\n  \"entities\": [";
    bool first = true;
    for (const auto& entity : json.at("entities").as<A>()) {
        if (!first) { text += ','; }
        text += "\n    " + entity.dump(); first = false;
    }
    text += "\n  ]\n}\n";
    if (text.size() > 1024 * 1024) { throw std::invalid_argument("Scene exceeds 1 MiB"); }
    // Round-trip validation happens before any filesystem mutation.
    (void)parse_scene(Json::parse(text));
    publish_text_file(path, text);
}
}
