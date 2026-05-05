#pragma once

#include "render_tree.hpp"
#include "canvas.hpp"
#include "../layout/layout_algorithm.hpp"
#include "../layout/flexbox_algorithm.hpp"
#include <iostream>
#include <chrono>

namespace xiaopeng {
namespace renderer {

// IncrementalUpdateManager handles efficient layout and paint updates
// It only processes the parts of the tree that have changed
class IncrementalUpdateManager {
public:
    IncrementalUpdateManager() = default;
    ~IncrementalUpdateManager() = default;

    // Perform incremental layout on dirty objects
    void performIncrementalLayout(RenderTree& renderTree, 
                                   int viewportWidth, int viewportHeight) {
        auto dirtyObjects = renderTree.collectDirtyLayoutObjects();
        
        if (dirtyObjects.empty()) {
            return; // Nothing to update
        }

        std::cout << "[IncrementalUpdate] Performing incremental layout on " 
                  << dirtyObjects.size() << " objects" << std::endl;

        for (auto& obj : dirtyObjects) {
            layoutSingleObject(obj, viewportWidth, viewportHeight);
        }
    }

    // Perform incremental paint on dirty objects
    void performIncrementalPaint(RenderTree& renderTree, Canvas& canvas) {
        auto dirtyObjects = renderTree.collectDirtyPaintObjects();
        
        if (dirtyObjects.empty()) {
            return; // Nothing to paint
        }

        std::cout << "[IncrementalUpdate] Performing incremental paint on " 
                  << dirtyObjects.size() << " objects" << std::endl;

        // Calculate damage region
        RenderObject::BoundingBox damageRegion;
        for (auto& obj : dirtyObjects) {
            damageRegion = damageRegion.width == 0 
                ? obj->boundingBox() 
                : damageRegion.united(obj->boundingBox());
        }

        // Clear the damage region
        if (damageRegion.width > 0 && damageRegion.height > 0) {
            // For now, we'll just clear the entire canvas
            // A more sophisticated implementation would only clear the damage region
            canvas.clear(Color::White());
        }

        // Paint only dirty objects
        for (auto& obj : dirtyObjects) {
            paintSingleObject(obj, canvas);
        }

        // Clear dirty flags after painting
        renderTree.clearAllDirty();
    }

    // Full layout (used when entire tree needs re-layout)
    void performFullLayout(RenderTree& renderTree, 
                           int viewportWidth, int viewportHeight) {
        std::cout << "[IncrementalUpdate] Performing full layout" << std::endl;
        
        auto root = renderTree.root();
        if (root) {
            layoutSubtree(root, viewportWidth, viewportHeight);
        }
        
        renderTree.clearAllDirty();
    }

    // Full paint (used when entire canvas needs repaint)
    void performFullPaint(RenderTree& renderTree, Canvas& canvas) {
        std::cout << "[IncrementalUpdate] Performing full paint" << std::endl;
        
        canvas.clear(Color::White());
        
        auto root = renderTree.root();
        if (root) {
            paintSubtree(root, canvas, 0, 0);
        }
        
        renderTree.clearAllDirty();
    }

private:
    // Layout a single render object
    void layoutSingleObject(RenderObjectPtr obj, int viewportWidth, int viewportHeight) {
        if (!obj) return;

        auto domNode = obj->domNode();
        if (!domNode) return;

        // In a real implementation, we would:
        // 1. Get or create a LayoutBox with proper style
        // 2. Run the layout algorithm
        // 3. Store the resulting dimensions
        
        // For now, just mark it as processed
        obj->clearDirty();
    }

    // Layout a subtree starting from the given object
    void layoutSubtree(RenderObjectPtr obj, int viewportWidth, int viewportHeight) {
        layoutSingleObject(obj, viewportWidth, viewportHeight);
        
        for (auto& child : obj->children()) {
            layoutSubtree(child, viewportWidth, viewportHeight);
        }
    }

    // Paint a single render object
    void paintSingleObject(RenderObjectPtr obj, Canvas& canvas) {
        if (!obj) return;

        auto layoutBox = obj->layoutBox();
        if (!layoutBox) {
            // No layout box, skip painting
            return;
        }

        // Get bounding box from layout
        const auto& dims = layoutBox->dimensions();
        RenderObject::BoundingBox bbox;
        bbox.x = static_cast<int>(dims.content.x);
        bbox.y = static_cast<int>(dims.content.y);
        bbox.width = static_cast<int>(dims.content.width);
        bbox.height = static_cast<int>(dims.content.height);
        
        obj->setBoundingBox(bbox);

        // Draw background if present
        auto style = layoutBox->style();
        if (style.backgroundColor.a > 0) {
            Color color{style.backgroundColor.r, style.backgroundColor.g, 
                       style.backgroundColor.b, style.backgroundColor.a};
            canvas.fillRect(bbox.x, bbox.y, bbox.width, bbox.height, color);
        }
    }

    // Paint a subtree starting from the given object
    void paintSubtree(RenderObjectPtr obj, Canvas& canvas, int parentX, int parentY) {
        paintSingleObject(obj, canvas);
        
        auto bbox = obj->boundingBox();
        for (auto& child : obj->children()) {
            paintSubtree(child, canvas, bbox.x, bbox.y);
        }
    }
};

// Performance statistics for incremental updates
struct UpdateStats {
    int layoutCount = 0;
    int paintCount = 0;
    std::chrono::microseconds layoutTime{0};
    std::chrono::microseconds paintTime{0};
    
    void reset() {
        layoutCount = 0;
        paintCount = 0;
        layoutTime = std::chrono::microseconds{0};
        paintTime = std::chrono::microseconds{0};
    }
    
    void print() const {
        std::cout << "[UpdateStats] Layouts: " << layoutCount 
                  << " (" << layoutTime.count() << "us), "
                  << "Paints: " << paintCount 
                  << " (" << paintTime.count() << "us)" << std::endl;
    }
};

} // namespace renderer
} // namespace xiaopeng
