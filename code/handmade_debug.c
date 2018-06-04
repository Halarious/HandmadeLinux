
#include <stdio.h>

#include "handmade_debug.h"
#include "handmade_debug_variables.h"

internal void
RestartCollation(debug_state* DebugState, u32 InvalidEventArrayIndex);

/* 
   0x5c0f 小
   0x8033 耳
   0x6728 木
   0x514e 兎 
*/

internal inline debug_state*
DEBUGGetState(memory* Memory)
{
  debug_state* DebugState = (debug_state*)Memory->DebugStorage;
  Assert(DebugState->Initialized)
    
  return(DebugState);
}

internal inline debug_state*
DEBUGGetStateGlobalMemory()
{
  debug_state* Result = DEBUGGetState(DebugGlobalMemory);
  return(Result);  
}

internal debug_tree*
AddTree(debug_state* DebugState, debug_variable* Group, v2 AtP)
{
  debug_tree* Tree = PushStruct(&DebugState->DebugArena, debug_tree);

  Tree->UIP = AtP;
  Tree->Group = Group;

  DLIST_INSERT(&DebugState->TreeSentinel, Tree);
  
  return(Tree);
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

internal rectangle2
DEBUGTextOp(debug_state* DebugState, debug_text_op Op, v2 P,
	    char* String, v4 Colour)
{
  rectangle2 Result = InvertedInfinityRectangle2();
  if(DebugState && DebugState->DebugFont)
    {
      render_group* RenderGroup = DebugState->RenderGroup;
      loaded_font* Font = DebugState->DebugFont;
      hha_font* Info = DebugState->DebugFontInfo; 

      u32 PrevCodePoint = 0;
      r32 CharScale = DebugState->FontScale;
      r32 AtX = P.x;
      r32 AtY = P.y;
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
	      Colour = V4(Clamp01(CScale * (r32)(At[2] - '0')),
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
	      CharScale = DebugState->FontScale * Clamp01(CScale * (r32)(At[2] - '0'));
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

		  r32 BitmapScale = CharScale*(r32)Info->Dim[1];
		  v3 BitmapOffset = V3(AtX, AtY, 0.0f);
		  if(Op == DEBUGTextOp_DrawText)
		    {
		      PushBitmapByID(RenderGroup, BitmapID,
				     BitmapOffset,
				     BitmapScale,
				     Colour, 1.0f);
		    }
		  else
		    {
		      Assert(DEBUGTextOp_SizeText);

		      loaded_bitmap* Bitmap = GetBitmap(RenderGroup->Assets, BitmapID, RenderGroup->GenerationID);
		      if(Bitmap)
			{
			  used_bitmap_dim Dim = GetBitmapDim(RenderGroup, Bitmap,
							     BitmapOffset,
							     BitmapScale, 1.0f);
			  rectangle2 GlyphDim = RectMinDim2(Dim.P.xy, Dim.Size);
			  Result = Union2(Result, GlyphDim);
			}
		    }
		}
	      
	      PrevCodePoint = CodePoint;
	      ++At;
	    }
	}
    }

  return(Result);
}

internal void
DEBUGTextOutAt(v2 P, char* String, v4 Colour)
{
  debug_state* DebugState = DEBUGGetStateGlobalMemory();
  if(DebugState)
    {
      render_group* RenderGroup = DebugState->RenderGroup;
      DEBUGTextOp(DebugState, DEBUGTextOp_DrawText, P, String, Colour);
    }  
}


internal rectangle2
DEBUGGetTextSize(debug_state* DebugState, char* String)
{
  rectangle2 Result = DEBUGTextOp(DebugState, DEBUGTextOp_SizeText,
				  V2(0.0f, 0.0f), String, V4(1.0f, 1.0f, 1.0f, 1.0f));
  return(Result);
}

