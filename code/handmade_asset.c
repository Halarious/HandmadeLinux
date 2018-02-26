
typedef struct
{
  task_with_memory* Task;
  asset* Asset;

  platform_file_handle* Handle;
  u64 Offset;
  u64 Size;
  void* Destination;
  
  u32 FinalState;
} load_asset_work;

internal PLATFORM_WORK_QUEUE_CALLBACK(LoadAssetWork)
{
  load_asset_work* Work = (load_asset_work*) Data;

  Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);
  
  CompletePreviousWritesBeforeFutureWrites;

  if(!PlatformNoFileErrors(Work->Handle))
    {
      ZeroSize(Work->Size, Work->Destination);
    }

  Work->Asset->State = Work->FinalState;
  
  EndTaskWithMemory(Work->Task);
}

internal inline platform_file_handle*
GetFileHandleFor(assets* Assets, u32 FileIndex)
{
  Assert(FileIndex < Assets->FileCount);

  platform_file_handle* Result = Assets->Files[FileIndex].Handle;

  return(Result);
}

internal void*
AcquireAssetMemory(assets* Assets, memory_index Size)
{
  void* Result = Platform.AllocateMemory(Size);
  if(Result)
    {
      Assets->TotalMemoryUsed += Size;
    }
  return(Result);
}

internal void
ReleaseAssetMemory(assets* Assets, memory_index Size, void* Memory)
{
  if(Memory)
    {
      Assets->TotalMemoryUsed -= Size;
    }
  
  Platform.DeallocateMemory(Memory, Size);
}

typedef struct
{
  u32 Total;
  u32 Data;
  u32 Section;
} asset_memory_size;

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
AddAssetHeaderToList(assets* Assets, u32 AssetIndex, asset_memory_size Size)
{
  asset_memory_header* Header = Assets->Assets[AssetIndex].Header;
  Header->AssetIndex = AssetIndex;
  Header->TotalSize = Size.Total;

  InsertAssetHeaderAtFront(Assets, Header);
}

internal inline void
RemoveAssetHeaderFromList(asset_memory_header* Header)
{
  Header->Prev->Next = Header->Next;
  Header->Next->Prev = Header->Prev;

  Header->Next = Header->Prev = 0;
}

