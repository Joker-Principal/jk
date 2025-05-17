#pragma once

#define JK_STRINGIZE(x) #x
#define JK_PRAGMA(x) _Pragma(JK_STRINGIZE(x))
#define JK_CONCAT(x, y) x ## y

/// "overload" function macro by argument count
// see also: https://stackoverflow.com/a/8814003
// c++20 __VA_OPT__: https://en.cppreference.com/w/cpp/preprocessor/replace
#define JK_OVERLOAD(x, args, func, ...) func

/// detect compiler
#if defined(__clang__) 
#   define JK_CLANG
#   if defined(_MSC_VER)
#      define JK_CLANG_CL
#   else
#      define JK_CLANG_GNU
#   endif
#endif
#if defined(_MSC_VER)
#   define JK_MSVC
#   if !defined(__clang__)
#       define JK_MSVC_ONLY
#   endif
#endif
#if defined(__GNUC__)
#   define JK_GNU
#endif

/// warning
#if defined(JK_CLANG)
#   define JK_DISABLE_WARNING_PUSH JK_PRAGMA(clang diagnostic push)
#   define JK_DISABLE_WARNING_POP JK_PRAGMA(clang diagnostic pop)
#   define JK_DISABLE_WARNING(name) JK_PRAGMA(clang diagnostic ignored name)
#   define JK_DISABLE_WARNING_INVALID_OFFSETOF JK_DISABLE_WARNING("-Winvalid-offsetof")
#   define JK_DISABLE_WARNING_UNUSED_MACRO JK_DISABLE_WARNING("-Wunused-macros")
#   define JK_DISABLE_WARNING_UNSAFE_BUFFER_USAGE JK_DISABLE_WARNING("-Wunsafe-buffer-usage")
#   define JK_DISABLE_WARNING_HEADER_HYGIENE JK_DISABLE_WARNING("-Wheader-hygiene")
#elif defined(JK_GNU)
#   define JK_DISABLE_WARNING_PUSH JK_PRAGMA(GCC diagnostic push)
#   define JK_DISABLE_WARNING_POP JK_PRAGMA(GCC diagnostic pop)
#   define JK_DISABLE_WARNING(name) JK_PRAGMA(GCC diagnostic ignored name)
#   define JK_DISABLE_WARNING_INVALID_OFFSETOF JK_DISABLE_WARNING("-Winvalid-offsetof")
#   define JK_DISABLE_WARNING_UNUSED_MACRO JK_DISABLE_WARNING("-Wunused-macros")
#   define JK_DISABLE_WARNING_UNSAFE_BUFFER_USAGE
#   define JK_DISABLE_WARNING_HEADER_HYGIENE
#elif defined(JK_MSVC)
#   define JK_DISABLE_WARNING_PUSH JK_PRAGMA(warning(push))
#   define JK_DISABLE_WARNING_POP JK_PRAGMA(warning(pop))
#   define JK_DISABLE_WARNING(warningNumber) JK_PRAGMA(warning(disable: warningNumber))
#   define JK_DISABLE_WARNING_INVALID_OFFSETOF
#   define JK_DISABLE_WARNING_UNUSED_MACRO
#   define JK_DISABLE_WARNING_UNSAFE_BUFFER_USAGE
#   define JK_DISABLE_WARNING_HEADER_HYGIENE
#else
#   define JK_DISABLE_WARNING_PUSH
#   define JK_DISABLE_WARNING_POP
#   define JK_DISABLE_WARNING(...)
#   define JK_DISABLE_WARNING_INVALID_OFFSETOF
#   define JK_DISABLE_WARNING_UNUSED_MACRO
#   define JK_DISABLE_WARNING_UNSAFE_BUFFER_USAGE
#   define JK_DISABLE_WARNING_HEADER_HYGIENE
#endif

JK_DISABLE_WARNING_PUSH
JK_DISABLE_WARNING_UNUSED_MACRO

/// unreachable
#if defined(__cpp_lib_unreachable) // C++23
#   define JK_UNREACHABLE std::unreachable()
#elif defined(JK_MSVC_ONLY)
#   define JK_UNREACHABLE __assume(false)
#else
#   define JK_UNREACHABLE __builtin_unreachable()
#endif

/// assume
#if __cplusplus >= 202302L && __has_cpp_attribute(assume) // C++23
#   define JK_ASSUME(x) [[assume(x)]]
#elif defined(JK_CLANG)
#   define JK_ASSUME(x) __builtin_assume(x)
#elif defined(JK_MSVC_ONLY)
#   define JK_ASSUME(x) __assume(x)
#elif defined(JK_GNU)
#   define JK_ASSUME(x) __attribute__((assume(x)))
#endif

/// string view
// #include <string_view>
// using namespace std::string_view_literals;
// JK_STR_VIEW("abc") => "abc"sv
// JK_STR_VIEW("abc", u) => u"abc"sv
#define JK_STR_VIEW(x, ...) __VA_ARGS__##"" x ""sv

JK_DISABLE_WARNING_POP
