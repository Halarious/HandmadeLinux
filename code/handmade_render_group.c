
internal inline v4
SRGB255ToLinear1(v4 C)
{
  v4 Result;

  r32 Inv255 = 1.0f / 255.0f;
  
  Result.r = Square(Inv255*C.r);
  Result.g = Square(Inv255*C.g);
  Result.b = Square(Inv255*C.b);
  Result.a = Inv255*C.a;

  return(Result);
}

internal inline v4
Linear1ToSRGB255(v4 C)
{
  v4 Result;

  r32 One255 = 255.0f;

  Result.r = One255*SquareRoot(C.r);
  Result.g = One255*SquareRoot(C.g);
  Result.b = One255*SquareRoot(C.b);
  Result.a = One255*C.a;

  return(Result);  
}

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
	      r32 R, r32 G, r32 B, r32 A)
{
  s32 MinX = RoundReal32ToInt32(vMin.x);
  s32 MinY = RoundReal32ToInt32(vMin.y);
  s32 MaxX = RoundReal32ToInt32(vMax.x);
  s32 MaxY = RoundReal32ToInt32(vMax.y);

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

  u32 Color = ((RoundReal32ToUInt32(A * 255.0f) << 24) |
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
DrawRectangleSlowly(loaded_bitmap *Buffer,
		    v2 Origin, v2 XAxis, v2 YAxis,
		    v4 Color, loaded_bitmap* Texture)
{
  r32 InvXAxisLengthSq = 1.0f / V2LengthSq(XAxis);
  r32 InvYAxisLengthSq = 1.0f / V2LengthSq(YAxis);
  
  u32 Color32 = ((RoundReal32ToUInt32(Color.a * 255.0f) << 24) |
		 (RoundReal32ToUInt32(Color.r * 255.0f) << 16) |
		 (RoundReal32ToUInt32(Color.g * 255.0f) << 8)  |
		 (RoundReal32ToUInt32(Color.b * 255.0f)));

  s32 WidthMax  = Buffer->Width - 1;
  s32 HeightMax = Buffer->Height - 1;

  s32 XMin = WidthMax;
  s32 XMax = 0;
  s32 YMin = HeightMax;
  s32 YMax = 0;
  
  v2 P[4] = {Origin,
	     V2Add(Origin, XAxis),
	     V2Add(V2Add(Origin, XAxis),
		   YAxis),
	     V2Add(Origin, YAxis)};

  for(u32 PIndex = 0;
      PIndex < ArrayCount(P);
      ++PIndex)
    {
      v2 TestP = P[PIndex];
      s32 FloorX = FloorReal32ToInt32(TestP.x);
      s32 CeilX  = CeilReal32ToInt32(TestP.x);
      s32 FloorY = FloorReal32ToInt32(TestP.y);
      s32 CeilY  = CeilReal32ToInt32(TestP.y);

      if(XMin > FloorX) {XMin = FloorX;}
      if(YMin > FloorY) {YMin = FloorY;}
      if(XMax < CeilX)  {XMax = CeilX;}
      if(YMax < CeilY)  {YMax = CeilY;}
    }

  if(XMin < 0)
    {
      XMin = 0;
    }
  if(XMax > WidthMax)
    {
      XMax = WidthMax;
    }
  if(YMin < 0)
    {
      YMin = 0;
    }
  if(YMax > HeightMax)
    {
      YMax = HeightMax;
    }
  
  u32 BytesPerPixel = BITMAP_BYTES_PER_PIXEL;
  u8 *Row = Buffer->Memory
    + Buffer->Pitch * YMin
    + BytesPerPixel * XMin;
  for(int Y = YMin;
      Y <= YMax;
      ++Y)
    {
      u32 *Pixel = (u32*) Row;
      for(int X = XMin;
	  X <= XMax;
	  ++X)
	{
#if 1
	  v2 PixelP = V2i(X, Y);
	  v2 d = V2Sub(PixelP, Origin);

	  r32 Edge0 = V2Inner(d,
			      V2MulS(-1.0f, V2Perp(XAxis)));
	  r32 Edge1 = V2Inner(V2Sub(d, XAxis),
			      V2MulS(-1.0f, V2Perp(YAxis)));
	  r32 Edge2 = V2Inner(V2Sub(V2Sub(d, XAxis), YAxis),
			      V2Perp(XAxis));
	  r32 Edge3 = V2Inner(V2Sub(d, YAxis),
			      V2Perp(YAxis));	  
	  
	  if((Edge0 < 0) &&
	     (Edge1 < 0) &&
	     (Edge2 < 0) &&
	     (Edge3 < 0))
	    {
	      r32 U = InvXAxisLengthSq * V2Inner(d, XAxis);
	      r32 V = InvYAxisLengthSq * V2Inner(d, YAxis);

	      Assert((U >= 0.0f) && (U <= 1.0f));
	      Assert((V >= 0.0f) && (V <= 1.0f));

	      r32 tX = 1.0f + ((U * (r32)(Texture->Width  - 3)));
	      r32 tY = 1.0f + ((V * (r32)(Texture->Height - 3)));
	      
	      s32 X = (s32) tX;
	      s32 Y = (s32) tY;

	      r32 fX = tX - (r32)X;
	      r32 fY = tY - (r32)Y;
	      
	      Assert((X >= 0.0f) && (X < Texture->Width));
	      Assert((Y >= 0.0f) && (Y < Texture->Height));

	      u8* TexelPtr = (((u8*)Texture->Memory)
			      + Y * Texture->Pitch
			      + X * BITMAP_BYTES_PER_PIXEL);
	      u32 TexelPtrA = *(u32*)(TexelPtr);
	      u32 TexelPtrB = *(u32*)(TexelPtr + BITMAP_BYTES_PER_PIXEL);
	      u32 TexelPtrC = *(u32*)(TexelPtr + Texture->Pitch);
	      u32 TexelPtrD = *(u32*)(TexelPtr + Texture->Pitch + BITMAP_BYTES_PER_PIXEL);

	      v4 TexelA = V4((r32)((TexelPtrA >> 16) & 0xff),
			     (r32)((TexelPtrA >> 8) & 0xff),
			     (r32)((TexelPtrA >> 0) & 0xff),
			     (r32)((TexelPtrA >> 24) & 0xff));

	      v4 TexelB = V4((r32)((TexelPtrB >> 16) & 0xff),
			     (r32)((TexelPtrB >> 8) & 0xff),
			     (r32)((TexelPtrB >> 0) & 0xff),
			     (r32)((TexelPtrB >> 24) & 0xff));
	      
	      v4 TexelC = V4((r32)((TexelPtrC >> 16) & 0xff),
			     (r32)((TexelPtrC >> 8) & 0xff),
			     (r32)((TexelPtrC >> 0) & 0xff),
			     (r32)((TexelPtrC >> 24) & 0xff));
	      
	      v4 TexelD = V4((r32)((TexelPtrD >> 16) & 0xff),
			     (r32)((TexelPtrD >> 8) & 0xff),
			     (r32)((TexelPtrD >> 0) & 0xff),
			     (r32)((TexelPtrD >> 24) & 0xff));
	      
	      TexelA = SRGB255ToLinear1(TexelA);
	      TexelB = SRGB255ToLinear1(TexelB);
	      TexelC = SRGB255ToLinear1(TexelC);
	      TexelD = SRGB255ToLinear1(TexelD);
	      
	      v4 Texel = V4Lerp(V4Lerp(TexelA, fX, TexelB),
				fY,
				V4Lerp(TexelC, fX, TexelD));

	      r32 RSA = Color.a * Texel.a;
	      
	      v4 Dest = V4(((*Pixel >> 16) & 0xff),
			   ((*Pixel >>  8) & 0xff),
			   ((*Pixel >>  0) & 0xff),
			   ((*Pixel >> 24) & 0xff));
	      Dest = SRGB255ToLinear1(Dest);

	      r32 RDA = Dest.a;
 
	      r32 InvRSA= (1.0f - RSA);
	      
	      v4 Blended = V4(InvRSA*Dest.r + Color.a*Color.r*Texel.r,
			      InvRSA*Dest.g + Color.a*Color.g*Texel.g,
			      InvRSA*Dest.b + Color.a*Color.b*Texel.b,
			      (RSA + RDA - RSA*RDA));

	      v4 Blended255 = Linear1ToSRGB255(Blended);
		
	      *Pixel = (((u32)(Blended255.a + 0.5f) << 24) |
			((u32)(Blended255.r + 0.5f) << 16) |
			((u32)(Blended255.g + 0.5f) << 8)  |
			((u32)(Blended255.b + 0.5f) << 0));

	    }
#else
	  *Pixel = Color32;
#endif

	  ++Pixel;
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
		V2(vMin.x - R, vMin.y - R),
		V2(vMax.x + R, vMin.y + R),
		Color.r, Color.g, Color.b,
		1.0f);
  DrawRectangle(Buffer,
		V2(vMin.x - R, vMax.y - R),
		V2(vMax.x + R, vMax.y + R),
		Color.r, Color.g, Color.b,
		1.0f);
    
  DrawRectangle(Buffer,
		V2(vMin.x - R, vMin.y - R),
		V2(vMin.x + R, vMax.y + R),
		Color.r, Color.g, Color.b,
		1.0f);
  DrawRectangle(Buffer,
		V2(vMax.x - R, vMin.y - R),
		V2(vMax.x + R, vMax.y + R),
		Color.r, Color.g, Color.b,
		1.0f);
}

internal inline v2
GetRenderEntityBasisP(render_group* RenderGroup,
		      render_entity_basis* EntityBasis,
		      v2 ScreenCenter)
{
  v3 EntityBaseP = EntityBasis->Basis->P;
  r32 ZFudge = (1.0f + 0.1f * (EntityBasis->OffsetZ + EntityBaseP.z));
	    
  r32 EntityGroundPointX = ScreenCenter.x
    + RenderGroup->MetersToPixels*ZFudge*EntityBaseP.x;
  r32 EntityGroundPointY = ScreenCenter.y
    - RenderGroup->MetersToPixels*ZFudge*EntityBaseP.y;
  r32 EntityZ = -RenderGroup->MetersToPixels*EntityBaseP.z;

  v2 Center = {EntityBasis->Offset.x + EntityGroundPointX,
	       EntityBasis->Offset.y + EntityGroundPointY
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
	    DrawRectangle(OutputTarget,
			  V2(0.0f, 0.0f),
			  V2((r32)OutputTarget->Width,
			     (r32)OutputTarget->Height),
			  Entry->Color.r, Entry->Color.g,
			  Entry->Color.b, Entry->Color.a);


	    BaseAddress += sizeof(*Entry);
	  } break;

	case RenderGroupEntryType_render_entry_bitmap:
	  {
	    render_entry_bitmap* Entry = (render_entry_bitmap*) Header;
	    v2 P = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenCenter);

	    Assert(Entry->Bitmap);
	    DrawBitmap(OutputTarget, Entry->Bitmap,
		       P.x, P.y, Entry->A);

	    BaseAddress += sizeof(*Entry);
	  } break;

	case RenderGroupEntryType_render_entry_rectangle:
	  {
	    render_entry_rectangle* Entry = (render_entry_rectangle*) Header;
	    v2 P = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenCenter);
	    	    
	    DrawRectangle(OutputTarget,
			  P,
			  V2Add(P, Entry->Dim),
			  Entry->R, Entry->G, Entry->B, 1.0f);
	    
	    BaseAddress += sizeof(*Entry);
	  } break;
	case RenderGroupEntryType_render_entry_coordinate_system:
	  {
	    render_entry_coordinate_system* Entry = (render_entry_coordinate_system*) Header;
	    
	    DrawRectangleSlowly(OutputTarget,
				Entry->Origin,
				Entry->XAxis,
				Entry->YAxis,
				Entry->Color,
				Entry->Texture);

	    v4 Color = V4(1.0f, 1.0f, 0.0f, 1.0f);
	    v2 Dim = V2(2, 2);
	    v2 P = Entry->Origin; 
	    DrawRectangle(OutputTarget,
			  V2Sub(P, Dim),
			  V2Add(P, Dim),
			  Color.r, Color.g, Color.b, Color.a);
	    
	    P = V2Add(Entry->Origin, Entry->XAxis); 
	    DrawRectangle(OutputTarget,
			  V2Sub(P, Dim),
			  V2Add(P, Dim),
			  Color.r, Color.g, Color.b, Color.a);

	    P = V2Add(Entry->Origin, Entry->YAxis); 
	    DrawRectangle(OutputTarget,
			  V2Sub(P, Dim),
			  V2Add(P, Dim),
			  Color.r, Color.g, Color.b, Color.a);

#if 0	    	    
	    for(u32 PIndex = 0;
		PIndex < ArrayCount(Entry->Points);
		 ++PIndex)
	      {
		v2 P = Entry->Points[PIndex];
		P = V2Add(Entry->Origin, 
			  V2Add(V2MulS(P.x, Entry->XAxis),
				V2MulS(P.y, Entry->YAxis)));
		DrawRectangle(OutputTarget,
			      V2Sub(P, Dim),
			      V2Add(P, Dim),
			      Entry->Color.r, Entry->Color.g, Entry->Color.b, Entry->Color.a);
	    			
	      }
#endif	    
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
				   V2(Offset.x, -Offset.y)),
			    Align);  
      Piece->EntityBasis.OffsetZ = OffsetZ;
      Piece->EntityBasis.EntityZC = EntityZC;
      Piece->A = Color.a;
      Piece->R = Color.r;
      Piece->G = Color.g;
      Piece->B = Color.b;
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
				   V2(Offset.x, -Offset.y)),
			    HalfDim);  
      Piece->EntityBasis.OffsetZ = OffsetZ;
      Piece->EntityBasis.EntityZC = EntityZC;
      Piece->A = Color.a;
      Piece->R = Color.r;
      Piece->G = Color.g;
      Piece->B = Color.b;
      Piece->Dim = V2MulS(Group->MetersToPixels, Dim);
    }
}

