
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
	      v4 Color)
{
  r32 R = Color.r;
  r32 G = Color.g;
  r32 B = Color.b;
  r32 A = Color.a;
  
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

  u32 Color32 = ((RoundReal32ToUInt32(A * 255.0f) << 24) |
		 (RoundReal32ToUInt32(R * 255.0f) << 16) |
		 (RoundReal32ToUInt32(G * 255.0f) << 8)  |
		 (RoundReal32ToUInt32(B * 255.0f)));
  
  u32 BytesPerPixel = BITMAP_BYTES_PER_PIXEL;
  u8 *Row = (u8*)Buffer->Memory
    + Buffer->Pitch * MinY
    + BytesPerPixel * MinX;
  for(int Y = MinY; Y < MaxY; ++Y)
    {
      u32 *Pixel = (u32*) Row;
      for(int X = MinX; X < MaxX; ++X)
	{
	  *Pixel++ = Color32;
	}
      Row += Buffer->Pitch; 
    }
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
  BEGIN_TIMED_BLOCK(DrawRectangleSlowly);

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
  for(int Y = YMin;
      Y <= YMax;
      ++Y)
    {
      u32* Pixel = (u32*) Row;
      for(int X = XMin;
	  X <= XMax;
	  ++X)
	{
	  BEGIN_TIMED_BLOCK(FillPixel);
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
	      BEGIN_TIMED_BLOCK(TestPixel);
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

	      END_TIMED_BLOCK(TestPixel);
	    }
#else
	  *Pixel = Color32;
#endif

	  ++Pixel;

	  END_TIMED_BLOCK(FillPixel);
	}
      Row += Buffer->Pitch; 
    }
  
  END_TIMED_BLOCK(DrawRectangleSlowly);
}

