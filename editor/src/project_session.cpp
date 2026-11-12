#include "project_session.hpp"
#include "project_manifest.hpp"
namespace lyko {
namespace {
std::string utf8_path(const std::filesystem::path &path) {
  const auto text = path.generic_u8string();
  return std::string(text.begin(), text.end());
}
Json session_json(const ProjectSession &session) {
  return Json::Object{{"path", session.path},
                      {"root", session.root},
                      {"manifest", project_manifest_json(session.manifest)}};
}
} // namespace
ProjectSession load_project_session(const std::filesystem::path &path) {
  try {
    const auto canonical = std::filesystem::canonical(path);
    ProjectSession session{load_project_manifest(canonical),
                           utf8_path(canonical),
                           utf8_path(canonical.parent_path())};
    if (auto issue = project_session_issue(session)) {
      throw ProjectManifestError(issue->field, issue->message);
    }
    return session;
  } catch (const ProjectManifestError &) {
    throw;
  } catch (const std::exception &error) {
    throw ProjectManifestError("path", error.what());
  }
}
Json project_state_json(const ProjectState &state) {
  Json::Array recent;
  for (const auto &session : state.recent) {
    recent.emplace_back(session_json(session));
  }
  return Json::Object{
      {"active", state.active ? session_json(*state.active) : Json(nullptr)},
      {"recent", std::move(recent)}};
}
} // namespace lyko
