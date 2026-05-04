#pragma once

#include "../dom/dom.hpp"
#include "computed_style.hpp"
#include "css_types.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace xiaopeng {
namespace css {

struct MatchResult {
  const Rule *rule;
  Selector::Specificity specificity;
  size_t position;
};

class StyleResolver {
public:
  StyleResolver() = default;

  ComputedStyle resolveStyle(dom::ElementPtr element, const StyleSheet &sheet) {
    ComputedStyle style;

    // 1. Collect matching rules
    std::vector<MatchResult> matches;
    size_t ruleIdx = 0;
    for (const auto &rule : sheet.rules) {
      for (const auto &selector : rule.selectors) {
        if (matchSelector(element, selector)) {
          matches.push_back({&rule, selector.specificity(), ruleIdx});
        }
      }
      ruleIdx++;
    }

    // 2. Sort by specificity and position
    std::sort(matches.begin(), matches.end(),
              [](const MatchResult &a, const MatchResult &b) {
                if (a.specificity != b.specificity) {
                  return a.specificity < b.specificity; // Sort ascending
                }
                return a.position < b.position;
              });

    // 3. Apply declarations
    for (const auto &match : matches) {
      applyDeclarations(style, match.rule->declarations);
    }

    return style;
  }

private:
  bool matchSelector(dom::ElementPtr element, const Selector &selector) {
    if (selector.parts.empty() || !element)
      return false;

    // Start matching from the last part of the selector against the current
    // element
    return matchRecursively(element, selector,
                            static_cast<int>(selector.parts.size()) - 1);
  }

  bool matchRecursively(dom::ElementPtr element, const Selector &selector,
                        int index) {
    if (index < 0)
      return true;
    if (!element)
      return false;

    // 1. Match the "Compound Selector" ending at 'index'
    // A compound selector consists of a chain of parts separated by
    // Combinator::None.
    int currentIndex = index;

    // Check parts belonging to the same element
    while (currentIndex >= 0) {
      // Check if current part matches
      if (!matchSimpleSelector(element, selector.parts[currentIndex])) {
        return false;
      }

      // Check if there is a 'None' combinator preceding this part
      if (currentIndex > 0 &&
          selector.combinators[currentIndex - 1] == Combinator::None) {
        currentIndex--;
      } else {
        break;
      }
    }

    // If we processed all parts and index became -1 (actually currentIndex will
    // be 0 if we consumed all), we need to check if we are done. If
    // currentIndex == 0, we consumed the whole selector chain successfully on
    // this element. BUT wait, a compound selector might be just "div".
    // currentIndex goes from 0 to 0. The loop breaks. If currentIndex == 0, we
    // are done!

    if (currentIndex == 0)
      return true;

    // 2. If not done, there must be a combinator at [currentIndex - 1]
    Combinator combinator = selector.combinators[currentIndex - 1];
    int nextIndex = currentIndex - 1;

    if (combinator == Combinator::Descendant) {
      dom::NodePtr parent = element->parentNode();
      while (parent && parent->nodeType() == dom::NodeType::Element) {
        auto parentElem = std::static_pointer_cast<dom::Element>(parent);
        if (matchRecursively(parentElem, selector, nextIndex)) {
          return true;
        }
        parent = parent->parentNode();
      }
      return false;
    } else if (combinator == Combinator::Child) {
      dom::NodePtr parent = element->parentNode();
      if (parent && parent->nodeType() == dom::NodeType::Element) {
        auto parentElem = std::static_pointer_cast<dom::Element>(parent);
        return matchRecursively(parentElem, selector, nextIndex);
      }
      return false;
    } else if (combinator == Combinator::NextSibling) {
      dom::ElementPtr prev = element->previousElementSibling();
      if (prev) {
        return matchRecursively(prev, selector, nextIndex);
      }
      return false;
    } else if (combinator == Combinator::SubsequentSibling) {
      dom::ElementPtr prev = element->previousElementSibling();
      while (prev) {
        if (matchRecursively(prev, selector, nextIndex)) {
          return true;
        }
        prev = prev->previousElementSibling();
      }
      return false;
    }

    return false;
  }

