
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
	      v4 Color,
	      rectangle2i ClipRect,
	      bool32 Even)
{
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

  BEGIN_TIMED_BLOCK(ProcessPixel);
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
  END_TIMED_BLOCK_COUNTED(ProcessPixel, (XMax - XMin + 1) * (YMax - YMin + 1));
 
  END_TIMED_BLOCK(DrawRectangleSlowly);
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

#if 0
#include "iacaMarks.h"
#else
#define IACA_START
#define IACA_END
#endif

internal void
DrawRectangleQuickly(loaded_bitmap *Buffer,
		     v2 Origin, v2 XAxis, v2 YAxis,
		     v4 Color, loaded_bitmap* Texture,
		     r32 PixelsToMeters,
		     rectangle2i ClipRect,
		     bool32 Even)
{
  BEGIN_TIMED_BLOCK(DrawRectangleQuickly);

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

  rectangle2i FillRect = InvertedInfinityRectangle();
  
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
      s32 CeilX  = CeilReal32ToInt32(TestP.x) + 1;
      s32 FloorY = FloorReal32ToInt32(TestP.y);
      s32 CeilY  = CeilReal32ToInt32(TestP.y) + 1;

      if(FillRect.MinX > FloorX) {FillRect.MinX = FloorX;}
      if(FillRect.MinY > FloorY) {FillRect.MinY = FloorY;}
      if(FillRect.MaxX < CeilX)  {FillRect.MaxX = CeilX;}
      if(FillRect.MaxY < CeilY)  {FillRect.MaxY = CeilY;}
    }

  //rectangle2i ClipRect = {128, 128, 256, 256};
  //rectangle2i ClipRect = {0, 0, WidthMax, HeightMax};
  FillRect = Intersect(ClipRect, FillRect);

  if((!Even) == (FillRect.MinY & 1))
    {
      FillRect.MinY += 1;
    }

  if(HasArea(FillRect))
    {
      __m128i StartupClipMask = _mm_set1_epi32(0xFFFFFFFF);
      s32 FillWidth = FillRect.MaxX - FillRect.MinX;
      s32 FillWidthAlign = FillWidth & 3;
      if(FillWidthAlign > 0)
	{
	  s32 Adjustment = (4 - FillWidthAlign);
	  switch(Adjustment)
	    {
	  
	    case 1: { StartupClipMask = _mm_slli_si128(StartupClipMask, 1*4); } break;
	    case 2: { StartupClipMask = _mm_slli_si128(StartupClipMask, 2*4); } break;
	    case 3: { StartupClipMask = _mm_slli_si128(StartupClipMask, 3*4); } break;

	      InvalidDefaultCase;
	    }
      
	  FillWidth += Adjustment;
	  FillRect.MinX = FillRect.MaxX - FillWidth;
	}
    
      v2 nXAxis = V2MulS(InvXAxisLengthSq, XAxis);
      v2 nYAxis = V2MulS(InvYAxisLengthSq, YAxis);

      r32 Inv255 = 1.0f / 255.0f;
      __m128 Inv255_4x = _mm_set1_ps(Inv255);
      r32 One255 = 255.0f;
      __m128 One255_4x = _mm_set1_ps(One255);

      r32 NormalizeC = 1.0f / 255.0f;
      r32 NormalizeSqC = 1.0f / Square(255.0f);
  
      __m128 Zero = _mm_set1_ps(0.0f);
      __m128 One  = _mm_set1_ps(1.0f);
      __m128 Four_4x  = _mm_set1_ps(4.0f);
      __m128 MaskFF = _mm_set1_epi32(0xff);
      __m128 MaskFFFF = _mm_set1_epi32(0xffff);
      __m128 MaskFF00FF = _mm_set1_epi32(0x00ff00ff);
      __m128 Colorr_4x = _mm_set1_ps(Color.r);
      __m128 Colorg_4x = _mm_set1_ps(Color.g);
      __m128 Colorb_4x = _mm_set1_ps(Color.b);
      __m128 Colora_4x = _mm_set1_ps(Color.a);
      __m128 nXAxisx_4x = _mm_set1_ps(nXAxis.x);
      __m128 nXAxisy_4x = _mm_set1_ps(nXAxis.y);
      __m128 nYAxisx_4x = _mm_set1_ps(nYAxis.x);
      __m128 nYAxisy_4x = _mm_set1_ps(nYAxis.y);
      __m128 Originx_4x = _mm_set1_ps(Origin.x);
      __m128 Originy_4x = _mm_set1_ps(Origin.y);
      __m128 MaxColorValue = _mm_set1_ps(255.0f*255.0f);
      __m128 WidthM2  = _mm_set1_ps((r32)(Texture->Width - 2));
      __m128 HeightM2 = _mm_set1_ps((r32)(Texture->Height - 2));
      __m128i TexturePitch_4x = _mm_set1_epi32(Texture->Pitch);
 
      u32 BytesPerPixel = BITMAP_BYTES_PER_PIXEL;
      u8 *Row = ((u8*)Buffer->Memory
		 + Buffer->Pitch * FillRect.MinY
		 + BytesPerPixel * FillRect.MinX);
      u32 RowAdvance = 2*Buffer->Pitch;

      u32   TexturePitch  = Texture->Pitch;
      void* TextureMemory = Texture->Memory;
  
      s32 MinY = FillRect.MinY;
      s32 MaxY = FillRect.MaxY;
      s32 MinX = FillRect.MinX;
      s32 MaxX = FillRect.MaxX;
      BEGIN_TIMED_BLOCK(ProcessPixel);
      for(int Y = MinY;
	  Y < MaxY;
	  Y += 2)
	{
	  __m128 PixelPy = _mm_set1_ps((r32)(Y - Origin.y));
	  //__m128 PixelPy = _mm_set1_ps((r32)Y);
	  __m128 PynX = _mm_mul_ps(PixelPy, nXAxisy_4x);
	  __m128 PynY = _mm_mul_ps(PixelPy, nYAxisy_4x);

	  __m128 PixelPx = _mm_set_ps((r32)(MinX + 3),
				      (r32)(MinX + 2),
				      (r32)(MinX + 1),
				      (r32)(MinX + 0));
	  PixelPx = _mm_sub_ps(PixelPx, Originx_4x);

	  __m128 ClipMask = StartupClipMask;
      
	  u32* Pixel = (u32*) Row;
	  for(int XI = MinX;
	      XI < MaxX;
	      XI += 4)
	    {
#define mmSquare(V) _mm_mul_ps(V, V)	  
#define M(a, i) ((float *)&(a))[i]
#define Mi(a, i) ((u32 *)&(a))[i]

	      IACA_START;

	      __m128 U = _mm_add_ps(_mm_mul_ps(PixelPx, nXAxisx_4x), PynX);
	      __m128 V = _mm_add_ps(_mm_mul_ps(PixelPx, nYAxisx_4x), PynY);

	      __m128i WriteMask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(U, Zero),
									 _mm_cmple_ps(U, One)),
							      _mm_and_ps(_mm_cmpge_ps(V, Zero),
									 _mm_cmple_ps(V, One))));	  
	      WriteMask = _mm_and_si128(WriteMask, ClipMask);
	      //if(_mm_movemask_epi8(WriteMask))
	      {
		__m128i OriginalDest = _mm_loadu_si128((__m128i*) Pixel);
 
		U = _mm_min_ps(_mm_max_ps(U, Zero), One);
		V = _mm_min_ps(_mm_max_ps(V, Zero), One);
	  
		__m128 tX = _mm_mul_ps(U, WidthM2);
		__m128 tY = _mm_mul_ps(V, HeightM2);

		__m128i FetchX_4x = _mm_cvttps_epi32(tX);
		__m128i FetchY_4x = _mm_cvttps_epi32(tY);
	  
		__m128 fX = _mm_sub_ps(tX, _mm_cvtepi32_ps(FetchX_4x));
		__m128 fY = _mm_sub_ps(tY, _mm_cvtepi32_ps(FetchY_4x));

		FetchX_4x = _mm_slli_epi32(FetchX_4x, 2);
		FetchY_4x = _mm_or_si128(_mm_mullo_epi16(FetchY_4x, TexturePitch_4x),
					 _mm_slli_epi32(_mm_mulhi_epi16(FetchY_4x, TexturePitch_4x), 16));
		__m128i Fetch_4x = _mm_add_epi32(FetchX_4x, FetchY_4x);

		s32 Fetch0 = Mi(Fetch_4x, 0);
		s32 Fetch1 = Mi(Fetch_4x, 1);
		s32 Fetch2 = Mi(Fetch_4x, 2);
		s32 Fetch3 = Mi(Fetch_4x, 3);
 
		u8* TexelPtr0 = (((u8*)TextureMemory)
				 + Fetch0);

		u8* TexelPtr1 = (((u8*)TextureMemory)
				 + Fetch1);

		u8* TexelPtr2 = (((u8*)TextureMemory)
				 + Fetch2);

		u8* TexelPtr3 = (((u8*)TextureMemory)
				 + Fetch3);

		__m128i SampleA = _mm_setr_epi32(*(u32*)(TexelPtr0),
						 *(u32*)(TexelPtr1),
						 *(u32*)(TexelPtr2),
						 *(u32*)(TexelPtr3));
	      
		__m128i SampleB = _mm_setr_epi32(*(u32*)(TexelPtr0 + BITMAP_BYTES_PER_PIXEL),
						 *(u32*)(TexelPtr1 + BITMAP_BYTES_PER_PIXEL),
						 *(u32*)(TexelPtr2 + BITMAP_BYTES_PER_PIXEL),
						 *(u32*)(TexelPtr3 + BITMAP_BYTES_PER_PIXEL));
	      
		__m128i SampleC = _mm_setr_epi32(*(u32*)(TexelPtr0 + TexturePitch),
						 *(u32*)(TexelPtr1 + TexturePitch),
						 *(u32*)(TexelPtr2 + TexturePitch),
						 *(u32*)(TexelPtr3 + TexturePitch));
	      
		__m128i SampleD = _mm_setr_epi32(*(u32*)(TexelPtr0 + TexturePitch + BITMAP_BYTES_PER_PIXEL),
						 *(u32*)(TexelPtr1 + TexturePitch + BITMAP_BYTES_PER_PIXEL),
						 *(u32*)(TexelPtr2 + TexturePitch + BITMAP_BYTES_PER_PIXEL),
						 *(u32*)(TexelPtr3 + TexturePitch + BITMAP_BYTES_PER_PIXEL));

		__m128i TexelaAG = _mm_and_si128(_mm_srli_epi32(SampleA, 8),  MaskFF00FF);
		__m128i TexelaRB = _mm_and_si128(SampleA, MaskFF00FF);
		TexelaRB = _mm_mullo_epi16(TexelaRB, TexelaRB);
		__m128 TexelaA = _mm_cvtepi32_ps(_mm_srli_epi32(TexelaAG, 16));
		TexelaAG = _mm_mullo_epi16(TexelaAG, TexelaAG);

		__m128i TexelbAG = _mm_and_si128(_mm_srli_epi32(SampleB, 8),  MaskFF00FF);
		__m128i TexelbRB = _mm_and_si128(SampleB, MaskFF00FF);
		TexelbRB = _mm_mullo_epi16(TexelbRB, TexelbRB);
		__m128 TexelbA = _mm_cvtepi32_ps(_mm_srli_epi32(TexelbAG, 16));
		TexelbAG = _mm_mullo_epi16(TexelbAG, TexelbAG);

		__m128i TexelcAG = _mm_and_si128(_mm_srli_epi32(SampleC, 8),  MaskFF00FF);
		__m128i TexelcRB = _mm_and_si128(SampleC, MaskFF00FF);
		TexelcRB = _mm_mullo_epi16(TexelcRB, TexelcRB);
		__m128 TexelcA = _mm_cvtepi32_ps(_mm_srli_epi32(TexelcAG, 16));
		TexelcAG = _mm_mullo_epi16(TexelcAG, TexelcAG);

		__m128i TexeldAG = _mm_and_si128(_mm_srli_epi32(SampleD, 8),  MaskFF00FF);
		__m128i TexeldRB = _mm_and_si128(SampleD, MaskFF00FF);
		TexeldRB = _mm_mullo_epi16(TexeldRB, TexeldRB);
		__m128 TexeldA = _mm_cvtepi32_ps(_mm_srli_epi32(TexeldAG, 16));
		TexeldAG = _mm_mullo_epi16(TexeldAG, TexeldAG);

		__m128 DestA = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 24), MaskFF));
		__m128 DestR = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 16), MaskFF));
		__m128 DestG = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 8),  MaskFF));
		__m128 DestB = _mm_cvtepi32_ps(_mm_and_si128(OriginalDest, MaskFF));
	  	  
		__m128 TexelaR = _mm_cvtepi32_ps(_mm_srli_epi32(TexelaRB, 16));
		__m128 TexelaG = _mm_cvtepi32_ps(_mm_and_si128(TexelaAG, MaskFFFF));
		__m128 TexelaB = _mm_cvtepi32_ps(_mm_and_si128(TexelaRB, MaskFFFF));
	  	  
		__m128 TexelbR = _mm_cvtepi32_ps(_mm_srli_epi32(TexelbRB, 16));
		__m128 TexelbG = _mm_cvtepi32_ps(_mm_and_si128(TexelbAG, MaskFFFF));
		__m128 TexelbB = _mm_cvtepi32_ps(_mm_and_si128(TexelbRB, MaskFFFF));
	  	  
		__m128 TexelcR = _mm_cvtepi32_ps(_mm_srli_epi32(TexelcRB, 16));
		__m128 TexelcG = _mm_cvtepi32_ps(_mm_and_si128(TexelcAG, MaskFFFF));
		__m128 TexelcB = _mm_cvtepi32_ps(_mm_and_si128(TexelcRB, MaskFFFF));
	  	  
		__m128 TexeldR = _mm_cvtepi32_ps(_mm_srli_epi32(TexeldRB, 16));
		__m128 TexeldG = _mm_cvtepi32_ps(_mm_and_si128(TexeldAG, MaskFFFF));
		__m128 TexeldB = _mm_cvtepi32_ps(_mm_and_si128(TexeldRB, MaskFFFF));
			      
		__m128 ifX = _mm_sub_ps(One, fX);
		__m128 ifY = _mm_sub_ps(One, fY);

		__m128 l0 = _mm_mul_ps(ifY, ifX);
		__m128 l1 = _mm_mul_ps(ifY, fX);
		__m128 l2 = _mm_mul_ps(fY, ifX);
		__m128 l3 = _mm_mul_ps(fY, fX);
	  
		__m128 TexelR = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelaR), _mm_mul_ps(l1, TexelbR)),
					   _mm_add_ps(_mm_mul_ps(l2, TexelcR), _mm_mul_ps(l3, TexeldR)));
		__m128 TexelG = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelaG), _mm_mul_ps(l1, TexelbG)),
					   _mm_add_ps(_mm_mul_ps(l2, TexelcG), _mm_mul_ps(l3, TexeldG)));
		__m128 TexelB = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelaB), _mm_mul_ps(l1, TexelbB)),
					   _mm_add_ps(_mm_mul_ps(l2, TexelcB), _mm_mul_ps(l3, TexeldB)));
		__m128 TexelA = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelaA), _mm_mul_ps(l1, TexelbA)),
					   _mm_add_ps(_mm_mul_ps(l2, TexelcA), _mm_mul_ps(l3, TexeldA)));
	  
		TexelR = _mm_mul_ps(TexelR, Colorr_4x);
		TexelG = _mm_mul_ps(TexelG, Colorg_4x);
		TexelB = _mm_mul_ps(TexelB, Colorb_4x);
		TexelA = _mm_mul_ps(TexelA, Colora_4x);

		TexelR = _mm_min_ps(_mm_max_ps(TexelR, Zero), MaxColorValue);
		TexelG = _mm_min_ps(_mm_max_ps(TexelG, Zero), MaxColorValue);
		TexelB = _mm_min_ps(_mm_max_ps(TexelB, Zero), MaxColorValue);
	  
		DestR = mmSquare(DestR);
		DestG = mmSquare(DestG);
		DestB = mmSquare(DestB);
	      	  
		__m128 InvTexelA = _mm_sub_ps(One, _mm_mul_ps(Inv255_4x, TexelA));
		__m128 BlendedR = _mm_add_ps(_mm_mul_ps(InvTexelA, DestR), TexelR);
		__m128 BlendedG = _mm_add_ps(_mm_mul_ps(InvTexelA, DestG), TexelG);
		__m128 BlendedB = _mm_add_ps(_mm_mul_ps(InvTexelA, DestB), TexelB);
		__m128 BlendedA = _mm_add_ps(_mm_mul_ps(InvTexelA, DestA), TexelA);

