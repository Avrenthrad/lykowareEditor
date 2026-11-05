#pragma once
#include "command_bus.hpp"
#include "json.hpp"
#include "ui/design_tokens.hpp"
#include "ui/dock_layout_json.hpp"

namespace lyko {
inline Json scene_tree_json(const Document& document) {
    std::map<EntityId, std::vector<EntityId>> children;
    for (const auto& [id, name] : document.entities) { (void)name; children[document.parents.contains(id) ? document.parents.at(id) : EntityId{0}].push_back(id); }
    std::vector<std::pair<EntityId, std::uint64_t>> pending;
    for (auto it = children[0].rbegin(); it != children[0].rend(); ++it) { pending.emplace_back(*it, 0); }
    Json::Array nodes;
    while (!pending.empty()) {
        const auto [id, depth] = pending.back(); pending.pop_back();
        nodes.emplace_back(Json::Object{{"id",id},{"name",document.entities.at(id)},{"parent",document.parents.contains(id) ? document.parents.at(id) : EntityId{0}},{"depth",depth}});
        for (auto it = children[id].rbegin(); it != children[id].rend(); ++it) { pending.emplace_back(*it, depth + 1); }
    }
    return Json::Object{{"schema_version",std::uint64_t{1}},{"nodes",std::move(nodes)}};
}
inline Json workspace_json(const Workspace& workspace) {
    using O = Json::Object;
    O panels, splits, keymap;
    for (const auto& [name, dock] : workspace.panels) { panels.emplace(name, dock); }
    for (const auto& [name, percent] : workspace.splits) { splits.emplace(name, percent); }
    for (const auto& [name, shortcut] : workspace.keymap) { keymap.emplace(name, shortcut); }
    return O{{"scale_percent", workspace.scale_percent}, {"theme", workspace.theme},
             {"reduced_motion", workspace.reduced_motion}, {"layout",dock_layout_json(effective_dock_layout(workspace))}, {"panels", std::move(panels)},
             {"splits", std::move(splits)}, {"keymap", std::move(keymap)},
             {"selection", workspace.selection ? Json(*workspace.selection) : Json(nullptr)}};
}
inline Json selection_json(const Document& document) {
    return Json::Object{{"entity", document.workspace.selection ?
        Json(Json::Object{{"id", *document.workspace.selection}, {"name", document.entities.at(*document.workspace.selection)}}) : Json(nullptr)}};
}
inline Json logs_json(const RuntimeState& runtime) {
    Json::Array logs;
    for (const auto& message : runtime.logs) { logs.emplace_back(message); }
    return Json::Object{{"mock", true}, {"messages", std::move(logs)}};
}
inline Json history_json(const HistorySnapshot& history) {
    Json::Array entries;
    for (const auto& entry : history.entries) {
        Json::Array names;
        for (const auto& name : entry.commands) { names.emplace_back(name); }
        entries.emplace_back(Json::Object{{"revision", entry.revision}, {"commands", std::move(names)},
                                         {"mutation_count", static_cast<std::uint64_t>(entry.commands.size())}, {"applied", entry.applied}});
    }
    return Json::Object{{"cursor", static_cast<std::uint64_t>(history.cursor)}, {"can_undo", history.cursor != 0},
                        {"can_redo", history.cursor < history.entries.size()}, {"entries", std::move(entries)}};
}
}
