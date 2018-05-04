
internal v4
UnscaleAndBiasNormal(v4 Normal)
{
  v4 Result;

  r32 Inv255 = 1.0f / 255.0f;

  Result.x = (2.0f*(Inv255 * Normal.x)) - 1.0f;
  Result.y = (2.0f*(Inv255 * Normal.y)) - 1.0f;
  Result.z = (2.0f*(Inv255 * Normal.z)) - 1.0f;

  Result.w = Inv255 * Normal.w;
  
  return(Result);
}

internal void
DrawBitmap(loaded_bitmap *Buffer, loaded_bitmap *Bitmap,
	   r32 RealX,  r32 RealY, r32 CAlpha)
{
  timed_block TB_DrawBitmap = BEGIN_TIMED_BLOCK(1);
  
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
	  v4 Texel = V4((r32)((*Source >> 16) & 0xff),
			(r32)((*Source >> 8)  & 0xff),
			(r32)((*Source >> 0)  & 0xff),
			(r32)((*Source >> 24) & 0xff));

	  Texel = SRGB255ToLinear1(Texel);
	  Texel = V4MulS(CAlpha, Texel); 
	  
	  v4 D = V4((r32)((*Dest >> 16) & 0xff),
		    (r32)((*Dest >> 8)  & 0xff),
		    (r32)((*Dest >> 0)  & 0xff),
		    (r32)((*Dest >> 24) & 0xff));

	  D = SRGB255ToLinear1(D);
	  
	  r32 InvRSA= (1.0f - Texel.a);

	  v4 Result = V4(InvRSA*D.r + Texel.r,
			 InvRSA*D.g + Texel.g,
			 InvRSA*D.b + Texel.b,
			 InvRSA*D.a + Texel.a);

	  Result = Linear1ToSRGB255(Result);

	  *Dest = (((u32)(Result.a + 0.5f) << 24) |
		   ((u32)(Result.r + 0.5f) << 16) |
		   ((u32)(Result.g + 0.5f) << 8)  |
		   ((u32)(Result.b + 0.5f) << 0));
	  
	  ++Dest;
	  ++Source;
	}
      SourceRow += Bitmap->Pitch;
      DestRow   += Buffer->Pitch;
    }

  END_TIMED_BLOCK(TB_DrawBitmap);
}

internal void
ChangeSaturation(loaded_bitmap *Buffer, r32 Level)
{
  u8* DestRow = (u8*)Buffer->Memory;
  for(int Y = 0;
      Y < Buffer->Height;
      ++Y)
    {
      u32 *Dest   = (u32*) DestRow;
      for(int X = 0;
	  X < Buffer->Width;
	  ++X)
	{
	  v4 D = V4((r32)((*Dest >> 16) & 0xff),
		    (r32)((*Dest >> 8)  & 0xff),
		    (r32)((*Dest >> 0)  & 0xff),
		    (r32)((*Dest >> 24) & 0xff));
	  
	  D = SRGB255ToLinear1(D);

	  r32 Avg  = (1.0f / 3.0f) * (D.r + D.g + D.b);
	  v3 Delta = V3(D.r - Avg, D.g - Avg, D.b - Avg);
	  	  
	  v4 Result = ToV4(V3Add(V3(Avg, Avg, Avg),
				 V3MulS(Level, Delta)),
			   D.a);

	  Result = Linear1ToSRGB255(Result);

	  *Dest = (((u32)(Result.a + 0.5f) << 24) |
		   ((u32)(Result.r + 0.5f) << 16) |
		   ((u32)(Result.g + 0.5f) << 8)  |
		   ((u32)(Result.b + 0.5f) << 0));
	  
	  ++Dest;
	}
      DestRow += Buffer->Pitch;
    }
}

