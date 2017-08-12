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
      
      bit_scan_result RedScan   = FindLeastSignificantSetBit(RedMask);
      bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
      bit_scan_result BlueScan  = FindLeastSignificantSetBit(BlueMask);
      bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);

      Assert(RedScan.Found);
      Assert(GreenScan.Found);
      Assert(BlueScan.Found);
      Assert(AlphaScan.Found);

      s32 RedShift = 16 - (s32)RedScan.Index;
      s32 GreenShift = 8  - (s32)GreenScan.Index;
      s32 BlueShift = 0  - (s32)BlueScan.Index;
      s32 AlphaShift = 24 - (s32)AlphaScan.Index;
      
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

	      *SourceDest++ = (RotateLeft(Color & RedMask, RedShift) |
			       RotateLeft(Color & GreenMask, GreenShift) |
			       RotateLeft(Color & BlueMask, BlueShift) |
			       RotateLeft(Color & AlphaMask, AlphaShift));
	    }
	}
    }  
  return(Result);
}

internal void
DrawBitmap(offscreen_buffer *Buffer, loaded_bitmap *Bitmap,
	   r32 RealX,  r32 RealY,
	   s32 AlignX, s32 AlignY,
	   r32 CAlpha)
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
	  Alpha *= CAlpha;
	  
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

internal void
ChangeEntityResidence(state *State, u32 EntityIndex, entity_residence Residence)
{
  if(Residence == EntityResidence_High)
    {
      if(State->EntityResidence[EntityIndex] != EntityResidence_High)
	{
	  high_entity *HighEntity = &State->HighEntities[EntityIndex];
	  dormant_entity *DormantEntity = &State->DormantEntities[EntityIndex];
  
	  tile_map_difference Diff = Subtract(State->World->TileMap, &DormantEntity->P, &State->CameraP);
	  HighEntity->P = Diff.dXY;
	  HighEntity->dP = (v2){0, 0};
	  HighEntity->AbsTileZ = DormantEntity->dAbsTileZ;
	  HighEntity->FacingDirection = 0;
	}
    }

  State->EntityResidence[EntityIndex] = Residence;
}

internal inline entity
GetEntity(state *State, entity_residence Residence, u32 Index)
{
  entity Entity = {};
  
  if((Index > 0) && (Index < State->EntityCount))
    {
      if(State->EntityResidence[Index] < Residence)
	{
	  ChangeEntityResidence(State, Index, Residence);	  
	  Assert(State->EntityResidence[Index] >= Residence);
	}
      
      Entity.Residence = Residence;
      Entity.Dormant = &State->DormantEntities[Index];
      Entity.Low = &State->LowEntities[Index];
      Entity.High = &State->HighEntities[Index];
    }

  return(Entity);
}

internal void
InitializePlayer(state *State, r32 EntityIndex)
{
  entity Entity = GetEntity(State, EntityResidence_Dormant, EntityIndex);

  Entity.Dormant->P.AbsTileX = 1;
  Entity.Dormant->P.AbsTileY = 3;
  Entity.Dormant->P.Offset_.X = 0.0f;
  Entity.Dormant->P.Offset_.Y = 0.0f;  
  Entity.Dormant->Height = 0.5f;//1.4f;
  Entity.Dormant->Width  = 1.0f;// * Entity->Height;
  Entity.Dormant->Collides = true;
  
  ChangeEntityResidence(State, EntityIndex, EntityResidence_High);
  
  if(GetEntity(State, EntityResidence_Dormant, State->CameraFollowingEntityIndex).Residence ==
     EntityResidence_NonExistent)
    {
      State->CameraFollowingEntityIndex = EntityIndex;
    }
}

internal u32
AddEntity(state *State)
{
  u32 EntityIndex = State->EntityCount++;

  Assert(State->EntityCount < ArrayCount(State->DormantEntities));
  Assert(State->EntityCount < ArrayCount(State->LowEntities));
  Assert(State->EntityCount < ArrayCount(State->HighEntities));

  dormant_entity ZeroDormant = {};
  low_entity  ZeroLow  = {};
  high_entity ZeroHigh = {};
  
  State->EntityResidence[EntityIndex] = EntityResidence_Dormant;
  State->DormantEntities[EntityIndex] = ZeroDormant;
  State->LowEntities[EntityIndex] = ZeroLow;
  State->HighEntities[EntityIndex] = ZeroHigh;
    
  return(EntityIndex);
}

