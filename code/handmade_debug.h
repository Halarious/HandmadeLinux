
typedef struct debug_variable debug_variable;
typedef struct debug_variable_reference debug_variable_reference;

typedef enum
  {
    DEBUGVarToText_AddDebugUI = 0x1,
    DEBUGVarToText_AddName = 0x2,
    DEBUGVarToText_FloatSuffix = 0x4,
    DEBUGVarToText_LineFeedEnd = 0x8,
    DEBUGVarToText_NullTerminator = 0x10,
    DEBUGVarToText_Colon = 0x20,
    DEBUGVarToText_PrettyBools = 0x40,
  } debug_variable_to_text_flag;

typedef enum
  {
    DebugVariable_Bool32,
    DebugVariable_Int32,
    DebugVariable_UInt32,
    DebugVariable_Real32,
    DebugVariable_V2,
    DebugVariable_V3,
    DebugVariable_V4,

    DebugVariable_CounterThreadList,
    //DebugVariable_CounterFunctionList,
    DebugVariable_BitmapDisplay,
    
    DebugVariable_Group,
  } debug_variable_type;

internal inline bool32
DEBUGShouldBeWritten(debug_variable_type Type)
{
  bool32 Result = ((Type != DebugVariable_CounterThreadList) &&
		   (Type != DebugVariable_BitmapDisplay));
  return(Result);
}

typedef struct 
{
  bool32 Expanded;
  debug_variable_reference* FirstChild;
  debug_variable_reference* LastChild;
} debug_variable_group;

typedef struct debug_variable_hierarchy debug_variable_hierarchy;
struct debug_variable_hierarchy
{
  v2 UIP;
  debug_variable_reference* Group;

  debug_variable_hierarchy* Next;
  debug_variable_hierarchy* Prev;
};

typedef struct
{
  v2 Dimension;
} debug_profile_setting;

struct debug_variable_reference
{
  debug_variable* Var;
  debug_variable_reference* Next;
  debug_variable_reference* Parent;
}; 

typedef struct
{
  bitmap_id ID;
  v2 Dim;
  bool32 Alpha;
} debug_bitmap_display;

struct debug_variable
{
  debug_variable_type Type;
  char* Name;

  union
  {
    bool32 Bool32;
    s32 Int32;
    u32 UInt32;
    r32 Real32;
    v2  Vector2;
    v3  Vector3;
    v4  Vector4;
    debug_variable_group Group;
    debug_profile_setting Profile;
    debug_bitmap_display BitmapDisplay;
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

typedef enum
  {
    DebugInteraction_None,

    DebugInteraction_NOP,

    DebugInteraction_AutoModifyVariable,
    
    DebugInteraction_DragValue,
    DebugInteraction_ToggleValue,
    DebugInteraction_TearValue,

    DebugInteraction_Resize,
    DebugInteraction_Move,
  } debug_interaction_type;

typedef struct
{
  debug_interaction_type Type;  

  union
  {
    void* Generic;
    debug_variable* Var;
    debug_variable_hierarchy* Hierarchy;
    v2* P;
  };
} debug_interaction;

typedef struct
{
  bool32 Initialized;

  platform_work_queue* HighPriorityQueue;
  
  memory_arena DebugArena;

  struct render_group* RenderGroup;
  loaded_font* DebugFont;
  hha_font* DebugFontInfo;

  bool32 Compiling;
  debug_executing_process Compiler;
  
  v2 MenuP;
  bool32 MenuActive;

  debug_variable_reference *RootGroup;
  debug_variable_hierarchy HierarchySentinel;

  v2 LastMouseP;
  debug_interaction Interaction;
  debug_interaction HotInteraction;
  debug_interaction NextHotInteraction;
      
  r32 LeftEdge;
  r32 RightEdge;
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



