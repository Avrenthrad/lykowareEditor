#pragma once
#include "json.hpp"
#include <algorithm>
#include <filesystem>
#include <optional>
#include <set>
namespace lyko {
struct Asset {
  std::string path, kind;
  bool operator==(const Asset &) const = default;
};
struct AssetCatalog {
  std::map<std::uint64_t, Asset> entries;
  std::uint64_t next_id = 1;
  bool operator==(const AssetCatalog &) const = default;
};
struct RegisterAsset {
  std::string path, kind;
};
struct RelocateAsset {
  std::uint64_t id;
  std::string path;
};
struct ForgetAsset {
  std::uint64_t id;
};
struct ReplaceCatalog {
  AssetCatalog catalog;
};
using AssetMutation =
    std::variant<RegisterAsset, RelocateAsset, ForgetAsset, ReplaceCatalog>;
inline std::string asset_fold(std::string text) {
  for (auto &c : text) {
    if (c >= 'A' && c <= 'Z') {
      c = static_cast<char>(c + 'a' - 'A');
    }
  }
  return text;
}
inline std::optional<std::string> asset_path_error(const std::string &path) {
  if (path.empty() || path.size() > 512 || !json_detail::valid_utf8(path)) {
    return "Asset path needs 1 to 512 UTF-8 bytes.";
  }
  for (unsigned char c : path) {
    if (c < 32 || c == 127 || c == '\\' || c == ':' || c == '*' || c == '?' ||
        c == '"' || c == '<' || c == '>' || c == '|') {
      return "Use a portable project-relative path with forward slashes.";
    }
  }
  std::size_t start = 0;
  for (;;) {
    const auto end = path.find('/', start);
    const auto part =
        path.substr(start, end == std::string::npos ? end : end - start);
    if (part.empty() || part == "." || part == ".." || part.back() == '.' ||
        part.back() == ' ') {
      return "Asset path cannot contain empty/dot segments or trailing "
             "dots/spaces.";
    }
    const auto base = asset_fold(part.substr(0, part.find('.')));
    if (base == "con" || base == "prn" || base == "aux" || base == "nul" ||
        (base.size() == 4 &&
         (base.starts_with("com") || base.starts_with("lpt")) &&
         base[3] >= '1' && base[3] <= '9')) {
      return "Asset path contains a reserved platform name.";
    }
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }
  return {};
}
inline std::optional<std::string> catalog_error(const AssetCatalog &catalog) {
  if (!catalog.next_id || catalog.entries.size() > 4096) {
    return "Catalog needs a positive next ID and at most 4096 entries.";
  }
  std::set<std::string> paths;
  for (const auto &[id, asset] : catalog.entries) {
    if (!id || id >= catalog.next_id) {
      return "Invalid catalog asset ID.";
    }
    if (auto issue = asset_path_error(asset.path)) {
      return issue;
    }
    if (asset.kind != "scene" && asset.kind != "prefab" &&
        asset.kind != "mesh" && asset.kind != "texture" &&
        asset.kind != "script" && asset.kind != "graph") {
      return "Choose scene, prefab, mesh, texture, script or graph.";
    }
    const auto folded = asset_fold(asset.path);
    if (!paths.insert(folded).second) {
      return "Asset paths must be unique ignoring ASCII case.";
    }
  }
  return {};
}
inline Json catalog_json(const AssetCatalog &catalog) {
  Json::Array entries;
  for (const auto &[id, asset] : catalog.entries) {
    entries.emplace_back(
        Json::Object{{"id", id}, {"path", asset.path}, {"kind", asset.kind}});
  }
  return Json::Object{{"schema_version", std::uint64_t{1}},
                      {"next_id", catalog.next_id},
                      {"assets", std::move(entries)}};
}
AssetCatalog load_catalog(const std::filesystem::path &path);
void save_catalog(const std::filesystem::path &path,
                  const AssetCatalog &catalog);
} // namespace lyko
