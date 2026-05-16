# Fetch API Implementation Plan

## 1. Overview

This document outlines the implementation plan for the Web Fetch API in XiaopengKernel, following the `cpp-dev-standards` guidelines.

### 1.1 Objectives

- Implement `fetch()` global function
- Create `Request`, `Response`, and `Headers` JS bindings
- Integrate with existing Promise system and event loop
- Use existing `HttpClient` from loader module

### 1.2 Target API

```javascript
// Basic usage
fetch('https://api.example.com/data')
  .then(response => response.json())
  .then(data => console.log(data));

// With options
fetch('https://api.example.com/data', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ name: 'test' })
});

// With async/await
const response = await fetch('https://api.example.com/data');
const data = await response.json();
```

---

## 2. Architecture Design

### 2.1 Component Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                      JavaScript Layer                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌────────────┐  │
│  │  fetch   │  │ Request  │  │ Response │  │  Headers   │  │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └─────┬──────┘  │
└───────┼─────────────┼─────────────┼───────────────┼──────────┘
        │             │             │               │
        ▼             ▼             ▼               │
┌─────────────────────────────────────────────────────────────┐
│                     Binding Layer (C++)                      │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              FetchBinding class                       │   │
│  │  - fetch_init(): Register global fetch function      │   │
│  │  - createPromise(): Create fetch Promise             │   │
│  │  - handleResponse(): Convert HttpResponse to JS     │   │
│  └──────────────────────────┬───────────────────────────┘   │
│                             │                                │
│  ┌────────────┐  ┌──────────┴─────┐  ┌────────────────┐    │
│  │RequestBind │  │ ResponseBinding │  │ HeadersBinding  │    │
│  └────────────┘  └─────────────────┘  └────────────────┘    │
└─────────────────────────────┬───────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                     Service Layer                           │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              HttpClient (existing)                   │   │
│  │  - executeAsync(): Returns Future<Result<Response>>  │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 File Structure

```
include/script/
├── fetch_binding.hpp      # NEW: Main fetch API implementation
└── fetch_types.hpp       # NEW: Request/Response/Headers types

src/script/
└── fetch_binding.cpp     # NEW: Implementation
```

---

## 3. Implementation Details

### 3.1 Request Class (C++ side)

```cpp
// include/script/fetch_types.hpp
#pragma once

#include <string>
#include <map>
#include <optional>

namespace xiaopeng {
namespace script {

struct FetchRequestInit {
    std::string method = "GET";
    std::map<std::string, std::string> headers;
    std::optional<std::string> body;
    std::string referrer;
    std::string referrerPolicy;
    std::string mode;           // "cors", "no-cors", "same-origin"
    std::string credentials;     // "omit", "same-origin", "include"
    std::string cache;          // "default", "no-store", "reload"
    std::string redirect;       // "follow", "error", "manual"
    std::string integrity;
    bool keepalive = false;
};

struct FetchResponseData {
    uint16_t status;
    std::string statusText;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string url;
    bool redirected;
    std::string type;            // "basic", "cors", "error", "opaque"
};

class FetchRequest {
public:
    explicit FetchRequest(const std::string& input,
                         std::optional<FetchRequestInit> init = std::nullopt);
    
    const std::string& url() const { return m_url; }
    const std::string& method() const { return m_method; }
    const std::map<std::string, std::string>& headers() const { return m_headers; }
    const std::optional<std::string>& body() const { return m_body; }
    const std::string& mode() const { return m_mode; }
    
private:
    std::string m_url;
    std::string m_method;
    std::map<std::string, std::string> m_headers;
    std::optional<std::string> m_body;
    std::string m_mode;
};

} // namespace script
} // namespace xiaopeng
```

### 3.2 HeadersBinding Class

