#pragma once
#include "command_bus.hpp"
#include "json.hpp"
#include "document_tracking.hpp"

namespace lyko {
// Sole named-command entry point for all frontends. Serializes a complete call,
// including target/revision checks, queries and response snapshots.
class CommandService {
public:
    CommandService(std::string instance, const EngineBridge& bridge);
    Json call(const Json& request);
    Json descriptors() const;
private:
    std::string instance_;
    CommandBus bus_;
    DocumentTracking tracking_;
    std::mutex mutex_;
};
Json document_json(const Document& document);
Json error_json(std::uint64_t revision, const std::string& code,
                const std::string& field, const std::string& message);
}
