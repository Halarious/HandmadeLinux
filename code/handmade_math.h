
typedef union
{
  struct
  {
    r32 X;
    r32 Y;
  };
  r32 E[2];
} v2;

internal inline v2
V2(r32 X, r32 Y)
{
  v2 Result;

  Result.X = X;
  Result.Y = Y;

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
