#pragma once
#include "project_model.hpp"
namespace lyko {
struct ProjectSession {
  ProjectManifest manifest;
  std::string path,
      root; // Canonical absolute generic UTF-8 paths from the IO adapter.
  bool operator==(const ProjectSession &) const = default;
};
struct ProjectState {
  std::optional<ProjectSession> active;
  std::vector<ProjectSession> recent; // Session-local MRU, most recent first.
  std::optional<ProjectSession> opened_baseline = {};
  bool operator==(const ProjectState &) const = default;
};
struct OpenProject {
  ProjectSession project;
  std::optional<ProjectSession> clean_origin = {};
};
struct CloseProject {
  std::optional<ProjectSession> clean_origin = {};
};
struct ConfigureProject {
  std::string name;
  std::map<std::string, std::string> directories;
  std::optional<std::string> target = {};
};
struct ClearRecentProjects {};
struct ReplaceRecentProjects {
  std::vector<ProjectSession> recent;
};
using ProjectMutation =
    std::variant<OpenProject, CloseProject, ClearRecentProjects,
                 ConfigureProject, ReplaceRecentProjects>;
inline std::optional<ProjectIssue>
project_session_issue(const ProjectSession &session) {
  if (auto issue = project_manifest_issue(session.manifest)) {
    return issue;
  }
  for (const auto *text : {&session.path, &session.root}) {
    if (text->empty() || text->size() > 32768 ||
        text->find('\0') != std::string::npos ||
        !json_detail::valid_utf8(*text)) {
      return ProjectIssue{
          "path",
          "Project paths need bounded absolute UTF-8 without null characters."};
    }
  }
  const auto path = std::filesystem::path(
      std::u8string(session.path.begin(), session.path.end()));
  const auto root = std::filesystem::path(
      std::u8string(session.root.begin(), session.root.end()));
  if (!path.is_absolute() || !root.is_absolute() || path.filename().empty() ||
      path.lexically_normal() != path || root.lexically_normal() != root ||
      path.parent_path() != root) {
    return ProjectIssue{"path", "Manifest must be an absolute normalized file "
                                "path beneath its containing root."};
  }
  return {};
}
inline std::optional<ProjectIssue>
recent_projects_issue(const std::vector<ProjectSession> &recent) {
  if (recent.size() > 16) {
    return ProjectIssue{"recent",
                        "Recent project history supports at most 16 entries."};
  }
  std::set<std::string> paths;
  for (const auto &entry : recent) {
    if (auto issue = project_session_issue(entry)) {
      return ProjectIssue{"recent." + issue->field, issue->message};
    }
    if (!paths.insert(entry.path).second) {
      return ProjectIssue{"recent", "Duplicate project paths are not allowed."};
    }
  }
  return {};
}
ProjectSession load_project_session(const std::filesystem::path &path);
Json project_state_json(const ProjectState &state);
} // namespace lyko
