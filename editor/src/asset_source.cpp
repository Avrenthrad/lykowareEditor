#include "asset_source.hpp"
#include "content_hash.hpp"
#include "prefab.hpp"
#include "text_file.hpp"
namespace lyko {
namespace {
std::string root_text(const std::filesystem::path &path) {
  const auto encoded = path.generic_u8string();
  std::string text(encoded.begin(), encoded.end());
  if (!json_detail::valid_utf8(text)) {
    throw AssetSourceError(
        "root", "Resolved project directory is not representable as UTF-8.");
  }
  return text;
}
std::filesystem::path source_directory(const std::filesystem::path &root) {
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
  return directory;
}
} // namespace
AssetSource read_asset_source(const Asset &asset,
                              const std::filesystem::path &root) {
  if (auto issue = asset_path_error(asset.path)) {
    throw AssetSourceError("path", *issue);
  }
  if (asset.kind != "scene" && asset.kind != "prefab") {
    throw AssetSourceError(
        "kind", "No content validator is implemented for this asset kind.");
  }
  const auto directory = source_directory(root);
  const auto resolved_root = root_text(directory);
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
                       static_cast<std::uint64_t>(text.size()),
                       content_sha256(text), resolved_root};
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
        {"root", source.root},
        {"valid", true},
        {"bytes", source.bytes},
        {"sha256", source.sha256},
        {"entities", static_cast<std::uint64_t>(document.entities.size())}};
  } catch (const std::exception &error) {
    throw AssetSourceError("path", error.what());
  }
}
Json audit_asset_sources(const AssetCatalog &catalog,
                         const std::filesystem::path &root) {
  const auto directory = source_directory(root);
  const auto resolved_root = root_text(directory);
  Json::Array entries;
  std::uint64_t valid = 0, invalid = 0, skipped = 0;
  for (const auto &[id, asset] : catalog.entries) {
    Json::Object row{{"id", id}, {"path", asset.path}, {"kind", asset.kind}};
    if (asset.kind != "scene" && asset.kind != "prefab") {
      ++skipped;
      row.emplace("status", "skipped");
      row.emplace("reason", "unsupported_asset_kind");
    } else {
      try {
        row = validate_asset_source(id, asset, directory).as<Json::Object>();
        row.erase("root");
        row.emplace("status", "valid");
        ++valid;
      } catch (const AssetSourceError &error) {
        // A root failure invalidates the operation, rather than every entry.
        if (error.field() == "root") {
          throw;
        }
        ++invalid;
        row.emplace("status", "invalid");
        row.emplace("error", Json::Object{{"code", "invalid_asset_source"},
                                          {"field", error.field()},
                                          {"message", error.what()}});
      }
    }
    entries.emplace_back(std::move(row));
  }
  return Json::Object{
      {"root", resolved_root},
      {"total", static_cast<std::uint64_t>(catalog.entries.size())},
      {"valid", valid},
      {"invalid", invalid},
      {"skipped", skipped},
      {"entries", std::move(entries)}};
}
} // namespace lyko
