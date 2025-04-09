#!/bin/bash

# Create the shared include directories
mkdir -p ../shared/include/qoi
mkdir -p ../shared/include/stb
mkdir -p ../shared/include/resvg
mkdir -p ../shared/include/webp

# Copy header files to their new locations if they don't exist in the include directory
if [ ! -f "../shared/include/qoi/qoi.h" ] && [ -f "../shared/qoi.h" ]; then
    cp -n ../shared/qoi.h ../shared/include/qoi/qoi.h
fi

if [ ! -f "../shared/include/stb/stb_image.h" ] && [ -f "../shared/stb_image.h" ]; then
    cp -n ../shared/stb_image.h ../shared/include/stb/stb_image.h
fi

# Copy or create PluginGfxe.h if needed
if [ ! -f "../shared/include/PluginGfxe.h" ]; then
    # Create a basic header file for the plugin
    cat > ../shared/include/PluginGfxe.h << EOF
#ifndef PLUGIN_GFXE_H
#define PLUGIN_GFXE_H

#include <CoronaLua.h>
#include <CoronaLibrary.h>
#include <CoronaLog.h>
#include <CoronaMacros.h>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define SOLAR2D_WIN32 1
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #define SOLAR2D_IOS 1
    #else
        #define SOLAR2D_MACOS 1
    #endif
#endif

// Forward declarations for public functions
#ifdef __cplusplus
extern "C" {
#endif

CORONA_EXPORT int luaopen_plugin_gfxe(lua_State* L);

// Plugin API functions
int newStaticTexture(lua_State* L);
int newScalableTexture(lua_State* L);
int newAnimatedTexture(lua_State* L);
int update(lua_State* L);
int modify(lua_State* L);
int reset(lua_State* L);
int completed(lua_State* L, void* context);
int frames(lua_State* L, void* context);
int trait(lua_State* L, void* context);

// Helper functions
int PushCachedFunction(lua_State* L, lua_CFunction f);
void premultiplyAlpha(uint32_t* pixels, size_t numPixels);
CoronaExternalBitmapFormat strToFmt(const char* format);

// Utility functions
bool asBoolean(lua_State* L, int index, const char* field);
double asDouble(lua_State* L, int index, const char* field);
const char* asString(lua_State* L, int index, const char* field, size_t* length = NULL);
std::vector<double> asArray(lua_State* L, int index, const char* field);
void* readFile(const char* filename, size_t* size);

#ifdef __cplusplus
}
#endif

#endif // PLUGIN_GFXE_H
EOF
fi

echo "Directory structure and header files set up successfully"
