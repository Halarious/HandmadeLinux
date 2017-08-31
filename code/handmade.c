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
#include "handmade_entity.c"

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

typedef struct
{
  low_entity *Low;
  u32 LowIndex;
} add_low_entity_result;

internal add_low_entity_result
AddLowEntity(state *State, entity_type Type, world_position P)
{  
  Assert(State->LowEntityCount < ArrayCount(State->LowEntities));
  u32 EntityIndex = State->LowEntityCount++;

  low_entity *LowEntity = State->LowEntities + EntityIndex;
  low_entity  ZeroLow  = {};
  *LowEntity = ZeroLow;
  LowEntity->Sim.Type = Type;
  LowEntity->P = NullPosition();

  ChangeEntityLocation(&State->WorldArena, State->World, EntityIndex,
		       LowEntity, P);
  
  add_low_entity_result Result;
  Result.Low = LowEntity;
  Result.LowIndex  = EntityIndex;
  
  return(Result);
}

internal add_low_entity_result
AddWall(state *State, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ)
{
  world_position P = ChunkPositionFromTilePosition(State->World, AbsTileX, AbsTileY, AbsTileZ, V3(0, 0, 0));
  add_low_entity_result Entity = AddLowEntity(State, EntityType_Wall, P);
  
  Entity.Low->Sim.Dim.Y = State->World->TileSideInMeters;
  Entity.Low->Sim.Dim.X  = Entity.Low->Sim.Dim.Y;
  AddFlags(&Entity.Low->Sim, EntityFlag_Collides);

  return(Entity);
}

internal add_low_entity_result
AddStair(state *State, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ)
{
  world_position P = ChunkPositionFromTilePosition(State->World, AbsTileX, AbsTileY, AbsTileZ,
						   V3(0.0f, 0.0f, 0.5f*State->World->TileDepthInMeters));
  add_low_entity_result Entity = AddLowEntity(State, EntityType_Stairwell, P);
  
  Entity.Low->Sim.Dim.Y = State->World->TileSideInMeters;
  Entity.Low->Sim.Dim.X  = Entity.Low->Sim.Dim.Y;
  Entity.Low->Sim.Dim.Z  = 1.2f * State->World->TileDepthInMeters;
  
  return(Entity);
}

internal void
InitHitPoint(low_entity *EntityLow, u32 HitPointCount)
{
  Assert(HitPointCount <= ArrayCount(EntityLow->Sim.HitPoints));
  EntityLow->Sim.HitPointMax = HitPointCount;
  for(u32 HitPointIndex = 0;
      HitPointIndex < EntityLow->Sim.HitPointMax;
      ++HitPointIndex)
    {
      hit_point *HitPoint = EntityLow->Sim.HitPoints + HitPointIndex;
      HitPoint->Flags = 0;
      HitPoint->FilledAmount = HIT_POINT_SUB_COUNT;
    }
}

internal add_low_entity_result
AddSword(state *State)
{
  add_low_entity_result Entity = AddLowEntity(State, EntityType_Sword, NullPosition());

  Entity.Low->Sim.Dim.Y = 0.5f;//1.4f;
  Entity.Low->Sim.Dim.X  = 1.0f;// * Entity->Height;
  AddFlags(&Entity.Low->Sim, EntityFlag_Moveable);
 
  return(Entity);
}

internal void
ClearCollisionRuleFor(state *State, u32 StorageIndex)
{
  for(u32 HashBucket = 0;
      HashBucket < ArrayCount(State->CollisionRuleHash);
      ++ HashBucket)
    {
      for(pairwise_collision_rule **Rule = &State->CollisionRuleHash[HashBucket];
	  *Rule;)
	{
	  if(((*Rule)->StorageIndexA == StorageIndex) ||
	     ((*Rule)->StorageIndexB == StorageIndex))
	    {
	      pairwise_collision_rule *RemovedRule = *Rule;
	      *Rule = (*Rule)->NextInHash;

	      RemovedRule->NextInHash = State->FirstFreeCollisionRule;
	      State->FirstFreeCollisionRule = RemovedRule;
	    }
	  else
	    {
	      Rule = &((*Rule)->NextInHash);
	    }
	}
    }  
}

