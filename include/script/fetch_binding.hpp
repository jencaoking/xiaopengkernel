#pragma once

#include <quickjs.h>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <script/fetch_types.hpp>

namespace xiaopeng {
namespace loader {
class HttpClient;
class HttpResponse;
} // namespace loader

namespace script {

class JSBinding;

class HeadersBinding {
public:
    static JSClassID s_classId;
    
    static void registerClass(JSContext* ctx);
    static JSValue create(JSContext* ctx, const std::map<std::string, std::string>& headers);

private:
    struct HeadersData {
        std::map<std::string, std::string> headers;
    };

    static HeadersData* getData(JSContext* ctx, JSValueConst this_val);

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
    static JSValue headers_append(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv);
};

class ResponseBinding {
public:
    static JSClassID s_classId;
    
    static void registerClass(JSContext* ctx);
    static JSValue create(JSContext* ctx, const FetchResponseData& data);
    static JSValue createError(JSContext* ctx, const std::string& message);

    static JSValue json(JSContext* ctx, JSValueConst this_val,
                        int argc, JSValueConst* argv);
    static JSValue text(JSContext* ctx, JSValueConst this_val,
                        int argc, JSValueConst* argv);
    static JSValue arrayBuffer(JSContext* ctx, JSValueConst this_val,
                               int argc, JSValueConst* argv);
    static JSValue blob(JSContext* ctx, JSValueConst this_val,
                        int argc, JSValueConst* argv);
    static JSValue clone(JSContext* ctx, JSValueConst this_val,
                         int argc, JSValueConst* argv);

private:
    struct ResponseData {
        FetchResponseData response;
        bool bodyUsed = false;
        bool isError = false;
        std::string errorMessage;
    };

    static ResponseData* getData(JSContext* ctx, JSValueConst this_val);

    static JSValue response_constructor(JSContext* ctx, JSValueConst new_target,
                                       int argc, JSValueConst* argv);
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
    static JSValue response_get_isError(JSContext* ctx, JSValueConst this_val,
                                         int argc, JSValueConst* argv);

    static JSValue createBodyPromise(JSContext* ctx, JSValueConst this_val,
                                     const std::function<JSValue(JSContext*, const std::string&)>& bodyConverter);
};

class FetchBinding {
public:
    static void init(JSContext* ctx, std::shared_ptr<loader::HttpClient> httpClient);

private:
    static std::shared_ptr<loader::HttpClient> s_httpClient;

    static JSValue fetch_function(JSContext* ctx, JSValueConst this_val,
                                  int argc, JSValueConst* argv);
    
    static std::optional<FetchRequestInit> parseRequestInit(JSContext* ctx,
                                                             JSValueConst options);
    
    static FetchResponseData convertResponse(const loader::HttpResponse& response);
    
    static JSValue createFetchPromise(JSContext* ctx,
                                       const std::string& url,
                                       const FetchRequestInit& init);
    
    static void executeHttpRequest(const std::string& url,
                                    const FetchRequestInit& init,
                                    JSContext* ctx,
                                    JSValue promise);
    
    static JSValue createTypeError(JSContext* ctx, const std::string& message);
    
    static void resolvePromise(JSContext* ctx, JSValue promise, JSValue value);
    static void rejectPromise(JSContext* ctx, JSValue promise, JSValue error);
};

} // namespace script
} // namespace xiaopeng
