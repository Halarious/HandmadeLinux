
typedef enum
  {
    FontType_Default = 0,
    FontType_Debug = 10,
  } asset_font_type;

typedef enum
  {
    Tag_Smoothness,
    Tag_Flatness,
    Tag_FacingDirection,
    Tag_UnicodeCodepoint,
    Tag_FontType,
    
    Tag_Count,
  } asset_tag_id;

typedef enum
  {
    Asset_None,
    
    Asset_Shadow,
    Asset_Tree,
    Asset_Sword,
    Asset_Rock,

    Asset_Grass,
    Asset_Tuft,
    Asset_Stone,

    Asset_Head,
    Asset_Cape,
    Asset_Torso,

    Asset_Font,
    Asset_FontGlyph,
    
    Asset_Bloop,
    Asset_Crack,
    Asset_Drop,
    Asset_Glide,
    Asset_Music,
    Asset_Puhp,
    
    Asset_Count,
  } asset_type_id;

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
  u32 Value;
} font_id;

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

typedef enum
  {
    HHASoundChain_None,
    HHASoundChain_Loop,
    HHASoundChain_Advance,
  } hha_sound_chain;

typedef struct
{
  u32 Dim[2];
  r32 AlignPercentage[2];
} hha_bitmap;

typedef struct
{
  u32 SampleCount;
  u32 ChannelCount;
  u32 Chain;
} hha_sound;

typedef struct
{
  u32 OnePastHighestCodePoint;
  u32 GlyphCount;
  r32 AscenderHeight;
  r32 DescenderHeight;
  r32 ExternalLeading;
} hha_font;

typedef struct
{
  u32 UnicodeCodePoint;
  bitmap_id BitmapID;
} hha_font_glyph;

typedef struct
{
  u64 DataOffset;
  u32 FirstTagIndex;
  u32 OnePastLastTagIndex;
  union
  {
    hha_bitmap Bitmap;
    hha_sound Sound;
    hha_font Font;
  };
} hha_asset; 

#pragma pack(pop)
