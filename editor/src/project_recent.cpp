#include "project_recent.hpp"
#include "text_file.hpp"
namespace lyko {
namespace {
void fields(const Json &value, std::initializer_list<const char *> expected) {
  if (value.as<Json::Object>().size() != expected.size()) {
    throw std::invalid_argument("Unexpected recent-project fields.");
  }
  for (const auto *field : expected) {
    (void)value.at(field);
  }
}
} // namespace
std::vector<ProjectSession>
load_recent_projects(const std::filesystem::path &path) {
  std::string text;
  try {
    if (!std::filesystem::is_regular_file(path)) {
      throw ProjectRecentError("path",
                               "Recent-project store must be a regular file.");
    }
    text = read_text_file(path);
  } catch (const ProjectRecentError &) {
    throw;
  } catch (const std::exception &error) {
    throw ProjectRecentError("path", error.what());
  }
  try {
    return parse_recent_projects(Json::parse(text));
  } catch (const ProjectRecentError &) {
    throw;
  } catch (const std::exception &error) {
    throw ProjectRecentError("recent", error.what());
  }
}
std::vector<ProjectSession> parse_recent_projects(const Json &value) {
  try {
    fields(value, {"schema_version", "recent"});
    if (value.at("schema_version").as<std::uint64_t>() != 1) {
      throw std::invalid_argument("Unsupported recent-project store version.");
    }
    std::vector<ProjectSession> recent;
    const auto &entries = value.at("recent").as<Json::Array>();
    if (entries.size() > 16) {
      throw std::invalid_argument("Recent-project store exceeds 16 entries.");
    }
    for (const auto &entry : entries) {
      fields(entry, {"path", "root", "manifest"});
      const auto &metadata = entry.at("manifest");
      fields(metadata, {"schema_version", "project_id", "name", "directories"});
      if (metadata.at("schema_version").as<std::uint64_t>() != 1) {
        throw std::invalid_argument("Unsupported stored manifest version.");
      }
      ProjectManifest manifest;
      manifest.project_id = metadata.at("project_id").as<std::string>();
      manifest.name = metadata.at("name").as<std::string>();
      manifest.directories.clear();
      for (const auto &[role, directory] :
           metadata.at("directories").as<Json::Object>()) {
        manifest.directories.emplace(role, directory.as<std::string>());
      }
      recent.push_back(ProjectSession{std::move(manifest),
                                      entry.at("path").as<std::string>(),
                                      entry.at("root").as<std::string>()});
    }
    if (auto issue = recent_projects_issue(recent)) {
      throw ProjectRecentError(issue->field, issue->message);
    }
    return recent;
  } catch (const ProjectRecentError &) {
    throw;
  } catch (const std::exception &error) {
    throw ProjectRecentError("recent", error.what());
  }
}
std::string recent_projects_text(const std::vector<ProjectSession> &recent) {
  if (auto issue = recent_projects_issue(recent)) {
    throw ProjectRecentError(issue->field, issue->message);
  }
  const auto entries =
      project_state_json(ProjectState{{}, recent}).at("recent");
  std::string text = "{\n  \"schema_version\": 1,\n  \"recent\": [";
  bool first = true;
  for (const auto &entry : entries.as<Json::Array>()) {
    if (!first) {
      text += ',';
    }
    first = false;
    text += "\n    " + entry.dump();
  }
  text += "\n  ]\n}\n";
  return text;
}
void save_recent_projects(const std::filesystem::path &path,
                          const std::vector<ProjectSession> &recent) {
  try {
    publish_text_file(path, recent_projects_text(recent));
  } catch (const ProjectRecentError &) {
    throw;
  } catch (const std::exception &error) {
    throw ProjectRecentError("path", error.what());
  }
}
} // namespace lyko
