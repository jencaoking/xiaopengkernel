#include "../../include/renderer/render_tree_builder.hpp"

namespace xiaopeng {
namespace renderer {

RenderObjectPtr RenderTreeBuilder::build(dom::NodePtr domRoot, RenderTree& tree) {
    if (!domRoot) return nullptr;
    
    // Clear any existing tree
    tree.domToRenderMap_.clear();
    
    auto root = buildRecursively(domRoot, nullptr, tree);
    tree.root_ = root;
    return root;
}

RenderObjectPtr RenderTreeBuilder::buildRecursively(dom::NodePtr domNode, RenderObjectPtr parent, RenderTree& tree) {
    if (!domNode) return nullptr;

    // Skip comment nodes
    if (domNode->nodeType() == dom::NodeType::Comment) {
        return nullptr;
    }

    css::ComputedStyle style;
    bool skipNode = false;

    // Compute style for element nodes
    if (domNode->nodeType() == dom::NodeType::Element) {
        auto element = std::dynamic_pointer_cast<dom::Element>(domNode);
        style = resolver_.resolveStyle(element, stylesheet_);
        
        // Skip display: none elements
        if (style.display == css::Display::None) {
            skipNode = true;
        }
    } else if (domNode->nodeType() == dom::NodeType::Document) {
        // Document node doesn't have styling
    }

    if (skipNode) {
        return nullptr;
    }

    // Create render object
    auto renderObj = RenderObject::create(domNode, style);
    
    // Store in map for fast lookup
    tree.domToRenderMap_[domNode] = renderObj;

    if (parent) {
        parent->appendChild(renderObj);
    }

    // Recursively build children
    for (const auto& domChild : domNode->childNodes()) {
        buildRecursively(domChild, renderObj, tree);
    }

    return renderObj;
}

} // namespace renderer
} // namespace xiaopeng
