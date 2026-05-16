#include <script/fetch_binding.hpp>
#include <script/promise_binding_simple.hpp>
#include <script/js_binding.hpp>
#include <loader/http_client.hpp>
#include <loader/http_message.hpp>
#include <loader/url.hpp>
#include <engine/event_loop.hpp>
#include <cstring>
#include <sstream>
#include <thread>
#include <future>
#include <algorithm>
#include <vector>

namespace xiaopeng {
namespace script {

JSClassID HeadersBinding::s_classId = 0;
JSClassID ResponseBinding::s_classId = 0;
std::shared_ptr<loader::HttpClient> FetchBinding::s_httpClient;

HeadersBinding::HeadersData* HeadersBinding::getData(JSContext* ctx, JSValueConst this_val) {
    (void)ctx;
    return static_cast<HeadersData*>(JS_GetOpaque(this_val, s_classId));
}

JSValue HeadersBinding::create(JSContext* ctx, const std::map<std::string, std::string>& headers) {
    JSValue obj = JS_NewObjectClass(ctx, s_classId);
    auto* data = new HeadersData();
    data->headers = headers;
    JS_SetOpaque(obj, data);
    return obj;
}

JSValue HeadersBinding::headers_constructor(JSContext* ctx, JSValueConst new_target,
                                           int argc, JSValueConst* argv) {
    (void)new_target;
    JSValue obj = JS_NewObjectClass(ctx, s_classId);
    auto* data = new HeadersData();
    
    if (argc >= 1) {
        JSValue init = argv[0];
        
        if (JS_IsObject(init)) {
            JSPropertyEnum* props = nullptr;
            uint32_t propCount = 0;
            
            if (JS_GetOwnPropertyNames(ctx, &props, &propCount, init,
                                       JS_GPN_STRING_MASK) == 0) {
                for (uint32_t i = 0; i < propCount; i++) {
                    JSValue key = JS_AtomToString(ctx, props[i].atom);
                    JSValue val = JS_GetProperty(ctx, init, props[i].atom);
                    
                    if (!JS_IsException(key) && !JS_IsException(val)) {
                        std::string k = JSBinding::toStdString(ctx, key);
                        std::string v = JSBinding::toStdString(ctx, val);
                        data->headers[k] = v;
                    }
                    
                    JS_FreeValue(ctx, key);
                    JS_FreeValue(ctx, val);
                }
                js_free(ctx, props);
            }
        } else if (JS_IsArray(ctx, init)) {
            JSValue length = JS_GetPropertyStr(ctx, init, "length");
            int64_t len = 0;
            JS_ToInt64(ctx, &len, length);
            JS_FreeValue(ctx, length);
            
            for (int64_t i = 0; i < len; i++) {
                JSValue item = JS_GetPropertyUint32(ctx, init, static_cast<uint32_t>(i));
                if (JS_IsArray(ctx, item)) {
                    JSValue key = JS_GetPropertyUint32(ctx, item, 0);
                    JSValue val = JS_GetPropertyUint32(ctx, item, 1);
                    
                    if (!JS_IsException(key) && !JS_IsException(val)) {
                        std::string k = JSBinding::toStdString(ctx, key);
                        std::string v = JSBinding::toStdString(ctx, val);
                        data->headers[k] = v;
                    }
                    
                    JS_FreeValue(ctx, key);
                    JS_FreeValue(ctx, val);
                }
                JS_FreeValue(ctx, item);
            }
        }
    }
    
    JS_SetOpaque(obj, data);
    return obj;
}

JSValue HeadersBinding::headers_get(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    auto* data = getData(ctx, this_val);
    if (!data || argc < 1)
        return JS_EXCEPTION;
    
    std::string name = JSBinding::toStdString(ctx, argv[0]);
    
    auto it = data->headers.find(name);
    if (it != data->headers.end()) {
        return JSBinding::toJSString(ctx, it->second);
    }
    return JS_NULL;
}

JSValue HeadersBinding::headers_set(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    auto* data = getData(ctx, this_val);
    if (!data || argc < 2)
        return JS_EXCEPTION;
    
    std::string name = JSBinding::toStdString(ctx, argv[0]);
    std::string value = JSBinding::toStdString(ctx, argv[1]);
    data->headers[name] = value;
    
    return JS_UNDEFINED;
}

JSValue HeadersBinding::headers_has(JSContext* ctx, JSValueConst this_val,
                                   int argc, JSValueConst* argv) {
    auto* data = getData(ctx, this_val);
    if (!data || argc < 1)
        return JS_EXCEPTION;
    
    std::string name = JSBinding::toStdString(ctx, argv[0]);
    return JS_NewBool(ctx, data->headers.count(name) > 0);
}

JSValue HeadersBinding::headers_delete(JSContext* ctx, JSValueConst this_val,
                                     int argc, JSValueConst* argv) {
    auto* data = getData(ctx, this_val);
    if (!data || argc < 1)
        return JS_EXCEPTION;
    
    std::string name = JSBinding::toStdString(ctx, argv[0]);
    data->headers.erase(name);
    
    return JS_UNDEFINED;
}

JSValue HeadersBinding::headers_append(JSContext* ctx, JSValueConst this_val,
                                      int argc, JSValueConst* argv) {
    auto* data = getData(ctx, this_val);
    if (!data || argc < 2)
        return JS_EXCEPTION;
    
    std::string name = JSBinding::toStdString(ctx, argv[0]);
    std::string value = JSBinding::toStdString(ctx, argv[1]);
    data->headers[name] = value;
    
    return JS_UNDEFINED;
}

JSValue HeadersBinding::headers_keys(JSContext* ctx, JSValueConst this_val,
                                    int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* data = getData(ctx, this_val);
    if (!data)
        return JS_EXCEPTION;
    
    JSValue arr = JS_NewArray(ctx);
    size_t idx = 0;
    for (const auto& [key, _] : data->headers) {
        JS_DefinePropertyValueUint32(ctx, arr, idx++,
            JSBinding::toJSString(ctx, key),
            JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    return arr;
}

JSValue HeadersBinding::headers_values(JSContext* ctx, JSValueConst this_val,
                                      int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* data = getData(ctx, this_val);
    if (!data)
        return JS_EXCEPTION;
    
    JSValue arr = JS_NewArray(ctx);
    size_t idx = 0;
    for (const auto& [_, value] : data->headers) {
        JS_DefinePropertyValueUint32(ctx, arr, idx++,
            JSBinding::toJSString(ctx, value),
            JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    return arr;
}

JSValue HeadersBinding::headers_entries(JSContext* ctx, JSValueConst this_val,
                                       int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* data = getData(ctx, this_val);
    if (!data)
        return JS_EXCEPTION;
    
    JSValue arr = JS_NewArray(ctx);
    size_t idx = 0;
    for (const auto& [key, value] : data->headers) {
        JSValue entry = JS_NewArray(ctx);
        JS_DefinePropertyValueUint32(ctx, entry, 0,
            JSBinding::toJSString(ctx, key),
            JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_DefinePropertyValueUint32(ctx, entry, 1,
            JSBinding::toJSString(ctx, value),
            JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_DefinePropertyValueUint32(ctx, arr, idx++, entry,
            JS_PROP_WRITABLE | JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
    }
    return arr;
}

JSValue HeadersBinding::headers_forEach(JSContext* ctx, JSValueConst this_val,
                                        int argc, JSValueConst* argv) {
    auto* data = getData(ctx, this_val);
    if (!data || argc < 1)
        return JS_EXCEPTION;
    
    JSValue callback = argv[0];
    JSValue thisArg = (argc >= 2) ? argv[1] : JS_UNDEFINED;
    
    for (const auto& [key, value] : data->headers) {
        JSValue args[3] = {
            JSBinding::toJSString(ctx, value),
            JSBinding::toJSString(ctx, key),
            JS_DupValue(ctx, this_val)
        };
        JS_Call(ctx, callback, thisArg, 3, args);
        JS_FreeValue(ctx, args[0]);
        JS_FreeValue(ctx, args[1]);
        JS_FreeValue(ctx, args[2]);
    }
    
    return JS_UNDEFINED;
}

void HeadersBinding::registerClass(JSContext* ctx) {
    if (s_classId == 0) {
        JS_NewClassID(&s_classId);
        JSClassDef def;
        memset(&def, 0, sizeof(JSClassDef));
        def.class_name = "Headers";
        def.finalizer = [](JSRuntime*, JSValue val) {
            auto* data = static_cast<HeadersData*>(JS_GetOpaque(val, s_classId));
            delete data;
        };
        JS_NewClass(JS_GetRuntime(ctx), s_classId, &def);
    }
    
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, proto, "get",
        JS_NewCFunction(ctx, headers_get, "get", 1));
    JS_SetPropertyStr(ctx, proto, "set",
        JS_NewCFunction(ctx, headers_set, "set", 2));
    JS_SetPropertyStr(ctx, proto, "has",
        JS_NewCFunction(ctx, headers_has, "has", 1));
    JS_SetPropertyStr(ctx, proto, "delete",
        JS_NewCFunction(ctx, headers_delete, "delete", 1));
    JS_SetPropertyStr(ctx, proto, "append",
        JS_NewCFunction(ctx, headers_append, "append", 2));
    JS_SetPropertyStr(ctx, proto, "keys",
        JS_NewCFunction(ctx, headers_keys, "keys", 0));
    JS_SetPropertyStr(ctx, proto, "values",
        JS_NewCFunction(ctx, headers_values, "values", 0));
    JS_SetPropertyStr(ctx, proto, "entries",
        JS_NewCFunction(ctx, headers_entries, "entries", 0));
    JS_SetPropertyStr(ctx, proto, "forEach",
        JS_NewCFunction(ctx, headers_forEach, "forEach", 1));
    
    JSValue constructor = JS_NewCFunction2(ctx, headers_constructor, "Headers", 1,
                                           JS_CFUNC_constructor, 0);
    JS_SetPrototype(ctx, constructor, proto);
    JS_SetClassProto(ctx, s_classId, proto);
    
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "Headers", constructor);
    JS_FreeValue(ctx, global);
}

ResponseBinding::ResponseData* ResponseBinding::getData(JSContext* ctx, JSValueConst this_val) {
    (void)ctx;
    return static_cast<ResponseData*>(JS_GetOpaque(this_val, s_classId));
}

JSValue ResponseBinding::create(JSContext* ctx, const FetchResponseData& data) {
    JSValue obj = JS_NewObjectClass(ctx, s_classId);
    auto* responseData = new ResponseData();
    responseData->response = data;
    responseData->bodyUsed = false;
    responseData->isError = false;
    JS_SetOpaque(obj, responseData);
    return obj;
}

JSValue ResponseBinding::createError(JSContext* ctx, const std::string& message) {
    JSValue obj = JS_NewObjectClass(ctx, s_classId);
    auto* responseData = new ResponseData();
    responseData->isError = true;
    responseData->errorMessage = message;
    JS_SetOpaque(obj, responseData);
    return obj;
}

JSValue ResponseBinding::response_constructor(JSContext* ctx, JSValueConst new_target,
                                            int argc, JSValueConst* argv) {
    (void)new_target;
    JSValue obj = JS_NewObjectClass(ctx, s_classId);
    auto* responseData = new ResponseData();
    
    if (argc >= 1 && JS_IsString(argv[0])) {
        responseData->response.body = JSBinding::toStdString(ctx, argv[0]);
    }
    
    JS_SetOpaque(obj, responseData);
    return obj;
}

JSValue ResponseBinding::response_get_status(JSContext* ctx, JSValueConst this_val,
                                           int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* data = getData(ctx, this_val);
    if (!data)
        return JS_EXCEPTION;
    return JS_NewInt64(ctx, data->response.status);
}

JSValue ResponseBinding::response_get_statusText(JSContext* ctx, JSValueConst this_val,
                                                int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* data = getData(ctx, this_val);
    if (!data)
        return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, data->response.statusText);
}

JSValue ResponseBinding::response_get_ok(JSContext* ctx, JSValueConst this_val,
                                        int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* data = getData(ctx, this_val);
    if (!data)
        return JS_EXCEPTION;
    return JS_NewBool(ctx, data->response.status >= 200 && data->response.status < 300);
}

JSValue ResponseBinding::response_get_headers(JSContext* ctx, JSValueConst this_val,
                                            int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* data = getData(ctx, this_val);
    if (!data)
        return JS_EXCEPTION;
    return HeadersBinding::create(ctx, data->response.headers);
}

JSValue ResponseBinding::response_get_url(JSContext* ctx, JSValueConst this_val,
                                         int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* data = getData(ctx, this_val);
    if (!data)
        return JS_EXCEPTION;
    if (data->response.url.empty())
        return JS_NULL;
    return JSBinding::toJSString(ctx, data->response.url);
}

JSValue ResponseBinding::response_get_redirected(JSContext* ctx, JSValueConst this_val,
                                                 int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* data = getData(ctx, this_val);
    if (!data)
        return JS_EXCEPTION;
    return JS_NewBool(ctx, data->response.redirected);
}

JSValue ResponseBinding::response_get_type(JSContext* ctx, JSValueConst this_val,
                                          int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* data = getData(ctx, this_val);
    if (!data)
        return JS_EXCEPTION;
    return JSBinding::toJSString(ctx, data->response.type);
}

JSValue ResponseBinding::response_get_body(JSContext* ctx, JSValueConst this_val,
                                           int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* data = getData(ctx, this_val);
    if (!data || data->isError)
        return JS_EXCEPTION;
    
    if (data->response.body.empty())
        return JS_NULL;
    
    return JSBinding::toJSString(ctx, data->response.body);
}

JSValue ResponseBinding::response_get_bodyUsed(JSContext* ctx, JSValueConst this_val,
                                               int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* data = getData(ctx, this_val);
    if (!data)
        return JS_EXCEPTION;
    return JS_NewBool(ctx, data->bodyUsed);
}

JSValue ResponseBinding::response_get_isError(JSContext* ctx, JSValueConst this_val,
                                              int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* data = getData(ctx, this_val);
    if (!data)
        return JS_EXCEPTION;
    return JS_NewBool(ctx, data->isError);
}

JSValue ResponseBinding::createBodyPromise(JSContext* ctx, JSValueConst this_val,
                                          const std::function<JSValue(JSContext*, const std::string&)>& bodyConverter) {
    auto* data = getData(ctx, this_val);
    if (!data || data->isError)
        return JS_EXCEPTION;
    
    if (data->bodyUsed) {
        return JS_ThrowTypeError(ctx, "Body has already been consumed");
    }
    data->bodyUsed = true;
    
    JSValue resolvingFuncs[2];
    JSValue promise = JS_NewPromiseCapability(ctx, resolvingFuncs);
    if (JS_IsException(promise))
        return JS_EXCEPTION;
    
    std::string body = data->response.body;
    
    auto task = [ctx, promise, body, bodyConverter, resolvingFuncs]() {
        JSContext* ctx2 = JS_DupContext(ctx);
        
        JSValue result = bodyConverter(ctx2, body);
        
        if (JS_IsException(result)) {
            JS_Call(ctx2, resolvingFuncs[1], JS_UNDEFINED, 1, &result);
        } else {
            JS_Call(ctx2, resolvingFuncs[0], JS_UNDEFINED, 1, &result);
        }
        
        JS_FreeValue(ctx2, result);
        JS_FreeValue(ctx2, resolvingFuncs[0]);
        JS_FreeValue(ctx2, resolvingFuncs[1]);
        JS_FreeValue(ctx2, promise);
        JS_FreeContext(ctx2);
    };
    
    PromiseBinding::enqueueMicroTask(task);
    
    return promise;
}

JSValue ResponseBinding::text(JSContext* ctx, JSValueConst this_val,
                              int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    return createBodyPromise(ctx, this_val,
        [](JSContext* c, const std::string& body) -> JSValue {
            return JSBinding::toJSString(c, body);
        });
}

JSValue ResponseBinding::json(JSContext* ctx, JSValueConst this_val,
                              int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    return createBodyPromise(ctx, this_val,
        [](JSContext* c, const std::string& body) -> JSValue {
            JSValue result = JS_ParseJSON(c, body.c_str(), body.length(), "<json>");
            if (JS_IsException(result)) {
                JSValue error = JS_ThrowTypeError(c, "Failed to parse JSON");
                JS_FreeValue(c, result);
                return error;
            }
            return result;
        });
}

JSValue ResponseBinding::arrayBuffer(JSContext* ctx, JSValueConst this_val,
                                    int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    return createBodyPromise(ctx, this_val,
        [](JSContext* c, const std::string& body) -> JSValue {
            uint8_t* buffer = (uint8_t*)js_malloc(c, body.length());
            if (!buffer)
                return JS_EXCEPTION;
            memcpy(buffer, body.data(), body.length());
            return JS_NewArrayBuffer(c, buffer, body.length(), nullptr, nullptr, true);
        });
}

JSValue ResponseBinding::blob(JSContext* ctx, JSValueConst this_val,
                              int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    return arrayBuffer(ctx, this_val, 0, nullptr);
}

JSValue ResponseBinding::clone(JSContext* ctx, JSValueConst this_val,
                               int argc, JSValueConst* argv) {
    (void)argc;
    (void)argv;
    auto* data = getData(ctx, this_val);
    if (!data)
        return JS_EXCEPTION;
    
    return create(ctx, data->response);
}

void ResponseBinding::registerClass(JSContext* ctx) {
    if (s_classId == 0) {
        JS_NewClassID(&s_classId);
        JSClassDef def;
        memset(&def, 0, sizeof(JSClassDef));
        def.class_name = "Response";
        def.finalizer = [](JSRuntime*, JSValue val) {
            auto* data = static_cast<ResponseData*>(JS_GetOpaque(val, s_classId));
            delete data;
        };
        JS_NewClass(JS_GetRuntime(ctx), s_classId, &def);
    }
    
    JSValue proto = JS_NewObject(ctx);
    
    JSAtom statusAtom = JS_NewAtom(ctx, "status");
    JS_DefinePropertyGetSet(ctx, proto, statusAtom,
        JS_NewCFunction(ctx, response_get_status, "get_status", 0),
        JS_UNDEFINED, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, statusAtom);
    
    JSAtom statusTextAtom = JS_NewAtom(ctx, "statusText");
    JS_DefinePropertyGetSet(ctx, proto, statusTextAtom,
        JS_NewCFunction(ctx, response_get_statusText, "get_statusText", 0),
        JS_UNDEFINED, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, statusTextAtom);
    
    JSAtom okAtom = JS_NewAtom(ctx, "ok");
    JS_DefinePropertyGetSet(ctx, proto, okAtom,
        JS_NewCFunction(ctx, response_get_ok, "get_ok", 0),
        JS_UNDEFINED, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, okAtom);
    
    JSAtom headersAtom = JS_NewAtom(ctx, "headers");
    JS_DefinePropertyGetSet(ctx, proto, headersAtom,
        JS_NewCFunction(ctx, response_get_headers, "get_headers", 0),
        JS_UNDEFINED, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, headersAtom);
    
    JSAtom urlAtom = JS_NewAtom(ctx, "url");
    JS_DefinePropertyGetSet(ctx, proto, urlAtom,
        JS_NewCFunction(ctx, response_get_url, "get_url", 0),
        JS_UNDEFINED, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, urlAtom);
    
    JSAtom redirectedAtom = JS_NewAtom(ctx, "redirected");
    JS_DefinePropertyGetSet(ctx, proto, redirectedAtom,
        JS_NewCFunction(ctx, response_get_redirected, "get_redirected", 0),
        JS_UNDEFINED, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, redirectedAtom);
    
    JSAtom typeAtom = JS_NewAtom(ctx, "type");
    JS_DefinePropertyGetSet(ctx, proto, typeAtom,
        JS_NewCFunction(ctx, response_get_type, "get_type", 0),
        JS_UNDEFINED, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, typeAtom);
    
    JSAtom bodyAtom = JS_NewAtom(ctx, "body");
    JS_DefinePropertyGetSet(ctx, proto, bodyAtom,
        JS_NewCFunction(ctx, response_get_body, "get_body", 0),
        JS_UNDEFINED, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, bodyAtom);
    
    JSAtom bodyUsedAtom = JS_NewAtom(ctx, "bodyUsed");
    JS_DefinePropertyGetSet(ctx, proto, bodyUsedAtom,
        JS_NewCFunction(ctx, response_get_bodyUsed, "get_bodyUsed", 0),
        JS_UNDEFINED, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, bodyUsedAtom);
    
    JS_SetPropertyStr(ctx, proto, "json",
        JS_NewCFunction(ctx, json, "json", 0));
    JS_SetPropertyStr(ctx, proto, "text",
        JS_NewCFunction(ctx, text, "text", 0));
    JS_SetPropertyStr(ctx, proto, "arrayBuffer",
        JS_NewCFunction(ctx, arrayBuffer, "arrayBuffer", 0));
    JS_SetPropertyStr(ctx, proto, "blob",
        JS_NewCFunction(ctx, blob, "blob", 0));
    JS_SetPropertyStr(ctx, proto, "clone",
        JS_NewCFunction(ctx, clone, "clone", 0));
    
    JSValue constructor = JS_NewCFunction2(ctx, response_constructor, "Response", 1,
                                           JS_CFUNC_constructor, 0);
    JS_SetPrototype(ctx, constructor, proto);
    JS_SetClassProto(ctx, s_classId, proto);
    
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "Response", constructor);
    JS_FreeValue(ctx, global);
}

JSValue FetchBinding::createTypeError(JSContext* ctx, const std::string& message) {
    JSValue error = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, error, "name", JS_NewString(ctx, "TypeError"));
    JS_SetPropertyStr(ctx, error, "message", JSBinding::toJSString(ctx, message));
    return error;
}

void FetchBinding::resolvePromise(JSContext* ctx, JSValue promise, JSValue value) {
    JSValue resolveFn = JS_GetPropertyStr(ctx, promise, "_resolve");
    if (!JS_IsNull(resolveFn) && !JS_IsUndefined(resolveFn)) {
        JS_Call(ctx, resolveFn, JS_UNDEFINED, 1, &value);
    }
    JS_FreeValue(ctx, resolveFn);
}

void FetchBinding::rejectPromise(JSContext* ctx, JSValue promise, JSValue error) {
    JSValue rejectFn = JS_GetPropertyStr(ctx, promise, "_reject");
    if (!JS_IsNull(rejectFn) && !JS_IsUndefined(rejectFn)) {
        JS_Call(ctx, rejectFn, JS_UNDEFINED, 1, &error);
    }
    JS_FreeValue(ctx, rejectFn);
}

FetchResponseData FetchBinding::convertResponse(const loader::HttpResponse& response) {
    FetchResponseData result;
    result.status = static_cast<uint16_t>(response.statusCode);
    result.statusText = response.statusMessage;
    result.body = std::string(response.body.begin(), response.body.end());
    result.url = response.finalUrl.toString();
    result.redirected = false;
    result.type = "basic";
    
    for (const auto& [name, value] : response.headers.all()) {
        result.headers[name] = value;
    }
    
    return result;
}

std::optional<FetchRequestInit> FetchBinding::parseRequestInit(JSContext* ctx,
                                                              JSValueConst options) {
    if (JS_IsUndefined(options) || JS_IsNull(options))
        return FetchRequestInit{};
    
    if (!JS_IsObject(options))
        return std::nullopt;
    
    FetchRequestInit init;
    
    JSValue method = JS_GetPropertyStr(ctx, options, "method");
    if (!JS_IsUndefined(method)) {
        init.method = JSBinding::toStdString(ctx, method);
    }
    JS_FreeValue(ctx, method);
    
    JSValue headers = JS_GetPropertyStr(ctx, options, "headers");
    if (JS_IsObject(headers)) {
        JSPropertyEnum* props = nullptr;
        uint32_t propCount = 0;
        
        if (JS_GetOwnPropertyNames(ctx, &props, &propCount, headers,
                                  JS_GPN_STRING_MASK) == 0) {
            for (uint32_t i = 0; i < propCount; i++) {
                JSValue key = JS_AtomToString(ctx, props[i].atom);
                JSValue val = JS_GetProperty(ctx, headers, props[i].atom);
                
                if (!JS_IsException(key) && !JS_IsException(val)) {
                    init.headers[JSBinding::toStdString(ctx, key)] = JSBinding::toStdString(ctx, val);
                }
                
                JS_FreeValue(ctx, key);
                JS_FreeValue(ctx, val);
            }
            js_free(ctx, props);
        }
    }
    JS_FreeValue(ctx, headers);
    
    JSValue body = JS_GetPropertyStr(ctx, options, "body");
    if (!JS_IsUndefined(body) && !JS_IsNull(body)) {
        if (JS_IsString(body)) {
            init.body = JSBinding::toStdString(ctx, body);
        }
    }
    JS_FreeValue(ctx, body);
    
    return init;
}

void FetchBinding::executeHttpRequest(const std::string& url,
                                     const FetchRequestInit& init,
                                     JSContext* ctx,
                                     JSValue promise) {
    std::thread([url, init, ctx, promise]() {
        JSContext* ctx2 = JS_DupContext(ctx);
        
        if (!s_httpClient) {
            JSValue error = createTypeError(ctx2, "HTTP client not initialized");
            rejectPromise(ctx2, promise, error);
            JS_FreeValue(ctx2, error);
            JS_FreeValue(ctx2, promise);
            JS_FreeContext(ctx2);
            return;
        }
        
        auto urlResult = loader::Url::parse(url);
        if (!urlResult.isOk()) {
            JSValue error = createTypeError(ctx2, "Invalid URL: " + url);
            rejectPromise(ctx2, promise, error);
            JS_FreeValue(ctx2, error);
            JS_FreeValue(ctx2, promise);
            JS_FreeContext(ctx2);
            return;
        }
        
        loader::HttpRequest request;
        request.url = urlResult.unwrap();
        
        std::string methodUpper = init.method;
        std::transform(methodUpper.begin(), methodUpper.end(), methodUpper.begin(), ::toupper);
        if (methodUpper == "GET") {
            request.method = loader::HttpMethod::Get;
        } else if (methodUpper == "POST") {
            request.method = loader::HttpMethod::Post;
        } else if (methodUpper == "PUT") {
            request.method = loader::HttpMethod::Put;
        } else if (methodUpper == "DELETE") {
            request.method = loader::HttpMethod::Delete;
        } else if (methodUpper == "HEAD") {
            request.method = loader::HttpMethod::Head;
        } else if (methodUpper == "OPTIONS") {
            request.method = loader::HttpMethod::Options;
        } else {
            request.method = loader::HttpMethod::Get;
        }
        
        for (const auto& [name, value] : init.headers) {
            request.headers.add(name, value);
        }
        
        if (init.body.has_value()) {
            request.body = std::vector<uint8_t>(init.body->begin(), init.body->end());
        }
        
        auto result = s_httpClient->execute(request);
        
        if (result.isErr()) {
            JSValue error = createTypeError(ctx2, "Network error: " + std::to_string(static_cast<int>(result.error())));
            rejectPromise(ctx2, promise, error);
            JS_FreeValue(ctx2, error);
        } else {
            auto response = result.unwrap();
            FetchResponseData fetchResponse = convertResponse(response);
            JSValue responseObj = ResponseBinding::create(ctx2, fetchResponse);
            resolvePromise(ctx2, promise, responseObj);
            JS_FreeValue(ctx2, responseObj);
        }
        
        JS_FreeValue(ctx2, promise);
        JS_FreeContext(ctx2);
    }).detach();
}

JSValue FetchBinding::fetch_function(JSContext* ctx, JSValueConst this_val,
                                    int argc, JSValueConst* argv) {
    (void)this_val;
    
    if (argc < 1)
        return JS_EXCEPTION;
    
    std::string url = JSBinding::toStdString(ctx, argv[0]);
    
    FetchRequestInit init;
    if (argc >= 2) {
        auto parsedInit = parseRequestInit(ctx, argv[1]);
        if (!parsedInit.has_value()) {
            return JS_ThrowTypeError(ctx, "Invalid request init");
        }
        init = parsedInit.value();
    }
    
    return createFetchPromise(ctx, url, init);
}

JSValue FetchBinding::createFetchPromise(JSContext* ctx,
                                         const std::string& url,
                                         const FetchRequestInit& init) {
    JSValue resolvingFuncs[2];
    JSValue promise = JS_NewPromiseCapability(ctx, resolvingFuncs);
    if (JS_IsException(promise)) {
        JS_FreeValue(ctx, resolvingFuncs[0]);
        JS_FreeValue(ctx, resolvingFuncs[1]);
        return JS_EXCEPTION;
    }
    
    executeHttpRequest(url, init, ctx, promise);
    
    JS_FreeValue(ctx, resolvingFuncs[0]);
    JS_FreeValue(ctx, resolvingFuncs[1]);
    
    return promise;
}

void FetchBinding::init(JSContext* ctx, std::shared_ptr<loader::HttpClient> httpClient) {
    s_httpClient = std::move(httpClient);
    
    HeadersBinding::registerClass(ctx);
    ResponseBinding::registerClass(ctx);
    
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "fetch",
        JS_NewCFunction(ctx, fetch_function, "fetch", 2));
    JS_FreeValue(ctx, global);
}

} // namespace script
} // namespace xiaopeng
