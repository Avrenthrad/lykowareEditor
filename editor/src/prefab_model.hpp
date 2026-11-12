#pragma once
#include "command_bus.hpp"
#include "json.hpp"
namespace lyko {
inline std::optional<std::string> prefab_error(const Document &scene,
                                               EntityId root) {
  if (!scene.entities.contains(root) || scene.parents.contains(root)) {
    return "Prefab needs one existing unparented root.";
  }
  if (auto issue = hierarchy_error(scene)) {
    return issue;
  }
  for (const auto &[id, name] : scene.entities) {
    if (!id || id >= scene.next_id || name.empty() || name.size() > 128 ||
        !json_detail::valid_utf8(name)) {
      return "Invalid prefab entity.";
    }
    auto ancestor = id;
    while (scene.parents.contains(ancestor)) {
      ancestor = scene.parents.at(ancestor);
    }
    if (ancestor != root) {
      return "Every prefab entity must descend from its root.";
    }
  }
  for (const auto &[id, transform] : scene.transforms) {
    if (!scene.entities.contains(id)) {
      return "Prefab transform references a missing entity.";
    }
    if (auto issue = transform_error(transform)) {
      return issue;
    }
  }
  return {};
}
} // namespace lyko
