/*Personal TODO list:
 
- Investigate frame skippage, it seems Casey doesn't have it as often
- Controller input, there is code in the old engine which was taken 
  from SDL but never fully investigated for best results (also hotplugging
  was not really a thing but seemed easish to implement.
- Fullscreen support, not that much of a pain but wanted some time to
  learn about it because it's a lot of Xlib stuff that isn't well 
  documented
  http://www.tonyobryan.com/index.php?article=9 
- Day 46 was done laxly because of the lack of controller support
  but most things should be there. Q&A pointed out a rotate intrinsic
  which is not applied here. It was supposedly microcoded and not 
  a big deal anyway (especially since it is a debug thing we use it
  for), and finding ones for llvm/clang is a bit of a pain 
 */

#include "handmade.h"
#include "handmade_tile.c"
#include "handmade_random.h"

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h"

internal void
ClearBitmap(loaded_bitmap* Bitmap)
{
  if(Bitmap->Memory)
    {
      u32 TotalBitmapSize = Bitmap->Width*Bitmap->Height*4;
      ZeroSize(TotalBitmapSize, Bitmap->Memory);
    }
}

internal loaded_bitmap*
MakeEmptyBitmap(memory_arena* Arena, s32 Width, s32 Height, bool32 ClearToZero)
{
  loaded_bitmap* Result = PushStruct(Arena, loaded_bitmap);
  Result->Width  = Width;
  Result->Height = Height;
  Result->Pitch  = Width*4;

  s32 TotalBitmapSize = Width*Height*4;
  Result->Memory      = PushSize(Arena, TotalBitmapSize);

  if(ClearToZero)
  {
    ClearBitmap(Result);
  }
  
  return(Result); 
}

internal glyph_bitmap*
MakeEmptyGlyphBitmap(memory_arena* GlyphArena, memory_arena* BitmapArena,
		     s32 Width, s32 Height, s32 XOffset, s32 YOffset,
		     bool32 ClearToZero)
{
  glyph_bitmap* Result = PushStruct(GlyphArena, glyph_bitmap);
  Result->XOffset = XOffset;
  Result->YOffset = YOffset;    
    
  Result->Bitmap = PushStruct(BitmapArena, loaded_bitmap);
  Result->Bitmap->Width  = Width;
  Result->Bitmap->Height = Height;
  Result->Bitmap->Pitch  = Width*4;

  s32 TotalBitmapSize    = Width*Height*4;
  Result->Bitmap->Memory = PushSize(BitmapArena, TotalBitmapSize);

  if(ClearToZero)
  {
    ClearBitmap(Result->Bitmap);
  }
  
  return(Result); 
}
/*
internal void
InitFont(thread_context* Thread, font* FontState, memory_arena* BitmapArena, memory *Memory)
{
  loaded_file FontTTF = Memory->DEBUGPlatformReadEntireFile(Thread, "fonts/LiberationMono-Regular.ttf");

  stbtt_fontinfo Font;
  stbtt_InitFont(&Font, FontTTF.Contents, stbtt_GetFontOffsetForIndex(FontTTF.Contents, 0));

  r32 ScaleForPixelHeight = stbtt_ScaleForPixelHeight(&Font, 30);
  int    Ascent, Descent, LineGap;
  stbtt_GetFontVMetrics(&Font,
			&Ascent, &Descent, &LineGap);
  font_metrics* FontMetrics = &FontState->Metrics;
  FontMetrics->ScaleForPixelHeight = ScaleForPixelHeight;
  FontMetrics->Ascent  = ScaleForPixelHeight*Ascent;
  FontMetrics->Descent = ScaleForPixelHeight*Descent;
  FontMetrics->LineGap = ScaleForPixelHeight*LineGap;
  FontMetrics->VerticalAdvance = FontMetrics->Ascent -
    FontMetrics->Descent + FontMetrics->LineGap;
  for(char codePoint = '!'; codePoint <= 'z'; ++codePoint)
    {
      s32 BitmapWidth, BitmapHeight,
	  XOffset, YOffset;
      u8* STBCharacterBitmap = stbtt_GetCodepointBitmap(&Font, 0, ScaleForPixelHeight,
							codePoint, &BitmapWidth, &BitmapHeight, &XOffset, &YOffset);
      glyph_bitmap* Glyph = MakeEmptyGlyphBitmap(&FontState->GlyphArena, BitmapArena, BitmapWidth, BitmapHeight, XOffset, YOffset, true);

      u8* SourceRow = STBCharacterBitmap;
      u8* DestRow   = Glyph->Bitmap->Memory;
      for(int Y = 0;
	  Y < BitmapHeight;
	  ++Y)
	{
	  u8*  Source = SourceRow;
	  u32* Dest   = (u32*)DestRow;
	  for(int X = 0;
	      X < BitmapWidth;
	      ++X)
	    {
	      u8 Alpha = *Source;
	      *Dest = ((Alpha << 24)|
		       (Alpha << 16)|
		       (Alpha << 8) |
		       (Alpha << 0)); 	      
	      ++Dest;
	      ++Source;
	    }
	  SourceRow += BitmapWidth;
	  DestRow   += Glyph->Bitmap->Pitch;
	}

      stbtt_FreeBitmap(STBCharacterBitmap, 0);
  }
}
*/
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

