#pragma once
#include "assets.hpp"
#include <stdexcept>
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
class ProjectManifestError : public std::invalid_argument {
public:
  ProjectManifestError(std::string field, std::string message)
      : std::invalid_argument(std::move(message)), field_(std::move(field)) {}
  const std::string &field() const { return field_; }

private:
  std::string field_;
};
void validate_project_manifest(const ProjectManifest &manifest);
ProjectManifest parse_project_manifest(const std::string &text);
ProjectManifest load_project_manifest(const std::filesystem::path &path);
std::string project_manifest_toml(const ProjectManifest &manifest);
Json project_manifest_json(const ProjectManifest &manifest);
void save_project_manifest(const std::filesystem::path &path,
                           const ProjectManifest &manifest);
} // namespace lyko
