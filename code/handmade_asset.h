
typedef struct
{
  loaded_bitmap Head;
  loaded_bitmap Cape;
  loaded_bitmap Torso;
} hero_bitmaps;

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
  loaded_bitmap* Bitmap;
} asset_slot;

typedef enum
  {
    Tag_Smoothness,
    Tag_Flatness,
    
    Tag_Count,
  } asset_tag_id;

typedef enum
  {
    Asset_None,
    
    Asset_Backdrop,
    Asset_Shadow,
    Asset_Tree,
    Asset_Sword,
    Asset_Stairwell,
    Asset_Rock,

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
  u32 FirstAssetIndex;
  u32 OnePastLastAssetIndex;
} asset_type;

typedef struct
{
  v2 AlignPercentage;
  r32 WidthOverHeight;
  int Width;
  int Height;
} asset_bitmap_info;

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

  u32 BitmapCount;
  asset_slot* Bitmaps;

  u32 SoundCount;
  asset_slot* Sound;

  u32 TagCount;
  asset_tag* Tags;

  u32 AssetCount;
  asset* Assets;
  
  asset_type AssetTypes[Asset_Count];

  loaded_bitmap Grass[2];
  loaded_bitmap Stone[4];
  loaded_bitmap Tuft[3];

  hero_bitmaps HeroBitmaps[4];
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

