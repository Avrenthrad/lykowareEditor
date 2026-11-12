#include "project_creation.hpp"
#include "project_manifest.hpp"
#include "text_file.hpp"
#include <random>
namespace lyko {
namespace {
std::string utf8(const std::filesystem::path &path) {
  const auto text = path.generic_u8string();
  return std::string(text.begin(), text.end());
}
} // namespace
ProjectSession create_project_directory(const std::filesystem::path &root,
                                        const ProjectManifest &manifest) {
  auto value = manifest;
  if (value.project_id.empty()) {
    std::random_device entropy;
    std::uniform_int_distribution<unsigned> byte(0, 255);
    constexpr char digits[] = "0123456789abcdef";
    for (unsigned i = 0; i < 16; ++i) {
      const auto next = byte(entropy);
      value.project_id += digits[next >> 4];
      value.project_id += digits[next & 15];
    }
  }
  validate_project_manifest(value);
  const auto name = utf8(root.filename());
  if (auto issue = asset_path_error(name)) {
    throw ProjectCreationError("path", *issue);
  }
  for (const auto &[key, path] : value.directories) {
    if (asset_fold(path.substr(0, path.find('/'))) == "lykoware.toml") {
      throw ProjectManifestError(
          "directories." + key,
          "Project directories cannot occupy the lykoware.toml manifest path.");
    }
  }
  std::filesystem::path directory;
  ProjectSession session;
  try {
    const auto parent = std::filesystem::canonical(
        root.has_parent_path() ? root.parent_path()
                               : std::filesystem::path("."));
    if (!std::filesystem::is_directory(parent)) {
      throw ProjectCreationError(
          "path", "Project parent must be an existing directory.");
    }
    directory = parent / root.filename();
    session = ProjectSession{value, utf8(directory / "lykoware.toml"),
                             utf8(directory)};
    if (auto issue = project_session_issue(session)) {
      throw ProjectCreationError(issue->field, issue->message);
    }
    // Atomic reservation: even an existing empty directory is never adopted.
    if (!std::filesystem::create_directory(directory)) {
      throw ProjectCreationError(
          "path", "Project destination must be a new directory.");
    }
  } catch (const ProjectCreationError &) {
    throw;
  } catch (const std::exception &error) {
    throw ProjectCreationError("path", error.what());
  }
  try {
    for (const auto &[key, path] : value.directories) {
      (void)key;
      std::filesystem::create_directories(directory / path_from_utf8(path));
    }
    // An opener cannot observe a ready project before all folder creation
    // succeeds.
    save_project_manifest(directory / "lykoware.toml", value);
  } catch (const std::exception &error) {
    std::error_code cleanup;
    std::filesystem::remove_all(directory, cleanup);
    if (cleanup) {
      throw ProjectCreationError(
          "path", std::string(error.what()) +
                      "; cleanup of the reserved project directory failed; "
                      "inspect the destination.");
    }
    throw ProjectCreationError("path", error.what());
  }
  return session;
}
} // namespace lyko
