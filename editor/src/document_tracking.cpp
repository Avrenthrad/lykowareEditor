#include "document_tracking.hpp"
#include "project_manifest.hpp"
#include "scene_io.hpp"
#include "text_file.hpp"
#include "ui/shell_snapshot.hpp"
namespace lyko {
std::map<std::string, Json> persisted_documents(const Document &document) {
  auto workspace = workspace_json(document.workspace).as<Json::Object>();
  workspace.erase("selection");
  return {
      {"scene", scene_json(document)},
      {"catalogue", catalog_json(document.assets)},
      {"workspace", std::move(workspace)},
      {"project", document.project.active
                      ? project_manifest_json(document.project.active->manifest)
                      : Json(nullptr)},
      {"recents", project_state_json(document.project).at("recent")}};
}
std::string observation_path(const std::string &path) {
  const auto text = std::filesystem::weakly_canonical(
                        std::filesystem::absolute(path_from_utf8(path)))
                        .generic_u8string();
  std::string result(text.begin(), text.end());
  if (!json_detail::valid_utf8(result)) {
    throw std::invalid_argument("Observed document path must be UTF-8.");
  }
  return result;
}
DocumentTracking::DocumentTracking()
    : initial_(persisted_documents(Document{})) {}
void DocumentTracking::observe(const DocumentObservation &observation) {
  const auto key = observation.kind == "project" ? "project@" + observation.path
                                                 : observation.kind;
  records_.insert_or_assign(
      key, Record{observation.value, observation.path, observation.sha256});
}
std::optional<std::string>
DocumentTracking::fingerprint(const std::string &kind,
                              const std::string &path) const {
  const auto key = kind == "project" ? "project@" + path : kind;
  const auto found = records_.find(key);
  if (found == records_.end() || found->second.path != path ||
      found->second.sha256.empty()) {
    return {};
  }
  return found->second.sha256;
}
std::optional<Json>
DocumentTracking::clean_project(const std::string &path) const {
  const auto found = records_.find("project@" + path);
  return found == records_.end() ? std::nullopt
                                 : std::optional<Json>(found->second.clean);
}
Json DocumentTracking::status(const Document &document) const {
  Json::Object result;
  for (const auto &[kind, value] : persisted_documents(document)) {
    const bool available =
        kind != "project" || document.project.active.has_value();
    const auto key = kind == "project" && available
                         ? "project@" + document.project.active->path
                         : kind;
    const auto saved = records_.find(key);
    const auto &clean =
        saved == records_.end() ? initial_.at(kind) : saved->second.clean;
    result.emplace(
        kind,
        Json::Object{{"available", available},
                     {"dirty", available && value != clean},
                     {"unsaved", available && saved == records_.end()},
                     {"source_sha256",
                      saved == records_.end() || saved->second.sha256.empty()
                          ? Json(nullptr)
                          : Json(saved->second.sha256)},
                     {"path", saved == records_.end()
                                  ? Json(nullptr)
                                  : Json(saved->second.path)}});
  }
  return Json::Object{{"documents", std::move(result)}};
}
std::optional<DocumentObservation> loaded_document(const std::string &command,
                                                   const Json &args,
                                                   const Mutation &mutation) {
  Document value;
  std::string kind, path;
  if (command == "scene.load") {
    value = std::get<ReplaceScene>(mutation).document;
    kind = "scene";
  } else if (command == "asset.catalog.load") {
    value.assets =
        std::get<ReplaceCatalog>(std::get<AssetMutation>(mutation)).catalog;
    kind = "catalogue";
  } else if (command == "workspace.load") {
    value.workspace = std::get<ReplaceWorkspace>(mutation).workspace;
    kind = "workspace";
  } else if (command == "project.recent.load") {
    value.project.recent =
        std::get<ReplaceRecentProjects>(std::get<ProjectMutation>(mutation))
            .recent;
    kind = "recents";
  } else if (command == "project.open") {
    value.project.active =
        std::get<OpenProject>(std::get<ProjectMutation>(mutation)).project;
    kind = "project";
    path = value.project.active->path;
  } else {
    return {};
  }
  if (path.empty()) {
    path = observation_path(args.at("path").as<std::string>());
  }
  return DocumentObservation{kind, path, persisted_documents(value).at(kind)};
}
} // namespace lyko
