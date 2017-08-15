
typedef struct
{
  u32 StorageIndex;
  
  v2 P;

  u32 ChunkZ;
  
  r32 Z;
  r32 dZ;
  
} sim_entity;

typedef struct
{
  world *World;
  world_position Origin;
  rectangle2 Bounds;
  
  u32 MaxEntityCount;
  u32 EntityCount;
  sim_entity *Entities;

} sim_region;
