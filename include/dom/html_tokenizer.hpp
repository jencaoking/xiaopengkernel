#pragma once

#include "html_types.hpp"
#include "../loader/error.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <unordered_map>
#include <optional>

namespace xiaopeng {
namespace dom {

enum class TokenizerState : uint8_t {
    Data,
    TagOpen,
    EndTagOpen,
    TagName,
    BeforeAttributeName,
    AttributeName,
    AfterAttributeName,
    BeforeAttributeValue,
    AttributeValueDoubleQuoted,
    AttributeValueSingleQuoted,
    AttributeValueUnquoted,
    AfterAttributeValueQuoted,
    SelfClosingStartTag,
    BogusComment,
    MarkupDeclarationOpen,
    CommentStart,
    CommentStartDash,
    Comment,
    CommentEndDash,
    CommentEnd,
    CommentEndBang,
    Doctype,
    BeforeDoctypeName,
    DoctypeName,
    AfterDoctypeName,
    AfterDoctypePublicKeyword,
    BeforeDoctypePublicIdentifier,
    DoctypePublicIdentifierDoubleQuoted,
    DoctypePublicIdentifierSingleQuoted,
    AfterDoctypePublicIdentifier,
    BetweenDoctypePublicAndSystemIdentifiers,
    AfterDoctypeSystemKeyword,
    BeforeDoctypeSystemIdentifier,
    DoctypeSystemIdentifierDoubleQuoted,
    DoctypeSystemIdentifierSingleQuoted,
    AfterDoctypeSystemIdentifier,
    BogusDoctype,
    CdataSection,
    CdataSectionBracket,
    CdataSectionEnd,
    Rcdata,
    RcdataLessThanSign,
    RcdataEndTagOpen,
    RcdataEndTagName,
    Rawtext,
    RawtextLessThanSign,
    RawtextEndTagOpen,
    RawtextEndTagName,
    Plaintext
};

struct ParseError {
    std::string message;
    size_t position = 0;
    size_t line = 1;
    size_t column = 1;
    
    ParseError(std::string msg, size_t pos, size_t l, size_t c)
        : message(std::move(msg)), position(pos), line(l), column(c) {}
};

class HtmlTokenizer {
public:
    using ErrorCallback = std::function<void(const ParseError&)>;
    
    explicit HtmlTokenizer(std::string_view input);
    explicit HtmlTokenizer(const loader::ByteBuffer& input);
    explicit HtmlTokenizer(const std::string& input);
    
    Token nextToken();
    std::vector<Token> tokenize();
    
    bool hasError() const { return !errors_.empty(); }
    const std::vector<ParseError>& errors() const { return errors_; }
    void clearErrors() { errors_.clear(); }
    
    void setErrorCallback(ErrorCallback callback) { errorCallback_ = std::move(callback); }
    
    size_t position() const { return position_; }
    size_t line() const { return line_; }
    size_t column() const { return column_; }
    bool isEof() const { return position_ >= input_.size(); }
    
    TokenizerState state() const { return state_; }
    void setState(TokenizerState state) { state_ = state; }
    
    void setLastStartTag(const std::string& tagName) { lastStartTag_ = tagName; }
    const std::string& lastStartTag() const { return lastStartTag_; }
    
    void setAllowCdata(bool allow) { allowCdata_ = allow; }
    bool allowCdata() const { return allowCdata_; }

private:
    char consumeNextInputCharacter();
    char consumeCharWithReconsume();
    char currentInputCharacter() const;
    char peekNextCharacter(size_t offset = 1) const;
    
    void emitToken(Token token);
    void emitCharacterToken(char c);
    void emitCharacterToken(const std::string& s);
    void emitEofToken();
    void emitError(const std::string& message);
    
    void createStartTagToken();
    void createEndTagToken();
    void createCommentToken();
    void createDoctypeToken();
    
    void appendToTagName(char c);
    void appendToCommentData(char c);
    void appendToAttributeName(char c);
    void appendToAttributeValue(char c);
    void appendToPublicIdentifier(char c);
    void appendToSystemIdentifier(char c);
    
    void startNewAttribute();
    void finishAttribute();
    
    bool isAppropriateEndTag() const;
    
    std::string consumeCharacterReference();
    
    void handleDataState();
    void handleTagOpenState();
    void handleEndTagOpenState();
    void handleTagNameState();
    void handleBeforeAttributeNameState();
    void handleAttributeNameState();
    void handleAfterAttributeNameState();
    void handleBeforeAttributeValueState();
    void handleAttributeValueDoubleQuotedState();
    void handleAttributeValueSingleQuotedState();
    void handleAttributeValueUnquotedState();
    void handleAfterAttributeValueQuotedState();
    void handleSelfClosingStartTagState();
    void handleMarkupDeclarationOpenState();
    void handleCommentStartState();
    void handleCommentStartDashState();
    void handleCommentState();
    void handleCommentEndDashState();
    void handleCommentEndState();
    void handleCommentEndBangState();
    void handleBogusCommentState();
    void handleDoctypeState();
    void handleBeforeDoctypeNameState();
    void handleDoctypeNameState();
    void handleAfterDoctypeNameState();
    void handleAfterDoctypePublicKeywordState();
    void handleBeforeDoctypePublicIdentifierState();
    void handleDoctypePublicIdentifierDoubleQuotedState();
    void handleDoctypePublicIdentifierSingleQuotedState();
    void handleAfterDoctypePublicIdentifierState();
    void handleBetweenDoctypePublicAndSystemIdentifiersState();
    void handleAfterDoctypeSystemKeywordState();
    void handleBeforeDoctypeSystemIdentifierState();
    void handleDoctypeSystemIdentifierDoubleQuotedState();
    void handleDoctypeSystemIdentifierSingleQuotedState();
    void handleAfterDoctypeSystemIdentifierState();
    void handleBogusDoctypeState();
    
