#include "command_service.hpp"
#include "ui/layout_form.hpp"
#include <cstdlib>
#include <iostream>
using J = lyko::Json;
using O = J::Object;
void check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}
J request(const std::string &name, J args = O{}) {
  return O{
      {"instance", "test"}, {"command", name}, {"arguments", std::move(args)}};
}
std::string group(const lyko::LayoutForm &form, const std::string &panel) {
  for (const auto &[id, node] : form.layout().nodes) {
    if (std::find(node.panels.begin(), node.panels.end(), panel) !=
        node.panels.end()) {
      return id;
    }
  }
  throw std::invalid_argument("Panel is hidden");
}
int main() {
  lyko::MockEngineBridge backend;
  lyko::CommandService service("test", backend);
  lyko::LayoutForm form(service.call(request("layout.query")).at("data"));
  const auto original = form.value();
  const auto failed_edit = [&](auto operation) {
    const auto before = form.value();
    bool rejected = false;
    try {
      operation();
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    check(rejected && form.value() == before,
          "invalid edit preserves valid draft");
  };
  failed_edit([&] { form.resize("root", "horizontal", 499); });
  failed_edit([&] { form.resize("root", "diagonal", 5000); });
  failed_edit([&] { form.move("unknown", ""); });
  failed_edit([&] { form.move("scene", "root"); });
  failed_edit(
      [&] { form.divide("scene", group(form, "scene"), "horizontal", true); });
  auto malformed = original.as<O>();
  malformed.emplace("extra", true);
  failed_edit([&] { form.import(malformed); });
  form.resize("root", "vertical", 3456);
  check(form.layout().nodes.at("root").ratio == 3456 &&
            form.layout().nodes.at("root").axis == "vertical",
        "exact ratios and axis retained");
  form.move("inspector", group(form, "scene"));
  auto id = group(form, "scene");
  check(form.layout().nodes.at(id).active == "inspector",
        "moved panel becomes active");
  form.activate(id, "scene");
  check(form.layout().nodes.at(id).active == "scene", "active tab edited");
  failed_edit([&] { form.activate(id, "console"); });
  failed_edit([&] { form.reorder(id, 0, true); });
  form.reorder(id, 1, true);
  check(form.layout().nodes.at(id).panels.front() == "inspector",
        "tab order edited");
  const auto before_same = form.value();
  form.move("scene", id);
  check(form.value() == before_same, "same-group placement is a no-op");
  form.move("inspector", "");
  check(std::find(form.layout().hidden.begin(), form.layout().hidden.end(),
                  "inspector") != form.layout().hidden.end(),
        "hide panel");
  form.divide("inspector", group(form, "scene"), "horizontal", false);
  check(form.layout().hidden.empty() &&
            group(form, "inspector") != group(form, "scene"),
        "split restores hidden panel");
  check(!lyko::dock_layout_error(form.layout()),
        "structural edits preserve graph invariants");
  check(service.call(request("layout.query")).at("data") == original &&
            service.call(request("history.query"))
                    .at("revision")
                    .as<std::uint64_t>() == 0,
        "draft edits leave live layout and history untouched");
  check(service.call(request("layout.replace", O{{"layout", form.value()}}))
            .at("ok")
            .as<bool>(),
        "one submitted layout accepted by bus");
  check(service.call(request("history.query"))
                .at("data")
                .at("entries")
                .as<J::Array>()
                .size() == 1,
        "one history record");
  check(service.call(request("history.undo")).at("ok").as<bool>() &&
            service.call(request("layout.query")).at("data") == original,
        "one undo restores original layout");
  check(service.call(request("history.redo")).at("ok").as<bool>() &&
            service.call(request("layout.query")).at("data") == form.value(),
        "one redo restores edited tree");
  const auto roundtrip = lyko::LayoutForm(J::parse(form.value().dump()));
  check(roundtrip.value() == form.value(), "JSON roundtrip equal");
  for (const auto &panel :
       {"scene", "inspector", "viewport", "console", "history"}) {
    form.move(panel, "");
  }
  check(form.layout().nodes.size() == 1 && form.layout().hidden.size() == 5,
        "all-hidden canonical root");
  form.move("scene", "root");
  check(group(form, "scene") == "root" && form.layout().hidden.size() == 4,
        "restore into empty root");
  form.move("inspector", "root");
  form.divide("inspector", "root", "vertical", true);
  const auto &root = form.layout().nodes.at("root");
  check(root.axis == "vertical" &&
            form.layout().nodes.at(root.first).panels ==
                std::vector<std::string>{"inspector"} &&
            form.layout().nodes.at(root.second).panels ==
                std::vector<std::string>{"scene"},
        "split panel out of its own tab group with explicit first-child "
        "placement");
  std::cout << "Passed layout draft structure, validation, JSON, atomic "
               "submission and history\n";
}