internal void
DrawRectangle(loaded_bitmap *Buffer,
	      v2 vMin, v2 vMax,
	      v4 Color,
	      rectangle2i ClipRect,
	      bool32 Even)
{
  timed_block TB_DrawRectangle = BEGIN_TIMED_BLOCK(1);

  r32 R = Color.r;
  r32 G = Color.g;
  r32 B = Color.b;
  r32 A = Color.a;

  rectangle2i FillRect;
  FillRect.MinX = RoundReal32ToInt32(vMin.x);
  FillRect.MinY = RoundReal32ToInt32(vMin.y);
  FillRect.MaxX = RoundReal32ToInt32(vMax.x);
  FillRect.MaxY = RoundReal32ToInt32(vMax.y);

  FillRect = Intersect(FillRect, ClipRect);
  if((!Even) == (FillRect.MinY & 1))
    {
      FillRect.MinY += 1;
    }

  u32 Color32 = ((RoundReal32ToUInt32(A * 255.0f) << 24) |
		 (RoundReal32ToUInt32(R * 255.0f) << 16) |
		 (RoundReal32ToUInt32(G * 255.0f) << 8)  |
		 (RoundReal32ToUInt32(B * 255.0f)));
  
  u32 BytesPerPixel = BITMAP_BYTES_PER_PIXEL;
  u8 *Row = (u8*)Buffer->Memory
    + Buffer->Pitch * FillRect.MinY
    + BytesPerPixel * FillRect.MinX;
  for(int Y = FillRect.MinY;
      Y < FillRect.MaxY;
      Y+= 2)
    {
      u32 *Pixel = (u32*) Row;
      for(int X = FillRect.MinX;
	  X < FillRect.MaxX;
	  ++X)
	{
	  *Pixel++ = Color32;
	}
      Row += 2*Buffer->Pitch; 
    }

  END_TIMED_BLOCK(TB_DrawRectangle);
}

internal inline v4
Unpack4x8(u32 Packed)
{
  v4 Result = V4((r32)((Packed >> 16) & 0xff),
		 (r32)((Packed >> 8) & 0xff),
		 (r32)((Packed >> 0) & 0xff),
		 (r32)((Packed >> 24) & 0xff));
  return(Result);
}

typedef struct
{
  u32 A, B, C, D;
} bilinear_sample;
  
internal inline bilinear_sample
BilinearSample(loaded_bitmap* Texture, s32 X, s32 Y)
{
  bilinear_sample Result;
  
  u8* TexelPtr = (((u8*)Texture->Memory)
		  + Y * Texture->Pitch
		  + X * BITMAP_BYTES_PER_PIXEL);

  Result.A = *(u32*)(TexelPtr);
  Result.B = *(u32*)(TexelPtr + BITMAP_BYTES_PER_PIXEL);
  Result.C = *(u32*)(TexelPtr + Texture->Pitch);
  Result.D = *(u32*)(TexelPtr + Texture->Pitch + BITMAP_BYTES_PER_PIXEL);

  return(Result);
}

internal inline v4
SRGBBilinearBlend(bilinear_sample TexelSample, r32 fX, r32 fY)
{  	      
  v4 TexelA = Unpack4x8(TexelSample.A);
  v4 TexelB = Unpack4x8(TexelSample.B);
  v4 TexelC = Unpack4x8(TexelSample.C);
  v4 TexelD = Unpack4x8(TexelSample.D);
	      
  TexelA = SRGB255ToLinear1(TexelA);
  TexelB = SRGB255ToLinear1(TexelB);
  TexelC = SRGB255ToLinear1(TexelC);
  TexelD = SRGB255ToLinear1(TexelD);
	      
  v4 Result = V4Lerp(V4Lerp(TexelA, fX, TexelB),
		     fY,
		     V4Lerp(TexelC, fX, TexelD));

  return(Result);
}

internal v3
SampleEnvironmentMap(v2 ScreenSpaceUV, v3 SampleDirection, r32 Roughness,
		     environment_map *Map, r32 DistanceFromMapInZ)
{

  u32 LODIndex = (u32)(Roughness * (r32)(ArrayCount(Map->LOD) - 1) + 0.5f);
  Assert(LODIndex < ArrayCount(Map->LOD));

  loaded_bitmap* LOD = &Map->LOD[LODIndex];

  r32 UVsPerMeter = 0.1f;
  r32 C = (UVsPerMeter*DistanceFromMapInZ) / SampleDirection.y;
  v2 Offset = V2MulS(C, V2(SampleDirection.x, SampleDirection.z));

  v2 UV = V2Add(ScreenSpaceUV, Offset);
  
  UV.x = Clamp01(UV.x);
  UV.y = Clamp01(UV.y);
  
  r32 tX = ((UV.x * (r32)(LOD->Width  - 2)));
  r32 tY = ((UV.y * (r32)(LOD->Height - 2)));
  
  s32 X = (s32) tX;
  s32 Y = (s32) tY;

  r32 fX = tX - (r32)X;
  r32 fY = tY - (r32)Y;
	      
  Assert((X >= 0.0f) && (X < LOD->Width));
  Assert((Y >= 0.0f) && (Y < LOD->Height));

#if 0
  u8* TexelPtr = ((u8*)LOD->Memory) + Y*LOD->Pitch + X*BITMAP_BYTES_PER_PIXEL;
  *(u32*)TexelPtr = 0xFFFFFFFF;
#endif  
  bilinear_sample Sample = BilinearSample(LOD, X, Y);
  v3 Result = SRGBBilinearBlend(Sample, fX, fY).xyz;

  return(Result);
}


