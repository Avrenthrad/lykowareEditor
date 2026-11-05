#include "command_service.hpp"
#include "scene_io.hpp"
#include "transform_json.hpp"
#include "text_file.hpp"
#include "software_preview.hpp"
#include "ui/design_tokens.hpp"
#include "ui/workspace_io.hpp"
#include "ui/shell_snapshot.hpp"
#include <algorithm>
#include <functional>
#include <stdexcept>

namespace lyko {
namespace {
using O = Json::Object;
using A = Json::Array;
struct Field { std::string name; std::string type; O constraints = {}; };
enum class Action { mutation, undo, redo, scene, save, discovery, transaction, logs, preview, workspace, tokens, palette, selection, workspace_save, history, snapshot, layout, tree, transform };
struct Entry {
    std::string name, description;
    std::vector<Field> fields;
    std::function<Mutation(const Json&)> decode;
    Action action = Action::mutation;
};
const Entry& lookup(const std::string& name);
const std::vector<Entry>& registry() {
    static const std::vector<Entry> entries = {
        {"workspace.load", "Validate TOML UI settings, layout and keymap and replace them undoably.", {{"path", "string"}}, [](const Json& a) -> Mutation {
            auto workspace = load_workspace(path_from_utf8(a.at("path").as<std::string>()));
            for (const auto& [command, shortcut] : workspace.keymap) { (void)shortcut; (void)lookup(command); }
            return ReplaceWorkspace{workspace}; }},
        {"workspace.save", "Publish TOML UI settings, layout and keymap to a new path; export cannot be undone.", {{"path", "string"}}, {}, Action::workspace_save},
        {"workspace.configure", "Set UI scaling (75..250%), dark/high_contrast theme and reduced motion atomically.", {{"scale_percent", "integer", {{"title","UI scale (%)"},{"minimum",std::uint64_t{75}},{"maximum",std::uint64_t{250}},{"default",std::uint64_t{100}}}}, {"theme", "string", {{"enum",A{"dark","high_contrast"}},{"default","dark"}}}, {"reduced_motion", "boolean", {{"title","Reduce motion"},{"default",false}}}}, [](const Json& a) -> Mutation { return ConfigureWorkspace{a.at("scale_percent").as<std::uint64_t>(), a.at("theme").as<std::string>(), a.at("reduced_motion").as<bool>()}; }},
        {"panel.dock", "Apply a starter panel slot preset; this resets a custom dock tree.", {{"panel", "string", {{"enum",A{"scene","inspector","viewport","console","history"}},{"default","scene"}}}, {"dock", "string", {{"enum",A{"left","right","center","bottom","hidden"}},{"default","left"}}}}, [](const Json& a) -> Mutation { return SetPanelDock{a.at("panel").as<std::string>(), a.at("dock").as<std::string>()}; }},
        {"layout.query", "Read the effective validated dock split/tab tree.", {}, {}, Action::layout},
        {"layout.replace", "Replace the complete dock tree as one validated undoable transaction.", {{"layout","object",dock_layout_constraints()}}, [](const Json& a)->Mutation{return ReplaceDockLayout{parse_dock_layout(a.at("layout"))};}},
        {"layout.reset", "Restore the starter slot layout as one undoable transaction.", {}, [](const Json&)->Mutation{return ResetDockLayout{};}},
        {"layout.resize", "Resize a starter dock slot and reset a custom tree; retain usable center space.", {{"dock", "string", {{"enum",A{"left","right","bottom"}},{"default","left"}}}, {"percent", "integer", {{"title","Size (%)"},{"minimum",std::uint64_t{10}},{"maximum",std::uint64_t{50}},{"default",std::uint64_t{25}}}}}, [](const Json& a) -> Mutation { return ResizeDock{a.at("dock").as<std::string>(), a.at("percent").as<std::uint64_t>()}; }},
        {"keymap.bind", "Bind a canonical shortcut to a registered command; empty shortcut unbinds.", {{"command", "string"}, {"shortcut", "string"}}, [](const Json& a) -> Mutation {
            auto command = a.at("command").as<std::string>(); (void)lookup(command);
            return BindShortcut{command, a.at("shortcut").as<std::string>()}; }},
        {"selection.set", "Select an existing entity; ID zero clears selection.", {{"id", "integer"}}, [](const Json& a) -> Mutation { return SelectEntity{a.at("id").as<std::uint64_t>()}; }},
        {"workspace.query", "Read UI settings, panel positions, split sizes, selection and keymap.", {}, {}, Action::workspace},
        {"design.tokens", "Read toolkit-independent accessible colour, scale, typography and motion tokens.", {}, {}, Action::tokens},
        {"palette.search", "Search every registered command and show its current shortcut; empty query lists all.", {{"query", "string"}}, {}, Action::palette},
        {"selection.query", "Read the selected entity for the inspector, or null when unselected.", {}, {}, Action::selection},
        {"entity.create", "Create a named entity with a stable ID.", {{"name", "string", {{"default","New entity"}}}}, [](const Json& a) -> Mutation { return CreateEntity{a.at("name").as<std::string>()}; }},
        {"entity.rename", "Rename an existing entity.", {{"id", "integer", {{"title","Entity ID"},{"minimum",std::uint64_t{1}},{"default",std::uint64_t{1}}}}, {"name", "string"}}, [](const Json& a) -> Mutation { return RenameEntity{a.at("id").as<std::uint64_t>(), a.at("name").as<std::string>()}; }},
        {"entity.delete", "Delete an entity and clear its selection atomically; undo restores both.", {{"id", "integer", {{"title","Entity ID"},{"minimum",std::uint64_t{1}},{"default",std::uint64_t{1}}}}}, [](const Json& a) -> Mutation { return DeleteEntity{a.at("id").as<std::uint64_t>()}; }},
        {"entity.duplicate", "Duplicate an entity under a new stable ID and select the copy atomically.", {{"id", "integer", {{"title","Source entity ID"},{"minimum",std::uint64_t{1}},{"default",std::uint64_t{1}}}}, {"name", "string", {{"default","Entity copy"}}}}, [](const Json& a) -> Mutation { return DuplicateEntity{a.at("id").as<std::uint64_t>(), a.at("name").as<std::string>()}; }},
        {"entity.reparent", "Move an entity under another entity, or parent zero for scene root; reject cycles and depth over 64.", {{"id","integer",{{"minimum",std::uint64_t{1}},{"default",std::uint64_t{1}}}}, {"parent","integer",{{"default",std::uint64_t{0}}}}}, [](const Json& a)->Mutation { return ReparentEntity{a.at("id").as<std::uint64_t>(),a.at("parent").as<std::uint64_t>()}; }},
        {"scene.tree", "Read a deterministic parent-first scene tree with parent IDs and depths.", {}, {}, Action::tree},
        {"transform.set", "Set local position, intrinsic XYZ Euler degrees and nonzero signed scale atomically; authoring only.", {{"id","integer",{{"minimum",std::uint64_t{1}},{"default",std::uint64_t{1}}}}, {"px","number",{{"title","Position X"},{"default",0.0}}}, {"py","number",{{"title","Position Y"},{"default",0.0}}}, {"pz","number",{{"title","Position Z"},{"default",0.0}}}, {"rx","number",{{"title","Rotation X (degrees)"},{"default",0.0}}}, {"ry","number",{{"title","Rotation Y (degrees)"},{"default",0.0}}}, {"rz","number",{{"title","Rotation Z (degrees)"},{"default",0.0}}}, {"sx","number",{{"title","Scale X"},{"default",1.0}}}, {"sy","number",{{"title","Scale Y"},{"default",1.0}}}, {"sz","number",{{"title","Scale Z"},{"default",1.0}}}}, [](const Json& a)->Mutation { O values=a.as<O>();values.erase("id");return SetTransform{a.at("id").as<std::uint64_t>(),parse_transform(values)}; }},
        {"transform.query", "Read an entity's local authoring transform; default is identity.", {{"id","integer",{{"minimum",std::uint64_t{1}},{"default",std::uint64_t{1}}}}}, {}, Action::transform},
        {"play.start", "Start the simulated runtime.", {}, [](const Json&) -> Mutation { return Play{}; }},
        {"play.stop", "Stop the simulated runtime.", {}, [](const Json&) -> Mutation { return Stop{}; }},
        {"play.input", "Record simulated input while playing.", {{"value", "string"}}, [](const Json& a) -> Mutation { return SendInput{a.at("value").as<std::string>()}; }},
        {"scene.load", "Validate and load a scene as one undoable transaction.", {{"path", "string"}}, [](const Json& a) -> Mutation { return ReplaceScene{load_scene(path_from_utf8(a.at("path").as<std::string>()))}; }},
        {"scene.save", "Atomically save authoring JSON to a NEW path; export cannot be undone.", {{"path", "string"}}, {}, Action::save},
        {"history.query", "Read transaction names, applied/redo entries and undo/redo availability.", {}, {}, Action::history},
        {"editor.snapshot", "Read a coherent revision of all desktop panel models without rendering or file I/O.", {}, {}, Action::snapshot},
        {"history.undo", "Undo one complete transaction.", {}, {}, Action::undo},
        {"history.redo", "Redo one complete transaction.", {}, {}, Action::redo},
        {"logs.query", "Read the last 256 simulated runtime log messages.", {}, {}, Action::logs},
        {"preview.capture", "Capture a deterministic 64x36 software mock preview as PPM text.", {}, {}, Action::preview},
        {"scene.query", "Read the simulated scene and runtime state.", {}, {}, Action::scene},
        {"commands.list", "Discover versioned command schemas.", {}, {}, Action::discovery},
        {"transaction.execute", "Commit a nonempty array of mutations atomically.", {{"commands", "array"}}, {}, Action::transaction}
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
        if (field.type == "string") { const auto& text=value.as<std::string>(); if(!json_detail::valid_utf8(text)){throw std::invalid_argument("Field "+field.name+" must be valid UTF-8");} }
        else if (field.type == "number") { if(!std::isfinite(json_number(value))){throw std::invalid_argument("Number must be finite.");} }
        else if (field.type == "integer") { (void)value.as<std::uint64_t>(); }
        else if (field.type == "boolean") { (void)value.as<bool>(); }
        else if (field.type == "object") { (void)value.as<O>(); }
        else if (field.type == "array") { (void)value.as<A>(); }
    }
    keys(args, names);
}
Json schema(const Entry& entry) {
    O properties; A required;
    for (const auto& field : entry.fields) {
        O spec{{"type", field.type}};
        if (field.type == "string") {
            spec.emplace("minLength", std::uint64_t{field.name == "query" || field.name == "shortcut" ? 0U : 1U});
            if (field.name == "name" || field.name == "value") { spec.emplace("maxLength", std::uint64_t{128}); spec.emplace("x-maxBytes",std::uint64_t{128}); }
        }
        if (field.type == "integer") { spec.emplace("minimum", std::uint64_t{0}); }
        for(const auto& [key,value]:field.constraints){spec.insert_or_assign(key,value);}
        if(field.name=="path"){spec.emplace("description","Path to a local file. Saving requires a new destination.");}
        if(field.name=="command" && entry.name=="keymap.bind") { A names; for(const auto& command:registry()){names.emplace_back(command.name);} spec.emplace("enum",std::move(names)); }
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
                            {"effect", (entry.action == Action::save || entry.action == Action::workspace_save) ? "filesystem_export" :
                                (entry.decode || entry.action == Action::transaction || entry.action == Action::undo || entry.action == Action::redo) ? "document" : "query"},
                            {"undoable", static_cast<bool>(entry.decode) || entry.action == Action::transaction},
                            {"mutation", static_cast<bool>(entry.decode) || entry.action == Action::transaction}});
    }
    return list;
}
Json CommandService::call(const Json& request) {
    std::lock_guard lock(mutex_);
    const auto revision = bus_.revision();
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
        if (entry.decode) { result = bus_.execute(instance_, expected, {entry.decode(args)}, {entry.name}); }
        else if (entry.action == Action::transaction) {
            std::vector<Mutation> mutations;
            std::vector<std::string> command_names;
            for (const auto& item : args.at("commands").as<A>()) {
                keys(item, {"command", "arguments"});
                const auto& mutation = lookup(item.at("command").as<std::string>());
                if (!mutation.decode) { throw std::invalid_argument("Only mutations are allowed in transactions"); }
                const Json a = item.contains("arguments") ? item.at("arguments") : Json(O{});
                validate(mutation, a); mutations.push_back(mutation.decode(a)); command_names.push_back(mutation.name);
            }
            result = bus_.execute(instance_, expected, mutations, std::move(command_names));
        } else if (entry.action == Action::undo) { result = bus_.undo(instance_, expected); }
        else if (entry.action == Action::redo) { result = bus_.redo(instance_, expected); }
        else if (entry.action == Action::save) { save_scene(path_from_utf8(args.at("path").as<std::string>()), bus_.snapshot().first); data = O{{"saved", true}}; }
        else if (entry.action == Action::workspace_save) { save_workspace(path_from_utf8(args.at("path").as<std::string>()), bus_.snapshot().first.workspace); data = O{{"saved", true}}; }
        else if (entry.action == Action::discovery) { data = descriptors(); }
        else if (entry.action == Action::logs) { data = logs_json(bus_.snapshot().first.runtime); }
        else if (entry.action == Action::preview) { data = software_preview(bus_.snapshot().first); }
        else if (entry.action == Action::workspace) { data = workspace_json(bus_.snapshot().first.workspace); }
        else if (entry.action == Action::layout) { data=dock_layout_json(effective_dock_layout(bus_.snapshot().first.workspace)); }
        else if (entry.action == Action::transform) {
            const auto document=bus_.snapshot().first;const auto id=args.at("id").as<std::uint64_t>();
            if(!document.entities.contains(id)){return error_json(revision,"not_found","id","Entity does not exist.");}
            data=transform_json(document.transforms.contains(id)?document.transforms.at(id):Transform{});
        }
        else if (entry.action == Action::tree) { data = scene_tree_json(bus_.snapshot().first); }
        else if (entry.action == Action::history) { data = history_json(bus_.history_snapshot()); }
        else if (entry.action == Action::snapshot) {
            const auto state = bus_.editor_snapshot();
            data = O{{"schema_version", std::uint64_t{1}}, {"mock", true}, {"scene", document_json(state.document)},
                     {"workspace", workspace_json(state.document.workspace)}, {"selection", selection_json(state.document)},
                     {"tokens", design_tokens(state.document.workspace)}, {"logs", logs_json(state.document.runtime)},
                     {"transforms", transforms_json(state.document)}, {"scene_tree", scene_tree_json(state.document)}, {"history", history_json(state.history)}};
            result.revision = state.revision;
        }
        else if (entry.action == Action::tokens) { data = design_tokens(bus_.snapshot().first.workspace); }
        else if (entry.action == Action::selection) { data = selection_json(bus_.snapshot().first); }
        else if (entry.action == Action::palette) {
            auto query = args.at("query").as<std::string>();
            if (query.size() > 128) { throw std::invalid_argument("Palette query must not exceed 128 bytes"); }
            const auto lower = [](std::string text) {
                for (char& c : text) { if (c >= 'A' && c <= 'Z') { c = static_cast<char>(c - 'A' + 'a'); } }
                return text;
            };
            query = lower(std::move(query));
            const auto workspace = bus_.snapshot().first.workspace;
            struct Match { unsigned rank; std::string name; Json descriptor; };
            std::vector<Match> matches;
            const auto all = descriptors();
            for (auto descriptor : all.as<A>()) {
                const auto name = descriptor.at("name").as<std::string>();
                const auto description = lower(descriptor.at("description").as<std::string>());
                unsigned rank = 0;
                if (!query.empty() && !name.starts_with(query)) {
                    if (name.find(query) != std::string::npos) { rank = 1; }
                    else if (description.find(query) != std::string::npos) { rank = 2; }
                    else { continue; }
                }
                const auto shortcut = workspace.keymap.find(name);
                auto object = descriptor.as<O>();
                object.emplace("shortcut", shortcut == workspace.keymap.end() ? "" : shortcut->second);
                descriptor = Json(std::move(object));
                matches.push_back({rank, name, std::move(descriptor)});
            }
            std::sort(matches.begin(), matches.end(), [](const auto& a, const auto& b) { return a.rank != b.rank ? a.rank < b.rank : a.name < b.name; });
            A output; for (auto& match : matches) { output.push_back(std::move(match.descriptor)); }
            data = std::move(output);
        }
        else if (entry.action == Action::scene) { data = document_json(bus_.snapshot().first); }
        return response(result, std::move(data));
    } catch (const std::invalid_argument& e) {
        return error_json(revision, "invalid_request", "request", e.what());
    } catch (const std::exception& e) {
        return error_json(revision, "operation_failed", "command", e.what());
    }
}
}
