#pragma once
#include "json.hpp"
#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace lyko {
struct CommandHint {
  std::string name, description, shortcut;
  bool needs_arguments = false;
  std::string tooltip() const {
    return name + "\n" + description +
           "\nShortcut: " + (shortcut.empty() ? "Unbound" : shortcut) +
           (needs_arguments && name != "palette.search"
                ? "\nShortcut opens an argument form."
                : "") +
           "\nEdit bindings in Shortcuts.";
  }
};
// Read-only presentation from registry documentation and the confirmed keymap.
class CommandHints {
public:
  CommandHints(const Json &catalog, const Json &keymap) {
    for (const auto &item : catalog.as<Json::Array>()) {
      auto name = item.at("name").as<std::string>();
      const auto binding = keymap.as<Json::Object>().find(name);
      entries_.emplace(
          name, CommandHint{name, item.at("description").as<std::string>(),
                            binding == keymap.as<Json::Object>().end()
                                ? ""
                                : binding->second.as<std::string>(),
                            !item.at("inputSchema")
                                 .at("required")
                                 .as<Json::Array>()
                                 .empty()});
    }
  }
  const CommandHint &at(const std::string &name) const {
    return entries_.at(name);
  }
  std::string shortcut(const std::string &name) const {
    const auto found = entries_.find(name);
    return found == entries_.end() ? "" : found->second.shortcut;
  }
  std::vector<CommandHint> search(const std::string &query) const {
    const auto needle = fold(query);
    std::vector<CommandHint> results;
    for (const auto &[name, hint] : entries_) {
      if (fold(name + " " + hint.description + " " + hint.shortcut)
              .find(needle) != std::string::npos) {
        results.push_back(hint);
      }
    }
    return results;
  }

private:
  static std::string fold(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
      return c >= 'A' && c <= 'Z' ? static_cast<char>(c + ('a' - 'A'))
                                  : static_cast<char>(c);
    });
    return text;
  }
  std::map<std::string, CommandHint> entries_;
};
} // namespace lyko
