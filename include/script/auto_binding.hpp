#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>


#ifdef ENABLE_QUICKJS
#include "quickjs.h"
#endif

namespace xiaopeng {
namespace script {
namespace auto_bind {

#ifdef ENABLE_QUICKJS

// --- Concept/Type Traits for JS value conversions ---

template <typename T> struct JSValueConverter;

template <> struct JSValueConverter<int> {
  static int fromJS(JSContext *ctx, JSValue val) {
    int res;
    if (JS_ToInt32(ctx, &res, val) < 0)
      return 0;
    return res;
  }
  static JSValue toJS(JSContext *ctx, int val) { return JS_NewInt32(ctx, val); }
};

template <> struct JSValueConverter<double> {
  static double fromJS(JSContext *ctx, JSValue val) {
    double res;
    if (JS_ToFloat64(ctx, &res, val) < 0)
      return 0.0;
    return res;
  }
  static JSValue toJS(JSContext *ctx, double val) {
    return JS_NewFloat64(ctx, val);
  }
};

template <> struct JSValueConverter<bool> {
  static bool fromJS(JSContext *ctx, JSValue val) {
    return JS_ToBool(ctx, val) > 0;
  }
  static JSValue toJS(JSContext *ctx, bool val) { return JS_NewBool(ctx, val); }
};

template <> struct JSValueConverter<std::string> {
  static std::string fromJS(JSContext *ctx, JSValue val) {
    if (JS_IsNull(val) || JS_IsUndefined(val))
      return "";
    const char *str = JS_ToCString(ctx, val);
    if (!str)
      return "";
    std::string res(str);
    JS_FreeCString(ctx, str);
    return res;
  }
  static JSValue toJS(JSContext *ctx, const std::string &val) {
    return JS_NewString(ctx, val.c_str());
  }
};

template <> struct JSValueConverter<JSValue> {
  static JSValue fromJS(JSContext *ctx, JSValue val) {
    return JS_DupValue(ctx, val);
  }
  static JSValue toJS(JSContext *ctx, JSValue val) {
    return JS_DupValue(ctx, val);
  }
};

// --- Tuple Expansion and Function Invocation ---

template <typename Func, typename Tuple, std::size_t... I>
auto invoke_with_tuple_impl(Func &&f, Tuple &&t, std::index_sequence<I...>) {
  return std::invoke(std::forward<Func>(f),
                     std::get<I>(std::forward<Tuple>(t))...);
}

template <typename Func, typename Tuple>
auto invoke_with_tuple(Func &&f, Tuple &&t) {
  constexpr auto size = std::tuple_size_v<std::remove_reference_t<Tuple>>;
  return invoke_with_tuple_impl(std::forward<Func>(f), std::forward<Tuple>(t),
                                std::make_index_sequence<size>{});
}

// Convert JS arguments to C++ tuple
template <typename... Args, std::size_t... I>
std::tuple<Args...> extract_args_impl(JSContext *ctx, int argc,
                                      JSValueConst *argv,
                                      std::index_sequence<I...>) {
  return std::make_tuple(
      (I < static_cast<size_t>(argc)
           ? JSValueConverter<std::remove_cv_t<std::remove_reference_t<Args>>>::
                 fromJS(ctx, argv[I])
           : typename std::remove_cv_t<std::remove_reference_t<Args>>{})...);
}

template <typename... Args>
std::tuple<Args...> extract_args(JSContext *ctx, int argc, JSValueConst *argv) {
  return extract_args_impl<Args...>(
      ctx, argc, argv, std::make_index_sequence<sizeof...(Args)>{});
}

// Global function binding
template <typename Ret, typename... Args>
JSValue bind_function(JSContext *ctx, JSValueConst this_val, int argc,
                      JSValueConst *argv, int magic, JSValue *func_data) {
  static JSClassID s_closure_class_id = 0;
  if (s_closure_class_id == 0) {
    JS_NewClassID(&s_closure_class_id);
  }
  auto *func_ptr = reinterpret_cast<std::function<Ret(Args...)> *>(JS_GetOpaque(
      *func_data, s_closure_class_id));
  if (!func_ptr)
    return JS_EXCEPTION;

  try {
    auto args_tuple = extract_args<Args...>(ctx, argc, argv);
    if constexpr (std::is_void_v<Ret>) {
      invoke_with_tuple(*func_ptr, args_tuple);
      return JS_UNDEFINED;
    } else {
      Ret result = invoke_with_tuple(*func_ptr, args_tuple);
      return JSValueConverter<Ret>::toJS(ctx, result);
    }
  } catch (const std::exception &e) {
    JS_ThrowInternalError(ctx, "%s", e.what());
    return JS_EXCEPTION;
  } catch (...) {
    JS_ThrowInternalError(ctx, "Unknown C++ exception");
    return JS_EXCEPTION;
  }
}

// Utility to create a JS function from a C++ std::function or lambda
template <typename Ret, typename... Args>
JSValue create_function(JSContext *ctx, std::function<Ret(Args...)> func) {
  static JSClassID s_closure_class_id = 0;
  if (s_closure_class_id == 0) {
    JS_NewClassID(&s_closure_class_id);
    JSClassDef def = {};
    def.class_name = "CppClosure";
    def.finalizer = [](JSRuntime *rt, JSValue val) {
      auto *p = reinterpret_cast<std::function<Ret(Args...)> *>(
          JS_GetOpaque(val, s_closure_class_id));
      delete p;
    };
    JS_NewClass(JS_GetRuntime(ctx), s_closure_class_id, &def);
  }

  auto heap_func = std::make_unique<std::function<Ret(Args...)>>(std::move(func));

  JSValue data_obj = JS_NewObjectClass(ctx, s_closure_class_id);
  if (JS_IsException(data_obj)) {
    return JS_EXCEPTION;
  }
  JS_SetOpaque(data_obj, heap_func.get());

  JSValue js_func = JS_NewCFunctionData(ctx, bind_function<Ret, Args...>,
                                        sizeof...(Args), 0, 1, &data_obj);
  if (JS_IsException(js_func)) {
    JS_FreeValue(ctx, data_obj);
    return JS_EXCEPTION;
  }

  heap_func.release();
  JS_FreeValue(ctx, data_obj);

  return js_func;
}

// Deduction guides for lambdas
template <typename F> auto wrap_lambda(JSContext *ctx, F &&f) {
  // This requires some C++ template metaprogramming to deduce the signature.
  // For now, developers should explicitly cast their lambdas to std::function.
  // Example: auto jsFunc = create_function(ctx, std::function<int(int)>([](int
  // x){ return x*2; }));
  return JS_UNDEFINED;
}

#endif // ENABLE_QUICKJS

} // namespace auto_bind
} // namespace script
} // namespace xiaopeng
