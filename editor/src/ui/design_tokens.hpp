#pragma once
#include "workspace.hpp"
#include "json.hpp"

namespace lyko {
// Toolkit-independent tokens. Desktop adapters consume these rather than
// embedding colours, sizes or motion durations in individual panels.
inline Json design_tokens(const Workspace& workspace) {
    using O = Json::Object;
    const bool high = workspace.theme == "high_contrast";
    const auto size = [&](std::uint64_t logical) { return Json(static_cast<double>(logical) * static_cast<double>(workspace.scale_percent) / 100.0); };
    return O{
        {"theme", workspace.theme}, {"scale_percent", workspace.scale_percent},
        {"colors", O{{"background", high ? "#000000" : "#15171D"},
                     {"surface", high ? "#0D0D0D" : "#1E222B"},
                     {"text", high ? "#FFFFFF" : "#F1F3F8"},
                     {"muted_text", high ? "#CCCCCC" : "#B0B8C9"},
                     {"accent", high ? "#00D9FF" : "#72B7FF"},
                     {"focus", high ? "#FFFF00" : "#FFD166"}}},
        {"spacing", O{{"xs", size(4)}, {"sm", size(8)}, {"md", size(12)}, {"lg", size(16)}, {"xl", size(24)}}},
        {"typography", O{{"body_px", size(14)}, {"heading_px", size(18)}, {"line_height_percent", std::uint64_t{150}}}},
        {"geometry", O{{"control_height_px", size(32)}, {"touch_target_px", size(44)}, {"focus_ring_px", size(2)}, {"corner_radius_px", size(4)}}},
        {"motion", O{{"transition_ms", std::uint64_t{workspace.reduced_motion ? 0U : 120U}}}}
    };
}
}
