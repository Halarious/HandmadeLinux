
typedef struct
{
  v2 AlignPercentage;
  r32 WidthOverHeight;
    
  int Width;
  int Height;
  int Pitch;
  void* Memory;
} loaded_bitmap;

typedef struct
{
  loaded_bitmap LOD[4];
  r32 Pz;
} environment_map;

typedef struct
{
  v3 P;
} render_basis;

typedef struct
{
  render_basis *Basis;
  v3 Offset;
} render_entity_basis;
  
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
  render_entity_basis EntityBasis;
  v2 Size;
  v4 Color;
} render_entry_bitmap;

typedef struct
{
  render_entity_basis EntityBasis;

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

  environment_map* Top;
  environment_map* Middle;
  environment_map* Bottom;
} render_entry_coordinate_system;

typedef struct
{
  r32 GlobalAlpha;

  render_basis *DefaultBasis;
    
  u32 PushBufferSize;
  u32 MaxPushBufferSize;
  u8* PushBufferBase;
} render_group;


//NOTE: Renderer "API"

internal inline void
PushBitmap(render_group *Group, loaded_bitmap* Bitmap, v3 Offset, r32 Height, v4 Color);

internal inline void
PushRect(render_group *Group, v3 Offset, v2 Dim, v4 Color);

internal inline void
PushRectOutline(render_group *Group, v3 Offset, v2 Dim, v4 Color);

internal inline void
Clear(render_group* Group, v4 Color);

