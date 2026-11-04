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
                           const std::vector<Mutation>& mutations) {
    std::lock_guard lock(mutex_);
    if (auto error = validate_target(instance, expected)) { return {revision_, error}; }
    if (mutations.empty()) { return {revision_, Error{"empty_transaction", "commands", "Supply at least one mutation."}}; }
    auto candidate = document_;
    for (const auto& mutation : mutations) {
        std::optional<Error> error;
        try {
        error = std::visit([&](const auto& command) -> std::optional<Error> {
            using T = std::decay_t<decltype(command)>;
            if constexpr (std::is_same_v<T, CreateEntity> || std::is_same_v<T, RenameEntity>) {
                if (command.name.empty() || command.name.size() > 128) {
                    return Error{"invalid_name", "name", "Name must contain 1 to 128 bytes."};
                }
                if constexpr (std::is_same_v<T, CreateEntity>) {
                    if (candidate.next_id == std::numeric_limits<EntityId>::max()) {
                        return Error{"id_exhausted", "id", "No identifiers remain."};
                    }
                    candidate.entities.emplace(candidate.next_id++, command.name);
                } else {
                    auto entity = candidate.entities.find(command.id);
                    if (entity == candidate.entities.end()) { return Error{"not_found", "id", "Entity does not exist."}; }
                    entity->second = command.name;
                }
            } else if constexpr (std::is_same_v<T, Play>) {
                candidate.runtime = bridge_.play(candidate.runtime);
            } else if constexpr (std::is_same_v<T, Stop>) {
                candidate.runtime = bridge_.stop(candidate.runtime);
            } else if constexpr (std::is_same_v<T, SendInput>) {
                if (!candidate.runtime.playing) { return Error{"not_playing", "value", "Start play mode before sending input."}; }
                if (command.value.empty() || command.value.size() > 128) { return Error{"invalid_input", "value", "Input must contain 1 to 128 bytes."}; }
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
    next_history.push_back({document_, candidate});
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
}
