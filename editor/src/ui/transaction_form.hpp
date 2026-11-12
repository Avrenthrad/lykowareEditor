#pragma once
#include "command_form.hpp"

namespace lyko {
// Validates a draft batch against discovery, without executing any row.
class TransactionForm {
public:
  static constexpr std::size_t max_rows = 64;
  explicit TransactionForm(const Json &catalog) {
    for (const auto &item : catalog.as<Json::Array>()) {
      const auto &name = item.at("name").as<std::string>();
      if (item.at("mutation").as<bool>() && name != "transaction.execute") {
        choices_.emplace(name, item);
      }
    }
  }
  const std::map<std::string, Json> &choices() const { return choices_; }
  Json arguments(const Json::Array &rows) const {
    if (rows.empty() || rows.size() > max_rows) {
      throw std::invalid_argument("Add 1 to 64 mutation commands.");
    }
    for (std::size_t i = 0; i < rows.size(); ++i) {
      try {
        const auto &row = rows[i];
        if (row.as<Json::Object>().size() != 2) {
          throw std::invalid_argument("Expected command and arguments.");
        }
        const auto &name = row.at("command").as<std::string>();
        const auto found = choices_.find(name);
        if (found == choices_.end()) {
          throw std::invalid_argument("Choose a registered mutation.");
        }
        CommandForm form(found->second.at("inputSchema"));
        const auto &args = row.at("arguments").as<Json::Object>();
        if (args.size() !=
            form.schema().at("properties").as<Json::Object>().size()) {
          throw std::invalid_argument("Supply all command fields.");
        }
        for (const auto &[key, value] : args) {
          form.set(key, value);
        }
        (void)form.arguments();
      } catch (const std::invalid_argument &error) {
        throw std::invalid_argument("Row " + std::to_string(i + 1) + ": " +
                                    error.what());
      }
    }
    return Json::Object{{"commands", rows}};
  }

private:
  std::map<std::string, Json> choices_;
};
} // namespace lyko
