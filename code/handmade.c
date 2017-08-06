#include "handmade.h"
#include "handmade_tile.c"
#include "handmade_random.h"

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h"

internal void
ClearBitmap(loaded_bitmap* Bitmap)
{
  if(Bitmap->Memory)
    {
      u32 TotalBitmapSize = Bitmap->Width*Bitmap->Height*4;
      ZeroSize(TotalBitmapSize, Bitmap->Memory);
    }
}

internal loaded_bitmap*
MakeEmptyBitmap(memory_arena* Arena, s32 Width, s32 Height, bool32 ClearToZero)
{
  loaded_bitmap* Result = PushStruct(Arena, loaded_bitmap);
  Result->Width  = Width;
  Result->Height = Height;
  Result->Pitch  = Width*4;

  s32 TotalBitmapSize = Width*Height*4;
  Result->Memory      = PushSize(Arena, TotalBitmapSize);

  if(ClearToZero)
  {
    ClearBitmap(Result);
  }
  
  return(Result); 
}

internal glyph_bitmap*
MakeEmptyGlyphBitmap(memory_arena* GlyphArena, memory_arena* BitmapArena,
		     s32 Width, s32 Height, s32 XOffset, s32 YOffset,
		     bool32 ClearToZero)
{
  glyph_bitmap* Result = PushStruct(GlyphArena, glyph_bitmap);
  Result->XOffset = XOffset;
  Result->YOffset = YOffset;    
    
  Result->Bitmap = PushStruct(BitmapArena, loaded_bitmap);
  Result->Bitmap->Width  = Width;
  Result->Bitmap->Height = Height;
  Result->Bitmap->Pitch  = Width*4;

  s32 TotalBitmapSize    = Width*Height*4;
  Result->Bitmap->Memory = PushSize(BitmapArena, TotalBitmapSize);

  if(ClearToZero)
  {
    ClearBitmap(Result->Bitmap);
  }
  
  return(Result); 
}

internal void
InitFont(thread_context* Thread, font* FontState, memory_arena* BitmapArena, memory *Memory)
{
  loaded_file FontTTF = Memory->DEBUGPlatformReadEntireFile(Thread, "fonts/LiberationMono-Regular.ttf");

  stbtt_fontinfo Font;
  stbtt_InitFont(&Font, FontTTF.Contents, stbtt_GetFontOffsetForIndex(FontTTF.Contents, 0));

  r32 ScaleForPixelHeight = stbtt_ScaleForPixelHeight(&Font, 30);
  int    Ascent, Descent, LineGap;
  stbtt_GetFontVMetrics(&Font,
			&Ascent, &Descent, &LineGap);
  font_metrics* FontMetrics = &FontState->Metrics;
  FontMetrics->ScaleForPixelHeight = ScaleForPixelHeight;
  FontMetrics->Ascent  = ScaleForPixelHeight*Ascent;
  FontMetrics->Descent = ScaleForPixelHeight*Descent;
  FontMetrics->LineGap = ScaleForPixelHeight*LineGap;
  FontMetrics->VerticalAdvance = FontMetrics->Ascent -
    FontMetrics->Descent + FontMetrics->LineGap;
  for(char codePoint = '!'; codePoint <= 'z'; ++codePoint)
    {
      s32 BitmapWidth, BitmapHeight,
	  XOffset, YOffset;
      u8* STBCharacterBitmap = stbtt_GetCodepointBitmap(&Font, 0, ScaleForPixelHeight,
							codePoint, &BitmapWidth, &BitmapHeight, &XOffset, &YOffset);
      glyph_bitmap* Glyph = MakeEmptyGlyphBitmap(&FontState->GlyphArena, BitmapArena, BitmapWidth, BitmapHeight, XOffset, YOffset, true);

      u8* SourceRow = STBCharacterBitmap;
      u8* DestRow   = Glyph->Bitmap->Memory;
      for(int Y = 0;
	  Y < BitmapHeight;
	  ++Y)
	{
	  u8*  Source = SourceRow;
	  u32* Dest   = (u32*)DestRow;
	  for(int X = 0;
	      X < BitmapWidth;
	      ++X)
	    {
	      u8 Alpha = *Source;
	      *Dest = ((Alpha << 24)|
		       (Alpha << 16)|
		       (Alpha << 8) |
		       (Alpha << 0)); 	      
	      ++Dest;
	      ++Source;
	    }
	  SourceRow += BitmapWidth;
	  DestRow   += Glyph->Bitmap->Pitch;
	}

      stbtt_FreeBitmap(STBCharacterBitmap, 0);
  }
}

