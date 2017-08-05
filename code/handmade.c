#include "handmade.h"
#include "handmade_intrinsics.h"

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h"

#define PushStruct(Arena, type) (type*)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type*)PustSize_(Arena, sizeof(type)*Count)
#define PushSize(Arena, Size) PushSize_(Arena, Size)
internal void*
PushSize_(memory_arena* Arena, memory_index Size)
{
  Assert((Arena->Used + Size) <= Arena->Size); 
  void* Result = Arena->Base + Arena->Used;
  Arena->Used += Size;
  return(Result);
}

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

internal void
InitializeArena(memory_arena* Arena, memory_index Size, u8* StorageBase)
{
  Arena->Size = Size;
  Arena->Base = StorageBase;
  Arena->Used = 0;
}

inline internal tile_chunk*
GetTileChunk(world *World, s32 TileChunkX, s32 TileChunkY)
{
  tile_chunk *TileChunk = 0;
  if((TileChunkX >= 0) && (TileChunkX < World->TileChunkCountX) &&
     (TileChunkY >= 0) && (TileChunkY < World->TileChunkCountY))
    {
      TileChunk = &World->TileChunks[TileChunkY*World->TileChunkCountX + TileChunkX];  
    }
  
  return(TileChunk);
}

internal inline void
RecanonicalizeCoord(world *World, u32 *Tile, r32 *TileRel)
{
  //NOTE: Apparently the world is assumed to be toroidal
  s32 Offset = FloorReal32ToInt32(*TileRel / World->TileSideInMeters);
  *Tile += Offset;
  *TileRel -= Offset*World->TileSideInMeters;
  
  Assert(*TileRel >= 0);
  Assert(*TileRel < World->TileSideInMeters);
}

internal inline world_position
RecanonicalizePosition(world *World, world_position Position)
{
  world_position Result = Position;

  RecanonicalizeCoord(World, &Result.AbsTileX, &Result.TileRelX);
  RecanonicalizeCoord(World, &Result.AbsTileY, &Result.TileRelY);

  return(Result);
}

internal inline tile_chunk_position
GetChunkPositionFor(world *World, u32 AbsTileX, u32 AbsTileY)
{
  tile_chunk_position Result = {};
  
  Result.TileChunkX = AbsTileX >> World->ChunkShift;
  Result.TileChunkY = AbsTileY >> World->ChunkShift;
  Result.RelTileX = AbsTileX & World->ChunkMask;
  Result.RelTileY = AbsTileY & World->ChunkMask;

  return(Result);
}

inline internal u32
GetTileValueUnchecked(world *World, tile_chunk *TileChunk, u32 TileX, u32 TileY)
{
  Assert(TileChunk);
  Assert((TileX < World->ChunkDim) &&
	 (TileY < World->ChunkDim))
  u32 TileChunkValue = TileChunk->Tiles[TileY*World->ChunkDim + TileX];  
  return(TileChunkValue);
}

internal u32
_GetTileValue(world *World, tile_chunk *TileChunk, u32 TestTileX, u32 TestTileY)
{
  u32 TileChunkValue = 0;
  if(TileChunk)
    {      
      TileChunkValue = GetTileValueUnchecked(World, TileChunk, TestTileX, TestTileY);      
    }
  return(TileChunkValue);
}

internal u32
GetTileValue(world *World, u32 AbsTileX, u32 AbsTileY)
{
  tile_chunk_position ChunkPosition = GetChunkPositionFor(World, AbsTileX, AbsTileY);
  tile_chunk *TileChunk = GetTileChunk(World, ChunkPosition.TileChunkX, ChunkPosition.TileChunkY);
  u32 TileChunkValue = _GetTileValue(World, TileChunk, ChunkPosition.RelTileX, ChunkPosition.RelTileY);
    
  return(TileChunkValue);
}

internal bool32
IsWorldPointEmpty(world *World, world_position CanPos)
{
  u32 TileChunkValue = GetTileValue(World, CanPos.AbsTileX, CanPos.AbsTileY);
  bool32 IsEmpty = (TileChunkValue == 0);
  
  return(IsEmpty);
}

