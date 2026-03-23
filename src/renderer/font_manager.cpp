#include "renderer/font_manager.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>

#ifdef ENABLE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

namespace xiaopeng {
namespace renderer {

FontManager &FontManager::instance() {
  static FontManager instance;
  return instance;
}

FontManager::~FontManager() { shutdown(); }

bool FontManager::initialize() {
  if (initialized_)
    return true;
#ifdef ENABLE_FREETYPE
  if (FT_Init_FreeType(&library_)) {
    std::cerr << "[FontManager] Failed to initialize FreeType library"
              << std::endl;
    return false;
  }
  std::cout << "[FontManager] FreeType initialized" << std::endl;
  initialized_ = true;

  // Auto-load a default system font
  if (!defaultLoaded_) {
    if (loadSystemFont("Arial")) {
      defaultLoaded_ = true;
    } else if (loadSystemFont("Segoe UI")) {
      defaultLoaded_ = true;
    } else if (loadSystemFont("Verdana")) {
      defaultLoaded_ = true;
    }
    if (defaultLoaded_) {
      std::cout << "[FontManager] Default system font loaded" << std::endl;
    } else {
      std::cerr << "[FontManager] WARNING: No system font found" << std::endl;
    }
  }

  return true;
#else
  std::cout << "[FontManager] FreeType support disabled" << std::endl;
  return false;
#endif
}

void FontManager::shutdown() {
  if (!initialized_)
    return;
#ifdef ENABLE_FREETYPE
  fonts_.clear(); // Release faces before library
  FT_Done_FreeType(library_);
  library_ = nullptr;
  initialized_ = false;
  defaultLoaded_ = false;
#endif
}

std::shared_ptr<FontFace> FontManager::loadFont(const std::string &family,
                                                const std::string &path) {
#ifdef ENABLE_FREETYPE
  if (!initialized_ && !initialize())
    return nullptr;

  auto face = std::make_shared<FontFace>();
  if (FT_New_Face(library_, path.c_str(), 0, &face->ftFace)) {
    std::cerr << "[FontManager] Failed to load font: " << path << std::endl;
    return nullptr;
  }

  std::cout << "[FontManager] Loaded font: " << family << " from " << path
            << std::endl;
  fonts_[family] = face;
  return face;
#else
  (void)family;
  (void)path;
  return nullptr;
#endif
}

std::shared_ptr<FontFace> FontManager::getFont(const std::string &family) {
  auto it = fonts_.find(family);
  if (it != fonts_.end()) {
    return it->second;
  }
  return nullptr;
}

std::string FontManager::resolveSystemFontPath(const std::string &family) {
  // Windows system font directory
  std::string fontsDir = "C:\\Windows\\Fonts\\";

  // Map common family names to actual font files
  static const std::unordered_map<std::string, std::string> fontMap = {
      {"arial", "arial.ttf"},
      {"arial bold", "arialbd.ttf"},
      {"times new roman", "times.ttf"},
      {"courier new", "cour.ttf"},
      {"verdana", "verdana.ttf"},
      {"tahoma", "tahoma.ttf"},
      {"georgia", "georgia.ttf"},
      {"segoe ui", "segoeui.ttf"},
      {"consolas", "consola.ttf"},
      {"calibri", "calibri.ttf"},
      {"trebuchet ms", "trebuc.ttf"},
      {"lucida console", "lucon.ttf"},
      {"comic sans ms", "comic.ttf"},
      {"impact", "impact.ttf"},
      {"microsoft yahei", "msyh.ttc"},
      {"simsun", "simsun.ttc"},
      {"simhei", "simhei.ttf"},
      // Generic family names
      {"serif", "times.ttf"},
      {"sans-serif", "arial.ttf"},
      {"monospace", "consola.ttf"},
  };

  // Lowercase the family name for matching
  std::string lower = family;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  auto it = fontMap.find(lower);
  if (it != fontMap.end()) {
    std::string fullPath = fontsDir + it->second;
    if (std::filesystem::exists(fullPath)) {
      return fullPath;
    }
  }

  // Try direct filename match in Fonts directory
  std::string directPath = fontsDir + lower + ".ttf";
  if (std::filesystem::exists(directPath)) {
    return directPath;
  }

  return "";
}

bool FontManager::loadSystemFont(const std::string &family) {
#ifdef ENABLE_FREETYPE
  if (!initialized_ && !initialize())
    return false;

  // Check if already loaded
  if (getFont(family))
    return true;

  std::string path = resolveSystemFontPath(family);
  if (path.empty())
    return false;

  auto face = loadFont(family, path);
  if (!face)
    return false;

  // Register common aliases for the loaded font
  std::string lower = family;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  if (lower == "arial") {
    fonts_["sans-serif"] = face;
    fonts_["Arial"] = face;
    fonts_["arial"] = face;
  } else if (lower == "times new roman") {
    fonts_["serif"] = face;
    fonts_["Times New Roman"] = face;
  } else if (lower == "segoe ui") {
    if (!getFont("sans-serif"))
      fonts_["sans-serif"] = face;
    if (!getFont("Arial"))
      fonts_["Arial"] = face;
    fonts_["Segoe UI"] = face;
    fonts_["segoe ui"] = face;
  } else if (lower == "consolas") {
    fonts_["monospace"] = face;
    fonts_["Consolas"] = face;
  }

  return true;
#else
  (void)family;
  return false;
#endif
}

} // namespace renderer
} // namespace xiaopeng
