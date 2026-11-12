#pragma once
#include "json.hpp"
#include <optional>

namespace lyko {
struct QueuedCommand {
  Json message;
  std::optional<std::uint64_t> form;
};
struct PendingCommand {
  std::string name;
  std::optional<std::uint64_t> form;
};
// UI-owned lifecycle. Leaving a draft does not cancel its submitted command.
class FormFeedback {
public:
  void reset() {
    ++generation_;
    busy_ = false;
    ticket_.reset();
    result_.reset();
    submitted_.reset();
    preview_.clear();
  }
  std::uint64_t generation() const { return generation_; }
  bool busy() const { return busy_; }
  std::optional<std::uint64_t> ticket() const { return ticket_; }
  const std::optional<Json> &result() const { return result_; }
  const std::string &preview() const { return preview_; }
  const Json *error() const {
    return result_ && !result_->at("ok").as<bool>() ? &result_->at("error")
                                                    : nullptr;
  }
  bool begin(Json arguments) {
    if (busy_) {
      return false;
    }
    busy_ = true;
    ticket_.reset();
    result_.reset();
    preview_.clear();
    submitted_ = std::move(arguments);
    return true;
  }
  void accepted(std::uint64_t generation, std::uint64_t ticket) {
    if (generation == generation_ && busy_) {
      ticket_ = ticket;
    }
  }
  void complete(std::uint64_t generation, std::uint64_t ticket,
                const Json &response) {
    if (generation == generation_ && busy_ && ticket_ == ticket) {
      finish(response);
    }
  }
  void reject(std::uint64_t generation, const Json &response) {
    if (generation == generation_ && busy_ && !ticket_) {
      finish(response);
    }
  }
  void reconcile(const std::optional<Json> &draft) {
    if (!busy_ && result_ && draft != submitted_) {
      result_.reset();
      preview_.clear();
      submitted_.reset();
    }
  }

private:
  void finish(const Json &response) {
    result_ = response;
    busy_ = false;
    if (response.at("ok").as<bool>()) {
      preview_ = response.at("data").dump();
      if (preview_.size() > 4096) {
        preview_.resize(4096);
        while (!json_detail::valid_utf8(preview_)) {
          preview_.pop_back();
        }
        preview_ += "\n[Result preview truncated]";
      }
    }
  }
  std::uint64_t generation_ = 0;
  bool busy_ = false;
  std::optional<std::uint64_t> ticket_;
  std::optional<Json> result_, submitted_;
  std::string preview_;
};
} // namespace lyko
