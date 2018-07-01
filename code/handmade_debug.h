
typedef struct debug_variable debug_variable;
typedef struct debug_variable_link debug_variable_link;
typedef struct debug_tree debug_tree;
typedef struct debug_view debug_view;

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
    
    DebugVariable_VarGroup,
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
  v2 Dim;
} debug_view_inline_block;

typedef enum
  {
    DebugViewType_Unknown,
    
    DebugViewType_Basic,
    DebugView_InlineBlock,
    DebugView_Collaspible,
  } debug_view_type;

typedef struct
  {
    bool32 ExpandedAlways;
    bool32 ExpandedAltView;
  } debug_view_collapsible;

typedef struct
{
  void* Value[2];
} debug_id;

struct debug_view
{
  debug_id ID;
  debug_view* NextInHash;

  debug_view_type Type;
  union
  {
    debug_view_inline_block InlineBlock;
    debug_view_collapsible Collapsible;
  };
}; 

struct debug_tree
{
  v2 UIP;
  debug_variable* Group;

  debug_tree* Next;
  debug_tree* Prev;
};

typedef struct
{
  int Placeholder;
} debug_profile_setting;

typedef struct
{
  bitmap_id ID;
} debug_bitmap_display;

struct debug_variable_link
{
  debug_variable_link* Next;
  debug_variable_link* Prev;
  debug_variable* Var;
};

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
    debug_profile_setting Profile;
    debug_bitmap_display BitmapDisplay;
    debug_variable_link VarGroup;
  };
};

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
  open_debug_block* FirstOpenCodeBlock;
  open_debug_block* FirstOpenDataBlock;  
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
  debug_id ID;
  debug_interaction_type Type;  

  union
  {
    void* Generic;
    debug_variable* Var;
    debug_tree* Tree;
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

  debug_variable *RootGroup;
  debug_view* ViewHash[4096];
  debug_tree TreeSentinel;

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




