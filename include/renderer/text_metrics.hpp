#pragma once

#include <string>

#ifdef ENABLE_FREETYPE
#include "renderer/font_manager.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

namespace xiaopeng {
namespace renderer {

struct TextMeasurement {
  float width = 0;
  float height = 0;
  float ascender = 0;
  float descender = 0;
};

/// Provides text measurement for the layout engine.
/// Uses FreeType when available, otherwise falls back to 8x8 bitmap metrics.
class TextMetrics {
public:
  static TextMetrics &instance() {
    static TextMetrics inst;
    return inst;
  }

  /// Measure a word's pixel width and height at the given font size.
  TextMeasurement measureWord(const std::string &word,
                              const std::string &fontFamily, int fontSize) {
    TextMeasurement m;
#ifdef ENABLE_FREETYPE
    auto &fm = FontManager::instance();
    if (!fm.initialize()) {
      return fallbackMeasure(word, fontSize);
    }
    auto font = fm.getFont(fontFamily);
    if (!font) {
      fm.loadSystemFont(fontFamily);
      font = fm.getFont(fontFamily);
    }
    if (!font || !font->ftFace) {
      return fallbackMeasure(word, fontSize);
    }

    FT_Set_Pixel_Sizes(font->ftFace, 0, fontSize);
    float totalWidth = 0;
    for (unsigned char c : word) {
      if (FT_Load_Char(font->ftFace, c, FT_LOAD_NO_BITMAP)) {
        totalWidth += fontSize * 0.5f; // fallback
        continue;
      }
      totalWidth += (font->ftFace->glyph->advance.x >> 6);
    }

    m.width = totalWidth;
    m.ascender = static_cast<float>(font->ftFace->size->metrics.ascender >> 6);
    m.descender =
        static_cast<float>(font->ftFace->size->metrics.descender >> 6);
    m.height = m.ascender - m.descender;
    return m;
#else
    return fallbackMeasure(word, fontSize);
#endif
  }

  /// Average character width for a given font.
  float charWidth(const std::string &fontFamily, int fontSize) {
#ifdef ENABLE_FREETYPE
    auto &fm = FontManager::instance();
    if (!fm.initialize())
      return static_cast<float>(fontSize) * 0.5f;
    auto font = fm.getFont(fontFamily);
    if (!font) {
      fm.loadSystemFont(fontFamily);
      font = fm.getFont(fontFamily);
    }
    if (!font || !font->ftFace)
      return static_cast<float>(fontSize) * 0.5f;

    FT_Set_Pixel_Sizes(font->ftFace, 0, fontSize);
    // Use 'x' as reference character
    if (FT_Load_Char(font->ftFace, 'x', FT_LOAD_NO_BITMAP) == 0) {
      return static_cast<float>(font->ftFace->glyph->advance.x >> 6);
    }
    return static_cast<float>(fontSize) * 0.5f;
#else
    (void)fontFamily;
    return 8.0f * (fontSize / 16.0f);
#endif
  }

  /// Compute line height for a given font.
  float lineHeight(const std::string &fontFamily, int fontSize) {
#ifdef ENABLE_FREETYPE
    auto &fm = FontManager::instance();
    if (!fm.initialize())
      return static_cast<float>(fontSize) * 1.2f;
    auto font = fm.getFont(fontFamily);
    if (!font) {
      fm.loadSystemFont(fontFamily);
      font = fm.getFont(fontFamily);
    }
    if (!font || !font->ftFace)
      return static_cast<float>(fontSize) * 1.2f;

    FT_Set_Pixel_Sizes(font->ftFace, 0, fontSize);
    float asc = static_cast<float>(font->ftFace->size->metrics.ascender >> 6);
    float desc = static_cast<float>(font->ftFace->size->metrics.descender >> 6);
    return asc - desc;
#else
    (void)fontFamily;
    return 20.0f * (fontSize / 16.0f);
#endif
  }

  /// Measure space width
  float spaceWidth(const std::string &fontFamily, int fontSize) {
#ifdef ENABLE_FREETYPE
    auto &fm = FontManager::instance();
    if (!fm.initialize())
      return static_cast<float>(fontSize) * 0.25f;
    auto font = fm.getFont(fontFamily);
    if (!font) {
      fm.loadSystemFont(fontFamily);
      font = fm.getFont(fontFamily);
    }
    if (!font || !font->ftFace)
      return static_cast<float>(fontSize) * 0.25f;

    FT_Set_Pixel_Sizes(font->ftFace, 0, fontSize);
    if (FT_Load_Char(font->ftFace, ' ', FT_LOAD_NO_BITMAP) == 0) {
      return static_cast<float>(font->ftFace->glyph->advance.x >> 6);
    }
    return static_cast<float>(fontSize) * 0.25f;
#else
    (void)fontFamily;
    return 8.0f * (fontSize / 16.0f);
#endif
  }

private:
  TextMetrics() = default;

  TextMeasurement fallbackMeasure(const std::string &word, int fontSize) {
    TextMeasurement m;
    float scale = static_cast<float>(fontSize) / 16.0f;
    m.width = static_cast<float>(word.length()) * 8.0f * scale;
    m.height = 16.0f * scale;
    m.ascender = 12.0f * scale;
    m.descender = -4.0f * scale;
    return m;
  }
};

} // namespace renderer
} // namespace xiaopeng
