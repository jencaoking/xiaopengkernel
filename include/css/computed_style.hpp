#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace xiaopeng {
namespace css {

// Represents a CSS color value
struct Color {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t a = 255;

  static Color Black() { return {0, 0, 0, 255}; }
  static Color White() { return {255, 255, 255, 255}; }
  static Color Red() { return {255, 0, 0, 255}; }
  static Color Green() { return {0, 255, 0, 255}; }
  static Color Blue() { return {0, 0, 255, 255}; }
  static Color Transparent() { return {0, 0, 0, 0}; }
};

// Represents a CSS length value
struct Length {
  float value = 0.0f;
  enum class Unit { Px, Percent, Em, Rem, Auto } unit = Unit::Auto;

  static Length Px(float v) { return {v, Unit::Px}; }
  static Length Percent(float v) { return {v, Unit::Percent}; }
  static Length Auto() { return {0, Unit::Auto}; }
};

// Custom property value - can be string, length, or color (defined after Length)
using CustomPropertyValue = std::variant<std::string, Length, Color>;

enum class Display {
  None,
  Block,
  Inline,
  InlineBlock,
  Flex,
  Grid
  // ...
};

// Flexbox specific properties
enum class FlexDirection {
  Row,
  RowReverse,
  Column,
  ColumnReverse
};

enum class FlexWrap {
  NoWrap,
  Wrap,
  WrapReverse
};

enum class JustifyContent {
  FlexStart,
  FlexEnd,
  Center,
  SpaceBetween,
  SpaceAround,
  SpaceEvenly
};

enum class AlignItems {
  Stretch,
  FlexStart,
  FlexEnd,
  Center,
  Baseline
};

enum class AlignContent {
  Stretch,
  FlexStart,
  FlexEnd,
  Center,
  SpaceBetween,
  SpaceAround
};

enum class AlignSelf {
  Auto,
  Stretch,
  FlexStart,
  FlexEnd,
  Center,
  Baseline
};

enum class Position { Static, Relative, Absolute, Fixed, Sticky };

enum class BoxSizing { ContentBox, BorderBox };

enum class TextAlign { Left, Right, Center, Justify };

enum class Overflow { Visible, Hidden, Scroll, Auto };

enum class Float { None, Left, Right };

enum class Clear { None, Left, Right, Both };

enum class Isolation { Auto, Isolate };

// Grid specific properties
enum class GridAutoFlow {
  Row, Column, RowDense, ColumnDense
};

enum class GridJustifyItems {
  Start, End, Center, Stretch, Auto
};

enum class GridAlignItems {
  Start, End, Center, Stretch, Auto
};

enum class GridJustifyContent {
  Start, End, Center, Stretch, SpaceAround, SpaceBetween, SpaceEvenly
};

enum class GridAlignContent {
  Start, End, Center, Stretch, SpaceAround, SpaceBetween, SpaceEvenly
};

enum class GridJustifySelf {
  Auto, Start, End, Center, Stretch
};

enum class GridAlignSelf {
  Auto, Start, End, Center, Stretch
};

struct TrackSize {
  enum class Type {
    Auto,
    Px,
    Percent,
    Fr,
    MinContent,
    MaxContent
  };
  Type type = Type::Auto;
  float value = 0.0f;
  
  static TrackSize Auto() { return {Type::Auto, 0.0f}; }
  static TrackSize Px(float v) { return {Type::Px, v}; }
  static TrackSize Percent(float v) { return {Type::Percent, v}; }
  static TrackSize Fr(float v) { return {Type::Fr, v}; }
};

// Holds the computed style for a DOM element
struct ComputedStyle {
  // Layout properties
  Display display = Display::Inline; // Default depends on element type usually
  Position position = Position::Static;
  BoxSizing boxSizing = BoxSizing::ContentBox;

  Length width = Length::Auto();
  Length height = Length::Auto();
  Length minWidth = Length::Auto();
  Length maxWidth = Length::Auto();
  Length minHeight = Length::Auto();
  Length maxHeight = Length::Auto();

  Length marginTop = Length::Px(0);
  Length marginRight = Length::Px(0);
  Length marginBottom = Length::Px(0);
  Length marginLeft = Length::Px(0);

