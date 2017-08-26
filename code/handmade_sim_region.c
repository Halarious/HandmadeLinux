
internal sim_entity_hash* 
GetHashFromStorageIndex(sim_region *SimRegion, u32 StorageIndex)
{
  Assert(StorageIndex);

  sim_entity_hash *Result = 0;
  u32 HashValue = StorageIndex;
  for(u32 Offset = 0;
      Offset < ArrayCount(SimRegion->Hash);
      ++Offset)
    {
      u32 HashMask = ArrayCount(SimRegion->Hash) - 1;
      u32 HashIndex = (HashValue + Offset) & HashMask;
      sim_entity_hash *Entry = SimRegion->Hash + HashIndex;
      if((Entry->Index == 0) || (Entry->Index == StorageIndex))
	{
	  Result = Entry;
	  break;
	}
    }
  
  return(Result);
}

internal inline sim_entity*
CheckEntityByStorageIndex(sim_region *SimRegion, u32 StorageIndex)
{
  sim_entity_hash *Entry = GetHashFromStorageIndex(SimRegion, StorageIndex);
  sim_entity *Result = Entry->Ptr;
  return(Result);
}

internal inline v2
GetSimSpaceP(sim_region *SimRegion, low_entity *Stored)
{
  v2 Result = InvalidP;
  if(!IsSet(&Stored->Sim, EntityFlag_Nonspatial))
    {
      world_difference Diff = Subtract(SimRegion->World,
				       &Stored->P, &SimRegion->Origin);
      Result = Diff.dXY;
    }
    
  return(Result);
}

internal sim_entity*
AddEntity(state *State, sim_region *SimRegion, u32 StorageIndex, low_entity *Source, v2 *SimP);

internal inline void 
LoadEntityRefernce(state *State, sim_region *SimRegion, entity_reference *Ref)
{
  if(Ref->Index)
    {
      sim_entity_hash *Entry = GetHashFromStorageIndex(SimRegion, Ref->Index);
      if(Entry->Ptr == 0)
	{
	  Entry->Index = Ref->Index;
	  low_entity *LowEntity = GetLowEntity(State, Ref->Index);
	  v2 P = GetSimSpaceP(SimRegion, LowEntity);
	  Entry->Ptr   = AddEntity(State, SimRegion, Ref->Index, LowEntity, &P);
	}
      
      Ref->Ptr = Entry->Ptr;
    }
}

internal inline void
StoreEntityReference(entity_reference *Ref)
{
  if(Ref->Ptr != 0)
    {
      Ref->Index = Ref->Ptr->StorageIndex;
    }
}

internal sim_entity*
_AddEntity(state *State, sim_region *SimRegion, u32 StorageIndex, low_entity *Source)
{
  Assert(StorageIndex);
  sim_entity *Entity = 0;

  sim_entity_hash *Entry = GetHashFromStorageIndex(SimRegion, StorageIndex);
  if(Entry->Ptr == 0)
    {  
      if(SimRegion->EntityCount < SimRegion->MaxEntityCount)
	{
	  Entity = SimRegion->Entities + SimRegion->EntityCount++;

	  Entry->Index = StorageIndex;
	  Entry->Ptr = Entity;

	  if(Source)
	    {
	      *Entity = Source->Sim;
	      LoadEntityRefernce(State, SimRegion, &Entity->Sword);

	      Assert(!IsSet(&Source->Sim, EntityFlag_Simming));
	      AddFlag(&Source->Sim, EntityFlag_Simming);
	    }
      
	  Entity->StorageIndex = StorageIndex;
	  Entity->Updatable = false; 
	}
      else
	{
	  InvalidCodePath;
	}
    }  
  return(Entity);
}