internal loaded_bitmap
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile,
	     char* Filename)
{
  loaded_bitmap Result = {};

  loaded_file ReadResult = ReadEntireFile(Thread, Filename);
  if(ReadResult.ContentsSize != 0)
    {
      bitmap_header *Header = (bitmap_header*) ReadResult.Contents;
      Result.Memory = (u32*)((u8*)ReadResult.Contents + Header->BitmapOffset); 
      Result.Width  = Header->Width;
      Result.Height = Header->Height;
      Result.Pitch  = Result.Width;

      Assert(Header->Compression == 3);
      
      u32 RedMask   = Header->RedMask;
      u32 GreenMask = Header->GreenMask;
      u32 BlueMask  = Header->BlueMask;
      u32 AlphaMask = ~(RedMask | GreenMask | BlueMask);
      
      bit_scan_result RedShift   = FindLeastSignificantSetBit(RedMask);
      bit_scan_result GreenShift = FindLeastSignificantSetBit(GreenMask);
      bit_scan_result BlueShift  = FindLeastSignificantSetBit(BlueMask);
      bit_scan_result AlphaShift = FindLeastSignificantSetBit(AlphaMask);

      Assert(RedShift.Found);
      Assert(GreenShift.Found);
      Assert(BlueShift.Found);
      Assert(AlphaShift.Found);
      
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
	      *SourceDest++ = ((((Color >> AlphaShift.Index) & 0xFF) << 24) |
			       (((Color >> RedShift.Index)   & 0xFF) << 16) |
			       (((Color >> GreenShift.Index) & 0xFF) << 8)  |
			       (((Color >> BlueShift.Index)  & 0xFF) << 0));
			       
	    }
	}
    }  
  return(Result);
}

