#pragma once
#include "assets.hpp"
#include "platform.hpp"
#include <array>
namespace lyko {
struct AssetAction {
  std::string command;
  Json::Object arguments;
};
class AssetBrowser {
public:
  bool open = false;
  std::optional<AssetAction> draw(const Json &catalog) {
    if (!open) {
      return {};
    }
    std::optional<AssetAction> action;
    rendered_rows_ = 0;
    ImGui::SetNextWindowSize(ImVec2(780, 500), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Asset catalogue", &open, ImGuiWindowFlags_NoDocking)) {
      ImGui::TextWrapped(
          "Project-relative metadata. Registering does not import a file; "
          "relocating or forgetting does not move or delete files.");
      if (ImGui::Button("Register asset...")) {
        action = AssetAction{"asset.register", {}};
      }
      ImGui::SameLine();
      if (ImGui::Button("Load catalogue...")) {
        action = AssetAction{"asset.catalog.load", {}};
      }
      ImGui::SameLine();
      if (ImGui::Button("Save catalogue...")) {
        action = AssetAction{"asset.catalog.save", {}};
      }
      if (ImGui::Button("Audit sources...")) {
        action = AssetAction{"asset.audit", {}};
      }
      ImGui::InputTextWithHint("##filter", "Search paths or kinds...",
                               query_.data(), query_.size());
      const auto query = asset_fold(query_.data());
      const float scale = ImGui::GetFontSize() / 15.f;
      if (ImGui::BeginTable(
              "assets", 4,
              ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                  ImGuiTableFlags_ScrollX | ImGuiTableFlags_BordersInnerH,
              ImVec2(0, 0),
              std::max(ImGui::GetContentRegionAvail().x, 720.f * scale))) {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed,
                                60 * scale);
        ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthFixed,
                                70 * scale);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed,
                                230 * scale);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        std::vector<const Json *> filtered;
        for (const auto &item : catalog.at("assets").as<Json::Array>()) {
          if (asset_fold(item.at("path").as<std::string>() + " " +
                         item.at("kind").as<std::string>())
                  .find(query) != std::string::npos) {
            filtered.push_back(&item);
          }
        }
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(filtered.size()));
        while (clipper.Step()) {
          for (int index = clipper.DisplayStart; index < clipper.DisplayEnd;
               ++index) {
            const auto &item = *filtered[static_cast<std::size_t>(index)];
            ++rendered_rows_;
            const auto id = item.at("id").as<std::uint64_t>();
            const auto &path = item.at("path").as<std::string>();
            const auto &kind = item.at("kind").as<std::string>();
            ImGui::PushID(std::to_string(id).c_str());
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%llu", static_cast<unsigned long long>(id));
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(path.c_str());
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip("%s", path.c_str());
            }
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(kind.c_str());
            ImGui::TableNextColumn();
            ImGui::BeginDisabled(kind != "prefab");
            if (ImGui::SmallButton("Instantiate")) {
              action = AssetAction{"asset.instantiate", {{"id", id}}};
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            const bool can_validate = kind == "scene" || kind == "prefab";
            ImGui::BeginDisabled(!can_validate);
            if (ImGui::SmallButton("Validate")) {
              action = AssetAction{"asset.validate", {{"id", id}}};
            }
            ImGui::EndDisabled();
            if (!can_validate &&
                ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
              ImGui::SetTooltip(
                  "Content validation is available for scenes and prefabs.");
            }
            if (ImGui::SmallButton("Relocate")) {
              action =
                  AssetAction{"asset.relocate", {{"id", id}, {"path", path}}};
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Forget")) {
              action = AssetAction{"asset.forget", {{"id", id}}};
            }
            forget_center_ = ImVec2(
                (ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * .5f,
                (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * .5f);
            ImGui::PopID();
          }
        }
        ImGui::EndTable();
      }
    }
    ImGui::End();
    return action;
  }

  unsigned test_rows() const { return rendered_rows_; }
  ImVec2 test_forget_center() const { return forget_center_; }

private:
  unsigned rendered_rows_ = 0;
  ImVec2 forget_center_{};
  std::array<char, 256> query_{};
};
} // namespace lyko
