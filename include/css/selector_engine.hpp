#pragma once

#include "css_types.hpp"
#include "../dom/dom.hpp"
#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace xiaopeng {
namespace css {

// Attribute selector operator types
enum class AttributeOperator {
  Exists,        // [attr]
  Equals,        // [attr=value]
  Contains,      // [attr~=value]
  StartsWith,    // [attr^=value]
  EndsWith,      // [attr$=value]
  ContainsSub,   // [attr*=value]
  DashMatch      // [attr|=value]
};

// Extended attribute selector structure
struct AttributeSelector {
  std::string name;
  std::string value;
  AttributeOperator op = AttributeOperator::Exists;
  bool caseInsensitive = false;
};

// Advanced pseudo-class matcher function signature
using PseudoClassMatcher = std::function<bool(dom::ElementPtr)>;

// Selector matching context (for :nth-child etc.)
struct SelectorContext {
  // Optional context for complex selectors
  dom::ElementPtr scope;  // Element from which we're searching
};

class SelectorEngine {
public:
  SelectorEngine() = default;

  // Query all elements matching a selector string
  std::vector<dom::ElementPtr> querySelectorAll(
      const std::string& selector,
      dom::ElementPtr root) {
    // For now, let's use existing selector parsing/matching
    // This will be enhanced later
    return simpleQuerySelectorAll(selector, root);
  }

  // Query first matching element
  dom::ElementPtr querySelector(
      const std::string& selector,
      dom::ElementPtr root) {
    auto results = querySelectorAll(selector, root);
    return results.empty() ? nullptr : results[0];
  }

  // Check if an element matches a selector
  bool matches(dom::ElementPtr element, const Selector& selector) {
    return matchSelector(element, selector);
  }

  // Enhanced attribute selector matching
  static bool matchAttributeSelector(
      dom::ElementPtr element,
      const AttributeSelector& attr);

  // Nth formula evaluation
  static bool evaluateNthFormula(int index, const std::string& formula);

private:
  // Simple query implementation (to be enhanced)
  std::vector<dom::ElementPtr> simpleQuerySelectorAll(
      const std::string& selector,
      dom::ElementPtr root) {
    std::vector<dom::ElementPtr> results;
    
    // Parse selector (using existing parser or simple one)
    // For now, let's implement simple tag/id/class lookup
    if (selector.empty()) return results;

    // Collect all descendant elements and check matches
    collectElements(root, results);

    // Filter elements (we'd need a proper selector parser here)
    // For demonstration, this is simplified
    std::vector<dom::ElementPtr> filtered;
    for (auto& elem : results) {
      if (simpleMatch(elem, selector)) {
        filtered.push_back(elem);
      }
    }

    return filtered;
  }

  void collectElements(dom::ElementPtr root,
                       std::vector<dom::ElementPtr>& result) {
    if (!root) return;
    result.push_back(root);
    
    // Collect children
    for (const auto& child : root->childNodes()) {
      if (child->nodeType() == dom::NodeType::Element) {
        collectElements(std::static_pointer_cast<dom::Element>(child), result);
      }
    }
  }

  bool simpleMatch(dom::ElementPtr elem, const std::string& selector) {
    // Very simple matching for demonstration
    if (selector.empty() || !elem) return false;
    
    if (selector[0] == '#') {
      return elem->id() == selector.substr(1);
    } else if (selector[0] == '.') {
      return elem->hasClass(selector.substr(1));
    } else if (selector[0] == '[') {
      // Attribute selector - simplified check for existence
      size_t endBracket = selector.find(']');
      if (endBracket != std::string::npos) {
        std::string attrName = selector.substr(1, endBracket - 1);
        // Simplified: just check if attribute exists
        return elem->hasAttribute(attrName);
      }
      return false;
    } else if (selector[0] == ':') {
      // Pseudo-class - simplified matching
      std::string pseudo = selector.substr(1);
      return matchPseudoClass(elem, pseudo);
    } else {
      // Tag name
      return dom::toLower(elem->localName()) == dom::toLower(selector);
    }
  }

