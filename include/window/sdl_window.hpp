#pragma once

#include <window/window.hpp>

#ifdef ENABLE_SDL
#include <SDL.h>
#endif

#ifdef ENABLE_OPENGL
#include <GL/gl.h>
#else
// Provide a stub GLuint type when OpenGL is not available
typedef unsigned int GLuint;
#endif

namespace xiaopeng::window {

class SdlWindow : public Window {
public:
  SdlWindow();
  ~SdlWindow() override;

  bool initialize(int width, int height, const std::string &title) override;
  void drawBuffer(const xiaopeng::renderer::BitmapCanvas &canvas) override;
  void drawOpenGLTexture(GLuint texture); // New method for OpenGL rendering
  bool processEvents() override;
  bool isOpen() const override { return m_isOpen; }

  // Return and clear pending mouse clicks
  std::vector<MouseEvent> pendingMouseClicks() override;

  // Return and clear pending keyboard inputs
  std::vector<char> pendingKeyInputs() override;

private:
  bool m_isOpen = false;
  std::vector<MouseEvent> m_pendingClicks;
  std::vector<char> m_pendingKeys;
#ifdef ENABLE_SDL
  SDL_Window *m_window = nullptr;
  SDL_Renderer *m_renderer = nullptr;
  SDL_Texture *m_texture = nullptr;
  int m_width = 0;
  int m_height = 0;
#endif
  static int s_sdlRefCount;
};

} // namespace xiaopeng::window
