#pragma once
#include "command_form.hpp"
#include "ui/transaction_form.hpp"
#include <memory>

namespace lyko {
class DesktopTransactionForm {
public:
  explicit DesktopTransactionForm(const Json &catalog) : model_(catalog) {}
  std::optional<Json> draw() {
    ImGui::TextWrapped("Build one atomic transaction. Rows run in order; a "
                       "failure rolls back the entire batch.");
    ImGui::BeginDisabled(rows_.size() >= TransactionForm::max_rows);
    if (ImGui::BeginCombo("Add command", "Choose mutation...")) {
      for (const auto &[name, descriptor] : model_.choices()) {
        if (ImGui::Selectable(name.c_str())) {
          rows_.push_back(
              Row{next_id_++, name,
                  std::make_unique<DesktopCommandForm>(descriptor)});
        }
        choices_centers_[name] = center();
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip(
              "%s", descriptor.at("description").as<std::string>().c_str());
        }
      }
      ImGui::EndCombo();
    }
    add_center_ = center();
    ImGui::EndDisabled();
    std::optional<std::size_t> remove, move_up, move_down;
    Json::Array commands;
    bool valid = true;
    for (std::size_t i = 0; i < rows_.size(); ++i) {
      auto &row = rows_[i];
      ImGui::PushID(row.id);
      ImGui::Separator();
      ImGui::Text("%zu. %s", i + 1, row.name.c_str());
      ImGui::BeginDisabled(i == 0);
      if (ImGui::Button("Move up")) {
        move_up = i;
      }
      ImGui::EndDisabled();
      ImGui::SameLine();
      ImGui::BeginDisabled(i + 1 == rows_.size());
      if (ImGui::Button("Move down")) {
        move_down = i;
      }
      ImGui::EndDisabled();
      ImGui::SameLine();
      if (ImGui::Button("Remove")) {
        remove = i;
      }
      auto args = row.form->draw(false);
      if (args) {
        commands.emplace_back(
            Json::Object{{"command", row.name}, {"arguments", *args}});
      } else {
        valid = false;
      }
      ImGui::PopID();
    }
    // Apply structure changes after drawing, and never submit a stale row
    // order.
    if (remove) {
      rows_.erase(rows_.begin() + static_cast<std::ptrdiff_t>(*remove));
    } else if (move_up) {
      std::swap(rows_[*move_up], rows_[*move_up - 1]);
    } else if (move_down) {
      std::swap(rows_[*move_down], rows_[*move_down + 1]);
    }
    valid = valid && !remove && !move_up && !move_down;
    std::optional<Json> arguments;
    if (valid) {
      try {
        arguments = model_.arguments(commands);
      } catch (const std::invalid_argument &error) {
        ImGui::TextWrapped("%s", error.what());
      }
    }
    ImGui::TextDisabled("%zu / %zu commands", rows_.size(),
                        TransactionForm::max_rows);
    ImGui::BeginDisabled(!arguments);
    const bool submit = ImGui::Button("Execute transaction");
    execute_center_ = center();
    ImGui::EndDisabled();
    arguments_ = arguments;
    return submit ? arguments : std::nullopt;
  }

  const std::optional<Json> &draft_arguments() const { return arguments_; }
  ImVec2 test_add_center() const { return add_center_; }
  ImVec2 test_choice_center(const std::string &name) const {
    return choices_centers_.at(name);
  }
  ImVec2 test_execute_center() const { return execute_center_; }

private:
  static ImVec2 center() {
    return ImVec2(
        (ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * 0.5f,
        (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f);
  }
  int next_id_ = 0;
  ImVec2 add_center_{}, execute_center_{};
  std::map<std::string, ImVec2> choices_centers_;
  struct Row {
    int id;
    std::string name;
    std::unique_ptr<DesktopCommandForm> form;
  };
  std::optional<Json> arguments_;
  TransactionForm model_;
  std::vector<Row> rows_;
};
} // namespace lyko
