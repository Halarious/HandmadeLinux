
#define WORLD_CHUNK_SAFE_MARGIN (INT32_MAX/64)
#define WORLD_CHUNK_UNINITIALIZED INT32_MAX
#define TILES_PER_CHUNK 16

internal inline world_position
NullPosition()
{
  world_position Result = {};

  Result.ChunkX = WORLD_CHUNK_UNINITIALIZED;
  Result.ChunkY = WORLD_CHUNK_UNINITIALIZED;
  Result.ChunkZ = WORLD_CHUNK_UNINITIALIZED;
  
  return(Result);
}

internal inline bool32
IsValid(world_position P)
{
  bool32 Result = (P.ChunkX != WORLD_CHUNK_UNINITIALIZED);
  return(Result);
}

internal inline bool32
IsCannonical(r32 ChunkDim, r32 TileRel)
{
  r32 Epsilon = 0.0001f;
  bool32 Result = ((TileRel >= -(0.5f * ChunkDim + Epsilon)) &&
		   (TileRel <=  (0.5f * ChunkDim + Epsilon)));
  return(Result);
}

internal inline bool32
IsCannonicalV(world* World, v3 Offset)
{
  bool32 Result = ((IsCannonical(World->ChunkDimInMeters.X, Offset.X)) &&
		   (IsCannonical(World->ChunkDimInMeters.Y, Offset.Y)) &&
		   (IsCannonical(World->ChunkDimInMeters.Z, Offset.Z)));
  return(Result);
}

internal inline bool32
AreInSameChunk(world *World, world_position *A, world_position *B)
{
  Assert(IsCannonicalV(World, A->Offset_));
  Assert(IsCannonicalV(World, B->Offset_));

  bool32 Result = ((A->ChunkX == B->ChunkX) &&
		   (A->ChunkY == B->ChunkY) &&
		   (A->ChunkZ == B->ChunkZ));
  return(Result);
}

inline internal world_chunk*
GetWorldChunk(world *World, s32 ChunkX, s32 ChunkY, s32 ChunkZ,
	     memory_arena *Arena)
{
  Assert(ChunkX > -WORLD_CHUNK_SAFE_MARGIN);
  Assert(ChunkY > -WORLD_CHUNK_SAFE_MARGIN);
  Assert(ChunkZ > -WORLD_CHUNK_SAFE_MARGIN);
  Assert(ChunkX < WORLD_CHUNK_SAFE_MARGIN);
  Assert(ChunkY < WORLD_CHUNK_SAFE_MARGIN);
  Assert(ChunkZ < WORLD_CHUNK_SAFE_MARGIN);
  
  u32 HashValue = 19*ChunkX + 7*ChunkY + 3*ChunkZ;
  u32 HashSlot = HashValue & (ArrayCount(World->ChunkHash) - 1);
  Assert(HashSlot < ArrayCount(World->ChunkHash));

  world_chunk *Chunk = World->ChunkHash + HashSlot;
  do
    {
      if((ChunkX == Chunk->ChunkX) &&
	 (ChunkY == Chunk->ChunkY) &&
	 (ChunkZ == Chunk->ChunkZ))
	{
	  break;
	}
      if(Arena && (Chunk->ChunkX != WORLD_CHUNK_UNINITIALIZED)  && (!Chunk->NextInHash))
	{
	  Chunk->NextInHash = PushStruct(Arena, world_chunk);
	  Chunk->NextInHash->ChunkX = WORLD_CHUNK_UNINITIALIZED;
	  Chunk = Chunk->NextInHash;
	}
            
      if(Arena && (Chunk->ChunkX == WORLD_CHUNK_UNINITIALIZED))
	{
	  Chunk->ChunkX = ChunkX;
	  Chunk->ChunkY = ChunkY;
	  Chunk->ChunkZ = ChunkZ;

	  Chunk->NextInHash = 0;
	  break;
	}

      Chunk = Chunk->NextInHash;
    } while(Chunk);

  return(Chunk);
}

internal void
InitializeWorld(world *World, r32 TileSideInMeters)
{
  World->TileSideInMeters  = TileSideInMeters;
  World->TileDepthInMeters = TileSideInMeters;
  World->ChunkDimInMeters = V3((r32)TILES_PER_CHUNK*TileSideInMeters,
			       (r32)TILES_PER_CHUNK*TileSideInMeters,
			       (r32)TileSideInMeters);
  World->FirstFree = 0;
  
  for(u32 ChunkIndex = 0;
      ChunkIndex < ArrayCount(World->ChunkHash);
      ++ChunkIndex)
    {
      World->ChunkHash[ChunkIndex].ChunkX = WORLD_CHUNK_UNINITIALIZED;
      World->ChunkHash[ChunkIndex].FirstBlock.EntityCount = 0;
    }
}

internal inline void
RecanonicalizeCoord(r32 ChunkDim, s32 *Tile, r32 *TileRel)
{
  //NOTE: Apparently the world is assumed to be toroidal
  s32 Offset = RoundReal32ToInt32(*TileRel / ChunkDim);
  *Tile += Offset;
  *TileRel -= Offset*ChunkDim;

  Assert(IsCannonical(ChunkDim, *TileRel));
}

