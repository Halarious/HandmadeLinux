

internal void
DrawBitmap(loaded_bitmap *Buffer, loaded_bitmap *Bitmap,
	   r32 RealX,  r32 RealY, r32 CAlpha)
{
  s32 MinX = RoundReal32ToInt32(RealX);
  s32 MinY = RoundReal32ToInt32(RealY);
  s32 MaxX = MinX + (r32)Bitmap->Width;
  s32 MaxY = MinY + (r32)Bitmap->Height;

  s32 SourceOffsetX = 0;
  if(MinX < 0)
    {
      SourceOffsetX = -MinX;
      MinX = 0;
    }
  if(MaxX > Buffer->Width)
    {
      MaxX = Buffer->Width;
    }
  s32 SourceOffsetY = 0;
  if(MinY < 0)
    {
      SourceOffsetY = -MinY;
      MinY = 0;
    }
  if(MaxY > Buffer->Height)
    {
      MaxY = Buffer->Height;
    }
  
  s32 BytesPerPixel = BITMAP_BYTES_PER_PIXEL;
  u8* SourceRow = ((u8*)Bitmap->Memory +
		   SourceOffsetY*Bitmap->Pitch +
		   SourceOffsetX*BytesPerPixel);
  u8* DestRow   = ((u8*)Buffer->Memory +
		   MinX*BytesPerPixel +
		    MinY*Buffer->Pitch);
  for(int Y = MinY;
      Y < MaxY;
      ++Y)
    {
      u32 *Dest   = (u32*) DestRow;
      u32 *Source = (u32*) SourceRow;
      for(int X = MinX;
	  X < MaxX;
	  ++X)
	{
	  r32 sAlpha = ((*Source >> 24) & 0xff);
	  r32 rsAlpha = CAlpha*(sAlpha / 255.0f);
	  r32 sChannel1 = CAlpha*((*Source >> 16) & 0xff);
	  r32 sChannel2 = CAlpha*((*Source >> 8) & 0xff);
	  r32 sChannel3 = CAlpha*((*Source >> 0) & 0xff);
 
	  r32 dAlpha = ((*Dest >> 24) & 0xff);
	  r32 dChannel1 = ((*Dest >> 16) & 0xff);
	  r32 dChannel2 = ((*Dest >> 8) & 0xff);
	  r32 dChannel3 = ((*Dest >> 0) & 0xff);
	  r32 rdAlpha = (dAlpha / 255.0f);
 
	  r32 InvRSA= (1.0f - rsAlpha);
	  r32 Alpha = 255.0f*(rsAlpha + rdAlpha - rsAlpha*rdAlpha);
	  r32 Channel1 = InvRSA*dChannel1 + sChannel1;
	  r32 Channel2 = InvRSA*dChannel2 + sChannel2;
	  r32 Channel3 = InvRSA*dChannel3 + sChannel3;

	  *Dest = (((u32)(Alpha + 0.5f) << 24) |
		   ((u32)(Channel1 + 0.5f) << 16) |
		   ((u32)(Channel2 + 0.5f) << 8)  |
		   ((u32)(Channel3 + 0.5f) << 0));
	  
	  ++Dest;
	  ++Source;
	}
      SourceRow += Bitmap->Pitch;
      DestRow   += Buffer->Pitch;
    }
}

internal void
DrawRectangle(loaded_bitmap *Buffer,
	      v2 vMin, v2 vMax,
	      r32 R, r32 G, r32 B)
{
  s32 MinX = RoundReal32ToInt32(vMin.X);
  s32 MinY = RoundReal32ToInt32(vMin.Y);
  s32 MaxX = RoundReal32ToInt32(vMax.X);
  s32 MaxY = RoundReal32ToInt32(vMax.Y);
  if(MinX < 0)
    {
      MinX = 0;
    }
  if(MaxX > Buffer->Width)
    {
      MaxX = Buffer->Width;
    }
  if(MinY < 0)
    {
      MinY = 0;
    }
  if(MaxY > Buffer->Height)
    {
      MaxY = Buffer->Height;
    }

  u32 Color = (u32) ((0xFF << 24) |
		     (RoundReal32ToUInt32(R * 255.0f) << 16) |
		     (RoundReal32ToUInt32(G * 255.0f) << 8)  |
		     (RoundReal32ToUInt32(B * 255.0f)));
  
  u32 BytesPerPixel = BITMAP_BYTES_PER_PIXEL;
  u8 *Row = Buffer->Memory
    + Buffer->Pitch * MinY
    + BytesPerPixel * MinX;
  for(int Y = MinY; Y < MaxY; ++Y)
    {
      u32 *Pixel = (u32*) Row;
      for(int X = MinX; X < MaxX; ++X)
	{
	  *Pixel++ = Color;
	}
      Row += Buffer->Pitch; 
    }
}


internal void
DrawRectangleOutline(loaded_bitmap *Buffer,
		     v2 vMin, v2 vMax,
		     v3 Color, r32 R)
{   
  DrawRectangle(Buffer,
		V2(vMin.X - R, vMin.Y - R),
		V2(vMax.X + R, vMin.Y + R),
		Color.R, Color.G, Color.B);
  DrawRectangle(Buffer,
		V2(vMin.X - R, vMax.Y - R),
		V2(vMax.X + R, vMax.Y + R),
		Color.R, Color.G, Color.B);
    
  DrawRectangle(Buffer,
		V2(vMin.X - R, vMin.Y - R),
		V2(vMin.X + R, vMax.Y + R),
		Color.R, Color.G, Color.B);
  DrawRectangle(Buffer,
		V2(vMax.X - R, vMin.Y - R),
		V2(vMax.X + R, vMax.Y + R),
		Color.R, Color.G, Color.B);
}

