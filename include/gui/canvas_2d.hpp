#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace xiaopeng {
namespace gui {

// Color representation
struct Color {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t a = 255;

  Color() = default;
  Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
      : r(r), g(g), b(b), a(a) {}

  std::string toString() const;
};

// Point
struct Point {
  float x = 0;
  float y = 0;

  Point() = default;
  Point(float x, float y) : x(x), y(y) {}
};

// Size
struct Size {
  float width = 0;
  float height = 0;

  Size() = default;
  Size(float width, float height) : width(width), height(height) {}
};

// Rectangle
struct Rect {
  float x = 0;
  float y = 0;
  float width = 0;
  float height = 0;

  Rect() = default;
  Rect(float x, float y, float width, float height)
      : x(x), y(y), width(width), height(height) {}
};

// Line cap style
enum class LineCap {
  Butt,
  Round,
  Square
};

// Line join style
enum class LineJoin {
  Round,
  Bevel,
  Miter
};

// Text alignment
enum class TextAlign {
  Start,
  End,
  Left,
  Right,
  Center
};

// Text baseline
enum class TextBaseline {
  Top,
  Hanging,
  Middle,
  Alphabetic,
  Ideographic,
  Bottom
};

// Canvas 2D rendering context
class Canvas2D {
public:
  Canvas2D(int width, int height);
  ~Canvas2D();

  // Size properties
  int width() const { return m_width; }
  int height() const { return m_height; }

  // Drawing state
  void save();
  void restore();

  // Transformations
  void translate(float x, float y);
  void rotate(float angle);
  void scale(float x, float y);
  void transform(float a, float b, float c, float d, float e, float f);
  void setTransform(float a, float b, float c, float d, float e, float f);
  void resetTransform();

  // Compositing
  void setGlobalAlpha(float alpha);
  float globalAlpha() const { return m_globalAlpha; }

  // Colors and styles
  void setFillStyle(const Color &color);
  void setFillStyle(const std::string &colorStr);
  Color fillStyle() const { return m_fillStyle; }

  void setStrokeStyle(const Color &color);
  void setStrokeStyle(const std::string &colorStr);
  Color strokeStyle() const { return m_strokeStyle; }

  // Line styles
  void setLineWidth(float width);
  float lineWidth() const { return m_lineWidth; }

  void setLineCap(LineCap cap);
  LineCap lineCap() const { return m_lineCap; }

  void setLineJoin(LineJoin join);
  LineJoin lineJoin() const { return m_lineJoin; }

  void setMiterLimit(float limit);
  float miterLimit() const { return m_miterLimit; }

  // Paths
  void beginPath();
  void closePath();
  void moveTo(float x, float y);
  void lineTo(float x, float y);
  void quadraticCurveTo(float cpx, float cpy, float x, float y);
  void bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y);
  void arc(float x, float y, float radius, float startAngle, float endAngle, bool counterclockwise = false);
  void arcTo(float x1, float y1, float x2, float y2, float radius);
  void rect(float x, float y, float width, float height);
  void ellipse(float x, float y, float radiusX, float radiusY, float rotation, float startAngle, float endAngle, bool counterclockwise = false);

  // Drawing paths
  void fill();
  void stroke();
  void fillRect(float x, float y, float width, float height);
  void strokeRect(float x, float y, float width, float height);
  void clearRect(float x, float y, float width, float height);

  // Text
  void setFont(const std::string &font);
  std::string font() const { return m_font; }

  void setTextAlign(TextAlign align);
  TextAlign textAlign() const { return m_textAlign; }

  void setTextBaseline(TextBaseline baseline);
  TextBaseline textBaseline() const { return m_textBaseline; }

  void fillText(const std::string &text, float x, float y);
  void strokeText(const std::string &text, float x, float y);

  // Image drawing
  void drawImage(int imageId, float x, float y);
  void drawImage(int imageId, float x, float y, float width, float height);
  void drawImage(int imageId, float sx, float sy, float sw, float sh,
                 float dx, float dy, float dw, float dh);

  // Get image data
  std::vector<uint8_t> getImageData(int x, int y, int width, int height) const;
  void putImageData(const std::vector<uint8_t> &data, int x, int y);

  // Clear canvas
  void clear();

  // Get raw pixel data
  const std::vector<uint8_t> &pixels() const { return m_pixels; }

private:
  int m_width;
  int m_height;
  std::vector<uint8_t> m_pixels;

  // Drawing state
  Color m_fillStyle{0, 0, 0};
  Color m_strokeStyle{0, 0, 0};
  float m_lineWidth = 1.0f;
  LineCap m_lineCap = LineCap::Butt;
  LineJoin m_lineJoin = LineJoin::Miter;
  float m_miterLimit = 10.0f;
  float m_globalAlpha = 1.0f;

  // Text state
  std::string m_font = "10px sans-serif";
  TextAlign m_textAlign = TextAlign::Start;
  TextBaseline m_textBaseline = TextBaseline::Alphabetic;

  // Transform stack
  std::vector<std::tuple<float, float, float, float, float, float>> m_transformStack;
  float m_transform[6] = {1, 0, 0, 1, 0, 0};

  // Current path
  std::vector<Point> m_currentPath;
  bool m_pathStarted = false;

  // Helper methods
  Color parseColor(const std::string &colorStr);
  void setPixel(int x, int y, const Color &color);
};

} // namespace gui
} // namespace xiaopeng
