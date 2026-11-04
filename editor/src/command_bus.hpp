#pragma once
#include "backend/engine_bridge.hpp"
#include "ui/workspace.hpp"
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace lyko {
using EntityId = std::uint64_t;
struct CreateEntity { std::string name; };
struct RenameEntity { EntityId id; std::string name; };
struct Play {};
struct Stop {};
struct SendInput { std::string value; };
struct Document {
    std::map<EntityId, std::string> entities;
    EntityId next_id = 1;
    RuntimeState runtime;
    Workspace workspace;
    bool operator==(const Document&) const = default;
};
struct ReplaceWorkspace { Workspace workspace; };
struct ConfigureWorkspace { std::uint64_t scale_percent; std::string theme; bool reduced_motion; };
struct SetPanelDock { std::string panel; std::string dock; };
struct ResizeDock { std::string dock; std::uint64_t percent; };
struct BindShortcut { std::string command; std::string shortcut; };
struct SelectEntity { EntityId id; }; // Zero clears selection.
struct ReplaceScene { Document document; };
using Mutation = std::variant<CreateEntity, RenameEntity, Play, Stop, SendInput, ReplaceScene, ConfigureWorkspace, SetPanelDock, ResizeDock, BindShortcut, SelectEntity, ReplaceWorkspace>;
struct Error { std::string code; std::string field; std::string message; };
struct Result {
    std::uint64_t revision;
    std::optional<Error> error;
    explicit operator bool() const { return !error; }
};
class CommandBus {
public:
    explicit CommandBus(std::string instance, const EngineBridge& bridge);
    Result execute(const std::string& instance, std::optional<std::uint64_t> expected,
                   const std::vector<Mutation>& mutations);
    Result undo(const std::string& instance, std::optional<std::uint64_t> expected = {});
    Result redo(const std::string& instance, std::optional<std::uint64_t> expected = {});
    std::pair<Document, std::uint64_t> snapshot() const;
private:
    struct Change { Document before; Document after; };
    std::optional<Error> validate_target(const std::string&, std::optional<std::uint64_t>) const;
    Result history(const std::string&, std::optional<std::uint64_t>, bool forward);
    const std::string instance_;
    const EngineBridge& bridge_;
    mutable std::mutex mutex_;
    Document document_;
    std::vector<Change> history_;
    std::size_t cursor_ = 0;
    std::uint64_t revision_ = 0;
};
}
