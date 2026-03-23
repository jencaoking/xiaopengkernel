#pragma once

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <loader/loader.hpp>
#include <loader/resource.hpp>
#include <loader/url.hpp>

namespace xiaopeng {
namespace engine {

/// How a <script> should be executed relative to parsing.
enum class ScriptMode {
  Classic, // Blocks parsing; executed in document order
  Async,   // Fetched in parallel; executed as soon as available
  Defer    // Fetched in parallel; executed after parsing, in document order
};

/// A single sub-resource discovered during HTML parsing.
struct ResourceNode {
  std::string url; // Resolved absolute URL
  loader::ResourceType type = loader::ResourceType::Unknown;
  loader::LoadState state = loader::LoadState::Pending;
  ScriptMode scriptMode = ScriptMode::Classic; // Only meaningful for JS

  std::string content;        // Downloaded text (CSS / JS)
  loader::ByteBuffer rawData; // Downloaded binary (Image)
  std::string error;          // Error message if state == Failed

  mutable bool executed = false; // Whether this script has been executed

  bool isLoaded() const { return state == loader::LoadState::Loaded; }
  bool isFailed() const { return state == loader::LoadState::Failed; }
  bool isPending() const { return state == loader::LoadState::Pending; }
  bool isLoading() const { return state == loader::LoadState::Loading; }
  bool isTerminal() const { return isLoaded() || isFailed(); }
};

/// Manages all sub-resources for a single document.
///
/// Lifecycle:
///   1. Discovery — addStylesheet / addScript / addImage called during parsing
///   2. Fetch     — fetchAll() downloads all pending resources in parallel
///   3. Query     — stylesheets() / syncScripts() / etc. used by consumers
class ResourceTree {
public:
  ResourceTree() = default;

  // ─── Discovery ──────────────────────────────────────────────────────

  /// Register an external stylesheet (<link rel="stylesheet">).
  void addStylesheet(const std::string &url) {
    std::lock_guard<std::mutex> lock(mutex_);
    ResourceNode node;
    node.url = url;
    node.type = loader::ResourceType::Css;
    node.state = loader::LoadState::Pending;
    nodes_.push_back(std::move(node));
  }

  /// Register a script.
  /// If inlineContent is non-empty, the script is treated as inline (already
  /// loaded). Otherwise it will be fetched from url.
  void addScript(const std::string &url, ScriptMode mode,
                 const std::string &inlineContent = "") {
    std::lock_guard<std::mutex> lock(mutex_);
    ResourceNode node;
    node.url = url;
    node.type = loader::ResourceType::JavaScript;
    node.scriptMode = mode;

    if (!inlineContent.empty()) {
      // Inline scripts are immediately available
      node.content = inlineContent;
      node.state = loader::LoadState::Loaded;
    } else {
      node.state = loader::LoadState::Pending;
    }
    nodes_.push_back(std::move(node));
  }

  /// Register an image resource (<img src="...">).
  void addImage(const std::string &url) {
    std::lock_guard<std::mutex> lock(mutex_);
    ResourceNode node;
    node.url = url;
    node.type = loader::ResourceType::Image;
    node.state = loader::LoadState::Pending;
    nodes_.push_back(std::move(node));
  }

  // ─── Fetch ──────────────────────────────────────────────────────────

  /// Download all pending resources in parallel using the Loader singleton.
  /// @param baseUrl  Base URL for resolving relative paths.
  void fetchAll(const std::string &baseUrl) {
    // Collect indices of nodes that need fetching
    std::vector<size_t> pendingIndices;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      for (size_t i = 0; i < nodes_.size(); ++i) {
        if (nodes_[i].state == loader::LoadState::Pending &&
            !nodes_[i].url.empty()) {
          nodes_[i].state = loader::LoadState::Loading;
          pendingIndices.push_back(i);
        }
      }
    }

    if (pendingIndices.empty()) {
      return;
    }

    std::cout << "[ResourceTree] Fetching " << pendingIndices.size()
              << " resources in parallel..." << std::endl;

    // Launch all async loads
    struct PendingFetch {
      size_t index;
      std::string resolvedUrl;
      loader::Future<loader::Result<loader::Ptr<loader::Resource>>> future;
    };
    std::vector<PendingFetch> fetches;
    fetches.reserve(pendingIndices.size());

    for (size_t idx : pendingIndices) {
      std::string resolved;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        resolved = resolveUrl(baseUrl, nodes_[idx].url);
      }

