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
#include "handmade_render_group.h"
#include "handmade_render_group.c"
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
      u32 TotalBitmapSize = Bitmap->Width*Bitmap->Height*BITMAP_BYTES_PER_PIXEL;
      ZeroSize(TotalBitmapSize, Bitmap->Memory);
    }
}

internal loaded_bitmap
MakeEmptyBitmap(memory_arena* Arena, s32 Width, s32 Height, bool32 ClearToZero)
{
  loaded_bitmap Result = {};
  Result.Width  = Width;
  Result.Height = Height;
  Result.Pitch  = Width*BITMAP_BYTES_PER_PIXEL;

  s32 TotalBitmapSize = Width*Height*4;
  Result.Memory      = PushSize(Arena, TotalBitmapSize);
  
  if(ClearToZero)
  {
    ClearBitmap(&Result);
  }

  return(Result); 
}

/*
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

	      r32 R = (r32)((Color & RedMask) >> RedShiftDown);
	      r32 G = (r32)((Color & GreenMask) >> GreenShiftDown);
	      r32 B = (r32)((Color & BlueMask) >> BlueShiftDown);
	      r32 A = (r32)((Color & AlphaMask) >> AlphaShiftDown);
	      r32 AN = (A / 255.0f);

	      R = R*AN;
	      G = G*AN;
	      B = B*AN;

	      *SourceDest++ = (((u32)(A + 0.5f) << 24) |
			       ((u32)(R + 0.5f) << 16) |
			       ((u32)(G + 0.5f) << 8)  |
			       ((u32)(B + 0.5f) << 0));
	    }
	}
    }  

  s32 BytesPerPixel = BITMAP_BYTES_PER_PIXEL;
  Result.Pitch  = -Result.Width*BytesPerPixel;
  Result.Memory = ((u8*)Result.Memory -
		   Result.Pitch*(Result.Height - 1));
  
  return(Result);
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
  LowEntity->Sim.Collision = State->NullCollision;
  LowEntity->P = NullPosition();

  ChangeEntityLocation(&State->WorldArena, State->World, EntityIndex,
		       LowEntity, P);
  
  add_low_entity_result Result;
  Result.Low = LowEntity;
  Result.LowIndex  = EntityIndex;
  
  return(Result);
}

internal add_low_entity_result
AddGroundedEntity(state *State, entity_type Type, world_position P,
		  sim_entity_collision_volume_group *Collision)
{
  add_low_entity_result Entity = AddLowEntity(State, Type, P);
  Entity.Low->Sim.Collision = Collision;

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

internal world_position
ChunkPositionFromTilePosition(world *World,
			      s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ,
			      v3 AdditionalOffset)
{
  world_position BasePos = {};

  r32 TileSideInMeters  = 1.4f;
  r32 TileDepthInMeters = 3.0f;
  
  v3 TileDim = V3(TileSideInMeters, TileSideInMeters, TileDepthInMeters);
  v3 Offset  = V3Hadamard(TileDim,
			  V3((r32)AbsTileX, (r32)AbsTileY, (r32)AbsTileZ));
  world_position Result = MapIntoChunkSpace(World, BasePos,
					    V3Add(Offset, AdditionalOffset));

  Assert(IsCannonicalV(World, Result.Offset_));
  
  return(Result);
}

internal add_low_entity_result
AddStandardSpace(state *State, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ)
{
  world_position P = ChunkPositionFromTilePosition(State->World, AbsTileX, AbsTileY, AbsTileZ, V3(0, 0, 0));
  add_low_entity_result Entity = AddGroundedEntity(State, EntityType_Space, P,
						   State->StandardRoomCollision);
  AddFlags(&Entity.Low->Sim, EntityFlag_Traversable);

  return(Entity);
}

internal add_low_entity_result
AddWall(state *State, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ)
{
  world_position P = ChunkPositionFromTilePosition(State->World, AbsTileX, AbsTileY, AbsTileZ, V3(0, 0, 0));
  add_low_entity_result Entity = AddGroundedEntity(State, EntityType_Wall, P,
						   State->WallCollision);
  AddFlags(&Entity.Low->Sim, EntityFlag_Collides);

  return(Entity);
}

internal add_low_entity_result
AddStair(state *State, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ)
{
  world_position P = ChunkPositionFromTilePosition(State->World, AbsTileX, AbsTileY, AbsTileZ,
						   V3(0.0f, 0.0f, 0.0f));
  add_low_entity_result Entity = AddGroundedEntity(State, EntityType_Stairwell, P,
						   State->StairCollision);
  AddFlags(&Entity.Low->Sim, EntityFlag_Collides);
  Entity.Low->Sim.WalkableDim = Entity.Low->Sim.Collision->TotalVolume.Dim.xy;
  Entity.Low->Sim.WalkableHeight = State->TypicalFloorHeight;
  
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

  Entity.Low->Sim.Collision = State->SwordCollision;
  AddFlags(&Entity.Low->Sim, EntityFlag_Moveable);
 
  return(Entity);
}

internal add_low_entity_result
AddPlayer(state *State)
{
  world_position P = State->CameraP;
  add_low_entity_result Entity = AddGroundedEntity(State, EntityType_Hero, P,
						   State->PlayerCollision);

  InitHitPoint(Entity.Low, 3);
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
  add_low_entity_result Entity = AddGroundedEntity(State, EntityType_Monstar, P,
						   State->MonstarCollision);

  InitHitPoint(Entity.Low, 3);
  AddFlags(&Entity.Low->Sim, EntityFlag_Collides|EntityFlag_Moveable);
    
  return(Entity);
}

internal add_low_entity_result
AddFamiliar(state *State, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ)
{
  world_position P = ChunkPositionFromTilePosition(State->World, AbsTileX, AbsTileY, AbsTileZ,
						   V3(0,0,0));
  add_low_entity_result Entity = AddGroundedEntity(State, EntityType_Familiar, P,
						   State->FamiliarCollision);
  AddFlags(&Entity.Low->Sim, EntityFlag_Collides|EntityFlag_Moveable);
  
  return(Entity);
}

internal void
DrawHitpoints(sim_entity *Entity, render_group *Group)
{
  if(Entity->HitPointMax >= 1)
    {
      v2 HealthDim = {0.2f, 0.2f};
      r32 SpacingX = 1.5f*HealthDim.x;
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
	      Color.r = 0.2f;
	      Color.g = 0.2f;
	      Color.b = 0.2f;
	    }
	  PushRect(Group, HitP, 0, HealthDim, Color, 0.0f);
	  HitP = V2Add(HitP, dHitP);
	}
    }
}

internal sim_entity_collision_volume_group*
MakeSimpleGroundedCollision(state* State, r32 DimX, r32 DimY, r32 DimZ)
{
  sim_entity_collision_volume_group *Group = PushStruct(&State->WorldArena, sim_entity_collision_volume_group);
  Group->VolumeCount = 1;
  Group->Volumes = PushArray(&State->WorldArena, Group->VolumeCount, sim_entity_collision_volume);
  Group->TotalVolume.OffsetP = V3(0, 0, 0.5f*DimZ);
  Group->TotalVolume.Dim = V3(DimX, DimY, DimZ);
  Group->Volumes[0] = Group->TotalVolume;
  
  return(Group);
}

internal sim_entity_collision_volume_group*
MakeNullCollision(state *State)
{
  sim_entity_collision_volume_group *Group = PushStruct(&State->WorldArena, sim_entity_collision_volume_group);
  Group->VolumeCount = 0;
  Group->Volumes = 0;
  Group->TotalVolume.OffsetP = V3(0.0f, 0.0f, 0.0f);
  Group->TotalVolume.Dim = V3(0.0f, 0.0f, 0.0f);
    
  return(Group);  
}

internal void
FillGroundChunk(transient_state *TransState, state *State, ground_buffer *GroundBuffer,
		world_position* ChunkP)
{
  temporary_memory GroundMemory = BeginTemporaryMemory(&TransState->TransientArena);  
  render_group* RenderGroup = AllocateRenderGroup(&TransState->TransientArena, Megabytes(4),
							1.0f);
  Clear(RenderGroup, V4(1.0f, 1.0f, 0.0f, 1.0f));
  
  loaded_bitmap *Buffer = &GroundBuffer->Bitmap;

  GroundBuffer->P = *ChunkP;

  r32 Width  = (r32)Buffer->Width;
  r32 Height = (r32)Buffer->Height;  

  for(s32 ChunkOffsetY = -1;
      ChunkOffsetY <= 1;
      ++ChunkOffsetY)
    {
      for(s32 ChunkOffsetX = -1;
	  ChunkOffsetX <= 1;
	  ++ChunkOffsetX)
	{
	  s32 ChunkX = ChunkP->ChunkX + ChunkOffsetX;
	  s32 ChunkY = ChunkP->ChunkY + ChunkOffsetY;
	  s32 ChunkZ = ChunkP->ChunkZ;
  
	  random_series Series = Seed(139*ChunkX + 593*ChunkY + 329*ChunkZ);
	  
	  v2 Center = V2(ChunkOffsetX*Width, -ChunkOffsetY*Height);
	  for(u32 GrassIndex = 0;
	      GrassIndex < 100;
	      ++GrassIndex)
	    {
	      loaded_bitmap *Stamp;

	      if(RandomChoice(&Series, 2))
		{
		  Stamp = State->Grass + RandomChoice(&Series, ArrayCount(State->Grass));
		}
	      else
		{
		  Stamp = State->Stone + RandomChoice(&Series, ArrayCount(State->Stone));
		}

	      v2 BitmapCenter = V2MulS(0.5f, V2i(Stamp->Width, Stamp->Height));
      	      v2 Offset = {Width*RandomUnilateral(&Series),
			   Height*RandomUnilateral(&Series)};
	      v2 P = V2Add(Center,
			   V2Sub(Offset, BitmapCenter));
      
	      PushBitmap(RenderGroup, Stamp, P, 0.0f, V2(0.0f, 0.0f),
			 1.0f, 1.0f);
	    }
	}
    }
  
    for(s32 ChunkOffsetY = -1;
      ChunkOffsetY <= 1;
      ++ChunkOffsetY)
    {
      for(s32 ChunkOffsetX = -1;
	  ChunkOffsetX <= 1;
	  ++ChunkOffsetX)
	{
	  s32 ChunkX = ChunkP->ChunkX + ChunkOffsetX;
	  s32 ChunkY = ChunkP->ChunkY + ChunkOffsetY;
	  s32 ChunkZ = ChunkP->ChunkZ;
  
	  random_series Series = Seed(139*ChunkX + 593*ChunkY + 329*ChunkZ);
	  
	  v2 Center = V2(ChunkOffsetX*Width, -ChunkOffsetY*Height);
	  for(u32 TuftIndex = 0;
	      TuftIndex < 30;
	      ++TuftIndex)
	    {
	      loaded_bitmap *Stamp = State->Tuft + RandomChoice(&Series, ArrayCount(State->Tuft));

	      v2 BitmapCenter = V2MulS(0.5f, V2i(Stamp->Width, Stamp->Height));
	      v2 Offset = {Width*RandomUnilateral(&Series),
			   Height*RandomUnilateral(&Series)};
	      v2 P = V2Add(Center,
			   V2Sub(Offset, BitmapCenter));
      
	      PushBitmap(RenderGroup, Stamp, P, 0.0f, V2(0.0f, 0.0f),
			 1.0f, 1.0f);
	    }
	}
    }
    
    RenderGroupToOutput(RenderGroup, Buffer);
    EndTemporaryMemory(GroundMemory);
}

extern UPDATE_AND_RENDER(UpdateAndRender)
{    
  u32 GroundBufferWidth = 256;
  u32 GroundBufferHeight = 256;
        
  Assert(sizeof(state) <= Memory->PermanentStorageSize);
  state* State = (state*) Memory->PermanentStorage;

  if(!Memory->IsInitialized)
    {            
      //NOTE We will do this much differently once we have a system
      //     for the assets like fonts and bitmaps.
      /*InitializeArena(&State->BitmapArena, Megabytes(1),
	(u8*)Memory->PermanentStorage + sizeof(state));
	InitializeArena(&State->Font.GlyphArena, Memory->PermanentStorageSize - sizeof(state),
	(u8*)Memory->PermanentStorage + sizeof(state) + Megabytes(1));
      */
            
      u32 TilesPerWidth = 17;
      u32 TilesPerHeight = 9;

      State->TypicalFloorHeight = 3.0f;
      State->MetersToPixels = 42.0f;
      State->PixelsToMeters = 1.0f / State->MetersToPixels;

      v3 WorldChunkDimInMeters = V3(State->PixelsToMeters*(r32)GroundBufferWidth,
				    State->PixelsToMeters*(r32)GroundBufferHeight,
				    State->TypicalFloorHeight);
      
      InitializeArena(&State->WorldArena, Memory->PermanentStorageSize - sizeof(state),
		      (u8*)Memory->PermanentStorage + sizeof(state));
      
      AddLowEntity(State, EntityType_Null, NullPosition());
           
      State->World   = PushStruct(&State->WorldArena, world);
      world *World   = State->World;
      
      InitializeWorld(World, WorldChunkDimInMeters);
      
      random_series Series = Seed(1234);

      r32 TileSideInMeters = 1.4f;
      r32 TileDepthInMeters = State->TypicalFloorHeight;
      
      State->NullCollision     = MakeNullCollision(State);
      State->SwordCollision    = MakeSimpleGroundedCollision(State,
							     1.0f,
							     0.5f,
							     0.1f);
      State->StairCollision    = MakeSimpleGroundedCollision(State,
							     TileSideInMeters,
							     2.0f*TileSideInMeters,
							     1.1f*TileDepthInMeters);
      State->PlayerCollision   = MakeSimpleGroundedCollision(State,
							     1.0f,
							     0.5f,
							     1.2f);
      State->FamiliarCollision = MakeSimpleGroundedCollision(State,
							     1.0f,
							     0.5f,
							     0.5f);
      State->MonstarCollision  = MakeSimpleGroundedCollision(State,
							     1.0f,
							     0.5f,
							     0.5f);
      State->WallCollision     = MakeSimpleGroundedCollision(State,
							     TileSideInMeters,
							     TileSideInMeters,
							     TileDepthInMeters);
      State->StandardRoomCollision  = MakeSimpleGroundedCollision(State,
								  TilesPerWidth*TileSideInMeters,
								  TilesPerHeight*TileSideInMeters,
								  0.9f*TileDepthInMeters);
      
      State->Grass[0]
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test2/grass00.bmp");
      State->Grass[1]
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test2/grass01.bmp");

      State->Stone[0]
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test2/ground00.bmp");
      State->Stone[1]
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test2/ground01.bmp");
      State->Stone[2]
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test2/ground02.bmp");
      State->Stone[3]
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test2/ground03.bmp");

      State->Tuft[0]
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test2/tuft00.bmp");
      State->Tuft[1]
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test2/tuft01.bmp");
      State->Tuft[2]
	= DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "../data/test2/tuft02.bmp");
           
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
	  
	  //u32 DoorDirection = RandomChoice(&Series, (DoorUp || DoorDown) ? 2 : 3);
	  u32 DoorDirection = RandomChoice(&Series, 2);
	  
	  bool32 CreatedZDoor = false;
	  if(DoorDirection == 2)
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
	  else if (DoorDirection == 1)
	    {
	      DoorRight = true;
	    }
	  else
	    {
	      DoorTop = true;
	    }

	  AddStandardSpace(State,
			   ScreenX * TilesPerWidth  + TilesPerWidth/2,
			   ScreenY * TilesPerHeight + TilesPerHeight/2,
			   AbsTileZ);
	  
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
		      if((TileX == 10) && (TileY == 5))
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
	  	  
	  if(DoorDirection == 2)
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
	  else if(DoorDirection == 1)
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
	  s32 FamiliarOffsetX =  RandomBetweenS32(&Series, -7, 2);
	  s32 FamiliarOffsetY =  RandomBetweenS32(&Series, -3, 2);
	  AddFamiliar(State,
		      CameraTileX + FamiliarOffsetX,
		      CameraTileY + FamiliarOffsetY, CameraTileZ);
	}

      Memory->IsInitialized = true;
    }
  
  Assert(sizeof(transient_state) <= Memory->TransientStorageSize);
  transient_state* TransState
    = (transient_state*) Memory->TransientStorage;
  if(!TransState->IsInitialized)
    {
      InitializeArena(&TransState->TransientArena, Memory->TransientStorageSize - sizeof(transient_state),
		      (u8*)Memory->TransientStorage + sizeof(transient_state));

      TransState->GroundBufferCount = 64;//128;
      TransState->GroundBuffers = PushArray(&TransState->TransientArena,
					    TransState->GroundBufferCount,
					    ground_buffer);
      for(u32 GroundBufferIndex = 0;
	  GroundBufferIndex < TransState->GroundBufferCount;
	  ++GroundBufferIndex)
	{
	  ground_buffer* GroundBuffer = TransState->GroundBuffers + GroundBufferIndex;
	  GroundBuffer->Bitmap = MakeEmptyBitmap(&TransState->TransientArena,
						 GroundBufferWidth,
						 GroundBufferHeight,
						 false);
	  GroundBuffer->P = NullPosition();
	}
      
      TransState->IsInitialized = true;
    }

