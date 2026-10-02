#if defined(MFB_HAS_RUNTIME_VERSION) && MFB_HAS_RUNTIME_VERSION

#include <stdint.h>
#include <MiniFB.h>
#include <minifb_version.h>

#if defined(__APPLE__)
    #include <TargetConditionals.h>
#endif

uint64_t mfb_get_version(void) {
    return MINIFB_VERSION_NUMERIC;
}

// variant string, distinguishing platform and specific build options, including a generic "unix" platform
static const char s_version_variant[] =
#if defined(_WIN32) || defined(WIN32)
    "windows"
#elif defined(__APPLE__) && defined(TARGET_OS_OSX) && TARGET_OS_OSX
    "macos"
#elif defined(__APPLE__) && defined(TARGET_OS_IOS) && TARGET_OS_IOS
    "ios"
#elif defined(__linux__) && !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    "linux"
#elif defined(__ANDROID__)
    "android"
#elif defined(__EMSCRIPTEN__)
    "emscripten"
#elif defined(__DJGPP__)
    "dos"
#elif defined(__unix__)
    "unix"
#else
    "unknown"
#endif
#if defined(USE_OPENGL_API)
    "-opengl"
#endif
#if defined(USE_METAL_API)
    "-metal"
#endif
#if defined(USE_WAYLAND_API)
    "-wayland"
#endif
#if defined(MINIFB_X11_USE_XIM)
    "-xim"
#endif
#if defined(USE_INVERTED_Y_ON_MACOS)
    "-invertedy"
#endif
    ;

const char * mfb_get_version_variant(void) {
    return s_version_variant;
}
#endif
