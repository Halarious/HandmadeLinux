
typedef struct
{
  bool32 UnitMaxAccelVector;
  r32 Speed;
  r32 Drag;
} move_spec;

typedef enum
{
  EntityType_Null,
  EntityType_Hero,
  EntityType_Wall,
  EntityType_Familiar,
  EntityType_Monstar,
  EntityType_Sword,
} entity_type;

#define HIT_POINT_SUB_COUNT 4
typedef struct
{
  u8 Flags;
  u8 FilledAmount;
} hit_point; 

typedef struct sim_entity sim_entity;
typedef union
{
  sim_entity* Ptr;
  u32 Index;
}entity_reference;

struct sim_entity
{
  entity_type Type;
  u32 StorageIndex;
  
  v2 P;

  r32 Z;
  r32 dZ;

  r32 Height;
  r32 Width;

  v2 dP;
  u32 FacingDirection;
  r32 tBob;
   
  s32 ChunkZ;
  bool32 Collides;

  u32 HitPointMax;
  hit_point HitPoints[16];

  entity_reference Sword;
  r32 DistanceRemaining;
};

typedef struct
{
  sim_entity *Ptr;
  u32 Index;
}sim_entity_hash; 

typedef struct
{
  world *World;
  world_position Origin;
  rectangle2 Bounds;
  
  u32 MaxEntityCount;
  u32 EntityCount;
  sim_entity *Entities;

  sim_entity_hash Hash[4096]; 
} sim_region;
