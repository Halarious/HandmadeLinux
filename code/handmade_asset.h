
typedef struct
{
  u32 SampleCount;
  u32 ChannelCount;
  s16* Samples[2];
} loaded_sound;

typedef enum
  {
    AssetState_Unloaded,
    AssetState_Queued,
    AssetState_Loaded,
    AssetState_Locked,
  } asset_state;

typedef struct
{
  asset_state State;  
  union
  {
    loaded_bitmap* Bitmap;
    loaded_sound* Sound;
  };
} asset_slot;

typedef enum
  {
    Tag_Smoothness,
    Tag_Flatness,
    Tag_FacingDirection,
    
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
    
    Asset_Count,
  } asset_type_id;


typedef struct
{
  u32 ID;
  r32 Value;
} asset_tag;

typedef struct
{
  u32 FirstTagIndex;
  u32 OnePastLastTagIndex;
  u32 SlotID;
} asset;

typedef struct
{
  r32 E[Tag_Count];
} asset_vector;

typedef struct
{
  u32 FirstAssetIndex;
  u32 OnePastLastAssetIndex;
} asset_type;

typedef struct
{
  char* Filename;
  v2 AlignPercentage;
} asset_bitmap_info;

typedef struct
{
  char* Filename;
} asset_sound_info;

typedef struct
{
  u32 FirstTagIndex;
  u32 OnePastLastTagIndex;
} asset_group;

typedef struct transient_state transient_state;
struct assets
{
  transient_state* TransState;
  
  memory_arena Arena;

  r32 TagRange[Tag_Count];
  
  u32 BitmapCount;
  asset_bitmap_info* BitmapInfos ;
  asset_slot* Bitmaps;

  u32 SoundCount;
  asset_sound_info* SoundInfos;
  asset_slot* Sounds;

  u32 TagCount;
  asset_tag* Tags;

  u32 AssetCount;
  asset* Assets;
  
  asset_type AssetTypes[Asset_Count];

  //hero_bitmaps HeroBitmaps[4];

  u32 DEBUGUsedBitmapCount;
  u32 DEBUGUsedAssetCount;
  u32 DEBUGUsedTagCount;
  asset_type* DEBUGAssetType;
  asset* DEBUGAsset;
};

typedef struct
{
  u32 Value;
} bitmap_id;

typedef struct
{
  u32 Value;
} sound_id;

internal inline loaded_bitmap*
GetBitmap(assets* Assets, bitmap_id ID)
{
  loaded_bitmap* Result = Assets->Bitmaps[ID.Value].Bitmap;
  return(Result);
}

internal void
LoadBitmap(assets* Assets, bitmap_id ID);
internal void
LoadSound(assets* Assets, sound_id ID);

