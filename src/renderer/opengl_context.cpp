#include "renderer/opengl_context.hpp"
#include <iostream>

#ifdef ENABLE_OPENGL
#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#elif defined(__linux__)
#include <GL/gl.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#endif
#endif

namespace xiaopeng {
namespace renderer {

OpenGLContext::OpenGLContext() {
}

OpenGLContext::~OpenGLContext() {
  shutdown();
}

bool OpenGLContext::initialize() {
#ifdef ENABLE_OPENGL
  // For now, we'll use SDL's OpenGL context creation
  // This will be integrated with SDLWindow
  initialized_ = true;
  return true;
#else
  std::cout << "[OpenGLContext] OpenGL not enabled in this build" << std::endl;
  return false;
#endif
}

void OpenGLContext::shutdown() {
#ifdef ENABLE_OPENGL
  if (context_) {
    // Platform-specific cleanup
    context_ = nullptr;
  }
  initialized_ = false;
#endif
}

void OpenGLContext::makeCurrent() {
#ifdef ENABLE_OPENGL
  if (context_) {
    // Platform-specific make current
  }
#endif
}

void OpenGLContext::swapBuffers() {
#ifdef ENABLE_OPENGL
  if (context_) {
    // Platform-specific swap buffers
  }
#endif
}

} // namespace renderer
} // namespace xiaopeng