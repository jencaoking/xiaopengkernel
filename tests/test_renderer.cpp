#include "renderer/bitmap_canvas.hpp"
#include "renderer/image.hpp"
#include "test_framework.hpp"
#include <fstream>
#include <iostream>
#include <vector>

using namespace xiaopeng::renderer;

TEST(Renderer_BitmapCanvasBasic) {
  BitmapCanvas canvas(100, 100);

  // Fill Red
  canvas.clear(Color::Red());

  // Check top-left pixel (should be red)
  // Format is 0xAARRGGBB
  EXPECT_EQ(canvas.buffer()[0], 0xFFFF0000);

  // Draw Blue Rect
  canvas.fillRect(10, 10, 50, 50, Color::Blue());
  // Check pixel at 15,15 (inside blue rect)
  EXPECT_EQ(canvas.buffer()[15 * 100 + 15], 0xFF0000FF);
}

TEST(Renderer_PpmColors) {
  // Test if saveToPPM writes the correct R G B channels.
  // Using a 1x1 canvas to easily read the output.
  BitmapCanvas canvas(1, 1);
  canvas.clear(Color::Red()); // 0xFFFF0000

  std::string filename = "test_ppm_colors.ppm";
  bool saved = canvas.saveToPPM(filename);
  EXPECT_TRUE(saved);

  std::ifstream ifs(filename);
  ASSERT_TRUE(ifs.is_open());
  std::string magic, width, height, maxval;
  ifs >> magic >> width >> height >> maxval;
  EXPECT_EQ(magic, "P3");
  EXPECT_EQ(width, "1");
  EXPECT_EQ(height, "1");
  EXPECT_EQ(maxval, "255");

  int r, g, b;
  ifs >> r >> g >> b;
  EXPECT_EQ(r, 255);
  EXPECT_EQ(g, 0);
  EXPECT_EQ(b, 0);
}

TEST(Renderer_ImageDecode) {
  // Minimal 1x1 24-bit BMP data (red pixel)
  std::vector<uint8_t> bmpData = {
      0x42, 0x4D, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00,
      0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x13, 0x0B, 0x00, 0x00, 0x13, 0x0B, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00};

  Image img(bmpData);
  EXPECT_TRUE(img.isValid());
  EXPECT_EQ(img.width(), 1);
  EXPECT_EQ(img.height(), 1);
  EXPECT_EQ(img.channels(), 4); // We force 4 channels in Image constructor
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  return xiaopeng::test::runTests();
}
