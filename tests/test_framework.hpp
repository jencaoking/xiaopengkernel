#pragma once

// ═══════════════════════════════════════════════════════════════
//  XiaopengKernel Test Framework
//
//  Features:
//    - Automatic test registration via TEST() macro
//    - EXPECT_* macros (continue on failure)
//    - ASSERT_* macros (abort test on failure)
//    - Test filtering: --filter=Pattern
//    - Test listing:  --list
//    - XML output:    --output=xml
//    - Test timing
//    - Colored output (terminal)
//
//  Usage:
//    #include "test_framework.hpp"
//
//    TEST(MyTest) {
//      EXPECT_EQ(1 + 1, 2);
//      ASSERT_TRUE(true);
//    }
//
//    int main() { return xiaopeng::test::runTests(); }
// ═══════════════════════════════════════════════════════════════

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace xiaopeng {
namespace test {

// ── Exception thrown by ASSERT macros ────────────────────────
class TestAssertionFailed : public std::exception {
public:
  TestAssertionFailed(const std::string &msg) : message_(msg) {}
  const char *what() const noexcept override { return message_.c_str(); }
private:
  std::string message_;
};

// ── Test Result ──────────────────────────────────────────────
struct TestResult {
  std::string name;
  bool passed = true;
  std::string message;
  std::string file;
  int line = 0;
  double duration_ms = 0.0;
};

// ── Configuration (parsed from argv) ─────────────────────────
struct TestConfig {
  std::string filter;           // --filter=Pattern
  bool listOnly = false;        // --list
  std::string outputFormat;     // --output=xml
  bool coloredOutput = true;    // --no-color to disable
};

inline TestConfig &getConfig() {
  static TestConfig config;
  return config;
}

// ── Parse command line arguments ─────────────────────────────
inline void parseArgs(int argc, char *argv[]) {
  auto &cfg = getConfig();
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg.find("--filter=") == 0) {
      cfg.filter = arg.substr(9);
    } else if (arg == "--list") {
      cfg.listOnly = true;
    } else if (arg.find("--output=") == 0) {
      cfg.outputFormat = arg.substr(9);
    } else if (arg == "--no-color") {
      cfg.coloredOutput = false;
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "Usage: [options]\n"
                << "  --filter=Pattern   Run only tests matching pattern\n"
                << "  --list             List all tests without running\n"
                << "  --output=xml       Output results in XML format\n"
                << "  --no-color         Disable colored output\n";
      std::exit(0);
    }
  }
}

// ── Color helpers ────────────────────────────────────────────
inline const char *colorGreen() {
  return getConfig().coloredOutput ? "\033[32m" : "";
}
inline const char *colorRed() {
  return getConfig().coloredOutput ? "\033[31m" : "";
}
inline const char *colorYellow() {
  return getConfig().coloredOutput ? "\033[33m" : "";
}
inline const char *colorGray() {
  return getConfig().coloredOutput ? "\033[90m" : "";
}
inline const char *colorReset() {
  return getConfig().coloredOutput ? "\033[0m" : "";
}

// ── Test Runner ──────────────────────────────────────────────
class TestRunner {
public:
  static TestRunner &instance() {
    static TestRunner runner;
    return runner;
  }

  void addTest(const std::string &name, std::function<void()> test) {
    tests_.push_back({name, test});
  }

