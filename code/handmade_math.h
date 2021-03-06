
typedef union
{
  struct
  {
    r32 x;
    r32 y;
  };
  struct
  {
    r32 u;
    r32 v;
  };
  r32 E[2];
} v2;

typedef union
{
  struct
  {
    r32 x;
    r32 y;
    r32 z;
  };
  struct
  {
    r32 r;
    r32 g;
    r32 b;
  };
  struct
  {
    r32 u;
    r32 v;
    r32 w;
  };
  struct
  {
    v2 xy;
    r32 Ignored0_;
  };
  struct
  {
    r32 Ignored1_;
    v2 yz;
  };
  struct
  {
    v2 uv;
    r32 Ignored2_;
  };
  struct
  {
    r32 Ignored3_;
    v2 vw;
  };
  r32 E[3];
} v3;

typedef union
{
  struct
  {
    union
    {
      v3 xyz;
      struct
      {
	r32 x;
	r32 y;
	r32 z;
      };
    };
    r32 w;
  };
  struct
  {
    union
    {
      v3 rgb;
      struct
      {
	r32 r;
	r32 g;
	r32 b;
      };
    };
    r32 a;
  };
  struct
  {
    v2 xy;
    r32 Ignored0_;
    r32 Ignored1_;
  };
  struct
  {
    r32 Ignored2_;
    v2 yz;
    r32 Ignored3_;
  };
  struct
  {
    r32 Ignored4_;
    r32 Ignored5_;
    v2 zy;
  };
  r32 E[4];
} v4;

typedef struct rectangle2 rectangle2;
introspect(category:"math") struct rectangle2 
{
  v2 Min;
  v2 Max;
};

typedef struct
{
  s32 MinX, MinY;
  s32 MaxX, MaxY;
} rectangle2i;

typedef struct rectangle3 rectangle3;
introspect(category:"math") struct rectangle3
{
  v3 Min;
  v3 Max;
};

internal inline v2
V2i(u32 X, u32 Y)
{
  v2 Result = {(r32)X, (r32)Y};
  return(Result);
}

internal inline v2
V2u(s32 X, s32 Y)
{
  v2 Result = {(r32)X, (r32)Y};
  return(Result);
}

internal inline v2
V2(r32 X, r32 Y)
{
  v2 Result;

  Result.x = X;
  Result.y = Y;

  return(Result);  
}

internal inline v3
V3(r32 X, r32 Y, r32 Z)
{
  v3 Result;

  Result.x = X;
  Result.y = Y;
  Result.z = Z;
  
  return(Result);  
}

internal inline v3
ToV3(v2 XY, r32 Z)
{
  v3 Result;

  Result.x = XY.x;
  Result.y = XY.y;
  Result.z = Z;
  
  return(Result);  
}

internal inline v4
V4(r32 X, r32 Y, r32 Z, r32 W)
{
  v4 Result;

  Result.x = X;
  Result.y = Y;
  Result.z = Z;
  Result.w = W;

  return(Result);  
}

internal inline v4
ToV4(v3 XYZ, r32 W)
{
  v4 Result;

  Result.x = XYZ.x;
  Result.y = XYZ.y;
  Result.z = XYZ.z;
  Result.w = W;
  
  return(Result);  
}

internal inline r32
Square(r32 V)
{
  r32 Result = V*V;
  return(Result);
}

internal inline r32
Lerp(r32 A, r32 t, r32 B)
{
  r32 Result = (1.0f - t)*A + t*B;
  return(Result);
}

internal inline r32
Clamp(r32 Min, r32 Value, r32 Max)
{
  r32 Result = Value;

  if(Result < Min)
    {
      Result = Min;
    }
  else if(Result > Max)
    {
      Result = Max;
    }
  
  return(Result);
}
#define Clamp01(Value) Clamp(0.0f, Value, 1.0f)

internal inline r32
Clamp01MapToRange(r32 Min, r32 t, r32 Max)
{
  r32 Result = 0.0f;

  r32 Range = Max - Min;
  if(Range != 0.0f )
    {
      Result = Clamp01((t - Min) / Range);
    }

  return(Result);
}

///
///
///

internal inline v2
V2MulS(r32 S, v2 V)
{
  v2 Result;

  Result.x = S * V.x;
  Result.y = S * V.y;

  return(Result);  
}

