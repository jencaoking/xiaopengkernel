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

int test_element_properties() {
    std::cout << "=== Testing Element Properties ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    auto div = doc->createElement("div");
    div->setId("test-div");
    div->setClassName("container active");
    body->appendChild(div);

    ScriptEngine engine;
    if (!engine.initialize()) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.registerDOM(doc);

    bool all_passed = true;

    engine.evaluate(R"(
        var div = document.getElementById("test-div");
        console.log("Testing element.id getter: " + div.id);
        console.log("Testing element.tagName: " + div.tagName);
        console.log("Testing element.className: " + div.className);
    )", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return all_passed ? 0 : 1;
}

int test_classlist_api() {
    std::cout << "=== Testing classList API ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    auto div = doc->createElement("div");
    div->setId("test-div");
    div->setClassName("container");
    body->appendChild(div);

    ScriptEngine engine;
    if (!engine.initialize()) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.registerDOM(doc);

    engine.evaluate(R"(
        var div = document.getElementById("test-div");
        var list = div.classList;

        console.log("Initial classList: " + list.toString());
        console.log("classList.length: " + list.length);
        console.log("classList.contains('container'): " + list.contains("container"));

        list.add("active");
        console.log("After add('active'): " + div.className);

        list.remove("container");
        console.log("After remove('container'): " + div.className);

        list.toggle("hidden");
        console.log("After toggle('hidden'): " + div.className);

        var item0 = list.item(0);
        console.log("classList.item(0): " + item0);

        list.replace("hidden", "visible");
        console.log("After replace('hidden', 'visible'): " + div.className);
    )", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_document_properties() {
    std::cout << "=== Testing Document Properties ===" << std::endl;

    auto doc = std::make_shared<Document>();

    auto html = doc->createElement("html");
    doc->appendChild(html);

    auto head = doc->createElement("head");
    html->appendChild(head);

    auto title = doc->createElement("title");
    head->appendChild(title);
    auto titleText = doc->createTextNode("Test Page Title");
    title->appendChild(titleText);

    auto body = doc->createElement("body");
    html->appendChild(body);

    auto div = doc->createElement("div");
    div->setId("main");
    body->appendChild(div);

    ScriptEngine engine;
    if (!engine.initialize()) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.registerDOM(doc);

    engine.evaluate(R"(
        console.log("document.body exists: " + (document.body !== null));
        console.log("document.documentElement tagName: " + document.documentElement.tagName);
        console.log("document.title: " + document.title);

        document.title = "New Title";
        console.log("document.title after set: " + document.title);
    )", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_element_navigation() {
    std::cout << "=== Testing Element Navigation ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    auto div1 = doc->createElement("div");
    div1->setId("first");
    body->appendChild(div1);

    auto span1 = doc->createElement("span");
    span1->setId("span1");
    div1->appendChild(span1);

    auto span2 = doc->createElement("span");
    span2->setId("span2");
    div1->appendChild(span2);

    auto div2 = doc->createElement("div");
    div2->setId("second");
    body->appendChild(div2);

    ScriptEngine engine;
    if (!engine.initialize()) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.registerDOM(doc);

    engine.evaluate(R"(
        var first = document.getElementById("first");
        var span1 = document.getElementById("span1");
        var span2 = document.getElementById("span2");
        var body = document.getElementById("first").parentElement;

        console.log("Testing children count: " + first.childElementCount);
        console.log("Testing parentElement: " + (body !== null));

        var children = first.children;
        console.log("Testing children.length: " + children.length);

        var firstChild = first.firstElementChild;
        console.log("Testing firstElementChild: " + firstChild.id);

        var lastChild = first.lastElementChild;
        console.log("Testing lastElementChild: " + lastChild.id);

        console.log("Testing nextElementSibling: " + span1.nextElementSibling.id);
        console.log("Testing previousElementSibling: " + span2.previousElementSibling.id);
    )", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_query_selector_all() {
    std::cout << "=== Testing querySelectorAll ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    for (int i = 0; i < 3; i++) {
        auto div = doc->createElement("div");
        div->setClassName("item");
        body->appendChild(div);
    }

    auto span = doc->createElement("span");
    span->setClassName("item");
    body->appendChild(span);

    ScriptEngine engine;
    if (!engine.initialize()) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.registerDOM(doc);

    engine.evaluate(R"(
        var items = document.querySelectorAll(".item");
        console.log("querySelectorAll('.item') count: " + items.length);

        var divs = document.querySelectorAll("div");
        console.log("querySelectorAll('div') count: " + divs.length);

        var first = document.querySelector(".item");
        console.log("querySelector('.item') tagName: " + first.tagName);
    )", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_event_system() {
    std::cout << "=== Testing Event System ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    auto div = doc->createElement("div");
    div->setId("test-div");
    body->appendChild(div);

    ScriptEngine engine;
    if (!engine.initialize()) {
        std::cout << "  Test FAILED - engine init failed!" << std::endl;
        return 1;
    }

    engine.registerDOM(doc);

    engine.evaluate(R"(
        var div = document.getElementById("test-div");
        var eventFired = false;

        div.addEventListener("click", function(e) {
            console.log("Event type: " + e.type);
            console.log("Event bubbles: " + e.bubbles);
            console.log("Event cancelable: " + e.cancelable);
            console.log("Event has timeStamp: " + (e.timeStamp !== undefined));
            eventFired = true;
        });

        div.dispatchEvent("click");
        console.log("Event fired: " + eventFired);
    )", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int test_create_and_manipulate_elements() {
    std::cout << "=== Testing Create and Manipulate Elements ===" << std::endl;

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
        var newDiv = document.createElement("div");
        newDiv.id = "created-div";
        newDiv.className = "created dynamic";
        newDiv.setAttribute("data-test", "value");

        console.log("Created div id: " + newDiv.id);
        console.log("Created div className: " + newDiv.className);
        console.log("Created div data-test: " + newDiv.getAttribute("data-test"));
        console.log("Created div hasAttribute('id'): " + newDiv.hasAttribute("id"));

        newDiv.removeAttribute("data-test");
        console.log("After removeAttribute: " + newDiv.getAttribute("data-test"));

        var text = document.createElement("span");
        text.textContent = "Hello World";
        newDiv.appendChild(text);
        console.log("After appendChild, textContent: " + newDiv.textContent);

        var body = document.body;
        body.appendChild(newDiv);
        console.log("After appending to body, div exists: " + (document.getElementById("created-div") !== null));
    )", "test.js");

    std::cout << "  Test PASSED" << std::endl;
    return 0;
}

int main() {
    std::cout << "\n=== DOM Binding Enhanced Tests ===" << std::endl;

    int failures = 0;

    failures += test_basic_dom_binding();
    failures += test_element_properties();
    failures += test_classlist_api();
    failures += test_document_properties();
    failures += test_element_navigation();
    failures += test_query_selector_all();
    failures += test_event_system();
    failures += test_create_and_manipulate_elements();

    if (failures == 0) {
        std::cout << "\n All DOM Binding tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n " << failures << " DOM Binding tests failed!" << std::endl;
        return 1;
    }
}
