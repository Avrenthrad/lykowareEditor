#pragma once
#include <string>
#include <vector>
#include <utility>

namespace lyko {
struct RuntimeState {
    bool playing = false;
    std::vector<std::string> inputs;
    std::vector<std::string> logs;
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
    static void log(RuntimeState& state, std::string message) {
        if (state.logs.size() == 256) { state.logs.erase(state.logs.begin()); }
        state.logs.push_back(std::move(message));
    }
public:
    RuntimeState play(const RuntimeState& state) const override {
        auto next = state;
        if (!next.playing) { next.playing = true; log(next, "mock: play started"); }
        return next;
    }
    RuntimeState stop(const RuntimeState& state) const override {
        auto next = state;
        if (next.playing) { next.playing = false; log(next, "mock: play stopped"); }
        return next;
    }
    RuntimeState input(const RuntimeState& state, const std::string& value) const override {
        auto next = state; next.inputs.push_back(value); log(next, "mock: input " + value); return next;
    }
};
}