internal inline v2
V2Neg(v2 V)
{
  v2 Result;

  Result.x = -V.x;
  Result.y = -V.y;

  return(Result);
}

internal inline v2
V2Add(v2 A, v2 B)
{
  v2 Result;

  Result.x = A.x + B.x;
  Result.y = A.y + B.y;
  
  return(Result);
}

internal inline v2
V2Sub(v2 A, v2 B)
{
  v2 Result;

  Result.x = A.x - B.x;
  Result.y = A.y - B.y;
  
  return(Result);
}

internal inline v2
V2Hadamard(v2 A, v2 B)
{
  v2 Result = V2(A.x * B.x,  A.y * B.y);
  return(Result);
}

internal inline r32
V2Inner(v2 A, v2 B)
{
  r32 Result = A.x * B.x + A.y * B.y;
  return(Result);
}

internal inline r32
V2LengthSq(v2 V)
{
  r32 Result = V2Inner(V, V);
  return(Result);
}

internal inline r32
V2Length(v2 V)
{
  r32 Result = SquareRoot(V2LengthSq(V));
  return(Result);
}

internal inline v2
V2Clamp01(v2 V)
{
  v2 Result;

  Result.x = Clamp01(V.x);
  Result.y = Clamp01(V.y);
    
  return(Result);
}

internal inline v2
Arm2(r32 Angle)
{
  v2 Result = {Cos(Angle), Sin(Angle)};
  return(Result);
}
///
///
///


internal inline v3
V3MulS(r32 S, v3 V)
{
  v3 Result;

  Result.x = S * V.x;
  Result.y = S * V.y;
  Result.z = S * V.z;

  return(Result);  
}

internal inline v3
V3Neg(v3 V)
{
  v3 Result;

  Result.x = -V.x;
  Result.y = -V.y;
  Result.z = -V.z;

  return(Result);
}

internal inline v3
V3Add(v3 A, v3 B)
{
  v3 Result;

  Result.x = A.x + B.x;
  Result.y = A.y + B.y;
  Result.z = A.z + B.z;
  
  return(Result);
}

internal inline v3
V3Sub(v3 A, v3 B)
{
  v3 Result;

  Result.x = A.x - B.x;
  Result.y = A.y - B.y;
  Result.z = A.z - B.z;
  
  return(Result);
}

internal inline v3
V3Hadamard(v3 A, v3 B)
{
  v3 Result = V3(A.x * B.x,  A.y * B.y, A.z * B.z);
  return(Result);
}

internal inline r32
V3Inner(v3 A, v3 B)
{
  r32 Result = A.x * B.x + A.y * B.y + A.z * B.z;
  return(Result);
}

internal inline r32
V3LengthSq(v3 V)
{
  r32 Result = V3Inner(V, V);
  return(Result);
}

internal inline r32
V3Length(v3 V)
{
  r32 Result = SquareRoot(V3LengthSq(V));
  return(Result);
}

internal inline v3
V3Clamp01(v3 V)
{
  v3 Result;

  Result.x = Clamp01(V.x);
  Result.y = Clamp01(V.y);
  Result.z = Clamp01(V.z);
  
  return(Result);
}

internal inline v3
V3Normalize(v3 V)
{
  v3 Result = V3MulS(1.0f/V3Length(V), V);
  return(Result);
}

internal inline v3
V3Lerp(v3 A, r32 t, v3 B)
{
  v3 Result = V3Add(V3MulS((1.0f - t), A),
		    V3MulS(t, B));
  return(Result);
}

///
///
///


internal inline v4
V4MulS(r32 S, v4 V)
{
  v4 Result;

  Result.x = S * V.x;
  Result.y = S * V.y;
  Result.z = S * V.z;
  Result.w = S * V.w;

  return(Result);  
}

internal inline v4
V4Neg(v4 V)
{
  v4 Result;

  Result.x = -V.x;
  Result.y = -V.y;
  Result.z = -V.z;
  Result.w = -V.w;

  return(Result);
}

internal inline v4
V4Add(v4 A, v4 B)
{
  v4 Result;

  Result.x = A.x + B.x;
  Result.y = A.y + B.y;
  Result.z = A.z + B.z;
  Result.w = A.w + B.w;
  
  return(Result);
}