  int runAll(int argc = 0, char *argv[] = nullptr) {
    if (argc > 0 && argv) {
      parseArgs(argc, argv);
    }

    const auto &cfg = getConfig();

    // Filter tests
    std::vector<size_t> indicesToRun;
    for (size_t i = 0; i < tests_.size(); ++i) {
      if (cfg.filter.empty() ||
          tests_[i].first.find(cfg.filter) != std::string::npos) {
        indicesToRun.push_back(i);
      }
    }

    // List mode
    if (cfg.listOnly) {
      std::cout << "Available tests (" << indicesToRun.size() << "):" << std::endl;
      for (size_t idx : indicesToRun) {
        std::cout << "  " << tests_[idx].first << std::endl;
      }
      return 0;
    }

    // Run tests
    int passed = 0;
    int failed = 0;
    double totalTime = 0.0;

    std::cout << "\n" << colorGreen()
              << "========================================" << colorReset()
              << std::endl;
    std::cout << "Running " << indicesToRun.size() << " of " << tests_.size()
              << " tests..." << std::endl;
    std::cout << colorGreen()
              << "========================================" << colorReset()
              << "\n"
              << std::endl;

    for (size_t idx : indicesToRun) {
      const auto &[name, test] = tests_[idx];
      currentTest_ = name;
      currentResult_ = TestResult{name, true, "", "", 0, 0.0};

      auto startTime = std::chrono::steady_clock::now();

      try {
        test();
      } catch (const TestAssertionFailed &e) {
        currentResult_.passed = false;
        currentResult_.message = e.what();
      } catch (const std::exception &e) {
        currentResult_.passed = false;
        currentResult_.message = std::string("Exception: ") + e.what();
      } catch (...) {
        currentResult_.passed = false;
        currentResult_.message = "Unknown exception";
      }

      auto endTime = std::chrono::steady_clock::now();
      currentResult_.duration_ms =
          std::chrono::duration<double, std::milli>(endTime - startTime).count();
      totalTime += currentResult_.duration_ms;

      if (currentResult_.passed) {
        std::cout << colorGreen() << "[PASS] " << colorReset() << name
                  << colorGray() << " (" << currentResult_.duration_ms << "ms)"
                  << colorReset() << std::endl;
        ++passed;
      } else {
        std::cout << colorRed() << "[FAIL] " << colorReset() << name << std::endl;
        if (!currentResult_.message.empty()) {
          std::cout << "       " << colorYellow() << currentResult_.message
                    << colorReset() << std::endl;
        }
        if (!currentResult_.file.empty()) {
          std::cout << "       " << colorGray() << "at " << currentResult_.file
                    << ":" << currentResult_.line << colorReset() << std::endl;
        }
        ++failed;
      }

      results_.push_back(currentResult_);
    }

    // Summary
    std::cout << "\n" << colorGreen()
              << "========================================" << colorReset()
              << std::endl;
    if (failed == 0) {
      std::cout << colorGreen() << "All " << passed << " tests passed!"
                << colorReset() << std::endl;
    } else {
      std::cout << colorGreen() << passed << " passed" << colorReset() << ", "
                << colorRed() << failed << " failed" << colorReset()
                << std::endl;
    }
    std::cout << colorGray() << "Total time: " << totalTime << "ms"
              << colorReset() << std::endl;
    std::cout << colorGreen()
              << "========================================" << colorReset()
              << std::endl;

    // XML output
    if (cfg.outputFormat == "xml") {
      outputXml();
    }

    return failed > 0 ? 1 : 0;
  }

  void fail(const std::string &message, const std::string &file, int line) {
    currentResult_.passed = false;
    currentResult_.message = message;
    currentResult_.file = file;
    currentResult_.line = line;
  }

  // Called by ASSERT macros to throw
  [[noreturn]] void failAssert(const std::string &message,
                               const std::string &file, int line) {
    currentResult_.passed = false;
    currentResult_.message = message;
    currentResult_.file = file;
    currentResult_.line = line;
    throw TestAssertionFailed(message);
  }

  const std::vector<TestResult> &results() const { return results_; }

private:
  TestRunner() = default;

  void outputXml() {
    std::cout << "\n<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    std::cout << "<testsuites>\n";
    std::cout << "  <testsuite name=\"XiaopengKernel\" tests=\"" << results_.size()
              << "\">\n";

    for (const auto &r : results_) {
      std::cout << "    <testcase name=\"" << r.name << "\" time=\""
                << (r.duration_ms / 1000.0) << "\">";
      if (!r.passed) {
        std::cout << "\n      <failure message=\"" << r.message << "\">";
        std::cout << r.file << ":" << r.line;
        std::cout << "</failure>\n    ";
      }
      std::cout << "</testcase>\n";
    }

    std::cout << "  </testsuite>\n";
    std::cout << "</testsuites>\n";
  }

