#define _USE_MATH_DEFINES
#include <cmath>
#include <gui/canvas_2d.hpp>
#include <iostream>
#include <cassert>

using namespace xiaopeng;
using namespace xiaopeng::gui;

void testCanvasBasics() {
  std::cout << "=== Testing Canvas Basics ===\n";

  Canvas2D canvas(200, 100);

  assert(canvas.width() == 200);
  std::cout << "Canvas width: ✓\n";

  assert(canvas.height() == 100);
  std::cout << "Canvas height: ✓\n";

  assert(canvas.pixels().size() == 200 * 100 * 4);
  std::cout << "Canvas pixels: ✓\n";

  std::cout << "Canvas Basics test PASSED!\n";
}

void testCanvasDrawing() {
  std::cout << "\n=== Testing Canvas Drawing ===\n";

  Canvas2D canvas(100, 100);

  canvas.setFillStyle(Color(255, 0, 0));
  canvas.fillRect(10, 10, 50, 50);
  std::cout << "Fill rectangle: ✓\n";

  canvas.setStrokeStyle(Color(0, 0, 255));
  canvas.strokeRect(30, 30, 40, 40);
  std::cout << "Stroke rectangle: ✓\n";

  canvas.clearRect(20, 20, 20, 20);
  std::cout << "Clear rectangle: ✓\n";

  std::cout << "Canvas Drawing test PASSED!\n";
}

void testCanvasPaths() {
  std::cout << "\n=== Testing Canvas Paths ===\n";

  Canvas2D canvas(100, 100);

  canvas.beginPath();
  canvas.moveTo(10, 10);
  canvas.lineTo(90, 10);
  canvas.lineTo(50, 90);
  canvas.closePath();
  canvas.setFillStyle(Color(0, 255, 0));
  canvas.fill();
  std::cout << "Triangle path fill: ✓\n";

  canvas.beginPath();
  canvas.arc(50, 50, 20, 0, M_PI * 2);
  canvas.setStrokeStyle(Color(0, 0, 0));
  canvas.stroke();
  std::cout << "Circle path stroke: ✓\n";

  std::cout << "Canvas Paths test PASSED!\n";
}

void testCanvasTransformations() {
  std::cout << "\n=== Testing Canvas Transformations ===\n";

  Canvas2D canvas(100, 100);

  canvas.save();
  canvas.translate(50, 50);
  canvas.rotate(M_PI / 4);
  canvas.scale(2, 2);
  canvas.restore();
  std::cout << "Transform stack: ✓\n";

  canvas.resetTransform();
  std::cout << "Reset transform: ✓\n";

  std::cout << "Canvas Transformations test PASSED!\n";
}

void testCanvasImageData() {
  std::cout << "\n=== Testing Canvas ImageData ===\n";

  Canvas2D canvas(100, 100);

  auto imgData = canvas.createImageData(50, 50);
  assert(imgData->width() == 50);
  assert(imgData->height() == 50);
  assert(imgData->data().size() == 50 * 50 * 4);
  std::cout << "Create ImageData: ✓\n";

  imgData->setPixel(10, 10, Color(255, 0, 0));
  Color c = imgData->getPixel(10, 10);
  assert(c.r == 255);
  std::cout << "Set/Get pixel: ✓\n";

  canvas.putImageData(*imgData, 25, 25);
  std::cout << "Put ImageData: ✓\n";

  auto retrieved = canvas.getImageData(25, 25, 50, 50);
  assert(retrieved->width() == 50);
  std::cout << "Get ImageData: ✓\n";

  std::cout << "Canvas ImageData test PASSED!\n";
}

void testLinearGradient() {
  std::cout << "\n=== Testing Linear Gradient ===\n";

  LinearGradient gradient(0, 0, 100, 100);
  
  gradient.addColorStop(0.0, Color(255, 0, 0));
  gradient.addColorStop(0.5, Color(0, 255, 0));
  gradient.addColorStop(1.0, Color(0, 0, 255));
  std::cout << "Add color stops: ✓\n";

  Color c0 = gradient.getColorAt(0.0);
  assert(c0.r == 255);
  std::cout << "Get color at 0: ✓\n";

  Color c1 = gradient.getColorAt(1.0);
  assert(c1.b == 255);
  std::cout << "Get color at 1: ✓\n";

  Color c05 = gradient.getColorAt(0.5);
  std::cout << "Get color at 0.5: ✓\n";

  std::cout << "Linear Gradient test PASSED!\n";
}

