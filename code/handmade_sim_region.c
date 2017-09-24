
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
EntityOverlapsRectangle(v3 P, sim_entity_collision_volume Volume, rectangle3 Rect)
{
  rectangle3 Grown = AddRadiusTo(Rect, V3MulS(0.5f, Volume.Dim));
  bool32 Result = IsInRectangle(Grown, V3Add(P, Volume.OffsetP));
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
	  Dest->Updatable = EntityOverlapsRectangle(Dest->P, Dest->Collision->TotalVolume, SimRegion->UpdatableBounds);
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
			      if(EntityOverlapsRectangle(SimSpaceP,
							 Low->Sim.Collision->TotalVolume,
							 SimRegion->Bounds))
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

typedef struct
{
  r32  X;
  r32  RelX;
  r32  RelY;
  r32  DeltaX;
  r32  DeltaY;
  r32  MinY;
  r32  MaxY;
  v3   Normal;
} test_wall;

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
      
      if(IsSet(A, EntityFlag_Collides) &&
	 IsSet(B, EntityFlag_Collides))
	{      
	  if(!IsSet(A, EntityFlag_Nonspatial) &&
	     !IsSet(B, EntityFlag_Nonspatial))
	    {      
	      Result = true;
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
      *Ground = GetStairGround(Region, GetEntityGroundPoint(Mover));
    }  
}

internal bool32
SpeculativeCollide(sim_entity *Mover, sim_entity *Region)
{
  bool32 Result = true;
  
  if(Region->Type == EntityType_Stairwell)
    {
      
      r32 StepHeight = 0.1f;
#if 0
      Result = ((AbsoluteValue(GetEntityGroundPoint(Mover).Z - Ground) > StepHeight) ||
		((Bary.Y > 0.1f) && (Bary.Y < 0.9f)));
#endif
      v3 MoverGroundPoint = GetEntityGroundPoint(Mover);
      r32 Ground = GetStairGround(Region, MoverGroundPoint);
      Result = (AbsoluteValue(GetEntityGroundPoint(Mover).Z - Ground) > StepHeight);
    }

  return(Result);
}

internal bool32
EntitiesOverlap(sim_entity* Entity, sim_entity* TestEntity, v3 Epsilon)
{
  bool32 Result = false;
  
  for(u32 EntityVolumeIndex = 0;
      !Result && (EntityVolumeIndex < Entity->Collision->VolumeCount);
      ++EntityVolumeIndex)
    {
      sim_entity_collision_volume *Volume = Entity->Collision->Volumes + EntityVolumeIndex; 
      for(u32 TestVolumeIndex = 0;
	  !Result && (TestVolumeIndex < TestEntity->Collision->VolumeCount);
	  ++TestVolumeIndex)
	{
	  sim_entity_collision_volume *TestVolume = TestEntity->Collision->Volumes + TestVolumeIndex;
		  	    

	  rectangle3 EntityRect = RectCenterDim(V3Add(Entity->P, Volume->OffsetP),
						V3Add(Epsilon, Volume->Dim));

	  rectangle3 TestEntityRect = RectCenterDim(V3Add(TestEntity->P, TestVolume->OffsetP),
						    TestVolume->Dim);
	  Result = RectanglesIntersect(EntityRect, TestEntityRect);			   
	}
    }
  
  return(Result);
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

  v3 Drag = V3MulS(-MoveSpec->Drag, Entity->dP);
  Drag.Z = 0.0f;
  
  ddEntity = V3Add(ddEntity, Drag);
  if(!IsSet(Entity,EntityFlag_ZSupported))
    {
      ddEntity = V3Add(ddEntity,
		       V3(0, 0, -9.8f));
    }
  
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
      r32 tMax = 0.0f;
 
      r32 PlayerDeltaLength = V3Length(PlayerDelta);
      if(PlayerDeltaLength > 0.0f)
	{
	  if(PlayerDeltaLength > DistanceRemaining)
	    {
	      tMin = DistanceRemaining / PlayerDeltaLength; 
	    }
      
	  v3 WallNormalMin = {};
	  v3 WallNormalMax = {};
	  
	  sim_entity *HitEntityMin = 0;
	  sim_entity *HitEntityMax = 0;
      
	  v3 DesiredPosition = V3Add(Entity->P, PlayerDelta);

	  if(!IsSet(Entity, EntityFlag_Nonspatial))
	    {
	      for(u32 TestHighEntityIndex = 0;
		  TestHighEntityIndex < SimRegion->EntityCount;
		  ++TestHighEntityIndex)
		{
		  sim_entity *TestEntity = SimRegion->Entities + TestHighEntityIndex; 

		  r32 OverlapEpsilon = 0.001f;
		  if((IsSet(TestEntity, EntityFlag_Traversable) &&
		      EntitiesOverlap(Entity, TestEntity, V3(OverlapEpsilon, OverlapEpsilon, OverlapEpsilon))) ||
		     CanCollide(State, Entity, TestEntity))
		    {
		      for(u32 EntityVolumeIndex = 0;
			  EntityVolumeIndex < Entity->Collision->VolumeCount;
			  ++EntityVolumeIndex)
			{
			  sim_entity_collision_volume *Volume = Entity->Collision->Volumes + EntityVolumeIndex; 
			  for(u32 TestVolumeIndex = 0;
			      TestVolumeIndex < TestEntity->Collision->VolumeCount;
			      ++TestVolumeIndex)
			    {
			      if(TestEntity->Type == EntityType_Space)
				{
				  int BreakHere = 3;
				}
			      
			      sim_entity_collision_volume *TestVolume = TestEntity->Collision->Volumes + TestVolumeIndex; 
			      v3 MinkowskiDiameter = {TestVolume->Dim.X + Volume->Dim.X,
						      TestVolume->Dim.Y + Volume->Dim.Y,
						      TestVolume->Dim.Z + Volume->Dim.Z};

			      v3 MinCorner = V3MulS(-0.5f, MinkowskiDiameter);
			      v3 MaxCorner = V3MulS( 0.5f, MinkowskiDiameter);

			      v3 Rel = V3Sub(V3Add(Entity->P, Volume->OffsetP),
					     V3Add(TestEntity->P, TestVolume->OffsetP));

			      if((Rel.Z >= MinCorner.Z) &&
				 (Rel.Z <  MaxCorner.Z))
				{
				  test_wall Walls[] =
				    {
				      {MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y, MinCorner.Y, MaxCorner.Y, V3(-1,  0, 0)},
				      {MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y, MinCorner.Y, MaxCorner.Y, V3( 1,  0, 0)},
				      {MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X, MinCorner.X, MaxCorner.X, V3( 0, -1, 0)},
				      {MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X, MinCorner.X, MaxCorner.X, V3( 0,  1, 0)}
				    };

				  if(IsSet(TestEntity, EntityFlag_Traversable))
				    {				      
				      r32 tMaxTest = tMax;
				      bool32 HitThis = false;

				      v3 TestWallNormal = {};
				      for(u32 WallIndex = 0;
					  WallIndex < ArrayCount(Walls);
					  ++WallIndex)
					{
					  test_wall *Wall = Walls + WallIndex;
				      
					  r32 tEpsilon = 0.001f;
					  if(Wall->DeltaX != 0.0f)
					    {
					      r32 tResult = (Wall->X - Wall->RelX) / Wall->DeltaX;
					      r32 Y = Wall->RelY + (tResult * Wall->DeltaY);
					      if((tResult >= 0.0f) && (tMaxTest < tResult))
						{
						  if((Y >= Wall->MinY) && (Y <= Wall->MaxY))
						    {
						      tMaxTest = Maximum(0.0f, tResult - tEpsilon);
						      TestWallNormal = Wall->Normal;
						      HitThis = true;
						    }
						}
					    }
					}
					  					      
				      if(HitThis)
					{
					  tMax = tMaxTest;
					  WallNormalMax = TestWallNormal;
					  HitEntityMax  = TestEntity;
					}
				    }
				  else
				    {
				      r32 tMinTest = tMin;
				      bool32 HitThis = false;

				      v3 TestWallNormal = {};
				      for(u32 WallIndex = 0;
					  WallIndex < ArrayCount(Walls);
					  ++WallIndex)
					{
					  test_wall *Wall = Walls + WallIndex;
				      
					  r32 tEpsilon = 0.001f;
					  if(Wall->DeltaX != 0.0f)
					    {
					      r32 tResult = (Wall->X - Wall->RelX) / Wall->DeltaX;
					      r32 Y = Wall->RelY + (tResult * Wall->DeltaY);
					      if((tResult >= 0.0f) && (tMinTest > tResult))
						{
						  if((Y >= Wall->MinY) && (Y <= Wall->MaxY))
						    {
						      tMinTest = Maximum(0.0f, tResult - tEpsilon);
						      TestWallNormal = Wall->Normal;
						      HitThis = true;
						    }
						}
					    }
					}

				      if(HitThis)
					{
					  v3 TestP = V3Add(Entity->P,
							   V3MulS(tMinTest,
								  PlayerDelta));
					  if(SpeculativeCollide(Entity, TestEntity))
					    {
					      tMin = tMinTest;
					      WallNormalMin = TestWallNormal;
					      HitEntityMin  = TestEntity;
					    }
					}				      
				    }				    
				}
			    }
			}
		    }
		}
	    }
	  
	  v3 WallNormal;
	  sim_entity* HitEntity;
	  r32 tStop;
	  if(tMin < tMax)
	    {
	      tStop = tMin;
	      HitEntity = HitEntityMin;
	      WallNormal = WallNormalMin;
	    }
	  else
	    {
	      tStop = tMax;
	      HitEntity = HitEntityMax;
	      WallNormal = WallNormalMax;
	    }
	  
	  Entity->P = V3Add(Entity->P,
			    V3MulS(tStop, PlayerDelta));
	  DistanceRemaining -= tStop * PlayerDeltaLength;
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
    for(u32 TestHighEntityIndex = 0;
	TestHighEntityIndex < SimRegion->EntityCount;
	++TestHighEntityIndex)
      {
	sim_entity *TestEntity = SimRegion->Entities + TestHighEntityIndex; 
	if(CanOverlap(State, Entity, TestEntity) &&
	   EntitiesOverlap(Entity, TestEntity, V3(0.0f, 0.0f, 0.0f)))
	  {
	    HandleOverlap(State, Entity, TestEntity, dt, &Ground);
	  }
      }
  }
  
  Ground += Entity->P.Z - GetEntityGroundPoint(Entity).Z;
  if((Entity->P.Z <= Ground) ||
     (IsSet(Entity, EntityFlag_ZSupported) &&
      (Entity->dP.Z == 0.0f))) 
    {
      Entity->P.Z = Ground;
      Entity->dP.Z = 0;
      AddFlags(Entity, EntityFlag_ZSupported);
    }
  else
    {
      ClearFlags(Entity, EntityFlag_ZSupported);
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