internal void
LoadBitmap(assets* Assets, bitmap_id ID, bool32 Locked)
{
  asset* Asset = Assets->Assets + ID.Value;

  if(ID.Value &&
     (AtomicCompareExchangeUInt32((u32*)&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
      AssetState_Unloaded))
    {      
      task_with_memory* Task = BeginTaskWithMemory(Assets->TransState);
      if(Task)
	{
	  asset* Asset = Assets->Assets + ID.Value;
	  hha_bitmap* Info = &Asset->HHA.Bitmap;

	  u32 Width  = SafeTruncateUInt16(Info->Dim[0]);
	  u32 Height = SafeTruncateUInt16(Info->Dim[1]);
	  
	  asset_memory_size Size = {};
	  Size.Section = 4*Width;
	  Size.Data = Height * Size.Section;
	  Size.Total = Size.Data + sizeof(asset_memory_header);

	  Asset->Header = (asset_memory_header*) AcquireAssetMemory(Assets, Size.Total);
	  
	  loaded_bitmap* Bitmap = &Asset->Header->Bitmap;
	  Bitmap->AlignPercentage = V2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
	  Bitmap->WidthOverHeight = (r32)Info->Dim[0] / (r32)Info->Dim[1];
	  Bitmap->Width  = SafeTruncateUInt16(Info->Dim[0]);
	  Bitmap->Height = SafeTruncateUInt16(Info->Dim[1]);
	  Bitmap->Pitch = SafeTruncateInt16(Size.Section);
	  Bitmap->Memory = (Asset->Header + 1);

	  load_asset_work* Work = PushStruct(&Task->Arena, load_asset_work);
	  Work->Task = Task;
	  Work->Asset = Assets->Assets + ID.Value;
	  Work->Handle = GetFileHandleFor(Assets, Asset->FileIndex);
	  Work->Offset = Asset->HHA.DataOffset;
	  Work->Size = Size.Data;
	  Work->Destination = Bitmap->Memory;
	  Work->FinalState = (AssetState_Loaded) | (Locked ? AssetState_Lock : 0) ;

	  Asset->State |= AssetState_Lock;
      
	  if(!Locked)
	    {
	      AddAssetHeaderToList(Assets, ID.Value, Size);
	    }
	  	  
	  Platform.AddEntry(Assets->TransState->LowPriorityQueue, LoadAssetWork, Work);
	}
      else
	{
	  Asset->State = AssetState_Unloaded;
	}
    }
}

internal void
LoadSound(assets* Assets, sound_id ID)
{
  asset* Asset = Assets->Assets + ID.Value;

  if(ID.Value &&
     (AtomicCompareExchangeUInt32(&Asset->State, AssetState_Queued, AssetState_Unloaded) ==
      AssetState_Unloaded))
    {
      task_with_memory* Task = BeginTaskWithMemory(Assets->TransState);
      if(Task)
	{
	  asset* Asset = Assets->Assets + ID.Value;
	  hha_sound* Info = &Asset->HHA.Sound;

	  asset_memory_size Size = {};
	  Size.Section = Info->SampleCount * sizeof(u16);
	  Size.Data = Info->ChannelCount*Size.Section;
	  Size.Total = Size.Data + sizeof(asset_memory_header);
  
	  Asset->Header = (asset_memory_header*) AcquireAssetMemory(Assets, Size.Total);
	  loaded_sound* Sound = &Asset->Header->Sound;

	  Sound->SampleCount = Info->SampleCount;
	  Sound->ChannelCount = Info->ChannelCount;
	  u32 ChannelSize = Size.Section;

	  void* Memory = (Asset->Header + 1);

	  s16* SoundAt = (s16*)Memory;
	  for(u32 ChannelIndex = 0;
	      ChannelIndex < Sound->ChannelCount;
	      ++ChannelIndex)
	    {
	      Sound->Samples[ChannelIndex] = SoundAt;
	      SoundAt += ChannelSize;
	    }
	  
	  load_asset_work* Work = PushStruct(&Task->Arena, load_asset_work);
	  Work->Task = Task;
	  Work->Asset = Assets->Assets + ID.Value;
	  Work->Handle = GetFileHandleFor(Assets, Asset->FileIndex);
	  Work->Offset = Asset->HHA.DataOffset;
	  Work->Size = Size.Data;
	  Work->Destination = Memory;
	  Work->FinalState = (AssetState_Loaded);

	  AddAssetHeaderToList(Assets, ID.Value, Size);
	  
	  Platform.AddEntry(Assets->TransState->LowPriorityQueue, LoadAssetWork, Work);
	}
      else
	{
	  Asset->State = AssetState_Unloaded;
	}
    }
}

internal u32
GetBestMatchAssetFrom(assets* Assets, asset_type_id TypeID,
		      asset_vector* MatchVector, asset_vector* WeightVector)
{
  u32 Result = 0;

  r32 BestDiff = Real32Maximum;

  asset_type* Type = Assets->AssetTypes + TypeID;
    for(u32 AssetIndex = Type->FirstAssetIndex;
      AssetIndex < Type->OnePastLastAssetIndex;
      ++AssetIndex)
    {
      asset* Asset = Assets->Assets + AssetIndex;

      r32 TotalWeightedDiff = 0.0f;
      for(u32 TagIndex = Asset->HHA.FirstTagIndex;
	  TagIndex < Asset->HHA.OnePastLastTagIndex;
	  ++TagIndex)
	{
	  hha_tag* Tag = Assets->Tags + TagIndex;

	  r32 A = MatchVector->E[Tag->ID];
	  r32 B = Tag->Value;
	  r32 D0 = AbsoluteValue(A - B);
	  r32 D1 = AbsoluteValue((A - Assets->TagRange[Tag->ID] * SignOfR32(A)) - B);
	  r32 Difference = Minimum(D0, D1);
	  
	  r32 Weighted = WeightVector->E[Tag->ID] * Difference;
	  TotalWeightedDiff += Weighted;
	}
      
      if(BestDiff > TotalWeightedDiff)
	{
	  BestDiff = TotalWeightedDiff;
	  Result = AssetIndex;
	}
    }
  
  return(Result);
}

internal u32
GetRandomAssetFrom(assets* Assets, asset_type_id TypeID, random_series* Series)
{
  u32 Result = 0;
  
  asset_type* Type = Assets->AssetTypes + TypeID;
  if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex)
    {
      u32 Count = Type->OnePastLastAssetIndex - Type->FirstAssetIndex;
      u32 Choice = RandomChoice(Series, Count);
      Result = Type->FirstAssetIndex + Choice;
    }
    
  return(Result);
  
}
 
