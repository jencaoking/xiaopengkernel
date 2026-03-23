#include <engine/minimal_demo.hpp>
#include <engine/browser_engine.hpp>
#include <iostream>
#include <string>

// Lightweight, end-to-end demo: loads inline HTML and renders via BrowserEngine
void runMinimalDemo() {
  xiaopeng::engine::BrowserEngine engine;
  xiaopeng::engine::BrowserEngine::Config cfg;
  cfg.title = "XiaopengKernel - Minimal Demo";

  if (!engine.initialize(cfg)) {
    std::cerr << "Engine initialization failed!" << std::endl;
    return;
  }

  // Minimal inline HTML/CSS to render a simple red square and text
  std::string html = R"(
    <!doctype html>
    <html>
    <head>
      <title>Minimal Demo</title>
      <style>
        body { margin: 0; background: #111; font-family: Arial, sans-serif; }
        #demo {
          width: 320px;
          height: 180px;
          margin: 40px auto;
          background: #222;
          display: flex;
          align-items: center;
          justify-content: center;
          color: #fff;
          border: 2px solid #444;
        }
        #box {
          width: 100px;
          height: 100px;
          background: #e74c3c;
        }
      </style>
    </head>
    <body>
      <div id="demo">
        <div id="box"></div>
      </div>
      <p style="color: #ddd; text-align: center;">Minimal Demo: red square rendered via BrowserEngine</p>
      <script>
        console.log("Minimal demo loaded.");
      </script>
    </body>
    </html>
  )";

  engine.loadHTML(html);
  engine.run();
}
