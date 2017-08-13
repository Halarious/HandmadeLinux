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
#include "handmade_world.c"
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
  s32 MaxX = MinX + (r32)Bitmap->Width;
  s32 MaxY = MinY + (r32)Bitmap->Height;

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

internal inline low_entity*
GetLowEntity(state *State, u32 Index)
{
  low_entity *Result = 0;
  
  if((Index > 0) && (Index < State->LowEntityCount))
    {
      Result = State->LowEntities + Index; 
    }

  return(Result);
}

internal high_entity*
MakeEntityHighFrequency(state *State, u32 LowIndex)
{
  high_entity *HighEntity = 0;
  
  low_entity *LowEntity = &State->LowEntities[LowIndex];
  if(LowEntity->HighEntityIndex)
    {
      HighEntity = State->HighEntities_ + LowEntity->HighEntityIndex;
    }
  else
    {
      if(State->HighEntityCount < ArrayCount(State->HighEntities_))
	{
	  u32 HighIndex = State->HighEntityCount++;
	  HighEntity = State->HighEntities_ + HighIndex;
   
	  world_difference Diff = Subtract(State->World,
					  &LowEntity->P, &State->CameraP);
	  HighEntity->P = Diff.dXY;
	  HighEntity->dP = (v2){0, 0};
	  HighEntity->ChunkZ = LowEntity->P.ChunkZ;
	  HighEntity->FacingDirection = 0;
	  HighEntity->LowEntityIndex = LowIndex;
	  
	  LowEntity->HighEntityIndex = HighIndex;
	}
      else
	{
	  InvalidCodePath;
	}
    }
  
  return(HighEntity);
}

internal inline entity
GetHighEntity(state *State, u32 LowIndex)
{
  entity Result = {};
  
  if((LowIndex > 0) && (LowIndex < State->LowEntityCount))
  {
    Result.LowIndex = LowIndex;
    Result.Low      = State->LowEntities + LowIndex;
    Result.High     = MakeEntityHighFrequency(State, LowIndex);
  }
  
  return(Result);
}

internal inline void
MakeEntityLowFrequency(state *State, u32 LowIndex)
{
  low_entity *LowEntity = &State->LowEntities[LowIndex];
  u32 HighIndex = LowEntity->HighEntityIndex;
  if(HighIndex)
    {
      u32 LastHighIndex = State->HighEntityCount - 1;
      if(HighIndex != LastHighIndex)
	{
	  high_entity *LastEntity = State->HighEntities_ + LastHighIndex;
	  high_entity *DelEntity  = State->HighEntities_ + HighIndex;

	  *DelEntity = *LastEntity;
	  State->LowEntities[LastEntity->LowEntityIndex].HighEntityIndex = HighIndex;
	}
      --State->HighEntityCount;
      LowEntity->HighEntityIndex = 0;
    }  
}

internal inline void
OffsetAndCheckFrequencyByArea(state *State, v2 Offset, rectangle2 HighFrequencyBounds)
{
  for(u32 EntityIndex = 1;
      EntityIndex < State->HighEntityCount;
      )
    {
      high_entity *High = State->HighEntities_ + EntityIndex;

      High->P = VAdd(High->P, Offset);
      if(IsInRectangle(HighFrequencyBounds, High->P))
	{
	  ++EntityIndex;
	}
      else
	{
	  MakeEntityLowFrequency(State, High->LowEntityIndex);
	}
    } 
}

internal u32
AddLowEntity(state *State, entity_type Type)
{
  Assert(State->LowEntityCount < ArrayCount(State->LowEntities));
  u32 EntityIndex = State->LowEntityCount++;

  low_entity  ZeroLow  = {};

  State->LowEntities[EntityIndex] = ZeroLow;
  State->LowEntities[EntityIndex].Type = Type;
  
  return(EntityIndex);
}

internal u32
AddWall(state *State, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ)
{
  u32 EntityIndex = AddLowEntity(State, EntityType_Wall);
  low_entity *Entity = GetLowEntity(State, EntityIndex);

  Entity->P = ChunkPositionFromTilePosition(State->World, AbsTileX, AbsTileY, AbsTileZ);
  Entity->Height = State->World->TileSideInMeters;
  Entity->Width  = Entity->Height;
  Entity->Collides = true;

  return(EntityIndex);
}

