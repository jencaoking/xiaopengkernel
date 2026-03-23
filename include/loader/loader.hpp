#pragma once

#include "cache.hpp"
#include "connection_pool.hpp"
#include "error.hpp"
#include "http_client.hpp"
#include "http_message.hpp"
#include "renderer/image.hpp"
#include "resource.hpp"
#include "resource_handler.hpp"
#include "scheduler.hpp"
#include "security.hpp"
#include "types.hpp"
#include "url.hpp"
#include <memory>
#include <optional>

namespace xiaopeng {
namespace loader {

struct LoaderConfig {
  HttpClient::Config httpClient;
  ConnectionPool::Config connectionPool;
  HttpCache::Config cache;
  LoadScheduler::Config scheduler;
  SecurityPolicy security;

  std::string userAgent = "XiaopengKernel/0.1.0";
  std::string caBundlePath;
  std::string proxyUrl;

  size_t maxConcurrentLoads = 6;
  size_t maxCacheSize = 50 * 1024 * 1024;
  std::chrono::seconds defaultTimeout{30};
  bool enableHttp2 = true;
  bool enableCache = true;
  bool enableSecurity = true;
};

class Loader {
private:
  Loader() { initialize(LoaderConfig{}); }
  explicit Loader(const LoaderConfig &config) { initialize(config); }

public:
  ~Loader() { shutdown(); }

  static Loader &instance() {
    static Loader loader(LoaderConfig{});
    return loader;
  }

  void initialize(const LoaderConfig &config) {
    config_ = config;

    auto httpClient = std::make_shared<HttpClient>(config.httpClient);
    httpClient->setConfig(config.httpClient);
    httpClient_ = httpClient;

    connectionPool_ = std::make_shared<ConnectionPool>(config.connectionPool);

    cache_ = std::make_shared<HttpCache>(config.cache);

    handlerRegistry_ = std::make_shared<ResourceHandlerRegistry>();
    handlerRegistry_->registerHandler(std::make_shared<HtmlHandler>());
    handlerRegistry_->registerHandler(std::make_shared<CssHandler>());
    handlerRegistry_->registerHandler(std::make_shared<JavaScriptHandler>());
    handlerRegistry_->registerHandler(std::make_shared<ImageHandler>());

    scheduler_ = std::make_shared<LoadScheduler>(config.scheduler);
    scheduler_->setHttpClient(httpClient);
    scheduler_->setCache(cache_);
    scheduler_->setHandlerRegistry(handlerRegistry_);

    sameOriginPolicy_ = std::make_shared<SameOriginPolicy>(config.security);
    corsChecker_ = std::make_shared<CorsChecker>(config.security);

    scheduler_->start();

    initialized_ = true;
  }

  void shutdown() {
    if (scheduler_) {
      scheduler_->stop();
    }

    if (connectionPool_) {
      connectionPool_->clear();
    }

    if (cache_) {
      cache_->clear();
    }

    initialized_ = false;
  }

  Future<Result<Ptr<Resource>>> loadAsync(const std::string &url,
                                          const LoadOptions &options = {}) {
    auto urlResult = Url::parse(url);
    if (urlResult.isErr()) {
      Promise<Result<Ptr<Resource>>> promise;
      promise.set_value(
          Result<Ptr<Resource>>(urlResult.error(), urlResult.errorMessage()));
      return promise.get_future();
    }

    return loadAsync(urlResult.unwrap(), options);
  }

  Future<Result<Ptr<Resource>>> loadAsync(const Url &url,
                                          const LoadOptions &options = {}) {
    if (!initialized_) {
      Promise<Result<Ptr<Resource>>> promise;
      promise.set_value(Result<Ptr<Resource>>(ErrorCode::InvalidState,
                                              "Loader not initialized"));
      return promise.get_future();
    }

    return scheduler_->loadAsync(url, options);
  }

  Result<Ptr<Resource>> load(const std::string &url,
                             const LoadOptions &options = {}) {
    auto future = loadAsync(url, options);
    return future.get();
  }

  Result<Ptr<Resource>> load(const Url &url, const LoadOptions &options = {}) {
    auto future = loadAsync(url, options);
    return future.get();
  }

  // Load image from URL
  std::shared_ptr<renderer::Image> loadImage(const std::string &url) {
    // Synchronous load for now
    auto result = load(url);
    if (result.isOk()) {
      auto resource = result.unwrap();
      if (resource && !resource->data.empty()) {
        return std::make_shared<renderer::Image>(resource->data);
      }
    }
    return nullptr;
  }

  std::string
  scheduleLoad(const std::string &url, const LoadOptions &options,
               std::function<void(Result<Ptr<Resource>>)> callback) {
    auto urlResult = Url::parse(url);
    if (urlResult.isErr()) {
      if (callback) {
        callback(
            Result<Ptr<Resource>>(urlResult.error(), urlResult.errorMessage()));
      }
      return "";
    }

    return scheduleLoad(urlResult.unwrap(), options, std::move(callback));
  }

  std::string
  scheduleLoad(const Url &url, const LoadOptions &options,
               std::function<void(Result<Ptr<Resource>>)> callback) {
    if (!initialized_) {
      if (callback) {
        callback(Result<Ptr<Resource>>(ErrorCode::InvalidState,
                                       "Loader not initialized"));
      }
      return "";
    }

    return scheduler_->schedule(url, options, std::move(callback));
  }

