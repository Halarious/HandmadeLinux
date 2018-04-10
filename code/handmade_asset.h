
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
  u32 NextGenerationID;

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

  u32 OperationLock;

  u32 InFlightGenerationCount;
  u32 InFlightGenerations[16];
};

internal inline void
BeginAssetLock(assets* Assets)
{
  for(;;)
    {
      if(AtomicCompareExchangeUInt32(&Assets->OperationLock, 1, 0) == 0)
	{
	  break;
	}
    }
}

internal inline void
EndAssetLock(assets* Assets)
{
  CompletePreviousWritesBeforeFutureWrites;
  Assets->OperationLock = 0;
}

internal inline void
InsertAssetHeaderAtFront(assets* Assets, asset_memory_header* Header)
{
  asset_memory_header* Sentinel = &Assets->LoadedAssetSentinel;

  Header->Prev = Sentinel;
  Header->Next = Sentinel->Next;

  Header->Next->Prev = Header;
  Header->Prev->Next = Header;
}

internal inline void
RemoveAssetHeaderFromList(asset_memory_header* Header)
{
  Header->Prev->Next = Header->Next;
  Header->Next->Prev = Header->Prev;

  Header->Next = Header->Prev = 0;
}

internal inline asset_memory_header*
GetAsset(assets* Assets, u32 ID, u32 GenerationID)
{
  Assert(ID <= Assets->AssetCount);
  asset* Asset = Assets->Assets + ID; 

  asset_memory_header* Result = 0;
  BeginAssetLock(Assets);

  if(Asset->State == AssetState_Loaded)
    {
      Result = Asset->Header;
            
      RemoveAssetHeaderFromList(Result);
      InsertAssetHeaderAtFront(Assets, Result);

      if(Asset->Header->GenerationID < GenerationID)
	{
	  Asset->Header->GenerationID = GenerationID;
	}

      CompletePreviousWritesBeforeFutureWrites;
    }

  EndAssetLock(Assets);
  
  return(Result);  
}

internal inline loaded_bitmap*
GetBitmap(assets* Assets, bitmap_id ID, u32 GenerationID)
{
  asset_memory_header* Header = GetAsset(Assets, ID.Value, GenerationID);
  loaded_bitmap* Result = Header ? &Header->Bitmap : 0;
  
  return(Result);
}

internal inline hha_bitmap*
GetBitmapInfo(assets* Assets, bitmap_id ID)
{
  Assert(ID.Value <= Assets->AssetCount);
  
  hha_bitmap* Result = &Assets->Assets[ID.Value].HHA.Bitmap;
  return(Result);
}

internal inline loaded_sound*
GetSound(assets* Assets, sound_id ID, u32 GenerationID)
{
  asset_memory_header* Header = GetAsset(Assets, ID.Value, GenerationID);
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

internal void LoadBitmap(assets* Assets, bitmap_id ID, bool32 Immediate);
internal void LoadSound(assets* Assets, sound_id ID);
internal inline void PrefetchBitmap(assets* Assets, bitmap_id ID) {LoadBitmap(Assets, ID, false);}
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

internal inline u32
BeginGeneration(assets* Assets)
{
  BeginAssetLock(Assets);

  Assert(Assets->InFlightGenerationCount < ArrayCount(Assets->InFlightGenerations));
  u32 Result = Assets->NextGenerationID++;
  Assets->InFlightGenerations[Assets->InFlightGenerationCount++] = Result;

  EndAssetLock(Assets);

  return(Result);
}

internal inline void
EndGeneration(assets* Assets, u32 GenerationID)
{
  BeginAssetLock(Assets);  

  for(u32 Index = 0;
      Index < Assets->InFlightGenerationCount;
      ++Index)
    {
      if(Assets->InFlightGenerations[Index] == GenerationID)
	{
	  Assets->InFlightGenerations[Index] = Assets->InFlightGenerations[--Assets->InFlightGenerationCount];
	  break;
	}
    }
  
  EndAssetLock(Assets);
}

