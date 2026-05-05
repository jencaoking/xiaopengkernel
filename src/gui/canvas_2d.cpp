#include <gui/canvas_2d.hpp>
#include <cmath>
#include <algorithm>
#include <regex>
#include <sstream>

namespace xiaopeng {
namespace gui {

// Color implementation
std::string Color::toString() const {
  std::stringstream ss;
  if (a == 255) {
    ss << "rgb(" << static_cast<int>(r) << "," << static_cast<int>(g) << "," << static_cast<int>(b) << ")";
  } else {
    ss << "rgba(" << static_cast<int>(r) << "," << static_cast<int>(g) << "," << static_cast<int>(b) << "," << (a / 255.0) << ")";
  }
  return ss.str();
}

// LinearGradient implementation
LinearGradient::LinearGradient(float x0, float y0, float x1, float y1)
    : m_x0(x0), m_y0(y0), m_x1(x1), m_y1(y1) {}

void LinearGradient::addColorStop(float offset, const Color &color) {
  m_stops.emplace_back(offset, color);
  std::sort(m_stops.begin(), m_stops.end(), [](const GradientStop &a, const GradientStop &b) {
    return a.offset < b.offset;
  });
}

void LinearGradient::addColorStop(float offset, const std::string &colorStr) {
  addColorStop(offset, parseColor(colorStr));
}

Color LinearGradient::getColorAt(float t) const {
  if (m_stops.empty()) return Color(0, 0, 0);
  
  if (t <= 0) return m_stops.front().color;
  if (t >= 1) return m_stops.back().color;
  
  for (size_t i = 0; i < m_stops.size() - 1; ++i) {
    if (t >= m_stops[i].offset && t <= m_stops[i + 1].offset) {
      float t2 = (t - m_stops[i].offset) / (m_stops[i + 1].offset - m_stops[i].offset);
      return m_stops[i].color.blend(m_stops[i + 1].color, t2);
    }
  }
  
  return m_stops.back().color;
}

Color LinearGradient::parseColor(const std::string &colorStr) const {
  if (colorStr.substr(0, 1) == "#") {
    std::string hex = colorStr.substr(1);
    if (hex.length() == 6) {
      return Color(
          static_cast<uint8_t>(std::stoi(hex.substr(0, 2), nullptr, 16)),
          static_cast<uint8_t>(std::stoi(hex.substr(2, 2), nullptr, 16)),
          static_cast<uint8_t>(std::stoi(hex.substr(4, 2), nullptr, 16))
      );
    }
  }
  
  std::regex rgbPattern(R"(rgba?\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*(?:,\s*([\d.]+)\s*)?\))");
  std::smatch match;
  if (std::regex_search(colorStr, match, rgbPattern)) {
    uint8_t r = static_cast<uint8_t>(std::stoi(match[1].str()));
    uint8_t g = static_cast<uint8_t>(std::stoi(match[2].str()));
    uint8_t b = static_cast<uint8_t>(std::stoi(match[3].str()));
    uint8_t a = match[4].matched ? static_cast<uint8_t>(std::stod(match[4].str()) * 255) : 255;
    return Color(r, g, b, a);
  }
  
  return Color(0, 0, 0);
}

// RadialGradient implementation
RadialGradient::RadialGradient(float x0, float y0, float r0, float x1, float y1, float r1)
    : m_x0(x0), m_y0(y0), m_r0(r0), m_x1(x1), m_y1(y1), m_r1(r1) {}

void RadialGradient::addColorStop(float offset, const Color &color) {
  m_stops.emplace_back(offset, color);
  std::sort(m_stops.begin(), m_stops.end(), [](const GradientStop &a, const GradientStop &b) {
    return a.offset < b.offset;
  });
}

void RadialGradient::addColorStop(float offset, const std::string &colorStr) {
  addColorStop(offset, parseColor(colorStr));
}

Color RadialGradient::getColorAt(float t) const {
  if (m_stops.empty()) return Color(0, 0, 0);
  
  if (t <= 0) return m_stops.front().color;
  if (t >= 1) return m_stops.back().color;
  
  for (size_t i = 0; i < m_stops.size() - 1; ++i) {
    if (t >= m_stops[i].offset && t <= m_stops[i + 1].offset) {
      float t2 = (t - m_stops[i].offset) / (m_stops[i + 1].offset - m_stops[i].offset);
      return m_stops[i].color.blend(m_stops[i + 1].color, t2);
    }
  }
  
  return m_stops.back().color;
}

Color RadialGradient::parseColor(const std::string &colorStr) const {
  if (colorStr.substr(0, 1) == "#") {
    std::string hex = colorStr.substr(1);
    if (hex.length() == 6) {
      return Color(
          static_cast<uint8_t>(std::stoi(hex.substr(0, 2), nullptr, 16)),
          static_cast<uint8_t>(std::stoi(hex.substr(2, 2), nullptr, 16)),
          static_cast<uint8_t>(std::stoi(hex.substr(4, 2), nullptr, 16))
      );
    }
  }
  
  std::regex rgbPattern(R"(rgba?\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*(?:,\s*([\d.]+)\s*)?\))");
  std::smatch match;
  if (std::regex_search(colorStr, match, rgbPattern)) {
    uint8_t r = static_cast<uint8_t>(std::stoi(match[1].str()));
    uint8_t g = static_cast<uint8_t>(std::stoi(match[2].str()));
    uint8_t b = static_cast<uint8_t>(std::stoi(match[3].str()));
    uint8_t a = match[4].matched ? static_cast<uint8_t>(std::stod(match[4].str()) * 255) : 255;
    return Color(r, g, b, a);
  }
  
  return Color(0, 0, 0);
}

// Pattern implementation
Pattern::Pattern(int imageId, PatternRepeat repeat)
    : m_imageId(imageId), m_repeat(repeat) {}

// ImageData implementation
ImageData::ImageData(int width, int height)
    : m_width(width), m_height(height), m_data(width * height * 4, 0) {}

ImageData::ImageData(int width, int height, const std::vector<uint8_t> &data)
    : m_width(width), m_height(height), m_data(data) {}

Color ImageData::getPixel(int x, int y) const {
  if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
    return Color(0, 0, 0, 0);
  }
  size_t idx = (y * m_width + x) * 4;
  return Color(
      m_data[idx],
      m_data[idx + 1],
      m_data[idx + 2],
      m_data[idx + 3]
  );
}

void ImageData::setPixel(int x, int y, const Color &color) {
  if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
    return;
  }
  size_t idx = (y * m_width + x) * 4;
  m_data[idx] = color.r;
  m_data[idx + 1] = color.g;
  m_data[idx + 2] = color.b;
  m_data[idx + 3] = color.a;
}

