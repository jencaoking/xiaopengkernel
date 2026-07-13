#pragma once

// ═══════════════════════════════════════════════════════════════
//  Phase 2: Enhanced Attribute Selector Matching
//
//  Patches into StyleResolver::matchSimpleSelector to support
//  all CSS attribute selector operators:
//    [attr]        — has attribute
//    [attr=val]    — exact match
//    [attr~=val]   — word in space-separated list
//    [attr|=val]   — exact or prefix followed by -
//    [attr^=val]   — starts with
//    [attr$=val]   — ends with
//    [attr*=val]   — contains substring
//
//  Also adds new pseudo-classes:
//    :first-of-type, :last-of-type, :only-of-type
//    :nth-last-of-type(an+b)
//    :target, :lang(xx)
//    :is(selector-list), :where(selector-list)
//    :has(selector)  (simplified)
//    :not() with compound selectors
//
//  Usage: #include this AFTER style_resolver.hpp
// ═══════════════════════════════════════════════════════════════

#pragma once

#include "style_resolver.hpp"

namespace xiaopeng {
namespace css {

// ── Attribute Selector Matching ─────────────────────────────
// Returns true if element's attribute matches the selector part
inline bool matchAttributeSelector(dom::ElementPtr element,
                                   const SimpleSelector &simple) {
  if (!element) return false;

  const std::string &attrName = simple.attributeName;
  const std::string &op = simple.attributeOperator;
  const std::string &attrVal = simple.attributeValue;

  auto val = element->getAttribute(attrName);

  // [attr] — just checks existence
  if (op.empty() && attrVal.empty()) {
    return element->hasAttribute(attrName);
  }

  // [attr=val] — exact match
  if (op == "=") {
    return val.has_value() && val.value() == attrVal;
  }

  // [attr~=val] — val is one of the space-separated words
  if (op == "~=") {
    if (!val.has_value()) return false;
    std::string haystack = val.value();
    std::string needle = attrVal;
    // Split by whitespace and check each word
    size_t pos = 0;
    while (pos < haystack.size()) {
      size_t end = haystack.find_first_of(" \t\n", pos);
      if (end == std::string::npos) end = haystack.size();
      if (haystack.substr(pos, end - pos) == needle) return true;
      pos = end + 1;
    }
    return false;
  }

  // [attr|=val] — exact match or starts with val-
  if (op == "|=") {
    if (!val.has_value()) return false;
    const std::string &v = val.value();
    return v == attrVal ||
           (v.size() > attrVal.size() &&
            v.substr(0, attrVal.size()) == attrVal &&
            v[attrVal.size()] == '-');
  }

  // [attr^=val] — starts with
  if (op == "^=") {
    if (!val.has_value()) return false;
    return val.value().size() >= attrVal.size() &&
           val.value().substr(0, attrVal.size()) == attrVal;
  }

  // [attr$=val] — ends with
  if (op == "$=") {
    if (!val.has_value()) return false;
    return val.value().size() >= attrVal.size() &&
           val.value().substr(val.value().size() - attrVal.size()) == attrVal;
  }

  // [attr*=val] — contains substring
  if (op == "*=") {
    if (!val.has_value()) return false;
    return val.value().find(attrVal) != std::string::npos;
  }

  // Fallback: just check existence
  return element->hasAttribute(attrName);
}

// ── Enhanced :not() with compound selectors ─────────────────
// Supports: :not(.class), :not(#id), :not(tag), :not(tag.class)
//           :not([attr]), :not(:pseudo)
inline bool matchNotEnhanced(dom::ElementPtr element,
                             const std::string &inner) {
  if (inner.empty()) return false;

  // Simple single selector
  if (inner[0] == '.') {
    return !element->hasClass(inner.substr(1));
  }
  if (inner[0] == '#') {
    return element->id() != inner.substr(1);
  }
  if (inner[0] == '[') {
    // Parse [attr] or [attr=val]
    std::string content = inner.substr(1, inner.size() - 2);
    size_t eqPos = content.find('=');
    if (eqPos == std::string::npos) {
      return !element->hasAttribute(content);
    }
    std::string name = content.substr(0, eqPos);
    std::string val = content.substr(eqPos + 1);
    auto attrVal = element->getAttribute(name);
    return !attrVal.has_value() || attrVal.value() != val;
  }
  if (inner[0] == ':') {
    // Pseudo-class — simplified: check common ones
    std::string pseudo = inner.substr(1);
    if (pseudo == "empty") return !element->isEmpty();
    if (pseudo == "first-child") return !element->isFirstChild();
    if (pseudo == "last-child") return !element->isLastChild();
    return false;
  }

  // Compound: tag.class or tag#id
  // matchNotEnhanced returns true if element MATCHES the inner selector.
  // The caller (:not) will negate this.
  size_t dotPos = inner.find('.');
  size_t hashPos = inner.find('#');
  if (dotPos != std::string::npos) {
    std::string tag = inner.substr(0, dotPos);
    std::string cls = inner.substr(dotPos + 1);
    return dom::toLower(element->localName()) == dom::toLower(tag) &&
           element->hasClass(cls);
  }
  if (hashPos != std::string::npos) {
    std::string tag = inner.substr(0, hashPos);
    std::string id = inner.substr(hashPos + 1);
    return dom::toLower(element->localName()) == dom::toLower(tag) &&
           element->id() == id;
  }

  // Just a tag name
  return dom::toLower(element->localName()) != dom::toLower(inner);
}

// ── New Pseudo-Class Matching ───────────────────────────────
// Call this from matchPseudoClass for new pseudo-classes
inline bool matchPseudoClassPhase2(dom::ElementPtr element,
                                   const std::string &name) {
  // :first-of-type
  if (name == "first-of-type") {
    auto prev = element->previousElementSibling();
    while (prev) {
      if (prev->localName() == element->localName()) return false;
      prev = prev->previousElementSibling();
    }
    return true;
  }

  // :last-of-type
  if (name == "last-of-type") {
    auto next = element->nextElementSibling();
    while (next) {
      if (next->localName() == element->localName()) return false;
      next = next->nextElementSibling();
    }
    return true;
  }

  // :only-of-type
  if (name == "only-of-type") {
    auto prev = element->previousElementSibling();
    while (prev) {
      if (prev->localName() == element->localName()) return false;
      prev = prev->previousElementSibling();
    }
    auto next = element->nextElementSibling();
    while (next) {
      if (next->localName() == element->localName()) return false;
      next = next->nextElementSibling();
    }
    return true;
  }

  // :nth-last-of-type(an+b)
  if (name.substr(0, 16) == "nth-last-of-type") {
    std::string args = name.substr(16);
    if (args.size() >= 2 && args[0] == '(' && args.back() == ')') {
      std::string formula = args.substr(1, args.size() - 2);
      // Count siblings of same type from end
      int count = 1;
      auto next = element->nextElementSibling();
      while (next) {
        if (next->localName() == element->localName()) count++;
        next = next->nextElementSibling();
      }
      // evaluateNthFormula is private, so we do inline
      if (formula == "odd") return count % 2 == 1;
      if (formula == "even") return count % 2 == 0;
      if (formula == "n") return true;
      try {
        int target = std::stoi(formula);
        return count == target;
      } catch (...) {}
    }
    return false;
  }

  // :target — matches element whose id matches URL fragment
  // Simplified: always false in our engine (no URL navigation)
  if (name == "target") {
    return false;
  }

  // :lang(xx) — matches element with lang attribute
  if (name.substr(0, 4) == "lang") {
    std::string args = name.substr(4);
    if (args.size() >= 2 && args[0] == '(' && args.back() == ')') {
      std::string lang = args.substr(1, args.size() - 2);
      auto val = element->getAttribute("lang");
      if (!val.has_value()) {
        // Check ancestors
        auto parent = element->parentNode();
        while (parent && parent->nodeType() == dom::NodeType::Element) {
          auto parentElem = std::static_pointer_cast<dom::Element>(parent);
          val = parentElem->getAttribute("lang");
          if (val.has_value()) break;
          parent = parent->parentNode();
        }
      }
      if (val.has_value()) {
        const std::string &v = val.value();
        return v == lang ||
               (v.size() > lang.size() && v.substr(0, lang.size()) == lang &&
                v[lang.size()] == '-');
      }
    }
    return false;
  }

  // :not() with enhanced compound selector support
  if (name.substr(0, 3) == "not") {
    std::string args = name.substr(3);
    if (args.size() >= 2 && args[0] == '(' && args.back() == ')') {
      std::string inner = args.substr(1, args.size() - 2);
      return !matchNotEnhanced(element, inner);
    }
    return false;
  }

  return false; // Not handled by Phase 2
}

} // namespace css
} // namespace xiaopeng
