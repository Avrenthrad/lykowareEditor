#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>
namespace lyko {
struct DockNode {
  // Empty axis denotes a tab group; split children are ordered left/right or
  // top/bottom.
  std::string axis;
  std::uint64_t ratio = 0; // First-child share in basis points.
  std::string first, second;
  std::vector<std::string> panels;
  std::string active;
  bool operator==(const DockNode &) const = default;
};
struct DockLayout {
  std::string root;
  std::map<std::string, DockNode> nodes;
  std::vector<std::string> hidden;
  bool operator==(const DockLayout &) const = default;
};
inline bool valid_dock_id(const std::string &id) {
  if (id.empty() || id.size() > 64) {
    return false;
  }
  for (char c : id) {
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '_' || c == '-')) {
      return false;
    }
  }
  return true;
}
inline std::optional<std::string> dock_layout_error(const DockLayout &layout) {
  if (layout.nodes.empty() || layout.nodes.size() > 63 ||
      !layout.nodes.contains(layout.root)) {
    return "Layout needs a root and 1 to 63 nodes";
  }
  const std::set<std::string> known{"scene", "inspector", "viewport", "console",
                                    "history"};
  std::set<std::string> seen_nodes, seen_panels;
  const auto panel = [&](const std::string &name) {
    return known.contains(name) && seen_panels.insert(name).second;
  };
  for (const auto &name : layout.hidden) {
    if (!panel(name)) {
      return "Hidden panels must be registered and unique";
    }
  }
  std::function<std::optional<std::string>(const std::string &, unsigned)>
      visit;
  visit = [&](const std::string &id,
              unsigned depth) -> std::optional<std::string> {
    if (depth > 8 || !valid_dock_id(id) || !layout.nodes.contains(id) ||
        !seen_nodes.insert(id).second) {
      return "Layout has an invalid reference, cycle, shared node or excessive "
             "depth";
    }
    const auto &node = layout.nodes.at(id);
    if (node.axis.empty()) {
      if (node.ratio != 0 || !node.first.empty() || !node.second.empty()) {
        return "Tab nodes cannot contain split fields";
      }
      if (node.panels.empty() != node.active.empty()) {
        return "A nonempty tab group needs an active panel";
      }
      bool active = node.panels.empty();
      for (const auto &name : node.panels) {
        if (!panel(name)) {
          return "Panels must be registered and occur exactly once";
        }
        active = active || name == node.active;
      }
      if (!active) {
        return "Active panel must belong to its tab group";
      }
    } else {
      if ((node.axis != "horizontal" && node.axis != "vertical") ||
          node.ratio < 500 || node.ratio > 9500 || !node.panels.empty() ||
          !node.active.empty()) {
        return "Split nodes need a horizontal/vertical axis, 500..9500 ratio "
               "and no tabs";
      }
      if (auto error = visit(node.first, depth + 1)) {
        return error;
      }
      if (auto error = visit(node.second, depth + 1)) {
        return error;
      }
    }
    return {};
  };
  if (auto error = visit(layout.root, 0)) {
    return error;
  }
  if (seen_nodes.size() != layout.nodes.size()) {
    return "Layout contains unreachable nodes";
  }
  if (seen_panels != known) {
    return "Every registered panel must be docked or explicitly hidden";
  }
  return {};
}
// Node identifiers are local structural paths, not durable external handles.
inline DockLayout canonical_dock_layout(const DockLayout &source) {
  DockLayout compact;
  compact.hidden = source.hidden;
  std::function<std::optional<std::string>(const std::string &)> trim;
  trim = [&](const std::string &id) -> std::optional<std::string> {
    auto node = source.nodes.at(id);
    if (node.axis.empty()) {
      if (node.panels.empty()) {
        return {};
      }
      compact.nodes.emplace(id, node);
      return id;
    }
    auto a = trim(node.first), b = trim(node.second);
    if (!a) {
      return b;
    }
    if (!b) {
      return a;
    }
    node.first = *a;
    node.second = *b;
    compact.nodes.emplace(id, node);
    return id;
  };
  auto root = trim(source.root);
  if (!root) {
    compact.root = "root";
    compact.nodes.emplace("root", DockNode{});
  } else {
    compact.root = *root;
  }
  DockLayout result;
  result.root = "root";
  result.hidden = source.hidden;
  std::sort(result.hidden.begin(), result.hidden.end());
  std::function<void(const std::string &, const std::string &)> visit;
  visit = [&](const std::string &old_id, const std::string &id) {
    auto node = compact.nodes.at(old_id);
    if (!node.axis.empty()) {
      auto a = node.first, b = node.second;
      node.first = id + "-0";
      node.second = id + "-1";
      visit(a, node.first);
      visit(b, node.second);
    }
    result.nodes.emplace(id, std::move(node));
  };
  visit(compact.root, result.root);
  return result;
}
inline DockLayout
starter_dock_layout(const std::map<std::string, std::string> &panels,
                    const std::map<std::string, std::uint64_t> &splits) {
  DockLayout layout;
  layout.root = "root";
  layout.nodes.emplace(
      "root",
      DockNode{"horizontal", splits.at("left") * 100, "left", "body", {}, ""});
  layout.nodes.emplace("body", DockNode{"horizontal",
                                        10000 - (splits.at("right") * 10000 /
                                                 (100 - splits.at("left"))),
                                        "middle",
                                        "right",
                                        {},
                                        ""});
  layout.nodes.emplace("middle", DockNode{"vertical",
                                          (100 - splits.at("bottom")) * 100,
                                          "center",
                                          "bottom",
                                          {},
                                          ""});
  for (auto name : {"left", "right", "center", "bottom"}) {
    layout.nodes.emplace(name, DockNode{});
  }
  for (const auto &[panel, dock] : panels) {
    if (dock == "hidden") {
      layout.hidden.push_back(panel);
    } else {
      auto &node = layout.nodes.at(dock);
      node.panels.push_back(panel);
      if (node.active.empty()) {
        node.active = panel;
      }
    }
  }
  return canonical_dock_layout(layout);
}
} // namespace lyko