// Canvas2D implementation
Canvas2D::Canvas2D(int width, int height)
    : m_width(width), m_height(height), m_pixels(width * height * 4, 255) {}

Canvas2D::~Canvas2D() {}

void Canvas2D::save() {
  m_transformStack.push_back(m_transform);
}

void Canvas2D::restore() {
  if (!m_transformStack.empty()) {
    m_transform = m_transformStack.back();
    m_transformStack.pop_back();
  }
}

void Canvas2D::translate(float x, float y) {
  transform(1, 0, 0, 1, x, y);
}

void Canvas2D::rotate(float angle) {
  float cosAngle = std::cos(angle);
  float sinAngle = std::sin(angle);
  transform(cosAngle, sinAngle, -sinAngle, cosAngle, 0, 0);
}

void Canvas2D::scale(float x, float y) {
  transform(x, 0, 0, y, 0, 0);
}

void Canvas2D::transform(float a, float b, float c, float d, float e, float f) {
  float newA = a * m_transform.a + b * m_transform.c;
  float newB = a * m_transform.b + b * m_transform.d;
  float newC = c * m_transform.a + d * m_transform.c;
  float newD = c * m_transform.b + d * m_transform.d;
  float newE = e * m_transform.a + f * m_transform.c + m_transform.e;
  float newF = e * m_transform.b + f * m_transform.d + m_transform.f;
  
  m_transform.a = newA;
  m_transform.b = newB;
  m_transform.c = newC;
  m_transform.d = newD;
  m_transform.e = newE;
  m_transform.f = newF;
}

void Canvas2D::setTransform(float a, float b, float c, float d, float e, float f) {
  m_transform = Transform(a, b, c, d, e, f);
}

void Canvas2D::resetTransform() {
  m_transform = Transform();
}

void Canvas2D::setGlobalAlpha(float alpha) {
  m_globalAlpha = std::max(0.0f, std::min(1.0f, alpha));
}

void Canvas2D::setGlobalCompositeOperation(const std::string &operation) {
  m_globalCompositeOperation = operation;
}

