#pragma once

#include "renderer/font_manager.hpp"
#include <cstdint>
#include <string>
#include <vector>


// Forward declarations
typedef struct hb_buffer_t hb_buffer_t;
typedef struct hb_font_t hb_font_t;

namespace xiaopeng {
namespace renderer {

struct GlyphInfo {
  uint32_t index;
  int32_t x_offset;
  int32_t y_offset;
  int32_t x_advance;
  int32_t y_advance;
};

class TextShaper {
public:
  TextShaper();
  ~TextShaper();

  // Shape text into glyphs and positions
  std::vector<GlyphInfo> shape(const std::string &text,
                               std::shared_ptr<FontFace> font, int fontSize,
                               const std::string &language = "en");

private:
  hb_buffer_t *buffer_ = nullptr;
};

} // namespace renderer
} // namespace xiaopeng
