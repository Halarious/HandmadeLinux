
typedef struct
{
  task_with_memory* Task;
  asset_slot* Slot;

  platform_file_handle* Handle;
  u64 Offset;
  u64 Size;
  void* Destination;

  asset_state FinalState;
} load_asset_work;

internal PLATFORM_WORK_QUEUE_CALLBACK(LoadAssetWork)
{
  load_asset_work* Work = (load_asset_work*) Data;

  Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);
  
  CompletePreviousWritesBeforeFutureWrites;

  if(PlatformNoFileErrors(Work->Handle))
    {
      Work->Slot->State = Work->FinalState;
    }
      
  EndTaskWithMemory(Work->Task);
}

internal inline platform_file_handle*
GetFileHandleFor(assets* Assets, u32 FileIndex)
{
  Assert(FileIndex < Assets->FileCount);

  platform_file_handle* Result = Assets->Files[FileIndex].Handle;

  return(Result);
}

internal void
LoadBitmap(assets* Assets, bitmap_id ID)
{
  if(ID.Value &&
     (AtomicCompareExchangeUInt32(&Assets->Slots[ID.Value].State, AssetState_Queued, AssetState_Unloaded) ==
      AssetState_Unloaded))
    {
      task_with_memory* Task = BeginTaskWithMemory(Assets->TransState);
      if(Task)
	{
	  asset* Asset = Assets->Assets + ID.Value;
	  hha_bitmap* Info = &Asset->HHA.Bitmap;
	  loaded_bitmap* Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);

	  Bitmap->AlignPercentage = V2(Info->AlignPercentage[0], Info->AlignPercentage[1]);
	  Bitmap->WidthOverHeight = (r32)Info->Dim[0] / (r32)Info->Dim[1];
	  Bitmap->Width = Info->Dim[0];
	  Bitmap->Height = Info->Dim[1];
	  Bitmap->Pitch = 4*Info->Dim[0];
	  u32 MemorySize = Bitmap->Pitch * Bitmap->Height;
	  Bitmap->Memory = PushSize(&Assets->Arena, MemorySize);

	  load_asset_work* Work = PushStruct(&Task->Arena, load_asset_work);
	  Work->Task = Task;
	  Work->Slot = Assets->Slots + ID.Value;
	  Work->Handle = GetFileHandleFor(Assets, Asset->FileIndex);
	  Work->Offset = Asset->HHA.DataOffset;
	  Work->Size = MemorySize;
	  Work->Destination = Bitmap->Memory;
	  Work->FinalState = AssetState_Loaded;
	  Work->Slot->Bitmap = Bitmap;	  
	  
	  Platform.AddEntry(Assets->TransState->LowPriorityQueue, LoadAssetWork, Work);
	}
      else
	{
	  Assets->Slots[ID.Value].State = AssetState_Unloaded;
	}
    }
}

