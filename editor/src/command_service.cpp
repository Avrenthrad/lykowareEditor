#include "command_service.hpp"
#include <algorithm>
#include <functional>
#include <stdexcept>

namespace lyko {
namespace {
using O = Json::Object;
using A = Json::Array;
struct Field { std::string name; std::string type; };
struct Entry {
    std::string name, description;
    std::vector<Field> fields;
    std::function<Mutation(const Json&)> decode;
};
const std::vector<Entry>& registry() {
    static const std::vector<Entry> entries = {
        {"entity.create", "Create a named entity with a stable ID.", {{"name", "string"}}, [](const Json& a) -> Mutation { return CreateEntity{a.at("name").as<std::string>()}; }},
        {"entity.rename", "Rename an existing entity.", {{"id", "integer"}, {"name", "string"}}, [](const Json& a) -> Mutation { return RenameEntity{a.at("id").as<std::uint64_t>(), a.at("name").as<std::string>()}; }},
        {"play.start", "Start the simulated runtime.", {}, [](const Json&) -> Mutation { return Play{}; }},
        {"play.stop", "Stop the simulated runtime.", {}, [](const Json&) -> Mutation { return Stop{}; }},
        {"play.input", "Record simulated input while playing.", {{"value", "string"}}, [](const Json& a) -> Mutation { return SendInput{a.at("value").as<std::string>()}; }},
        {"history.undo", "Undo one complete transaction.", {}, {}},
        {"history.redo", "Redo one complete transaction.", {}, {}},
        {"scene.query", "Read the simulated scene and runtime state.", {}, {}},
        {"commands.list", "Discover versioned command schemas.", {}, {}},
        {"transaction.execute", "Commit a nonempty array of mutations atomically.", {{"commands", "array"}}, {}}
    };
    return entries;
}
const Entry& lookup(const std::string& name) {
    const auto& entries = registry();
    auto entry = std::find_if(entries.begin(), entries.end(), [&](const auto& e) { return e.name == name; });
    if (entry == entries.end()) { throw std::invalid_argument("Unknown command: " + name); }
    return *entry;
}
void keys(const Json& j, const std::vector<std::string>& allowed) {
    for (const auto& [key, value] : j.as<O>()) {
        (void)value;
        if (std::find(allowed.begin(), allowed.end(), key) == allowed.end()) { throw std::invalid_argument("Unknown field: " + key); }
    }
}
void validate(const Entry& entry, const Json& args) {
    std::vector<std::string> names;
    for (const auto& field : entry.fields) {
        names.push_back(field.name);
        const auto& value = args.at(field.name);
        if (field.type == "string") { (void)value.as<std::string>(); }
        else if (field.type == "integer") { (void)value.as<std::uint64_t>(); }
        else if (field.type == "array") { (void)value.as<A>(); }
    }
    keys(args, names);
}
Json schema(const Entry& entry) {
    O properties; A required;
    for (const auto& field : entry.fields) {
        O spec{{"type", field.type}};
        if (field.type == "integer") { spec.emplace("minimum", std::uint64_t{0}); }
        properties.emplace(field.name, std::move(spec)); required.emplace_back(field.name);
    }
    return O{{"type", "object"}, {"properties", std::move(properties)},
             {"required", std::move(required)}, {"additionalProperties", false}};
}
Json response(const Result& result, Json data) {
    if (result.error) { return error_json(result.revision, result.error->code, result.error->field, result.error->message); }
    return O{{"ok", true}, {"revision", result.revision}, {"data", std::move(data)}};
}
}
Json error_json(std::uint64_t revision, const std::string& code, const std::string& field, const std::string& message) {
    return O{{"ok", false}, {"revision", revision}, {"error", O{{"code", code}, {"field", field}, {"message", message}}}};
}
Json document_json(const Document& document) {
    A entities, inputs;
    for (const auto& [id, name] : document.entities) { entities.emplace_back(O{{"id", id}, {"name", name}}); }
    for (const auto& input : document.runtime.inputs) { inputs.emplace_back(input); }
    return O{{"schema_version", std::uint64_t{1}}, {"entities", std::move(entities)},
             {"mock", true}, {"runtime", O{{"playing", document.runtime.playing}, {"inputs", std::move(inputs)}}}};
}
CommandService::CommandService(std::string instance, const EngineBridge& bridge)
    : instance_(std::move(instance)), bus_(instance_, bridge) {}
Json CommandService::descriptors() const {
    A list;
    for (const auto& entry : registry()) {
        list.emplace_back(O{{"name", entry.name}, {"version", std::uint64_t{1}},
                            {"description", entry.description}, {"inputSchema", schema(entry)},
                            {"mutation", static_cast<bool>(entry.decode) || entry.name == "transaction.execute"}});
    }
    return list;
}
Json CommandService::call(const Json& request) {
    std::lock_guard lock(mutex_);
    const auto revision = bus_.snapshot().second;
    try {
        keys(request, {"instance", "command", "arguments", "expected_revision"});
        if (request.at("instance").as<std::string>() != instance_) {
            return error_json(revision, "instance_mismatch", "instance", "Target must match this editor instance.");
        }
        std::optional<std::uint64_t> expected;
        if (request.contains("expected_revision")) { expected = request.at("expected_revision").as<std::uint64_t>(); }
        if (expected && *expected != revision) { return error_json(revision, "stale_revision", "expected_revision", "Read current state before retrying."); }
        const auto& entry = lookup(request.at("command").as<std::string>());
        const Json args = request.contains("arguments") ? request.at("arguments") : Json(O{});
        validate(entry, args);
        Result result{revision, {}};
        Json data = O{};
        if (entry.decode) { result = bus_.execute(instance_, expected, {entry.decode(args)}); }
        else if (entry.name == "transaction.execute") {
            std::vector<Mutation> mutations;
            for (const auto& item : args.at("commands").as<A>()) {
                keys(item, {"command", "arguments"});
                const auto& mutation = lookup(item.at("command").as<std::string>());
                if (!mutation.decode) { throw std::invalid_argument("Only mutations are allowed in transactions"); }
                const Json a = item.contains("arguments") ? item.at("arguments") : Json(O{});
                validate(mutation, a); mutations.push_back(mutation.decode(a));
            }
            result = bus_.execute(instance_, expected, mutations);
        } else if (entry.name == "history.undo") { result = bus_.undo(instance_, expected); }
        else if (entry.name == "history.redo") { result = bus_.redo(instance_, expected); }
        else if (entry.name == "commands.list") { data = descriptors(); }
        else if (entry.name == "scene.query") { data = document_json(bus_.snapshot().first); }
        return response(result, std::move(data));
    } catch (const std::invalid_argument& e) {
        return error_json(revision, "invalid_request", "request", e.what());
    }
}
}