internal void
DrawBitmap(offscreen_buffer *Buffer, loaded_bitmap *Bitmap,
	   r32 RealX,  r32 RealY,
	   s32 AlignX, s32 AlignY)
{
  RealX -= (r32)AlignX;
  RealY -= (r32)AlignY;
  
  s32 MinX = RoundReal32ToInt32(RealX);
  s32 MinY = RoundReal32ToInt32(RealY);
  s32 MaxX = RoundReal32ToInt32(RealX + (r32)Bitmap->Width);
  s32 MaxY = RoundReal32ToInt32(RealY + (r32)Bitmap->Height);

  s32 SourceOffsetX = 0;
  if(MinX < 0)
    {
      SourceOffsetX = -MinX;
      MinX = 0;
    }
  if(MaxX > Buffer->Width)
    {
      MaxX = Buffer->Width;
    }
  s32 SourceOffsetY = 0;
  if(MinY < 0)
    {
      SourceOffsetY = -MinY;
      MinY = 0;
    }
  if(MaxY > Buffer->Height)
    {
      MaxY = Buffer->Height;
    }

  u32* SourceRow = Bitmap->Memory + Bitmap->Width*(Bitmap->Height-1);
  SourceRow     += -SourceOffsetY*Bitmap->Width + SourceOffsetX;
  u8* DestRow    = ((u8*)Buffer->BitmapMemory +
		    MinX*Buffer->BytesPerPixel +
		    MinY*Buffer->Pitch);
  for(int Y = MinY;
      Y < MaxY;
      ++Y)
    {
      u32 *Dest   = (u32*) DestRow;
      u32 *Source = SourceRow;
      for(int X = MinX;
	  X < MaxX;
	  ++X)
	{
	  r32 Alpha = ((*Source >> 24) & 0xff) / 255.0f;
	  r32 sChannel1 = ((*Source >> 16) & 0xff);
	  r32 sChannel2 = ((*Source >> 8) & 0xff);
	  r32 sChannel3 = ((*Source >> 0) & 0xff);

	  r32 dChannel1 = ((*Dest >> 16) & 0xff);
	  r32 dChannel2 = ((*Dest >> 8) & 0xff);
	  r32 dChannel3 = ((*Dest >> 0) & 0xff);

	  r32 Channel1 = (1.0f - Alpha)*dChannel1 + Alpha*sChannel1;
	  r32 Channel2 = (1.0f - Alpha)*dChannel2 + Alpha*sChannel2;
	  r32 Channel3 = (1.0f - Alpha)*dChannel3 + Alpha*sChannel3;

	  *Dest = (((u32)(Channel1 + 0.5f) << 16) |
		   ((u32)(Channel2 + 0.5f) << 8)  |
		   ((u32)(Channel3 + 0.5f) << 0));
	  
	  ++Dest;
	  ++Source;
	}
      SourceRow -= Bitmap->Pitch;
      DestRow   += Buffer->Pitch;
    }
}

internal void
DrawRectangle(offscreen_buffer *Buffer,
	      v2 vMin, v2 vMax,
	      r32 R, r32 G, r32 B)
{
  s32 MinX = RoundReal32ToInt32(vMin.X);
  s32 MinY = RoundReal32ToInt32(vMin.Y);
  s32 MaxX = RoundReal32ToInt32(vMax.X);
  s32 MaxY = RoundReal32ToInt32(vMax.Y);
  if(MinX < 0)
    {
      MinX = 0;
    }
  if(MaxX > Buffer->Width)
    {
      MaxX = Buffer->Width;
    }
  if(MinY < 0)
    {
      MinY = 0;
    }
  if(MaxY > Buffer->Height)
    {
      MaxY = Buffer->Height;
    }

  u32 Color = (u32) ((0xFF << 24) |
		     (RoundReal32ToUInt32(R * 255.0f) << 16) |
		     (RoundReal32ToUInt32(G * 255.0f) << 8)  |
		     (RoundReal32ToUInt32(B * 255.0f)));
  
  u8 *Row = Buffer->BitmapMemory
    + Buffer->Pitch * MinY
    + Buffer->BytesPerPixel * MinX;
  for(int Y = MinY; Y < MaxY; ++Y)
    {
      u32 *Pixel = (u32*) Row;
      for(int X = MinX; X < MaxX; ++X)
	{
	  *Pixel++ = Color;
	}
      Row += Buffer->Pitch; 
    }
}

internal inline entity*
GetEntity(state *State, u32 Index)
{
  entity *Entity = 0;
  
  if((Index > 0) && (Index < ArrayCount(State->Entities)))
    {
      Entity = &State->Entities[Index];
    }

  return(Entity);
}

internal void
InitializePlayer(state *State, r32 EntityIndex)
{
  entity *Entity = GetEntity(State, EntityIndex);

  Entity->Exists = true;
  Entity->P.AbsTileX = 1;
  Entity->P.AbsTileY = 3;
  Entity->P.Offset.X = 5.0f;
  Entity->P.Offset.Y = 5.0f;  
  Entity->Height = 1.4f;
  Entity->Width  = 0.75f * Entity->Height;

  if(!GetEntity(State, State->CameraFollowingEntityIndex))
    {
      State->CameraFollowingEntityIndex = EntityIndex;
    }
}

