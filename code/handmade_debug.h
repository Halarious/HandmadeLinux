
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
  debug_record* Record;
  u64 CycleCount;
  u32 LaneIndex;
  r32 MinT;
  r32 MaxT;
} debug_frame_region;

#define MAX_REGIONS_PER_FRAME 4096
typedef struct
{
  u64 BeginClock;
  u64 EndClock;
  r32 WallSecondsElapsed;
  
  u32 RegionCount;
  debug_frame_region* Regions;
} debug_frame;

typedef struct open_debug_block open_debug_block;
struct open_debug_block
{
  u32 StartingFrameIndex;
  debug_event* OpeningEvent;
  open_debug_block* Parent;

  open_debug_block* NextFree;
};

typedef struct debug_thread debug_thread;
struct debug_thread
{
  u32 ID;
  u32 LaneIndex;
  open_debug_block* FirstOpenBlock;  
  debug_thread* Next;
};

typedef struct
{
  bool32 Initialized;
  bool32 Paused;
  
  memory_arena CollateArena;
  temporary_memory CollateTemp;
  
  u32 FrameBarLaneCount;
  u32 FrameCount;
  r32 FrameBarScale;	  
  
  debug_frame* Frames;
  debug_thread* FirstThread;
  open_debug_block* FirstFreeBlock;
} debug_state;

typedef struct render_group render_group;
global_variable struct render_group* DEBUGRenderGroup;

typedef struct assets assets;
internal void
DEBUGReset(assets* Assets, u32 Width, u32 Height);

internal void
DEBUGOverlay(memory* Memory, input* Input);