```cpp
// Part of fetch_binding.hpp

class HeadersBinding {
public:
    static JSClassID s_classId;
    
    static void registerClass(JSContext* ctx);
    static JSValue create(JSContext* ctx, const std::map<std::string, std::string>& headers);
    
private:
    static JSValue headers_constructor(JSContext* ctx, JSValueConst new_target,
                                        int argc, JSValueConst* argv);
    static JSValue headers_get(JSContext* ctx, JSValueConst this_val,
                               int argc, JSValueConst* argv);
    static JSValue headers_set(JSContext* ctx, JSValueConst this_val,
                               int argc, JSValueConst* argv);
    static JSValue headers_has(JSContext* ctx, JSValueConst this_val,
                               int argc, JSValueConst* argv);
    static JSValue headers_delete(JSContext* ctx, JSValueConst this_val,
                                  int argc, JSValueConst* argv);
    static JSValue headers_keys(JSContext* ctx, JSValueConst this_val,
                                int argc, JSValueConst* argv);
    static JSValue headers_values(JSContext* ctx, JSValueConst this_val,
                                  int argc, JSValueConst* argv);
    static JSValue headers_entries(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv);
    static JSValue headers_forEach(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv);
};
```

### 3.3 ResponseBinding Class

```cpp
// Part of fetch_binding.hpp

class ResponseBinding {
public:
    static JSClassID s_classId;
    
    static void registerClass(JSContext* ctx);
    static JSValue create(JSContext* ctx, const FetchResponseData& data);
    
    // Static methods
    static JSValue json(JSContext* ctx, JSValueConst this_val,
                        int argc, JSValueConst* argv);
    static JSValue text(JSContext* ctx, JSValueConst this_val,
                        int argc, JSValueConst* argv);
    static JSValue blob(JSContext* ctx, JSValueConst this_val,
                        int argc, JSValueConst* argv);
    static JSValue arrayBuffer(JSContext* ctx, JSValueConst this_val,
                               int argc, JSValueConst* argv);
    static JSValue clone(JSContext* ctx, JSValueConst this_val,
                         int argc, JSValueConst* argv);

private:
    static JSValue response_get_status(JSContext* ctx, JSValueConst this_val,
                                       int argc, JSValueConst* argv);
    static JSValue response_get_statusText(JSContext* ctx, JSValueConst this_val,
                                            int argc, JSValueConst* argv);
    static JSValue response_get_ok(JSContext* ctx, JSValueConst this_val,
                                    int argc, JSValueConst* argv);
    static JSValue response_get_headers(JSContext* ctx, JSValueConst this_val,
                                         int argc, JSValueConst* argv);
    static JSValue response_get_url(JSContext* ctx, JSValueConst this_val,
                                    int argc, JSValueConst* argv);
    static JSValue response_get_redirected(JSContext* ctx, JSValueConst this_val,
                                            int argc, JSValueConst* argv);
    static JSValue response_get_type(JSContext* ctx, JSValueConst this_val,
                                      int argc, JSValueConst* argv);
    static JSValue response_get_body(JSContext* ctx, JSValueConst this_val,
                                      int argc, JSValueConst* argv);
    static JSValue response_get_bodyUsed(JSContext* ctx, JSValueConst this_val,
                                         int argc, JSValueConst* argv);
};
```

### 3.4 FetchBinding Main Class

```cpp
// include/script/fetch_binding.hpp

class FetchBinding {
public:
    static void init(JSContext* ctx, std::shared_ptr<loader::HttpClient> httpClient);
    
private:
    // Main fetch function
    static JSValue fetch_function(JSContext* ctx, JSValueConst this_val,
                                  int argc, JSValueConst* argv);
    
    // Parse Request options from JS object
    static std::optional<FetchRequestInit> parseRequestInit(JSContext* ctx,
                                                             JSValueConst options);
    
    // Convert HttpResponse to FetchResponseData
    static FetchResponseData convertResponse(const loader::HttpResponse& response);
    
    // Create and resolve/reject Promise
    static JSValue createFetchPromise(JSContext* ctx,
                                       const std::string& url,
                                       const FetchRequestInit& init);
    
    // Thread-safe HTTP execution
    static void executeHttpRequest(std::string url,
                                    FetchRequestInit init,
                                    JSContext* ctx,
                                    JSValue promise);
    
    static std::shared_ptr<loader::HttpClient> s_httpClient;
};
```

---

## 4. Implementation Steps

### Phase 1: Type Definitions and Headers

**Task 1.1: Create fetch_types.hpp**

Create `include/script/fetch_types.hpp` with:
- `FetchRequestInit` struct
- `FetchResponseData` struct
- `FetchRequest` class

