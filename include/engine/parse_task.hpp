#pragma once

#include <atomic>
#include <css/css_parser.hpp>
#include <dom/dom.hpp>
#include <dom/html_parser.hpp>
#include <engine/resource_tree.hpp>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace xiaopeng {
namespace engine {

/// Result of an asynchronous HTML+CSS parse operation.
struct ParseResult {
  std::shared_ptr<dom::Document> document;
  css::StyleSheet stylesheet;
  std::vector<dom::HtmlParser::ScriptData> scripts;
  std::shared_ptr<ResourceTree> resourceTree; // Sub-resource dependency graph
  std::string baseUrl; // Resolved base URL (may be updated by <base> tag)
  bool success = false;
  std::string error;
};

/// A thread-safe handle representing a single in-flight parse operation.
/// The producer (parse thread) sets the result via setResult().
/// The consumer (main thread) polls isReady() and retrieves via getResult().
class ParseTask {
public:
  ParseTask() : ready_(false) {}

  /// Non-blocking check — has the parse thread finished?
  bool isReady() const { return ready_.load(std::memory_order_acquire); }

  /// Retrieve the result. Must only be called once, after isReady() == true.
  ParseResult getResult() { return future_.get(); }

  /// Called by the parse thread to deliver its result.
  void setResult(ParseResult result) {
    promise_.set_value(std::move(result));
    ready_.store(true, std::memory_order_release);
  }

private:
  std::promise<ParseResult> promise_;
  std::future<ParseResult> future_{promise_.get_future()};
  std::atomic<bool> ready_;
};

} // namespace engine
} // namespace xiaopeng
