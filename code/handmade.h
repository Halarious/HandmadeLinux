#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

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
  void* Memory;
  
  int Width;
  int Height;
  int Pitch;
} loaded_bitmap;

typedef struct
{
  s32 XOffset;
  s32 YOffset;
  loaded_bitmap* Bitmap;
} glyph_bitmap;

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

controller_input*
GetController(input *Input, u32 ControllerIndex)
{
  controller_input *Result = &Input->Controllers[ControllerIndex];
  return(Result);
}

typedef struct
{
  
}drawBuffer;

typedef struct
{
  s32         HotItem;
  s32         ActiveItem;
  
  drawBuffer  DrawBuffer;
  input*      InputState;
}guiContext;

typedef struct
{
  r32 ScaleForPixelHeight;
  
  s32 Ascent;
  s32 Descent;
  s32 LineGap;

  s32 VerticalAdvance;
} font_metrics;

typedef struct
{
  void* Base;
  memory_index Size;
  memory_index Used;
} memory_arena;
  
typedef struct
{
  char* FontTTFPath;
  char* FontName;

  font_metrics Metrics; 
  memory_arena GlyphArena;
} font;

//NOTE: Services the platform layer provides to the game

typedef struct
{
  void* Contents;
  s64   ContentsSize;
} loaded_file;

typedef struct
{
} thread_context;

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) loaded_file name(thread_context* Thread, char* Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context* Thread, loaded_file* LoadedFile)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context* Thread, char* Filename, u32 MemorySize, void* Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

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

void
ZeroSize(memory_index Size, void* Memory)
{
  u8* Byte = (u8*) Memory;
  while(Size--)
    {
      *Byte++ = 0;
    }
}

inline u32
SafeTruncateUInt64(u64 Value)
{
  Assert(Value <= 0xFFFFFFFF);
  u32 Result = (u32) Value;
  return Result;
}

#define UPDATE_AND_RENDER(name) void name(thread_context* Thread, memory* Memory, input* Input, offscreen_buffer* Buffer)
typedef UPDATE_AND_RENDER(update_and_render);
UPDATE_AND_RENDER(UpdateAndRenderStub)
{
}

internal void
InitializeArena(memory_arena* Arena, memory_index Size, u8* StorageBase)
{
  Arena->Size = Size;
  Arena->Base = StorageBase;
  Arena->Used = 0;
}

#define PushStruct(Arena, type) (type*)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type*)PushSize_(Arena, sizeof(type)*Count)
#define PushSize(Arena, Size) PushSize_(Arena, Size)
internal void*
PushSize_(memory_arena* Arena, memory_index Size)
{
  Assert((Arena->Used + Size) <= Arena->Size); 
  void* Result = Arena->Base + Arena->Used;
  Arena->Used += Size;
  return(Result);
}

#include "handmade_intrinsics.h"
#include "handmade_tile.h"

typedef struct
{
  tile_map *TileMap;
} world;

typedef struct
{
  memory_arena WorldArena;
  memory_arena BitmapArena;
  font         Font;

  world *World;
  tile_map_position PlayerP;
}state;
