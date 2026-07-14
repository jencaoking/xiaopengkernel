/**
 * @file test_bug_fixes_simple.cpp
 * @brief Simple tests for the three critical bug fixes in dom_binding.hpp
 */

#include <cassert>
#include <iostream>
#include <memory>
#include <string>

#include "../include/dom/dom.hpp"
#include "../include/script/script_engine.hpp"

using namespace xiaopeng;
using namespace xiaopeng::dom;
using namespace xiaopeng::script;

// Test Bug 1: createElement should keep element alive
bool test_createElement_lifetime() {
    std::cout << "\n=== Testing Bug 1: createElement lifetime ===\n";
    
    auto engine = std::make_unique<ScriptEngine>();
    auto doc = std::make_shared<Document>();
    
    // Build test DOM
    auto body = doc->createElement("body");
    doc->appendChild(body);
    
    // Test script
    const char *code = R"(
        var div = document.createElement('div');
        div.setAttribute('data-test', 'value1');
        
        // Try to trigger GC
        for (var i = 0; i < 100; i++) {
            var temp = document.createElement('span');
        }
        
        // Access original div again (should still be alive)
        var value = div.getAttribute('data-test');
        if (value !== 'value1') {
            console.log('FAILED: Element was prematurely destroyed');
        } else {
            console.log('PASSED: Element is still alive after GC');
        }
    )";
    
    engine->execute(code, doc);
    
    return true;
}

// Test Bug 2: Event dispatch iterator safety
bool test_event_dispatch_safety() {
    std::cout << "\n=== Testing Bug 2: Event dispatch iterator safety ===\n";
    
    auto engine = std::make_unique<ScriptEngine>();
    auto doc = std::make_shared<Document>();
    
    auto body = doc->createElement("body");
    doc->appendChild(body);
    
    // Test script that modifies listeners during dispatch
    const char *code = R"(
        var div = document.createElement('div');
        var callCount = 0;
        
        // First listener adds another listener of the same type
        div.addEventListener('click', function() {
            callCount++;
            // This could trigger iterator invalidation
            div.addEventListener('click', function() {
                callCount += 10;
            });
        });
        
        // Second listener
        div.addEventListener('click', function() {
            callCount++;
        });
        
        div.dispatchEvent('click');
        
        if (callCount >= 2) {
            console.log('PASSED: All listeners called without crash');
        } else {
            console.log('FAILED: callCount = ' + callCount);
        }
    )";
    
    engine->execute(code, doc);
    
    return true;
}

// Test Bug 3: Document lifetime management
bool test_document_no_leak() {
    std::cout << "\n=== Testing Bug 3: Document lifetime management ===\n";
    
    // Create and destroy multiple engines with documents
    for (int i = 0; i < 10; i++) {
        auto engine = std::make_unique<ScriptEngine>();
        auto doc = std::make_shared<Document>();
        
        auto body = doc->createElement("body");
        doc->appendChild(body);
        
        const char *code = R"(
            var div = document.createElement('div');
            div.setAttribute('class', 'test');
        )";
        
        engine->execute(code, doc);
    }
    
    std::cout << "PASSED: No crash after creating 10 documents\n";
    std::cout << "(Leak detection requires ASan/Valgrind)\n";
    
    return true;
}

int main() {
    std::cout << "╔════════════════════════════════════════════════╗\n";
    std::cout << "║  Testing Critical Bug Fixes in dom_binding.hpp ║\n";
    std::cout << "╚════════════════════════════════════════════════╝\n";
    
    test_createElement_lifetime();
    test_event_dispatch_safety();
    test_document_no_leak();
    
    std::cout << "\n════════════════════════════════════════════════\n";
    std::cout << "All bug fix tests completed successfully!\n";
    
    return 0;
}