#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <mutex>

namespace xiaopeng {
namespace network {

// WebSocket ready state
enum class WebSocketReadyState {
  Connecting = 0,
  Open = 1,
  Closing = 2,
  Closed = 3
};

// WebSocket close codes
enum class WebSocketCloseCode : uint16_t {
  NormalClosure = 1000,
  GoingAway = 1001,
  ProtocolError = 1002,
  UnsupportedData = 1003,
  NoStatus = 1005,
  AbnormalClosure = 1006,
  InvalidFramePayloadData = 1007,
  PolicyViolation = 1008,
  MessageTooBig = 1009,
  MandatoryExtension = 1010,
  InternalServerError = 1011,
  TLSHandshake = 1015
};

// WebSocket opcodes
enum class WebSocketOpcode : uint8_t {
  Continuation = 0x0,
  Text = 0x1,
  Binary = 0x2,
  Close = 0x8,
  Ping = 0x9,
  Pong = 0xA
};

// WebSocket message
struct WebSocketMessage {
  WebSocketOpcode opcode;
  std::vector<uint8_t> data;
  bool isText;

  std::string text() const {
    return std::string(data.begin(), data.end());
  }
};

// WebSocket event types
enum class WebSocketEventType {
  Open,
  Message,
  Close,
  Error
};

// WebSocket events
struct WebSocketEvent {
  WebSocketEventType type;
  std::string error;
  WebSocketMessage message;
  uint16_t closeCode;
  std::string closeReason;
};

// WebSocket configuration
struct WebSocketConfig {
  std::string subprotocol;
  std::vector<std::string> protocols;
  std::chrono::milliseconds handshakeTimeout{5000};
  std::chrono::milliseconds pingInterval{20000};
  bool maskOutgoing = true;
  size_t maxFrameSize = 65536;
  size_t maxMessageSize = 16 * 1024 * 1024;
};

// WebSocket base interface
class IWebSocket {
public:
  virtual ~IWebSocket() = default;

  virtual void connect(const std::string &url) = 0;
  virtual void close(uint16_t code = 1000, const std::string &reason = "") = 0;
  virtual void send(const std::string &message) = 0;
  virtual void send(const std::vector<uint8_t> &data) = 0;
  virtual void sendPing() = 0;

  virtual WebSocketReadyState readyState() const = 0;
  virtual std::string url() const = 0;
  virtual std::string protocol() const = 0;

  virtual void setOnOpen(std::function<void()> callback) = 0;
  virtual void setOnMessage(std::function<void(const WebSocketMessage &)> callback) = 0;
  virtual void setOnClose(std::function<void(uint16_t, const std::string &)> callback) = 0;
  virtual void setOnError(std::function<void(const std::string &)> callback) = 0;
};

// WebSocket implementation
class WebSocket : public IWebSocket {
public:
  explicit WebSocket(const WebSocketConfig &config = WebSocketConfig());
  ~WebSocket() override;

  void connect(const std::string &url) override;
  void close(uint16_t code = 1000, const std::string &reason = "") override;
  void send(const std::string &message) override;
  void send(const std::vector<uint8_t> &data) override;
  void sendPing() override;

  WebSocketReadyState readyState() const override;
  std::string url() const override;
  std::string protocol() const override;

  void setOnOpen(std::function<void()> callback) override;
  void setOnMessage(std::function<void(const WebSocketMessage &)> callback) override;
  void setOnClose(std::function<void(uint16_t, const std::string &)> callback) override;
  void setOnError(std::function<void(const std::string &)> callback) override;

private:
  void performHandshake();
  void readFrame();
  void writeFrame(WebSocketOpcode opcode, const std::vector<uint8_t> &data);
  void handleClose(uint16_t code, const std::string &reason);
  void fireEvent(const WebSocketEvent &event);

  std::string generateRandomKey();
  std::string generateWebSocketKeyAccept(const std::string &key);

  WebSocketConfig m_config;
  WebSocketReadyState m_readyState = WebSocketReadyState::Connecting;
  std::string m_url;
  std::string m_protocol;
  
  void *m_socket = nullptr;
  std::mutex m_mutex;

  std::function<void()> m_onOpen;
  std::function<void(const WebSocketMessage &)> m_onMessage;
  std::function<void(uint16_t, const std::string &)> m_onClose;
  std::function<void(const std::string &)> m_onError;

  bool m_shouldClose = false;
  uint16_t m_closeCode = 0;
  std::string m_closeReason;
};

} // namespace network
} // namespace xiaopeng