internal u32
GetFirstAssetFrom(assets* Assets, asset_type_id TypeID)
{
  u32 Result = 0;
  
  asset_type* Type = Assets->AssetTypes + TypeID;
  if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex)
    {
      Result = Type->FirstAssetIndex;
    }
    
  return(Result);
}

internal bitmap_id
GetBestMatchBitmapFrom(assets* Assets, asset_type_id TypeID,
		       asset_vector* MatchVector, asset_vector* WeightVector)
{
  bitmap_id Result = {GetBestMatchAssetFrom(Assets, TypeID, MatchVector, WeightVector)};
  return(Result);
}

internal inline bitmap_id
GetFirstBitmapFrom(assets* Assets, asset_type_id TypeID)
{
  bitmap_id Result = {GetFirstAssetFrom(Assets, TypeID)};
  return(Result);
}

internal inline bitmap_id
GetRandomBitmapFrom(assets* Assets, asset_type_id TypeID, random_series* Series)
{
  bitmap_id Result = {GetRandomAssetFrom(Assets, TypeID, Series)};
  return(Result);
}

internal inline sound_id
GetBestMatchSoundFrom(assets* Assets, asset_type_id TypeID,
		      asset_vector* MatchVector, asset_vector* WeightVector)
{
  sound_id Result = {GetBestMatchAssetFrom(Assets, TypeID, MatchVector, WeightVector)};
  return(Result);
}

internal inline sound_id
GetFirstSoundFrom(assets* Assets, asset_type_id TypeID)
{
  sound_id Result = {GetFirstAssetFrom(Assets, TypeID)};
  return(Result);
}

internal inline sound_id
GetRandomSoundFrom(assets* Assets, asset_type_id TypeID, random_series* Series)
{
  sound_id Result = {GetRandomAssetFrom(Assets, TypeID, Series)};
  return(Result);
}

