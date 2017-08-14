
#define WORLD_CHUNK_SAFE_MARGIN (INT32_MAX/64)
#define WORLD_CHUNK_UNINITIALIZED INT32_MAX
#define TILES_PER_CHUNK 16

internal inline bool32
IsCannonical(world* World, r32 TileRel)
{
  bool32 Result = ((TileRel >= -0.5f * World->ChunkSideInMeters) &&
		   (TileRel <=  0.5f * World->ChunkSideInMeters));
  return(Result);
}

internal inline bool32
IsCannonicalV(world* World, v2 Offset)
{
  bool32 Result = ((IsCannonical(World, Offset.X)) &&
		   (IsCannonical(World, Offset.Y)));
  return(Result);
}

internal inline bool32
AreInSameChunk(world *World, world_position *A, world_position *B)
{
  Assert(IsCannonicalV(World, A->Offset_));
  Assert(IsCannonicalV(World, B->Offset_));

  bool32 Result = ((A->ChunkX == B->ChunkX) &&
		   (A->ChunkY == B->ChunkY) &&
		   (A->ChunkZ == B->ChunkZ));
  return(Result);
}

inline internal world_chunk*
GetWorldChunk(world *World, s32 ChunkX, s32 ChunkY, s32 ChunkZ,
	     memory_arena *Arena)
{
  Assert(ChunkX > -WORLD_CHUNK_SAFE_MARGIN);
  Assert(ChunkY > -WORLD_CHUNK_SAFE_MARGIN);
  Assert(ChunkZ > -WORLD_CHUNK_SAFE_MARGIN);
  Assert(ChunkX < WORLD_CHUNK_SAFE_MARGIN);
  Assert(ChunkY < WORLD_CHUNK_SAFE_MARGIN);
  Assert(ChunkZ < WORLD_CHUNK_SAFE_MARGIN);
  
  u32 HashValue = 19*ChunkX + 7*ChunkY + 3*ChunkZ;
  u32 HashSlot = HashValue & (ArrayCount(World->ChunkHash) - 1);
  Assert(HashSlot < ArrayCount(World->ChunkHash));

  world_chunk *Chunk = World->ChunkHash + HashSlot;
  do
    {
      if((ChunkX == Chunk->ChunkX) &&
	 (ChunkY == Chunk->ChunkY) &&
	 (ChunkZ == Chunk->ChunkZ))
	{
	  break;
	}
      if(Arena && (Chunk->ChunkX != WORLD_CHUNK_UNINITIALIZED)  && (!Chunk->NextInHash))
	{
	  Chunk->NextInHash = PushStruct(Arena, world_chunk);
	  Chunk->NextInHash->ChunkX = WORLD_CHUNK_UNINITIALIZED;
	  Chunk = Chunk->NextInHash;
	}
            
      if(Arena && (Chunk->ChunkX == WORLD_CHUNK_UNINITIALIZED))
	{
	  Chunk->ChunkX = ChunkX;
	  Chunk->ChunkY = ChunkY;
	  Chunk->ChunkZ = ChunkZ;

	  Chunk->NextInHash = 0;
	  break;
	}

      Chunk = Chunk->NextInHash;
    } while(Chunk);

  return(Chunk);
}

internal void
InitializeWorld(world *World, r32 TileSideInMeters)
{
  World->TileSideInMeters = TileSideInMeters;
  World->ChunkSideInMeters = (r32)TILES_PER_CHUNK*TileSideInMeters;
  World->FirstFree = 0;
  
  for(u32 ChunkIndex = 0;
      ChunkIndex < ArrayCount(World->ChunkHash);
      ++ChunkIndex)
    {
      World->ChunkHash[ChunkIndex].ChunkX = WORLD_CHUNK_UNINITIALIZED;
      World->ChunkHash[ChunkIndex].FirstBlock.EntityCount = 0;
    }
}

internal inline void
RecanonicalizeCoord(world *World, s32 *Tile, r32 *TileRel)
{
  //NOTE: Apparently the world is assumed to be toroidal
  s32 Offset = RoundReal32ToInt32(*TileRel / World->ChunkSideInMeters);
  *Tile += Offset;
  *TileRel -= Offset*World->ChunkSideInMeters;

  Assert(IsCannonical(World, *TileRel));
}

internal inline world_position
MapIntoChunkSpace(world *World, world_position BasePosition, v2 Offset)
{
  world_position Result = BasePosition;

  Result.Offset_ = VAdd(Result.Offset_, Offset);
  RecanonicalizeCoord(World, &Result.ChunkX, &Result.Offset_.X);
  RecanonicalizeCoord(World, &Result.ChunkY, &Result.Offset_.Y);

  return(Result);
}

