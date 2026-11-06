#pragma once
#include "project_session.hpp"
#include <stdexcept>
namespace lyko {
class ProjectCreationError : public std::runtime_error {
public:
  ProjectCreationError(std::string field, std::string message)
      : std::runtime_error(std::move(message)), field_(std::move(field)) {}
  const std::string &field() const { return field_; }

private:
  std::string field_;
};
// Reserve a new root, populate directories, publish the readiness manifest
// last. Filesystem export only: callers explicitly open the returned manifest
// afterward.
ProjectSession create_project_directory(const std::filesystem::path &root,
                                        const ProjectManifest &manifest);
} // namespace lyko
