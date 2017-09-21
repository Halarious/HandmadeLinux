
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

internal inline v3
GetSimSpaceP(sim_region *SimRegion, low_entity *Stored)
{
  v3 Result = InvalidP;
  if(!IsSet(&Stored->Sim, EntityFlag_Nonspatial))
    {
      Result = Subtract(SimRegion->World,
			&Stored->P, &SimRegion->Origin);
    }
  
  return(Result);
}

internal sim_entity*
AddEntity(state *State, sim_region *SimRegion, u32 StorageIndex, low_entity *Source, v3 *SimP);

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
	  v3 P = GetSimSpaceP(SimRegion, LowEntity);
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
	      AddFlags(&Source->Sim, EntityFlag_Simming);
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

internal inline bool32
EntityOverlapsRectangle(v3 P, v3 Dim, rectangle3 Rect)
{
  rectangle3 Grown = AddRadiusTo(Rect, V3MulS(0.5f, Dim));
  bool32 Result = IsInRectangle(Grown, P);
  return(Result);
}

internal sim_entity*
AddEntity(state *State, sim_region *SimRegion, u32 StorageIndex, low_entity *Source, v3 *SimP)
{
  sim_entity* Dest = _AddEntity(State, SimRegion, StorageIndex, Source);
  if(Dest)
    {
      if(SimP)
	{
	  Dest->P = *SimP;
	  Dest->Updatable = EntityOverlapsRectangle(Dest->P, Dest->Dim, SimRegion->UpdatableBounds);
	}
      else
	{
	  Dest->P = GetSimSpaceP(SimRegion, Source);
	}
    }
  
  return(Dest);
}