  std::vector<std::pair<std::string, std::function<void()>>> tests_;
  std::vector<TestResult> results_;
  std::string currentTest_;
  TestResult currentResult_;
};

// ── Test Registrar ───────────────────────────────────────────
class TestRegistrar {
public:
  TestRegistrar(const std::string &name, std::function<void()> test) {
    TestRunner::instance().addTest(name, test);
  }
};

// ── TEST macro ───────────────────────────────────────────────
#define TEST(name)                                                             \
  void test_##name();                                                          \
  xiaopeng::test::TestRegistrar registrar_##name(#name, test_##name);          \
  void test_##name()

// ── EXPECT macros (continue on failure) ──────────────────────

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

#define EXPECT_LE(a, b)                                                        \
  do {                                                                         \
    if (!((a) <= (b))) {                                                       \
      std::ostringstream oss;                                                  \
      oss << "EXPECT_LE failed: " << #a << " > " << #b;                        \
      xiaopeng::test::TestRunner::instance().fail(oss.str(), __FILE__,         \
                                                  __LINE__);                   \
      return;                                                                  \
    }                                                                          \
  } while (0)

// ── ASSERT macros (abort test on failure) ────────────────────

#define ASSERT_TRUE(condition)                                                 \
  do {                                                                         \
    if (!(condition)) {                                                        \
      std::ostringstream oss;                                                  \
      oss << "ASSERT_TRUE failed: " << #condition;                             \
      xiaopeng::test::TestRunner::instance().failAssert(oss.str(), __FILE__,   \
                                                        __LINE__);             \
    }                                                                          \
  } while (0)

#define ASSERT_FALSE(condition)                                                \
  do {                                                                         \
    if (condition) {                                                           \
      std::ostringstream oss;                                                  \
      oss << "ASSERT_FALSE failed: " << #condition;                            \
      xiaopeng::test::TestRunner::instance().failAssert(oss.str(), __FILE__,   \
                                                        __LINE__);             \
    }                                                                          \
  } while (0)

#define ASSERT_EQ(a, b)                                                        \
  do {                                                                         \
    if (!((a) == (b))) {                                                       \
      std::ostringstream oss;                                                  \
      oss << "ASSERT_EQ failed: " << #a << " != " << #b;                       \
      xiaopeng::test::TestRunner::instance().failAssert(oss.str(), __FILE__,   \
                                                        __LINE__);             \
    }                                                                          \
  } while (0)

#define ASSERT_NE(a, b)                                                        \
  do {                                                                         \
    if ((a) == (b)) {                                                          \
      std::ostringstream oss;                                                  \
      oss << "ASSERT_NE failed: " << #a << " == " << #b;                       \
      xiaopeng::test::TestRunner::instance().failAssert(oss.str(), __FILE__,   \
                                                        __LINE__);             \
    }                                                                          \
  } while (0)

#define ASSERT_STREQ(a, b)                                                     \
  do {                                                                         \
    if (std::string(a) != std::string(b)) {                                    \
      std::ostringstream oss;                                                  \
      oss << "ASSERT_STREQ failed: \"" << a << "\" != \"" << b << "\"";        \
      xiaopeng::test::TestRunner::instance().failAssert(oss.str(), __FILE__,   \
                                                        __LINE__);             \
    }                                                                          \
  } while (0)

// ── Convenience: run with argc/argv ──────────────────────────
inline int runTests(int argc = 0, char *argv[] = nullptr) {
  return TestRunner::instance().runAll(argc, argv);
}

} // namespace test
} // namespace xiaopeng
