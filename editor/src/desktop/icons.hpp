#pragma once
#include "platform.hpp"
#include <cmath>
#include <string>

namespace lyko {
// Original vector artwork, one 16-unit grid and consistent stroke weight.
enum class Icon {
  add,
  check,
  play,
  stop,
  undo,
  redo,
  entity,
  properties,
  console,
  history
};
inline void draw_icon(Icon icon, ImVec2 origin, float size, ImU32 color) {
  auto *draw = ImGui::GetWindowDrawList();
  const float unit = size / 16.f, stroke = 1.5f * unit;
  const auto p = [&](float x, float y) {
    return ImVec2(origin.x + x * unit, origin.y + y * unit);
  };
  const auto line = [&](float x, float y, float a, float b) {
    draw->AddLine(p(x, y), p(a, b), color, stroke);
  };
  switch (icon) {
  case Icon::add:
    line(3, 8, 13, 8);
    line(8, 3, 8, 13);
    break;
  case Icon::check:
    line(3, 8, 6, 11);
    line(6, 11, 13, 4);
    break;
  case Icon::play:
    draw->AddTriangle(p(5, 3), p(13, 8), p(5, 13), color, stroke);
    break;
  case Icon::stop:
    draw->AddRect(p(4, 4), p(12, 12), color, unit, 0, stroke);
    break;
  case Icon::undo:
  case Icon::redo: {
    const bool back = icon == Icon::undo;
    const auto q = [&](float x, float y) { return p(back ? x : 16 - x, y); };
    draw->AddLine(q(2, 6), q(7, 2), color, stroke);
    draw->AddLine(q(2, 6), q(7, 10), color, stroke);
    draw->AddBezierCubic(q(3, 6), q(14, 3), q(15, 13), q(8, 13), color, stroke);
    break;
  }
  case Icon::entity:
    line(8, 2, 14, 5);
    line(14, 5, 14, 11);
    line(14, 11, 8, 14);
    line(8, 14, 2, 11);
    line(2, 11, 2, 5);
    line(2, 5, 8, 2);
    line(2, 5, 8, 8);
    line(14, 5, 8, 8);
    line(8, 8, 8, 14);
    break;
  case Icon::properties:
    line(3, 4, 13, 4);
    line(3, 8, 13, 8);
    line(3, 12, 13, 12);
    draw->AddCircleFilled(p(6, 4), 2 * unit, color);
    draw->AddCircleFilled(p(10, 8), 2 * unit, color);
    draw->AddCircleFilled(p(6, 12), 2 * unit, color);
    break;
  case Icon::console:
    line(3, 4, 7, 8);
    line(7, 8, 3, 12);
    line(9, 12, 14, 12);
    break;
  case Icon::history:
    draw->AddCircle(p(8, 8), 6 * unit, color, 0, stroke);
    line(8, 4, 8, 8);
    line(8, 8, 11, 10);
    break;
  }
}
inline bool icon_button(Icon icon, const char *label) {
  const float size = ImGui::GetFontSize(),
              gap = ImGui::GetStyle().ItemInnerSpacing.x;
  const float space = ImGui::CalcTextSize(" ").x;
  const auto count = static_cast<std::size_t>(std::ceil((size + gap) / space));
  // Keep a native labelled button, with a stable ID independent of font
  // metrics.
  const std::string caption = std::string(count, ' ') + label + "###" + label;
  const bool clicked = ImGui::Button(caption.c_str());
  auto min = ImGui::GetItemRectMin(), max = ImGui::GetItemRectMax();
  draw_icon(icon,
            ImVec2(min.x + ImGui::GetStyle().FramePadding.x,
                   (min.y + max.y - size) * 0.5f),
            size, ImGui::GetColorU32(ImGuiCol_Text));
  return clicked;
}
inline void panel_heading(Icon icon, const char *title) {
  const float size = ImGui::GetFontSize();
  draw_icon(icon, ImGui::GetCursorScreenPos(), size,
            ImGui::GetColorU32(ImGuiCol_TextDisabled));
  ImGui::Dummy(ImVec2(size, size));
  ImGui::SameLine();
  ImGui::TextDisabled("%s", title);
  ImGui::Spacing();
}
} // namespace lyko
