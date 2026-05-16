#include <script/fetch_api.hpp>
#include <gui/canvas_2d.hpp>
#include <iostream>
#include <cassert>

using namespace xiaopeng;
using namespace xiaopeng::script;
using namespace xiaopeng::gui;

void testCanvasBasics() {
  std::cout << "=== Testing Canvas 2D Basics ===\n";

  Canvas2D canvas(100, 100);

  assert(canvas.width() == 100);
  assert(canvas.height() == 100);
  std::cout << "Canvas size: 100x100 ✓\n";

  // Test fillRect
  canvas.setFillStyle(Color(255, 0, 0));
  canvas.fillRect(10, 10, 20, 20);

  // Check pixel at (10, 10)
  const auto& pixels = canvas.pixels();
  int idx = (10 * 100 + 10) * 4;
  assert(pixels[idx] == 255);  // R
  assert(pixels[idx + 1] == 0);  // G
  assert(pixels[idx + 2] == 0);  // B
  std::cout << "fillRect with Color: ✓\n";

  // Test fillRect with string color
  canvas.setFillStyle("blue");
  canvas.fillRect(40, 40, 20, 20);

  idx = (40 * 100 + 40) * 4;
  assert(pixels[idx] == 0);
  assert(pixels[idx + 1] == 0);
  assert(pixels[idx + 2] == 255);
  std::cout << "fillRect with color string: ✓\n";

  // Test strokeRect
  canvas.setStrokeStyle("green");
  canvas.strokeRect(70, 70, 20, 20);
  std::cout << "strokeRect: ✓\n";

  // Test clearRect
  canvas.clearRect(10, 10, 20, 20);
  idx = (10 * 100 + 10) * 4;
  assert(pixels[idx] == 0);
  assert(pixels[idx + 1] == 0);
  assert(pixels[idx + 2] == 0);
  std::cout << "clearRect: ✓\n";

  // Test globalAlpha
  canvas.setGlobalAlpha(0.5f);
  canvas.setFillStyle("red");
  canvas.fillRect(50, 50, 10, 10);
  std::cout << "globalAlpha: ✓\n";

  // Test transformation
  canvas.save();
  canvas.translate(10, 10);
  canvas.restore();
  std::cout << "save/restore transformation: ✓\n";

  std::cout << "Canvas 2D Basics test PASSED!\n";
}

void testCanvasPaths() {
  std::cout << "\n=== Testing Canvas 2D Paths ===\n";

  Canvas2D canvas(100, 100);

  // Test line
  canvas.setStrokeStyle("blue");
  canvas.beginPath();
  canvas.moveTo(10, 10);
  canvas.lineTo(90, 90);
  canvas.stroke();
  std::cout << "Line drawing: ✓\n";

  // Test rectangle path
  canvas.rect(20, 20, 30, 30);
  canvas.setFillStyle("yellow");
  canvas.fill();
  std::cout << "Rectangle path: ✓\n";

  // Test arc
  canvas.beginPath();
  canvas.arc(70, 50, 20, 0, 3.14159f);
  canvas.stroke();
  std::cout << "Arc drawing: ✓\n";

  std::cout << "Canvas 2D Paths test PASSED!\n";
}

void testFetchBasics() {
  std::cout << "\n=== Testing Fetch API Basics ===\n";

  FetchOptions options;
  options.method = HttpMethod::Get;

  auto response = FetchManager::instance().fetchSync("http://example.com");
  assert(response != nullptr);
  std::cout << "Fetch sync call: ✓\n";

  std::cout << "Status: " << response->status() << "\n";
  std::cout << "Status text: " << response->statusText() << "\n";
  std::cout << "Headers count: " << response->headers().size() << "\n";

  if (response->status() == 200) {
    std::cout << "Body size: " << response->text().size() << " bytes\n";
  }

  std::cout << "Fetch API Basics test PASSED!\n";
}

void testFetchOptions() {
  std::cout << "\n=== Testing Fetch API Options ===\n";

  FetchOptions options;
  options.method = HttpMethod::Get;
  options.followRedirects = true;
  options.timeout = std::chrono::milliseconds(5000);

  auto response = FetchManager::instance().fetchSync("http://httpbin.org/get", options);
  assert(response != nullptr);
  std::cout << "Custom options: ✓\n";

  // Test headers
  FetchOptions optionsWithHeaders;
  optionsWithHeaders.method = HttpMethod::Get;
  optionsWithHeaders.headers["X-Custom-Header"] = "test-value";

  response = FetchManager::instance().fetchSync("http://httpbin.org/headers", optionsWithHeaders);
  assert(response != nullptr);
  std::cout << "Custom headers: ✓\n";

  std::cout << "Fetch API Options test PASSED!\n";
}

void testFetchAsync() {
  std::cout << "\n=== Testing Fetch API Async ===\n";

  auto future = FetchManager::instance().fetch("http://httpbin.org/delay/1");
  
  std::cout << "Async fetch started... ";
  
  auto response = future.get();
  assert(response != nullptr);
  std::cout << "completed ✓\n";

  std::cout << "Fetch API Async test PASSED!\n";
}

int main() {
  std::cout << "Running Web API tests (Fetch + Canvas 2D)\n";
  std::cout << "================================================\n";

  try {
    testCanvasBasics();
    testCanvasPaths();
    testFetchBasics();
    testFetchOptions();
    testFetchAsync();

    std::cout << "================================================\n";
    std::cout << "All Web API tests PASSED!\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Test FAILED: " << e.what() << "\n";
    return 1;
  }
}