internal u32
AddPlayer(state *State)
{
  u32 EntityIndex = AddLowEntity(State, EntityType_Hero);
  low_entity *LowEntity = GetLowEntity(State, EntityIndex);

  LowEntity->P = State->CameraP;
  LowEntity->Height = 0.5f;//1.4f;
  LowEntity->Width  = 1.0f;// * Entity->Height;
  LowEntity->Collides = true;
  
  if(State->CameraFollowingEntityIndex == 0)
    {
      State->CameraFollowingEntityIndex = EntityIndex;
    }
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
  world *World = State->World; 

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

  u32 EntityTileWidth = CeilReal32ToInt32(Entity.Low->Width / World->TileSideInMeters);
  u32 EntityTileHeight = CeilReal32ToInt32(Entity.Low->Height / World->TileSideInMeters);
  
  MinTileX -= EntityTileWidth;
  MinTileY -= EntityTileHeight;
  MaxTileX += EntityTileWidth;
  MaxTileY += EntityTileHeight;

  u32 AbsTileZ = Entity.High->P.AbsTileZ;
  */
  for(u32 Iteration = 0;
      Iteration < 4;
      ++Iteration)
    {
      r32 tMin = 1.0f;
      v2 WallNormal = {};
      s32 HitHighEntityIndex = 0;
      
      v2 DesiredPosition = VAdd(Entity.High->P, PlayerDelta);

      for(u32 TestHighEntityIndex = 1;
	  TestHighEntityIndex < State->HighEntityCount;
	  ++TestHighEntityIndex)
	{	
	  if(TestHighEntityIndex != Entity.Low->HighEntityIndex)
	    {
	      entity TestEntity;
	      TestEntity.High = State->HighEntities_ + TestHighEntityIndex; ;
	      TestEntity.LowIndex = TestEntity.High->LowEntityIndex;
	      TestEntity.Low  = State->LowEntities + TestEntity.LowIndex;
	      if(TestEntity.Low->Collides)
		{
		  r32 DiameterW = TestEntity.Low->Width + Entity.Low->Width;
		  r32 DiameterH = TestEntity.Low->Height + Entity.Low->Height;
		  v2 MinCorner = (v2){-0.5f*DiameterW,
				      -0.5f*DiameterH};
		  v2 MaxCorner = (v2){0.5f*DiameterW,
				      0.5f*DiameterH};

		  v2 Rel = VSub(Entity.High->P, TestEntity.High->P);
	  
		  if(TestWall(MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
			      &tMin, MinCorner.Y, MaxCorner.Y))
		    {
		      HitHighEntityIndex = TestHighEntityIndex;
		      WallNormal = (v2){-1, 0};
		    }
		  if(TestWall(MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
			      &tMin, MinCorner.Y, MaxCorner.Y))
		    {
		      HitHighEntityIndex = TestHighEntityIndex;
		      WallNormal = (v2){1, 0};
		    }
		  if(TestWall(MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
			      &tMin, MinCorner.X, MaxCorner.X))
		    {
		      HitHighEntityIndex = TestHighEntityIndex;
		      WallNormal = (v2){0, -1};
		    }
		  if(TestWall(MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
			      &tMin, MinCorner.X, MaxCorner.X))
		    {
		      HitHighEntityIndex = TestHighEntityIndex;
		      WallNormal = (v2){0, 1};
		    }
		}
	    }
	}
      
      Entity.High->P = VAdd(Entity.High->P,
			    VMulS(tMin, PlayerDelta));
      if(HitHighEntityIndex)
	{
	  Entity.High->dP = VSub(Entity.High->dP,
				 VMulS(1 * Inner(Entity.High->dP, WallNormal),
				       WallNormal));
	  PlayerDelta = VSub(DesiredPosition, Entity.High->P);
	  PlayerDelta = VSub(PlayerDelta,
			     VMulS(1 * Inner(PlayerDelta, WallNormal),
				   WallNormal));
	  
	  high_entity *HitHigh = State->HighEntities_ + HitHighEntityIndex;
	  low_entity  *HitLow  = State->LowEntities + HitHigh->LowEntityIndex;
	  //Entity.High->P.ChunkZ += HitLow->dAbsTileZ;
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
  //State->World
  Entity.Low->P = MapIntoWorldSpace(World, State->CameraP, Entity.High->P);
}

internal void
SetCamera(state *State, world_position NewCameraP)
{
  world *World = State->World;

  world_difference dCameraP = Subtract(World, &NewCameraP, &State->CameraP);
  State->CameraP = NewCameraP;
  
  u32 TileSpanX = 17*3;
  u32 TileSpanY = 9*3;
  rectangle2 CameraBounds = RectCenterDim(V2(0,0),
					  VMulS(World->TileSideInMeters,
						V2((r32)TileSpanX, (r32)TileSpanY)));
  v2 EntityOffsetForFrame = VMulS(-1, dCameraP.dXY);
  OffsetAndCheckFrequencyByArea(State, EntityOffsetForFrame, CameraBounds);
#if 0  
  s32 MinTileX = NewCameraP.AbsTileX - TileSpanX/2;
  s32 MinTileY = NewCameraP.AbsTileY - TileSpanY/2;
  s32 MaxTileX = NewCameraP.AbsTileX + TileSpanX/2;
  s32 MaxTileY = NewCameraP.AbsTileY + TileSpanY/2;
  for(u32 EntityIndex = 1;
      EntityIndex < State->LowEntityCount;
      ++EntityIndex)
    {
      low_entity *Low = State->LowEntities + EntityIndex;
      if(Low->HighEntityIndex == 0)
	{
	  if((Low->P.AbsTileZ == NewCameraP.AbsTileZ) &&
	     (Low->P.AbsTileX >= MinTileX) &&
	     (Low->P.AbsTileX <= MaxTileX) &&
	     (Low->P.AbsTileY >= MinTileY) &&
	     (Low->P.AbsTileY <= MaxTileY))
	    {
	      MakeEntityHighFrequency(State, EntityIndex);
	    }
	}
    }
#endif
}

extern UPDATE_AND_RENDER(UpdateAndRender)
{  
  state* State = (state*) Memory->PermanentStorage;
  if(!Memory->isInitialized)
    {
      AddLowEntity(State, EntityType_Null);
      State->HighEntityCount = 1;
      
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
           
      State->World   = PushStruct(&State->WorldArena, world);
      world *World   = State->World;
      InitializeWorld(World, 1.4f);
      
      u32 RandomNumberIndex = 0;
	        
      u32 TilesPerWidth = 17;
      u32 TilesPerHeight = 9;
      u32 ScreenBaseX = 0;
      u32 ScreenBaseY = 0;
      u32 ScreenBaseZ = 0;
      u32 ScreenX = ScreenBaseX;
      u32 ScreenY = ScreenBaseY;
      u32 AbsTileZ = ScreenBaseZ;
      
      bool32 DoorLeft   = false;
      bool32 DoorRight  = false;
      bool32 DoorTop    = false;
      bool32 DoorBottom = false;
      bool32 DoorUp     = false;
      bool32 DoorDown   = false;
	    
      for(s32 ScreenIndex = 0;
	  ScreenIndex < 2000;
	  ++ScreenIndex)
	{
	  Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
	  u32 RandomChoice;
	  if(1)//(DoorUp || DoorDown)
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
	      if(AbsTileZ == ScreenBaseZ)
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
		  
		  if(TileValue == 2)
		    {
		      AddWall(State, AbsTileX, AbsTileY, AbsTileZ);
		    }
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
	      if(AbsTileZ == ScreenBaseZ)
		{
		  AbsTileZ = ScreenBaseZ + 1;
		}
	      else
		{
		  AbsTileZ = ScreenBaseZ;
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
#if 0
      while(State->LowEntityCount < (ArrayCount(State->LowEntities) - 16))
	{
	  u32 Coordinate = 1024 + State->LowEntityCount;
	  AddWall(State, Coordinate, Coordinate, Coordinate);
	}
#endif      
      world_position NewCameraP = {};
      NewCameraP = ChunkPositionFromTilePosition(World,
						 ScreenBaseX*TilesPerWidth + 17/2,
						 ScreenBaseY*TilesPerHeight + 9/2,
						 ScreenBaseZ);
      SetCamera(State, NewCameraP);

      Memory->isInitialized = true;
    }

  world *World = State->World;
  
  s32 TileSideInPixels = 60;
  r32 MetersToPixels = (r32)TileSideInPixels / (r32)World->TileSideInMeters;

  for(int ControllerIndex = 0;
      ControllerIndex < ArrayCount(Input->Controllers);
      ++ControllerIndex)
    {
      controller_input *Controller = GetController(Input, ControllerIndex);
      u32 LowIndex = State->PlayerIndexForController[ControllerIndex];
      if(LowIndex == 0)
	{
	  if(Controller->Start.EndedDown)
	    {
	      u32 EntityIndex = AddPlayer(State);
	      State->PlayerIndexForController[ControllerIndex] = EntityIndex;
	    }
	}
      else
	{
	  entity ControllingEntity = GetHighEntity(State, LowIndex);
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
    }

  v2 EntityOffsetForFrame = {};
  entity CameraFollowingEntity = GetHighEntity(State,
					       State->CameraFollowingEntityIndex);
  if(CameraFollowingEntity.High)
    {
      world_position NewCameraP = State->CameraP;
      
      State->CameraP.ChunkZ = CameraFollowingEntity.Low->P.ChunkZ;
#if 0
      if(CameraFollowingEntity.High->P.X > (9.0f * World->TileSideInMeters))
	{
	  NewCameraP.AbsTileX += 17;
	}
      if(CameraFollowingEntity.High->P.X < -(9.0f * World->TileSideInMeters))
	{
	  NewCameraP.AbsTileX -= 17;
	}
      if(CameraFollowingEntity.High->P.Y > (5.0f * World->TileSideInMeters))
	{
	  NewCameraP.AbsTileY += 9;
	}
      if(CameraFollowingEntity.High->P.Y < -(5.0f * World->TileSideInMeters))
	{
	  NewCameraP.AbsTileY -= 9;
	}
#else
      NewCameraP = CameraFollowingEntity.Low->P;
#endif

      SetCamera(State, NewCameraP);
    }

  DrawBitmap(Buffer, &State->Backdrop, 0, 0, 0, 0, 1.0f);
  
  r32 ScreenCenterX = 0.5f * (r32)Buffer->Width;
  r32 ScreenCenterY = 0.5f * (r32)Buffer->Height;

#if 0
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
	  u32 TileID = GetTileValueC(World, Column, Row, State->CameraP.AbsTileZ);
	  
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
#endif
  for(u32 HighEntityIndex = 1;
      HighEntityIndex < State->HighEntityCount;
      ++HighEntityIndex)
    {
      
      high_entity *HighEntity = State->HighEntities_ + HighEntityIndex;
      low_entity *LowEntity = State->LowEntities + HighEntity->LowEntityIndex;

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
      v2 PlayerLeftTop = {PlayerGroundPointX - 0.5f * MetersToPixels*LowEntity->Width,
			  PlayerGroundPointY - 0.5f * MetersToPixels*LowEntity->Height};
      v2 PlayerWidthHeight = {LowEntity->Width, LowEntity->Height};
      if(LowEntity->Type == EntityType_Hero)
	{
	  hero_bitmaps *Hero = &State->HeroBitmaps[HighEntity->FacingDirection];
	  DrawBitmap(Buffer, &State->Shadow, PlayerGroundPointX, PlayerGroundPointY, Hero->AlignX, Hero->AlignY, CAlpha);
	  DrawBitmap(Buffer, &Hero->Torso, PlayerGroundPointX, PlayerGroundPointY + Z, Hero->AlignX, Hero->AlignY, 1.0f);
	  DrawBitmap(Buffer, &Hero->Cape,  PlayerGroundPointX, PlayerGroundPointY + Z, Hero->AlignX, Hero->AlignY, 1.0f);
	  DrawBitmap(Buffer, &Hero->Head,  PlayerGroundPointX, PlayerGroundPointY + Z, Hero->AlignX, Hero->AlignY, 1.0f);
	
	}
      else
	{
	    

	  DrawRectangle(Buffer,
			PlayerLeftTop,
			VAdd(PlayerLeftTop,
			     VMulS(MetersToPixels,
				   PlayerWidthHeight)),
			PlayerR, PlayerG, PlayerB);
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
