
typedef struct
{
  v3 P;
} render_basis;

typedef struct
{
  render_basis *Basis;
  v2 Offset;
  r32 OffsetZ;
  r32 EntityZC;

} render_entity_basis;
  
typedef enum
{
  RenderGroupEntryType_render_entry_clear,
  RenderGroupEntryType_render_entry_bitmap,
  RenderGroupEntryType_render_entry_rectangle,
  RenderGroupEntryType_render_entry_coordinate_system,
} render_group_entry_type;

typedef struct
{
  render_group_entry_type Type;
} render_group_entry_header;

typedef struct
{
  render_group_entry_header Header;
  v4 Color;
} render_entry_clear;

typedef struct
{
  render_group_entry_header Header;
  v4 Color;
  v2 Origin;
  v2 XAxis;
  v2 YAxis;

  v2 Points[16];
} render_entry_coordinate_system;

typedef struct
{
  render_group_entry_header Header;
  render_entity_basis EntityBasis;
  
  loaded_bitmap *Bitmap;
  
  r32 R, G, B, A;
} render_entry_bitmap;

typedef struct
{
  render_group_entry_header Header;
  render_entity_basis EntityBasis;

  v2 Dim;  
  r32 R, G, B, A;
} render_entry_rectangle;

typedef struct
{
  render_basis *DefaultBasis;
  r32 MetersToPixels;
  
  u32 PushBufferSize;
  u32 MaxPushBufferSize;
  u8* PushBufferBase;
} render_group;

