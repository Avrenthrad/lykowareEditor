#include "desktop/project_menu.hpp"
#include <cstdlib>
#include <iostream>
using J = lyko::Json;
using O = J::Object;
void check(bool value, const char *message) {
  if (!value) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}
int main(int argc, char **argv) {
  lyko::DesktopPlatform platform;
  lyko::ProjectMenu menu;
  lyko::ProjectManifest manifest;
  manifest.project_id = std::string(32, 'a');
  manifest.name = "Fixture Studio";
  const lyko::ProjectSession session{manifest, "/example/lykoware.toml",
                                     "/example"};
  const auto state =
      lyko::project_state_json(lyko::ProjectState{session, {session}});
  std::optional<lyko::ProjectMenuAction> action;
  auto frame = [&](const std::string &capture = std::string{}) {
    check(platform.begin_frame(), "window stays open");
    if (ImGui::BeginMainMenuBar()) {
      if (auto result = menu.draw(state)) {
        action = std::move(result);
      }
      ImGui::EndMainMenuBar();
    }
    platform.end_frame(capture);
  };
  auto click = [&](ImVec2 center) {
    platform.test_pointer(center, true);
    frame();
    platform.test_pointer(center, false);
    frame();
    frame();
  };
  frame();
  frame();
  frame();
  click(menu.test_menu_center());
  frame();
  click(menu.test_create_center());
  check(action && action->command == "project.create" &&
            action->arguments.empty(),
        "create item navigates shared form");
  action.reset();
  click(menu.test_menu_center());
  frame();
  click(menu.test_open_center());
  check(action && action->command == "project.open" &&
            action->arguments.empty(),
        "open item navigates shared form");
  action.reset();
  click(menu.test_menu_center());
  frame();
  click(menu.test_recent_center());
  check(action && action->command == "project.open" &&
            action->arguments.at("path") == J(session.path),
        "recent item passes captured path to authoritative open form");
  action.reset();
  click(menu.test_menu_center());
  frame();
  click(menu.test_load_center());
  check(action && action->command == "project.recent.load",
        "load history uses shared form");
  action.reset();
  click(menu.test_menu_center());
  frame();
  click(menu.test_save_center());
  check(action && action->command == "project.recent.save",
        "save history uses shared form");
  click(menu.test_menu_center());
  frame();
  if (argc == 2) {
    frame(argv[1]);
  }
  std::cout << "Passed native project menu and recent-path form navigation\n";
}
