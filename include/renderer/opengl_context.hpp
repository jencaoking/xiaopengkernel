#pragma once

namespace xiaopeng {
namespace renderer {

class OpenGLContext {
public:
  OpenGLContext();
  ~OpenGLContext();

  bool initialize();
  void shutdown();

  void makeCurrent();
  void swapBuffers();

  bool isInitialized() const { return initialized_; }

private:
  bool initialized_ = false;
#ifdef ENABLE_OPENGL
  // Platform-specific context handle
  void* context_ = nullptr;
  void* window_ = nullptr;
#endif
};

} // namespace renderer
} // namespace xiaopeng