#pragma once
#include "platform.hpp"
#include "ui/source_audit.hpp"
#include <array>

namespace lyko {
class SourceAuditView {
public:
  std::optional<AuditAction> draw(const Json &report, const std::string &root,
                                  std::uint64_t generation) {
    if (generation_ != generation) {
      generation_ = generation;
      query_.fill(0);
      status_ = 0;
      selected_.reset();
    }
    std::optional<AuditAction> action;
    rendered_rows_ = 0;
    ImGui::PushID("source-audit");
    ImGui::SeparatorText("Source audit");
    ImGui::TextWrapped("Observed sources under: %s", root.c_str());
    ImGui::TextWrapped(
        "This report records a past observation. Actions validate "
        "the current catalogue and source again.");
    if (ImGui::InputTextWithHint("##audit-search",
                                 "Search paths, IDs or diagnostics...",
                                 query_.data(), query_.size())) {
      selected_.reset();
    }
    search_center_ = item_center();
    if (ImGui::Combo("Status", &status_, "All\0Valid\0Invalid\0Skipped\0")) {
      selected_.reset();
    }
    constexpr std::array<const char *, 4> statuses{"", "valid", "invalid",
                                                   "skipped"};
    std::vector<const Json *> filtered;
    const Json *selected = nullptr;
    for (const auto &row : report.at("entries").as<Json::Array>()) {
      if (selected_ && row.at("id").as<std::uint64_t>() == *selected_) {
        selected = &row;
      }
      if (audit_row_matches(row, query_.data(),
                            statuses[static_cast<std::size_t>(status_)])) {
        filtered.push_back(&row);
      }
    }
    ImGui::Text("%llu of %llu entries",
                static_cast<unsigned long long>(filtered.size()),
                static_cast<unsigned long long>(
                    report.at("entries").as<Json::Array>().size()));
    const float scale = ImGui::GetFontSize() / 15.f;
    if (ImGui::BeginTable(
            "report", 4,
            ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
                ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH,
            ImVec2(0, 180 * scale),
            std::max(ImGui::GetContentRegionAvail().x, 650 * scale))) {
      ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed,
                              55 * scale);
      ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthFixed,
                              70 * scale);
      ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed,
                              80 * scale);
      ImGui::TableSetupScrollFreeze(0, 1);
      ImGui::TableHeadersRow();
      ImGuiListClipper clipper;
      clipper.Begin(static_cast<int>(filtered.size()));
      while (clipper.Step()) {
        for (int index = clipper.DisplayStart; index < clipper.DisplayEnd;
             ++index) {
          ++rendered_rows_;
          const auto &row = *filtered[static_cast<std::size_t>(index)];
          const auto id = row.at("id").as<std::uint64_t>();
          ImGui::PushID(std::to_string(id).c_str());
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          if (ImGui::Selectable(std::to_string(id).c_str(), selected_ == id,
                                ImGuiSelectableFlags_SpanAllColumns)) {
            selected_ = id;
            selected = &row;
          }
          row_center_ = item_center();
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(row.at("path").as<std::string>().c_str());
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", row.at("path").as<std::string>().c_str());
          }
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(row.at("kind").as<std::string>().c_str());
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(row.at("status").as<std::string>().c_str());
          ImGui::PopID();
        }
      }
      ImGui::EndTable();
    }
    if (selected) {
      ImGui::SeparatorText("Selected source");
      ImGui::TextWrapped("%s", selected->at("path").as<std::string>().c_str());
      if (selected->contains("error")) {
        const auto &error = selected->at("error");
        ImGui::TextWrapped("%s / %s: %s",
                           error.at("code").as<std::string>().c_str(),
                           error.at("field").as<std::string>().c_str(),
                           error.at("message").as<std::string>().c_str());
      } else if (selected->at("status").as<std::string>() == "skipped") {
        ImGui::TextWrapped("No content validator is available for this kind.");
      } else {
        ImGui::Text("%llu bytes / %llu entities",
                    static_cast<unsigned long long>(
                        selected->at("bytes").as<std::uint64_t>()),
                    static_cast<unsigned long long>(
                        selected->at("entities").as<std::uint64_t>()));
        std::array<char, 65> digest{};
        const auto &hash = selected->at("sha256").as<std::string>();
        std::copy_n(hash.begin(), std::min<std::size_t>(64, hash.size()),
                    digest.begin());
        ImGui::InputText("SHA-256", digest.data(), digest.size(),
                         ImGuiInputTextFlags_ReadOnly);
      }
      const auto validate = audit_row_action(*selected, root, false);
      ImGui::BeginDisabled(!validate);
      if (ImGui::Button("Validate current source...")) {
        action = validate;
      }
      validate_center_ = item_center();
      ImGui::EndDisabled();
      const auto instantiate = audit_row_action(*selected, root, true);
      ImGui::BeginDisabled(!instantiate);
      if (ImGui::Button("Instantiate audited bytes...")) {
        action = instantiate;
      }
      instantiate_center_ = item_center();
      ImGui::EndDisabled();
    } else {
      ImGui::TextDisabled("Select an entry to inspect its details.");
    }
    ImGui::PopID();
    return action;
  }

  unsigned test_rendered_rows() const { return rendered_rows_; }
  ImVec2 test_search_center() const { return search_center_; }
  ImVec2 test_row_center() const { return row_center_; }
  ImVec2 test_validate_center() const { return validate_center_; }
  ImVec2 test_instantiate_center() const { return instantiate_center_; }
  std::optional<std::uint64_t> test_selected() const { return selected_; }

private:
  static ImVec2 item_center() {
    const auto a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    return ImVec2((a.x + b.x) * .5f, (a.y + b.y) * .5f);
  }
  unsigned rendered_rows_ = 0;
  ImVec2 search_center_{}, row_center_{}, validate_center_{},
      instantiate_center_{};
  std::array<char, 256> query_{};
  int status_ = 0;
  std::optional<std::uint64_t> generation_, selected_;
};
} // namespace lyko
