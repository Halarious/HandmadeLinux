
#include <stdio.h>

global_variable r32 LeftEdge;
global_variable r32 AtY;
global_variable r32 FontScale;
global_variable font_id FontID;
global_variable r32 GlobalWidth;
global_variable r32 GlobalHeight;

/* 
   0x5c0f 小
   0x8033 耳
   0x6728 木
   0x514e 兎 
*/

internal void
DEBUGReset(assets* Assets, u32 Width, u32 Height)
{
  BEGIN_TIMED_FUNCTION(1);

  GlobalWidth  = (r32)Width;
  GlobalHeight = (r32)Height;
  
  asset_vector MatchVector  = {};
  asset_vector WeightVector = {};
  MatchVector.E[Tag_FontType] = (r32)FontType_Debug;
  WeightVector.E[Tag_FontType] = 1.0f;
  FontID  = GetBestMatchFontFrom(Assets,
				 Asset_Font,
				 &MatchVector, &WeightVector);

  FontScale = 1.0f;
  Orthographic(DEBUGRenderGroup, Width, Height, 1.0f);
  LeftEdge = -0.5f * (r32)Width;

  hha_font* Info = GetFontInfo(Assets, FontID);
  AtY = 0.5f * (r32)Height - FontScale*GetStartingBaselineY(Info);    

  END_TIMED_FUNCTION();
}

inline internal bool32
IsHex(char Char)
{
  bool32 Result = (((Char >= '0') && (Char <= '9')) ||
		   ((Char >= 'A') && (Char <= 'F')) ||
		   ((Char >= 'a') && (Char <= 'f')));
  return(Result);
}

inline internal bool32
GetHex(char Char)
{
  u32 Result = 0;
  if((Char >= '0') && (Char <= '9'))
    {
      Result = Char - '0';
    }
  else if ((Char >= 'A') && (Char <= 'F'))
    {
      Result = 0xa + (Char - 'A');
    }
  else if((Char >= 'a') && (Char <= 'f'))
    {
      Result = 0xa + (Char - 'a');      
    }
  
  return(Result);
}

internal void
DEBUGTextLine(char* String)
{
  if(DEBUGRenderGroup)
    {
      render_group* RenderGroup = DEBUGRenderGroup;

      loaded_font* Font = PushFont(RenderGroup, FontID);
      if(Font)
	{
	  hha_font* Info = GetFontInfo(RenderGroup->Assets, FontID);

	  u32 PrevCodePoint = 0;
	  r32 CharScale = FontScale;
	  v4 Color = V4(1.0f, 1.0, 1.0f, 1.0f);
	  r32 AtX = LeftEdge;
	  for(char* At = String;
	      *At;)
	    {
	      if( (At[0] == '\\') &&
		  (At[1] == '#')  &&
		  (At[2] != 0)    &&
		  (At[3] != 0)    &&
		  (At[4] != 0))
		{
		  r32 CScale = 1.0f / 9.0f;
		  Color = V4(Clamp01(CScale * (r32)(At[2] - '0')),
			     Clamp01(CScale * (r32)(At[3] - '0')),
			     Clamp01(CScale * (r32)(At[4] - '0')),
			     1.0f);
		  At += 5;
		}
	      else if( (At[0] == '\\') &&
		       (At[1] == '^')  &&
		       (At[2] != 0))
		{
		  r32 CScale = 1.0f / 9.0f;
		  CharScale = FontScale * Clamp01(CScale * (r32)(At[2] - '0'));
		  At += 3;
		}
	      else
		{
		  u32 CodePoint = *At;

		  if( (At[0] == '\\') &&
		      (IsHex(At[1]))  &&
		      (IsHex(At[2]))  &&
		      (IsHex(At[3]))  &&
		      (IsHex(At[4])))
		    {
		      CodePoint = ((GetHex(At[1]) << 12) |
				   (GetHex(At[2]) << 8)  |
				   (GetHex(At[3]) << 4)  |
				   (GetHex(At[4]) << 0));

		      At += 4;
		    }		  

		  r32 AdvanceX = CharScale * GetHorizontalAdvanceForPair(Info, Font, PrevCodePoint, CodePoint);
		  AtX += AdvanceX;

		  if(CodePoint != ' ')
		    {
		      bitmap_id BitmapID = GetBitmapForGlyph(RenderGroup->Assets, Info, Font, CodePoint);
		      hha_bitmap* Info = GetBitmapInfo(RenderGroup->Assets, BitmapID);

		      PushBitmapByID(RenderGroup, BitmapID,
				     V3(AtX, AtY, 0.0f),
				     CharScale * (r32)Info->Dim[1],
				     Color);
		    }
	      
		  PrevCodePoint = CodePoint;
		  ++At;
		}
	    }

	  AtY -= GetLineAdvanceFor(Info)*FontScale;
	}
    }
}