internal inline v2
GetRenderEntityBasisP(render_group* RenderGroup,
		      render_entity_basis* EntityBasis,
		      v2 ScreenCenter)
{
  v3 EntityBaseP = EntityBasis->Basis->P;
  r32 ZFudge = (1.0f + 0.1f * (EntityBasis->OffsetZ + EntityBaseP.Z));
	    
  r32 EntityGroundPointX = ScreenCenter.X
    + RenderGroup->MetersToPixels*ZFudge*EntityBaseP.X;
  r32 EntityGroundPointY = ScreenCenter.Y
    - RenderGroup->MetersToPixels*ZFudge*EntityBaseP.Y;
  r32 EntityZ = -RenderGroup->MetersToPixels*EntityBaseP.Z;

  v2 Center = {EntityBasis->Offset.X + EntityGroundPointX,
	       EntityBasis->Offset.Y + EntityGroundPointY
	       + EntityBasis->EntityZC*EntityZ};
  return(Center);
}

internal void
RenderGroupToOutput(render_group* RenderGroup, loaded_bitmap* OutputTarget)
{    
  v2 ScreenCenter = V2(0.5f * (r32)OutputTarget->Width,
		       0.5f * (r32)OutputTarget->Height);
  
  for(u32 BaseAddress = 0;
      BaseAddress < RenderGroup->PushBufferSize;)
    {	      
      render_group_entry_header *Header
	= (render_group_entry_header*)(RenderGroup->PushBufferBase + BaseAddress);

      switch(Header->Type)
	{
	case RenderGroupEntryType_render_entry_clear:
	  {
	    render_entry_clear* Entry = (render_entry_clear*) Header; 

	    BaseAddress += sizeof(*Entry);
	  } break;

	case RenderGroupEntryType_render_entry_bitmap:
	  {
	    render_entry_bitmap* Entry = (render_entry_bitmap*) Header;
	    v2 P = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenCenter);

	    Assert(Entry->Bitmap);
	    DrawBitmap(OutputTarget, Entry->Bitmap,
		       P.X, P.Y, Entry->A);

	    BaseAddress += sizeof(*Entry);
	  } break;

	case RenderGroupEntryType_render_entry_rectangle:
	  {
	    render_entry_rectangle* Entry = (render_entry_rectangle*) Header;
	    v2 P = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenCenter);
	    	    
	    DrawRectangle(OutputTarget,
			  P,
			  V2Add(P, Entry->Dim),
			  Entry->R, Entry->G, Entry->B);
	    
	    BaseAddress += sizeof(*Entry);
	  } break;
	  
	  InvalidDefaultCase;
	}
    }
}

internal render_group*
AllocateRenderGroup(memory_arena *Arena, u32 MaxPushBufferSize,
		    r32 MetersToPixels)
{
  render_group* Result = PushStruct(Arena, render_group);
  Result->PushBufferBase = PushSize(Arena, MaxPushBufferSize);

  Result->DefaultBasis = PushStruct(Arena, render_basis);
  Result->DefaultBasis->P = V3(0.0f, 0.0f, 0.0f);
  Result->MetersToPixels = MetersToPixels;
    
  Result->MaxPushBufferSize = MaxPushBufferSize;
  Result->PushBufferSize = 0;
  
  return(Result);
}


#define PushRenderElement(Group, type) (type*) PushRenderElement_(Group, sizeof(type), RenderGroupEntryType_##type) 
internal inline render_group_entry_header*
PushRenderElement_(render_group *Group, u32 Size, render_group_entry_type Type)
{
  render_group_entry_header* Result = 0;
  
  if((Group->PushBufferSize + Size) < Group->MaxPushBufferSize)
    {
      Result = (render_group_entry_header*) (Group->PushBufferBase + Group->PushBufferSize);
      Result->Type = Type;
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
  render_entry_bitmap* Piece = PushRenderElement(Group, render_entry_bitmap);
  if(Piece)
    {
      Piece->EntityBasis.Basis  = Group->DefaultBasis; 
      Piece->Bitmap = Bitmap;
      Piece->EntityBasis.Offset = V2Sub(V2MulS(Group->MetersToPixels,
				   V2(Offset.X, -Offset.Y)),
			    Align);  
      Piece->EntityBasis.OffsetZ = OffsetZ;
      Piece->EntityBasis.EntityZC = EntityZC;
      Piece->A = Color.A;
      Piece->R = Color.R;
      Piece->G = Color.G;
      Piece->B = Color.B;
    }
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
  render_entry_rectangle* Piece = PushRenderElement(Group, render_entry_rectangle);
  if(Piece)
    {
      v2 HalfDim = V2MulS(0.5f * Group->MetersToPixels, Dim);
      
      Piece->EntityBasis.Basis  = Group->DefaultBasis; 
      Piece->EntityBasis.Offset = V2Sub(V2MulS(Group->MetersToPixels,
				   V2(Offset.X, -Offset.Y)),
			    HalfDim);  
      Piece->EntityBasis.OffsetZ = OffsetZ;
      Piece->EntityBasis.EntityZC = EntityZC;
      Piece->A = Color.A;
      Piece->R = Color.R;
      Piece->G = Color.G;
      Piece->B = Color.B;
      Piece->Dim = V2MulS(Group->MetersToPixels, Dim);
    }
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

