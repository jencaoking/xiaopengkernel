#pragma once

#include "types.hpp"
#include "error.hpp"
#include "url.hpp"
#include "http_headers.hpp"
#include <chrono>

namespace xiaopeng {
namespace loader {

struct HttpRequest {
    Url url;
    HttpMethod method = HttpMethod::Get;
    HttpHeaders headers;
    ByteBuffer body;
    std::chrono::milliseconds timeout{30000};
    bool followRedirects = true;
    int maxRedirects = 20;
    bool verifySsl = true;
    std::string httpVersion = "HTTP/1.1";
    
    static HttpRequest get(const Url& url) {
        HttpRequest req;
        req.url = url;
        req.method = HttpMethod::Get;
        return req;
    }
    
    static HttpRequest post(const Url& url, ByteBuffer body = {}) {
        HttpRequest req;
        req.url = url;
        req.method = HttpMethod::Post;
        req.body = std::move(body);
        return req;
    }
    
    static HttpRequest post(const Url& url, const std::string& body) {
        HttpRequest req;
        req.url = url;
        req.method = HttpMethod::Post;
        req.body.assign(body.begin(), body.end());
        req.headers.setContentType("text/plain; charset=utf-8");
        return req;
    }
    
    static HttpRequest postJson(const Url& url, const std::string& json) {
        HttpRequest req;
        req.url = url;
        req.method = HttpMethod::Post;
        req.body.assign(json.begin(), json.end());
        req.headers.setContentType("application/json; charset=utf-8");
        return req;
    }
    
    static HttpRequest put(const Url& url, ByteBuffer body = {}) {
        HttpRequest req;
        req.url = url;
        req.method = HttpMethod::Put;
        req.body = std::move(body);
        return req;
    }
    
    static HttpRequest del(const Url& url) {
        HttpRequest req;
        req.url = url;
        req.method = HttpMethod::Delete;
        return req;
    }
    
    static HttpRequest head(const Url& url) {
        HttpRequest req;
        req.url = url;
        req.method = HttpMethod::Head;
        return req;
    }
    
    static HttpRequest options(const Url& url) {
        HttpRequest req;
        req.url = url;
        req.method = HttpMethod::Options;
        return req;
    }
    
    HttpRequest& withHeader(const std::string& name, const std::string& value) & {
        headers.set(name, value);
        return *this;
    }
    
    HttpRequest&& withHeader(const std::string& name, const std::string& value) && {
        headers.set(name, value);
        return std::move(*this);
    }
    
    HttpRequest& withBody(ByteBuffer data) & {
        body = std::move(data);
        return *this;
    }
    
    HttpRequest&& withBody(ByteBuffer data) && {
        body = std::move(data);
        return std::move(*this);
    }
    
    HttpRequest& withTimeout(std::chrono::milliseconds ms) & {
        timeout = ms;
        return *this;
    }
    
    HttpRequest&& withTimeout(std::chrono::milliseconds ms) && {
        timeout = ms;
        return std::move(*this);
    }
    
    HttpRequest& withFollowRedirects(bool follow, int maxRedirs = 20) & {
        followRedirects = follow;
        maxRedirects = maxRedirs;
        return *this;
    }
    
    HttpRequest&& withFollowRedirects(bool follow, int maxRedirs = 20) && {
        followRedirects = follow;
        maxRedirects = maxRedirs;
        return std::move(*this);
    }
    
    HttpRequest& withSslVerification(bool verify) & {
        verifySsl = verify;
        return *this;
    }
    
    HttpRequest&& withSslVerification(bool verify) && {
        verifySsl = verify;
        return std::move(*this);
    }
    
    HttpRequest& withHttpVersion(const std::string& version) & {
        httpVersion = version;
        return *this;
    }
    
    HttpRequest&& withHttpVersion(const std::string& version) && {
        httpVersion = version;
        return std::move(*this);
    }
};

enum class HttpStatusCode : int {
    Continue = 100,
    SwitchingProtocols = 101,
    Processing = 102,
    EarlyHints = 103,
    
