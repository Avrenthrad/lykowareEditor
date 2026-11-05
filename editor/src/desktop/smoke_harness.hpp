#pragma once
#include "command_form.hpp"
namespace lyko {
struct SmokeContext {
  DesktopPlatform &platform;
  ImGuiID root;
  const Json &frame;
  std::uint64_t revision;
  ImVec2 create_center, name_center, search_center, settings_center;
  DesktopCommandForm *command_form;
  bool palette_open;
  const std::string &status;
};
class DesktopSmokeHarness {
public:
  DesktopSmokeHarness(bool input, bool docking)
      : input_(input), docking_(docking) {}
  void step(unsigned frames, const SmokeContext &context);
  void validate(const Json &frame) const;
  unsigned target_frames() const {
    return docking_ ? 370U : (input_ ? 210U : 90U);
  }

private:
  bool input_, docking_;
  ImVec2 dock_start_{}, dock_target_{};
  std::uint64_t resized_ratio_ = 0, before_float_revision_ = 0;
  Json before_float_;
};
} // namespace lyko