internal void
DrawRectangleHopefullyQuickly(loaded_bitmap *Buffer,
			      v2 Origin, v2 XAxis, v2 YAxis,
			      v4 Color, loaded_bitmap* Texture,
			      r32 PixelsToMeters)
{
  BEGIN_TIMED_BLOCK(DrawRectangleHopefullyQuickly);

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

  v2 nXAxis = V2MulS(InvXAxisLengthSq, XAxis);
  v2 nYAxis = V2MulS(InvYAxisLengthSq, YAxis);

  r32 Inv255 = 1.0f / 255.0f;
  r32 One255 = 255.0f;
  
  u32 BytesPerPixel = BITMAP_BYTES_PER_PIXEL;
  u8 *Row = ((u8*)Buffer->Memory
	     + Buffer->Pitch * YMin
	     + BytesPerPixel * XMin);
  for(int Y = YMin;
      Y <= YMax;
      ++Y)
    {
      u32* Pixel = (u32*) Row;
      for(int X = XMin;
	  X <= XMax;
	  ++X)
	{
	  BEGIN_TIMED_BLOCK(FillPixel);

	  v2 PixelP = V2i(X, Y);
	  v2 d = V2Sub(PixelP, Origin);

	  r32 U = V2Inner(d, nXAxis);
	  r32 V = V2Inner(d, nYAxis);
	  
	  if((U >= 0.0f) &&
	     (U <= 1.0f) &&
	     (V >= 0.0f) &&
	     (V <= 1.0f))
	    {
	      BEGIN_TIMED_BLOCK(TestPixel);

	      r32 tX = ((U * (r32)(Texture->Width  - 2)));
	      r32 tY = ((V * (r32)(Texture->Height - 2)));
	      
	      s32 X = (s32) tX;
	      s32 Y = (s32) tY;

	      r32 fX = tX - (r32)X;
	      r32 fY = tY - (r32)Y;
	      
	      Assert((X >= 0.0f) && (X < Texture->Width));
	      Assert((Y >= 0.0f) && (Y < Texture->Height));

	      u8* TexelPtr = (((u8*)Texture->Memory)
			      + Y * Texture->Pitch
			      + X * BITMAP_BYTES_PER_PIXEL);
	      u32 SampleA = *(u32*)(TexelPtr);
	      u32 SampleB = *(u32*)(TexelPtr + BITMAP_BYTES_PER_PIXEL);
	      u32 SampleC = *(u32*)(TexelPtr + Texture->Pitch);
	      u32 SampleD = *(u32*)(TexelPtr + Texture->Pitch + BITMAP_BYTES_PER_PIXEL);
	      
	      r32 TexelaR = (r32)((SampleA >> 16) & 0xff);
	      r32 TexelaG = (r32)((SampleA >> 8) & 0xff);
	      r32 TexelaB = (r32)((SampleA >> 0) & 0xff);
	      r32 TexelaA = (r32)((SampleA >> 24) & 0xff);
	      
	      r32 TexelbR = (r32)((SampleB >> 16) & 0xff);
	      r32 TexelbG = (r32)((SampleB >> 8) & 0xff);
	      r32 TexelbB = (r32)((SampleB >> 0) & 0xff);
	      r32 TexelbA = (r32)((SampleB >> 24) & 0xff);
	      	      
	      r32 TexelcR = (r32)((SampleC >> 16) & 0xff);
	      r32 TexelcG = (r32)((SampleC >> 8) & 0xff);
	      r32 TexelcB = (r32)((SampleC >> 0) & 0xff);
	      r32 TexelcA = (r32)((SampleC >> 24) & 0xff);
	      	     
	      r32 TexeldR = (r32)((SampleD >> 16) & 0xff);
	      r32 TexeldG = (r32)((SampleD >> 8) & 0xff);
	      r32 TexeldB = (r32)((SampleD >> 0) & 0xff);
	      r32 TexeldA = (r32)((SampleD >> 24) & 0xff);
	      	      
	      TexelaR = Square(Inv255*TexelaR);
	      TexelaG = Square(Inv255*TexelaG);
	      TexelaB = Square(Inv255*TexelaB);
	      TexelaA = Inv255*TexelaA;

	      TexelbR = Square(Inv255*TexelbR);
	      TexelbG = Square(Inv255*TexelbG);
	      TexelbB = Square(Inv255*TexelbB);
	      TexelbA = Inv255*TexelbA;

	      TexelcR = Square(Inv255*TexelcR);
	      TexelcG = Square(Inv255*TexelcG);
	      TexelcB = Square(Inv255*TexelcB);
	      TexelcA = Inv255*TexelcA;

	      TexeldR = Square(Inv255*TexeldR);
	      TexeldG = Square(Inv255*TexeldG);
	      TexeldB = Square(Inv255*TexeldB);
	      TexeldA = Inv255*TexeldA;

	      r32 ifX = 1.0f - fX;
	      r32 ifY = 1.0f - fY;

	      r32 l0 = ifY*ifX;
	      r32 l1 = ifY*fX;
	      r32 l2 = fY*ifX;
	      r32 l3 = fY*fX;
	      
	      r32 TexelR = l0*TexelaR + l1*TexelbR + l2*TexelcR + l3*TexeldR;
	      r32 TexelG = l0*TexelaG + l1*TexelbG + l2*TexelcG + l3*TexeldG;
	      r32 TexelB = l0*TexelaB + l1*TexelbB + l2*TexelcB + l3*TexeldB;
	      r32 TexelA = l0*TexelaA + l1*TexelbA + l2*TexelcA + l3*TexeldA;
	      
	      TexelR = TexelR * Color.r;
	      TexelG = TexelG * Color.g;
	      TexelB = TexelB * Color.b;
	      TexelA = TexelA * Color.a;

	      TexelR = Clamp01(TexelR);
	      TexelG = Clamp01(TexelG);
	      TexelB = Clamp01(TexelB);
	      	      
	      r32 DestR = (r32)((*Pixel >> 16) & 0xff);
	      r32 DestG = (r32)((*Pixel >>  8) & 0xff);
	      r32 DestB = (r32)((*Pixel >>  0) & 0xff);
	      r32 DestA = (r32)((*Pixel >> 24) & 0xff);

	      DestR = Square(Inv255*DestR);
	      DestG = Square(Inv255*DestG);
	      DestB = Square(Inv255*DestB);
	      DestA = Inv255*DestA;

	      r32 InvTexelA = 1.0f - TexelA;
	      r32 BlendedR = InvTexelA * DestR + TexelR;
	      r32 BlendedG = InvTexelA * DestG + TexelG;
	      r32 BlendedB = InvTexelA * DestB + TexelB;
	      r32 BlendedA = InvTexelA * DestA + TexelA;
	      
	      BlendedR = One255*SquareRoot(BlendedR);
	      BlendedG = One255*SquareRoot(BlendedG);
	      BlendedB = One255*SquareRoot(BlendedB);
	      BlendedA = One255*BlendedA;

	      *Pixel = (((u32)(BlendedA + 0.5f) << 24) |
			((u32)(BlendedR + 0.5f) << 16) |
			((u32)(BlendedG + 0.5f) << 8)  |
			((u32)(BlendedB + 0.5f) << 0));

	      END_TIMED_BLOCK(TestPixel);
	    }

	  ++Pixel;

	  END_TIMED_BLOCK(FillPixel);
	}
      Row += Buffer->Pitch; 
    }
  
  END_TIMED_BLOCK(DrawRectangleHopefullyQuickly);
}