**Task 1.2: Create fetch_binding.hpp**

Create `include/script/fetch_binding.hpp` with:
- `HeadersBinding` class declaration
- `ResponseBinding` class declaration
- `FetchBinding` class declaration

**Task 1.3: Create fetch_binding.cpp**

Create `src/script/fetch_binding.cpp` with:
- Class method implementations
- Promise creation and resolution
- HTTP request execution

### Phase 2: Headers Implementation

**Task 2.1: HeadersBinding::registerClass()**

Register Headers class with QuickJS:
- Constructor
- Instance properties
- Methods: get, set, has, delete, keys, values, entries, forEach

**Task 2.2: HeadersBinding Methods**

Implement all Headers methods:
```cpp
static JSValue headers_get(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
static JSValue headers_set(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
static JSValue headers_has(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
static JSValue headers_delete(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
```

### Phase 3: Response Implementation

**Task 3.1: ResponseBinding::registerClass()**

Register Response class with QuickJS:
- Constructor
- Read-only properties: status, statusText, ok, headers, url, redirected, type
- Body properties: body, bodyUsed
- Methods: json(), text(), blob(), arrayBuffer(), clone()

**Task 3.2: Body Methods**

Implement body consumption methods:
```cpp
static JSValue json(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
static JSValue text(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
static JSValue arrayBuffer(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv);
```

### Phase 4: Fetch Function Implementation

**Task 4.1: Parse Request Options**

Implement `parseRequestInit()`:
- Extract method, headers, body from JS object
- Handle default values per Fetch spec
- Validate input

**Task 4.2: Create Fetch Promise**

Implement `createFetchPromise()`:
- Create Pending Promise
- Execute HTTP request asynchronously
- Resolve with Response or reject with TypeError

**Task 4.3: HTTP Request Execution**

Implement `executeHttpRequest()`:
- Use existing `HttpClient`
- Convert headers to curl format
- Handle response and errors

### Phase 5: Integration

**Task 5.1: Register in ScriptEngine**

Update `ScriptEngine` to call `FetchBinding::init()`:
```cpp
void ScriptEngine::initialize() {
    // ... existing code ...
    FetchBinding::init(m_ctx, std::make_shared<loader::HttpClient>());
}
```

**Task 5.2: Error Handling**

Implement proper error types:
- TypeError for invalid URL
- TypeError for body already used
- Network error handling

### Phase 6: Testing

**Task 6.1: Unit Tests**

Create `tests/test_fetch.cpp`:
- Test Headers operations
- Test Response creation
- Test basic fetch calls

**Task 6.2: Integration Tests**

Test complete workflows:
```javascript
// Test 1: Simple GET
fetch('https://httpbin.org/get')
  .then(r => r.json())
  .then(console.log);

// Test 2: POST with JSON
fetch('https://httpbin.org/post', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ test: true })
}).then(r => r.json());

// Test 3: Error handling
fetch('https://httpbin.org/status/404')
  .then(r => {
    if (!r.ok) throw new Error('HTTP error');
    return r.json();
  })
  .catch(console.error);
```

---

## 5. C++ Standards Compliance

### 5.1 Naming Conventions

| Element | Pattern | Example |
|---------|---------|---------|
| Classes | PascalCase | `FetchBinding`, `HeadersBinding` |
| Methods | snake_case | `headers_get`, `response_get_status` |
| Members | m_ prefix | `m_url`, `m_method` |
| Constants | UPPER_SNAKE | `DEFAULT_METHOD` |
| Files | snake_case | `fetch_binding.hpp` |

### 5.2 Modern C++ Features

- **Smart Pointers**: Use `std::shared_ptr<loader::HttpClient>`
- **std::optional**: For nullable fields like body, referrer
- **std::map**: For headers storage
- **Lambda Expressions**: For async callbacks
- **Structured Bindings**: Where applicable

### 5.3 Error Handling

```cpp
// Use exceptions for exceptional cases
if (!urlValid) {
    return JS_ThrowTypeError(ctx, "Failed to parse URL: %s", url.c_str());
}

// Return JS_EXCEPTION for binding errors
if (!ctx) return JS_EXCEPTION;
```

### 5.4 RAII Compliance

