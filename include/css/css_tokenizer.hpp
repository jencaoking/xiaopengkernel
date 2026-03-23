#pragma once

#include "css_types.hpp"
#include <cctype>
#include <string>
#include <vector>

namespace xiaopeng {
namespace css {

class CssTokenizer {
public:
  CssTokenizer(const std::string &input) : input_(input), position_(0) {}

  std::vector<Token> tokenize() {
    std::vector<Token> tokens;
    while (position_ < input_.length()) {
      tokens.push_back(consumeToken());
    }
    tokens.push_back({TokenType::EndOfFile});
    return tokens;
  }

private:
  std::string input_;
  size_t position_;

  char peek() const {
    if (position_ >= input_.length())
      return 0;
    return input_[position_];
  }

  char peekNext() const {
    if (position_ + 1 >= input_.length())
      return 0;
    return input_[position_ + 1];
  }

  char peekNextNext() const {
    if (position_ + 2 >= input_.length())
      return 0;
    return input_[position_ + 2];
  }

  char consume() {
    if (position_ >= input_.length())
      return 0;
    return input_[position_++];
  }

  bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
  }

  bool isDigit(char c) { return c >= '0' && c <= '9'; }

  bool isHexDigit(char c) {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
  }

  bool isNameStart(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ||
           static_cast<unsigned char>(c) >= 128;
  }

  bool isName(char c) { return isNameStart(c) || isDigit(c) || c == '-'; }

  void consumeWhitespace() {
    while (position_ < input_.length() && isWhitespace(peek())) {
      consume();
    }
  }

  void consumeComment() {
    consume(); // /
    consume(); // *
    while (position_ < input_.length()) {
      if (peek() == '*' && peekNext() == '/') {
        consume();
        consume();
        return;
      }
      consume();
    }
  }

  Token consumeString(char ending) {
    std::string value;
    while (position_ < input_.length()) {
      char c = consume();
      if (c == ending) {
        return {TokenType::String, value};
      }
      if (c == '\\') {
        if (position_ >= input_.length())
          break;
        // Simplified escape sequence handling: just take the next char
        // literally Supports newlines in strings if escaped
        char next = peek();
        if (next == '\n' || next == '\f') {
          consume(); // consume newline
        } else {
          value += consume();
        }
      } else if (c == '\n' || c == '\r' || c == '\f') {
        // Unescaped newline in string -> Bad String
        return {TokenType::BadString};
      } else {
        value += c;
      }
    }
    return {TokenType::String, value};
  }

  std::string consumeName() {
    std::string result;
    while (position_ < input_.length()) {
      char c = peek();
      if (isName(c)) {
        result += consume();
      } else if (c == '\\') {
        // Escape sequence start
        consume();
        if (position_ < input_.length()) {
          // Should verify escape validity
          result += consume();
        }
      } else {
        break;
      }
    }
    return result;
  }

  Token consumeNumeric() {
    std::string buffer;
    if (peek() == '+' || peek() == '-') {
      buffer += consume();
    }
    while (isDigit(peek())) {
      buffer += consume();
    }
    if (peek() == '.' && isDigit(peekNext())) {
      buffer += consume(); // .
      while (isDigit(peek())) {
        buffer += consume();
      }
    }
    if ((peek() == 'e' || peek() == 'E') &&
        (isDigit(peekNext()) || ((peekNext() == '+' || peekNext() == '-') &&
                                 isDigit(peekNextNext())))) {
      buffer += consume(); // e/E
      if (peek() == '+' || peek() == '-') {
        buffer += consume();
      }
      while (isDigit(peek())) {
        buffer += consume();
      }
    }

    double val = 0.0;
    try {
      val = std::stod(buffer);
    } catch (const std::exception &e) {
      std::cerr << "[CssTokenizer] Error parsing number '" << buffer
                << "': " << e.what() << std::endl;
    }

    if (peek() == '%') {
      consume();
      return {TokenType::Percentage, "", "", val};
    }

    if (isNameStart(peek())) {
      std::string unit = consumeName();
      return {TokenType::Dimension, "", unit, val};
    }

    return {TokenType::Number, "", "", val};
  }

  Token consumeIdentLike() {
    std::string name = consumeName();
    if (name == "url" && peek() == '(') {
      consume();
      // In a real tokenizer, this would consume the URL content
      // Simplified: return Function
      return {TokenType::Function, name};
    }
    if (peek() == '(') {
      consume();
      return {TokenType::Function, name};
    }
    return {TokenType::Ident, name};
  }

  Token consumeToken() {
    if (position_ >= input_.length())
      return {TokenType::EndOfFile};

    consumeWhitespace();
    if (position_ >= input_.length())
      return {TokenType::EndOfFile};

    char c = peek();

    if (c == '/' && peekNext() == '*') {
      consumeComment();
      return consumeToken();
    }

    if (c == '"' || c == '\'') {
      return consumeString(consume());
    }

    if (c == '#') {
      consume();
      if (isName(peek()) || peek() == '\\') {
        Token t = {TokenType::Hash};
        t.flag = isNameStart(peek());
        t.value = consumeName();
        return t;
      }
      return {TokenType::Delim, "#"};
    }

    if (c == '(') {
      consume();
      return {TokenType::OpenParen, "("};
    }
    if (c == ')') {
      consume();
      return {TokenType::CloseParen, ")"};
    }
    if (c == '[') {
      consume();
      return {TokenType::OpenSquare, "["};
    }
    if (c == ']') {
      consume();
      return {TokenType::CloseSquare, "]"};
    }
    if (c == '{') {
      consume();
      return {TokenType::OpenCurly, "{"};
    }
    if (c == '}') {
      consume();
      return {TokenType::CloseCurly, "}"};
    }
    if (c == ',') {
      consume();
      return {TokenType::Comma, ","};
    }
    if (c == ':') {
      consume();
      return {TokenType::Colon, ":"};
    }
    if (c == ';') {
      consume();
      return {TokenType::Semicolon, ";"};
    }

    if (c == '@') {
      consume();
      if (isNameStart(peek())) {
        return {TokenType::AtKeyword, consumeName()};
      }
      return {TokenType::Delim, "@"};
    }

    if (c == '+' || c == '-') {
      if (isDigit(peekNext()) ||
          (peekNext() == '.' && isDigit(peekNextNext()))) {
        return consumeNumeric();
      }
      if (isNameStart(peekNext()) ||
          (c == '-' && peekNext() == '-')) { // -foo or --foo
        return consumeIdentLike();
      }
    }

    if (isDigit(c) || (c == '.' && isDigit(peekNext()))) {
      return consumeNumeric();
    }

    if (isNameStart(c)) {
      return consumeIdentLike();
    }

    consume();
    return {TokenType::Delim, std::string(1, c)};
  }
};

} // namespace css
} // namespace xiaopeng
