#pragma once

#include <string>
#include <vector>
#include <memory>

namespace xiaopeng {
namespace loader {
  class Url; // Forward declaration
}

namespace engine {

// Single history entry
struct HistoryEntry {
  std::string url;
  std::string title;
  std::string state;
  std::string scrollPosition;
};

// Browsing context (manages history and navigation)
class BrowsingContext {
public:
  BrowsingContext() = default;

  // History API
  void pushState(const std::string &state, const std::string &title,
                 const std::string &url);
  void replaceState(const std::string &state, const std::string &title,
                    const std::string &url);
  void go(int delta);
  void back();
  void forward();
  size_t length() const { return m_history.size(); }

  // Current state
  std::string getState() const;
  std::string getUrl() const;

  // Get history index
  int getIndex() const { return m_currentIndex; }

  // Set event target for history events
  void setEventTarget(std::shared_ptr<void> target);

  // Navigate to URL
  void navigate(const std::string &url);

  // Get history list (for inspection)
  const std::vector<HistoryEntry> &getHistory() const { return m_history; }

  // Clear browsing context
  void clear();

private:
  // Fire history event
  void firePopStateEvent(const std::string &state);

  std::vector<HistoryEntry> m_history;
  int m_currentIndex = -1;
  std::shared_ptr<void> m_eventTarget;
};

#ifdef ENABLE_QUICKJS
// History API binding for JavaScript
class HistoryBinding {
public:
  static void registerBinding(void *ctx,
                              std::shared_ptr<BrowsingContext> context);

  // Get/set the browsing context
  static void setContext(std::shared_ptr<BrowsingContext> context);
  static std::shared_ptr<BrowsingContext> getContext();

private:
  static std::shared_ptr<BrowsingContext> g_browsingContext;
};
#endif

} // namespace engine
} // namespace xiaopeng
