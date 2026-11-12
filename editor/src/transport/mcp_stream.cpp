#include "mcp_stream.hpp"
#include "mcp_server.hpp"
#include <array>
#include <chrono>
#include <iostream>
#include <thread>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <poll.h>
#include <unistd.h>
#endif
namespace lyko {
namespace {
// -1: EOF/error, 0: no data yet, >0: bytes read. Never waits for a full line.
int read_chunk(char *buffer, unsigned size) {
#ifdef _WIN32
  const auto input = GetStdHandle(STD_INPUT_HANDLE);
  const auto type = GetFileType(input);
  DWORD available = size;
  if (type == FILE_TYPE_PIPE) {
    if (!PeekNamedPipe(input, nullptr, 0, nullptr, &available, nullptr)) {
      return -1;
    }
    if (available == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(25));
      return 0;
    }
  } else if (type != FILE_TYPE_DISK) {
    return -1;
  }
  DWORD count = 0;
  return ReadFile(input, buffer, available < size ? available : size, &count,
                  nullptr) &&
                 count
             ? static_cast<int>(count)
             : -1;
#else
  pollfd descriptor{STDIN_FILENO, POLLIN, 0};
  const int ready = poll(&descriptor, 1, 25);
  if (ready < 0) {
    return errno == EINTR ? 0 : -1;
  }
  if (ready == 0) {
    return 0;
  }
  if (descriptor.revents & (POLLIN | POLLHUP)) {
    const auto count = read(STDIN_FILENO, buffer, size);
    if (count < 0 && (errno == EINTR || errno == EAGAIN)) {
      return 0;
    }
    return count > 0 ? static_cast<int>(count) : -1;
  }
  return -1;
#endif
}
} // namespace
void run_mcp_stream(CommandService &service, std::stop_token stop) {
#ifndef _WIN32
  // A disconnected agent must not terminate the desktop with SIGPIPE.
  std::signal(SIGPIPE, SIG_IGN);
#endif
  McpServer server(service);
  std::string line;
  bool oversized = false;
  const auto dispatch = [&] {
    std::optional<Json> reply;
    try {
      if (oversized) {
        throw std::invalid_argument("Request exceeds 1 MiB");
      }
      reply = server.receive(Json::parse(line));
    } catch (const std::invalid_argument &error) {
      reply = rpc_error(nullptr, -32700, error.what());
    }
    if (reply) {
      std::cout << reply->dump() << '\n' << std::flush;
      if (!std::cout) {
        return false;
      }
    }
    line.clear();
    oversized = false;
    return true;
  };
  std::array<char, 4096> buffer{};
  while (!stop.stop_requested()) {
    const int count =
        read_chunk(buffer.data(), static_cast<unsigned>(buffer.size()));
    if (count < 0) {
      if (!line.empty() || oversized) {
        dispatch();
      }
      return;
    }
    for (int i = 0; i < count; ++i) {
      const char c = buffer[static_cast<std::size_t>(i)];
      if (c == '\n') {
        if (!dispatch()) {
          return;
        }
      } else if (line.size() < 1024 * 1024) {
        line += c;
      } else {
        oversized = true;
      }
    }
  }
}
} // namespace lyko