internal void
DrawRectangleOutline(loaded_bitmap *Buffer,
		     v2 vMin, v2 vMax,
		     v3 Color, r32 R)
{   
  DrawRectangle(Buffer,
		V2(vMin.x - R, vMin.y - R),
		V2(vMax.x + R, vMin.y + R),
		ToV4(Color, 1.0f));
  DrawRectangle(Buffer,
		V2(vMin.x - R, vMax.y - R),
		V2(vMax.x + R, vMax.y + R),
		ToV4(Color, 1.0f));
    
  DrawRectangle(Buffer,
		V2(vMin.x - R, vMin.y - R),
		V2(vMin.x + R, vMax.y + R),
		ToV4(Color, 1.0f));
  DrawRectangle(Buffer,
		V2(vMax.x - R, vMin.y - R),
		V2(vMax.x + R, vMax.y + R),
		ToV4(Color, 1.0f));
}

typedef struct
{
  v2 P;
  r32 Scale;
  bool32 Valid;
} entity_basis_p;
  
internal inline entity_basis_p
GetRenderEntityBasisP(render_group* RenderGroup,
		      render_entity_basis* EntityBasis,
		      v2 ScreenDim)
{ 
  entity_basis_p Result = {};

  v2 ScreenCenter = V2MulS(0.5f, ScreenDim);
  
  v3 EntityBaseP = EntityBasis->Basis->P;
 
  r32 DistanceToPZ = RenderGroup->RenderCamera.DistanceAboveTarget - EntityBaseP.z;
  r32 NearClipPlane = 0.2f;

  v3 RawXY = ToV3(V2Add(EntityBaseP.xy,
			EntityBasis->Offset.xy),
		  1.0f); 

  if(DistanceToPZ > NearClipPlane)
    {
      v3 ProjectedXY = V3MulS((1.0f / DistanceToPZ),
			      V3MulS(RenderGroup->RenderCamera.FocalLength,
				     RawXY));
  
      Result.P = V2Add(ScreenCenter, V2MulS(RenderGroup->MetersToPixels,
					    ProjectedXY.xy));
      Result.Scale = RenderGroup->MetersToPixels * ProjectedXY.z;
      Result.Valid = true;
    }
  
  return(Result);
}

internal void
RenderGroupToOutput(render_group* RenderGroup, loaded_bitmap* OutputTarget)
{
  BEGIN_TIMED_BLOCK(RenderGroupToOutput);
  
  v2 ScreenDim = V2((r32)OutputTarget->Width,
		    (r32)OutputTarget->Height);
  
  r32 PixelsToMeters = 1.0f / RenderGroup->MetersToPixels;

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
			  Entry->Color);


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
	    entity_basis_p Basis = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenDim);
	    Assert(Entry->Bitmap);
#if 0 
	    DrawBitmap(OutputTarget, Entry->Bitmap,
		       P.x, P.y, Entry->Color.a);
#else
	    DrawRectangleHopefullyQuickly(OutputTarget,
					  Basis.P,
					  V2MulS(Basis.Scale, V2(Entry->Size.x, 0.0f)),
					  V2MulS(Basis.Scale, V2(0.0f, Entry->Size.y)),
					  Entry->Color,
					  Entry->Bitmap,
					  PixelsToMeters);
