#pragma once
#include "platform.hpp"
#include "ui/form_feedback.hpp"
#include <array>
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
      const auto &data = result.at("data");
      if (std::holds_alternative<Json::Object>(data.value) &&
          data.contains("sha256")) {
        std::array<char, 65> digest{};
        const auto &text = data.at("sha256").as<std::string>();
        std::copy_n(text.begin(), std::min<std::size_t>(64, text.size()),
                    digest.begin());
        ImGui::InputText("SHA-256", digest.data(), digest.size(),
                         ImGuiInputTextFlags_ReadOnly);
        if (data.contains("matches")) {
          ImGui::TextUnformatted(
              data.at("matches").as<bool>()
                  ? "Source bytes match the expected version."
                  : "Source bytes have changed.");
        }
      }
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