internal void
DrawBitmap(offscreen_buffer *Buffer, loaded_bitmap *Bitmap,
	   s32 X, s32 Y)
{
  s32 MinX = X;
  s32 MinY = Y;
  s32 MaxX = MinX + Bitmap->Width;
  s32 MaxY = MinY + Bitmap->Height;

  if(MinX < 0)
    {
      MinX = 0;
    }
  if(MaxX > Buffer->Width)
    {
      MaxX = Buffer->Width;
    }
  if(MinY < 0)
    {
      MinY = 0;
    }
  if(MaxY > Buffer->Height)
    {
      MaxY = Buffer->Height;
    }

  u8* SourceRow = Bitmap->Memory;
  u8* DestRow   = (u8*)Buffer->BitmapMemory + MinX*4 + MinY*Buffer->Pitch;
  for(int Y = MinY;
      Y < MaxY;
      ++Y)
    {
      u32 *Source = (u32*) SourceRow;
      u32 *Dest   = (u32*) DestRow;
      for(int X = MinX;
	  X < MaxX;
	  ++X)
	{
	  r32 Alpha = ((*Source >> 24) & 0xff) / 255.0f;
	  r32 sChannel1 = ((*Source >> 16) & 0xff);
	  r32 sChannel2 = ((*Source >> 8) & 0xff);
	  r32 sChannel3 = ((*Source >> 0) & 0xff);

	  r32 dChannel1 = ((*Dest >> 16) & 0xff);
	  r32 dChannel2 = ((*Dest >> 8) & 0xff);
	  r32 dChannel3 = ((*Dest >> 0) & 0xff);

	  r32 Channel1 = (1.0f - Alpha)*dChannel1 + Alpha*sChannel1;
	  r32 Channel2 = (1.0f - Alpha)*dChannel2 + Alpha*sChannel2;
	  r32 Channel3 = (1.0f - Alpha)*dChannel3 + Alpha*sChannel3;

	  *Dest = (((u32)(Channel1 + 0.5f) << 16) |
		   ((u32)(Channel2 + 0.5f) << 8)  |
		   ((u32)(Channel3 + 0.5f) << 0));
	  
	  ++Dest;
	  ++Source;
	}
      SourceRow += Bitmap->Pitch;
      DestRow   += Buffer->Pitch;
    }
}

internal void
DrawRectangle(offscreen_buffer *Buffer,
	      r32 RealMinX, r32 RealMinY,
	      r32 RealMaxX, r32 RealMaxY,
	      r32 R, r32 G, r32 B)
{
  s32 MinX = RoundReal32ToInt32(RealMinX);
  s32 MinY = RoundReal32ToInt32(RealMinY);
  s32 MaxX = RoundReal32ToInt32(RealMaxX);
  s32 MaxY = RoundReal32ToInt32(RealMaxY);
  if(MinX < 0)
    {
      MinX = 0;
    }
  if(MaxX > Buffer->Width)
    {
      MaxX = Buffer->Width;
    }
  if(MinY < 0)
    {
      MinY = 0;
    }
  if(MaxY > Buffer->Height)
    {
      MaxY = Buffer->Height;
    }

  u32 Color = (u32) ((0xFF << 24) |
		     (RoundReal32ToUInt32(R * 255.0f) << 16) |
		     (RoundReal32ToUInt32(G * 255.0f) << 8)  |
		     (RoundReal32ToUInt32(B * 255.0f)));
  
  u8 *Row = Buffer->BitmapMemory
    + Buffer->Pitch * MinY
    + Buffer->BytesPerPixel * MinX;
  for(int Y = MinY; Y < MaxY; ++Y)
    {
      u32 *Pixel = (u32*) Row;
      for(int X = MinX; X < MaxX; ++X)
	{
	  *Pixel++ = Color;
	}
      Row += Buffer->Pitch; 
    }
}

