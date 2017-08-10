
typedef struct
{  
  v2 dXY;
  r32 dZ;
} tile_map_difference; 

typedef struct
{  
  u32 AbsTileX;
  u32 AbsTileY;
  u32 AbsTileZ;

  v2 Offset_;
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
