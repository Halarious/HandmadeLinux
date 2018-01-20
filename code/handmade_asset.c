
#pragma pack(push, 1)
typedef struct
{
  u16 FileType;
  u32 FileSize;
  u16 Reserved1;
  u16 Reserved2;
  u32 BitmapOffset;
  u32 Size;
  s32 Width;
  s32 Height;
  u16 Planes;
  u16 BitsPerPixel;
  u32 Compression;
  u32 SizeOfBitmap;
  s32 HorzResolution;
  s32 VertResolution;
  u32 ColorsUsed;
  u32 ColorsImportant;

  u32 RedMask;
  u32 GreenMask;
  u32 BlueMask;
} bitmap_header; 


typedef struct
{
  u32 RIFFID;
  u32 Size;
  u32 WAVEID;
} WAVE_header;

#define RIFF_CODE(a, b, c, d) (((u32)a << 0) | ((u32)b << 8) | ((u32)c << 16) | ((u32)d << 24))

enum
  {
    WAVE_ChunkID_fmt  = RIFF_CODE('f', 'm', 't', ' '),
    WAVE_ChunkID_data = RIFF_CODE('d', 'a', 't', 'a'),
    WAVE_ChunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
    WAVE_ChunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
  };

typedef struct
{
  u32 ID;
  u32 Size;
} WAVE_Chunk;

typedef struct
{
  u16 wFormatTag;
  u16 nChannels;
  u32 nSamplesPerSec;
  u32 nAvgBytesPerSec;
  u16 nBlockAlign;
  u16 wBitsPerSample;
  u16 cbSize;
  u16 wValidBitsPerSample;
  u32 dwChannelMask;
  u8  SubFormat[16];
} WAVE_fmt;

#pragma pack(pop)

internal inline v2
TopDownAlign(loaded_bitmap* Bitmap, v2 Align)
{
  Align.y = (Bitmap->Height - 1) - Align.y;

  Align.x = SafeRatio0(Align.x, (r32)Bitmap->Width);
  Align.y = SafeRatio0(Align.y, (r32)Bitmap->Height);
  
  return(Align);
}

internal loaded_bitmap
DEBUGLoadBMP(char* Filename, v2 AlignPercentage)
{
  loaded_bitmap Result = {};

  loaded_file ReadResult = DEBUGPlatformReadEntireFile(Filename);
  if(ReadResult.ContentsSize != 0)
    {
      bitmap_header *Header = (bitmap_header*) ReadResult.Contents;
      Result.Memory = (u32*)((u8*)ReadResult.Contents + Header->BitmapOffset); 
      Result.Width  = Header->Width;
      Result.Height = Header->Height;
      Result.Pitch  = Result.Width;
      Result.AlignPercentage  = AlignPercentage;
      Result.WidthOverHeight  = SafeRatio0((r32)Result.Width, (r32)Result.Height);

      Assert(Result.Height >= 0);
      Assert(Header->Compression == 3);
      
      u32 RedMask   = Header->RedMask;
      u32 GreenMask = Header->GreenMask;
      u32 BlueMask  = Header->BlueMask;
      u32 AlphaMask = ~(RedMask | GreenMask | BlueMask);
      
      bit_scan_result RedScan   = FindLeastSignificantSetBit(RedMask);
      bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
      bit_scan_result BlueScan  = FindLeastSignificantSetBit(BlueMask);
      bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);

      Assert(RedScan.Found);
      Assert(GreenScan.Found);
      Assert(BlueScan.Found);
      Assert(AlphaScan.Found);

      s32 RedShiftDown = (s32)RedScan.Index;
      s32 GreenShiftDown = (s32)GreenScan.Index;
      s32 BlueShiftDown = (s32)BlueScan.Index;
      s32 AlphaShiftDown = (s32)AlphaScan.Index;
      
      u32 *SourceDest = Result.Memory;
      for(s32 Y = 0;
	  Y < Header->Height;
	  ++Y)
	{
	  for(s32 X = 0;
	      X < Header->Width;
	      ++X)
	    {
	      u32 Color = *SourceDest;

	      v4 Texel = V4((r32)((Color & RedMask) >> RedShiftDown),
			    (r32)((Color & GreenMask) >> GreenShiftDown),
			    (r32)((Color & BlueMask) >> BlueShiftDown),
			    (r32)((Color & AlphaMask) >> AlphaShiftDown));
	      Texel = SRGB255ToLinear1(Texel);

#if 1
	      Texel.rgb = V3MulS(Texel.a, Texel.rgb);
#endif
	      Texel = Linear1ToSRGB255(Texel);
	      
	      *SourceDest++ = (((u32)(Texel.a + 0.5f) << 24) |
			       ((u32)(Texel.r + 0.5f) << 16) |
			       ((u32)(Texel.g + 0.5f) << 8)  |
			       ((u32)(Texel.b + 0.5f) << 0));
	    }
	}
    }  

  s32 BytesPerPixel = BITMAP_BYTES_PER_PIXEL;
  Result.Pitch  = Result.Width*BytesPerPixel;