      // Set priority based on type: CSS > JS (sync) > JS (defer/async) > Image
      loader::LoadOptions opts;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        auto &n = nodes_[idx];
        if (n.type == loader::ResourceType::Css) {
          opts.priority = loader::ResourcePriority::Highest;
        } else if (n.type == loader::ResourceType::JavaScript) {
          if (n.scriptMode == ScriptMode::Classic) {
            opts.priority = loader::ResourcePriority::High;
          } else {
            opts.priority = loader::ResourcePriority::Medium;
          }
        } else {
          opts.priority = loader::ResourcePriority::Low;
        }
      }

      auto future = loader::Loader::instance().loadAsync(resolved, opts);
      fetches.push_back({idx, resolved, std::move(future)});
    }

    // Wait for all results and populate nodes
    for (auto &f : fetches) {
      auto result = f.future.get(); // Blocks until this resource is loaded

      std::lock_guard<std::mutex> lock(mutex_);
      auto &node = nodes_[f.index];

      if (result.isOk()) {
        auto resource = result.unwrap();
        if (node.type == loader::ResourceType::Image) {
          node.rawData = std::move(resource->data);
        } else {
          node.content =
              std::string(resource->data.begin(), resource->data.end());
        }
        node.state = loader::LoadState::Loaded;
        std::cout << "[ResourceTree] Loaded: " << f.resolvedUrl << " ("
                  << (node.type == loader::ResourceType::Image
                          ? std::to_string(node.rawData.size())
                          : std::to_string(node.content.size()))
                  << " bytes)" << std::endl;
      } else {
        node.state = loader::LoadState::Failed;
        node.error = result.errorMessage();
        std::cerr << "[ResourceTree] Failed: " << f.resolvedUrl << " - "
                  << node.error << std::endl;
      }
    }

    std::cout << "[ResourceTree] All fetches completed" << std::endl;
  }

  // ─── Query ──────────────────────────────────────────────────────────

  /// Are all discovered resources in a terminal state (Loaded or Failed)?
  bool allLoaded() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &n : nodes_) {
      if (!n.isTerminal())
        return false;
    }
    return true;
  }

  /// Are all render-blocking resources (CSS) loaded?
  bool criticalReady() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &n : nodes_) {
      if (n.type == loader::ResourceType::Css && !n.isTerminal())
        return false;
    }
    return true;
  }

  /// Total number of resource nodes.
  size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return nodes_.size();
  }

  /// Get all stylesheets (in document order).
  std::vector<const ResourceNode *> stylesheets() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<const ResourceNode *> result;
    for (const auto &n : nodes_) {
      if (n.type == loader::ResourceType::Css)
        result.push_back(&n);
    }
    return result;
  }

  /// Get classic (synchronous) scripts — no async or defer.
  std::vector<const ResourceNode *> syncScripts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<const ResourceNode *> result;
    for (const auto &n : nodes_) {
      if (n.type == loader::ResourceType::JavaScript &&
          n.scriptMode == ScriptMode::Classic)
        result.push_back(&n);
    }
    return result;
  }

  /// Get defer scripts (in document order).
  std::vector<const ResourceNode *> deferScripts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<const ResourceNode *> result;
    for (const auto &n : nodes_) {
      if (n.type == loader::ResourceType::JavaScript &&
          n.scriptMode == ScriptMode::Defer)
        result.push_back(&n);
    }
    return result;
  }

  /// Get async scripts.
  std::vector<const ResourceNode *> asyncScripts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<const ResourceNode *> result;
    for (const auto &n : nodes_) {
      if (n.type == loader::ResourceType::JavaScript &&
          n.scriptMode == ScriptMode::Async)
        result.push_back(&n);
    }
    return result;
  }

  /// Get image resources.
  std::vector<const ResourceNode *> images() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<const ResourceNode *> result;
    for (const auto &n : nodes_) {
      if (n.type == loader::ResourceType::Image)
        result.push_back(&n);
    }
    return result;
  }

  /// Direct access to all nodes (const).
  const std::vector<ResourceNode> &nodes() const { return nodes_; }

private:
  /// Resolve a relative URL against a base URL.
  static std::string resolveUrl(const std::string &baseUrl,
                                const std::string &relativeUrl) {
    if (relativeUrl.find("://") != std::string::npos) {
      return relativeUrl;
    }
    if (baseUrl.empty()) {
      return relativeUrl;
    }
    auto baseResult = loader::Url::parse(baseUrl);
    if (baseResult.isOk()) {
      auto resolved = baseResult.unwrap().resolve(relativeUrl);
      if (resolved.isValid()) {
        return resolved.toString();
      }
    }
    // Fallback: simple concatenation
    std::string base = baseUrl;
    if (!base.empty() && base.back() != '/') {
      base += '/';
    }
    return base + relativeUrl;
  }

  mutable std::mutex mutex_;
  std::vector<ResourceNode> nodes_;
};

} // namespace engine
} // namespace xiaopeng
