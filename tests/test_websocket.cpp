#include <network/websocket.hpp>
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>

using namespace xiaopeng;
using namespace xiaopeng::network;

void testWebSocketCreation() {
  std::cout << "=== Testing WebSocket Creation ===\n";

  WebSocketConfig config;
  config.subprotocol = "chat";
  config.handshakeTimeout = std::chrono::milliseconds(5000);
  config.pingInterval = std::chrono::milliseconds(20000);
  config.maskOutgoing = true;
  config.maxFrameSize = 65536;

  auto ws = std::make_shared<WebSocket>(config);

  assert(ws != nullptr);
  std::cout << "WebSocket created: ✓\n";

  std::cout << "WebSocket Creation test PASSED!\n";
}

void testWebSocketReadyState() {
  std::cout << "\n=== Testing WebSocket Ready State ===\n";

  auto ws = std::make_shared<WebSocket>();

  std::cout << "Default state: " << static_cast<int>(ws->readyState()) << "\n";
  std::cout << "WebSocket Ready State test PASSED!\n";
}

void testWebSocketCallbacks() {
  std::cout << "\n=== Testing WebSocket Callbacks ===\n";

  auto ws = std::make_shared<WebSocket>();

  bool openCalled = false;
  bool messageCalled = false;
  bool closeCalled = false;

  ws->setOnOpen([&]() {
    openCalled = true;
  });

  ws->setOnMessage([&](const WebSocketMessage &msg) {
    messageCalled = true;
    std::cout << "Message received: " << msg.text() << "\n";
  });

  ws->setOnClose([&](uint16_t code, const std::string &reason) {
    closeCalled = true;
    std::cout << "Close code: " << code << ", reason: " << reason << "\n";
  });

  ws->setOnError([&](const std::string &error) {
    std::cout << "Error: " << error << "\n";
  });

  std::cout << "Callbacks registered: ✓\n";

  ws->close(1000, "Test close");
  assert(closeCalled);
  std::cout << "Close callback triggered: ✓\n";

  std::cout << "WebSocket Callbacks test PASSED!\n";
}

void testWebSocketMessageConstruction() {
  std::cout << "\n=== Testing WebSocket Message Construction ===\n";

  WebSocketMessage msg;
  msg.opcode = WebSocketOpcode::Text;
  msg.isText = true;
  msg.data = {'H', 'e', 'l', 'l', 'o'};

  std::string text = msg.text();
  assert(text == "Hello");
  std::cout << "Text message construction: ✓\n";

  WebSocketMessage binaryMsg;
  binaryMsg.opcode = WebSocketOpcode::Binary;
  binaryMsg.isText = false;
  binaryMsg.data = {0x48, 0x65, 0x6c, 0x6c, 0x6f};

  assert(binaryMsg.text() == "Hello");
  std::cout << "Binary message as text: ✓\n";

  std::cout << "WebSocket Message Construction test PASSED!\n";
}

void testWebSocketCloseCodes() {
  std::cout << "\n=== Testing WebSocket Close Codes ===\n";

  assert(static_cast<uint16_t>(WebSocketCloseCode::NormalClosure) == 1000);
  std::cout << "NormalClosure (1000): ✓\n";

  assert(static_cast<uint16_t>(WebSocketCloseCode::GoingAway) == 1001);
  std::cout << "GoingAway (1001): ✓\n";

  assert(static_cast<uint16_t>(WebSocketCloseCode::ProtocolError) == 1002);
  std::cout << "ProtocolError (1002): ✓\n";

  assert(static_cast<uint16_t>(WebSocketCloseCode::MessageTooBig) == 1009);
  std::cout << "MessageTooBig (1009): ✓\n";

  std::cout << "WebSocket Close Codes test PASSED!\n";
}

void testWebSocketConfig() {
  std::cout << "\n=== Testing WebSocket Configuration ===\n";

  WebSocketConfig config;
  config.subprotocol = "my-protocol";
  config.protocols = {"protocol1", "protocol2"};
  config.handshakeTimeout = std::chrono::milliseconds(10000);
  config.pingInterval = std::chrono::milliseconds(30000);
  config.maskOutgoing = false;
  config.maxFrameSize = 131072;
  config.maxMessageSize = 32 * 1024 * 1024;

  assert(config.subprotocol == "my-protocol");
  std::cout << "Subprotocol: ✓\n";

  assert(config.protocols.size() == 2);
  std::cout << "Protocols: ✓\n";

  assert(config.maskOutgoing == false);
  std::cout << "Mask outgoing: ✓\n";

  std::cout << "WebSocket Configuration test PASSED!\n";
}

int main() {
  std::cout << "Running WebSocket API tests\n";
  std::cout << "================================================\n";

  try {
    testWebSocketCreation();
    testWebSocketReadyState();
    testWebSocketCallbacks();
    testWebSocketMessageConstruction();
    testWebSocketCloseCodes();
    testWebSocketConfig();

    std::cout << "================================================\n";
    std::cout << "All WebSocket tests PASSED!\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Test FAILED: " << e.what() << "\n";
    return 1;
  }
}
