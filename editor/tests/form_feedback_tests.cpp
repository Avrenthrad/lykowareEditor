#include "command_service.hpp"
#include "ui/form_feedback.hpp"
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
int main() {
  lyko::MockEngineBridge bridge;
  lyko::CommandService service("test", bridge);
  lyko::FormFeedback feedback;
  const J args = O{{"id", std::uint64_t{99}}, {"name", "Missing"}};
  feedback.reset();
  const auto first = feedback.generation();
  check(feedback.begin(args) && !feedback.begin(args),
        "duplicate pending submission blocked");
  feedback.accepted(first, 41);
  const auto failure = service.call(O{
      {"instance", "test"}, {"command", "entity.rename"}, {"arguments", args}});
  feedback.complete(first, 40, failure);
  check(feedback.busy() && !feedback.result(), "wrong ticket ignored");
  feedback.complete(first, 41, failure);
  check(!feedback.busy() && feedback.error() &&
            feedback.error()->at("field").as<std::string>() == "id",
        "structured field failure retained");
  feedback.reconcile(args);
  check(feedback.error(), "error stays with unchanged offending draft");
  feedback.reconcile(std::nullopt);
  check(!feedback.result(), "invalid edited draft clears obsolete result");
  feedback.reset();
  const auto second = feedback.generation();
  check(feedback.begin(args), "new draft can submit");
  feedback.accepted(first, 42);
  feedback.complete(first, 41, failure);
  check(feedback.busy() && !feedback.ticket() && !feedback.result(),
        "late reply from old draft ignored");
  feedback.accepted(second, 43);
  const J success = O{{"ok", true},
                      {"revision", std::uint64_t{5}},
                      {"data", O{{"saved", true}}}};
  feedback.complete(second, 43, success);
  check(feedback.result() && !feedback.error() &&
            feedback.preview().find("saved") != std::string::npos,
        "success and query result preview retained");
  feedback.reconcile(J(O{{"id", std::uint64_t{1}}, {"name", "Corrected"}}));
  check(!feedback.result() && feedback.preview().empty(),
        "changed draft clears previous success");
  check(feedback.begin(args), "rejected submissions can begin");
  feedback.reject(second, lyko::error_json(5, "queue_full", "command",
                                           "Wait for completion."));
  check(!feedback.busy() &&
            feedback.error()->at("code").as<std::string>() == "queue_full",
        "pre-ticket rejection visible and retryable");
  check(feedback.begin(args), "retry begins only on explicit submission");
  feedback.accepted(second, 44);
  feedback.reject(second, failure);
  check(feedback.busy(), "local rejection cannot override accepted ticket");
  std::string unicode;
  for (unsigned i = 0; i < 2000; ++i) {
    unicode += "\xf0\x9f\x98\x80";
  }
  feedback.complete(
      second, 44,
      O{{"ok", true}, {"revision", std::uint64_t{6}}, {"data", unicode}});
  check(feedback.preview().size() < 4200 &&
            lyko::json_detail::valid_utf8(feedback.preview()) &&
            feedback.preview().find("truncated") != std::string::npos,
        "large Unicode result preview is bounded and valid");
  check(feedback.result()->at("data").as<std::string>() == unicode,
        "preview truncation preserves original response");
  feedback.reset();
  check(!feedback.busy() && !feedback.result() && !feedback.ticket(),
        "navigation resets local feedback");
  std::cout << "Passed request generations, ticket correlation, draft "
               "invalidation, rejection, retries and result bounds\n";
}
