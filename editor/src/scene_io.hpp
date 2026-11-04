#pragma once
#include "command_bus.hpp"
#include "json.hpp"
#include <filesystem>

namespace lyko {
Json scene_json(const Document& document);
Document parse_scene(const Json& json);
Document load_scene(const std::filesystem::path& path);
// Publishes a fully written scene only if the destination does not exist.
// Never overwrites a file. Save to a new path when preserving a new version.
void save_scene(const std::filesystem::path& path, const Document& document);
}
