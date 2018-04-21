
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "handmade_platform.h"
#include "handmade_file_formats.h"
#include "handmade_intrinsics.h"
#include "handmade_math.h"

typedef enum
  {
    AssetType_Sound,
    AssetType_Bitmap,
    AssetType_Font,
    AssetType_FontGlyph,
  } asset_type;

typedef struct loaded_font loaded_font;
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

