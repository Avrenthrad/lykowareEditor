#pragma once
#include "platform.hpp"
#include "ui/dock_layout_json.hpp"
#include <map>
namespace lyko {
inline const std::map<std::string, std::string> dock_panel_titles{
    {"scene", "Scene"},
    {"inspector", "Inspector"},
    {"viewport", "Viewport"},
    {"console", "Console"},
    {"history", "History"}};
struct DockProposal {
  DockLayout layout;
  std::uint64_t expected_revision;
};
class DockingAdapter {
public:
  void sync(ImGuiID root, const Json &layout, std::uint64_t revision);
  std::optional<DockProposal> finish();
  void submitted(std::uint64_t ticket) { ticket_ = ticket; }
  void rejected(const std::string &message) {
    message_ = message;
    restore_ = true;
    proposal_ = false;
  }
  void completed(std::uint64_t ticket) {
    if (ticket_ == ticket) {
      resolved_ = true;
    }
  }
  void snapshot() {
    if (resolved_) {
      ticket_.reset();
      resolved_ = false;
      proposal_ = false;
      restore_ = true;
    }
  }
  std::string take_message() {
    auto result = message_;
    message_.clear();
    return result;
  }

private:
  void apply(const DockLayout &layout);
  DockLayout capture() const;
  ImGuiID root_ = 0;
  DockLayout confirmed_;
  std::string model_key_, baseline_, message_;
  std::uint64_t gesture_revision_ = 0;
  std::optional<std::uint64_t> ticket_;
  bool restore_ = true, resolved_ = false, proposal_ = false, conflict_ = false;
  unsigned settle_ = 0, capture_delay_ = 0;
  ImVec2 viewport_size_{};
  std::vector<std::string> active_targets_;
};
} // namespace lyko
