
typedef struct
{
  s16* Samples[2];
  u32 SampleCount;
  u32 ChannelCount;
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
    AssetState_Operating,
  } asset_state;

typedef struct asset_memory_header asset_memory_header;
struct asset_memory_header
{
  asset_memory_header* Next;
  asset_memory_header* Prev;

  u32 AssetIndex;
  u32 TotalSize;
  u32 GenerationID;
  union
  {
    loaded_bitmap Bitmap;
    loaded_sound Sound;
  };
};

typedef struct
{
  u32 State;
  asset_memory_header* Header;

  hha_asset HHA;
  u32 FileIndex;
} asset;

typedef struct
{
  platform_file_handle Handle;
  
  hha_header Header;
  hha_asset_type* AssetTypeArray;

  u32 TagBase;
} asset_file;

typedef enum
  {
    AssetMemory_Used = 0x1,
  } asset_memory_block_flags;

typedef struct asset_memory_block asset_memory_block;
struct asset_memory_block
{
  asset_memory_block* Prev;
  asset_memory_block* Next;
  memory_index Flags;
  memory_index Size;
};

typedef struct transient_state transient_state;
struct assets
{
  transient_state* TransState;  

  asset_memory_block MemorySentinel;
  asset_memory_header LoadedAssetSentinel;
  
  r32 TagRange[Tag_Count];

  u32 FileCount;
  asset_file* Files;
  
  u32 TagCount;
  hha_tag* Tags;
  
  u32 AssetCount;
  asset* Assets;
    
  asset_type AssetTypes[Asset_Count];
};

internal void
MoveHeaderToFront(assets* Assets, asset* Asset);

internal inline asset_memory_header*
GetAsset(assets* Assets, u32 ID)
{
  Assert(ID <= Assets->AssetCount);
  asset* Asset = Assets->Assets + ID; 

  asset_memory_header* Result = 0;
  for(;;)
    {
      u32 State = Asset->State;
      if(State == AssetState_Loaded)
	{
	  if(AtomicCompareExchangeUInt32(&Asset->State, AssetState_Operating, State) == State)
	    {
	      Result = Asset->Header;
	      MoveHeaderToFront(Assets, Asset);
#if 0
	      if(Asset->Header->GenerationID < GenerationID)
		{
		  Asset->Header->GenerationID = GenerationID;
		}
#endif	      
	      CompletePreviousWritesBeforeFutureWrites;

	      Asset->State = State;

	      break;
	    }
	}
      else if(State != AssetState_Operating)
	{
	  break;
	}
    }
  
  return(Result);  
}

internal inline loaded_bitmap*
GetBitmap(assets* Assets, bitmap_id ID)
{
  asset_memory_header* Header = GetAsset(Assets, ID.Value);
  loaded_bitmap* Result = Header ? &Header->Bitmap : 0;
  
  return(Result);
}

internal inline loaded_sound*
GetSound(assets* Assets, sound_id ID)
{
  asset_memory_header* Header = GetAsset(Assets, ID.Value);
  loaded_sound* Result = Header ? &Header->Sound : 0;
  
  return(Result);
}

internal inline hha_sound*
GetSoundInfo(assets* Assets, sound_id ID)
{
  Assert(ID.Value <= Assets->AssetCount);
  
  hha_sound* Result = &Assets->Assets[ID.Value].HHA.Sound;
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

internal inline sound_id
GetNextSoundInChain(assets* Assets, sound_id ID)
{
  sound_id Result = {};

  hha_sound* Info = GetSoundInfo(Assets, ID);
  switch(Info->Chain)
    {

    case(HHASoundChain_None):
      {
      } break;
      
    case(HHASoundChain_Loop):
      {
	Result = ID;
      } break;
      
    case(HHASoundChain_Advance):
      {
	Result.Value = ID.Value + 1;
      } break;

      InvalidCase;
    }

  return(Result);
}