Color Canvas2D::parseColor(const std::string &colorStr) {
  if (colorStr.substr(0, 1) == "#") {
    std::string hex = colorStr.substr(1);
    if (hex.length() == 6) {
      return Color(
          static_cast<uint8_t>(std::stoi(hex.substr(0, 2), nullptr, 16)),
          static_cast<uint8_t>(std::stoi(hex.substr(2, 2), nullptr, 16)),
          static_cast<uint8_t>(std::stoi(hex.substr(4, 2), nullptr, 16))
      );
    }
  }
  
  std::regex rgbPattern(R"(rgba?\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*(?:,\s*([\d.]+)\s*)?\))");
  std::smatch match;
  if (std::regex_search(colorStr, match, rgbPattern)) {
    uint8_t r = static_cast<uint8_t>(std::stoi(match[1].str()));
    uint8_t g = static_cast<uint8_t>(std::stoi(match[2].str()));
    uint8_t b = static_cast<uint8_t>(std::stoi(match[3].str()));
    uint8_t a = match[4].matched ? static_cast<uint8_t>(std::stod(match[4].str()) * 255) : 255;
    return Color(r, g, b, a);
  }
  
  return Color(0, 0, 0);
}

void Canvas2D::setFillStyle(const Color &color) {
  m_fillStyleColor = color;
  m_fillStyleGradient = nullptr;
  m_fillStylePattern = nullptr;
}

void Canvas2D::setFillStyle(const std::string &colorStr) {
  setFillStyle(parseColor(colorStr));
}

void Canvas2D::setFillStyle(Gradient *gradient) {
  m_fillStyleGradient = gradient;
  m_fillStylePattern = nullptr;
}

void Canvas2D::setFillStyle(Pattern *pattern) {
  m_fillStylePattern = pattern;
  m_fillStyleGradient = nullptr;
}

void Canvas2D::setStrokeStyle(const Color &color) {
  m_strokeStyleColor = color;
  m_strokeStyleGradient = nullptr;
  m_strokeStylePattern = nullptr;
}

void Canvas2D::setStrokeStyle(const std::string &colorStr) {
  setStrokeStyle(parseColor(colorStr));
}

void Canvas2D::setStrokeStyle(Gradient *gradient) {
  m_strokeStyleGradient = gradient;
  m_strokeStylePattern = nullptr;
}

void Canvas2D::setStrokeStyle(Pattern *pattern) {
  m_strokeStylePattern = pattern;
  m_strokeStyleGradient = nullptr;
}

void Canvas2D::setLineWidth(float width) {
  m_lineWidth = std::max(0.0f, width);
}

void Canvas2D::setLineCap(LineCap cap) {
  m_lineCap = cap;
}

void Canvas2D::setLineJoin(LineJoin join) {
  m_lineJoin = join;
}

void Canvas2D::setMiterLimit(float limit) {
  m_miterLimit = std::max(0.0f, limit);
}

void Canvas2D::setLineDash(const std::vector<float> &dash) {
  m_lineDash = dash;
}

void Canvas2D::setLineDashOffset(float offset) {
  m_lineDashOffset = offset;
}

void Canvas2D::setShadowColor(const Color &color) {
  m_shadowColor = color;
}

void Canvas2D::setShadowColor(const std::string &colorStr) {
  setShadowColor(parseColor(colorStr));
}

void Canvas2D::setShadowBlur(float blur) {
  m_shadowBlur = std::max(0.0f, blur);
}

void Canvas2D::setShadowOffsetX(float offset) {
  m_shadowOffsetX = offset;
}

void Canvas2D::setShadowOffsetY(float offset) {
  m_shadowOffsetY = offset;
}

void Canvas2D::setFont(const std::string &font) {
  m_font = font;
}

void Canvas2D::setTextAlign(TextAlign align) {
  m_textAlign = align;
}

void Canvas2D::setTextBaseline(TextBaseline baseline) {
  m_textBaseline = baseline;
}

void Canvas2D::setTextShadow(const std::string &shadow) {
  m_textShadow = shadow;
}

void Canvas2D::beginPath() {
  m_currentPath.clear();
  m_pathStarted = false;
}

void Canvas2D::closePath() {
  if (m_currentPath.empty()) return;
  
  auto firstCmd = m_currentPath.front();
  if (firstCmd.type == PathCommand::Type::MoveTo && !m_currentPath.empty()) {
    float x = firstCmd.args[0];
    float y = firstCmd.args[1];
    m_currentPath.push_back({PathCommand::Type::LineTo, {x, y}});
  }
}

