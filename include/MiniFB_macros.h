#pragma once

#include <stdint.h>

//-------------------------------------
// cross-platform deprecation macro, try to use the clean [[deprecated]] if it's avalible, if not, use compiler-specific fallbacks
//-------------------------------------

// C++ [[deprecated]] attribute
#if !defined(MFB_DEPRECATED) && defined(__cplusplus) && defined(__has_cpp_attribute)
    #if __has_cpp_attribute(deprecated)
        #define MFB_DEPRECATED(msg) [[deprecated(msg)]]
    #endif
#endif

// C23 [[deprecated]] attribute
#if !defined(MFB_DEPRECATED) && defined(__has_c_attribute)
    #if __has_c_attribute(deprecated)
        #define MFB_DEPRECATED(msg) [[deprecated(msg)]]
    #endif
#endif

// gcc/clang __attribute__ method
#if !defined(MFB_DEPRECATED) && (defined(__GNUC__) || defined(__clang__))
    #define MFB_DEPRECATED(msg) __attribute__((deprecated(msg)))
#endif

// msvc __declspec method
#if !defined(MFB_DEPRECATED) && defined(_MSC_VER)
    #define MFB_DEPRECATED(msg) __declspec(deprecated(msg))
#endif

// if we can't use any of those, just don't bother
#if !defined(MFB_DEPRECATED)
    #define MFB_DEPRECATED(msg)
#endif

// Enumerator deprecation macro.
// Note: __declspec(deprecated) is intentionally not used here because it is
// not portable for enum constants in C mode across compilers.
#if !defined(MFB_ENUM_DEPRECATED) && defined(__cplusplus) && defined(__has_cpp_attribute)
    #if __has_cpp_attribute(deprecated)
        #define MFB_ENUM_DEPRECATED(msg) [[deprecated(msg)]]
    #endif
#endif

#if !defined(MFB_ENUM_DEPRECATED) && defined(__has_c_attribute)
    #if __has_c_attribute(deprecated)
        #define MFB_ENUM_DEPRECATED(msg) [[deprecated(msg)]]
    #endif
#endif

#if !defined(MFB_ENUM_DEPRECATED) && (defined(__GNUC__) || defined(__clang__))
    #define MFB_ENUM_DEPRECATED(msg) __attribute__((deprecated(msg)))
#endif

#if !defined(MFB_ENUM_DEPRECATED)
    #define MFB_ENUM_DEPRECATED(msg)
#endif

//-------------------------------------
#if defined(_MSC_VER)
    #define MFB_FUNC_NAME __FUNCTION__
#else
    #define MFB_FUNC_NAME __func__
#endif

#define MFB_LOG(level, tag, ...)                                                            \
    do {                                                                                    \
        const mfb_log_info mfb_log_info_aux = { level, __FILE__, MFB_FUNC_NAME, __LINE__ }; \
        mfb_log(&mfb_log_info_aux, tag, __VA_ARGS__);                                       \
    } while (0)

#define MFB_LOGT(tag, ...) MFB_LOG(MFB_LOG_TRACE,   tag, __VA_ARGS__)
#define MFB_LOGD(tag, ...) MFB_LOG(MFB_LOG_DEBUG,   tag, __VA_ARGS__)
#define MFB_LOGI(tag, ...) MFB_LOG(MFB_LOG_INFO,    tag, __VA_ARGS__)
#define MFB_LOGW(tag, ...) MFB_LOG(MFB_LOG_WARNING, tag, __VA_ARGS__)
#define MFB_LOGE(tag, ...) MFB_LOG(MFB_LOG_ERROR,   tag, __VA_ARGS__)

//-------------------------------------
#if !defined(__ANDROID__)
    #define __MFB_A_SHIFT 24
    #define __MFB_R_SHIFT 16
    #define __MFB_G_SHIFT 8
    #define __MFB_B_SHIFT 0
#else
    #if defined(HOST_WORDS_BIGENDIAN)
        #define __MFB_A_SHIFT 0
        #define __MFB_R_SHIFT 24
        #define __MFB_G_SHIFT 16
        #define __MFB_B_SHIFT 8
    #else
        #define __MFB_A_SHIFT 24
        #define __MFB_R_SHIFT 0
        #define __MFB_G_SHIFT 8
        #define __MFB_B_SHIFT 16
    #endif
#endif

#define MFB_RGB(r, g, b) MFB_ARGB(0xFF, r, g, b)
#define MFB_ARGB(a, r, g, b) ( \
    (((uint32_t) (a)) << __MFB_A_SHIFT) | \
    (((uint32_t) (r)) << __MFB_R_SHIFT) | \
    (((uint32_t) (g)) << __MFB_G_SHIFT) | \
    (((uint32_t) (b)) << __MFB_B_SHIFT) )

#define MFB_GET_A(col) (((uint32_t) (col) >> __MFB_A_SHIFT) & 0x000000FF)
#define MFB_GET_R(col) (((uint32_t) (col) >> __MFB_R_SHIFT) & 0x000000FF)
#define MFB_GET_G(col) (((uint32_t) (col) >> __MFB_G_SHIFT) & 0x000000FF)
#define MFB_GET_B(col) (((uint32_t) (col) >> __MFB_B_SHIFT) & 0x000000FF)

//-------------------------------------
// Redefine the version macros from minifb_version.h, so that they use the MFB_ prefix, which is more consistent with
// the rest of the library's API, and to avoid consumers to have to explicitly include minifb_version.h.
// Reuse the same definition as in minifb_version.h, instead of delegating to the MINIFB_VERSION_GET_* macros, to avoid
// transitively including minifb_version.h when using the MFB_VERSION_GET_* macros. Code duplication is intentional here

// Get the major version number of the version obtained from mfb_get_build_version()
#define MFB_VERSION_GET_MAJOR(v) ((uint16_t)(((uint64_t)(v) >> 32) & 0xFFFFu))
// Get the minor version number of the version obtained from mfb_get_build_version()
#define MFB_VERSION_GET_MINOR(v) ((uint16_t)(((uint64_t)(v) >> 16) & 0xFFFFu))
// Get the patch version number of the version obtained from mfb_get_build_version()
#define MFB_VERSION_GET_PATCH(v) ((uint16_t)(((uint64_t)(v) >>  0) & 0xFFFFu))