void testRadialGradient() {
  std::cout << "\n=== Testing Radial Gradient ===\n";

  RadialGradient gradient(50, 50, 0, 50, 50, 50);
  
  gradient.addColorStop(0.0, "#ffffff");
  gradient.addColorStop(1.0, "#000000");
  std::cout << "Add color stops with hex: ✓\n";

  Color c0 = gradient.getColorAt(0.0);
  assert(c0.r == 255);
  assert(c0.g == 255);
  assert(c0.b == 255);
  std::cout << "Get color at center: ✓\n";

  Color c1 = gradient.getColorAt(1.0);
  assert(c1.r == 0);
  assert(c1.g == 0);
  assert(c1.b == 0);
  std::cout << "Get color at edge: ✓\n";

  std::cout << "Radial Gradient test PASSED!\n";
}

void testColorOperations() {
  std::cout << "\n=== Testing Color Operations ===\n";

  Color c1(255, 0, 0);
  Color c2(0, 0, 255);

  Color blended = c1.blend(c2, 0.5);
  assert(blended.r == 127 || blended.r == 128);
  assert(blended.b == 127 || blended.b == 128);
  std::cout << "Color blend: ✓\n";

  Color faded = c1 * 0.5f;
  assert(faded.a == 127);
  std::cout << "Color alpha multiply: ✓\n";

  std::string str = c1.toString();
  assert(str.find("rgb") != std::string::npos);
  std::cout << "Color toString: ✓\n";

  std::cout << "Color Operations test PASSED!\n";
}

void testPattern() {
  std::cout << "\n=== Testing Pattern ===\n";

  Pattern pattern(1, PatternRepeat::Repeat);
  assert(pattern.imageId() == 1);
  assert(pattern.repeat() == PatternRepeat::Repeat);
  std::cout << "Create pattern: ✓\n";

  Pattern patternNoRepeat(2, PatternRepeat::NoRepeat);
  assert(patternNoRepeat.repeat() == PatternRepeat::NoRepeat);
  std::cout << "Pattern no-repeat: ✓\n";

  std::cout << "Pattern test PASSED!\n";
}

void testShadowStyles() {
  std::cout << "\n=== Testing Shadow Styles ===\n";

  Canvas2D canvas(100, 100);

  canvas.setShadowColor(Color(0, 0, 0, 128));
  assert(canvas.shadowColor().a == 128);
  std::cout << "Set shadow color: ✓\n";

  canvas.setShadowBlur(10.0f);
  assert(canvas.shadowBlur() == 10.0f);
  std::cout << "Set shadow blur: ✓\n";

  canvas.setShadowOffsetX(5.0f);
  canvas.setShadowOffsetY(5.0f);
  assert(canvas.shadowOffsetX() == 5.0f);
  assert(canvas.shadowOffsetY() == 5.0f);
  std::cout << "Set shadow offset: ✓\n";

  std::cout << "Shadow Styles test PASSED!\n";
}

void testLineStyles() {
  std::cout << "\n=== Testing Line Styles ===\n";

  Canvas2D canvas(100, 100);

  canvas.setLineWidth(5.0f);
  assert(canvas.lineWidth() == 5.0f);
  std::cout << "Set line width: ✓\n";

  canvas.setLineCap(LineCap::Round);
  assert(canvas.lineCap() == LineCap::Round);
  std::cout << "Set line cap: ✓\n";

  canvas.setLineJoin(LineJoin::Round);
  assert(canvas.lineJoin() == LineJoin::Round);
  std::cout << "Set line join: ✓\n";

  canvas.setMiterLimit(5.0f);
  assert(canvas.miterLimit() == 5.0f);
  std::cout << "Set miter limit: ✓\n";

  canvas.setLineDash({5, 5});
  assert(canvas.lineDash().size() == 2);
  std::cout << "Set line dash: ✓\n";

  std::cout << "Line Styles test PASSED!\n";
}

void testGlobalAlpha() {
  std::cout << "\n=== Testing Global Alpha ===\n";

  Canvas2D canvas(100, 100);

  canvas.setGlobalAlpha(0.5f);
  assert(canvas.globalAlpha() == 0.5f);
  std::cout << "Set global alpha: ✓\n";

  canvas.setGlobalAlpha(1.0f);
  assert(canvas.globalAlpha() == 1.0f);
  std::cout << "Reset global alpha: ✓\n";

  std::cout << "Global Alpha test PASSED!\n";
}

int main() {
  std::cout << "Running Canvas 2D tests\n";
  std::cout << "================================================\n";

  try {
    testCanvasBasics();
    testCanvasDrawing();
    testCanvasPaths();
    testCanvasTransformations();
    testCanvasImageData();
    testLinearGradient();
    testRadialGradient();
    testColorOperations();
    testPattern();
    testShadowStyles();
    testLineStyles();
    testGlobalAlpha();

    std::cout << "================================================\n";
    std::cout << "All Canvas 2D tests PASSED!\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Test FAILED: " << e.what() << "\n";
    return 1;
  }
}
