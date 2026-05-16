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
    
    engine.evaluate("console.log('Hello from JS!');", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_classlist_binding() {
    std::cout << "=== Testing classList Binding ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    auto div = doc->createElement("div");
    div->setId("test-div");
    div->setClassName("initial-class");
    body->appendChild(div);

    ScriptEngine engine;
    bool initialized = engine.initialize();
    if (!initialized) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.registerDOM(doc);
    
    engine.evaluate(R"(
        var div = document.getElementById('test-div');
        console.log('Initial className:', div.className);
        
        div.classList.add('added-class');
        console.log('After add:', div.className);
        
        div.classList.remove('initial-class');
        console.log('After remove:', div.className);
        
        var hasAdded = div.classList.contains('added-class');
        console.log('Contains added-class:', hasAdded);
        
        div.classList.toggle('toggled-class');
        console.log('After toggle:', div.className);
        
        div.classList.toggle('toggled-class');
        console.log('After second toggle:', div.className);
    )", "test_classlist.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_dom_properties() {
    std::cout << "=== Testing DOM Properties Binding ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    auto div = doc->createElement("div");
    div->setId("parent");
    body->appendChild(div);

    auto span1 = doc->createElement("span");
    span1->setId("child1");
    span1->setTextContent("Child 1");
    div->appendChild(span1);

    auto span2 = doc->createElement("span");
    span2->setId("child2");
    span2->setTextContent("Child 2");
    div->appendChild(span2);

    ScriptEngine engine;
    bool initialized = engine.initialize();
    if (!initialized) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.registerDOM(doc);
    
    engine.evaluate(R"(
        var parent = document.getElementById('parent');
        console.log('tagName:', parent.tagName);
        console.log('id:', parent.id);
        console.log('childElementCount:', parent.childElementCount);
        console.log('firstElementChild:', parent.firstElementChild ? parent.firstElementChild.id : 'null');
        console.log('lastElementChild:', parent.lastElementChild ? parent.lastElementChild.id : 'null');
        console.log('children length:', parent.children.length);
        console.log('childNodes length:', parent.childNodes.length);
        console.log('hasChildNodes:', parent.hasChildNodes());
        
        var child1 = document.getElementById('child1');
        console.log('nextElementSibling:', child1.nextElementSibling ? child1.nextElementSibling.id : 'null');
        console.log('parentNode:', child1.parentNode ? child1.parentNode.id : 'null');
    )", "test_properties.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_dom_methods() {
    std::cout << "=== Testing DOM Methods Binding ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    ScriptEngine engine;
    bool initialized = engine.initialize();
    if (!initialized) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.registerDOM(doc);
    
    engine.evaluate(R"(
        var div1 = document.createElement('div');
        div1.setId('new-div');
        div1.className = 'new-class';
        body.appendChild(div1);
        console.log('appendChild worked:', document.getElementById('new-div') !== null);
        
        var div2 = document.createElement('div');
        div2.setId('inserted-div');
        body.insertBefore(div2, div1);
        console.log('insertBefore worked:', document.getElementById('inserted-div') !== null);
        
        var cloned = div1.cloneNode(true);
        console.log('cloneNode worked:', cloned !== null);
        
        console.log('matches div:', div1.matches('div'));
        console.log('matches span:', div1.matches('span'));
        
        var closestDiv = div1.closest('body');
        console.log('closest body:', closestDiv !== null);
        
        div1.normalize();
        console.log('normalize worked without error');
    )", "test_methods.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_query_selector() {
    std::cout << "=== Testing Query Selector Binding ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    auto div1 = doc->createElement("div");
    div1->setId("qtest");
    div1->setClassName("test-class");
    body->appendChild(div1);

    auto div2 = doc->createElement("div");
    div2->setClassName("test-class");
    div1->appendChild(div2);

    ScriptEngine engine;
    bool initialized = engine.initialize();
    if (!initialized) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.registerDOM(doc);
    
    engine.evaluate(R"(
        var byId = document.querySelector('#qtest');
        console.log('querySelector #id:', byId !== null ? byId.id : 'null');
        
        var byClass = document.querySelector('.test-class');
        console.log('querySelector .class:', byClass !== null ? byClass.id : 'null');
        
        var allByClass = document.querySelectorAll('.test-class');
        console.log('querySelectorAll length:', allByClass.length);
        
        var nestedQuery = div1.querySelector('.test-class');
        console.log('nested querySelector:', nestedQuery !== null);
        
        var allNested = div1.querySelectorAll('.test-class');
        console.log('nested querySelectorAll length:', allNested.length);
    )", "test_query.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_event_system() {
    std::cout << "=== Testing Event System Binding ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    auto button = doc->createElement("button");
    button->setId("test-button");
    button->setTextContent("Click me");
    body->appendChild(button);

    ScriptEngine engine;
    bool initialized = engine.initialize();
    if (!initialized) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.registerDOM(doc);
    
    engine.evaluate(R"(
        var clicked = false;
        var button = document.getElementById('test-button');
        
        button.addEventListener('click', function() {
            clicked = true;
            console.log('Button clicked!');
        });
        
        button.dispatchEvent('click');
        console.log('Event dispatched, clicked:', clicked);
        
        button.removeEventListener('click', function() {});
        console.log('removeEventListener called without error');
    )", "test_events.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_attribute_methods() {
    std::cout << "=== Testing Attribute Methods Binding ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    auto input = doc->createElement("input");
    input->setId("test-input");
    input->setAttribute("type", "text");
    input->setAttribute("placeholder", "Enter text");
    body->appendChild(input);

    ScriptEngine engine;
    bool initialized = engine.initialize();
    if (!initialized) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.registerDOM(doc);
    
    engine.evaluate(R"(
        var input = document.getElementById('test-input');
        console.log('getAttribute type:', input.getAttribute('type'));
        console.log('hasAttribute placeholder:', input.hasAttribute('placeholder'));
        console.log('hasAttribute value:', input.hasAttribute('value'));
        
        input.setAttribute('disabled', 'true');
        console.log('After setAttribute disabled:', input.hasAttribute('disabled'));
        
        input.removeAttribute('disabled');
        console.log('After removeAttribute disabled:', input.hasAttribute('disabled'));
    )", "test_attributes.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int main() {
    std::cout << "\n=== DOM Binding Tests ===" << std::endl;
    
    int failures = 0;
    
    failures += test_basic_dom_binding();
    failures += test_classlist_binding();
    failures += test_dom_properties();
    failures += test_dom_methods();
    failures += test_query_selector();
    failures += test_event_system();
    failures += test_attribute_methods();

    if (failures == 0) {
        std::cout << "\n✅ All DOM Binding tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ " << failures << " DOM Binding tests failed!" << std::endl;
        return 1;
    }
}
