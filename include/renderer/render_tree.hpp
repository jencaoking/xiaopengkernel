#pragma once

#include "../dom/html_types.hpp"
#include "../layout/layout_box.hpp"
#include "../css/computed_style.hpp"
#include <memory>
#include <vector>
#include <unordered_set>
#include <functional>

namespace xiaopeng {
namespace renderer {

// Forward declarations
class RenderObject;
using RenderObjectPtr = std::shared_ptr<RenderObject>;
using WeakRenderObjectPtr = std::weak_ptr<RenderObject>;

// RenderObject represents a renderable node in the render tree
// It mirrors the DOM tree but is optimized for layout and painting
class RenderObject : public std::enable_shared_from_this<RenderObject> {
public:
    // Dirty state for incremental updates
    enum class DirtyState : uint8_t {
        Clean = 0,
        NeedsPaint = 1,
        NeedsLayout = 2,
        NeedsFullLayout = 3
    };

    // Create a RenderObject for a DOM node
    static RenderObjectPtr create(dom::NodePtr domNode) {
        return std::shared_ptr<RenderObject>(new RenderObject(domNode));
    }

    ~RenderObject() = default;

    // DOM node association
    dom::NodePtr domNode() const { return domNode_.lock(); }
    
    // Layout box (computed during layout)
    layout::LayoutBoxPtr layoutBox() const { return layoutBox_; }
    void setLayoutBox(layout::LayoutBoxPtr box) { layoutBox_ = box; }

    // Parent/Children in render tree
    RenderObjectPtr parent() const { return parent_.lock(); }
    const std::vector<RenderObjectPtr>& children() const { return children_; }

    void appendChild(RenderObjectPtr child) {
        if (!child) return;
        
        if (auto oldParent = child->parent_.lock()) {
            oldParent->removeChild(child);
        }
        
        child->parent_ = weak_from_this();
        children_.push_back(child);
        markDirty(DirtyState::NeedsLayout);
    }

    void removeChild(RenderObjectPtr child) {
        if (!child) return;
        
        auto it = std::find(children_.begin(), children_.end(), child);
        if (it != children_.end()) {
            children_.erase(it);
            child->parent_.reset();
            markDirty(DirtyState::NeedsLayout);
        }
    }

    // Dirty state management
    DirtyState dirtyState() const { return dirtyState_; }
    
    void markDirty(DirtyState state) {
        if (static_cast<uint8_t>(state) > static_cast<uint8_t>(dirtyState_)) {
            dirtyState_ = state;
            // Propagate to parent
            if (auto p = parent_.lock()) {
                p->markDirty(state);
            }
        }
    }

    void clearDirty() { 
        dirtyState_ = DirtyState::Clean; 
    }

    bool isDirty() const { return dirtyState_ != DirtyState::Clean; }
    bool needsLayout() const { 
        return dirtyState_ == DirtyState::NeedsLayout || 
               dirtyState_ == DirtyState::NeedsFullLayout; 
    }
    bool needsPaint() const { return dirtyState_ >= DirtyState::NeedsPaint; }

    // Bounding box for damage tracking
    struct BoundingBox {
        int x = 0, y = 0;
        int width = 0, height = 0;
        
        bool intersects(const BoundingBox& other) const {
            return !(x + width <= other.x || other.x + other.width <= x ||
                     y + height <= other.y || other.y + other.height <= y);
        }
        
        BoundingBox united(const BoundingBox& other) const {
            BoundingBox result;
            result.x = std::min(x, other.x);
            result.y = std::min(y, other.y);
            result.width = std::max(x + width, other.x + other.width) - result.x;
            result.height = std::max(y + height, other.y + other.height) - result.y;
            return result;
        }
    };

    BoundingBox boundingBox() const { return boundingBox_; }
    void setBoundingBox(const BoundingBox& box) { boundingBox_ = box; }

    // Damage rect for partial repaint
    BoundingBox damageRect() const { return damageRect_; }
    void addDamageRect(const BoundingBox& rect) {
        damageRect_ = damageRect_.width == 0 ? rect : damageRect_.united(rect);
    }
    void clearDamageRect() { damageRect_ = BoundingBox(); }

private:
    explicit RenderObject(dom::NodePtr domNode) 
        : domNode_(domNode), dirtyState_(DirtyState::NeedsLayout) {}

    dom::WeakNodePtr domNode_;
    layout::LayoutBoxPtr layoutBox_;
    
