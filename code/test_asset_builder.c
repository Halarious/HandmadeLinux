#include "test_asset_builder.h"

#define USE_FONTS_FROM_LINUX 1
#if USE_FONTS_FROM_LINUX
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define MAX_FONT_WIDTH  1024
#define MAX_FONT_HEIGHT 1024

global_variable Display* OpenDisplay = 0;
global_variable Pixmap   FontPixmap;
global_variable int      DefaultScreen;
global_variable GC       DefaultGC;

#define USE_FTLIB 1
#if USE_FTLIB
#undef internal
#include <X11/Xft/Xft.h>
#include <freetype/freetype.h>
#include <freetype/fttypes.h>
#endif
#define internal static

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

struct loaded_font
{
  XftFont* Linux32Handle;
  FT_Face Metrics;
  u32 CodePointCount;
  r32 LineAdvance;

  bitmap_id* BitmapIDs;
  r32* HorizontalAdvance;
};

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

internal loaded_font*
LoadFont(char* FileName, char* FontName, u32 CodePointCount)
{
  loaded_font* Font = (loaded_font*) malloc(sizeof(loaded_font));

  Font->Linux32Handle = XftFontOpen(OpenDisplay,
				      DefaultScreen(OpenDisplay),
				      XFT_FAMILY,     XftTypeString, "Liberation Sans",
				      XFT_PIXEL_SIZE, XftTypeDouble, 64.0,
				      XFT_ANTIALIAS,  XftTypeBool, true,
				      NULL);

  Font->Metrics = XftLockFace(Font->Linux32Handle);

  //NOTE: This gives us the same result as the height in the XftFont struct (but in 26.6 pixel format(?))
  //u32 LineAdvance = FT_MulFix(Font->Metrics->height, Font->Metrics->size->metrics.x_scale ); 
  
  Font->LineAdvance = (r32)Font->Linux32Handle->height;
  Font->CodePointCount = CodePointCount;
  Font->BitmapIDs = (bitmap_id*) malloc(sizeof(bitmap_id) * CodePointCount);
  Font->HorizontalAdvance = (r32*) malloc(sizeof(r32) * CodePointCount * CodePointCount);
  
  for(u32 CodePointIndex = 0;
      CodePointIndex < Font->CodePointCount;
      ++CodePointIndex)
    {
#if USE_FTLIB
      FT_UInt CodePoint = XftCharIndex(OpenDisplay, Font->Linux32Handle, CodePointIndex);
      
      XGlyphInfo XftCharInfo = {};
      XftGlyphExtents(OpenDisplay, Font->Linux32Handle, &CodePoint, 1, &XftCharInfo);
#else
#endif
      for(u32 OtherCodePointIndex = 0;
	  OtherCodePointIndex < Font->CodePointCount;
	  ++OtherCodePointIndex)
	{
	  Font->HorizontalAdvance[CodePointIndex*Font->CodePointCount + OtherCodePointIndex] = (r32)XftCharInfo.xOff;
	}
    }
  
  for(u32 CodePointIndex = 0;
      CodePointIndex < Font->CodePointCount;
      ++CodePointIndex)
    {
      FT_UInt CodePoint = XftCharIndex(OpenDisplay, Font->Linux32Handle, CodePointIndex);
      for(u32 OtherCodePointIndex = 0;
	  OtherCodePointIndex < Font->CodePointCount;
	  ++OtherCodePointIndex)
	{
	  FT_UInt OtherCodePoint = XftCharIndex(OpenDisplay, Font->Linux32Handle, OtherCodePointIndex);
#if 0
	  Font->HorizontalAdvance[CodePointIndex*Font->CodePointCount + OtherCodePointIndex] = (r32)Font->Linux32Handle->max_advance_width;
#else
	  FT_Vector akerning = {};
	  FT_Get_Kerning(Font->Metrics,
			 (FT_UInt)CodePoint,
			 (FT_UInt)OtherCodePoint,
			 FT_KERNING_DEFAULT,
			 &akerning );
	  Font->HorizontalAdvance[CodePointIndex*Font->CodePointCount + OtherCodePointIndex] += (r32)(akerning.x / 64.0f);
#endif
	}
    }
  
  return(Font);
}

internal void
FreeFont(loaded_font* Font)
{
  XftUnlockFace(Font->Linux32Handle);
  free(Font);
  //TODO: Implement this for both paths 

}

