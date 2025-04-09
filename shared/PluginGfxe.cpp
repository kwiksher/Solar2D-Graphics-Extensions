// Graphics Extensions for Solar2D
// Plugin Implementation

// Include Lua headers
// #include <lua/lua.h>
// #include <lua/lauxlib.h>
// #include <lua/lualib.h>

// Include Corona SDK headers
// #include <CoronaLua.h>
// #include <CoronaLibrary.h>
// #include <CoronaLog.h>
// #include <CoronaMacros.h>

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

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO

#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG

#define QOI_IMPLEMENTATION
#define QOI_NO_STDIO

#include "PluginGfxe.h"

#define STATIC 1
#define ANIMATED 2
#define SCALABLE 3

#define FREE_BUFFER if(!is_raw) { free((void*)data); }
#define EVEN_NUMBER(num) ((int)(num) % 2 == 0 ? (num) : (num) + 1)

#define animated_child static_cast<AnimatedTexture*>(texture->child)
#define scalable_child static_cast<ScalableTexture*>(texture->child)

// Forward declaration with proper linkage
#if defined(_WIN32) || defined(_WIN64)
    #define GFXE_EXPORT __declspec(dllexport)
#else
    #define GFXE_EXPORT __attribute__((visibility("default")))
#endif

// Declare external buffer from plugin_gfxe.c
extern "C" {
    // Reference the buffer and bufferLoader from plugin_gfxe.c
    extern const unsigned char kBuffer[];
    extern const size_t kBufferSize;
    extern int bufferLoader(lua_State* L);
}

// Add debug logging function
static void debugLog(lua_State* L, const char* format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);

    CoronaLuaLog(L, "%s", buffer); // Use CoronaLuaLog for logging

    va_end(args);
}

// Forward declarations
static int hello(lua_State* L);
static int loadSvg(lua_State* L);

// Plugin Lua Registration
CORONA_EXPORT int luaopen_plugin_gfxe(lua_State* L)
{
    // Register the library
    debugLog(L, "Starting luaopen_plugin_gfxe");

    try {
        lua_CFunction factory = Corona::Lua::Open<bufferLoader>;
        debugLog(L, "Got factory function");

        const char kName[] = "plugin_gfxe";
        debugLog(L, "Calling CoronaLibraryNewWithFactory");
        int result = CoronaLibraryNewWithFactory(L, factory, NULL, NULL);
        debugLog(L, "CoronaLibraryNewWithFactory returned: %d", result);

        // Setup the plugin API
        if (result)
        {
            debugLog(L, "Setting up plugin API");
            static const luaL_Reg kVTable[] = {
                {"_newStaticTexture", newStaticTexture},
                {"_newScalableTexture", newScalableTexture},
                {"_newAnimatedTexture", newAnimatedTexture},
                {NULL, NULL}
            };

            luaL_register(L, NULL, kVTable);
            debugLog(L, "Plugin initialization complete");
        }
        else {
            debugLog(L, "Failed to initialize plugin: CoronaLibraryNewWithFactory returned 0");
        }

        return result;
    }
    catch (const std::exception& e) {
        debugLog(L, "Exception during plugin initialization: %s", e.what());
        return 0;
    }
    catch (...) {
        debugLog(L, "Unknown exception during plugin initialization");
        return 0;
    }
}

// Plugin implementation

// Simple test function
static int hello(lua_State* L)
{
    lua_pushstring(L, "Hello from Graphics Extensions Plugin!");
    return 1;
}

// SVG loading function (stub for now)
static int loadSvg(lua_State* L)
{
    const char* filePath = luaL_checkstring(L, 1);

    // Log that we received the request
    CoronaLuaLog(L, "loadSvg called with path: %s", filePath);

    // For now, just return a status table
    lua_newtable(L);

    lua_pushboolean(L, true);
    lua_setfield(L, -2, "success");

    lua_pushstring(L, filePath);
    lua_setfield(L, -2, "path");

    // Platform-specific information
    #if defined(SOLAR2D_IOS)
        lua_pushstring(L, "iOS");
    #elif defined(SOLAR2D_MACOS)
        lua_pushstring(L, "macOS");
    #elif defined(SOLAR2D_WIN32)
        lua_pushstring(L, "Windows");
    #else
        lua_pushstring(L, "Unknown");
    #endif
    lua_setfield(L, -2, "platform");

    return 1;
}

