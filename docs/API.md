# XiaopengKernel Loader API 文档

## 目录

1. [核心类型](#核心类型)
2. [错误处理](#错误处理)
3. [URL 解析](#url-解析)
4. [HTTP 消息](#http-消息)
5. [HTTP 客户端](#http-客户端)
6. [缓存系统](#缓存系统)
7. [连接池](#连接池)
8. [资源处理](#资源处理)
9. [加载调度](#加载调度)
10. [安全机制](#安全机制)
11. [Loader 接口](#loader-接口)

---

## 核心类型

### 命名空间

所有类型定义在 `xiaopeng::loader` 命名空间中。

```cpp
namespace xiaopeng::loader {
    // 所有类型...
}
```

### 基础类型别名

```cpp
using Byte = std::uint8_t;                    // 字节类型
using ByteBuffer = std::vector<Byte>;         // 字节缓冲区
using StringView = std::string_view;          // 字符串视图
using TimePoint = std::chrono::steady_clock::time_point;  // 时间点
using Duration = std::chrono::milliseconds;   // 时间间隔

template<typename T>
using Ptr = std::shared_ptr<T>;               // 共享指针

template<typename T>
using Future = std::future<T>;                // Future 类型

template<typename K, typename V, typename Hash = std::hash<K>>
using HashMap = std::unordered_map<K, V, Hash>;  // 哈希映射
```

---

## 错误处理

### ErrorCode

错误码枚举，定义所有可能的错误类型。

```cpp
enum class ErrorCode : int {
    // 通用错误
    Success = 0,
    Unknown = 1,
    InvalidArgument = 2,
    InvalidState = 3,
    NotImplemented = 4,
    
    // URL 错误
    UrlParseError = 100,
    UrlInvalidScheme = 101,
    UrlInvalidHost = 102,
    UrlInvalidPort = 103,
    
    // 网络错误
    NetworkError = 200,
    ConnectionFailed = 201,
    ConnectionTimeout = 202,
    ConnectionRefused = 203,
    DnsResolutionFailed = 204,
    SslHandshakeFailed = 205,
    SslCertificateError = 206,
    
    // HTTP 错误
    HttpRequestError = 300,
    HttpResponseError = 301,
    HttpInvalidHeader = 302,
    HttpInvalidStatus = 303,
    HttpRedirectLimitExceeded = 304,
    HttpVersionNotSupported = 305,
    
    // 资源错误
    ResourceNotFound = 400,
    ResourceLoadFailed = 401,
    ResourceParseError = 402,
    ResourceDecodeError = 403,
    ResourceTimeout = 404,
    ResourceCancelled = 405,
    
    // 缓存错误
    CacheError = 500,
    CacheMiss = 501,
    CacheExpired = 502,
    CacheInvalid = 503,
    
    // 安全错误
    SecurityError = 600,
    CorsViolation = 601,
    SameOriginViolation = 602,
    MixedContentBlocked = 603,
    InsecureContentBlocked = 604,
    
    // 解析错误
    ParserError = 700,
    HtmlParseError = 701,
    CssParseError = 702,
    JsParseError = 703,
    ImageDecodeError = 704,
};
```

### LoaderException

异常类，用于抛出错误。

```cpp
class LoaderException : public std::runtime_error {
public:
    explicit LoaderException(ErrorCode code, const std::string& message = "");
    
    ErrorCode code() const noexcept;  // 获取错误码
    const char* what() const noexcept; // 获取错误消息
};
```

### Result<T>

结果类型，用于返回可能失败的操作结果。

```cpp
template<typename T>
class Result {
public:
    Result(T value);                          // 成功结果
    Result(ErrorCode error, std::string message = ""); // 失败结果
    
    bool isOk() const;                        // 是否成功
    bool isErr() const;                       // 是否失败
    explicit operator bool() const;           // 隐式转换为 bool
    
    T& unwrap();                              // 获取值（失败时抛出异常）
    T unwrapOr(T defaultValue) const;         // 获取值或默认值
    
    ErrorCode error() const;                  // 获取错误码
    const std::string& errorMessage() const;  // 获取错误消息
    
    template<typename U>
    Result<U> map(std::function<U(const T&)> f) const;  // 映射值
    
    template<typename U>
    Result<U> andThen(std::function<Result<U>(const T&)> f) const;  // 链式操作
};
```

**示例：**

```cpp
Result<int> divide(int a, int b) {
    if (b == 0) {
        return Result<int>(ErrorCode::InvalidArgument, "Division by zero");
    }
    return a / b;
}

auto result = divide(10, 2);
if (result.isOk()) {
    std::cout << "Result: " << result.unwrap() << std::endl;  // 输出: 5
} else {
    std::cerr << "Error: " << result.errorMessage() << std::endl;
}
```

---

## URL 解析

### UrlScheme

URL 协议枚举。

```cpp
enum class UrlScheme {
    Http,    // http://
    Https,   // https://
    Ftp,     // ftp://
    Ws,      // ws://
    Wss,     // wss://
    Data,    // data:
    Blob,    // blob:
    File,    // file://
    About,   // about:
    Unknown  // 未知协议
};
```

### Url

URL 解析和操作类。

```cpp
class Url {
public:
    Url() = default;
    explicit Url(const std::string& url);
    
    // 解析 URL
    static Result<Url> parse(const std::string& url);
    
    // 从组件构造 URL
    static Url fromParts(
        UrlScheme scheme,
        const std::string& host,
        std::uint16_t port,
        const std::string& path = "/",
        const std::string& query = "",
        const std::string& fragment = "",
        const std::string& userinfo = ""
    );
    
    // 属性访问
    bool isValid() const;
    UrlScheme scheme() const;
    const std::string& schemeString() const;
    const std::string& host() const;
    std::uint16_t port() const;
    std::uint16_t defaultPort() const;
    const std::string& path() const;
    const std::string& query() const;
    const std::string& fragment() const;
    const std::string& userinfo() const;
    
    // 组合属性
    std::string authority() const;        // host:port (含 userinfo)
    std::string pathWithQuery() const;    // /path?query
    std::string toString() const;         // 完整 URL 字符串
    std::string origin() const;           // 协议 + 主机 + 端口
    
    // 安全检查
    bool isSecure() const;                // 是否为安全协议 (https/wss)
    
    // 源比较
    bool sameOrigin(const Url& other) const;
    
    // URL 解析
    Url resolve(const std::string& relative) const;
    
    // URL 编解码
    static std::string encode(const std::string& input);
    static std::string decode(const std::string& input);
    
    // 比较运算符
    bool operator==(const Url& other) const;
    bool operator!=(const Url& other) const;
    bool operator<(const Url& other) const;
};
```

**示例：**

```cpp
// 解析 URL
auto result = Url::parse("https://example.com:8080/path?query=value#fragment");
if (result.isOk()) {
    Url url = result.unwrap();
    std::cout << url.host() << std::endl;      // example.com
    std::cout << url.port() << std::endl;      // 8080
    std::cout << url.origin() << std::endl;    // https://example.com:8080
}

// URL 编码
std::string encoded = Url::encode("hello world");  // hello%20world

// 解析相对 URL
Url base = Url::parse("https://example.com/page/").unwrap();
Url resolved = base.resolve("../other.html");
std::cout << resolved.path() << std::endl;  // /other.html
```

---

## HTTP 消息

### HttpMethod

HTTP 请求方法枚举。

```cpp
enum class HttpMethod {
    Get,       // GET
    Post,      // POST
    Put,       // PUT
    Delete,    // DELETE
    Head,      // HEAD
    Options,   // OPTIONS
    Patch,     // PATCH
    Trace,     // TRACE
    Connect    // CONNECT
};

// 辅助函数
std::string methodToString(HttpMethod method);
HttpMethod stringToMethod(const std::string& str);
```

### HttpHeaders

HTTP 头处理类。

```cpp
class HttpHeaders {
public:
    HttpHeaders() = default;
    
    // 设置头（覆盖同名头）
    void set(const std::string& name, const std::string& value);
    
    // 添加头（保留同名头）
    void add(const std::string& name, const std::string& value);
    
    // 追加头值（用逗号分隔）
    void append(const std::string& name, const std::string& value);
    
    // 获取头值
    std::optional<std::string> get(const std::string& name) const;
    std::vector<std::string> getAll(const std::string& name) const;
    
    // 检查头是否存在
    bool has(const std::string& name) const;
    
    // 删除头
    void remove(const std::string& name);
    
    // 清空所有头
    void clear();
    
    // 容量
    bool empty() const;
    size_t size() const;
    
    // 获取所有头
    const HeaderMap& all() const;
    
    // 转换为字符串
    std::string toString() const;
    
    // 常用头便捷方法
    std::uint64_t contentLength() const;
    void setContentLength(std::uint64_t length);
    std::string contentType() const;
    void setContentType(const std::string& type);
    std::string contentEncoding() const;
    std::string transferEncoding() const;
    bool isChunked() const;
    std::string eTag() const;
    std::string lastModified() const;
    std::string cacheControl() const;
    std::string location() const;
    std::string host() const;
    void setHost(const std::string& host);
    std::string userAgent() const;
    void setUserAgent(const std::string& agent);
    std::string accept() const;
    void setAccept(const std::string& accept);
    std::string acceptEncoding() const;
    void setAcceptEncoding(const std::string& encoding);
    std::string authorization() const;
    void setAuthorization(const std::string& auth);
    void setBearerToken(const std::string& token);
    void setBasicAuth(const std::string& username, const std::string& password);
    std::string cookie() const;
    void setCookie(const std::string& cookie);
    std::vector<std::string> setCookie() const;
    std::string origin() const;
    void setOrigin(const std::string& origin);
    std::string referer() const;
    void setReferer(const std::string& referer);
};
```

### HttpStatusCode

HTTP 状态码枚举。

```cpp
enum class HttpStatusCode : int {
    // 1xx 信息响应
    Continue = 100,
    SwitchingProtocols = 101,
    
    // 2xx 成功
    Ok = 200,
    Created = 201,
    Accepted = 202,
    NoContent = 204,
    PartialContent = 206,
    
    // 3xx 重定向
    MultipleChoices = 300,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    NotModified = 304,
    TemporaryRedirect = 307,
    PermanentRedirect = 308,
    
    // 4xx 客户端错误
    BadRequest = 400,
    Unauthorized = 401,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    RequestTimeout = 408,
    TooManyRequests = 429,
    
    // 5xx 服务器错误
    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
};

// 辅助函数
bool isInformational(HttpStatusCode code);
bool isSuccess(HttpStatusCode code);
bool isRedirect(HttpStatusCode code);
bool isClientError(HttpStatusCode code);
bool isServerError(HttpStatusCode code);
std::string getStatusMessage(HttpStatusCode code);
```

### HttpRequest

HTTP 请求结构体。

```cpp
struct HttpRequest {
    Url url;                                    // 请求 URL
    HttpMethod method = HttpMethod::Get;        // 请求方法
    HttpHeaders headers;                        // 请求头
    ByteBuffer body;                            // 请求体
    std::chrono::milliseconds timeout{30000};  // 超时时间
    bool followRedirects = true;                // 是否跟随重定向
    int maxRedirects = 20;                      // 最大重定向次数
    bool verifySsl = true;                      // 是否验证 SSL
    std::string httpVersion = "HTTP/1.1";       // HTTP 版本
    
    // 静态工厂方法
    static HttpRequest get(const Url& url);
    static HttpRequest post(const Url& url, ByteBuffer body = {});
    static HttpRequest post(const Url& url, const std::string& body);
    static HttpRequest postJson(const Url& url, const std::string& json);
    static HttpRequest put(const Url& url, ByteBuffer body = {});
    static HttpRequest del(const Url& url);
    static HttpRequest head(const Url& url);
    static HttpRequest options(const Url& url);
    
    // 链式配置方法
    HttpRequest& withHeader(const std::string& name, const std::string& value) &;
    HttpRequest&& withHeader(const std::string& name, const std::string& value) &&;
    HttpRequest& withBody(ByteBuffer data) &;
    HttpRequest&& withBody(ByteBuffer data) &&;
    HttpRequest& withTimeout(std::chrono::milliseconds ms) &;
    HttpRequest&& withTimeout(std::chrono::milliseconds ms) &&;
    HttpRequest& withFollowRedirects(bool follow, int maxRedirs = 20) &;
    HttpRequest&& withFollowRedirects(bool follow, int maxRedirs = 20) &&;
    HttpRequest& withSslVerification(bool verify) &;
    HttpRequest&& withSslVerification(bool verify) &&;
    HttpRequest& withHttpVersion(const std::string& version) &;
    HttpRequest&& withHttpVersion(const std::string& version) &&;
};
```

### HttpResponse

HTTP 响应结构体。

```cpp
struct HttpResponse {
    HttpStatusCode statusCode = HttpStatusCode::Ok;  // 状态码
    std::string statusMessage;                       // 状态消息
    std::string httpVersion = "HTTP/1.1";            // HTTP 版本
    HttpHeaders headers;                             // 响应头
    ByteBuffer body;                                 // 响应体
    Url finalUrl;                                    // 最终 URL（重定向后）
    TimePoint receivedAt;                            // 接收时间
    Duration totalTime{0};                           // 总耗时
    Duration dnsTime{0};                             // DNS 解析耗时
    Duration connectTime{0};                         // 连接耗时
    Duration sslTime{0};                             // SSL 握手耗时
    Duration downloadTime{0};                        // 下载耗时
    bool fromCache = false;                          // 是否来自缓存
    bool cancelled = false;                          // 是否被取消
    
    // 辅助方法
    bool ok() const;                    // 是否成功 (2xx)
    bool isRedirect() const;            // 是否重定向 (3xx)
    std::string text() const;           // 获取文本内容
    std::string_view textView() const;  // 获取文本视图
    std::span<const Byte> bytes() const;// 获取字节视图
    std::string contentType() const;    // 获取 Content-Type
    std::string mimeType() const;       // 获取 MIME 类型
    std::string charset() const;        // 获取字符集
    std::uint64_t contentLength() const;// 获取内容长度
    std::string eTag() const;           // 获取 ETag
    std::string lastModified() const;   // 获取 Last-Modified
    std::string cacheControl() const;   // 获取 Cache-Control
    std::string location() const;       // 获取 Location
    bool isCacheable() const;           // 是否可缓存
    std::optional<std::chrono::seconds> maxAge() const;  // 获取 max-age
};
```

---

## HTTP 客户端

### HttpClient

HTTP 客户端类，支持 HTTP/1.1 和 HTTP/2。

```cpp
class HttpClient {
public:
    struct Config {
        std::string userAgent = "XiaopengKernel/0.1.0";
        std::chrono::milliseconds defaultTimeout{30000};
        std::chrono::milliseconds connectTimeout{10000};
        bool followRedirects = true;
        int maxRedirects = 20;
        bool verifySsl = true;
        std::string caBundlePath;      // CA 证书路径
        std::string proxyUrl;          // 代理 URL
        std::string proxyUsername;
        std::string proxyPassword;
        bool enableHttp2 = true;
        bool enableCompression = true;
        size_t maxResponseSize = 100 * 1024 * 1024;  // 最大响应大小
    };
    
    HttpClient() = default;
    explicit HttpClient(const Config& config);
    
    // 异步执行请求
    Future<Result<HttpResponse>> executeAsync(const HttpRequest& request);
    
    // 同步执行请求
    Result<HttpResponse> execute(const HttpRequest& request);
    
    // 便捷方法
    Result<HttpResponse> get(const Url& url);
    Result<HttpResponse> post(const Url& url, const ByteBuffer& body);
    Result<HttpResponse> post(const Url& url, const std::string& body);
    Result<HttpResponse> postJson(const Url& url, const std::string& json);
    Result<HttpResponse> put(const Url& url, const ByteBuffer& body);
    Result<HttpResponse> del(const Url& url);
    Result<HttpResponse> head(const Url& url);
    
    // 配置
    void setConfig(const Config& config);
    const Config& config() const;
    
    // Cookie 管理
    void setCookieJar(Ptr<CookieJar> jar);
    Ptr<CookieJar> cookieJar() const;
};
```

**示例：**

```cpp
HttpClient::Config config;
config.userAgent = "MyApp/1.0";
config.enableHttp2 = true;
config.defaultTimeout = std::chrono::milliseconds(10000);

HttpClient client(config);

// GET 请求
auto response = client.get(Url::parse("https://api.example.com/data").unwrap());
if (response.isOk()) {
    auto& resp = response.unwrap();
    std::cout << "Status: " << static_cast<int>(resp.statusCode) << std::endl;
    std::cout << "Body: " << resp.text() << std::endl;
    std::cout << "Time: " << resp.totalTime.count() << "ms" << std::endl;
}

// POST JSON
auto postResponse = client.postJson(
    Url::parse("https://api.example.com/submit").unwrap(),
    R"({"name": "test", "value": 123})"
);

// 链式配置
auto request = HttpRequest::get(Url::parse("https://example.com").unwrap())
    .withHeader("Authorization", "Bearer token123")
    .withTimeout(std::chrono::milliseconds(5000));

auto result = client.execute(request);
```

---

## 缓存系统

### HttpCache

HTTP 缓存管理类，使用 LRU 淘汰策略。

```cpp
class HttpCache {
public:
    struct Config {
        size_t maxSize = 50 * 1024 * 1024;           // 最大缓存大小 (50MB)
        std::chrono::seconds defaultTtl{3600};       // 默认 TTL (1小时)
        std::chrono::seconds maxTtl{86400 * 30};     // 最大 TTL (30天)
        bool enabled = true;
        bool ignoreNoStore = false;
    };
    
    HttpCache() = default;
    explicit HttpCache(const Config& config);
    
    // 获取缓存
    std::optional<CacheEntry> get(const Url& url, const std::string& varyKey = "");
    
    // 存入缓存
    void put(const Url& url, const HttpResponse& response, const std::string& varyKey = "");
    
    // 删除缓存
    void remove(const Url& url, const std::string& varyKey = "");
    
    // 清空缓存
    void clear();
    
    // 清理过期缓存
    void cleanup();
    
    // 检查缓存是否存在
    bool has(const Url& url, const std::string& varyKey = "") const;
    
    // 统计信息
    size_t size() const;   // 当前缓存大小
    size_t count() const;  // 缓存条目数
    size_t maxSize() const;
    
    // 配置
    bool enabled() const;
    void setEnabled(bool enabled);
    void setMaxSize(size_t maxSize);
    
    // 验证请求
    HttpRequest createValidationRequest(const Url& url, const CacheEntry& entry);
    bool shouldValidate(const CacheEntry& entry) const;
    void updateFromValidation(const Url& url, CacheEntry& entry, const HttpResponse& response);
};
```

### CacheEntry

缓存条目结构体。

```cpp
struct CacheEntry {
    std::string key;            // 缓存键
    ByteBuffer data;            // 缓存数据
    HttpHeaders headers;        // 响应头
    std::string eTag;           // ETag
    std::string lastModified;   // Last-Modified
    TimePoint createdAt;        // 创建时间
    TimePoint expiresAt;        // 过期时间
    std::string mimeType;       // MIME 类型
    std::uint64_t size = 0;     // 数据大小
    int hitCount = 0;           // 命中次数
    bool mustRevalidate = false;
    bool noStore = false;
    
    bool isExpired() const;     // 是否过期
    bool isFresh() const;       // 是否新鲜
    bool needsRevalidation() const;  // 是否需要验证
    std::chrono::seconds age() const;       // 缓存年龄
    std::chrono::seconds remainingTtl() const;  // 剩余 TTL
};
```

**示例：**

```cpp
HttpCache::Config config;
config.maxSize = 100 * 1024 * 1024;  // 100MB
config.defaultTtl = std::chrono::seconds(7200);  // 2小时

HttpCache cache(config);

// 存入缓存
HttpResponse response;
response.statusCode = HttpStatusCode::Ok;
response.body = ByteBuffer{'d', 'a', 't', 'a'};
response.headers.setContentType("text/plain");
response.headers.set("Cache-Control", "max-age=3600");

Url url = Url::parse("https://example.com/resource").unwrap();
cache.put(url, response);

// 获取缓存
auto cached = cache.get(url);
if (cached && cached->isFresh()) {
    std::cout << "Cache hit!" << std::endl;
    std::cout << "Data: " << std::string(cached->data.begin(), cached->data.end()) << std::endl;
}

// 清理过期缓存
cache.cleanup();
```

---

## 连接池

### ConnectionPool

连接池管理类。

```cpp
class ConnectionPool {
public:
    struct Config {
        size_t maxConnectionsPerHost = 6;    // 每主机最大连接数
        size_t maxTotalConnections = 100;    // 总最大连接数
        std::chrono::seconds idleTimeout{60}; // 空闲超时
        std::chrono::seconds connectTimeout{10};
    };
    
    ConnectionPool() = default;
    explicit ConnectionPool(const Config& config);
    
    // 获取连接
    Ptr<Connection> acquire(const Url& url);
    
    // 释放连接
    void release(Ptr<Connection> connection);
    
    // 清理空闲连接
    void cleanupIdleConnections();
    
    // 清空连接池
    void clear();
    
    // 统计信息
    size_t totalConnections() const;
    size_t activeConnections() const;
    size_t idleConnections() const;
    
    // 配置
    const Config& config() const;
    void setConfig(const Config& config);
};
```

---

## 资源处理

### ResourceType

资源类型枚举。

```cpp
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
```

### ResourcePriority

资源加载优先级。

```cpp
enum class ResourcePriority {
    Highest,
    High,
    Medium,
    Low,
    Lowest
};
```

### LoadState

资源加载状态。

```cpp
enum class LoadState {
    Pending,    // 等待中
    Loading,    // 加载中
    Loaded,     // 已加载
    Failed,     // 加载失败
    Cancelled   // 已取消
};
```

### Resource

资源结构体。

```cpp
struct Resource {
    Url url;                                    // 资源 URL
    ResourceType type = ResourceType::Unknown;  // 资源类型
    ResourcePriority priority = ResourcePriority::Medium;
    LoadState state = LoadState::Pending;       // 加载状态
    
    ByteBuffer data;                            // 资源数据
    std::string mimeType;                       // MIME 类型
    std::string charset;                        // 字符集
    std::string encoding;                       // 编码
    
    HttpHeaders responseHeaders;                // 响应头
    HttpStatusCode statusCode = HttpStatusCode::Ok;
    
    TimePoint startTime;                        // 开始时间
    TimePoint endTime;                          // 结束时间
    Duration loadTime{0};                       // 加载耗时
    
    std::uint64_t size = 0;                     // 数据大小
    std::uint64_t transferred = 0;              // 已传输大小
    
    bool fromCache = false;                     // 是否来自缓存
    bool cancelled = false;                     // 是否被取消
    ErrorCode error = ErrorCode::Success;       // 错误码
    std::string errorMessage;                   // 错误消息
    
    // 辅助方法
    std::string text() const;
    std::string_view textView() const;
    std::span<const Byte> bytes() const;
    bool isLoaded() const;
    bool isFailed() const;
    bool isLoading() const;
    bool isPending() const;
    bool isCancelled() const;
};
```

### LoadOptions

加载选项结构体。

```cpp
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
```

### ResourceHandler

资源处理器基类。

```cpp
class ResourceHandler {
public:
    virtual ~ResourceHandler() = default;
    
    virtual bool canHandle(ResourceType type) const = 0;
    virtual Result<void> process(Resource& resource) = 0;
    virtual std::vector<std::string> extractDependencies(const Resource& resource);
    virtual std::string name() const = 0;
};
```

### 内置资源处理器

```cpp
class HtmlHandler : public ResourceHandler;       // HTML 处理器
class CssHandler : public ResourceHandler;        // CSS 处理器
class JavaScriptHandler : public ResourceHandler; // JavaScript 处理器
class ImageHandler : public ResourceHandler;      // 图像处理器
```

### ImageHandler::ImageInfo

图像信息结构体。

```cpp
struct ImageInfo {
    int width = 0;
    int height = 0;
    int channels = 0;
    size_t dataSize = 0;
    std::string format;  // png, jpeg, gif, webp, tiff, ico, svg
};
```

### 辅助函数

```cpp
// 从 MIME 类型检测资源类型
ResourceType detectResourceType(const std::string& mimeType);

// 从 URL 检测资源类型
ResourceType detectResourceTypeFromUrl(const Url& url);

// 资源类型转 MIME 类型
std::string resourceTypeToMime(ResourceType type);
```

---

## 加载调度

### LoadScheduler

加载调度器类。

```cpp
class LoadScheduler {
public:
    struct Config {
        size_t maxConcurrent = 6;                    // 最大并发数
        size_t maxConcurrentPerHost = 4;             // 每主机最大并发
        std::chrono::milliseconds taskTimeout{60000};
        bool enablePreload = true;
        bool enableLazyLoad = true;
    };
    
    LoadScheduler() = default;
    explicit LoadScheduler(const Config& config);
    ~LoadScheduler();
    
    // 启动/停止调度器
    void start();
    void stop();
    
    // 调度加载任务
    std::string schedule(const Url& url, 
                         const LoadOptions& options,
                         std::function<void(Result<Ptr<Resource>>)> callback);
    
    // 异步加载
    Future<Result<Ptr<Resource>>> loadAsync(const Url& url, const LoadOptions& options = {});
    
    // 同步加载
    Result<Ptr<Resource>> load(const Url& url, const LoadOptions& options = {});
    
    // 取消任务
    bool cancel(const std::string& taskId);
    void cancelAll();
    
    // 设置依赖
    void setHttpClient(Ptr<HttpClient> client);
    void setCache(Ptr<HttpCache> cache);
    void setHandlerRegistry(Ptr<ResourceHandlerRegistry> registry);
    
    // 状态查询
    SchedulerStats stats() const;
    bool isRunning() const;
    void setMaxConcurrent(size_t max);
    size_t queueSize() const;
};
```

### SchedulerStats

调度器统计信息。

```cpp
struct SchedulerStats {
    std::uint64_t totalTasks = 0;
    std::uint64_t completedTasks = 0;
    std::uint64_t failedTasks = 0;
    std::uint64_t cancelledTasks = 0;
    std::uint64_t pendingTasks = 0;
    std::uint64_t activeTasks = 0;
    std::uint64_t maxConcurrent = 0;
};
```

---

## 安全机制

### Origin

源类，用于同源策略检查。

```cpp
class Origin {
public:
    Origin() = default;
    explicit Origin(const Url& url);
    
    static Origin parse(const std::string& origin);
    static Origin opaque();  // 创建不透明源
    
    bool isValid() const;
    bool isOpaque() const;
    
    const std::string& scheme() const;
    const std::string& host() const;
    std::uint16_t port() const;
    
    std::string toString() const;
    
    bool sameOrigin(const Origin& other) const;
    bool sameOriginDomain(const Origin& other) const;
    bool isSecure() const;
    
    bool operator==(const Origin& other) const;
    bool operator!=(const Origin& other) const;
    bool operator<(const Origin& other) const;
};
```

### SecurityPolicy

安全策略配置。

```cpp
struct SecurityPolicy {
    bool allowMixedContent = false;
    bool blockInsecureRequests = true;
    bool upgradeInsecureRequests = true;
    bool enforceSameOrigin = true;
    bool enforceCors = true;
    std::set<std::string> trustedSchemes{"http", "https", "data", "blob", "file", "about"};
    std::set<std::string> blockedHosts;
    std::set<std::string> allowedOrigins;
};
```

### SameOriginPolicy

同源策略检查类。

```cpp
class SameOriginPolicy {
public:
    explicit SameOriginPolicy(const SecurityPolicy& policy = {});
    
    bool isSameOrigin(const Url& a, const Url& b) const;
    bool isSameOrigin(const Origin& a, const Origin& b) const;
    bool isCrossOrigin(const Url& a, const Url& b) const;
    
    Result<void> checkSameOrigin(const Url& documentUrl, const Url& resourceUrl) const;
    
    bool isAllowed(const Url& documentUrl, const Url& resourceUrl, 
                   ResourceType resourceType) const;
    
    bool isMixedContent(const Url& documentUrl, const Url& resourceUrl) const;
    Result<void> checkMixedContent(const Url& documentUrl, const Url& resourceUrl) const;
    
    Url upgradeInsecureUrl(const Url& url) const;
    
    bool isBlockedHost(const std::string& host) const;
    bool isAllowedOrigin(const Origin& origin) const;
    
    void setPolicy(const SecurityPolicy& policy);
    const SecurityPolicy& policy() const;
};
```

### CorsChecker

CORS 检查类。

```cpp
enum class CorsResult {
    Success,
    Failure,
    NotCors
};

class CorsChecker {
public:
    explicit CorsChecker(const SecurityPolicy& policy = {});
    
    CorsResult checkPreflight(const HttpRequest& request, 
                              const HttpResponse& response,
                              const Origin& requestOrigin) const;
    
    CorsResult checkActualRequest(const HttpRequest& request, 
                                  const HttpResponse& response,
                                  const Origin& requestOrigin) const;
    
    bool needsPreflight(const HttpRequest& request, 
                        const Origin& requestOrigin,
                        const Origin& targetOrigin) const;
    
    HttpRequest createPreflightRequest(const Url& url, 
                                       const Origin& origin,
                                       const HttpRequest& actualRequest) const;
    
    void setPolicy(const SecurityPolicy& policy);
};
```

---

## Loader 接口

### LoaderConfig

Loader 配置结构体。

```cpp
struct LoaderConfig {
    HttpClient::Config httpClient;
    ConnectionPool::Config connectionPool;
    HttpCache::Config cache;
    LoadScheduler::Config scheduler;
    SecurityPolicy security;
    
    std::string userAgent = "XiaopengKernel/0.1.0";
    std::string caBundlePath;
    std::string proxyUrl;
    
    size_t maxConcurrentLoads = 6;
    size_t maxCacheSize = 50 * 1024 * 1024;
    std::chrono::seconds defaultTimeout{30};
    bool enableHttp2 = true;
    bool enableCache = true;
    bool enableSecurity = true;
};
```

### Loader

统一的资源加载接口。

```cpp
class Loader {
public:
    Loader() = default;
    explicit Loader(const LoaderConfig& config);
    ~Loader();
    
    // 单例访问
    static Loader& instance();
    
    // 初始化/关闭
    void initialize(const LoaderConfig& config);
    void shutdown();
    
    // 异步加载资源
    Future<Result<Ptr<Resource>>> loadAsync(const std::string& url, 
                                            const LoadOptions& options = {});
    Future<Result<Ptr<Resource>>> loadAsync(const Url& url, 
                                            const LoadOptions& options = {});
    
    // 同步加载资源
    Result<Ptr<Resource>> load(const std::string& url, const LoadOptions& options = {});
    Result<Ptr<Resource>> load(const Url& url, const LoadOptions& options = {});
    
    // 调度加载任务（回调方式）
    std::string scheduleLoad(const std::string& url, 
                             const LoadOptions& options,
                             std::function<void(Result<Ptr<Resource>>)> callback);
    std::string scheduleLoad(const Url& url, 
                             const LoadOptions& options,
                             std::function<void(Result<Ptr<Resource>>)> callback);
    
    // 取消加载
    bool cancelLoad(const std::string& taskId);
    void cancelAllLoads();
    
    // HTTP 请求
    Result<HttpResponse> fetch(const std::string& url, const HttpRequest& request = {});
    Result<HttpResponse> fetch(const Url& url, const HttpRequest& request = {});
    Future<Result<HttpResponse>> fetchAsync(const Url& url, const HttpRequest& request = {});
    
    // 文档源设置
    void setDocumentOrigin(const Origin& origin);
    void setDocumentOrigin(const Url& url);
    const std::optional<Origin>& documentOrigin() const;
    
    // 安全检查
    Result<void> checkSecurity(const Origin& origin, const Url& targetUrl) const;
    bool isSameOrigin(const Url& a, const Url& b) const;
    CorsResult checkCors(const HttpRequest& request, 
                         const HttpResponse& response,
                         const Origin& origin) const;
    
    // 缓存操作
    void clearCache();
    size_t cacheSize() const;
    size_t cacheCount() const;
    void setCacheEnabled(bool enabled);
    void setMaxCacheSize(size_t maxSize);
    void cleanupCache();
    
    // 统计信息
    SchedulerStats schedulerStats() const;
    size_t pendingLoads() const;
    size_t activeConnections() const;
    size_t idleConnections() const;
    
    // 配置
    void setMaxConcurrent(size_t max);
    
    // 组件访问
    Ptr<HttpClient> httpClient() const;
    Ptr<HttpCache> cache() const;
    Ptr<ConnectionPool> connectionPool() const;
    Ptr<LoadScheduler> scheduler() const;
    Ptr<ResourceHandlerRegistry> handlerRegistry() const;
    Ptr<SameOriginPolicy> sameOriginPolicy() const;
    Ptr<CorsChecker> corsChecker() const;
    
    bool isInitialized() const;
    const LoaderConfig& config() const;
};

// 全局访问函数
Loader& GetLoader();
```

**完整示例：**

```cpp
#include <loader/loader_all.hpp>

using namespace xiaopeng::loader;

int main() {
    // 配置 Loader
    LoaderConfig config;
    config.userAgent = "MyBrowser/1.0";
    config.enableHttp2 = true;
    config.enableCache = true;
    config.maxConcurrentLoads = 8;
    config.maxCacheSize = 100 * 1024 * 1024;  // 100MB
    
    // 创建 Loader
    Loader loader(config);
    
    // 设置文档源（用于安全检查）
    loader.setDocumentOrigin("https://example.com");
    
    // 同步加载 HTML 页面
    auto htmlResult = loader.load("https://example.com/index.html");
    if (htmlResult.isOk()) {
        auto html = htmlResult.unwrap();
        std::cout << "Loaded HTML: " << html->size << " bytes" << std::endl;
    }
    
    // 异步加载 CSS
    auto cssFuture = loader.loadAsync("https://example.com/style.css",
        LoadOptions{.priority = ResourcePriority::High});
    
    // 使用回调加载图像
    loader.scheduleLoad("https://example.com/logo.png",
        LoadOptions{.priority = ResourcePriority::Low},
        [](Result<Ptr<Resource>> result) {
            if (result.isOk()) {
                auto img = result.unwrap();
                std::cout << "Image loaded: " << img->mimeType << std::endl;
            }
        });
    
    // 等待 CSS 加载完成
    auto cssResult = cssFuture.get();
    if (cssResult.isOk()) {
        auto css = cssResult.unwrap();
        std::cout << "CSS loaded from cache: " << css->fromCache << std::endl;
    }
    
    // 获取统计信息
    auto stats = loader.schedulerStats();
    std::cout << "Completed: " << stats.completedTasks << std::endl;
    std::cout << "Cache size: " << loader.cacheSize() << " bytes" << std::endl;
    
    return 0;
}
```

---

## HTML 解析器模块

命名空间：`xiaopeng::dom`

### TokenType 枚举

定义HTML词法分析器产生的Token类型。

```cpp
enum class TokenType : uint8_t {
    Doctype,        // DOCTYPE声明
    StartTag,       // 开始标签
    EndTag,         // 结束标签
    Comment,        // 注释
    Character,      // 字符
    EndOfFile,      // 文件结束
    Cdata,          // CDATA区块
    CharacterReference  // 字符引用
};
```

### Attribute 结构体

表示HTML元素的属性。

```cpp
struct Attribute {
    std::string name;   // 属性名
    std::string value;  // 属性值
    
    Attribute() = default;
    Attribute(std::string n, std::string v);
    bool operator==(const Attribute& other) const;
};
```

### Token 结构体

表示词法分析产生的词元。

```cpp
struct Token {
    TokenType type;                     // Token类型
    std::string name;                   // 标签名（StartTag/EndTag/Doctype）
    std::string data;                   // 数据（Character/Comment/Cdata）
    std::vector<Attribute> attributes;  // 属性列表
    bool selfClosing = false;           // 是否自闭合
    bool forceQuirks = false;           // 是否强制怪异模式
    std::string systemIdentifier;       // DOCTYPE系统标识符
    std::string publicIdentifier;       // DOCTYPE公共标识符
    size_t position = 0;                // 在输入中的位置
    size_t line = 1;                    // 行号
    size_t column = 1;                  // 列号
    
    // 静态工厂方法
    static Token makeDoctype(const std::string& name = "", 
                             const std::string& publicId = "",
                             const std::string& systemId = "",
                             bool forceQuirks = false);
    static Token makeStartTag(const std::string& name,
                              std::vector<Attribute> attrs = {},
                              bool selfClosing = false);
    static Token makeEndTag(const std::string& name);
    static Token makeComment(const std::string& data);
    static Token makeCharacter(char c);
    static Token makeCharacter(const std::string& str);
    static Token makeEndOfFile();
    static Token makeCdata(const std::string& data);
    
    // 类型检查方法
    bool isStartTag() const;
    bool isEndTag() const;
    bool isCharacter() const;
    bool isComment() const;
    bool isDoctype() const;
    bool isEndOfFile() const;
    bool isCdata() const;
    
    // 属性访问
    std::optional<std::string> getAttribute(const std::string& attrName) const;
    bool hasAttribute(const std::string& attrName) const;
};
```

### NodeType 枚举

定义DOM节点类型。

```cpp
enum class NodeType : uint8_t {
    Element,            // 元素节点
    Text,               // 文本节点
    Comment,            // 注释节点
    Document,           // 文档节点
    DocumentType,       // 文档类型节点
    DocumentFragment,   // 文档片段
    CdataSection        // CDATA区块
};
```

### Node 类

DOM节点基类。

```cpp
class Node : public std::enable_shared_from_this<Node> {
public:
    virtual ~Node() = default;
    
    // 节点信息
    NodeType nodeType() const;
    const std::string& nodeName() const;
    
    // 树遍历
    NodePtr parentNode() const;
    NodePtr firstChild() const;
    NodePtr lastChild() const;
    NodePtr nextSibling() const;
    NodePtr previousSibling() const;
    const std::vector<NodePtr>& childNodes() const;
    
    // 内容操作
    virtual std::string textContent() const;
    virtual void setTextContent(const std::string&);
    
    // 子节点操作
    bool hasChildNodes() const;
    size_t childCount() const;
    virtual NodePtr cloneNode(bool deep = false) const = 0;
    
    void appendChild(NodePtr child);
    void insertBefore(NodePtr newChild, NodePtr refChild);
    void removeChild(NodePtr child);
    void replaceChild(NodePtr newChild, NodePtr oldChild);
    void normalize();
    
    // 树操作
    NodePtr getRootNode() const;
    bool contains(const NodePtr& other) const;
    
    // 序列化
    virtual std::string toHtml() const = 0;
};
```

### Element 类

DOM元素节点。

```cpp
class Element : public Node {
public:
    Element(const std::string& localName, 
            const std::string& namespaceUri = "",
            const std::string& prefix = "");
    
    // 元素信息
    const std::string& localName() const;
    const std::string& namespaceUri() const;
    const std::string& prefix() const;
    std::string tagName() const;
    std::string qualifiedName() const;
    
    // 属性操作
    const std::vector<Attribute>& attributes() const;
    std::optional<std::string> getAttribute(const std::string& name) const;
    void setAttribute(const std::string& name, const std::string& value);
    void removeAttribute(const std::string& name);
    bool hasAttribute(const std::string& name) const;
    void toggleAttribute(const std::string& name);
    
    // ID和类名
    std::string id() const;
    void setId(const std::string& id);
    std::string className() const;
    void setClassName(const std::string& className);
    std::vector<std::string> classList() const;
    void addClass(const std::string& className);
    void removeClass(const std::string& className);
    bool hasClass(const std::string& className) const;
    
    // 内容
    std::string textContent() const override;
    void setTextContent(const std::string& text) override;
    std::string innerHTML() const;
    std::string outerHTML() const;
    
    // 元素遍历
    ElementPtr firstElementChild() const;
    ElementPtr lastElementChild() const;
    size_t childElementCount() const;
    ElementPtr previousElementSibling() const;
    ElementPtr nextElementSibling() const;
    
    // DOM查询
    std::vector<ElementPtr> getElementsByTagName(const std::string& name) const;
    std::vector<ElementPtr> getElementsByClassName(const std::string& className) const;
    ElementPtr getElementById(const std::string& id) const;
    std::vector<ElementPtr> querySelectorAll(const std::string& selector) const;
    ElementPtr querySelector(const std::string& selector) const;
    bool matches(const std::string& selector) const;
    
    // 克隆
    NodePtr cloneNode(bool deep = false) const override;
    
    // 序列化
    std::string toHtml() const override;
    
    // 静态工具方法
    static bool isVoidElement(const std::string& tagName);
    static bool isRawTextElement(const std::string& tagName);
    static bool isEscapableRawTextElement(const std::string& tagName);
};
```

### Document 类

DOM文档对象。

```cpp
class Document : public Node {
public:
    Document();
    
    // 文档属性
    std::string contentType() const;
    void setContentType(const std::string& type);
    std::string characterSet() const;
    void setCharacterSet(const std::string& charset);
    std::string url() const;
    void setUrl(const std::string& url);
    std::string documentUri() const;
    std::string compatMode() const;
    void setCompatMode(const std::string& mode);
    
    // 文档结构
    std::string doctypeName() const;
    void setDoctypeName(const std::string& name);
    NodePtr doctype() const;
    ElementPtr documentElement() const;
    ElementPtr head() const;
    ElementPtr body() const;
    void setBody(ElementPtr newBody);
    ElementPtr title() const;
    std::string titleText() const;
    void setTitleText(const std::string& text);
    
    // 节点创建
    ElementPtr createElement(const std::string& localName);
    ElementPtr createElementNS(const std::string& namespaceUri, 
                               const std::string& qualifiedName);
    NodePtr createTextNode(const std::string& data);
    NodePtr createComment(const std::string& data);
    NodePtr createDocumentType(const std::string& name,
                               const std::string& publicId = "",
                               const std::string& systemId = "");
    std::shared_ptr<DocumentFragment> createDocumentFragment();
    
    // DOM查询
    ElementPtr getElementById(const std::string& id) const;
    std::vector<ElementPtr> getElementsByTagName(const std::string& name) const;
    std::vector<ElementPtr> getElementsByClassName(const std::string& className) const;
    std::vector<ElementPtr> querySelectorAll(const std::string& selector) const;
    ElementPtr querySelector(const std::string& selector) const;
    
    // 克隆和序列化
    NodePtr cloneNode(bool deep = false) const override;
    std::string toHtml() const override;
};
```

### TextNode 类

文本节点。

```cpp
class TextNode : public Node {
public:
    explicit TextNode(const std::string& text);
    
    const std::string& data() const;
    void setData(const std::string& text);
    std::string textContent() const override;
    void setTextContent(const std::string& text) override;
    size_t length() const;
    
    NodePtr cloneNode(bool = false) const override;
    std::string toHtml() const override;
};
```

### CommentNode 类

注释节点。

```cpp
class CommentNode : public Node {
public:
    explicit CommentNode(const std::string& data);
    
    const std::string& data() const;
    void setData(const std::string& data);
    std::string textContent() const override;
    void setTextContent(const std::string& data) override;
    
    NodePtr cloneNode(bool = false) const override;
    std::string toHtml() const override;
};
```

### DocumentTypeNode 类

文档类型节点。

```cpp
class DocumentTypeNode : public Node {
public:
    DocumentTypeNode(const std::string& name = "html",
                     const std::string& publicId = "",
                     const std::string& systemId = "");
    
    const std::string& name() const;
    const std::string& publicId() const;
    const std::string& systemId() const;
    
    NodePtr cloneNode(bool = false) const override;
    std::string toHtml() const override;
};
```

### DocumentFragment 类

文档片段。

```cpp
class DocumentFragment : public Node {
public:
    DocumentFragment();
    
    NodePtr cloneNode(bool deep = false) const override;
    std::string toHtml() const override;
};
```

### HtmlTokenizer 类

HTML词法分析器。

```cpp
class HtmlTokenizer {
public:
    using ErrorCallback = std::function<void(const ParseError&)>;
    
    // 构造函数
    explicit HtmlTokenizer(std::string_view input);
    explicit HtmlTokenizer(const ByteBuffer& input);
    explicit HtmlTokenizer(const std::string& input);
    
    // 解析方法
    Token nextToken();
    std::vector<Token> tokenize();
    
    // 错误处理
    bool hasError() const;
    const std::vector<ParseError>& errors() const;
    void clearErrors();
    void setErrorCallback(ErrorCallback callback);
    
    // 状态查询
    size_t position() const;
    size_t line() const;
    size_t column() const;
    bool isEof() const;
    
    // 状态控制
    TokenizerState state() const;
    void setState(TokenizerState state);
    void setLastStartTag(const std::string& tagName);
    const std::string& lastStartTag() const;
    void setAllowCdata(bool allow);
    bool allowCdata() const;
};
```

### HtmlTreeBuilder 类

DOM树构建器。

```cpp
class HtmlTreeBuilder {
public:
    using ErrorCallback = std::function<void(const ParseError&)>;
    
    HtmlTreeBuilder();
    
    // 构建方法
    std::shared_ptr<Document> build(const std::vector<Token>& tokens);
    std::shared_ptr<Document> build(HtmlTokenizer& tokenizer);
    std::shared_ptr<Document> build(const std::string& html);
    std::shared_ptr<Document> build(const ByteBuffer& html);
    
    // 错误处理
    void setErrorCallback(ErrorCallback callback);
    bool hasError() const;
    const std::vector<ParseError>& errors() const;
    
    // 配置
    void setScriptingFlag(bool scripting);
    bool scripting() const;
    bool quirksMode() const;
};
```

### ParserOptions 结构体

HTML解析器配置选项。

```cpp
struct ParserOptions {
    bool scripting = false;      // 是否启用脚本
    bool allowCdata = false;     // 是否允许CDATA
    bool strictMode = false;     // 严格模式
    std::string encoding = "UTF-8";  // 字符编码
    std::string baseUrl;         // 基础URL
};
```

### ParserResult 结构体

HTML解析结果。

```cpp
struct ParserResult {
    std::shared_ptr<Document> document;  // 解析后的文档
    std::vector<ParseError> errors;      // 错误列表
    bool hasErrors = false;              // 是否有错误
    std::string quirksMode;              // 怪异模式
    
    bool ok() const;  // 解析是否成功
};
```

### HtmlParser 类

HTML解析器统一接口。

```cpp
class HtmlParser {
public:
    using ErrorCallback = std::function<void(const ParseError&)>;
    
    HtmlParser() = default;
    explicit HtmlParser(const ParserOptions& options);
    
    // 解析方法
    ParserResult parse(const std::string& html);
    ParserResult parse(const ByteBuffer& html);
    ParserResult parse(std::string_view html);
    ParserResult parse(const Resource& resource);
    
    // 静态便捷方法
    static ParserResult parseHtml(const std::string& html, 
                                  const ParserOptions& options = {});
    static ParserResult parseHtml(const ByteBuffer& html, 
                                  const ParserOptions& options = {});
    static ParserResult parseHtml(const Resource& resource, 
                                  const ParserOptions& options = {});
    
    // 配置
    void setOptions(const ParserOptions& options);
    const ParserOptions& options() const;
    void setErrorCallback(ErrorCallback callback);
    
    // 资源提取静态方法
    static std::vector<std::string> extractLinks(
        const std::shared_ptr<Document>& document);
    static std::vector<std::string> extractScripts(
        const std::shared_ptr<Document>& document);
    static std::vector<std::string> extractStylesheets(
        const std::shared_ptr<Document>& document);
    static std::vector<std::string> extractImages(
        const std::shared_ptr<Document>& document);
    static std::vector<std::string> extractFavicons(
        const std::shared_ptr<Document>& document);
    static std::string extractTitle(
        const std::shared_ptr<Document>& document);
    static std::string extractMetaDescription(
        const std::shared_ptr<Document>& document);
    static std::unordered_map<std::string, std::string> extractMetaTags(
        const std::shared_ptr<Document>& document);
    static std::string extractBaseHref(
        const std::shared_ptr<Document>& document);
};
```

### ParseError 结构体

解析错误信息。

```cpp
struct ParseError {
    std::string message;   // 错误消息
    size_t position = 0;   // 位置
    size_t line = 1;       // 行号
    size_t column = 1;     // 列号
    
    ParseError(std::string msg, size_t pos, size_t l, size_t c);
};
```

### HTML解析器使用示例

```cpp
#include <loader/loader_all.hpp>

using namespace xiaopeng::loader;
using namespace xiaopeng::dom;

void example1_basicParsing() {
    // 基本HTML解析
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="UTF-8">
            <title>Example Page</title>
            <link rel="stylesheet" href="/style.css">
        </head>
        <body>
            <h1 id="main-title">Hello World</h1>
            <p class="description">This is an example.</p>
            <img src="/image.png" alt="Example">
            <script src="/app.js"></script>
        </body>
        </html>
    )";
    
    auto result = HtmlParser::parseHtml(html);
    
    if (result.ok()) {
        auto doc = result.document;
        
        // 获取文档信息
        std::cout << "Title: " << HtmlParser::extractTitle(doc) << std::endl;
        std::cout << "Description: " << HtmlParser::extractMetaDescription(doc) << std::endl;
        
        // 提取资源
        auto scripts = HtmlParser::extractScripts(doc);
        auto stylesheets = HtmlParser::extractStylesheets(doc);
        auto images = HtmlParser::extractImages(doc);
        
        // DOM查询
        auto title = doc->getElementById("main-title");
        if (title) {
            std::cout << "Found title: " << title->textContent() << std::endl;
        }
        
        auto paragraphs = doc->getElementsByTagName("p");
        for (const auto& p : paragraphs) {
            std::cout << "Paragraph: " << p->textContent() << std::endl;
        }
    }
}

void example2_tokenizer() {
    // 直接使用词法分析器
    std::string html = "<div id=\"container\"><p>Text</p></div>";
    HtmlTokenizer tokenizer(html);
    
    std::vector<Token> tokens = tokenizer.tokenize();
    
    for (const auto& token : tokens) {
        switch (token.type) {
            case TokenType::StartTag:
                std::cout << "Start tag: " << token.name << std::endl;
                for (const auto& attr : token.attributes) {
                    std::cout << "  " << attr.name << "=\"" << attr.value << "\"" << std::endl;
                }
                break;
            case TokenType::EndTag:
                std::cout << "End tag: " << token.name << std::endl;
                break;
            case TokenType::Character:
                std::cout << "Character: " << token.data << std::endl;
                break;
            default:
                break;
        }
    }
}

void example3_treeBuilder() {
    // 直接使用树构建器
    HtmlTreeBuilder builder;
    builder.setScriptingFlag(false);  // 禁用脚本
    
    auto doc = builder.build("<p>Hello <strong>World</strong></p>");
    
    if (doc) {
        auto html = doc->documentElement();
        std::cout << "Document HTML:\n" << doc->toHtml() << std::endl;
    }
}

void example4_domManipulation() {
    auto result = HtmlParser::parseHtml("<div></div>");
    auto doc = result.document;
    
    // 创建新元素
    auto div = doc->createElement("div");
    div->setId("container");
    div->addClass("main");
    div->setAttribute("data-value", "123");
    
    // 创建文本节点
    auto text = doc->createTextNode("Hello World");
    div->appendChild(text);
    
    // 添加到body
    doc->body()->appendChild(div);
    
    // 克隆元素
    auto clone = std::static_pointer_cast<Element>(div->cloneNode(true));
    clone->setId("container-clone");
    doc->body()->appendChild(clone);
    
    // 输出结果
    std::cout << doc->toHtml() << std::endl;
}

void example5_withResource() {
    // 从Resource对象解析
    Resource resource;
    resource.url = Url::parse("https://example.com/page.html");
    resource.type = ResourceType::Html;
    resource.mimeType = "text/html";
    resource.charset = "UTF-8";
    resource.data = ByteBuffer{'<', 'h', 't', 'm', 'l', '>', '<', '/', 'h', 't', 'm', 'l', '>'};
    
    auto result = HtmlParser::parseHtml(resource);
    
    if (result.ok()) {
        std::cout << "Parsed from resource: " << result.document->url() << std::endl;
    }
}
```

---

## CSS 解析器模块

命名空间：`xiaopeng::css`

### CssTokenizer 类

CSS 词法分析器。

```cpp
class CssTokenizer {
public:
    explicit CssTokenizer(const std::string& input);
    
    std::vector<CssToken> tokenize();
};
```

### CssParser 类

CSS 语法解析器。

```cpp
class CssParser {
public:
    explicit CssParser(const std::string& input);
    
    StyleSheet parse();
};
```

---

## 布局引擎模块

命名空间：`xiaopeng::layout`

### LayoutBox 类

布局的核心节点单元。

```cpp
class LayoutBox {
public:
    // 获取盒模型尺寸
    Dimensions& dimensions();
    
    // 盒类型
    BoxType type() const;
    
    // 行盒列表 (仅 IFC)
    std::vector<LineBox>& lineBoxes();
};
```

### LayoutAlgorithm 类

核心布局算法实现。

```cpp
class LayoutAlgorithm {
public:
    // 执行布局
    void layout(LayoutBoxPtr root, const Dimensions& viewport);
};
```

---

## 渲染模块

命名空间：`xiaopeng::renderer`

### Canvas 接口

基础绘图接口抽象。

```cpp
class Canvas {
public:
    virtual void fillRect(int x, int y, int width, int height, Color color) = 0;
    virtual void drawRect(int x, int y, int width, int height, Color color) = 0;
    virtual void drawText(int x, int y, const std::string& text, Color color) = 0;
};
```

### BitmapCanvas 类

基于位图的 Canvas 实现。

```cpp
class BitmapCanvas : public Canvas {
public:
    BitmapCanvas(int width, int height);
    
    // 导出为 PPM 图像
    bool saveToPPM(const std::string& filename);
};
```

### PaintingAlgorithm 类

绘图算法，将布局树转化为 Canvas 操作。

```cpp
class PaintingAlgorithm {
public:
    void paint(layout::LayoutBoxPtr root, Canvas& canvas);
};
```

---

## 窗口模块 (Window Module)

命名空间：`xiaopeng::window`

### Window 类 (抽象基类)

窗口操作的抽象接口。

```cpp
class Window {
public:
    virtual ~Window() = default;

    // 初始化窗口 (宽, 高, 标题)
    virtual bool initialize(int width, int height, const std::string& title) = 0;
    
    // 将位图画布内容绘制到窗口
    virtual void drawBuffer(const xiaopeng::renderer::BitmapCanvas& canvas) = 0;
    
    // 处理窗口事件 (如退出、重绘等)，返回 false 表示请求退出
    virtual bool processEvents() = 0;
    
    // 窗口是否处于打开状态
    virtual bool isOpen() const = 0;
};
```

### SdlWindow 类

基于 SDL2 的交互式窗口实现。

```cpp
class SdlWindow : public Window {
public:
    SdlWindow();
    
    // 实现 Window 接口
    bool initialize(int width, int height, const std::string& title) override;
    void drawBuffer(const xiaopeng::renderer::BitmapCanvas& canvas) override;
    bool processEvents() override;
    bool isOpen() const override;
};
```

---


---

## 脚本引擎模块 (Scripting Module)

命名空间：`xiaopeng::script`

### ScriptEngine 类 (Script Engine Class)

基于 QuickJS 的轻量级 JavaScript 引擎封装。

```cpp
class ScriptEngine {
public:
    ScriptEngine();
    ~ScriptEngine();

    // 初始化引擎 (创建 Runtime 和 Context)
    // 如果构建中禁用了 QuickJS，此方法将返回 false
    bool initialize();

    // 执行 JavaScript 代码
    // @param script: 要执行的 JS 代码
    // @param filename: 用于错误堆栈的文件名 (默认为 "<input>")
    void evaluate(const std::string& script, const std::string& filename = "<input>");

    // 检查引擎是否已成功初始化
    bool isValid() const;
};
```

---

## 版本历史

### v0.4.0 (开发中 - JavaScript Engine)

- 集成 **QuickJS** 引擎 core
- 实现 ScriptEngine 基础封装
- 支持执行简单 JavaScript 代码
- 完善 CMake 构建系统以支持 C/C++ 混合编译

### v0.3.0 (Interactive Window)

- 完成 **布局引擎 (Layout Engine)** 与 **渲染模块 (Renderer)** 开发
- 新增 **窗口模块 (Window Module)**：
  - 集成 **SDL2** 实现交互式图形窗口
  - 支持实时位图渲染与事件处理循环
  - 自动环境检测与降级机制（ headless 模式自动切换至 PPM 导出）
- 布局特性：
  - 实现 BFC/IFC 混合排版
  - 支持自动换行 (Text Wrapping) 与行盒 (LineBox) 管理
  - 支持外边距、内边距、边框等标准盒模型计算
- 渲染特性：
  - 实现自研软件光栅化器 (Software Rasterizer)
  - 导出支持：PPM 图像格式导出
  - 字体支持：内置 8x8 VGA 点阵字体
- 包含 **100+** 个单元测试，全面覆盖绘制路径与布局逻辑
- 优化了 CMake 构建系统，支持第三方库（SDL2）的自动下载与配置

### v0.2.0

- 完成HTML解析器模块开发
- 实现HTML5词法分析器 (Tokenizer)
  - 支持50+种状态转换
  - 支持字符引用解码
  - 符合W3C HTML5规范
- 实现DOM树构建器 (TreeBuilder)
  - 实现23种插入模式
  - 支持元素作用域检查
  - 支持隐式标签生成
- 实现DOM节点系统
  - Node、Element、Document类
  - TextNode、CommentNode、DocumentTypeNode类
  - DocumentFragment类
- 提供统一HTML解析器接口
  - 静态便捷方法
  - 资源提取功能
- 更新HtmlHandler集成新解析器
- 包含 **79** 个单元测试（Loader模块44个 + HTML解析器模块35个）

### v0.1.0

- 完成网络与资源加载层 (Loader) 开发
- 支持 HTTP/1.1 和 HTTP/2 协议
- 实现资源缓存系统
- 实现连接池管理
- 实现同源策略和 CORS 检查
- 支持 HTML/CSS/JavaScript/图像资源处理
- 提供同步和异步加载 API
- 包含 44 个单元测试

---

## 脚本引擎模块 (Script Engine)

XiaopengKernel 集成了 QuickJS 引擎，提供现代 JavaScript (ES2020) 支持。

### 1. ScriptEngine

脚本引擎核心类，负责管理 JS 运行时和上下文。

```cpp
#include <script/script_engine.hpp>

// 初始化引擎
xiaopeng::script::ScriptEngine engine;
if (engine.initialize()) {
    // 执行 JS 代码
    engine.evaluate("console.log('Hello World');");
    
    // 注册 DOM
    engine.registerDOM(document);
}
```

### 2. DOM 绑定 (DOM Binding)

目前支持以下 DOM 对象和 API：

#### Document 对象 (`document`)

- `getElementById(id)`: 返回具有指定 ID 的 Element 对象。
- `createElement(tagName)`: 创建一个新的 Element 对象。
- `addEventListener(type, callback)`: 在 Document 上注册事件监听器。
- `removeEventListener(type, callback)`: 移除 Document 上的事件监听器。
- `dispatchEvent(type)`: 触发 Document 上的指定事件。

#### Element 对象

- `id`: 获取元素 ID (只读)。
- `tagName`: 获取元素标签名 (只读, 大写)。
- `innerHTML`: 获取或设置元素的 HTML 内容 (Setter 会解析 HTML 片段并替换子节点)。
- `style`: 获取或设置 `style` 属性字符串。
- `setAttribute(name, value)`: 设置元素属性。
- `appendChild(child)`: 将子元素添加到当前元素末尾，返回添加的子元素。
- `removeChild(child)`: 移除指定的子元素，返回被移除的子元素。
- `addEventListener(type, callback)`: 注册事件监听器。
- `removeEventListener(type, callback)`: 移除事件监听器。
- `dispatchEvent(type)`: 触发指定事件。

### 3. 事件系统 (Event System)

事件系统允许 JavaScript 代码为 DOM 节点注册/移除事件监听器，并支持手动触发事件。

#### addEventListener(type, callback)

为当前节点注册一个事件监听器。`type` 为事件名称字符串，`callback` 为回调函数。同一事件类型可注册多个监听器，按注册顺序触发。

```javascript
element.addEventListener("click", function(event) {
    console.log("Clicked! type=" + event.type);
});
```

#### removeEventListener(type, callback)

移除当前节点上最后注册的指定类型事件监听器。

```javascript
element.removeEventListener("click", handler);
```

#### dispatchEvent(type)

手动触发指定类型的事件。所有已注册该事件的监听器都会被调用，回调函数接收一个事件对象作为参数。

```javascript
element.dispatchEvent("click");
```

#### 事件对象 (Event Object)

回调函数接收的事件对象包含以下属性：

| 属性 | 类型 | 说明 |
|------|------|------|
| `type` | `string` | 事件类型名称 |
| `target` | `null` | 事件目标节点（当前版本为 null） |
| `bubbles` | `boolean` | 是否冒泡（当前版本为 false） |
| `cancelable` | `boolean` | 是否可取消（当前版本为 false） |

#### 完整示例

```javascript
// 注册事件
var demo = document.getElementById("demo");
demo.addEventListener("click", function(e) {
    console.log("Handler A fired! type=" + e.type);
});
demo.addEventListener("click", function(e) {
    console.log("Handler B fired!");
});

// 触发事件 (两个 handler 都会执行)
demo.dispatchEvent("click");

// 移除最后注册的 handler (Handler B)
demo.removeEventListener("click", null);

// 再次触发 (只有 Handler A 会执行)
demo.dispatchEvent("click");

// Document 级事件
document.addEventListener("DOMContentLoaded", function(e) {
    console.log("DOM ready!");
});
document.dispatchEvent("DOMContentLoaded");
```

### 4. Console 对象 (`console`)

- `console.log(...)`: 打印普通日志。
- `console.info(...)`: 打印信息日志。
- `console.warn(...)`: 打印警告日志。
- `console.error(...)`: 打印错误日志。

支持多参数打印，参数自动转换为字符串。

## 碰撞检测与鼠标交互 (Hit Testing)

### MouseEvent 结构体

在 `window/window.hpp` 中定义，表示一次鼠标点击事件：

```cpp
struct MouseEvent {
  int x = 0;        // 点击的 X 坐标 (像素)
  int y = 0;        // 点击的 Y 坐标 (像素)
  MouseButton button; // Left, Right, Middle
};
```

### Window::pendingMouseClicks()

返回自上次调用以来所有待处理的鼠标点击事件，并清空队列。

```cpp
auto clicks = window.pendingMouseClicks();
for (const auto &click : clicks) {
  // 处理 click.x, click.y, click.button
}
```

### HitTest::test()

在 `layout/hit_test.hpp` 中定义。根据像素坐标查找布局树中最深层的 DOM 元素：

```cpp
#include <layout/hit_test.hpp>

// 查找 (x, y) 处最深层的 DOM 元素
auto hitNode = HitTest::test(layoutRoot, x, y);
if (hitNode) {
  // hitNode 是一个 dom::NodePtr
}
```

**算法**：
- 递归遍历布局树（子节点优先，深度优先）
- 使用 **border box**（content + padding + border）作为碰撞区域
- 返回最深层匹配的 DOM 节点，无匹配返回 `nullptr`

### 完整事件流

```
SDL_MOUSEBUTTONDOWN
       ↓
  processEvents()  →  m_pendingClicks
       ↓
  pendingMouseClicks()  →  vector<MouseEvent>
       ↓
  HitTest::test(layoutRoot, x, y)  →  dom::NodePtr
       ↓
  查找 eventListenerIds_["click"]
       ↓
  EventBinding::dispatch() → 调用 JS 回调函数
```

## 定时器 API (Timer API)

在 `script/timer_binding.hpp` 中定义，提供浏览器标准的异步定时器功能。

### setTimeout(callback, delayMs)

延迟 `delayMs` 毫秒后执行 `callback`，仅执行一次。返回 `timerId`。

```javascript
var id = setTimeout(function() {
    console.log("Fired after 1 second!");
}, 1000);
```

### setInterval(callback, intervalMs)

每隔 `intervalMs` 毫秒重复执行 `callback`。返回 `timerId`。

```javascript
var count = 0;
var id = setInterval(function() {
    count++;
    console.log("Tick #" + count);
    if (count >= 5) {
        clearInterval(id);
    }
}, 100);
```

### clearTimeout(id) / clearInterval(id)

取消通过 `setTimeout` 或 `setInterval` 创建的定时器。

```javascript
var id = setTimeout(function() {
    console.log("This will NOT print");
}, 500);
clearTimeout(id); // 取消
```

### 主循环集成

定时器通过 `ScriptEngine::tickTimers()` 在每帧中检查触发：

```cpp
// 在主事件循环中
while (running) {
    window.processEvents();
    // ... hit testing ...
    
    if (scriptEngine.tickTimers()) {
        needsRepaint = true; // 定时器可能修改了 DOM
    }
    
    if (needsRepaint) {
        painter.paint(layoutRoot, canvas);
        window.drawBuffer(canvas);
    }
}
```

---

## 浏览器引擎 (BrowserEngine)

`BrowserEngine` 类提供了最简化的接口来运行整个内核管线。

### Config 结构

用于配置引擎参数。

```cpp
struct Config {
    int windowWidth = 800;   // 窗口宽度
    int windowHeight = 600;  // 窗口高度
    std::string title = "..."; // 窗口标题
    std::string baseUrl;     // 用于解析相对路径的基础 URL
};
```

### 方法

#### initialize(const Config& cfg)

初始化所有子模组（ScriptEngine, SdlWindow, PaintingAlgorithm 等）。

#### loadHTML(const std::string& html)

直接从字符串加载 HTML。会自动提取并应用内联 CSS 和 JS。

#### loadFile(const std::string& filepath)

从本地文件路径加载 HTML。

#### run()

进入主事件循环。处理 SDL 事件、分发点击事件到 DOM、处理定时器、自动重绘。该方法为阻塞调用，直到窗口关闭。

#### shutdown()

关闭并清理引擎资源。会依次关闭 Loader（停止网络请求）、释放 Canvas 内存。

### Config 配置

```cpp
struct Config {
    int windowWidth = 800;              // 窗口宽度
    int windowHeight = 600;             // 窗口高度
    std::string title = "XiaopengKernel Browser"; // 窗口标题
    std::string baseUrl;                // 基础 URL，用于解析相对路径
    bool enableNetworkLoading = true;   // 是否启用外部资源加载
    loader::LoaderConfig loaderConfig;  // Loader 模块配置（详见 Loader 章节）
};
```

**配置项说明：**

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `windowWidth` | `int` | `800` | SDL 窗口宽度（像素） |
| `windowHeight` | `int` | `600` | SDL 窗口高度（像素） |
| `title` | `std::string` | `"XiaopengKernel Browser"` | 窗口标题栏文字 |
| `baseUrl` | `std::string` | `""` | 基础 URL，用于将相对路径解析为绝对 URL |
| `enableNetworkLoading` | `bool` | `true` | 是否启用网络加载外部 CSS/JS |
| `loaderConfig` | `LoaderConfig` | (默认) | Loader 模块配置，包含 HTTP 客户端、缓存、调度器等设置 |

### 外部资源加载 (External Resource Loading)

当 `enableNetworkLoading = true` 时，`BrowserEngine` 会自动处理以下外部资源：

#### 外部 CSS 样式表

引擎在 `extractAndApplyCSS()` 阶段自动扫描 `<link rel="stylesheet" href="...">` 标签，通过 Loader 模块发起 HTTP 请求下载 CSS 文件，并与内联 `<style>` 标签的内容合并后统一解析。

```html
<!-- 以下标签会被自动识别并下载 -->
<link rel="stylesheet" href="css/main.css">
<link rel="stylesheet" href="https://cdn.example.com/bootstrap.css">
```

#### 外部 JavaScript 脚本

引擎在 `extractAndExecuteScripts()` 阶段自动扫描 `<script src="...">` 标签，通过 Loader 模块下载 JS 文件后交给 ScriptEngine 执行。同时支持内联和外部混合：

```html
<script src="lib/utils.js"></script>     <!-- 先下载并执行 -->
<script>console.log("inline");</script>  <!-- 再执行内联脚本 -->
<script src="app.js"></script>           <!-- 最后下载并执行 -->
```

#### Base URL 解析

相对路径通过以下优先级进行解析：

1. **`<base href="...">` 标签**（最高优先级）：HTML 中的 `<base>` 标签指定的 URL
2. **`Config::baseUrl`**：通过代码配置的基础 URL
3. **文件路径推导**：当使用 `loadFile()` 时，自动从文件路径提取目录作为 `file://` 基础 URL

```cpp
// 手动设置 Base URL
cfg.baseUrl = "https://example.com/assets/";

// loadFile() 自动推导:
// 加载 "j:/project/pages/index.html"
// 自动设置 baseUrl = "file:///j:/project/pages/"
engine.loadFile("j:/project/pages/index.html");
```

URL 解析使用 `Url::resolve()` 方法，支持标准的相对路径语法：

| 相对路径 | Base URL | 解析结果 |
|----------|----------|----------|
| `style.css` | `https://example.com/page/` | `https://example.com/page/style.css` |
| `../css/main.css` | `https://example.com/page/` | `https://example.com/css/main.css` |
| `/assets/app.js` | `https://example.com/page/` | `https://example.com/assets/app.js` |
| `https://cdn.io/lib.js` | (任意) | `https://cdn.io/lib.js` (绝对 URL 不变) |

### 示例用法

#### 基础用法（内联内容）

```cpp
#include <engine/browser_engine.hpp>

int main() {
    xiaopeng::engine::BrowserEngine engine;
    
    // 初始化
    if (!engine.initialize({})) return 1;

    // 加载内容
    engine.loadHTML("<html><body><h1>Hello World</h1></body></html>");

    // 启动交互
    return engine.run();
}
```

#### 加载外部资源

```cpp
#include <engine/browser_engine.hpp>

int main() {
    xiaopeng::engine::BrowserEngine engine;
    xiaopeng::engine::BrowserEngine::Config cfg;
    
    // 设置基础 URL 用于解析相对路径
    cfg.baseUrl = "https://example.com/";
    cfg.enableNetworkLoading = true;  // 默认即为 true

    if (!engine.initialize(cfg)) return 1;

    // HTML 中的 <link> 和 <script src> 会自动下载
    engine.loadHTML(R"(
        <html>
          <head>
            <link rel="stylesheet" href="css/main.css">
            <script src="js/app.js"></script>
          </head>
          <body>
            <div class="container">Hello</div>
          </body>
        </html>
    )");

    return engine.run();
}
```

#### 加载本地 HTML 文件

```cpp
#include <engine/browser_engine.hpp>

int main(int argc, char* argv[]) {
    xiaopeng::engine::BrowserEngine engine;
    xiaopeng::engine::BrowserEngine::Config cfg;
    cfg.title = "Local Page Viewer";

    if (!engine.initialize(cfg)) return 1;

    // loadFile() 自动推导 base URL
    // 文件中引用的相对路径资源会正确解析
    engine.loadFile(argv[1]);

    return engine.run();
}
```

#### 禁用网络加载（纯离线模式）

```cpp
xiaopeng::engine::BrowserEngine::Config cfg;
cfg.enableNetworkLoading = false;  // 禁用外部资源加载

// 此模式下只处理内联 <style> 和 <script> 内容
// 外部 <link> 和 <script src> 会被跳过（打印日志提示）
```