internal u32
AddEntity(state *State)
{
  u32 EntityIndex = State->EntityCount++;
  Assert(State->EntityCount < ArrayCount(State->Entities));
  entity *Entity = &State->Entities[EntityIndex];

  Entity->Exists = false;
  Entity->P.AbsTileX = 0;
  Entity->P.AbsTileY = 0;
  Entity->P.Offset.X = 0.0f;
  Entity->P.Offset.Y = 0.0f;
  Entity->Height = 0.0f;
  Entity->Width  = 0.0f;
    
  return(EntityIndex);
}

internal void
MovePlayer(state *State, entity *Entity, r32 dt, v2 ddPlayer)
{
  tile_map *TileMap = State->World->TileMap; 
  if((ddPlayer.X != 0) && (ddPlayer.Y != 0))
    {
      ddPlayer = VMulS(0.707106781187f, ddPlayer);
    }

  r32 PlayerSpeed = 50.0f;
  ddPlayer = VMulS(PlayerSpeed, ddPlayer);
  ddPlayer = VAdd(ddPlayer,
		  VMulS(-8.0f, Entity->dP));

  tile_map_position OldPlayerP = Entity->P;
  tile_map_position NewPlayerP = OldPlayerP;
  v2 PlayerDelta = VAdd(VMulS(1.5f ,
			      VMulS(Square(dt),
				    ddPlayer)),
			VMulS(dt, Entity->dP));
  NewPlayerP.Offset = VAdd(NewPlayerP.Offset, PlayerDelta);
  Entity->dP = VAdd(VMulS(dt, ddPlayer),
		    Entity->dP);
  NewPlayerP = RecanonicalizePosition(TileMap, NewPlayerP);
#if 1
  tile_map_position PlayerLeft = NewPlayerP;
  PlayerLeft.Offset.X -= 0.5f * Entity->Width;
  PlayerLeft = RecanonicalizePosition(TileMap, PlayerLeft);
 
  tile_map_position PlayerRight = NewPlayerP;
  PlayerRight.Offset.X += 0.5f * Entity->Width;
  PlayerRight = RecanonicalizePosition(TileMap, PlayerRight);

  bool32 Collided = false;
  tile_map_position ColP = {};
  if(!IsTileMapPointEmpty(TileMap, NewPlayerP))
    {
      Collided = true;
      ColP = NewPlayerP;
    }
  if(!IsTileMapPointEmpty(TileMap, PlayerLeft))
    {
      Collided = true;
      ColP = PlayerLeft;
    }
  if(!IsTileMapPointEmpty(TileMap, PlayerRight))
    {
      Collided = true;
      ColP = PlayerRight;
    }
  if(Collided)
    {
      v2 r = {};
      if(ColP.AbsTileX < Entity->P.AbsTileX)
	{
	  r = (v2){1, 0};
	}
      if(ColP.AbsTileX > Entity->P.AbsTileX)
	{
	  r = (v2){-1, 0};
	}
      if(ColP.AbsTileY < Entity->P.AbsTileY)
	{
	  r = (v2){0, 1};
	}
      if(ColP.AbsTileY > Entity->P.AbsTileY)
	{
	  r = (v2){0, -1};
	}

      Entity->dP = VSub(Entity->dP,
			VMulS(1 * Inner(Entity->dP, r),
			      r));
    }
  else
    { 
      Entity->P = NewPlayerP;
    }
#else
  u32 MinTileX = 0;
  u32 MinTileY = 0;
  u32 AbsTileZ = Entity->P.AbsTileZ;
  u32 OnePastMaxTileY = 0;
  u32 OnePastMaxTileX = 0;
  tile_map_position BestPlayerP = Entity->P;
  r32 BestDistanceSq = LegthSq(PlayerDelta);
  for(u32 AbsTileY = MinTileY;
      AbsTileY != OnePastMaxTileY;
      ++AbsTileY)
    {
      for(u32 AbsTileX = MinTileX;
	  AbsTileX != OnePastMaxTileX;
	  ++AbsTileX)
	{
	  tile_map_position TestTileP = CenteredTilePoint(AbsTileX, AbsTileY, AbsTileZ);
	  u32 TileValue = GetTileValueP(TileMap, TestTileP);
	  if(IsTileValueEmpty(TileValue))
	    {
	      v2 MinCorner = (v2){-0.5f*TileMap->TileSideInMeters,
				  -0.5f*TileMap->TileSideInMeters};
	      v2 MaxCorner = (v2){0.5f*TileMap->TileSideInMeters,
				  0.5f*TileMap->TileSideInMeters};

	      tile_map_difference RelNewPlayerP = Subtract(TileMap, &TestTileP, &NewPlayerP);
	      v2 TesP = ClosestPointInRectangle(MinCorner, MaxCorner, RelNewPlayerP);
	    }
	}
    }
#endif

  if(!AreOnSameTile(&OldPlayerP, &Entity->P))
    {
      u32 NewTileValue = GetTileValueP(TileMap, Entity->P);
	      
      if(NewTileValue == 3)
	{
	  ++Entity->P.AbsTileZ;
	}
      else if(NewTileValue == 4)
	{
	  --Entity->P.AbsTileZ;
	}
    }
  if((Entity->dP.X == 0) &&
     (Entity->dP.Y == 0))
    {
      
    }
  else if(AbsoluteValue(Entity->dP.X) > AbsoluteValue(Entity->dP.Y))
    {
      if(Entity->dP.X > 0)
	{
	  Entity->FacingDirection = 0;
	}
      else
	{
	  Entity->FacingDirection = 2;
	}
    }
  else 
    {
      if(Entity->dP.Y > 0)
	{
	  Entity->FacingDirection = 1;
	}
      else
	{
	  Entity->FacingDirection = 3;
	}
      
    }
}


