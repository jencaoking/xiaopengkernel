#include "test_framework.hpp"
#include <engine/resource_tree.hpp>

using namespace xiaopeng::engine;
using namespace xiaopeng::loader;

// ─── ResourceNode Tests ─────────────────────────────────────────────────────

TEST(ResourceNode_DefaultState) {
  ResourceNode node;
  EXPECT_TRUE(node.isPending());
  EXPECT_FALSE(node.isLoaded());
  EXPECT_FALSE(node.isFailed());
  EXPECT_FALSE(node.isTerminal());
  EXPECT_FALSE(node.executed);
}

TEST(ResourceNode_LoadedIsTerminal) {
  ResourceNode node;
  node.state = LoadState::Loaded;
  EXPECT_TRUE(node.isLoaded());
  EXPECT_TRUE(node.isTerminal());
}

TEST(ResourceNode_FailedIsTerminal) {
  ResourceNode node;
  node.state = LoadState::Failed;
  EXPECT_TRUE(node.isFailed());
  EXPECT_TRUE(node.isTerminal());
}

// ─── ResourceTree: Discovery Tests ──────────────────────────────────────────

TEST(ResourceTree_InitiallyEmpty) {
  ResourceTree tree;
  EXPECT_EQ(tree.size(), 0);
  EXPECT_TRUE(tree.allLoaded());     // Vacuously true
  EXPECT_TRUE(tree.criticalReady()); // No CSS = critical ready
}

TEST(ResourceTree_AddStylesheet) {
  ResourceTree tree;
  tree.addStylesheet("style.css");
  EXPECT_EQ(tree.size(), 1);
  auto sheets = tree.stylesheets();
  EXPECT_EQ(sheets.size(), 1);
  EXPECT_STREQ(sheets[0]->url.c_str(), "style.css");
  EXPECT_TRUE(sheets[0]->isPending());
}

TEST(ResourceTree_AddScript_Classic) {
  ResourceTree tree;
  tree.addScript("app.js", ScriptMode::Classic);
  EXPECT_EQ(tree.size(), 1);
  auto syncs = tree.syncScripts();
  EXPECT_EQ(syncs.size(), 1);
  EXPECT_TRUE(syncs[0]->isPending());
  EXPECT_EQ(syncs[0]->scriptMode, ScriptMode::Classic);
}

TEST(ResourceTree_AddScript_Async) {
  ResourceTree tree;
  tree.addScript("analytics.js", ScriptMode::Async);
  auto asyncs = tree.asyncScripts();
  EXPECT_EQ(asyncs.size(), 1);
  EXPECT_EQ(asyncs[0]->scriptMode, ScriptMode::Async);
  EXPECT_TRUE(asyncs[0]->isPending());
}

TEST(ResourceTree_AddScript_Defer) {
  ResourceTree tree;
  tree.addScript("main.js", ScriptMode::Defer);
  auto defers = tree.deferScripts();
  EXPECT_EQ(defers.size(), 1);
  EXPECT_EQ(defers[0]->scriptMode, ScriptMode::Defer);
  EXPECT_TRUE(defers[0]->isPending());
}

TEST(ResourceTree_InlineScript_ImmediatelyLoaded) {
  ResourceTree tree;
  tree.addScript("", ScriptMode::Classic, "console.log('hello')");
  EXPECT_EQ(tree.size(), 1);
  auto syncs = tree.syncScripts();
  EXPECT_EQ(syncs.size(), 1);
  EXPECT_TRUE(syncs[0]->isLoaded());
  EXPECT_STREQ(syncs[0]->content.c_str(), "console.log('hello')");
}

TEST(ResourceTree_AddImage) {
  ResourceTree tree;
  tree.addImage("logo.png");
  EXPECT_EQ(tree.size(), 1);
  auto imgs = tree.images();
  EXPECT_EQ(imgs.size(), 1);
  EXPECT_STREQ(imgs[0]->url.c_str(), "logo.png");
  EXPECT_TRUE(imgs[0]->isPending());
}

TEST(ResourceTree_AddMultipleResources) {
  ResourceTree tree;
  tree.addStylesheet("a.css");
  tree.addStylesheet("b.css");
  tree.addScript("app.js", ScriptMode::Classic);
  tree.addScript("boot.js", ScriptMode::Defer);
  tree.addScript("tracking.js", ScriptMode::Async);
  tree.addScript("", ScriptMode::Classic, "var x = 1;");
  tree.addImage("hero.jpg");

  EXPECT_EQ(tree.size(), 7);
  EXPECT_EQ(tree.stylesheets().size(), 2);
  EXPECT_EQ(tree.syncScripts().size(), 2); // Classic + Inline
  EXPECT_EQ(tree.deferScripts().size(), 1);
  EXPECT_EQ(tree.asyncScripts().size(), 1);
  EXPECT_EQ(tree.images().size(), 1);
}

