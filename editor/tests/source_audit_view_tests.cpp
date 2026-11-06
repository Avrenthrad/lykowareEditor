#include "ui/form_feedback.hpp"
#include "ui/source_audit.hpp"
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
int main() {
  const J prefab = O{{"id", std::uint64_t{4096}},
                     {"path", "trees/Oak.json"},
                     {"kind", "prefab"},
                     {"status", "valid"},
                     {"sha256", std::string(64, 'a')}};
  const J bad = O{{"id", std::uint64_t{1}},
                  {"path", "Broken.json"},
                  {"kind", "scene"},
                  {"status", "invalid"},
                  {"error", O{{"message", "Unsupported schema version"}}}};
  const J skipped = O{{"id", std::uint64_t{2}},
                      {"path", "oak.gltf"},
                      {"kind", "mesh"},
                      {"status", "skipped"}};
  check(lyko::audit_row_matches(prefab, "OAK", "valid") &&
            lyko::audit_row_matches(prefab, "4096", "") &&
            !lyko::audit_row_matches(prefab, "oak", "invalid") &&
            lyko::audit_row_matches(bad, "SCHEMA", "invalid"),
        "search by path/ID/diagnostic with status filter");
  const auto action = lyko::audit_row_action(prefab, "/project", true);
  check(action && action->command == "asset.instantiate_checked" &&
            action->arguments.at("sha256") == prefab.at("sha256") &&
            action->arguments.at("id") == prefab.at("id") &&
            action->arguments.at("root") == J("/project"),
        "checked form preserves selected identity, root and observation");
  check(!lyko::audit_row_action(bad, "/project", true) &&
            !lyko::audit_row_action(skipped, "/project", true) &&
            !lyko::audit_row_action(skipped, "/project", false),
        "invalid and unsupported sources cannot offer checked consumption");
  check(lyko::audit_row_action(bad, "/project", false)->command ==
            "asset.validate",
        "failed sources can be revalidated");
  // The full response remains available beyond the generic bounded JSON
  // preview.
  J::Array entries;
  for (std::uint64_t i = 1; i < 4096; ++i) {
    auto row = bad.as<O>();
    row["id"] = i;
    entries.emplace_back(std::move(row));
  }
  entries.push_back(prefab);
  const J result = O{{"ok", true},
                     {"revision", std::uint64_t{0}},
                     {"data", O{{"entries", entries}}}};
  lyko::FormFeedback feedback;
  const J args = O{{"root", "/project"}};
  check(feedback.begin(args), "begin audit");
  feedback.accepted(feedback.generation(), 1);
  feedback.complete(feedback.generation(), 1, result);
  check(feedback.preview().size() < 4200 &&
            feedback.result()->at("data").at("entries").as<J::Array>().back() ==
                prefab,
        "full report survives preview truncation");
  feedback.reconcile(J(O{{"root", "/other-project"}}));
  check(!feedback.result(), "edited roots remove obsolete report actions");
  std::cout
      << "Passed audit navigation, source guards and full-report lifecycle\n";
}
