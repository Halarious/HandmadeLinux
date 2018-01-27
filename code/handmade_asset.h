
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

    
    Asset_Bloop,
    Asset_Crack,
    Asset_Drop,
    Asset_Glide,
    Asset_Music,
    Asset_Puhp,
        
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
  u32 FirstSampleIndex;
  u32 SampleCount;
  sound_id NextIDToPlay;
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
  u32 DEBUGUsedSoundCount;
  u32 DEBUGUsedAssetCount;
  u32 DEBUGUsedTagCount;
  asset_type* DEBUGAssetType;
  asset* DEBUGAsset;
};

internal inline loaded_bitmap*
GetBitmap(assets* Assets, bitmap_id ID)
{
  Assert(ID.Value <= Assets->BitmapCount);
    
  loaded_bitmap* Result = Assets->Bitmaps[ID.Value].Bitmap;
  return(Result);
}

internal inline loaded_sound*
GetSound(assets* Assets, sound_id ID)
{
  Assert(ID.Value <= Assets->SoundCount);
  
  loaded_sound* Result = Assets->Sounds[ID.Value].Sound;
  return(Result);
}

internal inline asset_sound_info*
GetSoundInfo(assets* Assets, sound_id ID)
{
  Assert(ID.Value <= Assets->SoundCount);
  
  asset_sound_info* Result = Assets->SoundInfos + ID.Value;
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

