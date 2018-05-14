
#if !defined(COMPILER_MSCV)
#define COMPILER_MSCV 0
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSCV && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSCV
#define COMPILER_MSCV 1
#elif __clang__
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#elif COMPILER_LLVM
#include <x86intrin.h>
#else
#error SSE/NEON optimizations are not available for this compiler!!!
#endif

#include <unistd.h>
#include <string.h>
#include <stdint.h>
//TODO: PATH_MAX seems problematic in general (same in Win though), but
//      the consensus seems to be that its usually 4096 chars (not necessarily bytes)
//      so we will use it for now
#include <limits.h>
#include <float.h>

typedef int32_t bool32;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int32_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef uintptr_t uintptr;
typedef  intptr_t intptr;

typedef size_t memory_index;

typedef float  r32;
typedef double r64;

#define Real32Maximum FLT_MAX 

#define false 0
#define true !false

#define internal        static
#define local_persist   static
#define global_variable static

#define Pi32  3.14159265359f
#define Tau32 6.28318530717f

#if COMPILER_LLVM
#define Assert(Expresion) if(!(Expresion)){ __builtin_trap();}
#else
#define Assert(Expresion) if(!(Expresion)){(*((int*)0) = 0);}
#endif

#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidCase default: {InvalidCodePath;} break;

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Kilobytes(Value) (Value*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define AlignPow2(Value, Alignment) ((Value + (Alignment - 1)) & ~(Alignment - 1))
#define Align4(Value)  ((Value + 3)  & ~3)
#define Align8(Value)  ((Value + 7)  & ~7)
#define Align16(Value) ((Value + 15) & ~15)

internal inline u32 
SafeTruncateUInt64(u64 Value)
{
  Assert(Value <= 0xffffffff);
  u32 Result = (u32)Value;
  return(Result);
}

internal inline u16 
SafeTruncateUInt16(u32 Value)
{
  Assert(Value <= 65535);
  Assert(Value >= 0);
  u16 Result = (u16)Value;
  return(Result);
}

internal inline s16 
SafeTruncateInt16(s32 Value)
{
  Assert(Value <= 32767);
  Assert(Value >= -32768);
  s16 Result = (s16)Value;
  return(Result);
}

#if HANDMADE_INTERNAL

typedef struct
{
  void* Contents;
  s64   ContentsSize;
} loaded_file;

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) loaded_file name(char* Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(loaded_file* LoadedFile)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(char* Filename, u32 MemorySize, void* Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

extern struct memory* DebugGlobalMemory;

//NOTE: HANDMADE_INTERNAL
#endif

#define BITMAP_BYTES_PER_PIXEL 4
typedef struct
{
  //NOTE: Memory order: aRGB
  void* Memory;

  int Width;
  int Height;
  int Pitch;
} offscreen_buffer;

typedef struct
{
  s32 SamplesPerSecond;
  s32 SampleCount;
  s16* Samples;
} sound_output_buffer;

typedef struct
{
  bool32 EndedDown;
  s32    HalfTransitionCount;
} button_state;

typedef struct
{
  bool32 IsConnected;
  bool32 IsAnalog;
  r32 StickAverageX;
  r32 StickAverageY;

  union
  {
    button_state Buttons[13];
    struct
    {
      button_state MoveLeft;
      button_state MoveRight;
      button_state MoveUp;
      button_state MoveDown;

      button_state ActionLeft;
      button_state ActionRight;
      button_state ActionUp;
      button_state ActionDown;

      button_state LeftShoulder;
      button_state RightShoulder;
      
      button_state Back;
      button_state Start;
      
      //NOTE: All buttons must be added above this line
      button_state Terminator;
    };
  };
} controller_input;

typedef enum
  {
    PlatformMouseButton_Left,
    PlatformMouseButton_Middle,
    PlatformMouseButton_Right,
    PlatformMouseButton_Extended0,
    PlatformMouseButton_Extended1,

    PlatformMouseButton_Count,
  } input_mouse_button;
  
typedef struct
{
  button_state MouseButtons[5];
  r32 MouseX;
  r32 MouseY;
  r32 MouseZ;

  bool32 ExecutableReloaded;
  r32 dtForFrame;

  controller_input Controllers[2];
} input;

internal inline bool32
WasPressed(button_state State)
{
  bool32 Result = ((State.HalfTransitionCount > 1) ||
		   ((State.HalfTransitionCount == 1) && (State.EndedDown)));
  return(Result);
}

typedef struct
{
  bool32 NoErrors;
  void* Platform;
} platform_file_handle;

typedef struct
{
  u32 FileCount;
  void* Platform;
} platform_file_group;

typedef enum
  {
    PlatformFileType_AssetFile,
    PlatformFileType_SavedGameFile,

    PlatformFileType_Count,
  } platform_file_type;

#define PlatformNoFileErrors(Handle) ((Handle)->NoErrors)

#define PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN(name) platform_file_group name(platform_file_type Type)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN(platform_get_all_files_of_type_begin);

#define PLATFORM_GET_ALL_FILES_OF_TYPE_END(name) void name(platform_file_group* FileGroup)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_END(platform_get_all_files_of_type_end);

#define PLATFORM_OPEN_FILE(name) platform_file_handle name(platform_file_group* FileGroup)
typedef PLATFORM_OPEN_FILE(platform_open_next_file);

#define PLATFORM_READ_DATA_FROM_FILE(name) void name(platform_file_handle* Source, u32 Offset, u32 Size, void* Dest)
typedef PLATFORM_READ_DATA_FROM_FILE(platform_read_data_from_file);

#define PLATFORM_FILE_ERROR(name) void name(platform_file_handle* Handle, char* Message)
typedef PLATFORM_FILE_ERROR(platform_file_error);

typedef struct platform_work_queue platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue* Queue, void* Data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

#define PLATFORM_ALLOCATE_MEMORY(name) void* name(memory_index Size)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);

#define PLATFORM_DEALLOCATE_MEMORY(name) void name(void* Memory, memory_index Size)
typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);

typedef void platform_add_entry(platform_work_queue* Queue, platform_work_queue_callback* Callback, void* Data);
typedef void platform_complete_all_work(platform_work_queue* Queue);

typedef struct
{
  platform_add_entry* AddEntry;
  platform_complete_all_work* CompleteAllWork;

  platform_get_all_files_of_type_begin* GetAllFilesOfTypeBegin;
  platform_get_all_files_of_type_end* GetAllFilesOfTypeEnd;
  platform_open_next_file* OpenNextFile;
  platform_read_data_from_file* ReadDataFromFile;
  platform_file_error* FileError;

  platform_allocate_memory* AllocateMemory;
  platform_deallocate_memory* DeallocateMemory;
  
  debug_platform_read_entire_file* DEBUGReadEntireFile;
  debug_platform_free_file_memory* DEBUGFreeFileMemory;
  debug_platform_write_entire_file* DEBUGWriteEntireFile;
} platform_api;

typedef struct memory memory;
struct memory
{
  u64   PermanentStorageSize;
  void* PermanentStorage;

  u64   TransientStorageSize;
  void* TransientStorage;

  u64   DebugStorageSize;
  void* DebugStorage;

  platform_work_queue* HighPriorityQueue;
  platform_work_queue* LowPriorityQueue;

  platform_api PlatformAPI;
};

#define UPDATE_AND_RENDER(name) void name(memory* Memory, input* Input, offscreen_buffer* Buffer)
typedef UPDATE_AND_RENDER(update_and_render);

#define GET_SOUND_SAMPLES(name) void name(memory* Memory, sound_output_buffer* SoundBuffer)
typedef UPDATE_AND_RENDER(get_sound_samples);


#if COMPILER_MSVC
#define CompletePreviousReadsBeforeFutureReads _ReadBarrier() ; 
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier() 

internal inline u32
AtomicCompareExchangeUInt32(u32 volatile* Value, u32 New, u32 Expected)
{
  u32 Result = _InterlockedCompareExchange((long*)Value,
					   New,
					   Expected);
  return(Result);
}

internal inline u64
AtomicExchangeUInt64(u64 volatile* Value, u64 New)
{
  u64 Result = _InterlockedExchange((__int64*)Value,
				    New);
  return(Result);
}

internal inline u32
AtomicAddUInt32(u32 volatile* Value, u32 Addend)
{
  u32 Result = _InterlockedExchangedAdd((long*)Value,
					Addend);
  return(Result);
}

internal inline u64
AtomicAddUInt64(u32 volatile* Value, u32 Addend)
{
  u64 Result = _InterlockedExchangedAdd((__int64*)Value,
					Addend);
  return(Result);
}

internal inline u32
GetThreadID()
{
  u32* ThreadLocalStorage = (u8*)__readgsqword(0x30);
  u32 ThreadID = *(u32*)(ThreadLocalStorage + 0x48);
  
  return(ThreadID);
}

#elif COMPILER_LLVM
#define CompletePreviousReadsBeforeFutureReads   asm volatile("" ::: "memory");
#define CompletePreviousWritesBeforeFutureWrites asm volatile("" ::: "memory");

internal inline u32
AtomicCompareExchangeUInt32(u32 volatile* Value, u32 New, u32 Expected)
{
  u32 Result = __sync_val_compare_and_swap(Value,
					   Expected,
					   New);
  return(Result);
}

internal inline u64
AtomicExchangeUInt64(u64 volatile* Value, u64 New)
{
  u64 Result = __sync_lock_test_and_set(Value, New);
  //u64 Result = __sync_swap(Value, New);
  return(Result);
}

internal inline u32
AtomicAddUInt32(u32 volatile* Value, u32 Addend)
{
  u32 Result = __sync_fetch_and_add(Value,
				    Addend);
  return(Result);
}

internal inline u64
AtomicAddUInt64(u64 volatile* Value, u64 Addend)
{
  u64 Result = __sync_fetch_and_add(Value,
				    Addend);
  return(Result);
}

internal inline u32
GetThreadID()
{
  u32 ThreadID;
  unsigned long TestID;
  __asm__ __volatile__("movl %%fs:%a[Offset], %k[ThreadID]" : [ThreadID] "=r" (ThreadID) : [Offset] "ir" (0x10));
  __asm__ __volatile__("movl %%fs:%a[Offset], %k[TestID]" : [TestID] "=r" (TestID) : [Offset] "ir" (0x10));
  return(ThreadID);
}

#else
#error Intrinsics not defined for this compiler!
#endif

typedef struct debug_table debug_table;
#define DEBUG_FRAME_END(name) debug_table* name(memory* Memory)
typedef DEBUG_FRAME_END(debug_frame_end);

internal inline controller_input*
GetController(input *Input, u32 ControllerIndex)
{
  controller_input *Result = &Input->Controllers[ControllerIndex];
  return(Result);
}

typedef struct
{
  char* FileName;
  char* BlockName;

  u32 LineNumber;
  u32 Reserved;
} debug_record;

typedef enum
  {
    DebugEvent_FrameMarker,
    DebugEvent_BeginBlock,
    DebugEvent_EndBlock,
  } debug_event_type; 

typedef struct
{
  u16 ThreadID;
  u16 CoreIndex;
} threadid_coreindex;
  
typedef struct
{
  u64 Clock;
  union
  {
    threadid_coreindex TC;
    r32 SecondsElapsed;
  };
  u16 DebugRecordIndex;
  u8 TranslationUnit;
  u8 Type;
} debug_event;

#define MAX_DEBUG_THREAD_COUNT 256
#define MAX_DEBUG_EVENT_ARRAY_COUNT 8
#define MAX_DEBUG_TRANSLATION_UNITS 3
#define MAX_DEBUG_EVENT_COUNT (16* 65536)
#define MAX_DEBUG_RECORD_COUNT (65536)

struct debug_table
{
  u32 CurrentEventArrayIndex;
  u64 volatile EventArrayIndex_EventIndex;
  u32 EventCount[MAX_DEBUG_EVENT_ARRAY_COUNT];
  debug_event Events[MAX_DEBUG_EVENT_ARRAY_COUNT][MAX_DEBUG_EVENT_COUNT];

  u32 RecordCount[MAX_DEBUG_TRANSLATION_UNITS];
  debug_record Records[MAX_DEBUG_TRANSLATION_UNITS][MAX_DEBUG_RECORD_COUNT];
};

extern debug_table* GlobalDebugTable;

#define RecordDebugEventCommon(RecordIndex, EventType)			\
  u64 ArrayIndex_EventIndex = AtomicAddUInt64(&GlobalDebugTable->EventArrayIndex_EventIndex, 1); \
  u32 EventIndex = (ArrayIndex_EventIndex & 0xffffffff);		\
  Assert(EventIndex < MAX_DEBUG_EVENT_COUNT);				\
  debug_event* Event = GlobalDebugTable->Events[ArrayIndex_EventIndex >> 32] + (EventIndex); \
  Event->Clock = __builtin_readcyclecounter();				\
  Event->DebugRecordIndex = (u16)RecordIndex;				\
  Event->TranslationUnit = TRANSLATION_UNIT_INDEX;			\
  Event->Type = (u8)EventType;						\
  
#define RecordDebugEvent(RecordIndex, EventType)	\
  {							\
    RecordDebugEventCommon(RecordIndex, EventType);	\
    Event->TC.ThreadID = (u16) GetThreadID();		\
    Event->TC.CoreIndex = 0;				\
  }

#define FRAME_MARKER(SecondsElapsedInit)				\
  { 									\
    int Counter = __COUNTER__;						\
    RecordDebugEventCommon(Counter, DebugEvent_FrameMarker);		\
    Event->SecondsElapsed = SecondsElapsedInit;				\
    debug_record* Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
    Record->FileName = __FILE__;					\
    Record->LineNumber = __LINE__;					\
    Record->BlockName = "Frame Marker";					\
  }									\
  
#if HANDMADE_PROFILE

//NOTE: We are not doing anything with the line number passed as 'Number' since we don't do the constructor/destructor C++ method
#define BEGIN_NAMED_BLOCK__(Name, Number, ...) timed_block TB_##Name = ConstructTimedBlock(__COUNTER__, __FILE__, __LINE__, #Name, ## __VA_ARGS__)
#define BEGIN_NAMED_BLOCK_(Name, Number, ...) BEGIN_NAMED_BLOCK__(Name, Number, ## __VA_ARGS__)
#define BEGIN_NAMED_BLOCK(Name, ...) BEGIN_NAMED_BLOCK_(Name, __LINE__, ## __VA_ARGS__)
#define END_NAMED_BLOCK(Name) DestructTimedBlock(TB_##Name)

//#define BEGIN_TIMED_FUNCTION(...) BEGIN_NAMED_BLOCK_(__FUNCTION__, __LINE__, ## __VA_ARGS__)
#define BEGIN_TIMED_FUNCTION(...) timed_block TB___FUNCTION__ = ConstructTimedBlock(__COUNTER__, __FILE__, __LINE__, (char*)__FUNCTION__, ## __VA_ARGS__)
#define END_TIMED_FUNCTION() END_NAMED_BLOCK(__FUNCTION__)

#define BEGIN_TIMED_BLOCK_(Counter, _FileName, _LineNumber, _BlockName)	\
  {debug_record* Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
    Record->FileName = _FileName;					\
    Record->LineNumber = _LineNumber;					\
    Record->BlockName = _BlockName;					\
    RecordDebugEvent(Counter, DebugEvent_BeginBlock);}

#define END_TIMED_BLOCK_(Counter)			\
  RecordDebugEvent(Counter, DebugEvent_EndBlock);  

#define BEGIN_TIMED_BLOCK(Name)						\
  int Counter_##Name = __COUNTER__;					\
  BEGIN_TIMED_BLOCK_(Counter_##Name, __FILE__, __LINE__, #Name);

#define END_TIMED_BLOCK(Name)			\
  END_TIMED_BLOCK_(Counter_##Name);
 
typedef struct
{
  int Counter;
} timed_block;

internal timed_block
ConstructTimedBlock(int Counter, char* FileName, int LineNumber, char* BlockName, u32 HitCountInit)
{
  BEGIN_TIMED_BLOCK_(Counter, FileName, LineNumber, BlockName)

  timed_block Result = {Counter};
  return(Result);
}

internal void
DestructTimedBlock(timed_block Block)
{
  END_TIMED_BLOCK_(Block.Counter);
}

#else
#define BEGIN_NAMED_BLOCK(Name, ...) 
#define END_NAMED_BLOCK(Name)

#define BEGIN_TIMED_FUNCTION(...) 
#define END_TIMED_FUNCTION() 

#define BEGIN_TIMED_BLOCK(Name)
#define END_TIMED_BLOCK(Name)

#endif

