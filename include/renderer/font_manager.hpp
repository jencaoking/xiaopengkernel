#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations to avoid direct dependency if disabled
typedef struct FT_LibraryRec_ *FT_Library;
typedef struct FT_FaceRec_ *FT_Face;

namespace xiaopeng {
namespace renderer {

struct FontFace {
  FT_Face ftFace = nullptr;
  std::vector<uint8_t> data; // Keep font data alive if loaded from memory
};

class FontManager {
public:
  static FontManager &instance();

  bool initialize();
  void shutdown();

  // Load a font from file
  std::shared_ptr<FontFace> loadFont(const std::string &family,
                                     const std::string &path);

  // Get a loaded font
  std::shared_ptr<FontFace> getFont(const std::string &family);

  // System font discovery (Windows)
  bool loadSystemFont(const std::string &family);
  std::string resolveSystemFontPath(const std::string &family);

  FT_Library library() const { return library_; }

private:
  FontManager() = default;
  ~FontManager();

  FT_Library library_ = nullptr;
  std::unordered_map<std::string, std::shared_ptr<FontFace>> fonts_;
  bool initialized_ = false;
  bool defaultLoaded_ = false;
};

} // namespace renderer
} // namespace xiaopeng
