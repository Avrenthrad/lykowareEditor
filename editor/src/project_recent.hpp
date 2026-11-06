#pragma once
#include "project_session.hpp"
#include <stdexcept>
namespace lyko {
class ProjectRecentError : public std::runtime_error {
public:
  ProjectRecentError(std::string field, std::string message)
      : std::runtime_error(std::move(message)), field_(std::move(field)) {}
  const std::string &field() const { return field_; }

private:
  std::string field_;
};
std::vector<ProjectSession>
load_recent_projects(const std::filesystem::path &path);
void save_recent_projects(const std::filesystem::path &path,
                          const std::vector<ProjectSession> &recent);
} // namespace lyko
