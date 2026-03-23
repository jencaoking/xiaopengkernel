#pragma once

#include "../loader/error.hpp"
#include "../loader/types.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace xiaopeng {
namespace dom {

using uint8_t = std::uint8_t;

enum class TokenType : uint8_t {
  Doctype,
  StartTag,
  EndTag,
  Comment,
  Character,
  EndOfFile,
  Cdata,
  CharacterReference
};

struct Attribute {
  std::string name;
  std::string value;

  Attribute() = default;
  Attribute(std::string n, std::string v)
      : name(std::move(n)), value(std::move(v)) {}

  bool operator==(const Attribute &other) const {
    return name == other.name && value == other.value;
  }
};

struct Token {
  TokenType type = TokenType::Character;
  std::string name;
  std::string data;
  std::vector<Attribute> attributes;
  bool selfClosing = false;
  bool forceQuirks = false;

  std::string systemIdentifier;
  std::string publicIdentifier;

  size_t position = 0;
  size_t line = 1;
  size_t column = 1;

  Token() = default;

  static Token makeDoctype(const std::string &name = "",
                           const std::string &publicId = "",
                           const std::string &systemId = "",
                           bool forceQuirks = false) {
    Token token;
    token.type = TokenType::Doctype;
    token.name = name;
    token.publicIdentifier = publicId;
    token.systemIdentifier = systemId;
    token.forceQuirks = forceQuirks;
    return token;
  }

  static Token makeStartTag(const std::string &name,
                            std::vector<Attribute> attrs = {},
                            bool selfClosing = false) {
    Token token;
    token.type = TokenType::StartTag;
    token.name = name;
    token.attributes = std::move(attrs);
    token.selfClosing = selfClosing;
    return token;
  }

  static Token makeEndTag(const std::string &name) {
    Token token;
    token.type = TokenType::EndTag;
    token.name = name;
    return token;
  }

  static Token makeComment(const std::string &data) {
    Token token;
    token.type = TokenType::Comment;
    token.data = data;
    return token;
  }

  static Token makeCharacter(char c) {
    Token token;
    token.type = TokenType::Character;
    token.data = std::string(1, c);
    return token;
  }

  static Token makeCharacter(const std::string &str) {
    Token token;
    token.type = TokenType::Character;
    token.data = str;
    return token;
  }

  static Token makeEndOfFile() {
    Token token;
    token.type = TokenType::EndOfFile;
    return token;
  }

  static Token makeCdata(const std::string &data) {
    Token token;
    token.type = TokenType::Cdata;
    token.data = data;
    return token;
  }

  bool isStartTag() const { return type == TokenType::StartTag; }
  bool isEndTag() const { return type == TokenType::EndTag; }
  bool isCharacter() const { return type == TokenType::Character; }
  bool isComment() const { return type == TokenType::Comment; }
  bool isDoctype() const { return type == TokenType::Doctype; }
  bool isEndOfFile() const { return type == TokenType::EndOfFile; }
  bool isCdata() const { return type == TokenType::Cdata; }

  std::optional<std::string> getAttribute(const std::string &attrName) const {
    for (const auto &attr : attributes) {
      if (attr.name == attrName) {
        return attr.value;
      }
    }
    return std::nullopt;
  }

  bool hasAttribute(const std::string &attrName) const {
    for (const auto &attr : attributes) {
      if (attr.name == attrName) {
        return true;
      }
    }
    return false;
  }
};

enum class NodeType : uint8_t {
  Element,
  Text,
  Comment,
  Document,
  DocumentType,
  DocumentFragment,
  CdataSection
};

// Dirty flag for incremental update: Layout > Paint > Clean
enum class DirtyFlag : uint8_t { Clean = 0, NeedsPaint = 1, NeedsLayout = 2 };

class Node;
using NodePtr = std::shared_ptr<Node>;
using WeakNodePtr = std::weak_ptr<Node>;

class Node : public std::enable_shared_from_this<Node> {
public:
  virtual ~Node() = default;