// ─── ResourceTree: State Query Tests ────────────────────────────────────────

TEST(ResourceTree_AllLoaded_FalseWithPending) {
  ResourceTree tree;
  tree.addStylesheet("style.css");
  EXPECT_FALSE(tree.allLoaded());
}

TEST(ResourceTree_AllLoaded_TrueWithInlineOnly) {
  ResourceTree tree;
  tree.addScript("", ScriptMode::Classic, "var x = 1;");
  EXPECT_TRUE(tree.allLoaded());
}

TEST(ResourceTree_CriticalReady_FalseWithPendingCSS) {
  ResourceTree tree;
  tree.addStylesheet("style.css");
  tree.addScript("app.js", ScriptMode::Classic);
  EXPECT_FALSE(tree.criticalReady());
}

TEST(ResourceTree_CriticalReady_TrueWithNonCSSPending) {
  ResourceTree tree;
  tree.addScript("app.js", ScriptMode::Classic);
  tree.addImage("hero.png");
  // No CSS at all => critical ready
  EXPECT_TRUE(tree.criticalReady());
}

// ─── ResourceTree: Script Mode Classification Tests ─────────────────────────

TEST(ResourceTree_ScriptModeClassification) {
  ResourceTree tree;
  tree.addScript("a.js", ScriptMode::Classic);
  tree.addScript("b.js", ScriptMode::Defer);
  tree.addScript("c.js", ScriptMode::Async);
  tree.addScript("d.js", ScriptMode::Classic);

  EXPECT_EQ(tree.syncScripts().size(), 2);
  EXPECT_EQ(tree.deferScripts().size(), 1);
  EXPECT_EQ(tree.asyncScripts().size(), 1);

  // Verify correct URL assignment
  auto syncs = tree.syncScripts();
  EXPECT_STREQ(syncs[0]->url.c_str(), "a.js");
  EXPECT_STREQ(syncs[1]->url.c_str(), "d.js");

  auto defers = tree.deferScripts();
  EXPECT_STREQ(defers[0]->url.c_str(), "b.js");

  auto asyncs = tree.asyncScripts();
  EXPECT_STREQ(asyncs[0]->url.c_str(), "c.js");
}

// ─── ResourceTree: FetchAll Without Network ─────────────────────────────────

TEST(ResourceTree_FetchAllWithoutNetwork_NoOp) {
  ResourceTree tree;
  // All inline scripts, nothing to fetch
  tree.addScript("", ScriptMode::Classic, "var x = 1;");
  tree.addScript("", ScriptMode::Defer, "var y = 2;");

  tree.fetchAll(""); // Should not crash

  EXPECT_TRUE(tree.allLoaded());
  EXPECT_EQ(tree.syncScripts().size(), 1);
  EXPECT_EQ(tree.deferScripts().size(), 1);
}

// ─── ResourceTree: Document Order Preservation ──────────────────────────────

TEST(ResourceTree_PreservesDocumentOrder) {
  ResourceTree tree;
  tree.addStylesheet("first.css");
  tree.addScript("second.js", ScriptMode::Classic);
  tree.addStylesheet("third.css");
  tree.addImage("fourth.png");

  auto &nodes = tree.nodes();
  EXPECT_EQ(nodes.size(), 4);
  EXPECT_EQ(nodes[0].type, ResourceType::Css);
  EXPECT_STREQ(nodes[0].url.c_str(), "first.css");
  EXPECT_EQ(nodes[1].type, ResourceType::JavaScript);
  EXPECT_STREQ(nodes[1].url.c_str(), "second.js");
  EXPECT_EQ(nodes[2].type, ResourceType::Css);
  EXPECT_STREQ(nodes[2].url.c_str(), "third.css");
  EXPECT_EQ(nodes[3].type, ResourceType::Image);
  EXPECT_STREQ(nodes[3].url.c_str(), "fourth.png");
}

// ─── ResourceTree: Executed Flag ────────────────────────────────────────────

TEST(ResourceTree_ExecutedFlagDefaultFalse) {
  ResourceTree tree;
  tree.addScript("app.js", ScriptMode::Async);
  auto asyncs = tree.asyncScripts();
  EXPECT_FALSE(asyncs[0]->executed);
}

TEST(ResourceTree_ExecutedFlagMutable) {
  ResourceTree tree;
  tree.addScript("app.js", ScriptMode::Async);
  auto asyncs = tree.asyncScripts();
  asyncs[0]->executed = true;

  // Re-query should reflect the change
  auto asyncs2 = tree.asyncScripts();
  EXPECT_TRUE(asyncs2[0]->executed);
}

int main() { return xiaopeng::test::runTests(); }
