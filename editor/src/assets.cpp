#include "assets.hpp"
#include "text_file.hpp"
namespace lyko {
AssetCatalog parse_catalog(const Json &json) {
  if (json.as<Json::Object>().size() != 3 ||
      json.at("schema_version").as<std::uint64_t>() != 1) {
    throw std::invalid_argument("Unsupported catalog fields/version.");
  }
  AssetCatalog catalog;
  catalog.next_id = json.at("next_id").as<std::uint64_t>();
  const auto &entries = json.at("assets").as<Json::Array>();
  if (entries.size() > 4096) {
    throw std::invalid_argument("Catalog exceeds 4096 entries.");
  }
  for (const auto &item : entries) {
    if (item.as<Json::Object>().size() != 3) {
      throw std::invalid_argument("Unexpected asset fields.");
    }
    const auto id = item.at("id").as<std::uint64_t>();
    if (!catalog.entries
             .emplace(id, Asset{item.at("path").as<std::string>(),
                                item.at("kind").as<std::string>()})
             .second) {
      throw std::invalid_argument("Duplicate asset ID.");
    }
  }
  if (auto issue = catalog_error(catalog)) {
    throw std::invalid_argument(*issue);
  }
  return catalog;
}
std::string catalog_text(const AssetCatalog &catalog) {
  if (auto issue = catalog_error(catalog)) {
    throw std::invalid_argument(*issue);
  }
  const auto json = catalog_json(catalog);
  std::string text = "{\n  \"schema_version\": 1,\n  \"next_id\": " +
                     std::to_string(catalog.next_id) + ",\n  \"assets\": [";
  bool first = true;
  for (const auto &item : json.at("assets").as<Json::Array>()) {
    if (!first) {
      text += ',';
    }
    first = false;
    text += "\n    " + item.dump();
  }
  text += "\n  ]\n}\n";
  return text;
}
AssetCatalog load_catalog(const std::filesystem::path &path) {
  return parse_catalog(Json::parse(read_text_file(path)));
}
void save_catalog(const std::filesystem::path &path,
                  const AssetCatalog &catalog) {
  publish_text_file(path, catalog_text(catalog));
}
} // namespace lyko