internal inline v4
V4Sub(v4 A, v4 B)
{
  v4 Result;

  Result.x = A.x - B.x;
  Result.y = A.y - B.y;
  Result.z = A.z - B.z;
  Result.w = A.w - B.w;
  
  return(Result);
}

internal inline v4
V4Hadamard(v4 A, v4 B)
{
  v4 Result = V4(A.x * B.x,  A.y * B.y, A.z * B.z, A.w * B.w);
  return(Result);
}

internal inline r32
V4Inner(v4 A, v4 B)
{
  r32 Result = A.x * B.x + A.y * B.y + A.z * B.z + A.w * B.w;
  return(Result);
}

internal inline r32
V4LengthSq(v4 V)
{
  r32 Result = V4Inner(V, V);
  return(Result);
}

internal inline r32
V4Length(v4 V)
{
  r32 Result = SquareRoot(V4LengthSq(V));
  return(Result);
}

internal inline v4
V4Clamp01(v4 V)
{
  v4 Result;

  Result.x = Clamp01(V.x);
  Result.y = Clamp01(V.y);
  Result.z = Clamp01(V.z);
  Result.z = Clamp01(V.w);
  
  return(Result);
}

internal inline v4
V4Lerp(v4 A, r32 t, v4 B)
{
  v4 Result = V4Add(V4MulS((1.0f - t), A),
		    V4MulS(t, B));
  return(Result);
}

///
///
///

internal inline rectangle2
InvertedInfinityRectangle2()
{
  rectangle2 Result;

  Result.Min.x = Real32Maximum;
  Result.Min.y = Real32Maximum;
  Result.Max.x = -Real32Maximum;
  Result.Max.y = -Real32Maximum;

  return(Result);
}

internal inline rectangle2
Union2(rectangle2 A, rectangle2 B)
{
  rectangle2 Result;

  Result.Min.x = (A.Min.x < B.Min.x) ? A.Min.x : B.Min.x;
  Result.Min.y = (A.Min.y < B.Min.y) ? A.Min.y : B.Min.y;
  Result.Max.x = (A.Max.x > B.Max.x) ? A.Max.x : B.Max.x;
  Result.Max.y = (A.Max.y > B.Max.y) ? A.Max.y : B.Max.y;
    
  return(Result);
}

internal inline v2
GetMinCorner2(rectangle2 Rect)
{
  v2 Result = Rect.Min;
  return(Result);  
}

internal inline v2
GetMaxCorner2(rectangle2 Rect)
{
  v2 Result = Rect.Max;
  return(Result);
}

internal inline v2
GetDim2(rectangle2 Rect)
{
  v2 Result = V2Sub(Rect.Max, Rect.Min);
  return(Result);
}

internal inline v2
GetCenter2(rectangle2 Rect)
{
  v2 Result = V2MulS(0.5f, V2Add(Rect.Min, Rect.Max));
  return(Result);  
}

internal inline rectangle2
RectMinMax2(v2 Min, v2 Max)
{
  rectangle2 Result;

  Result.Min = Min;
  Result.Max = Max;

  return(Result);
}

internal inline rectangle2
RectMinDim2(v2 Min, v2 Dim)
{
  rectangle2 Result;

  Result.Min = Min;
  Result.Max = V2Add(Min, Dim);

  return(Result);
}

internal inline rectangle2
RectCenterHalfDim2(v2 Center, v2 HalfDim)
{
  rectangle2 Result;

  Result.Min = V2Sub(Center, HalfDim);
  Result.Max = V2Add(Center, HalfDim);

  return(Result);
}

internal inline rectangle2
AddRadiusTo2(rectangle2 A, v2 Radius)
{
  rectangle2 Result;

  Result.Min = V2Sub(A.Min, Radius);
  Result.Max = V2Add(A.Max, Radius);

  return(Result);  
}

internal inline rectangle2
Offset2(rectangle2 R, v2 Offset)
{
  rectangle2 Result;

  Result.Min = V2Add(R.Min, Offset);
  Result.Max = V2Add(R.Max, Offset);

  return(Result);  
}

internal inline rectangle2
RectCenterDim2(v2 Center, v2 Dim)
{
  rectangle2 Result = RectCenterHalfDim2(Center,
					 V2MulS(0.5f, Dim));
  return(Result);
}

