#include "handmade_platform.h"

typedef struct
{
  u32* Memory;
  
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

#include "handmade_math.h"
#include "handmade_intrinsics.h"
#include "handmade_tile.h"

typedef struct
{
  tile_map *TileMap;
} world;

typedef struct
{
  s32 AlignX;
  s32 AlignY;
  
  loaded_bitmap Head;
  loaded_bitmap Cape;
  loaded_bitmap Torso;
} hero_bitmaps;

typedef struct
{
  memory_arena WorldArena;
  memory_arena BitmapArena;
  font         Font;
  world *World;
  
  tile_map_position PlayerP;
  tile_map_position CameraP;
  v2 dPlayerP;
  
  loaded_bitmap Backdrop;
  hero_bitmaps HeroBitmaps[4];
  u32 HeroFacingDirection;
  
} state;
