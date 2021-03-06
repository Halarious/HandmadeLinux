
typedef struct render_group render_group;
typedef struct assets assets;
typedef struct loaded_bitmap loaded_bitmap;
typedef struct loaded_font loaded_font;
typedef struct hha_font hha_font;

struct loaded_bitmap
{
  void* Memory;
  
  v2 AlignPercentage;
  r32 WidthOverHeight;    
  s32 Width;
  s32 Height;
  s32 Pitch;  
};

typedef struct
{
  loaded_bitmap LOD[4];
  r32 Pz;
} environment_map;
  
typedef enum
{
  RenderGroupEntryType_render_entry_clear,
  RenderGroupEntryType_render_entry_bitmap,
  RenderGroupEntryType_render_entry_rectangle,
  RenderGroupEntryType_render_entry_coordinate_system,
  RenderGroupEntryType_render_entry_saturation,
} render_group_entry_type;

typedef struct
{
  render_group_entry_type Type;
} render_group_entry_header;

typedef struct
{
  v4 Color;
} render_entry_clear;

typedef struct
{
  r32 Level;
} render_entry_saturation;

typedef struct
{
  loaded_bitmap *Bitmap;

  v2 P;
  v2 Size;
  v4 Color;
} render_entry_bitmap;

typedef struct
{
  v2 P;
  v2 Dim;  
  v4 Color;
} render_entry_rectangle;

typedef struct
{
  v4 Color;
  v2 Origin;
  v2 XAxis;
  v2 YAxis;
  loaded_bitmap* Texture;
  loaded_bitmap* NormalMap;

  //r32 PixelsToMeters;
  
  environment_map* Top;
  environment_map* Middle;
  environment_map* Bottom;
} render_entry_coordinate_system;

typedef struct
{
  bool32 Orthographic;

  r32 MetersToPixels;
  v2 ScreenCenter;
  
  r32 FocalLength;
  r32 DistanceAboveTarget;

  v3 OffsetP;
  r32 Scale;
} render_transform;

typedef struct assets assets;
struct render_group
{
  assets* Assets;
  r32 GlobalAlpha;

  u32 GenerationID;

  v2  MonitorHalfDimInMeters;
  render_transform Transform;  
    
  u32 PushBufferSize;
  u32 MaxPushBufferSize;
  u8* PushBufferBase;

  u32 MissingResourceCount;
  bool32 RendersInBackground;

  bool32 InsideRender;
};


//NOTE: Renderer "API"

internal inline void
PushBitmap(render_group *Group, loaded_bitmap* Bitmap, v3 Offset, r32 Height, v4 Color, r32 CAlign);

internal inline void
PushRect(render_group *Group, v3 Offset, v2 Dim, v4 Color);

internal inline void
PushRectOutline(render_group *Group, v3 Offset, v2 Dim, v4 Color, r32 Thickness);

internal inline void
Clear(render_group* Group, v4 Color);

typedef struct
{
  v2 P;
  r32 Scale;
  bool32 Valid;
} entity_basis_p;

typedef struct
{
  entity_basis_p Basis;
  v2 Size;
  v2 Align;
  v3 P;
} used_bitmap_dim;

void
DrawRectangleQuickly(loaded_bitmap *Buffer,
		     v2 Origin, v2 XAxis, v2 YAxis,
		     v4 Color, loaded_bitmap* Texture,
		     r32 PixelsToMeters,
		     rectangle2i ClipRect,
		     bool32 Even);


