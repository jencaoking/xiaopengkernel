#pragma once

#include "error.hpp"
#include "http_message.hpp"
#include "types.hpp"
#include "url.hpp"
#include <list>
#include <shared_mutex>

namespace xiaopeng {
namespace loader {

struct CacheKey;

struct CacheEntry {
  std::string key;
  ByteBuffer data;
  HttpHeaders headers;
  std::string eTag;
  std::string lastModified;
  TimePoint createdAt;
  TimePoint expiresAt;
  std::string mimeType;
  std::uint64_t size = 0;
  int hitCount = 0;
  bool mustRevalidate = false;
  bool noStore = false;
  std::list<CacheKey>::iterator lruIt;

  bool isExpired() const {
    return std::chrono::steady_clock::now() > expiresAt;
  }

  bool isFresh() const { return !isExpired(); }

  bool needsRevalidation() const { return mustRevalidate || isExpired(); }

  std::chrono::seconds age() const {
    auto elapsed = std::chrono::steady_clock::now() - createdAt;
    return std::chrono::duration_cast<std::chrono::seconds>(elapsed);
  }

  std::chrono::seconds remainingTtl() const {
    auto remaining = expiresAt - std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(remaining);
  }
};

struct CacheKey {
  std::string url;
  std::string varyKey;

  bool operator==(const CacheKey &other) const {
    return url == other.url && varyKey == other.varyKey;
  }
};

struct CacheKeyHash {
  size_t operator()(const CacheKey &key) const {
    size_t h = std::hash<std::string>{}(key.url);
    h ^= std::hash<std::string>{}(key.varyKey) + 0x9e3779b9 + (h << 6) +
         (h >> 2);
    return h;
  }
};

class HttpCache {
public:
  struct Config {
    size_t maxSize = 50 * 1024 * 1024;
    std::chrono::seconds defaultTtl{3600};
    std::chrono::seconds maxTtl{86400 * 30};
    bool enabled = true;
    bool ignoreNoStore = false;
  };

  HttpCache() = default;
  explicit HttpCache(const Config &config) : config_(config) {}

  std::optional<CacheEntry> get(const Url &url,
                                const std::string &varyKey = "") {
    if (!config_.enabled)
      return std::nullopt;

    CacheKey key{url.toString(), varyKey};

    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = cache_.find(key);
    if (it == cache_.end()) {
      return std::nullopt;
    }

    auto &entry = it->second;

    if (entry->noStore && !config_.ignoreNoStore) {
      return std::nullopt;
    }

    entry->hitCount++;

    touchEntry(entry);

    return *entry;
  }

  void put(const Url &url, const HttpResponse &response,
           const std::string &varyKey = "") {
    if (!config_.enabled)
      return;

    if (!isCacheable(response))
      return;

    CacheKey key{url.toString(), varyKey};

    auto entry = createEntry(url, response);
    if (!entry)
      return;

    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = cache_.find(key);
    if (it != cache_.end()) {
      currentSize_ -= it->second->size;
      lruList_.erase(it->second->lruIt);
    }

    while (currentSize_ + entry->size > config_.maxSize && !lruList_.empty()) {
      evictOldest();
    }

    entry->lruIt = lruList_.insert(lruList_.end(), key);
    cache_[key] = entry;
    currentSize_ += entry->size;
  }

  void remove(const Url &url, const std::string &varyKey = "") {
    CacheKey key{url.toString(), varyKey};

    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = cache_.find(key);
    if (it != cache_.end()) {
      currentSize_ -= it->second->size;
      lruList_.erase(it->second->lruIt);
      cache_.erase(it);
    }
  }

  void clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    cache_.clear();
    lruList_.clear();
    currentSize_ = 0;
  }

  void cleanup() {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    for (auto it = cache_.begin(); it != cache_.end();) {
      if (it->second->isExpired()) {
        currentSize_ -= it->second->size;
        lruList_.erase(it->second->lruIt);
        it = cache_.erase(it);
      } else {
        ++it;
      }
    }
  }

