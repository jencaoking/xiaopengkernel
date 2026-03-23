#pragma once

#include "canvas.hpp"
#include "image.hpp"
#include <string>
#include <vector>


namespace xiaopeng {
namespace renderer {

class BitmapCanvas : public Canvas {
public:
  BitmapCanvas(int width, int height);

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

  const std::vector<uint32_t> &buffer() const { return buffer_; }

  // Save buffer to PPM file
  bool saveToPPM(const std::string &filename);

private:
  int width_;
  int height_;
  std::vector<uint32_t>
      buffer_; // RGBA packed (0xAABBGGRR for easy viewing or R,G,B struct)
  // Actually PPM is RGB, so we can store struct Pixel { r,g,b } or uint32_t.
  // Let's use uint32_t 0xAABBGGRR (little endian) -> R at byte 0.

  void setPixel(int x, int y, Color color);
  void blendPixel(int x, int y, Color color); // Simple alpha blending

  // Font state
  std::string currentFontFamily_ = "Arial";
  int currentFontSize_ = 16;
};

} // namespace renderer
} // namespace xiaopeng