// ----------------------------------------------------------------------------

struct Texture {
  int width = 0;
  int height = 0;

  int trait = 0;
  void* child = NULL;

  unsigned char* pixels = NULL;
  CoronaExternalBitmapFormat format = kExternalBitmapFormat_Undefined;
};

struct ScalableTexture {
  resvg_options* opts;
  resvg_render_tree* tree = NULL;

  int data_size = 0;
  const char* data = NULL;
};

struct AnimatedTexture {
  int elapsed = 0;
  int timestamp = 0;

  bool completed = false;
  bool should_loop = false;

  WebPData* data = NULL;
  WebPAnimInfo* info = NULL;
  WebPAnimDecoder* decoder = NULL;
};

// ----------------------------------------------------------------------------

static unsigned int GetWidth(void* context) {
  return ((Texture*)context)->width;
}

static unsigned int GetHeight(void* context) {
  return ((Texture*)context)->height;
}

static const void* GetImage(void* context) {
  return ((Texture*)context)->pixels;
}

static CoronaExternalBitmapFormat GetFormat(void* context) {
  return ((Texture*)context)->format;
}

// ----------------------------------------------------------------------------

static int GetField(lua_State* L, const char* field, void* context) {
  int result = 0;

  if(strcmp(field, "update") == 0) {
      result = PushCachedFunction(L, update);
  }
  else if(strcmp(field, "modify") == 0) {
      result = PushCachedFunction(L, modify);
  }

  else if(strcmp(field, "reset") == 0) {
      result = PushCachedFunction(L, reset);
  }

  else if(strcmp(field, "completed") == 0) {
      result = completed(L, context);
  }

  else if(strcmp(field, "frameCount") == 0) {
      result = frames(L, context);
  }
  else if(strcmp(field, "trait") == 0) {
      result = trait(L, context);
  }

  return result;
}

static void Dispose(void* context) {
  Texture* texture = (Texture*)context;

  if(texture->trait == ANIMATED) {
      AnimatedTexture* child = (AnimatedTexture*)texture->child;

      WebPAnimDecoderDelete(child->decoder);
      free((void*)child->data->bytes);

      delete child->data;
      delete child->info;

      delete child;
      goto EXIT;
  }

  if(texture->trait == SCALABLE) {
      ScalableTexture* child = (ScalableTexture*)texture->child;

      resvg_options_destroy(child->opts);
      resvg_tree_destroy(child->tree);

      free((void*)child->data);
      delete child;
  }

  if(texture->pixels) {
      free(texture->pixels);
  }

EXIT:
  delete texture;
}

// ----------------------------------------------------------------------------

static int update(lua_State* L) {
  Texture* texture = (Texture*)CoronaExternalGetUserData(L, 1);

  if(texture->trait != ANIMATED) {
      return 0;
  }

  int delta = luaL_checkinteger(L, 2);

  if(delta > 0) {
      int now = animated_child->elapsed + delta;

      while(now > animated_child->timestamp) {
          if(WebPAnimDecoderHasMoreFrames(animated_child->decoder)) {
              uint8_t* pixels;

              WebPAnimDecoderGetNext(animated_child->decoder, &pixels, &animated_child->timestamp);
              texture->pixels = (unsigned char*)pixels;
          }
          else if(animated_child->should_loop) {
              now = 0;
              uint8_t* pixels;

              WebPAnimDecoderReset(animated_child->decoder);
              WebPAnimDecoderGetNext(animated_child->decoder, &pixels, &animated_child->timestamp);

              texture->pixels = (unsigned char*)pixels;
              break;
          }
          else {
              animated_child->completed = true;
              now = 0;
              break;
          }
      }

      animated_child->elapsed = now;
  }

  return 0;
}

