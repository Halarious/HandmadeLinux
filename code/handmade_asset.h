
typedef struct
{
  u32 SampleCount;
  u32 ChannelCount;
  s16* Samples[2];
} loaded_sound;

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
  u32 FirstTagIndex;
  u32 OnePastLastTagIndex;
} asset_group;

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

typedef struct
{
  platform_file_handle* Handle;
  
  hha_header Header;
  hha_asset_type* AssetTypeArray;

  u32 TagBase;
} asset_file;

typedef struct transient_state transient_state;
struct assets
{
  transient_state* TransState;
  
  memory_arena Arena;

  r32 TagRange[Tag_Count];

  u32 FileCount;
  asset_file* Files;
  
  u32 TagCount;
  hha_tag* Tags;

  
  u32 AssetCount;
  hha_asset* Assets;
  asset_slot* Slots;
  
  asset_type AssetTypes[Asset_Count];

  u8* HHAContents;
  #if 0


  //hero_bitmaps HeroBitmaps[4];

  u32 DEBUGUsedAssetCount;
  u32 DEBUGUsedTagCount;
  asset_type* DEBUGAssetType;
  asset* DEBUGAsset;
#endif
};

internal inline loaded_bitmap*
GetBitmap(assets* Assets, bitmap_id ID)
{
  Assert(ID.Value <= Assets->AssetCount);

  asset_slot* Slot = Assets->Slots + ID.Value; 
  loaded_bitmap* Result = (Slot->State >= AssetState_Loaded) ? Slot->Bitmap : 0;

  return(Result);
}

internal inline loaded_sound*
GetSound(assets* Assets, sound_id ID)
{
  Assert(ID.Value <= Assets->AssetCount);

  asset_slot* Slot = Assets->Slots + ID.Value; 
  loaded_sound* Result = (Slot->State >= AssetState_Loaded) ? Slot->Sound : 0;;

  return(Result);
}

internal inline hha_sound*
GetSoundInfo(assets* Assets, sound_id ID)
{
  Assert(ID.Value <= Assets->AssetCount);
  
  hha_sound* Result = &Assets->Assets[ID.Value].Sound;
  return(Result);
}

internal inline bool32
IsBitmapIDValid(bitmap_id ID)
{
  bool32 Result = (ID.Value != 0);
  return(Result);
}

internal inline bool32
IsSoundIDValid(sound_id ID)
{
  bool32 Result = (ID.Value != 0);
  return(Result);  
}

internal void LoadBitmap(assets* Assets, bitmap_id ID);
internal void LoadSound(assets* Assets, sound_id ID);
internal inline void PrefetchBitmap(assets* Assets, bitmap_id ID) {LoadBitmap(Assets, ID);}
internal inline void PrefetchSound(assets* Assets, sound_id ID)   {LoadSound(Assets, ID);}