#if 0
  if(Input->ExecutableReloaded)
    {
      for(u32 GroundBufferIndex = 0;
	  GroundBufferIndex < TransState->GroundBufferCount;
	  ++GroundBufferIndex)
	{
	  ground_buffer* GroundBuffer = TransState->GroundBuffers + GroundBufferIndex;
	  GroundBuffer->P = NullPosition();	  
	}
    }
#endif
  
  world *World = State->World;
  r32 MetersToPixels = State->MetersToPixels;
  r32 PixelsToMeters = 1.0f/State->MetersToPixels;

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
		  ConHero->ddP.y = 1.0f;
		}
	      if(Controller->MoveDown.EndedDown)
		{
		  ConHero->ddP.y = -1.0f;
		}
	      if(Controller->MoveLeft.EndedDown)
		{
		  ConHero->ddP.x = -1.0f;
		}
	      if(Controller->MoveRight.EndedDown)
		{
		  ConHero->ddP.x = 1.0f;
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

  temporary_memory RenderMemory = BeginTemporaryMemory(&TransState->TransientArena);  
  render_group* RenderGroup = AllocateRenderGroup(&TransState->TransientArena, Megabytes(4),
						  MetersToPixels);
    
  loaded_bitmap DrawBuffer_ = {};
  loaded_bitmap *DrawBuffer = &DrawBuffer_;
  DrawBuffer->Width = Buffer->Width;
  DrawBuffer->Height = Buffer->Height;
  DrawBuffer->Pitch = Buffer->Pitch;
  DrawBuffer->Memory = Buffer->Memory;

  Clear(RenderGroup, V4(1.0f, 0.0f, 1.0f, 1.0f));
  
  v2 ScreenCenter = V2(0.5f * (r32)DrawBuffer->Width,
		       0.5f * (r32)DrawBuffer->Height);

  r32 ScreenWidthInMeters  = DrawBuffer->Width*PixelsToMeters;
  r32 ScreenHeightInMeters = DrawBuffer->Height*PixelsToMeters;
  rectangle3 CameraBoundsInMeters = RectCenterDim(V3(0,0,0),
						  V3(ScreenWidthInMeters,
						     ScreenHeightInMeters,
						     0.0f));
  
  for(u32 GroundBufferIndex = 0;
      GroundBufferIndex < TransState->GroundBufferCount;
      ++GroundBufferIndex)
    {
      ground_buffer* GroundBuffer = TransState->GroundBuffers + GroundBufferIndex;
      if(IsValid(GroundBuffer->P))
	{
	  loaded_bitmap* Bitmap = &GroundBuffer->Bitmap;
	  v3 Delta = Subtract(State->World,
			      &GroundBuffer->P, &State->CameraP);

	  PushBitmap(RenderGroup, Bitmap,
		     Delta.xy, Delta.z,
		     V2MulS(0.5f, V2i(Bitmap->Width, Bitmap->Height)),
		     1.0f, 1.0f);
	}
    }  
  
  {
    world_position MinChunkP =
      MapIntoChunkSpace(World, State->CameraP, GetMinCorner(CameraBoundsInMeters));
    world_position MaxChunkP =
      MapIntoChunkSpace(World, State->CameraP, GetMaxCorner(CameraBoundsInMeters));

    for(s32 ChunkZ = MinChunkP.ChunkZ;
	ChunkZ <= MaxChunkP.ChunkZ;
	++ChunkZ)
      {
	for(s32 ChunkY = MinChunkP.ChunkY;
	    ChunkY <= MaxChunkP.ChunkY;
	    ++ChunkY)
	  {
	    for(s32 ChunkX = MinChunkP.ChunkX;
		ChunkX <= MaxChunkP.ChunkX;
		++ChunkX)
	      {
		//world_chunk *Chunk = GetWorldChunk(World, ChunkX, ChunkY, ChunkZ, 0);
		//if(Chunk)
		  {
		    world_position ChunkCenterP
		      = CenteredChunkPoint(ChunkX, ChunkY, ChunkZ);
		    v3 RelP = Subtract(World, &ChunkCenterP, &State->CameraP);
		    
		    r32 FurthestBufferLengthSq = 0.0f;
		    ground_buffer *FurthestBuffer = 0;
		    for(u32 GroundBufferIndex = 0;
			GroundBufferIndex < TransState->GroundBufferCount;
			++GroundBufferIndex)
		      {
			ground_buffer* GroundBuffer = TransState->GroundBuffers + GroundBufferIndex; 
			if(AreInSameChunk(World, &GroundBuffer->P, &ChunkCenterP))
			  {
			    FurthestBuffer = 0;
			    break;
			  }
			else if(IsValid(GroundBuffer->P))
			  {
			    v3 RelP = Subtract(World, &GroundBuffer->P, &State->CameraP);
			    r32 BufferLengthSq = V2LengthSq(RelP.xy);
			    if(FurthestBufferLengthSq < BufferLengthSq)
			      {
				FurthestBufferLengthSq = BufferLengthSq;
				FurthestBuffer = GroundBuffer;
			      }
			  }
			else
			  {
			    FurthestBufferLengthSq = Real32Maximum;
			    FurthestBuffer = GroundBuffer;
			  }
		      }

		    if(FurthestBuffer)
		      {
			FillGroundChunk(TransState, State, FurthestBuffer, &ChunkCenterP);
		      }
		    
		    PushRectOutline(RenderGroup, RelP.xy, 0.0f,
				    World->ChunkDimInMeters.xy,
				    V4(1.0f, 1.0f, 0.0f, 1.0f),
				    1.0f);
		  }
	      }
	  }
      }
  }

  v3 SimBoundsExpansion = {15.0f, 15.0f, 15.0f};
  rectangle3 SimBounds = AddRadiusTo(CameraBoundsInMeters,
				     SimBoundsExpansion);
  
  temporary_memory SimMemory = BeginTemporaryMemory(&TransState->TransientArena);
  sim_region *SimRegion = BeginSim(&TransState->TransientArena, State, World, State->CameraP, SimBounds, Input->dtForFrame);

  for(u32 EntityIndex = 0;
      EntityIndex < SimRegion->EntityCount;
      ++EntityIndex)
    {
      sim_entity *Entity = SimRegion->Entities + EntityIndex;
      if(Entity->Updatable)
	{
	  r32 dt  = Input->dtForFrame;

	  r32 ShadowAlpha = 1.0f - 0.5f * Entity->P.z;
	  if(ShadowAlpha < 0)
	    {
	      ShadowAlpha = 0.0f;
	    }

	  move_spec MoveSpec = DefaultMoveSpec();
	  v3 ddP = {};

	  render_basis *Basis = PushStruct(&TransState->TransientArena, render_basis);
	  RenderGroup->DefaultBasis = Basis;
	  
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
			    Entity->dP.z = ConHero->dZ;
			  }
		    
			MoveSpec.UnitMaxAccelVector = true;
			MoveSpec.Speed = 50.0f;
			MoveSpec.Drag = 8.0f;
			ddP = ToV3(ConHero->ddP, 0);

			if((ConHero->dSword.x != 0.0f) ||
			   (ConHero->dSword.y != 0.0f))
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
	  
		PushBitmap(RenderGroup, &State->Shadow, V2(0, 0), 0 ,Hero->Align, ShadowAlpha, 0.0f);
		PushBitmap(RenderGroup, &Hero->Torso, V2(0, 0), 0 ,Hero->Align, 1.0f, 1.0f);
		PushBitmap(RenderGroup, &Hero->Cape,  V2(0, 0), 0 ,Hero->Align, 1.0f, 1.0f);
		PushBitmap(RenderGroup, &Hero->Head,  V2(0, 0), 0 ,Hero->Align, 1.0f, 1.0f);	

		DrawHitpoints(Entity, RenderGroup);
	    	    
	      } break;
	    case EntityType_Wall:
	      {
		PushBitmap(RenderGroup, &State->Tree, V2(0, 0), 0 ,V2(40, 80), 1.0f, 1.0f);
	      } break;
	    case EntityType_Stairwell:
	      {
		PushRect(RenderGroup, V2(0,0), 0, Entity->WalkableDim, V4(1, 0.5f, 0, 1), 0.0f);
		PushRect(RenderGroup, V2(0,0), Entity->WalkableHeight, Entity->WalkableDim, V4(1, 1, 0, 1), 0.0f);
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

		PushBitmap(RenderGroup, &State->Shadow, V2(0, 0), 0 ,Hero->Align, ShadowAlpha, 0.0f);
		PushBitmap(RenderGroup, &State->Sword, V2(0, 0), 0 ,V2(29, 10), 1.0f, 1.0f);
	      } break;
	    case EntityType_Familiar:
	      {
		sim_entity *ClosestHero = 0;
		r32 ClosestHeroDSq = Square(10.0f);

#if 0
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
#endif
		
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

		PushBitmap(RenderGroup, &State->Shadow, V2(0, 0), 0, Hero->Align, (0.5f*ShadowAlpha) + 0.2f*BobSin, 0.0f);
		PushBitmap(RenderGroup, &Hero->Head, V2(0, 0), 0.25f*BobSin, Hero->Align, 1.0f, 1.0f);
	      } break;
	    case EntityType_Monstar:
	      {
		DrawHitpoints(Entity, RenderGroup);

		PushBitmap(RenderGroup, &State->Shadow, V2(0, 0), 0 ,Hero->Align, ShadowAlpha, 1.0f);
		PushBitmap(RenderGroup, &Hero->Torso, V2(0, 0), 0 ,Hero->Align, 1.0f, 1.0f);
	      } break;
	      
	    case EntityType_Space:
	      {
		for(u32 VolumeIndex = 0;
		    VolumeIndex < Entity->Collision->VolumeCount;
		    ++VolumeIndex)
		  {
		    sim_entity_collision_volume *Volume = Entity->Collision->Volumes + VolumeIndex;
		    PushRectOutline(RenderGroup, Volume->OffsetP.xy, 0, Volume->Dim.xy, V4(0, 0.5f, 1, 1), 0.0f);    
		  }
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

	  Basis->P = GetEntityGroundPointWithoutP(Entity);
	}
    }

  State->Time += Input->dtForFrame;
  r32 Angle = 0.1f * State->Time;
  r32 Displacement = 100.0f*Cos(5.0f*Angle);
  
  v2 Origin = ScreenCenter;
