#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// Canvas and Color live in the xiaopeng::renderer namespace
#include <renderer/canvas.hpp>

namespace xiaopeng {
namespace layout {

// Base class for all form controls
class FormControl {
public:
  FormControl(const std::string &id, int x = 0, int y = 0, int w = 100,
              int h = 24)
      : id_(id), x_(x), y_(y), w_(w), h_(h) {}
  virtual ~FormControl() = default;

  virtual void paint(xiaopeng::renderer::Canvas *canvas) = 0;

  // Input-like semantics
  virtual void insertChar(char /*c*/) { /* default no-op */ }
  virtual void backspace() { /* default no-op */ }
  void setPosition(int x, int y) {
    x_ = x;
    y_ = y;
  }
  void setSize(int w, int h) {
    w_ = w;
    h_ = h;
  }

  void setId(const std::string &id) { id_ = id; }
  const std::string &id() const { return id_; }

  void setFocused(bool f) { focused_ = f; }
  bool isFocused() const { return focused_; }

  // Simple accessors for rendering helpers
  int x() const { return x_; }
  int y() const { return y_; }
  int w() const { return w_; }
  int h() const { return h_; }

  void focus() { focused_ = true; }
  void blur() { focused_ = false; }

private:
  std::string id_;
  int x_ = 0;
  int y_ = 0;
  int w_ = 100;
  int h_ = 24;
  bool focused_ = false;
};

// Text input control
class TextInput : public FormControl {
public:
  TextInput(const std::string &id) : FormControl(id, 0, 0, 260, 28) {}
  std::string value;
  void paint(xiaopeng::renderer::Canvas *canvas) override;
  void insertChar(char c) override;
  void backspace() override;
  // Simple font state for rendering
  std::string fontFamily_ = "Arial";
  int fontSize_ = 16;
};

// Text area control
// Text area control
class TextArea : public FormControl {
public:
  TextArea(const std::string &id) : FormControl(id, 0, 0, 260, 96) {}
  std::string value;
  void paint(xiaopeng::renderer::Canvas *canvas) override;
  void insertChar(char c) override;
  void backspace() override;
  // Font state
  std::string fontFamily_ = "Arial";
  int fontSize_ = 14;
};

// Simple button control
// Simple button control
class Button : public FormControl {
public:
  Button(const std::string &id, const std::string &label)
      : FormControl(id, 0, 0, 120, 32), label_(label) {}
  std::string label_;
  void paint(xiaopeng::renderer::Canvas *canvas) override;
  int fontSize_ = 14;
  std::string fontFamily_ = "Arial";
};

// Manager for all form controls
class FormManager {
public:
  FormManager() = default;

  // Replace/add a control; returns raw pointer for binding
  FormControl *add(std::unique_ptr<FormControl> ctrl) {
    FormControl *ptr = ctrl.get();
    m_controls.emplace_back(std::move(ctrl));
    return ptr;
  }

  void clear() {
    m_controls.clear();
    m_byDomId.clear();
    m_focused = nullptr;
  }

  void renderAll(renderer::Canvas &canvas) {
    for (auto &c : m_controls) {
      if (c)
        c->paint(&canvas);
    }
  }

  void focusDomId(const std::string &domId) {
    auto it = m_byDomId.find(domId);
    if (it != m_byDomId.end()) {
      if (m_focused && m_focused != it->second) m_focused->blur();
      m_focused = it->second;
      if (m_focused) m_focused->focus();
    }
  }

  void registerDomControl(const std::string &domId, FormControl *ctrl) {
    if (!ctrl) return;
    m_byDomId[domId] = ctrl;
  }

  void focusNext() {
    if (m_controls.empty()) return;
    // Find current focus index
    size_t idx = 0;
    if (m_focused) {
      for (size_t i = 0; i < m_controls.size(); ++i) {
        if (m_controls[i].get() == m_focused) {
          idx = i;
          break;
        }
      }
    }
    if (m_focused) m_focused->blur();
    idx = (idx + 1) % m_controls.size();
    m_focused = m_controls[idx].get();
    if (m_focused) m_focused->focus();
  }

  bool hasFocused() const { return m_focused != nullptr; }
  void insertCharToFocused(char c) {
    if (m_focused) m_focused->insertChar(c);
  }
  void backspaceInFocused() {
    if (m_focused) m_focused->backspace();
  }

private:
  std::vector<std::unique_ptr<FormControl>> m_controls;
  std::unordered_map<std::string, FormControl *> m_byDomId;
  FormControl *m_focused = nullptr;
};

} // namespace layout
} // namespace xiaopeng
