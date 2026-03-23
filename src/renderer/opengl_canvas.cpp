#include "renderer/opengl_canvas.hpp"
#include "renderer/font_manager.hpp"
#include "renderer/text_shaper.hpp"
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>

#ifdef ENABLE_OPENGL
// Include OpenGL headers via SDL for portability
#ifdef ENABLE_SDL
#include <SDL.h>
#include <SDL_opengl.h>
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#endif

// Function pointers for OpenGL extensions
static PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = nullptr;
static PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = nullptr;
static PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = nullptr;
static PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = nullptr;
static PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = nullptr;
static PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = nullptr;
static PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = nullptr;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = nullptr;
static PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = nullptr;
static PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = nullptr;

// Helper function to load OpenGL extensions
static void loadOpenGLExtensions() {
  glGenFramebuffers =
      (PFNGLGENFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glGenFramebuffers");
  glBindFramebuffer =
      (PFNGLBINDFRAMEBUFFERPROC)SDL_GL_GetProcAddress("glBindFramebuffer");
  glGenRenderbuffers =
      (PFNGLGENRENDERBUFFERSPROC)SDL_GL_GetProcAddress("glGenRenderbuffers");
  glBindRenderbuffer =
      (PFNGLBINDRENDERBUFFERPROC)SDL_GL_GetProcAddress("glBindRenderbuffer");
  glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)SDL_GL_GetProcAddress(
      "glRenderbufferStorage");
  glFramebufferRenderbuffer =
      (PFNGLFRAMEBUFFERRENDERBUFFERPROC)SDL_GL_GetProcAddress(
          "glFramebufferRenderbuffer");
  glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)SDL_GL_GetProcAddress(
      "glFramebufferTexture2D");
  glCheckFramebufferStatus =
      (PFNGLCHECKFRAMEBUFFERSTATUSPROC)SDL_GL_GetProcAddress(
          "glCheckFramebufferStatus");
  glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)SDL_GL_GetProcAddress(
      "glDeleteFramebuffers");
  glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)SDL_GL_GetProcAddress(
      "glDeleteRenderbuffers");
}
#endif

namespace xiaopeng {
namespace renderer {

OpenGLCanvas::OpenGLCanvas(int width, int height)
    : width_(width), height_(height) {
  // Always initialize fallback buffer since some methods (like drawImage and
  // drawText) still use software fallback even when OpenGL is enabled.
  fallbackBuffer_.resize(width * height, 0);

#ifdef ENABLE_OPENGL
  initializeOpenGL();
#endif
}

OpenGLCanvas::~OpenGLCanvas() {
#ifdef ENABLE_OPENGL
  cleanupOpenGL();
#endif
}

void OpenGLCanvas::initializeOpenGL() {
#ifdef ENABLE_OPENGL
  // Load OpenGL extensions
  loadOpenGLExtensions();

  // Check if required extensions are available
  if (!glGenFramebuffers || !glBindFramebuffer) {
    std::cerr << "Required OpenGL extensions not available" << std::endl;
    return;
  }

  // Create framebuffer
  glGenFramebuffers(1, &framebuffer_);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

  // Create renderbuffer for depth/stencil (optional)
  glGenRenderbuffers(1, &renderbuffer_);
  glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width_, height_);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, renderbuffer_);

  // Create texture for rendering
  glGenTextures(1, &texture_);
  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Attach texture to framebuffer
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture_, 0);

  // Check framebuffer status
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Framebuffer incomplete: " << status << std::endl;
  }

  // Unbind
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

void OpenGLCanvas::cleanupOpenGL() {
#ifdef ENABLE_OPENGL
  if (texture_) {
    glDeleteTextures(1, &texture_);
    texture_ = 0;
  }
  if (framebuffer_) {
    glDeleteFramebuffers(1, &framebuffer_);
    framebuffer_ = 0;
  }
  if (renderbuffer_) {
    glDeleteRenderbuffers(1, &renderbuffer_);
    renderbuffer_ = 0;
  }
#endif
}

void OpenGLCanvas::resize(int width, int height) {
  width_ = width;
  height_ = height;

#ifdef ENABLE_OPENGL
  // Recreate OpenGL resources
  cleanupOpenGL();
  initializeOpenGL();
#else
  // Software fallback
  fallbackBuffer_.resize(width * height, 0);
#endif
}

void OpenGLCanvas::clear(Color color) {
#ifdef ENABLE_OPENGL
  // Bind framebuffer and clear
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  glClearColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f,
               color.a / 255.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
#else
  // Software fallback
  uint32_t val = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
  std::fill(fallbackBuffer_.begin(), fallbackBuffer_.end(), val);
#endif
}

void OpenGLCanvas::fillRect(int x, int y, int width, int height, Color color) {
#ifdef ENABLE_OPENGL
  // For GPU acceleration, we'll use a simple approach:
  // 1. Bind framebuffer
  // 2. Use OpenGL to draw a rectangle
  // 3. For now, we'll use immediate mode for simplicity

  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

  // Setup 2D orthographic projection for pixel-perfect rendering
  glViewport(0, 0, width_, height_);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, width_, height_, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Enable blending for alpha support
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Set color
  glColor4f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f,
            color.a / 255.0f);

  // Draw rectangle
  glBegin(GL_QUADS);
  glVertex2f(x, y);
  glVertex2f(x + width, y);
  glVertex2f(x + width, y + height);
  glVertex2f(x, y + height);
  glEnd();

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
#else
  fillRectSoftware(x, y, width, height, color);
#endif
}

