
#include <stdio.h>
#include <stdlib.h>
#include "handmade_platform.h"
#include "handmade_asset_type_id.h"
#include "handmade_file_formats.h"

typedef struct
{
  u32 Value;
} bitmap_id;

typedef struct
{
  u32 Value;
} sound_id;

typedef struct
{
  char* Filename;
  r32 AlignPercentage[2];
} asset_bitmap_info;

typedef struct
{
  char* Filename;
  u32 FirstSampleIndex;
  u32 SampleCount;
  sound_id NextIDToPlay;
} asset_sound_info;

typedef struct
{
  u64 DataOffset;
  u32 FirstTagIndex;
  u32 OnePastLastTagIndex;
  union
  {
    asset_bitmap_info Bitmap;
    asset_sound_info Sound;
  };
} asset;

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
  
  u32 AssetCount;
  asset Assets[VERY_LARGE_NUMBER];
  
  u32 AssetTypeCount; 
  hha_asset_type AssetTypes[Asset_Count];
  
  hha_asset_type* DEBUGAssetType;
  asset* DEBUGAsset;
} assets;

