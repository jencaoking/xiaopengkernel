#pragma once

#ifdef ENABLE_OPENGL

#include "canvas.hpp"
#include "image.hpp"
#include <string>
#include <unordered_map>
#include <vector>

// Include OpenGL headers via SDL for portability
#ifdef ENABLE_SDL
#include <SDL.h>
#include <SDL_opengl.h>
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#endif

namespace xiaopeng {
namespace renderer {

/// GPU-accelerated Canvas implementation using OpenGL 2.1 immediate mode.
/// Serves as a drop-in replacement for BitmapCanvas when a GL context is active.
class GLCanvas : public Canvas {
public:
  GLCanvas(int width, int height);
  ~GLCanvas() override;

  void resize(int width, int height) override;
  void clear(Color color) override;

  void fillRect(int x, int y, int width, int height, Color color) override;
  void drawRect(int x, int y, int width, int height, Color color) override;
  void drawText(int x, int y, const std::string &text, Color color,
                int scale = 1) override;

  void setFont(const std::string &family, int size) override;
  void drawTextVector(int x, int y, const std::string &text,
                      Color color) override;

  void drawImage(const Image &image, int x, int y, int width,
                 int height) override;

  int width() const override { return width_; }
  int height() const override { return height_; }

private:
  int width_;
  int height_;

  // Font state (mirrors BitmapCanvas)
  std::string currentFontFamily_ = "Arial";
  int currentFontSize_ = 16;

  // --- Texture caching ---

  // Image texture cache: keyed by raw pointer address of Image data
  // (since Images are cached by PaintingAlgorithm, pointer is stable)
  std::unordered_map<const uint8_t *, GLuint> imageTextureCache_;

  // 8x8 bitmap font texture (created once on first drawText call)
  GLuint bitmapFontTexture_ = 0;
  bool bitmapFontInitialized_ = false;
  void ensureBitmapFontTexture();

  // Helper to upload Image data as GL texture
  GLuint getOrCreateImageTexture(const Image &image);

  // Setup 2D orthographic projection
  void setupProjection();
};

} // namespace renderer
} // namespace xiaopeng

#endif // ENABLE_OPENGL
