#pragma once
#include "command_bus.hpp"
#include "json.hpp"

namespace lyko {
// Simulated visual feedback, not a renderer: coloured markers encode entity IDs,
// input history and play state. Stable output supports automated comparisons.
inline Json software_preview(const Document& document) {
    constexpr unsigned width = 64, height = 36;
    unsigned input_hash = 0;
    for (const auto& value : document.runtime.inputs) {
        for (unsigned char c : value) { input_hash = (input_hash * 31 + c) % 65521; }
    }
    std::string ppm = "P3\n# Lykoware deterministic MOCK preview\n64 36\n255\n";
    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            unsigned r = 24, g = 26, b = 32;
            if (x % 8 == 0 || y % 8 == 0) { r = 38; g = 40; b = 48; }
            for (const auto& [id, name] : document.entities) {
                (void)name;
                const auto px = static_cast<unsigned>((id % 56 + input_hash % 56) % 56);
                const auto py = static_cast<unsigned>((id % 28 * 7 + input_hash % 28) % 28);
                if (x >= px && x < px + 6 && y >= py && y < py + 6) {
                    r = 80; g = document.runtime.playing ? 210 : 130; b = 230;
                }
            }
            // A status strip remains observable even for an empty scene.
            if (y == height - 1) { r = input_hash % 256; g = document.runtime.playing ? 220 : 80; b = 100; }
            ppm += std::to_string(r) + ' ' + std::to_string(g) + ' ' + std::to_string(b) + ' ';
        }
        ppm += '\n';
    }
    return Json::Object{{"mock", true}, {"width", std::uint64_t{width}}, {"height", std::uint64_t{height}},
                        {"mimeType", "image/x-portable-pixmap"}, {"ppm", std::move(ppm)}};
}
}