static int modify(lua_State* L) {
  double multiplier = 0;
  bool should_crop = false;

  size_t length = 0;

  const char* nodeID = NULL;
  const char* svg_data = NULL;

  Texture* texture = (Texture*)CoronaExternalGetUserData(L, 1);

  if(texture->trait != SCALABLE) {
      return 0;
  }

  int bpp = CoronaExternalFormatBPP(texture->format);

  resvg_transform transform = resvg_transform_identity();

  if(lua_istable(L, 2)) {
      std::vector<double> r = asArray(L, 2, "render");
      std::vector<double> s = asArray(L, 2, "sizing");
      std::vector<double> t = asArray(L, 2, "transform");

      const char* res_path = asString(L, 2, "resourceDir");
      const char* languages = asString(L, 2, "languages");
      const char* font_family = asString(L, 2, "fontFamily");

      nodeID = asString(L, 2, "nodeID");
      svg_data = asString(L, 2, "svgData", &length);

      double font_size = asDouble(L, 2, "fontSize");

      size_t font_length = 0;
      const char* font_data = asString(L, 2, "fontData", &font_length);

      if(res_path) {
          resvg_options_set_resources_dir(scalable_child->opts, res_path);
      }

      if(languages) {
          resvg_options_set_languages(scalable_child->opts, languages);
      }

      if(font_family) {
          resvg_options_set_font_family(scalable_child->opts, font_family);
      }

      if(font_size > 0) {
          resvg_options_set_font_size(scalable_child->opts, font_size);
      }

      if(font_data) {
          resvg_options_load_font_data(scalable_child->opts, font_data, font_length);
      }

      should_crop = setupSVG(scalable_child->opts, &transform, &multiplier, r, s, t);
  }

  if(svg_data) {
      void* bytes;

      int result = resvg_parse_tree_from_data(svg_data, (int)length, scalable_child->opts, &scalable_child->tree);

      if(result != RESVG_OK) {
          goto MODIFY_FAIL;
      }

      bytes = realloc((void*)scalable_child->data, length * sizeof(const char));

      if(!bytes) {
          goto MODIFY_FAIL;
      }

      memcpy(bytes, svg_data, length * sizeof(const char));

      scalable_child->data = (const char*)bytes;
      scalable_child->data_size = (int)length;
  }
  else {
      int result = resvg_parse_tree_from_data(scalable_child->data, scalable_child->data_size, scalable_child->opts, &scalable_child->tree);

      if (result != RESVG_OK) {
          goto MODIFY_FAIL;
      }
  }

  if(resvg_is_image_empty(scalable_child->tree)) {
      goto MODIFY_FAIL;
  }

  if(nodeID && !resvg_node_exists(scalable_child->tree, nodeID)) {
      goto MODIFY_FAIL;
  }

  if(!nodeID && should_crop) {
      resvg_rect bbox;
      resvg_get_image_bbox(scalable_child->tree, &bbox);

      resvg_rect vbox = resvg_get_image_viewbox(scalable_child->tree);

      if(multiplier > 0) {
          transform.e -= ((bbox.x - vbox.x) * multiplier) - 2;
          transform.f -= ((bbox.y - vbox.y) * multiplier) - 2;

          texture->width = EVEN_NUMBER(bbox.width * multiplier) + 4;
          texture->height = EVEN_NUMBER(bbox.height * multiplier) + 4;
      }
      else {
          transform.e -= bbox.x - vbox.x - 2;
          transform.f -= bbox.y - vbox.y - 2;

          texture->width = EVEN_NUMBER(bbox.width) + 4;
          texture->height = EVEN_NUMBER(bbox.height) + 4;
      }
  }
  else {
      resvg_size size;
      int width = 0, height = 0;

      if(nodeID) {
          resvg_rect pbox;
          resvg_get_node_stroke_bbox(scalable_child->tree, nodeID, &pbox);

          size.width = pbox.width;
          size.height = pbox.height;
      }
      else {
          size = resvg_get_image_size(scalable_child->tree);
      }

      if(multiplier > 0) {
          width = size.width * multiplier;
          height = size.height * multiplier;
      }

      texture->width = width >= 1 ? width : size.width;
      texture->height = height >= 1 ? height : size.height;
  }

  if(multiplier > 0) {
      transform.a *= multiplier;
      transform.d *= multiplier;
  }

{
  char* pixels = (char*)calloc(texture->width * texture->height * 4, sizeof(char));

  if(!pixels) {
      goto MODIFY_FAIL;
  }

  if(texture->pixels) {
      free(texture->pixels);
  }

  if(nodeID) {
      resvg_render_node(scalable_child->tree, nodeID, transform, texture->width, texture->height, pixels);
  }
  else {
      resvg_render(scalable_child->tree, transform, texture->width, texture->height, pixels);
  }

  texture->pixels = stbi__convert_format((unsigned char*)pixels, 4, bpp, texture->width, texture->height);

  if(texture->pixels) {
      lua_pushboolean(L, true);
      return 1;
  }
}

MODIFY_FAIL:
  lua_pushboolean(L, false);
  return 1;
}

