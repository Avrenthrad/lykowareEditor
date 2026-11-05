#pragma once
#include "imgui.h"
#include <string>
#include <vector>
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
namespace lyko {
class DesktopPlatform {
public:
  DesktopPlatform();
  ~DesktopPlatform();
  DesktopPlatform(const DesktopPlatform &) = delete;
  DesktopPlatform &operator=(const DesktopPlatform &) = delete;
  bool begin_frame();
  void end_frame(const std::string &capture = {});
  void set_preview(const std::string &ppm);
  void draw_preview(ImVec2 size);
  // Synthetic SDL input is used only by the opt-in interaction smoke harness.
  void test_pointer(ImVec2 position, bool down);
  void test_key(char key, bool ctrl, bool shift = false);
  void test_text(const char *text);
  std::vector<std::string> shortcuts;

private:
  void cleanup();
  SDL_Window *window_ = nullptr;
  SDL_Renderer *renderer_ = nullptr;
  SDL_Texture *preview_ = nullptr;
  bool sdl_backend_ = false, render_backend_ = false;
};
} // namespace lyko