internal void
DEBUGTextLine(char* String)
{
  debug_state* DebugState = DEBUGGetStateGlobalMemory();
  if(DebugState)
    {
      render_group* RenderGroup = DebugState->RenderGroup;

      loaded_font* Font = PushFont(RenderGroup, DebugState->FontID);
      if(Font)
	{
	  hha_font* Info = GetFontInfo(RenderGroup->Assets, DebugState->FontID);

	  DEBUGTextOutAt(V2(DebugState->LeftEdge,
			    DebugState->AtY- DebugState->FontScale*GetStartingBaselineY(DebugState->DebugFontInfo)), String, V4(1.0f, 1.0f, 1.0f, 1.0f));

	  DebugState->AtY -= GetLineAdvanceFor(Info)*DebugState->FontScale;
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

internal memory_index
DEBUGVariableToText(char* Buffer, char* End, debug_variable* Var, u32 Flags)
{
  char* At = Buffer;

  if(Flags & DEBUGVarToText_AddDebugUI)
    {
      At += snprintf(At, (size_t)(End - At),	
		     "#define DEBUGUI_");
    }

  if(Flags & DEBUGVarToText_AddName)
    {
      At += snprintf(At, (size_t)(End - At),	
		     "%s%s ", Var->Name, Flags & DEBUGVarToText_Colon ? ":" : "");
    }
  
  switch(Var->Type)
    {
    case(DebugVariable_Bool32):
      {
	if(Flags & DEBUGVarToText_PrettyBools)
	  {
	    At += snprintf(At, (size_t)(End - At),	
			   "%s",
			   Var->Bool32 ? "true" : "false");
	  }
	else
	  {
	    At += snprintf(At, (size_t)(End - At),	
			   "%d",
			   Var->Bool32);
	  }
      } break;

    case(DebugVariable_Int32):
      {
	At += snprintf(At, (size_t)(End - At),	
		       "%d",
		       Var->Int32);
      } break;

    case(DebugVariable_UInt32):
      {
	At += snprintf(At, (size_t)(End - At),
		       "%u",
		       Var->UInt32);
      } break;

    case(DebugVariable_V2):
      {
	At += snprintf(At, (size_t)(End - At),
		       "V2(%f, %f)",
		       Var->Vector2.x, Var->Vector2.y);
      } break;

    case(DebugVariable_V3):
      {
	At += snprintf(At, (size_t)(End - At),
		       "V3(%f, %f, %f)",
		       Var->Vector3.x, Var->Vector3.y,
		       Var->Vector3.z);
      
      } break;
      
    case(DebugVariable_V4):
      {
	At += snprintf(At, (size_t)(End - At),
		       "V4(%f, %f, %f, %f)",
		       Var->Vector4.x, Var->Vector4.y,
		       Var->Vector4.z, Var->Vector4.w);
      
      } break;

    case(DebugVariable_Real32):
      {
	At += snprintf(At, (size_t)(End - At),
		       "%f",
		       Var->Real32);
	
	if(Flags & DEBUGVarToText_FloatSuffix)
	  {
	    *At++ = 'f';
	  }

      } break;
	  	  
    case(DebugVariable_VarGroup):
    case(DebugVariable_CounterThreadList):
    case(DebugVariable_BitmapDisplay):
      {
	
      } break;

      InvalidCase;
    }

  if(Flags & DEBUGVarToText_LineFeedEnd)
    {
      *At++ = '\n';
    }

  if(Flags & DEBUGVarToText_NullTerminator)
    {
      *At++ = 0;
    }
  
  return(At - Buffer);
}

typedef struct
{
  debug_variable_link* Link;
  debug_variable_link* Sentinel;
} debug_variable_iterator;

#define HANDMADE_CONFIG_FILE_NAME "../code/handmade_config.h"
internal inline void
WriteHandmadeConfig(debug_state* DebugState)
{
  char Temp[4096];
  char* At = Temp;
  char* End = Temp + sizeof(Temp);

  u32 Depth = 0;
  debug_variable_iterator Stack[DEBUG_MAX_VARIABLE_STACK_DEPTH];

  Stack[Depth].Link = DebugState->RootGroup->VarGroup.Next;
  Stack[Depth].Sentinel = &DebugState->RootGroup->VarGroup;
  ++Depth;
  while(Depth > 0)
    {
      debug_variable_iterator* Iter = Stack + (Depth - 1);
      if(Iter->Link != Iter->Sentinel)
	{
	  --Depth;
	}
      else
	{
	  debug_variable* Var = Iter->Link->Var;
	  Iter->Link = Iter->Link->Next;
	  
	  if(DEBUGShouldBeWritten(Var->Type))
	    {
	      for(int Indent = 0;
		  Indent < Depth;
		  ++Indent)
		{
		  *At++ = ' ';
		  *At++ = ' ';
		  *At++ = ' ';
		  *At++ = ' ';
		}

	      if(Var->Type == DebugVariable_VarGroup)
		{
		  At += snprintf(At, (size_t)(End - At),
				 "//");
		}
      
	      At += DEBUGVariableToText(At, End, Var,
					DEBUGVarToText_AddDebugUI  |
					DEBUGVarToText_AddName     |
					DEBUGVarToText_FloatSuffix |
					DEBUGVarToText_LineFeedEnd);
	    }
      
	  if(Var->Type == DebugVariable_VarGroup)
	    {
	      debug_variable_iterator* Iter = Stack + Depth;
	      Iter->Link = Var->VarGroup.Next;
	      Iter->Sentinel = &Var->VarGroup;
	      ++Depth;
	    }
	}
    }  
  Platform.DEBUGWriteEntireFile(HANDMADE_CONFIG_FILE_NAME, (u32)(At - Temp), Temp);

  if(!DebugState->Compiling)
    {
      DebugState->Compiling = true;
      DebugState->Compiler = Platform.DEBUGExecuteSystemCommand("../code", "/bin/sh", "../code/build.linux");
    }
}

internal void
DrawProfileIn(debug_state* DebugState, rectangle2 ProfileRect, v2 MouseP)
{
  PushRect_Rect2(DebugState->RenderGroup, ProfileRect,
		 0.0f, V4(0.0f, 0.0f, 0.0f, 0.25f));
	  
  r32 BarSpacing = 4.0f;
  r32 LaneHeight = 0.0f;
  r32 LaneCount = DebugState->FrameBarLaneCount;

  u32 MaxFrame = DebugState->FrameCount;
  if(MaxFrame > 10)
    {
      MaxFrame = 10;
    }
	  
  if((LaneCount > 0) && (MaxFrame > 0))
    {
      r32 PixelsPerFramePlusSpacing = ((GetDim2(ProfileRect).y / (r32)MaxFrame));
      r32 PixelsPerFrame = PixelsPerFramePlusSpacing - BarSpacing; 
      LaneHeight = PixelsPerFrame / (r32)LaneCount;
    }
	  
  r32 BarHeight = LaneCount*LaneHeight;
  r32 BarPlusSpacing = BarHeight + BarSpacing;
  r32 ChartLeft = ProfileRect.Min.x;
  r32 ChartWidth = GetDim2(ProfileRect).x;
  r32 ChartHeight = BarPlusSpacing*(r32)MaxFrame;
  r32 ChartTop = ProfileRect.Max.y;
  r32 Scale = ChartWidth*DebugState->FrameBarScale;	  

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

  for(u32 FrameIndex = 0;
      FrameIndex < MaxFrame;
      ++FrameIndex)
    {
      debug_frame* Frame = DebugState->Frames + DebugState->FrameCount - (FrameIndex + 1);
      r32 StackX = ChartLeft;	  
      r32 StackY = ChartTop - (r32)FrameIndex*BarPlusSpacing;
      for(u32 RegionIndex = 0;
	  RegionIndex < Frame->RegionCount;
	  ++RegionIndex)
	{
	  debug_frame_region* Region = Frame->Regions + RegionIndex;

	  r32 ThisMinX = StackX + Scale*Region->MinT;
	  r32 ThisMaxX = StackX + Scale*Region->MaxT;

	  //v3 Colour = Colours[RegionIndex % ArrayCount(Colours)];
	  v3 Colour = Colours[Region->ColourIndex % ArrayCount(Colours)];

	  rectangle2 RegionRect = RectMinMax2(V2(ThisMinX, StackY - LaneHeight*(Region->LaneIndex + 1)),
					      V2(ThisMaxX, StackY - LaneHeight*Region->LaneIndex));

	  PushRect_Rect2(DebugState->RenderGroup, RegionRect, 0.0f, ToV4(Colour, 1.0f));

	  if(IsInRectangle2(RegionRect, MouseP))
	    {
	      debug_record* Record = Region->Record;
	      char TextBuffer[256];
	      snprintf(TextBuffer, sizeof(TextBuffer),
		       "%s: %10lucy [%s(%d)]",
		       Record->BlockName,
		       Region->CycleCount,
		       Record->FileName,
		       Record->LineNumber);
	      DEBUGTextOutAt(V2Add(MouseP, V2(0.0f, 10.0f)), TextBuffer, V4(1.0f, 1.0f, 1.0f, 1.0f));
		      
	      //HotRecord = Record;
			
	    }
	}
    }
#if 0
  PushRect(RenderGroup,
	   V3(ChartLeft + 0.5f*ChartWidth,
	      ChartMinY + ChartHeight, 0.0f),
	   V2(ChartWidth, 1.0),
	   V4(1.0f, 1.0f, 1.0f, 1.0f));
#endif
}

internal inline bool32
InteractionsAreEqual(debug_interaction A, debug_interaction B)
{
  bool32 Result = ((A.Type == B.Type) &&
		   (A.Generic == B.Generic)); 
  return(Result);
}

internal inline bool32
InteractionIsHot(debug_state* DebugState, debug_interaction Interaction)
{
  bool32 Result = InteractionsAreEqual(DebugState->HotInteraction, Interaction);
  return(Result);
}

typedef struct
{
  debug_state* DebugState;

  v2 MouseP;
  v2 At;
  u32 Depth;
  r32 LineAdvance;  
  r32 SpacingY;
} layout;

typedef struct
{
  layout* Layout;
  v2* Dim;
  v2* Size;
  debug_interaction Interaction;

  rectangle2 Bounds;
} layout_element;

internal inline layout_element
BeginElementRectangle(layout* Layout, v2* Dim)
{
  layout_element Element = {};
  Element.Layout = Layout;
  Element.Dim = Dim;
    
  return(Element);
}

internal inline void
MakeElementSizeable(layout_element* Element)
{
  Element->Size = Element->Dim;
}

internal inline void
DefaultInteraction(layout_element* Element, debug_interaction Interaction)
{
  Element->Interaction = Interaction;
}

internal inline void
EndElement(layout_element* Element)
{
  layout* Layout = Element->Layout;
  debug_state* DebugState = Layout->DebugState;
    
  r32 SizeHandlePixels = 4.0f;
  
  v2 Frame = {0.0f, 0.0f};
  if(Element->Size)
    {
      Frame.x = SizeHandlePixels;
      Frame.y = SizeHandlePixels;
    }

  v2 TotalDim  = V2Add(*Element->Dim, V2MulS(2.0f, Frame));
  
  v2 TotalMinCorner = V2(Layout->At.x + Layout->Depth*2.0f*Layout->LineAdvance,
			 Layout->At.y - TotalDim.y);
  v2 TotalMaxCorner = V2Add(TotalMinCorner, TotalDim);

  v2 InteriorMinCorner = V2Add(TotalMinCorner, Frame);
  v2 InteriorMaxCorner = V2Add(InteriorMinCorner, *Element->Dim);

  rectangle2 TotalBounds = RectMinMax2(TotalMinCorner, TotalMaxCorner);
  Element->Bounds = RectMinMax2(InteriorMinCorner, InteriorMaxCorner);

  if(Element->Interaction.Type &&
     IsInRectangle2(Element->Bounds, Layout->MouseP))
    {
      DebugState->NextHotInteraction = Element->Interaction;
    }

  if(Element->Size)
    {
      PushRect_Rect2(DebugState->RenderGroup, RectMinMax2(V2(TotalMinCorner.x, InteriorMinCorner.y),
							  V2(InteriorMinCorner.x, InteriorMaxCorner.y)),
		     0.0f,
		     V4(0.0f, 0.0f, 0.0f, 1.0f));
      PushRect_Rect2(DebugState->RenderGroup, RectMinMax2(V2(InteriorMaxCorner.x, InteriorMinCorner.y),
							  V2(TotalMaxCorner.x, InteriorMaxCorner.y)),
		     0.0f,
		     V4(0.0f, 0.0f, 0.0f, 1.0f));
      PushRect_Rect2(DebugState->RenderGroup, RectMinMax2(V2(InteriorMinCorner.x, TotalMinCorner.y),
							  V2(InteriorMaxCorner.x, InteriorMinCorner.y)),
		     0.0f,
		     V4(0.0f, 0.0f, 0.0f, 1.0f));
      PushRect_Rect2(DebugState->RenderGroup, RectMinMax2(V2(InteriorMinCorner.x, InteriorMaxCorner.y),
							  V2(InteriorMaxCorner.x, TotalMaxCorner.y)),
		     0.0f,
		     V4(0.0f, 0.0f, 0.0f, 1.0f));

      debug_interaction SizeInteraction = {};
      SizeInteraction.Type = DebugInteraction_Resize;
      SizeInteraction.P    = Element->Size;
  
      rectangle2 SizeBox = RectMinMax2(V2(InteriorMaxCorner.x, TotalMinCorner.y),
				       V2(TotalMaxCorner.x,    InteriorMinCorner.y));
      PushRect_Rect2(DebugState->RenderGroup, SizeBox, 0.0f,
		     (InteractionIsHot(DebugState, SizeInteraction) ?
		      V4(1.0f, 1.0f, 0.0f, 1.0f) : V4(1.0f, 1.0f, 1.0f, 1.0f)));

      if(IsInRectangle2(SizeBox, Layout->MouseP))
	{
	  DebugState->NextHotInteraction = SizeInteraction;
	}
    }
  
  r32 SpacingY = Layout->SpacingY;
  if(0)
    {
      SpacingY = 0.0f;
    }
  Layout->At.y = GetMinCorner2(TotalBounds).y - SpacingY;
}

internal debug_view*
GetDebugViewFor(debug_state* DebugState, debug_variable* Var)
{
  debug_view* Result = 0;
  return(Result);
}

internal void
DEBUGDrawMainMenu(debug_state* DebugState, render_group* RenderGroup,
		  v2 MouseP)
{
  for(debug_tree* Tree = DebugState->TreeSentinel.Next;
      Tree != &DebugState->TreeSentinel;
      Tree = Tree->Next)
    {
      layout Layout = {};
      Layout.DebugState = DebugState;
      Layout.MouseP = MouseP;
      Layout.At = Tree->UIP;
      Layout.LineAdvance = DebugState->FontScale*GetLineAdvanceFor(DebugState->DebugFontInfo);
      Layout.SpacingY = 4.0f;  

      u32 Depth = 0;
      debug_variable_iterator Stack[DEBUG_MAX_VARIABLE_STACK_DEPTH];

      Stack[Depth].Link = DebugState->RootGroup->VarGroup.Next;
      Stack[Depth].Sentinel = &DebugState->RootGroup->VarGroup;
      ++Depth;
      while(Depth > 0)
	{
	  debug_variable_iterator* Iter = Stack + (Depth - 1);
	  if(Iter->Link != Iter->Sentinel)
	    {
	      --Depth;
	    }
	  else
	    {
	      debug_variable* Var = Iter->Link->Var;
	      Iter->Link = Iter->Link->Next;

	      debug_interaction ItemInteraction = {};
	      ItemInteraction.Type = DebugInteraction_AutoModifyVariable;
	      ItemInteraction.Var  = Var;

	      bool32 IsHot = InteractionIsHot(DebugState, ItemInteraction);
	  
	      v4 ItemColour = IsHot ?
		V4(1.0f, 1.0f, 0.0f, 1.0f) : V4(1.0f, 1.0f, 1.0f, 1.0f);

	      debug_view* View = GetDebugViewFor(DebugState, Var);
	      switch(Var->Type)
		{
		case(DebugVariable_CounterThreadList):
		  {		    
		    layout_element Element =
		      BeginElementRectangle(&Layout, &View->InlineBlock.Dim);
		    MakeElementSizeable(&Element);
		    DefaultInteraction(&Element, ItemInteraction);
		    EndElement(&Element);

		    DrawProfileIn(DebugState, Element.Bounds, MouseP);
		  } break;

		case(DebugVariable_BitmapDisplay):
		  {
		    loaded_bitmap* Bitmap = GetBitmap(RenderGroup->Assets, Var->BitmapDisplay.ID, RenderGroup->GenerationID);
		    r32 BitmapScale = View->InlineBlock.Dim.y;
		    if(Bitmap)
		      {
			used_bitmap_dim Dim = GetBitmapDim(RenderGroup, Bitmap,
							   V3(0.0f, 0.0f, 0.0f),
							   BitmapScale, 1.0f);
			View->InlineBlock.Dim.x = Dim.Size.x;
		      }

		    debug_interaction TearInteraction = {};
		    TearInteraction.Type = DebugInteraction_TearValue;
		    TearInteraction.Var  = Var;
				
		    layout_element Element =
		      BeginElementRectangle(&Layout, &View->InlineBlock.Dim);
		    MakeElementSizeable(&Element);
		    DefaultInteraction(&Element, TearInteraction);
		    EndElement(&Element);
				
		    PushRect_Rect2(DebugState->RenderGroup, Element.Bounds, 0.0f,
				   V4(0.0f, 0.0f, 0.0f, 1.0f));
		    PushBitmapByID(DebugState->RenderGroup, Var->BitmapDisplay.ID,
				   ToV3(GetMinCorner2(Element.Bounds), 0.0f), BitmapScale, V4(1.0f, 1.0f, 1.0f, 1.0f), 0.0f);
		  } break;
	      
		default:
		  {
		    char Text[256];
		    DEBUGVariableToText(Text, Text + sizeof(Text), Var,
					DEBUGVarToText_AddName        |
					DEBUGVarToText_NullTerminator |
					DEBUGVarToText_Colon|
					DEBUGVarToText_PrettyBools);

		    rectangle2 TextBounds = DEBUGGetTextSize(DebugState, Text);
		    v2 Dim = {GetDim2(TextBounds).x, Layout.LineAdvance};

		    layout_element Element = BeginElementRectangle(&Layout, &Dim);
		    DefaultInteraction(&Element, ItemInteraction);
		    EndElement(&Element);

		    DEBUGTextOutAt(V2(GetMinCorner2(Element.Bounds).x,
				      GetMaxCorner2(Element.Bounds).y - DebugState->FontScale*GetStartingBaselineY(DebugState->DebugFontInfo)),
				   Text, ItemColour);      
		  }
		}
      
	      if(Var->Type == DebugVariable_VarGroup)
		{
		  debug_variable_iterator* Iter = Stack + Depth;
		  Iter->Link = Var->VarGroup.Next;
		  Iter->Sentinel = &Var->VarGroup;
		  ++Depth;
		}
	    }
	}
      
      DebugState->AtY = Layout.At.y;
      
      if(1)
	{
	  debug_interaction MoveInteraction = {};
	  MoveInteraction.Type = DebugInteraction_Move;
	  MoveInteraction.P    = &Tree->UIP;
	  
	  rectangle2 MoveBox = RectCenterHalfDim2(V2Sub(Tree->UIP, V2(4.0f, 4.0f)), V2(4.0f, 4.0f));
	  PushRect_Rect2(DebugState->RenderGroup, MoveBox, 0.0f,
			 InteractionIsHot(DebugState, MoveInteraction) ?
			 V4(1.0f, 1.0f, 0.0f, 1.0f) : V4(1.0f, 1.0f, 1.0f, 1.0f));

	  if(IsInRectangle2(MoveBox, MouseP))
	    {
	      DebugState->NextHotInteraction = MoveInteraction;
	    }
	}

    }
  
#if 0
  u32 NewHotMenuIndex = ArrayCount(DebugVariableList);
  r32 BestDistanceSq = Real32Maximum;

  r32 MenuRadius = 300.0f;
  r32 AngleStep = Tau32 / (r32)ArrayCount(DebugVariableList);
  for(u32 MenuItemIndex = 0;
      MenuItemIndex <  ArrayCount(DebugVariableList);
      ++MenuItemIndex)
    {
      debug_variable* Var = DebugVariableList + MenuItemIndex;
      char* Text = Var->Name;
      
      v4 ItemColour = Var->Value ? V4(1.0f, 1.0f, 1.0f, 1.0f) : V4(0.5f, 0.5f, 0.5f, 1.0f);
      if(MenuItemIndex == DebugState->HotMenuIndex)
	{
	  ItemColour = V4(1.0f, 1.0f, 0.0f, 1.0f);
	}
      
      r32 Angle = (r32)MenuItemIndex * AngleStep;
      v2 TextP = V2Add(DebugState->MenuP, V2MulS(MenuRadius, Arm2(Angle)));

      r32 ThisDistanceSq = V2LengthSq(V2Sub(TextP, MouseP));
      if(BestDistanceSq > ThisDistanceSq)
	{
	  BestDistanceSq = ThisDistanceSq;
	  NewHotMenuIndex = MenuItemIndex;
	}
      
      rectangle2 TextBounds = DEBUGGetTextSize(DebugState, Text);
      DEBUGTextOutAt(V2Sub(TextP, V2MulS(0.5f, GetDim2(TextBounds))), Text, ItemColour);
    }

  if(V2LengthSq(V2Sub(MouseP, DebugState->MenuP)) > Square(MenuRadius))
    {
      DebugState->HotMenuIndex = NewHotMenuIndex;
    }
  else
    {
      DebugState->HotMenuIndex = ArrayCount(DebugVariableList);
    }
#endif
}

internal void
DEBUGBeginInteract(debug_state* DebugState, input* Input, v2 MouseP, bool32 AltUI)
{
  if(DebugState->HotInteraction.Type)
    {
      if(DebugState->HotInteraction.Type == DebugInteraction_AutoModifyVariable)
	{
	  switch(DebugState->HotInteraction.Var->Type)
	    {
	    case(DebugVariable_Bool32):
	      {
		DebugState->HotInteraction.Type = DebugInteraction_ToggleValue;
	      } break;

	    case(DebugVariable_Real32):
	      {
		DebugState->HotInteraction.Type = DebugInteraction_DragValue;
	      } break;

	    case(DebugVariable_VarGroup):
	      {
		DebugState->HotInteraction.Type = DebugInteraction_ToggleValue;
	      } break;

	    default: {}
	    }
	        
	  if(AltUI)
	    {
	      DebugState->HotInteraction.Type = DebugInteraction_TearValue;
	    }
	}

      switch(DebugState->HotInteraction.Type)
	{
	case(DebugInteraction_TearValue):
	  {
#if 0
	    debug_variable_reference* RootGroup = DEBUGAddRootGroup(DebugState, "NewUserGroup");
	    DEBUGAddVariableReference(DebugState, RootGroup, DebugState->HotInteraction.Var);
	    debug_variable_tree* Tree = AddTree(DebugState, RootGroup, V2(0, 0));
	    Tree->UIP = MouseP;

	    DebugState->HotInteraction.Type = DebugInteraction_Move;
	    DebugState->HotInteraction.P = &Tree->UIP;
#endif
	  } break;

	default: {}
	}

      DebugState->Interaction = DebugState->HotInteraction;	  
    }
  else
    {
      DebugState->Interaction.Type = DebugInteraction_NOP;
    }
}

internal void
DEBUGEndInteract(debug_state* DebugState, input* Input, v2 MouseP)
{
  switch(DebugState->Interaction.Type)
    {
    case(DebugInteraction_ToggleValue):
      {
	debug_variable* Var = DebugState->Interaction.Var;
	Assert(Var);

	switch(Var->Type)
	  {
	  case(DebugVariable_Bool32):
	    {
	      Var->Bool32 = !Var->Bool32;
	    } break;

	  case(DebugVariable_VarGroup):
	    {
	      debug_view* View = GetDebugViewFor(DebugState, Var);
	      View->Collapsible.ExpandedAlways = !View->Collapsible.ExpandedAlways;
	    } break;

	  default: {}
	  }
      }

    default: {}
    }
  
  WriteHandmadeConfig(DebugState);

  DebugState->Interaction.Type = DebugInteraction_None;
  DebugState->Interaction.Generic = 0;
}

internal void
DEBUGInteract(debug_state* DebugState, input* Input, v2 MouseP)
{
  v2 dMouseP = V2Sub(MouseP, DebugState->LastMouseP);
  
#if 0
  if(Input->MouseButtons[PlatformMouseButton_Right].EndedDown)
    {
      if(Input->MouseButtons[PlatformMouseButton_Right].HalfTransitionCount > 0)
	{
	  DebugState->MenuP = MouseP;
	}
      DrawDebugMainMenu(DebugState, RenderGroup, MouseP);
    }
  else if(Input->MouseButtons[PlatformMouseButton_Right].HalfTransitionCount > 0)
#endif
    
    if(DebugState->Interaction.Type)
      {
	debug_variable* Var = DebugState->Interaction.Var;
	debug_tree* Tree = DebugState->Interaction.Tree;
	v2* P = DebugState->Interaction.P;
	
	switch(DebugState->Interaction.Type)
	  {
	  case(DebugInteraction_ToggleValue):
	    {
	    } break;

	  case(DebugInteraction_DragValue):
	    {
	      switch(Var->Type)
		{
		case(DebugVariable_Real32):
		  {
		    Var->Real32 += 0.1f*dMouseP.y;
		  } break;
		
		default:
		  {
		  }
		}
	    } break;

	  case(DebugInteraction_Resize):
	    {
	      *P  = V2Add(*P, V2(dMouseP.x, -dMouseP.y));
	      P->x = Maximum(10.0f, P->x);
	      P->y = Maximum(10.0f, P->y);
	    } break;
	    
	  case(DebugInteraction_Move):
	    {
	      *P = V2Add(*P, V2(dMouseP.x, dMouseP.y));
	    } break;

	  default: {}
	  }	

	bool32 AltUI = Input->MouseButtons[PlatformMouseButton_Right].EndedDown;
	
	for(u32 TransitionIndex = Input->MouseButtons[PlatformMouseButton_Left].HalfTransitionCount;
	    TransitionIndex > 1;
	    --TransitionIndex)
	  {
	    DEBUGEndInteract(DebugState, Input, MouseP);
	    DEBUGBeginInteract(DebugState, Input, MouseP, AltUI);
	  }

	if(!Input->MouseButtons[PlatformMouseButton_Left].EndedDown)
	  {
	    DEBUGEndInteract(DebugState, Input, MouseP);	    
	  }
      }
    else
      {
	DebugState->HotInteraction = DebugState->NextHotInteraction;
		
	bool32 AltUI = Input->MouseButtons[PlatformMouseButton_Right].EndedDown;
	
	for(u32 TransitionIndex = Input->MouseButtons[PlatformMouseButton_Left].HalfTransitionCount;
	    TransitionIndex > 1;
	    --TransitionIndex)
	  {
	    DEBUGBeginInteract(DebugState, Input, MouseP, AltUI);
	    DEBUGEndInteract(DebugState, Input, MouseP);
	  }

	if(Input->MouseButtons[PlatformMouseButton_Left].EndedDown)
	  {
	    DEBUGBeginInteract(DebugState, Input, MouseP, AltUI);
	  }
      }
	
  DebugState->LastMouseP = MouseP;
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

internal inline debug_record*
GetRecordFrom(open_debug_block* Block)
{
  debug_record* Result = Block ?  Block->Source : 0;
  return(Result);
}
	   
internal void
CollateDebugRecords(debug_state* DebugState, u32 InvalidEventArrayIndex)
{
  for(;;
      ++DebugState->CollationArrayIndex)
    {
      if(DebugState->CollationArrayIndex == MAX_DEBUG_EVENT_ARRAY_COUNT)
	{
	  DebugState->CollationArrayIndex = 0;
	}

      u32 EventArrayIndex = DebugState->CollationArrayIndex;
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
	      if(DebugState->CollationFrame)
		{
		  DebugState->CollationFrame->EndClock = Event->Clock;
		  DebugState->CollationFrame->WallSecondsElapsed = Event->SecondsElapsed;
		  ++DebugState->FrameCount;

		  r32 ClockRange = (r32)(DebugState->CollationFrame->EndClock - DebugState->CollationFrame->BeginClock);
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
	      
	      DebugState->CollationFrame = DebugState->Frames + DebugState->FrameCount;
	      DebugState->CollationFrame->BeginClock = Event->Clock;
	      DebugState->CollationFrame->EndClock = 0;
	      DebugState->CollationFrame->RegionCount = 0;
	      DebugState->CollationFrame->Regions = PushArray(&DebugState->CollateArena, MAX_REGIONS_PER_FRAME, debug_frame_region);
	      DebugState->CollationFrame->WallSecondsElapsed = 0.0f;

	    }
	  else if(DebugState->CollationFrame)
	    {
	      u32 FrameIndex = DebugState->FrameCount - 1;
	      debug_thread* Thread = GetDebugThread(DebugState, Event->TC.ThreadID);
	      u64 RelativeClock = Event->Clock - DebugState->CollationFrame->BeginClock;

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
		  DebugBlock->Source = Source;
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
			      if(GetRecordFrom(MatchingBlock->Parent) == DebugState->ScopeToRecord)
				{
				  r32 MinT = (r32)(OpeningEvent->Clock - DebugState->CollationFrame->BeginClock);
				  r32 MaxT = (r32)(Event->Clock - DebugState->CollationFrame->BeginClock);
				  r32 ThresholdT = 0.01f;
				  if((MaxT - MinT) > ThresholdT)
				    {
				      debug_frame_region* Region = AddRegion(DebugState, DebugState->CollationFrame);
				      Region->Record = Source;
				      Region->CycleCount = (Event->Clock - OpeningEvent->Clock);
				      Region->LaneIndex = (u16)Thread->LaneIndex;
				      Region->MinT = MinT;
				      Region->MaxT = MaxT;
				      Region->ColourIndex = OpeningEvent->DebugRecordIndex;
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
}

internal void
RestartCollation(debug_state* DebugState, u32 InvalidEventArrayIndex)
{
  EndTemporaryMemory(DebugState->CollateTemp);
  DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->CollateArena);

  DebugState->FirstThread = 0;
  DebugState->FirstFreeBlock = 0;

  DebugState->Frames = PushArray(&DebugState->CollateArena, MAX_DEBUG_EVENT_ARRAY_COUNT*4, debug_frame);
  DebugState->FrameBarLaneCount = 0;
  DebugState->FrameCount = 0;
  DebugState->FrameBarScale = 1.0f / 60000000.0f;	  

  DebugState->CollationArrayIndex = InvalidEventArrayIndex + 1;
  DebugState->CollationFrame = 0;
}

internal void
RefreshCollation(debug_state* DebugState)
{
  RestartCollation(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
  CollateDebugRecords(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
}

internal void
DEBUGStart(debug_state* DebugState, assets* Assets, u32 Width, u32 Height)
{
  BEGIN_TIMED_FUNCTION(1);
  if(!DebugState->Initialized)
    {
      DebugState->HighPriorityQueue = DebugGlobalMemory->HighPriorityQueue;
      DebugState->TreeSentinel.Next = &DebugState->TreeSentinel;
      DebugState->TreeSentinel.Prev = &DebugState->TreeSentinel; 
      DebugState->TreeSentinel.Group = 0;
      InitializeArena(&DebugState->DebugArena, DebugGlobalMemory->DebugStorageSize - sizeof(debug_state), DebugState + 1);
	  
      debug_variable_definition_context Context = {};
      Context.State = DebugState;
      Context.Arena = &DebugState->DebugArena;
      Context.GroupStack[0] = 0;

      DebugState->RootGroup = DEBUGBeginVariableGroup(&Context, "Root");
      DEBUGBeginVariableGroup(&Context, "Debugging");	  
	  
      DEBUGCreateVariables(&Context);
      DEBUGBeginVariableGroup(&Context, "Profile");
      DEBUGBeginVariableGroup(&Context, "By Thread");
      DEBUGAddVariableWithContext(&Context, DebugVariable_CounterThreadList, "");
      DEBUGEndVariableGroup(&Context);
      DEBUGBeginVariableGroup(&Context, "By Function");
      DEBUGAddVariableWithContext(&Context, DebugVariable_CounterThreadList, "");
      DEBUGEndVariableGroup(&Context);
      DEBUGEndVariableGroup(&Context);
	    
      asset_vector MatchVector  = {};
      MatchVector.E[Tag_FacingDirection] = 0.0f;
      asset_vector WeightVector = {};
      WeightVector.E[Tag_FacingDirection] = 1.0f;
      bitmap_id ID = GetBestMatchBitmapFrom(Assets, Asset_Head,
					    &MatchVector,
					    &WeightVector);
      DEBUGAddBitmapVariable(&Context, "TestBitmap", ID);

      DEBUGEndVariableGroup(&Context);
      Assert(Context.GroupDepth == 0);
	  
      DebugState->RenderGroup = AllocateRenderGroup(Assets,
						    &DebugState->DebugArena,
						    Megabytes(16), false);
	  
      DebugState->Paused = false;
      DebugState->ScopeToRecord = 0;

      DebugState->Initialized = true;

      SubArena(&DebugState->CollateArena, &DebugState->DebugArena, Megabytes(32), 4);
      DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->CollateArena);

      RestartCollation(DebugState, 0);

      AddTree(DebugState, DebugState->RootGroup, V2(-0.5f*Width , 0.5f* Height));
    }

  BeginRender(DebugState->RenderGroup);
  DebugState->DebugFont = PushFont(DebugState->RenderGroup, DebugState->FontID);
  DebugState->DebugFontInfo = GetFontInfo(DebugState->RenderGroup->Assets, DebugState->FontID);
      
  DebugState->GlobalWidth  = (r32)Width;
  DebugState->GlobalHeight = (r32)Height;
  
  asset_vector MatchVector  = {};
  asset_vector WeightVector = {};
  MatchVector.E[Tag_FontType] = (r32)FontType_Debug;
  WeightVector.E[Tag_FontType] = 1.0f;
  DebugState->FontID  = GetBestMatchFontFrom(Assets,
					     Asset_Font,
					     &MatchVector, &WeightVector);
  
  DebugState->FontScale = 1.0f;
  Orthographic(DebugState->RenderGroup, Width, Height, 1.0f);
  DebugState->LeftEdge = -0.5f * (r32)Width;
  DebugState->RightEdge = -0.5f * (r32)Width;

  DebugState->AtY = 0.5f * (r32)Height;

  END_TIMED_FUNCTION();
}

internal void
DEBUGEnd(debug_state* DebugState, input* Input, loaded_bitmap* DrawBuffer)
{
  BEGIN_TIMED_FUNCTION(1);
  render_group* RenderGroup = DebugState->RenderGroup;

  ZeroStruct(DebugState->NextHotInteraction);
  debug_record* HotRecord = 0;
      
  v2 MouseP = V2(Input->MouseX, Input->MouseY); 
  DEBUGDrawMainMenu(DebugState, RenderGroup, MouseP);
  DEBUGInteract(DebugState, Input, MouseP);
      
  if(DebugState->Compiling)
    {
      debug_process_state State = Platform.DEBUGGetProcessState(DebugState->Compiler);
      if(State.IsRunning)
	{
	  DEBUGTextLine("COMPILING");
	}
      else
	{
	  DebugState->Compiling = false;
	}
    }
      
  loaded_font* Font = DebugState->DebugFont;
  hha_font* Info = DebugState->DebugFontInfo;
  if(Font)
    {
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
    }
      
  if(WasPressed(Input->MouseButtons[PlatformMouseButton_Left]))
    {
      if(HotRecord)
	{
	  DebugState->ScopeToRecord = HotRecord;
	}
      else
	{
	  DebugState->ScopeToRecord = 0;	
	}
      RefreshCollation(DebugState);
    }

  TiledRenderGroupToOutput(DebugState->HighPriorityQueue,
			   DebugState->RenderGroup, DrawBuffer);
  EndRender(DebugState->RenderGroup);

  END_TIMED_FUNCTION();
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
  
  debug_state* DebugState = (debug_state*) Memory->DebugStorage;
  if(DebugState)
    {
      assets* Assets = DEBUGGetGameAssets(Memory);

      DEBUGStart(DebugState, Assets, Buffer->Width, Buffer->Height);
      
      if(Memory->ExecutableReloaded)
	{
	  RestartCollation(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
	}
      
      if(!DebugState->Paused)
	{
	  if(DebugState->FrameCount >= MAX_DEBUG_EVENT_ARRAY_COUNT*4)
	    {
	      RestartCollation(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
	    }
	  CollateDebugRecords(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
	}
        
      loaded_bitmap DrawBuffer = {};
      DrawBuffer.Width  = Buffer->Width;
      DrawBuffer.Height = Buffer->Height;
      DrawBuffer.Pitch  = Buffer->Pitch;
      DrawBuffer.Memory = Buffer->Memory;
      
      DEBUGEnd(DebugState, Input, &DrawBuffer);
    }
  
  return(GlobalDebugTable);
}

