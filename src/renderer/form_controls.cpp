#include "layout/form_controls.hpp"
#include "renderer/canvas.hpp"
#include <algorithm>

namespace xiaopeng {
namespace layout {

void TextInput::paint(xiaopeng::renderer::Canvas *canvas) {
  if (!canvas) return;

  int x = this->x();
  int y = this->y();
  int w = this->w();
  int h = this->h();

  canvas->fillRect(x, y, w, h, xiaopeng::renderer::Color::White());
  canvas->drawRect(x, y, w, h, xiaopeng::renderer::Color{128, 128, 128, 255});

  canvas->drawTextVector(x + 4, y + h / 2 - fontSize_ / 2, value,
                        xiaopeng::renderer::Color::Black());
}

void TextInput::insertChar(char c) {
  value += c;
}

void TextInput::backspace() {
  if (!value.empty()) {
    value.pop_back();
  }
}

void TextArea::paint(xiaopeng::renderer::Canvas *canvas) {
  if (!canvas) return;

  int x = this->x();
  int y = this->y();
  int w = this->w();
  int h = this->h();

  canvas->fillRect(x, y, w, h, xiaopeng::renderer::Color::White());
  canvas->drawRect(x, y, w, h, xiaopeng::renderer::Color{128, 128, 128, 255});

  canvas->drawTextVector(x + 4, y + fontSize_, value,
                        xiaopeng::renderer::Color::Black());
}

void TextArea::insertChar(char c) {
  value += c;
}

void TextArea::backspace() {
  if (!value.empty()) {
    value.pop_back();
  }
}

void Button::paint(xiaopeng::renderer::Canvas *canvas) {
  if (!canvas) return;

  int x = this->x();
  int y = this->y();
  int w = this->w();
  int h = this->h();

  canvas->fillRect(x, y, w, h, xiaopeng::renderer::Color(200, 200, 200, 255));
  canvas->drawRect(x, y, w, h, xiaopeng::renderer::Color{128, 128, 128, 255});

  int textWidth = static_cast<int>(label_.length()) * 8;
  int textX = x + (w - textWidth) / 2;
  int textY = y + (h - fontSize_) / 2;

  canvas->drawTextVector(textX, textY, label_,
                        xiaopeng::renderer::Color::Black());
}

} // namespace layout
} // namespace xiaopeng