internal void
AddCollisionRule(state *State, u32 StorageIndexA, u32 StorageIndexB, bool32 CanCollide)
{
  if(StorageIndexA > StorageIndexB)
    {
      u32 Temp = StorageIndexA;
      StorageIndexA = StorageIndexB;
      StorageIndexB = Temp;
    }

  pairwise_collision_rule *Found = 0;
  u32 HashBucket = StorageIndexA & (ArrayCount(State->CollisionRuleHash) - 1);
  for(pairwise_collision_rule *Rule = State->CollisionRuleHash[HashBucket];
      Rule;
      Rule = Rule->NextInHash)
    {
      if((Rule->StorageIndexA == StorageIndexA) &&
	 (Rule->StorageIndexB == StorageIndexB))
	{
	  Found = Rule;
	  break;
	}
    }

  if(!Found)
    {
      Found = State->FirstFreeCollisionRule;
      if(Found)
	{
	  State->FirstFreeCollisionRule = Found->NextInHash;
	}
      else
	{
	  Found = PushStruct(&State->WorldArena, pairwise_collision_rule);
	}
            
      Found->NextInHash = State->CollisionRuleHash[HashBucket];
      State->CollisionRuleHash[HashBucket] = Found;
    }
  
  if(Found)
    {
      Found->StorageIndexA = StorageIndexA;
      Found->StorageIndexB = StorageIndexB;
      Found->CanCollide = CanCollide;
    }
}

internal add_low_entity_result
AddPlayer(state *State)
{
  world_position P = State->CameraP;
  add_low_entity_result Entity = AddLowEntity(State, EntityType_Hero, P);

  InitHitPoint(Entity.Low, 3);
  Entity.Low->Sim.Dim.Y = 0.5f;
  Entity.Low->Sim.Dim.X  = 1.0f;
  AddFlags(&Entity.Low->Sim, EntityFlag_Collides|EntityFlag_Moveable);
  
  add_low_entity_result Sword = AddSword(State);
  Entity.Low->Sim.Sword.Index = Sword.LowIndex;

  AddCollisionRule(State, Sword.LowIndex, Entity.LowIndex, false);
  
  if(State->CameraFollowingEntityIndex == 0)
    {
      State->CameraFollowingEntityIndex = Entity.LowIndex;
    }

  return(Entity);
}

internal add_low_entity_result
AddMonstar(state *State, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ)
{
  world_position P = ChunkPositionFromTilePosition(State->World, AbsTileX, AbsTileY, AbsTileZ,
						   V3(0,0,0));
  add_low_entity_result Entity = AddLowEntity(State, EntityType_Monstar, P);

  InitHitPoint(Entity.Low, 3);
  Entity.Low->Sim.Dim.Y = 0.5f;//1.4f;
  Entity.Low->Sim.Dim.X  = 1.0f;// * Entity->Height;
  AddFlags(&Entity.Low->Sim, EntityFlag_Collides|EntityFlag_Moveable);
    
  return(Entity);
}

internal add_low_entity_result
AddFamiliar(state *State, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ)
{
  world_position P = ChunkPositionFromTilePosition(State->World, AbsTileX, AbsTileY, AbsTileZ,
						   V3(0,0,0));
  add_low_entity_result Entity = AddLowEntity(State, EntityType_Familiar, P);
  
  Entity.Low->Sim.Dim.Y = 0.5f;//1.4f;
  Entity.Low->Sim.Dim.X  = 1.0f;// * Entity->Height;
  AddFlags(&Entity.Low->Sim, EntityFlag_Collides|EntityFlag_Moveable);
  
  return(Entity);
}