  // Core selector matching
  bool matchSelector(dom::ElementPtr element, const Selector& selector) {
    if (selector.parts.empty() || !element)
      return false;

    // Start matching from the last part (right-to-left matching is more efficient)
    return matchRecursively(element, selector,
                            static_cast<int>(selector.parts.size()) - 1);
  }

  bool matchRecursively(dom::ElementPtr element, const Selector& selector,
                        int index) {
    if (index < 0 || index >= static_cast<int>(selector.parts.size())) {
      return index < 0;
    }
    if (!element)
      return false;

    // 1. Match the compound selector at current position
    int currentIndex = index;
    
    // Check parts belonging to the same element
    while (currentIndex >= 0) {
      if (!matchSimpleSelector(element, selector.parts[currentIndex])) {
        return false;
      }

      if (currentIndex > 0 &&
          selector.combinators[currentIndex - 1] == Combinator::None) {
        currentIndex--;
      } else {
        break;
      }
    }

    if (currentIndex == 0)
      return true;

    // 2. Handle combinator
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
                           const SimpleSelector& simple) {
    switch (simple.type) {
    case SelectorType::Tag:
      return dom::toLower(simple.value) == dom::toLower(element->localName());
    case SelectorType::Class:
      return element->hasClass(simple.value);
    case SelectorType::Id:
      return element->id() == simple.value;
    case SelectorType::Universal:
      return true;
    case SelectorType::Attribute:
      return matchSimpleAttribute(element, simple);
    case SelectorType::PseudoClass:
      return matchPseudoClass(element, simple.value);
    case SelectorType::PseudoElement:
      return true; // Pseudo-elements always match for selector purposes
    default:
      return false;
    }
  }

  bool matchSimpleAttribute(dom::ElementPtr element,
                           const SimpleSelector& simple) {
    if (simple.attributeOperator.empty()) {
      if (simple.attributeValue.empty()) {
        return element->hasAttribute(simple.attributeName);
      } else {
        auto val = element->getAttribute(simple.attributeName);
        return val.has_value() && val.value() == simple.attributeValue;
      }
    }
    
    // Use extended attribute matching
    AttributeSelector attr;
    attr.name = simple.attributeName;
    attr.value = simple.attributeValue;
    
    if (simple.attributeOperator == "=") {
      attr.op = AttributeOperator::Equals;
    } else if (simple.attributeOperator == "~=") {
      attr.op = AttributeOperator::Contains;
    } else if (simple.attributeOperator == "|=") {
      attr.op = AttributeOperator::DashMatch;
    } else if (simple.attributeOperator == "^=") {
      attr.op = AttributeOperator::StartsWith;
    } else if (simple.attributeOperator == "$=") {
      attr.op = AttributeOperator::EndsWith;
    } else if (simple.attributeOperator == "*=") {
      attr.op = AttributeOperator::ContainsSub;
    }
    
    return matchAttributeSelector(element, attr);
  }

