#pragma once
#include "json.hpp"
#include <algorithm>

namespace lyko {
// Draft-only model shared by the GUI controls and tests. It never edits a
// document: validated arguments still go through the authoritative service.
class CommandForm {
public:
  using Errors = std::map<std::string, std::string>;
  explicit CommandForm(Json schema) : schema_(std::move(schema)) {
    for (const auto &[name, spec] :
         schema_.at("properties").as<Json::Object>()) {
      const auto &type = spec.at("type").as<std::string>();
      if (spec.contains("default")) {
        values_.emplace(name, spec.at("default"));
      } else if (type == "string") {
        values_.emplace(name, "");
      } else if (type == "integer") {
        values_.emplace(name, spec.contains("minimum")
                                  ? spec.at("minimum")
                                  : Json(std::uint64_t{0}));
      } else if (type == "boolean") {
        values_.emplace(name, false);
      } else if (type == "array") {
        values_.emplace(name, Json::Array{});
      } else if (type == "object") {
        values_.emplace(name, Json::Object{});
      } else {
        throw std::invalid_argument("Unsupported command form field type");
      }
    }
  }
  void set(const std::string &name, Json value) {
    if (!values_.contains(name)) {
      throw std::invalid_argument("Unknown form field");
    }
    values_.at(name) = std::move(value);
  }
  const Json &value(const std::string &name) const { return values_.at(name); }
  const Json &schema() const { return schema_; }
  Errors errors() const {
    Errors errors;
    for (const auto &[name, spec] :
         schema_.at("properties").as<Json::Object>()) {
      const auto &value = values_.at(name);
      const auto &type = spec.at("type").as<std::string>();
      if (type == "string") {
        const auto *text = std::get_if<std::string>(&value.value);
        if (!text || !json_detail::valid_utf8(*text)) {
          errors[name] = "Enter valid text.";
          continue;
        }
        std::uint64_t count = 0;
        for (unsigned char c : *text) {
          if ((c & 0xc0) != 0x80) {
            ++count;
          }
        }
        if (spec.contains("minLength") &&
            count < spec.at("minLength").as<std::uint64_t>()) {
          errors[name] = "This field is required.";
        }
        if (spec.contains("maxLength") &&
            count > spec.at("maxLength").as<std::uint64_t>()) {
          errors[name] = "Text is too long.";
        }
        if (spec.contains("x-maxBytes") &&
            text->size() > spec.at("x-maxBytes").as<std::uint64_t>()) {
          errors[name] = "Text exceeds the UTF-8 byte limit.";
        }
      } else if (type == "integer") {
        const auto *number = std::get_if<std::uint64_t>(&value.value);
        if (!number) {
          errors[name] = "Enter a whole nonnegative number.";
          continue;
        }
        if (spec.contains("minimum") &&
            *number < spec.at("minimum").as<std::uint64_t>()) {
          errors[name] = "Number is below the allowed minimum.";
        }
        if (spec.contains("maximum") &&
            *number > spec.at("maximum").as<std::uint64_t>()) {
          errors[name] = "Number is above the allowed maximum.";
        }
      } else if (type == "boolean" &&
                 !std::holds_alternative<bool>(value.value)) {
        errors[name] = "Choose on or off.";
      } else if (type == "array" &&
                 !std::holds_alternative<Json::Array>(value.value)) {
        errors[name] = "Enter a JSON array.";
      }
      if (type == "object" &&
          !std::holds_alternative<Json::Object>(value.value)) {
        errors[name] = "Enter a JSON object.";
      }
      if (spec.contains("enum")) {
        const auto &choices = spec.at("enum").as<Json::Array>();
        if (std::find(choices.begin(), choices.end(), value) == choices.end()) {
          errors[name] = "Choose a value from the list.";
        }
      }
    }
    return errors;
  }
  Json arguments() const {
    if (!errors().empty()) {
      throw std::invalid_argument(
          "Correct the highlighted fields before executing");
    }
    return values_;
  }

private:
  Json schema_;
  Json::Object values_;
};
} // namespace lyko
