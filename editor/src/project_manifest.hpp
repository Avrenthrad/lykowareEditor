#pragma once
#include "project_model.hpp"
#include <stdexcept>
namespace lyko {
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
ProjectManifest project_manifest_from_json(const Json &value);
Json project_manifest_json(const ProjectManifest &manifest);
void save_project_manifest(const std::filesystem::path &path,
                           const ProjectManifest &manifest);
} // namespace lyko