  bool matchSimpleSelector(dom::ElementPtr element,
                           const SimpleSelector &simple) {
    switch (simple.type) {
    case SelectorType::Tag:
      // Case-insensitive check generally appropriate for HTML
      // But my tokenizer might store simplified names.
      // dom::Element::tagName returns UPPERCASE. dom::Element::localName
      // returns original/lowercase. Let's assume lowercase for now as
      // normalized by tree builder.
      return dom::toLower(simple.value) == dom::toLower(element->localName());
    case SelectorType::Class:
      return element->hasClass(simple.value);
    case SelectorType::Id:
      return element->id() == simple.value;
    case SelectorType::Universal:
      return true;
    case SelectorType::Attribute:
      if (simple.value.empty()) {
        return element->hasAttribute(simple.attributeName);
      } else {
        auto val = element->getAttribute(simple.attributeName);
        return val.has_value() && val.value() == simple.attributeValue;
        // Operator support would be here
      }
    default:
      return false;
    }
  }

  void applyDeclarations(ComputedStyle &style,
                         const std::vector<Declaration> &decls) {
    for (const auto &decl : decls) {
      if (decl.property == "display") {
        if (decl.value == "none")
          style.display = Display::None;
        else if (decl.value == "block")
          style.display = Display::Block;
        else if (decl.value == "inline")
          style.display = Display::Inline;
        else if (decl.value == "flex")
          style.display = Display::Flex;
        else if (decl.value == "inline-block")
          style.display = Display::InlineBlock;
      } else if (decl.property == "width") {
        style.width = parseLength(decl.value);
      } else if (decl.property == "height") {
        style.height = parseLength(decl.value);
      } else if (decl.property == "color") {
        style.color = parseColor(decl.value);
      } else if (decl.property == "background-color") {
        style.backgroundColor = parseColor(decl.value);
      } else if (decl.property == "margin") {
        auto l = parseLength(decl.value);
        style.marginTop = style.marginRight = style.marginBottom =
            style.marginLeft = l;
      } else if (decl.property == "padding") {
        auto l = parseLength(decl.value);
        style.paddingTop = style.paddingRight = style.paddingBottom =
            style.paddingLeft = l;
      } else if (decl.property == "margin-top") {
        style.marginTop = parseLength(decl.value);
      } else if (decl.property == "margin-right") {
        style.marginRight = parseLength(decl.value);
      } else if (decl.property == "margin-bottom") {
        style.marginBottom = parseLength(decl.value);
      } else if (decl.property == "margin-left") {
        style.marginLeft = parseLength(decl.value);
      } else if (decl.property == "padding-top") {
        style.paddingTop = parseLength(decl.value);
      } else if (decl.property == "padding-right") {
        style.paddingRight = parseLength(decl.value);
      } else if (decl.property == "padding-bottom") {
        style.paddingBottom = parseLength(decl.value);
      } else if (decl.property == "padding-left") {
        style.paddingLeft = parseLength(decl.value);
      } else if (decl.property == "font-size") {
        style.fontSize = parseLength(decl.value);
      } else if (decl.property == "font-family") {
        style.fontFamily = decl.value;
      } else if (decl.property == "flex-direction") {
        if (decl.value == "row")
          style.flexDirection = FlexDirection::Row;
        else if (decl.value == "row-reverse")
          style.flexDirection = FlexDirection::RowReverse;
        else if (decl.value == "column")
          style.flexDirection = FlexDirection::Column;
        else if (decl.value == "column-reverse")
          style.flexDirection = FlexDirection::ColumnReverse;
      } else if (decl.property == "flex-wrap") {
        if (decl.value == "nowrap")
          style.flexWrap = FlexWrap::NoWrap;
        else if (decl.value == "wrap")
          style.flexWrap = FlexWrap::Wrap;
        else if (decl.value == "wrap-reverse")
          style.flexWrap = FlexWrap::WrapReverse;
      } else if (decl.property == "justify-content") {
        if (decl.value == "flex-start")
          style.justifyContent = JustifyContent::FlexStart;
        else if (decl.value == "flex-end")
          style.justifyContent = JustifyContent::FlexEnd;
        else if (decl.value == "center")
          style.justifyContent = JustifyContent::Center;
        else if (decl.value == "space-between")
          style.justifyContent = JustifyContent::SpaceBetween;
        else if (decl.value == "space-around")
          style.justifyContent = JustifyContent::SpaceAround;
        else if (decl.value == "space-evenly")
          style.justifyContent = JustifyContent::SpaceEvenly;
      } else if (decl.property == "align-items") {
        if (decl.value == "stretch")
          style.alignItems = AlignItems::Stretch;
        else if (decl.value == "flex-start")
          style.alignItems = AlignItems::FlexStart;
        else if (decl.value == "flex-end")
          style.alignItems = AlignItems::FlexEnd;
        else if (decl.value == "center")
          style.alignItems = AlignItems::Center;
        else if (decl.value == "baseline")
          style.alignItems = AlignItems::Baseline;
      } else if (decl.property == "flex-grow") {
        float f = 0.0f;
        try {
          f = std::stof(decl.value);
        } catch (...) {}
        style.flexGrow = f;
      } else if (decl.property == "flex-shrink") {
        float f = 1.0f;
        try {
          f = std::stof(decl.value);
        } catch (...) {}
        style.flexShrink = f;
      } else if (decl.property == "flex-basis") {
        style.flexBasis = parseLength(decl.value);
      } else if (decl.property.substr(0, 2) == "--") {
        // CSS Custom Properties (CSS Variables)
        std::string varName = decl.property;
        std::string varValue = decl.value;
        if (varValue.find("rgb") != std::string::npos) {
          style.setCustomProperty(varName, parseFunctionalColor(varValue));
        } else if (varValue.find("hsl") != std::string::npos) {
          style.setCustomProperty(varName, parseHSLColor(varValue));
        } else if (varValue[0] == '#') {
          style.setCustomProperty(varName, parseColor(varValue));
        } else if (varValue.find("px") != std::string::npos || 
                   varValue.find("em") != std::string::npos ||
                   varValue.find("rem") != std::string::npos ||
                   varValue.find("%") != std::string::npos) {
          style.setCustomProperty(varName, parseLength(varValue));
        } else {
          style.setCustomProperty(varName, varValue);
        }
      } else if (decl.property == "border") {
        // Simple shorthand parsing: "width style color"
        // We only care about width and color for now.
        // E.g. "2px solid red"
        std::stringstream ss(decl.value);
        std::string part;
        while (ss >> part) {
          if (part == "solid" || part == "dashed" || part == "dotted") {
            // Style ignored for now
          } else if (std::isdigit(part[0]) ||
                     (part.size() > 1 && part[0] == '.' &&
                      std::isdigit(part[1]))) {
            auto l = parseLength(part);
            style.borderTopWidth = style.borderRightWidth =
                style.borderBottomWidth = style.borderLeftWidth = l;
          } else {
            // Assume color
            auto c = parseColor(part);
            style.borderTopColor = style.borderRightColor =
                style.borderBottomColor = style.borderLeftColor = c;
          }
        }
      } else if (decl.property == "border-width") {
        auto l = parseLength(decl.value);
        style.borderTopWidth = style.borderRightWidth =
            style.borderBottomWidth = style.borderLeftWidth = l;
      } else if (decl.property == "border-color") {
        auto c = parseColor(decl.value);
        style.borderTopColor = style.borderRightColor =
            style.borderBottomColor = style.borderLeftColor = c;
      } else if (decl.property == "border-left-width") {
        style.borderLeftWidth = parseLength(decl.value);
      } else if (decl.property == "border-right-width") {
        style.borderRightWidth = parseLength(decl.value);
      } else if (decl.property == "border-top-width") {
        style.borderTopWidth = parseLength(decl.value);
      } else if (decl.property == "border-bottom-width") {
        style.borderBottomWidth = parseLength(decl.value);
      } else if (decl.property == "border-left-color") {
        style.borderLeftColor = parseColor(decl.value);
      } else if (decl.property == "border-right-color") {
        style.borderRightColor = parseColor(decl.value);
      } else if (decl.property == "border-top-color") {
        style.borderTopColor = parseColor(decl.value);
      } else if (decl.property == "border-bottom-color") {
        style.borderBottomColor = parseColor(decl.value);
      }

      style.otherProperties[decl.property] = decl.value;
    }
  }

