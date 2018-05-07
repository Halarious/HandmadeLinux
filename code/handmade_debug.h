
//NOTE: We are not doing anything with the line number passed as 'Number' since we don't do the constructor/destructor C++ method
#define BEGIN_TIMED_BLOCK__(Number, ...) ConstructTimedBlock(__COUNTER__, __FILE__, __LINE__, (char*)__FUNCTION__, ## __VA_ARGS__)
#define BEGIN_TIMED_BLOCK_(Number, ...) BEGIN_TIMED_BLOCK__(Number, ## __VA_ARGS__)
#define BEGIN_TIMED_BLOCK(...) BEGIN_TIMED_BLOCK_(__LINE__, ## __VA_ARGS__)

#define END_TIMED_BLOCK(timed_block) DestructTimedBlock(timed_block)

typedef struct
{
  char* FileName;
  char* FunctionName;

  u32 LineNumber;
  u32 Reserved;
  
  u64 HitCount_CycleCount;  
} debug_record;

typedef enum
  {
    DebugEvent_BeginBlock,
    DebugEvent_EndBlock,
  } debug_event_type; 

typedef struct
{
  u64 Clock;
  u16 ThreadIndex;
  u16 CoreIndex;
  u16 DebugRecordIndex;
  u8 DebugRecordArrayIndex;
  u8 Type;
} debug_event;

debug_record DebugRecordArray[];

#define MAX_DEBUG_EVENT_COUNT (16* 65536)
extern u64 Global_DebugEventArrayIndex_DebugEventIndex;
extern debug_event GlobalDebugEventArray[2][MAX_DEBUG_EVENT_COUNT];

#define RecordDebugEvent(RecordIndex, EventType)			\
  u64 ArrayIndex_EventIndex = AtomicAddUInt64(&Global_DebugEventArrayIndex_DebugEventIndex, 1);	\
  u32 EventIndex = (ArrayIndex_EventIndex & 0xffffffff);		\
  Assert(EventIndex < MAX_DEBUG_EVENT_COUNT);				\
  debug_event* Event = GlobalDebugEventArray[ArrayIndex_EventIndex >> 32] + (EventIndex); \
  Event->Clock = __builtin_readcyclecounter();				\
  Event->ThreadIndex = 0;						\
  Event->CoreIndex = 0;							\
  Event->DebugRecordIndex = (u16)RecordIndex;				\
  Event->DebugRecordArrayIndex = DebugRecordArrayIndexConstant;		\
  Event->Type = EventType;

typedef struct
{
  debug_record* Record;
  u64 StartCycles;
  u32 HitCount;
  int Counter;
} timed_block;

/*
timed_block
ConstructTimedBlock(int Counter, char* FileName, int LineNumber, char* FunctionName, u32 HitCountInit);

void
DestructTimedBlock(timed_block Block);
*/

internal timed_block
ConstructTimedBlock(int Counter, char* FileName, int LineNumber, char* FunctionName, u32 HitCountInit)
{
  debug_record* Record = DebugRecordArray + Counter;
  Record->FileName = FileName;
  Record->FunctionName = FunctionName;
  Record->LineNumber = LineNumber;
  
  timed_block Result = {Record, __builtin_readcyclecounter(), HitCountInit};

  RecordDebugEvent(Counter, DebugEvent_BeginBlock);
  
  return(Result);
}

internal void
DestructTimedBlock(timed_block Block)
{
  u64 Delta = (__builtin_readcyclecounter() - Block.StartCycles) | (((u64)Block.HitCount) << 32);
  AtomicAddUInt64(&Block.Record->HitCount_CycleCount, Delta);
  //__sync_fetch_and_add(&Block.Record->HitCount_CycleCount, Delta);


  RecordDebugEvent(Block.Counter, DebugEvent_EndBlock);
  
}

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




