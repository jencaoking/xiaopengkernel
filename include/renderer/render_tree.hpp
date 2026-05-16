#pragma once

#include "../dom/html_types.hpp"
#include "../layout/layout_box.hpp"
#include "../css/computed_style.hpp"
#include "../css/css_parser.hpp"
#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>
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

    // Create a RenderObject with computed style
    static RenderObjectPtr create(dom::NodePtr domNode, const css::ComputedStyle& style) {
        auto obj = std::shared_ptr<RenderObject>(new RenderObject(domNode));
        obj->style_ = style;
        return obj;
    }

    ~RenderObject() = default;

    // DOM node association
    dom::NodePtr domNode() const { return domNode_.lock(); }
    
    // Computed style for this render object
    const css::ComputedStyle& style() const { return style_; }
    void setStyle(const css::ComputedStyle& style) { 
        style_ = style; 
        markDirty(DirtyState::NeedsLayout);
    }
    
    // Layout box (computed during layout)
    layout::LayoutBoxPtr layoutBox() const { return layoutBox_; }
    void setLayoutBox(layout::LayoutBoxPtr box) { layoutBox_ = box; }

    // Parent/Children in render tree
    RenderObjectPtr parent() const { return parent_.lock(); }
    const std::vector<RenderObjectPtr>& children() const { return children_; }
    
    // Sibling traversal
    RenderObjectPtr firstChild() const { return children_.empty() ? nullptr : children_.front(); }
    RenderObjectPtr lastChild() const { return children_.empty() ? nullptr : children_.back(); }
    RenderObjectPtr nextSibling() const {
        if (auto p = parent_.lock()) {
            auto it = std::find(p->children_.begin(), p->children_.end(), 
                                  std::const_pointer_cast<RenderObject>(shared_from_this()));
            if (it != p->children_.end() && std::next(it) != p->children_.end()) {
                return *std::next(it);
            }
        }
        return nullptr;
    }
    RenderObjectPtr previousSibling() const {
        if (auto p = parent_.lock()) {
            auto it = std::find(p->children_.begin(), p->children_.end(), 
                                  std::const_pointer_cast<RenderObject>(shared_from_this()));
            if (it != p->children_.begin()) {
                return *std::prev(it);
            }
        }
        return nullptr;
    }

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
                p->markDirty(DirtyState::NeedsPaint);
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
    css::ComputedStyle style_;
    layout::LayoutBoxPtr layoutBox_;
    
    WeakRenderObjectPtr parent_;
    std::vector<RenderObjectPtr> children_;
    
    DirtyState dirtyState_ = DirtyState::Clean;
    BoundingBox boundingBox_;
    BoundingBox damageRect_;
};

// Forward declaration for RenderTreeBuilder
class RenderTreeBuilder;

// RenderTree manages the entire render tree and coordinates updates
class RenderTree {
public:
    RenderTree() = default;
    ~RenderTree() = default;

    // Build render tree from DOM with stylesheet
    void build(dom::NodePtr domRoot, const css::StyleSheet& stylesheet);

    // Get root of render tree
    RenderObjectPtr root() const { return root_; }

    // Set root of layout box (for backwards compatibility)
    layout::LayoutBoxPtr layoutRoot() const { 
        return root_ ? root_->layoutBox() : nullptr; 
    }

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

    // Find render object for a DOM node (fast lookup using map)
    RenderObjectPtr findRenderObject(dom::NodePtr domNode) {
        if (!domNode) return nullptr;
        auto it = domToRenderMap_.find(domNode);
        return it != domToRenderMap_.end() ? it->second : nullptr;
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
            }
        }
    }

    // Update bounding boxes from layout boxes
    void syncBoundingBoxesFromLayout() {
        if (root_) {
            syncBoundingBoxesRecursive(root_);
        }
    }

private:
    RenderObjectPtr root_;
    std::unordered_map<dom::NodePtr, RenderObjectPtr> domToRenderMap_;

    friend class RenderTreeBuilder;

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

    void syncBoundingBoxesRecursive(RenderObjectPtr obj) {
        if (auto box = obj->layoutBox()) {
            auto& dims = box->dimensions();
            RenderObject::BoundingBox bbox;
            bbox.x = static_cast<int>(dims.content.x);
            bbox.y = static_cast<int>(dims.content.y);
            bbox.width = static_cast<int>(dims.content.width);
            bbox.height = static_cast<int>(dims.content.height);
            obj->setBoundingBox(bbox);
        }
        for (const auto& child : obj->children()) {
            syncBoundingBoxesRecursive(child);
        }
    }
};

} // namespace renderer
} // namespace xiaopeng
