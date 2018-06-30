
typedef struct
{
  bool32 UnitMaxAccelVector;
  r32 Speed;
  r32 Drag;
} move_spec;

typedef enum
{
  EntityType_Null,

  EntityType_Space,
  
  EntityType_Hero,
  EntityType_Wall,
  EntityType_Familiar,
  EntityType_Monstar,
  EntityType_Sword,
  EntityType_Stairwell,
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

typedef struct 
{
  v3 OffsetP;
  v3 Dim;
  
} sim_entity_collision_volume;

typedef struct 
{
  sim_entity_collision_volume TotalVolume;
  
  u32 VolumeCount;
  sim_entity_collision_volume *Volumes;
} sim_entity_collision_volume_group;
  
introspect(category:"regular butter") struct sim_entity
{
  world_chunk *OldChunk;
  u32 StorageIndex;
  bool32 Updatable;
  
  entity_type Type;
  u32 Flags;

  v3 P;
  v3 dP;

  r32 DistanceLimit;

  sim_entity_collision_volume_group *Collision;
    
  r32 FacingDirection;
  r32 tBob;   

  u32 HitPointMax;
  hit_point HitPoints[16];

  entity_reference Sword;

  v2 WalkableDim;
  r32 WalkableHeight;
};

typedef enum
{
  EntityFlag_Collides = (1 << 0),
  EntityFlag_Nonspatial = (1 << 1),
  EntityFlag_Moveable = (1 << 2),
  EntityFlag_ZSupported = (1 << 3),
  EntityFlag_Traversable = (1 << 4),

  EntityFlag_Simming = (1 << 30),
} sim_entity_flags;

typedef struct
{
  sim_entity *Ptr;
  u32 Index;
}sim_entity_hash; 

typedef struct sim_region sim_region;
introspect(category:"regular butter") struct sim_region
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
};
