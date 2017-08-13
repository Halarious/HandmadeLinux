
internal inline tile_chunk_position
GetChunkPositionFor(tile_map *TileMap,
		    s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ)
{
  tile_chunk_position Result = {};
  
  Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
  Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
  Result.TileChunkZ = AbsTileZ;
  Result.RelTileX = AbsTileX & TileMap->ChunkMask;
  Result.RelTileY = AbsTileY & TileMap->ChunkMask;

  return(Result);
}

#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX/64)
#define TILE_CHUNK_UNINITIALIZED INT32_MAX

inline internal tile_chunk*
GetTileChunk(tile_map *TileMap, s32 TileChunkX, s32 TileChunkY, s32 TileChunkZ,
	     memory_arena *Arena)
{
  Assert(TileChunkX > -TILE_CHUNK_SAFE_MARGIN);
  Assert(TileChunkY > -TILE_CHUNK_SAFE_MARGIN);
  Assert(TileChunkZ > -TILE_CHUNK_SAFE_MARGIN);
  Assert(TileChunkX < TILE_CHUNK_SAFE_MARGIN);
  Assert(TileChunkY < TILE_CHUNK_SAFE_MARGIN);
  Assert(TileChunkZ < TILE_CHUNK_SAFE_MARGIN);
  
  u32 HashValue = 19*TileChunkX + 7*TileChunkY + 3*TileChunkZ;
  u32 HashSlot = HashValue & (ArrayCount(TileMap->TileChunkHash) - 1);
  Assert(HashSlot < ArrayCount(TileMap->TileChunkHash));

  tile_chunk *Chunk = TileMap->TileChunkHash + HashSlot;
  do
    {
      if((TileChunkX == Chunk->TileChunkX) &&
	 (TileChunkY == Chunk->TileChunkY) &&
	 (TileChunkZ == Chunk->TileChunkZ))
	{
	  break;
	}
      if(Arena && (Chunk->TileChunkX != TILE_CHUNK_UNINITIALIZED)  && (!Chunk->NextInHash))
	{
	  Chunk->NextInHash = PushStruct(Arena, tile_chunk);
	  Chunk->NextInHash->TileChunkX = TILE_CHUNK_UNINITIALIZED;
	  Chunk = Chunk->NextInHash;
	}
            
      if(Arena && (Chunk->TileChunkX == TILE_CHUNK_UNINITIALIZED))
	{
	  u32 TileCount = TileMap->ChunkDim * TileMap->ChunkDim;

	  Chunk->TileChunkX = TileChunkX;
	  Chunk->TileChunkY = TileChunkY;
	  Chunk->TileChunkZ = TileChunkZ;

	  Chunk->Tiles = PushArray(Arena, TileCount, u32);
	  for(u32 TileIndex = 0;
	      TileIndex < TileCount;
	      ++TileIndex)
	    {
	      Chunk->Tiles[TileIndex] = 1;
	    }

	  Chunk->NextInHash = 0;
	  break;
	}

      Chunk = Chunk->NextInHash;
    } while(Chunk);

  return(Chunk);
}

inline internal u32
GetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, s32 TileX, s32 TileY)
{
  Assert(TileChunk);
  Assert((TileX < TileMap->ChunkDim) &&
	 (TileY < TileMap->ChunkDim))
  u32 TileChunkValue = TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX];  
  return(TileChunkValue);
}

inline internal void
SetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, s32 TileX, s32 TileY,
		      u32 TileValue)
{
  Assert(TileChunk);
  Assert((TileX < TileMap->ChunkDim) &&
	 (TileY < TileMap->ChunkDim));
  TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX] = TileValue;  
}

internal void
_SetTileValue(tile_map *TileMap, tile_chunk *TileChunk,
	      u32 TestTileX, u32 TestTileY,
	      u32 TileValue)
{
  if(TileChunk && TileChunk->Tiles)
    {      
      SetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY, TileValue);      
    }
}

internal u32
_GetTileValue(tile_map *TileMap, tile_chunk *TileChunk,
	      u32 TestTileX, u32 TestTileY)
{
  u32 TileChunkValue = 0;
  if(TileChunk && TileChunk->Tiles)
    {      
      TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);      
    }
  return(TileChunkValue);
}