  NodeType nodeType() const { return nodeType_; }
  const std::string &nodeName() const { return nodeName_; }

  NodePtr parentNode() const { return parentNode_.lock(); }
  NodePtr firstChild() const { return firstChild_; }
  NodePtr lastChild() const { return lastChild_; }
  NodePtr nextSibling() const { return nextSibling_.lock(); }
  NodePtr previousSibling() const { return previousSibling_.lock(); }

  const std::vector<NodePtr> &childNodes() const { return childNodes_; }

  virtual std::string textContent() const {
    std::string result;
    for (const auto &child : childNodes_) {
      result += child->textContent();
    }
    return result;
  }

  // Dirty flag accessors for incremental update
  DirtyFlag dirtyFlag() const { return dirtyFlag_; }
  void markDirty(DirtyFlag flag) {
    if (static_cast<uint8_t>(flag) > static_cast<uint8_t>(dirtyFlag_))
      dirtyFlag_ = flag;
  }
  void clearDirty() { dirtyFlag_ = DirtyFlag::Clean; }

  virtual void notifyMutation(DirtyFlag hint = DirtyFlag::NeedsLayout) {
    markDirty(hint);
    if (auto parent = parentNode_.lock()) {
      parent->notifyMutation(hint);
    }
  }

  virtual void setTextContent(const std::string &) {}

  bool hasChildNodes() const { return !childNodes_.empty(); }

  size_t childCount() const { return childNodes_.size(); }

  virtual NodePtr cloneNode(bool deep = false) const = 0;

  void appendChild(NodePtr child) {
    if (!child)
      return;

    if (auto oldParent = child->parentNode_.lock()) {
      oldParent->removeChild(child);
    }

    child->parentNode_ = weak_from_this();

    if (lastChild_) {
      lastChild_->nextSibling_ = child;
      child->previousSibling_ = lastChild_;
    } else {
      firstChild_ = child;
    }

    lastChild_ = child;
    childNodes_.push_back(child);
    notifyMutation();
  }

  void insertBefore(NodePtr newChild, NodePtr refChild) {
    if (!newChild)
      return;

    if (!refChild) {
      appendChild(newChild);
      return;
    }

    if (auto oldParent = newChild->parentNode_.lock()) {
      oldParent->removeChild(newChild);
    }

    auto it = std::find(childNodes_.begin(), childNodes_.end(), refChild);
    if (it == childNodes_.end()) {
      appendChild(newChild);
      return;
    }

    newChild->parentNode_ = weak_from_this();

    if (auto prev = refChild->previousSibling_.lock()) {
      prev->nextSibling_ = newChild;
      newChild->previousSibling_ = prev;
    } else {
      firstChild_ = newChild;
    }

    newChild->nextSibling_ = refChild;
    refChild->previousSibling_ = newChild;

    childNodes_.insert(it, newChild);
    notifyMutation();
  }

  void removeChild(NodePtr child) {
    if (!child)
      return;

    auto it = std::find(childNodes_.begin(), childNodes_.end(), child);
    if (it == childNodes_.end())
      return;

    if (auto prev = child->previousSibling_.lock()) {
      prev->nextSibling_ = child->nextSibling_;
    } else {
      firstChild_ = child->nextSibling_.lock();
    }

    if (auto next = child->nextSibling_.lock()) {
      next->previousSibling_ = child->previousSibling_;
    } else {
      lastChild_ = child->previousSibling_.lock();
    }

    child->parentNode_.reset();
    child->previousSibling_.reset();
    child->nextSibling_.reset();

    childNodes_.erase(it);
    notifyMutation();
  }

