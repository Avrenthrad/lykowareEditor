#pragma once
#include "command_service.hpp"
#include <condition_variable>
#include <deque>
#include <thread>

namespace lyko {
struct Submission {
    std::optional<std::uint64_t> ticket;
    std::optional<Error> error;
    explicit operator bool() const { return ticket.has_value(); }
};
struct QueueStatus { std::size_t queued; std::size_t running; std::size_t ready; std::size_t outstanding; bool closed; };
struct Completion { std::uint64_t ticket; Json response; };
// A desktop adapter submits requests and polls completions; it never calls the
// synchronous service on its render thread. The service must outlive this queue.
class CommandQueue {
public:
    explicit CommandQueue(CommandService& service, std::size_t capacity = 64);
    ~CommandQueue();
    CommandQueue(const CommandQueue&) = delete;
    CommandQueue& operator=(const CommandQueue&) = delete;
    Submission try_submit(Json request);
    std::optional<Completion> try_pop();
    std::optional<QueueStatus> try_status();
    // Reject new work; cancel requests not started. An in-flight call completes.
    // Destruction joins that call and is intended for application shutdown only.
    void close();
private:
    struct Request { std::uint64_t ticket; Json message; };
    void run(std::stop_token stop);
    CommandService& service_;
    const std::size_t capacity_;
    std::mutex mutex_;
    std::condition_variable_any wake_;
    std::deque<Request> pending_;
    std::deque<Completion> completed_;
    std::size_t outstanding_ = 0;
    std::uint64_t next_ticket_ = 1;
    bool closed_ = false;
    // Destroy/join first, while all worker-accessed state is still alive.
    std::jthread worker_;
};
}
