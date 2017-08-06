
typedef struct
{  
  u32 AbsTileX;
  u32 AbsTileY;
  u32 AbsTileZ;

  r32 OffsetX;
  r32 OffsetY;
} tile_map_position; 

typedef struct
{
  u32 *Tiles;
} tile_chunk;

typedef struct
{
  u32 TileChunkX;
  u32 TileChunkY;
  u32 TileChunkZ;
  
  u32 RelTileX;
  u32 RelTileY;
} tile_chunk_position; 

typedef struct
{
  r32 TileSideInMeters;

  u32 ChunkShift;
  u32 ChunkMask;
  u32 ChunkDim;  

  u32 TileChunkCountX;
  u32 TileChunkCountY;
  u32 TileChunkCountZ;
  
  tile_chunk *TileChunks;
} tile_map;
