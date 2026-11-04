#pragma once

#if defined(MINIFB_BUILD_SHARED)
    #if defined(_WIN32)
        #if defined(minifb_EXPORTS)
            #define MFB_EXPORT __declspec(dllexport)
        #else
            #define MFB_EXPORT __declspec(dllimport)
        #endif
    #else
        #define MFB_EXPORT __attribute__((visibility("default")))
    #endif
#else
    #define MFB_EXPORT
#endif