  Length paddingTop = Length::Px(0);
  Length paddingRight = Length::Px(0);
  Length paddingBottom = Length::Px(0);
  Length paddingLeft = Length::Px(0);

  Length borderTopWidth = Length::Px(0);
  Length borderRightWidth = Length::Px(0);
  Length borderBottomWidth = Length::Px(0);
  Length borderLeftWidth = Length::Px(0);

  Color borderTopColor = Color::Transparent();
  Color borderRightColor = Color::Transparent();
  Color borderBottomColor = Color::Transparent();
  Color borderLeftColor = Color::Transparent();

  // Visual properties
  Color color = Color::Black();
  Color backgroundColor = Color::Transparent();

  // Typography (simplified)
  Length fontSize = Length::Px(16);
  std::string fontFamily = "serif";

  // Generic storage for other properties
  std::unordered_map<std::string, std::string> otherProperties;

  // Flexbox properties
  FlexDirection flexDirection = FlexDirection::Row;
  FlexWrap flexWrap = FlexWrap::NoWrap;
  JustifyContent justifyContent = JustifyContent::FlexStart;
  AlignItems alignItems = AlignItems::Stretch;
  AlignContent alignContent = AlignContent::Stretch;
  AlignSelf alignSelf = AlignSelf::Auto;
  
  float flexGrow = 0.0f;
  float flexShrink = 1.0f;
  Length flexBasis = Length::Auto();

  // Grid container properties
  GridAutoFlow gridAutoFlow = GridAutoFlow::Row;
  std::vector<TrackSize> gridTemplateColumns;
  std::vector<TrackSize> gridTemplateRows;
  std::vector<TrackSize> gridAutoColumns;
  std::vector<TrackSize> gridAutoRows;
  Length gridColumnGap = Length::Px(0);
  Length gridRowGap = Length::Px(0);
  GridJustifyItems gridJustifyItems = GridJustifyItems::Stretch;
  GridAlignItems gridAlignItems = GridAlignItems::Stretch;
  GridJustifyContent gridJustifyContent = GridJustifyContent::Start;
  GridAlignContent gridAlignContent = GridAlignContent::Start;
  
  // Grid item properties
  int gridColumnStart = 1;
  int gridColumnEnd = 2;
  int gridRowStart = 1;
  int gridRowEnd = 2;
  GridJustifySelf gridJustifySelf = GridJustifySelf::Auto;
  GridAlignSelf gridAlignSelf = GridAlignSelf::Auto;

  // Position properties
  Length top = Length::Auto();
  Length right = Length::Auto();
  Length bottom = Length::Auto();
  Length left = Length::Auto();

  // z-index
  int zIndex = 0;

  // Stacking context properties
  float opacity = 1.0f;
  std::string transform;
  std::string filter;
  std::string perspective;
  std::string willChange;
  Isolation isolation = Isolation::Auto;

  // Text and overflow properties
  TextAlign textAlign = TextAlign::Left;
  Overflow overflowX = Overflow::Visible;
  Overflow overflowY = Overflow::Visible;
  Float cssFloat = Float::None;
  Clear clear = Clear::None;

  std::unordered_map<std::string, CustomPropertyValue> customProperties;

  // Helper to get custom property value
  const CustomPropertyValue* getCustomProperty(const std::string& name) const {
    auto it = customProperties.find(name);
    if (it != customProperties.end()) {
      return &(it->second);
    }
    return nullptr;
  }

  // Helper to set custom property value
  void setCustomProperty(const std::string& name, const CustomPropertyValue& value) {
    customProperties[name] = value;
  }

  // Helper to get property string
  std::string getProperty(const std::string &name) const {
    if (name == "display") {
      switch (display) {
        case Display::None: return "none";
        case Display::Block: return "block";
        case Display::Inline: return "inline";
        case Display::InlineBlock: return "inline-block";
        case Display::Flex: return "flex";
        case Display::Grid: return "grid";
      }
    }
    if (name == "position") {
      switch (position) {
        case Position::Static: return "static";
        case Position::Relative: return "relative";
        case Position::Absolute: return "absolute";
        case Position::Fixed: return "fixed";
        case Position::Sticky: return "sticky";
      }
    }
    auto it = otherProperties.find(name);
    if (it != otherProperties.end())
      return it->second;
    return "";
  }
};

} // namespace css
} // namespace xiaopeng