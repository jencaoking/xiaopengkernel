#pragma once

#include <memory>
#include <string>
#include <vector>

#include <css/css_parser.hpp>
#include <dom/dom.hpp>
#include <dom/html_parser.hpp>
#include <engine/parse_task.hpp>
#include <engine/parse_thread.hpp>
#include <engine/resource_tree.hpp>
#include <layout/hit_test.hpp>
#include "layout/form_controls.hpp"
#include <layout/layout_algorithm.hpp>
#include <layout/layout_tree_builder.hpp>
#include <loader/loader.hpp>
#include <renderer/bitmap_canvas.hpp>
#include <renderer/opengl_canvas.hpp>
#include <renderer/painting_algorithm.hpp>
#include <script/script_engine.hpp>
#include <window/sdl_window.hpp>

namespace xiaopeng {
namespace engine {

/// Engine state machine for the async loading pipeline.
enum class EngineState {
  Idle,    // No document loaded
  Loading, // Parse thread is working
  Ready    // Document parsed, layout built, rendering active
};

class BrowserEngine {
public:
  struct Config {
    int windowWidth = 800;
    int windowHeight = 600;
    std::string title = "XiaopengKernel Browser";
    std::string baseUrl;               // For resolving relative paths
    bool enableNetworkLoading = true;  // Enable external resource fetching
    loader::LoaderConfig loaderConfig; // Loader configuration
  };

  BrowserEngine() = default;
  ~BrowserEngine() { shutdown(); }

  // Initialize the engine (script engine, window, parse thread, etc.)
  bool initialize(const Config &cfg);

  // Load and render an HTML string (async — returns immediately)
  void loadHTML(const std::string &html);

  // Load and render a local HTML file
  void loadFile(const std::string &filepath);

  // Enter the event loop (blocking). Returns exit code.
  int run();

  // Mark that a DOM mutation requires an update
  void markDirty(dom::DirtyFlag hint) {
    if (hint == dom::DirtyFlag::NeedsLayout)
      m_needsLayout = true;
    m_needsPaint = true; // Layout always implies paint
  }

  // Current engine state
  EngineState state() const { return m_state; }

  // Shutdown and cleanup
  void shutdown();

private:
  // Called on the main thread when the parse thread delivers a result.
  void onParseComplete(ParseResult result);

  // Internal pipeline stages (main-thread only)
  void executeScripts(const std::vector<dom::HtmlParser::ScriptData> &scripts);
  void executeDeferScripts();
  void checkAsyncScripts();
  void buildLayout();

  // URL resolution & resource fetching (used by script execution)
  std::string resolveUrl(const std::string &relativeUrl) const;
  std::string fetchResource(const std::string &url);

  // Event loop internals
  void handleMouseClicks();
  void handleTimers();
  void handleMicrotasks();
  // Keyboard input handling for simple form controls
  void handleKeyboardInputs();
  // Bind DOM inputs to form controls
  void bindFormControlsFromDocument();

  Config m_config;
  bool m_initialized = false;
  bool m_needsLayout = false;
  bool m_needsPaint = false;
  EngineState m_state = EngineState::Idle;

  // Async parsing infrastructure
  std::unique_ptr<ParseThread> m_parseThread;
  std::shared_ptr<ParseTask> m_pendingParse;

  // Core modules
  // Loader is a singleton, accessed via loader::GetLoader()
  script::ScriptEngine m_scriptEngine;
  window::SdlWindow m_window;
  std::unique_ptr<renderer::BitmapCanvas> m_canvas;
  std::unique_ptr<renderer::OpenGLCanvas> m_openglCanvas;
  renderer::PaintingAlgorithm m_painter;

  // Document state
  std::shared_ptr<dom::Document> m_document;
  css::StyleSheet m_stylesheet;
  layout::LayoutBoxPtr m_layoutRoot;
  std::string m_baseUrl; // Resolved base URL for the current document
  std::shared_ptr<ResourceTree> m_resourceTree; // Sub-resource dependency graph
  layout::FormManager m_formMgr; // Form controls manager (DOM bound)
};

} // namespace engine
} // namespace xiaopeng
