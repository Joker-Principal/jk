#pragma once

#define JK_STRINGIZE(x) #x
#define JK_PRAGMA(x) _Pragma(JK_STRINGIZE(x))
#define JK_CONCAT(x, y) x ## y
#define JK_EXEC(f, a) f a
#define JK_EMPTY(...)
#define JK_COMMA(...) ,
#define JK_EXPAND(x) x

#define JK_SELECT0(_0, ...) _0
#define JK_SELECT1(_0, _1, ...) _1
#define JK_SELECT2(_0, _1, _2, ...) _2
#define JK_SELECT3(_0, _1, _2, _3, ...) _3
#define JK_SELECT4(_0, _1, _2, _3, _4, ...) _4
#define JK_SELECT5(_0, _1, _2, _3, _4, _5, ...) _5
#define JK_SELECT6(_0, _1, _2, _3, _4, _5, _6, ...) _6
#define JK_SELECT7(_0, _1, _2, _3, _4, _5, _6, _7, ...) _7
#define JK_SELECT8(_0, _1, _2, _3, _4, _5, _6, _7, _8, ...) _8
#define JK_SELECT9(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, ...) _9
#define JK_SELECT(i, ...) JK_SELECT##i(__VA_ARGS__)

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

// get count of __VA_ARGS__
#if defined(JK_CLANG_CL) || (defined(JK_MSVC_ONLY) && (!defined(_MSVC_TRADITIONAL) || _MSVC_TRADITIONAL))
#   define JK_X_ARGS_COUNT(...) unused, __VA_ARGS__
#   define JK_Y_ARGS_COUNT(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count
#   define JK_Z_ARGS_COUNT(...) JK_EXPAND(JK_Y_ARGS_COUNT(__VA_ARGS__, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#   define JK_ARGS_COUNT(...)  JK_Z_ARGS_COUNT(JK_X_ARGS_COUNT(__VA_ARGS__))
#else
#   define JK_Y_ARGS_COUNT(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count
#   define JK_ARGS_COUNT(...) JK_Y_ARGS_COUNT(0, ## __VA_ARGS__, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#endif

// check if __VA_OPT__ is supported
// https://stackoverflow.com/a/48045656/21866441
#define JK_Y_VA_OPT_SUPPORTED(...) JK_EXEC(JK_SELECT2, (__VA_OPT__(,), 1, 0))
#define JK_VA_OPT_SUPPORTED JK_Y_VA_OPT_SUPPORTED(?)

// emulate __VA_OPT__ for compilers that don't support it
#if JK_VA_OPT_SUPPORTED
#    define JK_VA_OPT(opt, ...) __VA_OPT__(opt)
#    define JK_VA_OPT_FUNC(opt, ...) __VA_OPT__(opt)
#    define JK_VA_OPT_COMMA(...) __VA_OPT__(,)
#else
#    define JK_X_OPT_IS_0() ,
#    define JK_X_OPT_COUNT(count) JK_CONCAT(JK_X_OPT_IS_, count)
#    define JK_X_OPT_SELECT(empty, opt, ...) (JK_X_OPT_COUNT(JK_ARGS_COUNT(__VA_ARGS__))(), empty, opt)
#    define JK_VA_OPT(opt, ...) JK_EXEC(JK_SELECT2, JK_X_OPT_SELECT(, opt, __VA_ARGS__))
#    define JK_VA_OPT_FUNC(opt, ...) JK_EXEC(JK_SELECT2, JK_X_OPT_SELECT(JK_EMPTY, opt, __VA_ARGS__))
#    define JK_VA_OPT_COMMA(...) JK_EXEC(JK_SELECT2, JK_X_OPT_SELECT(JK_EMPTY, JK_COMMA,__VA_ARGS__))
#endif

/// unreachable
#if defined(__cpp_lib_unreachable) // C++23
#   include <utility>
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