internal inline world_position
MapIntoChunkSpace(world *World, world_position BasePosition, v3 Offset)
{
  world_position Result = BasePosition;

  Result.Offset_ = V3Add(Result.Offset_, Offset);
  RecanonicalizeCoord(World->ChunkDimInMeters.X, &Result.ChunkX, &Result.Offset_.X);
  RecanonicalizeCoord(World->ChunkDimInMeters.Y, &Result.ChunkY, &Result.Offset_.Y);
  RecanonicalizeCoord(World->ChunkDimInMeters.Z, &Result.ChunkY, &Result.Offset_.Z);

  return(Result);
}

internal world_position
ChunkPositionFromTilePosition(world *World,
			      s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ)
{
  world_position BasePos = {};

  v3 Offset = V3Hadamard(World->ChunkDimInMeters,
			 V3((r32)AbsTileX, (r32)AbsTileY, (r32)AbsTileZ));
  world_position Result = MapIntoChunkSpace(World, BasePos, Offset);

  Assert(IsCannonicalV(World, Result.Offset_));
  
  return(Result);
}

internal inline v3
Subtract(world *World, world_position *A, world_position* B)
{
  v3 dTile = (v3){(r32)A->ChunkX - (r32)B->ChunkX,
		  (r32)A->ChunkY - (r32)B->ChunkY,
		  (r32)A->ChunkZ - (r32)B->ChunkZ};
    
  v3 Result = V3Add(V3Hadamard(World->ChunkDimInMeters,
			       dTile),
		    V3Sub(A->Offset_, B->Offset_));
  
  return(Result);
}

internal inline world_position
CenteredChunkPoint(s32 ChunkX, s32 ChunkY, s32 ChunkZ)
{
  world_position Result = {};

  Result.ChunkX = ChunkX;
  Result.ChunkY = ChunkY;
  Result.ChunkZ = ChunkZ;
  
  return(Result);
}

internal void
ChangeEntityLocationRaw(memory_arena *Arena, world *World, u32 LowEntityIndex,
			world_position *OldP, world_position *NewP)
{
  Assert(!OldP || IsValid(*OldP));
  Assert(!NewP || IsValid(*NewP));
  
  if(OldP && NewP && AreInSameChunk(World, OldP, NewP))
    {
    }
  else
    {
      if(OldP)
	{
	  world_chunk *Chunk = GetWorldChunk(World, OldP->ChunkX, OldP->ChunkY, OldP->ChunkZ, 0);
	  Assert(Chunk);
	  if(Chunk)
	    {
	      bool32 NotFound = true;
	      world_entity_block *FirstBlock = &Chunk->FirstBlock; 
	      for(world_entity_block *Block = FirstBlock;
		  Block && NotFound;
		  Block = Block->Next)
		{
		  for(u32 Index = 0;
		      (Index < Block->EntityCount) && NotFound;
		      ++Index)
		    {
		      if(Block->LowEntityIndex[Index] == LowEntityIndex)
			{
			  Assert(FirstBlock->EntityCount > 0);
			  Block->LowEntityIndex[Index] =
			    FirstBlock->LowEntityIndex[--FirstBlock->EntityCount];
			  if(FirstBlock->EntityCount == 0)
			    {
			      if(FirstBlock->Next)
				{
				  world_entity_block *NextBlock = FirstBlock->Next;
				  *FirstBlock = *NextBlock;

				  NextBlock->Next = World->FirstFree;
				  World->FirstFree = NextBlock;
				}
			    }
			  
			  NotFound = false;
			}
		    }
		}
	    }
	}
      if(NewP)
	{
	  world_chunk *Chunk = GetWorldChunk(World, NewP->ChunkX, NewP->ChunkY, NewP->ChunkZ, Arena);
	  Assert(Chunk);

	  world_entity_block *Block = &Chunk->FirstBlock;
	  if(Block->EntityCount == ArrayCount(Block->LowEntityIndex))
	    {
	      world_entity_block *OldBlock = World->FirstFree;
	      if(OldBlock)
		{
		  World->FirstFree = OldBlock->Next;
		}
	      else
		{
		  OldBlock = PushStruct(Arena, world_entity_block);
		}
	  
	      *OldBlock = *Block;
	      Block->Next = OldBlock;
	      Block->EntityCount = 0;
	    }

	  Assert(Block->EntityCount < ArrayCount(Block->LowEntityIndex));
	  Block->LowEntityIndex[Block->EntityCount++] = LowEntityIndex;
	}
    }
}


internal void
ChangeEntityLocation(memory_arena *Arena, world *World,
		     u32 LowEntityIndex, low_entity *LowEntity,
		     world_position NewPInit)
{
  world_position *OldP = 0;
  world_position *NewP = 0;

  if(!IsSet(&LowEntity->Sim, EntityFlag_Nonspatial) &&
     IsValid(LowEntity->P))
    {
      OldP = &LowEntity->P;
    }
  
  if(IsValid(NewPInit))
    {
      NewP = &NewPInit;
    }
  
  ChangeEntityLocationRaw(Arena, World, LowEntityIndex, OldP, NewP);
  if(NewP)
    {
      LowEntity->P = *NewP;
      ClearFlag(&LowEntity->Sim, EntityFlag_Nonspatial);
    }
  else
    {
      LowEntity->P = NullPosition();
      AddFlag(&LowEntity->Sim, EntityFlag_Nonspatial);
    }
}

