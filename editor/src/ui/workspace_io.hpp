#pragma once
#include "workspace.hpp"
#include <filesystem>

namespace lyko {
// Versioned TOML authoring config; selection stays with the live scene.
Workspace load_workspace(const std::filesystem::path& path);
void save_workspace(const std::filesystem::path& path, const Workspace& workspace);
}
