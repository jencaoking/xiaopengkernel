#include <gui/canvas_2d.hpp>
#include <cmath>
#include <cstring>

namespace xiaopeng {
namespace gui {

// Color implementation
std::string Color::toString() const {
  return "rgba(" + std::to_string(r) + "," + std::to_string(g) + "," +
         std::to_string(b) + "," + std::to_string(a) + ")";
}

// Canvas2D implementation
Canvas2D::Canvas2D(int width, int height) : m_width(width), m_height(height) {
  m_pixels.resize(width * height * 4, 0);
}

Canvas2D::~Canvas2D() {}

// Drawing state
void Canvas2D::save() {
  m_transformStack.emplace_back(m_transform[0], m_transform[1], m_transform[2],
                               m_transform[3], m_transform[4], m_transform[5]);
}

void Canvas2D::restore() {
  if (!m_transformStack.empty()) {
    auto [a, b, c, d, e, f] = m_transformStack.back();
    m_transformStack.pop_back();
    m_transform[0] = a;
    m_transform[1] = b;
    m_transform[2] = c;
    m_transform[3] = d;
    m_transform[4] = e;
    m_transform[5] = f;
  }
}

// Transformations
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
  float newTransform[6];
  newTransform[0] = a * m_transform[0] + b * m_transform[2];
  newTransform[1] = a * m_transform[1] + b * m_transform[3];
  newTransform[2] = c * m_transform[0] + d * m_transform[2];
  newTransform[3] = c * m_transform[1] + d * m_transform[3];
  newTransform[4] = e * m_transform[0] + f * m_transform[2] + m_transform[4];
  newTransform[5] = e * m_transform[1] + f * m_transform[3] + m_transform[5];
  std::memcpy(m_transform, newTransform, sizeof(m_transform));
}

void Canvas2D::setTransform(float a, float b, float c, float d, float e, float f) {
  m_transform[0] = a;
  m_transform[1] = b;
  m_transform[2] = c;
  m_transform[3] = d;
  m_transform[4] = e;
  m_transform[5] = f;
}

void Canvas2D::resetTransform() {
  m_transform[0] = 1;
  m_transform[1] = 0;
  m_transform[2] = 0;
  m_transform[3] = 1;
  m_transform[4] = 0;
  m_transform[5] = 0;
}

// Compositing
void Canvas2D::setGlobalAlpha(float alpha) {
  m_globalAlpha = std::max(0.0f, std::min(1.0f, alpha));
}

// Colors and styles
void Canvas2D::setFillStyle(const Color &color) {
  m_fillStyle = color;
}

void Canvas2D::setFillStyle(const std::string &colorStr) {
  m_fillStyle = parseColor(colorStr);
}

void Canvas2D::setStrokeStyle(const Color &color) {
  m_strokeStyle = color;
}

void Canvas2D::setStrokeStyle(const std::string &colorStr) {
  m_strokeStyle = parseColor(colorStr);
}

// Line styles
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

// Paths
void Canvas2D::beginPath() {
  m_currentPath.clear();
  m_pathStarted = false;
}

void Canvas2D::closePath() {
  if (m_currentPath.size() >= 2) {
    m_currentPath.push_back(m_currentPath[0]);
  }
}

void Canvas2D::moveTo(float x, float y) {
  m_currentPath.emplace_back(x, y);
  m_pathStarted = true;
}

void Canvas2D::lineTo(float x, float y) {
  if (m_currentPath.empty()) {
    moveTo(x, y);
  } else {
    m_currentPath.emplace_back(x, y);
  }
}

void Canvas2D::quadraticCurveTo(float cpx, float cpy, float x, float y) {
  if (m_currentPath.empty()) {
    moveTo(cpx, cpy);
  }
  m_currentPath.emplace_back(x, y);
}

void Canvas2D::bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y) {
  if (m_currentPath.empty()) {
    moveTo(cp1x, cp1y);
  }
  m_currentPath.emplace_back(x, y);
}