static int reset(lua_State* L) {
  Texture* texture = (Texture*)CoronaExternalGetUserData(L, 1);

  if(texture->trait != ANIMATED) {
      return 0;
  }

  animated_child->should_loop = lua_toboolean(L, 2);
  animated_child->completed = false;
  animated_child->elapsed = 0;

  uint8_t* pixels;

  WebPAnimDecoderReset(animated_child->decoder);
  WebPAnimDecoderGetNext(animated_child->decoder, &pixels, &animated_child->timestamp);

  texture->pixels = (unsigned char*)pixels;

  return 0;
}

// ----------------------------------------------------------------------------

static int frames(lua_State* L, void* context) {
  Texture* texture = (Texture*)context;

  switch(texture->trait) {
      case ANIMATED:
          lua_pushnumber(L, animated_child->info->frame_count);
          break;
      default:
          lua_pushnil(L);
          break;
  }

  return 1;
}

static int trait(lua_State* L, void* context) {
  Texture* texture = (Texture*)context;

  switch(texture->trait) {
      case STATIC:
          lua_pushstring(L, "static");
          break;
      case ANIMATED:
          lua_pushstring(L, "animated");
          break;
      case SCALABLE:
          lua_pushstring(L, "scalable");
          break;
      default:
          lua_pushstring(L, "unknown");
          break;
  }

  return 1;
}

static int completed(lua_State* L, void* context) {
  Texture* texture = (Texture*)context;

  switch(texture->trait) {
      case ANIMATED:
          lua_pushboolean(L, animated_child->completed);
          break;
      default:
          lua_pushnil(L);
          break;
  }

  return 1;
}

// ----------------------------------------------------------------------------

bool decodeSTBI(Texture* texture, unsigned char* data, int length) {
  int channels = 0;
  int width = 0, height = 0;
  int bpp = CoronaExternalFormatBPP(texture->format);

  unsigned char* pixels = stbi_load_from_memory(data, length, &width, &height, &channels, bpp);

  if(!pixels) {
      return false;
  }

  if(bpp == 4 && channels == 4) {
      size_t buffer_size = width * height;
      premultiplyAlpha((uint32_t*)pixels, buffer_size);
  }

  texture->width = width;
  texture->height = height;

  texture->pixels = pixels;

  return true;
}

bool decodeQOI(Texture* texture, unsigned char* data, int length) {
  qoi_desc info;
  int bpp = CoronaExternalFormatBPP(texture->format);

  unsigned char* pixels = (unsigned char*)qoi_decode(data, length, &info, 0);

  if(!pixels) {
      return false;
  }

  if(bpp == 4 && info.channels == 4) {
      size_t buffer_size = info.width * info.height;
      premultiplyAlpha((uint32_t*)pixels, buffer_size);
  }

  texture->pixels = stbi__convert_format(pixels, info.channels, bpp, info.width, info.height);

  if(texture->pixels) {
      texture->width = info.width;
      texture->height = info.height;

      return true;
  }

  return false;
}