extern UPDATE_AND_RENDER(UpdateAndRender)
{
#define TILEMAP_COUNT_Y 256
#define TILEMAP_COUNT_X 256
  u32 TempTiles[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] =
    {
      {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1},
      {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  1},
      {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 1, 1,  0, 0, 0, 0,  1},
      {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1, 1, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 1,  1, 1, 0, 0,  1},
      {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
      {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1, 1, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
      {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1, 1, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
      {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
      {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1,  1},
      {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1,  1},
      {1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  1},
      {1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 1, 1,  0, 0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 1, 1,  0, 0, 0, 0,  1},
      {1, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 1,  1, 1, 0, 0, 1, 1, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 1,  1, 1, 0, 0,  1},
      {1, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
      {1, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1, 1, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
      {1, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1, 1, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
      {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1, 1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
      {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1},
    };

  world World;
  World.ChunkShift = 8;
  World.ChunkMask  = (1 << World.ChunkShift) - 1;  
  World.ChunkDim = 256;
  
  World.TileSideInMeters = 1.4f;
  World.TileSideInPixels = 60;
  World.MetersToPixels   = (r32)World.TileSideInPixels / (r32)World.TileSideInMeters;

  tile_chunk TileChunk;
  TileChunk.Tiles = (u32*)TempTiles;
  World.TileChunks = &TileChunk;

  World.TileChunkCountX = 1;
  World.TileChunkCountY = 1;

  r32 PlayerHeight = 1.4f;
  r32 PlayerWidth  = 0.75f * PlayerHeight;

  state* State = (state*) Memory->PermanentStorage;
  if(!Memory->isInitialized)
    {
      State->PlayerP.AbsTileX = 3;
      State->PlayerP.AbsTileY = 3;
      State->PlayerP.TileRelX = 5.0f;
      State->PlayerP.TileRelY = 5.0f;
      
      //NOTE We will do this much differently once we have a system
      //     for the assets like fonts and bitmaps.
      InitializeArena(&State->BitmapArena, Megabytes(1),
		      (u8*)Memory->PermanentStorage + sizeof(state));
      InitializeArena(&State->Font.GlyphArena, Memory->PermanentStorageSize - sizeof(state),
		      (u8*)Memory->PermanentStorage + sizeof(state) + Megabytes(1));

      Memory->isInitialized = true;
    }
  
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
	  dPlayerX *= 2.0f;
	  dPlayerY *= 2.0f;

	  world_position NewPlayerP = State->PlayerP;
	  NewPlayerP.TileRelX += Input->dtForFrame*dPlayerX;
	  NewPlayerP.TileRelY += Input->dtForFrame*dPlayerY;
	  NewPlayerP = RecanonicalizePosition(&World, NewPlayerP);

	  world_position PlayerLeft = NewPlayerP;
	  PlayerLeft.TileRelX -= 0.5f * PlayerWidth;
	  PlayerLeft = RecanonicalizePosition(&World, PlayerLeft);
 
	  world_position PlayerRight = NewPlayerP;
	  PlayerRight.TileRelX += 0.5f * PlayerWidth;
	  PlayerRight = RecanonicalizePosition(&World, PlayerRight);
 
	  if(IsWorldPointEmpty(&World, NewPlayerP) &&
	     IsWorldPointEmpty(&World, PlayerLeft) &&
	     IsWorldPointEmpty(&World, PlayerRight))
	    {
	      State->PlayerP = NewPlayerP;
	    }
	}
    }
  
  DrawRectangle(Buffer,
		0.0f, 0.0f,
		Buffer->Width, Buffer->Height,
		1.0f, 0.0f, 1.0f);

  r32 CenterX = 0.5f * Buffer->Width;
  r32 CenterY = 0.5f * Buffer->Height;
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
	  u32 TileID = GetTileValue(&World, Column, Row);
	  r32 Grey = 0.5f;
	  if(TileID == 1)
	    {
	      Grey = 1.0f;
	    }

	  if((Row == State->PlayerP.AbsTileY) &&
	     (Column == State->PlayerP.AbsTileX))
	    {
	      Grey = 0.0f;
	    }
	  r32 MinX = CenterX + ((r32)RelColumn * World.TileSideInPixels);
	  r32 MinY = CenterY - ((r32)RelRow * World.TileSideInPixels);
	  r32 MaxX = MinX + World.TileSideInPixels;
	  r32 MaxY = MinY - World.TileSideInPixels;
	  DrawRectangle(Buffer,
			MinX, MaxY,
			MaxX, MinY,
			Grey, Grey, Grey);
	}
    }
  r32 PlayerR = 1.0f;
  r32 PlayerG = 1.0f;
  r32 PlayerB = 0.0f;
  r32 PlayerLeft = CenterX + World.MetersToPixels*State->PlayerP.TileRelX - 0.5f * World.MetersToPixels*PlayerWidth;
  r32 PlayerTop  = CenterY - World.MetersToPixels*State->PlayerP.TileRelY - World.MetersToPixels*PlayerHeight;

  DrawRectangle(Buffer,
		PlayerLeft, PlayerTop,
		PlayerLeft + World.MetersToPixels*PlayerWidth,
		PlayerTop  + World.MetersToPixels*PlayerHeight,
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
