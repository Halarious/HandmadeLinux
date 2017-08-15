
internal sim_entity*
_AddEntity(sim_region *SimRegion)
{
  sim_entity *Entity = 0;
  
  if(SimRegion->EntityCount < SimRegion->MaxEntityCount)
    {
      Entity = SimRegion->Entities + SimRegion->EntityCount++;

      sim_entity ZeroEntity = {};
      *Entity = ZeroEntity;
    }
  else
    {
      InvalidCodePath;
    }
  
  return(Entity);
}

internal inline v2
GetSimSpaceP(sim_region *SimRegion, low_entity *Stored)
{
  world_difference Diff = Subtract(SimRegion->World,
				   &Stored->P, &SimRegion->Origin);
  v2 Result = Diff.dXY;

  return(Result);
}


internal sim_entity*
AddEntity(sim_region *SimRegion, low_entity *Source, v2 *SimP)
{
  sim_entity* Dest = _AddEntity(SimRegion);
  if(Dest)
    {
      if(SimP)
	{
	  Dest->P = *SimP;
	}
      else
	{
	  Dest->P = GetSimSpaceP(SimRegion, Source);
	}
    }
  
  return(Dest);
}

internal sim_region*
BeginSim(memory_arena *SimArena, state* State, world* World, world_position Origin, rectangle2 Bounds)
{
  sim_region *SimRegion = PushStruct(SimArena, sim_region);

  SimRegion->MaxEntityCount = 4096;
  SimRegion->EntityCount = 0;
  SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, sim_entity);
    
  SimRegion->World = World;
  SimRegion->Origin = Origin;
  SimRegion->Bounds = Bounds;
  
  world_position MinChunkP =
    MapIntoChunkSpace(World, SimRegion->Origin, GetMinCorner(SimRegion->Bounds));
  world_position MaxChunkP =
    MapIntoChunkSpace(World, SimRegion->Origin, GetMaxCorner(SimRegion->Bounds));
  for(s32 ChunkY = MinChunkP.ChunkY;
      ChunkY <= MaxChunkP.ChunkY;
      ++ChunkY)
    {
      for(s32 ChunkX = MinChunkP.ChunkX;
	  ChunkX <= MaxChunkP.ChunkX;
	  ++ChunkX)
	{
	  world_chunk *Chunk = GetWorldChunk(World, ChunkX, ChunkY, SimRegion->Origin.ChunkZ, 0);
	  if(Chunk)
	    {
	      for(world_entity_block *Block = &Chunk->FirstBlock;
		  Block;
		  Block = Block->Next)
		{
		  for(u32 EntityIndex = 0;
		      EntityIndex < Block->EntityCount;
		      ++EntityIndex)
		    {
		      u32 LowEntityIndex = Block->LowEntityIndex[EntityIndex];
		      low_entity *Low = State->LowEntities + LowEntityIndex;

		      v2 SimSpaceP = GetSimSpaceP(SimRegion, Low);
		      if(IsInRectangle(SimRegion->Bounds, SimSpaceP))
			{
			  AddEntity(SimRegion, Low, &SimSpaceP);
			}
		      
		    }
		}
	    }
	}
    }
  
  return(SimRegion);
}

internal void
EndSim(sim_region *SimRegion, state *State)
{
  sim_entity *Entity = SimRegion->Entities;
  for(u32 EntityIndex = 0;
      EntityIndex < SimRegion->EntityCount;
      ++EntityIndex, ++Entity)
    {
      low_entity *Stored = State->LowEntities + Entity->StorageIndex;
      
      world_position NewP = MapIntoChunkSpace(SimRegion->World, SimRegion->Origin, Entity->P);
      ChangeEntityLocation(&State->WorldArena, State->World, Entity->StorageIndex,
			   Stored, &Stored->P, &NewP);
      
      entity CameraFollowingEntity = ForceEntityIntoHigh(State,
							 State->CameraFollowingEntityIndex);
      if(CameraFollowingEntity.High)
	{
	  world_position NewCameraP = State->CameraP;
      
	  State->CameraP.ChunkZ = CameraFollowingEntity.Low->P.ChunkZ;
#if 0
	  if(CameraFollowingEntity.High->P.X > (9.0f * World->TileSideInMeters))
	    {
	      NewCameraP.AbsTileX += 17;
	    }
	  if(CameraFollowingEntity.High->P.X < -(9.0f * World->TileSideInMeters))
	    {
	      NewCameraP.AbsTileX -= 17;
	    }
	  if(CameraFollowingEntity.High->P.Y > (5.0f * World->TileSideInMeters))
	    {
	      NewCameraP.AbsTileY += 9;
	    }
	  if(CameraFollowingEntity.High->P.Y < -(5.0f * World->TileSideInMeters))
	    {
	      NewCameraP.AbsTileY -= 9;
	    }
#else
	  NewCameraP = CameraFollowingEntity.Low->P;
#endif

	  SetCamera(State, NewCameraP);
	}

    }
}
