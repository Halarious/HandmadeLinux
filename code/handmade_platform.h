#include <unistd.h>
#include <stdint.h>

typedef int32_t bool32;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int32_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef size_t memory_index;

typedef float  r32;
typedef double r64;

#define false 0
#define true !false

#define internal        static
#define local_persist   static
#define global_variable static

#ifdef __clang__
#define Assert(Expresion) if(!(Expresion)){ __builtin_trap();}
#else
#define Assert(Expresion) if(!(Expresion)){(*((int*)0) = 0);}
#endif

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Kilobytes(Value) (Value*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

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

#endif

typedef struct
{
  //NOTE: Memory order: aRGB
  void* BitmapMemory;

  int BitmapMemorySize;
  int BytesPerPixel;
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
  bool32 IsAnalog;

  union
  {
    button_state Buttons[9];
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

  r32 dtForFrame;
  controller_input Controllers[2];
} input;

typedef struct
{
  bool32 isInitialized;
  
  u64   PermanentStorageSize;
  u64   TransientStorageSize;

  void* PermanentStorage;
  void* TransientStorage;

  debug_platform_read_entire_file* DEBUGPlatformReadEntireFile;
  debug_platform_free_file_memory* DEBUGPlatformFreeFileMemory;
  debug_platform_write_entire_file* DEBUGPlatformWriteEntireFile;
} memory;


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