#if 0
  Result.Memory = ((u8*)Result.Memory +
		   Result.Pitch*(Result.Height - 1));
  Result.Pitch  = -Result.Width*BytesPerPixel;
#endif

  return(Result);
}

typedef struct
{
  u8* At;
  u8* Stop;
} riff_iterator;

internal inline riff_iterator
ParseChunkAt(void* At, void* Stop)
{
  riff_iterator Iter;

  Iter.At = (u8*)At;
  Iter.Stop = (u8*)Stop;

  return(Iter);
}

internal inline riff_iterator
NextChunk(riff_iterator Iter)
{
  WAVE_Chunk* Chunk = (WAVE_Chunk*) Iter.At;
  u32 Size = (Chunk->Size + 1) & ~1;
  Iter.At += sizeof(WAVE_Chunk) + Size;
  
  return(Iter);
}

internal inline bool32
RiffIteratorIsValid(riff_iterator Iter)
{
  bool32 Result = (Iter.At < Iter.Stop);
  return(Result);
}

internal inline void*
GetChunkData(riff_iterator Iter)
{
  void* Result = (Iter.At + sizeof(WAVE_Chunk));
  return(Result);
}

internal inline u32
GetType(riff_iterator Iter)
{
  WAVE_Chunk* Chunk = (WAVE_Chunk*) Iter.At;
  u32 Result = Chunk->ID;
  return(Result);
}

internal inline u32
GetChunkDataSize(riff_iterator Iter)
{
  WAVE_Chunk* Chunk = (WAVE_Chunk*) Iter.At;
  u32 Result = Chunk->Size;
  return(Result);
}

internal loaded_sound
DEBUGLoadWAV(char* Filename)
{
  loaded_sound Result = {};
  
  loaded_file ReadResult = DEBUGPlatformReadEntireFile(Filename);
  if(ReadResult.ContentsSize != 0)
    {
      WAVE_header* Header = (WAVE_header*)ReadResult.Contents;

      Assert(Header->RIFFID == WAVE_ChunkID_RIFF);
      Assert(Header->WAVEID == WAVE_ChunkID_WAVE);

      u32 ChannelCount = 0;
      u32 SampleDataSize = 0;
      s16* SampleData = 0;
      for(riff_iterator Iter = ParseChunkAt(Header + 1, (u8*)(Header + 1) + Header->Size - 4);
	  RiffIteratorIsValid(Iter);
	  Iter = NextChunk(Iter))
	{
	  switch(GetType(Iter))
	    {
	      
	    case(WAVE_ChunkID_fmt):
	      {
		WAVE_fmt* fmt = (WAVE_fmt*) GetChunkData(Iter);

		Assert(fmt->wFormatTag == 1);
		Assert(fmt->nSamplesPerSec == 48000);
		Assert(fmt->wBitsPerSample == 16);
		Assert(fmt->nBlockAlign == sizeof(u16)*fmt->nChannels);

		ChannelCount = fmt->nChannels;
	      } break;
	      
	    case(WAVE_ChunkID_data):
	      {
		SampleData = (s16*)GetChunkData(Iter);
		SampleDataSize = GetChunkDataSize(Iter);
	      } break;	      
	    }
	}
      
      Assert(ChannelCount && SampleData);

      Result.ChannelCount = ChannelCount;
      Result.SampleCount  = SampleDataSize / (ChannelCount*sizeof(u16));
      if(ChannelCount == 1)
	{
	  Result.Samples[0] = SampleData;
	  Result.Samples[1] = 0;
	}
      else if (ChannelCount == 2)
	{
	  Result.Samples[0] = SampleData;
	  Result.Samples[1] = SampleData + Result.SampleCount;

	  for(u32 SampleIndex = 0;
	      SampleIndex < Result.SampleCount;
	      ++SampleIndex)
	    {
	      s16 Source = SampleData[2*SampleIndex];
	      SampleData[2*SampleIndex] = SampleData[SampleIndex];
	      SampleData[SampleIndex] = Source;
	    }

	}
      else
	{
	  Assert(!"Invalid channel count in WAV file");
	}

      Result.ChannelCount = 1;
    }

  return(Result);
}

