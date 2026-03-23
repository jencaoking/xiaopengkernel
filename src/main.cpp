#include <engine/browser_engine.hpp>
#include <engine/minimal_demo.hpp>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  // If --minimal-demo is passed, run minimal demo and exit
  if (argc > 1 && std::string(argv[1]) == "--minimal-demo") {
    runMinimalDemo();
    return 0;
  }
  // Use BrowserEngine to orchestrate all components
  xiaopeng::engine::BrowserEngine engine;
  xiaopeng::engine::BrowserEngine::Config cfg;
  cfg.title = "XiaopengKernel - All-in-One Engine";

  if (!engine.initialize(cfg)) {
    std::cerr << "Engine initialization failed!" << std::endl;
    return 1;
  }

  // Default HTML for demo if no file is provided
  std::string defaultHtml = R"(
    <html>
      <head>
        <title>XiaopengKernel Demo</title>
        <style>
          body { display: block; margin: 0; padding: 0; }
          div { display: block; margin: 10px; padding: 10px; border: 1px solid black; }
          .container { width: 400px; background-color: #f0f0f0; }
          .inner { width: 100px; height: 100px; background-color: #ff0000; }
          p { color: #0000ff; display: block; }
          span { background-color: #00ff00; }
        </style>
      </head>
      <body>
        <div class="container" id="demo">
          <div class="inner" id="clickTarget"></div>
          <p>This is the <span>BrowserEngine</span> glue layer.</p>
          <p>Everything is now orchestrated via a <span>single class</span>.</p>
        </div>
        <script>
            console.log("=== BrowserEngine Integrated Test ===");
            var target = document.getElementById("clickTarget");
            if(target) {
                target.addEventListener("click", function() {
                    console.log("BOOM! Clicked via BrowserEngine!");
                    target.innerHTML = "<span>Clicked!<" + "/span>";
                });
                console.log("Click handler registered on #clickTarget");
            }
            
            var count = 0;
            setInterval(function() {
                count++;
                console.log("Engine Heartbeat #" + count);
            }, 5000);
        </script>
      </body>
    </html>
  )";

  if (argc > 1) {
    std::cout << "Loading file: " << argv[1] << std::endl;
    engine.loadFile(argv[1]);
  } else {
    std::cout << "Loading default demo HTML..." << std::endl;
    engine.loadHTML(defaultHtml);
  }

  // Enter the event loop
  return engine.run();
}
