#include <iostream>
#include <cassert>
#include <script/script_engine.hpp>

using namespace xiaopeng::script;

void test_basic_evaluation() {
    std::cout << "Testing basic JS evaluation..." << std::endl;

    ScriptEngine engine;
    assert(engine.initialize() && "Engine should initialize");

    ScriptResult result = engine.evaluateWithResult("1 + 2");
    if (!result.success) {
        std::cerr << "Error: " << result.error_message << std::endl;
    } else {
        std::cout << "Result: " << result.return_value << std::endl;
    }

    std::cout << "Basic evaluation done!" << std::endl;
}

void test_version_info() {
    std::cout << "Testing version info..." << std::endl;

    std::cout << "QuickJS version: " << ScriptEngine::getQuickJSVersion() << std::endl;
    std::cout << "Engine version: " << ScriptEngine::getEngineVersion() << std::endl;

    ScriptEngine engine;
    assert(engine.initialize());

    ScriptResult result = engine.evaluateWithResult("__xiaopeng__");
    if (result.success) {
        std::cout << "JS __xiaopeng__: " << result.return_value << std::endl;
    }

    std::cout << "Version info done!" << std::endl;
}

void test_globals() {
    std::cout << "Testing global variables..." << std::endl;

    ScriptEngine engine;
    assert(engine.initialize());

    engine.setGlobal("testNumber", 42);
    ScriptResult result = engine.evaluateWithResult("testNumber");
    if (result.success) {
        std::cout << "Global number test passed: " << result.return_value << std::endl;
    } else {
        std::cout << "Global number failed: " << result.error_message << std::endl;
    }

    std::cout << "Global variables test done!" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  XiaopengKernel QuickJS Upgrade Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        test_basic_evaluation();
        test_version_info();
        test_globals();

        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  All tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}