void Canvas2D::arc(float x, float y, float radius, float startAngle, float endAngle, bool counterclockwise) {
  float step = counterclockwise ? -0.1f : 0.1f;
  float angle = startAngle;
  
  if (m_currentPath.empty()) {
    moveTo(x + radius * std::cos(angle), y + radius * std::sin(angle));
  }
  
  if (counterclockwise) {
    while (angle > endAngle) {
      angle += step;
      lineTo(x + radius * std::cos(angle), y + radius * std::sin(angle));
    }
  } else {
    while (angle < endAngle) {
      angle += step;
      lineTo(x + radius * std::cos(angle), y + radius * std::sin(angle));
    }
  }
}

void Canvas2D::arcTo(float x1, float y1, float x2, float y2, float radius) {
  if (m_currentPath.empty()) {
    moveTo(x1, y1);
  }
  lineTo(x2, y2);
}

void Canvas2D::rect(float x, float y, float width, float height) {
  beginPath();
  moveTo(x, y);
  lineTo(x + width, y);
  lineTo(x + width, y + height);
  lineTo(x, y + height);
  closePath();
}

void Canvas2D::ellipse(float x, float y, float radiusX, float radiusY, float rotation,
                       float startAngle, float endAngle, bool counterclockwise) {
  float step = counterclockwise ? -0.1f : 0.1f;
  float angle = startAngle;
  
  float cosRot = std::cos(rotation);
  float sinRot = std::sin(rotation);
  
  if (m_currentPath.empty()) {
    float px = x + (radiusX * std::cos(angle) * cosRot - radiusY * std::sin(angle) * sinRot);
    float py = y + (radiusX * std::cos(angle) * sinRot + radiusY * std::sin(angle) * cosRot);
    moveTo(px, py);
  }
  
  if (counterclockwise) {
    while (angle > endAngle) {
      angle += step;
      float px = x + (radiusX * std::cos(angle) * cosRot - radiusY * std::sin(angle) * sinRot);
      float py = y + (radiusX * std::cos(angle) * sinRot + radiusY * std::sin(angle) * cosRot);
      lineTo(px, py);
    }
  } else {
    while (angle < endAngle) {
      angle += step;
      float px = x + (radiusX * std::cos(angle) * cosRot - radiusY * std::sin(angle) * sinRot);
      float py = y + (radiusX * std::cos(angle) * sinRot + radiusY * std::sin(angle) * cosRot);
      lineTo(px, py);
    }
  }
}

// Drawing paths
void Canvas2D::fill() {
  if (m_currentPath.size() < 3) return;
  
  Color color = m_fillStyle;
  color.a = static_cast<uint8_t>(color.a * m_globalAlpha);
  
  for (const auto &point : m_currentPath) {
    int x = static_cast<int>(point.x);
    int y = static_cast<int>(point.y);
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
      setPixel(x, y, color);
    }
  }
}

void Canvas2D::stroke() {
  if (m_currentPath.size() < 2) return;
  
  Color color = m_strokeStyle;
  color.a = static_cast<uint8_t>(color.a * m_globalAlpha);
  
  for (size_t i = 1; i < m_currentPath.size(); ++i) {
    float x0 = m_currentPath[i-1].x;
    float y0 = m_currentPath[i-1].y;
    float x1 = m_currentPath[i].x;
    float y1 = m_currentPath[i].y;
    
    float dx = x1 - x0;
    float dy = y1 - y0;
    float length = std::sqrt(dx * dx + dy * dy);
    
    if (length < 1e-6) continue;
    
    float stepX = dx / length;
    float stepY = dy / length;
    
    float x = x0;
    float y = y0;
    
    for (float t = 0; t <= length; t += 0.5f) {
      int px = static_cast<int>(x);
      int py = static_cast<int>(y);
      if (px >= 0 && px < m_width && py >= 0 && py < m_height) {
        setPixel(px, py, color);
      }
      x += stepX * 0.5f;
      y += stepY * 0.5f;
    }
  }
}