```cpp
// Proper resource management
class ResponseBinding {
private:
    struct ResponseData {
        FetchResponseData response;
        bool bodyUsed = false;
    };
    
    // Body is consumed only once
    JSValue consumeBody(JSContext* ctx, JSValueConst this_val) {
        auto* data = static_cast<ResponseData*>(JS_GetOpaque(this_val, s_classId));
        if (data->bodyUsed) {
            JS_ThrowTypeError(ctx, "Body already used");
            return JS_EXCEPTION;
        }
        data->bodyUsed = true;
        // ... return body
    }
};
```

---

## 6. Files to Create/Modify

### 6.1 New Files

| File | Description |
|------|-------------|
| `include/script/fetch_types.hpp` | Type definitions |
| `include/script/fetch_binding.hpp` | Binding declarations |
| `src/script/fetch_binding.cpp` | Implementation |
| `tests/test_fetch.cpp` | Unit tests |

### 6.2 Modified Files

| File | Changes |
|------|---------|
| `include/script/script_engine.hpp` | Add FetchBinding include |
| `src/script/script_engine.cpp` | Call FetchBinding::init() |
| `docs/API.md` | Document fetch API |

---

## 7. Estimated Timeline

| Phase | Task | Estimated Time |
|-------|------|----------------|
| 1 | Type definitions and headers | 2 hours |
| 2 | Headers implementation | 3 hours |
| 3 | Response implementation | 3 hours |
| 4 | Fetch function | 4 hours |
| 5 | Integration | 2 hours |
| 6 | Testing | 3 hours |
| **Total** | | **~17 hours** |

---

## 8. Dependencies

### 8.1 Internal Dependencies

- `loader::HttpClient` - Existing HTTP client
- `loader::HttpRequest` / `HttpResponse` - Existing types
- `PromiseBinding` - Existing Promise system
- `JSBinding::toStdString()` - String conversion utilities

### 8.2 External Dependencies

- QuickJS (already integrated)
- curl (already integrated)

---

## 9. Success Criteria

### 9.1 Functional Requirements

- [ ] `fetch(url)` returns a Promise
- [ ] `fetch(url, options)` works with method, headers, body
- [ ] Response.json() parses JSON body
- [ ] Response.text() returns text body
- [ ] Response.headers provides access to headers
- [ ] Proper error handling for network failures
- [ ] Body cannot be consumed twice

### 9.2 Non-Functional Requirements

- [ ] Follows cpp-dev-standards naming conventions
- [ ] Uses modern C++ features (smart pointers, std::optional)
- [ ] Thread-safe HTTP execution
- [ ] All tests pass
- [ ] API documentation updated

---

## 10. API Reference

### 10.1 fetch()

```javascript
Promise<Response> fetch(input[, init])
```

**Parameters:**
- `input`: USVString (URL) or Request object
- `init` (optional): RequestInit object

**RequestInit properties:**
```javascript
{
  method: string,           // GET, POST, PUT, DELETE, etc.
  headers: HeadersInit,    // object or Headers
  body: BodyInit,          // string, ArrayBuffer, FormData, etc.
  referrer: string,
  referrerPolicy: string,
  mode: string,            // 'cors', 'no-cors', 'same-origin'
  credentials: string,     // 'omit', 'same-origin', 'include'
  cache: string,           // 'default', 'no-store', 'reload'
  redirect: string,        // 'follow', 'error', 'manual'
  integrity: string,
  keepalive: boolean
}
```

### 10.2 Response

```javascript
new Response(body, init)
```

**Properties:**
- `ok`: boolean
- `status`: number
- `statusText`: string
- `headers`: Headers
- `url`: string
- `redirected`: boolean
- `type`: string
- `body`: ReadableStream (not implemented in v1)
- `bodyUsed`: boolean

**Methods:**
- `json()`: Promise<any>
- `text()`: Promise<string>
- `arrayBuffer()`: Promise<ArrayBuffer>
- `clone()`: Response

### 10.3 Headers

```javascript
new Headers(init)
```

**Methods:**
- `get(name)`: string | null
- `set(name, value)`: void
- `has(name)`: boolean
- `delete(name)`: void
- `keys()`: Iterator
- `values()`: Iterator
- `entries()`: Iterator
- `forEach(fn)`: void