    std::string_view input_;
    size_t position_ = 0;
    size_t line_ = 1;
    size_t column_ = 1;
    
    TokenizerState state_ = TokenizerState::Data;
    TokenizerState returnState_ = TokenizerState::Data;
    
    Token currentToken_;
    Token emittedToken_;
    bool hasEmittedToken_ = false;
    
    std::string currentAttributeName_;
    std::string currentAttributeValue_;
    std::string tempBuffer_;
    std::string lastStartTag_;
    std::string characterTokenBuffer_;
    
    std::vector<ParseError> errors_;
    ErrorCallback errorCallback_;
    
    bool reconsume_ = false;
    bool allowCdata_ = false;
    
    static std::unordered_map<std::string, std::string>& getNamedEntities();
};

inline HtmlTokenizer::HtmlTokenizer(std::string_view input)
    : input_(input) {
    currentToken_ = Token::makeEndOfFile();
}

inline HtmlTokenizer::HtmlTokenizer(const loader::ByteBuffer& input)
    : input_(reinterpret_cast<const char*>(input.data()), input.size()) {
    currentToken_ = Token::makeEndOfFile();
}

inline HtmlTokenizer::HtmlTokenizer(const std::string& input)
    : input_(input) {
    currentToken_ = Token::makeEndOfFile();
}

inline char HtmlTokenizer::currentInputCharacter() const {
    if (position_ >= input_.size()) {
        return '\0';
    }
    return input_[position_];
}

inline char HtmlTokenizer::peekNextCharacter(size_t offset) const {
    size_t pos = position_ + offset;
    if (pos >= input_.size()) {
        return '\0';
    }
    return input_[pos];
}

inline char HtmlTokenizer::consumeNextInputCharacter() {
    if (position_ >= input_.size()) {
        return '\0';
    }
    
    char c = input_[position_];
    
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    position_++;
    
    return c;
}

inline char HtmlTokenizer::consumeCharWithReconsume() {
    if (reconsume_) {
        reconsume_ = false;
        if (position_ > 0) {
            position_--;
        }
        return consumeNextInputCharacter();
    }
    return consumeNextInputCharacter();
}

inline void HtmlTokenizer::emitToken(Token token) {
    emittedToken_ = token;
    hasEmittedToken_ = true;
}

inline void HtmlTokenizer::emitCharacterToken(char c) {
    characterTokenBuffer_ += c;
}

inline void HtmlTokenizer::emitCharacterToken(const std::string& s) {
    characterTokenBuffer_ += s;
}

inline void HtmlTokenizer::emitEofToken() {
    if (!characterTokenBuffer_.empty()) {
        emitToken(Token::makeCharacter(characterTokenBuffer_));
        characterTokenBuffer_.clear();
        return;
    }
    emitToken(Token::makeEndOfFile());
}

inline void HtmlTokenizer::emitError(const std::string& message) {
    ParseError error(message, position_, line_, column_);
    errors_.push_back(error);
    if (errorCallback_) {
        errorCallback_(error);
    }
}

inline void HtmlTokenizer::createStartTagToken() {
    currentToken_ = Token();
    currentToken_.type = TokenType::StartTag;
    currentToken_.name.clear();
    currentToken_.attributes.clear();
    currentToken_.selfClosing = false;
}

inline void HtmlTokenizer::createEndTagToken() {
    currentToken_ = Token();
    currentToken_.type = TokenType::EndTag;
    currentToken_.name.clear();
    currentToken_.attributes.clear();
    currentToken_.selfClosing = false;
}

inline void HtmlTokenizer::createCommentToken() {
    currentToken_ = Token();
    currentToken_.type = TokenType::Comment;
    currentToken_.data.clear();
}

inline void HtmlTokenizer::createDoctypeToken() {
    currentToken_ = Token();
    currentToken_.type = TokenType::Doctype;
    currentToken_.name.clear();
    currentToken_.publicIdentifier.clear();
    currentToken_.systemIdentifier.clear();
    currentToken_.forceQuirks = false;
}

inline void HtmlTokenizer::appendToTagName(char c) {
    currentToken_.name += c;
}

inline void HtmlTokenizer::appendToCommentData(char c) {
    currentToken_.data += c;
}

inline void HtmlTokenizer::appendToAttributeName(char c) {
    currentAttributeName_ += c;
}

inline void HtmlTokenizer::appendToAttributeValue(char c) {
    currentAttributeValue_ += c;
}

inline void HtmlTokenizer::appendToPublicIdentifier(char c) {
    currentToken_.publicIdentifier += c;
}

inline void HtmlTokenizer::appendToSystemIdentifier(char c) {
    currentToken_.systemIdentifier += c;
}

inline void HtmlTokenizer::startNewAttribute() {
    currentAttributeName_.clear();
    currentAttributeValue_.clear();
}

inline void HtmlTokenizer::finishAttribute() {
    if (!currentAttributeName_.empty()) {
        bool found = false;
        for (const auto& attr : currentToken_.attributes) {
            if (attr.name == currentAttributeName_) {
                found = true;
                break;
            }
        }
        if (!found) {
            currentToken_.attributes.emplace_back(currentAttributeName_, currentAttributeValue_);
        }
    }
    currentAttributeName_.clear();
    currentAttributeValue_.clear();
}

inline bool HtmlTokenizer::isAppropriateEndTag() const {
    return currentToken_.isEndTag() && 
           toLower(currentToken_.name) == toLower(lastStartTag_);
}

inline Token HtmlTokenizer::nextToken() {
    while (!hasEmittedToken_) {
        switch (state_) {
            case TokenizerState::Data:
                handleDataState();
                break;
            case TokenizerState::TagOpen:
                handleTagOpenState();
                break;
            case TokenizerState::EndTagOpen:
                handleEndTagOpenState();
                break;
            case TokenizerState::TagName:
                handleTagNameState();
                break;
            case TokenizerState::BeforeAttributeName:
                handleBeforeAttributeNameState();
                break;
            case TokenizerState::AttributeName:
                handleAttributeNameState();
                break;
            case TokenizerState::AfterAttributeName:
                handleAfterAttributeNameState();
                break;
            case TokenizerState::BeforeAttributeValue:
                handleBeforeAttributeValueState();
                break;
            case TokenizerState::AttributeValueDoubleQuoted:
                handleAttributeValueDoubleQuotedState();
                break;
            case TokenizerState::AttributeValueSingleQuoted:
                handleAttributeValueSingleQuotedState();
                break;
            case TokenizerState::AttributeValueUnquoted:
                handleAttributeValueUnquotedState();
                break;
            case TokenizerState::AfterAttributeValueQuoted:
                handleAfterAttributeValueQuotedState();
                break;
            case TokenizerState::SelfClosingStartTag:
                handleSelfClosingStartTagState();
                break;
            case TokenizerState::MarkupDeclarationOpen:
                handleMarkupDeclarationOpenState();
                break;
            case TokenizerState::CommentStart:
                handleCommentStartState();
                break;
            case TokenizerState::CommentStartDash:
                handleCommentStartDashState();
                break;
            case TokenizerState::Comment:
                handleCommentState();
                break;
            case TokenizerState::CommentEndDash:
                handleCommentEndDashState();
                break;
            case TokenizerState::CommentEnd:
                handleCommentEndState();
                break;
            case TokenizerState::CommentEndBang:
                handleCommentEndBangState();
                break;
            case TokenizerState::BogusComment:
                handleBogusCommentState();
                break;
            case TokenizerState::Doctype:
                handleDoctypeState();
                break;
            case TokenizerState::BeforeDoctypeName:
                handleBeforeDoctypeNameState();
                break;
            case TokenizerState::DoctypeName:
                handleDoctypeNameState();
                break;
            case TokenizerState::AfterDoctypeName:
                handleAfterDoctypeNameState();
                break;
            case TokenizerState::AfterDoctypePublicKeyword:
                handleAfterDoctypePublicKeywordState();
                break;
            case TokenizerState::BeforeDoctypePublicIdentifier:
                handleBeforeDoctypePublicIdentifierState();
                break;
            case TokenizerState::DoctypePublicIdentifierDoubleQuoted:
                handleDoctypePublicIdentifierDoubleQuotedState();
                break;
            case TokenizerState::DoctypePublicIdentifierSingleQuoted:
                handleDoctypePublicIdentifierSingleQuotedState();
                break;
            case TokenizerState::AfterDoctypePublicIdentifier:
                handleAfterDoctypePublicIdentifierState();
                break;
            case TokenizerState::BetweenDoctypePublicAndSystemIdentifiers:
                handleBetweenDoctypePublicAndSystemIdentifiersState();
                break;
            case TokenizerState::AfterDoctypeSystemKeyword:
                handleAfterDoctypeSystemKeywordState();
                break;
            case TokenizerState::BeforeDoctypeSystemIdentifier:
                handleBeforeDoctypeSystemIdentifierState();
                break;
            case TokenizerState::DoctypeSystemIdentifierDoubleQuoted:
                handleDoctypeSystemIdentifierDoubleQuotedState();
                break;
            case TokenizerState::DoctypeSystemIdentifierSingleQuoted:
                handleDoctypeSystemIdentifierSingleQuotedState();
                break;
            case TokenizerState::AfterDoctypeSystemIdentifier:
                handleAfterDoctypeSystemIdentifierState();
                break;
            case TokenizerState::BogusDoctype:
                handleBogusDoctypeState();
                break;
            default:
                emitEofToken();
                break;
        }
    }
    
    hasEmittedToken_ = false;
    return emittedToken_;
}

inline std::vector<Token> HtmlTokenizer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        Token token = nextToken();
        tokens.push_back(token);
        if (token.isEndOfFile()) {
            break;
        }
    }
    return tokens;
}

