#pragma once
#include "assets.hpp"
#include <optional>

namespace lyko {
struct AuditAction {
  std::string command;
  Json::Object arguments;
};
// Transient report navigation. Never reads files or mutates editor state.
inline bool audit_row_matches(const Json &row, const std::string &query,
                              const std::string &status) {
  if (!status.empty() && row.at("status").as<std::string>() != status) {
    return false;
  }
  auto text = std::to_string(row.at("id").as<std::uint64_t>()) + " " +
              row.at("path").as<std::string>() + " " +
              row.at("kind").as<std::string>() + " " +
              row.at("status").as<std::string>();
  if (row.contains("error")) {
    text += " " + row.at("error").at("message").as<std::string>();
  }
  return asset_fold(text).find(asset_fold(query)) != std::string::npos;
}
inline std::optional<AuditAction>
audit_row_action(const Json &row, const std::string &root, bool instantiate) {
  const auto &kind = row.at("kind").as<std::string>();
  if (kind != "scene" && kind != "prefab") {
    return {};
  }
  Json::Object arguments{{"id", row.at("id")}, {"root", root}};
  if (instantiate) {
    if (kind != "prefab" || row.at("status").as<std::string>() != "valid" ||
        !row.contains("sha256")) {
      return {};
    }
    arguments.emplace("sha256", row.at("sha256"));
    arguments.emplace("parent", std::uint64_t{0});
    return AuditAction{"asset.instantiate_checked", std::move(arguments)};
  }
  return AuditAction{"asset.validate", std::move(arguments)};
}
} // namespace lyko
