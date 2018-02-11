
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "handmade_platform.h"
#include "handmade_asset_type_id.h"
#include "handmade_file_formats.h"
#include "handmade_intrinsics.h"
#include "handmade_math.h"

typedef enum
  {
    AssetType_Sound,
    AssetType_Bitmap,
  } asset_type;

typedef struct
{
  asset_type Type;
  char* Filename;
  u32 FirstSampleIndex;
} asset_source;

typedef struct
{
  char* Filename;
  r32 Alignment[2];
  
} bitmap_asset;

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

