#pragma once

#include <renderer/bitmap_canvas.hpp>
#include <string>
#include <vector>

namespace xiaopeng::window {

// Mouse button identifiers
enum class MouseButton { Left, Right, Middle };

// A mouse click event captured from the window system
struct MouseEvent {
  int x = 0;
  int y = 0;
  MouseButton button = MouseButton::Left;
};

class Window {
public:
  virtual ~Window() = default;

  virtual bool initialize(int width, int height, const std::string &title) = 0;
  virtual void drawBuffer(const xiaopeng::renderer::BitmapCanvas &canvas) = 0;
  virtual bool processEvents() = 0; // Returns false if quit requested
  virtual bool isOpen() const = 0;

  // Retrieve and clear pending mouse click events since last call
  virtual std::vector<MouseEvent> pendingMouseClicks() { return {}; }

  // Retrieve and clear pending keyboard input characters since last call
  virtual std::vector<char> pendingKeyInputs() { return {}; }
};

} // namespace xiaopeng::window
