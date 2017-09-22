
typedef union
{
  struct
  {
    r32 X;
    r32 Y;
  };
  r32 E[2];
} v2;

typedef union
{
  struct
  {
    r32 X;
    r32 Y;
    r32 Z;
  };
  struct
  {
    r32 R;
    r32 G;
    r32 B;
  };
  struct
  {
    v2 XY;
    r32 Ignored0_;
  };
  struct
  {
    r32 Ignored1_;
    v2 YZ;
  };
  r32 E[3];
} v3;

typedef union
{
  struct
  {
    r32 X;
    r32 Y;
    r32 Z;
    r32 W;
  };
  struct
  {
    r32 R;
    r32 G;
    r32 B;
    r32 A;
  };
  r32 E[4];
} v4;

typedef struct
{
  v2 Min;
  v2 Max;
} rectangle2;

typedef struct
{
  v3 Min;
  v3 Max;
} rectangle3;

internal inline v2
V2(r32 X, r32 Y)
{
  v2 Result;

  Result.X = X;
  Result.Y = Y;

  return(Result);  
}

internal inline v3
V3(r32 X, r32 Y, r32 Z)
{
  v3 Result;

  Result.X = X;
  Result.Y = Y;
  Result.Z = Z;
  
  return(Result);  
}

internal inline v3
ToV3(v2 XY, r32 Z)
{
  v3 Result;

  Result.X = XY.X;
  Result.Y = XY.Y;
  Result.Z = Z;
  
  return(Result);  
}

internal inline v4
V4(r32 X, r32 Y, r32 Z, r32 W)
{
  v4 Result;

  Result.X = X;
  Result.Y = Y;
  Result.Z = Z;
  Result.W = W;

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


///
///
///

internal inline v2
V2MulS(r32 S, v2 V)
{
  v2 Result;

  Result.X = S * V.X;
  Result.Y = S * V.Y;

  return(Result);  
}

internal inline v2
V2Neg(v2 V)
{
  v2 Result;

  Result.X = -V.X;
  Result.Y = -V.Y;

  return(Result);
}

internal inline v2
V2Add(v2 A, v2 B)
{
  v2 Result;

  Result.X = A.X + B.X;
  Result.Y = A.Y + B.Y;
  
  return(Result);
}

internal inline v2
V2Sub(v2 A, v2 B)
{
  v2 Result;

  Result.X = A.X - B.X;
  Result.Y = A.Y - B.Y;
  
  return(Result);
}

internal inline v2
V2Hadamard(v2 A, v2 B)
{
  v2 Result = V2(A.X * B.X,  A.Y * B.Y);
  return(Result);
}

internal inline r32
V2Inner(v2 A, v2 B)
{
  r32 Result = A.X * B.X + A.Y * B.Y;
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

///
///
///


internal inline v3
V3MulS(r32 S, v3 V)
{
  v3 Result;

  Result.X = S * V.X;
  Result.Y = S * V.Y;
  Result.Z = S * V.Z;

  return(Result);  
}

internal inline v3
V3Neg(v3 V)
{
  v3 Result;

  Result.X = -V.X;
  Result.Y = -V.Y;
  Result.Z = -V.Z;

  return(Result);
}

internal inline v3
V3Add(v3 A, v3 B)
{
  v3 Result;

  Result.X = A.X + B.X;
  Result.Y = A.Y + B.Y;
  Result.Z = A.Z + B.Z;
  
  return(Result);
}

internal inline v3
V3Sub(v3 A, v3 B)
{
  v3 Result;

  Result.X = A.X - B.X;
  Result.Y = A.Y - B.Y;
  Result.Z = A.Z - B.Z;
  
  return(Result);
}

internal inline v3
V3Hadamard(v3 A, v3 B)
{
  v3 Result = V3(A.X * B.X,  A.Y * B.Y, A.Z * B.Z);
  return(Result);
}

internal inline r32
V3Inner(v3 A, v3 B)
{
  r32 Result = A.X * B.X + A.Y * B.Y + A.Z * B.Z;
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

  Result.X = Clamp01(V.X);
  Result.Y = Clamp01(V.Y);
  Result.Z = Clamp01(V.Z);
  
  return(Result);
}

///
///
///

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
RectCenterDim2(v2 Center, v2 Dim)
{
  rectangle2 Result = RectCenterHalfDim2(Center,
					 V2MulS(0.5f, Dim));
  return(Result);
}

internal inline bool32
IsInRectangle2(rectangle2 Rectangle, v2 Test)
{
  bool32 Result = ((Test.X >= Rectangle.Min.X) &&
		   (Test.Y >= Rectangle.Min.Y) &&
		   (Test.X  < Rectangle.Max.X) &&
		   (Test.Y  < Rectangle.Max.Y));
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
RectCenterDim(v3 Center, v3 Dim)
{
  rectangle3 Result = RectCenterHalfDim(Center,
					V3MulS(0.5f, Dim));
  return(Result);
}

internal inline bool32
IsInRectangle(rectangle3 Rectangle, v3 Test)
{
  bool32 Result = ((Test.X >= Rectangle.Min.X) &&
		   (Test.Y >= Rectangle.Min.Y) &&
		   (Test.Z >= Rectangle.Min.Z) &&
		   (Test.X  < Rectangle.Max.X) &&
		   (Test.Y  < Rectangle.Max.Y) &&
		   (Test.Z  < Rectangle.Max.Z));
  return(Result);
}


internal inline bool32
RectanglesIntersect(rectangle3 A, rectangle3 B)
{
  bool32 Result = !((B.Max.X <= A.Min.X) ||
		    (B.Min.X >= A.Max.X) ||
		    (B.Max.Y <= A.Min.Y) ||
		    (B.Min.Y >= A.Max.Y) ||
		    (B.Max.Z <= A.Min.Z) ||
		    (B.Min.Z >= A.Max.Z));
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

internal inline v3
GetBarycentric(rectangle3 R, v3 P)
{
  v3 Result;

  Result.X = SafeRatio0((P.X - R.Min.X), (R.Max.X - R.Min.X));
  Result.Y = SafeRatio0((P.Y - R.Min.Y), (R.Max.Y - R.Min.Y));
  Result.Z = SafeRatio0((P.Z - R.Min.Z), (R.Max.Z - R.Min.Z));

  return(Result);
}