void Canvas2D::fillRect(float x, float y, float width, float height) {
  Color color = m_fillStyle;
  color.a = static_cast<uint8_t>(color.a * m_globalAlpha);
  
  int startX = std::max(0, static_cast<int>(x));
  int startY = std::max(0, static_cast<int>(y));
  int endX = std::min(m_width, static_cast<int>(x + width));
  int endY = std::min(m_height, static_cast<int>(y + height));
  
  for (int py = startY; py < endY; ++py) {
    for (int px = startX; px < endX; ++px) {
      setPixel(px, py, color);
    }
  }
}

void Canvas2D::strokeRect(float x, float y, float width, float height) {
  Color color = m_strokeStyle;
  color.a = static_cast<uint8_t>(color.a * m_globalAlpha);
  
  int startX = std::max(0, static_cast<int>(x));
  int startY = std::max(0, static_cast<int>(y));
  int endX = std::min(m_width, static_cast<int>(x + width));
  int endY = std::min(m_height, static_cast<int>(y + height));
  
  for (int px = startX; px < endX; ++px) {
    if (startY < m_height) setPixel(px, startY, color);
    if (endY > 0) setPixel(px, endY - 1, color);
  }
  
  for (int py = startY; py < endY; ++py) {
    if (startX < m_width) setPixel(startX, py, color);
    if (endX > 0) setPixel(endX - 1, py, color);
  }
}

void Canvas2D::clearRect(float x, float y, float width, float height) {
  int startX = std::max(0, static_cast<int>(x));
  int startY = std::max(0, static_cast<int>(y));
  int endX = std::min(m_width, static_cast<int>(x + width));
  int endY = std::min(m_height, static_cast<int>(y + height));
  
  for (int py = startY; py < endY; ++py) {
    for (int px = startX; px < endX; ++px) {
      int idx = (py * m_width + px) * 4;
      m_pixels[idx] = 0;
      m_pixels[idx + 1] = 0;
      m_pixels[idx + 2] = 0;
      m_pixels[idx + 3] = 0;
    }
  }
}

// Text
void Canvas2D::setFont(const std::string &font) {
  m_font = font;
}

void Canvas2D::setTextAlign(TextAlign align) {
  m_textAlign = align;
}

void Canvas2D::setTextBaseline(TextBaseline baseline) {
  m_textBaseline = baseline;
}

void Canvas2D::fillText(const std::string &text, float x, float y) {
  Color color = m_fillStyle;
  color.a = static_cast<uint8_t>(color.a * m_globalAlpha);
  
  int startX = static_cast<int>(x);
  int startY = static_cast<int>(y);
  
  for (size_t i = 0; i < text.size(); ++i) {
    int px = startX + i * 8;
    int py = startY;
    if (px >= 0 && px < m_width && py >= 0 && py < m_height) {
      setPixel(px, py, color);
      if (py + 1 < m_height) setPixel(px, py + 1, color);
    }
  }
}

void Canvas2D::strokeText(const std::string &text, float x, float y) {
  fillText(text, x, y);
}

// Image drawing
void Canvas2D::drawImage(int imageId, float x, float y) {
}

void Canvas2D::drawImage(int imageId, float x, float y, float width, float height) {
}

void Canvas2D::drawImage(int imageId, float sx, float sy, float sw, float sh,
                         float dx, float dy, float dw, float dh) {
}

// Get image data
std::vector<uint8_t> Canvas2D::getImageData(int x, int y, int width, int height) const {
  std::vector<uint8_t> result;
  
  int startX = std::max(0, x);
  int startY = std::max(0, y);
  int endX = std::min(m_width, x + width);
  int endY = std::min(m_height, y + height);
  
  for (int py = startY; py < endY; ++py) {
    for (int px = startX; px < endX; ++px) {
      int idx = (py * m_width + px) * 4;
      result.push_back(m_pixels[idx]);
      result.push_back(m_pixels[idx + 1]);
      result.push_back(m_pixels[idx + 2]);
      result.push_back(m_pixels[idx + 3]);
    }
  }
  
  return result;
}