internal inline void
PushPiece(entity_visible_piece_group *Group, loaded_bitmap* Bitmap,
	  v2 Offset, r32 OffsetZ, v2 Align, v2 Dim, v4 Color, r32 EntityZC)
{
  Assert(Group->Count < ArrayCount(Group->Pieces));
  entity_visible_piece *Piece = Group->Pieces + Group->Count++;

  Piece->Bitmap = Bitmap;
  Piece->Offset = V2Sub(V2MulS(Group->State->MetersToPixels,
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

internal void
DrawHitpoints(sim_entity *Entity, entity_visible_piece_group *Group)
{
  if(Entity->HitPointMax >= 1)
    {
      v2 HealthDim = {0.2f, 0.2f};
      r32 SpacingX = 1.5f*HealthDim.X;
      v2 HitP = {-0.5f*(Entity->HitPointMax - 1)*SpacingX, -0.2f};
      v2 dHitP = {SpacingX, 0.0f};
      for(u32 HealthIndex = 0;
	  HealthIndex < Entity->HitPointMax;
	  ++HealthIndex)
	{
	  hit_point *HitPoint = Entity->HitPoints + HealthIndex;
	  v4 Color = {1.0f, 0.0f, 0.0f, 1.0f};
	  if(HitPoint->FilledAmount == 0)
	    {
	      Color.R = 0.2f;
	      Color.G = 0.2f;
	      Color.B = 0.2f;
	    }
	  PushRect(Group, HitP, 0, HealthDim, Color, 0.0f);
	  HitP = V2Add(HitP, dHitP);
	}
    }
}

extern UPDATE_AND_RENDER(UpdateAndRender)
{  
  state* State = (state*) Memory->PermanentStorage;
  if(!Memory->isInitialized)
    {
      AddLowEntity(State, EntityType_Null, NullPosition());
            
      State->Backdrop
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_background.bmp");
      State->Shadow
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test/test_hero_shadow.bmp");
      State->Tree
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test2/tree00.bmp");
      State->Sword
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test2/rock03.bmp");
      State->Stairwell
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test2/rock02.bmp");

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

		  bool32 ShouldBeDoor = false;
		  if((TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight/2))))
		    { 
		      ShouldBeDoor = true;
		    }		  
		  if((TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != (TilesPerHeight/2))))
		    {
		      ShouldBeDoor = true;
		    }
		  if((TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth/2))))
		    {
		      ShouldBeDoor = true;
		    }
		  if((TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth/2))))
		    {
		      ShouldBeDoor = true;
		    }

		  if(ShouldBeDoor)
		    {
		      AddWall(State, AbsTileX, AbsTileY, AbsTileZ);
		    }
		  else if(CreatedZDoor)
		    {
		      if((TileX == 10) && (TileY == 6))
			{
			  AddStair(State, AbsTileX, AbsTileY, DoorDown ?  (AbsTileZ - 1) : AbsTileZ);
			}
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
						 CameraTileZ,
						 V3(0,0,0));
      State->CameraP = NewCameraP;
      
      AddMonstar(State, CameraTileX - 3, CameraTileY + 2, CameraTileZ);
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
      
      Memory->isInitialized = true;
    }

  world *World = State->World;
  r32 MetersToPixels = State->MetersToPixels;

  for(int ControllerIndex = 0;
      ControllerIndex < ArrayCount(Input->Controllers);
      ++ControllerIndex)
    {
      controller_input *Controller = GetController(Input, ControllerIndex);
      controlled_hero *ConHero = State->ControlledHeroes + ControllerIndex;
      if(ConHero->EntityIndex == 0)
	{
	  if(Controller->Start.EndedDown)
	    {
	      controlled_hero ZeroHero = {};
	      *ConHero = ZeroHero;
	      
	      ConHero->EntityIndex = AddPlayer(State).LowIndex;
	    }
	}
      else
	{
	  ConHero->ddP = V2(0, 0);
	  ConHero->dSword  = V2(0, 0);
	  ConHero->dZ  = 0.0f;
	  
	  if(Controller->IsAnalog)
	    {
	      ConHero->ddP = (v2){Controller->StickAverageX,
				  Controller->StickAverageY};
	    }
	  else
	    {
	      if(Controller->MoveUp.EndedDown)
		{
		  ConHero->ddP.Y = 1.0f;
		}
	      if(Controller->MoveDown.EndedDown)
		{
		  ConHero->ddP.Y = -1.0f;
		}
	      if(Controller->MoveLeft.EndedDown)
		{
		  ConHero->ddP.X = -1.0f;
		}
	      if(Controller->MoveRight.EndedDown)
		{
		  ConHero->ddP.X = 1.0f;
		}	  
	    }

	  ConHero->dSword = V2(0, 0);	  
	  if(Controller->Start.EndedDown)
	    {
	      ConHero->dZ = 3.0f;
	    }

 	  if(Controller->ActionUp.EndedDown)
	    {
	      ConHero->dSword = V2(0.0f, 1.0f);
	    }
 	  if(Controller->ActionDown.EndedDown)
	    {
	      ConHero->dSword = V2(0.0f, -1.0f);
	    } 
	  if(Controller->ActionLeft.EndedDown)
	    {
	      ConHero->dSword = V2(-1.0f, 0.0f);
	    }
	  if(Controller->ActionRight.EndedDown)
	    {
	      ConHero->dSword = V2(1.0f, 0.0f);
	    }
	}
    }

  u32 TileSpanX = 17*3;
  u32 TileSpanY = 9*3;
  u32 TileSpanZ = 1;
  rectangle3 CameraBounds = RectCenterDim(V3(0,0,0),
					  V3MulS(World->TileSideInMeters,
						 V3((r32)TileSpanX,
						    (r32)TileSpanY,
						    (r32)TileSpanZ)));

  memory_arena SimArena;
  InitializeArena(&SimArena, Memory->TransientStorageSize, Memory->TransientStorage);
  sim_region *SimRegion = BeginSim(&SimArena, State, World, State->CameraP, CameraBounds, Input->dtForFrame);

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
  sim_entity *Entity = SimRegion->Entities;
  for(u32 EntityIndex = 0;
      EntityIndex < SimRegion->EntityCount;
      ++EntityIndex, ++Entity)
    {
      if(Entity->Updatable)
	{
	  PieceGroup.Count = 0;      
	  r32 dt  = Input->dtForFrame;

	  r32 ShadowAlpha = 1.0f - 0.5f*Entity->P.Z;
	  if(ShadowAlpha < 0)
	    {
	      ShadowAlpha = 0;
	    }

	  move_spec MoveSpec = DefaultMoveSpec();;
	  v3 ddP = {};
	  
	  hero_bitmaps *Hero = &State->HeroBitmaps[Entity->FacingDirection];
	  switch(Entity->Type)
	    {
	    case EntityType_Hero:
	      {
		for(u32 ControlIndex = 0;
		    ControlIndex < ArrayCount(State->ControlledHeroes);
		    ++ControlIndex)
		  {
		    controlled_hero *ConHero = State->ControlledHeroes + ControlIndex;

		    if(ConHero->EntityIndex == Entity->StorageIndex)
		      {
			if(ConHero->dZ != 0.0f)
			  {
			    Entity->dP.Z = ConHero->dZ;
			  }
		    
			MoveSpec.UnitMaxAccelVector = true;
			MoveSpec.Speed = 50.0f;
			MoveSpec.Drag = 8.0f;
			ddP = ToV3(ConHero->ddP, 0);

			if((ConHero->dSword.X != 0.0f) ||
			   (ConHero->dSword.Y != 0.0f))
			  {
			    sim_entity *Sword = Entity->Sword.Ptr;
			    if(Sword && IsSet(Sword, EntityFlag_Nonspatial))
			      {
				Sword->DistanceLimit = 5.0f;
				MakeEntitySpatial(Sword, Entity->P,
						  V3MulS(5.0f, ToV3(ConHero->dSword, 0)));
				AddCollisionRule(State, Sword->StorageIndex, Entity->StorageIndex, false);
			      }
			  }
		      }
		  }
	  
		PushBitmap(&PieceGroup, &State->Shadow, V2(0, 0), 0 ,Hero->Align, ShadowAlpha, 0.0f);
		PushBitmap(&PieceGroup, &Hero->Torso, V2(0, 0), 0 ,Hero->Align, 1.0f, 1.0f);
		PushBitmap(&PieceGroup, &Hero->Cape,  V2(0, 0), 0 ,Hero->Align, 1.0f, 1.0f);
		PushBitmap(&PieceGroup, &Hero->Head,  V2(0, 0), 0 ,Hero->Align, 1.0f, 1.0f);	

		DrawHitpoints(Entity, &PieceGroup);
	    	    
	      } break;
	    case EntityType_Wall:
	      {
		PushBitmap(&PieceGroup, &State->Tree, V2(0, 0), 0 ,V2(40, 80), 1.0f, 1.0f);
	      } break;
	    case EntityType_Stairwell:
	      {
		PushRect(&PieceGroup, V2(0,0), 0, Entity->Dim.XY, V4(1, 1, 0, 1), 0.0f);
	      } break;
	    case EntityType_Sword:
	      {
		MoveSpec.UnitMaxAccelVector = false;
		MoveSpec.Speed = 0.0f;
		MoveSpec.Drag = 0.0f;

		if(Entity->DistanceLimit == 0.0f)
		  {
		    ClearCollisionRuleFor(State, Entity->StorageIndex);
		    MakeEntityNonSpatial(Entity);
		  }

		PushBitmap(&PieceGroup, &State->Shadow, V2(0, 0), 0 ,Hero->Align, ShadowAlpha, 0.0f);
		PushBitmap(&PieceGroup, &State->Sword, V2(0, 0), 0 ,V2(29, 10), 1.0f, 1.0f);
	      } break;
	    case EntityType_Familiar:
	      {
		sim_entity *ClosestHero = 0;
		r32 ClosestHeroDSq = Square(10.0f);

		sim_entity *TestEntity = SimRegion->Entities;
		for(u32 TestEntityIndex = 1;
		    TestEntityIndex < SimRegion->EntityCount;
		    ++TestEntityIndex, ++TestEntity)
		  {
		    if(TestEntity->Type == EntityType_Hero)
		      {
			r32 TestDSq = V3LengthSq(V3Sub(TestEntity->P,
						       Entity->P));
			if(ClosestHeroDSq > TestDSq)
			  {
			    ClosestHero = TestEntity;
			    ClosestHeroDSq = TestDSq; 
			  }
		      }
		  }
  
		if((ClosestHero) && (ClosestHeroDSq > Square(3.0f)))
		  {
		    r32 Acceleration = 0.5f;
		    r32 OneOverLength = Acceleration / SquareRoot(ClosestHeroDSq);
		    ddP = V3MulS(OneOverLength,
				 V3Sub(ClosestHero->P, Entity->P));
		  }

		MoveSpec.UnitMaxAccelVector = true;
		MoveSpec.Speed = 50.0f;
		MoveSpec.Drag = 8.0f;

		Entity->tBob += dt;
		if(Entity->tBob > (2.0f * Pi32))
		  {
		    Entity->tBob -= 2.0f * Pi32;
		  }
		r32 BobSin = Sin(2.0f*Entity->tBob);

		PushBitmap(&PieceGroup, &State->Shadow, V2(0, 0), 0, Hero->Align, (0.5f*ShadowAlpha) + 0.2f*BobSin, 0.0f);
		PushBitmap(&PieceGroup, &Hero->Head, V2(0, 0), 0.25f*BobSin, Hero->Align, 1.0f, 1.0f);
	      } break;
	    case EntityType_Monstar:
	      {
		DrawHitpoints(Entity, &PieceGroup);

		PushBitmap(&PieceGroup, &State->Shadow, V2(0, 0), 0 ,Hero->Align, ShadowAlpha, 1.0f);
		PushBitmap(&PieceGroup, &Hero->Torso, V2(0, 0), 0 ,Hero->Align, 1.0f, 1.0f);
	      } break;
	    default:
	      {
		InvalidCodePath;
	      } break;
	    }

	  if(!IsSet(Entity, EntityFlag_Nonspatial) &&
	     IsSet(Entity, EntityFlag_Moveable))
	    {	      
	      MoveEntity(State, SimRegion, Entity,
			 Input->dtForFrame, &MoveSpec, ddP);
	    }
	  
	  r32 EntityGroundPointX = ScreenCenterX + MetersToPixels*Entity->P.X;
	  r32 EntityGroundPointY = ScreenCenterY - MetersToPixels*Entity->P.Y;
	  r32 EntityZ = -MetersToPixels*Entity->P.Z;
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
		  v2 HalfDim = V2MulS(0.5f*MetersToPixels, Piece->Dim);
		  DrawRectangle(Buffer,
				V2Sub(Center, HalfDim),
				V2Add(Center, HalfDim),
				Piece->R, Piece->G, Piece->B);
		}
	    }
	}
    }
  
  world_position WorldOrigin = {};
  v3 Diff = Subtract(SimRegion->World, &WorldOrigin, &SimRegion->Origin);
  DrawRectangle(Buffer,
		Diff.XY,
		V2(10.0f,
		   10.0f),
		1.0f, 1.0f, 0.0f);
  
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
