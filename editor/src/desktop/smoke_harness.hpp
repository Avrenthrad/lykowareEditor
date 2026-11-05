#pragma once
#include "asset_browser.hpp"
#include "command_form.hpp"
#include "shortcut_browser.hpp"
#include "transaction_form.hpp"
#include "ui/form_feedback.hpp"
namespace lyko {
struct SmokeContext {
  DesktopPlatform &platform;
  ImGuiID root;
  const Json &frame;
  std::uint64_t revision;
  ImVec2 create_center, name_center, search_center, settings_center;
  DesktopCommandForm *command_form;
  DesktopTransactionForm *transaction_form;
  ShortcutBrowser *shortcuts;
  ImVec2 shortcuts_center;
  const FormFeedback *feedback;
  AssetBrowser *assets;
  ImVec2 assets_center;
  bool palette_open;
  const std::string &status;
};
class DesktopSmokeHarness {
public:
  DesktopSmokeHarness(bool input, bool docking, bool layout_form = false,
                      bool feedback = false, bool assets = false)
      : input_(input), docking_(docking), layout_form_(layout_form),
        feedback_(feedback), assets_(assets) {}
  void step(unsigned frames, const SmokeContext &context);
  void validate(const Json &frame) const;
  unsigned target_frames() const {
    if (assets_) {
      return 140;
    }
    return feedback_
               ? 225U
               : (layout_form_ ? 300U
                               : (docking_ ? 370U : (input_ ? 530U : 90U)));
  }

private:
  bool input_, docking_, layout_form_, feedback_, assets_;
  std::uint64_t feedback_revision_ = 0;
  Json before_layout_form_;
  std::uint64_t layout_form_revision_ = 0;
  ImVec2 dock_start_{}, dock_target_{};
  std::uint64_t resized_ratio_ = 0, before_float_revision_ = 0;
  Json before_float_;
};
} // namespace lyko