#if 1
  v2 XAxis = V2MulS(100.0f, V2(Cos(Angle), Sin(Angle)));
  v2 YAxis = V2Perp(XAxis);
#else
  v2 XAxis = V2(100.0f, 0.0f);
  v2 YAxis = V2(0.0f, 100.0f);
#endif
  u32 PIndex = 0;
  r32 CAngle = 5.0f * Angle;
  v4 Color = V4(0.5f + 0.5f*Sin(CAngle),
		0.5f + 0.5f*Sin(2.9*CAngle),
		0.5f + 0.5f*Cos(9.9*CAngle),
	        0.5f + 0.5f*Sin(10.0*CAngle));
  render_entry_coordinate_system *C = CoordinateSystem(RenderGroup,
						       V2Add(V2(Displacement, 0.0f),
							     V2Sub(V2Sub(Origin, V2MulS(0.5, XAxis)),
								   V2MulS(0.5, YAxis))),
						       XAxis,
						       YAxis,
						       Color,
						       &State->Tree);
  
  for(r32 Y = 0.0f;
      Y < 1.0f;
      Y += 0.25f)
    {
      for(r32 X = 0.0f;
	  X < 1.0f;
	  X += 0.25f)
	{
	  C->Points[PIndex++] = V2(X, Y);
	}
    }
  
  RenderGroupToOutput(RenderGroup, DrawBuffer);
    
  EndSim(SimRegion, State);
  EndTemporaryMemory(SimMemory);
  EndTemporaryMemory(RenderMemory);
 
  CheckArena(&State->WorldArena);
  CheckArena(&TransState->TransientArena);
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
