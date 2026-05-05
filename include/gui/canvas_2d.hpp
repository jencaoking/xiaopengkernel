#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <array>
#include <cmath>

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
  
  Color operator*(float alpha) const {
    return Color(r, g, b, static_cast<uint8_t>(a * alpha));
  }
  
  Color blend(const Color &other, float t) const {
    return Color(
        static_cast<uint8_t>(r * (1 - t) + other.r * t),
        static_cast<uint8_t>(g * (1 - t) + other.g * t),
        static_cast<uint8_t>(b * (1 - t) + other.b * t),
        static_cast<uint8_t>(a * (1 - t) + other.a * t)
    );
  }
};

// Point
struct Point {
  float x = 0;
  float y = 0;

  Point() = default;
  Point(float x, float y) : x(x), y(y) {}
  
  Point operator+(const Point &other) const {
    return Point(x + other.x, y + other.y);
  }
  
  Point operator-(const Point &other) const {
    return Point(x - other.x, y - other.y);
  }
  
  Point operator*(float scalar) const {
    return Point(x * scalar, y * scalar);
  }
  
  float length() const {
    return std::sqrt(x * x + y * y);
  }
  
  Point normalize() const {
    float len = length();
    if (len == 0) return Point(0, 0);
    return Point(x / len, y / len);
  }
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

// Gradient type
enum class GradientType {
  Linear,
  Radial
};

// Gradient stop
struct GradientStop {
  float offset;
  Color color;
  
  GradientStop(float offset, const Color &color) : offset(offset), color(color) {}
};

// Gradient interface
class Gradient {
public:
  virtual ~Gradient() = default;
  
  virtual void addColorStop(float offset, const Color &color) = 0;
  virtual void addColorStop(float offset, const std::string &colorStr) = 0;
  virtual Color getColorAt(float t) const = 0;
  virtual GradientType type() const = 0;
};

// Linear gradient
class LinearGradient : public Gradient {
public:
  LinearGradient(float x0, float y0, float x1, float y1);
  
  void addColorStop(float offset, const Color &color) override;
  void addColorStop(float offset, const std::string &colorStr) override;
  Color getColorAt(float t) const override;
  GradientType type() const override { return GradientType::Linear; }
  
  float x0() const { return m_x0; }
  float y0() const { return m_y0; }
  float x1() const { return m_x1; }
  float y1() const { return m_y1; }
  
private:
  float m_x0, m_y0, m_x1, m_y1;
  std::vector<GradientStop> m_stops;
  
  Color parseColor(const std::string &colorStr) const;
};

// Radial gradient
class RadialGradient : public Gradient {
public:
  RadialGradient(float x0, float y0, float r0, float x1, float y1, float r1);
  
  void addColorStop(float offset, const Color &color) override;
  void addColorStop(float offset, const std::string &colorStr) override;
  Color getColorAt(float t) const override;
  GradientType type() const override { return GradientType::Radial; }
  
  float x0() const { return m_x0; }
  float y0() const { return m_y0; }
  float r0() const { return m_r0; }
  float x1() const { return m_x1; }
  float y1() const { return m_y1; }
  float r1() const { return m_r1; }
  
private:
  float m_x0, m_y0, m_r0, m_x1, m_y1, m_r1;
  std::vector<GradientStop> m_stops;
  
  Color parseColor(const std::string &colorStr) const;
};

// Pattern repetition
enum class PatternRepeat {
  Repeat,
  RepeatX,
  RepeatY,
  NoRepeat
};

// Pattern
class Pattern {
public:
  Pattern(int imageId, PatternRepeat repeat = PatternRepeat::Repeat);
  
  int imageId() const { return m_imageId; }
  PatternRepeat repeat() const { return m_repeat; }
  
private:
  int m_imageId;
  PatternRepeat m_repeat;
};

// Image data
class ImageData {
public:
  ImageData(int width, int height);
  ImageData(int width, int height, const std::vector<uint8_t> &data);
  
  int width() const { return m_width; }
  int height() const { return m_height; }
  std::vector<uint8_t> &data() { return m_data; }
  const std::vector<uint8_t> &data() const { return m_data; }
  
  Color getPixel(int x, int y) const;
  void setPixel(int x, int y, const Color &color);
  
private:
  int m_width;
  int m_height;
  std::vector<uint8_t> m_data;
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
  
  void setGlobalCompositeOperation(const std::string &operation);
  std::string globalCompositeOperation() const { return m_globalCompositeOperation; }

  // Colors and styles
  void setFillStyle(const Color &color);
  void setFillStyle(const std::string &colorStr);
  void setFillStyle(Gradient *gradient);
  void setFillStyle(Pattern *pattern);
  Color fillStyleColor() const { return m_fillStyleColor; }
  Gradient* fillStyleGradient() const { return m_fillStyleGradient; }
  Pattern* fillStylePattern() const { return m_fillStylePattern; }

  void setStrokeStyle(const Color &color);
  void setStrokeStyle(const std::string &colorStr);
  void setStrokeStyle(Gradient *gradient);
  void setStrokeStyle(Pattern *pattern);
  Color strokeStyleColor() const { return m_strokeStyleColor; }
  Gradient* strokeStyleGradient() const { return m_strokeStyleGradient; }
  Pattern* strokeStylePattern() const { return m_strokeStylePattern; }

