#pragma once

#include "../dom/html_parser.hpp"
#include "error.hpp"
#include "resource.hpp"
#include "types.hpp"
#include <functional>
#include <mutex>

namespace xiaopeng {
namespace loader {

class ResourceHandler {
public:
  virtual ~ResourceHandler() = default;

  virtual bool canHandle(ResourceType type) const = 0;

  virtual Result<void> process(Resource &resource) = 0;

  virtual std::vector<std::string>
  extractDependencies(const Resource &resource) {
    (void)resource;
    return {};
  }

  virtual std::string name() const = 0;
};

class HtmlHandler : public ResourceHandler {
public:
  struct ParseResult {
    bool success = false;
    std::string title;
    std::string description;
    std::string baseHref;
    std::vector<std::string> links;
    std::vector<std::string> scripts;
    std::vector<std::string> stylesheets;
    std::vector<std::string> images;
    std::vector<dom::ParseError> errors;
    std::shared_ptr<dom::Document> document;
  };

  bool canHandle(ResourceType type) const override {
    return type == ResourceType::Html;
  }

  Result<void> process(Resource &resource) override {
    if (resource.data.empty()) {
      return Result<void>(ErrorCode::ResourceParseError, "Empty HTML content");
    }

    dom::ParserOptions options;
    if (!resource.charset.empty()) {
      options.encoding = resource.charset;
    }
    options.baseUrl = resource.url.toString();

    dom::HtmlParser parser(options);
    auto result = parser.parse(resource);

    if (!result.ok()) {
      return Result<void>(ErrorCode::HtmlParseError,
                          result.errors.empty() ? "HTML parsing failed"
                                                : result.errors[0].message);
    }

    {
      std::lock_guard<std::mutex> lock(cacheMutex_);
      auto cachedResult = std::make_shared<ParseResult>();
      cachedResult->success = true;
      cachedResult->document = result.document;
      cachedResult->title = dom::HtmlParser::extractTitle(result.document);
      cachedResult->description =
          dom::HtmlParser::extractMetaDescription(result.document);
      cachedResult->baseHref =
          dom::HtmlParser::extractBaseHref(result.document);
      cachedResult->links = dom::HtmlParser::extractLinks(result.document);
      cachedResult->scripts = dom::HtmlParser::extractScripts(result.document);
      cachedResult->stylesheets =
          dom::HtmlParser::extractStylesheets(result.document);
      cachedResult->images = dom::HtmlParser::extractImages(result.document);
      cachedResult->errors = result.errors;

      parseCache_[&resource] = cachedResult;
    }

    return Result<void>();
  }

  std::vector<std::string>
  extractDependencies(const Resource &resource) override {
    std::vector<std::string> dependencies;

    {
      std::lock_guard<std::mutex> lock(cacheMutex_);
      auto it = parseCache_.find(&resource);
      if (it != parseCache_.end() && it->second->success) {
        const auto &result = it->second;
        dependencies.insert(dependencies.end(), result->scripts.begin(),
                            result->scripts.end());
        dependencies.insert(dependencies.end(), result->stylesheets.begin(),
                            result->stylesheets.end());
        dependencies.insert(dependencies.end(), result->images.begin(),
                            result->images.end());

        for (const auto &link : result->links) {
          if (std::find(dependencies.begin(), dependencies.end(), link) ==
              dependencies.end()) {
            dependencies.push_back(link);
          }
        }
        return dependencies;
      }
    }

    if (resource.data.empty()) {
      return dependencies;
    }

    std::string_view content = resource.textView();
    extractLinks(content, "src=\"", "\"", dependencies);
    extractLinks(content, "href=\"", "\"", dependencies);
    extractLinks(content, "src='", "'", dependencies);
    extractLinks(content, "href='", "'", dependencies);

    return dependencies;
  }

  std::shared_ptr<ParseResult> getParseResult(const Resource &resource) const {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    auto it = parseCache_.find(&resource);
    if (it != parseCache_.end()) {
      return it->second;
    }
    return nullptr;
  }

  std::shared_ptr<dom::Document> getDocument(const Resource &resource) const {
    auto result = getParseResult(resource);
    return result ? result->document : nullptr;
  }

  void clearCache() {
    std::lock_guard<std::mutex> lock(cacheMutex_);
    parseCache_.clear();
  }