internal sim_entity*
AddEntity(state *State, sim_region *SimRegion, u32 StorageIndex, low_entity *Source, v2 *SimP)
{
  sim_entity* Dest = _AddEntity(State, SimRegion, StorageIndex, Source);
  if(Dest)
    {
      if(SimP)
	{
	  Dest->P = *SimP;
	  Dest->Updatable = IsInRectangle(SimRegion->UpdatableBounds, Dest->P);
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
  ZeroStruct(SimRegion->Hash);

  r32 UpdateSafetyMargin = 1.0f;
  
  SimRegion->World = World;
  SimRegion->Origin = Origin;
  SimRegion->UpdatableBounds = Bounds;
  SimRegion->Bounds = AddRadiusTo(SimRegion->UpdatableBounds, UpdateSafetyMargin, UpdateSafetyMargin);
  
  SimRegion->MaxEntityCount = 4096;
  SimRegion->EntityCount = 0;
  SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, sim_entity);
  
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
		      if(!IsSet(&Low->Sim, EntityFlag_Nonspatial))
			{
			  v2 SimSpaceP = GetSimSpaceP(SimRegion, Low);
			  if(IsInRectangle(SimRegion->Bounds, SimSpaceP))
			    {
			      AddEntity(State, SimRegion, LowEntityIndex, Low, &SimSpaceP);
			    }
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

      Assert(IsSet(&Stored->Sim, EntityFlag_Simming));
      Stored->Sim = *Entity;
      Assert(!IsSet(&Stored->Sim, EntityFlag_Simming));
      
      StoreEntityReference(&Stored->Sim.Sword);
	
      world_position NewP = IsSet(Entity, EntityFlag_Nonspatial) ?
	NullPosition() :
	MapIntoChunkSpace(SimRegion->World, SimRegion->Origin, Entity->P);
      ChangeEntityLocation(&State->WorldArena, State->World, Entity->StorageIndex,
			   Stored, NewP);

      if(Entity->StorageIndex == State->CameraFollowingEntityIndex)
	{      
	  world_position NewCameraP = State->CameraP;
      
	  State->CameraP.ChunkZ = Stored->P.ChunkZ;
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
	  NewCameraP = Stored->P;
#endif
	  State->CameraP = NewCameraP;
	}
    }
}

internal bool32
TestWall(r32 WallX, r32 RelX, r32 RelY, r32 PlayerDeltaX, r32 PlayerDeltaY,
	 r32 *tMin, r32 MinY, r32 MaxY)
{
  bool32 Hit = false;

  r32 Epsilon = 0.001f;
  if(PlayerDeltaX != 0.0f)
    {
      r32 tResult = (WallX - RelX) / PlayerDeltaX;
      r32 Y = RelY + (tResult * PlayerDeltaY);
      if((tResult >= 0.0f) && (*tMin > tResult))
	{
	  if((Y >= MinY) && (Y <= MaxY))
	    {
	      *tMin = Maximum(0.0f, tResult - Epsilon);
	      Hit = true;
	    }
	}
    }

  return(Hit);
}

internal void
MoveEntity(sim_region *SimRegion, sim_entity *Entity, r32 dt, move_spec *MoveSpec, v2 ddEntity)
{
  Assert(!IsSet(Entity, EntityFlag_Nonspatial));
  
  world *World = SimRegion->World; 

  if(MoveSpec->UnitMaxAccelVector)
    {
      r32 ddPSqLength = LengthSq(ddEntity);
      if(ddPSqLength > 1.0f)
	{
	  ddEntity = VMulS(1.0f / SquareRoot(ddPSqLength), ddEntity);
	}
    }
  
  ddEntity = VMulS(MoveSpec->Speed, ddEntity);

  ddEntity = VAdd(ddEntity,
		  VMulS(-MoveSpec->Drag, Entity->dP));

  v2 OldPlayerP = Entity->P;
  v2 PlayerDelta = VAdd(VMulS(0.5f ,
			      VMulS(Square(dt),
				    ddEntity)),
			VMulS(dt, Entity->dP));
  Entity->dP = VAdd(VMulS(dt, ddEntity),
			 Entity->dP);
  v2 NewPlayerP = VAdd(OldPlayerP, PlayerDelta);
      
  r32 ddZ = -9.8f;
  Entity->Z = 0.5f * ddZ * Square(dt) + Entity->dZ*dt + Entity->Z;
  Entity->dZ = ddZ * dt + Entity->dZ;
  if(Entity->Z < 0)
    {
      Entity->Z = 0;
    }

  for(u32 Iteration = 0;
      Iteration < 4;
      ++Iteration)
    {
      r32 tMin = 1.0f;
      v2 WallNormal = {};
      sim_entity *HitEntity = 0;
      
      v2 DesiredPosition = VAdd(Entity->P, PlayerDelta);

      if(IsSet(Entity, EntityFlag_Collides) &&
	 !IsSet(Entity, EntityFlag_Nonspatial))
	{
	  for(u32 TestHighEntityIndex = 1;
	      TestHighEntityIndex < SimRegion->EntityCount;
	      ++TestHighEntityIndex)
	    {
	      sim_entity *TestEntity = SimRegion->Entities + TestHighEntityIndex; 
	      if(Entity != TestEntity)
		{
		  if(IsSet(TestEntity, EntityFlag_Collides) &&
		     !IsSet(Entity, EntityFlag_Nonspatial))
		    {
		      r32 DiameterW = TestEntity->Width + Entity->Width;
		      r32 DiameterH = TestEntity->Height + Entity->Height;
		      v2 MinCorner = (v2){-0.5f*DiameterW,
					  -0.5f*DiameterH};
		      v2 MaxCorner = (v2){0.5f*DiameterW,
					  0.5f*DiameterH};

		      v2 Rel = VSub(Entity->P, TestEntity->P);
	  
		      if(TestWall(MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
				  &tMin, MinCorner.Y, MaxCorner.Y))
			{
			  HitEntity = TestEntity;
			  WallNormal = (v2){-1, 0};
			}
		      if(TestWall(MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
				  &tMin, MinCorner.Y, MaxCorner.Y))
			{
			  HitEntity = TestEntity;
			  WallNormal = (v2){1, 0};
			}
		      if(TestWall(MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
				  &tMin, MinCorner.X, MaxCorner.X))
			{
			  HitEntity = TestEntity;
			  WallNormal = (v2){0, -1};
			}
		      if(TestWall(MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
				  &tMin, MinCorner.X, MaxCorner.X))
			{
			  HitEntity = TestEntity;
			  WallNormal = (v2){0, 1};
			}
		    }
		}
	    }
	}
      
      Entity->P = VAdd(Entity->P,
			    VMulS(tMin, PlayerDelta));
      if(HitEntity)
	{
	  Entity->dP = VSub(Entity->dP,
				 VMulS(1 * Inner(Entity->dP, WallNormal),
				       WallNormal));
	  PlayerDelta = VSub(DesiredPosition, Entity->P);
	  PlayerDelta = VSub(PlayerDelta,
			     VMulS(1 * Inner(PlayerDelta, WallNormal),
				   WallNormal));
	}
      else
	{
	  break;
	}
    }
  
  if((Entity->dP.X == 0) &&
     (Entity->dP.Y == 0))
    {
      
    }
  else if(AbsoluteValue(Entity->dP.X) > AbsoluteValue(Entity->dP.Y))
    {
      if(Entity->dP.X > 0)
	{
	  Entity->FacingDirection = 0;
	}
      else
	{
	  Entity->FacingDirection = 2;
	}
    }
  else 
    {
      if(Entity->dP.Y > 0)
	{
	  Entity->FacingDirection = 1;
	}
      else
	{
	  Entity->FacingDirection = 3;
	}
      
    }
}
