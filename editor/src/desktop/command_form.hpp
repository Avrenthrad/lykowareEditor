#pragma once
#include "layout_form.hpp"
#include "platform.hpp"
#include "ui/command_form.hpp"
#include <array>
#include <optional>

namespace lyko {
class DesktopCommandForm {
public:
  explicit DesktopCommandForm(const Json &descriptor,
                              const Json::Object &current = {})
      : descriptor_(descriptor), form_(descriptor.at("inputSchema")) {
    for (const auto &[name, value] : current) {
      form_.set(name, value);
    }
    if (descriptor_.at("name").as<std::string>() == "layout.replace") {
      layout_.emplace(form_.schema().at("properties").at("layout"),
                      form_.value("layout"));
      form_.set("layout", layout_->value());
    }
    for (const auto &[name, spec] :
         form_.schema().at("properties").as<Json::Object>()) {
      const auto type = spec.at("type").as<std::string>();
      if (type == "string" || type == "array" || type == "object") {
        auto text = type == "string" ? form_.value(name).as<std::string>()
                                     : form_.value(name).dump();
        if (text.size() >= buffers_[name].size()) {
          throw std::invalid_argument("Form default exceeds input buffer");
        }
        std::copy(text.begin(), text.end(), buffers_[name].begin());
      }
    }
  }
  // Returns validated arguments only when the user presses Execute.
  std::optional<Json> draw(bool show_execute = true,
                           const Json *command_error = nullptr) {
    using O = Json::Object;
    using A = Json::Array;
    const auto label = [](std::string text) {
      std::replace(text.begin(), text.end(), '_', ' ');
      if (!text.empty() && text.front() >= 'a' && text.front() <= 'z') {
        text.front() = static_cast<char>(text.front() - 'a' + 'A');
      }
      return text;
    };
    ImGui::TextWrapped("%s",
                       descriptor_.at("description").as<std::string>().c_str());
    ImGui::Separator();
    for (const auto &[name, spec] : form_.schema().at("properties").as<O>()) {
      ImGui::PushID(name.c_str());
      auto title =
          spec.contains("title") ? spec.at("title").as<std::string>() : name;
      title = label(std::move(title));
      const auto type = spec.at("type").as<std::string>();
      if (name == "layout" && layout_) {
        layout_->draw();
        form_.set(name, layout_->value());
        if (layout_->valid()) {
          parse_errors_.erase(name);
        } else {
          parse_errors_[name] =
              "Resolve the layout draft error before executing.";
        }
      } else if (spec.contains("enum")) {
        const auto current = form_.value(name).as<std::string>();
        if (ImGui::BeginCombo(title.c_str(), current.empty()
                                                 ? "Choose..."
                                                 : label(current).c_str())) {
          for (const auto &choice : spec.at("enum").as<A>()) {
            const auto &text = choice.as<std::string>();
            if (ImGui::Selectable(label(text).c_str(), text == current)) {
              form_.set(name, text);
            }
          }
          ImGui::EndCombo();
        }
      } else if (type == "boolean") {
        auto checked = form_.value(name).as<bool>();
        if (ImGui::Checkbox(title.c_str(), &checked)) {
          form_.set(name, checked);
        }
      } else if (type == "integer") {
        ImU64 number =
            static_cast<ImU64>(form_.value(name).as<std::uint64_t>());
        if (ImGui::InputScalar(title.c_str(), ImGuiDataType_U64, &number)) {
          form_.set(name, static_cast<std::uint64_t>(number));
        }
      } else if (type == "string") {
        auto &buffer = buffers_.at(name);
        if (ImGui::InputText(title.c_str(), buffer.data(), buffer.size())) {
          form_.set(name, std::string(buffer.data()));
        }
      } else if (type == "array" || type == "object") {
        ImGui::TextUnformatted(title.c_str());
        ImGui::TextDisabled(type == "array" ? "Advanced: JSON array"
                                            : "Advanced: JSON object");
        auto &buffer = buffers_.at(name);
        if (ImGui::InputTextMultiline("##array", buffer.data(), buffer.size(),
                                      ImVec2(-1, 120))) {
          try {
            auto value = Json::parse(buffer.data());
            if (type == "array") {
              (void)value.as<A>();
            } else {
              (void)value.as<O>();
            }
            form_.set(name, std::move(value));
            parse_errors_.erase(name);
          } catch (const std::invalid_argument &) {
            parse_errors_[name] = "Enter valid JSON matching the field type.";
          }
        }
      }
      if (type == "string" || type == "array" || type == "object") {
        const auto &buffer = buffers_.at(name);
        if (std::char_traits<char>::length(buffer.data()) >=
            buffer.size() - 1) {
          parse_errors_[name] =
              "Input limit reached; shorten this field before executing.";
        } else if (type == "string") {
          parse_errors_.erase(name);
        }
      }
      centers_[name] = ImVec2(
          (ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * 0.5f,
          (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f);
      if (spec.contains("description") && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s",
                          spec.at("description").as<std::string>().c_str());
      }
      const auto errors = form_.errors();
      if (errors.contains(name)) {
        ImGui::TextWrapped("%s", errors.at(name).c_str());
      }
      if (parse_errors_.contains(name)) {
        ImGui::TextWrapped("%s", parse_errors_.at(name).c_str());
      }
      if (command_error &&
          command_error->at("field").as<std::string>() == name) {
        ImGui::TextWrapped(
            "%s", command_error->at("message").as<std::string>().c_str());
      }
      ImGui::PopID();
    }
    if (!show_execute) {
      return draft_arguments();
    }
    const bool invalid = !form_.errors().empty() || !parse_errors_.empty();
    ImGui::BeginDisabled(invalid);
    const bool execute = ImGui::Button("Execute command");
    execute_center_ =
        ImVec2((ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * 0.5f,
               (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f);
    ImGui::EndDisabled();
    if (execute) {
      return form_.arguments();
    }
    return {};
  }

  std::optional<Json> draft_arguments() const {
    if (!form_.errors().empty() || !parse_errors_.empty()) {
      return {};
    }
    return form_.arguments();
  }

  DesktopLayoutForm *test_layout_form() {
    return layout_ ? &*layout_ : nullptr;
  }

  ImVec2 test_center(const std::string &name) const {
    return centers_.at(name);
  }
  ImVec2 test_execute_center() const { return execute_center_; }

private:
  std::map<std::string, ImVec2> centers_;
  ImVec2 execute_center_{};
  std::optional<DesktopLayoutForm> layout_;
  Json descriptor_;
  CommandForm form_;
  std::map<std::string, std::array<char, 16384>> buffers_;
  CommandForm::Errors parse_errors_;
};
} // namespace lyko
