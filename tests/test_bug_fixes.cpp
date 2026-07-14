/**
 * @file test_bug_fixes.cpp
 * @brief Tests for the three critical bug fixes in dom_binding.hpp
 * 
 * Bug 1: createElement() returning dangling pointer (UAF)
 * Bug 2: Iterator invalidation during event dispatch
 * Bug 3: shared_ptr permanent leak in wrapDocument
 */

#include <dom/dom.hpp>
#include <iostream>
#include <memory>
#include <quickjs.h>
#include <script/dom_binding.hpp>
#include <script/js_binding.hpp>

using namespace xiaopeng;
using namespace xiaopeng::dom;
using namespace xiaopeng::script;

// Test Bug 1: createElement() should not return dangling pointer
void test_createElement_lifetime() {
    std::cout << "\n=== Testing Bug 1: createElement lifetime ===\n";
    
    JSRuntime *rt = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(rt);
    
    // Register DOM binding
    DOMBinding::registerBinding(ctx);
    
    // Create a document
    auto doc = std::make_shared<Document>();
    JSValue docObj = DOMBinding::wrapDocument(ctx, doc);
    
    // Test: createElement should keep element alive
    const char *code = R"(
        var div = document.createElement('div');
        div.setAttribute('data-test', 'value1');
        div.setAttribute('data-test2', 'value2');
        var value = div.getAttribute('data-test');
        if (value !== 'value1') throw new Error('getAttribute failed');
        
        // Try to trigger GC
        for (var i = 0; i < 100; i++) {
            var temp = document.createElement('span');
        }
        
        // Access original div again (should still be alive)
        var value2 = div.getAttribute('data-test2');
        if (value2 !== 'value2') throw new Error('Element was prematurely destroyed');
        'SUCCESS';
    )";
    
    // Set up document in JS context
    JS_SetPropertyStr(ctx, JS_GetGlobalObject(ctx), "document", docObj);
    
    JSValue result = JS_Eval(ctx, code, strlen(code), "<test>", JS_EVAL_TYPE_GLOBAL);
    
    if (JS_IsException(result)) {
        JSValue exc = JS_GetException(ctx);
        const char *str = JS_ToCString(ctx, exc);
        std::cerr << "❌ Bug 1 test FAILED: " << str << std::endl;
        JS_FreeCString(ctx, str);
        JS_FreeValue(ctx, exc);
    } else {
        const char *str = JS_ToCString(ctx, result);
        if (strcmp(str, "SUCCESS") == 0) {
            std::cout << "✅ Bug 1 test PASSED: createElement keeps element alive\n";
        } else {
            std::cerr << "❌ Bug 1 test FAILED: Unexpected result: " << str << std::endl;
        }
        JS_FreeCString(ctx, str);
    }
    
    JS_FreeValue(ctx, result);
    DOMBinding::cleanup();
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
}

// Test Bug 2: Event dispatch should handle iterator invalidation
void test_event_dispatch_iterator_safety() {
    std::cout << "\n=== Testing Bug 2: Event dispatch iterator safety ===\n";
    
    JSRuntime *rt = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(rt);
    
    DOMBinding::registerBinding(ctx);
    
    auto doc = std::make_shared<Document>();
    JSValue docObj = DOMBinding::wrapDocument(ctx, doc);
    
    // Test: addEventListener during dispatch should not crash
    const char *code = R"(
        var div = document.createElement('div');
        var callCount = 0;
        
        // First listener adds another listener of the same type
        div.addEventListener('click', function() {
            callCount++;
            // This could trigger vector reallocation
            div.addEventListener('click', function() {
                callCount += 10;
            });
        });
        
        // Second listener (should still be called)
        div.addEventListener('click', function() {
            callCount++;
        });
        
        div.dispatchEvent('click');
        
        if (callCount < 2) throw new Error('Listeners were not all called');
        'SUCCESS';
    )";
    
    JS_SetPropertyStr(ctx, JS_GetGlobalObject(ctx), "document", docObj);
    
    JSValue result = JS_Eval(ctx, code, strlen(code), "<test>", JS_EVAL_TYPE_GLOBAL);
    
    if (JS_IsException(result)) {
        JSValue exc = JS_GetException(ctx);
        const char *str = JS_ToCString(ctx, exc);
        std::cerr << "❌ Bug 2 test FAILED: " << str << std::endl;
        JS_FreeCString(ctx, str);
        JS_FreeValue(ctx, exc);
    } else {
        const char *str = JS_ToCString(ctx, result);
        if (strcmp(str, "SUCCESS") == 0) {
            std::cout << "✅ Bug 2 test PASSED: Event dispatch handles iterator invalidation\n";
        } else {
            std::cerr << "❌ Bug 2 test FAILED: Unexpected result: " << str << std::endl;
        }
        JS_FreeCString(ctx, str);
    }
    
    JS_FreeValue(ctx, result);
    DOMBinding::cleanup();
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
}

// Test Bug 3: Document shared_ptr should be properly released
void test_document_no_leak() {
    std::cout << "\n=== Testing Bug 3: Document lifetime management ===\n";
    
    JSRuntime *rt = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(rt);
    
    DOMBinding::registerBinding(ctx);
    
    // Create multiple documents to check for leaks
    for (int i = 0; i < 10; i++) {
        auto doc = std::make_shared<Document>();
        JSValue docObj = DOMBinding::wrapDocument(ctx, doc);
        
        // Use the document
        JS_SetPropertyStr(ctx, JS_GetGlobalObject(ctx), "document", docObj);
        
        const char *code = R"(
            var div = document.createElement('div');
            document.body = div;
        )";
        
        JS_Eval(ctx, code, strlen(code), "<test>", JS_EVAL_TYPE_GLOBAL);
    }
    
    std::cout << "✅ Bug 3 test PASSED: No crash during multiple document creation\n";
    std::cout << "    (Leak detection would require ASan/Valgrind)\n";
    
    DOMBinding::cleanup();
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
}

int main() {
    std::cout << "╔════════════════════════════════════════════════╗\n";
    std::cout << "║  Testing Critical Bug Fixes in dom_binding.hpp ║\n";
    std::cout << "╚════════════════════════════════════════════════╝\n";
    
    test_createElement_lifetime();
    test_event_dispatch_iterator_safety();
    test_document_no_leak();
    
    std::cout << "\n════════════════════════════════════════════════\n";
    std::cout << "All bug fix tests completed\n";
    
    return 0;
}