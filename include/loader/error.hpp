#pragma once

#include "types.hpp"
#include <system_error>
#include <stdexcept>

namespace xiaopeng {
namespace loader {

enum class ErrorCode : int {
    Success = 0,
    
    Unknown = 1,
    InvalidArgument = 2,
    InvalidState = 3,
    NotImplemented = 4,
    
    UrlParseError = 100,
    UrlInvalidScheme = 101,
    UrlInvalidHost = 102,
    UrlInvalidPort = 103,
    
    NetworkError = 200,
    ConnectionFailed = 201,
    ConnectionTimeout = 202,
    ConnectionRefused = 203,
    DnsResolutionFailed = 204,
    SslHandshakeFailed = 205,
    SslCertificateError = 206,
    
    HttpRequestError = 300,
    HttpResponseError = 301,
    HttpInvalidHeader = 302,
    HttpInvalidStatus = 303,
    HttpRedirectLimitExceeded = 304,
    HttpVersionNotSupported = 305,
    
    ResourceNotFound = 400,
    ResourceLoadFailed = 401,
    ResourceParseError = 402,
    ResourceDecodeError = 403,
    ResourceTimeout = 404,
    ResourceCancelled = 405,
    
    CacheError = 500,
    CacheMiss = 501,
    CacheExpired = 502,
    CacheInvalid = 503,
    
    SecurityError = 600,
    CorsViolation = 601,
    SameOriginViolation = 602,
    MixedContentBlocked = 603,
    InsecureContentBlocked = 604,
    
    ParserError = 700,
    HtmlParseError = 701,
    CssParseError = 702,
    JsParseError = 703,
    ImageDecodeError = 704,
    
    HtmlTokenizerError = 710,
    HtmlUnexpectedToken = 711,
    HtmlUnclosedTag = 712,
    HtmlInvalidTag = 713,
    HtmlInvalidAttribute = 714,
    HtmlInvalidEntity = 715,
    HtmlTreeBuilderError = 720,
    HtmlInvalidNesting = 721,
    HtmlMissingRequiredElement = 722,
};

class LoaderException : public std::runtime_error {
public:
    explicit LoaderException(ErrorCode code, const std::string& message = "")
        : std::runtime_error(message.empty() ? GetDefaultMessage(code) : message)
        , code_(code) {}
    
    ErrorCode code() const noexcept { return code_; }
    
    static std::string GetDefaultMessage(ErrorCode code) {
        switch (code) {
            case ErrorCode::Success: return "Success";
            case ErrorCode::Unknown: return "Unknown error";
            case ErrorCode::InvalidArgument: return "Invalid argument";
            case ErrorCode::InvalidState: return "Invalid state";
            case ErrorCode::NotImplemented: return "Not implemented";
            
            case ErrorCode::UrlParseError: return "URL parse error";
            case ErrorCode::UrlInvalidScheme: return "Invalid URL scheme";
            case ErrorCode::UrlInvalidHost: return "Invalid URL host";
            case ErrorCode::UrlInvalidPort: return "Invalid URL port";
            
            case ErrorCode::NetworkError: return "Network error";
            case ErrorCode::ConnectionFailed: return "Connection failed";
            case ErrorCode::ConnectionTimeout: return "Connection timeout";
            case ErrorCode::ConnectionRefused: return "Connection refused";
            case ErrorCode::DnsResolutionFailed: return "DNS resolution failed";
            case ErrorCode::SslHandshakeFailed: return "SSL handshake failed";
            case ErrorCode::SslCertificateError: return "SSL certificate error";
            
            case ErrorCode::HttpRequestError: return "HTTP request error";
            case ErrorCode::HttpResponseError: return "HTTP response error";
            case ErrorCode::HttpInvalidHeader: return "Invalid HTTP header";
            case ErrorCode::HttpInvalidStatus: return "Invalid HTTP status";
            case ErrorCode::HttpRedirectLimitExceeded: return "Redirect limit exceeded";
            case ErrorCode::HttpVersionNotSupported: return "HTTP version not supported";
            
            case ErrorCode::ResourceNotFound: return "Resource not found";
            case ErrorCode::ResourceLoadFailed: return "Resource load failed";
            case ErrorCode::ResourceParseError: return "Resource parse error";
            case ErrorCode::ResourceDecodeError: return "Resource decode error";
            case ErrorCode::ResourceTimeout: return "Resource load timeout";
            case ErrorCode::ResourceCancelled: return "Resource load cancelled";
            
            case ErrorCode::CacheError: return "Cache error";
            case ErrorCode::CacheMiss: return "Cache miss";
            case ErrorCode::CacheExpired: return "Cache expired";
            case ErrorCode::CacheInvalid: return "Cache invalid";
            
            case ErrorCode::SecurityError: return "Security error";
            case ErrorCode::CorsViolation: return "CORS violation";
            case ErrorCode::SameOriginViolation: return "Same-origin policy violation";
            case ErrorCode::MixedContentBlocked: return "Mixed content blocked";
            case ErrorCode::InsecureContentBlocked: return "Insecure content blocked";
            
            case ErrorCode::ParserError: return "Parser error";
            case ErrorCode::HtmlParseError: return "HTML parse error";
            case ErrorCode::CssParseError: return "CSS parse error";
            case ErrorCode::JsParseError: return "JavaScript parse error";
            case ErrorCode::ImageDecodeError: return "Image decode error";
            
            case ErrorCode::HtmlTokenizerError: return "HTML tokenizer error";
            case ErrorCode::HtmlUnexpectedToken: return "Unexpected token in HTML";
            case ErrorCode::HtmlUnclosedTag: return "Unclosed HTML tag";
            case ErrorCode::HtmlInvalidTag: return "Invalid HTML tag";
            case ErrorCode::HtmlInvalidAttribute: return "Invalid HTML attribute";
            case ErrorCode::HtmlInvalidEntity: return "Invalid HTML entity";
            case ErrorCode::HtmlTreeBuilderError: return "HTML tree builder error";
            case ErrorCode::HtmlInvalidNesting: return "Invalid HTML element nesting";
            case ErrorCode::HtmlMissingRequiredElement: return "Missing required HTML element";
            
            default: return "Unknown error code";
        }
    }

private:
    ErrorCode code_;
};

template<typename T>
class Result {
public:
    using ValueType = T;
    using ErrorType = ErrorCode;
    
