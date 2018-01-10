
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

#define Pi32 3.14159265359f

#if COMPILER_LLVM
#define Assert(Expresion) if(!(Expresion)){ __builtin_trap();}
#else
#define Assert(Expresion) if(!(Expresion)){(*((int*)0) = 0);}
#endif

#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase default: {InvalidCodePath;} break;

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Kilobytes(Value) (Value*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define Align16(Value) ((Value + 15) & ~15)

typedef struct
{
} thread_context;


#if HANDMADE_INTERNAL

typedef struct
{
  void* Contents;
  s64   ContentsSize;
} loaded_file;

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) loaded_file name(thread_context* Thread, char* Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context* Thread, loaded_file* LoadedFile)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context* Thread, char* Filename, u32 MemorySize, void* Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

enum
  {
    DebugCycleCounter_UpdateAndRender,
    DebugCycleCounter_RenderGroupToOutput,
    DebugCycleCounter_DrawRectangleSlowly,
    DebugCycleCounter_DrawRectangleQuickly,
    DebugCycleCounter_ProcessPixel,
    DebugCycleCounter_Count,
  };

typedef struct
{
  u64 CycleCount;
  u32 HitCount;
} debug_cycle_counter;


extern struct memory* DebugGlobalMemory;

#if COMPILER_LLVM
#define BEGIN_TIMED_BLOCK(ID) u64 StartCycleCount##ID = __builtin_readcyclecounter();
#define END_TIMED_BLOCK(ID)   DebugGlobalMemory->Counters[DebugCycleCounter_##ID].CycleCount += (__builtin_readcyclecounter() - StartCycleCount##ID); ++DebugGlobalMemory->Counters[DebugCycleCounter_##ID].HitCount;
#define END_TIMED_BLOCK_COUNTED(ID, Count) DebugGlobalMemory->Counters[DebugCycleCounter_##ID].CycleCount += (__builtin_readcyclecounter() - StartCycleCount##ID); DebugGlobalMemory->Counters[DebugCycleCounter_##ID].HitCount += Count;

#elif COMPILER_MSVC
#define BEGIN_TIMED_BLOCK(ID) u64 StartCycleCount##ID = __rdtsc();
#define END_TIMED_BLOCK(ID)   DebugGlobalMemory->Counters[DebugCycleCounter_##ID].CycleCount += (__rdtsc() - StartCycleCount##ID); ++DebugGlobalMemory->Counters[DebugCycleCounter_##ID].HitCount;
#define END_TIMED_BLOCK_COUNTED(ID, Count) DebugGlobalMemory->Counters[DebugCycleCounter_##ID].CycleCount += (__rdtsc() - StartCycleCount##ID); DebugGlobalMemory->Counters[DebugCycleCounter_##ID].HitCount += Count;

#else
#define BEGIN_TIMED_BLOCK(ID) 
#define END_TIMED_BLOCK(ID)
#endif

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

typedef struct
{
  button_state MouseButtons[5];
  s32 MouseX;
  s32 MouseY;
  s32 MouseZ;

  bool32 ExecutableReloaded;
  r32 dtForFrame;

  controller_input Controllers[2];
} input;

typedef struct platform_work_queue platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue* Queue, void* Data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

typedef void platform_add_entry(platform_work_queue* Queue, platform_work_queue_callback* Callback, void* Data);
typedef void platform_complete_all_work(platform_work_queue* Queue);

typedef struct memory memory;
struct memory
{
  bool32 IsInitialized;
  
  u64   PermanentStorageSize;
  u64   TransientStorageSize;

  void* PermanentStorage;
  void* TransientStorage;

  platform_work_queue* HighPriorityQueue;
  
  platform_add_entry* PlatformAddEntry;
  platform_complete_all_work* PlatformCompleteAllWork;
  
  debug_platform_read_entire_file* DEBUGPlatformReadEntireFile;
  debug_platform_free_file_memory* DEBUGPlatformFreeFileMemory;
  debug_platform_write_entire_file* DEBUGPlatformWriteEntireFile;

#if HANDMADE_INTERNAL
  debug_cycle_counter Counters[DebugCycleCounter_Count];
#endif
};

#define UPDATE_AND_RENDER(name) void name(thread_context* Thread, memory* Memory, input* Input, offscreen_buffer* Buffer)
typedef UPDATE_AND_RENDER(update_and_render);
UPDATE_AND_RENDER(UpdateAndRenderStub)
{
}

controller_input*
GetController(input *Input, u32 ControllerIndex)
{
  controller_input *Result = &Input->Controllers[ControllerIndex];
  return(Result);
}