internal bool32
TestWall(r32 WallX, r32 RelX, r32 RelY, r32 PlayerDeltaX, r32 PlayerDeltaY,
	 r32 *tMin, r32 MinY, r32 MaxY)
{
  bool32 Hit = false;

  r32 Epsilon = 0.001f;
  if(PlayerDeltaX != 0.0f)
    {
      r32 tResult = (WallX - RelX) / PlayerDeltaX;
      r32 Y = RelY + (tResult * PlayerDeltaY);
      if((tResult >= 0.0f) && (*tMin > tResult))
	{
	  if((Y >= MinY) && (Y <= MaxY))
	    {
	      *tMin = Maximum(0.0f, tResult - Epsilon);
	      Hit = true;
	    }
	}
    }

  return(Hit);
}

internal void
MovePlayer(state *State, entity Entity, r32 dt, v2 ddPlayer)
{
  tile_map *TileMap = State->World->TileMap; 

  r32 ddPSqLength = LengthSq(ddPlayer);
  if(ddPSqLength > 1.0f)
    {
      ddPlayer = VMulS(1.0f / SquareRoot(ddPSqLength), ddPlayer);
    }
  
  r32 PlayerSpeed = 50.0f;
  ddPlayer = VMulS(PlayerSpeed, ddPlayer);
  ddPlayer = VAdd(ddPlayer,
		  VMulS(-8.0f, Entity.High->dP));

  v2 OldPlayerP = Entity.High->P;
  v2 PlayerDelta = VAdd(VMulS(0.5f ,
			      VMulS(Square(dt),
				    ddPlayer)),
			VMulS(dt, Entity.High->dP));
  Entity.High->dP = VAdd(VMulS(dt, ddPlayer),
		    Entity.High->dP);
  v2 NewPlayerP = VAdd(OldPlayerP, PlayerDelta);
  /*
  u32 MinTileX = Minimum(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
  u32 MinTileY = Minimum(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);
  u32 MaxTileX = Maximum(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
  u32 MaxTileY = Maximum(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);

  u32 EntityTileWidth = CeilReal32ToInt32(Entity.Dormant->Width / TileMap->TileSideInMeters);
  u32 EntityTileHeight = CeilReal32ToInt32(Entity.Dormant->Height / TileMap->TileSideInMeters);
  
  MinTileX -= EntityTileWidth;
  MinTileY -= EntityTileHeight;
  MaxTileX += EntityTileWidth;
  MaxTileY += EntityTileHeight;

  u32 AbsTileZ = Entity.High->P.AbsTileZ;
  */
  r32 tRemaining = 1.0f;
  for(u32 Iteration = 0;
      (Iteration < 4) && (tRemaining > 0.0f);
      ++Iteration)
    {
      r32 tMin = 1.0f;
      v2 WallNormal = {};
      s32 HitEntityIndex = 0;
      
      for(u32 EntityIndex = 1;
	  EntityIndex < State->EntityCount;
	  ++EntityIndex)
	{
	  entity TestEntity = GetEntity(State, EntityResidence_High, EntityIndex);
	  if(TestEntity.High != Entity.High)
	    {
	      if(TestEntity.Dormant->Collides)
		{
		  r32 DiameterW = TestEntity.Dormant->Width + Entity.Dormant->Width;
		  r32 DiameterH = TestEntity.Dormant->Height + Entity.Dormant->Height;
		  v2 MinCorner = (v2){-0.5f*DiameterW,
				      -0.5f*DiameterH};
		  v2 MaxCorner = (v2){0.5f*DiameterW,
				      0.5f*DiameterH};

		  v2 Rel = VSub(Entity.High->P, TestEntity.High->P);
	  
		  if(TestWall(MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
			      &tMin, MinCorner.Y, MaxCorner.Y))
		    {
		      HitEntityIndex = EntityIndex;
		      WallNormal = (v2){-1, 0};
		    }
		  if(TestWall(MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
			      &tMin, MinCorner.Y, MaxCorner.Y))
		    {
		      HitEntityIndex = EntityIndex;
		      WallNormal = (v2){1, 0};
		    }
		  if(TestWall(MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
			      &tMin, MinCorner.X, MaxCorner.X))
		    {
		      HitEntityIndex = EntityIndex;
		      WallNormal = (v2){0, -1};
		    }
		  if(TestWall(MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
			      &tMin, MinCorner.X, MaxCorner.X))
		    {
		      HitEntityIndex = EntityIndex;
		      WallNormal = (v2){0, 1};
		    }
		}
	    }
	}
      
      Entity.High->P = VAdd(Entity.High->P,
			    VMulS(tMin, PlayerDelta));
      if(HitEntityIndex)
	{
	  Entity.High->dP = VSub(Entity.High->dP,
				 VMulS(1 * Inner(Entity.High->dP, WallNormal),
				       WallNormal));
	  PlayerDelta = VSub(PlayerDelta,
			     VMulS(1 * Inner(PlayerDelta, WallNormal),
				   WallNormal));
	  tRemaining -= tMin*tRemaining;	  

	  entity HitEntity = GetEntity(State, EntityResidence_High, HitEntityIndex);
	  Entity.Dormant->P.AbsTileZ += HitEntity.Dormant->dAbsTileZ;
	}
      else
	{
	  break;
	}
    }
  
  if((Entity.High->dP.X == 0) &&
     (Entity.High->dP.Y == 0))
    {
      
    }
  else if(AbsoluteValue(Entity.High->dP.X) > AbsoluteValue(Entity.High->dP.Y))
    {
      if(Entity.High->dP.X > 0)
	{
	  Entity.High->FacingDirection = 0;
	}
      else
	{
	  Entity.High->FacingDirection = 2;
	}
    }
  else 
    {
      if(Entity.High->dP.Y > 0)
	{
	  Entity.High->FacingDirection = 1;
	}
      else
	{
	  Entity.High->FacingDirection = 3;
	}
      
    }
  //State->World->TileMap
  Entity.Dormant->P = MapIntoTileSpace(State->World->TileMap, State->CameraP, Entity.High->P);
}


