
/*

#if 0
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
#endif

 */