void Canvas2D::putImageData(const std::vector<uint8_t> &data, int x, int y) {
  int dataIdx = 0;
  
  for (int py = y; py < y + m_height && dataIdx < static_cast<int>(data.size()); ++py) {
    for (int px = x; px < x + m_width && dataIdx < static_cast<int>(data.size()); ++px) {
      if (px >= 0 && px < m_width && py >= 0 && py < m_height) {
        int idx = (py * m_width + px) * 4;
        m_pixels[idx] = data[dataIdx++];
        if (dataIdx < static_cast<int>(data.size())) m_pixels[idx + 1] = data[dataIdx++];
        if (dataIdx < static_cast<int>(data.size())) m_pixels[idx + 2] = data[dataIdx++];
        if (dataIdx < static_cast<int>(data.size())) m_pixels[idx + 3] = data[dataIdx++];
      }
    }
  }
}

// Clear canvas
void Canvas2D::clear() {
  std::fill(m_pixels.begin(), m_pixels.end(), 0);
}

// Helper methods
Color Canvas2D::parseColor(const std::string &colorStr) {
  if (colorStr.empty()) return Color();
  
  if (colorStr.substr(0, 4) == "rgba") {
    size_t start = colorStr.find('(') + 1;
    size_t end = colorStr.find(')');
    std::string values = colorStr.substr(start, end - start);
    
    std::vector<int> rgba;
    size_t pos = 0;
    while (pos < values.size()) {
      size_t comma = values.find(',', pos);
      if (comma == std::string::npos) comma = values.size();
      rgba.push_back(std::stoi(values.substr(pos, comma - pos)));
      pos = comma + 1;
      while (pos < values.size() && values[pos] == ' ') pos++;
    }
    
    if (rgba.size() >= 3) {
      uint8_t a = rgba.size() >= 4 ? static_cast<uint8_t>(rgba[3]) : 255;
      return Color(static_cast<uint8_t>(rgba[0]), static_cast<uint8_t>(rgba[1]),
                   static_cast<uint8_t>(rgba[2]), a);
    }
  }
  
  if (colorStr.substr(0, 3) == "rgb") {
    size_t start = colorStr.find('(') + 1;
    size_t end = colorStr.find(')');
    std::string values = colorStr.substr(start, end - start);
    
    std::vector<int> rgb;
    size_t pos = 0;
    while (pos < values.size()) {
      size_t comma = values.find(',', pos);
      if (comma == std::string::npos) comma = values.size();
      rgb.push_back(std::stoi(values.substr(pos, comma - pos)));
      pos = comma + 1;
      while (pos < values.size() && values[pos] == ' ') pos++;
    }
    
    if (rgb.size() >= 3) {
      return Color(static_cast<uint8_t>(rgb[0]), static_cast<uint8_t>(rgb[1]),
                   static_cast<uint8_t>(rgb[2]));
    }
  }
  
  std::map<std::string, Color> namedColors = {
    {"black", Color(0, 0, 0)},
    {"white", Color(255, 255, 255)},
    {"red", Color(255, 0, 0)},
    {"green", Color(0, 255, 0)},
    {"blue", Color(0, 0, 255)},
    {"yellow", Color(255, 255, 0)},
    {"cyan", Color(0, 255, 255)},
    {"magenta", Color(255, 0, 255)},
    {"gray", Color(128, 128, 128)},
    {"grey", Color(128, 128, 128)}
  };
  
  auto it = namedColors.find(colorStr);
  if (it != namedColors.end()) {
    return it->second;
  }
  
  return Color(0, 0, 0);
}

void Canvas2D::setPixel(int x, int y, const Color &color) {
  if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
    return;
  }
  
  int idx = (y * m_width + x) * 4;
  
  float alpha = color.a / 255.0f;
  float invAlpha = 1.0f - alpha;
  
  m_pixels[idx] = static_cast<uint8_t>(color.r * alpha + m_pixels[idx] * invAlpha);
  m_pixels[idx + 1] = static_cast<uint8_t>(color.g * alpha + m_pixels[idx + 1] * invAlpha);
  m_pixels[idx + 2] = static_cast<uint8_t>(color.b * alpha + m_pixels[idx + 2] * invAlpha);
  m_pixels[idx + 3] = static_cast<uint8_t>(color.a + m_pixels[idx + 3] * invAlpha);
}

} // namespace gui
} // namespace xiaopeng
