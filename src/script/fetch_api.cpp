#include <script/fetch_api.hpp>
#include <loader/http_client.hpp>
#include <loader/url.hpp>

namespace xiaopeng {
namespace script {

// Helper function to convert ErrorCode to message
std::string getErrorMessage(loader::ErrorCode code) {
  switch (code) {
  case loader::ErrorCode::Success: return "Success";
  case loader::ErrorCode::NetworkError: return "Network error";
  case loader::ErrorCode::HttpRequestError: return "HTTP request error";
  case loader::ErrorCode::HttpResponseError: return "HTTP response error";
  case loader::ErrorCode::UrlParseError: return "URL parse error";
  case loader::ErrorCode::UrlInvalidScheme: return "Invalid URL scheme";
  case loader::ErrorCode::DnsResolutionFailed: return "DNS resolution failed";
  case loader::ErrorCode::ConnectionFailed: return "Connection failed";
  case loader::ErrorCode::ConnectionTimeout: return "Connection timeout";
  case loader::ErrorCode::SslHandshakeFailed: return "SSL handshake failed";
  case loader::ErrorCode::SslCertificateError: return "SSL certificate error";
  case loader::ErrorCode::HttpRedirectLimitExceeded: return "Redirect limit exceeded";
  default: return "Unknown error";
  }
}

// FetchRequest implementation
FetchRequest::FetchRequest(const std::string &url) : m_url(url) {}

FetchRequest::FetchRequest(const std::string &url, const FetchOptions &options)
    : m_url(url),
      m_method(options.method),
      m_body(options.body),
      m_headers(options.headers),
      m_followRedirects(options.followRedirects),
      m_maxRedirects(options.maxRedirects),
      m_verifySsl(options.verifySsl),
      m_timeout(options.timeout) {}

// FetchResponse implementation
std::string FetchResponse::json() const {
  return text();
}

// Helper function to convert HttpMethod to loader::HttpMethod
loader::HttpMethod toLoaderHttpMethod(HttpMethod method) {
  switch (method) {
  case HttpMethod::Get: return loader::HttpMethod::Get;
  case HttpMethod::Post: return loader::HttpMethod::Post;
  case HttpMethod::Put: return loader::HttpMethod::Put;
  case HttpMethod::Delete: return loader::HttpMethod::Delete;
  case HttpMethod::Head: return loader::HttpMethod::Head;
  case HttpMethod::Options: return loader::HttpMethod::Options;
  case HttpMethod::Patch: return loader::HttpMethod::Patch;
  default: return loader::HttpMethod::Get;
  }
}

// FetchManager implementation
FetchManager &FetchManager::instance() {
  static FetchManager inst;
  return inst;
}

std::future<std::shared_ptr<FetchResponse>> FetchManager::fetch(const std::string &url) {
  return std::async(std::launch::async, [this, url]() {
    return fetchSync(url);
  });
}

std::future<std::shared_ptr<FetchResponse>> FetchManager::fetch(const std::string &url,
                                                               const FetchOptions &options) {
  return std::async(std::launch::async, [this, url, options]() {
    return fetchSync(url, options);
  });
}

std::future<std::shared_ptr<FetchResponse>> FetchManager::fetch(const FetchRequest &request) {
  FetchOptions options;
  options.method = request.method();
  options.body = request.body();
  options.headers = request.headers();
  options.followRedirects = request.followRedirects();
  options.maxRedirects = request.maxRedirects();
  options.verifySsl = request.verifySsl();
  options.timeout = request.timeout();

  return fetch(request.url(), options);
}

std::shared_ptr<FetchResponse> FetchManager::fetchSync(const std::string &url) {
  FetchOptions options;
  return fetchSync(url, options);
}

std::shared_ptr<FetchResponse> FetchManager::fetchSync(const std::string &url,
                                                      const FetchOptions &options) {
  auto response = std::make_shared<FetchResponse>();

  try {
    loader::HttpClient httpClient;

    auto urlResult = loader::Url::parse(url);
    if (!urlResult.isOk()) {
      response->setStatus(0);
      response->setStatusText("Invalid URL");
      return response;
    }

    loader::HttpRequest request;
    request.url = urlResult.unwrap();
    request.method = toLoaderHttpMethod(options.method);
    
    if (!options.body.empty()) {
      request.body.assign(options.body.begin(), options.body.end());
    }
    
    request.followRedirects = options.followRedirects;
    request.maxRedirects = options.maxRedirects;
    request.verifySsl = options.verifySsl;
    request.timeout = options.timeout;

    for (const auto &[name, value] : options.headers) {
      request.headers.add(name, value);
    }

    auto result = httpClient.execute(request);

    if (!result.isOk()) {
      response->setStatus(0);
      response->setStatusText(getErrorMessage(result.error()));
      return response;
    }

    const auto &httpResponse = result.unwrap();

    response->setStatus(static_cast<int>(httpResponse.statusCode));
    response->setStatusText(httpResponse.statusMessage);
    response->setBody(httpResponse.body);

    std::map<std::string, std::string> headers;
    for (const auto &[name, value] : httpResponse.headers.all()) {
      headers[name] = value;
    }
    response->setHeaders(headers);

  } catch (const std::exception &e) {
    response->setStatus(0);
    response->setStatusText(e.what());
  }

  return response;
}

} // namespace script
} // namespace xiaopeng