inline void HtmlTokenizer::handleDataState() {
    char c = consumeCharWithReconsume();
    
    if (c == '\0' && position_ >= input_.size()) {
        emitEofToken();
        return;
    }
    
    if (c == '&') {
        returnState_ = TokenizerState::Data;
        std::string ref = consumeCharacterReference();
        if (!ref.empty()) {
            if (!characterTokenBuffer_.empty()) {
                emitToken(Token::makeCharacter(characterTokenBuffer_));
                characterTokenBuffer_.clear();
                return;
            }
            emitToken(Token::makeCharacter(ref));
            return;
        }
        emitCharacterToken('&');
        return;
    }
    
    if (c == '<') {
        if (!characterTokenBuffer_.empty()) {
            emitToken(Token::makeCharacter(characterTokenBuffer_));
            characterTokenBuffer_.clear();
            reconsume_ = true;
            return;
        }
        state_ = TokenizerState::TagOpen;
        return;
    }
    
    if (c == '\0') {
        emitError("Unexpected null character");
        emitCharacterToken('\xFF');
        return;
    }
    
    emitCharacterToken(c);
}

inline void HtmlTokenizer::handleTagOpenState() {
    char c = consumeCharWithReconsume();
    
    if (c == '!') {
        state_ = TokenizerState::MarkupDeclarationOpen;
        return;
    }
    
    if (c == '/') {
        state_ = TokenizerState::EndTagOpen;
        return;
    }
    
    if (isAlpha(c)) {
        createStartTagToken();
        appendToTagName(toLower(c));
        state_ = TokenizerState::TagName;
        return;
    }
    
    if (c == '?') {
        emitError("Unexpected question mark instead of tag name");
        createCommentToken();
        state_ = TokenizerState::BogusComment;
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF before tag name");
        emitCharacterToken('<');
        emitEofToken();
        return;
    }
    
    emitError("Invalid first character of tag name");
    emitCharacterToken('<');
    reconsume_ = true;
    state_ = TokenizerState::Data;
}