internal inline void
PushRectOutline(render_group *Group, v2 Offset, r32 OffsetZ,
		v2 Dim, v4 Color, r32 EntityZC)
{
  r32 Thickness = 0.05f;
    
  PushRect(Group,
	    V2Sub(Offset, V2(0, 0.5f*Dim.y)),
	    OffsetZ,
	    V2(Dim.x, Thickness),
	    Color, EntityZC);
  PushRect(Group,
	    V2Add(Offset,V2(0, 0.5f*Dim.y)),
	    OffsetZ,
	    V2(Dim.x, Thickness),
	    Color, EntityZC);
  
  PushRect(Group,
	    V2Sub(Offset, V2(0.5f*Dim.x, 0)),
	    OffsetZ,
	    V2(Thickness, Dim.y),
	    Color, EntityZC);
  PushRect(Group,
	    V2Add(Offset,V2(0.5f*Dim.x, 0)),
	    OffsetZ,
	    V2(Thickness, Dim.y),
	    Color, EntityZC);
}

internal inline void
Clear(render_group* Group, v4 Color)
{
  render_entry_clear* Entry = PushRenderElement(Group, render_entry_clear);
  if(Entry)
    {
      Entry->Color = Color;      
    }  
}

internal inline render_entry_coordinate_system*
CoordinateSystem(render_group* Group, v2 Origin, v2 XAxis, v2 YAxis,
		 v4 Color, loaded_bitmap* Texture)
{
  render_entry_coordinate_system* Entry = PushRenderElement(Group, render_entry_coordinate_system);
  if(Entry)
    {
      Entry->Origin = Origin;
      Entry->XAxis = XAxis;
      Entry->YAxis = YAxis;
      Entry->Color = Color;     
      Entry->Texture = Texture;
    }
  return(Entry);
}

