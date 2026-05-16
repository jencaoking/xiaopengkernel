#pragma once

#include "render_tree.hpp"
#include "../css/style_resolver.hpp"
#include "../dom/dom.hpp"
#include <memory>

namespace xiaopeng {
namespace renderer {

class RenderTreeBuilder {
public:
    RenderTreeBuilder(const css::StyleSheet& stylesheet) 
        : stylesheet_(stylesheet), resolver_() {}

    // Build render tree from DOM
    RenderObjectPtr build(dom::NodePtr domRoot, RenderTree& tree);

private:
    const css::StyleSheet& stylesheet_;
    css::StyleResolver resolver_;

    // Recursive building helper
    RenderObjectPtr buildRecursively(dom::NodePtr domNode, RenderObjectPtr parent, RenderTree& tree);
};

} // namespace renderer
} // namespace xiaopeng
