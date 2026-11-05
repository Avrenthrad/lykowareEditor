#include "command_form.hpp"
#include "imgui_internal.h"
#include "platform.hpp"
#include "transport/mcp_stream.hpp"
#include "ui/command_queue.hpp"
#include <array>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <iostream>
#include <map>
namespace {
using J = lyko::Json;
using O = J::Object;
using A = J::Array;
const std::map<std::string, std::string> titles{{"scene", "Scene"},
                                                {"inspector", "Inspector"},
                                                {"viewport", "Viewport"},
                                                {"console", "Console"},
                                                {"history", "History"}};
ImVec4 color(const J &token) {
  const auto &s = token.as<std::string>();
  return ImVec4(
      static_cast<float>(std::stoul(s.substr(1, 2), nullptr, 16)) / 255.f,
      static_cast<float>(std::stoul(s.substr(3, 2), nullptr, 16)) / 255.f,
      static_cast<float>(std::stoul(s.substr(5, 2), nullptr, 16)) / 255.f, 1);
}
void theme(const J &tokens) {
  ImGui::StyleColorsDark();
  auto &s = ImGui::GetStyle();
  const auto &c = tokens.at("colors");
  s.Colors[ImGuiCol_WindowBg] = color(c.at("background"));
  s.Colors[ImGuiCol_ChildBg] = color(c.at("surface"));
  s.Colors[ImGuiCol_FrameBg] = color(c.at("surface"));
  s.Colors[ImGuiCol_Text] = color(c.at("text"));
  s.Colors[ImGuiCol_TextDisabled] = color(c.at("muted_text"));
  s.Colors[ImGuiCol_CheckMark] = color(c.at("accent"));
  s.Colors[ImGuiCol_NavCursor] = color(c.at("focus"));
  const auto numeric = [](const J &v) {
    return std::holds_alternative<double>(v.value)
               ? static_cast<float>(v.as<double>())
               : static_cast<float>(v.as<std::uint64_t>());
  };
  const auto &spacing = tokens.at("spacing");
  const float large = numeric(spacing.at("lg")),
              small = numeric(spacing.at("sm"));
  s.WindowRounding = numeric(tokens.at("geometry").at("corner_radius_px"));
  s.FrameRounding = s.WindowRounding;
  s.WindowPadding = ImVec2(large, large);
  s.FramePadding = ImVec2(small, small / 2);
  s.ItemSpacing = ImVec2(small, small);
  s.FontSizeBase = 14;
  s.FontScaleMain =
      static_cast<float>(tokens.at("scale_percent").as<std::uint64_t>()) /
      100.f;
}
void docks(ImGuiID root, const J &w) {
  ImGui::DockBuilderRemoveNode(root);
  ImGui::DockBuilderAddNode(root, ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(root, ImGui::GetMainViewport()->WorkSize);
  ImGuiID center = root;
  const auto &sizes = w.at("splits");
  float l = static_cast<float>(sizes.at("left").as<std::uint64_t>()) / 100.f,
        r = static_cast<float>(sizes.at("right").as<std::uint64_t>()) / 100.f,
        b = static_cast<float>(sizes.at("bottom").as<std::uint64_t>()) / 100.f;
  auto left =
      ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, l, nullptr, &center);
  auto right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, r / (1 - l),
                                           nullptr, &center);
  auto bottom =
      ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, b, nullptr, &center);
  const std::map<std::string, ImGuiID> ids{
      {"left", left}, {"right", right}, {"center", center}, {"bottom", bottom}};
  for (const auto &[panel, dock] : w.at("panels").as<O>()) {
    if (dock.as<std::string>() != "hidden") {
      ImGui::DockBuilderDockWindow(titles.at(panel).c_str(),
                                   ids.at(dock.as<std::string>()));
    }
  }
  ImGui::DockBuilderFinish(root);
}
} // namespace
int main(int argc, char **argv) {
  bool smoke = false, mcp = false, smoke_input = false;
  std::string capture, instance = "desktop";
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--smoke-input") {
      smoke = true;
      smoke_input = true;
    } else if (arg == "--smoke") {
      smoke = true;
    } else if (arg == "--mcp") {
      mcp = true;
    } else if ((arg == "--capture" || arg == "--instance") && i + 1 < argc) {
      if (arg == "--capture") {
        capture = argv[++i];
      } else {
        instance = argv[++i];
      }
    } else {
      std::cerr << "Usage: lykoware_editor [--instance NAME] [--mcp] [--smoke "
                   "[--capture NEW.bmp]]\n";
      return 2;
    }
  }
  if (instance.empty() ||
      (!capture.empty() && (!smoke || std::filesystem::exists(capture)))) {
    std::cerr << "Capture requires --smoke and a new path\n";
    return 2;
  }
  try {
    lyko::MockEngineBridge backend;
    lyko::CommandService service(instance, backend);
    lyko::CommandQueue queue(service);
    lyko::DesktopPlatform platform;
    std::jthread protocol;
    if (mcp) {
      protocol = std::jthread(
          [&](std::stop_token stop) { lyko::run_mcp_stream(service, stop); });
    }
    J frame, palette = A{}, catalog = A{};
    bool ready = false, refresh = true, snapshot_pending = false,
         palette_open = false, focus_search = false;
    std::uint64_t revision = 0, editing = 0;
    std::string status = "Ready", layout, style, active, editing_name;
    std::deque<J> outgoing;
    std::map<std::uint64_t, std::string> pending;
    std::array<char, 256> search{}, rename{};
    std::optional<lyko::DesktopCommandForm> command_form;
    auto request = [&](const std::string &name, J args = O{},
                       bool mutation = true) {
      if (outgoing.size() >= 64) {
        status = "Too many pending commands; wait for completion.";
        return;
      }
      O object{{"instance", instance},
               {"command", name},
               {"arguments", std::move(args)}};
      if (mutation && ready) {
        object.emplace("expected_revision", revision);
      }
      outgoing.emplace_back(std::move(object));
    };
    const auto choose_command = [&](const J &item) {
      const auto name = item.at("name").as<std::string>();
      active = name;
      O current;
      if (ready && name == "workspace.configure") {
        const auto &w = frame.at("workspace");
        for (auto key : {"scale_percent", "theme", "reduced_motion"}) {
          current.emplace(key, w.at(key));
        }
      }
      if (ready && name == "entity.rename") {
        const auto &entity = frame.at("selection").at("entity");
        if (std::holds_alternative<O>(entity.value)) {
          current.emplace("id", entity.at("id"));
          current.emplace("name", entity.at("name"));
        }
      }
      command_form.emplace(item, current);
    };
    request("commands.list", O{}, false);
    if (smoke) {
      request("transaction.execute",
              O{{"commands", A{O{{"command", "entity.create"},
                                 {"arguments", O{{"name", "Player"}}}},
                               O{{"command", "entity.create"},
                                 {"arguments", O{{"name", "Ground"}}}},
                               O{{"command", "entity.create"},
                                 {"arguments", O{{"name", "Key light"}}}},
                               O{{"command", "selection.set"},
                                 {"arguments", O{{"id", std::uint64_t{1}}}}},
                               O{{"command", "play.start"}},
                               O{{"command", "play.input"},
                                 {"arguments", O{{"value", "right"}}}}}}},
              false);
    }
    unsigned frames = 0;
    ImVec2 create_center{}, name_center{}, search_center{}, settings_center{};
    while (platform.begin_frame()) {
      while (auto done = queue.try_pop()) {
        auto name = pending.at(done->ticket);
        pending.erase(done->ticket);
        if (name == "editor.snapshot") {
          snapshot_pending = false;
        }
        const auto &response = done->response;
        if (!response.at("ok").as<bool>()) {
          status = response.at("error").at("message").as<std::string>();
          refresh = true;
          continue;
        }
        if (name == "editor.snapshot") {
          frame = response.at("data");
          revision = response.at("revision").as<std::uint64_t>();
          ready = true;
          request("preview.capture", O{}, false);
        } else if (name == "preview.capture") {
          platform.set_preview(response.at("data").at("ppm").as<std::string>());
        } else if (name == "commands.list") {
          catalog = response.at("data");
        } else if (name == "palette.search") {
          palette = response.at("data");
        } else {
          status = "Command completed";
          refresh = true;
        }
      }
      if (refresh && !snapshot_pending) {
        request("editor.snapshot", O{}, false);
        snapshot_pending = true;
        refresh = false;
      }
      if (ready) {
        auto next = frame.at("tokens").dump();
        if (next != style) {
          theme(frame.at("tokens"));
          style = next;
        }
        for (const auto &shortcut : platform.shortcuts) {
          for (const auto &[name, key] :
               frame.at("workspace").at("keymap").as<O>()) {
            if (key.as<std::string>() != shortcut) {
              continue;
            }
            if (name == "palette.search") {
              palette_open = !palette_open;
              focus_search = palette_open;
              if (palette_open) {
                request("palette.search",
                        O{{"query", std::string(search.data())}}, false);
              }
            } else if (!ImGui::GetIO().WantTextInput) {
              for (const auto &descriptor : catalog.as<A>()) {
                if (descriptor.at("name").as<std::string>() != name) {
                  continue;
                }
                if (descriptor.at("inputSchema")
                        .at("required")
                        .as<A>()
                        .empty()) {
                  request(name);
                } else {
                  choose_command(descriptor);
                  palette_open = true;
                }
                break;
              }
            }
          }
        }
      }
      const auto key_for = [&](const std::string &name) {
        if (!ready) {
          return std::string{};
        }
        const auto &keymap = frame.at("workspace").at("keymap").as<O>();
        auto it = keymap.find(name);
        return it == keymap.end() ? std::string{}
                                  : it->second.as<std::string>();
      };
      if (ImGui::BeginMainMenuBar()) {
        ImGui::TextUnformatted("LYKOWARE");
        ImGui::SameLine();
        ImGui::TextDisabled("/ Untitled project");
        if (ready && ImGui::BeginMenu("Workspace")) {
          const auto &w = frame.at("workspace");
          for (auto choice : {"dark", "high_contrast"}) {
            if (ImGui::MenuItem(choice)) {
              request("workspace.configure",
                      O{{"scale_percent", w.at("scale_percent")},
                        {"theme", choice},
                        {"reduced_motion", w.at("reduced_motion")}});
            }
          }
          for (const auto &[panel, title] : titles) {
            if (ImGui::BeginMenu(title.c_str())) {
              for (auto dock :
                   {"left", "right", "center", "bottom", "hidden"}) {
                if (ImGui::MenuItem(dock)) {
                  request("panel.dock", O{{"panel", panel}, {"dock", dock}});
                }
              }
              ImGui::EndMenu();
            }
          }
          ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Commands", key_for("palette.search").c_str())) {
          palette_open = true;
          focus_search = true;
          active.clear();
          command_form.reset();
          request("palette.search", O{{"query", std::string(search.data())}},
                  false);
        }
        ImGui::SameLine();
        ImGui::TextDisabled(" | rev %llu | %s",
                            static_cast<unsigned long long>(revision),
                            status.c_str());
        ImGui::EndMainMenuBar();
      }
      auto root = ImGui::DockSpaceOverViewport(
          0, ImGui::GetMainViewport(),
          ImGuiDockNodeFlags_NoUndocking | ImGuiDockNodeFlags_NoDockingSplit |
              ImGuiDockNodeFlags_NoResize);
      if (ready) {
        const auto &w = frame.at("workspace");
        auto next = w.at("panels").dump() + w.at("splits").dump();
        if (next != layout) {
          docks(root, w);
          layout = next;
        }
        auto visible = [&](const char *id) {
          return w.at("panels").at(id).as<std::string>() != "hidden";
        };
        if (visible("scene")) {
          ImGui::Begin("Scene");
          ImGui::TextDisabled("SCENE COLLECTION");
          if (ImGui::Button("+ Create entity")) {
            request("entity.create", O{{"name", "New entity"}});
          }
          create_center = ImVec2(
              (ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * 0.5f,
              (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f);
          ImGui::Separator();
          for (const auto &entity : frame.at("scene").at("entities").as<A>()) {
            auto id = entity.at("id").as<std::uint64_t>();
            ImGui::PushID(std::to_string(id).c_str());
            const auto &selection = w.at("selection");
            bool selected =
                std::holds_alternative<std::uint64_t>(selection.value) &&
                selection.as<std::uint64_t>() == id;
            if (ImGui::Selectable(entity.at("name").as<std::string>().c_str(),
                                  selected)) {
              request("selection.set", O{{"id", id}});
            }
            if (ImGui::BeginPopupContextItem("entity_actions")) {
              if (ImGui::MenuItem("Inspect",
                                  key_for("selection.set").c_str())) {
                request("selection.set", O{{"id", id}});
              }
              ImGui::Separator();
              if (ImGui::MenuItem("Undo", key_for("history.undo").c_str())) {
                request("history.undo");
              }
              if (ImGui::MenuItem("Redo", key_for("history.redo").c_str())) {
                request("history.redo");
              }
              ImGui::EndPopup();
            }
            ImGui::PopID();
          }
          ImGui::End();
        }
        if (visible("inspector")) {
          ImGui::Begin("Inspector");
          ImGui::TextDisabled("ENTITY PROPERTIES");
          ImGui::Separator();
          const auto &entity = frame.at("selection").at("entity");
          if (std::holds_alternative<std::nullptr_t>(entity.value)) {
            ImGui::TextWrapped("Select an entity to inspect its properties.");
            editing = 0;
          } else {
            auto id = entity.at("id").as<std::uint64_t>();
            if (editing != id ||
                editing_name != entity.at("name").as<std::string>()) {
              editing_name = entity.at("name").as<std::string>();
              std::snprintf(rename.data(), rename.size(), "%s",
                            editing_name.c_str());
              editing = id;
            }
            ImGui::Text("Entity %llu", static_cast<unsigned long long>(id));
            if (ImGui::InputText("Name", rename.data(), rename.size(),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
              request("entity.rename",
                      O{{"id", id}, {"name", std::string(rename.data())}});
            }
            name_center = ImVec2(
                (ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * 0.5f,
                (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f);
            if (ImGui::Button("Apply name")) {
              request("entity.rename",
                      O{{"id", id}, {"name", std::string(rename.data())}});
            }
            ImGui::TextDisabled("Enter to apply / Ctrl+Z to undo");
          }
          ImGui::End();
        }
        if (visible("viewport")) {
          ImGui::Begin("Viewport");
          bool playing =
              frame.at("scene").at("runtime").at("playing").as<bool>();
          if (ImGui::Button(playing ? "Stop" : "Play")) {
            request(playing ? "play.stop" : "play.start");
          }
          ImGui::SameLine();
          ImGui::TextDisabled("MOCK / SOFTWARE PREVIEW");
          ImGui::Separator();
          auto size = ImGui::GetContentRegionAvail();
          float width = size.x, height = width * 36.f / 64.f;
          if (height > size.y - 30) {
            height = size.y - 30;
            width = height * 64.f / 36.f;
          }
          platform.draw_preview(
              ImVec2(width > 1 ? width : 1, height > 1 ? height : 1));
          ImGui::TextDisabled("No engine or real simulation connected.");
          ImGui::End();
        }
        if (visible("console")) {
          ImGui::Begin("Console");
          ImGui::TextDisabled("RUNTIME LOGS");
          for (const auto &message : frame.at("logs").at("messages").as<A>()) {
            ImGui::TextUnformatted(message.as<std::string>().c_str());
          }
          ImGui::Separator();
          ImGui::TextWrapped("%s", status.c_str());
          ImGui::End();
        }
        if (visible("history")) {
          ImGui::Begin("History");
          if (ImGui::Button("Undo")) {
            request("history.undo");
          }
          ImGui::SameLine();
          if (ImGui::Button("Redo")) {
            request("history.redo");
          }
          ImGui::Separator();
          for (const auto &entry : frame.at("history").at("entries").as<A>()) {
            ImGui::Text("%s / revision %llu",
                        entry.at("applied").as<bool>() ? "Applied" : "Redo",
                        static_cast<unsigned long long>(
                            entry.at("revision").as<std::uint64_t>()));
            for (const auto &command : entry.at("commands").as<A>()) {
              ImGui::BulletText("%s", command.as<std::string>().c_str());
            }
          }
          ImGui::End();
        }
      }
      if (palette_open && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        if (active.empty()) {
          palette_open = false, focus_search = false;
        } else {
          active.clear();
          command_form.reset();
        }
      }
      if (palette_open) {
        ImGui::SetNextWindowSize(ImVec2(660, 520), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
                                ImGuiCond_Appearing, ImVec2(0.5f, 0.35f));
        ImGui::Begin("Command palette", &palette_open,
                     ImGuiWindowFlags_NoDocking);
        if (focus_search) {
          ImGui::SetKeyboardFocusHere();
          focus_search = false;
        }
        if (ImGui::InputTextWithHint("##query", "Search every command...",
                                     search.data(), search.size(),
                                     ImGuiInputTextFlags_AutoSelectAll)) {
          active.clear();
          command_form.reset();
          request("palette.search", O{{"query", std::string(search.data())}},
                  false);
        }
        search_center = ImVec2(
            (ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * 0.5f,
            (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f);
        if (active.empty()) {
          for (const auto &item : palette.as<A>()) {
            auto name = item.at("name").as<std::string>();
            if (ImGui::Selectable(
                    (name + "    " + item.at("shortcut").as<std::string>())
                        .c_str())) {
              choose_command(item);
            }
            if (name == "workspace.configure") {
              settings_center = ImVec2(
                  (ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) *
                      0.5f,
                  (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) *
                      0.5f);
            }
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip(
                  "%s", item.at("description").as<std::string>().c_str());
            }
          }
        } else {
          ImGui::TextUnformatted(active.c_str());
          if (command_form) {
            if (auto arguments = command_form->draw()) {
              request(active, *arguments);
            }
          }
          ImGui::SameLine();
          if (ImGui::Button("Back")) {
            active.clear();
            command_form.reset();
            focus_search = true;
          }
        }
        ImGui::End();
      }
      while (!outgoing.empty()) {
        auto submitted = queue.try_submit(outgoing.front());
        if (!submitted) {
          if (submitted.error->code == "queue_busy" ||
              submitted.error->code == "queue_full") {
            break;
          }
          status = submitted.error->message;
          outgoing.pop_front();
          continue;
        }
        pending.emplace(*submitted.ticket,
                        outgoing.front().at("command").as<std::string>());
        outgoing.pop_front();
      }
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
            const auto expected =
                (checkpoint == 38 || checkpoint == 78) ? 4U : 3U;
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
          platform.test_pointer(command_form->test_execute_center(),
                                frames == 183);
        }
        if (frames == 200 &&
            !frame.at("workspace").at("reduced_motion").as<bool>()) {
          throw std::runtime_error(
              "Interaction smoke: typed form did not apply settings");
        }
      }
      ++frames;
      if (mcp && frames % 30 == 0) {
        refresh = true;
      }
      bool finish = smoke && frames >= (smoke_input ? 210U : 90U) && ready &&
                    pending.empty() && outgoing.empty();
      platform.end_frame(finish ? capture : "");
      if (finish) {
        if (smoke_input &&
            frame.at("selection").at("entity").at("name").as<std::string>() !=
                "Hero") {
          throw std::runtime_error(
              "Interaction smoke: inspector rename failed");
        }
        if (frame.at("scene").at("entities").as<A>().size() != 3 ||
            frame.at("workspace").at("selection").as<std::uint64_t>() != 1 ||
            frame.at("scene").at("runtime").at("inputs").as<A>().size() != 1) {
          return 1;
        }
        (mcp ? std::cerr : std::cout)
            << "Desktop smoke passed: three entities, inspector, history and "
               "software preview\n";
        return 0;
      }
      if (smoke && frames > 600) {
        std::cerr << "Smoke timed out\n";
        return 1;
      }
    }
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
