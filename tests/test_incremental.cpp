#include "test_framework.hpp"

#include "../include/dom/dom.hpp"
#include "../include/renderer/render_tree.hpp"
#include "../include/renderer/incremental_manager.hpp"
#include "../include/renderer/bitmap_canvas.hpp"

using namespace xiaopeng;
using namespace xiaopeng::dom;
using namespace xiaopeng::renderer;

// ── Tests ────────────────────────────────────────────────────

TEST(Incremental_RenderTreeConstruction) {
  auto doc = std::make_shared<Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  auto div = doc->createElement("div");
  div->setId("test-div");
  body->appendChild(div);

  RenderTree renderTree;
  renderTree.build(doc);

  auto root = renderTree.root();
  ASSERT_TRUE(root != nullptr);
  EXPECT_GE(root->children().size(), 0u);
}

TEST(Incremental_DirtyFlagPropagation) {
  auto doc = std::make_shared<Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  auto div = doc->createElement("div");
  body->appendChild(div);

  RenderTree renderTree;
  renderTree.build(doc);

  auto root = renderTree.root();
  ASSERT_TRUE(root != nullptr);
  ASSERT_TRUE(!root->children().empty());

  auto child = root->children()[0];
  child->markDirty(RenderObject::DirtyState::NeedsLayout);

  // Parent should also be dirty
  EXPECT_TRUE(root->isDirty());
}

TEST(Incremental_FullLayout) {
  auto doc = std::make_shared<Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  auto div = doc->createElement("div");
  body->appendChild(div);

  RenderTree renderTree;
  renderTree.build(doc);

  IncrementalUpdateManager manager;
  manager.performFullLayout(renderTree, 800, 600);

  // After layout, tree should be clean
  auto root = renderTree.root();
  ASSERT_TRUE(root != nullptr);
  EXPECT_TRUE(!root->isDirty());
}

TEST(Incremental_FindRenderObject) {
  auto doc = std::make_shared<Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  auto div = doc->createElement("div");
  div->setId("target");
  body->appendChild(div);

  RenderTree renderTree;
  renderTree.build(doc);

  auto renderObj = renderTree.findRenderObject(div);
  EXPECT_TRUE(renderObj != nullptr);
}

TEST(Incremental_BoundingBoxIntersection) {
  RenderObject::BoundingBox box1{0, 0, 100, 100};
  RenderObject::BoundingBox box2{50, 50, 100, 100};
  RenderObject::BoundingBox box3{200, 200, 50, 50};

  EXPECT_TRUE(box1.intersects(box2));
  EXPECT_FALSE(box1.intersects(box3));
}

TEST(Incremental_BoundingBoxUnion) {
  RenderObject::BoundingBox box1{0, 0, 100, 100};
  RenderObject::BoundingBox box2{50, 50, 100, 100};

  auto united = box1.united(box2);
  EXPECT_EQ(united.x, 0);
  EXPECT_EQ(united.y, 0);
  EXPECT_EQ(united.width, 150);
  EXPECT_EQ(united.height, 150);
}

TEST(Incremental_DomChangeNotification) {
  auto doc = std::make_shared<Document>();
  auto body = doc->createElement("body");
  doc->appendChild(body);

  RenderTree renderTree;
  renderTree.build(doc);

  renderTree.onDOMChanged(body, dom::DirtyFlag::NeedsLayout);

  auto root = renderTree.root();
  ASSERT_TRUE(root != nullptr);
  EXPECT_TRUE(root->isDirty());
}

int main() { return xiaopeng::test::runTests(); }