inline void HtmlTokenizer::handleEndTagOpenState() {
    char c = consumeCharWithReconsume();
    
    if (isAlpha(c)) {
        createEndTagToken();
        appendToTagName(toLower(c));
        state_ = TokenizerState::TagName;
        return;
    }
    
    if (c == '>') {
        emitError("Missing end tag name");
        state_ = TokenizerState::Data;
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF before tag name");
        emitCharacterToken('<');
        emitCharacterToken('/');
        emitEofToken();
        return;
    }
    
    emitError("Invalid first character of tag name");
    createCommentToken();
    state_ = TokenizerState::BogusComment;
    reconsume_ = true;
}

inline void HtmlTokenizer::handleTagNameState() {
    char c = consumeCharWithReconsume();
    
    if (isWhitespace(c)) {
        state_ = TokenizerState::BeforeAttributeName;
        return;
    }
    
    if (c == '/') {
        state_ = TokenizerState::SelfClosingStartTag;
        return;
    }
    
    if (c == '>') {
        state_ = TokenizerState::Data;
        if (currentToken_.isStartTag()) {
            lastStartTag_ = currentToken_.name;
        }
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in tag");
        emitEofToken();
        return;
    }
    
    if (c == '\0') {
        emitError("Unexpected null character");
        appendToTagName('\xFF');
        return;
    }
    
    if (isAlpha(c)) {
        appendToTagName(toLower(c));
    } else {
        appendToTagName(c);
    }
}

inline void HtmlTokenizer::handleBeforeAttributeNameState() {
    char c = consumeCharWithReconsume();
    
    if (isWhitespace(c)) {
        return;
    }
    
    if (c == '/' || c == '>') {
        reconsume_ = true;
        state_ = TokenizerState::AfterAttributeName;
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in tag");
        emitEofToken();
        return;
    }
    
    if (c == '=' || c == '\'' || c == '"') {
        emitError("Unexpected character in attribute name");
    }
    
    startNewAttribute();
    if (isAlpha(c)) {
        appendToAttributeName(toLower(c));
    } else if (c == '\0') {
        emitError("Unexpected null character");
        appendToAttributeName('\xFF');
    } else {
        appendToAttributeName(c);
    }
    state_ = TokenizerState::AttributeName;
}

inline void HtmlTokenizer::handleAttributeNameState() {
    char c = consumeCharWithReconsume();
    
    if (isWhitespace(c) || c == '/' || c == '>') {
        reconsume_ = true;
        state_ = TokenizerState::AfterAttributeName;
        return;
    }
    
    if (c == '=') {
        state_ = TokenizerState::BeforeAttributeValue;
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in tag");
        emitEofToken();
        return;
    }
    
    if (c == '\0') {
        emitError("Unexpected null character");
        appendToAttributeName('\xFF');
        return;
    }
    
    if (c == '\'' || c == '"') {
        emitError("Unexpected character in attribute name");
    }
    
    if (isAlpha(c)) {
        appendToAttributeName(toLower(c));
    } else {
        appendToAttributeName(c);
    }
}

inline void HtmlTokenizer::handleAfterAttributeNameState() {
    char c = consumeCharWithReconsume();
    
    if (isWhitespace(c)) {
        return;
    }
    
    if (c == '/') {
        finishAttribute();
        state_ = TokenizerState::SelfClosingStartTag;
        return;
    }
    
    if (c == '=') {
        state_ = TokenizerState::BeforeAttributeValue;
        return;
    }
    
    if (c == '>') {
        finishAttribute();
        state_ = TokenizerState::Data;
        if (currentToken_.isStartTag()) {
            lastStartTag_ = currentToken_.name;
        }
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in tag");
        emitEofToken();
        return;
    }
    
    finishAttribute();
    startNewAttribute();
    
    if (c == '\0') {
        emitError("Unexpected null character");
        appendToAttributeName('\xFF');
    } else if (isAlpha(c)) {
        appendToAttributeName(toLower(c));
    } else {
        appendToAttributeName(c);
    }
    state_ = TokenizerState::AttributeName;
}

inline void HtmlTokenizer::handleBeforeAttributeValueState() {
    char c = consumeCharWithReconsume();
    
    if (isWhitespace(c)) {
        return;
    }
    
    if (c == '"') {
        state_ = TokenizerState::AttributeValueDoubleQuoted;
        return;
    }
    
    if (c == '\'') {
        state_ = TokenizerState::AttributeValueSingleQuoted;
        return;
    }
    
    if (c == '>') {
        emitError("Missing attribute value");
        finishAttribute();
        state_ = TokenizerState::Data;
        if (currentToken_.isStartTag()) {
            lastStartTag_ = currentToken_.name;
        }
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in tag");
        emitEofToken();
        return;
    }
    
    emitError("Unquoted attribute value");
    reconsume_ = true;
    state_ = TokenizerState::AttributeValueUnquoted;
}

