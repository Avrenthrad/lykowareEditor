#include "command_bus.hpp"
#include "prefab_model.hpp"
#include "preserve_world.hpp"
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace lyko {
CommandBus::CommandBus(std::string instance, const EngineBridge& bridge)
    : instance_(std::move(instance)), bridge_(bridge) {
    if (instance_.empty()) { throw std::invalid_argument("instance must not be empty"); }
}
std::optional<Error> CommandBus::validate_target(const std::string& instance,
                                                std::optional<std::uint64_t> expected) const {
    if (instance != instance_) { return Error{"instance_mismatch", "instance", "Explicit target must match this editor instance."}; }
    if (expected && *expected != revision_) { return Error{"stale_revision", "revision", "Read the latest revision before retrying."}; }
    return {};
}
Result CommandBus::execute(const std::string& instance, std::optional<std::uint64_t> expected,
                           const std::vector<Mutation>& mutations, std::vector<std::string> command_names) {
    std::lock_guard lock(mutex_);
    if (auto error = validate_target(instance, expected)) { return {revision_, error}; }
    if (mutations.empty()) { return {revision_, Error{"empty_transaction", "commands", "Supply at least one mutation."}}; }
    if (command_names.empty()) { command_names.assign(mutations.size(), "mutation"); }
    if (command_names.size() != mutations.size()) { return {revision_, Error{"invalid_history_metadata", "commands", "History metadata must match transaction size."}}; }
    for (const auto& name : command_names) {
        if (name.empty() || name.size() > 128) { return {revision_, Error{"invalid_history_metadata", "commands", "Invalid history command name."}}; }
    }
    auto candidate = document_;
    for (const auto& mutation : mutations) {
        std::optional<Error> error;
        try {
        error = std::visit([&](const auto& command) -> std::optional<Error> {
            using T = std::decay_t<decltype(command)>;
            if constexpr (std::is_same_v<T, ProjectMutation>) {
                return std::visit([&](const auto& edit)->std::optional<Error>{
                    using P=std::decay_t<decltype(edit)>;
                    if constexpr(std::is_same_v<P,ClearRecentProjects>){candidate.project.recent.clear();return {};}
                    else if constexpr(std::is_same_v<P,ReplaceRecentProjects>){
                        if(auto issue=recent_projects_issue(edit.recent)){return Error{"invalid_project_recents",issue->field,issue->message};}
                        candidate.project.recent=edit.recent;return {};
                    }
                    else if constexpr(std::is_same_v<P,ConfigureProject>){
                        if(!candidate.project.active){return Error{"no_active_project","project","Open a project before editing its settings."};}
                        if(edit.target&&candidate.project.active->path!=*edit.target){return Error{"project_context_changed","project","Commit the project context change before reverting settings."};}
                        auto manifest=candidate.project.active->manifest;manifest.name=edit.name;
                        for(const auto& [role,path]:edit.directories){manifest.directories.insert_or_assign(role,path);}
                        if(auto issue=project_manifest_issue(manifest)){return Error{"invalid_project_manifest",issue->field,issue->message};}
                        candidate.project.active->manifest=std::move(manifest);
                        for(auto& recent:candidate.project.recent){if(recent.path==candidate.project.active->path){recent=*candidate.project.active;}}
                        return {};
                    }
                    else {
                        if constexpr(std::is_same_v<P,OpenProject>){
                            if(auto issue=project_session_issue(edit.project)){return Error{"invalid_project",issue->field,issue->message};}
                            if(candidate.project.active==edit.project){return {};}
                        } else {if(!candidate.project.active){return {};}}
                        const auto clean=edit.clean_origin&&candidate.project.active&&edit.clean_origin->path==candidate.project.active->path?edit.clean_origin:candidate.project.opened_baseline;
                        if(candidate.project.active&&clean&&candidate.project.active->manifest!=clean->manifest){return Error{"unsaved_project_settings","project","Save or explicitly revert project settings before closing or switching context."};}
                        if(!candidate.entities.empty()||candidate.next_id!=1||!candidate.assets.entries.empty()||candidate.assets.next_id!=1||candidate.runtime.playing||!candidate.runtime.inputs.empty()||!candidate.runtime.logs.empty()){
                            return Error{"project_has_content","project","Project context changes require an empty/reset scene, catalogue and mock runtime; save work before explicitly resetting them."};
                        }
                        if constexpr(std::is_same_v<P,OpenProject>){
                            candidate.project.active=edit.project;candidate.project.opened_baseline=edit.project;
                            auto& recent=candidate.project.recent;
                            std::erase_if(recent,[&](const auto& item){return item.path==edit.project.path;});
                            recent.insert(recent.begin(),edit.project);
                            if(recent.size()>16){recent.resize(16);}
                        } else {candidate.project.active.reset();candidate.project.opened_baseline.reset();}
                        return {};
                    }
                },command);
            } else if constexpr (std::is_same_v<T, AssetMutation>) {
                auto assets=candidate.assets;
                auto issue=std::visit([&](const auto& edit)->std::optional<Error>{
                    using A=std::decay_t<decltype(edit)>;
                    if constexpr(std::is_same_v<A,RegisterAsset>){
                        if(assets.next_id==std::numeric_limits<std::uint64_t>::max()){return Error{"id_exhausted","id","No asset IDs remain."};}
                        assets.entries.emplace(assets.next_id++,Asset{edit.path,edit.kind});
                    }else if constexpr(std::is_same_v<A,RelocateAsset>){
                        if(!assets.entries.contains(edit.id)){return Error{"not_found","id","Asset does not exist."};}assets.entries.at(edit.id).path=edit.path;
                    }else if constexpr(std::is_same_v<A,ForgetAsset>){
                        if(!assets.entries.erase(edit.id)){return Error{"not_found","id","Asset does not exist."};}
                    }else{assets=edit.catalog;}
                    if(auto error=catalog_error(assets)){return Error{"invalid_catalog","catalog",*error};}
                    return {};
                },command);
                if(issue){return issue;}candidate.assets=std::move(assets);
            } else if constexpr (std::is_same_v<T, InstantiatePrefab>) {
                if(command.project_origin && candidate.project.active!=command.project_origin){return Error{"project_context_changed","root","Active project changed in this transaction; commit that change before resolving an implicit source root."};}

                if(command.asset_origin){const auto& [id,asset]=*command.asset_origin;const auto found=candidate.assets.entries.find(id);
                    if(found==candidate.assets.entries.end()||found->second!=asset){return Error{"asset_reference_changed","id","Catalogue entry changed in this transaction; commit the catalogue edit before instantiating."};}
                }
                if(auto issue=prefab_error(command.source,command.root)){return Error{"invalid_prefab","path",*issue};}
                if(command.parent&&!candidate.entities.contains(command.parent)){return Error{"not_found","parent","Parent entity does not exist."};}
                if(command.source.entities.size()>std::numeric_limits<EntityId>::max()-candidate.next_id){return Error{"id_exhausted","id","No identifiers remain."};}
                std::map<EntityId,EntityId> ids;
                for(const auto& [id,name]:command.source.entities){const auto next=candidate.next_id++;ids.emplace(id,next);candidate.entities.emplace(next,name);}
                for(const auto& [id,parent]:command.source.parents){candidate.parents.emplace(ids.at(id),ids.at(parent));}
                for(const auto& [id,transform]:command.source.transforms){candidate.transforms.emplace(ids.at(id),transform);}
                if(command.parent){candidate.parents.emplace(ids.at(command.root),command.parent);}
                if(auto issue=hierarchy_error(candidate)){return Error{"invalid_hierarchy","parent",*issue};}
                candidate.workspace.selection=ids.at(command.root);
            } else if constexpr (std::is_same_v<T, CreateEntity> || std::is_same_v<T, RenameEntity> || std::is_same_v<T, DuplicateEntity>) {
                if (command.name.empty() || command.name.size() > 128 || !json_detail::valid_utf8(command.name)) {
                    return Error{"invalid_name", "name", "Name must contain 1 to 128 UTF-8 bytes."};
                }
                if constexpr (std::is_same_v<T, CreateEntity>) {
                    if (candidate.next_id == std::numeric_limits<EntityId>::max()) {
                        return Error{"id_exhausted", "id", "No identifiers remain."};
                    }
                    candidate.entities.emplace(candidate.next_id++, command.name);
                } else if constexpr (std::is_same_v<T, DuplicateEntity>) {
                    if (!candidate.entities.contains(command.id)) { return Error{"not_found", "id", "Entity does not exist."}; }
                    if (candidate.next_id == std::numeric_limits<EntityId>::max()) { return Error{"id_exhausted", "id", "No identifiers remain."}; }
                    const auto id = candidate.next_id++;
                    candidate.entities.emplace(id, command.name);
                    if (candidate.parents.contains(command.id)) { candidate.parents.emplace(id, candidate.parents.at(command.id)); }
                    if (candidate.transforms.contains(command.id)) { candidate.transforms.emplace(id, candidate.transforms.at(command.id)); }
                    candidate.workspace.selection = id;
                } else {
                    auto entity = candidate.entities.find(command.id);
                    if (entity == candidate.entities.end()) { return Error{"not_found", "id", "Entity does not exist."}; }
                    entity->second = command.name;
                }
            } else if constexpr (std::is_same_v<T, DeleteEntity>) {
                if (!candidate.entities.erase(command.id)) { return Error{"not_found", "id", "Entity does not exist."}; }
                candidate.transforms.erase(command.id);
                candidate.parents.erase(command.id);
                std::erase_if(candidate.parents, [&](const auto& edge) { return edge.second == command.id; });
                if (candidate.workspace.selection == command.id) { candidate.workspace.selection.reset(); }
            } else if constexpr (std::is_same_v<T, SetTransform>) {
                if (!candidate.entities.contains(command.id)) { return Error{"not_found", "id", "Entity does not exist."}; }
                if (auto issue=transform_error(command.transform)) { return Error{"invalid_transform", "transform", *issue}; }
                if (command.transform == Transform{}) { candidate.transforms.erase(command.id); }
                else { candidate.transforms.insert_or_assign(command.id, command.transform); }
            } else if constexpr (std::is_same_v<T, ReparentEntity>) {
                if (!candidate.entities.contains(command.id)) { return Error{"not_found", "id", "Entity does not exist."}; }
                const auto before_reparent = command.preserve_world ? std::optional<Document>(candidate) : std::nullopt;
                if (command.parent == 0) { candidate.parents.erase(command.id); }
                else { candidate.parents.insert_or_assign(command.id, command.parent); }
                if (auto issue = hierarchy_error(candidate)) { return Error{"invalid_hierarchy", "parent", *issue}; }
                if(before_reparent){
                    if(before_reparent->parents==candidate.parents){return {};}
                    try {preserve_world(*before_reparent,candidate,command.id,command.parent);}
                    catch(const std::exception& issue){return Error{"cannot_preserve_world","parent",issue.what()};}
                }
            } else if constexpr (std::is_same_v<T, ReplaceScene>) {
                if (auto issue = hierarchy_error(command.document)) { return Error{"invalid_hierarchy", "parents", *issue}; }
                for(const auto& [id, transform]:command.document.transforms){
                    if(!command.document.entities.contains(id)){return Error{"invalid_transform","id","Transform references a missing entity."};}
                    if(auto issue=transform_error(transform)){return Error{"invalid_transform","transform",*issue};}
                }
                if (command.document.next_id == 0) { return Error{"invalid_scene", "next_id", "Next ID must be positive."}; }
                for (const auto& [id, name] : command.document.entities) {
                    if (id == 0 || id >= command.document.next_id || name.empty() || name.size() > 128 || !json_detail::valid_utf8(name)) {
                        return Error{"invalid_scene", "entities", "Invalid entity identifier or name."};
                    }
                }
                if (command.document.runtime.playing || !command.document.runtime.inputs.empty() || !command.document.runtime.logs.empty()) {
                    return Error{"invalid_scene", "runtime", "Loaded scenes must not contain runtime state."};
                }
                candidate.entities = command.document.entities;
                candidate.parents = command.document.parents;
                candidate.transforms = command.document.transforms;
                candidate.next_id = command.document.next_id;
                candidate.runtime = command.document.runtime;
                candidate.workspace.selection.reset();
            } else if constexpr (std::is_same_v<T, ReplaceDockLayout>) {
                if(auto error=dock_layout_error(command.layout)){return Error{"invalid_layout","layout",*error};}
                const auto current=effective_dock_layout(candidate.workspace);
                const auto normalized=canonical_dock_layout(command.layout);if(current!=normalized){candidate.workspace.layout=normalized;}
            } else if constexpr (std::is_same_v<T, ResetDockLayout>) {
                candidate.workspace.layout.reset();
            } else if constexpr (std::is_same_v<T, ReplaceWorkspace>) {
                if (auto message = workspace_error(command.workspace)) { return Error{"invalid_workspace", "workspace", *message}; }
                const auto selection = candidate.workspace.selection;
                candidate.workspace = command.workspace;
                candidate.workspace.selection = selection;
            } else if constexpr (std::is_same_v<T, ConfigureWorkspace>) {
                if (command.scale_percent < 75 || command.scale_percent > 250) {
                    return Error{"invalid_scale", "scale_percent", "UI scale must be between 75 and 250 percent."};
                }
                if (command.theme != "dark" && command.theme != "high_contrast") {
                    return Error{"invalid_theme", "theme", "Choose dark or high_contrast."};
                }
                candidate.workspace.scale_percent = command.scale_percent;
                candidate.workspace.theme = command.theme;
                candidate.workspace.reduced_motion = command.reduced_motion;
            } else if constexpr (std::is_same_v<T, SetPanelDock>) {
                auto panel = candidate.workspace.panels.find(command.panel);
                if (panel == candidate.workspace.panels.end()) { return Error{"unknown_panel", "panel", "Panel is not registered."}; }
                if (command.dock != "left" && command.dock != "right" && command.dock != "center" && command.dock != "bottom" && command.dock != "hidden") {
                    return Error{"invalid_dock", "dock", "Choose left, right, center, bottom or hidden."};
                }
                panel->second = command.dock;
                candidate.workspace.layout.reset();
            } else if constexpr (std::is_same_v<T, ResizeDock>) {
                if (!candidate.workspace.splits.contains(command.dock)) { return Error{"invalid_dock", "dock", "Only left, right and bottom splits can be resized."}; }
                if (command.percent < 10 || command.percent > 50) { return Error{"invalid_split", "percent", "Dock size must be 10 to 50 percent."}; }
                auto splits = candidate.workspace.splits;
                splits.at(command.dock) = command.percent;
                if (splits.at("left") + splits.at("right") > 70) { return Error{"invalid_split", "percent", "Keep at least 30 percent of the width for the center."}; }
                candidate.workspace.splits = std::move(splits);
                candidate.workspace.layout.reset();
            } else if constexpr (std::is_same_v<T, BindShortcut>) {
                if (command.command.empty() || command.command.size() > 128 || !valid_shortcut(command.shortcut)) {
                    return Error{"invalid_shortcut", "shortcut", "Use canonical Ctrl+Alt+Shift+Super modifiers followed by a supported key; empty unbinds."};
                }
                for (const auto& [name, shortcut] : candidate.workspace.keymap) {
                    if (name != command.command && !command.shortcut.empty() && shortcut == command.shortcut) {
                        return Error{"shortcut_conflict", "shortcut", "Shortcut is already bound; unbind it first."};
                    }
                }
                if (command.shortcut.empty()) { candidate.workspace.keymap.erase(command.command); }
                else { candidate.workspace.keymap[command.command] = command.shortcut; }
            } else if constexpr (std::is_same_v<T, SelectEntity>) {
                if (command.id == 0) { candidate.workspace.selection.reset(); }
                else {
                    if (!candidate.entities.contains(command.id)) { return Error{"not_found", "id", "Entity does not exist."}; }
                    candidate.workspace.selection = command.id;
                }
            } else if constexpr (std::is_same_v<T, Play>) {
                candidate.runtime = bridge_.play(candidate.runtime);
            } else if constexpr (std::is_same_v<T, Stop>) {
                candidate.runtime = bridge_.stop(candidate.runtime);
            } else if constexpr (std::is_same_v<T, SendInput>) {
                if (!candidate.runtime.playing) { return Error{"not_playing", "value", "Start play mode before sending input."}; }
                if (command.value.empty() || command.value.size() > 128) { return Error{"invalid_input", "value", "Input must contain 1 to 128 bytes."}; }
                if (candidate.runtime.inputs.size() >= 1024) { return Error{"input_limit", "value", "Mock session accepts at most 1024 inputs; undo or load a scene to reset."}; }
                candidate.runtime = bridge_.input(candidate.runtime, command.value);
            }
            return {};
        }, mutation);
        } catch (const std::exception&) {
            return {revision_, Error{"backend_failure", "backend", "Backend operation failed; transaction was not committed."}};
        }
        if (error) { return {revision_, error}; }
    }
    if (candidate == document_) { return {revision_, {}}; }
    // Prepare every potentially allocating operation before the commit swaps.
    auto next_history = history_;
    next_history.erase(next_history.begin() + static_cast<std::ptrdiff_t>(cursor_), next_history.end());
    next_history.push_back({document_, candidate, revision_ + 1, std::move(command_names)});
    using std::swap;
    swap(document_, candidate);
    history_.swap(next_history);
    cursor_ = history_.size();
    ++revision_;
    return {revision_, {}};
}
Result CommandBus::history(const std::string& instance, std::optional<std::uint64_t> expected, bool forward) {
    std::lock_guard lock(mutex_);
    if (auto error = validate_target(instance, expected)) { return {revision_, error}; }
    if (forward ? cursor_ == history_.size() : cursor_ == 0) {
        return {revision_, Error{"history_empty", "history", "No transaction available in this direction."}};
    }
    auto candidate = forward ? history_[cursor_].after : history_[cursor_ - 1].before;
    using std::swap;
    swap(document_, candidate);
    if (forward) { ++cursor_; } else { --cursor_; }
    return {++revision_, {}};
}
Result CommandBus::undo(const std::string& instance, std::optional<std::uint64_t> expected) { return history(instance, expected, false); }
Result CommandBus::redo(const std::string& instance, std::optional<std::uint64_t> expected) { return history(instance, expected, true); }
std::pair<Document, std::uint64_t> CommandBus::snapshot() const {
    std::lock_guard lock(mutex_);
    return {document_, revision_};
}
std::uint64_t CommandBus::revision() const {
    std::lock_guard lock(mutex_); return revision_;
}
HistorySnapshot CommandBus::history_unlocked() const {
    HistorySnapshot result{cursor_, {}};
    result.entries.reserve(history_.size());
    for (std::size_t i = 0; i < history_.size(); ++i) {
        result.entries.push_back({history_[i].revision, history_[i].commands, i < cursor_});
    }
    return result;
}
HistorySnapshot CommandBus::history_snapshot() const {
    std::lock_guard lock(mutex_); return history_unlocked();
}
EditorSnapshot CommandBus::editor_snapshot() const {
    std::lock_guard lock(mutex_); return {document_, revision_, history_unlocked()};
}

}