internal sim_region*
BeginSim(memory_arena *SimArena, state* State, world* World, world_position Origin, rectangle3 Bounds, r32 dt)
{
  sim_region *SimRegion = PushStruct(SimArena, sim_region);
  ZeroStruct(SimRegion->Hash);

  SimRegion->MaxEntityRadius = 5.0f;
  SimRegion->MaxEntityVelocity = 30.0f;
  r32 UpdateSafetyMargin  = SimRegion->MaxEntityRadius + SimRegion->MaxEntityVelocity*dt;
  r32 UpdateSafetyMarginZ = 1.0f;  

  SimRegion->World  = World;
  SimRegion->Origin = Origin;
  SimRegion->UpdatableBounds = AddRadiusTo(Bounds, V3(SimRegion->MaxEntityRadius,
						      SimRegion->MaxEntityRadius,
						      SimRegion->MaxEntityRadius));
  SimRegion->Bounds = AddRadiusTo(SimRegion->UpdatableBounds, V3(UpdateSafetyMargin,
								 UpdateSafetyMargin,
								 UpdateSafetyMarginZ));
  
  SimRegion->MaxEntityCount = 4096;
  SimRegion->EntityCount = 0;
  SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, sim_entity);
  
  world_position MinChunkP =
    MapIntoChunkSpace(World, SimRegion->Origin, GetMinCorner(SimRegion->Bounds));
  world_position MaxChunkP =
    MapIntoChunkSpace(World, SimRegion->Origin, GetMaxCorner(SimRegion->Bounds));

  for(s32 ChunkZ = MinChunkP.ChunkZ;
      ChunkZ <= MaxChunkP.ChunkZ;
      ++ChunkZ)
    {
      for(s32 ChunkY = MinChunkP.ChunkY;
	  ChunkY <= MaxChunkP.ChunkY;
	  ++ChunkY)
	{
	  for(s32 ChunkX = MinChunkP.ChunkX;
	      ChunkX <= MaxChunkP.ChunkX;
	      ++ChunkX)
	    {
	      world_chunk *Chunk = GetWorldChunk(World, ChunkX, ChunkY, ChunkZ, 0);
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
			      v3 SimSpaceP = GetSimSpaceP(SimRegion, Low);
			      if(EntityOverlapsRectangle(SimSpaceP, Low->Sim.Dim, SimRegion->Bounds))
				{
				  AddEntity(State, SimRegion, LowEntityIndex, Low, &SimSpaceP);
				}
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
      
	  NewCameraP.ChunkZ = Stored->P.ChunkZ;
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
	  r32 CamZOffset = NewCameraP.Offset_.Z;
	  NewCameraP = Stored->P;
	  NewCameraP.Offset_.Z = CamZOffset;
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

internal bool32
CanCollide(state *State, sim_entity *A, sim_entity *B)
{
  bool32 Result = false;
  if(A != B)
    {
      if(A->StorageIndex > B->StorageIndex)
	{
	  sim_entity *Temp = A;
	  A = B;
	  B = Temp;
	}
    
      if(!IsSet(A, EntityFlag_Nonspatial) &&
	 !IsSet(B, EntityFlag_Nonspatial))
	{
      
	  Result = true;
	}
    
      if(A->Type == EntityType_Stairwell ||
	 B->Type == EntityType_Stairwell)
	{
      
	  Result = false;
	}

      u32 HashBucket = A->StorageIndex & (ArrayCount(State->CollisionRuleHash) - 1);
      for(pairwise_collision_rule *Rule = State->CollisionRuleHash[HashBucket];
	  Rule;
	  Rule = Rule->NextInHash)
	{
	  if((Rule->StorageIndexA == A->StorageIndex) &&
	     (Rule->StorageIndexB == B->StorageIndex))
	    {
	      Result = Rule->CanCollide;
	      break;
	    }
	}
    }  
  return(Result);
}

internal bool32
HandleCollision(state *State, sim_entity *A, sim_entity *B)
{
  bool32 StopsOnCollision = false;

  if(A->Type == EntityType_Sword)
    {
      AddCollisionRule(State, A->StorageIndex, B->StorageIndex, false);
      StopsOnCollision = false; 
    }
  else
    {
      StopsOnCollision = true;
    }
  
  if(A->Type > B->Type)
    {
      sim_entity *Temp = A;
      A = B;
      B = Temp;
    }
  
  if((A->Type == EntityType_Monstar) &&
     (B->Type == EntityType_Sword))
    {
      if(A->HitPointMax > 0)
	{
	  --A->HitPointMax;
	}
    }
  
  return(StopsOnCollision);  
}

internal bool32
CanOverlap(state *State, sim_entity *Mover, sim_entity *Region)
{
  bool32 Result = false;

  if(Mover != Region)
    {
      if(Region->Type == EntityType_Stairwell)
	{
	  Result = true;
	}
    }

  return(Result);
}

internal void 
HandleOverlap(state *State, sim_entity *Mover, sim_entity *Region, r32 dt, r32 *Ground)
{
  if(Region->Type == EntityType_Stairwell)
    {
      rectangle3 RegionRect = RectCenterDim(Region->P, Region->Dim);
      v3 Bary = V3Clamp01(GetBarycentric(RegionRect, Mover->P));

      *Ground = Lerp(RegionRect.Min.Z, Bary.Y, RegionRect.Max.Z);
    }  
}

internal void
MoveEntity(state *State, sim_region *SimRegion, sim_entity *Entity, r32 dt,
	   move_spec *MoveSpec, v3 ddEntity)
{
  Assert(!IsSet(Entity, EntityFlag_Nonspatial));
  
  world *World = SimRegion->World; 

  if(MoveSpec->UnitMaxAccelVector)
    {
      r32 ddPSqLength = V3LengthSq(ddEntity);
      if(ddPSqLength > 1.0f)
	{
	  ddEntity = V3MulS(1.0f / SquareRoot(ddPSqLength), ddEntity);
	}
    }
  
  ddEntity = V3MulS(MoveSpec->Speed, ddEntity);
  ddEntity = V3Add(ddEntity,
		  V3MulS(-MoveSpec->Drag, Entity->dP));
  ddEntity = V3Add(ddEntity,
		   V3(0, 0, -9.8f));
  v3 OldPlayerP = Entity->P;
  v3 PlayerDelta = V3Add(V3MulS(0.5f ,
			      V3MulS(Square(dt),
				    ddEntity)),
			V3MulS(dt, Entity->dP));
  Entity->dP = V3Add(V3MulS(dt, ddEntity),
			 Entity->dP);
  Assert(V3LengthSq(Entity->dP) <= Square(SimRegion->MaxEntityVelocity));
  v3 NewPlayerP = V3Add(OldPlayerP, PlayerDelta);
  
  r32 DistanceRemaining = Entity->DistanceLimit;
  if(DistanceRemaining == 0.0f)
    {
      DistanceRemaining = 10000.0f;
    }
  
  for(u32 Iteration = 0;
      Iteration < 4;
      ++Iteration)
    {
      r32 tMin = 1.0f;
 
      r32 PlayerDeltaLength = V3Length(PlayerDelta);
      if(PlayerDeltaLength > 0.0f)
	{
	  if(PlayerDeltaLength > DistanceRemaining)
	    {
	      tMin = DistanceRemaining / PlayerDeltaLength; 
	    }
      
	  v3 WallNormal = {};
	  sim_entity *HitEntity = 0;
      
	  v3 DesiredPosition = V3Add(Entity->P, PlayerDelta);

	  if(!IsSet(Entity, EntityFlag_Nonspatial))
	    {
	      for(u32 TestHighEntityIndex = 1;
		  TestHighEntityIndex < SimRegion->EntityCount;
		  ++TestHighEntityIndex)
		{
		  sim_entity *TestEntity = SimRegion->Entities + TestHighEntityIndex; 
		  if(CanCollide(State, Entity, TestEntity) &&
		     (TestEntity->P.Z == Entity->P.Z))
		    {
		      v3 MinkowskiDiameter = {TestEntity->Dim.X + Entity->Dim.X,
					      TestEntity->Dim.Y + Entity->Dim.Y,
					      TestEntity->Dim.Z + Entity->Dim.Z};

		      v3 MinCorner = V3MulS(-0.5f, MinkowskiDiameter);
		      v3 MaxCorner = V3MulS( 0.5f, MinkowskiDiameter);

		      v3 Rel = V3Sub(Entity->P, TestEntity->P);
	  
		      if(TestWall(MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
				  &tMin, MinCorner.Y, MaxCorner.Y))
			{
			  HitEntity = TestEntity;
			  WallNormal = V3(-1, 0, 0);
			}
		      if(TestWall(MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
				  &tMin, MinCorner.Y, MaxCorner.Y))
			{
			  HitEntity = TestEntity;
			  WallNormal = V3(1, 0, 0);
			}
		      if(TestWall(MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
				  &tMin, MinCorner.X, MaxCorner.X))
			{
			  HitEntity = TestEntity;
			  WallNormal = V3(0, -1, 0);
			}
		      if(TestWall(MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
				  &tMin, MinCorner.X, MaxCorner.X))
			{
			  HitEntity = TestEntity;
			  WallNormal = V3(0, 1, 0);
			}			
		    }
		}
	    }
	
	  Entity->P = V3Add(Entity->P,
			    V3MulS(tMin, PlayerDelta));
	  DistanceRemaining -= tMin * PlayerDeltaLength;
	  if(HitEntity)
	    {
	      PlayerDelta = V3Sub(DesiredPosition, Entity->P);

	      bool32 StopsOnCollision = HandleCollision(State, Entity, HitEntity);
	      if(StopsOnCollision)
		{
		  PlayerDelta = V3Sub(PlayerDelta,
				      V3MulS(1 * V3Inner(PlayerDelta, WallNormal),
					     WallNormal));
		  Entity->dP = V3Sub(Entity->dP,
				     V3MulS(1 * V3Inner(Entity->dP, WallNormal),
					    WallNormal));
		}
	    }
	  else
	    {
	      break;
	    }
	}
      else
	{
	  break;
	}
    }

  r32 Ground = 0.0f;

  {
    rectangle3 EntityRect = RectCenterDim(Entity->P, Entity->Dim);

    for(u32 TestHighEntityIndex = 0;
	TestHighEntityIndex < SimRegion->EntityCount;
	++TestHighEntityIndex)
      {
	sim_entity *TestEntity = SimRegion->Entities + TestHighEntityIndex; 
	if(CanOverlap(State, Entity, TestEntity))
	  {
	    rectangle3 TestEntityRect =
	      RectCenterDim(TestEntity->P, TestEntity->Dim);
	    if(RectanglesIntersect(EntityRect, TestEntityRect))			   
	      {
		HandleOverlap(State, Entity, TestEntity, dt, &Ground);
	      }
	  }
      }
  }
    
  if(Entity->P.Z < Ground)
    {
      Entity->P.Z = Ground;
      Entity->dP.Z = 0;
    }
  
  if(Entity->DistanceLimit != 0.0f)
    {
      Entity->DistanceLimit = DistanceRemaining;
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