typedef struct
{
  assets *Assets;
  bitmap_id ID;
  task_with_memory* Task;
  loaded_bitmap* Bitmap;

  asset_state FinalState;
} load_bitmap_work;

internal PLATFORM_WORK_QUEUE_CALLBACK(LoadBitmapWork)
{
  load_bitmap_work* Work = (load_bitmap_work*) Data;

  asset_bitmap_info* Info = Work->Assets->BitmapInfos + Work->ID.Value;
  *Work->Bitmap
    = DEBUGLoadBMP(Info->Filename, Info->AlignPercentage);

  CompletePreviousWritesBeforeFutureWrites;
    
  Work->Assets->Bitmaps[Work->ID.Value].Bitmap = Work->Bitmap;
  Work->Assets->Bitmaps[Work->ID.Value].State  = Work->FinalState;
  
  EndTaskWithMemory(Work->Task);
}

internal void
LoadBitmap(assets* Assets, bitmap_id ID)
{
  if(ID.Value &&
     (AtomicCompareExchangeUInt32(&Assets->Bitmaps[ID.Value].State, AssetState_Loaded, AssetState_Queued) ==
      AssetState_Unloaded))
    {
      task_with_memory* Task = BeginTaskWithMemory(Assets->TransState);
      if(Task)
	{
	  load_bitmap_work* Work = PushStruct(&Task->Arena, load_bitmap_work);
	  
	  Work->Assets = Assets;
	  Work->ID = ID;
	  Work->Task = Task;
	  Work->Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);
	  Work->FinalState = AssetState_Loaded;
	  
	  PlatformAddEntry(Assets->TransState->LowPriorityQueue, LoadBitmapWork, Work);
	}
    }
}

typedef struct
{
  assets *Assets;
  sound_id ID;
  task_with_memory* Task;
  loaded_sound* Sound;

  asset_state FinalState;
} load_sound_work;

internal PLATFORM_WORK_QUEUE_CALLBACK(LoadSoundWork)
{
  load_sound_work* Work = (load_sound_work*) Data;

  asset_sound_info* Info = Work->Assets->SoundInfos + Work->ID.Value;
  *Work->Sound
    = DEBUGLoadWAV(Info->Filename);

  CompletePreviousWritesBeforeFutureWrites;
    
  Work->Assets->Sounds[Work->ID.Value].Sound = Work->Sound;
  Work->Assets->Sounds[Work->ID.Value].State = Work->FinalState;
  
  EndTaskWithMemory(Work->Task);
}

