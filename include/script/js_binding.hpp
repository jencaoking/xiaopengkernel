#pragma once

#include <cstdint>
#include <functional>
#include <quickjs.h>
#include <string>
#include <vector>

namespace xiaopeng {
namespace script {

class JSBinding {
public:
  // Convert JSValue to std::string
  static std::string toStdString(JSContext *ctx, JSValueConst val) {
    const char *str = JS_ToCString(ctx, val);
    if (!str)
      return "";
    std::string result(str);
    JS_FreeCString(ctx, str);
    return result;
  }

  // Convert std::string to JSValue
  static JSValue toJSString(JSContext *ctx, const std::string &str) {
    return JS_NewStringLen(ctx, str.c_str(), str.length());
  }

  // Convert JSValue to int
  static int toInt(JSContext *ctx, JSValueConst val) {
    int32_t res;
    JS_ToInt32(ctx, &res, val);
    return res;
  }

  // Helpers for checking types
  static bool isString(JSValueConst val) { return JS_IsString(val); }
  static bool isNumber(JSValueConst val) { return JS_IsNumber(val); }
  static bool isObject(JSValueConst val) { return JS_IsObject(val); }
  static bool isFunction(JSContext *ctx, JSValueConst val) {
    return JS_IsFunction(ctx, val);
  }
};

} // namespace script
} // namespace xiaopeng