typedef struct
{
  r32 Count;
  r64 Min;
  r64 Max;
  r64 Avg;
} debug_statistic;

internal inline void
BeginDebugStatistic(debug_statistic* Stat)
{
  Stat->Count = 0.0f;
  Stat->Min = Real32Maximum;
  Stat->Max = -Real32Maximum;
  Stat->Avg = 0.0f;
}

internal inline void
UpdateDebugStatistic(debug_statistic* Stat, r64 Value)
{
  ++Stat->Count;

  if(Stat->Min > Value)
    {
      Stat->Min = Value;
    }

  if(Stat->Max < Value)
    {
      Stat->Max = Value;
    }

  Stat->Avg += Value;
}

internal inline void
EndDebugStatistic(debug_statistic* Stat)
{
  if(Stat->Count)
    {
      Stat->Avg /= (r64)Stat->Count;
    }
  else
    {
      Stat->Min = Stat->Max = 0.0f;
    }
}

internal void
DEBUGOverlay(memory* Memory, input* Input)
{
  debug_state* DebugState = (debug_state*)Memory->DebugStorage;
  if(DebugState && DEBUGRenderGroup)
    {
      render_group* RenderGroup = DEBUGRenderGroup;

      v2 MouseP = V2(Input->MouseX, Input->MouseY); 
      if(WasPressed(Input->MouseButtons[PlatformMouseButton_Right]))
	{
	  DebugState->Paused = !DebugState->Paused;
	}
      
      loaded_font* Font = PushFont(RenderGroup, FontID);
      if(Font)
	{
	  hha_font* Info = GetFontInfo(RenderGroup->Assets, FontID);

#if 0
	  for(u32 CounterIndex = 0;
	      CounterIndex < DebugState->CounterCount;
	      ++CounterIndex)
	    {
	      debug_counter_state* Counter = DebugState->CounterStates + CounterIndex;

	      debug_statistic HitCount, CycleCount, CycleOverHit;
	      BeginDebugStatistic(&HitCount);
	      BeginDebugStatistic(&CycleCount);
	      BeginDebugStatistic(&CycleOverHit);
	      for(u32 SnapshotIndex = 0;
		  SnapshotIndex < DEBUG_SNAPSHOT_COUNT;
		  ++SnapshotIndex)
		{
		  UpdateDebugStatistic(&HitCount, Counter->Snapshots[SnapshotIndex].HitCount);
		  UpdateDebugStatistic(&CycleCount, Counter->Snapshots[SnapshotIndex].CycleCount);

		  r64 COH = 0.0f;
		  if(Counter->Snapshots[SnapshotIndex].HitCount)
		    {
		      COH = ((r64)Counter->Snapshots[SnapshotIndex].CycleCount /
			     (r64)Counter->Snapshots[SnapshotIndex].HitCount);
		    }
		  UpdateDebugStatistic(&CycleOverHit, COH);
		}
	      EndDebugStatistic(&HitCount);
	      EndDebugStatistic(&CycleCount);
	      EndDebugStatistic(&CycleOverHit);

	      if(Counter->BlockName)
		{
		  if(CycleCount.Max > 0.0f)
		    {
		      r32 BarWidth = 4.0f;
		      r32 ChartLeft = 0.0f;
		      r32 ChartMinY = AtY;
		      r32 ChartHeight = Info->AscenderHeight*FontScale;
		      r32 Scale = 1.0f / (r32)CycleCount.Max;
		      for(u32 SnapshotIndex = 0;
			  SnapshotIndex < DEBUG_SNAPSHOT_COUNT;
			  ++SnapshotIndex)
			{
			  r32 ThisProportion = Scale*(r32)Counter->Snapshots[SnapshotIndex].CycleCount;
			  r32 ThisHeight = ChartHeight*ThisProportion;
			  PushRect(RenderGroup,
				   V3(ChartLeft + (r32)SnapshotIndex*BarWidth - 0.5f*BarWidth,
				      ChartMinY + 0.5f*ThisHeight, 0.0f),
				   V2(BarWidth, ThisHeight),
				   V4(ThisProportion, 1.0f, 0.0f, 1.0f));
			}
		    }

#if 1
		  char TextBuffer[256];
		  snprintf(TextBuffer, sizeof(TextBuffer),
			   "%24s(%4d): %10ucy %8uh %18ucy/h\n",
			   Counter->BlockName,
			   Counter->LineNumber,
			   (u32)CycleCount.Avg,
			   (u32)HitCount.Avg,
			   (u32)CycleOverHit.Avg);

		  DEBUGTextLine(TextBuffer);
#endif
		}
	    }
#endif
	  if(DebugState->FrameCount)
	    {
	      char TextBuffer[256];
	      snprintf(TextBuffer, sizeof(TextBuffer),
		       "Last frame time: %.02fms",
		       DebugState->Frames[DebugState->FrameCount - 1].WallSecondsElapsed * 1000.0f);
	      DEBUGTextLine(TextBuffer);
	    }

	  AtY -= 300.0f;
	  r32 LaneWidth = 8.0f;
	  r32 LaneCount = DebugState->FrameBarLaneCount;
	  r32 BarWidth = LaneCount*LaneWidth;
	  r32 BarSpacing = BarWidth + 4.0f;
	  r32 ChartLeft = LeftEdge + 10.0f;
	  r32 ChartWidth = BarSpacing*(r32)DebugState->FrameCount;
	  r32 ChartHeight = 300.0f;
	  r32 ChartMinY = -0.5f*GlobalHeight + 10.0f;
	  r32 Scale = ChartHeight*DebugState->FrameBarScale;	  

	  v3 Colours[] =
	    {
	      {1.0f, 0.0f, 0.0f},
	      {0.0f, 1.0f, 0.0f},
	      {0.0f, 0.0f, 1.0f},
	      {1.0f, 1.0f, 0.0f},
	      {0.0f, 1.0f, 1.0f},
	      {1.0f, 0.0f, 1.0f},

	      {1.0f, 0.5f, 0.0f},
	      {1.0f, 0.0f, 0.5f},
	      {0.5f, 1.0f, 0.0f},
	      {0.0f, 1.0f, 0.5f},
	      {0.0f, 1.0f, 1.0f},
	      {0.5f, 0.0f, 1.0f},
	      {0.0f, 0.5f, 1.0f},
	    };

	  u32 MaxFrame = DebugState->FrameCount;
	  if(MaxFrame > 10)
	    {
	      MaxFrame = 10;
	    }
	  for(u32 FrameIndex = 0;
	      FrameIndex < MaxFrame;
	      ++FrameIndex)
	    {
	      debug_frame* Frame = DebugState->Frames + DebugState->FrameCount - (FrameIndex + 1);
	      r32 StackX = ChartLeft + (r32)FrameIndex*BarSpacing;
	      r32 StackY = ChartMinY;	  
	      for(u32 RegionIndex = 0;
		  RegionIndex < Frame->RegionCount;
		  ++RegionIndex)
		{
		  debug_frame_region* Region = Frame->Regions + RegionIndex;

		  r32 ThisMinY = StackY + Scale*Region->MinT;
		  r32 ThisMaxY = StackY + Scale*Region->MaxT;

		  v3 Colour = Colours[RegionIndex % ArrayCount(Colours)];

		  rectangle2 RegionRect = RectMinMax2(V2(StackX + LaneWidth*Region->LaneIndex, ThisMinY),
						      V2(StackX + LaneWidth*(Region->LaneIndex + 1), ThisMaxY));

		  PushRect_Rect2(RenderGroup, RegionRect, 0.0f, ToV4(Colour, 1.0f));

		  if(IsInRectangle2(RegionRect, MouseP))
		    {
		      debug_record* Record = Region->Record;
		      char TextBuffer[256];
		      snprintf(TextBuffer, sizeof(TextBuffer),
			       "%24s: %10lucy [%s(%d)]",
			       Record->BlockName,
			       Region->CycleCount,
			       Record->FileName,
			       Record->LineNumber);
		      DEBUGTextLine(TextBuffer);
		    }
		}
	    }

	  PushRect(RenderGroup,
		   V3(ChartLeft + 0.5f*ChartWidth,
		      ChartMinY + ChartHeight, 0.0f),
		   V2(ChartWidth, 1.0),
		   V4(1.0f, 1.0f, 1.0f, 1.0f));
	}
      //DEBUGTextLine("\\5c0f\\8033\\6728\\514e");  
      //DEBUGTextLine("AVA WA Ta");
    }
}

