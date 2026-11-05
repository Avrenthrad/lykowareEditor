#pragma once
#include "dock_layout.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace lyko {
struct Workspace {
    std::uint64_t scale_percent = 100;
    std::string theme = "dark";
    bool reduced_motion = false;
    std::optional<DockLayout> layout;
    std::optional<std::uint64_t> selection;
    std::map<std::string, std::string> panels{
        {"scene", "left"}, {"inspector", "right"}, {"viewport", "center"},
        {"console", "bottom"}, {"history", "bottom"}};
    std::map<std::string, std::uint64_t> splits{{"left", 20}, {"right", 25}, {"bottom", 25}};
    std::map<std::string, std::string> keymap{
        {"palette.search", "Ctrl+K"}, {"history.undo", "Ctrl+Z"},
        {"history.redo", "Ctrl+Shift+Z"}, {"play.start", "F5"}, {"play.stop", "Shift+F5"}};
    bool operator==(const Workspace&) const = default;
};
inline bool valid_shortcut(const std::string& shortcut) {
    if (shortcut.empty()) { return true; } // Explicit unbind.
    std::string_view remaining(shortcut);
    for (auto modifier : {std::string_view("Ctrl+"), std::string_view("Alt+"),
                          std::string_view("Shift+"), std::string_view("Super+")}) {
        if (remaining.starts_with(modifier)) { remaining.remove_prefix(modifier.size()); }
    }
    if (remaining.size() == 1) {
        char key = remaining.front(); return (key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9');
    }
    for (auto key : {"Enter", "Escape", "Space", "Tab", "Backspace", "Delete", "Home", "End",
                     "PageUp", "PageDown", "Up", "Down", "Left", "Right"}) {
        if (remaining == key) { return true; }
    }
    for (unsigned i = 1; i <= 24; ++i) { if (remaining == "F" + std::to_string(i)) { return true; } }
    return false;
}
inline std::optional<std::string> workspace_error(const Workspace& w) {
    if (w.scale_percent < 75 || w.scale_percent > 250) { return "UI scale must be 75 to 250 percent"; }
    if (w.theme != "dark" && w.theme != "high_contrast") { return "Choose dark or high_contrast theme"; }
    const Workspace defaults;
    if (w.panels.size() != defaults.panels.size() || w.splits.size() != defaults.splits.size()) { return "Unexpected workspace panel or split count"; }
    for (const auto& [panel, dock] : w.panels) {
        if (!defaults.panels.contains(panel)) { return "Unknown panel"; }
        if (dock != "left" && dock != "right" && dock != "center" && dock != "bottom" && dock != "hidden") { return "Invalid panel dock"; }
    }
    for (const auto& [dock, percent] : w.splits) {
        if (!defaults.splits.contains(dock) || percent < 10 || percent > 50) { return "Invalid dock split"; }
    }
    if (w.splits.at("left") + w.splits.at("right") > 70) { return "Keep 30 percent width for the center"; }
    std::vector<std::string> assigned;
    for (const auto& [command, shortcut] : w.keymap) {
        if (command.empty() || command.size() > 128 || shortcut.empty() || !valid_shortcut(shortcut)) { return "Invalid shortcut binding"; }
        for (const auto& prior : assigned) { if (prior == shortcut) { return "Conflicting shortcut binding"; } }
        assigned.push_back(shortcut);
    }
    if(w.layout){if(auto error=dock_layout_error(*w.layout)){return error;}}
    return {};
}
inline DockLayout effective_dock_layout(const Workspace& w){return w.layout?canonical_dock_layout(*w.layout):starter_dock_layout(w.panels,w.splits);}

}
