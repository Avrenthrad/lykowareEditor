#include "workspace_io.hpp"
#include "dock_layout_json.hpp"
#include "json.hpp"
#include "text_file.hpp"
#include <set>
#include <sstream>
#include <stdexcept>

namespace lyko {
namespace {
std::string trim(const std::string &text) {
  const auto first = text.find_first_not_of(" \t\r\n");
  return first == std::string::npos
             ? ""
             : text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}
// Find a delimiter outside TOML basic quoted strings (the generated v1 format).
std::size_t outside(const std::string &text, char delimiter) {
  bool quoted = false, escaped = false;
  for (std::size_t i = 0; i < text.size(); ++i) {
    const char c = text[i];
    if (escaped) {
      escaped = false;
      continue;
    }
    if (quoted && c == '\\') {
      escaped = true;
      continue;
    }
    if (c == '"') {
      quoted = !quoted;
      continue;
    }
    if (!quoted && c == delimiter) {
      return i;
    }
  }
  return std::string::npos;
}
using Table = std::map<std::string, Json>;
} // namespace
Workspace parse_workspace(const std::string &source) {
  std::istringstream input(source);
  std::map<std::string, Table> tables{{"", {}}};
  std::string section;
  for (std::string line; std::getline(input, line);) {
    const auto comment = outside(line, '#');
    if (comment != std::string::npos) {
      line.resize(comment);
    }
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    if (line.front() == '[' && line.back() == ']') {
      section = trim(line.substr(1, line.size() - 2));
      if (section != "panels" && section != "splits" && section != "keymap" &&
          section != "layout" &&
          !(section.starts_with("layout.nodes.") &&
            valid_dock_id(section.substr(13)))) {
        throw std::invalid_argument("Unknown workspace TOML table");
      }
      if (!tables.emplace(section, Table{}).second) {
        throw std::invalid_argument("Duplicate workspace TOML table");
      }
      continue;
    }
    const auto equal = outside(line, '=');
    if (equal == std::string::npos) {
      throw std::invalid_argument("Expected TOML key = value");
    }
    auto key = trim(line.substr(0, equal));
    if (key.empty()) {
      throw std::invalid_argument("Empty TOML key");
    }
    if (key.front() == '"') {
      key = Json::parse(key).as<std::string>();
    } else {
      for (char c : key) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) {
          throw std::invalid_argument("Unsupported TOML key syntax");
        }
      }
    }
    auto value = Json::parse(trim(line.substr(equal + 1)));
    if (!tables.at(section).emplace(std::move(key), std::move(value)).second) {
      throw std::invalid_argument("Duplicate workspace TOML key");
    }
  }
  if (!tables.contains("panels") || !tables.contains("splits") ||
      !tables.contains("keymap")) {
    throw std::invalid_argument("Missing workspace TOML table");
  }
  const auto &root = tables.at("");
  if (root.size() != 4 || !root.contains("schema_version") ||
      !root.contains("scale_percent") || !root.contains("theme") ||
      !root.contains("reduced_motion")) {
    throw std::invalid_argument("Workspace requires schema_version, "
                                "scale_percent, theme and reduced_motion");
  }
  const auto version = root.at("schema_version").as<std::uint64_t>();
  if (version != 1 && version != 2) {
    throw std::invalid_argument("Unsupported workspace schema version");
  }
  if (version == 1 && tables.size() != 4) {
    throw std::invalid_argument("Version 1 does not support layout tables");
  }
  Workspace workspace;
  workspace.scale_percent = root.at("scale_percent").as<std::uint64_t>();
  workspace.theme = root.at("theme").as<std::string>();
  workspace.reduced_motion = root.at("reduced_motion").as<bool>();
  workspace.panels.clear();
  workspace.splits.clear();
  workspace.keymap.clear();
  for (const auto &[name, value] : tables.at("panels")) {
    workspace.panels.emplace(name, value.as<std::string>());
  }
  for (const auto &[name, value] : tables.at("splits")) {
    workspace.splits.emplace(name, value.as<std::uint64_t>());
  }
  for (const auto &[name, value] : tables.at("keymap")) {
    workspace.keymap.emplace(name, value.as<std::string>());
  }
  if (version == 2) {
    if (!tables.contains("layout") || tables.at("layout").size() != 2) {
      throw std::invalid_argument("Missing layout root/hidden fields");
    }
    Json::Object nodes;
    for (const auto &[name, table] : tables) {
      if (name.starts_with("layout.nodes.")) {
        nodes.emplace(name.substr(13), table);
      }
    }
    const auto &layout = tables.at("layout");
    if (!layout.contains("root") || !layout.contains("hidden")) {
      throw std::invalid_argument("Missing layout root/hidden fields");
    }
    workspace.layout =
        parse_dock_layout(Json::Object{{"root", layout.at("root")},
                                       {"hidden", layout.at("hidden")},
                                       {"nodes", std::move(nodes)}});
  }
  if (auto error = workspace_error(workspace)) {
    throw std::invalid_argument(*error);
  }
  return workspace;
}
std::string workspace_text(const Workspace &workspace) {
  if (auto error = workspace_error(workspace)) {
    throw std::invalid_argument(*error);
  }
  std::string text =
      "# Lykoware workspace configuration v2\nschema_version = "
      "2\nscale_percent = " +
      std::to_string(workspace.scale_percent) +
      "\ntheme = " + Json(workspace.theme).dump() +
      "\nreduced_motion = " + (workspace.reduced_motion ? "true" : "false") +
      "\n\n[panels]\n";
  for (const auto &[panel, dock] : workspace.panels) {
    text += Json(panel).dump() + " = " + Json(dock).dump() + '\n';
  }
  text += "\n[splits]\n";
  for (const auto &[dock, percent] : workspace.splits) {
    text += Json(dock).dump() + " = " + std::to_string(percent) + '\n';
  }
  text += "\n[keymap]\n";
  for (const auto &[command, shortcut] : workspace.keymap) {
    text += Json(command).dump() + " = " + Json(shortcut).dump() + '\n';
  }
  const auto layout = dock_layout_json(effective_dock_layout(workspace));
  text += "\n[layout]\nroot = " + layout.at("root").dump() +
          "\nhidden = " + layout.at("hidden").dump() + "\n";
  for (const auto &[id, node] : layout.at("nodes").as<Json::Object>()) {
    text += "\n[layout.nodes." + id + "]\n";
    for (const auto &[key, value] : node.as<Json::Object>()) {
      text += key + " = " + value.dump() + '\n';
    }
  }
  return text;
}
Workspace load_workspace(const std::filesystem::path &path) {
  return parse_workspace(read_text_file(path));
}
void save_workspace(const std::filesystem::path &path,
                    const Workspace &workspace) {
  publish_text_file(path, workspace_text(workspace));
}

} // namespace lyko
