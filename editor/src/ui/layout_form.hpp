#pragma once
#include "dock_layout_json.hpp"
#include "workspace.hpp"

namespace lyko {
// A private, always-valid draft. Candidate edits validate before replacement.
class LayoutForm {
public:
  LayoutForm() : layout_(effective_dock_layout(Workspace{})) {}
  explicit LayoutForm(const Json &value) : layout_(parse_dock_layout(value)) {}
  const DockLayout &layout() const { return layout_; }
  Json value() const { return dock_layout_json(layout_); }
  void import(const Json &value) { layout_ = parse_dock_layout(value); }
  void resize(const std::string &id, const std::string &axis,
              std::uint64_t ratio) {
    auto next = layout_;
    auto &node = split(next, id);
    node.axis = axis;
    node.ratio = ratio;
    commit(std::move(next));
  }
  void activate(const std::string &id, const std::string &panel) {
    auto next = layout_;
    tabs(next, id).active = panel;
    commit(std::move(next));
  }
  void reorder(const std::string &id, std::size_t index, bool earlier) {
    auto next = layout_;
    auto &panels = tabs(next, id).panels;
    if (index >= panels.size() || (earlier && index == 0) ||
        (!earlier && index + 1 >= panels.size())) {
      throw std::invalid_argument("Choose a neighboring tab.");
    }
    std::swap(panels[index], panels[earlier ? index - 1 : index + 1]);
    commit(std::move(next));
  }
  void move(const std::string &panel, const std::string &target) {
    auto next = layout_;
    if (!target.empty()) {
      const auto &node = tabs(next, target);
      if (std::find(node.panels.begin(), node.panels.end(), panel) !=
          node.panels.end()) {
        return;
      }
    } else if (std::find(next.hidden.begin(), next.hidden.end(), panel) !=
               next.hidden.end()) {
      return;
    }
    detach(next, panel);
    if (target.empty()) {
      next.hidden.push_back(panel);
    } else {
      auto &node = tabs(next, target);
      node.panels.push_back(panel);
      node.active = panel;
    }
    commit(std::move(next));
  }
  void divide(const std::string &panel, const std::string &target,
              const std::string &axis, bool first) {
    auto next = layout_;
    (void)tabs(next, target);
    detach(next, panel);
    auto old = tabs(next, target);
    if (old.panels.empty()) {
      throw std::invalid_argument(
          "A split needs another panel in its target group.");
    }
    const auto a = fresh(next);
    next.nodes.emplace(a, DockNode{});
    const auto b = fresh(next);
    DockNode single;
    single.panels = {panel};
    single.active = panel;
    next.nodes.at(a) = first ? single : old;
    next.nodes.emplace(b, first ? old : single);
    next.nodes.at(target) = DockNode{axis, 5000, a, b, {}, ""};
    commit(std::move(next));
  }

private:
  static DockNode &tabs(DockLayout &layout, const std::string &id) {
    auto it = layout.nodes.find(id);
    if (it == layout.nodes.end() || !it->second.axis.empty()) {
      throw std::invalid_argument("Choose an existing tab group.");
    }
    return it->second;
  }
  static DockNode &split(DockLayout &layout, const std::string &id) {
    auto it = layout.nodes.find(id);
    if (it == layout.nodes.end() || it->second.axis.empty()) {
      throw std::invalid_argument("Choose an existing split.");
    }
    return it->second;
  }
  static std::string fresh(const DockLayout &layout) {
    for (unsigned i = 0; i < 65; ++i) {
      auto id = "draft-" + std::to_string(i);
      if (!layout.nodes.contains(id)) {
        return id;
      }
    }
    throw std::invalid_argument("Too many layout nodes.");
  }
  static void detach(DockLayout &layout, const std::string &panel) {
    bool found = false;
    auto hidden = std::find(layout.hidden.begin(), layout.hidden.end(), panel);
    if (hidden != layout.hidden.end()) {
      layout.hidden.erase(hidden);
      found = true;
    }
    for (auto &[id, node] : layout.nodes) {
      (void)id;
      auto it = std::find(node.panels.begin(), node.panels.end(), panel);
      if (it == node.panels.end()) {
        continue;
      }
      node.panels.erase(it);
      found = true;
      if (node.active == panel) {
        node.active = node.panels.empty() ? "" : node.panels.front();
      }
    }
    if (!found) {
      throw std::invalid_argument("Choose a registered panel.");
    }
  }
  void commit(DockLayout next) {
    if (auto error = dock_layout_error(next)) {
      throw std::invalid_argument(*error);
    }
    layout_ = canonical_dock_layout(next);
  }
  DockLayout layout_;
};
} // namespace lyko