  // Line styles
  void setLineWidth(float width);
  float lineWidth() const { return m_lineWidth; }

  void setLineCap(LineCap cap);
  LineCap lineCap() const { return m_lineCap; }

  void setLineJoin(LineJoin join);
  LineJoin lineJoin() const { return m_lineJoin; }

  void setMiterLimit(float limit);
  float miterLimit() const { return m_miterLimit; }
  
  void setLineDash(const std::vector<float> &dash);
  std::vector<float> lineDash() const { return m_lineDash; }
  void setLineDashOffset(float offset);
  float lineDashOffset() const { return m_lineDashOffset; }

  // Shadow styles
  void setShadowColor(const Color &color);
  void setShadowColor(const std::string &colorStr);
  Color shadowColor() const { return m_shadowColor; }
  
  void setShadowBlur(float blur);
  float shadowBlur() const { return m_shadowBlur; }
  
  void setShadowOffsetX(float offset);
  float shadowOffsetX() const { return m_shadowOffsetX; }
  
  void setShadowOffsetY(float offset);
  float shadowOffsetY() const { return m_shadowOffsetY; }

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
  
  void clip();

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
  
  void setTextShadow(const std::string &shadow);
  std::string textShadow() const { return m_textShadow; }

  void fillText(const std::string &text, float x, float y);
  void strokeText(const std::string &text, float x, float y);
  
  std::tuple<float, float> measureText(const std::string &text) const;

  // Image drawing
  void drawImage(int imageId, float x, float y);
  void drawImage(int imageId, float x, float y, float width, float height);
  void drawImage(int imageId, float sx, float sy, float sw, float sh,
                 float dx, float dy, float dw, float dh);

  // Image data
  std::unique_ptr<ImageData> createImageData(int width, int height) const;
  std::unique_ptr<ImageData> createImageData(const ImageData &imageData) const;
  std::unique_ptr<ImageData> getImageData(int x, int y, int width, int height) const;
  void putImageData(const ImageData &imageData, int x, int y);
  void putImageData(const ImageData &imageData, int x, int y, int dirtyX, int dirtyY, int dirtyWidth, int dirtyHeight);

  // Gradient creation
  std::unique_ptr<LinearGradient> createLinearGradient(float x0, float y0, float x1, float y1);
  std::unique_ptr<RadialGradient> createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1);

  // Pattern creation
  std::unique_ptr<Pattern> createPattern(int imageId, const std::string &repeat);

  // Hit regions
  void addHitRegion(const std::map<std::string, std::string> &options);
  void removeHitRegion(const std::string &id);
  void clearHitRegions();

  // Clear canvas
  void clear();

  // Get raw pixel data
  const std::vector<uint8_t> &pixels() const { return m_pixels; }

private:
  int m_width;
  int m_height;
  std::vector<uint8_t> m_pixels;

  // Drawing state
  Color m_fillStyleColor{0, 0, 0};
  Gradient* m_fillStyleGradient = nullptr;
  Pattern* m_fillStylePattern = nullptr;
  
  Color m_strokeStyleColor{0, 0, 0};
  Gradient* m_strokeStyleGradient = nullptr;
  Pattern* m_strokeStylePattern = nullptr;
  
  float m_lineWidth = 1.0f;
  LineCap m_lineCap = LineCap::Butt;
  LineJoin m_lineJoin = LineJoin::Miter;
  float m_miterLimit = 10.0f;
  std::vector<float> m_lineDash;
  float m_lineDashOffset = 0.0f;
  
  float m_globalAlpha = 1.0f;
  std::string m_globalCompositeOperation = "source-over";
  
  // Shadow state
  Color m_shadowColor{0, 0, 0, 0};
  float m_shadowBlur = 0.0f;
  float m_shadowOffsetX = 0.0f;
  float m_shadowOffsetY = 0.0f;

  // Text state
  std::string m_font = "10px sans-serif";
  TextAlign m_textAlign = TextAlign::Start;
  TextBaseline m_textBaseline = TextBaseline::Alphabetic;
  std::string m_textShadow;

  // Transform stack
  struct Transform {
    float a, b, c, d, e, f;
    Transform() : a(1), b(0), c(0), d(1), e(0), f(0) {}
    Transform(float a, float b, float c, float d, float e, float f)
        : a(a), b(b), c(c), d(d), e(e), f(f) {}
  };
  
  std::vector<Transform> m_transformStack;
  Transform m_transform;

  // Current path
  struct PathCommand {
    enum Type { MoveTo, LineTo, QuadCurveTo, BezierCurveTo, Arc, ArcTo, Rect, Ellipse } type;
    std::vector<float> args;
  };
  
  std::vector<PathCommand> m_currentPath;
  Point m_currentPoint;
  bool m_pathStarted = false;

  // Helper methods
  Color parseColor(const std::string &colorStr);
  void setPixel(int x, int y, const Color &color);
  void drawLine(float x1, float y1, float x2, float y2);
  void fillPolygon(const std::vector<Point> &points);
};

} // namespace gui
} // namespace xiaopeng
