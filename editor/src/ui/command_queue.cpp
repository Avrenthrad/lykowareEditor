#include "command_queue.hpp"
#include <limits>
#include <stdexcept>

namespace lyko {
CommandQueue::CommandQueue(CommandService& service, std::size_t capacity)
    : service_(service), capacity_(capacity) {
    if (capacity == 0 || capacity > 256) { throw std::invalid_argument("Queue capacity must be between 1 and 256"); }
    worker_ = std::jthread([this](std::stop_token stop) { run(stop); });
}
CommandQueue::~CommandQueue() { close(); }
Submission CommandQueue::try_submit(Json request) {
    // Transport decoding/serialization runs on the caller; command execution and
    // all backend/file work run on the worker. Inputs are capped at 1 MiB.
    try {
        if (request.dump().size() > 1024 * 1024) {
            return {{}, Error{"request_too_large", "request", "Request exceeds 1 MiB."}};
        }
    } catch (const std::invalid_argument&) {
        return {{}, Error{"invalid_request", "request", "Request contains invalid JSON values."}};
    }
    std::unique_lock lock(mutex_, std::try_to_lock);
    if (!lock) { return {{}, Error{"queue_busy", "queue", "Retry submission on a later frame."}}; }
    if (closed_) { return {{}, Error{"queue_closed", "queue", "Queue is closed."}}; }
    if (outstanding_ == capacity_) { return {{}, Error{"queue_full", "queue", "Poll completed work before submitting more."}}; }
    if (next_ticket_ == std::numeric_limits<std::uint64_t>::max()) { return {{}, Error{"ticket_exhausted", "queue", "No queue tickets remain."}}; }
    const auto ticket = next_ticket_;
    pending_.push_back({ticket, std::move(request)});
    ++next_ticket_; ++outstanding_;
    lock.unlock(); wake_.notify_one();
    return {ticket, {}};
}
std::optional<Completion> CommandQueue::try_pop() {
    std::unique_lock lock(mutex_, std::try_to_lock);
    if (!lock || completed_.empty()) { return {}; }
    auto completion = std::move(completed_.front()); completed_.pop_front();
    --outstanding_;
    return completion;
}
std::optional<QueueStatus> CommandQueue::try_status() {
    std::unique_lock lock(mutex_, std::try_to_lock);
    if (!lock) { return {}; }
    return QueueStatus{pending_.size(), outstanding_ - pending_.size() - completed_.size(),
                       completed_.size(), outstanding_, closed_};
}
void CommandQueue::close() {
    { std::lock_guard lock(mutex_); closed_ = true; }
    wake_.notify_all();
}
void CommandQueue::run(std::stop_token stop) {
    for (;;) {
        Request request;
        bool cancelled;
        {
            std::unique_lock lock(mutex_);
            wake_.wait(lock, stop, [&] { return closed_ || !pending_.empty(); });
            if (pending_.empty() && (closed_ || stop.stop_requested())) { return; }
            if (pending_.empty()) { continue; }
            request = std::move(pending_.front()); pending_.pop_front();
            cancelled = closed_ || stop.stop_requested();
        }
        Json response;
        if (cancelled) { response = error_json(0, "cancelled", "queue", "Request was not started before shutdown."); }
        else {
            try { response = service_.call(request.message); }
            catch (const std::exception&) { response = error_json(0, "operation_failed", "queue", "Unexpected command execution failure."); }
        }
        { std::lock_guard lock(mutex_); completed_.push_back({request.ticket, std::move(response)}); }
    }
}
}
