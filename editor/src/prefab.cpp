#include "prefab.hpp"
#include "text_file.hpp"
namespace lyko {
Prefab parse_prefab(const Json &value) {
  if (value.as<Json::Object>().size() != 4 ||
      value.at("type").as<std::string>() != "lykoware.prefab" ||
      value.at("schema_version").as<std::uint64_t>() != 1) {
    throw std::invalid_argument("Unsupported prefab fields/type/version.");
  }
  Prefab prefab{parse_scene(value.at("scene")),
                value.at("root").as<std::uint64_t>()};
  if (auto issue = prefab_error(prefab.scene, prefab.root)) {
    throw std::invalid_argument(*issue);
  }
  return prefab;
}
Prefab load_prefab(const std::filesystem::path &path) {
  return parse_prefab(Json::parse(read_text_file(path)));
}
void export_prefab(const std::filesystem::path &path, const Document &document,
                   EntityId root) {
  if (!document.entities.contains(root)) {
    throw std::invalid_argument("Prefab source entity does not exist.");
  }
  if (auto issue = hierarchy_error(document)) {
    throw std::invalid_argument(*issue);
  }
  Document source;
  std::map<EntityId, EntityId> ids;
  std::map<EntityId, std::vector<EntityId>> children;
  for (const auto &[id, parent] : document.parents) {
    children[parent].push_back(id);
  }
  std::vector<EntityId> pending{root};
  while (!pending.empty()) {
    const auto id = pending.back();
    pending.pop_back();
    const auto local = source.next_id++;
    ids.emplace(id, local);
    source.entities.emplace(local, document.entities.at(id));
    if (document.transforms.contains(id)) {
      source.transforms.emplace(local, document.transforms.at(id));
    }
    if (id != root) {
      source.parents.emplace(local, ids.at(document.parents.at(id)));
    }
    for (auto it = children[id].rbegin(); it != children[id].rend(); ++it) {
      pending.push_back(*it);
    }
  }
  const Json value = Json::Object{{"type", "lykoware.prefab"},
                                  {"schema_version", std::uint64_t{1}},
                                  {"root", std::uint64_t{1}},
                                  {"scene", scene_json(source)}};
  (void)parse_prefab(value);
  // One nested scene entity per line, matching authoring scene diffs.
  const auto scene = value.at("scene");
  std::string text =
      "{\n  \"type\": \"lykoware.prefab\",\n  \"schema_version\": 1,\n  "
      "\"root\": 1,\n  \"scene\": {\n    \"schema_version\": " +
      scene.at("schema_version").dump() +
      ",\n    \"next_id\": " + scene.at("next_id").dump() +
      ",\n    \"entities\": [";
  bool first = true;
  for (const auto &entity : scene.at("entities").as<Json::Array>()) {
    if (!first) {
      text += ',';
    }
    first = false;
    text += "\n      " + entity.dump();
  }
  text += "\n    ]\n  }\n}\n";
  if (text.size() > 1024 * 1024) {
    throw std::invalid_argument("Prefab exceeds 1 MiB.");
  }
  publish_text_file(path, text);
}
} // namespace lyko