internal void
LoadSound(assets* Assets, sound_id ID)
{
  if(ID.Value &&
     (AtomicCompareExchangeUInt32(&Assets->Slots[ID.Value].State, AssetState_Queued, AssetState_Unloaded) ==
      AssetState_Unloaded))
    {
      task_with_memory* Task = BeginTaskWithMemory(Assets->TransState);
      if(Task)
	{
	  asset* Asset = Assets->Assets + ID.Value;
	  hha_sound* Info = &Asset->HHA.Sound;

	  loaded_sound* Sound = PushStruct(&Assets->Arena, loaded_sound);
	  Sound->SampleCount = Info->SampleCount;
	  Sound->ChannelCount = Info->ChannelCount;
	  u32 ChannelSize =  Sound->SampleCount * sizeof(u16);
	  u32 MemorySize = Sound->ChannelCount * ChannelSize;

	  void* Memory = PushSize(&Assets->Arena, MemorySize);

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
	  Work->Slot = Assets->Slots + ID.Value;
	  Work->Handle = GetFileHandleFor(Assets, Asset->FileIndex);
	  Work->Offset = Asset->HHA.DataOffset;
	  Work->Size = MemorySize;
	  Work->Destination = Memory;
	  Work->FinalState = AssetState_Loaded;
	  Work->Slot->Sound = Sound;	  

	  Platform.AddEntry(Assets->TransState->LowPriorityQueue, LoadAssetWork, Work);
	}
      else
	{
	  Assets->Slots[ID.Value].State = AssetState_Unloaded;
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
GetRandomSlotFrom(assets* Assets, asset_type_id TypeID, random_series* Series)
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
GetFirstSlotFrom(assets* Assets, asset_type_id TypeID)
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
  bitmap_id Result = {GetFirstSlotFrom(Assets, TypeID)};
  return(Result);
}

internal inline bitmap_id
GetRandomBitmapFrom(assets* Assets, asset_type_id TypeID, random_series* Series)
{
  bitmap_id Result = {GetRandomSlotFrom(Assets, TypeID, Series)};
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
  sound_id Result = {GetFirstSlotFrom(Assets, TypeID)};
  return(Result);
}

internal inline sound_id
GetRandomSoundFrom(assets* Assets, asset_type_id TypeID, random_series* Series)
{
  sound_id Result = {GetRandomSlotFrom(Assets, TypeID, Series)};
  return(Result);
}

internal assets*
AllocateGameAssets(memory_arena* Arena, memory_index Size, transient_state* TransState)
{
  assets* Assets = PushStruct(Arena, assets);

  SubArena(&Assets->Arena, Arena, Size, 4);
  Assets->TransState = TransState;

  for(u32 TagTypeIndex = 0;
      TagTypeIndex < Tag_Count;
      ++TagTypeIndex)
    {
      Assets->TagRange[TagTypeIndex] = 1000000.0f;
    }
  Assets->TagRange[Tag_FacingDirection] = Tau32;

  Assets->TagCount = 0;
  Assets->AssetCount = 0;

  {  
    platform_file_group FileGroup = Platform.GetAllFilesOfTypeBegin("hha");
    Assets->FileCount = FileGroup.FileCount;
    Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);
    for(u32 FileIndex = 0;
	FileIndex < Assets->FileCount;
	++FileIndex)
      {
	asset_file* File = Assets->Files + FileIndex;

	File->TagBase = Assets->TagCount;

	ZeroStruct(File->Header);
	File->Handle = Platform.OpenFile(FileGroup, FileIndex);
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
	    Assets->TagCount += File->Header.TagCount;
	    Assets->AssetCount += File->Header.AssetCount;
	  }
	else
	  {
	    InvalidCodePath;
	  }
      }
    Platform.GetAllFilesOfTypeEnd(FileGroup);
  }
      
  Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);
  Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);
  Assets->Tags = PushArray(Arena, Assets->TagCount, hha_tag);

  for(u32 FileIndex = 0;
      FileIndex < Assets->FileCount;
      ++FileIndex)
    {
      asset_file* File = Assets->Files + FileIndex;
      if(PlatformNoFileErrors(File->Handle))
	{
	  u32 TagArraySize = sizeof(hha_tag)*File->Header.TagCount;
	  Platform.ReadDataFromFile(File->Handle, File->Header.Tags,
				    TagArraySize, Assets->Tags + File->TagBase);
	}
    }
  
  u32 AssetCount = 0;
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
			  Asset->HHA.FirstTagIndex += File->TagBase;
			  Asset->HHA.OnePastLastTagIndex += File->TagBase;
			}
		      EndTemporaryMemory(TempMem);
		    }
		}
	    }
	}
      DestType->OnePastLastAssetIndex = AssetCount;
    }

  //Assert(AssetCount == Assets->AssetCount);
  
#if 0
  loaded_file ReadResult = Platform.DEBUGReadEntireFile("../data/test/test.hha");
  if(ReadResult.Contents)
    {      
      hha_header* Header = (hha_header*)ReadResult.Contents;
      
      Assets->AssetCount = Header->AssetCount;
      Assets->Assets = (hha_asset*)((u8*) ReadResult.Contents + Header->Assets);
      Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);

      Assets->TagCount = Header->AssetCount;
      Assets->Tags = (hha_tag*)((u8*) ReadResult.Contents + Header->Tags);

      hha_asset_type* HHAAssetTypes = (hha_asset_type*)((u8*) ReadResult.Contents + Header->AssetTypes);
      
      for(u32 AssetTypeIndex = 0;
	  AssetTypeIndex < Header->AssetTypeCount;
	  ++AssetTypeIndex)
	{
	  hha_asset_type* Source = HHAAssetTypes + AssetTypeIndex;

	  if(Source->TypeID < Asset_Count)
	    {
	      asset_type* Dest = Assets->AssetTypes + Source->TypeID;
	      Assert(Dest->FirstAssetIndex == 0);
	      Assert(Dest->OnePastLastAssetIndex == 0);
	      Dest->FirstAssetIndex = Source->FirstAssetIndex;
	      Dest->OnePastLastAssetIndex = Source->OnePastLastAssetIndex;
	    }
	}
      Assets->HHAContents = (u8*)ReadResult.Contents;
    }
#endif
  
  return(Assets);
}

