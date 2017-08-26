
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

internal inline v2
VMulS(r32 S, v2 V)
{
  v2 Result;

  Result.X = S * V.X;
  Result.Y = S * V.Y;

  return(Result);  
}

internal inline v2
VNeg(v2 V)
{
  v2 Result;

  Result.X = -V.X;
  Result.Y = -V.Y;

  return(Result);
}

internal inline v2
VAdd(v2 A, v2 B)
{
  v2 Result;

  Result.X = A.X + B.X;
  Result.Y = A.Y + B.Y;
  
  return(Result);
}

internal inline v2
VSub(v2 A, v2 B)
{
  v2 Result;

  Result.X = A.X - B.X;
  Result.Y = A.Y - B.Y;
  
  return(Result);
}

internal inline r32
Square(r32 V)
{
  r32 Result = V*V;
  return(Result);
}

internal inline r32
Inner(v2 A, v2 B)
{
  r32 Result = A.X * B.X + A.Y * B.Y;
  return(Result);
}

internal inline r32
LengthSq(v2 V)
{
  r32 Result = Inner(V, V);
  return(Result);
}

internal inline r32
Length(v2 V)
{
  r32 Result = SquareRoot(LengthSq(V));
  return(Result);
}

typedef struct
{
  v2 Min;
  v2 Max;
} rectangle2;

internal inline v2
GetMinCorner(rectangle2 Rect)
{
  v2 Result = Rect.Min;
  return(Result);  
}

internal inline v2
GetMaxCorner(rectangle2 Rect)
{
  v2 Result = Rect.Max;
  return(Result);
}

internal inline v2
GetCenter(rectangle2 Rect)
{
  v2 Result = VMulS(0.5f, VAdd(Rect.Min, Rect.Max));
  return(Result);  
}

internal inline rectangle2
RectMinMax(v2 Min, v2 Max)
{
  rectangle2 Result;

  Result.Min = Min;
  Result.Max = Max;

  return(Result);
}

internal inline rectangle2
RectMinDim(v2 Min, v2 Dim)
{
  rectangle2 Result;

  Result.Min = Min;
  Result.Max = VAdd(Min, Dim);

  return(Result);
}

internal inline rectangle2
RectCenterHalfDim(v2 Center, v2 HalfDim)
{
  rectangle2 Result;

  Result.Min = VSub(Center, HalfDim);
  Result.Max = VAdd(Center, HalfDim);

  return(Result);
}

internal inline rectangle2
AddRadiusTo(rectangle2 A, r32 RadiusW, r32 RadiusH)
{
  rectangle2 Result;

  Result.Min = VSub(A.Min, V2(RadiusW, RadiusW));
  Result.Max = VAdd(A.Max, V2(RadiusH, RadiusH));

  return(Result);  
}

internal inline rectangle2
RectCenterDim(v2 Center, v2 Dim)
{
  rectangle2 Result = RectCenterHalfDim(Center,
					VMulS(0.5f, Dim));
  return(Result);
}

internal inline bool32
IsInRectangle(rectangle2 Rectangle, v2 Test)
{
  bool32 Result = ((Test.X >= Rectangle.Min.X) &&
		   (Test.Y >= Rectangle.Min.Y) &&
		   (Test.X  < Rectangle.Max.X) &&
		   (Test.Y  < Rectangle.Max.Y));
  return(Result);
}

