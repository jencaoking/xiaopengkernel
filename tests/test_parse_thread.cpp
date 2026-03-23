#include "test_framework.hpp"
#include <chrono>
#include <engine/parse_task.hpp>
#include <engine/parse_thread.hpp>
#include <thread>

using namespace xiaopeng::engine;

// ─── ParseTask Tests ────────────────────────────────────────────────────────

TEST(ParseTask_InitiallyNotReady) {
  ParseTask task;
  EXPECT_FALSE(task.isReady());
}

TEST(ParseTask_ReadyAfterSetResult) {
  ParseTask task;
  ParseResult result;
  result.success = true;
  task.setResult(std::move(result));
  EXPECT_TRUE(task.isReady());
}

TEST(ParseTask_GetResultReturnsCorrectData) {
  ParseTask task;
  ParseResult result;
  result.success = true;
  result.baseUrl = "https://example.com/";
  task.setResult(std::move(result));

  auto got = task.getResult();
  EXPECT_TRUE(got.success);
  EXPECT_STREQ(got.baseUrl.c_str(), "https://example.com/");
}

// ─── ParseThread Tests ─────────────────────────────────────────────────────

TEST(ParseThread_StartStop) {
  ParseThread thread;
  thread.start();
  // Should be able to stop cleanly
  thread.stop();
}

TEST(ParseThread_DoubleStartSafe) {
  ParseThread thread;
  thread.start();
  thread.start(); // Should be a no-op
  thread.stop();
}

TEST(ParseThread_ParseSimpleHTML) {
  ParseThread thread;
  thread.start();

  std::string html = "<!DOCTYPE html><html><head><title>Test</title></head>"
                     "<body><p>Hello World</p></body></html>";

  auto task = thread.submit(html, "", false);

  // Wait for completion (with timeout)
  auto start = std::chrono::steady_clock::now();
  while (!task->isReady()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed > std::chrono::seconds(5)) {
      break; // Timeout
    }
  }

  EXPECT_TRUE(task->isReady());

  auto result = task->getResult();
  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.document != nullptr);

  // Verify DOM structure
  auto body = result.document->body();
  EXPECT_TRUE(body != nullptr);

  auto paragraphs = result.document->getElementsByTagName("p");
  EXPECT_EQ(paragraphs.size(), 1);
  EXPECT_STREQ(paragraphs[0]->textContent().c_str(), "Hello World");

  thread.stop();
}

TEST(ParseThread_ParseHTMLWithInlineCSS) {
  ParseThread thread;
  thread.start();

  std::string html = "<!DOCTYPE html><html><head>"
                     "<style>body { color: red; } div { width: 100px; }</style>"
                     "</head><body><div>Content</div></body></html>";

  auto task = thread.submit(html, "", false);

  auto start = std::chrono::steady_clock::now();
  while (!task->isReady()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5))
      break;
  }

  EXPECT_TRUE(task->isReady());

  auto result = task->getResult();
  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.document != nullptr);
  EXPECT_GE(result.stylesheet.rules.size(), 2); // body + div rules

  thread.stop();
}

TEST(ParseThread_ExtractsScripts) {
  ParseThread thread;
  thread.start();

  std::string html = "<html><head>"
                     "<script>console.log('hello');</script>"
                     "<script src=\"app.js\"></script>"
                     "</head><body></body></html>";

  auto task = thread.submit(html, "", false);

  auto start = std::chrono::steady_clock::now();
  while (!task->isReady()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5))
      break;
  }

  auto result = task->getResult();
  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.scripts.size(), 2);

  // Inline script
  EXPECT_TRUE(result.scripts[0].content.find("console.log") !=
              std::string::npos);
  EXPECT_TRUE(result.scripts[0].src.empty());

  // External script — content not fetched (network disabled), but src recorded
  EXPECT_STREQ(result.scripts[1].src.c_str(), "app.js");

  thread.stop();
}

TEST(ParseThread_EmptyHTML) {
  ParseThread thread;
  thread.start();

  auto task = thread.submit("", "", false);

  auto start = std::chrono::steady_clock::now();
  while (!task->isReady()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5))
      break;
  }

  auto result = task->getResult();
  // Empty HTML should still produce a document (with implicit html/head/body)
  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.document != nullptr);

  thread.stop();
}

TEST(ParseThread_MalformedHTML) {
  ParseThread thread;
  thread.start();

  std::string html = "<div><p>Unclosed<span>Nested";

  auto task = thread.submit(html, "", false);

  auto start = std::chrono::steady_clock::now();
  while (!task->isReady()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5))
      break;
  }

  auto result = task->getResult();
  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.document != nullptr);

  thread.stop();
}

TEST(ParseThread_MultipleSubmissions) {
  ParseThread thread;
  thread.start();

  // Submit multiple tasks rapidly
  auto task1 = thread.submit("<html><body><p>One</p></body></html>", "", false);
  auto task2 = thread.submit("<html><body><p>Two</p></body></html>", "", false);
  auto task3 =
      thread.submit("<html><body><p>Three</p></body></html>", "", false);

  auto start = std::chrono::steady_clock::now();
  while (!(task1->isReady() && task2->isReady() && task3->isReady())) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (std::chrono::steady_clock::now() - start > std::chrono::seconds(10))
      break;
  }

  EXPECT_TRUE(task1->isReady());
  EXPECT_TRUE(task2->isReady());
  EXPECT_TRUE(task3->isReady());

  auto r1 = task1->getResult();
  auto r2 = task2->getResult();
  auto r3 = task3->getResult();

  EXPECT_TRUE(r1.success);
  EXPECT_TRUE(r2.success);
  EXPECT_TRUE(r3.success);

  // Verify each got the right content
  auto p1 = r1.document->getElementsByTagName("p");
  EXPECT_STREQ(p1[0]->textContent().c_str(), "One");

  auto p2 = r2.document->getElementsByTagName("p");
  EXPECT_STREQ(p2[0]->textContent().c_str(), "Two");

  auto p3 = r3.document->getElementsByTagName("p");
  EXPECT_STREQ(p3[0]->textContent().c_str(), "Three");

  thread.stop();
}

TEST(ParseThread_BaseTagOverride) {
  ParseThread thread;
  thread.start();

  std::string html = "<html><head>"
                     "<base href=\"https://cdn.example.com/\">"
                     "</head><body></body></html>";

  auto task = thread.submit(html, "https://example.com/", false);

  auto start = std::chrono::steady_clock::now();
  while (!task->isReady()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5))
      break;
  }

  auto result = task->getResult();
  EXPECT_TRUE(result.success);
  // baseUrl should be overridden by <base> tag
  EXPECT_STREQ(result.baseUrl.c_str(), "https://cdn.example.com/");

  thread.stop();
}

int main() { return xiaopeng::test::runTests(); }