#define DebugRecords_Main_Count __COUNTER__
extern u32 DebugRecords_Optimized_Count;

global_variable debug_table GlobalDebugTable_;
debug_table* GlobalDebugTable = &GlobalDebugTable_;

internal inline u32
GetLaneFromThreadIndex(debug_state* DebugState, u32 ThreadIndex)
{
  u32 Result = 0;
  return(0);
}

internal debug_thread*
GetDebugThread(debug_state* DebugState, u32 ThreadID)
{
  debug_thread* Result = 0;
  for(debug_thread* Thread = DebugState->FirstThread;
      Thread;
      Thread = Thread->Next)
    {
      if(Thread->ID == ThreadID)
	{
	  Result = Thread;
	  break;
	}
    }

  if(!Result)
    {
      Result = PushStruct(&DebugState->CollateArena, debug_thread);
      Result->ID = ThreadID;
      Result->LaneIndex = DebugState->FrameBarLaneCount++;
      Result->FirstOpenBlock = 0;
      Result->Next = DebugState->FirstThread;
      DebugState->FirstThread = Result;
    }

  return(Result);
}

internal debug_frame_region*
AddRegion(debug_state* DebugState, debug_frame* CurrentFrame)
{
  Assert(CurrentFrame->RegionCount < MAX_REGIONS_PER_FRAME);
  debug_frame_region* Result = CurrentFrame->Regions + CurrentFrame->RegionCount++;
  
  return(Result);
}

