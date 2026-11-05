#include "command_service.hpp"
#include "ui/command_form.hpp"
#include "ui/transaction_form.hpp"
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
int main() {
  lyko::MockEngineBridge backend;
  lyko::CommandService service("test", backend);
  const auto all = service.descriptors();
  const auto descriptor = [&](const std::string &name) -> const J & {
    for (const auto &item : all.as<J::Array>()) {
      if (item.at("name").as<std::string>() == name) {
        return item;
      }
    }
    throw std::invalid_argument("Missing command");
  };
  lyko::CommandForm settings(
      descriptor("workspace.configure").at("inputSchema"));
  check(settings.errors().empty(), "settings defaults valid");
  check(settings.value("scale_percent").as<std::uint64_t>() == 100,
        "registry default scale");
  settings.set("scale_percent", std::uint64_t{74});
  check(settings.errors().contains("scale_percent"), "minimum checked");
  bool rejected = false;
  try {
    (void)settings.arguments();
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  check(rejected, "invalid draft cannot submit");
  settings.set("scale_percent", std::uint64_t{251});
  check(settings.errors().contains("scale_percent"), "maximum checked");
  settings.set("scale_percent", std::uint64_t{150});
  settings.set("theme", "unknown");
  check(settings.errors().contains("theme"), "enum checked");
  settings.set("theme", "high_contrast");
  settings.set("reduced_motion", "true");
  check(settings.errors().contains("reduced_motion"), "boolean type checked");
  settings.set("reduced_motion", true);
  check(settings.errors().empty(), "corrected draft valid");
  auto applied =
      service.call(request("workspace.configure", settings.arguments()));
  check(applied.at("ok").as<bool>(), "typed draft accepted by service");
  auto state = service.call(request("workspace.query"));
  check(state.at("data").at("scale_percent").as<std::uint64_t>() == 150,
        "same bus state");
  lyko::CommandForm create(descriptor("entity.create").at("inputSchema"));
  check(create.errors().empty(), "entity default valid");
  create.set("name", "");
  check(create.errors().contains("name"), "required string checked");
  std::string unicode;
  for (unsigned i = 0; i < 33; ++i) {
    unicode += "\xf0\x9f\x98\x80";
  }
  create.set("name", unicode);
  check(create.errors().contains("name"), "UTF-8 bytes match backend bound");
  create.set("name", std::string("\xc0\xaf"));
  check(create.errors().contains("name"), "malformed UTF-8 rejected");
  create.set("name", "Player");
  check(service.call(request("entity.create", create.arguments()))
            .at("ok")
            .as<bool>(),
        "entity form mutation succeeds");
  lyko::CommandForm rename(descriptor("entity.rename").at("inputSchema"));
  rename.set("id", std::uint64_t{18446744073709551615ULL});
  rename.set("name", "Maximum ID");
  check(rename.errors().empty(),
        "uint64 precision preserved without float coercion");
  rename.set("id", std::int64_t{-1});
  check(rename.errors().contains("id"), "signed ID rejected");
  rename.set("id", 1.5);
  check(rename.errors().contains("id"), "fractional ID rejected");
  lyko::CommandForm bind(descriptor("keymap.bind").at("inputSchema"));
  bind.set("command", "unknown.command");
  check(bind.errors().contains("command"), "registered command enum used");
  bind.set("command", "history.undo");
  bind.set("shortcut", "");
  check(bind.errors().empty(), "explicit unbind accepted");
  lyko::CommandForm batch(descriptor("transaction.execute").at("inputSchema"));
  batch.set("commands", "[]");
  check(batch.errors().contains("commands"), "array type checked");
  batch.set("commands", J::Array{});
  check(batch.errors().empty(),
        "array draft shape accepted; domain validation remains on bus");
  bool unknown = false;
  try {
    batch.set("typo", true);
  } catch (const std::invalid_argument &) {
    unknown = true;
  }
  check(unknown, "unknown fields cannot enter draft");
  const auto invalid_unicode = service.call(
      request("entity.create", O{{"name", std::string("\xc0\xaf")}}));
  check(!invalid_unicode.at("ok").as<bool>(),
        "named C++ service rejects malformed UTF-8 before writing");
  lyko::CommandForm layout_form(descriptor("layout.replace").at("inputSchema"));
  layout_form.set("layout", "not an object");
  check(layout_form.errors().contains("layout"),
        "layout draft object type checked");
  const auto tree = service.call(request("layout.query")).at("data");
  layout_form.set("layout", tree);
  check(service.call(request("layout.replace", layout_form.arguments()))
            .at("ok")
            .as<bool>(),
        "layout form dispatch shares authoritative validation");
  check(descriptor("layout.replace")
                .at("inputSchema")
                .at("properties")
                .at("layout")
                .at("properties")
                .at("nodes")
                .at("maxProperties")
                .as<std::uint64_t>() == 63,
        "discovery publishes nested dock schema");
  const auto unchanged = service.call(request("workspace.query"));
  check(unchanged.at("revision").as<std::uint64_t>() == 2,
        "draft edits do not mutate document or history");
  lyko::TransactionForm builder(all);
  check(builder.choices().contains("entity.create") &&
            !builder.choices().contains("scene.query") &&
            !builder.choices().contains("transaction.execute") &&
            !builder.choices().contains("history.undo") &&
            !builder.choices().contains("scene.save"),
        "builder offers only batchable mutations");
  const auto rejected_batch = [&](const J::Array &rows) {
    try {
      (void)builder.arguments(rows);
    } catch (const std::invalid_argument &) {
      return true;
    }
    return false;
  };
  check(rejected_batch({}), "empty batch blocked");
  check(rejected_batch({O{{"command", "scene.query"}, {"arguments", O{}}}}),
        "queries cannot enter builder");
  check(rejected_batch({O{{"command", "entity.rename"},
                          {"arguments", O{{"id", std::uint64_t{1}}}}}}),
        "missing fields rejected instead of silently defaulted");
  check(rejected_batch(
            {O{{"command", "entity.create"}, {"arguments", O{{"name", ""}}}}}),
        "invalid child draft blocked");
  J::Array rows{
      O{{"command", "entity.create"}, {"arguments", O{{"name", "First"}}}},
      O{{"command", "entity.create"}, {"arguments", O{{"name", "Second"}}}}};
  check(rejected_batch(J::Array(65, rows.front())),
        "builder row bound enforced");
  std::swap(rows[0], rows[1]);
  const auto draft = builder.arguments(rows);
  check(
      service.call(request("scene.query")).at("revision").as<std::uint64_t>() ==
          2,
      "assembling and reordering does not commit");
  check(service.call(request("transaction.execute", draft)).at("ok").as<bool>(),
        "assembled batch commits through shared service");
  auto entities = service.call(request("scene.query"))
                      .at("data")
                      .at("entities")
                      .as<J::Array>();
  check(entities.size() == 3 &&
            entities[1].at("name").as<std::string>() == "Second" &&
            entities[2].at("name").as<std::string>() == "First",
        "batch preserves user row order");
  check(service.call(request("history.undo")).at("ok").as<bool>() &&
            service.call(request("scene.query"))
                    .at("data")
                    .at("entities")
                    .as<J::Array>()
                    .size() == 1,
        "one undo removes entire assembled batch");
  check(service.call(request("history.redo")).at("ok").as<bool>(),
        "batch redo succeeds");
  rows.erase(rows.begin());
  rows.emplace_back(
      O{{"command", "entity.rename"},
        {"arguments", O{{"id", std::uint64_t{9999}}, {"name", "Missing"}}}});
  const auto before_failure = service.call(request("editor.snapshot"));
  check(!service.call(request("transaction.execute", builder.arguments(rows)))
             .at("ok")
             .as<bool>(),
        "domain errors remain authoritative on bus");
  check(service.call(request("editor.snapshot")) == before_failure,
        "failed assembled batch preserves state and history");
  std::cout << "Passed schema defaults, typed limits/enums, UTF-8 bounds, "
               "exact IDs and authoritative dispatch\n";
}