bool decodeWEBP(Texture* texture, const uint8_t* data, size_t length) {
  WebPDecoderConfig config;
  int width = 0, height = 0;
  int bpp = CoronaExternalFormatBPP(texture->format);

  if(!WebPGetInfo(data, length, &width, &height)) {
      return false;
  }

  if(!WebPInitDecoderConfig(&config)) {
      return false;
  }

  size_t buffer_size = width * height * 4;
  unsigned char* pixels = (unsigned char*)malloc(buffer_size * sizeof(unsigned char));

  if(!pixels) {
      return false;
  }

  config.options.use_threads = 1;

  config.output.colorspace = MODE_rgbA;
  config.output.is_external_memory = 1;

  config.output.u.RGBA.size = buffer_size;
  config.output.u.RGBA.stride = width * 4;
  config.output.u.RGBA.rgba = (uint8_t*)pixels;

  if(WebPDecode(data, length, &config) != VP8_STATUS_OK) {
      free(pixels);
      return false;
  }

  WebPFreeDecBuffer(&config.output);

  texture->pixels = stbi__convert_format(pixels, 4, bpp, width, height);

  if(texture->pixels) {
      texture->width = width;
      texture->height = height;

      return true;
  }

  return false;
}

// ----------------------------------------------------------------------------

bool setupSVG(resvg_options* opts, resvg_transform* transform, double* multiplier, std::vector<double> r, std::vector<double> s, std::vector<double> t) {
  bool should_crop = false;

  // Render
  if(r.size() > 0) {
      resvg_options_set_dpi(opts, r[0]);
      resvg_options_set_shape_rendering_mode(opts, (resvg_shape_rendering)r[1]);

      resvg_options_set_text_rendering_mode(opts, (resvg_text_rendering)r[2]);
      resvg_options_set_image_rendering_mode(opts, (resvg_image_rendering)r[3]);
  }

  // Sizing
  if(s.size() > 0) {
      (*multiplier) = s[0];
      should_crop = (bool)s[1];
  }

  // Transform
  if(t.size() > 0) {
      transform->a = t[0];
      transform->b = t[1];

      transform->c = t[2];
      transform->d = t[3];

      transform->e = t[4];
      transform->f = t[5];
  }

  return should_crop;
}

// ----------------------------------------------------------------------------

static int newTexture(lua_State* L, void* texture) {
  CoronaExternalTextureCallbacks callbacks = {};
  callbacks.size = sizeof(CoronaExternalTextureCallbacks);
  callbacks.getWidth = GetWidth;
  callbacks.getHeight = GetHeight;
  callbacks.onRequestBitmap = GetImage;
  callbacks.getFormat = GetFormat;
  callbacks.onGetField = GetField;
  callbacks.onFinalize = Dispose;

  return CoronaExternalPushTexture(L, &callbacks, texture);
}

// ----------------------------------------------------------------------------

static int newStaticTexture(lua_State* L) {
  const char* data;
  size_t length = 0;
  Texture* texture = new Texture;

  bool is_raw = lua_toboolean(L, 1);
  const char* format = luaL_checkstring(L, 2);

  if(is_raw) {
      data = luaL_checklstring(L, 3, &length);
  }
  else {
      const char* filename = luaL_checkstring(L, 3);
      void* file_content = readFile(filename, &length);

      if(file_content) {
          data = (const char*)file_content;
      }
      else {
          goto STATIC_FAIL;
      }
  }

  texture->trait = STATIC;
  texture->format = strToFmt(format);

  if(decodeQOI(texture, (unsigned char*)data, (int)length)) {
      FREE_BUFFER
      return newTexture(L, texture);
  }
  if(decodeWEBP(texture, (const uint8_t*)data, length)) {
      FREE_BUFFER
      return newTexture(L, texture);
  }
  if(decodeSTBI(texture, (unsigned char*)data, (int)length)) {
      FREE_BUFFER
      return newTexture(L, texture);
  }

  FREE_BUFFER

STATIC_FAIL:
  delete texture;

  lua_pushnil(L);
  return 1;
}