  Length parseLength(const std::string &val) {
    if (val == "auto")
      return Length::Auto();

    float f = 0.0f;
    try {
      f = std::stof(val);
    } catch (const std::exception &) {
      // Fallback to 0 if parsing fails
      f = 0.0f;
    }

    if (val.find("%") != std::string::npos)
      return Length::Percent(f);
    return Length::Px(f);
  }

  Color parseColor(std::string val) {
    // Trim whitespace and potential semicolon
    size_t first = val.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
      return Color::Black();
    val.erase(0, first);
    val.erase(val.find_last_not_of(" \t\r\n;") + 1);

    if (val.empty())
      return Color::Black();

    // 1. Named Colors
    if (val == "red")
      return Color::Red();
    if (val == "blue")
      return Color::Blue();
    if (val == "green")
      return Color::Green();
    if (val == "black")
      return Color::Black();
    if (val == "white")
      return Color::White();
    if (val == "transparent")
      return Color::Transparent();
    if (val == "gray" || val == "grey")
      return {128, 128, 128, 255};
    if (val == "cyan")
      return {0, 255, 255, 255};
    if (val == "magenta")
      return {255, 0, 255, 255};
    if (val == "yellow")
      return {255, 255, 0, 255};
    if (val == "orange")
      return {255, 165, 0, 255};
    if (val == "purple")
      return {128, 0, 128, 255};
    if (val == "pink")
      return {255, 192, 203, 255};
    if (val == "brown")
      return {165, 42, 42, 255};
    if (val == "navy")
      return {0, 0, 128, 255};
    if (val == "teal")
      return {0, 128, 128, 255};
    if (val == "olive")
      return {128, 128, 0, 255};
    if (val == "silver")
      return {192, 192, 192, 255};
    if (val == "lime")
      return {0, 255, 0, 255};
    if (val == "aqua")
      return {0, 255, 255, 255};
    if (val == "maroon")
      return {128, 0, 0, 255};
    if (val == "fuchsia")
      return {255, 0, 255, 255};

    // 2. rgb() and rgba() functional notation
    if (val.find("rgb") != std::string::npos) {
      return parseFunctionalColor(val);
    }

    // 3. hsl() and hsla() functional notation
    if (val.find("hsl") != std::string::npos) {
      return parseHSLColor(val);
    }

    // 4. Hex Colors
    if (val[0] == '#') {
      std::string hex = val.substr(1);
      if (hex.length() == 3) {
        // #RGB -> #RRGGBB
        uint8_t r = hexCharToInt(hex[0]) * 17;
        uint8_t g = hexCharToInt(hex[1]) * 17;
        uint8_t b = hexCharToInt(hex[2]) * 17;
        return {r, g, b, 255};
      } else if (hex.length() == 6 || hex.length() == 8) {
        // #RRGGBB or #RRGGBBAA
        uint8_t r = (hexCharToInt(hex[0]) << 4) | hexCharToInt(hex[1]);
        uint8_t g = (hexCharToInt(hex[2]) << 4) | hexCharToInt(hex[3]);
        uint8_t b = (hexCharToInt(hex[4]) << 4) | hexCharToInt(hex[5]);
        uint8_t a = 255;
        if (hex.length() == 8) {
          a = (hexCharToInt(hex[6]) << 4) | hexCharToInt(hex[7]);
        }
        return {r, g, b, a};
      } else {
        // Malformed hex color (not 3, 6, or 8 chars) — fallback
        return Color::Black();
      }
    }

    // Default fallback
    return Color::Black();
  }

