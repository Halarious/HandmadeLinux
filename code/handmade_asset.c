
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
DEBUGLoadBMP(char* Filename, s32 AlignX, s32 TopDownAlignY)
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
      Result.AlignPercentage  = TopDownAlign(&Result, V2i(AlignX, TopDownAlignY));
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
  char* Filename;
  bitmap_id ID;
  task_with_memory* Task;
  loaded_bitmap* Bitmap;

  bool32 HasAlignment;
  s32 AlignX;
  s32 TopDownAlignY;

  asset_state FinalState;
} load_bitmap_work;

internal PLATFORM_WORK_QUEUE_CALLBACK(LoadBitmapWork)
{
  load_bitmap_work* Work = (load_bitmap_work*) Data;

  if(Work->HasAlignment)
    {
      *Work->Bitmap
	= DEBUGLoadBMP(Work->Filename, Work->AlignX, Work->TopDownAlignY);
    }
  else
    {
      *Work->Bitmap
	= DEBUGLoadBMP(Work->Filename, 0, 0);
    }

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
	  Work->Filename = "";
	  Work->Task = Task;
	  Work->Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);
	  Work->HasAlignment = false;
	  Work->FinalState = AssetState_Loaded;
	  
	  switch(ID.Value)
	    {
	    case(Asset_Backdrop):
	      {
		Work->Filename = "../data/test/test_background.bmp";
	      } break;
	    case(Asset_Shadow):
	      {
		Work->Filename = "../data/test/test_hero_shadow.bmp";
		Work->HasAlignment = true;
		Work->AlignX = 72;
		Work->TopDownAlignY = 182;
	      } break;
	    case(Asset_Tree):
	      {
		Work->Filename = "../data/test2/tree00.bmp";
		Work->HasAlignment = true;
		Work->AlignX = 40;
		Work->TopDownAlignY = 80;
	      } break;
	    case(Asset_Stairwell):
	      {
		Work->Filename = "../data/test2/rock02.bmp";
	      } break;
	    case(Asset_Sword):
	      {
		Work->Filename = "../data/test2/rock03.bmp";//, 29, 10);
		Work->HasAlignment = true;
		Work->AlignX = 29;
		Work->TopDownAlignY = 10;
	      } break;

	      InvalidDefaultCase;
	    }
      
	  PlatformAddEntry(Assets->TransState->LowPriorityQueue, LoadBitmapWork, Work);
	}
    }
}

internal void
LoadSound(assets* Assets, sound_id ID)
{
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

internal assets*
AllocateGameAssets(memory_arena* Arena, memory_index Size, transient_state* TransState)
{
  assets* Assets = PushStruct(Arena, assets);

  SubArena(&Assets->Arena, Arena, Size, 4);
  Assets->TransState = TransState;

  Assets->BitmapCount = Asset_Count;
  Assets->Bitmaps = PushArray(Arena, Assets->BitmapCount, asset_slot);

  Assets->SoundCount = 1;
  Assets->Sound = PushArray(Arena, Assets->SoundCount, asset_slot);

  Assets->TagCount = 0;
  Assets->Tags = 0;

  Assets->AssetCount = Assets->BitmapCount;
  Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);

  for(u32 AssetID = 0;
      AssetID < Asset_Count;
      ++AssetID)
  {
    asset_type *Type = Assets->AssetTypes + AssetID;
    Type->FirstAssetIndex = AssetID;
    Type->OnePastLastAssetIndex = AssetID + 1;

    asset* Asset = Assets->Assets + Type->FirstAssetIndex;
    Asset->FirstTagIndex = 0;
    Asset->OnePastLastTagIndex = 0;
    Asset->SlotID = Type->FirstAssetIndex;
  }

  Assets->Grass[0]
    = DEBUGLoadBMP("../data/test2/grass00.bmp", 0, 0);
  Assets->Grass[1]
    = DEBUGLoadBMP("../data/test2/grass01.bmp", 0, 0);

  Assets->Stone[0]
    = DEBUGLoadBMP("../data/test2/ground00.bmp", 0, 0);
  Assets->Stone[1]
    = DEBUGLoadBMP("../data/test2/ground01.bmp", 0, 0);
  Assets->Stone[2]
    = DEBUGLoadBMP("../data/test2/ground02.bmp", 0, 0);
  Assets->Stone[3]
    = DEBUGLoadBMP("../data/test2/ground03.bmp", 0, 0);

  Assets->Tuft[0]
    = DEBUGLoadBMP("../data/test2/tuft00.bmp", 0, 0);
  Assets->Tuft[1]
    = DEBUGLoadBMP("../data/test2/tuft01.bmp", 0, 0);
  Assets->Tuft[2]
    = DEBUGLoadBMP("../data/test2/tuft02.bmp", 0, 0);

  hero_bitmaps *Bitmap = Assets->HeroBitmaps;
      
  Bitmap->Head
    = DEBUGLoadBMP("../data/test/test_hero_right_head.bmp", 0, 0);
  Bitmap->Cape
    = DEBUGLoadBMP("../data/test/test_hero_right_cape.bmp", 0, 0);
  Bitmap->Torso
    = DEBUGLoadBMP("../data/test/test_hero_right_torso.bmp", 0, 0);
  SetTopDownAlignHero(Bitmap, V2(72, 182));
  ++Bitmap;
      
  Bitmap->Head
    = DEBUGLoadBMP("../data/test/test_hero_back_head.bmp", 0, 0);
  Bitmap->Cape
    = DEBUGLoadBMP("../data/test/test_hero_back_cape.bmp", 0, 0);
  Bitmap->Torso
    = DEBUGLoadBMP("../data/test/test_hero_back_torso.bmp", 0, 0);
  SetTopDownAlignHero(Bitmap, V2(72, 182));
  ++Bitmap;
      
  Bitmap->Head
    = DEBUGLoadBMP("../data/test/test_hero_left_head.bmp", 0, 0);
  Bitmap->Cape
    = DEBUGLoadBMP("../data/test/test_hero_left_cape.bmp", 0, 0);
  Bitmap->Torso
    = DEBUGLoadBMP("../data/test/test_hero_left_torso.bmp", 0, 0);
  SetTopDownAlignHero(Bitmap, V2(72, 182));
  ++Bitmap;
      
  Bitmap->Head
    = DEBUGLoadBMP("../data/test/test_hero_front_head.bmp", 0, 0);
  Bitmap->Cape
    = DEBUGLoadBMP("../data/test/test_hero_front_cape.bmp", 0, 0);
  Bitmap->Torso
    = DEBUGLoadBMP("../data/test/test_hero_front_torso.bmp", 0, 0);
  SetTopDownAlignHero(Bitmap, V2(72, 182));

  return(Assets);
}

