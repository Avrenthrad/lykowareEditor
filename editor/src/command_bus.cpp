#include "command_bus.hpp"
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace lyko {
CommandBus::CommandBus(std::string instance, const EngineBridge& bridge)
    : instance_(std::move(instance)), bridge_(bridge) {
    if (instance_.empty()) { throw std::invalid_argument("instance must not be empty"); }
}
std::optional<Error> CommandBus::validate_target(const std::string& instance,
                                                std::optional<std::uint64_t> expected) const {
    if (instance != instance_) { return Error{"instance_mismatch", "instance", "Explicit target must match this editor instance."}; }
    if (expected && *expected != revision_) { return Error{"stale_revision", "revision", "Read the latest revision before retrying."}; }
    return {};
}
Result CommandBus::execute(const std::string& instance, std::optional<std::uint64_t> expected,
                           const std::vector<Mutation>& mutations, std::vector<std::string> command_names) {
    std::lock_guard lock(mutex_);
    if (auto error = validate_target(instance, expected)) { return {revision_, error}; }
    if (mutations.empty()) { return {revision_, Error{"empty_transaction", "commands", "Supply at least one mutation."}}; }
    if (command_names.empty()) { command_names.assign(mutations.size(), "mutation"); }
    if (command_names.size() != mutations.size()) { return {revision_, Error{"invalid_history_metadata", "commands", "History metadata must match transaction size."}}; }
    for (const auto& name : command_names) {
        if (name.empty() || name.size() > 128) { return {revision_, Error{"invalid_history_metadata", "commands", "Invalid history command name."}}; }
    }
    auto candidate = document_;
    for (const auto& mutation : mutations) {
        std::optional<Error> error;
        try {
        error = std::visit([&](const auto& command) -> std::optional<Error> {
            using T = std::decay_t<decltype(command)>;
            if constexpr (std::is_same_v<T, CreateEntity> || std::is_same_v<T, RenameEntity> || std::is_same_v<T, DuplicateEntity>) {
                if (command.name.empty() || command.name.size() > 128) {
                    return Error{"invalid_name", "name", "Name must contain 1 to 128 bytes."};
                }
                if constexpr (std::is_same_v<T, CreateEntity>) {
                    if (candidate.next_id == std::numeric_limits<EntityId>::max()) {
                        return Error{"id_exhausted", "id", "No identifiers remain."};
                    }
                    candidate.entities.emplace(candidate.next_id++, command.name);
                } else if constexpr (std::is_same_v<T, DuplicateEntity>) {
                    if (!candidate.entities.contains(command.id)) { return Error{"not_found", "id", "Entity does not exist."}; }
                    if (candidate.next_id == std::numeric_limits<EntityId>::max()) { return Error{"id_exhausted", "id", "No identifiers remain."}; }
                    const auto id = candidate.next_id++;
                    candidate.entities.emplace(id, command.name);
                    candidate.workspace.selection = id;
                } else {
                    auto entity = candidate.entities.find(command.id);
                    if (entity == candidate.entities.end()) { return Error{"not_found", "id", "Entity does not exist."}; }
                    entity->second = command.name;
                }
            } else if constexpr (std::is_same_v<T, DeleteEntity>) {
                if (!candidate.entities.erase(command.id)) { return Error{"not_found", "id", "Entity does not exist."}; }
                if (candidate.workspace.selection == command.id) { candidate.workspace.selection.reset(); }
            } else if constexpr (std::is_same_v<T, ReplaceScene>) {
                if (command.document.next_id == 0) { return Error{"invalid_scene", "next_id", "Next ID must be positive."}; }
                for (const auto& [id, name] : command.document.entities) {
                    if (id == 0 || id >= command.document.next_id || name.empty() || name.size() > 128) {
                        return Error{"invalid_scene", "entities", "Invalid entity identifier or name."};
                    }
                }
                if (command.document.runtime.playing || !command.document.runtime.inputs.empty() || !command.document.runtime.logs.empty()) {
                    return Error{"invalid_scene", "runtime", "Loaded scenes must not contain runtime state."};
                }
                candidate.entities = command.document.entities;
                candidate.next_id = command.document.next_id;
                candidate.runtime = command.document.runtime;
                candidate.workspace.selection.reset();
            } else if constexpr (std::is_same_v<T, ReplaceDockLayout>) {
                if(auto error=dock_layout_error(command.layout)){return Error{"invalid_layout","layout",*error};}
                const auto current=effective_dock_layout(candidate.workspace);
                const auto normalized=canonical_dock_layout(command.layout);if(current!=normalized){candidate.workspace.layout=normalized;}
            } else if constexpr (std::is_same_v<T, ResetDockLayout>) {
                candidate.workspace.layout.reset();
            } else if constexpr (std::is_same_v<T, ReplaceWorkspace>) {
                if (auto message = workspace_error(command.workspace)) { return Error{"invalid_workspace", "workspace", *message}; }
                const auto selection = candidate.workspace.selection;
                candidate.workspace = command.workspace;
                candidate.workspace.selection = selection;
            } else if constexpr (std::is_same_v<T, ConfigureWorkspace>) {
                if (command.scale_percent < 75 || command.scale_percent > 250) {
                    return Error{"invalid_scale", "scale_percent", "UI scale must be between 75 and 250 percent."};
                }
                if (command.theme != "dark" && command.theme != "high_contrast") {
                    return Error{"invalid_theme", "theme", "Choose dark or high_contrast."};
                }
                candidate.workspace.scale_percent = command.scale_percent;
                candidate.workspace.theme = command.theme;
                candidate.workspace.reduced_motion = command.reduced_motion;
            } else if constexpr (std::is_same_v<T, SetPanelDock>) {
                auto panel = candidate.workspace.panels.find(command.panel);
                if (panel == candidate.workspace.panels.end()) { return Error{"unknown_panel", "panel", "Panel is not registered."}; }
                if (command.dock != "left" && command.dock != "right" && command.dock != "center" && command.dock != "bottom" && command.dock != "hidden") {
                    return Error{"invalid_dock", "dock", "Choose left, right, center, bottom or hidden."};
                }
                panel->second = command.dock;
                candidate.workspace.layout.reset();
            } else if constexpr (std::is_same_v<T, ResizeDock>) {
                if (!candidate.workspace.splits.contains(command.dock)) { return Error{"invalid_dock", "dock", "Only left, right and bottom splits can be resized."}; }
                if (command.percent < 10 || command.percent > 50) { return Error{"invalid_split", "percent", "Dock size must be 10 to 50 percent."}; }
                auto splits = candidate.workspace.splits;
                splits.at(command.dock) = command.percent;
                if (splits.at("left") + splits.at("right") > 70) { return Error{"invalid_split", "percent", "Keep at least 30 percent of the width for the center."}; }
                candidate.workspace.splits = std::move(splits);
                candidate.workspace.layout.reset();
            } else if constexpr (std::is_same_v<T, BindShortcut>) {
                if (command.command.empty() || command.command.size() > 128 || !valid_shortcut(command.shortcut)) {
                    return Error{"invalid_shortcut", "shortcut", "Use canonical Ctrl+Alt+Shift+Super modifiers followed by a supported key; empty unbinds."};
                }
                for (const auto& [name, shortcut] : candidate.workspace.keymap) {
                    if (name != command.command && !command.shortcut.empty() && shortcut == command.shortcut) {
                        return Error{"shortcut_conflict", "shortcut", "Shortcut is already bound; unbind it first."};
                    }
                }
                if (command.shortcut.empty()) { candidate.workspace.keymap.erase(command.command); }
                else { candidate.workspace.keymap[command.command] = command.shortcut; }
            } else if constexpr (std::is_same_v<T, SelectEntity>) {
                if (command.id == 0) { candidate.workspace.selection.reset(); }
                else {
                    if (!candidate.entities.contains(command.id)) { return Error{"not_found", "id", "Entity does not exist."}; }
                    candidate.workspace.selection = command.id;
                }
            } else if constexpr (std::is_same_v<T, Play>) {
                candidate.runtime = bridge_.play(candidate.runtime);
            } else if constexpr (std::is_same_v<T, Stop>) {
                candidate.runtime = bridge_.stop(candidate.runtime);
            } else if constexpr (std::is_same_v<T, SendInput>) {
                if (!candidate.runtime.playing) { return Error{"not_playing", "value", "Start play mode before sending input."}; }
                if (command.value.empty() || command.value.size() > 128) { return Error{"invalid_input", "value", "Input must contain 1 to 128 bytes."}; }
                if (candidate.runtime.inputs.size() >= 1024) { return Error{"input_limit", "value", "Mock session accepts at most 1024 inputs; undo or load a scene to reset."}; }
                candidate.runtime = bridge_.input(candidate.runtime, command.value);
            }
            return {};
        }, mutation);
        } catch (const std::exception&) {
            return {revision_, Error{"backend_failure", "backend", "Backend operation failed; transaction was not committed."}};
        }
        if (error) { return {revision_, error}; }
    }
    if (candidate == document_) { return {revision_, {}}; }
    // Prepare every potentially allocating operation before the commit swaps.
    auto next_history = history_;
    next_history.erase(next_history.begin() + static_cast<std::ptrdiff_t>(cursor_), next_history.end());
    next_history.push_back({document_, candidate, revision_ + 1, std::move(command_names)});
    using std::swap;
    swap(document_, candidate);
    history_.swap(next_history);
    cursor_ = history_.size();
    ++revision_;
    return {revision_, {}};
}
Result CommandBus::history(const std::string& instance, std::optional<std::uint64_t> expected, bool forward) {
    std::lock_guard lock(mutex_);
    if (auto error = validate_target(instance, expected)) { return {revision_, error}; }
    if (forward ? cursor_ == history_.size() : cursor_ == 0) {
        return {revision_, Error{"history_empty", "history", "No transaction available in this direction."}};
    }
    auto candidate = forward ? history_[cursor_].after : history_[cursor_ - 1].before;
    using std::swap;
    swap(document_, candidate);
    if (forward) { ++cursor_; } else { --cursor_; }
    return {++revision_, {}};
}
Result CommandBus::undo(const std::string& instance, std::optional<std::uint64_t> expected) { return history(instance, expected, false); }
Result CommandBus::redo(const std::string& instance, std::optional<std::uint64_t> expected) { return history(instance, expected, true); }
std::pair<Document, std::uint64_t> CommandBus::snapshot() const {
    std::lock_guard lock(mutex_);
    return {document_, revision_};
}
std::uint64_t CommandBus::revision() const {
    std::lock_guard lock(mutex_); return revision_;
}
HistorySnapshot CommandBus::history_unlocked() const {
    HistorySnapshot result{cursor_, {}};
    result.entries.reserve(history_.size());
    for (std::size_t i = 0; i < history_.size(); ++i) {
        result.entries.push_back({history_[i].revision, history_[i].commands, i < cursor_});
    }
    return result;
}
HistorySnapshot CommandBus::history_snapshot() const {
    std::lock_guard lock(mutex_); return history_unlocked();
}
EditorSnapshot CommandBus::editor_snapshot() const {
    std::lock_guard lock(mutex_); return {document_, revision_, history_unlocked()};
}

}