extern UPDATE_AND_RENDER(UpdateAndRender)
{
  r32 PlayerHeight = 1.4f;
  r32 PlayerWidth  = 0.75f * PlayerHeight;
  
  state* State = (state*) Memory->PermanentStorage;
  if(!Memory->isInitialized)
    {            
      //NOTE We will do this much differently once we have a system
      //     for the assets like fonts and bitmaps.
      /*InitializeArena(&State->BitmapArena, Megabytes(1),
		      (u8*)Memory->PermanentStorage + sizeof(state));
      InitializeArena(&State->Font.GlyphArena, Memory->PermanentStorageSize - sizeof(state),
		      (u8*)Memory->PermanentStorage + sizeof(state) + Megabytes(1));
      */
      InitializeArena(&State->WorldArena, Memory->PermanentStorageSize - sizeof(state),
		      (u8*)Memory->PermanentStorage + sizeof(state));

      State->PlayerP.AbsTileX = 1;
      State->PlayerP.AbsTileY = 3;
      State->PlayerP.TileRelX = 5.0f;
      State->PlayerP.TileRelY = 5.0f;
      
      State->World   = PushStruct(&State->WorldArena, world);
      world *World   = State->World;
      World->TileMap = PushStruct(&State->WorldArena, tile_map);
      
      tile_map *TileMap = World->TileMap;

      TileMap->ChunkShift = 4;
      TileMap->ChunkMask  = (1 << TileMap->ChunkShift) - 1;  
      TileMap->ChunkDim   = (1 << TileMap->ChunkShift);

      TileMap->TileChunkCountX = 128;
      TileMap->TileChunkCountY = 128;
      TileMap->TileChunkCountZ = 2;

      TileMap->TileChunks = PushArray(&State->WorldArena,
				      TileMap->TileChunkCountX *
				      TileMap->TileChunkCountY *
				      TileMap->TileChunkCountZ,
				      tile_chunk);
	
      TileMap->TileSideInMeters = 1.4f;

      u32 RandomNumberIndex = 0;
	        
      u32 TilesPerWidth = 17;
      u32 TilesPerHeight = 9;
      u32 ScreenX = 0;
      u32 ScreenY = 0;
      u32 AbsTileZ = 0;
      
      bool32 DoorLeft   = false;
      bool32 DoorRight  = false;
      bool32 DoorTop    = false;
      bool32 DoorBottom = false;
      bool32 DoorUp     = false;
      bool32 DoorDown   = false;
	    
      for(s32 ScreenIndex = 0;
	  ScreenIndex < 100;
	  ++ScreenIndex)
	{
	  Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
	  u32 RandomChoice;
	  if(DoorUp || DoorDown)
	    {
	      RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
	    }
	  else
	    {
	      RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
	    }
	  
	  if(RandomChoice == 2)
	    {
	      if(AbsTileZ == 0)
		{
		  DoorUp = true;
		}
	      else
		{
		  DoorDown = true;
		}
	    }
	  else if (RandomChoice == 1)
	    {
	      DoorRight = true;
	    }
	  else
	    {
	      DoorTop = true;
	    }
	  
	  for(u32 TileY = 0;
	      TileY < TilesPerHeight;
	      ++TileY)
	    {
	      for(u32 TileX = 0;
		  TileX < TilesPerWidth;
		  ++TileX)
		{
		  u32 AbsTileX = ScreenX * TilesPerWidth  + TileX;
		  u32 AbsTileY = ScreenY * TilesPerHeight + TileY;

		  u32 TileValue = 1;
		  if((TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight/2))))
		    { 
		      TileValue = 2;
		    }		  
		  if((TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != (TilesPerHeight/2))))
		    {
		      TileValue = 2;
		    }
		  if((TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth/2))))
		    {
		      TileValue = 2;
		    }
		  if((TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth/2))))
		    {
		      TileValue = 2;
		    }
		  if((TileX == 10) && (TileY == 6))
		    {
		      if(DoorUp)
			{
			  TileValue = 3;
			}
		      if(DoorDown)
			{
			  TileValue = 4;
			}
		    }
		  
		  SetTileValue(&State->WorldArena, TileMap,
			       AbsTileX, AbsTileY, AbsTileZ,
			       TileValue);
		}
	    }

	  DoorLeft = DoorRight;
	  DoorBottom = DoorTop;

	  if(DoorUp)
	    {
	      DoorUp   = false;
	      DoorDown = true;
	    }
	  else if(DoorDown)
	    {
	      DoorUp   = true;
	      DoorDown = false;
	    }
	  else
	    {
	      DoorUp   = false;
	      DoorDown = false;
	    }
	  
	  DoorRight = false;
	  DoorTop   = false;
	  	  
	  if(RandomChoice == 2)
	    {
	      if(AbsTileZ == 0)
		{
		  AbsTileZ = 1;
		}
	      else
		{
		  AbsTileZ = 0;
		}
	    }
	  else if(RandomChoice == 1)
	    {
	      ScreenX += 1;
	    }
	  else
	    {
	      ScreenY += 1;
	    }
	}

      Memory->isInitialized = true;
    }

  world *World      = State->World;
  tile_map *TileMap = World->TileMap;

  s32 TileSideInPixels = 60;
  r32 MetersToPixels = (r32)TileSideInPixels / (r32)TileMap->TileSideInMeters;
  
  for(int ControllerIndex = 0;
      ControllerIndex < ArrayCount(Input->Controllers);
      ++ControllerIndex)
    {
      controller_input *Controller = GetController(Input, ControllerIndex);
      if(Controller->IsAnalog)
	{
	}
      else
	{
	  r32 dPlayerX = 0.0f;
	  r32 dPlayerY = 0.0f;
	  if(Controller->MoveUp.EndedDown)
	    {
	      dPlayerY = 1.0f;
	    }
	  if(Controller->MoveDown.EndedDown)
	    {
	      dPlayerY = -1.0f;
	    }
	  if(Controller->MoveLeft.EndedDown)
	    {
	      dPlayerX = -1.0f;
	    }
	  if(Controller->MoveRight.EndedDown)
	    {
	      dPlayerX = 1.0f;
	    }
	  r32 PlayerSpeed = 2.0f;
	  if(Controller->ActionUp.EndedDown)
	    {
	      PlayerSpeed *= 10.0f;
	    }
	  dPlayerX *= PlayerSpeed;
	  dPlayerY *= PlayerSpeed;

	  tile_map_position NewPlayerP = State->PlayerP;
	  NewPlayerP.TileRelX += Input->dtForFrame*dPlayerX;
	  NewPlayerP.TileRelY += Input->dtForFrame*dPlayerY;
	  NewPlayerP = RecanonicalizePosition(TileMap, NewPlayerP);

	  tile_map_position PlayerLeft = NewPlayerP;
	  PlayerLeft.TileRelX -= 0.5f * PlayerWidth;
	  PlayerLeft = RecanonicalizePosition(TileMap, PlayerLeft);
 
	  tile_map_position PlayerRight = NewPlayerP;
	  PlayerRight.TileRelX += 0.5f * PlayerWidth;
	  PlayerRight = RecanonicalizePosition(TileMap, PlayerRight);
 
	  if(IsTileMapPointEmpty(TileMap, NewPlayerP) &&
	     IsTileMapPointEmpty(TileMap, PlayerLeft) &&
	     IsTileMapPointEmpty(TileMap, PlayerRight))
	    {
	      State->PlayerP = NewPlayerP;
	    }
	}
    }
  
  DrawRectangle(Buffer,
		0.0f, 0.0f,
		Buffer->Width, Buffer->Height,
		1.0f, 0.0f, 1.0f);

  r32 ScreenCenterX = 0.5f * Buffer->Width;
  r32 ScreenCenterY = 0.5f * Buffer->Height;
  for(s32 RelRow = -10;
      RelRow < 10;
      ++RelRow)
    {
      for(s32 RelColumn = -20;
	  RelColumn < 20;
	  ++RelColumn)
	{
	  u32 Column = State->PlayerP.AbsTileX + RelColumn;
	  u32 Row    = State->PlayerP.AbsTileY + RelRow;
	  u32 TileID = GetTileValue(TileMap, Column, Row, State->PlayerP.AbsTileZ);
	  
	  if(TileID > 0)
	    {
	      r32 Grey = 0.5f;
	      if(TileID == 2)
		{
		  Grey = 1.0f;
		}
	      if(TileID > 2)
		{
		  Grey = 0.25f;
		}
	     
	      if((Row == State->PlayerP.AbsTileY) &&
		 (Column == State->PlayerP.AbsTileX))
		{
		  Grey = 0.0f;
		}
	      r32 CenterX = ScreenCenterX - MetersToPixels*State->PlayerP.TileRelX + ((r32)RelColumn * TileSideInPixels);
	      r32 CenterY = ScreenCenterY + MetersToPixels*State->PlayerP.TileRelY - ((r32)RelRow * TileSideInPixels);
	      r32 MinX = CenterX - 0.5f * TileSideInPixels;
	      r32 MinY = CenterY - 0.5f * TileSideInPixels;
	      r32 MaxX = CenterX + 0.5f * TileSideInPixels;
	      r32 MaxY = CenterY + 0.5f * TileSideInPixels;
	      DrawRectangle(Buffer,
			    MinX, MinY,
			    MaxX, MaxY,
			    Grey, Grey, Grey);
	    }
	}
    }
  r32 PlayerR = 1.0f;
  r32 PlayerG = 1.0f;
  r32 PlayerB = 0.0f;
  r32 PlayerLeft = ScreenCenterX - 0.5f * MetersToPixels*PlayerWidth;
  r32 PlayerTop  = ScreenCenterY - MetersToPixels*PlayerHeight;

  DrawRectangle(Buffer,
		PlayerLeft, PlayerTop,
		PlayerLeft + MetersToPixels*PlayerWidth,
		PlayerTop  + MetersToPixels*PlayerHeight,
		PlayerR, PlayerG, PlayerB);
}

