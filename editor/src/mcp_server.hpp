#pragma once
#include "command_service.hpp"

namespace lyko {
class McpServer {
public:
    explicit McpServer(CommandService& service) : service_(service) {}
    std::optional<Json> receive(const Json& message);
private:
    CommandService& service_;
    bool initialized_ = false;
    bool ready_ = false;
    bool structured_ = false;
};
Json rpc_error(Json id, std::int64_t code, const std::string& message);
}
