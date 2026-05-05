#include <network/websocket.hpp>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

namespace xiaopeng {
namespace network {

static std::string base64Encode(const unsigned char *input, int length) {
  BIO *bio, *b64;
  BUF_MEM *bufferPtr;
  
  b64 = BIO_new(BIO_f_base64());
  bio = BIO_new(BIO_s_mem());
  bio = BIO_push(b64, bio);
  
  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
  BIO_write(bio, input, length);
  BIO_flush(bio);
  
  BIO_get_mem_ptr(bio, &bufferPtr);
  
  std::string result(bufferPtr->data, bufferPtr->length);
  BIO_free_all(bio);
  
  return result;
}

WebSocket::WebSocket(const WebSocketConfig &config) : m_config(config) {}

WebSocket::~WebSocket() {
  if (m_socket) {
    close(1001, "Going away");
  }
}

void WebSocket::connect(const std::string &url) {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  if (m_readyState != WebSocketReadyState::Connecting &&
      m_readyState != WebSocketReadyState::Closed) {
    return;
  }

  m_url = url;
  m_readyState = WebSocketReadyState::Connecting;
  
  performHandshake();
}

void WebSocket::close(uint16_t code, const std::string &reason) {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  if (m_readyState == WebSocketReadyState::Closing ||
      m_readyState == WebSocketReadyState::Closed) {
    return;
  }

  m_readyState = WebSocketReadyState::Closing;
  m_closeCode = code;
  m_closeReason = reason;

  std::vector<uint8_t> closeData;
  closeData.push_back(static_cast<uint8_t>((code >> 8) & 0xFF));
  closeData.push_back(static_cast<uint8_t>(code & 0xFF));
  closeData.insert(closeData.end(), reason.begin(), reason.end());

  writeFrame(WebSocketOpcode::Close, closeData);
  
  handleClose(code, reason);
}

void WebSocket::send(const std::string &message) {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  if (m_readyState != WebSocketReadyState::Open) {
    return;
  }

  std::vector<uint8_t> data(message.begin(), message.end());
  writeFrame(WebSocketOpcode::Text, data);
}

void WebSocket::send(const std::vector<uint8_t> &data) {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  if (m_readyState != WebSocketReadyState::Open) {
    return;
  }

  writeFrame(WebSocketOpcode::Binary, data);
}

void WebSocket::sendPing() {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  if (m_readyState != WebSocketReadyState::Open) {
    return;
  }

  writeFrame(WebSocketOpcode::Ping, {});
}

WebSocketReadyState WebSocket::readyState() const {
  return m_readyState;
}

std::string WebSocket::url() const {
  return m_url;
}

std::string WebSocket::protocol() const {
  return m_protocol;
}

void WebSocket::setOnOpen(std::function<void()> callback) {
  m_onOpen = callback;
}

void WebSocket::setOnMessage(std::function<void(const WebSocketMessage &)> callback) {
  m_onMessage = callback;
}

void WebSocket::setOnClose(std::function<void(uint16_t, const std::string &)> callback) {
  m_onClose = callback;
}

void WebSocket::setOnError(std::function<void(const std::string &)> callback) {
  m_onError = callback;
}

void WebSocket::performHandshake() {
  try {
    std::string host = "localhost";
    std::string path = "/";
    int port = 80;

    size_t protocolStart = m_url.find("://");
    if (protocolStart != std::string::npos) {
      size_t hostStart = protocolStart + 3;
      size_t pathStart = m_url.find('/', hostStart);
      
      std::string hostPart;
      if (pathStart != std::string::npos) {
        hostPart = m_url.substr(hostStart, pathStart - hostStart);
        path = m_url.substr(pathStart);
      } else {
        hostPart = m_url.substr(hostStart);
      }

      size_t colonPos = hostPart.find(':');
      if (colonPos != std::string::npos) {
        host = hostPart.substr(0, colonPos);
        port = std::stoi(hostPart.substr(colonPos + 1));
      } else {
        host = hostPart;
        port = (m_url.substr(0, 5) == "https") ? 443 : 80;
      }
    }

    std::string key = generateRandomKey();
    std::string keyAccept = generateWebSocketKeyAccept(key);

    std::ostringstream request;
    request << "GET " << path << " HTTP/1.1\r\n";
    request << "Host: " << host << ":" << port << "\r\n";
    request << "Upgrade: websocket\r\n";
    request << "Connection: Upgrade\r\n";
    request << "Sec-WebSocket-Key: " << key << "\r\n";
    request << "Sec-WebSocket-Version: 13\r\n";
    
    if (!m_config.subprotocol.empty()) {
      request << "Sec-WebSocket-Protocol: " << m_config.subprotocol << "\r\n";
    }
    
    request << "\r\n";

    m_readyState = WebSocketReadyState::Open;
    
    WebSocketEvent event;
    event.type = WebSocketEventType::Open;
    fireEvent(event);
    
  } catch (const std::exception &e) {
    m_readyState = WebSocketReadyState::Closed;
    WebSocketEvent event;
    event.type = WebSocketEventType::Error;
    event.error = e.what();
    fireEvent(event);
  }
}

std::string WebSocket::generateRandomKey() {
  static const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string key;
  
  for (int i = 0; i < 22; ++i) {
    key += charset[rand() % (sizeof(charset) - 1)];
  }
  
  return key + "==";
}

std::string WebSocket::generateWebSocketKeyAccept(const std::string &key) {
  std::string combined = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  
  unsigned char hash[SHA_DIGEST_LENGTH];
  SHA1(reinterpret_cast<const unsigned char*>(combined.c_str()), combined.length(), hash);
  
  return base64Encode(hash, SHA_DIGEST_LENGTH);
}

void WebSocket::readFrame() {
  std::lock_guard<std::mutex> lock(m_mutex);
  
  std::vector<uint8_t> buffer(4096);
  
  WebSocketMessage message;
  message.opcode = WebSocketOpcode::Continuation;
  
  bool reading = true;
  while (reading && m_readyState == WebSocketReadyState::Open) {
    if (buffer.size() < 2) {
      break;
    }

    uint8_t firstByte = buffer[0];
    uint8_t secondByte = buffer[1];
    
    WebSocketOpcode opcode = static_cast<WebSocketOpcode>(firstByte & 0x0F);
    bool fin = (firstByte & 0x80) != 0;
    bool masked = (secondByte & 0x80) != 0;
    
    uint64_t payloadLength = secondByte & 0x7F;
    size_t headerSize = 2;
    
    if (payloadLength == 126) {
      headerSize += 2;
    } else if (payloadLength == 127) {
      headerSize += 8;
    }
    
    size_t maskKeySize = masked ? 4 : 0;
    
    if (buffer.size() < headerSize + maskKeySize) {
      break;
    }
    
    size_t offset = headerSize + maskKeySize;
    
    if (payloadLength == 126) {
      payloadLength = (buffer[2] << 8) | buffer[3];
    } else if (payloadLength == 127) {
      payloadLength = 0;
      for (int i = 0; i < 8; ++i) {
        payloadLength = (payloadLength << 8) | buffer[2 + i];
      }
    }
    
    if (buffer.size() < offset + payloadLength) {
      break;
    }
    
    std::vector<uint8_t> payload(buffer.begin() + offset, buffer.begin() + offset + payloadLength);
    
    if (masked) {
      uint8_t *mask = &buffer[headerSize];
      for (size_t i = 0; i < payload.size(); ++i) {
        payload[i] ^= mask[i % 4];
      }
    }
    
    switch (opcode) {
    case WebSocketOpcode::Text:
      message.opcode = opcode;
      message.isText = true;
      message.data = payload;
      break;
      
    case WebSocketOpcode::Binary:
      message.opcode = opcode;
      message.isText = false;
      message.data = payload;
      break;
      
    case WebSocketOpcode::Close:
      if (payload.size() >= 2) {
        uint16_t closeCode = (payload[0] << 8) | payload[1];
        std::string closeReason(payload.begin() + 2, payload.end());
        handleClose(closeCode, closeReason);
      } else {
        handleClose(1000, "");
      }
      reading = false;
      break;
      
    case WebSocketOpcode::Ping:
      writeFrame(WebSocketOpcode::Pong, payload);
      break;
      
    case WebSocketOpcode::Pong:
      break;
      
    case WebSocketOpcode::Continuation:
      message.data.insert(message.data.end(), payload.begin(), payload.end());
      break;
    }
    
    if (fin && message.opcode != WebSocketOpcode::Continuation) {
      WebSocketEvent event;
      event.type = WebSocketEventType::Message;
      event.message = message;
      fireEvent(event);
    }
  }
}

void WebSocket::writeFrame(WebSocketOpcode opcode, const std::vector<uint8_t> &data) {
  if (m_readyState != WebSocketReadyState::Open &&
      opcode != WebSocketOpcode::Close) {
    return;
  }

  std::vector<uint8_t> frame;
  
  frame.push_back(static_cast<uint8_t>(0x80 | static_cast<uint8_t>(opcode)));
  
  uint64_t payloadLength = data.size();
  if (payloadLength < 126) {
    frame.push_back(static_cast<uint8_t>(payloadLength));
  } else if (payloadLength < 65536) {
    frame.push_back(126);
    frame.push_back(static_cast<uint8_t>((payloadLength >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(payloadLength & 0xFF));
  } else {
    frame.push_back(127);
    for (int i = 7; i >= 0; --i) {
      frame.push_back(static_cast<uint8_t>((payloadLength >> (i * 8)) & 0xFF));
    }
  }
  
  if (m_config.maskOutgoing) {
    frame[1] |= 0x80;
    uint8_t mask[4];
    for (int i = 0; i < 4; ++i) {
      mask[i] = rand() & 0xFF;
    }
    frame.insert(frame.end(), mask, mask + 4);
    
    std::vector<uint8_t> maskedData = data;
    for (size_t i = 0; i < maskedData.size(); ++i) {
      maskedData[i] ^= mask[i % 4];
    }
    frame.insert(frame.end(), maskedData.begin(), maskedData.end());
  } else {
    frame.insert(frame.end(), data.begin(), data.end());
  }
}

void WebSocket::handleClose(uint16_t code, const std::string &reason) {
  if (m_readyState == WebSocketReadyState::Closed) {
    return;
  }

  m_readyState = WebSocketReadyState::Closed;
  m_closeCode = static_cast<uint16_t>(code);
  m_closeReason = reason;

  WebSocketEvent event;
  event.type = WebSocketEventType::Close;
  event.closeCode = code;
  event.closeReason = reason;
  fireEvent(event);
}

void WebSocket::fireEvent(const WebSocketEvent &event) {
  switch (event.type) {
  case WebSocketEventType::Open:
    if (m_onOpen) {
      m_onOpen();
    }
    break;
    
  case WebSocketEventType::Message:
    if (m_onMessage) {
      m_onMessage(event.message);
    }
    break;
    
  case WebSocketEventType::Close:
    if (m_onClose) {
      m_onClose(event.closeCode, event.closeReason);
    }
    break;
    
  case WebSocketEventType::Error:
    if (m_onError) {
      m_onError(event.error);
    }
    break;
  }
}

} // namespace network
} // namespace xiaopeng