inline void HtmlTokenizer::handleAttributeValueDoubleQuotedState() {
    char c = consumeNextInputCharacter();
    
    if (c == '"') {
        finishAttribute();
        state_ = TokenizerState::AfterAttributeValueQuoted;
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in tag");
        emitEofToken();
        return;
    }
    
    if (c == '\0') {
        emitError("Unexpected null character");
        appendToAttributeValue('\xFF');
        return;
    }
    
    if (c == '&') {
        std::string ref = consumeCharacterReference();
        if (!ref.empty()) {
            currentAttributeValue_ += ref;
            return;
        }
    }
    
    appendToAttributeValue(c);
}

inline void HtmlTokenizer::handleAttributeValueSingleQuotedState() {
    char c = consumeNextInputCharacter();
    
    if (c == '\'') {
        finishAttribute();
        state_ = TokenizerState::AfterAttributeValueQuoted;
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in tag");
        emitEofToken();
        return;
    }
    
    if (c == '\0') {
        emitError("Unexpected null character");
        appendToAttributeValue('\xFF');
        return;
    }
    
    if (c == '&') {
        std::string ref = consumeCharacterReference();
        if (!ref.empty()) {
            currentAttributeValue_ += ref;
            return;
        }
    }
    
    appendToAttributeValue(c);
}

inline void HtmlTokenizer::handleAttributeValueUnquotedState() {
    char c = consumeCharWithReconsume();
    
    if (isWhitespace(c)) {
        finishAttribute();
        state_ = TokenizerState::BeforeAttributeName;
        return;
    }
    
    if (c == '>') {
        finishAttribute();
        state_ = TokenizerState::Data;
        if (currentToken_.isStartTag()) {
            lastStartTag_ = currentToken_.name;
        }
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in tag");
        emitEofToken();
        return;
    }
    
    if (c == '\0') {
        emitError("Unexpected null character");
        appendToAttributeValue('\xFF');
        return;
    }
    
    if (c == '"' || c == '\'' || c == '<' || c == '=' || c == '`') {
        emitError("Unexpected character in unquoted attribute value");
    }
    
    if (c == '&') {
        std::string ref = consumeCharacterReference();
        if (!ref.empty()) {
            currentAttributeValue_ += ref;
            return;
        }
    }
    
    appendToAttributeValue(c);
}

inline void HtmlTokenizer::handleAfterAttributeValueQuotedState() {
    char c = consumeCharWithReconsume();
    
    if (isWhitespace(c)) {
        state_ = TokenizerState::BeforeAttributeName;
        return;
    }
    
    if (c == '/') {
        state_ = TokenizerState::SelfClosingStartTag;
        return;
    }
    
    if (c == '>') {
        state_ = TokenizerState::Data;
        if (currentToken_.isStartTag()) {
            lastStartTag_ = currentToken_.name;
        }
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in tag");
        emitEofToken();
        return;
    }
    
    emitError("Missing whitespace between attributes");
    reconsume_ = true;
    state_ = TokenizerState::BeforeAttributeName;
}

inline void HtmlTokenizer::handleSelfClosingStartTagState() {
    char c = consumeNextInputCharacter();
    
    if (c == '>') {
        currentToken_.selfClosing = true;
        state_ = TokenizerState::Data;
        if (currentToken_.isStartTag()) {
            lastStartTag_ = currentToken_.name;
        }
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in tag");
        emitEofToken();
        return;
    }
    
    emitError("Missing '>' after '/'");
    reconsume_ = true;
    state_ = TokenizerState::BeforeAttributeName;
}

inline void HtmlTokenizer::handleMarkupDeclarationOpenState() {
    if (position_ + 1 < input_.size() &&
        input_[position_] == '-' && input_[position_ + 1] == '-') {
        consumeNextInputCharacter();
        consumeNextInputCharacter();
        createCommentToken();
        state_ = TokenizerState::CommentStart;
        return;
    }
    
    if (position_ + 6 < input_.size() &&
        toLower(input_[position_]) == 'd' &&
        toLower(input_[position_ + 1]) == 'o' &&
        toLower(input_[position_ + 2]) == 'c' &&
        toLower(input_[position_ + 3]) == 't' &&
        toLower(input_[position_ + 4]) == 'y' &&
        toLower(input_[position_ + 5]) == 'p' &&
        toLower(input_[position_ + 6]) == 'e') {
        for (int i = 0; i < 7; i++) {
            consumeNextInputCharacter();
        }
        state_ = TokenizerState::Doctype;
        return;
    }
    
    if (allowCdata_ && position_ + 6 < input_.size() &&
        input_[position_] == '[' &&
        input_[position_ + 1] == 'C' &&
        input_[position_ + 2] == 'D' &&
        input_[position_ + 3] == 'A' &&
        input_[position_ + 4] == 'T' &&
        input_[position_ + 5] == 'A' &&
        input_[position_ + 6] == '[') {
        for (int i = 0; i < 7; i++) {
            consumeNextInputCharacter();
        }
        state_ = TokenizerState::CdataSection;
        return;
    }
    
    emitError("Incorrectly opened comment");
    createCommentToken();
    state_ = TokenizerState::BogusComment;
}

