#include "renderer/text_shaper.hpp"
#include <iostream>

#ifdef ENABLE_FREETYPE
#include <hb-ft.h>
#include <hb.h>

#endif

namespace xiaopeng {
namespace renderer {

TextShaper::TextShaper() {
#ifdef ENABLE_FREETYPE
  buffer_ = hb_buffer_create();
#endif
}

TextShaper::~TextShaper() {
#ifdef ENABLE_FREETYPE
  if (buffer_) {
    hb_buffer_destroy(buffer_);
  }
#endif
}

std::vector<GlyphInfo> TextShaper::shape(const std::string &text,
                                         std::shared_ptr<FontFace> font,
                                         int fontSize,
                                         const std::string &language) {
  std::vector<GlyphInfo> glyphs;
#ifdef ENABLE_FREETYPE
  if (!font || !font->ftFace)
    return glyphs;

  // Set font size in FreeType
  FT_Set_Pixel_Sizes(font->ftFace, 0, fontSize);

  // Create HarfBuzz font from FreeType face
  hb_font_t *hbFont = hb_ft_font_create(font->ftFace, nullptr);

  hb_buffer_reset(buffer_);
  hb_buffer_add_utf8(buffer_, text.c_str(), -1, 0, -1);
  hb_buffer_set_direction(buffer_, HB_DIRECTION_LTR);
  hb_buffer_set_script(buffer_, HB_SCRIPT_LATIN);
  hb_buffer_set_language(buffer_,
                         hb_language_from_string(language.c_str(), -1));

  // Shape!
  hb_shape(hbFont, buffer_, nullptr, 0);

  // Get glyph info
  unsigned int glyphCount;
  hb_glyph_info_t *glyphInfo = hb_buffer_get_glyph_infos(buffer_, &glyphCount);
  hb_glyph_position_t *glyphPos =
      hb_buffer_get_glyph_positions(buffer_, &glyphCount);

  for (unsigned int i = 0; i < glyphCount; i++) {
    glyphs.push_back({glyphInfo[i].codepoint, glyphPos[i].x_offset,
                      glyphPos[i].y_offset, glyphPos[i].x_advance,
                      glyphPos[i].y_advance});
  }

  hb_font_destroy(hbFont);
#else
  (void)text;
  (void)font;
  (void)fontSize;
  (void)language;
#endif
  return glyphs;
}

} // namespace renderer
} // namespace xiaopeng
