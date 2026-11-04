#include "command_service.hpp"
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3 || std::string(argv[1]) != "--instance" || std::string(argv[2]).empty()) {
        std::cerr << "Usage: lyko --instance NAME < requests.jsonl\n"; return 2;
    }
    lyko::MockEngineBridge backend;
    lyko::CommandService service(argv[2], backend);
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
            result = service.call(lyko::Json::parse(line));
        } catch (const std::invalid_argument& error) {
            result = lyko::error_json(0, "parse_error", "request", error.what());
        }
        failed = !result.at("ok").as<bool>() || failed;
        std::cout << result.dump() << '\n' << std::flush;
        if (!std::cin) { break; }
    }
    return failed ? 1 : 0;
}
