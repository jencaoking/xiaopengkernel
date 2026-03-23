#include "renderer/image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO // We load from memory
#include "../third_party/stb_image.h"

namespace xiaopeng {
namespace renderer {

Image::Image(const std::vector<uint8_t> &data) {
  // Force 4 channels (RGBA)
  data_ = stbi_load_from_memory(data.data(), static_cast<int>(data.size()),
                                &width_, &height_, &channels_, 4);
  if (data_) {
    channels_ = 4;
  }
}

Image::~Image() {
  if (data_) {
    stbi_image_free(data_);
  }
}

} // namespace renderer
} // namespace xiaopeng