internal void
DrawRectangleSlowly(loaded_bitmap *Buffer,
		    v2 Origin, v2 XAxis, v2 YAxis,
		    v4 Color, loaded_bitmap* Texture,
		    loaded_bitmap* NormalMap,
		    environment_map *Top,
		    environment_map *Middle,
		    environment_map *Bottom,
		    r32 PixelsToMeters)
{
  timed_block TB_DrawRectangleSlowly = BEGIN_TIMED_BLOCK(1);

  Color.rgb = V3MulS(Color.a, Color.rgb);
  
  r32 XAxisLength = V2Length(XAxis);
  r32 YAxisLength = V2Length(YAxis);
  
  v2 NxAxis = V2MulS(YAxisLength / XAxisLength,
		     XAxis);
  v2 NyAxis = V2MulS(XAxisLength / YAxisLength,
		     YAxis);
  r32 NzScale = 0.5f * (XAxisLength + YAxisLength);
		  
  r32 InvXAxisLengthSq = 1.0f / V2LengthSq(XAxis);
  r32 InvYAxisLengthSq = 1.0f / V2LengthSq(YAxis);

  u32 Color32 = ((RoundReal32ToUInt32(Color.a * 255.0f) << 24) |
		 (RoundReal32ToUInt32(Color.r * 255.0f) << 16) |
		 (RoundReal32ToUInt32(Color.g * 255.0f) << 8)  |
		 (RoundReal32ToUInt32(Color.b * 255.0f) << 0));

  s32 WidthMax  = Buffer->Width - 1;
  s32 HeightMax = Buffer->Height - 1;

  r32 InvWidthMax  = 1.0f / (r32)WidthMax;
  r32 InvHeightMax = 1.0f / (r32)HeightMax;

  r32 OriginZ = 0.0f;
  r32 OriginY = V2Add(V2Add(Origin,
			    V2MulS(0.5f, XAxis)),
		      V2MulS(0.5f, YAxis)).y;
  r32 FixedCastY = InvHeightMax * OriginY;
    
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
  u8 *Row = ((u8*)Buffer->Memory
	     + Buffer->Pitch * YMin
	     + BytesPerPixel * XMin);

  timed_block TB_ProcessPixel = BEGIN_TIMED_BLOCK((XMax - XMin + 1) * (YMax - YMin + 1));
  for(int Y = YMin;
      Y <= YMax;
      ++Y)
    {
      u32* Pixel = (u32*) Row;
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
#if 1
	      v2 ScreenSpaceUV = V2(InvWidthMax*(r32)X, FixedCastY);
	      r32 ZDiff = PixelsToMeters*((r32)Y - OriginY);
#else
	      v2 ScreenSpaceUV = V2(InvWidthMax*(r32)X, InvHeightMax*(r32)Y);
	      r32 ZDiff = 0.0f;
#endif
	      r32 U = InvXAxisLengthSq * V2Inner(d, XAxis);
	      r32 V = InvYAxisLengthSq * V2Inner(d, YAxis);

#if 0
	      Assert((U >= 0.0f) && (U <= 1.0f));
	      Assert((V >= 0.0f) && (V <= 1.0f));
#endif
	      r32 tX = ((U * (r32)(Texture->Width  - 2)));
	      r32 tY = ((V * (r32)(Texture->Height - 2)));
	      
	      s32 X = (s32) tX;
	      s32 Y = (s32) tY;

	      r32 fX = tX - (r32)X;
	      r32 fY = tY - (r32)Y;
	      
	      Assert((X >= 0.0f) && (X < Texture->Width));
	      Assert((Y >= 0.0f) && (Y < Texture->Height));

	      bilinear_sample TexelSample
		= BilinearSample(Texture, X, Y);
	      v4 Texel = SRGBBilinearBlend(TexelSample, fX, fY);

#if 0
	      if(NormalMap)
		{		  
		  bilinear_sample NormalSample
		    = BilinearSample(NormalMap, X, Y);
		  
		  v4 NormalA = Unpack4x8(NormalSample.A);
		  v4 NormalB = Unpack4x8(NormalSample.B);
		  v4 NormalC = Unpack4x8(NormalSample.C);
		  v4 NormalD = Unpack4x8(NormalSample.D);
	      
		  v4 Normal = V4Lerp(V4Lerp(NormalA, fX, NormalB),
				     fY,
				     V4Lerp(NormalC, fX, NormalD));

		  Normal = UnscaleAndBiasNormal(Normal);
		  
		  Normal.xy = V2Add(V2MulS(Normal.x, NxAxis),
				    V2MulS(Normal.y, NyAxis));
		  Normal.z *= NzScale;
		  Normal.xyz = V3Normalize(Normal.xyz);

		  v3 BounceDirection = V3MulS(2.0f * Normal.z,
					      Normal.xyz);
		  BounceDirection.z -= 1.0f;

		  BounceDirection.z = -BounceDirection.z;

		  environment_map* FarMap = 0;
		  r32 Pz = OriginZ + ZDiff;
		  r32 MapInZ  = 2.0f;
		  r32 tFarMap = 0.0f;
		  r32 tEnvMap = BounceDirection.y;
		  if(tEnvMap < -0.5f)
		    {
		      FarMap = Bottom;
		      tFarMap = -1.0f - 2.0f*tEnvMap;
		    }
		  else if(tEnvMap > 0.5f)
		    {
		      FarMap = Top;
		      tFarMap = -1.0f + 2.0f*tEnvMap;
		    }

		  tFarMap *= tFarMap;
		  
		  v3 LightColor = V3(0.0f, 0.0f, 0.0f);
		  if(FarMap)
		    {
		      r32 DistanceFromMapInZ = FarMap->Pz - Pz;
		      v3 FarMapColor = SampleEnvironmentMap(ScreenSpaceUV, BounceDirection, Normal.w,
							    FarMap, DistanceFromMapInZ);
		      LightColor = V3Lerp(LightColor, tFarMap, FarMapColor);
		    }

		  Texel.rgb = V3Add(Texel.rgb,
				    V3MulS(Texel.a, LightColor));
#if 0 
		  Texel.rgb = V3Add(V3(0.5f, 0.5f, 0.5f),
		  		    V3MulS(0.5f, BounceDirection));
		  Texel.rgb = V3MulS(Texel.a, Texel.rgb);
#endif
		}
#endif

	      Texel = V4Hadamard(Texel, Color);
	      Texel.r = Clamp01(Texel.r);
	      Texel.g = Clamp01(Texel.g);
	      Texel.b = Clamp01(Texel.b);
	      
	      v4 Dest = V4((r32)((*Pixel >> 16) & 0xff),
			   (r32)((*Pixel >>  8) & 0xff),
			   (r32)((*Pixel >>  0) & 0xff),
			   (r32)((*Pixel >> 24) & 0xff));

	      Dest = SRGB255ToLinear1(Dest);

	      v4 Blended = V4Add(V4MulS((1.0f - Texel.a), Dest),
				 Texel);
	      
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
  END_TIMED_BLOCK(TB_ProcessPixel);
 
  END_TIMED_BLOCK(TB_DrawRectangleSlowly);
}

typedef struct
{
  s32 mm_add_ps;
  s32 mm_mul_ps;
  s32 mm_castps_si128; 
  s32 mm_and_ps;
  s32 mm_or_si128;
  s32 mm_andnot_si128;
  s32 mm_min_ps;
  s32 mm_max_ps;
  s32 mm_cmple_ps;
  s32 mm_cmpge_ps;
  s32 mm_cvtepi32_ps;
  s32 mm_cvtps_epi32;
  s32 mm_cvttps_epi32;
  s32 mm_sub_ps;
  s32 mm_and_si128;
  s32 mm_srli_epi32;
  s32 mm_slli_epi32;
  s32 mm_sqrt_ps;
} counts;

internal void
RenderGroupToOutput(render_group* RenderGroup, loaded_bitmap* OutputTarget, rectangle2i ClipRect, bool32 Even)
{
  timed_block TB_RenderGroupToOutput = BEGIN_TIMED_BLOCK(1);

  Assert(RenderGroup->InsideRender);

  r32 NullPixelsToMeters = 1.0f;
  
  for(u32 BaseAddress = 0;
      BaseAddress < RenderGroup->PushBufferSize;)
    {	      
      render_group_entry_header *Header
	= (render_group_entry_header*)(RenderGroup->PushBufferBase + BaseAddress);
      BaseAddress += sizeof(*Header);

      void* Data = (u8*) Header + sizeof(*Header);
      switch(Header->Type)
	{
	case RenderGroupEntryType_render_entry_clear:
	  {
	    render_entry_clear* Entry = (render_entry_clear*) Data; 
	    DrawRectangle(OutputTarget,
			  V2(0.0f, 0.0f),
			  V2((r32)OutputTarget->Width,
			     (r32)OutputTarget->Height),
			  Entry->Color,
			  ClipRect,
			  Even);


	    BaseAddress += sizeof(*Entry);
	  } break;

	case RenderGroupEntryType_render_entry_saturation:
	  {
	    render_entry_saturation* Entry = (render_entry_saturation*) Data; 

	    ChangeSaturation(OutputTarget, Entry->Level);

	    BaseAddress += sizeof(*Entry);
	  } break;

	case RenderGroupEntryType_render_entry_bitmap:
	  {
	    render_entry_bitmap* Entry = (render_entry_bitmap*) Data;
	    Assert(Entry->Bitmap);
#if 0
	    DrawRectangleSlowly(OutputTarget,
				Entry->P,
				V2(Entry->Size.x, 0.0f),
				V2(0.0f, Entry->Size.y),
				Entry->Color,
				Entry->Bitmap,
				0, 0, 0, 0,
				NullPixelsToMeters);
#else
	    v2 XAxis = V2(1.0f, 0.0f);//V2(Cos(Angle), Sin(Angle));
	    v2 YAxis = V2(0.0f, 1.0f);//V2Perp(XAxis);
	    DrawRectangleQuickly(OutputTarget,
				 Entry->P,
				 V2MulS(Entry->Size.x, XAxis),
				 V2MulS(Entry->Size.y, YAxis),
				 Entry->Color,
				 Entry->Bitmap,
				 NullPixelsToMeters,
				 ClipRect,
				 Even);
#endif

	    BaseAddress += sizeof(*Entry);
	  } break;

	case RenderGroupEntryType_render_entry_rectangle:
	  {
	    render_entry_rectangle* Entry = (render_entry_rectangle*) Data;

	    DrawRectangle(OutputTarget,
	    		  Entry->P,
	    		  V2Add(Entry->P,
	    			Entry->Dim),
	    		  Entry->Color,
			  ClipRect,
			  Even);
	    
	    BaseAddress += sizeof(*Entry);
	  } break;
	case RenderGroupEntryType_render_entry_coordinate_system:
	  {
	    render_entry_coordinate_system* Entry = (render_entry_coordinate_system*) Data;
#if 0 	    
	    DrawRectangleSlowly(OutputTarget,
				Entry->Origin,
				Entry->XAxis,
				Entry->YAxis,
				Entry->Color,
				Entry->Texture,
				Entry->NormalMap,
				Entry->Top,
				Entry->Middle,
				Entry->Bottom,
				PixelsToMeters);
	    
	    v4 Color = V4(1.0f, 1.0f, 0.0f, 1.0f);
	    v2 Dim = V2(2, 2);
	    v2 P = Entry->Origin; 
	    DrawRectangle(OutputTarget,
			  V2Sub(P, Dim),
			  V2Add(P, Dim),
			  Color);
	    
	    P = V2Add(Entry->Origin, Entry->XAxis); 
	    DrawRectangle(OutputTarget,
			  V2Sub(P, Dim),
			  V2Add(P, Dim),
			  Color);

	    P = V2Add(Entry->Origin, Entry->YAxis); 
	    DrawRectangle(OutputTarget,
			  V2Sub(P, Dim),
			  V2Add(P, Dim),
			  Color);

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
#endif
	    BaseAddress += sizeof(*Entry);	    
	  } break;
	  
	  InvalidCase;
	}
    }

  END_TIMED_BLOCK(TB_RenderGroupToOutput);
}

typedef struct
{
  render_group*  RenderGroup;
  loaded_bitmap* OutputTarget;
  rectangle2i ClipRect;
} tile_render_work;

internal
PLATFORM_WORK_QUEUE_CALLBACK(DoTiledRenderWork)
{
  tile_render_work* Work = (tile_render_work*) Data;

  RenderGroupToOutput(Work->RenderGroup, Work->OutputTarget, Work->ClipRect, false);
  RenderGroupToOutput(Work->RenderGroup, Work->OutputTarget, Work->ClipRect, true);
}

//TODO: Casey can have multiple functions with the same name but we, alas, cannot. Consider
//      how we want to name this to not confuse us in future episodes.
internal void
RenderGroupToOutput2(render_group* RenderGroup, loaded_bitmap* OutputTarget)
{
  timed_block TB_RenderGroupToOutput2 = BEGIN_TIMED_BLOCK(1);

  Assert(RenderGroup->InsideRender);
  Assert(((uintptr)OutputTarget->Memory & 15) == 0);
  
  rectangle2i ClipRect;
  ClipRect.MinX = 0;
  ClipRect.MaxX = OutputTarget->Width;
  ClipRect.MinY = 0;
  ClipRect.MaxY = OutputTarget->Height;
	  
  tile_render_work Work;
  Work.RenderGroup  = RenderGroup;
  Work.OutputTarget = OutputTarget;
  Work.ClipRect = ClipRect;
    
  DoTiledRenderWork(0, &Work);  

  END_TIMED_BLOCK(TB_RenderGroupToOutput2);
}

internal void
TiledRenderGroupToOutput(platform_work_queue* RenderQueue,
			 render_group* RenderGroup, loaded_bitmap* OutputTarget)
{
  timed_block TB_TiledGroupToOutput = BEGIN_TIMED_BLOCK(1);

  Assert(RenderGroup->InsideRender);
  
  s32 const TileCountX = 4;
  s32 const TileCountY = 4;
  tile_render_work WorkArray[TileCountX*TileCountY];

  Assert(((uintptr)OutputTarget->Memory & 15) == 0);
  
  s32 TileWidth  = OutputTarget->Width  / TileCountX;
  s32 TileHeight = OutputTarget->Height / TileCountY;

  TileWidth = ((TileWidth + 3) / 4) * 4;

  s32 WorkCount = 0;
  for(s32 TileY = 0;
      TileY < TileCountY;
      ++TileY)
    {
      for(s32 TileX = 0;
	  TileX < TileCountX;
	  ++TileX)
	{
	  tile_render_work* Work = WorkArray + WorkCount++;

	  rectangle2i ClipRect;
	  ClipRect.MinX = TileX*TileWidth;
	  ClipRect.MaxX = ClipRect.MinX + TileWidth;
	  ClipRect.MinY = TileY*TileHeight;
	  ClipRect.MaxY = ClipRect.MinY + TileHeight;

	  if(TileX == (TileCountX - 1))
	    {
	      ClipRect.MaxX = OutputTarget->Width;
	    }

	  if(TileY == (TileCountY - 1))
	    {
	      ClipRect.MaxY = OutputTarget->Height;
	    }
	  
	  Work->RenderGroup  = RenderGroup;
	  Work->OutputTarget = OutputTarget;
	  Work->ClipRect = ClipRect;
#if 1
	  Platform.AddEntry(RenderQueue, DoTiledRenderWork, Work);
#else
	  DoTiledRenderWork(RenderQueue, Work)
#endif
	}
    }

  Platform.CompleteAllWork(RenderQueue);

  END_TIMED_BLOCK(TB_TiledGroupToOutput);
}

internal render_group*
AllocateRenderGroup(assets* Assets, memory_arena *Arena, u32 MaxPushBufferSize, bool32 RendersInBackground)
{
  render_group* Result = PushStruct(Arena, render_group);

  if(MaxPushBufferSize == 0)
    {
      MaxPushBufferSize = (u32)GetArenaSizeRemaining(Arena, 4);
    }

  Result->PushBufferBase = PushSize(Arena, MaxPushBufferSize);
    
  Result->MaxPushBufferSize = MaxPushBufferSize;
  Result->PushBufferSize = 0;

  Result->Assets = Assets;
  Result->GlobalAlpha = 1.0f;

  Result->GenerationID = 0;
    
  Result->Transform.OffsetP = V3(0.0f, 0.0f, 0.0f);
  Result->Transform.Scale = 1.0f;

  Result->MissingResourceCount = 0;
  Result->RendersInBackground = RendersInBackground;

  Result->InsideRender = false;
  
  return(Result);
}

internal void
BeginRender(render_group* Group)
{
  timed_block TB_BeginRender = BEGIN_TIMED_BLOCK(1);

  if(Group)
    {
      Assert(!Group->InsideRender);
      Group->InsideRender = true;
      
      Group->GenerationID = BeginGeneration(Group->Assets);
    }

  END_TIMED_BLOCK(TB_BeginRender);  
}

internal void
EndRender(render_group* Group)
{
  timed_block TB_EndRender = BEGIN_TIMED_BLOCK(1);

  if(Group)
    {
      Assert(Group->InsideRender);
      Group->InsideRender = false;

      EndGeneration(Group->Assets, Group->GenerationID);
      Group->GenerationID = 0;
      Group->PushBufferSize = 0;
    }

  END_TIMED_BLOCK(TB_EndRender);  
}

internal inline void
Perspective(render_group* RenderGroup, s32 PixelWidth, s32 PixelHeight,
	    r32 MetersToPixels, r32 FocalLength, r32 DistanceAboveTarget)
{
  r32 PixelsToMeters = SafeRatio1(1.0, MetersToPixels);
  RenderGroup->MonitorHalfDimInMeters = V2(0.5f* PixelWidth * PixelsToMeters,
				      0.5f* PixelHeight * PixelsToMeters);

  RenderGroup->Transform.MetersToPixels = MetersToPixels; 
  RenderGroup->Transform.FocalLength = FocalLength;
  RenderGroup->Transform.DistanceAboveTarget = DistanceAboveTarget;
  RenderGroup->Transform.ScreenCenter = V2(0.5f*PixelWidth,
				      0.5f*PixelHeight);

  RenderGroup->Transform.Orthographic = false;
}

internal inline void
Orthographic(render_group* RenderGroup, s32 PixelWidth, s32 PixelHeight,
	    r32 MetersToPixels)
{
  r32 PixelsToMeters = SafeRatio1(1.0, MetersToPixels);
  RenderGroup->MonitorHalfDimInMeters = V2(0.5f* PixelWidth * PixelsToMeters,
				      0.5f* PixelHeight * PixelsToMeters);

  RenderGroup->Transform.MetersToPixels = MetersToPixels; 
  RenderGroup->Transform.FocalLength = 1.0f;
  RenderGroup->Transform.DistanceAboveTarget = 1.0f;
  RenderGroup->Transform.ScreenCenter = V2(0.5f*PixelWidth,
				      0.5f*PixelHeight);

  RenderGroup->Transform.Orthographic = true;
}

typedef struct
{
  v2 P;
  r32 Scale;
  bool32 Valid;
} entity_basis_p;
  
internal inline entity_basis_p
GetRenderEntityBasisP(render_transform* Transform, v3 OriginalP)
{ 
  entity_basis_p Result = {};

  v3 P = V3Add(ToV3(OriginalP.xy, 0.0f), Transform->OffsetP);

  if(!Transform->Orthographic)
    {
      r32 OffsetZ = 0;
  
      r32 DistanceAboveTarget = Transform->DistanceAboveTarget;
#if 0
      if(1)
	{
	  DistanceAboveTarget += 50.0f;
	}
#endif
  
      r32 DistanceToPZ = DistanceAboveTarget - P.z;
      r32 NearClipPlane = 0.2f;

      v3 RawXY = ToV3(P.xy, 1.0f); 

      if(DistanceToPZ > NearClipPlane)
	{
	  v3 ProjectedXY = V3MulS((1.0f / DistanceToPZ),
				  V3MulS(Transform->FocalLength,
					 RawXY));

	  Result.Scale = Transform->MetersToPixels * ProjectedXY.z;
	  Result.P = V2Add(Transform->ScreenCenter,
			   V2Add(V2MulS(Transform->MetersToPixels,
					ProjectedXY.xy),
				 V2(0.0f, Result.Scale * OffsetZ)));
	  Result.Valid = true;
	}
    }
  else
    {
      Result.P = V2Add(Transform->ScreenCenter,
		       V2MulS(Transform->MetersToPixels,
			      P.xy));
      Result.Scale = Transform->MetersToPixels;
      Result.Valid = true;
    }
  
  return(Result);
}

#define PushRenderElement(Group, type) (type*) PushRenderElement_(Group, sizeof(type), RenderGroupEntryType_##type) 
internal inline void*
PushRenderElement_(render_group *Group, u32 Size, render_group_entry_type Type)
{
  timed_block TB_PushRenderElement_ = BEGIN_TIMED_BLOCK(1);  
  
  Assert(Group->InsideRender);
  
  void* Result = 0;

  Size += sizeof(render_group_entry_header);
  
  if((Group->PushBufferSize + Size) < Group->MaxPushBufferSize)
    {
      render_group_entry_header* Header = (render_group_entry_header*) (Group->PushBufferBase + Group->PushBufferSize);
      Header->Type = Type;
      Result = (Header + 1);
      Group->PushBufferSize += Size;
    }
  else
    {
      InvalidCodePath;
    }

  END_TIMED_BLOCK(TB_PushRenderElement_);  
  
  return(Result);
}

internal inline void
PushBitmap(render_group *Group, loaded_bitmap* Bitmap, v3 Offset, r32 Height, v4 Color)
{
  v2 Size  = V2(Height * Bitmap->WidthOverHeight, Height);
  v2 Align = V2Hadamard(Bitmap->AlignPercentage, Size);
  v3 P = V3Sub(Offset,
	       ToV3(Align, 0.0f));
  
  entity_basis_p Basis = GetRenderEntityBasisP(&Group->Transform, P);
  if(Basis.Valid)
    {
      render_entry_bitmap* Entry = PushRenderElement(Group, render_entry_bitmap);
      if(Entry)
	{
	  Entry->Bitmap = Bitmap;
	  Entry->P = Basis.P;  
	  Entry->Color = V4MulS(Group->GlobalAlpha, Color);
	  Entry->Size  = V2MulS(Basis.Scale, Size);
	}
    }
}

internal inline void
PushBitmapByID(render_group *Group, bitmap_id ID, v3 Offset, r32 Height, v4 Color)
{
  loaded_bitmap* Bitmap = GetBitmap(Group->Assets, ID, Group->GenerationID);
  if(Group->RendersInBackground && !Bitmap)
    {
      LoadBitmap(Group->Assets, ID, true);
      Bitmap = GetBitmap(Group->Assets, ID, Group->GenerationID);
    }

  if(Bitmap)
    {
      PushBitmap(Group, Bitmap, Offset, Height, Color);
    }
  else
    {
      Assert(!Group->RendersInBackground);
      LoadBitmap(Group->Assets, ID, false);
      ++Group->MissingResourceCount;
    }
}

internal inline loaded_font*
PushFont(render_group *Group, font_id ID)
{
  loaded_font* Font = GetFont(Group->Assets, ID, Group->GenerationID);
  if(Font)
    {
      
    }
  else
    {
      Assert(!Group->RendersInBackground);
      LoadFont(Group->Assets, ID, false);
      ++Group->MissingResourceCount;
    }
  
  return(Font);
}

internal inline void
PushRect(render_group *Group, v3 Offset, v2 Dim, v4 Color)
{
  v2 HalfDim = V2MulS(0.5f, Dim);
  v3 P = V3Sub(Offset,
	       ToV3(HalfDim, 0.0f));
  
  entity_basis_p Basis = GetRenderEntityBasisP(&Group->Transform, P);
  if(Basis.Valid)
    {
      render_entry_rectangle* Rect = PushRenderElement(Group, render_entry_rectangle);
      if(Rect)
	{
	  Rect->P = Basis.P;  
	  Rect->Color = Color;
	  Rect->Dim   = V2MulS(Basis.Scale, Dim);
	}
    }
}

internal inline void
PushRectOutline(render_group *Group, v3 Offset, v2 Dim, v4 Color)
{
  r32 Thickness = 0.1f;
    
  PushRect(Group,
	   V3Sub(Offset, V3(0.0f, 0.5f*Dim.y, 0.0f)),
	   V2(Dim.x, Thickness),
	   Color);
  PushRect(Group,
	   V3Add(Offset,V3(0.0f, 0.5f*Dim.y, 0.0f)),
	   V2(Dim.x, Thickness),
	   Color);
  
  PushRect(Group,
	   V3Sub(Offset, V3(0.5f*Dim.x, 0.0f, 0.0f)),
	   V2(Thickness, Dim.y),
	   Color);
  PushRect(Group,
	   V3Add(Offset,V3(0.5f*Dim.x, 0, 0.0f)),
	   V2(Thickness, Dim.y),
	   Color);
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

internal inline void
Saturation(render_group* Group, r32 Level)
{
  render_entry_saturation* Entry = PushRenderElement(Group, render_entry_saturation);
  if(Entry)
    {
      Entry->Level = Level;      
    }  
}

internal inline void
CoordinateSystem(render_group* Group, v2 Origin, v2 XAxis, v2 YAxis,
		 v4 Color, loaded_bitmap* Texture, loaded_bitmap* NormalMap,
		 environment_map* Top, environment_map* Middle, environment_map* Bottom)
{
#if 0
  entity_basis_p Basis = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenDim);
  if(Basis.Valid)
    {
      render_entry_coordinate_system* Entry = PushRenderElement(Group, render_entry_coordinate_system);
      if(Entry)
	{
	  Entry->Origin = Origin;
	  Entry->XAxis = XAxis;
	  Entry->YAxis = YAxis;
	  Entry->Color = Color;     
	  Entry->Texture = Texture;
	  Entry->NormalMap = NormalMap;
	  Entry->Top = Top;
	  Entry->Middle = Middle;
	  Entry->Bottom = Bottom;
	}
    }
#endif
}

inline internal v2
Unproject(render_group* Group, v2 ProjectedXY, r32 AtDistanceFromCamera)
{
  v2 WorldXY = V2MulS((AtDistanceFromCamera / Group->Transform.FocalLength), ProjectedXY);
  return(WorldXY);
}

inline internal rectangle2
GetCameraRectangleAtDistance(render_group* Group, r32 DistanceFromCamera)
{
  v2 RawXY = Unproject(Group, Group->MonitorHalfDimInMeters, DistanceFromCamera);

  rectangle2 Result = RectCenterHalfDim2(V2(0,0), RawXY);
  
  return(Result);
}
                            
inline internal rectangle2  
GetCameraRectangleAtTarget(render_group* Group)
{
  rectangle2 Result = GetCameraRectangleAtDistance(Group, Group->Transform.DistanceAboveTarget);
  return(Result);
}

internal inline bool32
AllResourcesPresent(render_group* Group)
{
  bool32 Result = (Group->MissingResourceCount == 0);
  return(Result);
}
