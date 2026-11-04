#include "command_bus.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

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
    FailingBridge failing;
    lyko::CommandBus broken("test", failing);
    auto failure = broken.execute("test", {}, {lyko::CreateEntity{"A"}, lyko::Play{}});
    check(!failure && failure.error->code == "backend_failure", "structured backend failure");
    check(broken.snapshot().first.entities.empty() && broken.snapshot().second == 0, "exception rollback");
    std::cout << "Passed transaction, history, target isolation and mock backend checks\n";
}