internal void
CollateDebugRecords(debug_state* DebugState, u32 InvalidEventArrayIndex)
{
  DebugState->Frames = PushArray(&DebugState->CollateArena, MAX_DEBUG_EVENT_ARRAY_COUNT*4, debug_frame);
  DebugState->FrameBarLaneCount = 0;
  DebugState->FrameCount = 0;
  DebugState->FrameBarScale = 1.0f / 60000000.0f;	  

  debug_frame* CurrentFrame = 0;
  for(u32 EventArrayIndex = InvalidEventArrayIndex + 1;
      ;
      ++EventArrayIndex)
    {
      if(EventArrayIndex == MAX_DEBUG_EVENT_ARRAY_COUNT)
	{
	  EventArrayIndex = 0;
	}

      if(EventArrayIndex == InvalidEventArrayIndex)
	{
	  break;
	}

      for(u32 EventIndex = 0;
	  EventIndex < GlobalDebugTable->EventCount[EventArrayIndex];
	  ++EventIndex)
	{
	  debug_event* Event = GlobalDebugTable->Events[EventArrayIndex] + EventIndex;
	  debug_record* Source = GlobalDebugTable->Records[Event->TranslationUnit] + Event->DebugRecordIndex;

	  if(Event->Type == DebugEvent_FrameMarker)
	    {
	      if(CurrentFrame)
		{
		  CurrentFrame->EndClock = Event->Clock;
		  CurrentFrame->WallSecondsElapsed = Event->SecondsElapsed;
		  ++DebugState->FrameCount;

		  r32 ClockRange = (r32)(CurrentFrame->EndClock - CurrentFrame->BeginClock);
#if 0
		  if(ClockRange > 0.0f)
		    {
		      r32 FrameBarScale = 1.0f / ClockRange;
		      if(DebugState->FrameBarScale > FrameBarScale)
			{
			  DebugState->FrameBarScale = FrameBarScale;
			}
		    }
#endif
		}
	      
	      CurrentFrame = DebugState->Frames + DebugState->FrameCount;
	      CurrentFrame->BeginClock = Event->Clock;
	      CurrentFrame->EndClock = 0;
	      CurrentFrame->RegionCount = 0;
	      CurrentFrame->Regions = PushArray(&DebugState->CollateArena, MAX_REGIONS_PER_FRAME, debug_frame_region);
	      CurrentFrame->WallSecondsElapsed = 0.0f;

	    }
	  else if(CurrentFrame)
	    {
	      u32 FrameIndex = DebugState->FrameCount - 1;
	      debug_thread* Thread = GetDebugThread(DebugState, Event->TC.ThreadID);
	      u64 RelativeClock = Event->Clock - CurrentFrame->BeginClock;

	      if(Event->Type == DebugEvent_BeginBlock)
		{
		  open_debug_block* DebugBlock = DebugState->FirstFreeBlock;
		  if(DebugBlock)
		    {
		      DebugState->FirstFreeBlock = DebugBlock->NextFree;
		    }
		  else
		    {
		      DebugBlock = PushStruct(&DebugState->CollateArena, open_debug_block); 
		    }

		  DebugBlock->StartingFrameIndex = FrameIndex;
		  DebugBlock->OpeningEvent = Event;
		  DebugBlock->Parent = Thread->FirstOpenBlock;
		  Thread->FirstOpenBlock = DebugBlock;
		  DebugBlock->NextFree = 0;
		}
	      else if(Event->Type == DebugEvent_EndBlock)
		{
		  if(StringsAreEqual(Source->BlockName, "GameUpdate"))
		    {
		      u32 Break = 0;
		    }

		  if(Thread->FirstOpenBlock)
		    {
		      open_debug_block* MatchingBlock = Thread->FirstOpenBlock;
		      debug_event* OpeningEvent = MatchingBlock->OpeningEvent;
		      if((OpeningEvent->TC.ThreadID == Event->TC.ThreadID) &&
			 (OpeningEvent->DebugRecordIndex == Event->DebugRecordIndex) &&
			 (OpeningEvent->TranslationUnit == Event->TranslationUnit))
			{
			  if(MatchingBlock->StartingFrameIndex == FrameIndex)
			    {
			      if(Thread->FirstOpenBlock->Parent == 0)
				{
				  r32 MinT = (r32)(OpeningEvent->Clock - CurrentFrame->BeginClock);
				  r32 MaxT = (r32)(Event->Clock - CurrentFrame->BeginClock);
				  r32 ThresholdT = 0.01f;
				  if((MaxT - MinT) > ThresholdT)
				    {
				      debug_frame_region* Region = AddRegion(DebugState, CurrentFrame);
				      Region->Record = Source;
				      Region->CycleCount = (Event->Clock - OpeningEvent->Clock);
				      Region->LaneIndex = Thread->LaneIndex;
				      Region->MinT = MinT;
				      Region->MaxT = MaxT;
				    }
				}
			    }
			  else
			    {
			    }

			  Thread->FirstOpenBlock->NextFree = DebugState->FirstFreeBlock;
			  DebugState->FirstFreeBlock = Thread->FirstOpenBlock;
			  Thread->FirstOpenBlock = MatchingBlock->Parent;
			}
		      else
			{
			  
			}		      
		    }
		}
	      else
		{
		  Assert(!"Invalid event type");
		}
	    }
	}
    }
  
#if 0
  debug_counter_state* CounterArray[MAX_DEBUG_TRANSLATION_UNITS];
  debug_counter_state* CurrentCount =  DebugState->CounterStates;
  u32 TotalRecordCount = 0;
  for(u32 UnitIndex = 0;
      UnitIndex < MAX_DEBUG_TRANSLATION_UNITS;
      ++UnitIndex)
    {
      CounterArray[UnitIndex] = CurrentCount;
      TotalRecordCount += GlobalDebugTable->RecordCount[UnitIndex];

      CurrentCount += GlobalDebugTable->RecordCount[UnitIndex];
    }
  DebugState->CounterCount = TotalRecordCount;

  for(u32 CounterIndex = 0;
      CounterIndex < DebugState->CounterCount;
      ++CounterIndex)
    {
      debug_counter_state* Dest = DebugState->CounterStates + CounterIndex;
      Dest->Snapshots[DebugState->SnapshotIndex].HitCount   = 0;
      Dest->Snapshots[DebugState->SnapshotIndex].CycleCount = 0;
    }  
    
  for(u32 EventIndex = 0;
      EventIndex < EventCount;
      ++EventIndex)
    {
      debug_event* Event = Events + EventIndex;

      debug_counter_state* Dest = CounterArray[Event->TranslationUnit] + Event->DebugRecordIndex;

      debug_record* Source = GlobalDebugTable->Records[Event->TranslationUnit] + Event->DebugRecordIndex;
      Dest->FileName = Source->FileName;
      Dest->BlockName = Source->BlockName;
      Dest->LineNumber = Source->LineNumber;
            
      if(Event->Type == DebugEvent_BeginBlock)
	{
	  ++Dest->Snapshots[DebugState->SnapshotIndex].HitCount;
	  Dest->Snapshots[DebugState->SnapshotIndex].CycleCount -= Event->Clock;
	}
      else if(Event->Type == DebugEvent_EndBlock)
	{
	  Dest->Snapshots[DebugState->SnapshotIndex].CycleCount += Event->Clock;
	}
    }
#endif
}