static int newScalableTexture(lua_State* L) {
  void* bytes;

  const char* data;
  size_t length = 0;

  double multiplier = 0;
  bool should_crop = false;

  Texture* texture = new Texture;

  bool is_raw = lua_toboolean(L, 1);
  const char* format = luaL_checkstring(L, 2);

  if(is_raw) {
      data = luaL_checklstring(L, 3, &length);
  }
  else {
      const char* filename = luaL_checkstring(L, 3);
      bytes = readFile(filename, &length);

      if(!bytes) {
          delete texture;

          lua_pushnil(L);
          return 1;
      }
  }

  texture->trait = SCALABLE;
  texture->format = strToFmt(format);

  texture->child = new ScalableTexture;
  scalable_child->opts = resvg_options_create();

  resvg_transform transform = resvg_transform_identity();

  if(is_raw) {
      bytes = malloc(length * sizeof(const char));

      if(!bytes) {
          goto SVG_FAIL;
      }

      memcpy(bytes, data, length * sizeof(const char));
  }

  scalable_child->data = (const char*)bytes;
  scalable_child->data_size = (int)length;

  if(lua_istable(L, 4)) {
      std::vector<double> r = asArray(L, 4, "render");

      std::vector<double> s = asArray(L, 4, "sizing");
      std::vector<double> t = asArray(L, 4, "transform");

      const char* res_path = asString(L, 4, "resourceDir");

      const char* languages = asString(L, 4, "languages");
      const char* font_family = asString(L, 4, "fontFamily");

      double font_size = asDouble(L, 4, "fontSize");
      bool system_fonts = asBoolean(L, 4, "systemFonts");

      size_t font_length = 0;
      const char* font_data = asString(L, 4, "fontData", &font_length);

      if(res_path) {
          resvg_options_set_resources_dir(scalable_child->opts, res_path);
      }

      if(languages) {
          resvg_options_set_languages(scalable_child->opts, languages);
      }

      if(font_family) {
          resvg_options_set_font_family(scalable_child->opts, font_family);
      }

      if(font_size > 0) {
          resvg_options_set_font_size(scalable_child->opts, font_size);
      }

      if(system_fonts) {
          resvg_options_load_system_fonts(scalable_child->opts);
      }

      if(font_data) {
          resvg_options_load_font_data(scalable_child->opts, font_data, font_length);
      }

      should_crop = setupSVG(scalable_child->opts, &transform, &multiplier, r, s, t);
  }

{
  int result = resvg_parse_tree_from_data(scalable_child->data, scalable_child->data_size, scalable_child->opts, &scalable_child->tree);

  if(result != RESVG_OK) {
      goto SVG_FAIL;
  }
}

  if(resvg_is_image_empty(scalable_child->tree)) {
      resvg_tree_destroy(scalable_child->tree);

      goto SVG_FAIL;
  }

  if(should_crop) {
      resvg_rect bbox;
      resvg_get_image_bbox(scalable_child->tree, &bbox);

      resvg_rect vbox = resvg_get_image_viewbox(scalable_child->tree);

      if(multiplier > 0) {
          transform.e -= ((bbox.x - vbox.x) * multiplier) - 2;
          transform.f -= ((bbox.y - vbox.y) * multiplier) - 2;

          texture->width = EVEN_NUMBER(bbox.width * multiplier) + 4;
          texture->height = EVEN_NUMBER(bbox.height * multiplier) + 4;
      }
      else {
          transform.e -= bbox.x - vbox.x - 2;
          transform.f -= bbox.y - vbox.y - 2;

          texture->width = EVEN_NUMBER(bbox.width) + 4;
          texture->height = EVEN_NUMBER(bbox.height) + 4;
      }
  }
  else {
      int width = 0, height = 0;
      resvg_size size = resvg_get_image_size(scalable_child->tree);

      if(multiplier > 0) {
          width = size.width * multiplier;
          height = size.height * multiplier;
      }

      texture->width = width >= 1 ? width : size.width;
      texture->height = height >= 1 ? height : size.height;
  }

  if(multiplier > 0) {
      transform.a *= multiplier;
      transform.d *= multiplier;
  }

{
  char* pixels = (char*)calloc(texture->width * texture->height * 4, sizeof(char));

  if(!pixels) {
      resvg_tree_destroy(scalable_child->tree);
      goto SVG_FAIL;
  }

  resvg_render(scalable_child->tree, transform, texture->width, texture->height, pixels);

  int bpp = CoronaExternalFormatBPP(texture->format);

  texture->pixels = stbi__convert_format((unsigned char*)pixels, 4, bpp, texture->width, texture->height);

  if(texture->pixels) {
      return newTexture(L, texture);
  }
  else {
      resvg_tree_destroy(scalable_child->tree);
  }
}

SVG_FAIL:
    resvg_options_destroy(scalable_child->opts);

    delete scalable_child;
    delete texture;

    lua_pushnil(L);
    return 1;
}

