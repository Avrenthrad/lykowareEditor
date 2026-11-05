#include "smoke_harness.hpp"
#include "imgui_internal.h"
#include <stdexcept>
namespace lyko {
void DesktopSmokeHarness::step(unsigned frames, const SmokeContext &context) {
  using J = Json;
  using A = Json::Array;
  using O = Json::Object;
  const bool smoke_input = input_, smoke_docking = docking_, ready = true;
  auto &platform = context.platform;
  const auto root = context.root;
  const auto &frame = context.frame;
  const auto revision = context.revision;
  const auto create_center = context.create_center,
             name_center = context.name_center,
             search_center = context.search_center,
             settings_center = context.settings_center;
  auto *command_form = context.command_form;
  const auto palette_open = context.palette_open;
  const auto &status = context.status;
  auto &dock_start = dock_start_;
  auto &dock_target = dock_target_;
  auto &resized_ratio = resized_ratio_;
  auto &before_float = before_float_;
  auto &before_float_revision = before_float_revision_;
  if (smoke_input && ready) {
    if (frames == 20) {
      platform.test_pointer(create_center, true);
    }
    if (frames == 21) {
      platform.test_pointer(create_center, false);
    }
    if (frames == 40 || frames == 80) {
      platform.test_key('z', true);
    }
    if (frames == 60) {
      platform.test_key('z', true, true);
    }
    for (auto checkpoint : {38U, 58U, 78U, 98U}) {
      if (frames == checkpoint) {
        const auto expected = (checkpoint == 38 || checkpoint == 78) ? 4U : 3U;
        if (frame.at("scene").at("entities").as<A>().size() != expected) {
          throw std::runtime_error(
              "Interaction smoke: create/undo/redo did not update scene");
        }
      }
    }
    if (frames == 100 || frames == 110) {
      platform.test_key('k', true);
    }
    if (frames == 108 && (!palette_open || !ImGui::GetIO().WantTextInput)) {
      throw std::runtime_error(
          "Interaction smoke: Ctrl+K did not open palette");
    }
    if (frames == 120) {
      platform.test_pointer(name_center, true);
    }
    if (frames == 121) {
      platform.test_pointer(name_center, false);
    }
    if (frames == 124) {
      platform.test_key('a', true);
    }
    if (frames == 126) {
      platform.test_text("Hero");
    }
    if (frames == 128) {
      platform.test_key('\r', false);
    }
  }
  if (smoke_input && ready) {
    if (frames == 148) {
      platform.test_key('k', true);
    }
    if (frames == 151) {
      platform.test_pointer(search_center, true);
    }
    if (frames == 152) {
      platform.test_pointer(search_center, false);
    }
    if (frames == 154) {
      platform.test_text("workspace.configure");
    }
    if (frames == 164) {
      platform.test_pointer(settings_center, true);
    }
    if (frames == 165) {
      platform.test_pointer(settings_center, false);
    }
    if (frames == 174 || frames == 175) {
      if (!command_form) {
        throw std::runtime_error(
            "Interaction smoke: settings form did not open");
      }
      platform.test_pointer(command_form->test_center("reduced_motion"),
                            frames == 174);
    }
    if (frames == 183 || frames == 184) {
      platform.test_pointer(command_form->test_execute_center(), frames == 183);
    }
    if (frames == 200 &&
        !frame.at("workspace").at("reduced_motion").as<bool>()) {
      throw std::runtime_error(
          "Interaction smoke: typed form did not apply settings");
    }
  }
  if (smoke_input) {
    if (frames == 210 || frames == 211) {
      platform.test_pointer(search_center, frames == 210);
    }
    if (frames == 214) {
      platform.test_key('a', true);
    }
    if (frames == 216) {
      platform.test_text("transaction.execute");
    }
    if (frames == 230 || frames == 231) {
      platform.test_pointer(settings_center, frames == 230);
    }
    auto *batch = context.transaction_form;
    if (frames == 238 && !batch) {
      throw std::runtime_error(
          "Interaction smoke: transaction builder did not open");
    }
    if (frames == 240 || frames == 241 || frames == 254 || frames == 255) {
      platform.test_pointer(batch->test_add_center(),
                            frames == 240 || frames == 254);
    }
    if (frames == 246 || frames == 247 || frames == 260 || frames == 261) {
      platform.test_pointer(batch->test_choice_center("entity.create"),
                            frames == 246 || frames == 260);
    }
    if (frames == 280 || frames == 281) {
      platform.test_pointer(batch->test_execute_center(), frames == 280);
    }
    if (frames == 300 && frame.at("scene").at("entities").as<A>().size() != 5) {
      throw std::runtime_error(
          "Interaction smoke: transaction form did not commit both rows: " +
          status);
    }
    if (frames == 305) {
      platform.test_key('k', true);
    }
    if (frames == 310 || frames == 355) {
      platform.test_key('z', true);
    }
    if (frames == 330 || frames == 375) {
      if (frame.at("scene").at("entities").as<A>().size() != 3) {
        throw std::runtime_error(
            "Interaction smoke: batch undo was not atomic");
      }
    }
    if (frames == 335) {
      platform.test_key('z', true, true);
    }
    if (frames == 350 && frame.at("scene").at("entities").as<A>().size() != 5) {
      throw std::runtime_error("Interaction smoke: batch redo was not atomic");
    }
  }
  if (smoke_input) {
    auto *browser = context.shortcuts;
    if (frames == 378) {
      platform.test_key('k', true);
    }
    if (frames == 382 || frames == 383) {
      platform.test_pointer(context.shortcuts_center, frames == 382);
    }
    if (frames == 388 && !browser->open) {
      throw std::runtime_error(
          "Interaction smoke: shortcut browser did not open");
    }
    if (frames == 390 || frames == 391) {
      platform.test_pointer(browser->test_search_center(), frames == 390);
    }
    if (frames == 394) {
      platform.test_text("history.undo");
    }
    if (frames == 404 || frames == 405) {
      platform.test_pointer(browser->test_edit_center("history.undo"),
                            frames == 404);
    }
    if (frames == 416) {
      if (!command_form || !command_form->draft_arguments() ||
          command_form->draft_arguments()->at("command").as<std::string>() !=
              "history.undo" ||
          command_form->draft_arguments()->at("shortcut").as<std::string>() !=
              "Ctrl+Z") {
        throw std::runtime_error(
            "Interaction smoke: binding form was not prefilled");
      }
    }
    if (frames == 420 || frames == 421) {
      platform.test_pointer(command_form->test_center("shortcut"),
                            frames == 420);
    }
    if (frames == 424) {
      platform.test_key('a', true);
    }
    if (frames == 426) {
      platform.test_text("Ctrl+U");
    }
    if (frames == 435 || frames == 436) {
      platform.test_pointer(command_form->test_execute_center(), frames == 435);
    }
    if (frames == 450 && frame.at("workspace")
                                 .at("keymap")
                                 .at("history.undo")
                                 .as<std::string>() != "Ctrl+U") {
      throw std::runtime_error(
          "Interaction smoke: shortcut edit did not commit: " + status);
    }
    if (frames == 455) {
      platform.test_key('z', true);
    }
    if (frames == 459 && frame.at("workspace")
                                 .at("keymap")
                                 .at("history.undo")
                                 .as<std::string>() != "Ctrl+U") {
      throw std::runtime_error(
          "Interaction smoke: old shortcut still dispatched");
    }
    if (frames == 465 || frames == 510) {
      platform.test_key('u', true);
    }
    if (frames == 480 || frames == 525) {
      if (frame.at("workspace")
              .at("keymap")
              .at("history.undo")
              .as<std::string>() != "Ctrl+Z") {
        throw std::runtime_error(
            "Interaction smoke: rebound undo did not restore old binding");
      }
    }
    if (frames == 485) {
      platform.test_key('z', true, true);
    }
    if (frames == 500 && frame.at("workspace")
                                 .at("keymap")
                                 .at("history.undo")
                                 .as<std::string>() != "Ctrl+U") {
      throw std::runtime_error("Interaction smoke: binding redo failed");
    }
  }
  if (smoke_docking && ready) {
    const auto merged = [&]() {
      for (const auto &[id, node] :
           frame.at("workspace").at("layout").at("nodes").as<O>()) {
        (void)id;
        if (node.at("kind").as<std::string>() != "tabs") {
          continue;
        }
        bool scene = false, inspector = false;
        for (const auto &panel : node.at("panels").as<A>()) {
          scene = scene || panel.as<std::string>() == "scene";
          inspector = inspector || panel.as<std::string>() == "inspector";
        }
        if (scene && inspector) {
          return true;
        }
      }
      return false;
    };
    if (frames == 16) {
      auto *node = ImGui::DockBuilderGetNode(root);
      auto *left = node->ChildNodes[0];
      dock_start = ImVec2(left->Pos.x + left->Size.x + 1,
                          node->Pos.y + node->Size.y * 0.5f);
      dock_target = ImVec2(dock_start.x + 80, dock_start.y);
      platform.test_pointer(dock_start, false);
    }
    if (frames == 20) {
      platform.test_pointer(dock_start, true);
    }
    if (frames == 22) {
      platform.test_motion(dock_target);
    }
    if (frames == 24) {
      platform.test_pointer(dock_target, false);
    }

    if (frames == 40) {
      resized_ratio = frame.at("workspace")
                          .at("layout")
                          .at("nodes")
                          .at("root")
                          .at("ratio")
                          .as<std::uint64_t>();
      if (resized_ratio <= 2000) {
        throw std::runtime_error("Dock smoke: native resize not committed: " +
                                 status);
      }
    }
    if (frames == 45) {
      platform.test_key('z', true);
    }
    if (frames == 60 && frame.at("workspace")
                                .at("layout")
                                .at("nodes")
                                .at("root")
                                .at("ratio")
                                .as<std::uint64_t>() != 2000) {
      throw std::runtime_error("Dock smoke: resize undo failed");
    }
    if (frames == 65) {
      platform.test_key('z', true, true);
    }
    if (frames == 80 && frame.at("workspace")
                                .at("layout")
                                .at("nodes")
                                .at("root")
                                .at("ratio")
                                .as<std::uint64_t>() != resized_ratio) {
      throw std::runtime_error("Dock smoke: resize redo failed");
    }
    if (frames == 82) {
      auto *scene = ImGui::FindWindowByName("Scene");
      auto *inspector = ImGui::FindWindowByName("Inspector");
      dock_start = scene->DC.DockTabItemRect.GetCenter();
      dock_target = inspector->DockNode->Rect().GetCenter();
      platform.test_pointer(dock_start, false);
    }
    if (frames == 85) {
      platform.test_pointer(dock_start, true);
    }
    if (frames == 87) {
      platform.test_motion(ImVec2(dock_start.x + 25, dock_start.y + 40));
    }
    if (frames == 89) {
      auto *target = ImGui::FindWindowByName("Inspector");
      auto *payload = ImGui::FindWindowByName("Scene");
      if (!ImGui::DockContextCalcDropPosForDocking(
              target, target->DockNode, payload, nullptr, ImGuiDir_None, false,
              &dock_target)) {
        throw std::runtime_error("Dock smoke: merge target unavailable");
      }
      platform.test_motion(dock_target);
    }
    if (frames == 93) {
      platform.test_pointer(dock_target, false);
    }

    if (frames == 110 && !merged()) {
      throw std::runtime_error("Dock smoke: tab merge not committed: " +
                               status);
    }
    if (frames == 115) {
      platform.test_key('z', true);
    }
    if (frames == 130 && merged()) {
      throw std::runtime_error("Dock smoke: merge undo failed");
    }
    if (frames == 135) {
      platform.test_key('z', true, true);
    }
    if (frames == 150 && !merged()) {
      throw std::runtime_error("Dock smoke: merge redo failed");
    }
    if (frames == 152) {
      dock_target =
          ImGui::FindWindowByName("Inspector")->DC.DockTabItemRect.GetCenter();
      platform.test_pointer(dock_target, false);
    }
    if (frames == 155) {
      platform.test_pointer(dock_target, true);
    }
    if (frames == 156) {
      platform.test_pointer(dock_target, false);
    }
    if (frames == 175) {
      bool active = false;
      for (const auto &[id, node] :
           frame.at("workspace").at("layout").at("nodes").as<O>()) {
        (void)id;
        if (node.at("kind").as<std::string>() == "tabs" &&
            node.at("active").as<std::string>() == "inspector") {
          active = true;
        }
      }
      if (!active) {
        throw std::runtime_error("Dock smoke: active tab was not committed");
      }
      before_float = frame.at("workspace").at("layout");
    }

    const auto scene_first = [&]() {
      for (const auto &[id, node] :
           frame.at("workspace").at("layout").at("nodes").as<O>()) {
        (void)id;
        if (node.at("kind").as<std::string>() == "tabs" &&
            node.at("panels") == J(A{"scene", "inspector"})) {
          return true;
        }
      }
      return false;
    };
    if (frames == 178) {
      dock_start =
          ImGui::FindWindowByName("Scene")->DC.DockTabItemRect.GetCenter();
      auto tab = ImGui::FindWindowByName("Inspector")->DC.DockTabItemRect;
      dock_target = ImVec2(tab.Min.x + 4, tab.GetCenter().y);
      platform.test_pointer(dock_start, false);
    }
    if (frames == 180) {
      platform.test_pointer(dock_start, true);
    }
    if (frames == 182) {
      platform.test_motion(dock_target);
    }
    if (frames == 186) {
      platform.test_pointer(dock_target, false);
    }
    if (frames == 202 && !scene_first()) {
      throw std::runtime_error("Dock smoke: tab reorder not committed: " +
                               status);
    }
    if (frames == 205) {
      platform.test_key('z', true);
    }
    if (frames == 220 && scene_first()) {
      throw std::runtime_error("Dock smoke: tab reorder undo failed");
    }
    if (frames == 225) {
      platform.test_key('z', true, true);
    }
    if (frames == 240 && !scene_first()) {
      throw std::runtime_error("Dock smoke: tab reorder redo failed");
    }
    const auto separated = [&]() {
      const auto &nodes =
          frame.at("workspace").at("layout").at("nodes").as<O>();
      for (const auto &[id, node] : nodes) {
        (void)id;
        if (node.at("kind").as<std::string>() != "split") {
          continue;
        }
        const auto &a = nodes.at(node.at("first").as<std::string>());
        const auto &b = nodes.at(node.at("second").as<std::string>());
        if (a.at("kind").as<std::string>() == "tabs" &&
            b.at("kind").as<std::string>() == "tabs" &&
            a.at("panels") == J(A{"scene"}) &&
            b.at("panels") == J(A{"viewport"})) {
          return true;
        }
      }
      return false;
    };
    if (frames == 248) {
      dock_start =
          ImGui::FindWindowByName("Scene")->DC.DockTabItemRect.GetCenter();
      platform.test_pointer(dock_start, false);
    }
    if (frames == 250) {
      platform.test_pointer(dock_start, true);
    }
    if (frames == 252) {
      platform.test_motion(ImVec2(dock_start.x + 25, dock_start.y + 40));
    }
    if (frames == 254) {
      auto *target = ImGui::FindWindowByName("Viewport");
      auto *payload = ImGui::FindWindowByName("Scene");
      if (!ImGui::DockContextCalcDropPosForDocking(
              target, target->DockNode, payload, nullptr, ImGuiDir_Left, false,
              &dock_target)) {
        throw std::runtime_error("Dock smoke: split target unavailable");
      }
      platform.test_motion(dock_target);
    }
    if (frames == 258) {
      platform.test_pointer(dock_target, false);
    }
    if (frames == 275 && !separated()) {
      throw std::runtime_error("Dock smoke: native split not committed: " +
                               status);
    }
    if (frames == 280) {
      platform.test_key('z', true);
    }
    if (frames == 295 && !merged()) {
      throw std::runtime_error("Dock smoke: split undo failed");
    }
    if (frames == 300) {
      platform.test_key('z', true, true);
    }
    if (frames == 315 && !separated()) {
      throw std::runtime_error("Dock smoke: split redo failed");
    }
    if (frames == 320) {
      before_float = frame.at("workspace").at("layout");
      before_float_revision = revision;
      dock_start =
          ImGui::FindWindowByName("Viewport")->DC.DockTabItemRect.GetCenter();
      platform.test_pointer(dock_start, false);
    }
    if (frames == 323) {
      platform.test_pointer(dock_start, true);
    }
    if (frames == 325) {
      platform.test_motion(ImVec2(dock_start.x + 25, dock_start.y + 40));
    }
    if (frames == 328) {
      platform.test_motion(ImVec2(-400, -300));
    }
    if (frames == 331) {
      platform.test_pointer(ImVec2(-400, -300), false);
    }
    if (frames == 355) {
      if (frame.at("workspace").at("layout") != before_float ||
          revision != before_float_revision ||
          !ImGui::FindWindowByName("Viewport")->DockNode) {
        throw std::runtime_error(
            "Dock smoke: unsupported floating drop was not restored");
      }
    }
  }
}
void DesktopSmokeHarness::validate(const Json &frame) const {
  using A = Json::Array;
  if (input_ &&
      frame.at("selection").at("entity").at("name").as<std::string>() !=
          "Hero") {
    throw std::runtime_error("Interaction smoke: inspector rename failed");
  }
  if (frame.at("scene").at("entities").as<A>().size() != 3 ||
      frame.at("workspace").at("selection").as<std::uint64_t>() != 1 ||
      frame.at("scene").at("runtime").at("inputs").as<A>().size() != 1) {
    throw std::runtime_error("Smoke: scene state mismatch");
  }
}
} // namespace lyko
