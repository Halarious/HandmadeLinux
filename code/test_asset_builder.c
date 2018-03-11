#include "test_asset_builder.h"

#define USE_FONTS_FROM_LINUX 1
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#if USE_FONTS_FROM_LINUX

#else
#define STB_TRUETYPE_IMPLEMENTATION 1
#include "stb_truetype.h"
#endif
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


typedef struct
{
  int Width;
  int Height;
  int Pitch;
  void* Memory;

  void* Free;
} loaded_bitmap;

typedef struct
{
  u32 ContentsSize;
  void* Contents;
} entire_file;

entire_file
ReadEntireFile(char* Filename)
{
  entire_file Result = {};

  FILE* In = fopen(Filename, "rb");
  if(In)
    {
      fseek(In, 0, SEEK_END);
      Result.ContentsSize = ftell(In);
      fseek(In, 0, SEEK_SET);
      
      Result.Contents = malloc(Result.ContentsSize);
      fread(Result.Contents, Result.ContentsSize, 1, In);

      fclose(In);
    }
  else
    {
      printf("Error, Cannot open file %s", Filename);
    }
    
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

typedef struct
{
  u32 SampleCount;
  u32 ChannelCount;
  s16* Samples[2];

  void* Free;
} loaded_sound;

internal loaded_sound
LoadWAV(char* Filename, u32 SectionFirstSampleIndex, u32 SectionSampleCount)
{
  loaded_sound Result = {};
  
  entire_file ReadResult = ReadEntireFile(Filename);
  if(ReadResult.ContentsSize != 0)
    {
      Result.Free = ReadResult.Contents;

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
      u32 SampleCount  = SampleDataSize / (ChannelCount*sizeof(u16));
      if(ChannelCount == 1)
	{
	  Result.Samples[0] = SampleData;
	  Result.Samples[1] = 0;
	}
      else if (ChannelCount == 2)
	{
	  Result.Samples[0] = SampleData;
	  Result.Samples[1] = SampleData + SampleCount;

	  for(u32 SampleIndex = 0;
	      SampleIndex < SampleCount;
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

      bool32 AtEnd = true;
      Result.ChannelCount = 1;
      if(SectionSampleCount)
	{
	  Assert(SectionFirstSampleIndex + SectionSampleCount <= SampleCount);
	  AtEnd = (SectionFirstSampleIndex + SectionSampleCount == SampleCount);
	  SampleCount = SectionSampleCount;
	  for(u32 ChannelIndex = 0;
	      ChannelIndex < Result.ChannelCount;
	      ++ChannelIndex)
	    {
	      Result.Samples[ChannelIndex] += SectionFirstSampleIndex;
	    }
	}
      
      if(AtEnd)
	{
	  u32 SampleCountAlign8 = Align8(SampleCount);
	  for(u32 ChannelIndex = 0;
	      ChannelIndex < Result.ChannelCount;
	      ++ChannelIndex)
	    {
	      for(u32 SampleIndex = SampleCount;
		  SampleIndex < (SampleCount + 8);
		  ++SampleIndex)
		{
		  Result.Samples[ChannelIndex][SampleIndex] = 0;	      
		}
	    }
	}
      Result.SampleCount = SampleCount;
    }

  return(Result);
}

internal loaded_bitmap
LoadBMP(char* Filename)
{
  loaded_bitmap Result = {};

  entire_file ReadResult = ReadEntireFile(Filename);
  if(ReadResult.ContentsSize != 0)
    {
      Result.Free = ReadResult.Contents;

      bitmap_header *Header = (bitmap_header*) ReadResult.Contents;
      Result.Memory = (u32*)((u8*)ReadResult.Contents + Header->BitmapOffset); 
      Result.Width  = Header->Width;
      Result.Height = Header->Height;
      Result.Pitch  = Result.Width;
      
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

internal loaded_bitmap
LoadGlyphBitmap(char* Filename, char* Fontname, u32 Codepoint)
{
  loaded_bitmap Result = {};

#if USE_FONTS_FROM_LINUX
  Display* Display = XOpenDisplay(0);
  int Screen = DefaultScreen(Display);
  GC GraphicsContext = DefaultGC(Display, Screen);

  Pixmap Pixmap = XCreatePixmap(Display,
				DefaultRootWindow(Display),
				1024,
				1024,
				DefaultDepth(Display, Screen));

  XSetBackground(Display, GraphicsContext, BlackPixel(Display, Screen));
  
  XFontStruct* FontStruct = XLoadQueryFont( Display, Fontname);
  
  wchar_t CheesePoint  = (wchar_t) Codepoint;
  XChar2b XCheesePoint = {Codepoint << 8, Codepoint };
  
  int Dir, Asc, Dsc;
  XCharStruct CharInfo;
  XTextExtents16(FontStruct, &XCheesePoint, 1,
		 &Dir,
		 &Asc,
		 &Dsc,
		 &CharInfo);
  
  int Height = CharInfo.ascent + CharInfo.descent + 1;
  int Width  = CharInfo.width;
  
  XSetForeground(Display, GraphicsContext, BlackPixel(Display, Screen));  
  XFillRectangle(Display, Pixmap, GraphicsContext, 0, 0, Width, Height);

  XSetForeground(Display, GraphicsContext, WhitePixel(Display, Screen));
  XDrawImageString16(Display, Pixmap, GraphicsContext,
		     FontStruct->max_bounds.lbearing + 0, //NOTE: For some reason Xlib ALWAYS draws chars 1 pixel to the left?
		     Asc - (Asc - CharInfo.ascent) + 0,
		     &XCheesePoint, 1);

  XImage* GlyphMemory = XGetImage(Display, Pixmap,
				  0,
				  0,
				  Width,
				  Height,
				  0xffffffffffffffffL,
				  ZPixmap);
#if 1
  s32 MinX = 10000;
  s32 MinY = 10000;
  s32 MaxX = -10000;
  s32 MaxY = -10000;
  for(s32 Y = 0;
      Y < Height;
      ++Y)
    {
      for(s32 X = 0;
	  X < Width;
	  ++X)
	{
	  u32 Pixel = XGetPixel(GlyphMemory, X, Y);

	  if(Pixel != 0)
	    {

	      if(MinX > X)
		{
		  MinX = X;
		}

	      if(MinY > Y)
		{
		  MinY = Y;
		}
	      
	      if(MaxX < X)
		{
		  MaxX = X;
		}

	      if(MaxY < Y)
		{
		  MaxY = Y;
		}
	    }
	}
    }   

  if(MinX <= MaxX)
#else
    u32 MinX = 0;
  u32 MinY = 0;
  u32 MaxX = Width;
  u32 MaxY = Height;
#endif
    {
  --MinX;
  --MinY;
  ++MaxX;
  ++MaxY;
  
      Width  = (MaxX - MinX) + 1;
      Height = (MaxY - MinY) + 1;
      
      Result.Width  = Width;
      Result.Height = Height;
      Result.Pitch  = Result.Width * BITMAP_BYTES_PER_PIXEL;
      Result.Memory = malloc(Height * Result.Pitch);
      Result.Free   = Result.Memory;

      u8* DestRow = (u8*)Result.Memory + (Height - 1)*Result.Pitch;
      for(s32 Y = MinY;
	  Y < MaxY;
	  ++Y)
	{
	  u32* Dest = (u32*) DestRow;
	  for(s32 X = MinX;
	      X < MaxX;
	      ++X)
	    {
	      unsigned long Pixel = XGetPixel(GlyphMemory, X, Y);
	      u8 Grey  = (u8)(Pixel & 0xFF);
	      u8 Alpha = 0xFF;
	      *Dest++ = ((Alpha << 24) |
			 (Grey << 16) |
			 (Grey <<  8) |
			 (Grey <<  0));
	    }
	  
	  DestRow -= Result.Pitch;
	}
    }  

    XUnloadFont(Display, FontStruct->fid);
#else  

  entire_file TTFFile = ReadEntireFile(Filename);
  if(TTFFile.ContentsSize != 0)
    {
      stbtt_fontinfo Font;
      stbtt_InitFont(&Font, (u8*)TTFFile.Contents, stbtt_GetFontOffsetForIndex((u8*)TTFFile.Contents, 0));
      
      int Width, Height, XOffset, YOffset;
      u8* MonoBitmap = stbtt_GetCodepointBitmap(&Font, 0, stbtt_ScaleForPixelHeight(&Font, 128.0f),
						Codepoint, &Width, &Height, &XOffset, &YOffset);

      Result.Width  = Width;
      Result.Height = Height;
      Result.Pitch  = Result.Width * BITMAP_BYTES_PER_PIXEL;
      Result.Memory = malloc(Height * Result.Pitch);
      Result.Free   = Result.Memory;

      u8* Source = MonoBitmap;
      u8* DestRow = (u8*)Result.Memory + (Height - 1)*Result.Pitch;
      for(s32 Y = 0;
	  Y < Height;
	  ++Y)
	{
	  u32* Dest = (u32*) DestRow;
	  for(s32 X = 0;
	      X < Width;
	      ++X)
	    {
	      u8 Alpha = *Source++;
	      *Dest++ = ((Alpha << 24) |
			 (Alpha << 16) |
			 (Alpha <<  8) |
			 (Alpha <<  0));
	    }
	  
	  DestRow -= Result.Pitch;
	}
      
      stbtt_FreeBitmap(MonoBitmap, 0);
      free(TTFFile.Contents);
    }
#endif  
  return(Result);
}

internal void
BeginAssetType(assets* Assets, asset_type_id TypeID)
{
  Assert(Assets->DEBUGAssetType == 0);
  Assets->DEBUGAssetType = Assets->AssetTypes + TypeID;
  Assets->DEBUGAssetType->TypeID = TypeID;
  Assets->DEBUGAssetType->FirstAssetIndex = Assets->AssetCount;
  Assets->DEBUGAssetType->OnePastLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;
}

internal bitmap_id
AddBitmapAsset(assets* Assets, char* Filename, r32 AlignPercentageX, r32 AlignPercentageY)
{
  Assert(Assets->DEBUGAssetType);
  Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

  bitmap_id Result = {Assets->DEBUGAssetType->OnePastLastAssetIndex++};
  asset_source* Source = Assets->AssetSources + Result.Value;
  hha_asset* HHA = Assets->Assets + Result.Value;
  HHA->FirstTagIndex = Assets->TagCount;
  HHA->OnePastLastTagIndex = HHA->FirstTagIndex;
  HHA->Bitmap.AlignPercentage[0] = AlignPercentageX;
  HHA->Bitmap.AlignPercentage[1] = AlignPercentageY;

  Source->Type = AssetType_Bitmap;
  Source->Filename = Filename;

  Assets->AssetIndex = Result.Value;
  
  return(Result);
}

internal bitmap_id
AddCharacterAsset(assets* Assets, char* FontFile, char* Fontname, u32 Codepoint, r32 AlignPercentageX, r32 AlignPercentageY)
{
  Assert(Assets->DEBUGAssetType);
  Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

  bitmap_id Result = {Assets->DEBUGAssetType->OnePastLastAssetIndex++};
  asset_source* Source = Assets->AssetSources + Result.Value;
  hha_asset* HHA = Assets->Assets + Result.Value;
  HHA->FirstTagIndex = Assets->TagCount;
  HHA->OnePastLastTagIndex = HHA->FirstTagIndex;
  HHA->Bitmap.AlignPercentage[0] = AlignPercentageX;
  HHA->Bitmap.AlignPercentage[1] = AlignPercentageY;

  Source->Type = AssetType_Font;
  Source->Filename = FontFile;
  Source->Codepoint = Codepoint;
  Source->Fontname = Fontname;

  Assets->AssetIndex = Result.Value;
  
  return(Result);
}

internal sound_id
AddSoundAsset(assets* Assets, char* Filename, u32 FirstSampleIndex, u32 SampleCount)
{
  Assert(Assets->DEBUGAssetType);
  Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

  sound_id Result = {Assets->DEBUGAssetType->OnePastLastAssetIndex++};
  asset_source* Source = Assets->AssetSources + Result.Value;
  hha_asset* HHA = Assets->Assets + Result.Value;
  HHA->FirstTagIndex = Assets->TagCount;
  HHA->OnePastLastTagIndex = HHA->FirstTagIndex;
  HHA->Sound.SampleCount = SampleCount;
  HHA->Sound.Chain = HHASoundChain_None;
  
  Source->Type = AssetType_Sound;
  Source->Filename = Filename;
  Source->FirstSampleIndex = FirstSampleIndex;

  Assets->AssetIndex = Result.Value;
    
  return(Result);
}

internal void
AddTag(assets* Assets, asset_tag_id ID, r32 Value)
{
  Assert(Assets->AssetIndex);

  hha_asset* HHA = Assets->Assets + Assets->AssetIndex;
  ++HHA->OnePastLastTagIndex;
  hha_tag* Tag = Assets->Tags + Assets->TagCount++;

  Tag->ID = ID;
  Tag->Value = Value;
}

internal void
EndAssetType(assets* Assets)
{
  Assert(Assets->DEBUGAssetType);
  Assets->AssetCount = Assets->DEBUGAssetType->OnePastLastAssetIndex;
  Assets->DEBUGAssetType = 0;
  Assets->AssetIndex = 0;
}

internal void
WriteHHA(assets* Assets, char* Filename)
{
  FILE* Out = fopen(Filename, "wb");
  if(Out)
    {
      hha_header Header = {};
      Header.MagicValue = HHA_MAGIC_VALUE;
      Header.Version = HHA_VERSION;
      Header.TagCount = Assets->TagCount;
      Header.AssetTypeCount = Asset_Count;
      Header.AssetCount = Assets->AssetCount;
      
      u32 TagArraySize = Header.TagCount*sizeof(hha_tag);
      u32 AssetTypesArraySize = Header.AssetTypeCount*sizeof(hha_asset_type);
      u32 AssetsArraySize = Header.AssetCount*sizeof(hha_asset);
      
      Header.Tags = sizeof(Header);
      Header.AssetTypes = Header.Tags + TagArraySize;
      Header.Assets = Header.AssetTypes + AssetTypesArraySize;

      fwrite(&Header, sizeof(Header), 1, Out);
      fwrite(Assets->Tags, TagArraySize, 1, Out);
      fwrite(Assets->AssetTypes, AssetTypesArraySize, 1, Out);
      fseek(Out, AssetsArraySize, SEEK_CUR);
      for(u32 AssetIndex = 1;
	  AssetIndex < Header.AssetCount;
	  ++AssetIndex)
	{
	  asset_source *Source = Assets->AssetSources + AssetIndex;
	  hha_asset *Dest = Assets->Assets + AssetIndex;

	  Dest->DataOffset = ftell(Out);
	  if(Source->Type == AssetType_Sound)
	    {
	      loaded_sound WAV = LoadWAV(Source->Filename,
					 Source->FirstSampleIndex,
					 Dest->Sound.SampleCount);
	      
	      Dest->Sound.SampleCount  = WAV.SampleCount;
	      Dest->Sound.ChannelCount = WAV.ChannelCount;
	      
	      for(u32 ChannelIndex = 0;
		  ChannelIndex < WAV.ChannelCount;
		  ++ChannelIndex)
		{
		  fwrite(WAV.Samples[ChannelIndex], Dest->Sound.SampleCount*sizeof(s16), 1, Out);
		}
	      	      
	      free(WAV.Free);
	    }
	  else
	    {
	      loaded_bitmap Bitmap;
	      if(Source->Type == AssetType_Font)
		{
		  Bitmap = LoadGlyphBitmap(Source->Filename, Source->Fontname, Source->Codepoint);
		}
	      else
		{
		  Assert(Source->Type == AssetType_Bitmap);
		  Bitmap = LoadBMP(Source->Filename);
		}
	      
	      Dest->Bitmap.Dim[0] = Bitmap.Width;
	      Dest->Bitmap.Dim[1] = Bitmap.Height;

	      Assert((Bitmap.Width*4) == Bitmap.Pitch);
	      fwrite(Bitmap.Memory, Bitmap.Width*Bitmap.Height*4, 1, Out);
	      
	      free(Bitmap.Free);
	    }
	}
      
      fseek(Out, (u32)Header.Assets, SEEK_SET);
      fwrite(Assets->Assets, AssetsArraySize, 1, Out);
      
      fclose(Out);
    }
  else
    {
      printf("Error: Couldn't open the file!");
    }  
}

internal void
Initialize(assets* Assets)
{
  Assets->TagCount = 1;
  Assets->AssetCount = 1;
  Assets->DEBUGAssetType = 0;
  Assets->AssetIndex = 0;

  Assets->AssetTypeCount = Asset_Count;
  memset( Assets->AssetTypes, 0, sizeof(Assets->AssetTypes));  
}

internal void
WriteHero()
{
  assets Assets_;
  assets* Assets = &Assets_;

  Initialize(Assets);
  
  r32 Angle_Right = 0.0f*Tau32;
  r32 Angle_Back = 0.25f*Tau32;
  r32 Angle_Left = 0.5f*Tau32;
  r32 Angle_Front = 0.75f*Tau32;

  r32 HeroAlignX = 0.5f;
  r32 HeroAlignY = 0.156682029f;
  
  BeginAssetType(Assets, Asset_Head);
  AddBitmapAsset(Assets, "../data/test/test_hero_right_head.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Right);
  AddBitmapAsset(Assets, "../data/test/test_hero_back_head.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Back);
  AddBitmapAsset(Assets, "../data/test/test_hero_left_head.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Left);
  AddBitmapAsset(Assets, "../data/test/test_hero_front_head.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Front);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Cape);
  AddBitmapAsset(Assets, "../data/test/test_hero_right_cape.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, 0.0f);
  AddBitmapAsset(Assets, "../data/test/test_hero_back_cape.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Back);
  AddBitmapAsset(Assets, "../data/test/test_hero_left_cape.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Left);
  AddBitmapAsset(Assets, "../data/test/test_hero_front_cape.bmp",HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Front);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Torso);
  AddBitmapAsset(Assets, "../data/test/test_hero_right_torso.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, 0.0f);
  AddBitmapAsset(Assets, "../data/test/test_hero_back_torso.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Back);
  AddBitmapAsset(Assets, "../data/test/test_hero_left_torso.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Left);
  AddBitmapAsset(Assets, "../data/test/test_hero_front_torso.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Front);
  EndAssetType(Assets);  

  WriteHHA(Assets, "test1.hha");
}

internal void
WriteNonHero()
{
  assets Assets_;
  assets* Assets = &Assets_;

  Initialize(Assets);
    
  BeginAssetType(Assets, Asset_Shadow);
  AddBitmapAsset(Assets, "../data/test/test_hero_shadow.bmp",
		 0.5f, 0.156602029f);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Tree);
  AddBitmapAsset(Assets, "../data/test2/tree00.bmp",
		 0.493827164f, 0.295652182f);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Sword);
  AddBitmapAsset(Assets, "../data/test2/rock03.bmp",
		 0.5f, 0.65625f);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Grass);
  AddBitmapAsset(Assets, "../data/test2/grass00.bmp",
		 0.5f, 0.5f);
  AddBitmapAsset(Assets, "../data/test2/grass01.bmp",
		 0.5f, 0.5f);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Stone);
  AddBitmapAsset(Assets, "../data/test2/ground00.bmp", 0.5f, 0.5f);
  AddBitmapAsset(Assets, "../data/test2/ground01.bmp", 0.5f, 0.5f);
  AddBitmapAsset(Assets, "../data/test2/ground02.bmp", 0.5f, 0.5f);
  AddBitmapAsset(Assets, "../data/test2/ground03.bmp", 0.5f, 0.5f);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Tuft);
  AddBitmapAsset(Assets, "../data/test2/tuft00.bmp", 0.5f, 0.5f);
  AddBitmapAsset(Assets, "../data/test2/tuft01.bmp", 0.5f, 0.5f);
  AddBitmapAsset(Assets, "../data/test2/tuft02.bmp", 0.5f, 0.5f);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Bloop);
  AddSoundAsset(Assets, "../data/test3/bloop_00.wav", 0, 0);
  AddSoundAsset(Assets, "../data/test3/bloop_01.wav", 0, 0);
  AddSoundAsset(Assets, "../data/test3/bloop_02.wav", 0, 0);
  AddSoundAsset(Assets, "../data/test3/bloop_03.wav", 0, 0);
  AddSoundAsset(Assets, "../data/test3/bloop_04.wav", 0, 0);
  EndAssetType(Assets);
  
  BeginAssetType(Assets, Asset_Font);
  for(u32 Character = 'A';
      Character <= 'Z';
      ++Character)
    {
      AddCharacterAsset(Assets, "/usr/share/fonts/TTF/LiberationSans-Regular.ttf", "-misc-liberation serif-medium-r-normal--0-0-0-0-p-0-ascii-0", Character, 0.5f, 0.5f);
      //AddCharacterAsset(Assets, "/usr/share/fonts/TTF/LiberationSans-Regular.ttf", "-adobe-courier-medium-o-normal--0-0-75-75-m-0-iso8859-1", Character, 0.5f, 0.5f);
      AddTag(Assets, Tag_UnicodeCodepoint, (r32)Character);
    }
  EndAssetType(Assets);

  WriteHHA(Assets, "test2.hha");
}

internal void
WriteSounds()
{
  assets Assets_;
  assets* Assets = &Assets_;

  Initialize(Assets);
    
  BeginAssetType(Assets, Asset_Crack);
  AddSoundAsset(Assets, "../data/test3/crack_00.wav", 0, 0);
  EndAssetType(Assets);
  
  BeginAssetType(Assets, Asset_Drop);
  AddSoundAsset(Assets, "../data/test3/drop_00.wav", 0, 0);
  EndAssetType(Assets);
  
  BeginAssetType(Assets, Asset_Glide);
  AddSoundAsset(Assets, "../data/test3/glide_00.wav", 0, 0);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Music);
  u32 OneMusicChunk = 10*48000;
  u32 TotalMusicSampleCount = 7468095;
  for(u32 FirstSampleIndex = 0;
      FirstSampleIndex < TotalMusicSampleCount;
      FirstSampleIndex += OneMusicChunk)
    {
      u32 SampleCount = TotalMusicSampleCount - FirstSampleIndex;
      if(SampleCount > OneMusicChunk)
	{
	  SampleCount = OneMusicChunk;
	}
      sound_id ThisMusic = AddSoundAsset(Assets, "../data/test3/music_test.wav", FirstSampleIndex, SampleCount);
      if((FirstSampleIndex + OneMusicChunk) < TotalMusicSampleCount)
	{
	  Assets->Assets[ThisMusic.Value].Sound.Chain = HHASoundChain_Advance;
	}
    }
  EndAssetType(Assets);
    
  BeginAssetType(Assets, Asset_Puhp);
  AddSoundAsset(Assets, "../data/test3/puhp_00.wav", 0, 0);
  AddSoundAsset(Assets, "../data/test3/puhp_01.wav", 0, 0);
  EndAssetType(Assets);  

  WriteHHA(Assets, "test3.hha");
}

int
main(int ArgC, char** Args)
{
  WriteNonHero();
  WriteHero();
  WriteSounds();
}

