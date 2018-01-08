#include "handmade_platform.h"

#define Minimum(A, B) ((A < B) ? A : B)
#define Maximum(A, B) ((A > B) ? A : B)

typedef struct
{
  r32 ScaleForPixelHeight;
  
  s32 Ascent;
  s32 Descent;
  s32 LineGap;

  s32 VerticalAdvance;
} font_metrics;

typedef struct
{
  void* Base;
  memory_index Size;
  memory_index Used;

  s32 TempCount;
} memory_arena;

typedef struct
{
  memory_arena *Arena;
  memory_index Used;
} temporary_memory;
  
typedef struct
{
  char* FontTTFPath;
  char* FontName;

  font_metrics Metrics; 
  memory_arena GlyphArena;
} font;

//NOTE: Services the platform layer provides to the game

inline u32
SafeTruncateUInt64(u64 Value)
{
  Assert(Value <= 0xFFFFFFFF);
  u32 Result = (u32) Value;
  return Result;
}

internal void
InitializeArena(memory_arena* Arena, memory_index Size, void* StorageBase)
{
  Arena->Size = Size;
  Arena->Base = (u8*)StorageBase;
  Arena->Used = 0;
  Arena->TempCount = 0;
}

#define PushStruct(Arena, type) (type*)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type*)PushSize_(Arena, sizeof(type)*Count)
#define PushSize(Arena, Size) PushSize_(Arena, Size)
internal void*
PushSize_(memory_arena* Arena, memory_index Size)
{
  Assert((Arena->Used + Size) <= Arena->Size); 
  void* Result = Arena->Base + Arena->Used;
  Arena->Used += Size;
  return(Result);
}

internal inline temporary_memory
BeginTemporaryMemory(memory_arena* Arena)
{
  temporary_memory Result;

  Result.Arena = Arena;
  Result.Used  = Arena->Used;

  ++Arena->TempCount;
  
  return(Result);
}

internal inline void
EndTemporaryMemory(temporary_memory TempMem)
{
  memory_arena* Arena = TempMem.Arena;
  Assert(Arena->Used >= TempMem.Used);
  Arena->Used = TempMem.Used;

  Assert(Arena->TempCount > 0);
  --Arena->TempCount;
}

internal inline void
CheckArena(memory_arena* Arena)
{
  Assert(Arena->TempCount == 0);
}

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
internal inline void
ZeroSize(memory_index Size, void *Ptr)
{
  u8 *Byte = (u8*)Ptr;
  while(--Size)
    {
      *Byte++ = 0;
    }
}

#include "handmade_intrinsics.h"
#include "handmade_math.h"
#include "handmade_world.h"
#include "handmade_sim_region.h"
#include "handmade_entity.h"
#include "handmade_render_group.h"

typedef struct
{
  loaded_bitmap Head;
  loaded_bitmap Cape;
  loaded_bitmap Torso;
} hero_bitmaps;

typedef struct
{
  world_position P;  
  sim_entity Sim;
} low_entity;

typedef struct
{
  u32 EntityIndex;
  
  v2 ddP;
  v2 dSword;
  r32 dZ;
}controlled_hero;

typedef struct pairwise_collision_rule pairwise_collision_rule;
struct pairwise_collision_rule
{
  bool32 CanCollide;
  u32 StorageIndexA;
  u32 StorageIndexB;

  pairwise_collision_rule *NextInHash;
};

typedef struct
{
  world_position P;
  loaded_bitmap Bitmap;
} ground_buffer;

typedef struct
{
  memory_arena WorldArena;
  world *World;

  r32 TypicalFloorHeight;
  
  u32 CameraFollowingEntityIndex;
  world_position CameraP;
  
  controlled_hero ControlledHeroes[ArrayCount(((input*)0)->Controllers)];
    
  u32 LowEntityCount;
  low_entity LowEntities[100000];

  loaded_bitmap Grass[2];
  loaded_bitmap Stone[4];
  loaded_bitmap Tuft[3];
  
  loaded_bitmap Backdrop;
  loaded_bitmap Shadow;
  hero_bitmaps HeroBitmaps[4];

  loaded_bitmap Tree;
  loaded_bitmap Sword;
  loaded_bitmap Stairwell;
  
  pairwise_collision_rule *CollisionRuleHash[256];
  pairwise_collision_rule *FirstFreeCollisionRule;

  sim_entity_collision_volume_group *NullCollision;
  sim_entity_collision_volume_group *SwordCollision;
  sim_entity_collision_volume_group *StairCollision;
  sim_entity_collision_volume_group *PlayerCollision;
  sim_entity_collision_volume_group *FamiliarCollision;
  sim_entity_collision_volume_group *MonstarCollision;
  sim_entity_collision_volume_group *WallCollision;
  sim_entity_collision_volume_group *StandardRoomCollision;
  
  r32 Time;

  loaded_bitmap TestDiffuse;
  loaded_bitmap TestNormal;
  
} state;

typedef struct
{
  bool32 IsInitialized;
  memory_arena TransientArena;

  u32 GroundBufferCount;
  ground_buffer* GroundBuffers;

  platform_work_queue* RenderQueue;
  
  u32 EnvMapWidth;
  u32 EnvMapHeight;
  environment_map EnvMaps[3];
  
} transient_state;

internal inline low_entity*
GetLowEntity(state *State, u32 Index)
{
  low_entity *Result = 0;
  
  if((Index > 0) && (Index < State->LowEntityCount))
    {
      Result = State->LowEntities + Index; 
    }

  return(Result);
}

global_variable platform_add_entry* PlatformAddEntry;
global_variable platform_complete_all_work* PlatformCompleteAllWork;

internal void
AddCollisionRule(state *State, u32 StorageIndexA, u32 StorageIndexB, bool32 CanCollide);
internal void
ClearCollisionRuleFor(state *State, u32 StorageIndex);

