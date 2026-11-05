#pragma once
#include "platform.hpp"
#include "ui/layout_form.hpp"
#include <array>
#include <cmath>
#include <functional>

namespace lyko {
class DesktopLayoutForm {
public:
  DesktopLayoutForm(const Json &spec, const Json &value)
      : panels_(spec.at("properties")
                    .at("hidden")
                    .at("items")
                    .at("enum")
                    .as<Json::Array>()) {
    if (!value.as<Json::Object>().empty()) {
      draft_.import(value);
    }
    sync_json();
  }
  Json value() const { return draft_.value(); }
  bool valid() const { return !dirty_ && error_.empty(); }
  void draw() {
    ImGui::TextWrapped("Layout draft. Edit groups or panel placement, then "
                       "Execute to apply the whole layout.");
    std::function<void()> edit;
    ImGui::BeginDisabled(dirty_);
    const auto &layout = draft_.layout();
    const bool show_tree = ImGui::TreeNodeEx("Split sizes and tabs",
                                             ImGuiTreeNodeFlags_DefaultOpen);
    tree_center_ = center();
    if (show_tree) {
      draw_node(layout.root, edit);
      ImGui::TreePop();
    }
    if (ImGui::TreeNode("Panel placement")) {
      for (const auto &item : panels_) {
        const auto panel = item.as<std::string>();
        std::string current;
        for (const auto &[id, node] : layout.nodes) {
          if (std::find(node.panels.begin(), node.panels.end(), panel) !=
              node.panels.end()) {
            current = id;
          }
        }
        ImGui::PushID(panel.c_str());
        if (ImGui::BeginCombo(panel.c_str(), current.empty()
                                                 ? "Hidden"
                                                 : group(current).c_str())) {
          if (ImGui::Selectable("Hidden", current.empty())) {
            edit = [this, panel] { draft_.move(panel, ""); };
          }
          for (const auto &[id, node] : layout.nodes) {
            if (!node.axis.empty()) {
              continue;
            }
            ImGui::PushID(id.c_str());
            if (ImGui::Selectable(group(id).c_str(), id == current)) {
              edit = [this, panel, id] { draft_.move(panel, id); };
            }
            ImGui::PopID();
          }
          ImGui::EndCombo();
        }
        ImGui::PopID();
      }
      ImGui::TreePop();
    }
    if (ImGui::TreeNode("Create split")) {
      if (ImGui::BeginCombo("Panel", split_panel_.c_str())) {
        for (const auto &item : panels_) {
          const auto panel = item.as<std::string>();
          if (ImGui::Selectable(panel.c_str(), panel == split_panel_)) {
            split_panel_ = panel;
          }
        }
        ImGui::EndCombo();
      }
      if (!layout.nodes.contains(split_target_) ||
          !layout.nodes.at(split_target_).axis.empty()) {
        for (const auto &[id, node] : layout.nodes) {
          if (node.axis.empty()) {
            split_target_ = id;
            break;
          }
        }
      }
      if (ImGui::BeginCombo("Beside group", group(split_target_).c_str())) {
        for (const auto &[id, node] : layout.nodes) {
          if (!node.axis.empty()) {
            continue;
          }
          ImGui::PushID(id.c_str());
          if (ImGui::Selectable(group(id).c_str(), id == split_target_)) {
            split_target_ = id;
          }
          ImGui::PopID();
        }
        ImGui::EndCombo();
      }
      ImGui::Checkbox("Vertical (top / bottom)", &vertical_);
      ImGui::Checkbox("Place panel first", &first_);
      if (ImGui::Button("Split draft group")) {
        edit = [this] {
          draft_.divide(split_panel_, split_target_,
                        vertical_ ? "vertical" : "horizontal", first_);
        };
      }
      ImGui::TreePop();
    }
    ImGui::EndDisabled();
    if (edit) {
      try {
        edit();
        error_.clear();
        sync_json();
      } catch (const std::invalid_argument &error) {
        error_ = error.what();
      }
    }
    const bool show_json = ImGui::TreeNode("Advanced JSON import");
    json_toggle_center_ = center();
    if (show_json) {
      if (ImGui::InputTextMultiline("##json", json_.data(), json_.size(),
                                    ImVec2(-1, 120))) {
        dirty_ = true;
        error_.clear();
      }
      json_center_ = center();
      if (ImGui::Button("Import into draft")) {
        try {
          if (std::char_traits<char>::length(json_.data()) >=
              json_.size() - 1) {
            throw std::invalid_argument("JSON input limit reached.");
          }
          draft_.import(Json::parse(json_.data()));
          dirty_ = false;
          error_.clear();
          sync_json();
        } catch (const std::invalid_argument &error) {
          error_ = error.what();
          dirty_ = true;
        }
      }
      import_center_ = center();
      ImGui::SameLine();
      if (ImGui::Button("Discard JSON edits")) {
        dirty_ = false;
        error_.clear();
        sync_json();
      }
      discard_center_ = center();
      ImGui::TreePop();
    }
    if (dirty_) {
      ImGui::TextWrapped("Import or discard the JSON edits before executing.");
    }
    if (!error_.empty()) {
      ImGui::TextWrapped("%s", error_.c_str());
      if (!dirty_ && ImGui::SmallButton("Keep valid draft")) {
        error_.clear();
      }
    }
  }
  ImVec2 test_tree_center() const { return tree_center_; }
  ImVec2 test_json_toggle_center() const { return json_toggle_center_; }
  ImVec2 test_json_center() const { return json_center_; }
  ImVec2 test_import_center() const { return import_center_; }
  ImVec2 test_discard_center() const { return discard_center_; }
  ImVec2 test_ratio_center(const std::string &id) const {
    return ratio_centers_.at(id);
  }

private:
  static ImVec2 center() {
    return ImVec2(
        (ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * 0.5f,
        (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f);
  }
  std::string group(const std::string &id) const {
    const auto &panels = draft_.layout().nodes.at(id).panels;
    std::string text;
    for (const auto &panel : panels) {
      if (!text.empty()) {
        text += " + ";
      }
      text += panel;
    }
    return text.empty() ? "Empty group" : text;
  }
  void draw_node(const std::string &id, std::function<void()> &edit) {
    const auto &node = draft_.layout().nodes.at(id);
    ImGui::PushID(id.c_str());
    if (!node.axis.empty()) {
      const bool expanded = ImGui::TreeNodeEx(
          "split",
          id == draft_.layout().root ? ImGuiTreeNodeFlags_DefaultOpen
                                     : ImGuiTreeNodeFlags_None,
          "%s",
          node.axis == "horizontal" ? "Left / right split"
                                    : "Top / bottom split");
      if (expanded) {
        bool vertical = node.axis == "vertical";
        if (ImGui::Checkbox("Vertical", &vertical)) {
          edit = [this, id, vertical, ratio = node.ratio] {
            draft_.resize(id, vertical ? "vertical" : "horizontal", ratio);
          };
        }
        double percent = static_cast<double>(node.ratio) / 100;
        if (ImGui::InputDouble("First share (%)", &percent, 1, 5, "%.2f")) {
          edit = [this, id, axis = node.axis, percent] {
            if (!std::isfinite(percent) || percent < 5 || percent > 95) {
              throw std::invalid_argument(
                  "Split share must be 5 to 95 percent.");
            }
            draft_.resize(
                id, axis,
                static_cast<std::uint64_t>(std::llround(percent * 100)));
          };
        }
        ratio_centers_[id] = center();
        draw_node(node.first, edit);
        draw_node(node.second, edit);
        ImGui::TreePop();
      }
    } else {
      if (ImGui::TreeNodeEx("tabs", ImGuiTreeNodeFlags_None, "%s",
                            group(id).c_str())) {
        if (ImGui::BeginCombo("Active tab", node.active.c_str())) {
          for (const auto &panel : node.panels) {
            if (ImGui::Selectable(panel.c_str(), panel == node.active)) {
              edit = [this, id, panel] { draft_.activate(id, panel); };
            }
          }
          ImGui::EndCombo();
        }
        for (std::size_t i = 0; i < node.panels.size(); ++i) {
          ImGui::PushID(static_cast<int>(i));
          ImGui::TextUnformatted(node.panels[i].c_str());
          ImGui::SameLine();
          ImGui::BeginDisabled(i == 0);
          if (ImGui::SmallButton("Earlier")) {
            edit = [this, id, i] { draft_.reorder(id, i, true); };
          }
          ImGui::EndDisabled();
          ImGui::SameLine();
          ImGui::BeginDisabled(i + 1 == node.panels.size());
          if (ImGui::SmallButton("Later")) {
            edit = [this, id, i] { draft_.reorder(id, i, false); };
          }
          ImGui::EndDisabled();
          ImGui::PopID();
        }
        ImGui::TreePop();
      }
    }
    ImGui::PopID();
  }
  void sync_json() {
    const auto text = draft_.value().dump();
    if (text.size() >= json_.size()) {
      throw std::invalid_argument("Layout draft exceeds input limit.");
    }
    json_.fill(0);
    std::copy(text.begin(), text.end(), json_.begin());
  }
  ImVec2 tree_center_{}, json_toggle_center_{}, json_center_{},
      import_center_{}, discard_center_{};
  LayoutForm draft_;
  Json::Array panels_;
  std::array<char, 16384> json_{};
  bool dirty_ = false, vertical_ = false, first_ = true;
  std::string error_, split_panel_ = "scene", split_target_;
  std::map<std::string, ImVec2> ratio_centers_;
};
} // namespace lyko