internal inline bool32
IsInRectangle2(rectangle2 Rectangle, v2 Test)
{
  bool32 Result = ((Test.x >= Rectangle.Min.x) &&
		   (Test.y >= Rectangle.Min.y) &&
		   (Test.x  < Rectangle.Max.x) &&
		   (Test.y  < Rectangle.Max.y));
  return(Result);
}

///
///
///


internal inline v3
GetMinCorner(rectangle3 Rect)
{
  v3 Result = Rect.Min;
  return(Result);  
}

internal inline v3
GetMaxCorner(rectangle3 Rect)
{
  v3 Result = Rect.Max;
  return(Result);
}

internal inline v3
GetCenter(rectangle3 Rect)
{
  v3 Result = V3MulS(0.5f, V3Add(Rect.Min, Rect.Max));
  return(Result);  
}

internal inline rectangle3
RectMinMax(v3 Min, v3 Max)
{
  rectangle3 Result;

  Result.Min = Min;
  Result.Max = Max;

  return(Result);
}

internal inline rectangle3
RectMinDim(v3 Min, v3 Dim)
{
  rectangle3 Result;

  Result.Min = Min;
  Result.Max = V3Add(Min, Dim);

  return(Result);
}

internal inline v3
GetDim(rectangle3 Rect)
{
  v3 Result = V3Sub(Rect.Max, Rect.Min);
  return(Result);
}

internal inline rectangle3
RectCenterHalfDim(v3 Center, v3 HalfDim)
{
  rectangle3 Result;

  Result.Min = V3Sub(Center, HalfDim);
  Result.Max = V3Add(Center, HalfDim);

  return(Result);
}

internal inline rectangle3
AddRadiusTo(rectangle3 A, v3 Radius)
{
  rectangle3 Result;

  Result.Min = V3Sub(A.Min, Radius);
  Result.Max = V3Add(A.Max, Radius);

  return(Result);  
}

internal inline rectangle3
Offset(rectangle3 R, v3 Offset)
{
  rectangle3 Result;

  Result.Min = V3Add(R.Min, Offset);
  Result.Max = V3Add(R.Max, Offset);

  return(Result);  
}

internal inline rectangle3
RectCenterDim(v3 Center, v3 Dim)
{
  rectangle3 Result = RectCenterHalfDim(Center,
					V3MulS(0.5f, Dim));
  return(Result);
}

internal inline bool32
IsInRectangle(rectangle3 Rectangle, v3 Test)
{
  bool32 Result = ((Test.x >= Rectangle.Min.x) &&
		   (Test.y >= Rectangle.Min.y) &&
		   (Test.z >= Rectangle.Min.z) &&
		   (Test.x  < Rectangle.Max.x) &&
		   (Test.y  < Rectangle.Max.y) &&
		   (Test.z  < Rectangle.Max.z));
  return(Result);
}


internal inline bool32
RectanglesIntersect(rectangle3 A, rectangle3 B)
{
  bool32 Result = !((B.Max.x <= A.Min.x) ||
		    (B.Min.x >= A.Max.x) ||
		    (B.Max.y <= A.Min.y) ||
		    (B.Min.y >= A.Max.y) ||
		    (B.Max.z <= A.Min.z) ||
		    (B.Min.z >= A.Max.z));
  return(Result);
}

internal inline r32
SafeRatioN(r32 Numerator, r32 Divisor, r32 N)
{
  r32 Result = N;

  if(Divisor != 0.0f)
    {
      Result = Numerator / Divisor;
    }

  return(Result);
}
#define SafeRatio0(Numerator, Divisor) SafeRatioN(Numerator, Divisor, 0.0f)
#define SafeRatio1(Numerator, Divisor) SafeRatioN(Numerator, Divisor, 1.0f)

internal inline v2
V2GetBarycentric(rectangle2 R, v2 P)
{
  v2 Result;

  Result.x = SafeRatio0((P.x - R.Min.x), (R.Max.x - R.Min.x));
  Result.y = SafeRatio0((P.y - R.Min.y), (R.Max.y - R.Min.y));
  
  return(Result);
}

internal inline v2
V2Perp(v2 V)
{
  v2 Result = V2(-V.y, V.x);
  return(Result);
}

