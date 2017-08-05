//TODO: Eventually implement these ourselves using
//      intrinsics and whatnot
#include <math.h>

inline internal s32
RoundReal32ToInt32(r32 Value)
{
  s32 Result = (s32)(Value + 0.5f);
  return(Result);
}

inline internal u32
RoundReal32ToUInt32(r32 Value)
{
  u32 Result = (u32)(Value + 0.5f);
  return(Result);
}

inline internal s32
FloorReal32ToInt32(r32 Value)
{
  u32 Result = floorf(Value);
  return(Result);
}

inline internal s32
TruncateReal32ToInt32(r32 Value)
{
  u32 Result = (u32)(Value);
  return(Result);
}

inline internal r32
Sin(r32 Angle)
{
  r32 Result = sinf(Angle);
  return(Result);
}

inline internal r32
Cos(r32 Angle)
{
  r32 Result = cosf(Angle);
  return(Result);
}

inline internal r32
Atan2(r32 Y, r32 X)
{
  r32 Result = atan2f(Y, X);
  return(Result);
}
