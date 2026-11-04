#include "command_bus.hpp"
#include <charconv>
#include <iomanip>
#include <iostream>
#include <sstream>

int main(int argc, char** argv) {
    if (argc != 3 || std::string(argv[1]) != "--instance" || std::string(argv[2]).empty()) {
        std::cerr << "Usage: lyko --instance NAME < commands.txt\n"; return 2;
    }
    const std::string instance = argv[2];
    lyko::MockEngineBridge backend;
    lyko::CommandBus bus(instance, backend);
    bool failed = false;
    for (std::string line; std::getline(std::cin, line);) {
        if (line.empty()) { continue; }
        std::istringstream stream(line);
        std::string command;
        stream >> command;
        lyko::Result result{bus.snapshot().second, {}};
        std::vector<lyko::Mutation> mutations;
        if (command == "entity.create") {
            std::string name; stream >> std::quoted(name);
            mutations.emplace_back(lyko::CreateEntity{name});
        } else if (command == "entity.rename") {
            std::string token, name; stream >> token >> std::quoted(name);
            lyko::EntityId id{};
            auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), id);
            if (error != std::errc{} || end != token.data() + token.size()) {
                result.error = lyko::Error{"invalid_id", "id", "Expected an unsigned integer."};
            } else { mutations.emplace_back(lyko::RenameEntity{id, name}); }
        } else if (command == "play.start") { mutations.emplace_back(lyko::Play{});
        } else if (command == "play.stop") { mutations.emplace_back(lyko::Stop{});
        } else if (command == "play.input") {
            std::string value; stream >> std::quoted(value); mutations.emplace_back(lyko::SendInput{value});
        } else if (command != "history.undo" && command != "history.redo" && command != "scene.query" && command != "commands.list") {
            result.error = lyko::Error{"unknown_command", "command", "Use commands.list to discover commands."};
        }
        const bool malformed = stream.fail();
        stream >> std::ws;
        if (malformed || !stream.eof() || stream.bad()) { result.error = lyko::Error{"invalid_arguments", "arguments", "Unexpected or malformed arguments."}; }
        if (!result.error) {
            if (!mutations.empty()) { result = bus.execute(instance, {}, mutations); }
            else if (command == "history.undo") { result = bus.undo(instance); }
            else if (command == "history.redo") { result = bus.redo(instance); }
            else if (command == "commands.list") {
                for (const auto& entry : lyko::commands()) { std::cout << entry.name << ' ' << entry.input << " : " << entry.description << '\n'; }
            } else if (command == "scene.query") {
                auto [state, revision] = bus.snapshot();
                std::cout << "mock=true playing=" << state.runtime.playing << " inputs=" << state.runtime.inputs.size() << '\n';
                for (const auto& [id, name] : state.entities) { std::cout << id << ' ' << std::quoted(name) << '\n'; }
                result.revision = revision;
            }
        }
        if (result.error) {
            failed = true;
            std::cout << "error " << result.error->code << ' ' << result.error->field << ' ' << std::quoted(result.error->message) << '\n';
        } else { std::cout << "ok revision=" << result.revision << '\n'; }
    }
    return failed ? 1 : 0;
}