  bool has(const Url &url, const std::string &varyKey = "") const {
    CacheKey key{url.toString(), varyKey};

    std::shared_lock<std::shared_mutex> lock(mutex_);
    return cache_.find(key) != cache_.end();
  }

  size_t size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return currentSize_;
  }

  size_t count() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return cache_.size();
  }

  size_t maxSize() const { return config_.maxSize; }

  bool enabled() const { return config_.enabled; }

  void setEnabled(bool enabled) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    config_.enabled = enabled;
  }

  void setMaxSize(size_t maxSize) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    config_.maxSize = maxSize;

    while (currentSize_ > config_.maxSize && !lruList_.empty()) {
      evictOldest();
    }
  }

  HttpRequest createValidationRequest(const Url &url, const CacheEntry &entry) {
    HttpRequest request = HttpRequest::get(url);

    if (!entry.eTag.empty()) {
      request.headers.set("If-None-Match", entry.eTag);
    }

    if (!entry.lastModified.empty()) {
      request.headers.set("If-Modified-Since", entry.lastModified);
    }

    return request;
  }

  bool shouldValidate(const CacheEntry &entry) const {
    return entry.needsRevalidation();
  }

  void updateFromValidation(const Url &url, CacheEntry &entry,
                            const HttpResponse &response) {
    if (response.statusCode == HttpStatusCode::NotModified) {
      entry.createdAt = std::chrono::steady_clock::now();
      entry.expiresAt = calculateExpiry(response);
      entry.hitCount++;
      return;
    }

    if (response.ok()) {
      put(url, response);
    }
  }

private:
  bool isCacheable(const HttpResponse &response) const {
    if (!response.ok() && response.statusCode != HttpStatusCode::NotModified) {
      return false;
    }

    auto cc = response.cacheControl();
    if (cc.find("no-store") != std::string::npos && !config_.ignoreNoStore) {
      return false;
    }

    if (cc.find("private") != std::string::npos) {
      return false;
    }

    if (cc.find("no-cache") != std::string::npos) {
      return true;
    }

    return true;
  }

  Ptr<CacheEntry> createEntry(const Url &url, const HttpResponse &response) {
    auto entry = std::make_shared<CacheEntry>();

    entry->key = url.toString();
    entry->data = response.body;
    entry->headers = response.headers;
    entry->eTag = response.eTag();
    entry->lastModified = response.lastModified();
    entry->createdAt = std::chrono::steady_clock::now();
    entry->expiresAt = calculateExpiry(response);
    entry->mimeType = response.mimeType();
    entry->size = response.body.size();

    auto cc = response.cacheControl();
    entry->mustRevalidate = cc.find("must-revalidate") != std::string::npos ||
                            cc.find("proxy-revalidate") != std::string::npos;
    entry->noStore = cc.find("no-store") != std::string::npos;

    return entry;
  }

  TimePoint calculateExpiry(const HttpResponse &response) const {
    auto maxAge = response.maxAge();

    if (maxAge) {
      auto ttl = std::min(*maxAge, config_.maxTtl);
      return std::chrono::steady_clock::now() + ttl;
    }

    auto lastMod = response.lastModified();
    if (!lastMod.empty()) {
      return std::chrono::steady_clock::now() + config_.defaultTtl;
    }

    return std::chrono::steady_clock::now() + config_.defaultTtl;
  }

  void touchEntry(Ptr<CacheEntry> entry) {
    // Preserve the original key (move it to the back of the LRU list)
    CacheKey key = *entry->lruIt;
    lruList_.erase(entry->lruIt);
    entry->lruIt = lruList_.insert(lruList_.end(), std::move(key));
  }

  void evictOldest() {
    if (lruList_.empty())
      return;

    CacheKey key = lruList_.front();
    lruList_.pop_front();

    auto it = cache_.find(key);
    if (it != cache_.end()) {
      currentSize_ -= it->second->size;
      cache_.erase(it);
    }
  }

  Config config_;
  HashMap<CacheKey, Ptr<CacheEntry>, CacheKeyHash> cache_;
  std::list<CacheKey> lruList_;
  size_t currentSize_ = 0;
  mutable std::shared_mutex mutex_;
};

} // namespace loader
} // namespace xiaopeng
