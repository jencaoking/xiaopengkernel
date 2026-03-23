#ifndef XIAOPENG_RENDERER_CANVAS_HPP
#define XIAOPENG_RENDERER_CANVAS_HPP

#include <cstdint>
#include <string>
#include <vector>

#ifdef Image
#undef Image
#endif

namespace xiaopeng {
namespace renderer {

class Image;

struct Color {
  uint8_t r, g, b, a;

  static Color Black() { return {0, 0, 0, 255}; }
  static Color White() { return {255, 255, 255, 255}; }
  static Color Red() { return {255, 0, 0, 255}; }
  static Color Blue() { return {0, 0, 255, 255}; }
  static Color Transparent() { return {0, 0, 0, 0}; }
};

class Canvas {
public:
  virtual ~Canvas() = default;

  virtual void resize(int width, int height) = 0;
  virtual void clear(Color color) = 0;

  // Basic primitives
  virtual void fillRect(int x, int y, int width, int height, Color color) = 0;
  virtual void drawRect(int x, int y, int width, int height,
                        Color color) = 0; // Border
  virtual void drawText(int x, int y, const std::string &text, Color color,
                        int scale = 1) = 0;

  // Advanced text rendering
  virtual void setFont(const std::string &family, int size) = 0;
  virtual void drawTextVector(int x, int y, const std::string &text,
                              Color color) = 0;

  // Image rendering
  virtual void drawImage(const xiaopeng::renderer::Image &image, int x, int y,
                         int width, int height) = 0;

  // Accessors
  virtual int width() const = 0;
  virtual int height() const = 0;
};

} // namespace renderer
} // namespace xiaopeng

#endif // XIAOPENG_RENDERER_CANVAS_HPP
