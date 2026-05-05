#include <engine/event_loop.hpp>
#include <engine/browsing_context.hpp>
#include <iostream>
#include <cassert>

using namespace xiaopeng;
using namespace xiaopeng::engine;

void testBasicEventLoop() {
    std::cout << "=== Testing Basic Event Loop ===" << std::endl;

    EventLoop loop;

    // Test macro task
    bool macro_ran = false;
    loop.queueMacroTask([&macro_ran]() { macro_ran = true; });

    // Test micro task
    bool micro_ran = false;
    loop.queueMicroTask([&micro_ran]() { micro_ran = true; });

    // Run tick
    loop.tick();

    // Check results
    assert(macro_ran && "Macro task should run");
    assert(micro_ran && "Micro task should run");

    std::cout << "Basic Event Loop test PASSED" << std::endl;
}

void testMicroTaskPriority() {
    std::cout << "=== Testing Micro Task Priority ===" << std::endl;

    EventLoop loop;

    std::vector<std::string> order;

    loop.queueMacroTask([&order]() { order.push_back("macro"); });
    loop.queueMicroTask([&order]() { order.push_back("micro1"); });
    loop.queueMicroTask([&order]() { order.push_back("micro2"); });

    loop.tick();

    // Micro tasks should run after macro task, in order
    assert(order.size() == 3);
    assert(order[0] == "macro");
    assert(order[1] == "micro1");
    assert(order[2] == "micro2");

    std::cout << "Micro Task Priority test PASSED" << std::endl;
}

void testMultipleTicks() {
    std::cout << "=== Testing Multiple Ticks ===" << std::endl;

    EventLoop loop;

    int count = 0;

    loop.queueMacroTask([&count]() { count++; });
    loop.queueMacroTask([&count]() { count++; });
    loop.queueMacroTask([&count]() { count++; });

    loop.tick();  // 1st tick: 1 macro
    assert(count == 1);

    loop.tick();  // 2nd tick: 2nd macro
    assert(count == 2);

    loop.tick();  // 3rd tick: 3rd macro
    assert(count == 3);

    std::cout << "Multiple Ticks test PASSED" << std::endl;
}

void testIdleState() {
    std::cout << "=== Testing Idle State ===" << std::endl;

    EventLoop loop;

    assert(loop.isIdle() && "Should start idle");
    assert(loop.macroTaskCount() == 0 && "Macro tasks should be 0");
    assert(loop.microTaskCount() == 0 && "Micro tasks should be 0");

    loop.queueMacroTask([]() {});
    assert(!loop.isIdle() && "Should not be idle with pending tasks");

    loop.tick();
    assert(loop.isIdle() && "Should return to idle");

    std::cout << "Idle State test PASSED" << std::endl;
}

void testFlushMicroTasks() {
    std::cout << "=== Testing Flush Micro Tasks ===" << std::endl;

    EventLoop loop;

    int micro_count = 0;

    for (int i = 0; i < 5; i++) {
        loop.queueMicroTask([&micro_count]() { micro_count++; });
    }

    loop.flushMicroTasks();
    assert(micro_count == 5);
    assert(loop.microTaskCount() == 0);

    std::cout << "Flush Micro Tasks test PASSED" << std::endl;
}

void testHistoryAPI() {
    std::cout << "=== Testing History API ===" << std::endl;

    auto browsing_ctx = std::make_shared<BrowsingContext>();

    // Initial state
    assert(browsing_ctx->length() == 0);
    assert(browsing_ctx->getIndex() == -1);

    // Push some states
    browsing_ctx->pushState("state1", "Title1", "/page1");
    assert(browsing_ctx->length() == 1);
    assert(browsing_ctx->getIndex() == 0);
    assert(browsing_ctx->getState() == "state1");

    browsing_ctx->pushState("state2", "Title2", "/page2");
    assert(browsing_ctx->length() == 2);
    assert(browsing_ctx->getIndex() == 1);
    assert(browsing_ctx->getState() == "state2");

    browsing_ctx->pushState("state3", "Title3", "/page3");
    assert(browsing_ctx->length() == 3);

    // Go back
    browsing_ctx->go(-1);
    assert(browsing_ctx->getIndex() == 1);
    assert(browsing_ctx->getState() == "state2");

    // Go back again
    browsing_ctx->back();
    assert(browsing_ctx->getIndex() == 0);
    assert(browsing_ctx->getState() == "state1");

    // Go forward
    browsing_ctx->forward();
    assert(browsing_ctx->getIndex() == 1);
    assert(browsing_ctx->getState() == "state2");

    // Replace state
    browsing_ctx->replaceState("newState2", "NewTitle2", "/newpage2");
    assert(browsing_ctx->length() == 3);  // Length should be same
    assert(browsing_ctx->getState() == "newState2");

    // Push state (should truncate history)
    browsing_ctx->pushState("state4", "Title4", "/page4");
    assert(browsing_ctx->length() == 3);  // History should be truncated

    std::cout << "History API test PASSED" << std::endl;
}

void testHistoryBounds() {
    std::cout << "=== Testing History Bounds ===" << std::endl;

    auto browsing_ctx = std::make_shared<BrowsingContext>();

    browsing_ctx->pushState("state1", "", "/1");
    browsing_ctx->pushState("state2", "", "/2");

    // Try to go back past start
    browsing_ctx->back();  // index 0
    browsing_ctx->back();  // still index 0 (can't go past start)
    browsing_ctx->back();  // still index 0
    assert(browsing_ctx->getIndex() == 0);

    // Try to go forward past end
    browsing_ctx->go(2);  // index 1
    browsing_ctx->forward();  // still index 1
    assert(browsing_ctx->getIndex() == 1);

    std::cout << "History Bounds test PASSED" << std::endl;
}

int main() {
    std::cout << "Running Event Loop and History API tests" << std::endl;
    std::cout << "================================================" << std::endl;

    try {
        testBasicEventLoop();
        testMicroTaskPriority();
        testMultipleTicks();
        testIdleState();
        testFlushMicroTasks();
        testHistoryAPI();
        testHistoryBounds();

        std::cout << "================================================" << std::endl;
        std::cout << "All Event Loop and History API tests PASSED!" << std::endl;
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Test FAILED: " << e.what() << std::endl;
        return 1;
    }
}
