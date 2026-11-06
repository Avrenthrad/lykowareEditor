#pragma once
#include "assets.hpp"
namespace lyko {
struct ProjectManifest {
  std::string project_id, name;
  std::map<std::string, std::string> directories{{"assets", "assets"},
                                                 {"scenes", "scenes"},
                                                 {"scripts", "scripts"},
                                                 {"graphs", "graphs"},
                                                 {"cache", ".lyko/cache"}};
  bool operator==(const ProjectManifest &) const = default;
};
struct ProjectIssue {
  std::string field, message;
};
inline std::optional<ProjectIssue>
project_manifest_issue(const ProjectManifest &manifest) {
  if (manifest.project_id.size() != 32 ||
      !std::all_of(manifest.project_id.begin(), manifest.project_id.end(),
                   [](char c) {
                     return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
                   })) {
    return ProjectIssue{
        "project_id",
        "Use 32 lowercase hexadecimal project identity characters."};
  }
  if (manifest.name.empty() || manifest.name.size() > 128 ||
      !json_detail::valid_utf8(manifest.name) ||
      std::any_of(manifest.name.begin(), manifest.name.end(),
                  [](unsigned char c) { return c < 32 || c == 127; })) {
    return ProjectIssue{
        "name", "Name needs 1 to 128 UTF-8 bytes without ASCII controls."};
  }
  const ProjectManifest defaults;
  if (manifest.directories.size() != defaults.directories.size()) {
    return ProjectIssue{"directories",
                        "Supply the five supported directory roles."};
  }
  for (const auto &[key, path] : manifest.directories) {
    if (!defaults.directories.contains(key)) {
      return ProjectIssue{"directories", "Unknown project directory role."};
    }
    if (auto issue = asset_path_error(path)) {
      return ProjectIssue{"directories." + key, *issue};
    }
  }
  const auto nested = [](const std::string &a, const std::string &b) {
    return a == b ||
           (a.size() > b.size() && a.starts_with(b) && a[b.size()] == '/');
  };
  const auto cache = asset_fold(manifest.directories.at("cache"));
  for (const auto &[key, path] : manifest.directories) {
    const auto source = asset_fold(path);
    if (key != "cache" && (nested(cache, source) || nested(source, cache))) {
      return ProjectIssue{"directories.cache",
                          "Cache and source directories must not overlap."};
    }
  }
  return {};
}
} // namespace lyko