  std::string name() const override { return "HtmlHandler"; }

private:
  void extractLinks(std::string_view content, const std::string &prefix,
                    const std::string &suffix,
                    std::vector<std::string> &links) {
    size_t pos = 0;
    while ((pos = content.find(prefix, pos)) != std::string_view::npos) {
      pos += prefix.length();
      size_t end = content.find(suffix, pos);
      if (end != std::string_view::npos) {
        std::string link(content.substr(pos, end - pos));
        if (!link.empty() && link[0] != '#' && link.find("javascript:") != 0 &&
            link.find("data:") != 0 && link.find("mailto:") != 0) {
          links.push_back(std::move(link));
        }
        pos = end + suffix.length();
      } else {
        break;
      }
    }
  }

  mutable std::mutex cacheMutex_;
  mutable std::unordered_map<const Resource *, std::shared_ptr<ParseResult>>
      parseCache_;
};

class CssHandler : public ResourceHandler {
public:
  bool canHandle(ResourceType type) const override {
    return type == ResourceType::Css;
  }

  Result<void> process(Resource &resource) override {
    if (resource.data.empty()) {
      return Result<void>(ErrorCode::ResourceParseError, "Empty CSS content");
    }

    return Result<void>();
  }

  std::vector<std::string>
  extractDependencies(const Resource &resource) override {
    std::vector<std::string> dependencies;
    std::string_view content = resource.textView();

    extractUrls(content, "url(", ")", dependencies);
    extractUrls(content, "@import \"", "\"", dependencies);
    extractUrls(content, "@import '", "'", dependencies);
    extractUrls(content, "@import url(", ")", dependencies);

    return dependencies;
  }

  std::string name() const override { return "CssHandler"; }

private:
  void extractUrls(std::string_view content, const std::string &prefix,
                   const std::string &suffix, std::vector<std::string> &urls) {
    size_t pos = 0;
    while ((pos = content.find(prefix, pos)) != std::string_view::npos) {
      pos += prefix.length();

      bool hasQuote = false;
      char quoteChar = '"';
      if (pos < content.size() &&
          (content[pos] == '"' || content[pos] == '\'')) {
        hasQuote = true;
        quoteChar = content[pos];
        pos++;
      }

      size_t end;
      if (hasQuote) {
        end = content.find(quoteChar, pos);
      } else {
        end = content.find(suffix, pos);
      }

      if (end != std::string_view::npos) {
        std::string url(content.substr(pos, end - pos));
        if (!url.empty() && url[0] != '#' && url.find("data:") != 0) {
          urls.push_back(std::move(url));
        }
        pos = end + 1;
        if (hasQuote) {
          auto suffixPos = content.find(suffix, end);
          if (suffixPos == std::string_view::npos)
            break;
          pos = suffixPos + 1;
        }
      } else {
        break;
      }
    }
  }
};

class JavaScriptHandler : public ResourceHandler {
public:
  bool canHandle(ResourceType type) const override {
    return type == ResourceType::JavaScript;
  }

  Result<void> process(Resource &resource) override {
    if (resource.data.empty()) {
      return Result<void>();
    }

    return Result<void>();
  }

  std::vector<std::string>
  extractDependencies(const Resource &resource) override {
    std::vector<std::string> dependencies;
    std::string_view content = resource.textView();

    extractDynamicImports(content, dependencies);
    extractImportStatements(content, dependencies);

    return dependencies;
  }

  std::string name() const override { return "JavaScriptHandler"; }

private:
  void extractDynamicImports(std::string_view content,
                             std::vector<std::string> &imports) {
    size_t pos = 0;
    while ((pos = content.find("import(", pos)) != std::string_view::npos) {
      pos += 7;

      while (pos < content.size() &&
             std::isspace(static_cast<unsigned char>(content[pos]))) {
        pos++;
      }

      if (pos < content.size() &&
          (content[pos] == '"' || content[pos] == '\'')) {
        char quote = content[pos];
        pos++;
        size_t end = content.find(quote, pos);
        if (end != std::string_view::npos) {
          std::string url(content.substr(pos, end - pos));
          if (!url.empty()) {
            imports.push_back(std::move(url));
          }
        }
      }
    }
  }

  void extractImportStatements(std::string_view content,
                               std::vector<std::string> &imports) {
    size_t pos = 0;
    while ((pos = content.find("from", pos)) != std::string_view::npos) {
      pos += 4;

      while (pos < content.size() &&
             std::isspace(static_cast<unsigned char>(content[pos]))) {
        pos++;
      }

      if (pos < content.size() &&
          (content[pos] == '"' || content[pos] == '\'')) {
        char quote = content[pos];
        pos++;
        size_t end = content.find(quote, pos);
        if (end != std::string_view::npos) {
          std::string url(content.substr(pos, end - pos));
          if (!url.empty()) {
            imports.push_back(std::move(url));
          }
        }
      }
    }
  }
};

class ImageHandler : public ResourceHandler {
public:
  struct ImageInfo {
    int width = 0;
    int height = 0;
    int channels = 0;
    size_t dataSize = 0;
    std::string format;
  };