  void replaceChild(NodePtr newChild, NodePtr oldChild) {
    if (!newChild || !oldChild)
      return;

    auto it = std::find(childNodes_.begin(), childNodes_.end(), oldChild);
    if (it == childNodes_.end())
      return;

    if (auto oldParent = newChild->parentNode_.lock()) {
      oldParent->removeChild(newChild);
    }

    newChild->parentNode_ = weak_from_this();

    if (auto prev = oldChild->previousSibling_.lock()) {
      prev->nextSibling_ = newChild;
      newChild->previousSibling_ = prev;
    } else {
      firstChild_ = newChild;
    }

    if (auto next = oldChild->nextSibling_.lock()) {
      next->previousSibling_ = newChild;
      newChild->nextSibling_ = next;
    } else {
      lastChild_ = newChild;
    }

    oldChild->parentNode_.reset();
    oldChild->previousSibling_.reset();
    oldChild->nextSibling_.reset();

    *it = newChild;
    notifyMutation();
  }

  void normalize() {
    std::vector<NodePtr> toRemove;
    NodePtr prevText;

    for (auto &child : childNodes_) {
      if (child->nodeType() == NodeType::Text) {
        if (child->textContent().empty()) {
          toRemove.push_back(child);
        } else if (prevText && prevText->nodeType() == NodeType::Text) {
          std::string combined = prevText->textContent() + child->textContent();
          prevText->setTextContent(combined);
          toRemove.push_back(child);
        } else {
          prevText = child;
        }
      } else {
        prevText.reset();
        child->normalize();
      }
    }

    for (auto &child : toRemove) {
      removeChild(child);
    }
    if (!toRemove.empty()) {
      notifyMutation();
    }
  }

  NodePtr getRootNode() const {
    NodePtr current = const_cast<Node *>(this)->shared_from_this();
    while (auto parent = current->parentNode()) {
      current = parent;
    }
    return current;
  }

  bool contains(const NodePtr &other) const {
    if (!other)
      return false;

    NodePtr current = other;
    while (current) {
      if (current.get() == this)
        return true;
      current = current->parentNode();
    }
    return false;
  }

  void removeAllChildren() {
    bool mutated = !childNodes_.empty();
    while (!childNodes_.empty()) {
      removeChild(childNodes_.back());
    }
    if (mutated) {
      notifyMutation();
    }
  }

  virtual std::string toHtml() const = 0;

protected:
  Node(NodeType type, std::string name)
      : nodeType_(type), nodeName_(std::move(name)) {}

  NodeType nodeType_;
  std::string nodeName_;
  WeakNodePtr parentNode_;
  NodePtr firstChild_;
  NodePtr lastChild_;
  WeakNodePtr nextSibling_;
  WeakNodePtr previousSibling_;
  std::vector<NodePtr> childNodes_;
  DirtyFlag dirtyFlag_ = DirtyFlag::Clean;

public:
  // Event listener IDs (the actual JS callbacks are stored in EventBinding)
  // Map: eventType -> list of listener IDs
  std::unordered_map<std::string, std::vector<uint32_t>> eventListenerIds_;
};

class TextNode : public Node {
public:
  explicit TextNode(const std::string &text)
      : Node(NodeType::Text, "#text"), text_(text) {}

  const std::string &data() const { return text_; }
  void setData(const std::string &text) { text_ = text; }

  std::string textContent() const override { return text_; }
  void setTextContent(const std::string &text) override {
    text_ = text;
    notifyMutation();
  }

  size_t length() const { return text_.length(); }

  NodePtr cloneNode(bool = false) const override {
    return std::make_shared<TextNode>(text_);
  }

  std::string toHtml() const override { return escapeText(text_); }

private:
  static std::string escapeText(const std::string &text) {
    std::string result;
    result.reserve(text.size());
    for (char c : text) {
      switch (c) {
      case '&':
        result += "&amp;";
        break;
      case '<':
        result += "&lt;";
        break;
      case '>':
        result += "&gt;";
        break;
      default:
        result += c;
        break;
      }
    }
    return result;
  }