    Ok = 200,
    Created = 201,
    Accepted = 202,
    NonAuthoritativeInformation = 203,
    NoContent = 204,
    ResetContent = 205,
    PartialContent = 206,
    MultiStatus = 207,
    AlreadyReported = 208,
    ImUsed = 226,
    
    MultipleChoices = 300,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    NotModified = 304,
    UseProxy = 305,
    TemporaryRedirect = 307,
    PermanentRedirect = 308,
    
    BadRequest = 400,
    Unauthorized = 401,
    PaymentRequired = 402,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    ProxyAuthenticationRequired = 407,
    RequestTimeout = 408,
    Conflict = 409,
    Gone = 410,
    LengthRequired = 411,
    PreconditionFailed = 412,
    PayloadTooLarge = 413,
    UriTooLong = 414,
    UnsupportedMediaType = 415,
    RangeNotSatisfiable = 416,
    ExpectationFailed = 417,
    ImATeapot = 418,
    MisdirectedRequest = 421,
    UnprocessableEntity = 422,
    Locked = 423,
    FailedDependency = 424,
    TooEarly = 425,
    UpgradeRequired = 426,
    PreconditionRequired = 428,
    TooManyRequests = 429,
    RequestHeaderFieldsTooLarge = 431,
    UnavailableForLegalReasons = 451,
    
    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    HttpVersionNotSupported = 505,
    VariantAlsoNegotiates = 506,
    InsufficientStorage = 507,
    LoopDetected = 508,
    NotExtended = 510,
    NetworkAuthenticationRequired = 511
};

inline bool isInformational(HttpStatusCode code) {
    return static_cast<int>(code) >= 100 && static_cast<int>(code) < 200;
}

inline bool isSuccess(HttpStatusCode code) {
    return static_cast<int>(code) >= 200 && static_cast<int>(code) < 300;
}

inline bool isRedirect(HttpStatusCode code) {
    return static_cast<int>(code) >= 300 && static_cast<int>(code) < 400;
}

inline bool isClientError(HttpStatusCode code) {
    return static_cast<int>(code) >= 400 && static_cast<int>(code) < 500;
}

inline bool isServerError(HttpStatusCode code) {
    return static_cast<int>(code) >= 500 && static_cast<int>(code) < 600;
}

inline std::string getStatusMessage(HttpStatusCode code) {
    switch (code) {
        case HttpStatusCode::Continue: return "Continue";
        case HttpStatusCode::SwitchingProtocols: return "Switching Protocols";
        case HttpStatusCode::Ok: return "OK";
        case HttpStatusCode::Created: return "Created";
        case HttpStatusCode::Accepted: return "Accepted";
        case HttpStatusCode::NoContent: return "No Content";
        case HttpStatusCode::PartialContent: return "Partial Content";
        case HttpStatusCode::MultipleChoices: return "Multiple Choices";
        case HttpStatusCode::MovedPermanently: return "Moved Permanently";
        case HttpStatusCode::Found: return "Found";
        case HttpStatusCode::SeeOther: return "See Other";
        case HttpStatusCode::NotModified: return "Not Modified";
        case HttpStatusCode::TemporaryRedirect: return "Temporary Redirect";
        case HttpStatusCode::PermanentRedirect: return "Permanent Redirect";
        case HttpStatusCode::BadRequest: return "Bad Request";
        case HttpStatusCode::Unauthorized: return "Unauthorized";
        case HttpStatusCode::Forbidden: return "Forbidden";
        case HttpStatusCode::NotFound: return "Not Found";
        case HttpStatusCode::MethodNotAllowed: return "Method Not Allowed";
        case HttpStatusCode::RequestTimeout: return "Request Timeout";
        case HttpStatusCode::Conflict: return "Conflict";
        case HttpStatusCode::Gone: return "Gone";
        case HttpStatusCode::PayloadTooLarge: return "Payload Too Large";
        case HttpStatusCode::UriTooLong: return "URI Too Long";
        case HttpStatusCode::UnsupportedMediaType: return "Unsupported Media Type";
        case HttpStatusCode::TooManyRequests: return "Too Many Requests";
        case HttpStatusCode::InternalServerError: return "Internal Server Error";
        case HttpStatusCode::NotImplemented: return "Not Implemented";
        case HttpStatusCode::BadGateway: return "Bad Gateway";
        case HttpStatusCode::ServiceUnavailable: return "Service Unavailable";
        case HttpStatusCode::GatewayTimeout: return "Gateway Timeout";
        case HttpStatusCode::HttpVersionNotSupported: return "HTTP Version Not Supported";
        default: return "Unknown";
    }
}

struct HttpResponse {
    HttpStatusCode statusCode = HttpStatusCode::Ok;
    std::string statusMessage;
    std::string httpVersion = "HTTP/1.1";
    HttpHeaders headers;
    ByteBuffer body;
    Url finalUrl;
    TimePoint receivedAt;
    Duration totalTime{0};
    Duration dnsTime{0};
    Duration connectTime{0};
    Duration sslTime{0};
    Duration downloadTime{0};
    bool fromCache = false;
    bool cancelled = false;
    
