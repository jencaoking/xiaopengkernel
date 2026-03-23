#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>


namespace xiaopeng {
namespace renderer {

class Image {
public:
  Image(const std::vector<uint8_t> &data);
  ~Image();

  int width() const { return width_; }
  int height() const { return height_; }
  int channels() const { return channels_; }
  const uint8_t *data() const { return data_; }
  bool isValid() const { return data_ != nullptr; }

private:
  int width_ = 0;
  int height_ = 0;
  int channels_ = 0;
  uint8_t *data_ = nullptr;
};

} // namespace renderer
} // namespace xiaopeng
