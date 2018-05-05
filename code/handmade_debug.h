
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

debug_record DebugRecordArray[];

typedef struct
{
  debug_record* Record;
  u64 StartCycles;
  u32 HitCount;
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
  return(Result);
}

internal void
DestructTimedBlock(timed_block Block)
{
  u64 Delta = (__builtin_readcyclecounter() - Block.StartCycles) | (((u64)Block.HitCount) << 32);
  //AtomicAddUInt64(&Block.Record->HitCount_CycleCount, Delta);
  __sync_fetch_and_add(&Block.Record->HitCount_CycleCount, Delta);
}


typedef struct
{
  u32 HitCount;
  u32 CycleCount;  
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
} debug_state;


