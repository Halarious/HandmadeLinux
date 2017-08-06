
typedef struct
{  
  u32 AbsTileX;
  u32 AbsTileY;

  r32 TileRelX;
  r32 TileRelY;
} tile_map_position; 

typedef struct
{
  u32 *Tiles;
} tile_chunk;

typedef struct
{
  u32 TileChunkX;
  u32 TileChunkY;
  
  u32 RelTileX;
  u32 RelTileY;
} tile_chunk_position; 

typedef struct
{
  r32 TileSideInMeters;
  s32 TileSideInPixels;
  r32 MetersToPixels;

  u32 ChunkShift;
  u32 ChunkMask;
  u32 ChunkDim;  

  u32 TileChunkCountX;
  u32 TileChunkCountY;
  
  tile_chunk *TileChunks;
} tile_map;