internal world_position
ChunkPositionFromTilePosition(world *World,
			      s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ)
{
  world_position Result = {};

  Result.ChunkX = (AbsTileX / TILES_PER_CHUNK);
  Result.ChunkY = (AbsTileY / TILES_PER_CHUNK);;
  Result.ChunkZ = (AbsTileZ / TILES_PER_CHUNK);;
  if(AbsTileX < 0)
    {
      --Result.ChunkX;
    }
  if(AbsTileY < 0)
    {
      --Result.ChunkY;
    }
  if(AbsTileZ < 0)
    {
      --Result.ChunkZ;
    }
  
  Result.Offset_.X = (r32)((AbsTileX - TILES_PER_CHUNK/2) - (Result.ChunkX*TILES_PER_CHUNK)) * World->TileSideInMeters;
  Result.Offset_.Y = (r32)((AbsTileY - TILES_PER_CHUNK/2) - (Result.ChunkY*TILES_PER_CHUNK)) * World->TileSideInMeters;

  Assert(IsCannonicalV(World, Result.Offset_));
  
  return(Result);
}

internal inline world_difference
Subtract(world *World, world_position *A, world_position* B)
{
  world_difference Result = {};

  v2 dTileXY = (v2){(r32)A->ChunkX - (r32)B->ChunkX,
		    (r32)A->ChunkY - (r32)B->ChunkY};
  r32 dTileZ = (r32)A->ChunkZ - (r32)B->ChunkZ;
  
  Result.dXY = VAdd(VMulS(World->ChunkSideInMeters,
			  dTileXY),
		    VSub(A->Offset_, B->Offset_));
  
  Result.dZ = dTileZ * World->ChunkSideInMeters + 0.0f;

  return(Result);
}

internal inline world_position
CenteredChunkPoint(s32 ChunkX, s32 ChunkY, s32 ChunkZ)
{
  world_position Result = {};

  Result.ChunkX = ChunkX;
  Result.ChunkY = ChunkY;
  Result.ChunkZ = ChunkZ;
  
  return(Result);
}

internal inline void
ChangeEntityLocation(memory_arena *Arena, world *World, u32 LowEntityIndex,
		     world_position *OldP, world_position *NewP)
{  
  if(OldP && AreInSameChunk(World, OldP, NewP))
    {
    }
  else
    {
      if(OldP)
	{
	  world_chunk *Chunk = GetWorldChunk(World, OldP->ChunkX, OldP->ChunkY, OldP->ChunkZ, 0);
	  Assert(Chunk);
	  if(Chunk)
	    {
	      bool32 NotFound = true;
	      world_entity_block *FirstBlock = &Chunk->FirstBlock; 
	      for(world_entity_block *Block = &Chunk->FirstBlock;
		  Block && NotFound;
		  Block = Block->Next)
		{
		  for(u32 Index = 0;
		      (Index < Block->EntityCount) && NotFound;
		      ++Index)
		    {
		      if(Block->LowEntityIndex[Index] == LowEntityIndex)
			{			  
			  FirstBlock->LowEntityIndex[Index] =
			    FirstBlock->LowEntityIndex[--FirstBlock->EntityCount];
			  if(FirstBlock->EntityCount == 0)
			    {
			      if(FirstBlock->Next)
				{
				  world_entity_block *NextBlock = FirstBlock->Next;
				  *FirstBlock = *NextBlock;

				  NextBlock->Next = World->FirstFree;
				  World->FirstFree = NextBlock;
				}
			    }
			  
			  NotFound = false;
			}
		    }
		}
	    }
	}

      world_chunk *Chunk = GetWorldChunk(World, NewP->ChunkX, NewP->ChunkY, NewP->ChunkZ, Arena);
      world_entity_block *Block = &Chunk->FirstBlock;
      if(Block->EntityCount == ArrayCount(Block->LowEntityIndex))
	{
	  world_entity_block *OldBlock = World->FirstFree;
	  if(OldBlock)
	    {
	      World->FirstFree = OldBlock->Next;
	    }
	  else
	    {
	        OldBlock = PushStruct(Arena, world_entity_block);
	    }
	  
	  *OldBlock = *Block;
	  Block->Next = OldBlock;
	  Block->EntityCount = 0;
	}

      Assert(Block->EntityCount < ArrayCount(Block->LowEntityIndex));
      Block->LowEntityIndex[Block->EntityCount++] = LowEntityIndex;
    }
}
