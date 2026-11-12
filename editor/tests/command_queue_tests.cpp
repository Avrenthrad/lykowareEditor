#include "ui/command_queue.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <semaphore>

using namespace std::chrono_literals;
void check(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
lyko::Json request(const std::string& command, lyko::Json arguments = lyko::Json::Object{},
                   const std::string& instance = "test") {
    return lyko::Json::Object{{"instance", instance}, {"command", command}, {"arguments", std::move(arguments)}};
}
class GateBridge final : public lyko::EngineBridge {
public:
    mutable std::binary_semaphore entered{0}, resume{0};
    lyko::RuntimeState play(const lyko::RuntimeState& state) const override {
        entered.release();
        if (!resume.try_acquire_for(5s)) { throw std::runtime_error("Test backend was not released"); }
        auto next = state; next.playing = true; return next;
    }
    lyko::RuntimeState stop(const lyko::RuntimeState& state) const override { return state; }
    lyko::RuntimeState input(const lyko::RuntimeState& state, const std::string&) const override { return state; }
};
lyko::Submission submit(lyko::CommandQueue& queue, const lyko::Json& input) {
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    for (;;) {
        auto result = queue.try_submit(input);
        if (result || result.error->code != "queue_busy") { return result; }
        check(std::chrono::steady_clock::now() < deadline, "submission did not become available");
        std::this_thread::yield();
    }
}
lyko::Completion pop(lyko::CommandQueue& queue) {
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    for (;;) {
        if (auto completion = queue.try_pop()) { return std::move(*completion); }
        check(std::chrono::steady_clock::now() < deadline, "completion timed out");
        std::this_thread::yield();
    }
}
bool ok(const lyko::Json& response) { return response.at("ok").as<bool>(); }
int main() {
    GateBridge bridge;
    lyko::CommandService service("test", bridge);
    {
        lyko::CommandQueue queue(service, 2);
        const auto first = submit(queue, request("play.start"));
        check(static_cast<bool>(first), "first request accepted");
        check(bridge.entered.try_acquire_for(5s), "backend entered");
        // The worker is blocked inside the backend; these calls must still work.
        const auto second = submit(queue, request("editor.snapshot"));
        check(static_cast<bool>(second), "UI can enqueue while backend is blocked");
        check(!queue.try_pop(), "poll does not wait for backend");
        const auto full = submit(queue, request("scene.query"));
        check(!full && full.error->code == "queue_full", "in-flight work counts toward capacity");
        bridge.resume.release();
        const auto a = pop(queue), b = pop(queue);
        check(a.ticket == *first.ticket && b.ticket == *second.ticket, "FIFO ticket correlation");
        check(ok(a.response) && ok(b.response) && b.response.at("revision").as<std::uint64_t>() == 1, "snapshot observes prior completion");
        const auto wrong = submit(queue, request("play.stop", {}, "other"));
        check(static_cast<bool>(wrong), "wrong-instance request reaches service validation");
        const auto rejected = pop(queue);
        check(!ok(rejected.response) && rejected.response.at("error").at("code").as<std::string>() == "instance_mismatch", "worker preserves structured errors");
    }
    GateBridge closing_bridge;
    lyko::CommandService closing_service("test", closing_bridge);
    {
        lyko::CommandQueue queue(closing_service, 2);
        check(static_cast<bool>(submit(queue, request("play.start"))), "closing test started");
        check(closing_bridge.entered.try_acquire_for(5s), "closing backend entered");
        check(static_cast<bool>(submit(queue, request("entity.create", lyko::Json::Object{{"name", "Cancelled"}}))), "pending write queued");
        queue.close();
        auto closed = submit(queue, request("scene.query"));
        check(!closed && closed.error->code == "queue_closed", "closed queue rejects new work");
        closing_bridge.resume.release();
        check(ok(pop(queue).response), "in-flight work completes during shutdown");
        auto cancelled = pop(queue).response;
        check(!ok(cancelled) && cancelled.at("error").at("code").as<std::string>() == "cancelled", "pending write cancelled");
    }
    auto scene = closing_service.call(request("scene.query"));
    check(scene.at("data").at("entities").as<lyko::Json::Array>().empty(), "cancelled request never mutates document");
    lyko::MockEngineBridge mock;
    lyko::CommandService bounded_service("test", mock);
    lyko::CommandQueue bounded(bounded_service, 1);
    auto large = submit(bounded, request("entity.create", lyko::Json::Object{{"name", std::string(1024 * 1024, 'x')}}));
    check(!large && large.error->code == "request_too_large", "oversized requests rejected before queueing");
    const auto accepted = submit(bounded, request("scene.query"));
    check(static_cast<bool>(accepted), "completed-result bound first request");
    // Wait for the result to be ready without collecting it. Capacity must not
    // be released merely because the worker finished.
    const auto ready_deadline = std::chrono::steady_clock::now() + 5s;
    for (;;) {
        if (auto status = bounded.try_status(); status && status->ready == 1) {
            check(status->outstanding == 1 && status->running == 0 && status->queued == 0, "completed status counts");
            break;
        }
        check(std::chrono::steady_clock::now() < ready_deadline, "result did not become ready");
        std::this_thread::yield();
    }
    auto no_capacity = submit(bounded, request("scene.query"));
    check(!no_capacity && no_capacity.error->code == "queue_full", "unpolled results count toward capacity");
    check(pop(bounded).ticket == *accepted.ticket, "poll frees capacity");
    check(static_cast<bool>(submit(bounded, request("scene.query"))), "capacity reusable after polling");
    check(ok(pop(bounded).response), "subsequent request completes");
    std::cout << "Passed blocked-backend UI submission/polling, FIFO, bounds, errors and shutdown cancellation\n";
}
