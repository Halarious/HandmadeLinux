//TODO: Eventually implement these ourselves using
//      intrinsics and whatnot
#include <math.h>


#if COMPILER_MSVC
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier() 
#elif COMPILER_LLVM
#define CompletePreviousWritesBeforeFutureWrites asm volatile("" ::: "memory");
#else
#error Intrinsics not defined for this compiler!
#endif


inline internal s32
SignOf(s32 Value)
{
  s32 Result = (Value >= 0) ? 1 : -1;
  //NOTE: This one returns a zero value for positive 
  //signbit(Value);
  return(Result);
}

inline internal r32
SquareRoot(r32 Value)
{
  r32 Result = sqrtf(Value);
  return(Result);
}

inline internal r32
AbsoluteValue(r32 Value)
{
  r32 Result = fabs(Value);
  return(Result);
}

inline internal u32
RotateLeft(u32 Value, s32 Amount)
{
#if COMPILER_MSVC
  u32 Result = _rotl(Value, Amount);
#else
  Amount &= 31;
  u32 Result = ((Value << Amount) | (Value >> (32 - Amount)));
#endif
  return(Result);
}

inline internal u32
RotateRight(u32 Value, s32 Amount)
{
#if COMPILER_MSVC
  u32 Result = _rotr(Value, Amount);;
#else
  Amount &= 31;
  u32 Result = ((Value >> Amount) | (Value << (32 - Amount)));
#endif
  return(Result);
}

inline internal s32
RoundReal32ToInt32(r32 Value)
{
  s32 Result = (s32)roundf(Value);
  return(Result);
}

inline internal u32
RoundReal32ToUInt32(r32 Value)
{
  u32 Result = (u32)roundf(Value);
  return(Result);
}

inline internal s32
FloorReal32ToInt32(r32 Value)
{
  s32 Result = floorf(Value);
  return(Result);
}

inline internal s32
CeilReal32ToInt32(r32 Value)
{
  s32 Result = ceilf(Value);
  return(Result);
}

inline internal s32
TruncateReal32ToInt32(r32 Value)
{
  s32 Result = (u32)(Value);
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


typedef struct
{
  bool32 Found;
  u32    Index;
} bit_scan_result;

internal inline bit_scan_result
FindLeastSignificantSetBit(u32 Value)
{
  bit_scan_result Result = {};

#if COMPILER_MSVC
  Result.Found = _BitScanForward((unsigned long*)&Result.Index, Value);
#elif COMPILER_LLVM
  Result.Index = __builtin_ctzll(Value);
  Result.Found = true;
#else
  for(u32 Test = 0;
      Test < 32;
      ++Test)
    {
      if(Value & (1 << Test))
	{
	  Result.Index = Test;
	  Result.Found = true;
	  break;
	}
    }
#endif
  return(Result);
}