extern DEBUG_FRAME_END(DEBUGFrameEnd)
{
  GlobalDebugTable->RecordCount[0] = DebugRecords_Main_Count;
  GlobalDebugTable->RecordCount[1] = DebugRecords_Optimized_Count;

  ++GlobalDebugTable->CurrentEventArrayIndex;
  if(GlobalDebugTable->CurrentEventArrayIndex >= ArrayCount(GlobalDebugTable->Events))
    {
      GlobalDebugTable->CurrentEventArrayIndex = 0;
    }
  
  u64 ArrayIndex_EventIndex = AtomicExchangeUInt64(&GlobalDebugTable->EventArrayIndex_EventIndex,
						   ((u64)GlobalDebugTable->CurrentEventArrayIndex << 32));

  u32 EventArrayIndex = ArrayIndex_EventIndex >> 32;
  u32 EventCount = ArrayIndex_EventIndex & 0xffffffff;
  GlobalDebugTable->EventCount[EventArrayIndex] = EventCount;
  
  debug_state* DebugState = (debug_state*)Memory->DebugStorage;
  if(DebugState)
    {
      if(!DebugState->Initialized)
	{
	  InitializeArena(&DebugState->CollateArena, Memory->DebugStorageSize - sizeof(debug_state), DebugState + 1);
	  DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->CollateArena);
	}

      if(!DebugState->Paused)
	{
	  EndTemporaryMemory(DebugState->CollateTemp);
	  DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->CollateArena);

	  DebugState->FirstThread = 0;
	  DebugState->FirstFreeBlock = 0;

	  CollateDebugRecords(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
	}
    }
  
  return(GlobalDebugTable);
}

