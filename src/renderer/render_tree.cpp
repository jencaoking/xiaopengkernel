#include "../../include/renderer/render_tree.hpp"
#include "../../include/renderer/render_tree_builder.hpp"

namespace xiaopeng {
namespace renderer {

void RenderTree::build(dom::NodePtr domRoot, const css::StyleSheet& stylesheet) {
    RenderTreeBuilder builder(stylesheet);
    builder.build(domRoot, *this);
}

} // namespace renderer
} // namespace xiaopeng
