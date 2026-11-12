#include "command_bus.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <atomic>
#include <thread>

void check(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
class FailingBridge final : public lyko::EngineBridge {
public:
    lyko::RuntimeState play(const lyko::RuntimeState&) const override { throw std::runtime_error("backend failed"); }
    lyko::RuntimeState stop(const lyko::RuntimeState& s) const override { return s; }
    lyko::RuntimeState input(const lyko::RuntimeState& s, const std::string&) const override { return s; }
};
int main() {
    lyko::MockEngineBridge backend;
    lyko::CommandBus bus("test", backend);
    check(!bus.execute("other", {}, {lyko::CreateEntity{"A"}}), "instance isolation");
    check(!bus.execute("test", 5, {lyko::CreateEntity{"A"}}), "stale revision");
    check(!bus.execute("test", {}, {}), "empty transaction rejected");
    check(!bus.execute("test", {}, {lyko::CreateEntity{"A"}, lyko::RenameEntity{99, "B"}}), "invalid batch rejected");
    check(bus.snapshot().first.entities.empty() && bus.snapshot().second == 0, "batch rollback");
    check(static_cast<bool>(bus.execute("test", 0, {lyko::CreateEntity{"A"}, lyko::RenameEntity{1, "B"}})), "dependent batch succeeds");
    check(bus.snapshot().first.entities.at(1) == "B" && bus.snapshot().second == 1, "single revision per batch");
    check(static_cast<bool>(bus.undo("test")), "undo succeeds");
    check(bus.snapshot().first.entities.empty(), "undo full batch");
    check(static_cast<bool>(bus.redo("test")), "redo succeeds");
    check(bus.snapshot().first.entities.at(1) == "B", "redo full batch");
    check(!bus.execute("test", {}, {lyko::SendInput{"jump"}}), "input requires play");
    check(static_cast<bool>(bus.execute("test", {}, {lyko::Play{}, lyko::SendInput{"jump"}, lyko::Stop{}})), "mock feedback transaction");
    check(!bus.snapshot().first.runtime.playing && bus.snapshot().first.runtime.inputs.size() == 1, "mock feedback state");
    check(static_cast<bool>(bus.undo("test")), "undo runtime");
    check(bus.snapshot().first.runtime.inputs.empty(), "runtime restored");
    check(static_cast<bool>(bus.execute("test", {}, {lyko::CreateEntity{"C"}})), "branch edit");
    check(!bus.redo("test"), "branch invalidates redo");
    auto before = bus.snapshot();
    check(static_cast<bool>(bus.execute("test", {}, {lyko::Stop{}})), "no-op succeeds");
    check(bus.snapshot() == before, "no-op keeps revision");
    lyko::CommandBus bounded("bounded", backend);
    std::vector<lyko::Mutation> batch{lyko::Play{}};
    for (unsigned i = 0; i < 1024; ++i) { batch.emplace_back(lyko::SendInput{"right"}); }
    check(static_cast<bool>(bounded.execute("bounded", {}, batch)), "bounded mock batch");
    check(bounded.snapshot().first.runtime.logs.size() == 256, "logs bounded to 256");
    check(!bounded.execute("bounded", {}, {lyko::SendInput{"overflow"}}), "input limit enforced");
    check(bounded.snapshot().second == 1, "input limit keeps revision");
    check(static_cast<bool>(bounded.undo("bounded")), "undo bounded batch");
    check(!bounded.execute("bounded", {}, {lyko::RenameEntity{99, "missing"}}), "failed edit after undo");
    check(static_cast<bool>(bounded.redo("bounded")), "failed edit preserves redo");
    lyko::CommandBus concurrent("concurrent", backend);
    std::atomic<unsigned> winners{0};
    {
        std::vector<std::jthread> workers;
        for (unsigned i = 0; i < 4; ++i) {
            workers.emplace_back([&] {
                if (concurrent.execute("concurrent", 0, {lyko::CreateEntity{"Winner"}})) { ++winners; }
            });
        }
    }
    check(winners == 1 && concurrent.snapshot().first.entities.size() == 1, "concurrent stale writes serialize");
    lyko::CommandBus layout_bus("layout", backend);
    lyko::DockLayout cycle;cycle.root="root";
    cycle.nodes.emplace("root",lyko::DockNode{"horizontal",5000,"root","leaf",{},""});cycle.nodes.emplace("leaf",lyko::DockNode{});
    auto bad_layout=layout_bus.execute("layout",{}, {lyko::CreateEntity{"Rollback"},lyko::ReplaceDockLayout{cycle}});
    check(!bad_layout&&bad_layout.error->code=="invalid_layout","typed layout mutation validates before commit");
    check(layout_bus.snapshot().first.entities.empty()&&layout_bus.revision()==0&&!layout_bus.history_snapshot().cursor,"typed layout failure rolls back batch and history");
    FailingBridge failing;
    lyko::CommandBus broken("test", failing);
    auto failure = broken.execute("test", {}, {lyko::CreateEntity{"A"}, lyko::Play{}});
    check(!failure && failure.error->code == "backend_failure", "structured backend failure");
    check(broken.snapshot().first.entities.empty() && broken.snapshot().second == 0, "exception rollback");
    std::cout << "Passed transaction, history, target isolation and mock backend checks\n";
}
