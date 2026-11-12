#include "desktop/source_audit.hpp"
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
  lyko::SourceAuditView view;
  J::Array entries;
  for (std::uint64_t id = 1; id <= 4096; ++id) {
    entries.emplace_back(
        O{{"id", id},
          {"path", "prefabs/tree-" + std::to_string(id) + ".json"},
          {"kind", "prefab"},
          {"status", "valid"},
          {"bytes", std::uint64_t{120}},
          {"entities", std::uint64_t{1}},
          {"sha256", std::string(64, 'a')}});
  }
  J report = O{{"entries", entries}};
  std::uint64_t generation = 0;
  std::optional<lyko::AuditAction> action;
  auto frame = [&](const std::string &capture = std::string{}) {
    check(platform.begin_frame(), "window stays open");
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(1050, 850));
    ImGui::Begin("Source report test");
    if (auto next = view.draw(report, "/project", generation)) {
      action = std::move(next);
    }
    ImGui::End();
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
  check(view.test_rendered_rows() > 0 && view.test_rendered_rows() < 50,
        "large report clips rows");
  click(view.test_search_center());
  platform.test_text("4096");
  frame();
  frame();
  check(view.test_rendered_rows() == 1,
        "search reaches final entry beyond JSON preview");
  click(view.test_row_center());
  check(view.test_selected() == 4096, "native pointer selects filtered entry");
  click(view.test_instantiate_center());
  check(action && action->command == "asset.instantiate_checked" &&
            action->arguments.at("id").as<std::uint64_t>() == 4096 &&
            action->arguments.at("sha256").as<std::string>() ==
                std::string(64, 'a'),
        "native checked action carries audited bytes");
  action.reset();
  click(view.test_validate_center());
  check(action && action->command == "asset.validate",
        "native validate navigation");
  // Replacing results re-resolves the selected row, never a retained row
  // pointer.
  auto changed = entries.back().as<O>();
  changed["status"] = "invalid";
  changed["error"] = O{{"code", "invalid_asset_source"},
                       {"field", "path"},
                       {"message", "Source missing"}};
  report = O{{"entries", J::Array{changed}}};
  action.reset();
  frame();
  click(view.test_instantiate_center());
  check(!action, "new invalid observation disables prior checked action");
  if (argc == 2) {
    frame(argv[1]);
  }
  ++generation;
  frame();
  check(!view.test_selected(), "new form clears obsolete selection");
  std::cout << "Passed native audit clipping, search, selection, actions and "
               "result replacement\n";
}
