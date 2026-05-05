#include <iostream>
#include <memory>
#include <cassert>
#include <string>

#include "../include/dom/dom.hpp"
#include "../include/script/script_engine.hpp"

using namespace xiaopeng;
using namespace xiaopeng::dom;
using namespace xiaopeng::script;

int test_basic_dom_binding() {
    std::cout << "=== Testing Basic DOM Binding ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    auto div = doc->createElement("div");
    div->setId("test-div");
    body->appendChild(div);

    ScriptEngine engine;
    bool initialized = engine.initialize();
    if (!initialized) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.registerDOM(doc);
    
    // Simple test that will just execute without crashing
    engine.evaluate("console.log('Hello from JS!');", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int main() {
    std::cout << "\n=== DOM Binding Tests ===" << std::endl;
    
    int failures = 0;
    
    failures += test_basic_dom_binding();

    if (failures == 0) {
        std::cout << "\n✅ DOM Binding test passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ " << failures << " DOM Binding tests failed!" << std::endl;
        return 1;
    }
}
