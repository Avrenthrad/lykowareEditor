#include "command_service.hpp"
#include "mcp_server.hpp"
#include <iostream>

int main(int argc, char** argv) {
    if ((argc != 3 && argc != 4) || std::string(argv[1]) != "--instance" || std::string(argv[2]).empty() || (argc == 4 && std::string(argv[3]) != "--mcp")) {
        std::cerr << "Usage: lyko --instance NAME < requests.jsonl (optional --mcp)\n"; return 2;
    }
    lyko::MockEngineBridge backend;
    lyko::CommandService service(argv[2], backend);
    const bool mcp = argc == 4;
    lyko::McpServer server(service);
    bool failed = false;
    // Bound memory while reading, including rejected oversized messages.
    for (;;) {
        std::string line; bool oversized = false; char c;
        while (std::cin.get(c) && c != '\n') {
            if (line.size() < 1024 * 1024) { line += c; } else { oversized = true; }
        }
        if (line.empty() && !oversized && !std::cin) { break; }
        lyko::Json result;
        try {
            if (oversized) { throw std::invalid_argument("Request exceeds 1 MiB"); }
            auto message = lyko::Json::parse(line);
            if (mcp) { auto reply = server.receive(message); if (!reply) { continue; } result = *reply; }
            else { result = service.call(message); }
        } catch (const std::invalid_argument& error) {
            result = mcp ? lyko::rpc_error(nullptr, -32700, error.what()) : lyko::error_json(0, "parse_error", "request", error.what());
        }
        if (!mcp) { failed = !result.at("ok").as<bool>() || failed; }
        std::cout << result.dump() << '\n' << std::flush;
        if (!std::cin) { break; }
    }
    return failed ? 1 : 0;
}
