#include "docking.hpp"
#include "imgui_internal.h"
#include <algorithm>
#include <cmath>
#include <functional>

namespace lyko {
void DockingAdapter::apply(const DockLayout &layout) {
  active_targets_.clear();
  ImGui::DockBuilderRemoveNode(root_);
  ImGui::DockBuilderAddNode(root_, ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(root_, ImGui::GetMainViewport()->WorkSize);
  std::function<void(const std::string &, ImGuiID)> build;
  build = [&](const std::string &id, ImGuiID runtime_id) {
    const auto &node = layout.nodes.at(id);
    if (!node.axis.empty()) {
      ImGuiID a = 0, b = 0;
      ImGui::DockBuilderSplitNode(
          runtime_id, node.axis == "horizontal" ? ImGuiDir_Left : ImGuiDir_Up,
          static_cast<float>(node.ratio) / 10000.f, &a, &b);
      build(node.first, a);
      build(node.second, b);
    } else {
      short order = 0;
      for (const auto &panel : node.panels) {
        const auto &title = dock_panel_titles.at(panel);
        ImGui::DockBuilderDockWindow(title.c_str(), runtime_id);
        if (auto *window = ImGui::FindWindowByName(title.c_str())) {
          window->DockOrder = order++;
        }
      }
      if (!node.active.empty()) {
        if (node.panels.size() > 1) {
          active_targets_.push_back(dock_panel_titles.at(node.active));
        }
        const auto &title = dock_panel_titles.at(node.active);
        const auto tab = ImHashStr("#TAB", 0, ImHashStr(title.c_str()));
        if (auto *runtime = ImGui::DockBuilderGetNode(runtime_id)) {
          runtime->SelectedTabId = tab;
        }
      }
    }
  };
  build(layout.root, root_);
  ImGui::DockBuilderFinish(root_);
}
DockLayout DockingAdapter::capture() const {
  DockLayout result;
  result.root = "root";
  result.hidden = confirmed_.hidden;
  const auto occupied = [](ImGuiDockNode *node) {
    std::function<bool(ImGuiDockNode *)> any;
    any = [&](ImGuiDockNode *n) {
      return n && (n->Windows.Size > 0 || any(n->ChildNodes[0]) ||
                   any(n->ChildNodes[1]));
    };
    return any(node);
  };
  std::function<void(ImGuiDockNode *, const std::string &, unsigned)> visit;
  visit = [&](ImGuiDockNode *runtime, const std::string &id, unsigned depth) {
    if (!runtime || depth > 8) {
      throw std::invalid_argument("Unsupported native dock tree");
    }
    DockNode node;
    if (runtime->IsSplitNode()) {
      auto *a = runtime->ChildNodes[0];
      auto *b = runtime->ChildNodes[1];
      if (!a || !b) {
        throw std::invalid_argument("Incomplete native split");
      }
      node.axis = runtime->SplitAxis == ImGuiAxis_X ? "horizontal" : "vertical";
      const float x = runtime->SplitAxis == ImGuiAxis_X ? a->Size.x : a->Size.y;
      const float y = runtime->SplitAxis == ImGuiAxis_X ? b->Size.x : b->Size.y;
      if (!occupied(a) || !occupied(b)) {
        node.ratio = 5000;
      } else {
        if (x <= 0 || y <= 0 || !std::isfinite(x) || !std::isfinite(y) ||
            !std::isfinite(x + y)) {
          throw std::invalid_argument("Invalid native split dimensions");
        }
        node.ratio =
            static_cast<std::uint64_t>(std::ceil(10000.f * x / (x + y)));
      }
      node.first = id + "-0";
      node.second = id + "-1";
      visit(a, node.first, depth + 1);
      visit(b, node.second, depth + 1);
    } else {
      std::vector<ImGuiWindow *> windows(runtime->Windows.begin(),
                                         runtime->Windows.end());
      std::sort(windows.begin(), windows.end(),
                [](const auto *a, const auto *b) {
                  return a->DockOrder < b->DockOrder;
                });
      for (auto *window : windows) {
        auto found = std::find_if(
            dock_panel_titles.begin(), dock_panel_titles.end(),
            [&](const auto &item) { return item.second == window->Name; });
        if (found == dock_panel_titles.end()) {
          throw std::invalid_argument("Unknown window in dock layout");
        }
        node.panels.push_back(found->first);
        if (window->TabId == runtime->SelectedTabId) {
          node.active = found->first;
        }
      }
      if (node.active.empty() && !node.panels.empty()) {
        node.active = node.panels.front();
      }
    }
    result.nodes.emplace(id, std::move(node));
  };
  visit(ImGui::DockBuilderGetNode(root_), result.root, 0);
  if (auto error = dock_layout_error(result)) {
    throw std::invalid_argument(*error);
  }
  return canonical_dock_layout(result);
}
void DockingAdapter::sync(ImGuiID root, const Json &json,
                          std::uint64_t revision) {
  root_ = root;
  const auto key = json.dump();
  if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END;
         ++key) {
      if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(key), false)) {
        gesture_revision_ = revision;
      }
    }
  }
  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    gesture_revision_ = revision;
  }
  if (key != model_key_) {
    confirmed_ = parse_dock_layout(json);
    model_key_ = key;
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
      conflict_ = true;
    } else {
      restore_ = true;
    }
  }
  const auto size = ImGui::GetMainViewport()->WorkSize;
  if (size.x != viewport_size_.x || size.y != viewport_size_.y) {
    viewport_size_ = size;
    restore_ = true;
  }
  if (restore_ && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    apply(confirmed_);
    restore_ = false;
    conflict_ = false;
    settle_ = 3;
    baseline_.clear();
    capture_delay_ = 0;
  }
}
std::optional<DockProposal> DockingAdapter::finish() {
  if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END;
         ++key) {
      if (ImGui::IsKeyReleased(static_cast<ImGuiKey>(key))) {
        capture_delay_ = 2;
      }
    }
  }
  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    capture_delay_ = 2;
  }
  if (settle_) {
    if (settle_ == 2) {
      for (const auto &title : active_targets_) {
        if (auto *window = ImGui::FindWindowByName(title.c_str())) {
          if (window->DockNode) {
            auto *node = window->DockNode;
            node->SelectedTabId = window->TabId;
            if (node->TabBar) {
              node->TabBar->SelectedTabId = node->TabBar->NextSelectedTabId =
                  window->TabId;
            }
            ImGui::FocusWindow(window);
          }
        }
      }
    }
    --settle_;
    if (!settle_) {
      try {
        baseline_ = dock_layout_json(capture()).dump();
      } catch (const std::exception &) {
        settle_ = 1;
      }
    }
    return {};
  }
  if (!capture_delay_ || ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    return {};
  }
  if (--capture_delay_) {
    return {};
  }
  if (ticket_ || proposal_) {
    return {};
  }
  if (conflict_) {
    rejected("Layout changed during the gesture; confirmed layout restored.");
    return {};
  }
  try {
    auto layout = capture();
    const auto key = dock_layout_json(layout).dump();
    if (key != baseline_) {
      proposal_ = true;
      return DockProposal{std::move(layout), gesture_revision_};
    }
  } catch (const std::exception &error) {
    rejected(
        std::string("Dock layout not accepted: ") + error.what() +
        ". Confirmed layout restored (floating windows are not supported).");
  }
  return {};
}
} // namespace lyko
