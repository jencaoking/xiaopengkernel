#pragma once

#include <map>
#include <optional>
#include <string>

namespace xiaopeng {
namespace script {

struct FetchRequestInit {
    std::string method = "GET";
    std::map<std::string, std::string> headers;
    std::optional<std::string> body = std::nullopt;
    std::string referrer;
    std::string referrerPolicy;
    std::string mode;
    std::string credentials;
    std::string cache;
    std::string redirect;
    std::string integrity;
    bool keepalive = false;
};

struct FetchResponseData {
    uint16_t status = 0;
    std::string statusText;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string url;
    bool redirected = false;
    std::string type = "basic";
};

class FetchRequest {
public:
    explicit FetchRequest() = default;
    
    explicit FetchRequest(const std::string& input,
                         std::optional<FetchRequestInit> init = std::nullopt);

    const std::string& url() const { return m_url; }
    const std::string& method() const { return m_method; }
    const std::map<std::string, std::string>& headers() const { return m_headers; }
    const std::optional<std::string>& body() const { return m_body; }
    const std::string& mode() const { return m_mode; }

private:
    std::string m_url;
    std::string m_method = "GET";
    std::map<std::string, std::string> m_headers;
    std::optional<std::string> m_body = std::nullopt;
    std::string m_mode;
};

inline FetchRequest::FetchRequest(const std::string& input,
                                  std::optional<FetchRequestInit> init)
    : m_url(input) {
    if (init.has_value()) {
        const auto& opts = init.value();
        m_method = opts.method;
        m_headers = opts.headers;
        m_body = opts.body;
        m_mode = opts.mode;
    }
}

} // namespace script
} // namespace xiaopeng