internal void
LoadSound(assets* Assets, sound_id ID)
{
  if(ID.Value &&
     (AtomicCompareExchangeUInt32(&Assets->Sounds[ID.Value].State, AssetState_Loaded, AssetState_Queued) ==
      AssetState_Unloaded))
    {
      task_with_memory* Task = BeginTaskWithMemory(Assets->TransState);
      if(Task)
	{
	  load_sound_work* Work = PushStruct(&Task->Arena, load_sound_work);
	  
	  Work->Assets = Assets;
	  Work->ID = ID;
	  Work->Task = Task;
	  Work->Sound = PushStruct(&Assets->Arena, loaded_sound);
	  Work->FinalState = AssetState_Loaded;
	  
	  PlatformAddEntry(Assets->TransState->LowPriorityQueue, LoadSoundWork, Work);
	}
    }
}

internal bitmap_id
BestMatchAsset(assets* Assets, asset_type_id TypeID,
	       asset_vector* MatchVector, asset_vector* WeightVector)
{
  bitmap_id Result = {};

  r32 BestDiff = Real32Maximum;

  asset_type* Type = Assets->AssetTypes + TypeID;
    for(u32 AssetIndex = Type->FirstAssetIndex;
      AssetIndex < Type->OnePastLastAssetIndex;
      ++AssetIndex)
    {
      asset* Asset = Assets->Assets + AssetIndex;

      r32 TotalWeightedDiff = 0.0f;
      for(u32 TagIndex = Asset->FirstTagIndex;
	  TagIndex < Asset->OnePastLastTagIndex;
	  ++TagIndex)
	{
	  asset_tag* Tag = Assets->Tags + TagIndex;

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
	  Result.Value = Asset->SlotID;
	}
    }
  
  return(Result);
}

internal bitmap_id
RandomAssetFrom(assets* Assets, asset_type_id TypeID, random_series* Series)
{
  bitmap_id Result = {};
  
  asset_type* Type = Assets->AssetTypes + TypeID;
  if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex)
    {
      u32 Count = Type->OnePastLastAssetIndex - Type->FirstAssetIndex;
      u32 Choice = RandomChoice(Series, Count);

      asset* Asset = Assets->Assets + Type->FirstAssetIndex + Choice;
      Result.Value = Asset->SlotID;
    }
    
  return(Result);
  
}

internal bitmap_id
GetFirstBitmapID(assets* Assets, asset_type_id TypeID)
{
  bitmap_id Result = {};
  
  asset_type* Type = Assets->AssetTypes + TypeID;
  if(Type->FirstAssetIndex != Type->OnePastLastAssetIndex)
    {
      asset* Asset = Assets->Assets + Type->FirstAssetIndex;
      Result.Value = Asset->SlotID;
    }
    
  return(Result);
}

internal bitmap_id
DEBUGAddBitmapInfo(assets* Assets, char* Filename, v2 AlignPercentage)
{
  Assert(Assets->DEBUGUsedAssetCount < Assets->BitmapCount);
  
  bitmap_id ID = {Assets->DEBUGUsedAssetCount++};

  asset_bitmap_info* Info = Assets->BitmapInfos + ID.Value;
  Info->AlignPercentage = AlignPercentage;
  Info->Filename = Filename;

  return(ID);
}

internal void
BeginAssetType(assets* Assets, asset_type_id TypeID)
{
  Assert(Assets->DEBUGAssetType == 0);
  Assets->DEBUGAssetType = Assets->AssetTypes + TypeID;
  Assets->DEBUGAssetType->FirstAssetIndex = Assets->DEBUGUsedAssetCount;
  Assets->DEBUGAssetType->OnePastLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;
}

internal void
AddBitmapAsset(assets* Assets, char* Filename, v2 AlignPercentage)
{
  Assert(Assets->DEBUGAssetType);
  Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < Assets->AssetCount);

  asset* Asset = Assets->Assets + Assets->DEBUGAssetType->OnePastLastAssetIndex++;
  Asset->FirstTagIndex = Assets->DEBUGUsedTagCount;
  Asset->OnePastLastTagIndex = Asset->FirstTagIndex;
  Asset->SlotID = DEBUGAddBitmapInfo(Assets, Filename, AlignPercentage).Value;

  Assets->DEBUGAsset = Asset;
}