#if 0
		BlendedR = _mm_sqrt_ps(BlendedR);
		BlendedG = _mm_sqrt_ps(BlendedG);
		BlendedB = _mm_sqrt_ps(BlendedB);
#else	      
		BlendedR = _mm_mul_ps(BlendedR, _mm_rsqrt_ps(BlendedR));
		BlendedG = _mm_mul_ps(BlendedG, _mm_rsqrt_ps(BlendedG));
		BlendedB = _mm_mul_ps(BlendedB, _mm_rsqrt_ps(BlendedB));
#endif
		__m128i IntR = _mm_cvtps_epi32(BlendedR);
		__m128i IntG = _mm_cvtps_epi32(BlendedG);
		__m128i IntB = _mm_cvtps_epi32(BlendedB);
		__m128i IntA = _mm_cvtps_epi32(BlendedA);

		__m128i sR = _mm_slli_epi32(IntR, 16);
		__m128i sG = _mm_slli_epi32(IntG,  8);
		__m128i sB = IntB;
		__m128i sA = _mm_slli_epi32(IntA, 24);
	  
		__m128i Out = _mm_or_si128(_mm_or_si128(sR, sG), _mm_or_si128(sB, sA));

		__m128i MaskedOut = _mm_or_si128(_mm_and_si128(WriteMask, Out),
						 _mm_andnot_si128(WriteMask, OriginalDest));
	  
		_mm_storeu_si128((__m128i*)Pixel, MaskedOut);
	      }

	      PixelPx  = _mm_add_ps(PixelPx, Four_4x);
	      Pixel += 4;	  
	      ClipMask = _mm_set1_epi32(0xFFFFFFFF);

	      IACA_END;
	    }
	  Row += RowAdvance; 
	}

      s32 result = GetClampedRectArea(FillRect) / 2;
      //printf("%d\t", result);
      u64 b = DebugGlobalMemory->Counters[DebugCycleCounter_ProcessPixel].CycleCount;
      END_TIMED_BLOCK_COUNTED(ProcessPixel, result);
      //printf("%lu\n", DebugGlobalMemory->Counters[DebugCycleCounter_ProcessPixel].CycleCount - b);
    }

  END_TIMED_BLOCK(DrawRectangleQuickly);
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
RenderGroupToOutput(render_group* RenderGroup, loaded_bitmap* OutputTarget, rectangle2i ClipRect, bool32 Even)
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
	    entity_basis_p Basis = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenDim);
	    Assert(Entry->Bitmap);
