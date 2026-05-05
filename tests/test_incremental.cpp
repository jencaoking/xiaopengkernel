#include <iostream>
#include <memory>
#include <cassert>

#include "../include/dom/dom.hpp"
#include "../include/renderer/render_tree.hpp"
#include "../include/renderer/incremental_manager.hpp"
#include "../include/renderer/bitmap_canvas.hpp"

using namespace xiaopeng;
using namespace xiaopeng::dom;
using namespace xiaopeng::renderer;

// Test render tree construction
int test_render_tree_construction() {
    std::cout << "=== Testing Render Tree Construction ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    auto div = doc->createElement("div");
    div->setId("test-div");
    body->appendChild(div);

    RenderTree renderTree;
    renderTree.build(doc);

    auto root = renderTree.root();
    bool success = root != nullptr;
    
    if (success) {
        std::cout << "  Render tree built successfully" << std::endl;
        std::cout << "  Root has " << root->children().size() << " children" << std::endl;
    }

    std::cout << "  Test " << (success ? "PASSED" : "FAILED") << std::endl;
    return success ? 0 : 1;
}

// Test dirty flag propagation
int test_dirty_flag_propagation() {
    std::cout << "=== Testing Dirty Flag Propagation ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    auto div = doc->createElement("div");
    body->appendChild(div);

    RenderTree renderTree;
    renderTree.build(doc);

    auto root = renderTree.root();
    bool success = true;

    // Mark a child as dirty
    if (root && !root->children().empty()) {
        auto child = root->children()[0];
        child->markDirty(RenderObject::DirtyState::NeedsLayout);
        
        // Parent should also be dirty
        success = root->isDirty();
        
        if (success) {
            std::cout << "  Dirty flag propagated correctly" << std::endl;
        }
    } else {
        success = false;
    }

    std::cout << "  Test " << (success ? "PASSED" : "FAILED") << std::endl;
    return success ? 0 : 1;
}

// Test incremental layout
int test_incremental_layout() {
    std::cout << "=== Testing Incremental Layout ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    auto div = doc->createElement("div");
    body->appendChild(div);

    RenderTree renderTree;
    renderTree.build(doc);

    IncrementalUpdateManager manager;
    
    // Perform incremental layout
    manager.performFullLayout(renderTree, 800, 600);

    // After layout, tree should be clean
    auto root = renderTree.root();
    bool success = root && !root->isDirty();

    std::cout << "  Test " << (success ? "PASSED" : "FAILED") << std::endl;
    return success ? 0 : 1;
}

// Test finding render object by DOM node
int test_find_render_object() {
    std::cout << "=== Testing Find Render Object ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    auto div = doc->createElement("div");
    div->setId("target");
    body->appendChild(div);

    RenderTree renderTree;
    renderTree.build(doc);

    // Find the div's render object
    auto renderObj = renderTree.findRenderObject(div);
    bool success = renderObj != nullptr;

    if (success) {
        std::cout << "  Found render object for DOM node" << std::endl;
    }

    std::cout << "  Test " << (success ? "PASSED" : "FAILED") << std::endl;
    return success ? 0 : 1;
}

// Test bounding box calculation
int test_bounding_box() {
    std::cout << "=== Testing Bounding Box ===" << std::endl;

    RenderObject::BoundingBox box1{0, 0, 100, 100};
    RenderObject::BoundingBox box2{50, 50, 100, 100};

    bool success = true;

    // Test intersection
    if (!box1.intersects(box2)) {
        success = false;
    }

    // Test union
    auto united = box1.united(box2);
    if (united.x != 0 || united.y != 0 || 
        united.width != 150 || united.height != 150) {
        success = false;
    }

    std::cout << "  Test " << (success ? "PASSED" : "FAILED") << std::endl;
    return success ? 0 : 1;
}

// Test DOM change notification
int test_dom_change_notification() {
    std::cout << "=== Testing DOM Change Notification ===" << std::endl;

    auto doc = std::make_shared<Document>();
    auto body = doc->createElement("body");
    doc->appendChild(body);

    RenderTree renderTree;
    renderTree.build(doc);

    // Simulate DOM change
    renderTree.onDOMChanged(body, dom::DirtyFlag::NeedsLayout);

    auto root = renderTree.root();
    bool success = root && root->isDirty();

    std::cout << "  Test " << (success ? "PASSED" : "FAILED") << std::endl;
    return success ? 0 : 1;
}

int main() {
    std::cout << "\n=== Incremental Update Tests ===" << std::endl;

    int failures = 0;
    
    failures += test_render_tree_construction();
    failures += test_dirty_flag_propagation();
    failures += test_incremental_layout();
    failures += test_find_render_object();
    failures += test_bounding_box();
    failures += test_dom_change_notification();

    if (failures == 0) {
        std::cout << "\n✅ All Incremental Update Tests Passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ " << failures << " Incremental Update Tests Failed!" << std::endl;
        return 1;
    }
}