/*

internal void
RenderWeirdGradient(offscreen_buffer* Buffer, memory_arena* Arena,
		    u32 BlueOffset, u32 GreenOffset)
{
  u8* Row = (u8*)Buffer->BitmapMemory;
  for(int Y = 0;
      Y < Buffer->Height;
      ++Y)
    {
      u32 *Pixel = (u32*)Row;
      for (int X = 0; 
	   X < Buffer->Width;
	   ++X)
	{
	  u8 Blue  = X + BlueOffset;
	  u8 Green = Y + GreenOffset;
	  *Pixel++ = ((Green << 8) | Blue);
	}
      Row += Buffer->Pitch;
    }
}

internal void
DrawGlyph(offscreen_buffer* Buffer, glyph_bitmap* Glyph,
	   s32 X, s32 Y)
{
  DrawBitmap(Buffer, Glyph->Bitmap,
	     X + Glyph->XOffset,
	     Y + Glyph->YOffset);
}

internal void
DebugRenderTextLine()
{
  
}

#if 0
internal bool32
Button(s32 ID, char* Label, v2 Position, guiContext* GUIContext)
{
  if(GUIContext->ActiveItem == ID)
    {
      inputState currentInputState = GUIContext->InputState;
      if(MouseWentUp)
	{
	  if(GUIContext->HotItem == ID)
	    {
	      return true;
	    }
	  GUIContext->ActiveItem = 0;
	}
    }
  else
    {
      if(MouseWentDown)
	{
	  GUIContext->ActiveItem = ID;
	}
    }
  
  if(Inside)
    {
      GUIContext->HotItem = ID;
    }
  
}

internal void
prepareUIFrame()
{
  guiContext GUIContext;
  InitGui(&GUIContext);
  
  if(Button(ID, Label, Position, &GUIContext))
    {
      //TODO
    }
  OutputList(Position, char** Strings, s32 numStrings);
}
#endif

*/
