
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
#pragma pack(pop)

internal inline v2
TopDownAlign(loaded_bitmap* Bitmap, v2 Align)
{
  Align.y = (Bitmap->Height - 1) - Align.y;

  Align.x = SafeRatio0(Align.x, (r32)Bitmap->Width);
  Align.y = SafeRatio0(Align.y, (r32)Bitmap->Height);
  
  return(Align);
}

internal inline void
SetTopDownAlignHero(hero_bitmaps* Bitmaps, v2 Align)
{  
  Align = TopDownAlign(&Bitmaps->Head, Align);
  
  Bitmaps->Head.AlignPercentage  = Align;
  Bitmaps->Cape.AlignPercentage  = Align;
  Bitmaps->Torso.AlignPercentage = Align;
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

internal void
LoadSound(assets* Assets, sound_id ID)
{
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
  asset* Asset = Assets->Assets + Assets->DEBUGAssetType->OnePastLastAssetIndex++;
  Asset->FirstTagIndex = 0;
  Asset->OnePastLastTagIndex = 0;
  Asset->SlotID = DEBUGAddBitmapInfo(Assets, Filename, AlignPercentage).Value;
}

internal void
EndAssetType(assets* Assets)
{
  Assert(Assets->DEBUGAssetType);
  Assets->DEBUGUsedAssetCount = Assets->DEBUGAssetType->OnePastLastAssetIndex;
  Assets->DEBUGAssetType = 0;
}

internal assets*
AllocateGameAssets(memory_arena* Arena, memory_index Size, transient_state* TransState)
{
  assets* Assets = PushStruct(Arena, assets);

  SubArena(&Assets->Arena, Arena, Size, 4);
  Assets->TransState = TransState;

  Assets->BitmapCount = 256*Asset_Count;
  Assets->DEBUGUsedBitmapCount = 1;
  Assets->BitmapInfos = PushArray(Arena, Assets->BitmapCount, asset_bitmap_info);
  Assets->Bitmaps = PushArray(Arena, Assets->BitmapCount, asset_slot);
 
  Assets->SoundCount = 1;
  Assets->Sound = PushArray(Arena, Assets->SoundCount, asset_slot);

  Assets->AssetCount = Assets->SoundCount + Assets->BitmapCount;
  Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);

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

  BeginAssetType(Assets, Asset_Stone);
  AddBitmapAsset(Assets, "../data/test2/tuft00.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "../data/test2/tuft01.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "../data/test2/tuft02.bmp", V2(0.5f, 0.5f));
  EndAssetType(Assets);
 
  hero_bitmaps *Bitmap = Assets->HeroBitmaps;
      
  Bitmap->Head
    = DEBUGLoadBMP("../data/test/test_hero_right_head.bmp", V2(0, 0));
  Bitmap->Cape
    = DEBUGLoadBMP("../data/test/test_hero_right_cape.bmp", V2(0, 0));
  Bitmap->Torso
    = DEBUGLoadBMP("../data/test/test_hero_right_torso.bmp", V2(0, 0));
  SetTopDownAlignHero(Bitmap, V2(72, 182));
  ++Bitmap;
      
  Bitmap->Head
    = DEBUGLoadBMP("../data/test/test_hero_back_head.bmp", V2(0, 0));
  Bitmap->Cape
    = DEBUGLoadBMP("../data/test/test_hero_back_cape.bmp", V2(0, 0));
  Bitmap->Torso
    = DEBUGLoadBMP("../data/test/test_hero_back_torso.bmp", V2(0, 0));
  SetTopDownAlignHero(Bitmap, V2(72, 182));
  ++Bitmap;
      
  Bitmap->Head
    = DEBUGLoadBMP("../data/test/test_hero_left_head.bmp", V2(0, 0));
  Bitmap->Cape
    = DEBUGLoadBMP("../data/test/test_hero_left_cape.bmp", V2(0, 0));
  Bitmap->Torso
    = DEBUGLoadBMP("../data/test/test_hero_left_torso.bmp", V2(0, 0));
  SetTopDownAlignHero(Bitmap, V2(72, 182));
  ++Bitmap;
      
  Bitmap->Head
    = DEBUGLoadBMP("../data/test/test_hero_front_head.bmp", V2(0, 0));
  Bitmap->Cape
    = DEBUGLoadBMP("../data/test/test_hero_front_cape.bmp", V2(0, 0));
  Bitmap->Torso
    = DEBUGLoadBMP("../data/test/test_hero_front_torso.bmp", V2(0, 0));
  SetTopDownAlignHero(Bitmap, V2(72, 182));
  
  return(Assets);
}

