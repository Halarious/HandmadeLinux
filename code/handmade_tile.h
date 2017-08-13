
typedef struct
{  
  v2 dXY;
  r32 dZ;
} tile_map_difference; 

typedef struct
{  
  s32 AbsTileX;
  s32 AbsTileY;
  s32 AbsTileZ;

  v2 Offset_;
} tile_map_position; 

typedef struct tile_chunk tile_chunk;
struct tile_chunk
{
  s32 TileChunkX;
  s32 TileChunkY;
  s32 TileChunkZ;

  u32 *Tiles;

  tile_chunk* NextInHash;
};

typedef struct
{
  s32 TileChunkX;
  s32 TileChunkY;
  s32 TileChunkZ;
  
  s32 RelTileX;
  s32 RelTileY;
} tile_chunk_position; 

typedef struct
{
  r32 TileSideInMeters;

  s32 ChunkShift;
  s32 ChunkMask;
  s32 ChunkDim;  

  tile_chunk TileChunkHash[4096];
} tile_map;
