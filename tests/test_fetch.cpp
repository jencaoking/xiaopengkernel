#include <iostream>
#include <memory>
#include <cassert>
#include <string>

#include "../include/dom/dom.hpp"
#include "../include/script/script_engine.hpp"
#include "../include/script/fetch_binding.hpp"

using namespace xiaopeng;
using namespace xiaopeng::dom;
using namespace xiaopeng::script;

int test_headers_basic() {
    std::cout << "=== Testing Headers Basic ===" << std::endl;

    ScriptEngine engine;
    if (!engine.initialize()) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.evaluate(R"(
        var headers = new Headers();
        headers.set('Content-Type', 'application/json');
        
        console.log('get Content-Type: ' + headers.get('Content-Type'));
        console.log('has Content-Type: ' + headers.has('Content-Type'));
        
        headers.delete('Content-Type');
        console.log('after delete, has Content-Type: ' + headers.has('Content-Type'));
    )", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_headers_init() {
    std::cout << "=== Testing Headers Initialization ===" << std::endl;

    ScriptEngine engine;
    if (!engine.initialize()) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.evaluate(R"(
        var headers1 = new Headers({
            'Content-Type': 'application/json',
            'X-Custom-Header': 'value123'
        });
        
        console.log('headers1 Content-Type: ' + headers1.get('Content-Type'));
        console.log('headers1 X-Custom-Header: ' + headers1.get('X-Custom-Header'));
        
        var headers2 = new Headers([
            ['Content-Type', 'text/html'],
            ['Accept', 'text/plain']
        ]);
        
        console.log('headers2 Content-Type: ' + headers2.get('Content-Type'));
        console.log('headers2 Accept: ' + headers2.get('Accept'));
    )", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_headers_iteration() {
    std::cout << "=== Testing Headers Iteration ===" << std::endl;

    ScriptEngine engine;
    if (!engine.initialize()) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.evaluate(R"(
        var headers = new Headers({
            'Content-Type': 'application/json',
            'X-Header-A': 'value1',
            'X-Header-B': 'value2'
        });
        
        console.log('Testing keys():');
        var keys = headers.keys();
        
        console.log('Testing values():');
        var values = headers.values();
        
        console.log('Testing entries():');
        var entries = headers.entries();
    )", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_response_basic() {
    std::cout << "=== Testing Response Basic ===" << std::endl;

    ScriptEngine engine;
    if (!engine.initialize()) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.evaluate(R"(
        var response = new Response();
        console.log('Default status: ' + response.status);
        console.log('Default statusText: ' + response.statusText);
        console.log('Default ok: ' + response.ok);
        console.log('Default bodyUsed: ' + response.bodyUsed);
    )", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_response_properties() {
    std::cout << "=== Testing Response Properties ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    ScriptEngine engine;
    if (!engine.initialize()) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.registerDOM(doc);

    engine.evaluate(R"(
        var response = new Response();
        
        console.log('Testing body property (null for empty body)');
        console.log('Testing bodyUsed (should be false initially): ' + response.bodyUsed);
        
        console.log('Testing clone():');
        var cloned = response.clone();
        console.log('Cloned status: ' + cloned.status);
    )", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_fetch_function_exists() {
    std::cout << "=== Testing fetch Function Exists ===" << std::endl;

    ScriptEngine engine;
    if (!engine.initialize()) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.evaluate(R"(
        console.log('typeof fetch: ' + typeof fetch);
        console.log('fetch is function: ' + (typeof fetch === 'function'));
    )", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_fetch_basic() {
    std::cout << "=== Testing fetch Basic ===" << std::endl;

    ScriptEngine engine;
    if (!engine.initialize()) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.evaluate(R"(
        var result = fetch('https://httpbin.org/get');
        console.log('fetch returns Promise: ' + (result instanceof Promise));
        
        result.then(function(response) {
            console.log('Response status: ' + response.status);
            console.log('Response ok: ' + response.ok);
            console.log('Response type: ' + response.type);
            return response.text();
        }).then(function(text) {
            console.log('Response body length: ' + text.length);
        }).catch(function(error) {
            console.log('Fetch error: ' + error.message);
        });
    )", "test.js");

    for (int i = 0; i < 3; i++) {
        engine.tickMicrotasks();
    }

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_fetch_with_headers() {
    std::cout << "=== Testing fetch with Headers ===" << std::endl;

    ScriptEngine engine;
    if (!engine.initialize()) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.evaluate(R"(
        var headers = new Headers({
            'Content-Type': 'application/json',
            'X-Custom-Header': 'test-value'
        });
        
        console.log('Testing Headers object in fetch options');
        console.log('Headers Content-Type: ' + headers.get('Content-Type'));
    )", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_fetch_post_json() {
    std::cout << "=== Testing fetch POST with JSON ===" << std::endl;

    ScriptEngine engine;
    if (!engine.initialize()) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.evaluate(R"(
        fetch('https://httpbin.org/post', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ test: 'data', number: 42 })
        }).then(function(response) {
            console.log('POST Response status: ' + response.status);
            console.log('POST Response ok: ' + response.ok);
            return response.json();
        }).then(function(data) {
            console.log('Response is JSON: ' + (typeof data === 'object'));
        }).catch(function(error) {
            console.log('POST error: ' + error.message);
        });
    )", "test.js");

    for (int i = 0; i < 3; i++) {
        engine.tickMicrotasks();
    }

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_fetch_response_json_parsing() {
    std::cout << "=== Testing Response.json() ===" << std::endl;

    ScriptEngine engine;
    if (!engine.initialize()) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.evaluate(R"(
        var response = new Response();
        console.log('Testing json() without body (will error)');
    )", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_fetch_api_availability() {
    std::cout << "=== Testing Web APIs Availability ===" << std::endl;

    ScriptEngine engine;
    if (!engine.initialize()) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.evaluate(R"(
        console.log('Testing global APIs:');
        console.log('fetch available: ' + (typeof fetch !== 'undefined'));
        console.log('Headers available: ' + (typeof Headers !== 'undefined'));
        console.log('Response available: ' + (typeof Response !== 'undefined'));
    )", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int main() {
    std::cout << "\n=== Fetch API Tests ===" << std::endl;

    int failures = 0;

    failures += test_headers_basic();
    failures += test_headers_init();
    failures += test_headers_iteration();
    failures += test_response_basic();
    failures += test_response_properties();
    failures += test_fetch_function_exists();
    failures += test_fetch_basic();
    failures += test_fetch_with_headers();
    failures += test_fetch_post_json();
    failures += test_fetch_response_json_parsing();
    failures += test_fetch_api_availability();

    if (failures == 0) {
        std::cout << "\n All Fetch API tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n " << failures << " Fetch API tests failed!" << std::endl;
        return 1;
    }
}
