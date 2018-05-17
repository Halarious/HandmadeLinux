#define IGNORED_BEGIN_TIMED_FUNCTION(...) BEGIN_TIMED_FUNCTION(1)
#define IGNORED_END_TIMED_FUNCTION(...)   END_TIMED_FUNCTION()
#define IGNORED_BEGIN_NAMED_BLOCK(Name, Number)    BEGIN_NAMED_BLOCK(Name, Number)
#define IGNORED_END_NAMED_BLOCK(Name)      END_NAMED_BLOCK(Name)

#include "handmade.h"

#if 0
#include "iacaMarks.h"
#else
#define IACA_START
#define IACA_END
#endif

void
DrawRectangleQuickly(loaded_bitmap *Buffer,
		     v2 Origin, v2 XAxis, v2 YAxis,
		     v4 Color, loaded_bitmap* Texture,
		     r32 PixelsToMeters,
		     rectangle2i ClipRect,
		     bool32 Even)
{
  IGNORED_BEGIN_TIMED_FUNCTION(1);

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

  rectangle2i FillRect = InvertedInfinityRectangle2i();
  
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
      __m128i StartClipMask = _mm_set1_epi8(-1);
      __m128i EndClipMask = _mm_set1_epi8(-1);

      __m128i StartClipMasks[] =
	{
	  _mm_slli_si128(StartClipMask, 0*4),
	  _mm_slli_si128(StartClipMask, 1*4),
	  _mm_slli_si128(StartClipMask, 2*4),
	  _mm_slli_si128(StartClipMask, 3*4),
	};

      __m128i EndClipMasks[] =
	{
	  _mm_srli_si128(EndClipMask, 0*4),
	  _mm_srli_si128(EndClipMask, 3*4),
	  _mm_srli_si128(EndClipMask, 2*4),
	  _mm_srli_si128(EndClipMask, 1*4),
	};
	
      if(FillRect.MinX & 3)
	{
	  StartClipMask = StartClipMasks[FillRect.MinX & 3];
	  FillRect.MinX = FillRect.MinX & ~3;
	}

      if(FillRect.MaxX & 3)
	{
	  EndClipMask = EndClipMasks[FillRect.MaxX & 3];
	  FillRect.MaxX = (FillRect.MaxX & ~3) + 4;
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
      __m128 Half = _mm_set1_ps(0.5f);
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
      IGNORED_BEGIN_NAMED_BLOCK(PixelFill, GetClampedRectArea(FillRect) / 2);
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

	  __m128 ClipMask = StartClipMask;
      
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
		__m128i OriginalDest = _mm_load_si128((__m128i*) Pixel);
 
		U = _mm_min_ps(_mm_max_ps(U, Zero), One);
		V = _mm_min_ps(_mm_max_ps(V, Zero), One);
	  
		__m128 tX = _mm_add_ps(_mm_mul_ps(U, WidthM2), Half);
		__m128 tY = _mm_add_ps(_mm_mul_ps(V, HeightM2), Half);

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
	  
		_mm_store_si128((__m128i*)Pixel, MaskedOut);
	      }

	      PixelPx  = _mm_add_ps(PixelPx, Four_4x);
	      Pixel += 4;

	      if((XI + 8) < MaxX)
		{
		  ClipMask = _mm_set1_epi32(0xFFFFFFFF);
		}
	      else
		{
		  ClipMask = EndClipMask;
		}
	      
	      IACA_END;
	    }
	  Row += RowAdvance; 
	}

      IGNORED_END_NAMED_BLOCK(PixelFill);
    }

  IGNORED_END_TIMED_FUNCTION();
}

u32 DebugRecords_Optimized_Count = __COUNTER__;