// ----------------------------------------------------------------------------

static int newAnimatedTexture(lua_State* L) {
  void* bytes;
  const char* data;

  size_t length = 0;
  Texture* texture = new Texture;

  bool is_raw = lua_toboolean(L, 1);
  bool loop = lua_toboolean(L, 2);

  if(is_raw) {
      data = luaL_checklstring(L, 3, &length);
  }
  else {
      const char* filename = luaL_checkstring(L, 3);
      bytes = readFile(filename, &length);

      if(!bytes) {
          delete texture;

          lua_pushnil(L);
          return 1;
      }
  }

  texture->trait = ANIMATED;
  texture->format = strToFmt("rgba");

  texture->child = new AnimatedTexture;
  animated_child->should_loop = loop;

  WebPAnimDecoderOptions opts;
  if(!WebPAnimDecoderOptionsInit(&opts)) {
      goto WEBP_FAIL;
  }

  opts.use_threads = 1;
  opts.color_mode = MODE_rgbA;

  if(is_raw) {
      bytes = malloc(length * sizeof(const char));

      if(!bytes) {
          goto WEBP_FAIL;
      }

      memcpy(bytes, data, length * sizeof(const char));
  }

  animated_child->data = new WebPData;
  animated_child->data->size = length;
  animated_child->data->bytes = (const uint8_t*)bytes;

  animated_child->decoder = WebPAnimDecoderNew(animated_child->data, &opts);

  if(!animated_child->decoder) {
      free((void*)animated_child->data->bytes);
      delete animated_child->data;

      goto WEBP_FAIL;
  }

  animated_child->info = new WebPAnimInfo;
  WebPAnimDecoderGetInfo(animated_child->decoder, animated_child->info);

  texture->width = animated_child->info->canvas_width;
  texture->height = animated_child->info->canvas_height;

{
  uint8_t* pixels;

  int result = WebPAnimDecoderGetNext(animated_child->decoder, &pixels, &animated_child->timestamp);

  if(!result) {
      WebPAnimDecoderDelete(animated_child->decoder);

      free((void*)animated_child->data->bytes);
      delete animated_child->data;
      delete animated_child->info;

      goto WEBP_FAIL;
  }

  texture->pixels = (unsigned char*)pixels;

  return newTexture(L, texture);
}

WEBP_FAIL:
  delete animated_child;
  delete texture;

  lua_pushnil(L);
  return 1;
}

// Analyzing plugin_gfxe implementation

// Key issues to investigate:
// 1. Symbol export format - Does the plugin correctly export Lua C functions?
// 2. Module initialization structure - Is luaopen_plugin function properly defined?
// 3. Library dependencies - Are all required libraries properly linked and initialized?
// 4. Error handling - Is proper error handling implemented for all operations?
// 5. Memory management - Are there memory leaks or improper memory handling?

/*
 Comparing with plugin_movie implementation:

 plugin_movie typically has:
 1. luaopen_plugin function that registers all plugin functions
 2. Proper CORONA_EXPORT macro usage
 3. C function implementations with lua_State* parameter
 4. Proper Lua stack manipulation
 5. Clear error handling

 Common issues in failed plugins:
 - Missing CORONA_EXPORT macros
 - Incorrect function signatures
 - Missing luaopen_plugin implementation
 - Improper library initialization
 - Memory leaks in C/C++ code
*/

// Check if the plugin_gfxe.c file exists and properly implements the module initialization:
// - luaopen_plugin function
// - CORONA_EXPORT macro
// - Correct registration of all Lua-callable functions

// Check if all external libraries are properly initialized before use

// Verify memory management in all functions using external resources
