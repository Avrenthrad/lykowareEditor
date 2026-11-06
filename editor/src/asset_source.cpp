#include "asset_source.hpp"
#include "prefab.hpp"
#include "text_file.hpp"
namespace lyko {
AssetSource read_asset_source(const Asset &asset,
                              const std::filesystem::path &root) {
  if (auto issue = asset_path_error(asset.path)) {
    throw AssetSourceError("path", *issue);
  }
  if (asset.kind != "scene" && asset.kind != "prefab") {
    throw AssetSourceError(
        "kind", "No content validator is implemented for this asset kind.");
  }
  std::filesystem::path directory;
  try {
    if (!root.is_absolute()) {
      throw AssetSourceError(
          "root", "Project root must be an absolute existing directory.");
    }
    directory = std::filesystem::canonical(root);
    if (!std::filesystem::is_directory(directory)) {
      throw AssetSourceError("root", "Project root must name a directory.");
    }
  } catch (const std::filesystem::filesystem_error &) {
    throw AssetSourceError("root", "Cannot resolve the project directory.");
  }
  std::filesystem::path source;
  try {
    source = std::filesystem::canonical(directory / path_from_utf8(asset.path));
    auto parent = directory.begin(), child = source.begin();
    for (; parent != directory.end(); ++parent, ++child) {
      if (child == source.end() || *parent != *child) {
        throw AssetSourceError(
            "path", "Resolved asset is outside the project directory.");
      }
    }
    if (!std::filesystem::is_regular_file(source)) {
      throw AssetSourceError("path", "Asset source must be a regular file.");
    }
  } catch (const std::filesystem::filesystem_error &) {
    throw AssetSourceError("path", "Cannot resolve the asset source.");
  }
  try {
    const auto text = read_text_file(source);
    return AssetSource{Json::parse(text),
                       static_cast<std::uint64_t>(text.size())};
  } catch (const std::exception &error) {
    throw AssetSourceError("path", error.what());
  }
}
Json validate_asset_source(std::uint64_t id, const Asset &asset,
                           const std::filesystem::path &root) {
  const auto source = read_asset_source(asset, root);
  try {
    const auto document = asset.kind == "scene"
                              ? parse_scene(source.json)
                              : parse_prefab(source.json).scene;
    return Json::Object{
        {"id", id},
        {"path", asset.path},
        {"kind", asset.kind},
        {"valid", true},
        {"bytes", source.bytes},
        {"entities", static_cast<std::uint64_t>(document.entities.size())}};
  } catch (const std::exception &error) {
    throw AssetSourceError("path", error.what());
  }
}
} // namespace lyko
