#pragma once
#include "command_service.hpp"
#include <stop_token>
namespace lyko {
// Native stdio polling allows the desktop to stop an idle MCP client on
// shutdown. The caller owns the thread and keeps the service alive until it has
// joined.
void run_mcp_stream(CommandService &service, std::stop_token stop);
} // namespace lyko