inline void HtmlTokenizer::handleCommentStartState() {
    char c = consumeNextInputCharacter();
    
    if (c == '-') {
        state_ = TokenizerState::CommentStartDash;
        return;
    }
    
    if (c == '>') {
        emitError("Abrupt closing of empty comment");
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in comment");
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    appendToCommentData(c);
    state_ = TokenizerState::Comment;
}

inline void HtmlTokenizer::handleCommentStartDashState() {
    char c = consumeNextInputCharacter();
    
    if (c == '-') {
        state_ = TokenizerState::CommentEnd;
        return;
    }
    
    if (c == '>') {
        emitError("Abrupt closing of empty comment");
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in comment");
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    appendToCommentData('-');
    appendToCommentData(c);
    state_ = TokenizerState::Comment;
}

inline void HtmlTokenizer::handleCommentState() {
    char c = consumeNextInputCharacter();
    
    if (c == '-') {
        state_ = TokenizerState::CommentEndDash;
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in comment");
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0') {
        emitError("Unexpected null character");
        appendToCommentData('\xFF');
        return;
    }
    
    appendToCommentData(c);
}

inline void HtmlTokenizer::handleCommentEndDashState() {
    char c = consumeNextInputCharacter();
    
    if (c == '-') {
        state_ = TokenizerState::CommentEnd;
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in comment");
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    appendToCommentData('-');
    appendToCommentData(c);
    state_ = TokenizerState::Comment;
}

inline void HtmlTokenizer::handleCommentEndState() {
    char c = consumeNextInputCharacter();
    
    if (c == '>') {
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '!') {
        state_ = TokenizerState::CommentEndBang;
        return;
    }
    
    if (c == '-') {
        appendToCommentData('-');
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in comment");
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    appendToCommentData('-');
    appendToCommentData('-');
    appendToCommentData(c);
    state_ = TokenizerState::Comment;
}

inline void HtmlTokenizer::handleCommentEndBangState() {
    char c = consumeNextInputCharacter();
    
    if (c == '-') {
        appendToCommentData('-');
        appendToCommentData('-');
        appendToCommentData('!');
        state_ = TokenizerState::CommentEndDash;
        return;
    }
    
    if (c == '>') {
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in comment");
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    appendToCommentData('-');
    appendToCommentData('-');
    appendToCommentData('!');
    appendToCommentData(c);
    state_ = TokenizerState::Comment;
}

inline void HtmlTokenizer::handleBogusCommentState() {
    char c = consumeNextInputCharacter();
    
    if (c == '>') {
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0') {
        appendToCommentData('\xFF');
        return;
    }
    
    appendToCommentData(c);
}

inline void HtmlTokenizer::handleDoctypeState() {
    char c = consumeCharWithReconsume();
    
    if (isWhitespace(c)) {
        state_ = TokenizerState::BeforeDoctypeName;
        return;
    }
    
    if (c == '>') {
        reconsume_ = true;
        state_ = TokenizerState::BeforeDoctypeName;
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in doctype");
        createDoctypeToken();
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    emitError("Missing whitespace before doctype name");
    reconsume_ = true;
    state_ = TokenizerState::BeforeDoctypeName;
}

inline void HtmlTokenizer::handleBeforeDoctypeNameState() {
    char c = consumeCharWithReconsume();
    
    if (isWhitespace(c)) {
        return;
    }
    
    if (c == '>') {
        emitError("Missing doctype name");
        createDoctypeToken();
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in doctype");
        createDoctypeToken();
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    createDoctypeToken();
    if (c == '\0') {
        emitError("Unexpected null character");
        appendToTagName('\xFF');
    } else if (isAlpha(c)) {
        appendToTagName(toLower(c));
    } else {
        appendToTagName(c);
    }
    state_ = TokenizerState::DoctypeName;
}

inline void HtmlTokenizer::handleDoctypeNameState() {
    char c = consumeNextInputCharacter();
    
    if (isWhitespace(c)) {
        state_ = TokenizerState::AfterDoctypeName;
        return;
    }
    
    if (c == '>') {
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in doctype");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0') {
        emitError("Unexpected null character");
        appendToTagName('\xFF');
        return;
    }
    
    if (isAlpha(c)) {
        appendToTagName(toLower(c));
    } else {
        appendToTagName(c);
    }
}

inline void HtmlTokenizer::handleAfterDoctypeNameState() {
    char c = consumeNextInputCharacter();
    
    if (isWhitespace(c)) {
        return;
    }
    
    if (c == '>') {
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in doctype");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (toLower(c) == 'p' && position_ + 5 < input_.size() &&
        toLower(input_[position_]) == 'u' &&
        toLower(input_[position_ + 1]) == 'b' &&
        toLower(input_[position_ + 2]) == 'l' &&
        toLower(input_[position_ + 3]) == 'i' &&
        toLower(input_[position_ + 4]) == 'c') {
        for (int i = 0; i < 5; i++) {
            consumeNextInputCharacter();
        }
        state_ = TokenizerState::AfterDoctypePublicKeyword;
        return;
    }
    
    if (toLower(c) == 's' && position_ + 5 < input_.size() &&
        toLower(input_[position_]) == 'y' &&
        toLower(input_[position_ + 1]) == 's' &&
        toLower(input_[position_ + 2]) == 't' &&
        toLower(input_[position_ + 3]) == 'e' &&
        toLower(input_[position_ + 4]) == 'm') {
        for (int i = 0; i < 5; i++) {
            consumeNextInputCharacter();
        }
        state_ = TokenizerState::AfterDoctypeSystemKeyword;
        return;
    }
    
    emitError("Invalid character sequence after doctype name");
    currentToken_.forceQuirks = true;
    state_ = TokenizerState::BogusDoctype;
}

inline void HtmlTokenizer::handleAfterDoctypePublicKeywordState() {
    char c = consumeNextInputCharacter();
    
    if (isWhitespace(c)) {
        state_ = TokenizerState::BeforeDoctypePublicIdentifier;
        return;
    }
    
    if (c == '"') {
        emitError("Missing whitespace after doctype public keyword");
        state_ = TokenizerState::DoctypePublicIdentifierDoubleQuoted;
        return;
    }
    
    if (c == '\'') {
        emitError("Missing whitespace after doctype public keyword");
        state_ = TokenizerState::DoctypePublicIdentifierSingleQuoted;
        return;
    }
    
    if (c == '>') {
        emitError("Missing doctype public identifier");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in doctype");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    emitError("Missing quote before doctype public identifier");
    currentToken_.forceQuirks = true;
    state_ = TokenizerState::BogusDoctype;
}

inline void HtmlTokenizer::handleBeforeDoctypePublicIdentifierState() {
    char c = consumeNextInputCharacter();
    
    if (isWhitespace(c)) {
        return;
    }
    
    if (c == '"') {
        state_ = TokenizerState::DoctypePublicIdentifierDoubleQuoted;
        return;
    }
    
    if (c == '\'') {
        state_ = TokenizerState::DoctypePublicIdentifierSingleQuoted;
        return;
    }
    
    if (c == '>') {
        emitError("Missing doctype public identifier");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in doctype");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    emitError("Missing quote before doctype public identifier");
    currentToken_.forceQuirks = true;
    state_ = TokenizerState::BogusDoctype;
}

inline void HtmlTokenizer::handleDoctypePublicIdentifierDoubleQuotedState() {
    char c = consumeNextInputCharacter();
    
    if (c == '"') {
        state_ = TokenizerState::AfterDoctypePublicIdentifier;
        return;
    }
    
    if (c == '>') {
        emitError("Abrupt doctype public identifier");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in doctype");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0') {
        emitError("Unexpected null character");
        appendToPublicIdentifier('\xFF');
        return;
    }
    
    appendToPublicIdentifier(c);
}

inline void HtmlTokenizer::handleDoctypePublicIdentifierSingleQuotedState() {
    char c = consumeNextInputCharacter();
    
    if (c == '\'') {
        state_ = TokenizerState::AfterDoctypePublicIdentifier;
        return;
    }
    
    if (c == '>') {
        emitError("Abrupt doctype public identifier");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in doctype");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0') {
        emitError("Unexpected null character");
        appendToPublicIdentifier('\xFF');
        return;
    }
    
    appendToPublicIdentifier(c);
}

inline void HtmlTokenizer::handleAfterDoctypePublicIdentifierState() {
    char c = consumeNextInputCharacter();
    
    if (isWhitespace(c)) {
        state_ = TokenizerState::BetweenDoctypePublicAndSystemIdentifiers;
        return;
    }
    
    if (c == '>') {
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '"') {
        emitError("Missing whitespace between doctype public and system identifiers");
        state_ = TokenizerState::DoctypeSystemIdentifierDoubleQuoted;
        return;
    }
    
    if (c == '\'') {
        emitError("Missing whitespace between doctype public and system identifiers");
        state_ = TokenizerState::DoctypeSystemIdentifierSingleQuoted;
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in doctype");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    emitError("Missing quote before doctype system identifier");
    currentToken_.forceQuirks = true;
    state_ = TokenizerState::BogusDoctype;
}

inline void HtmlTokenizer::handleBetweenDoctypePublicAndSystemIdentifiersState() {
    char c = consumeNextInputCharacter();
    
    if (isWhitespace(c)) {
        return;
    }
    
    if (c == '>') {
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '"') {
        state_ = TokenizerState::DoctypeSystemIdentifierDoubleQuoted;
        return;
    }
    
    if (c == '\'') {
        state_ = TokenizerState::DoctypeSystemIdentifierSingleQuoted;
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in doctype");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    emitError("Missing quote before doctype system identifier");
    currentToken_.forceQuirks = true;
    state_ = TokenizerState::BogusDoctype;
}

inline void HtmlTokenizer::handleAfterDoctypeSystemKeywordState() {
    char c = consumeNextInputCharacter();
    
    if (isWhitespace(c)) {
        state_ = TokenizerState::BeforeDoctypeSystemIdentifier;
        return;
    }
    
    if (c == '"') {
        emitError("Missing whitespace after doctype system keyword");
        state_ = TokenizerState::DoctypeSystemIdentifierDoubleQuoted;
        return;
    }
    
    if (c == '\'') {
        emitError("Missing whitespace after doctype system keyword");
        state_ = TokenizerState::DoctypeSystemIdentifierSingleQuoted;
        return;
    }
    
    if (c == '>') {
        emitError("Missing doctype system identifier");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in doctype");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    emitError("Missing quote before doctype system identifier");
    currentToken_.forceQuirks = true;
    state_ = TokenizerState::BogusDoctype;
}

inline void HtmlTokenizer::handleBeforeDoctypeSystemIdentifierState() {
    char c = consumeNextInputCharacter();
    
    if (isWhitespace(c)) {
        return;
    }
    
    if (c == '"') {
        state_ = TokenizerState::DoctypeSystemIdentifierDoubleQuoted;
        return;
    }
    
    if (c == '\'') {
        state_ = TokenizerState::DoctypeSystemIdentifierSingleQuoted;
        return;
    }
    
    if (c == '>') {
        emitError("Missing doctype system identifier");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in doctype");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    emitError("Missing quote before doctype system identifier");
    currentToken_.forceQuirks = true;
    state_ = TokenizerState::BogusDoctype;
}

inline void HtmlTokenizer::handleDoctypeSystemIdentifierDoubleQuotedState() {
    char c = consumeNextInputCharacter();
    
    if (c == '"') {
        state_ = TokenizerState::AfterDoctypeSystemIdentifier;
        return;
    }
    
    if (c == '>') {
        emitError("Abrupt doctype system identifier");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in doctype");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0') {
        emitError("Unexpected null character");
        appendToSystemIdentifier('\xFF');
        return;
    }
    
    appendToSystemIdentifier(c);
}

inline void HtmlTokenizer::handleDoctypeSystemIdentifierSingleQuotedState() {
    char c = consumeNextInputCharacter();
    
    if (c == '\'') {
        state_ = TokenizerState::AfterDoctypeSystemIdentifier;
        return;
    }
    
    if (c == '>') {
        emitError("Abrupt doctype system identifier");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in doctype");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0') {
        emitError("Unexpected null character");
        appendToSystemIdentifier('\xFF');
        return;
    }
    
    appendToSystemIdentifier(c);
}

inline void HtmlTokenizer::handleAfterDoctypeSystemIdentifierState() {
    char c = consumeNextInputCharacter();
    
    if (isWhitespace(c)) {
        return;
    }
    
    if (c == '>') {
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        emitError("EOF in doctype");
        currentToken_.forceQuirks = true;
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    emitError("Unexpected character after doctype system identifier");
    state_ = TokenizerState::BogusDoctype;
}

inline void HtmlTokenizer::handleBogusDoctypeState() {
    char c = consumeNextInputCharacter();
    
    if (c == '>') {
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
    
    if (c == '\0' && position_ >= input_.size()) {
        state_ = TokenizerState::Data;
        emitToken(currentToken_);
        return;
    }
}

inline std::string HtmlTokenizer::consumeCharacterReference() {
    if (position_ >= input_.size()) {
        return "";
    }
    
    char c = input_[position_];
    
    if (isWhitespace(c) || c == '<' || c == '&' || c == '\0') {
        return "";
    }
    
    consumeNextInputCharacter();
    
    if (c == '#') {
        int code = 0;
        bool valid = false;
        
        if (position_ < input_.size()) {
            char next = input_[position_];
            if (next == 'x' || next == 'X') {
                consumeNextInputCharacter();
                while (position_ < input_.size()) {
                    char hex = input_[position_];
                    if (isHexDigit(hex)) {
                        code *= 16;
                        if (isDigit(hex)) {
                            code += hex - '0';
                        } else if (hex >= 'a' && hex <= 'f') {
                            code += hex - 'a' + 10;
                        } else {
                            code += hex - 'A' + 10;
                        }
                        valid = true;
                        consumeNextInputCharacter();
                    } else {
                        break;
                    }
                }
            } else {
                while (position_ < input_.size()) {
                    char dec = input_[position_];
                    if (isDigit(dec)) {
                        code *= 10;
                        code += dec - '0';
                        valid = true;
                        consumeNextInputCharacter();
                    } else {
                        break;
                    }
                }
            }
        }
        
        if (!valid) {
            emitError("Missing digits in numeric character reference");
            return "";
        }
        
        if (position_ < input_.size() && input_[position_] == ';') {
            consumeNextInputCharacter();
        } else {
            emitError("Missing semicolon after numeric character reference");
        }
        
        if (code == 0) {
            emitError("Null character reference");
            code = 0xFFFD;
        } else if (code > 0x10FFFF) {
            emitError("Character reference outside unicode range");
            code = 0xFFFD;
        } else if ((code >= 0xD800 && code <= 0xDFFF) || code == 0xFFFE || code == 0xFFFF) {
            emitError("Surrogate character reference");
            code = 0xFFFD;
        }
        
        std::string result;
        if (code < 0x80) {
            result = static_cast<char>(code);
        } else if (code < 0x800) {
            result += static_cast<char>(0xC0 | (code >> 6));
            result += static_cast<char>(0x80 | (code & 0x3F));
        } else if (code < 0x10000) {
            result += static_cast<char>(0xE0 | (code >> 12));
            result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (code & 0x3F));
        } else {
            result += static_cast<char>(0xF0 | (code >> 18));
            result += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
            result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (code & 0x3F));
        }
        
        return result;
    }
    
    std::string entityName(1, c);
    while (position_ < input_.size()) {
        char next = input_[position_];
        if (isAlphanumeric(next)) {
            entityName += next;
            consumeNextInputCharacter();
        } else {
            break;
        }
    }
    
    auto& entities = getNamedEntities();
    auto it = entities.find(entityName);
    if (it != entities.end()) {
        if (position_ < input_.size() && input_[position_] == ';') {
            consumeNextInputCharacter();
        }
        return it->second;
    }
    
    return "";
}

inline std::unordered_map<std::string, std::string>& HtmlTokenizer::getNamedEntities() {
    static std::unordered_map<std::string, std::string> namedEntities = {
        {"nbsp", "\u00A0"}, {"lt", "<"}, {"gt", ">"}, {"amp", "&"},
        {"quot", "\""}, {"apos", "'"}, {"copy", "\u00A9"}, {"reg", "\u00AE"},
        {"trade", "\u2122"}, {"euro", "\u20AC"}, {"pound", "\u00A3"},
        {"yen", "\u00A5"}, {"cent", "\u00A2"}, {"deg", "\u00B0"},
        {"plusmn", "\u00B1"}, {"times", "\u00D7"}, {"divide", "\u00F7"}
    };
    return namedEntities;
}

}
}