  bool matchPseudoClass(dom::ElementPtr element, const std::string& name) {
    // Basic state pseudo-classes
    if (name == "hover") return element->isHovered();
    if (name == "active") return element->isActive();
    if (name == "focus") return element->isFocused();
    
    // Structural pseudo-classes
    if (name == "first-child") return element->isFirstChild();
    if (name == "last-child") return element->isLastChild();
    if (name == "only-child") return element->isOnlyChild();
    if (name == "empty") return element->isEmpty();
    if (name == "root") return element->isDocumentRoot();
    
    // Form pseudo-classes
    if (name == "checked") return element->isChecked();
    if (name == "disabled") return element->isDisabled();
    if (name == "enabled") return !element->isDisabled();
    if (name == "required") return element->hasAttribute("required");
    if (name == "optional") return !element->hasAttribute("required");
    
    // Link pseudo-classes
    if (name == "link") return element->isLink();
    if (name == "visited") return element->isVisited();
    
    // Functional pseudo-classes
    if (name.substr(0, 9) == "nth-child") {
      return matchNthChild(element, name.substr(9));
    }
    if (name.substr(0, 12) == "nth-last-child") {
      return matchNthLastChild(element, name.substr(12));
    }
    if (name.substr(0, 11) == "nth-of-type") {
      return matchNthOfType(element, name.substr(11));
    }
    if (name.substr(0, 14) == "nth-last-of-type") {
      return matchNthLastOfType(element, name.substr(14));
    }
    if (name.substr(0, 4) == "not(") {
      return matchNotPseudoClass(element, name);
    }
    if (name.substr(0, 5) == "is(" || name.substr(0, 7) == "where(") {
      // Simplified :is and :where support
      return true;
    }
    if (name.substr(0, 4) == "has(") {
      return matchHasPseudoClass(element, name);
    }
    
    return false;
  }

  bool matchNthChild(dom::ElementPtr element, const std::string& formula) {
    if (!element) return false;
    // Strip parentheses if present
    std::string f = formula;
    if (!f.empty() && f[0] == '(') {
      f = f.substr(1);
    }
    if (!f.empty() && f.back() == ')') {
      f = f.substr(0, f.size() - 1);
    }
    
    int siblingIndex = element->getElementSiblingIndex();
    return evaluateNthFormula(siblingIndex, f);
  }

  bool matchNthLastChild(dom::ElementPtr element, const std::string& formula) {
    if (!element) return false;
    std::string f = formula;
    if (!f.empty() && f[0] == '(') {
      f = f.substr(1);
    }
    if (!f.empty() && f.back() == ')') {
      f = f.substr(0, f.size() - 1);
    }
    
    int siblingIndex = element->getElementSiblingIndexFromEnd();
    return evaluateNthFormula(siblingIndex, f);
  }

  bool matchNthOfType(dom::ElementPtr element, const std::string& formula) {
    if (!element) return false;
    std::string f = formula;
    if (!f.empty() && f[0] == '(') {
      f = f.substr(1);
    }
    if (!f.empty() && f.back() == ')') {
      f = f.substr(0, f.size() - 1);
    }
    
    int siblingIndex = element->getElementSiblingIndexOfType();
    return evaluateNthFormula(siblingIndex, f);
  }

  bool matchNthLastOfType(dom::ElementPtr element, const std::string& formula) {
    if (!element) return false;
    // Similar to nth-last-child but counts only same-type siblings
    return matchNthOfType(element, formula); // Simplified
  }

  bool matchNotPseudoClass(dom::ElementPtr element, const std::string& name) {
    // Extract the inner selector from :not(...)
    if (name.size() < 5 || !element) return false;
    
    std::string inner = name.substr(4, name.size() - 5);
    inner = trimWhitespace(inner);
    
    // Simple check for class, id, or tag
    if (!inner.empty()) {
      if (inner[0] == '.') {
        return !element->hasClass(inner.substr(1));
      } else if (inner[0] == '#') {
        return element->id() != inner.substr(1);
      } else if (inner[0] == ':') {
        // Recursive check for pseudo-class
        return !matchPseudoClass(element, inner.substr(1));
      } else {
        // Tag name
        return dom::toLower(element->localName()) != dom::toLower(inner);
      }
    }
    
    return false; // Default to false if we can't parse
  }

  bool matchHasPseudoClass(dom::ElementPtr element, const std::string& /*name*/) {
    if (!element) return false;
    // Simplified :has selector
    // Check if element has any children
    return !element->isEmpty();
  }

