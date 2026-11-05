#include "platform.hpp"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
#include <array>
#include <cstdint>
#include <sstream>
#include <stdexcept>
namespace lyko {
DesktopPlatform::DesktopPlatform() {
  try {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      throw std::runtime_error(SDL_GetError());
    }
    window_ =
        SDL_CreateWindow("Lykoware Editor | Lykoleon Studios", 1440, 900,
                         SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window_) {
      throw std::runtime_error(SDL_GetError());
    }
    renderer_ = SDL_CreateRenderer(window_, "software");
    if (!renderer_) {
      throw std::runtime_error(SDL_GetError());
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.ConfigFlags |=
        ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigDpiScaleFonts = true;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    sdl_backend_ = ImGui_ImplSDL3_InitForSDLRenderer(window_, renderer_);
    render_backend_ = ImGui_ImplSDLRenderer3_Init(renderer_);
    if (!sdl_backend_ || !render_backend_) {
      throw std::runtime_error("GUI backend initialization failed");
    }
  } catch (...) {
    cleanup();
    throw;
  }
}
DesktopPlatform::~DesktopPlatform() { cleanup(); }
void DesktopPlatform::cleanup() {
  if (preview_) {
    SDL_DestroyTexture(preview_);
  }
  if (render_backend_) {
    ImGui_ImplSDLRenderer3_Shutdown();
  }
  if (sdl_backend_) {
    ImGui_ImplSDL3_Shutdown();
  }
  if (ImGui::GetCurrentContext()) {
    ImGui::DestroyContext();
  }
  if (renderer_) {
    SDL_DestroyRenderer(renderer_);
  }
  if (window_) {
    SDL_DestroyWindow(window_);
  }
  SDL_Quit();
}
bool DesktopPlatform::begin_frame() {
  shortcuts.clear();
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL3_ProcessEvent(&event);
    if (event.type == SDL_EVENT_QUIT ||
        (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
         event.window.windowID == SDL_GetWindowID(window_))) {
      return false;
    }
    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
      std::string key = SDL_GetKeyName(event.key.key), prefix;
      if (key == "Return") {
        key = "Enter";
      }
      if (key.size() == 1 && key[0] >= 'a' && key[0] <= 'z') {
        key[0] = static_cast<char>(key[0] - 'a' + 'A');
      }
      if (event.key.mod & SDL_KMOD_CTRL) {
        prefix += "Ctrl+";
      }
      if (event.key.mod & SDL_KMOD_ALT) {
        prefix += "Alt+";
      }
      if (event.key.mod & SDL_KMOD_SHIFT) {
        prefix += "Shift+";
      }
      if (event.key.mod & SDL_KMOD_GUI) {
        prefix += "Super+";
      }
      shortcuts.push_back(prefix + key);
    }
  }
  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
  return true;
}
void DesktopPlatform::end_frame(const std::string &capture) {
  ImGui::Render();
  SDL_SetRenderDrawColor(renderer_, 21, 23, 29, 255);
  SDL_RenderClear(renderer_);
  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_);
  if (!capture.empty()) {
    auto *pixels = SDL_RenderReadPixels(renderer_, nullptr);
    if (!pixels) {
      throw std::runtime_error(SDL_GetError());
    }
    const bool saved = SDL_SaveBMP(pixels, capture.c_str());
    SDL_DestroySurface(pixels);
    if (!saved) {
      throw std::runtime_error(SDL_GetError());
    }
  }
  SDL_RenderPresent(renderer_);
  SDL_Delay(16);
}
void DesktopPlatform::set_preview(const std::string &ppm) {
  std::istringstream input(ppm);
  std::string magic, comment;
  std::getline(input, magic);
  std::getline(input, comment);
  unsigned width{}, height{}, maximum{};
  input >> width >> height >> maximum;
  if (magic != "P3" || width != 64 || height != 36 || maximum != 255) {
    throw std::runtime_error("Invalid mock preview");
  }
  std::array<unsigned char, 64 * 36 * 4> rgba{};
  for (unsigned i = 0; i < 64 * 36; ++i) {
    for (unsigned c = 0; c < 3; ++c) {
      unsigned v{};
      if (!(input >> v) || v > 255) {
        throw std::runtime_error("Invalid mock pixel");
      }
      rgba[i * 4 + c] = static_cast<unsigned char>(v);
    }
    rgba[i * 4 + 3] = 255;
  }
  if (!preview_) {
    preview_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32,
                                 SDL_TEXTUREACCESS_STATIC, 64, 36);
  }
  if (!preview_ || !SDL_UpdateTexture(preview_, nullptr, rgba.data(), 64 * 4)) {
    throw std::runtime_error(SDL_GetError());
  }
  SDL_SetTextureScaleMode(preview_, SDL_SCALEMODE_NEAREST);
}
void DesktopPlatform::draw_preview(ImVec2 size) {
  if (preview_) {
    ImGui::Image(
        static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(preview_)),
        size);
  } else {
    ImGui::TextDisabled("Preparing mock preview...");
  }
}
void DesktopPlatform::test_pointer(ImVec2 position, bool down) {
  SDL_Event motion{};
  motion.type = SDL_EVENT_MOUSE_MOTION;
  motion.motion.windowID = SDL_GetWindowID(window_);
  motion.motion.x = position.x;
  motion.motion.y = position.y;
  SDL_PushEvent(&motion);
  SDL_Event button{};
  button.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
  button.button.windowID = SDL_GetWindowID(window_);
  button.button.button = SDL_BUTTON_LEFT;
  button.button.x = position.x;
  button.button.y = position.y;
  SDL_PushEvent(&button);
}
void DesktopPlatform::test_key(char key, bool ctrl, bool shift) {
  SDL_Event event{};
  event.type = SDL_EVENT_KEY_DOWN;
  event.key.windowID = SDL_GetWindowID(window_);
  event.key.key = static_cast<SDL_Keycode>(key);
  event.key.scancode = SDL_GetScancodeFromKey(event.key.key, nullptr);
  event.key.mod = static_cast<SDL_Keymod>((ctrl ? SDL_KMOD_CTRL : 0) |
                                          (shift ? SDL_KMOD_SHIFT : 0));
  SDL_PushEvent(&event);
  event.type = SDL_EVENT_KEY_UP;
  event.key.mod = SDL_KMOD_NONE;
  SDL_PushEvent(&event);
}
void DesktopPlatform::test_text(const char *text) {
  SDL_Event event{};
  event.type = SDL_EVENT_TEXT_INPUT;
  event.text.windowID = SDL_GetWindowID(window_);
  event.text.text = text;
  SDL_PushEvent(&event);
}
} // namespace lyko
