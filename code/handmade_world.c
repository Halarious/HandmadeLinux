
#define WORLD_CHUNK_SAFE_MARGIN (INT32_MAX/64)
#define WORLD_CHUNK_UNINITIALIZED INT32_MAX

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
	  u32 WorldCount = World->ChunkDim * World->ChunkDim;

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

#if 0
internal inline world_position
GetChunkPositionFor(world *World,
		    s32 AbsWorldX, s32 AbsWorldY, s32 AbsWorldZ)
{
  world_chunk_position Result = {};
  
  Result.ChunkX = AbsWorldX >> World->ChunkShift;
  Result.ChunkY = AbsWorldY >> World->ChunkShift;
  Result.ChunkZ = AbsWorldZ;
  Result.RelWorldX = AbsWorldX & World->ChunkMask;
  Result.RelWorldY = AbsWorldY & World->ChunkMask;

  return(Result);
}
#endif

internal void
InitializeWorld(world *World, r32 TileSideInMeters)
{
  World->ChunkShift = 4;
  World->ChunkMask  = (1 << World->ChunkShift) - 1;  
  World->ChunkDim   = (1 << World->ChunkShift);
  World->TileSideInMeters = TileSideInMeters;

  for(u32 ChunkIndex = 0;
      ChunkIndex < ArrayCount(World->ChunkHash);
      ++ChunkIndex)
    {
      World->ChunkHash[ChunkIndex].ChunkX = WORLD_CHUNK_UNINITIALIZED;
    }
}

internal inline void
RecanonicalizeCoord(world *World, s32 *Tile, r32 *TileRel)
{
  //NOTE: Apparently the world is assumed to be toroidal
  s32 Offset = RoundReal32ToInt32(*TileRel / World->TileSideInMeters);
  *Tile += Offset;
  *TileRel -= Offset*World->TileSideInMeters;
  
  Assert(*TileRel > -0.5f * World->TileSideInMeters);
  Assert(*TileRel <  0.5f * World->TileSideInMeters);
}

internal inline world_position
MapIntoWorldSpace(world *World, world_position BasePosition, v2 Offset)
{
  world_position Result = BasePosition;

  Result.Offset_ = VAdd(Result.Offset_, Offset);
  RecanonicalizeCoord(World, &Result.AbsTileX, &Result.Offset_.X);
  RecanonicalizeCoord(World, &Result.AbsTileY, &Result.Offset_.Y);

  return(Result);
}

internal inline bool32
AreOnSameTile(world_position *A, world_position *B)
{
  bool32 Result = ((A->AbsTileX == B->AbsTileX) &&
		   (A->AbsTileY == B->AbsTileY) &&
		   (A->AbsTileZ == B->AbsTileZ));
  return(Result);
}

internal inline world_difference
Subtract(world *World, world_position *A, world_position* B)
{
  world_difference Result = {};

  v2 dTileXY = (v2){(r32)A->AbsTileX - (r32)B->AbsTileX,
		    (r32)A->AbsTileY - (r32)B->AbsTileY};
  r32 dTileZ = (r32)A->AbsTileZ - (r32)B->AbsTileZ;
  
  Result.dXY = VAdd(VMulS(World->TileSideInMeters,
			  dTileXY),
		    VSub(A->Offset_, B->Offset_));
  
  Result.dZ = dTileZ * World->TileSideInMeters + 0.0f;

  return(Result);
}

internal inline world_position
CenteredTilePoint(s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ)
{
  world_position Result = {};

  Result.AbsTileX = AbsTileX;
  Result.AbsTileY = AbsTileY;
  Result.AbsTileZ = AbsTileZ;
  
  return(Result);
}
