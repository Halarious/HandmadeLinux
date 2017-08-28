
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
} entity_reference;

struct sim_entity
{
  world_chunk *OldChunk;
  u32 StorageIndex;
  bool32 Updatable;
  
  entity_type Type;
  u32 Flags;

  v3 P;
  v3 dP;

  r32 DistanceLimit;

  v3 Dim;
  
  u32 FacingDirection;
  r32 tBob;   

  u32 HitPointMax;
  hit_point HitPoints[16];

  entity_reference Sword;
};

typedef enum
{
  EntityFlag_Collides = (1 << 0),
  EntityFlag_Nonspatial = (1 << 1),
  

  EntityFlag_Simming = (1 << 30),
} sim_entity_flags;

typedef struct
{
  sim_entity *Ptr;
  u32 Index;
}sim_entity_hash; 

typedef struct
{
  world *World;
  r32 MaxEntityRadius;
  r32 MaxEntityVelocity;
  
  world_position Origin;
  rectangle3 Bounds;
  rectangle3 UpdatableBounds;
  
  u32 MaxEntityCount;
  u32 EntityCount;
  sim_entity *Entities;

  sim_entity_hash Hash[4096]; 
} sim_region;
