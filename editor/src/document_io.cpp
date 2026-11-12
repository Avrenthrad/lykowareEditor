#include "document_io.hpp"
#include "content_hash.hpp"
#include "project_manifest.hpp"
#include "project_recent.hpp"
#include "scene_io.hpp"
#include "text_file.hpp"
#include "ui/workspace_io.hpp"
namespace lyko {
ObservedText read_observed_text(const std::filesystem::path &path) {
  const auto source = std::filesystem::canonical(path);
  if (!std::filesystem::is_regular_file(source)) {
    throw std::runtime_error("Document source must be a regular file.");
  }
  const auto encoded = source.generic_u8string();
  std::string name(encoded.begin(), encoded.end());
  if (!json_detail::valid_utf8(name)) {
    throw std::invalid_argument("Document path must be UTF-8.");
  }
  auto text = read_text_file(source);
  const auto digest = content_sha256(text);
  return {std::move(name), std::move(text), digest};
}
std::string serialize_document(const std::string &kind,
                               const Document &document) {
  if (kind == "scene") {
    return scene_text(document);
  }
  if (kind == "catalogue") {
    return catalog_text(document.assets);
  }
  if (kind == "workspace") {
    return workspace_text(document.workspace);
  }
  if (kind == "recents") {
    return recent_projects_text(document.project.recent);
  }
  if (kind == "project" && document.project.active) {
    return project_manifest_toml(document.project.active->manifest);
  }
  throw std::invalid_argument("No active persisted document for this kind.");
}
} // namespace lyko