  std::string text_;
};

class CommentNode : public Node {
public:
  explicit CommentNode(const std::string &data)
      : Node(NodeType::Comment, "#comment"), data_(data) {}

  const std::string &data() const { return data_; }
  void setData(const std::string &data) { data_ = data; }

  std::string textContent() const override { return data_; }
  void setTextContent(const std::string &data) override { data_ = data; }

  NodePtr cloneNode(bool = false) const override {
    return std::make_shared<CommentNode>(data_);
  }

  std::string toHtml() const override { return "<!--" + data_ + "-->"; }

private:
  std::string data_;
};

class DocumentTypeNode : public Node {
public:
  DocumentTypeNode(const std::string &name = "html",
                   const std::string &publicId = "",
                   const std::string &systemId = "")
      : Node(NodeType::DocumentType, name), name_(name), publicId_(publicId),
        systemId_(systemId) {}

  const std::string &name() const { return name_; }
  const std::string &publicId() const { return publicId_; }
  const std::string &systemId() const { return systemId_; }

  NodePtr cloneNode(bool = false) const override {
    return std::make_shared<DocumentTypeNode>(name_, publicId_, systemId_);
  }

  std::string toHtml() const override {
    std::string result = "<!DOCTYPE " + name_;
    if (!publicId_.empty()) {
      result += " PUBLIC \"" + publicId_ + "\"";
    }
    if (!systemId_.empty()) {
      if (publicId_.empty()) {
        result += " SYSTEM";
      }
      result += " \"" + systemId_ + "\"";
    }
    result += ">";
    return result;
  }

private:
  std::string name_;
  std::string publicId_;
  std::string systemId_;
};

class CdataSectionNode : public Node {
public:
  explicit CdataSectionNode(const std::string &data)
      : Node(NodeType::CdataSection, "#cdata-section"), data_(data) {}

  const std::string &data() const { return data_; }
  void setData(const std::string &data) { data_ = data; }

  std::string textContent() const override { return data_; }
  void setTextContent(const std::string &data) override { data_ = data; }

  NodePtr cloneNode(bool = false) const override {
    return std::make_shared<CdataSectionNode>(data_);
  }

  std::string toHtml() const override { return "<![CDATA[" + data_ + "]]>"; }

private:
  std::string data_;
};

class DocumentFragment : public Node {
public:
  DocumentFragment() : Node(NodeType::DocumentFragment, "#document-fragment") {}

  NodePtr cloneNode(bool deep = false) const override {
    auto fragment = std::make_shared<DocumentFragment>();
    if (deep) {
      for (const auto &child : childNodes_) {
        fragment->appendChild(child->cloneNode(true));
      }
    }
    return fragment;
  }

  std::string toHtml() const override {
    std::string result;
    for (const auto &child : childNodes_) {
      result += child->toHtml();
    }
    return result;
  }
};

inline std::string toLower(const std::string &str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

inline char toLower(char c) {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

inline std::string toUpper(const std::string &str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return result;
}

inline bool isWhitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
}

inline bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool isDigit(char c) { return c >= '0' && c <= '9'; }

inline bool isAlphanumeric(char c) { return isAlpha(c) || isDigit(c); }

inline bool isHexDigit(char c) {
  return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

inline std::string trimWhitespace(const std::string &str) {
  size_t start = 0;
  while (start < str.size() && isWhitespace(str[start])) {
    start++;
  }

  size_t end = str.size();
  while (end > start && isWhitespace(str[end - 1])) {
    end--;
  }

  return str.substr(start, end - start);
}

inline std::string collapseWhitespace(const std::string &str) {
  std::string result;
  result.reserve(str.size());
  bool inWhitespace = false;

  for (char c : str) {
    if (isWhitespace(c)) {
      if (!inWhitespace) {
        result += ' ';
        inWhitespace = true;
      }
    } else {
      result += c;
      inWhitespace = false;
    }
  }

  return result;
}

} // namespace dom
} // namespace xiaopeng