extern UPDATE_AND_RENDER(UpdateAndRender)
{  
  state* State = (state*) Memory->PermanentStorage;
  if(!Memory->isInitialized)
    {
      AddEntity(State);

      State->Backdrop
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_background.bmp");

      hero_bitmaps *Bitmap = State->HeroBitmaps;
      
      Bitmap->Head
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_right_head.bmp");
      Bitmap->Cape
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_right_cape.bmp");
      Bitmap->Torso
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_right_torso.bmp");
      Bitmap->AlignX = 72;
      Bitmap->AlignY = 182;
      ++Bitmap;
      
      Bitmap->Head
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_back_head.bmp");
      Bitmap->Cape
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_back_cape.bmp");
      Bitmap->Torso
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_back_torso.bmp");
      Bitmap->AlignX = 72;
      Bitmap->AlignY = 182;
      ++Bitmap;
      
      Bitmap->Head
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_left_head.bmp");
      Bitmap->Cape
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_left_cape.bmp");
      Bitmap->Torso
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_left_torso.bmp");
      Bitmap->AlignX = 72;
      Bitmap->AlignY = 182;
      ++Bitmap;
      
      Bitmap->Head
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_front_head.bmp");
      Bitmap->Cape
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_front_cape.bmp");
      Bitmap->Torso
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_front_torso.bmp");
      Bitmap->AlignX = 72;
      Bitmap->AlignY = 182;
      
      //NOTE We will do this much differently once we have a system
      //     for the assets like fonts and bitmaps.
      /*InitializeArena(&State->BitmapArena, Megabytes(1),
		      (u8*)Memory->PermanentStorage + sizeof(state));
      InitializeArena(&State->Font.GlyphArena, Memory->PermanentStorageSize - sizeof(state),
		      (u8*)Memory->PermanentStorage + sizeof(state) + Megabytes(1));
      */
      InitializeArena(&State->WorldArena, Memory->PermanentStorageSize - sizeof(state),
		      (u8*)Memory->PermanentStorage + sizeof(state));

      State->CameraP.AbsTileX = 17/2;
      State->CameraP.AbsTileY = 9/2;
           
      State->World   = PushStruct(&State->WorldArena, world);
      world *World   = State->World;
      World->TileMap = PushStruct(&State->WorldArena, tile_map);
      
      tile_map *TileMap = World->TileMap;

      TileMap->ChunkShift = 4;
      TileMap->ChunkMask  = (1 << TileMap->ChunkShift) - 1;  
      TileMap->ChunkDim   = (1 << TileMap->ChunkShift);

      TileMap->TileChunkCountX = 128;
      TileMap->TileChunkCountY = 128;
      TileMap->TileChunkCountZ = 2;

      TileMap->TileChunks = PushArray(&State->WorldArena,
				      TileMap->TileChunkCountX *
				      TileMap->TileChunkCountY *
				      TileMap->TileChunkCountZ,
				      tile_chunk);
	
      TileMap->TileSideInMeters = 1.4f;

      u32 RandomNumberIndex = 0;
	        
      u32 TilesPerWidth = 17;
      u32 TilesPerHeight = 9;
      u32 ScreenX = 0;
      u32 ScreenY = 0;
      u32 AbsTileZ = 0;
      
      bool32 DoorLeft   = false;
      bool32 DoorRight  = false;
      bool32 DoorTop    = false;
      bool32 DoorBottom = false;
      bool32 DoorUp     = false;
      bool32 DoorDown   = false;
	    
      for(s32 ScreenIndex = 0;
	  ScreenIndex < 100;
	  ++ScreenIndex)
	{
	  Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
	  u32 RandomChoice;
	  if(DoorUp || DoorDown)
	    {
	      RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
	    }
	  else
	    {
	      RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
	    }
	  
	  bool32 CreatedZDoor = false;
	  if(RandomChoice == 2)
	    {
	      CreatedZDoor = true;
	      if(AbsTileZ == 0)
		{
		  DoorUp = true;
		}
	      else
		{
		  DoorDown = true;
		}
	    }
	  else if (RandomChoice == 1)
	    {
	      DoorRight = true;
	    }
	  else
	    {
	      DoorTop = true;
	    }
	  
	  for(u32 TileY = 0;
	      TileY < TilesPerHeight;
	      ++TileY)
	    {
	      for(u32 TileX = 0;
		  TileX < TilesPerWidth;
		  ++TileX)
		{
		  u32 AbsTileX = ScreenX * TilesPerWidth  + TileX;
		  u32 AbsTileY = ScreenY * TilesPerHeight + TileY;

		  u32 TileValue = 1;
		  if((TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight/2))))
		    { 
		      TileValue = 2;
		    }		  
		  if((TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != (TilesPerHeight/2))))
		    {
		      TileValue = 2;
		    }
		  if((TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth/2))))
		    {
		      TileValue = 2;
		    }
		  if((TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth/2))))
		    {
		      TileValue = 2;
		    }
		  if((TileX == 10) && (TileY == 6))
		    {
		      if(DoorUp)
			{
			  TileValue = 3;
			}
		      if(DoorDown)
			{
			  TileValue = 4;
			}
		    }
		  
		  SetTileValue(&State->WorldArena, TileMap,
			       AbsTileX, AbsTileY, AbsTileZ,
			       TileValue);
		}
	    }

	  DoorLeft = DoorRight;
	  DoorBottom = DoorTop;

	  if(CreatedZDoor)
	    {
	      DoorDown = !DoorDown;
	      DoorUp   = !DoorUp;
	    }
	  else
	    {
	      DoorUp   = false;
	      DoorDown = false;
	    }
	  
	  DoorRight = false;
	  DoorTop   = false;
	  	  
	  if(RandomChoice == 2)
	    {
	      if(AbsTileZ == 0)
		{
		  AbsTileZ = 1;
		}
	      else
		{
		  AbsTileZ = 0;
		}
	    }
	  else if(RandomChoice == 1)
	    {
	      ScreenX += 1;
	    }
	  else
	    {
	      ScreenY += 1;
	    }
	}

      Memory->isInitialized = true;
    }

  world *World      = State->World;
  tile_map *TileMap = World->TileMap;

  s32 TileSideInPixels = 60;
  r32 MetersToPixels = (r32)TileSideInPixels / (r32)TileMap->TileSideInMeters;

  for(int ControllerIndex = 0;
      ControllerIndex < ArrayCount(Input->Controllers);
      ++ControllerIndex)
    {
      controller_input *Controller = GetController(Input, ControllerIndex);
      entity *ControllingEntity = GetEntity(State,
					    State->PlayerIndexForController[ControllerIndex]);
      if(ControllingEntity)
	{
	  v2 ddPlayer = {};
	  if(Controller->IsAnalog)
	    {
	      ddPlayer = (v2){Controller->StickAverageX,
			      Controller->StickAverageY};
	    }
	  else
	    {
	      if(Controller->MoveUp.EndedDown)
		{
		  ddPlayer.Y = 1.0f;
		}
	      if(Controller->MoveDown.EndedDown)
		{
		  ddPlayer.Y = -1.0f;
		}
	      if(Controller->MoveLeft.EndedDown)
		{
		  ddPlayer.X = -1.0f;
		}
	      if(Controller->MoveRight.EndedDown)
		{
		  ddPlayer.X = 1.0f;
		}	  
	    }
	  
	  MovePlayer(State, ControllingEntity,
		     Input->dtForFrame, ddPlayer);
	}
      else
	{
	  if(Controller->Start.EndedDown)
	    {
	      u32 EntityIndex = AddEntity(State);
	      InitializePlayer(State, EntityIndex);
	      State->PlayerIndexForController[ControllerIndex] = EntityIndex;
	    }
	}
    }

  entity *CameraFollowingEntity
    = GetEntity(State, State->CameraFollowingEntityIndex);
  if(CameraFollowingEntity)
    {
      State->CameraP.AbsTileZ = CameraFollowingEntity->P.AbsTileZ;

      tile_map_difference Diff = Subtract(TileMap, &CameraFollowingEntity->P, &State->CameraP);
      if(Diff.dXY.X > (9.0f * TileMap->TileSideInMeters))
	{
	  State->CameraP.AbsTileX += 17;
	}
      if(Diff.dXY.X < -(9.0f * TileMap->TileSideInMeters))
	{
	  State->CameraP.AbsTileX -= 17;
	}
      if(Diff.dXY.Y > (5.0f * TileMap->TileSideInMeters))
	{
	  State->CameraP.AbsTileY += 9;
	}
      if(Diff.dXY.Y < -(5.0f * TileMap->TileSideInMeters))
	{
	  State->CameraP.AbsTileY -= 9;
	}
    }

  DrawBitmap(Buffer, &State->Backdrop, 0, 0, 0, 0);
  
  r32 ScreenCenterX = 0.5f * Buffer->Width;
  r32 ScreenCenterY = 0.5f * Buffer->Height;
  for(s32 RelRow = -10;
      RelRow < 10;
      ++RelRow)
    {
      for(s32 RelColumn = -20;
	  RelColumn < 20;
	  ++RelColumn)
	{
	  u32 Column = State->CameraP.AbsTileX + RelColumn;
	  u32 Row    = State->CameraP.AbsTileY + RelRow;
	  u32 TileID = GetTileValueC(TileMap, Column, Row, State->CameraP.AbsTileZ);
	  
	  if(TileID > 1)
	    {
	      r32 Grey = 0.5f;
	      if(TileID == 2)
		{
		  Grey = 1.0f;
		}
	      if(TileID > 2)
		{
		  Grey = 0.25f;
		}	     

	      v2 TileSide = {0.5f * TileSideInPixels,
			     0.5f * TileSideInPixels};
	      v2 Center   = {ScreenCenterX - MetersToPixels*State->CameraP.Offset.X + ((r32)RelColumn * TileSideInPixels),
			     ScreenCenterY + MetersToPixels*State->CameraP.Offset.Y - ((r32)RelRow * TileSideInPixels)};
	      v2 Min = VSub(Center, TileSide);
	      v2 Max = VAdd(Center, TileSide);
	      
	      DrawRectangle(Buffer,
			    Min, Max,
			    Grey, Grey, Grey);
	    }
	}
    }

  entity *Entity = State->Entities; 
  for(u32 EntityIndex = 0;
      EntityIndex < State->EntityCount;
      ++EntityIndex, ++Entity)
    {
      if(Entity->Exists)
	{
	  tile_map_difference Diff = Subtract(TileMap, &Entity->P, &State->CameraP);
    
	  r32 PlayerR = 1.0f;
	  r32 PlayerG = 1.0f;
	  r32 PlayerB = 0.0f;
	  r32 PlayerGroundPointX = ScreenCenterX + MetersToPixels*Diff.dXY.X;
	  r32 PlayerGroundPointY = ScreenCenterY - MetersToPixels*Diff.dXY.Y;
	  v2 PlayerLeftTop = {PlayerGroundPointX - 0.5f * MetersToPixels*Entity->Width,
			      PlayerGroundPointY - MetersToPixels*Entity->Height};
	  v2 PlayerWidthHeight = {Entity->Width, Entity->Height};
	  DrawRectangle(Buffer,
			PlayerLeftTop,
			VAdd(PlayerLeftTop,
			     VMulS(MetersToPixels,
				   PlayerWidthHeight)),
			PlayerR, PlayerG, PlayerB);
  
	  hero_bitmaps *Hero = &State->HeroBitmaps[Entity->FacingDirection];
	  DrawBitmap(Buffer, &Hero->Torso, PlayerGroundPointX, PlayerGroundPointY, Hero->AlignX, Hero->AlignY);
	  DrawBitmap(Buffer, &Hero->Cape,  PlayerGroundPointX, PlayerGroundPointY, Hero->AlignX, Hero->AlignY);
	  DrawBitmap(Buffer, &Hero->Head,  PlayerGroundPointX, PlayerGroundPointY, Hero->AlignX, Hero->AlignY);
	}
    }
}
/*

internal void
RenderWeirdGradient(offscreen_buffer* Buffer, memory_arena* Arena,
		    u32 BlueOffset, u32 GreenOffset)
{
  u8* Row = (u8*)Buffer->BitmapMemory;
  for(int Y = 0;
      Y < Buffer->Height;
      ++Y)
    {
      u32 *Pixel = (u32*)Row;
      for (int X = 0; 
	   X < Buffer->Width;
	   ++X)
	{
	  u8 Blue  = X + BlueOffset;
	  u8 Green = Y + GreenOffset;
	  *Pixel++ = ((Green << 8) | Blue);
	}
      Row += Buffer->Pitch;
    }
}

internal void
DrawGlyph(offscreen_buffer* Buffer, glyph_bitmap* Glyph,
	   s32 X, s32 Y)
{
  DrawBitmap(Buffer, Glyph->Bitmap,
	     X + Glyph->XOffset,
	     Y + Glyph->YOffset);
}

internal void
DebugRenderTextLine()
{
  
}

#if 0
internal bool32
Button(s32 ID, char* Label, v2 Position, guiContext* GUIContext)
{
  if(GUIContext->ActiveItem == ID)
    {
      inputState currentInputState = GUIContext->InputState;
      if(MouseWentUp)
	{
	  if(GUIContext->HotItem == ID)
	    {
	      return true;
	    }
	  GUIContext->ActiveItem = 0;
	}
    }
  else
    {
      if(MouseWentDown)
	{
	  GUIContext->ActiveItem = ID;
	}
    }
  
  if(Inside)
    {
      GUIContext->HotItem = ID;
    }
  
}

internal void
prepareUIFrame()
{
  guiContext GUIContext;
  InitGui(&GUIContext);
  
  if(Button(ID, Label, Position, &GUIContext))
    {
      //TODO
    }
  OutputList(Position, char** Strings, s32 numStrings);
}
#endif

*/
