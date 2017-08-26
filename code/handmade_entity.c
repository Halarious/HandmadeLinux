
internal inline move_spec
DefaultMoveSpec()
{  
  move_spec Result;

  Result.UnitMaxAccelVector = false;
  Result.Speed = 1.0f;
  Result.Drag = 0.0f;

  return(Result);
}

internal void
UpdateFamiliar(sim_region *SimRegion, sim_entity *Entity, r32 dt)
{
  sim_entity *ClosestHero = 0;
  r32 ClosestHeroDSq = Square(10.0f);

  sim_entity *TestEntity = SimRegion->Entities;
  for(u32 TestEntityIndex = 1;
      TestEntityIndex < SimRegion->EntityCount;
      ++TestEntityIndex)
    {
      if(TestEntity->Type == EntityType_Hero)
	{
	  r32 TestDSq = LengthSq(VSub(TestEntity->P,
				      Entity->P));
	  if(TestEntity->Type == EntityType_Hero)
	    {
	      TestDSq *= 0.75f;
	    }
	  if(ClosestHeroDSq > TestDSq)
	    {
	      ClosestHero = TestEntity;
	      ClosestHeroDSq = TestDSq; 
	    }
	}
    }
  
  v2 ddP = {};
  if((ClosestHero) && (ClosestHeroDSq > Square(3.0f)))
    {
      r32 Acceleration = 0.5f;
      r32 OneOverLength = Acceleration / SquareRoot(ClosestHeroDSq);
      ddP = VMulS(OneOverLength,
		  VSub(ClosestHero->P, Entity->P));
    }

  move_spec MoveSpec = DefaultMoveSpec();
  MoveSpec.UnitMaxAccelVector = true;
  MoveSpec.Speed = 50.0f;
  MoveSpec.Drag = 8.0f;
  MoveEntity(SimRegion, Entity, dt, &MoveSpec, ddP);
}

internal void
UpdateMonstar(sim_region *SimRegion, sim_entity *Entity, r32 dt)
{
}

internal void
UpdateSword(sim_region *SimRegion, sim_entity *Entity, r32 dt)
{
  if(IsSet(Entity, EntityFlag_Nonspatial))
    {
    }
  else
    {
      move_spec MoveSpec = DefaultMoveSpec();
      MoveSpec.UnitMaxAccelVector = false;
      MoveSpec.Speed = 0.0f;
      MoveSpec.Drag = 0.0f;

      v2 OldP = Entity->P;
      MoveEntity(SimRegion, Entity, dt, &MoveSpec, V2(0, 0));
      r32 DistanceTraveled = Length(VSub(Entity->P,
					 OldP));
      Entity->DistanceRemaining -= DistanceTraveled;
      if(Entity->DistanceRemaining < 0.0f)
	{
	  MakeEntityNonSpatial(Entity);
	}
    }
}