internal void
InitializeFontContext()
{
  OpenDisplay = XOpenDisplay(0);

  DefaultScreen = DefaultScreen(OpenDisplay);
  DefaultGC     = DefaultGC(OpenDisplay, DefaultScreen);
      
  FontPixmap = XCreatePixmap(OpenDisplay,
			     DefaultRootWindow(OpenDisplay),
			     MAX_FONT_WIDTH,
			     MAX_FONT_HEIGHT,
			     DefaultDepth(OpenDisplay, DefaultScreen));

  XSetBackground(OpenDisplay, DefaultGC, BlackPixel(OpenDisplay, DefaultScreen));
}

internal loaded_bitmap
LoadGlyphBitmap(loaded_font* Font, u32 CodePoint,
		hha_asset* Asset)
{
  loaded_bitmap Result = {};

#if USE_FONTS_FROM_LINUX

  int FudgeX = 100;
  int FudgeY = 100;

  wchar_t CheesePoint  = (wchar_t) CodePoint;

#if USE_FTLIB
  
  XftDraw* Bitmap = XftDrawCreate(OpenDisplay, FontPixmap,
				  DefaultVisual(OpenDisplay, DefaultScreen),
				  DefaultColormap(OpenDisplay, DefaultScreen));
  
  XftChar16 XftCheesePoint = (XftChar16) CodePoint;

  XGlyphInfo XftCharInfo;
  XftTextExtents16(OpenDisplay, Font->Linux32Handle, &XftCheesePoint, 1, &XftCharInfo);

  int BoundWidth  = XftCharInfo.width  + FudgeX;
  int BoundHeight = XftCharInfo.height + FudgeY;
  
  XftColor XBackgroundColor = {BlackPixel(OpenDisplay, DefaultScreen), {0x0, 0x0, 0x0, 0x0} };
  XftColor XForegroundColor = {WhitePixel(OpenDisplay, DefaultScreen), {0xffff, 0xffff, 0xffff, 0xffff} };
  
  XftDrawRect    (Bitmap, &XBackgroundColor,
		  0, 0,
		  MAX_FONT_WIDTH, MAX_FONT_HEIGHT);
  XftDrawString16(Bitmap, &XForegroundColor, Font->Linux32Handle,
		  FudgeX + XftCharInfo.x,
		  FudgeY + XftCharInfo.y,
		  &XftCheesePoint, 1);

  XftDrawDestroy(Bitmap);
#else    
  XFontStruct* FontStruct = XLoadQueryFont(Display, Fontname);
  XSetFont(Display, GraphicsContext, FontStruct->fid);

  XChar2b XCheesePoint = {(u8)(CodePoint << 8), (u8) CodePoint };
  
  int Dir, Asc, Dsc;
  XCharStruct CharInfo;
  XTextExtents16(FontStruct, &XCheesePoint, 1,
		 &Dir,
		 &Asc,
		 &Dsc,
		 &CharInfo);
  
  int Height = CharInfo.ascent   + CharInfo.descent + 1;
  int Width  = CharInfo.rbearing - CharInfo.lbearing + 1;
  
  XSetForeground(Display, GraphicsContext, BlackPixel(Display, Screen));  
  XFillRectangle(Display, Pixmap, GraphicsContext, FudgeX, FudgeY, Width, Height);

  XSetForeground(Display, GraphicsContext, WhitePixel(Display, Screen));
  XDrawImageString16(Display, Pixmap, GraphicsContext,
		     -CharInfo.lbearing            + FudgeX,
		     Asc - (Asc - CharInfo.ascent) + FudgeY + 1,
		     &XCheesePoint, 1);

  XUnloadFont(Display, FontStruct->fid);

#endif
  
  XImage* GlyphMemory = XGetImage(OpenDisplay, FontPixmap,
				  0,
				  0,
				  BoundWidth,
				  BoundHeight,
				  AllPlanes,
				  ZPixmap);

  s32 MinX = 10000;
  s32 MinY = 10000;
  s32 MaxX = -10000;
  s32 MaxY = -10000;
  for(s32 Y = 0;
      Y < BoundHeight;
      ++Y)
    {
      for(s32 X = 0;
	  X < BoundWidth;
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
    {
      int Width  = (MaxX - MinX) + 1;
      int Height = (MaxY - MinY) + 1;
      
      Result.Width  = Width + 2;
      Result.Height = Height + 2;
      Result.Pitch  = Result.Width * BITMAP_BYTES_PER_PIXEL;
      Result.Memory = malloc(Result.Height * Result.Pitch);
      Result.Free   = Result.Memory;

      memset(Result.Memory, 0, Result.Height * Result.Pitch);
      
      u8* DestRow = (u8*)Result.Memory + (Result.Height - 1 - 1)*Result.Pitch;
      for(s32 Y = MinY;
	  Y <= MaxY;
	  ++Y)
	{
	  u32* Dest = (u32*) DestRow;
	  for(s32 X = MinX;
	      X <= MaxX;
	      ++X)
	    {
	      unsigned long Pixel = XGetPixel(GlyphMemory, X, Y);

	      r32 Grey = (r32)(Pixel & 0xFF);
	      v4 Texel = V4(255.0f, 255.0f, 255.0f, Grey);

	      Texel = SRGB255ToLinear1(Texel);
	      Texel.rgb = V3MulS(Texel.a, Texel.rgb);
	      Texel = Linear1ToSRGB255(Texel);
	      
	      *Dest++ = (((u32)(Texel.a + 0.5f) << 24) |
			 ((u32)(Texel.r + 0.5f) << 16) |
			 ((u32)(Texel.g + 0.5f) << 8)  |
			 ((u32)(Texel.b + 0.5f) << 0));
	      
	    }
	  
	  DestRow -= Result.Pitch;
	}
    
      Asset->Bitmap.AlignPercentage[0] = (1.0f + XftCharInfo.x) / (r32)Result.Width;
      Asset->Bitmap.AlignPercentage[1] = (1.0f + (XftCharInfo.height - XftCharInfo.y)) / (r32)Result.Height;
      //Asset->Bitmap.AlignPercentage[1] = (1.0f + (MaxY - (BoundHeight - Font->Linux32Handle->descent))) / (r32)Result.Height;

      u32 Break = 0;
    }

#else  

  entire_file TTFFile = ReadEntireFile(Filename);
  if(TTFFile.ContentsSize != 0)
    {
      stbtt_fontinfo Font;
      stbtt_InitFont(&Font, (u8*)TTFFile.Contents, stbtt_GetFontOffsetForIndex((u8*)TTFFile.Contents, 0));
      
      int Width, Height, XOffset, YOffset;
      u8* MonoBitmap = stbtt_GetCodepointBitmap(&Font, 0, stbtt_ScaleForPixelHeight(&Font, 64.0f),
						CodePoint, &Width, &Height, &XOffset, &YOffset);

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

typedef struct
{
  u32 ID;
  hha_asset* HHA;
  asset_source* Source;
} added_asset;

internal added_asset
AddAsset(assets* Assets)
{
  Assert(Assets->DEBUGAssetType);
  Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

  u32 Index = Assets->DEBUGAssetType->OnePastLastAssetIndex++;
  asset_source* Source = Assets->AssetSources + Index;
  hha_asset* HHA = Assets->Assets + Index;
  HHA->FirstTagIndex = Assets->TagCount;
  HHA->OnePastLastTagIndex = HHA->FirstTagIndex;

  Assets->AssetIndex = Index;

  added_asset Result;
  Result.ID  = Index;
  Result.HHA = HHA;
  Result.Source = Source;
  return(Result);
}

internal bitmap_id
AddBitmapAsset(assets* Assets, char* Filename, r32 AlignPercentageX, r32 AlignPercentageY)
{
  added_asset Asset = AddAsset(Assets);

  Asset.HHA->Bitmap.AlignPercentage[0] = AlignPercentageX;
  Asset.HHA->Bitmap.AlignPercentage[1] = AlignPercentageY;

  Asset.Source->Type = AssetType_Bitmap;
  Asset.Source->Bitmap.Filename = Filename;

  bitmap_id Result = {Asset.ID};
  return(Result);
}

internal bitmap_id
AddCharacterAsset(assets* Assets, loaded_font* Font, u32 CodePoint, r32 AlignPercentageX, r32 AlignPercentageY)
{
  added_asset Asset = AddAsset(Assets);

  Asset.HHA->Bitmap.AlignPercentage[0] = 0.0f;
  Asset.HHA->Bitmap.AlignPercentage[1] = 0.0f;

  Asset.Source->Type = AssetType_FontGlyph;
  Asset.Source->Glyph.Font = Font;
  Asset.Source->Glyph.CodePoint = CodePoint;

  bitmap_id Result = {Asset.ID};  
  return(Result);
}

internal sound_id
AddSoundAsset(assets* Assets, char* Filename, u32 FirstSampleIndex, u32 SampleCount)
{
  added_asset Asset = AddAsset(Assets);

  Asset.HHA->Sound.SampleCount = SampleCount;
  Asset.HHA->Sound.Chain = HHASoundChain_None;
  
  Asset.Source->Type = AssetType_Sound;
  Asset.Source->Sound.Filename = Filename;
  Asset.Source->Sound.FirstSampleIndex = FirstSampleIndex;

  sound_id Result = {Asset.ID};      
  return(Result);
}

internal font_id
AddFontAsset(assets* Assets, loaded_font* Font)
{
  added_asset Asset = AddAsset(Assets);

  Asset.HHA->Font.CodePointCount = Font->CodePointCount;
  Asset.HHA->Font.LineAdvance    = Font->LineAdvance;
  
  Asset.Source->Type = AssetType_Font;
  Asset.Source->Font.Font = Font;

  font_id Result = {Asset.ID};      
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
	      loaded_sound WAV = LoadWAV(Source->Sound.Filename,
					 Source->Sound.FirstSampleIndex,
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
	  else if(Source->Type == AssetType_Font)
	    {
	      loaded_font* Font = Source->Font.Font;
	      u32 HorizontalAdvanceSize = sizeof(r32) * Font->CodePointCount* Font->CodePointCount;
	      u32 CodePointsSize = Font->CodePointCount*sizeof(bitmap_id);

	      fwrite(Font->BitmapIDs, CodePointsSize, 1, Out);
	      fwrite(Font->HorizontalAdvance, HorizontalAdvanceSize, 1, Out);
	    }
	  else
	    {
	      loaded_bitmap Bitmap;
	      if(Source->Type == AssetType_FontGlyph)
		{
		  Bitmap = LoadGlyphBitmap(Source->Glyph.Font,
					   Source->Glyph.CodePoint,
					   Dest);
		}
	      else
		{
		  Assert(Source->Type == AssetType_Bitmap);
		  Bitmap = LoadBMP(Source->Bitmap.Filename);
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
WriteFonts()
{
  assets Assets_;
  assets* Assets = &Assets_;

  Initialize(Assets);

  loaded_font* DebugFont = LoadFont("/usr/share/fonts/TTF/LiberationSans-Regular.ttf", "-misc-liberation serif-medium-r-normal--0-0-0-0-p-0-ascii-0", ('~' + 1));
  
  BeginAssetType(Assets, Asset_Font);
  AddFontAsset(Assets, DebugFont);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_FontGlyph);
    for(u32 Character = '!';
      Character <= '~';
      ++Character)
    {
      DebugFont->BitmapIDs[Character] = AddCharacterAsset(Assets, DebugFont, Character, 0.5f, 0.5f);
      //AddCharacterAsset(Assets, "/usr/share/fonts/TTF/LiberationSans-Regular.ttf", "-adobe-courier-medium-o-normal--0-0-75-75-m-0-iso8859-1", Character, 0.5f, 0.5f);
      //AddCharacterAsset(Assets, "/usr/share/fonts/TTF/times.ttf", "-adobe-times-medium-r-normal--0-0-75-75-p-0-iso8859-1", Character, 0.5f, 0.5f);
    }
  EndAssetType(Assets);

  WriteHHA(Assets, "testfonts.hha");
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

  WriteHHA(Assets, "test2.hha");
}

internal void
WriteSounds()
{
  assets Assets_;
  assets* Assets = &Assets_;

  Initialize(Assets);
  
  BeginAssetType(Assets, Asset_Bloop);
  AddSoundAsset(Assets, "../data/test3/bloop_00.wav", 0, 0);
  AddSoundAsset(Assets, "../data/test3/bloop_01.wav", 0, 0);
  AddSoundAsset(Assets, "../data/test3/bloop_02.wav", 0, 0);
  AddSoundAsset(Assets, "../data/test3/bloop_03.wav", 0, 0);
  AddSoundAsset(Assets, "../data/test3/bloop_04.wav", 0, 0);
  EndAssetType(Assets);

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
  InitializeFontContext();

  WriteFonts();
  WriteNonHero();
  WriteHero();
  WriteSounds();
}

