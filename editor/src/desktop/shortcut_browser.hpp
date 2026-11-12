#pragma once
#include "platform.hpp"
#include "ui/command_hints.hpp"
#include <array>
#include <optional>

namespace lyko {
inline void command_tooltip(const CommandHints &hints,
                            const std::string &name) {
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
    ImGui::SetTooltip("%s", hints.at(name).tooltip().c_str());
  }
}
class ShortcutBrowser {
public:
  bool open = false;
  std::optional<std::string> draw(const CommandHints &hints) {
    if (!open) {
      return {};
    }
    std::optional<std::string> edit;
    ImGui::SetNextWindowSize(ImVec2(760, 480), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Keyboard shortcuts", &open, ImGuiWindowFlags_NoDocking)) {
      ImGui::TextWrapped("Browse every command, including unbound actions. "
                         "Edit opens a binding draft; leave Shortcut empty to "
                         "unbind. Changes use undo/redo.");
      ImGui::InputTextWithHint("##shortcut_filter",
                               "Search commands, descriptions or shortcuts...",
                               filter_.data(), filter_.size());
      search_center_ = center();
      const auto results = hints.search(filter_.data());
      ImGui::TextDisabled("%zu %s", results.size(),
                          results.size() == 1 ? "command" : "commands");
      if (ImGui::BeginTable("bindings", 3,
                            ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                                ImGuiTableFlags_BordersInnerH)) {
        ImGui::TableSetupColumn("Command", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Shortcut", ImGuiTableColumnFlags_WidthFixed,
                                150);
        ImGui::TableSetupColumn("Binding", ImGuiTableColumnFlags_WidthFixed,
                                70);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();
        for (const auto &hint : results) {
          ImGui::PushID(hint.name.c_str());
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(hint.name.c_str());
          command_tooltip(hints, hint.name);
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(hint.shortcut.empty() ? "Unbound"
                                                       : hint.shortcut.c_str());
          ImGui::TableNextColumn();
          if (ImGui::SmallButton("Edit")) {
            edit = hint.name;
          }
          edit_centers_[hint.name] = center();
          ImGui::PopID();
        }
        ImGui::EndTable();
      }
    }
    ImGui::End();
    return edit;
  }
  ImVec2 test_search_center() const { return search_center_; }
  ImVec2 test_edit_center(const std::string &name) const {
    return edit_centers_.at(name);
  }

private:
  static ImVec2 center() {
    return ImVec2(
        (ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * 0.5f,
        (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f);
  }
  std::array<char, 256> filter_{};
  ImVec2 search_center_{};
  std::map<std::string, ImVec2> edit_centers_;
};
} // namespace lyko