void Canvas2D::moveTo(float x, float y) {
  m_currentPath.push_back({PathCommand::Type::MoveTo, {x, y}});
  m_currentPoint = Point(x, y);
  m_pathStarted = true;
}

void Canvas2D::lineTo(float x, float y) {
  m_currentPath.push_back({PathCommand::Type::LineTo, {x, y}});
  m_currentPoint = Point(x, y);
}

void Canvas2D::quadraticCurveTo(float cpx, float cpy, float x, float y) {
  m_currentPath.push_back({PathCommand::Type::QuadCurveTo, {cpx, cpy, x, y}});
  m_currentPoint = Point(x, y);
}

void Canvas2D::bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y) {
  m_currentPath.push_back({PathCommand::Type::BezierCurveTo, {cp1x, cp1y, cp2x, cp2y, x, y}});
  m_currentPoint = Point(x, y);
}

void Canvas2D::arc(float x, float y, float radius, float startAngle, float endAngle, bool counterclockwise) {
  m_currentPath.push_back({PathCommand::Type::Arc, {x, y, radius, startAngle, endAngle, counterclockwise ? 1.0f : 0.0f}});
  float finalAngle = counterclockwise ? startAngle - (startAngle - endAngle) : endAngle;
  m_currentPoint = Point(x + radius * std::cos(finalAngle), y + radius * std::sin(finalAngle));
}

void Canvas2D::arcTo(float x1, float y1, float x2, float y2, float radius) {
  m_currentPath.push_back({PathCommand::Type::ArcTo, {x1, y1, x2, y2, radius}});
  m_currentPoint = Point(x2, y2);
}

void Canvas2D::rect(float x, float y, float width, float height) {
  m_currentPath.push_back({PathCommand::Type::Rect, {x, y, width, height}});
  m_currentPoint = Point(x, y);
}

void Canvas2D::ellipse(float x, float y, float radiusX, float radiusY, float rotation, float startAngle, float endAngle, bool counterclockwise) {
  m_currentPath.push_back({PathCommand::Type::Ellipse, {x, y, radiusX, radiusY, rotation, startAngle, endAngle, counterclockwise ? 1.0f : 0.0f}});
  float finalAngle = counterclockwise ? startAngle - (startAngle - endAngle) : endAngle;
  float cosRot = std::cos(rotation);
  float sinRot = std::sin(rotation);
  float px = radiusX * std::cos(finalAngle);
  float py = radiusY * std::sin(finalAngle);
  m_currentPoint = Point(x + px * cosRot - py * sinRot, y + px * sinRot + py * cosRot);
}

void Canvas2D::clip() {
}

void Canvas2D::fill() {
  std::vector<Point> points;
  
  for (const auto &cmd : m_currentPath) {
    switch (cmd.type) {
    case PathCommand::Type::MoveTo:
      points.push_back(Point(cmd.args[0], cmd.args[1]));
      break;
    case PathCommand::Type::LineTo:
      points.push_back(Point(cmd.args[0], cmd.args[1]));
      break;
    case PathCommand::Type::Rect: {
      float x = cmd.args[0], y = cmd.args[1], w = cmd.args[2], h = cmd.args[3];
      fillRect(x, y, w, h);
      return;
    }
    case PathCommand::Type::Arc: {
      float cx = cmd.args[0], cy = cmd.args[1], r = cmd.args[2];
      for (int i = 0; i < 360; ++i) {
        float angle = i * M_PI / 180.0f;
        points.push_back(Point(cx + r * std::cos(angle), cy + r * std::sin(angle)));
      }
      break;
    }
    default:
      break;
    }
  }
  
  if (points.size() >= 3) {
    fillPolygon(points);
  }
}

void Canvas2D::stroke() {
  Point lastPoint;
  bool first = true;
  
  for (const auto &cmd : m_currentPath) {
    switch (cmd.type) {
    case PathCommand::Type::MoveTo:
      lastPoint = Point(cmd.args[0], cmd.args[1]);
      first = true;
      break;
    case PathCommand::Type::LineTo: {
      Point current(cmd.args[0], cmd.args[1]);
      if (!first) {
        drawLine(lastPoint.x, lastPoint.y, current.x, current.y);
      }
      lastPoint = current;
      first = false;
      break;
    }
    case PathCommand::Type::Rect: {
      float x = cmd.args[0], y = cmd.args[1], w = cmd.args[2], h = cmd.args[3];
      strokeRect(x, y, w, h);
      return;
    }
    default:
      break;
    }
  }
}

