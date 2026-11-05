#pragma once
#include "platform.hpp"
#include "ui/form_feedback.hpp"
namespace lyko {
inline void draw_form_feedback(const FormFeedback &feedback) {
  if (feedback.busy()) {
    ImGui::Separator();
    ImGui::TextUnformatted(feedback.ticket() ? "Command pending..."
                                             : "Waiting to submit...");
    ImGui::TextWrapped(
        "Leaving this form does not cancel a submitted command.");
  } else if (feedback.result()) {
    const auto &result = *feedback.result();
    ImGui::SeparatorText("Command result");
    if (const auto *error = feedback.error()) {
      ImGui::TextWrapped("%s", error->at("message").as<std::string>().c_str());
      if (ImGui::TreeNode("Error details")) {
        ImGui::TextWrapped("Code: %s / Field: %s",
                           error->at("code").as<std::string>().c_str(),
                           error->at("field").as<std::string>().c_str());
        if (feedback.ticket()) {
          ImGui::Text("Request %llu",
                      static_cast<unsigned long long>(*feedback.ticket()));
        }
        ImGui::TreePop();
      }
      if (error->at("code").as<std::string>() == "stale_revision") {
        ImGui::TextWrapped("State refresh requested. Review the latest state "
                           "before submitting again.");
      }
    } else {
      ImGui::TextUnformatted("Command completed.");
      if (feedback.preview() != "{}" && ImGui::TreeNode("Result data")) {
        ImGui::TextWrapped("%s", feedback.preview().c_str());
        ImGui::TreePop();
      }
    }
    ImGui::TextDisabled("Revision %llu",
                        static_cast<unsigned long long>(
                            result.at("revision").as<std::uint64_t>()));
  }
}
} // namespace lyko