    Result(T value) : data_(std::move(value)) {}
    Result(ErrorCode error, std::string message = "") 
        : data_(std::make_pair(error, message.empty() ? LoaderException::GetDefaultMessage(error) : std::move(message))) {}
    
    bool isOk() const { return std::holds_alternative<T>(data_); }
    bool isErr() const { return !isOk(); }
    
    explicit operator bool() const { return isOk(); }
    
    T& unwrap() & {
        if (isErr()) {
            throw LoaderException(error(), errorMessage());
        }
        return std::get<T>(data_);
    }
    
    T&& unwrap() && {
        if (isErr()) {
            throw LoaderException(error(), errorMessage());
        }
        return std::get<T>(std::move(data_));
    }
    
    const T& unwrap() const& {
        if (isErr()) {
            throw LoaderException(error(), errorMessage());
        }
        return std::get<T>(data_);
    }
    
    T unwrapOr(T defaultValue) const {
        return isOk() ? std::get<T>(data_) : std::move(defaultValue);
    }
    
    T unwrapOrElse(std::function<T()> f) const {
        return isOk() ? std::get<T>(data_) : f();
    }
    
    ErrorCode error() const {
        if (isOk()) return ErrorCode::Success;
        return std::get<std::pair<ErrorCode, std::string>>(data_).first;
    }
    
    const std::string& errorMessage() const {
        static const std::string empty;
        if (isOk()) return empty;
        return std::get<std::pair<ErrorCode, std::string>>(data_).second;
    }
    
    template<typename U>
    Result<U> map(std::function<U(const T&)> f) const {
        if (isOk()) {
            return Result<U>(f(unwrap()));
        }
        return Result<U>(error(), errorMessage());
    }
    
    template<typename U>
    Result<U> andThen(std::function<Result<U>(const T&)> f) const {
        if (isOk()) {
            return f(unwrap());
        }
        return Result<U>(error(), errorMessage());
    }

private:
    std::variant<T, std::pair<ErrorCode, std::string>> data_;
};

template<>
class Result<void> {
public:
    Result() : error_(ErrorCode::Success) {}
    Result(ErrorCode error, std::string message = "") 
        : error_(error), errorMessage_(message.empty() ? LoaderException::GetDefaultMessage(error) : std::move(message)) {}
    
    bool isOk() const { return error_ == ErrorCode::Success; }
    bool isErr() const { return !isOk(); }
    
    explicit operator bool() const { return isOk(); }
    
    void unwrap() const {
        if (isErr()) {
            throw LoaderException(error_, errorMessage_);
        }
    }
    
    ErrorCode error() const { return error_; }
    
    const std::string& errorMessage() const { return errorMessage_; }

private:
    ErrorCode error_;
    std::string errorMessage_;
};

}
}