internal void
AddTag(assets* Assets, asset_tag_id ID, r32 Value)
{
  Assert(Assets->DEBUGAsset);
  
  ++Assets->DEBUGAsset->OnePastLastTagIndex;
  asset_tag* Tag = Assets->Tags + Assets->DEBUGUsedTagCount++;

  Tag->ID = ID;
  Tag->Value = Value;
}

internal void
EndAssetType(assets* Assets)
{
  Assert(Assets->DEBUGAssetType);
  Assets->DEBUGUsedAssetCount = Assets->DEBUGAssetType->OnePastLastAssetIndex;
  Assets->DEBUGAssetType = 0;
  Assets->DEBUGAsset = 0;
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
  
  Assets->BitmapCount = 256*Asset_Count;
  Assets->DEBUGUsedBitmapCount = 1;
  Assets->BitmapInfos = PushArray(Arena, Assets->BitmapCount, asset_bitmap_info);
  Assets->Bitmaps = PushArray(Arena, Assets->BitmapCount, asset_slot);
 
  Assets->SoundCount = 1;
  Assets->Sounds = PushArray(Arena, Assets->SoundCount, asset_slot);

  Assets->AssetCount = Assets->SoundCount + Assets->BitmapCount;
  Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);

  Assets->TagCount = 1024*Asset_Count;
  Assets->Tags = PushArray(Arena, Assets->TagCount, asset_tag);
  
  Assets->DEBUGUsedBitmapCount = 1;
  Assets->DEBUGUsedAssetCount = 1;

  BeginAssetType(Assets, Asset_Shadow);
  AddBitmapAsset(Assets, "../data/test/test_hero_shadow.bmp",
		 V2(0.5f, 0.156602029f));
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Tree);
  AddBitmapAsset(Assets, "../data/test2/tree00.bmp",
		 V2(0.493827164f, 0.295652182f));
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Sword);
  AddBitmapAsset(Assets, "../data/test2/rock03.bmp",
		 V2(0.5f, 0.65625f));
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Grass);
  AddBitmapAsset(Assets, "../data/test2/grass00.bmp",
		 V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "../data/test2/grass01.bmp",
		 V2(0.5f, 0.5f));
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Stone);
  AddBitmapAsset(Assets, "../data/test2/ground00.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "../data/test2/ground01.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "../data/test2/ground02.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "../data/test2/ground03.bmp", V2(0.5f, 0.5f));
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Tuft);
  AddBitmapAsset(Assets, "../data/test2/tuft00.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "../data/test2/tuft01.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "../data/test2/tuft02.bmp", V2(0.5f, 0.5f));
  EndAssetType(Assets);

  r32 Angle_Right = 0.0f*Tau32;
  r32 Angle_Back = 0.25f*Tau32;
  r32 Angle_Left = 0.5f*Tau32;
  r32 Angle_Front = 0.75f*Tau32;

  v2 HeroAlign = {0.5f, 0.156682029f};
  
  BeginAssetType(Assets, Asset_Head);
  AddBitmapAsset(Assets, "../data/test/test_hero_right_head.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Right);
  AddBitmapAsset(Assets, "../data/test/test_hero_back_head.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Back);
  AddBitmapAsset(Assets, "../data/test/test_hero_left_head.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Left);
  AddBitmapAsset(Assets, "../data/test/test_hero_front_head.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Front);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Cape);
  AddBitmapAsset(Assets, "../data/test/test_hero_right_cape.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, 0.0f);
  AddBitmapAsset(Assets, "../data/test/test_hero_back_cape.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Back);
  AddBitmapAsset(Assets, "../data/test/test_hero_left_cape.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Left);
  AddBitmapAsset(Assets, "../data/test/test_hero_front_cape.bmp",HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Front);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Torso);
  AddBitmapAsset(Assets, "../data/test/test_hero_right_torso.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, 0.0f);
  AddBitmapAsset(Assets, "../data/test/test_hero_back_torso.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Back);
  AddBitmapAsset(Assets, "../data/test/test_hero_left_torso.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Left);
  AddBitmapAsset(Assets, "../data/test/test_hero_front_torso.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Front);
  EndAssetType(Assets);
  
  return(Assets);
}

