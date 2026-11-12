#include "mcp_server.hpp"

namespace lyko {
namespace {
using O = Json::Object;
using A = Json::Array;
Json success(const Json& id, Json result) { return O{{"jsonrpc", "2.0"}, {"id", id}, {"result", std::move(result)}}; }
}
Json rpc_error(Json id, std::int64_t code, const std::string& message) {
    return O{{"jsonrpc", "2.0"}, {"id", std::move(id)}, {"error", O{{"code", code}, {"message", message}}}};
}
std::optional<Json> McpServer::receive(const Json& message) {
    Json id;
    bool notification = false;
    try {
        if (!std::holds_alternative<O>(message.value) || !message.contains("jsonrpc") || !message.contains("method")
            || !std::holds_alternative<std::string>(message.at("jsonrpc").value)
            || !std::holds_alternative<std::string>(message.at("method").value)) {
            return rpc_error(nullptr, -32600, "Invalid JSON-RPC envelope");
        }
        if (message.contains("id")) {
            id = message.at("id");
            if (!std::holds_alternative<std::string>(id.value) && !std::holds_alternative<std::uint64_t>(id.value)
                && !std::holds_alternative<std::int64_t>(id.value)) { return rpc_error(nullptr, -32600, "Invalid request ID"); }
        } else { notification = true; }
        if (message.at("jsonrpc").as<std::string>() != "2.0") { return rpc_error(nullptr, -32600, "Expected JSON-RPC 2.0"); }
        const auto method = message.at("method").as<std::string>();
        const Json params = message.contains("params") ? message.at("params") : Json(O{});
        if (notification) {
            if (method == "notifications/initialized" && initialized_) { ready_ = true; }
            return {};
        }
        if (method == "ping") { return success(id, O{}); }
        if (method == "initialize") {
            if (initialized_) { return rpc_error(id, -32600, "Session already initialized"); }
            const auto version = params.at("protocolVersion").as<std::string>();
            (void)params.at("capabilities").as<O>();
            (void)params.at("clientInfo").at("name").as<std::string>();
            (void)params.at("clientInfo").at("version").as<std::string>();
            // For unsupported versions return a supported one, per negotiation.
            const std::string selected = version == "2024-11-05" ? version : "2025-06-18";
            structured_ = selected == "2025-06-18";
            initialized_ = true;
            return success(id, O{{"protocolVersion", selected}, {"capabilities", O{{"tools", O{{"listChanged", false}}}}},
                                 {"serverInfo", O{{"name", "lykoware-editor-mock"}, {"version", "0.1.0"}}}});
        }
        if (!ready_) { return rpc_error(id, -32600, "Initialize the session and send notifications/initialized first"); }
        if (method == "tools/list") {
            if (params.contains("cursor")) { return rpc_error(id, -32602, "No pagination cursor is supported"); }
            A tools;
            const auto descriptors = service_.descriptors();
            for (const auto& descriptor : descriptors.as<A>()) {
                O properties{{"instance", O{{"type", "string"}, {"minLength", std::uint64_t{1}}}},
                             {"expected_revision", O{{"type", "integer"}, {"minimum", std::uint64_t{0}}}},
                             {"arguments", descriptor.at("inputSchema")}};
                A required{"instance"};
                if (!descriptor.at("inputSchema").at("required").as<A>().empty()) { required.emplace_back("arguments"); }
                tools.emplace_back(O{{"name", descriptor.at("name")}, {"description", descriptor.at("description")},
                                     {"inputSchema", O{{"type", "object"}, {"properties", std::move(properties)},
                                                        {"required", std::move(required)}, {"additionalProperties", false}}}});
            }
            return success(id, O{{"tools", std::move(tools)}});
        }
        if (method == "tools/call") {
            const auto name = params.at("name").as<std::string>();
            O request = params.at("arguments").as<O>();
            if (request.contains("command")) { return rpc_error(id, -32602, "Command is selected by the tool name"); }
            request.emplace("command", name);
            auto result = service_.call(request);
            O payload{{"content", A{O{{"type", "text"}, {"text", result.dump()}}}}, {"isError", !result.at("ok").as<bool>()}};
            if (structured_) { payload.emplace("structuredContent", result); }
            return success(id, std::move(payload));
        }
        return rpc_error(id, -32601, "Method not found");
    } catch (const std::invalid_argument& error) {
        if (notification) { return {}; }
        return rpc_error(id, -32602, error.what());
    }
}
}