  Color parseFunctionalColor(const std::string &val) {
    size_t start = val.find('(');
    size_t end = val.find(')');
    if (start == std::string::npos || end == std::string::npos)
      return Color::Black();

    std::string content = val.substr(start + 1, end - start - 1);
    std::stringstream ss(content);
    std::string token;
    std::vector<float> values;

    while (std::getline(ss, token, ',')) {
      token.erase(remove_if(token.begin(), token.end(), isspace), token.end());
      float num = 0.0f;
      std::string percent;

      size_t percentPos = token.find('%');
      if (percentPos != std::string::npos) {
        percent = "%";
        token = token.substr(0, percentPos);
      }

      try {
        num = std::stof(token);
      } catch (...) {
        continue;
      }

      if (!percent.empty()) {
        num = num * 255.0f / 100.0f;
      }
      values.push_back(num);
    }

    if (values.size() >= 3) {
      uint8_t r = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, values[0])));
      uint8_t g = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, values[1])));
      uint8_t b = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, values[2])));
      uint8_t a = 255;
      if (values.size() >= 4) {
        if (values[3] <= 1.0f) {
          a = static_cast<uint8_t>(values[3] * 255.0f);
        } else {
          a = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, values[3])));
        }
      }
      return {r, g, b, a};
    }

    return Color::Black();
  }

  Color parseHSLColor(const std::string &val) {
    size_t start = val.find('(');
    size_t end = val.find(')');
    if (start == std::string::npos || end == std::string::npos)
      return Color::Black();

    std::string content = val.substr(start + 1, end - start - 1);
    std::stringstream ss(content);
    std::string token;
    std::vector<float> values;

    while (std::getline(ss, token, ',')) {
      token.erase(remove_if(token.begin(), token.end(), isspace), token.end());
      float num = 0.0f;
      std::string percent;

      size_t percentPos = token.find('%');
      if (percentPos != std::string::npos) {
        percent = "%";
        token = token.substr(0, percentPos);
      }

      try {
        num = std::stof(token);
      } catch (...) {
        continue;
      }

      if (!percent.empty() && values.size() == 1) {
        num = num * 255.0f / 100.0f;
      }
      values.push_back(num);
    }

    if (values.size() >= 3) {
      float h = values[0];
      float s = values.size() > 1 ? values[1] / 100.0f : 0.0f;
      float l = values.size() > 2 ? values[2] / 100.0f : 0.0f;

      if (h > 1) h = h / 360.0f;

      float r, g, b;
      if (s == 0) {
        r = g = b = l;
      } else {
        auto hue2rgb = [](float p, float q, float t) {
          if (t < 0) t += 1;
          if (t > 1) t -= 1;
          if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
          if (t < 1.0f/2.0f) return q;
          if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
          return p;
        };
        float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
        float p = 2.0f * l - q;
        r = hue2rgb(p, q, h + 1.0f/3.0f);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1.0f/3.0f);
      }

      uint8_t alpha = 255;
      if (values.size() >= 4) {
        if (values[3] <= 1.0f) {
          alpha = static_cast<uint8_t>(values[3] * 255.0f);
        } else {
          alpha = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, values[3])));
        }
      }

      return {
        static_cast<uint8_t>(r * 255),
        static_cast<uint8_t>(g * 255),
        static_cast<uint8_t>(b * 255),
        alpha
      };
    }

    return Color::Black();
  }

  std::string resolveCSSVariable(const std::string& value, const ComputedStyle& style) {
    (void)style; // Suppress unused warning - style used in property lookup
    // Check if value contains var()
    size_t varPos = value.find("var(");
    if (varPos == std::string::npos) {
      return value;
    }

    std::string result = value;
    size_t start = 0;

    while ((start = result.find("var(", start)) != std::string::npos) {
      size_t endParen = result.find(')', start);
      if (endParen == std::string::npos) {
        break;
      }

      std::string varContent = result.substr(start + 4, endParen - start - 4);

      // Remove whitespace
      varContent.erase(remove_if(varContent.begin(), varContent.end(), isspace), varContent.end());

      // Get variable name
      size_t commaPos = varContent.find(',');
      std::string varName = (commaPos != std::string::npos) 
                           ? varContent.substr(0, commaPos) 
                           : varContent;
      std::string fallback = (commaPos != std::string::npos) 
                              ? varContent.substr(commaPos + 1) 
                              : "";

      // Try to get custom property value from style
      const CustomPropertyValue* propValue = style.getCustomProperty(varName);
      std::string replacement;

      if (propValue) {
        if (std::holds_alternative<std::string>(*propValue)) {
          replacement = std::get<std::string>(*propValue);
        } else if (std::holds_alternative<Length>(*propValue)) {
          const Length& len = std::get<Length>(*propValue);
          if (len.unit == Length::Unit::Px) {
            replacement = std::to_string(len.value) + "px";
          } else if (len.unit == Length::Unit::Percent) {
            replacement = std::to_string(len.value) + "%";
          } else if (len.unit == Length::Unit::Em) {
            replacement = std::to_string(len.value) + "em";
          }
        } else if (std::holds_alternative<Color>(*propValue)) {
          const Color& col = std::get<Color>(*propValue);
          char hexBuffer[8];
          snprintf(hexBuffer, sizeof(hexBuffer), "#%02x%02x%02x", col.r, col.g, col.b);
          replacement = hexBuffer;
        }
      }

      // Use fallback if no value found
      if (replacement.empty()) {
        replacement = fallback;
      }

      // Replace the var() with the resolved value
      result.replace(start, endParen - start + 1, replacement);
      start += replacement.length();
    }

    return result;
  }

  uint8_t hexCharToInt(char c) {
    if (c >= '0' && c <= '9')
      return c - '0';
    if (c >= 'a' && c <= 'f')
      return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
      return c - 'A' + 10;
    return 0;
  }
};

} // namespace css
} // namespace xiaopeng