#endif

	    BaseAddress += sizeof(*Entry);
	  } break;

	case RenderGroupEntryType_render_entry_rectangle:
	  {
	    render_entry_rectangle* Entry = (render_entry_rectangle*) Data;
	    entity_basis_p Basis = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenDim);
	    	    
	    DrawRectangle(OutputTarget,
	    		  Basis.P,
	    		  V2Add(Basis.P,
	    			V2MulS(Basis.Scale, Entry->Dim)),
	    		  Entry->Color);
	    
	    BaseAddress += sizeof(*Entry);
	  } break;
	case RenderGroupEntryType_render_entry_coordinate_system:
	  {
	    render_entry_coordinate_system* Entry = (render_entry_coordinate_system*) Data;
	    
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
	    BaseAddress += sizeof(*Entry);	    
	  } break;
	  
	  InvalidDefaultCase;
	}
    }

  END_TIMED_BLOCK(RenderGroupToOutput);
}

internal render_group*
AllocateRenderGroup(memory_arena *Arena, u32 MaxPushBufferSize,
		    u32 ResolutionPixelsX, u32 ResolutionPixelsY)
{
  render_group* Result = PushStruct(Arena, render_group);
  Result->PushBufferBase = PushSize(Arena, MaxPushBufferSize);

  Result->DefaultBasis = PushStruct(Arena, render_basis);
  Result->DefaultBasis->P = V3(0.0f, 0.0f, 0.0f);
    
  Result->MaxPushBufferSize = MaxPushBufferSize;
  Result->PushBufferSize = 0;

  Result->GlobalAlpha = 1.0f;
  Result->GameCamera.FocalLength = 0.6f;
  Result->GameCamera.DistanceAboveTarget = 9.0f;

  Result->RenderCamera = Result->GameCamera;
  //Result->RenderCamera.DistanceAboveTarget = 50.0f;
  
  r32 WidthOfMonitor = 0.635f;
  Result->MetersToPixels = (r32)ResolutionPixelsX * WidthOfMonitor;
  r32 PixelsToMeters = 1.0f / Result->MetersToPixels;

  Result->MonitorHalfDimInMeters = V2(0.5f*ResolutionPixelsX * PixelsToMeters,
				      0.5f*ResolutionPixelsY * PixelsToMeters);
  
  return(Result);
}


#define PushRenderElement(Group, type) (type*) PushRenderElement_(Group, sizeof(type), RenderGroupEntryType_##type) 
internal inline void*
PushRenderElement_(render_group *Group, u32 Size, render_group_entry_type Type)
{
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

  return(Result);
}

internal inline void
PushBitmap(render_group *Group, loaded_bitmap* Bitmap, v3 Offset, r32 Height, v4 Color)
{
  render_entry_bitmap* Entry = PushRenderElement(Group, render_entry_bitmap);
  if(Entry)
    {
      Entry->EntityBasis.Basis  = Group->DefaultBasis; 
      Entry->Bitmap = Bitmap;
      v2 Size  = V2(Height * Bitmap->WidthOverHeight,
		    Height);
      v2 Align = V2Hadamard(Bitmap->AlignPercentage, Size);
      Entry->EntityBasis.Offset = V3Sub(Offset,
					ToV3(Align, 0.0f));  
      Entry->Color = V4MulS(Group->GlobalAlpha, Color);
      Entry->Size  = Size;
    }
}

internal inline void
PushRect(render_group *Group, v3 Offset, v2 Dim, v4 Color)
{
  render_entry_rectangle* Piece = PushRenderElement(Group, render_entry_rectangle);
  if(Piece)
    {
      v2 HalfDim = V2MulS(0.5f, Dim);
      
      Piece->EntityBasis.Basis  = Group->DefaultBasis; 
      Piece->EntityBasis.Offset = V3Sub(Offset,
					ToV3(HalfDim, 0.0f));  
      Piece->Color = Color;
      Piece->Dim   = Dim;
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

internal inline render_entry_coordinate_system*
CoordinateSystem(render_group* Group, v2 Origin, v2 XAxis, v2 YAxis,
		 v4 Color, loaded_bitmap* Texture, loaded_bitmap* NormalMap,
		 environment_map* Top, environment_map* Middle, environment_map* Bottom)
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
  return(Entry);
}

inline internal v2
Unproject(render_group* Group, v2 ProjectedXY, r32 AtDistanceFromCamera)
{
  v2 WorldXY = V2MulS((AtDistanceFromCamera / Group->GameCamera.FocalLength), ProjectedXY);
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
  rectangle2 Result = GetCameraRectangleAtDistance(Group, Group->GameCamera.DistanceAboveTarget);
  return(Result);
}