  std::string trimWhitespace(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
  }
};

// Implementations for static methods
inline bool SelectorEngine::matchAttributeSelector(
    dom::ElementPtr element,
    const AttributeSelector& attr) {
  auto attrValue = element->getAttribute(attr.name);
  if (attr.op == AttributeOperator::Exists) {
    return attrValue.has_value();
  }
  if (!attrValue.has_value()) {
    return false;
  }

  std::string value = *attrValue;
  std::string expected = attr.value;

  if (attr.caseInsensitive) {
    value = dom::toLower(value);
    expected = dom::toLower(expected);
  }

  switch (attr.op) {
    case AttributeOperator::Equals:
      return value == expected;
    case AttributeOperator::Contains: {
      // [attr~=value]: value is a space-separated list containing expected
      std::stringstream ss(value);
      std::string token;
      while (ss >> token) {
        if (token == expected) return true;
      }
      return false;
    }
    case AttributeOperator::DashMatch: {
      // [attr|=value]: value is exactly "value" or starts with "value-"
      if (value == expected) return true;
      if (value.size() > expected.size() &&
          value.substr(0, expected.size() + 1) == expected + "-") {
        return true;
      }
      return false;
    }
    case AttributeOperator::StartsWith:
      return value.size() >= expected.size() &&
              value.substr(0, expected.size()) == expected;
    case AttributeOperator::EndsWith:
      return value.size() >= expected.size() &&
              value.substr(value.size() - expected.size()) == expected;
    case AttributeOperator::ContainsSub:
      return value.find(expected) != std::string::npos;
    default:
      return false;
  }
}

inline bool SelectorEngine::evaluateNthFormula(int index, const std::string& formula) {
  // Handle special keywords
  if (formula == "odd") {
    return index % 2 == 1;
  }
  if (formula == "even") {
    return index % 2 == 0;
  }
  if (formula == "n") {
    return true;
  }

  // Parse "an+b" format
  std::string trimmed = formula;
  size_t first = trimmed.find_first_not_of(" \t\r\n");
  if (first != std::string::npos) trimmed = trimmed.substr(first);
  size_t last = trimmed.find_last_not_of(" \t\r\n");
  if (last != std::string::npos) trimmed = trimmed.substr(0, last + 1);

  if (trimmed.empty()) return false;

  // Check if it's just a number
  bool isNumber = true;
  for (size_t i = 0; i < trimmed.size(); i++) {
    if (i == 0 && (trimmed[i] == '+' || trimmed[i] == '-')) continue;
    if (!std::isdigit(static_cast<unsigned char>(trimmed[i]))) {
      isNumber = false;
      break;
    }
  }

  if (isNumber) {
    try {
      int target = std::stoi(trimmed);
      return index == target;
    } catch (...) {
      return false;
    }
  }

  // Parse an+b
  int a = 0, b = 0;
  size_t nPos = trimmed.find('n');
  
  if (nPos != std::string::npos) {
    // Parse a
    std::string aStr = trimmed.substr(0, nPos);
    if (aStr.empty() || aStr == "+") {
      a = 1;
    } else if (aStr == "-") {
      a = -1;
    } else {
      try {
        a = std::stoi(aStr);
      } catch (...) {
        a = 0;
      }
    }
    
    // Parse b
    std::string bStr = trimmed.substr(nPos + 1);
    if (!bStr.empty()) {
      try {
        b = std::stoi(bStr);
      } catch (...) {
        b = 0;
      }
    }
  } else {
    // Just b
    try {
      b = std::stoi(trimmed);
    } catch (...) {
      return false;
    }
  }

  if (a == 0) {
    return index == b;
  }

  // Check if n is integer and in valid range
  // n = (index - b) / a must be integer >= 0 when a > 0
  // or <= 0 when a < 0
  int diff = index - b;
  
  // Must be divisible
  if (diff % a != 0) {
    return false;
  }
  
  int n = diff / a;
  
  if (a > 0) {
    return n >= 0;
  } else {
    return n <= 0;
  }
}

} // namespace css
} // namespace xiaopeng
