#pragma once
#include "workspace.hpp"
#include <filesystem>

namespace lyko {
// Versioned TOML authoring config; selection stays with the live scene.
Workspace parse_workspace(const std::string &source);
std::string workspace_text(const Workspace &workspace);
Workspace load_workspace(const std::filesystem::path &path);
void save_workspace(const std::filesystem::path &path,
                    const Workspace &workspace);
} // namespace lyko