internal u32
GetTileValueC(tile_map *TileMap, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ)
{
  tile_chunk_position ChunkPosition = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
  tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY, ChunkPosition.TileChunkZ, 0);
  u32 TileChunkValue = _GetTileValue(TileMap, TileChunk, ChunkPosition.RelTileX, ChunkPosition.RelTileY);
    
  return(TileChunkValue);
}

internal u32
GetTileValueP(tile_map *TileMap, tile_map_position Position)
{
  u32 TileChunkValue = GetTileValueC(TileMap, Position.AbsTileX, Position.AbsTileY, Position.AbsTileZ);  
  return(TileChunkValue);
}

internal bool32
IsTileValueEmpty(u32 TileValue)
{
  bool32 IsEmpty = ((TileValue == 1) ||
		    (TileValue == 3) ||
		    (TileValue == 4));
  return(IsEmpty);
}

internal bool32
IsTileMapPointEmpty(tile_map *TileMap, tile_map_position Position)
{
  u32 TileChunkValue = GetTileValueP(TileMap, Position);
  bool32 IsEmpty     = IsTileValueEmpty(TileChunkValue);
  
  return(IsEmpty);
}

internal void
SetTileValue(memory_arena *Arena, tile_map *TileMap,
	     s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ,
	     u32 TileValue)
{
  tile_chunk_position ChunkPosition = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
  tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY, ChunkPosition.TileChunkZ, Arena);
  
  _SetTileValue(TileMap, TileChunk, ChunkPosition.RelTileX, ChunkPosition.RelTileY, TileValue);
}

internal void
InitializeTileMap(tile_map *TileMap, r32 TileSideInMeters)
{
  TileMap->ChunkShift = 4;
  TileMap->ChunkMask  = (1 << TileMap->ChunkShift) - 1;  
  TileMap->ChunkDim   = (1 << TileMap->ChunkShift);
  TileMap->TileSideInMeters = TileSideInMeters;

  for(u32 TileChunkIndex = 0;
      TileChunkIndex < ArrayCount(TileMap->TileChunkHash);
      ++TileChunkIndex)
    {
      TileMap->TileChunkHash[TileChunkIndex].TileChunkX = TILE_CHUNK_UNINITIALIZED;
    }
}

internal inline void
RecanonicalizeCoord(tile_map *TileMap, s32 *Tile, r32 *TileRel)
{
  //NOTE: Apparently the world is assumed to be toroidal
  s32 Offset = RoundReal32ToInt32(*TileRel / TileMap->TileSideInMeters);
  *Tile += Offset;
  *TileRel -= Offset*TileMap->TileSideInMeters;
  
  Assert(*TileRel > -0.5f * TileMap->TileSideInMeters);
  Assert(*TileRel <  0.5f * TileMap->TileSideInMeters);
}

internal inline tile_map_position
MapIntoTileSpace(tile_map *TileMap, tile_map_position BasePosition, v2 Offset)
{
  tile_map_position Result = BasePosition;

  Result.Offset_ = VAdd(Result.Offset_, Offset);
  RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.Offset_.X);
  RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Offset_.Y);

  return(Result);
}

internal inline bool32
AreOnSameTile(tile_map_position *A, tile_map_position *B)
{
  bool32 Result = ((A->AbsTileX == B->AbsTileX) &&
		   (A->AbsTileY == B->AbsTileY) &&
		   (A->AbsTileZ == B->AbsTileZ));
  return(Result);
}

internal inline tile_map_difference
Subtract(tile_map *TileMap, tile_map_position *A, tile_map_position* B)
{
  tile_map_difference Result = {};

  v2 dTileXY = (v2){(r32)A->AbsTileX - (r32)B->AbsTileX,
		    (r32)A->AbsTileY - (r32)B->AbsTileY};
  r32 dTileZ = (r32)A->AbsTileZ - (r32)B->AbsTileZ;
  
  Result.dXY = VAdd(VMulS(TileMap->TileSideInMeters,
			  dTileXY),
		    VSub(A->Offset_, B->Offset_));
  
  Result.dZ = dTileZ * TileMap->TileSideInMeters + 0.0f;

  return(Result);
}

internal inline tile_map_position
CenteredTilePoint(s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ)
{
  tile_map_position Result = {};

  Result.AbsTileX = AbsTileX;
  Result.AbsTileY = AbsTileY;
  Result.AbsTileZ = AbsTileZ;
  
  return(Result);
}