internal assets*
AllocateGameAssets(memory_arena* Arena, memory_index Size, transient_state* TransState)
{
  assets* Assets = PushStruct(Arena, assets);

  SubArena(&Assets->Arena, Arena, Size, 4);
  Assets->TransState = TransState;
  Assets->TotalMemoryUsed = 0;
  Assets->TargetMemoryUsed = Size;

  Assets->LoadedAssetSentinel.Next = &Assets->LoadedAssetSentinel;
  Assets->LoadedAssetSentinel.Prev = &Assets->LoadedAssetSentinel;
  
  for(u32 TagTypeIndex = 0;
      TagTypeIndex < Tag_Count;
      ++TagTypeIndex)
    {
      Assets->TagRange[TagTypeIndex] = 1000000.0f;
    }
  Assets->TagRange[Tag_FacingDirection] = Tau32;

  Assets->TagCount = 1;
  Assets->AssetCount = 1;
  
  {  
    platform_file_group* FileGroup = Platform.GetAllFilesOfTypeBegin("hha");
    Assets->FileCount = FileGroup->FileCount;
    Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);
    for(u32 FileIndex = 0;
	FileIndex < Assets->FileCount;
	++FileIndex)
      {
	asset_file* File = Assets->Files + FileIndex;

	File->TagBase = Assets->TagCount;

	ZeroStruct(File->Header);
	File->Handle = Platform.OpenNextFile(FileGroup);
	Platform.ReadDataFromFile(File->Handle, 0, sizeof(File->Header), &File->Header);

	u32 AssetTypeArraySize = File->Header.AssetTypeCount*sizeof(hha_asset_type);
	File->AssetTypeArray = (hha_asset_type*)PushSize(Arena, AssetTypeArraySize);
	Platform.ReadDataFromFile(File->Handle, File->Header.AssetTypes,
				  AssetTypeArraySize, File->AssetTypeArray);

	if(File->Header.MagicValue != HHA_MAGIC_VALUE)
	  {
	    Platform.FileError(File->Handle, "HHA File has invalid magic value");
	  }

	if(File->Header.Version > HHA_VERSION)
	  {
	    Platform.FileError(File->Handle, "HHA File is a late version");
	  }
      
	if(PlatformNoFileErrors(File->Handle))
	  {
	    Assets->TagCount   += (File->Header.TagCount - 1);
	    Assets->AssetCount += (File->Header.AssetCount - 1);
	  }
	else
	  {
	    InvalidCodePath;
	  }
      }
    Platform.GetAllFilesOfTypeEnd(FileGroup);
  }
      
  Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);
  Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);
  Assets->Tags = PushArray(Arena, Assets->TagCount, hha_tag);
  
  ZeroStruct(Assets->Tags[0]);

  for(u32 FileIndex = 0;
      FileIndex < Assets->FileCount;
      ++FileIndex)
    {
      asset_file* File = Assets->Files + FileIndex;
      if(PlatformNoFileErrors(File->Handle))
	{
	  u32 TagArraySize = sizeof(hha_tag)*(File->Header.TagCount - 1);
	  Platform.ReadDataFromFile(File->Handle, File->Header.Tags + sizeof(hha_tag),
				    TagArraySize, Assets->Tags + File->TagBase);
	}
    }
  
  u32 AssetCount = 0;
  ZeroStruct(*(Assets->Assets + AssetCount));
  ++AssetCount;
  
  for(u32 DestTypeID = 0;
      DestTypeID < Asset_Count;
      ++DestTypeID)
    {
      asset_type* DestType = Assets->AssetTypes + DestTypeID;
      DestType->FirstAssetIndex = AssetCount;
            
      for(u32 FileIndex = 0;
	  FileIndex < Assets->FileCount;
	  ++FileIndex)
	{
	  asset_file* File = Assets->Files + FileIndex;
	  if(PlatformNoFileErrors(File->Handle))
	    {
	      for(u32 SourceIndex = 0;
		  SourceIndex < File->Header.AssetTypeCount;
		  ++SourceIndex)
		{
		  hha_asset_type* SourceType = File->AssetTypeArray + SourceIndex;
		  if(SourceType->TypeID == DestTypeID)
		    {
		      u32 AssetCountForType = (SourceType->OnePastLastAssetIndex -
					       SourceType->FirstAssetIndex);
		      
		      temporary_memory TempMem = BeginTemporaryMemory(&TransState->TransientArena);
		      hha_asset* HHAAssetArray = PushArray(&TransState->TransientArena, AssetCountForType, hha_asset);
		      Platform.ReadDataFromFile(File->Handle,
						File->Header.Assets + SourceType->FirstAssetIndex*sizeof(hha_asset),
						AssetCountForType * sizeof(hha_asset),
						HHAAssetArray);
		      for(u32 AssetIndex = 0;
			  AssetIndex < AssetCountForType;
			  ++AssetIndex)
			{
			  hha_asset* HHAAsset = HHAAssetArray + AssetIndex;

			  Assert(AssetCount < Assets->AssetCount);
			  asset* Asset = Assets->Assets + AssetCount++;

			  Asset->FileIndex = FileIndex;
			  Asset->HHA = *HHAAsset;
			  if(Asset->HHA.FirstTagIndex == 0)
			    {
			      Asset->HHA.OnePastLastTagIndex = 0;
			    }
			  else
			    {
			      Asset->HHA.FirstTagIndex += (File->TagBase - 1);
			      Asset->HHA.OnePastLastTagIndex += (File->TagBase - 1);
			    }

			}
		      EndTemporaryMemory(TempMem);
		    }
		}
	    }
	}
      DestType->OnePastLastAssetIndex = AssetCount;
    }

  Assert(AssetCount == Assets->AssetCount);
  
  return(Assets);
}

internal void
MoveHeaderToFront(assets* Assets, asset* Asset)
{
  if(!IsLocked(Asset))
    {
      asset_memory_header* Header = Asset->Header;

      RemoveAssetHeaderFromList(Header);
      InsertAssetHeaderAtFront(Assets, Header);
    }
}

internal void
EvictAsset(assets* Assets, asset_memory_header* Header)
{
  u32 AssetIndex = Header->AssetIndex;
  asset* Asset = Assets->Assets + AssetIndex;

  Assert(GetState(Asset) == AssetState_Loaded);
  Assert(!IsLocked(Asset));

  RemoveAssetHeaderFromList(Header);
  ReleaseAssetMemory(Assets, Asset->Header->TotalSize, Asset->Header);
  Asset->State = AssetState_Unloaded;
  Asset->Header = 0;
}

internal void
FreeAssetsAsNecessary(assets* Assets)
{
  while(Assets->TotalMemoryUsed > Assets->TargetMemoryUsed)
    {
      asset_memory_header* Header = Assets->LoadedAssetSentinel.Prev;
      if( (Header != &Assets->LoadedAssetSentinel))
	{
	  asset* Asset = Assets->Assets + Header->AssetIndex;
	  if(GetState(Asset) >= AssetState_Loaded)
	    {
	      EvictAsset(Assets, Header);
	    }
	}
      else
	{
	  InvalidCodePath;
	  break;
	}
    }
}

