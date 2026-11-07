#include "scene_io.hpp"
#include "text_file.hpp"
#include "transform_json.hpp"
#include <stdexcept>

namespace lyko {
namespace {
using O = Json::Object;
using A = Json::Array;
void exact_keys(const Json &json, std::initializer_list<std::string> names) {
  if (json.as<O>().size() != names.size()) {
    throw std::invalid_argument("Unexpected scene fields");
  }
  for (const auto &name : names) {
    (void)json.at(name);
  }
}
} // namespace
Json scene_json(const Document &document) {
  if (auto issue = hierarchy_error(document)) {
    throw std::invalid_argument(*issue);
  }
  for (const auto &[id, transform] : document.transforms) {
    if (!document.entities.contains(id)) {
      throw std::invalid_argument("Transform references a missing entity.");
    }
    if (auto issue = transform_error(transform)) {
      throw std::invalid_argument(*issue);
    }
  }
  A entities;
  for (const auto &[id, name] : document.entities) {
    O entity{{"id", id}, {"name", name}};
    if (!document.parents.empty() || !document.transforms.empty()) {
      entity.emplace("parent", document.parents.contains(id)
                                   ? document.parents.at(id)
                                   : EntityId{0});
    }
    if (!document.transforms.empty()) {
      entity.emplace("transform",
                     transform_json(document.transforms.contains(id)
                                        ? document.transforms.at(id)
                                        : Transform{}));
    }
    entities.emplace_back(std::move(entity));
  }
  return O{{"schema_version",
            std::uint64_t{!document.transforms.empty()
                              ? 3U
                              : (document.parents.empty() ? 1U : 2U)}},
           {"next_id", document.next_id},
           {"entities", std::move(entities)}};
}
Document parse_scene(const Json &json) {
  exact_keys(json, {"schema_version", "next_id", "entities"});
  const auto version = json.at("schema_version").as<std::uint64_t>();
  if (version != 1 && version != 2 && version != 3) {
    throw std::invalid_argument("Unsupported scene schema version");
  }
  Document document;
  document.next_id = json.at("next_id").as<std::uint64_t>();
  if (document.next_id == 0) {
    throw std::invalid_argument("next_id must be positive");
  }
  for (const auto &entity : json.at("entities").as<A>()) {
    if (version == 1) {
      exact_keys(entity, {"id", "name"});
    } else if (version == 2) {
      exact_keys(entity, {"id", "name", "parent"});
    } else {
      exact_keys(entity, {"id", "name", "parent", "transform"});
    }
    auto id = entity.at("id").as<std::uint64_t>();
    auto name = entity.at("name").as<std::string>();
    if (id == 0 || id >= document.next_id || name.empty() ||
        name.size() > 128) {
      throw std::invalid_argument("Invalid entity identifier or name");
    }
    if (version >= 2) {
      const auto parent = entity.at("parent").as<std::uint64_t>();
      if (parent) {
        document.parents.emplace(id, parent);
      }
    }
    if (version == 3) {
      auto transform = parse_transform(entity.at("transform"));
      if (transform != Transform{}) {
        document.transforms.emplace(id, transform);
      }
    }
    if (!document.entities.emplace(id, std::move(name)).second) {
      throw std::invalid_argument("Duplicate entity identifier");
    }
  }
  if (auto issue = hierarchy_error(document)) {
    throw std::invalid_argument(*issue);
  }
  return document;
}
Document load_scene(const std::filesystem::path &path) {
  return parse_scene(Json::parse(read_text_file(path)));
}

std::string scene_text(const Document &document) {
  const auto json = scene_json(document);
  std::string text =
      "{\n  \"schema_version\": " + json.at("schema_version").dump() +
      ",\n  \"next_id\": " + std::to_string(document.next_id) +
      ",\n  \"entities\": [";
  bool first = true;
  for (const auto &entity : json.at("entities").as<A>()) {
    if (!first) {
      text += ',';
    }
    text += "\n    " + entity.dump();
    first = false;
  }
  text += "\n  ]\n}\n";
  if (text.size() > 1024 * 1024) {
    throw std::invalid_argument("Scene exceeds 1 MiB");
  }
  // Round-trip validation happens before any filesystem mutation.
  (void)parse_scene(Json::parse(text));
  return text;
}
void save_scene(const std::filesystem::path &path, const Document &document) {
  publish_text_file(path, scene_text(document));
}

} // namespace lyko