  bool cancelLoad(const std::string &taskId) {
    if (!scheduler_)
      return false;
    return scheduler_->cancel(taskId);
  }

  void cancelAllLoads() {
    if (scheduler_) {
      scheduler_->cancelAll();
    }
  }

  Result<HttpResponse> fetch(const std::string &url,
                             const HttpRequest &request = {}) {
    auto urlResult = Url::parse(url);
    if (urlResult.isErr()) {
      return Result<HttpResponse>(urlResult.error(), urlResult.errorMessage());
    }

    return fetch(urlResult.unwrap(), request);
  }

  Result<HttpResponse> fetch(const Url &url, const HttpRequest &request = {}) {
    if (!httpClient_) {
      return Result<HttpResponse>(ErrorCode::InvalidState,
                                  "HTTP client not initialized");
    }

    HttpRequest req = request;
    req.url = url;

    if (config_.enableSecurity && documentOrigin_.has_value()) {
      auto checkResult = checkSecurity(documentOrigin_.value(), url);
      if (checkResult.isErr()) {
        return Result<HttpResponse>(checkResult.error(),
                                    checkResult.errorMessage());
      }
    }

    return httpClient_->execute(req);
  }

  Future<Result<HttpResponse>> fetchAsync(const Url &url,
                                          const HttpRequest &request = {}) {
    HttpRequest req = request;
    req.url = url;
    return httpClient_->executeAsync(req);
  }

  void setDocumentOrigin(const Origin &origin) { documentOrigin_ = origin; }

  void setDocumentOrigin(const Url &url) { documentOrigin_ = Origin(url); }

  const std::optional<Origin> &documentOrigin() const {
    return documentOrigin_;
  }

  Result<void> checkSecurity(const Origin &origin, const Url &targetUrl) const {
    if (!config_.enableSecurity) {
      return Result<void>();
    }

    Origin targetOrigin(targetUrl);

    Url originUrl(origin.scheme() + "://" + origin.host());
    if (sameOriginPolicy_->isMixedContent(originUrl, targetUrl)) {
      return Result<void>(ErrorCode::MixedContentBlocked,
                          "Mixed content blocked: " + targetUrl.toString());
    }

    return Result<void>();
  }

  bool isSameOrigin(const Url &a, const Url &b) const {
    return sameOriginPolicy_->isSameOrigin(a, b);
  }

  CorsResult checkCors(const HttpRequest &request, const HttpResponse &response,
                       const Origin &origin) const {
    return corsChecker_->checkActualRequest(request, response, origin);
  }

  void clearCache() {
    if (cache_) {
      cache_->clear();
    }
  }

  size_t cacheSize() const { return cache_ ? cache_->size() : 0; }

  size_t cacheCount() const { return cache_ ? cache_->count() : 0; }

  void setCacheEnabled(bool enabled) {
    if (cache_) {
      cache_->setEnabled(enabled);
    }
  }

  void setMaxCacheSize(size_t maxSize) {
    if (cache_) {
      cache_->setMaxSize(maxSize);
    }
  }

  void cleanupCache() {
    if (cache_) {
      cache_->cleanup();
    }
  }

  SchedulerStats schedulerStats() const {
    return scheduler_ ? scheduler_->stats() : SchedulerStats{};
  }

  size_t pendingLoads() const {
    return scheduler_ ? scheduler_->queueSize() : 0;
  }

  size_t activeConnections() const {
    return connectionPool_ ? connectionPool_->activeConnections() : 0;
  }

  size_t idleConnections() const {
    return connectionPool_ ? connectionPool_->idleConnections() : 0;
  }

  void setMaxConcurrent(size_t max) {
    if (scheduler_) {
      scheduler_->setMaxConcurrent(max);
    }
  }

  Ptr<HttpClient> httpClient() const { return httpClient_; }
  Ptr<HttpCache> cache() const { return cache_; }
  Ptr<ConnectionPool> connectionPool() const { return connectionPool_; }
  Ptr<LoadScheduler> scheduler() const { return scheduler_; }
  Ptr<ResourceHandlerRegistry> handlerRegistry() const {
    return handlerRegistry_;
  }
  Ptr<SameOriginPolicy> sameOriginPolicy() const { return sameOriginPolicy_; }
  Ptr<CorsChecker> corsChecker() const { return corsChecker_; }

  bool isInitialized() const { return initialized_; }
  const LoaderConfig &config() const { return config_; }

private:
  Loader(const Loader &) = delete;
  Loader &operator=(const Loader &) = delete;

  LoaderConfig config_;
  bool initialized_ = false;

  Ptr<HttpClient> httpClient_;
  Ptr<HttpCache> cache_;
  Ptr<ConnectionPool> connectionPool_;
  Ptr<LoadScheduler> scheduler_;
  Ptr<ResourceHandlerRegistry> handlerRegistry_;
  Ptr<SameOriginPolicy> sameOriginPolicy_;
  Ptr<CorsChecker> corsChecker_;

  std::optional<Origin> documentOrigin_;
};

inline Loader &GetLoader() { return Loader::instance(); }

} // namespace loader
} // namespace xiaopeng