    bool ok() const {
        return isSuccess(statusCode);
    }
    
    bool isRedirect() const {
        return xiaopeng::loader::isRedirect(statusCode);
    }
    
    std::string text() const {
        return std::string(body.begin(), body.end());
    }
    
    std::string_view textView() const {
        return std::string_view(reinterpret_cast<const char*>(body.data()), body.size());
    }
    
    std::span<const Byte> bytes() const {
        return body;
    }
    
    std::string contentType() const {
        return headers.contentType();
    }
    
    std::string mimeType() const {
        auto ct = contentType();
        auto pos = ct.find(';');
        if (pos != std::string::npos) {
            return ct.substr(0, pos);
        }
        return ct;
    }
    
    std::string charset() const {
        auto ct = contentType();
        auto pos = ct.find("charset=");
        if (pos != std::string::npos) {
            auto start = pos + 8;
            auto end = ct.find(';', start);
            std::string result = ct.substr(start, end - start);
            if (!result.empty() && result[0] == '"') {
                result = result.substr(1);
            }
            if (!result.empty() && result.back() == '"') {
                result.pop_back();
            }
            return result;
        }
        return "utf-8";
    }
    
    std::uint64_t contentLength() const {
        return headers.contentLength();
    }
    
    std::string eTag() const {
        return headers.eTag();
    }
    
    std::string lastModified() const {
        return headers.lastModified();
    }
    
    std::string cacheControl() const {
        return headers.cacheControl();
    }
    
    std::string location() const {
        return headers.location();
    }
    
    bool isCacheable() const {
        if (!ok()) return false;
        
        auto cc = cacheControl();
        if (cc.find("no-store") != std::string::npos) return false;
        if (cc.find("private") != std::string::npos) return false;
        
        return true;
    }
    
    std::optional<std::chrono::seconds> maxAge() const {
        auto cc = cacheControl();
        auto pos = cc.find("max-age=");
        if (pos != std::string::npos) {
            try {
                auto start = pos + 8;
                auto end = cc.find(',', start);
                auto ageStr = cc.substr(start, end - start);
                return std::chrono::seconds(std::stol(ageStr));
            } catch (...) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }
};

struct HttpProgress {
    std::uint64_t bytesReceived = 0;
    std::uint64_t totalBytes = 0;
    std::uint64_t bytesSent = 0;
    std::uint64_t totalBytesToSend = 0;
    double downloadSpeed = 0.0;
    double uploadSpeed = 0.0;
    Duration elapsed{0};
    
    double downloadProgress() const {
        if (totalBytes == 0) return 0.0;
        return static_cast<double>(bytesReceived) / static_cast<double>(totalBytes);
    }
    
    double uploadProgress() const {
        if (totalBytesToSend == 0) return 0.0;
        return static_cast<double>(bytesSent) / static_cast<double>(totalBytesToSend);
    }
    
    bool isComplete() const {
        return bytesReceived >= totalBytes && totalBytes > 0;
    }
};

using ProgressCallback = std::function<void(const HttpProgress&)>;
using DataCallback = std::function<void(const ByteBuffer&)>;

}
}
