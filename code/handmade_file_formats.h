
#define HHA_CODE(a, b, c, d) (((u32)a << 0) | ((u32)b << 8) | ((u32)c << 16) | ((u32)d << 24))

#pragma pack(push, 1)

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
#define HHA_MAGIC_VALUE HHA_CODE('h', 'h', 'a', 'f')
  u32 MagicValue;

#define HHA_VERSION 0 
  u32 Version;

  u32 TagCount;
  u32 AssetTypeCount;
  u32 AssetCount;

  u64 Tags;
  u64 AssetTypes;
  u64 Assets;
} hha_header;

typedef struct
{
  u32 ID;
  r32 Value;
} hha_tag; 

typedef struct
{
  u32 TypeID;
  u32 FirstAssetIndex;
  u32 OnePastLastAssetIndex;
} hha_asset_type; 

typedef struct
{
  u32 Dim[2];
  r32 AlignPercentage[2];
} hha_bitmap;

typedef struct
{
  u32 SampleCount;
  u32 ChannelCount;
  sound_id NextIDToPlay;
} hha_sound;

typedef struct
{
  u64 DataOffset;
  u32 FirstTagIndex;
  u32 OnePastLastTagIndex;
  union
  {
    hha_bitmap Bitmap;
    hha_sound Sound;
  };
} hha_asset; 

#pragma pack(pop)
