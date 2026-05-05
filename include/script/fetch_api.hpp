#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <future>
#include <chrono>

namespace xiaopeng {
namespace script {

// Forward declaration
class FetchRequest;
class FetchResponse;

// HTTP methods
enum class HttpMethod {
  Get,
  Post,
  Put,
  Delete,
  Head,
  Options,
  Patch
};

// Fetch API options
struct FetchOptions {
  HttpMethod method = HttpMethod::Get;
  std::string body;
  std::map<std::string, std::string> headers;
  bool followRedirects = true;
  int maxRedirects = 20;
  bool verifySsl = true;
  std::chrono::milliseconds timeout = std::chrono::milliseconds(30000);
};

// Fetch Request class
class FetchRequest {
public:
  FetchRequest(const std::string &url);
  FetchRequest(const std::string &url, const FetchOptions &options);

  std::string url() const { return m_url; }
  HttpMethod method() const { return m_method; }
  const std::string &body() const { return m_body; }
  const std::map<std::string, std::string> &headers() const { return m_headers; }
  bool followRedirects() const { return m_followRedirects; }
  int maxRedirects() const { return m_maxRedirects; }
  bool verifySsl() const { return m_verifySsl; }
  std::chrono::milliseconds timeout() const { return m_timeout; }

private:
  std::string m_url;
  HttpMethod m_method = HttpMethod::Get;
  std::string m_body;
  std::map<std::string, std::string> m_headers;
  bool m_followRedirects = true;
  int m_maxRedirects = 20;
  bool m_verifySsl = true;
  std::chrono::milliseconds m_timeout = std::chrono::milliseconds(30000);
};

// Fetch Response class
class FetchResponse {
public:
  FetchResponse() = default;

  int status() const { return m_status; }
  void setStatus(int status) { m_status = status; }

  bool ok() const { return m_status >= 200 && m_status < 300; }

  std::string statusText() const { return m_statusText; }
  void setStatusText(const std::string &text) { m_statusText = text; }

  const std::map<std::string, std::string> &headers() const { return m_headers; }
  void setHeaders(const std::map<std::string, std::string> &headers) { m_headers = headers; }

  const std::vector<uint8_t> &body() const { return m_body; }
  void setBody(const std::vector<uint8_t> &body) { m_body = body; }
  void setBody(const std::string &body) { m_body.assign(body.begin(), body.end()); }

  std::string text() const { return std::string(m_body.begin(), m_body.end()); }
  std::string json() const;

private:
  int m_status = 0;
  std::string m_statusText;
  std::map<std::string, std::string> m_headers;
  std::vector<uint8_t> m_body;
};

// Fetch API manager
class FetchManager {
public:
  static FetchManager &instance();

  std::future<std::shared_ptr<FetchResponse>> fetch(const std::string &url);
  std::future<std::shared_ptr<FetchResponse>> fetch(const std::string &url,
                                                    const FetchOptions &options);
  std::future<std::shared_ptr<FetchResponse>> fetch(const FetchRequest &request);

  // Synchronous versions
  std::shared_ptr<FetchResponse> fetchSync(const std::string &url);
  std::shared_ptr<FetchResponse> fetchSync(const std::string &url,
                                           const FetchOptions &options);

private:
  FetchManager() = default;
  ~FetchManager() = default;

  FetchManager(const FetchManager &) = delete;
  FetchManager &operator=(const FetchManager &) = delete;
};

} // namespace script
} // namespace xiaopeng
