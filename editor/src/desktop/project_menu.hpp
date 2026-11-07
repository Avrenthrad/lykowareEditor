#pragma once
#include "platform.hpp"
#include "project_session.hpp"
namespace lyko {
struct ProjectMenuAction {
  std::string command;
  Json::Object arguments;
};
class ProjectMenu {
public:
  std::optional<ProjectMenuAction> draw(const Json &state) {
    std::optional<ProjectMenuAction> action;
    const bool expanded = ImGui::BeginMenu("Project");
    menu_center_ = center();
    if (expanded) {
      if (ImGui::MenuItem("Create project...")) {
        action = ProjectMenuAction{"project.create", {}};
      }
      create_center_ = center();
      if (ImGui::MenuItem("Write new manifest...")) {
        action = ProjectMenuAction{"project.manifest.write", {}};
      }
      if (ImGui::MenuItem("Open project...")) {
        action = ProjectMenuAction{"project.open", {}};
      }
      open_center_ = center();
      if (ImGui::MenuItem("Close project...", nullptr, false,
                          state.at("active") != Json(nullptr))) {
        action = ProjectMenuAction{"project.close", {}};
      }
      if (ImGui::MenuItem("Project settings...", nullptr, false,
                          state.at("active") != Json(nullptr))) {
        action = ProjectMenuAction{
            "project.configure",
            {{"name", state.at("active").at("manifest").at("name")},
             {"directories", Json::Object{}}}};
      }
      if (ImGui::MenuItem("Revert project settings...", nullptr, false,
                          state.at("active") != Json(nullptr))) {
        action = ProjectMenuAction{"project.settings.revert", {}};
      }
      if (ImGui::MenuItem("Document status...")) {
        action = ProjectMenuAction{"document.status", {}};
      }
      if (ImGui::MenuItem("Inspect project...")) {
        action = ProjectMenuAction{"project.query", {}};
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Load recent history...")) {
        action = ProjectMenuAction{"project.recent.load", {}};
      }
      load_center_ = center();
      if (ImGui::MenuItem("Save recent history...")) {
        action = ProjectMenuAction{"project.recent.save", {}};
      }
      save_center_ = center();
      ImGui::TextDisabled("Recent projects");
      for (const auto &item : state.at("recent").as<Json::Array>()) {
        const auto &path = item.at("path").as<std::string>();
        ImGui::PushID(path.c_str());
        if (ImGui::MenuItem(
                item.at("manifest").at("name").as<std::string>().c_str())) {
          action = ProjectMenuAction{"project.open", {{"path", path}}};
        }
        recent_center_ = center();
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s", path.c_str());
        }
        ImGui::PopID();
      }
      if (ImGui::MenuItem("Clear recent projects...", nullptr, false,
                          !state.at("recent").as<Json::Array>().empty())) {
        action = ProjectMenuAction{"project.recent.clear", {}};
      }
      ImGui::Separator();
      if (state.at("active") != Json(nullptr)) {
        ImGui::TextUnformatted(state.at("active")
                                   .at("manifest")
                                   .at("name")
                                   .as<std::string>()
                                   .c_str());
        ImGui::TextUnformatted(
            state.at("active").at("root").as<std::string>().c_str());
      } else {
        ImGui::TextDisabled("No active project");
      }
      ImGui::EndMenu();
    }
    return action;
  }
  ImVec2 test_load_center() const { return load_center_; }
  ImVec2 test_save_center() const { return save_center_; }
  ImVec2 test_create_center() const { return create_center_; }
  ImVec2 test_menu_center() const { return menu_center_; }
  ImVec2 test_open_center() const { return open_center_; }
  ImVec2 test_recent_center() const { return recent_center_; }

private:
  static ImVec2 center() {
    const auto a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    return ImVec2((a.x + b.x) * .5f, (a.y + b.y) * .5f);
  }
  ImVec2 menu_center_{}, create_center_{}, open_center_{}, recent_center_{},
      load_center_{}, save_center_{};
};
} // namespace lyko
