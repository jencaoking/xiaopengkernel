#pragma once

#include "types.hpp"
#include "error.hpp"
#include "url.hpp"
#include "http_message.hpp"
#include "cache.hpp"
#include "http_client.hpp"
#include "connection_pool.hpp"
#include <shared_mutex>

namespace xiaopeng {
namespace loader {

class ResourceLoader;

enum class ResourceType {
    Unknown,
    Html,
    Css,
    JavaScript,
    Image,
    Font,
    Media,
    Wasm,
    Json,
    Xml,
    Text,
    Binary
};

enum class ResourcePriority {
    Highest,
    High,
    Medium,
    Low,
    Lowest
};

enum class LoadState {
    Pending,
    Loading,
    Loaded,
    Failed,
    Cancelled
};

struct Resource {
    Url url;
    ResourceType type = ResourceType::Unknown;
    ResourcePriority priority = ResourcePriority::Medium;
    LoadState state = LoadState::Pending;
    
    ByteBuffer data;
    std::string mimeType;
    std::string charset;
    std::string encoding;
    
    HttpHeaders responseHeaders;
    HttpStatusCode statusCode = HttpStatusCode::Ok;
    
    TimePoint startTime;
    TimePoint endTime;
    Duration loadTime{0};
    
    std::uint64_t size = 0;
    std::uint64_t transferred = 0;
    
    bool fromCache = false;
    bool cancelled = false;
    ErrorCode error = ErrorCode::Success;
    std::string errorMessage;
    
    std::string text() const {
        return std::string(data.begin(), data.end());
    }
    
    std::string_view textView() const {
        return std::string_view(reinterpret_cast<const char*>(data.data()), data.size());
    }
    
    std::span<const Byte> bytes() const {
        return data;
    }
    
    bool isLoaded() const { return state == LoadState::Loaded; }
    bool isFailed() const { return state == LoadState::Failed; }
    bool isLoading() const { return state == LoadState::Loading; }
    bool isPending() const { return state == LoadState::Pending; }
    bool isCancelled() const { return state == LoadState::Cancelled; }
};

struct LoadOptions {
    ResourcePriority priority = ResourcePriority::Medium;
    bool useCache = true;
    bool forceReload = false;
    bool followRedirects = true;
    std::chrono::milliseconds timeout{30000};
    std::string accept;
    std::string acceptEncoding;
    std::map<std::string, std::string> headers;
    ByteBuffer postData;
    HttpMethod method = HttpMethod::Get;
    bool preflight = false;
    std::string corsMode;
    std::string integrity;
    std::string referrer;
    std::string referrerPolicy;
};

struct ResourceStats {
    std::uint64_t totalResources = 0;
    std::uint64_t loadedResources = 0;
    std::uint64_t failedResources = 0;
    std::uint64_t cachedResources = 0;
    std::uint64_t totalBytes = 0;
    std::uint64_t transferredBytes = 0;
    Duration totalTime{0};
    Duration averageTime{0};
};

inline ResourceType detectResourceType(const std::string& mimeType) {
    std::string lower = mimeType;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (lower.find("text/html") != std::string::npos) return ResourceType::Html;
    if (lower.find("text/css") != std::string::npos) return ResourceType::Css;
    if (lower.find("javascript") != std::string::npos) return ResourceType::JavaScript;
    if (lower.find("image/") != std::string::npos) return ResourceType::Image;
    if (lower.find("font/") != std::string::npos) return ResourceType::Font;
    if (lower.find("audio/") != std::string::npos || 
        lower.find("video/") != std::string::npos) return ResourceType::Media;
    if (lower.find("application/wasm") != std::string::npos) return ResourceType::Wasm;
    if (lower.find("application/json") != std::string::npos) return ResourceType::Json;
    if (lower.find("application/xml") != std::string::npos || 
        lower.find("text/xml") != std::string::npos) return ResourceType::Xml;
    if (lower.find("text/") != std::string::npos) return ResourceType::Text;
    
    return ResourceType::Unknown;
}

inline ResourceType detectResourceTypeFromUrl(const Url& url) {
    std::string path = url.path();
    std::transform(path.begin(), path.end(), path.begin(), ::tolower);
    
    if (path.ends_with(".html") || path.ends_with(".htm")) return ResourceType::Html;
    if (path.ends_with(".css")) return ResourceType::Css;
    if (path.ends_with(".js") || path.ends_with(".mjs")) return ResourceType::JavaScript;
    if (path.ends_with(".png") || path.ends_with(".jpg") || path.ends_with(".jpeg") ||
        path.ends_with(".gif") || path.ends_with(".webp") || path.ends_with(".svg") ||
        path.ends_with(".ico") || path.ends_with(".bmp")) return ResourceType::Image;
    if (path.ends_with(".woff") || path.ends_with(".woff2") || path.ends_with(".ttf") ||
        path.ends_with(".otf") || path.ends_with(".eot")) return ResourceType::Font;
    if (path.ends_with(".mp3") || path.ends_with(".mp4") || path.ends_with(".webm") ||
        path.ends_with(".ogg") || path.ends_with(".wav") || path.ends_with(".avi")) return ResourceType::Media;
    if (path.ends_with(".wasm")) return ResourceType::Wasm;
    if (path.ends_with(".json")) return ResourceType::Json;
    if (path.ends_with(".xml")) return ResourceType::Xml;
    
    return ResourceType::Unknown;
}

inline std::string resourceTypeToMime(ResourceType type) {
    switch (type) {
        case ResourceType::Html: return "text/html";
        case ResourceType::Css: return "text/css";
        case ResourceType::JavaScript: return "application/javascript";
        case ResourceType::Image: return "image/*";
        case ResourceType::Font: return "font/*";
        case ResourceType::Media: return "video/*";
        case ResourceType::Wasm: return "application/wasm";
        case ResourceType::Json: return "application/json";
        case ResourceType::Xml: return "application/xml";
        case ResourceType::Text: return "text/plain";
        default: return "*/*";
    }
}

}
}