void OpenGLCanvas::drawRect(int x, int y, int width, int height, Color color) {
  // Draw rectangle border
  fillRect(x, y, width, 1, color);              // Top
  fillRect(x, y + height - 1, width, 1, color); // Bottom
  fillRect(x, y, 1, height, color);             // Left
  fillRect(x + width - 1, y, 1, height, color); // Right
}

void OpenGLCanvas::drawText(int x, int y, const std::string &text, Color color,
                            int scale) {
#ifdef ENABLE_OPENGL
  // For now, use software rendering for text
  // In a full implementation, we would use OpenGL text rendering
  // For now, we'll fall back to software rendering
  fillRectSoftware(x, y, text.length() * 8 * scale, 8 * scale, color);
#else
  // Software fallback
  int curX = x;
  int curY = y;

  for (char c : text) {
    unsigned char uc = static_cast<unsigned char>(c);
    if (uc < 0x20 || uc > 0x7E) {
      uc = '?';
    }

    // Simple bitmap font rendering (simplified)
    for (int row = 0; row < 8; ++row) {
      for (int col = 0; col < 8; ++col) {
        if (scale == 1) {
          setPixel(curX + col, curY + row, color);
        } else {
          fillRectSoftware(curX + col * scale, curY + row * scale, scale, scale,
                           color);
        }
      }
    }

    curX += 8 * scale;
  }
#endif
}

void OpenGLCanvas::setFont(const std::string &family, int size) {
  currentFontFamily_ = family;
  currentFontSize_ = size;
}

void OpenGLCanvas::drawTextVector(int x, int y, const std::string &text,
                                  Color color) {
#ifdef ENABLE_OPENGL
  // For now, use software rendering for vector text
  // In a full implementation, we would use OpenGL text rendering
  fillRectSoftware(x, y, text.length() * 8, 8, color);
#else
  // Software fallback
  fillRectSoftware(x, y, text.length() * 8, 8, color);
#endif
}

void OpenGLCanvas::drawImage(const Image &image, int x, int y, int width,
                             int height) {
#ifdef ENABLE_OPENGL
  (void)image; // TODO: Implement GPU texture upload for images
  // For now, use software rendering for images (placeholder red box)
  fillRectSoftware(x, y, width, height, Color{255, 0, 0, 255});
#else
  // Software fallback
  if (!image.isValid())
    return;

  // Simple nearest neighbor scaling
  for (int j = 0; j < height; ++j) {
    int dstY = y + j;
    if (dstY < 0 || dstY >= height_)
      continue;

    for (int i = 0; i < width; ++i) {
      int dstX = x + i;
      if (dstX < 0 || dstX >= width_)
        continue;

      // Map to source coordinates
      int srcX = (i * image.width()) / width;
      int srcY = (j * image.height()) / height;

      // Clamp source coords
      if (srcX >= image.width())
        srcX = image.width() - 1;
      if (srcY >= image.height())
        srcY = image.height() - 1;

      const uint8_t *srcData = image.data();
      int srcIndex = (srcY * image.width() + srcX) * image.channels();

      Color color;
      color.r = srcData[srcIndex];
      color.g = srcData[srcIndex + 1];
      color.b = srcData[srcIndex + 2];

      if (image.channels() == 4) {
        color.a = srcData[srcIndex + 3];
        blendPixel(dstX, dstY, color);
      } else {
        color.a = 255;
        setPixel(dstX, dstY, color);
      }
    }
  }
#endif
}

void OpenGLCanvas::bindTexture() {
#ifdef ENABLE_OPENGL
  if (texture_) {
    glBindTexture(GL_TEXTURE_2D, texture_);
  }
#endif
}

void OpenGLCanvas::unbindTexture() {
#ifdef ENABLE_OPENGL
  glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

void OpenGLCanvas::updateTexture(const void *data) {
#ifdef ENABLE_OPENGL
  if (texture_) {
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_RGBA,
                    GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
#endif
}

// Software rendering fallback methods
void OpenGLCanvas::setPixel(int x, int y, Color color) {
  if (x < 0 || x >= width_ || y < 0 || y >= height_)
    return;

  uint32_t val = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
  fallbackBuffer_[y * width_ + x] = val;
}

void OpenGLCanvas::blendPixel(int x, int y, Color color) {
  if (x < 0 || x >= width_ || y < 0 || y >= height_)
    return;

  if (color.a == 0)
    return;
  if (color.a == 255) {
    setPixel(x, y, color);
    return;
  }

  uint32_t bg = fallbackBuffer_[y * width_ + x];
  uint8_t bg_b = bg & 0xFF;
  uint8_t bg_g = (bg >> 8) & 0xFF;
  uint8_t bg_r = (bg >> 16) & 0xFF;
  uint8_t bg_a = (bg >> 24) & 0xFF;

  float alpha = color.a / 255.0f;
  float inv_alpha = 1.0f - alpha;

  uint8_t r = static_cast<uint8_t>(color.r * alpha + bg_r * inv_alpha);
  uint8_t g = static_cast<uint8_t>(color.g * alpha + bg_g * inv_alpha);
  uint8_t b = static_cast<uint8_t>(color.b * alpha + bg_b * inv_alpha);
  uint8_t a = static_cast<uint8_t>(color.a + bg_a * inv_alpha);

  uint32_t val = (a << 24) | (r << 16) | (g << 8) | b;
  fallbackBuffer_[y * width_ + x] = val;
}

void OpenGLCanvas::fillRectSoftware(int x, int y, int width, int height,
                                    Color color) {
  for (int j = y; j < y + height; ++j) {
    for (int i = x; i < x + width; ++i) {
      setPixel(i, j, color);
    }
  }
}

} // namespace renderer
} // namespace xiaopeng