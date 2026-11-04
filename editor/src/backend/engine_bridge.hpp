#pragma once
#include <string>
#include <vector>

namespace lyko {
struct RuntimeState {
    bool playing = false;
    std::vector<std::string> inputs;
    bool operator==(const RuntimeState&) const = default;
};
// Pure value transformation: implementations must not mutate an external runtime.
// A real asynchronous engine adapter needs a separately reviewed commit protocol.
class EngineBridge {
public:
    virtual ~EngineBridge() = default;
    virtual RuntimeState play(const RuntimeState&) const = 0;
    virtual RuntimeState stop(const RuntimeState&) const = 0;
    virtual RuntimeState input(const RuntimeState&, const std::string&) const = 0;
};
class MockEngineBridge final : public EngineBridge {
public:
    RuntimeState play(const RuntimeState& state) const override {
        auto next = state; next.playing = true; return next;
    }
    RuntimeState stop(const RuntimeState& state) const override {
        auto next = state; next.playing = false; return next;
    }
    RuntimeState input(const RuntimeState& state, const std::string& value) const override {
        auto next = state; next.inputs.push_back(value); return next;
    }
};
}
