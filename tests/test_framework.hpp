#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace xiaopeng {
namespace test {

struct TestResult {
  std::string name;
  bool passed;
  std::string message;
  std::string file;
  int line;
};

class TestRunner {
public:
  static TestRunner &instance() {
    static TestRunner runner;
    return runner;
  }

  void addTest(const std::string &name, std::function<void()> test) {
    tests_.push_back({name, test});
  }

  int runAll() {
    int passed = 0;
    int failed = 0;

    std::cout << "\n========================================" << std::endl;
    std::cout << "Running " << tests_.size() << " tests..." << std::endl;
    std::cout << "========================================\n" << std::endl;

    for (const auto &[name, test] : tests_) {
      currentTest_ = name;
      currentResult_ = TestResult{name, true, "", "", 0};

      try {
        test();
      } catch (const std::exception &e) {
        currentResult_.passed = false;
        currentResult_.message = e.what();
      } catch (...) {
        currentResult_.passed = false;
        currentResult_.message = "Unknown exception";
      }

      if (currentResult_.passed) {
        std::cout << "[PASS] " << name << std::endl;
        ++passed;
      } else {
        std::cout << "[FAIL] " << name << std::endl;
        if (!currentResult_.message.empty()) {
          std::cout << "       " << currentResult_.message << std::endl;
        }
        ++failed;
      }

      results_.push_back(currentResult_);
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed"
              << std::endl;
    std::cout << "========================================" << std::endl;

    return failed > 0 ? 1 : 0;
  }

  void fail(const std::string &message, const std::string &file, int line) {
    currentResult_.passed = false;
    currentResult_.message = message;
    currentResult_.file = file;
    currentResult_.line = line;
  }

private:
  TestRunner() = default;

  std::vector<std::pair<std::string, std::function<void()>>> tests_;
  std::vector<TestResult> results_;
  std::string currentTest_;
  TestResult currentResult_;
};

class TestRegistrar {
public:
  TestRegistrar(const std::string &name, std::function<void()> test) {
    TestRunner::instance().addTest(name, test);
  }
};

#define TEST(name)                                                             \
  void test_##name();                                                          \
  xiaopeng::test::TestRegistrar registrar_##name(#name, test_##name);          \
  void test_##name()

#define EXPECT_TRUE(condition)                                                 \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::ostringstream oss;                                                  \
      oss << "EXPECT_TRUE failed: " << #condition;                             \
      xiaopeng::test::TestRunner::instance().fail(oss.str(), __FILE__,         \
                                                  __LINE__);                   \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define EXPECT_FALSE(condition)                                                \
  do {                                                                         \
    if (condition) {                                                           \
      std::ostringstream oss;                                                  \
      oss << "EXPECT_FALSE failed: " << #condition;                            \
      xiaopeng::test::TestRunner::instance().fail(oss.str(), __FILE__,         \
                                                  __LINE__);                   \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define EXPECT_EQ(a, b)                                                        \
  do {                                                                         \
    if (!((a) == (b))) {                                                       \
      std::ostringstream oss;                                                  \
      oss << "EXPECT_EQ failed: " << #a << " != " << #b;                       \
      xiaopeng::test::TestRunner::instance().fail(oss.str(), __FILE__,         \
                                                  __LINE__);                   \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define EXPECT_NE(a, b)                                                        \
  do {                                                                         \
    if ((a) == (b)) {                                                          \
      std::ostringstream oss;                                                  \
      oss << "EXPECT_NE failed: " << #a << " == " << #b;                       \
      xiaopeng::test::TestRunner::instance().fail(oss.str(), __FILE__,         \
                                                  __LINE__);                   \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define EXPECT_STREQ(a, b)                                                     \
  do {                                                                         \
    if (std::string(a) != std::string(b)) {                                    \
      std::ostringstream oss;                                                  \
      oss << "EXPECT_STREQ failed: \"" << a << "\" != \"" << b << "\"";        \
      xiaopeng::test::TestRunner::instance().fail(oss.str(), __FILE__,         \
                                                  __LINE__);                   \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define EXPECT_GT(a, b)                                                        \
  do {                                                                         \
    if (!((a) > (b))) {                                                        \
      std::ostringstream oss;                                                  \
      oss << "EXPECT_GT failed: " << #a << " <= " << #b;                       \
      xiaopeng::test::TestRunner::instance().fail(oss.str(), __FILE__,         \
                                                  __LINE__);                   \
      return;                                                                  \
    }                                                                          \
  } while (0)

#if defined(__GNUC__) || defined(__clang__)
#define EXPECT_GE(a, b)                                                        \
  do {                                                                         \
    _Pragma("GCC diagnostic push") _Pragma(                                    \
        "GCC diagnostic ignored \"-Wtype-limits\"") if (!((a) >= (b))) {       \
      std::ostringstream oss;                                                  \
      oss << "EXPECT_GE failed: " << #a << " < " << #b;                        \
      xiaopeng::test::TestRunner::instance().fail(oss.str(), __FILE__,         \
                                                  __LINE__);                   \
      _Pragma("GCC diagnostic pop") return;                                    \
    }                                                                          \
    _Pragma("GCC diagnostic pop")                                              \
  } while (0)
#else
#define EXPECT_GE(a, b)                                                        \
  do {                                                                         \
    if (!((a) >= (b))) {                                                       \
      std::ostringstream oss;                                                  \
      oss << "EXPECT_GE failed: " << #a << " < " << #b;                        \
      xiaopeng::test::TestRunner::instance().fail(oss.str(), __FILE__,         \
                                                  __LINE__);                   \
      return;                                                                  \
    }                                                                          \
  } while (0)
#endif

#define EXPECT_LT(a, b)                                                        \
  do {                                                                         \
    if (!((a) < (b))) {                                                        \
      std::ostringstream oss;                                                  \
      oss << "EXPECT_LT failed: " << #a << " >= " << #b;                       \
      xiaopeng::test::TestRunner::instance().fail(oss.str(), __FILE__,         \
                                                  __LINE__);                   \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define ASSERT_TRUE(condition) EXPECT_TRUE(condition)
#define ASSERT_FALSE(condition) EXPECT_FALSE(condition)
#define ASSERT_EQ(a, b) EXPECT_EQ(a, b)
#define ASSERT_NE(a, b) EXPECT_NE(a, b)
#define ASSERT_STREQ(a, b) EXPECT_STREQ(a, b)

inline int runTests() { return TestRunner::instance().runAll(); }

} // namespace test
} // namespace xiaopeng