extern UPDATE_AND_RENDER(UpdateAndRender)
{  
  state* State = (state*) Memory->PermanentStorage;
  if(!Memory->isInitialized)
    {
      AddEntity(State);

      State->Backdrop
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_background.bmp");
      State->Shadow
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_shadow.bmp");

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
#if 0
      u32 ScreenX = INT32_MAX / 2;
      u32 ScreenY = INT32_MAX / 2;
#else
      u32 ScreenX = 0;
      u32 ScreenY = 0;
#endif
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
      entity ControllingEntity = GetEntity(State, EntityResidence_High,
					    State->PlayerIndexForController[ControllerIndex]);
      if(ControllingEntity.Residence != EntityResidence_NonExistent)
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

	  if(Controller->ActionUp.EndedDown)
	    {
	      ControllingEntity.High->dZ = 3.0f;
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

  v2 EntityOffsetForFrame = {};
  entity CameraFollowingEntity
    = GetEntity(State, EntityResidence_High, State->CameraFollowingEntityIndex);
  if(CameraFollowingEntity.Residence != EntityResidence_NonExistent)
    {
      tile_map_position OldCameraP = State->CameraP;
      
      State->CameraP.AbsTileZ = CameraFollowingEntity.Dormant->P.AbsTileZ;

      if(CameraFollowingEntity.High->P.X > (9.0f * TileMap->TileSideInMeters))
	{
	  State->CameraP.AbsTileX += 17;
	}
      if(CameraFollowingEntity.High->P.X < -(9.0f * TileMap->TileSideInMeters))
	{
	  State->CameraP.AbsTileX -= 17;
	}
      if(CameraFollowingEntity.High->P.Y > (5.0f * TileMap->TileSideInMeters))
	{
	  State->CameraP.AbsTileY += 9;
	}
      if(CameraFollowingEntity.High->P.Y < -(5.0f * TileMap->TileSideInMeters))
	{
	  State->CameraP.AbsTileY -= 9;
	}
      
      tile_map_difference dCameraP = Subtract(TileMap, &State->CameraP, &OldCameraP);
      EntityOffsetForFrame = VMulS(-1, dCameraP.dXY);
    }

  DrawBitmap(Buffer, &State->Backdrop, 0, 0, 0, 0, 1.0f);
  
  r32 ScreenCenterX = 0.5f * (r32)Buffer->Width;
  r32 ScreenCenterY = 0.5f * (r32)Buffer->Height;
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
	      v2 Center   = {ScreenCenterX - MetersToPixels*State->CameraP.Offset_.X + ((r32)RelColumn * TileSideInPixels),
			     ScreenCenterY + MetersToPixels*State->CameraP.Offset_.Y - ((r32)RelRow * TileSideInPixels)};
	      v2 Min = VSub(Center, VMulS(0.9f,TileSide));
	      v2 Max = VAdd(Center, VMulS(0.9f,TileSide));
	      
	      DrawRectangle(Buffer,
			    Min, Max,
			    Grey, Grey, Grey);
	    }
	}
    }

  for(u32 EntityIndex = 0;
      EntityIndex < State->EntityCount;
      ++EntityIndex)
    {
      if(State->EntityResidence[EntityIndex] == EntityResidence_High)
	{
	  high_entity *HighEntity = &State->HighEntities[EntityIndex];
	  low_entity  *LowEntity = &State->LowEntities[EntityIndex];
	  dormant_entity *DormantEntity = &State->DormantEntities[EntityIndex];

	  HighEntity->P = VAdd(HighEntity->P, EntityOffsetForFrame);

	  r32 dt  = Input->dtForFrame;
	  r32 ddZ = -9.8f;
	  HighEntity->Z = 0.5f * ddZ * Square(dt) + HighEntity->dZ*dt + HighEntity->Z;
	  HighEntity->dZ = ddZ * dt + HighEntity->dZ;
	  if(HighEntity->Z < 0)
	    {
	      HighEntity->Z = 0;
	    }
	  r32 CAlpha = 1.0f - 0.5f*HighEntity->Z;
	    if(CAlpha < 0)
	      {
		CAlpha = 0;
	      }

	  r32 PlayerR = 1.0f;
	  r32 PlayerG = 1.0f;
	  r32 PlayerB = 0.0f;
	  r32 PlayerGroundPointX = ScreenCenterX + MetersToPixels*HighEntity->P.X;
	  r32 PlayerGroundPointY = ScreenCenterY - MetersToPixels*HighEntity->P.Y;
	  r32 Z = -MetersToPixels*HighEntity->Z;
	  v2 PlayerLeftTop = {PlayerGroundPointX - 0.5f * MetersToPixels*DormantEntity->Width,
			      PlayerGroundPointY - 0.5f * MetersToPixels*DormantEntity->Height};
	  v2 PlayerWidthHeight = {DormantEntity->Width, DormantEntity->Height};
#if 0
	  DrawRectangle(Buffer,
			PlayerLeftTop,
			VAdd(PlayerLeftTop,
			     VMulS(MetersToPixels,
				   PlayerWidthHeight)),
			PlayerR, PlayerG, PlayerB);
#endif
	  hero_bitmaps *Hero = &State->HeroBitmaps[HighEntity->FacingDirection];
	  DrawBitmap(Buffer, &State->Shadow, PlayerGroundPointX, PlayerGroundPointY, Hero->AlignX, Hero->AlignY, CAlpha);
	  DrawBitmap(Buffer, &Hero->Torso, PlayerGroundPointX, PlayerGroundPointY + Z, Hero->AlignX, Hero->AlignY, 1.0f);
	  DrawBitmap(Buffer, &Hero->Cape,  PlayerGroundPointX, PlayerGroundPointY + Z, Hero->AlignX, Hero->AlignY, 1.0f);
	  DrawBitmap(Buffer, &Hero->Head,  PlayerGroundPointX, PlayerGroundPointY + Z, Hero->AlignX, Hero->AlignY, 1.0f);
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
