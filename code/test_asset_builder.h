
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "handmade_platform.h"
#include "handmade_file_formats.h"
#include "handmade_intrinsics.h"
#include "handmade_math.h"

#define ONE_PAST_MAX_FONT_CODEPOINT (0x10ffff + 1)

#define USE_FONTS_FROM_LINUX 1
#if USE_FONTS_FROM_LINUX
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define MAX_FONT_WIDTH  1024
#define MAX_FONT_HEIGHT 1024

global_variable Display* OpenDisplay = 0;
global_variable Pixmap   FontPixmap;
global_variable int      DefaultScreen;
global_variable GC       DefaultGC;

#define USE_FTLIB 1
#if USE_FTLIB
#undef internal
#include <X11/Xft/Xft.h>
#include <freetype/freetype.h>
#include <freetype/fttypes.h>
#endif
#define internal static

#else
#define STB_TRUETYPE_IMPLEMENTATION 1
#include "stb_truetype.h"
#endif

typedef struct
{
  int Width;
  int Height;
  int Pitch;
  void* Memory;

  void* Free;
} loaded_bitmap;

typedef struct 
{
  XftFont* Linux32Handle;
  FT_Face Metrics;
  r32 LineAdvance;

  hha_font_glyph* Glyphs;
  r32* HorizontalAdvance;

  u32 MinCodePoint;
  u32 MaxCodePoint;
  
  u32 MaxGlyphCount;
  u32 GlyphCount;

  u32 OnePastHighestCodePoint;
  u32* GlyphIndexFromCodePoint;
} loaded_font;

typedef enum
  {
    AssetType_Sound,
    AssetType_Bitmap,
    AssetType_Font,
    AssetType_FontGlyph,
  } asset_type;

typedef struct
{
  loaded_font* Font;
} asset_source_font;

typedef struct
{
  loaded_font* Font;
  u32 CodePoint;
} asset_source_font_glyph;

typedef struct
{
    char* Filename;
}asset_source_bitmap;

typedef struct
{
  char* Filename;
  u32 FirstSampleIndex;
} asset_source_sound;

typedef struct
{
  asset_type Type;
  union
  {
    asset_source_bitmap Bitmap;
    asset_source_sound Sound;
    asset_source_font Font;
    asset_source_font_glyph Glyph;    
  };
} asset_source;

#define VERY_LARGE_NUMBER 4096

typedef struct
{
  u32 TagCount;
  hha_tag Tags[VERY_LARGE_NUMBER];
  
  u32 AssetTypeCount; 
  hha_asset_type AssetTypes[Asset_Count];
  
  u32 AssetCount;
  asset_source AssetSources[VERY_LARGE_NUMBER];
  hha_asset Assets[VERY_LARGE_NUMBER];
  
  hha_asset_type* DEBUGAssetType;
  u32 AssetIndex;
} assets;

