
typedef struct
{  
  s32 ChunkX;
  s32 ChunkY;
  s32 ChunkZ;

  v3 Offset_;
} world_position; 

typedef struct world_entity_block world_entity_block;
struct world_entity_block
{
  u32 EntityCount;
  u32 LowEntityIndex[16];
  world_entity_block *Next;
};

typedef struct world_chunk world_chunk;
struct world_chunk
{
  s32 ChunkX;
  s32 ChunkY;
  s32 ChunkZ;
 
  world_entity_block FirstBlock;
  world_chunk* NextInHash;
};

typedef struct
{
  r32 TileSideInMeters;
  r32 TileDepthInMeters;
  v3 ChunkDimInMeters;
  
  world_entity_block *FirstFree;  

  world_chunk ChunkHash[4096];
} world;
