
//NOTE: We are not doing anything with the line number passed as 'Number' since we don't do the constructor/destructor C++ method
#define BEGIN_TIMED_BLOCK__(Number, ...) ConstructTimedBlock(__COUNTER__, __FILE__, __LINE__, (char*)__FUNCTION__, ## __VA_ARGS__)
#define BEGIN_TIMED_BLOCK_(Number, ...) BEGIN_TIMED_BLOCK__(Number, ## __VA_ARGS__)
#define BEGIN_TIMED_BLOCK(...) BEGIN_TIMED_BLOCK_(__LINE__, ## __VA_ARGS__)

#define END_TIMED_BLOCK(timed_block) DestructTimedBlock(timed_block)

typedef struct
{
  u64 CycleCount;  

  char* FileName;
  char* FunctionName;

  u32 LineNumber;
  u32 HitCount;
} debug_record;

debug_record DebugRecordArray[];

typedef struct
{
  debug_record* Record;
} timed_block;

timed_block
ConstructTimedBlock(int Counter, char* FileName, int LineNumber, char* FunctionName, int HitCount);

void
DestructTimedBlock(timed_block Block);