  bool canHandle(ResourceType type) const override {
    return type == ResourceType::Image;
  }

  Result<void> process(Resource &resource) override {
    if (resource.data.empty()) {
      return Result<void>(ErrorCode::ResourceParseError, "Empty image data");
    }

    auto info = detectImageFormat(resource.data);
    if (info.format.empty()) {
      return Result<void>(ErrorCode::ImageDecodeError, "Unknown image format");
    }

    return Result<void>();
  }

  std::string name() const override { return "ImageHandler"; }

  static ImageInfo detectImageFormat(const ByteBuffer &data) {
    ImageInfo info;

    if (data.empty())
      return info;

    if (data.size() >= 8 && data[0] == 0x89 && data[1] == 0x50 &&
        data[2] == 0x4E && data[3] == 0x47 && data[4] == 0x0D &&
        data[5] == 0x0A && data[6] == 0x1A && data[7] == 0x0A) {
      info.format = "png";
      if (data.size() >= 24) {
        info.width =
            (data[16] << 24) | (data[17] << 16) | (data[18] << 8) | data[19];
        info.height =
            (data[20] << 24) | (data[21] << 16) | (data[22] << 8) | data[23];
      }
    } else if (data.size() >= 3 && data[0] == 0xFF && data[1] == 0xD8 &&
               data[2] == 0xFF) {
      info.format = "jpeg";
    } else if (data.size() >= 12 && data[0] == 'R' && data[1] == 'I' &&
               data[2] == 'F' && data[3] == 'F' && data[8] == 'W' &&
               data[9] == 'E' && data[10] == 'B' && data[11] == 'P') {
      info.format = "webp";
    } else if (data.size() >= 6 && data[0] == 'G' && data[1] == 'I' &&
               data[2] == 'F' && data[3] == '8' &&
               (data[4] == '7' || data[4] == '9') && data[5] == 'a') {
      info.format = "gif";
    } else if (data.size() >= 2 && ((data[0] == 0x49 && data[1] == 0x49) ||
                                    (data[0] == 0x4D && data[1] == 0x4D))) {
      info.format = "tiff";
    } else if (data.size() >= 4 && data[0] == 0x00 && data[1] == 0x00 &&
               data[2] == 0x01 && data[3] == 0x00) {
      info.format = "ico";
    } else if (data.size() >= 5 && data[0] == '<' && data[1] == 's' &&
               data[2] == 'v' && data[3] == 'g') {
      info.format = "svg";
    }

    info.dataSize = data.size();
    return info;
  }

  static bool isAnimated(const ByteBuffer &data) {
    if (data.size() < 8)
      return false;

    if (data[0] == 'G' && data[1] == 'I' && data[2] == 'F') {
      size_t pos = 6;
      while (pos + 1 < data.size()) {
        if (data[pos] == 0x21 && data[pos + 1] == 0xF9) {
          return true;
        }
        pos++;
      }
    }

    if (data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E &&
        data[3] == 0x47) {
      size_t pos = 8;
      while (pos + 8 < data.size()) {
        if (data[pos + 4] == 'a' && data[pos + 5] == 'c' &&
            data[pos + 6] == 'T' && data[pos + 7] == 'L') {
          return true;
        }
        pos += 4 +
               ((data[pos] << 24) | (data[pos + 1] << 16) |
                (data[pos + 2] << 8) | data[pos + 3]) +
               4;
      }
    }

    return false;
  }
};

class ResourceHandlerRegistry {
public:
  void registerHandler(Ptr<ResourceHandler> handler) {
    handlers_.push_back(std::move(handler));
  }

  ResourceHandler *getHandler(ResourceType type) const {
    for (const auto &handler : handlers_) {
      if (handler->canHandle(type)) {
        return handler.get();
      }
    }
    return nullptr;
  }

  Result<void> process(Resource &resource) const {
    auto *handler = getHandler(resource.type);
    if (handler) {
      return handler->process(resource);
    }
    return Result<void>();
  }

  std::vector<std::string> extractDependencies(const Resource &resource) const {
    auto *handler = getHandler(resource.type);
    if (handler) {
      return handler->extractDependencies(resource);
    }
    return {};
  }

private:
  std::vector<Ptr<ResourceHandler>> handlers_;
};

} // namespace loader
} // namespace xiaopeng
