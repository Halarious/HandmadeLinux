
typedef struct
{
  v3 P;
} render_basis;

typedef struct
{
  render_basis *Basis;
  loaded_bitmap *Bitmap;
  v2 Offset;
  r32 OffsetZ;
  r32 EntityZC;

  r32 R, G, B, A;
  v2 Dim;
} entity_visible_piece;

typedef struct
{
  render_basis *DefaultBasis;
  r32 MetersToPixels;
  u32 Count;
  
  u32 PushBufferSize;
  u32 MaxPushBufferSize;
  u8* PushBufferBase;
} render_group;

internal inline void* 
PushRenderElement(render_group *Group, u32 Size)
{
  void* Result = 0;
  
  if((Group->PushBufferSize + Size) < Group->MaxPushBufferSize)
    {
      Result = Group->PushBufferBase + Group->PushBufferSize;
      Group->PushBufferSize += Size;
    }
  else
    {
      InvalidCodePath;
    }

  return(Result);
}

internal inline void
PushPiece(render_group *Group, loaded_bitmap* Bitmap,
	  v2 Offset, r32 OffsetZ, v2 Align, v2 Dim, v4 Color, r32 EntityZC)
{
  entity_visible_piece* Piece = (entity_visible_piece*) PushRenderElement(Group, sizeof(entity_visible_piece));
  Piece->Basis  = Group->DefaultBasis; 
  Piece->Bitmap = Bitmap;
  Piece->Offset = V2Sub(V2MulS(Group->MetersToPixels,
			       V2(Offset.X, -Offset.Y)),
			Align);  
  Piece->OffsetZ = OffsetZ;
  Piece->EntityZC = EntityZC;
  Piece->A = Color.A;
  Piece->R = Color.R;
  Piece->G = Color.G;
  Piece->B = Color.B;
  Piece->Dim = Dim;
}

internal inline void
PushBitmap(render_group *Group, loaded_bitmap* Bitmap,
	   v2 Offset, r32 OffsetZ, v2 Align, r32 Alpha, r32 EntityZC)
{
  PushPiece(Group, Bitmap, Offset, OffsetZ, Align, V2(0, 0),
	    V4(1.0f, 1.0f, 1.0f, Alpha), EntityZC);
}

internal inline void
PushRect(render_group *Group, v2 Offset, r32 OffsetZ,
	 v2 Dim, v4 Color, r32 EntityZC)
{
  PushPiece(Group, 0, Offset, OffsetZ, V2(0, 0), Dim,
	    Color, EntityZC);
}

internal inline void
PushRectOutline(render_group *Group, v2 Offset, r32 OffsetZ,
		v2 Dim, v4 Color, r32 EntityZC)
{
  r32 Thickness = 0.05f;
    
  PushPiece(Group, 0,
	    V2Sub(Offset, V2(0, 0.5f*Dim.Y)),
	    OffsetZ,
	    V2(0, 0),
	    V2(Dim.X, Thickness),
	    Color, EntityZC);
  PushPiece(Group, 0,
	    V2Add(Offset,V2(0, 0.5f*Dim.Y)),
	    OffsetZ,
	    V2(0, 0),
	    V2(Dim.X, Thickness),
	    Color, EntityZC);
  
  PushPiece(Group, 0,
	    V2Sub(Offset, V2(0.5f*Dim.X, 0)),
	    OffsetZ,
	    V2(0, 0),
	    V2(Thickness, Dim.Y),
	    Color, EntityZC);
  PushPiece(Group, 0,
	    V2Add(Offset,V2(0.5f*Dim.X, 0)),
	    OffsetZ,
	    V2(0, 0),
	    V2(Thickness, Dim.Y),
	    Color, EntityZC);
}

