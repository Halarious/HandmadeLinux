
typedef struct debug_variable debug_variable;

typedef enum
  {
    DebugVariable_Boolean,
    DebugVariable_Group,
  } debug_variable_type;

typedef struct 
{
  bool32 Expanded;
  debug_variable* FirstChild;
  debug_variable* LastChild;
} debug_variable_group;

struct debug_variable
{
  debug_variable_type Type;
  char* Name;
  debug_variable* Next;
  debug_variable* Parent;

  union
  {
    bool32 Bool32;
    debug_variable_group Group;
  };
};

typedef struct render_group render_group;
typedef struct assets assets;
typedef struct loaded_bitmap loaded_bitmap;
typedef struct loaded_font loaded_font;
typedef struct hha_font hha_font;
  
typedef enum
  {
    DEBUGTextOp_DrawText,
    DEBUGTextOp_SizeText,
  } debug_text_op;

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
  u16 LaneIndex;
  u16 ColourIndex;
  r32 MinT;
  r32 MaxT;
} debug_frame_region;

#define MAX_REGIONS_PER_FRAME 4096*2
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
  debug_record* Source;
  
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

  platform_work_queue* HighPriorityQueue;
  
  memory_arena DebugArena;

  debug_variable* RootGroup;
  
  struct render_group* RenderGroup;
  loaded_font* DebugFont;
  hha_font* DebugFontInfo;

  bool32 Compiling;
  debug_executing_process Compiler;
  
  v2 MenuP;
  bool32 MenuActive;
  u32 HotMenuIndex;
  
  r32 LeftEdge;
  r32 AtY;
  r32 FontScale;
  font_id FontID;
  r32 GlobalWidth;
  r32 GlobalHeight;
  
  debug_record* ScopeToRecord;
  
  memory_arena CollateArena;
  temporary_memory CollateTemp;

  u32 CollationArrayIndex;
  debug_frame* CollationFrame;
  u32 FrameBarLaneCount;
  u32 FrameCount;
  r32 FrameBarScale;	  
  bool32 Paused;

  bool32 ProfileOn;
  rectangle2 ProfileRect;

  debug_frame* Frames;
  debug_thread* FirstThread;
  open_debug_block* FirstFreeBlock;
} debug_state;

internal void
DEBUGStart(assets* Assets, u32 Width, u32 Height);

internal void
DEBUGEnd(input* Input, loaded_bitmap* DrawBuffer);

internal void
RefreshCollation(debug_state* DebugState);



