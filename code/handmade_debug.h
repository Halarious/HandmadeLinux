
typedef struct
{
  u32 HitCount;
  u64 CycleCount;  
} debug_counter_snapshot;

#define DEBUG_SNAPSHOT_COUNT 120
typedef struct
{
  char* FileName;
  char* FunctionName;

  u32 LineNumber;

  debug_counter_snapshot Snapshots[DEBUG_SNAPSHOT_COUNT];
} debug_counter_state;
  
typedef struct
{
  u32 SnapshotIndex;
  u32 CounterCount;
  debug_counter_state CounterStates[512];
  debug_frame_end_info FrameEndInfos[DEBUG_SNAPSHOT_COUNT];
} debug_state;

typedef struct render_group render_group;
global_variable struct render_group* DEBUGRenderGroup;

typedef struct assets assets;
internal void
DEBUGReset(assets* Assets, u32 Width, u32 Height);

internal void
DEBUGOverlay(memory* Memory);



