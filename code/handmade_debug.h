
typedef struct
{
  u32 HitCount;
  u64 CycleCount;  
} debug_counter_snapshot;

typedef struct
{
  char* FileName;
  char* BlockName;

  u32 LineNumber;

} debug_counter_state;

typedef struct
{
  u32 LaneIndex;
  r32 MinT;
  r32 MaxT;
} debug_frame_region;

typedef struct
{
  u64 BeginClock;
  u64 EndClock;
  u32 RegionCount;
  debug_frame_region* Regions;
} debug_frame;

typedef struct
{
  bool32 Initialized;
  
  memory_arena CollateArena;
  temporary_memory CollateTemp;
  
  u32 FrameBarLaneCount;
  u32 FrameCount;
  r32 FrameBarScale;	  

  debug_frame* Frames;
} debug_state;

typedef struct render_group render_group;
global_variable struct render_group* DEBUGRenderGroup;

typedef struct assets assets;
internal void
DEBUGReset(assets* Assets, u32 Width, u32 Height);

internal void
DEBUGOverlay(memory* Memory);



