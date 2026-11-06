#include "asset_browser.hpp"
#include "command_form.hpp"
#include "docking.hpp"
#include "form_feedback.hpp"
#include "icons.hpp"
#include "platform.hpp"
#include "project_menu.hpp"
#include "shortcut_browser.hpp"
#include "smoke_harness.hpp"
#include "source_audit.hpp"
#include "transaction_form.hpp"
#include "transport/mcp_stream.hpp"
#include "ui/command_queue.hpp"
#include <array>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
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
  for (auto slot :
       {ImGuiCol_PopupBg, ImGuiCol_TitleBg, ImGuiCol_Tab, ImGuiCol_TabDimmed,
        ImGuiCol_TableHeaderBg, ImGuiCol_MenuBarBg}) {
    s.Colors[slot] = color(c.at("surface"));
  }
  for (auto slot :
       {ImGuiCol_Border, ImGuiCol_Separator, ImGuiCol_TableBorderStrong,
        ImGuiCol_TableBorderLight, ImGuiCol_ScrollbarGrab}) {
    s.Colors[slot] = color(c.at("border"));
  }
  for (auto slot : {ImGuiCol_FrameBgHovered, ImGuiCol_Button,
                    ImGuiCol_HeaderHovered, ImGuiCol_TabHovered}) {
    s.Colors[slot] = color(c.at("surface_hover"));
  }
  for (auto slot :
       {ImGuiCol_FrameBgActive, ImGuiCol_ButtonHovered, ImGuiCol_ButtonActive,
        ImGuiCol_Header, ImGuiCol_HeaderActive, ImGuiCol_TitleBgActive,
        ImGuiCol_TabSelected, ImGuiCol_TabDimmedSelected,
        ImGuiCol_TextSelectedBg}) {
    s.Colors[slot] = color(c.at("surface_active"));
  }
  for (auto slot : {ImGuiCol_ScrollbarBg, ImGuiCol_DockingEmptyBg,
                    ImGuiCol_TitleBgCollapsed}) {
    s.Colors[slot] = color(c.at("background"));
  }
  for (auto slot :
       {ImGuiCol_CheckMark, ImGuiCol_SliderGrab, ImGuiCol_SliderGrabActive,
        ImGuiCol_SeparatorHovered, ImGuiCol_SeparatorActive,
        ImGuiCol_ResizeGripHovered, ImGuiCol_ResizeGripActive,
        ImGuiCol_TabSelectedOverline, ImGuiCol_TabDimmedSelectedOverline,
        ImGuiCol_ScrollbarGrabActive}) {
    s.Colors[slot] = color(c.at("accent"));
  }
  s.Colors[ImGuiCol_ScrollbarGrabHovered] = color(c.at("muted_text"));
  s.Colors[ImGuiCol_ResizeGrip] = color(c.at("border"));
  s.Colors[ImGuiCol_DockingPreview] = color(c.at("accent"));
  s.Colors[ImGuiCol_DockingPreview].w = 0.35f;
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
  const float scale =
      static_cast<float>(tokens.at("scale_percent").as<std::uint64_t>()) /
      100.f;
  const float body = numeric(tokens.at("typography").at("body_px"));
  s.FramePadding = ImVec2(
      small,
      (numeric(tokens.at("geometry").at("control_height_px")) - body) / 2.f);
  s.CellPadding = ImVec2(small, small / 2);
  s.ItemInnerSpacing = ImVec2(small, small / 2);
  s.TabRounding = s.WindowRounding;
  s.PopupRounding = s.WindowRounding;
  s.ScrollbarRounding = s.WindowRounding;
  s.SeparatorTextBorderSize = 1;
  s.ItemSpacing = ImVec2(small, small);
  s.FontSizeBase = body / scale;
  s.FontScaleMain = scale;
}
} // namespace
int main(int argc, char **argv) {
  bool smoke = false, mcp = false, smoke_input = false, smoke_docking = false,
       smoke_layout_form = false, smoke_feedback = false, smoke_assets = false;
  std::string capture, instance = "desktop";
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--smoke-assets") {
      smoke = true;
      smoke_assets = true;
    } else if (arg == "--smoke-feedback") {
      smoke = true;
      smoke_feedback = true;
    } else if (arg == "--smoke-layout-form") {
      smoke = true;
      smoke_layout_form = true;
    } else if (arg == "--smoke-docking") {
      smoke = true;
      smoke_docking = true;
    } else if (arg == "--smoke-input") {
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
    lyko::DockingAdapter docking;
    lyko::ShortcutBrowser shortcuts;
    lyko::AssetBrowser assets;
    lyko::ProjectMenu projects;
    lyko::SourceAuditView source_audit;
    bool focus_palette = false;
    lyko::DesktopSmokeHarness harness(smoke_input, smoke_docking,
                                      smoke_layout_form, smoke_feedback,
                                      smoke_assets);
    std::jthread protocol;
    if (mcp) {
      protocol = std::jthread(
          [&](std::stop_token stop) { lyko::run_mcp_stream(service, stop); });
    }
    J frame, palette = A{}, catalog = A{};
    bool ready = false, refresh = true, snapshot_pending = false,
         palette_open = false, focus_search = false;
    std::uint64_t revision = 0, editing = 0;
    std::string status = "Ready", style, active, editing_name;
    std::deque<lyko::QueuedCommand> outgoing;
    std::map<std::uint64_t, lyko::PendingCommand> pending;
    std::array<char, 256> search{}, rename{};
    std::optional<lyko::DesktopCommandForm> command_form;
    std::optional<lyko::DesktopTransactionForm> transaction_form;
    lyko::FormFeedback feedback;
    auto request = [&](const std::string &name, J args = O{},
                       bool mutation = true, bool from_form = false) {
      if (outgoing.size() >= 64) {
        status = "Too many pending commands; wait for completion.";
        if (from_form) {
          feedback.reject(
              feedback.generation(),
              lyko::error_json(revision, "queue_full", "command", status));
        }
        return false;
      }
      O object{{"instance", instance},
               {"command", name},
               {"arguments", std::move(args)}};
      if (mutation && ready) {
        object.emplace("expected_revision", revision);
      }
      outgoing.push_back(
          {std::move(object),
           from_form ? std::optional(feedback.generation()) : std::nullopt});
      return true;
    };
    const auto choose_command = [&](const J &item, const O &prefill = O{}) {
      const auto name = item.at("name").as<std::string>();
      active = name;
      O current;
      if (ready && name == "workspace.configure") {
        const auto &w = frame.at("workspace");
        for (auto key : {"scale_percent", "theme", "reduced_motion"}) {
          current.emplace(key, w.at(key));
        }
      }
      if (ready &&
          (name == "entity.rename" || name == "entity.duplicate" ||
           name == "entity.delete" || name == "entity.reparent" ||
           name == "entity.reparent_world" || name == "prefab.export")) {
        const auto &entity = frame.at("selection").at("entity");
        if (std::holds_alternative<O>(entity.value)) {
          current.emplace("id", entity.at("id"));
          if (name == "entity.rename" || name == "entity.duplicate") {
            current.emplace("name", entity.at("name"));
          }
        }
      }
      if (ready && (name == "transform.set" || name == "transform.query" ||
                    name == "transform.world")) {
        const auto &entity = frame.at("selection").at("entity");
        if (std::holds_alternative<O>(entity.value)) {
          const auto id = entity.at("id");
          current.emplace("id", id);
          if (name == "transform.set") {
            const auto key = std::to_string(id.as<std::uint64_t>());
            const auto values = frame.at("transforms").contains(key)
                                    ? frame.at("transforms").at(key)
                                    : lyko::transform_json(lyko::Transform{});
            for (const auto &[field, value] : values.as<O>()) {
              current.emplace(field, value);
            }
          }
        }
      }
      if (ready && name == "layout.replace") {
        current.emplace("layout", frame.at("workspace").at("layout"));
      }
      for (const auto &[key, value] : prefill) {
        current.insert_or_assign(key, value);
      }
      transaction_form.reset();
      feedback.reset();
      command_form.reset();
      if (name == "transaction.execute") {
        transaction_form.emplace(catalog);
      } else {
        command_form.emplace(item, current);
      }
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
    if (smoke_assets) {
      request("asset.register",
              O{{"path", "prefabs/Tree.json"}, {"kind", "prefab"}}, false);
    }
    unsigned frames = 0;
    ImVec2 create_center{}, name_center{}, search_center{}, settings_center{},
        shortcuts_center{}, assets_center{};
    while (platform.begin_frame()) {
      while (auto done = queue.try_pop()) {
        docking.completed(done->ticket);
        const auto operation = pending.at(done->ticket);
        const auto &name = operation.name;
        if (operation.form) {
          feedback.complete(*operation.form, done->ticket, done->response);
        }
        pending.erase(done->ticket);
        if (name == "editor.snapshot") {
          snapshot_pending = false;
        }
        const auto &response = done->response;
        if (!response.at("ok").as<bool>()) {
          status = name + ": " +
                   response.at("error").at("message").as<std::string>();
          refresh = true;
          continue;
        }
        if (name == "editor.snapshot") {
          frame = response.at("data");
          docking.snapshot();
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
        if (request("editor.snapshot", O{}, false)) {
          snapshot_pending = true;
          refresh = false;
        }
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
      const lyko::CommandHints hints(
          catalog, ready ? frame.at("workspace").at("keymap") : J(O{}));
      const auto key_for = [&](const std::string &name) {
        return hints.shortcut(name);
      };
      const auto open_form = [&](const std::string &name, const O &values) {
        for (const auto &descriptor : catalog.as<A>()) {
          if (descriptor.at("name").as<std::string>() == name) {
            choose_command(descriptor, values);
            std::snprintf(search.data(), search.size(), "%s", name.c_str());
            request("palette.search", O{{"query", name}}, false);
            palette_open = true;
            focus_search = false;
            focus_palette = true;
            break;
          }
        }
      };
      if (ImGui::BeginMainMenuBar()) {
        ImGui::TextUnformatted("LYKOWARE");
        ImGui::SameLine();
        ImGui::TextDisabled(ready && frame.at("project").at("active") !=
                                         J(nullptr)
                                ? "/ Project open"
                                : "/ No active project");
        if (ready) {
          if (const auto action = projects.draw(frame.at("project"))) {
            open_form(action->command, action->arguments);
          }
        }
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
          ImGui::TextDisabled("Starter slot presets reset custom docking");
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
          if (ImGui::MenuItem("Reset starter layout")) {
            request("layout.reset");
          }
          ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Commands", key_for("palette.search").c_str())) {
          palette_open = true;
          focus_search = true;
          active.clear();
          command_form.reset();
          transaction_form.reset();
          feedback.reset();
          request("palette.search", O{{"query", std::string(search.data())}},
                  false);
        }
        if (ready) {
          if (ImGui::MenuItem("Assets")) {
            assets.open = true;
          }
          assets_center = ImVec2(
              (ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * .5f,
              (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * .5f);
          if (ImGui::MenuItem("Shortcuts")) {
            shortcuts.open = true;
          }
          shortcuts_center = ImVec2(
              (ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * 0.5f,
              (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f);
        }
        ImGui::SameLine();
        ImGui::TextDisabled(" | rev %llu | %s",
                            static_cast<unsigned long long>(revision),
                            status.c_str());
        ImGui::EndMainMenuBar();
      }
      auto root = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
      if (ready) {
        const auto &w = frame.at("workspace");
        docking.sync(root, w.at("layout"), revision);
        std::set<std::string> hidden;
        for (const auto &panel : w.at("layout").at("hidden").as<A>()) {
          hidden.insert(panel.as<std::string>());
        }
        auto visible = [&](const char *id) { return !hidden.contains(id); };
        if (visible("scene")) {
          ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_HorizontalScrollbar);
          lyko::panel_heading(lyko::Icon::entity, "SCENE COLLECTION");
          if (lyko::icon_button(lyko::Icon::add, "Create entity")) {
            request("entity.create", O{{"name", "New entity"}});
          }
          create_center = ImVec2(
              (ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * 0.5f,
              (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f);
          lyko::command_tooltip(hints, "entity.create");
          ImGui::Separator();
          for (const auto &entity :
               frame.at("scene_tree").at("nodes").as<A>()) {
            auto id = entity.at("id").as<std::uint64_t>();
            const float indent =
                static_cast<float>(entity.at("depth").as<std::uint64_t>()) *
                ImGui::GetStyle().IndentSpacing;
            if (indent > 0) {
              ImGui::Indent(indent);
            }
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
              lyko::command_tooltip(hints, "selection.set");
              if (ImGui::MenuItem("Rename...",
                                  key_for("entity.rename").c_str())) {
                open_form("entity.rename",
                          O{{"id", id}, {"name", entity.at("name")}});
              }
              lyko::command_tooltip(hints, "entity.rename");
              if (ImGui::MenuItem("Duplicate...",
                                  key_for("entity.duplicate").c_str())) {
                open_form("entity.duplicate",
                          O{{"id", id}, {"name", entity.at("name")}});
              }
              lyko::command_tooltip(hints, "entity.duplicate");
              if (ImGui::MenuItem("Export prefab...",
                                  key_for("prefab.export").c_str())) {
                open_form("prefab.export", O{{"id", id}});
              }
              lyko::command_tooltip(hints, "prefab.export");
              if (ImGui::MenuItem("Reparent...",
                                  key_for("entity.reparent").c_str())) {
                open_form("entity.reparent",
                          O{{"id", id}, {"parent", entity.at("parent")}});
              }
              lyko::command_tooltip(hints, "entity.reparent");
              if (ImGui::MenuItem("Reparent (preserve world)...",
                                  key_for("entity.reparent_world").c_str())) {
                open_form("entity.reparent_world",
                          O{{"id", id}, {"parent", entity.at("parent")}});
              }
              lyko::command_tooltip(hints, "entity.reparent_world");
              if (ImGui::MenuItem("Delete", key_for("entity.delete").c_str())) {
                request("entity.delete", O{{"id", id}});
              }
              lyko::command_tooltip(hints, "entity.delete");
              ImGui::Separator();
              if (ImGui::MenuItem("Undo", key_for("history.undo").c_str())) {
                request("history.undo");
              }
              lyko::command_tooltip(hints, "history.undo");
              if (ImGui::MenuItem("Redo", key_for("history.redo").c_str())) {
                request("history.redo");
              }
              lyko::command_tooltip(hints, "history.redo");
              ImGui::EndPopup();
            }
            ImGui::PopID();
            if (indent > 0) {
              ImGui::Unindent(indent);
            }
          }
          ImGui::End();
        }
        if (visible("inspector")) {
          ImGui::Begin("Inspector");
          lyko::panel_heading(lyko::Icon::properties, "ENTITY PROPERTIES");
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
            if (lyko::icon_button(lyko::Icon::check, "Apply name")) {
              request("entity.rename",
                      O{{"id", id}, {"name", std::string(rename.data())}});
            }
            lyko::command_tooltip(hints, "entity.rename");
            ImGui::Separator();
            const auto transform_key = std::to_string(id);
            const auto local =
                frame.at("transforms").contains(transform_key)
                    ? lyko::parse_transform(
                          frame.at("transforms").at(transform_key))
                    : lyko::Transform{};
            const auto &v = local.values;
            ImGui::Text("Position  %.4g  %.4g  %.4g", v[0], v[1], v[2]);
            ImGui::Text("Rotation  %.4g  %.4g  %.4g deg", v[3], v[4], v[5]);
            ImGui::Text("Scale     %.4g  %.4g  %.4g", v[6], v[7], v[8]);
            if (ImGui::Button("Edit local transform...")) {
              open_form("transform.set", O{{"id", id}});
            }
            lyko::command_tooltip(hints, "transform.set");
            if (ImGui::Button("Inspect world matrix...")) {
              open_form("transform.world", O{{"id", id}});
            }
            lyko::command_tooltip(hints, "transform.world");
            const auto undo_key = key_for("history.undo");
            ImGui::TextDisabled("Enter to apply / Undo: %s",
                                undo_key.empty() ? "Unbound"
                                                 : undo_key.c_str());
          }
          ImGui::End();
        }
        if (visible("viewport")) {
          ImGui::Begin("Viewport");
          bool playing =
              frame.at("scene").at("runtime").at("playing").as<bool>();
          if (lyko::icon_button(playing ? lyko::Icon::stop : lyko::Icon::play,
                                playing ? "Stop" : "Play")) {
            request(playing ? "play.stop" : "play.start");
          }
          lyko::command_tooltip(hints, playing ? "play.stop" : "play.start");
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
          lyko::panel_heading(lyko::Icon::console, "RUNTIME LOGS");
          for (const auto &message : frame.at("logs").at("messages").as<A>()) {
            ImGui::TextUnformatted(message.as<std::string>().c_str());
          }
          ImGui::Separator();
          ImGui::TextWrapped("%s", status.c_str());
          ImGui::End();
        }
        if (visible("history")) {
          ImGui::Begin("History");
          lyko::panel_heading(lyko::Icon::history, "TRANSACTION HISTORY");
          ImGui::BeginDisabled(!frame.at("history").at("can_undo").as<bool>());
          if (lyko::icon_button(lyko::Icon::undo, "Undo")) {
            request("history.undo");
          }
          ImGui::EndDisabled();
          lyko::command_tooltip(hints, "history.undo");
          ImGui::SameLine();
          ImGui::BeginDisabled(!frame.at("history").at("can_redo").as<bool>());
          if (lyko::icon_button(lyko::Icon::redo, "Redo")) {
            request("history.redo");
          }
          ImGui::EndDisabled();
          lyko::command_tooltip(hints, "history.redo");
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
      if (ready) {
        if (auto action = assets.draw(frame.at("assets"))) {
          open_form(action->command, action->arguments);
        }
        if (auto command = shortcuts.draw(hints)) {
          open_form("keymap.bind",
                    O{{"command", *command}, {"shortcut", key_for(*command)}});
        }
      }
      if (palette_open && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        if (active.empty()) {
          palette_open = false, focus_search = false;
        } else {
          active.clear();
          command_form.reset();
          transaction_form.reset();
          feedback.reset();
        }
      }
      if (palette_open) {
        ImGui::SetNextWindowSize(ImVec2(660, 520), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
                                ImGuiCond_Appearing, ImVec2(0.5f, 0.35f));
        if (focus_palette) {
          ImGui::SetNextWindowFocus();
          focus_palette = false;
        }
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
          transaction_form.reset();
          feedback.reset();
          request("palette.search", O{{"query", std::string(search.data())}},
                  false);
        }
        search_center = ImVec2(
            (ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * 0.5f,
            (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f);
        if (active.empty()) {
          for (const auto &item : palette.as<A>()) {
            auto name = item.at("name").as<std::string>();
            if (ImGui::Selectable((name + "    " + key_for(name)).c_str())) {
              choose_command(item);
            }
            if (name == "workspace.configure" ||
                name == "transaction.execute" || name == "layout.replace") {
              settings_center = ImVec2(
                  (ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) *
                      0.5f,
                  (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) *
                      0.5f);
            }
            lyko::command_tooltip(hints, name);
          }
        } else {
          ImGui::TextUnformatted(active.c_str());
          ImGui::BeginDisabled(feedback.busy());
          std::optional<J> arguments;
          if (transaction_form) {
            arguments = transaction_form->draw();
          } else if (command_form) {
            arguments = command_form->draw(true, feedback.error());
          }
          ImGui::EndDisabled();
          if (arguments && feedback.begin(*arguments)) {
            request(active, *arguments, true, true);
          }
          if (transaction_form) {
            feedback.reconcile(transaction_form->draft_arguments());
          } else if (command_form) {
            feedback.reconcile(command_form->draft_arguments());
          }
          lyko::draw_form_feedback(feedback);
          std::optional<std::pair<std::string, O>> next_source_form;
          if (active == "project.create" && feedback.result() &&
              feedback.result()->at("ok").as<bool>()) {
            const auto &project = feedback.result()->at("data").at("project");
            ImGui::TextWrapped("Project created: %s",
                               project.at("root").as<std::string>().c_str());
            if (ImGui::Button("Open created project...")) {
              next_source_form =
                  std::pair{"project.open", O{{"path", project.at("path")}}};
            }
          }
          if (active == "asset.audit" && feedback.result() &&
              feedback.result()->at("ok").as<bool>() && command_form) {
            const auto draft = command_form->draft_arguments();
            if (draft) {
              if (const auto action =
                      source_audit.draw(feedback.result()->at("data"),
                                        draft->at("root").as<std::string>(),
                                        feedback.generation())) {
                next_source_form =
                    std::pair{action->command, action->arguments};
              }
            }
          }
          if ((active == "asset.validate" || active == "asset.check") &&
              feedback.result() && feedback.result()->at("ok").as<bool>() &&
              command_form) {
            const auto draft = command_form->draft_arguments();
            const auto &data = feedback.result()->at("data");
            if (draft && data.contains("sha256")) {
              O values{{"id", draft->at("id")},
                       {"root", draft->at("root")},
                       {"sha256", data.at("sha256")}};
              if (ImGui::Button("Recheck this version")) {
                next_source_form = std::pair{"asset.check", values};
              }
              if (data.at("kind").as<std::string>() == "prefab" &&
                  ImGui::Button("Instantiate validated bytes...")) {
                values.emplace("parent", std::uint64_t{0});
                next_source_form =
                    std::pair{"asset.instantiate_checked", values};
              }
            }
          }
          if (active == "asset.instantiate_checked" && feedback.error() &&
              feedback.error()->at("code").as<std::string>() ==
                  "asset_source_changed" &&
              command_form) {
            const auto draft = command_form->draft_arguments();
            if (draft && ImGui::Button("Validate current source...")) {
              next_source_form =
                  std::pair{"asset.validate", O{{"id", draft->at("id")},
                                                {"root", draft->at("root")}}};
            }
          }
          if (next_source_form) {
            open_form(next_source_form->first, next_source_form->second);
          }
          ImGui::Spacing();
          if (ImGui::Button("Back")) {
            active.clear();
            command_form.reset();
            transaction_form.reset();
            feedback.reset();
            focus_search = true;
          }
        }
        ImGui::End();
      }
      if (ready) {
        if (auto change = docking.finish()) {
          J request =
              O{{"instance", instance},
                {"command", "layout.replace"},
                {"expected_revision", change->expected_revision},
                {"arguments",
                 O{{"layout", lyko::dock_layout_json(change->layout)}}}};
          auto accepted = queue.try_submit(std::move(request));
          if (accepted) {
            pending.emplace(
                *accepted.ticket,
                lyko::PendingCommand{"layout.replace", std::nullopt});
            docking.submitted(*accepted.ticket);
          } else {
            docking.rejected(accepted.error->message);
          }
        }
        if (auto message = docking.take_message(); !message.empty()) {
          status = message;
        }
      }
      while (!outgoing.empty()) {
        auto submitted = queue.try_submit(outgoing.front().message);
        if (!submitted) {
          if (submitted.error->code == "queue_busy" ||
              submitted.error->code == "queue_full") {
            break;
          }
          status = submitted.error->message;
          if (outgoing.front().form) {
            feedback.reject(*outgoing.front().form,
                            lyko::error_json(revision, submitted.error->code,
                                             submitted.error->field,
                                             submitted.error->message));
          }
          outgoing.pop_front();
          continue;
        }
        pending.emplace(
            *submitted.ticket,
            lyko::PendingCommand{
                outgoing.front().message.at("command").as<std::string>(),
                outgoing.front().form});
        if (outgoing.front().form) {
          feedback.accepted(*outgoing.front().form, *submitted.ticket);
        }
        outgoing.pop_front();
      }
      if (smoke && ready) {
        harness.step(frames, {platform, root, frame, revision, create_center,
                              name_center, search_center, settings_center,
                              command_form ? &*command_form : nullptr,
                              transaction_form ? &*transaction_form : nullptr,
                              &shortcuts, shortcuts_center, &feedback, &assets,
                              assets_center, palette_open, status});
      }
      ++frames;
      if (mcp && frames % 30 == 0) {
        refresh = true;
      }
      bool finish = smoke && frames >= harness.target_frames() && ready &&
                    pending.empty() && outgoing.empty();
      platform.end_frame(finish ? capture : "");
      if (finish) {
        harness.validate(frame);
        if (smoke_docking) {
          std::cerr
              << "Dock interaction passed: resize, merge, split, undo/redo, "
                 "active tab and floating-drop rollback\n";
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