void Canvas2D::fillRect(float x, float y, float width, float height) {
  int ix = static_cast<int>(std::max(0.0f, x));
  int iy = static_cast<int>(std::max(0.0f, y));
  int iw = static_cast<int>(std::min(static_cast<float>(m_width - ix), width));
  int ih = static_cast<int>(std::min(static_cast<float>(m_height - iy), height));
  
  for (int row = iy; row < iy + ih; ++row) {
    for (int col = ix; col < ix + iw; ++col) {
      setPixel(col, row, m_fillStyleColor * m_globalAlpha);
    }
  }
}

void Canvas2D::strokeRect(float x, float y, float width, float height) {
  drawLine(x, y, x + width, y);
  drawLine(x + width, y, x + width, y + height);
  drawLine(x + width, y + height, x, y + height);
  drawLine(x, y + height, x, y);
}

void Canvas2D::clearRect(float x, float y, float width, float height) {
  int ix = static_cast<int>(std::max(0.0f, x));
  int iy = static_cast<int>(std::max(0.0f, y));
  int iw = static_cast<int>(std::min(static_cast<float>(m_width - ix), width));
  int ih = static_cast<int>(std::min(static_cast<float>(m_height - iy), height));
  
  for (int row = iy; row < iy + ih; ++row) {
    for (int col = ix; col < ix + iw; ++col) {
      size_t idx = (row * m_width + col) * 4;
      m_pixels[idx] = 0;
      m_pixels[idx + 1] = 0;
      m_pixels[idx + 2] = 0;
      m_pixels[idx + 3] = 0;
    }
  }
}

void Canvas2D::fillText(const std::string &text, float x, float y) {
}

void Canvas2D::strokeText(const std::string &text, float x, float y) {
}

std::tuple<float, float> Canvas2D::measureText(const std::string &text) const {
  return {text.length() * 8.0f, 12.0f};
}

void Canvas2D::drawImage(int imageId, float x, float y) {
}

void Canvas2D::drawImage(int imageId, float x, float y, float width, float height) {
}

void Canvas2D::drawImage(int imageId, float sx, float sy, float sw, float sh,
                          float dx, float dy, float dw, float dh) {
}

std::unique_ptr<ImageData> Canvas2D::createImageData(int width, int height) const {
  return std::make_unique<ImageData>(width, height);
}

std::unique_ptr<ImageData> Canvas2D::createImageData(const ImageData &imageData) const {
  return std::make_unique<ImageData>(imageData.width(), imageData.height(), imageData.data());
}

std::unique_ptr<ImageData> Canvas2D::getImageData(int x, int y, int width, int height) const {
  int ix = std::max(0, x);
  int iy = std::max(0, y);
  int iw = std::min(m_width - ix, width);
  int ih = std::min(m_height - iy, height);
  
  std::unique_ptr<ImageData> imgData = std::make_unique<ImageData>(iw, ih);
  
  for (int row = 0; row < ih; ++row) {
    for (int col = 0; col < iw; ++col) {
      size_t srcIdx = ((iy + row) * m_width + (ix + col)) * 4;
      size_t dstIdx = (row * iw + col) * 4;
      imgData->data()[dstIdx] = m_pixels[srcIdx];
      imgData->data()[dstIdx + 1] = m_pixels[srcIdx + 1];
      imgData->data()[dstIdx + 2] = m_pixels[srcIdx + 2];
      imgData->data()[dstIdx + 3] = m_pixels[srcIdx + 3];
    }
  }
  
  return imgData;
}

void Canvas2D::putImageData(const ImageData &imageData, int x, int y) {
  putImageData(imageData, x, y, 0, 0, imageData.width(), imageData.height());
}

