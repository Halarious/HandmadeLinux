
internal inline tile_chunk_position
GetChunkPositionFor(tile_map *TileMap,
		    u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ)
{
  tile_chunk_position Result = {};
  
  Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
  Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
  Result.TileChunkZ = AbsTileZ;
  Result.RelTileX = AbsTileX & TileMap->ChunkMask;
  Result.RelTileY = AbsTileY & TileMap->ChunkMask;

  return(Result);
}

inline internal tile_chunk*
GetTileChunk(tile_map *TileMap, s32 TileChunkX, s32 TileChunkY, s32 TileChunkZ)
{
  tile_chunk *TileChunk = 0;
  if((TileChunkX >= 0) && (TileChunkX < TileMap->TileChunkCountX) &&
     (TileChunkY >= 0) && (TileChunkY < TileMap->TileChunkCountY) &&
     (TileChunkZ >= 0) && (TileChunkZ < TileMap->TileChunkCountZ))
    {
      TileChunk = &TileMap->TileChunks[
				       TileChunkZ*TileMap->TileChunkCountY*TileMap->TileChunkCountX +
				       TileChunkY*TileMap->TileChunkCountX +
				       TileChunkX];  
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
  tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY, ChunkPosition.TileChunkZ);
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
	     u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ,
	     u32 TileValue)
{
  tile_chunk_position ChunkPosition = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
  tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY, ChunkPosition.TileChunkZ);
  
  Assert(TileChunk);
  if(!TileChunk->Tiles)
    {
      u32 TileCount = TileMap->ChunkDim * TileMap->ChunkDim;
      TileChunk->Tiles = PushArray(Arena, TileCount, u32);
      for(u32 TileIndex = 0;
	  TileIndex < TileCount;
	  ++TileIndex)
	{
	  TileChunk->Tiles[TileIndex] = 1;
	}
    }
  
  _SetTileValue(TileMap, TileChunk, ChunkPosition.RelTileX, ChunkPosition.RelTileY, TileValue);
}

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

  RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.Offset.X);
  RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Offset.Y);

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

  v2 dTileXY = {(r32)A->AbsTileX - (r32)B->AbsTileX,
		(r32)A->AbsTileY - (r32)B->AbsTileY};
  r32 dTileZ = (r32)A->AbsTileZ - (r32)B->AbsTileZ;
  
  Result.dXY = VAdd(VMulS(TileMap->TileSideInMeters,
			  dTileXY),
		    VSub(A->Offset, B->Offset));
  
  Result.dZ = dTileZ * TileMap->TileSideInMeters + 0.0f;

  return(Result);
}

internal inline tile_map_position
CenteredTilePoint(u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ)
{
  tile_map_position Result = {};

  Result.AbsTileX = AbsTileX;
  Result.AbsTileY = AbsTileY;
  Result.AbsTileZ = AbsTileZ;
  
  return(Result);
}
