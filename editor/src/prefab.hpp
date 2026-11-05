#pragma once
#include "prefab_model.hpp"
#include "scene_io.hpp"
namespace lyko {
struct Prefab {
  Document scene;
  EntityId root;
};
Prefab parse_prefab(const Json &value);
Prefab load_prefab(const std::filesystem::path &path);
void export_prefab(const std::filesystem::path &path, const Document &document,
                   EntityId root);
} // namespace lyko