void Canvas2D::putImageData(const ImageData &imageData, int x, int y, int dirtyX, int dirtyY, int dirtyWidth, int dirtyHeight) {
  int ix = std::max(0, x + dirtyX);
  int iy = std::max(0, y + dirtyY);
  int iw = std::min(m_width - ix, dirtyWidth);
  int ih = std::min(m_height - iy, dirtyHeight);
  
  int srcX = std::max(0, -dirtyX);
  int srcY = std::max(0, -dirtyY);
  
  for (int row = 0; row < ih; ++row) {
    for (int col = 0; col < iw; ++col) {
      size_t srcIdx = ((srcY + row) * imageData.width() + (srcX + col)) * 4;
      size_t dstIdx = ((iy + row) * m_width + (ix + col)) * 4;
      m_pixels[dstIdx] = imageData.data()[srcIdx];
      m_pixels[dstIdx + 1] = imageData.data()[srcIdx + 1];
      m_pixels[dstIdx + 2] = imageData.data()[srcIdx + 2];
      m_pixels[dstIdx + 3] = imageData.data()[srcIdx + 3];
    }
  }
}

std::unique_ptr<LinearGradient> Canvas2D::createLinearGradient(float x0, float y0, float x1, float y1) {
  return std::make_unique<LinearGradient>(x0, y0, x1, y1);
}

std::unique_ptr<RadialGradient> Canvas2D::createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1) {
  return std::make_unique<RadialGradient>(x0, y0, r0, x1, y1, r1);
}

std::unique_ptr<Pattern> Canvas2D::createPattern(int imageId, const std::string &repeat) {
  PatternRepeat repeatMode = PatternRepeat::Repeat;
  if (repeat == "repeat-x") repeatMode = PatternRepeat::RepeatX;
  else if (repeat == "repeat-y") repeatMode = PatternRepeat::RepeatY;
  else if (repeat == "no-repeat") repeatMode = PatternRepeat::NoRepeat;
  
  return std::make_unique<Pattern>(imageId, repeatMode);
}

void Canvas2D::addHitRegion(const std::map<std::string, std::string> &options) {
}

void Canvas2D::removeHitRegion(const std::string &id) {
}

void Canvas2D::clearHitRegions() {
}

void Canvas2D::clear() {
  std::fill(m_pixels.begin(), m_pixels.end(), 255);
}

void Canvas2D::setPixel(int x, int y, const Color &color) {
  if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
    return;
  }
  size_t idx = (y * m_width + x) * 4;
  m_pixels[idx] = color.r;
  m_pixels[idx + 1] = color.g;
  m_pixels[idx + 2] = color.b;
  m_pixels[idx + 3] = color.a;
}

void Canvas2D::drawLine(float x1, float y1, float x2, float y2) {
  int ix1 = static_cast<int>(x1);
  int iy1 = static_cast<int>(y1);
  int ix2 = static_cast<int>(x2);
  int iy2 = static_cast<int>(y2);
  
  int dx = std::abs(ix2 - ix1);
  int dy = std::abs(iy2 - iy1);
  int sx = ix1 < ix2 ? 1 : -1;
  int sy = iy1 < iy2 ? 1 : -1;
  int err = dx - dy;
  
  int x = ix1;
  int y = iy1;
  
  while (true) {
    setPixel(x, y, m_strokeStyleColor * m_globalAlpha);
    
    if (x == ix2 && y == iy2) break;
    
    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x += sx;
    }
    if (e2 < dx) {
      err += dx;
      y += sy;
    }
  }
}

void Canvas2D::fillPolygon(const std::vector<Point> &points) {
  if (points.size() < 3) return;
  
  float minY = points[0].y, maxY = points[0].y;
  for (const auto &p : points) {
    minY = std::min(minY, p.y);
    maxY = std::max(maxY, p.y);
  }
  
  int minRow = std::max(0, static_cast<int>(minY));
  int maxRow = std::min(m_height - 1, static_cast<int>(maxY));
  
  for (int row = minRow; row <= maxRow; ++row) {
    std::vector<float> intersections;
    
    for (size_t i = 0; i < points.size(); ++i) {
      size_t j = (i + 1) % points.size();
      float y1 = points[i].y;
      float y2 = points[j].y;
      
      if ((y1 <= row && y2 > row) || (y2 <= row && y1 > row)) {
        float x = points[i].x + (row - y1) * (points[j].x - points[i].x) / (y2 - y1);
        intersections.push_back(x);
      }
    }
    
    std::sort(intersections.begin(), intersections.end());
    
    for (size_t i = 0; i < intersections.size(); i += 2) {
      int startX = std::max(0, static_cast<int>(intersections[i]));
      int endX = std::min(m_width - 1, static_cast<int>(intersections[i + 1]));
      
      for (int col = startX; col <= endX; ++col) {
        setPixel(col, row, m_fillStyleColor * m_globalAlpha);
      }
    }
  }
}

} // namespace gui
} // namespace xiaopeng