#if 0
	    DrawRectangleSlowly(OutputTarget,
				Basis.P,
				V2MulS(Basis.Scale, V2(Entry->Size.x, 0.0f)),
				V2MulS(Basis.Scale, V2(0.0f, Entry->Size.y)),
				Entry->Color,
				Entry->Bitmap,
				0, 0, 0, 0,
				PixelsToMeters);
#else
	    DrawRectangleQuickly(OutputTarget,
				 Basis.P,
				 V2MulS(Basis.Scale, V2(Entry->Size.x, 0.0f)),
				 V2MulS(Basis.Scale, V2(0.0f, Entry->Size.y)),
				 Entry->Color,
				 Entry->Bitmap,
				 PixelsToMeters,
				 ClipRect,
				 Even);
	    DrawRectangleQuickly(OutputTarget,
				 Basis.P,
				 V2MulS(Basis.Scale, V2(Entry->Size.x, 0.0f)),
				 V2MulS(Basis.Scale, V2(0.0f, Entry->Size.y)),
				 Entry->Color,
				 Entry->Bitmap,
				 PixelsToMeters,
				 ClipRect,
				 Even);
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
	  
	  InvalidDefaultCase;
	}
    }

  END_TIMED_BLOCK(RenderGroupToOutput);
}


internal void
TiledRenderGroupToOutput(render_group* RenderGroup, loaded_bitmap* OutputTarget)
{
  //rectangle2i ClipRect = {0, 0, OutputTarget->Width, OutputTarget->Height};
  rectangle2i ClipRect = {4, 4, OutputTarget->Width - 4, OutputTarget->Height - 4};

  RenderGroupToOutput(RenderGroup, OutputTarget, ClipRect, false);
  RenderGroupToOutput(RenderGroup, OutputTarget, ClipRect, true);
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