    WeakRenderObjectPtr parent_;
    std::vector<RenderObjectPtr> children_;
    
    DirtyState dirtyState_ = DirtyState::Clean;
    BoundingBox boundingBox_;
    BoundingBox damageRect_;
};

// RenderTree manages the entire render tree and coordinates updates
class RenderTree {
public:
    RenderTree() = default;
    ~RenderTree() = default;

    // Build render tree from DOM
    void build(dom::NodePtr domRoot) {
        root_ = buildRenderTree(domRoot, nullptr);
    }

    // Get root of render tree
    RenderObjectPtr root() const { return root_; }

    // Collect all dirty objects that need layout
    std::vector<RenderObjectPtr> collectDirtyLayoutObjects() {
        std::vector<RenderObjectPtr> result;
        if (root_) {
            collectDirtyLayoutObjectsRecursive(root_, result);
        }
        return result;
    }

    // Collect all dirty objects that need paint
    std::vector<RenderObjectPtr> collectDirtyPaintObjects() {
        std::vector<RenderObjectPtr> result;
        if (root_) {
            collectDirtyPaintObjectsRecursive(root_, result);
        }
        return result;
    }

    // Mark entire tree as needing layout (for resize, etc.)
    void markAllDirty() {
        if (root_) {
            root_->markDirty(RenderObject::DirtyState::NeedsFullLayout);
        }
    }

    // Clear all dirty flags after update
    void clearAllDirty() {
        if (root_) {
            clearAllDirtyRecursive(root_);
        }
    }

    // Find render object for a DOM node
    RenderObjectPtr findRenderObject(dom::NodePtr domNode) {
        if (!root_ || !domNode) return nullptr;
        return findRenderObjectRecursive(root_, domNode);
    }

    // Update render tree when DOM changes
    void onDOMChanged(dom::NodePtr changedNode, dom::DirtyFlag changeType) {
        auto renderObj = findRenderObject(changedNode);
        if (renderObj) {
            switch (changeType) {
                case dom::DirtyFlag::NeedsLayout:
                    renderObj->markDirty(RenderObject::DirtyState::NeedsLayout);
                    break;
                case dom::DirtyFlag::NeedsPaint:
                    renderObj->markDirty(RenderObject::DirtyState::NeedsPaint);
                    break;
                default:
                    break;
            }
        }
    }

private:
    RenderObjectPtr root_;

    // Recursively build render tree from DOM
    RenderObjectPtr buildRenderTree(dom::NodePtr domNode, RenderObjectPtr parent) {
        if (!domNode) return nullptr;

        // Skip non-element nodes (text, comment, etc. can be handled differently)
        if (domNode->nodeType() != dom::NodeType::Element && 
            domNode->nodeType() != dom::NodeType::Document) {
            // For text nodes, we might want to create a text render object
            // For simplicity, we'll skip them for now
            return nullptr;
        }

        auto renderObj = RenderObject::create(domNode);
        if (parent) {
            parent->appendChild(renderObj);
        }

        // Recursively build children
        for (const auto& domChild : domNode->childNodes()) {
            buildRenderTree(domChild, renderObj);
        }

        return renderObj;
    }

    void collectDirtyLayoutObjectsRecursive(RenderObjectPtr obj, 
                                            std::vector<RenderObjectPtr>& result) {
        if (obj->needsLayout()) {
            result.push_back(obj);
        }
        for (const auto& child : obj->children()) {
            collectDirtyLayoutObjectsRecursive(child, result);
        }
    }

    void collectDirtyPaintObjectsRecursive(RenderObjectPtr obj, 
                                           std::vector<RenderObjectPtr>& result) {
        if (obj->needsPaint()) {
            result.push_back(obj);
        }
        for (const auto& child : obj->children()) {
            collectDirtyPaintObjectsRecursive(child, result);
        }
    }

    void clearAllDirtyRecursive(RenderObjectPtr obj) {
        obj->clearDirty();
        for (const auto& child : obj->children()) {
            clearAllDirtyRecursive(child);
        }
    }

    RenderObjectPtr findRenderObjectRecursive(RenderObjectPtr obj, dom::NodePtr domNode) {
        if (obj->domNode() == domNode) return obj;
        
        for (const auto& child : obj->children()) {
            auto found = findRenderObjectRecursive(child, domNode);
            if (found) return found;
        }
        return nullptr;
    }
};

} // namespace renderer
} // namespace xiaopeng