internal inline v3
V3GetBarycentric(rectangle3 R, v3 P)
{
  v3 Result;

  Result.x = SafeRatio0((P.x - R.Min.x), (R.Max.x - R.Min.x));
  Result.y = SafeRatio0((P.y - R.Min.y), (R.Max.y - R.Min.y));
  Result.z = SafeRatio0((P.z - R.Min.z), (R.Max.z - R.Min.z));

  return(Result);
}

internal inline rectangle2
ToRectangleXY(rectangle3 R)
{
  rectangle2 Result;

  Result.Min = R.Min.xy;
  Result.Max = R.Max.xy;
  
  return(Result);
}

internal inline rectangle2i
Intersect(rectangle2i A, rectangle2i B)
{
  rectangle2i Result;

  Result.MinX = (A.MinX < B.MinX) ? B.MinX : A.MinX;
  Result.MinY = (A.MinY < B.MinY) ? B.MinY : A.MinY;
  Result.MaxX = (A.MaxX > B.MaxX) ? B.MaxX : A.MaxX;
  Result.MaxY = (A.MaxY > B.MaxY) ? B.MaxY : A.MaxY;
    
  return(Result);
}

internal inline rectangle2i
Union2i(rectangle2i A, rectangle2i B)
{
  rectangle2i Result;

  Result.MinX = (A.MinX < B.MinX) ? A.MinX : B.MinX;
  Result.MinY = (A.MinY < B.MinY) ? A.MinY : B.MinY;
  Result.MaxX = (A.MaxX > B.MaxX) ? A.MaxX : B.MaxX;
  Result.MaxY = (A.MaxY > B.MaxY) ? A.MaxY : B.MaxY;
    
  return(Result);
}

internal inline s32
GetClampedRectArea(rectangle2i A)
{
  s32 Width  = (A.MaxX - A.MinX);
  s32 Height = (A.MaxY - A.MinY);

  s32 Result = 0;
  if((Width > 0) && (Height > 0))
    {
      Result = Width * Height;
    }
  
  return(Result);
}  

internal inline bool32
HasArea(rectangle2i A)
{
  bool32 Result = ((A.MinX < A.MaxX) && (A.MinY < A.MaxY));
  return(Result);
}

internal inline rectangle2i
InvertedInfinityRectangle2i()
{
  rectangle2i Result;

  Result.MinX = INT_MAX;
  Result.MinY = INT_MAX;
  Result.MaxX = -INT_MAX;
  Result.MaxY = -INT_MAX;

  return(Result);
}

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

//
//
//

internal inline void
DEBUGValueSetEventData_v2(debug_event* Event, v2 Value)
{
  Event->Type = DebugEvent_V2;
  Event->VecR32[0] = Value.x;
  Event->VecR32[1] = Value.y;
}

internal inline void
DEBUGValueSetEventData_v3(debug_event* Event, v3 Value)
{
  Event->Type = DebugEvent_V3;
  Event->VecR32[0] = Value.x;
  Event->VecR32[1] = Value.y;
  Event->VecR32[2] = Value.z;
}

internal inline void
DEBUGValueSetEventData_v4(debug_event* Event, v4 Value)
{
  Event->Type = DebugEvent_V4;
  Event->VecR32[0] = Value.x;
  Event->VecR32[1] = Value.y;
  Event->VecR32[2] = Value.z;
  Event->VecR32[3] = Value.w;
}

internal inline void
DEBUGValueSetEventData_rectangle2(debug_event* Event, rectangle2 Value)
{
  Event->Type = DebugEvent_Rectangle2;
  Event->VecR32[0] = Value.Min.x;
  Event->VecR32[1] = Value.Min.y;
  Event->VecR32[2] = Value.Max.x;
  Event->VecR32[3] = Value.Max.y;
}

internal inline void
DEBUGValueSetEventData_rectangle3(debug_event* Event, rectangle3 Value)
{
  Event->Type = DebugEvent_Rectangle3;
  Event->VecR32[0] = Value.Min.x;
  Event->VecR32[1] = Value.Min.y;
  Event->VecR32[2] = Value.Min.z;
  Event->VecR32[3] = Value.Max.x;
  Event->VecR32[4] = Value.Max.y;
  Event->VecR32[5] = Value.Max.z;
}


