
internal inline void
RecanonicalizeCoord(tile_map *TileMap, u32 *Tile, r32 *TileRel)
{
  //NOTE: Apparently the world is assumed to be toroidal
  s32 Offset = RoundReal32ToInt32(*TileRel / TileMap->TileSideInMeters);
  *Tile += Offset;
  *TileRel -= Offset*TileMap->TileSideInMeters;
  
  Assert(*TileRel >= -0.5f * TileMap->TileSideInMeters);
  Assert(*TileRel <=  0.5f * TileMap->TileSideInMeters);
}

internal inline tile_map_position
RecanonicalizePosition(tile_map *TileMap, tile_map_position Position)
{
  tile_map_position Result = Position;

  RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.TileRelX);
  RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.TileRelY);

  return(Result);
}

internal inline tile_chunk_position
GetChunkPositionFor(tile_map *TileMap, u32 AbsTileX, u32 AbsTileY)
{
  tile_chunk_position Result = {};
  
  Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
  Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
  Result.RelTileX = AbsTileX & TileMap->ChunkMask;
  Result.RelTileY = AbsTileY & TileMap->ChunkMask;

  return(Result);
}

inline internal tile_chunk*
GetTileChunk(tile_map *TileMap, s32 TileChunkX, s32 TileChunkY)
{
  tile_chunk *TileChunk = 0;
  if((TileChunkX >= 0) && (TileChunkX < TileMap->TileChunkCountX) &&
     (TileChunkY >= 0) && (TileChunkY < TileMap->TileChunkCountY))
    {
      TileChunk = &TileMap->TileChunks[TileChunkY*TileMap->TileChunkCountX + TileChunkX];  
    }
  
  return(TileChunk);
}

inline internal u32
GetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, u32 TileX, u32 TileY)
{
  Assert(TileChunk);
  Assert((TileX < TileMap->ChunkDim) &&
	 (TileY < TileMap->ChunkDim))
  u32 TileChunkValue = TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX];  
  return(TileChunkValue);
}

inline internal void
SetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, u32 TileX, u32 TileY,
		      u32 TileValue)
{
  Assert(TileChunk);
  Assert((TileX < TileMap->ChunkDim) &&
	 (TileY < TileMap->ChunkDim));
  TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX] = TileValue;  
}

internal u32
_GetTileValue(tile_map *TileMap, tile_chunk *TileChunk, u32 TestTileX, u32 TestTileY)
{
  u32 TileChunkValue = 0;
  if(TileChunk)
    {      
      TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);      
    }
  return(TileChunkValue);
}

internal void
_SetTileValue(tile_map *TileMap, tile_chunk *TileChunk, u32 TestTileX, u32 TestTileY,
	      u32 TileValue)
{
  if(TileChunk)
    {      
      SetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY, TileValue);      
    }
}

internal u32
GetTileValue(tile_map *TileMap, u32 AbsTileX, u32 AbsTileY)
{
  tile_chunk_position ChunkPosition = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY);
  tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY);
  u32 TileChunkValue = _GetTileValue(TileMap, TileChunk, ChunkPosition.RelTileX, ChunkPosition.RelTileY);
    
  return(TileChunkValue);
}

internal bool32
IsTileMapPointEmpty(tile_map *TileMap, tile_map_position CanPos)
{
  u32 TileChunkValue = GetTileValue(TileMap, CanPos.AbsTileX, CanPos.AbsTileY);
  bool32 IsEmpty = (TileChunkValue == 0);
  
  return(IsEmpty);
}

internal void
SetTileValue(memory_arena *Arena, tile_map *TileMap, u32 AbsTileX, u32 AbsTileY, u32 TileValue)
{
  tile_chunk_position ChunkPosition = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY);
  tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY);
  
  Assert(TileChunk)
    {
      _SetTileValue(TileMap, TileChunk, ChunkPosition.RelTileX, ChunkPosition.RelTileY, TileValue);
    }
}
