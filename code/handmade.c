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

#include "handmade_sim_region.c"

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
	   r32 RealX,  r32 RealY, r32 CAlpha)
{
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

internal inline v2
GetCameraSpaceP(state *State, low_entity *LowEntity)
{
  world_difference Diff = Subtract(State->World,
				   &LowEntity->P, &State->CameraP);
  v2 Result = Diff.dXY;

  return(Result);
}

typedef struct
{
  low_entity *Low;
  u32 LowIndex;
} add_low_entity_result;

internal add_low_entity_result
AddLowEntity(state *State, entity_type Type, world_position *P)
{  
  Assert(State->LowEntityCount < ArrayCount(State->LowEntities));
  u32 EntityIndex = State->LowEntityCount++;

  low_entity  ZeroLow  = {};
  low_entity *LowEntity = State->LowEntities + EntityIndex;
  *LowEntity = ZeroLow;
  LowEntity->Type = Type;

  ChangeEntityLocation(&State->WorldArena, State->World, EntityIndex,
		       LowEntity, 0, P);
  
  add_low_entity_result Result;
  Result.Low = LowEntity;
  Result.LowIndex  = EntityIndex;
  
  return(Result);
}

internal add_low_entity_result
AddWall(state *State, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ)
{
  world_position P = ChunkPositionFromTilePosition(State->World, AbsTileX, AbsTileY, AbsTileZ);
  add_low_entity_result Entity = AddLowEntity(State, EntityType_Wall, &P);
  
  Entity.Low->Height = State->World->TileSideInMeters;
  Entity.Low->Width  = Entity.Low->Height;
  Entity.Low->Collides = true;

  return(Entity);
}

internal void
InitHitPoint(low_entity *EntityLow, u32 HitPointCount)
{
  Assert(HitPointCount <= ArrayCount(EntityLow->HitPoints));
  EntityLow->HitPointMax = HitPointCount;
  for(u32 HitPointIndex = 0;
      HitPointIndex < EntityLow->HitPointMax;
      ++HitPointIndex)
    {
      hit_point *HitPoint = EntityLow->HitPoints + HitPointIndex;
      HitPoint->Flags = 0;
      HitPoint->FilledAmount = HIT_POINT_SUB_COUNT;
    }
}

internal add_low_entity_result
AddSword(state *State)
{
  add_low_entity_result Entity = AddLowEntity(State, EntityType_Sword, 0);

  Entity.Low->Height = 0.5f;//1.4f;
  Entity.Low->Width  = 1.0f;// * Entity->Height;
  Entity.Low->Collides = false;

  return(Entity);
}

internal add_low_entity_result
AddPlayer(state *State)
{
  world_position P = State->CameraP;
  add_low_entity_result Entity = AddLowEntity(State, EntityType_Hero, &P);

  InitHitPoint(Entity.Low, 3);
  Entity.Low->Height = 0.5f;
  Entity.Low->Width  = 1.0f;
  Entity.Low->Collides = true;

  add_low_entity_result Sword = AddSword(State);
  Entity.Low->SwordLowIndex = Sword.LowIndex;
  
  if(State->CameraFollowingEntityIndex == 0)
    {
      State->CameraFollowingEntityIndex = Entity.LowIndex;
    }

  return(Entity);
}

internal add_low_entity_result
AddMonstar(state *State, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ)
{
  world_position P = ChunkPositionFromTilePosition(State->World, AbsTileX, AbsTileY, AbsTileZ);
  add_low_entity_result Entity = AddLowEntity(State, EntityType_Monstar, &P);

  InitHitPoint(Entity.Low, 3);
  Entity.Low->Height = 0.5f;//1.4f;
  Entity.Low->Width  = 1.0f;// * Entity->Height;
  Entity.Low->Collides = true;

  return(Entity);
}

internal add_low_entity_result
AddFamiliar(state *State, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ)
{
  world_position P = ChunkPositionFromTilePosition(State->World, AbsTileX, AbsTileY, AbsTileZ);
  add_low_entity_result Entity = AddLowEntity(State, EntityType_Familiar, &P);
  
  Entity.Low->Height = 0.5f;//1.4f;
  Entity.Low->Width  = 1.0f;// * Entity->Height;
  Entity.Low->Collides = true;

  return(Entity);
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

typedef struct
{
  bool32 UnitMaxAccelVector;
  r32 Speed;
  r32 Drag;
} move_spec;

internal inline move_spec
DefaultMoveSpec()
{  
  move_spec Result;

  Result.UnitMaxAccelVector = false;
  Result.Speed = 1.0f;
  Result.Drag = 0.0f;

  return(Result);
}

internal void
MoveEntity(state *State, entity Entity, r32 dt, move_spec *MoveSpec, v2 ddEntity)
{
  world *World = State->World; 

  if(MoveSpec->UnitMaxAccelVector)
    {
      r32 ddPSqLength = LengthSq(ddEntity);
      if(ddPSqLength > 1.0f)
	{
	  ddEntity = VMulS(1.0f / SquareRoot(ddPSqLength), ddEntity);
	}
    }
  
  ddEntity = VMulS(MoveSpec->Speed, ddEntity);

  ddEntity = VAdd(ddEntity,
		  VMulS(-MoveSpec->Drag, Entity.High->dP));

  v2 OldPlayerP = Entity.High->P;
  v2 PlayerDelta = VAdd(VMulS(0.5f ,
			      VMulS(Square(dt),
				    ddEntity)),
			VMulS(dt, Entity.High->dP));
  Entity.High->dP = VAdd(VMulS(dt, ddEntity),
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

      if(Entity.Low->Collides)
	{
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

  world_position NewP = MapIntoChunkSpace(World, State->CameraP, Entity.High->P);
  ChangeEntityLocation(&State->WorldArena, World, Entity.LowIndex,
		       Entity.Low, &Entity.Low->P, &NewP);
}

internal inline void
PushPiece(entity_visible_piece_group *Group, loaded_bitmap* Bitmap,
	  v2 Offset, r32 OffsetZ, v2 Align, v2 Dim, v4 Color, r32 EntityZC)
{
  Assert(Group->Count < ArrayCount(Group->Pieces));
  entity_visible_piece *Piece = Group->Pieces + Group->Count++;

  Piece->Bitmap = Bitmap;
  Piece->Offset = VSub(VMulS(Group->State->MetersToPixels,
			     V2(Offset.X, -Offset.Y)),
		       Align);  
  Piece->OffsetZ = Group->State->MetersToPixels * OffsetZ;
  Piece->EntityZC = EntityZC;
  Piece->A = Color.A;
  Piece->R = Color.R;
  Piece->G = Color.G;
  Piece->B = Color.B;
  Piece->Dim = Dim;
}

internal inline void
PushBitmap(entity_visible_piece_group *Group, loaded_bitmap* Bitmap,
	   v2 Offset, r32 OffsetZ, v2 Align, r32 Alpha, r32 EntityZC)
{
  PushPiece(Group, Bitmap, Offset, OffsetZ, Align, V2(0, 0),
	    V4(1.0f, 1.0f, 1.0f, Alpha), EntityZC);
}

internal inline void
PushRect(entity_visible_piece_group *Group, v2 Offset, r32 OffsetZ,
	 v2 Dim, v4 Color, r32 EntityZC)
{
  PushPiece(Group, 0, Offset, OffsetZ, V2(0, 0), Dim,
	    Color, EntityZC);
}

internal entity
EntityFromHighIndex(state *State, u32 HighEntityIndex)
{
  entity Result = {};

  if(HighEntityIndex)
    {
      Assert(HighEntityIndex < ArrayCount(State->HighEntities_));
      Result.High = State->HighEntities_ + HighEntityIndex;
      Result.LowIndex = Result.High->LowEntityIndex;
      Result.Low  = State->LowEntities + Result.LowIndex;
    }

  return(Result);
}

internal void
UpdateFamiliar(state *State, entity Entity, r32 dt)
{
  entity ClosestHero = {};
  r32 ClosestHeroDSq = Square(10.0f);
  for(u32 HighEntityIndex = 1;
      HighEntityIndex < State->HighEntityCount;
      ++HighEntityIndex)
    {
      entity TestEntity = EntityFromHighIndex(State, HighEntityIndex);

      if((HighEntityIndex != Entity.Low->HighEntityIndex) &&
	 ((TestEntity.Low->Type == EntityType_Hero)))
	{
	  r32 TestDSq = LengthSq(VSub(TestEntity.High->P,
				      Entity.High->P));
	  if(TestEntity.Low->Type == EntityType_Hero)
	    {
	      TestDSq *= 0.75f;
	    }
	  if(ClosestHeroDSq > TestDSq)
	    {
	      ClosestHero = TestEntity;
	      ClosestHeroDSq = TestDSq; 
	    }
	}
    }
  
  v2 ddP = {};
  if((ClosestHero.High) && (ClosestHeroDSq > Square(3.0f)))
    {
      r32 Acceleration = 0.5f;
      r32 OneOverLength = Acceleration / SquareRoot(ClosestHeroDSq);
      ddP = VMulS(OneOverLength,
		  VSub(ClosestHero.High->P, Entity.High->P));
    }

  move_spec MoveSpec = DefaultMoveSpec();
  MoveSpec.UnitMaxAccelVector = true;
  MoveSpec.Speed = 50.0f;
  MoveSpec.Drag = 8.0f;
  MoveEntity(State, Entity, dt, &MoveSpec, ddP);
}

internal void
UpdateMonstar(state *State, entity Entity, r32 dt)
{
}

internal void
UpdateSword(state *State, entity Entity, r32 dt)
{  
  move_spec MoveSpec = DefaultMoveSpec();
  MoveSpec.UnitMaxAccelVector = false;
  MoveSpec.Speed = 0.0f;
  MoveSpec.Drag = 0.0f;

  v2 OldP = Entity.High->P;
  MoveEntity(State, Entity, dt, &MoveSpec, V2(0, 0));
  r32 DistanceTraveled = Length(VSub(Entity.High->P,
				     OldP));
  Entity.Low->DistanceRemaining -= DistanceTraveled;
  if(Entity.Low->DistanceRemaining < 0.0f)
    {
      ChangeEntityLocation(&State->WorldArena, State->World,
			   Entity.LowIndex, Entity.Low, &Entity.Low->P, 0);
    }
}

internal void
DrawHitpoints(low_entity *LowEntity, entity_visible_piece_group *Group)
{
  if(LowEntity->HitPointMax >= 1)
    {
      v2 HealthDim = {0.2f, 0.2f};
      r32 SpacingX = 1.5f*HealthDim.X;
      v2 HitP = {-0.5f*(LowEntity->HitPointMax - 1)*SpacingX, -0.2f};
      v2 dHitP = {SpacingX, 0.0f};
      for(u32 HealthIndex = 0;
	  HealthIndex < LowEntity->HitPointMax;
	  ++HealthIndex)
	{
	  hit_point *HitPoint = LowEntity->HitPoints + HealthIndex;
	  v4 Color = {1.0f, 0.0f, 0.0f, 1.0f};
	  if(HitPoint->FilledAmount == 0)
	    {
	      Color.R = 0.2f;
	      Color.G = 0.2f;
	      Color.B = 0.2f;
	    }
	  PushRect(Group, HitP, 0, HealthDim, Color, 0.0f);
	  HitP = VAdd(HitP, dHitP);
	}
    }
}

extern UPDATE_AND_RENDER(UpdateAndRender)
{  
  state* State = (state*) Memory->PermanentStorage;
  if(!Memory->isInitialized)
    {
      AddLowEntity(State, EntityType_Null, 0);
      State->HighEntityCount = 1;
      
      State->Backdrop
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_background.bmp");
      State->Shadow
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_shadow.bmp");
      State->Tree
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test2/tree00.bmp");
      State->Sword
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test2/rock03.bmp");

      hero_bitmaps *Bitmap = State->HeroBitmaps;
      
      Bitmap->Head
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_right_head.bmp");
      Bitmap->Cape
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_right_cape.bmp");
      Bitmap->Torso
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_right_torso.bmp");
      Bitmap->Align = V2(72, 182);
      ++Bitmap;
      
      Bitmap->Head
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_back_head.bmp");
      Bitmap->Cape
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_back_cape.bmp");
      Bitmap->Torso
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_back_torso.bmp");
      Bitmap->Align = V2(72, 182);
      ++Bitmap;
      
      Bitmap->Head
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_left_head.bmp");
      Bitmap->Cape
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_left_cape.bmp");
      Bitmap->Torso
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_left_torso.bmp");
      Bitmap->Align = V2(72, 182);
      ++Bitmap;
      
      Bitmap->Head
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_front_head.bmp");
      Bitmap->Cape
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_front_cape.bmp");
      Bitmap->Torso
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_front_torso.bmp");
      Bitmap->Align = V2(72, 182);
      
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
      
      s32 TileSideInPixels = 60;
      State->MetersToPixels = (r32)TileSideInPixels / (r32)World->TileSideInMeters;
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
      u32 CameraTileX = ScreenBaseX*TilesPerWidth + 17/2;
      u32 CameraTileY = ScreenBaseY*TilesPerHeight + 9/2;
      u32 CameraTileZ = ScreenBaseZ;
      NewCameraP = ChunkPositionFromTilePosition(World,
						 CameraTileX,
						 CameraTileY,
						 CameraTileZ);
 
      AddMonstar(State, CameraTileX + 2, CameraTileY + 2, CameraTileZ);
      for(u32 FamiliarIndex = 0;
	  FamiliarIndex < 1;
	  ++FamiliarIndex)
	{
	  s32 FamiliarOffsetX = (RandomNumberTable[RandomNumberIndex++] % 10) - 5;
	  s32 FamiliarOffsetY = (RandomNumberTable[RandomNumberIndex++] % 10) - 10;
	  AddFamiliar(State,
		      CameraTileX + FamiliarOffsetX,
		      CameraTileY + FamiliarOffsetY, CameraTileZ);
	}
      
      SetCamera(State, NewCameraP);
 
      Memory->isInitialized = true;
    }

  world *World = State->World;
  r32 MetersToPixels = State->MetersToPixels;

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
	      u32 EntityIndex = AddPlayer(State).LowIndex;
	      State->PlayerIndexForController[ControllerIndex] = EntityIndex;
	    }
	}
      else
	{
	  entity ControllingEntity = ForceEntityIntoHigh(State, LowIndex);
	  v2 ddEntity = {};

	  if(Controller->IsAnalog)
	    {
	      ddEntity = (v2){Controller->StickAverageX,
			      Controller->StickAverageY};
	    }
	  else
	    {
	      if(Controller->MoveUp.EndedDown)
		{
		  ddEntity.Y = 1.0f;
		}
	      if(Controller->MoveDown.EndedDown)
		{
		  ddEntity.Y = -1.0f;
		}
	      if(Controller->MoveLeft.EndedDown)
		{
		  ddEntity.X = -1.0f;
		}
	      if(Controller->MoveRight.EndedDown)
		{
		  ddEntity.X = 1.0f;
		}	  
	    }
 
	  if(Controller->Start.EndedDown)
	    {
	      ControllingEntity.High->dZ = 3.0f;
	    }

	  v2 dSword = {};
 	  if(Controller->ActionUp.EndedDown)
	    {
	      dSword = V2(0.0f, 1.0f);
	    }
 	  if(Controller->ActionDown.EndedDown)
	    {
	      dSword = V2(0.0f, -1.0f);
	    } 
	  if(Controller->ActionLeft.EndedDown)
	    {
	      dSword = V2(-1.0f, 0.0f);
	    }
	  if(Controller->ActionRight.EndedDown)
	    {
	      dSword = V2(1.0f, 0.0f);
	    }
	  
	  move_spec MoveSpec = DefaultMoveSpec();
	  MoveSpec.UnitMaxAccelVector = true;
	  MoveSpec.Speed = 50.0f;
	  MoveSpec.Drag = 8.0f;
	  MoveEntity(State, ControllingEntity,
		     Input->dtForFrame, &MoveSpec, ddEntity);
	  if((dSword.X != 0.0f) || (dSword.Y != 0.0f))
	    {
	      low_entity *LowSword = GetLowEntity(State, ControllingEntity.Low->SwordLowIndex);
	      if(LowSword && !IsValid(LowSword->P))
		{
		  world_position SwordP = ControllingEntity.Low->P;
		  ChangeEntityLocation(&State->WorldArena, World, ControllingEntity.Low->SwordLowIndex, LowSword, 0, &SwordP);	  
		  entity Sword = ForceEntityIntoHigh(State, ControllingEntity.Low->SwordLowIndex);

		  Sword.Low->DistanceRemaining = 5.0f;
		  Sword.High->dP = VMulS(5.0f, dSword);
		}
	    }
	}
    }
  
  u32 TileSpanX = 17*3;
  u32 TileSpanY = 9*3;
  rectangle2 CameraBounds = RectCenterDim(V2(0,0),
					  VMulS(World->TileSideInMeters,
						V2((r32)TileSpanX, (r32)TileSpanY)));
  sim_region *SimRegion = BeginSim(SimArena, World, State->CameraP, CameraBounds);

#if 1
  DrawRectangle(Buffer,
		V2(0.0f, 0.0f),
		V2((r32)Buffer->Width,
		   (r32)Buffer->Height),
		0.5f, 0.5f, 0.5f);
#else
  DrawBitmap(Buffer, &State->Backdrop, 0, 0, 0, 0, 1.0f);
#endif

  r32 ScreenCenterX = 0.5f * (r32)Buffer->Width;
  r32 ScreenCenterY = 0.5f * (r32)Buffer->Height;

  entity_visible_piece_group PieceGroup = {};
  PieceGroup.State = State;
  entity *Entity = SimRegion->Entites;
  for(u32 EntityIndex = 0;
      EntityIndex < SimRegion->EntityCount;
      ++EntityIndex)
    {
      PieceGroup.Count = 0;
	
      low_entity *LowEntity = State->LowEntities + HighEntity->StorageIndex;

      r32 dt  = Input->dtForFrame;

      r32 ShadowAlpha = 1.0f - 0.5f*HighEntity->Z;
      if(ShadowAlpha < 0)
	{
	  ShadowAlpha = 0;
	}

      hero_bitmaps *Hero = &State->HeroBitmaps[LowEntity->FacingDirection];
      switch(LowEntity->Type)
	{
	case EntityType_Hero:
	  {
	    PushBitmap(&PieceGroup, &State->Shadow, V2(0, 0), 0 ,Hero->Align, ShadowAlpha, 0.0f);
	    PushBitmap(&PieceGroup, &Hero->Torso, V2(0, 0), 0 ,Hero->Align, 1.0f, 1.0f);
	    PushBitmap(&PieceGroup, &Hero->Cape,  V2(0, 0), 0 ,Hero->Align, 1.0f, 1.0f);
	    PushBitmap(&PieceGroup, &Hero->Head,  V2(0, 0), 0 ,Hero->Align, 1.0f, 1.0f);	

	    DrawHitpoints(LowEntity, &PieceGroup);
	    	    
	  } break;
	case EntityType_Wall:
	  {
	    PushBitmap(&PieceGroup, &State->Tree, V2(0, 0), 0 ,V2(40, 80), 1.0f, 1.0f);
	  } break;
	case EntityType_Sword:
	  {
	    UpdateSword(State, Entity, dt);
	    PushBitmap(&PieceGroup, &State->Shadow, V2(0, 0), 0 ,Hero->Align, ShadowAlpha, 0.0f);
	    PushBitmap(&PieceGroup, &State->Sword, V2(0, 0), 0 ,V2(29, 10), 1.0f, 1.0f);
	  } break;
	case EntityType_Familiar:
	  {
	    UpdateFamiliar(State, Entity, dt);
	    Entity.High->tBob += dt;
	    if(Entity.High->tBob > (2.0f * Pi32))
	      {
		Entity.High->tBob -= 2.0f * Pi32;
	      }
	    r32 BobSin = Sin(2.0f*Entity.High->tBob);
	    PushBitmap(&PieceGroup, &State->Shadow, V2(0, 0), 0, Hero->Align, (0.5f*ShadowAlpha) + 0.2f*BobSin, 0.0f);
	    PushBitmap(&PieceGroup, &Hero->Head, V2(0, 0), 0.25f*BobSin, Hero->Align, 1.0f, 1.0f);
	  } break;
	case EntityType_Monstar:
	  {
	    DrawHitpoints(LowEntity, &PieceGroup);
	    UpdateMonstar(State, Entity, dt);
	    PushBitmap(&PieceGroup, &State->Shadow, V2(0, 0), 0 ,Hero->Align, ShadowAlpha, 1.0f);
	    PushBitmap(&PieceGroup, &Hero->Torso, V2(0, 0), 0 ,Hero->Align, 1.0f, 1.0f);
	  } break;
	default:
	  {
	    InvalidCodePath;
	  } break;
	}
      
      r32 ddZ = -9.8f;
      HighEntity->Z = 0.5f * ddZ * Square(dt) + Entity->dZ*dt + Entity->Z;
      HighEntity->dZ = ddZ * dt + Entity->dZ;
      if(Entity->Z < 0)
	{
	  Entity->Z = 0;
	}
      	    
      r32 EntityGroundPointX = ScreenCenterX + MetersToPixels*Entity->P.X;
      r32 EntityGroundPointY = ScreenCenterY - MetersToPixels*Entity->P.Y;
      r32 EntityZ = -MetersToPixels*Entity->Z;
#if 0
      v2 PlayerLeftTop = {PlayerGroundPointX - 0.5f * MetersToPixels*LowEntity->Width,
			  PlayerGroundPointY - 0.5f * MetersToPixels*LowEntity->Height};
      v2 PlayerWidthHeight = {LowEntity->Width, LowEntity->Height};

      DrawRectangle(Buffer,
		    PlayerLeftTop,
		    VAdd(PlayerLeftTop,
			 VMulS(0.9f*MetersToPixels,
			       PlayerWidthHeight)),
		    1.0f, 1.0f, 0.0f);
#endif
      for(u32 PieceIndex = 0;
	  PieceIndex < PieceGroup.Count;
	  ++PieceIndex)
	{
	  entity_visible_piece *Piece = PieceGroup.Pieces + PieceIndex;
	  v2 Center = {Piece->Offset.X + EntityGroundPointX,
		       Piece->Offset.Y + EntityGroundPointY
		       + Piece->OffsetZ + Piece->EntityZC*EntityZ};;
	  if(Piece->Bitmap)
	    {
	      DrawBitmap(Buffer, Piece->Bitmap,
			 Center.X, Center.Y, Piece->A);
	    }
	  else
	    {
	      v2 HalfDim = VMulS(0.5f*MetersToPixels, Piece->Dim);
	      DrawRectangle(Buffer,
			    VSub(Center, HalfDim),
			    VAdd(Center, HalfDim),
			    Piece->R, Piece->G, Piece->B);
	    }
	}
    }
  
  EndSim(SimRegion, State);
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
