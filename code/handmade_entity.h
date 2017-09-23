#define InvalidP V3(100000.0f, 100000.0f, 100000.0f)

internal inline bool32
IsSet(sim_entity *Entity, u32 Flag)
{
  bool32 Result = Entity->Flags & Flag;
  return(Result);
}

internal inline void
AddFlags(sim_entity *Entity, u32 Flag)
{
  Entity->Flags |= Flag;
}

internal inline void
ClearFlags(sim_entity *Entity, u32 Flag)
{
  Entity->Flags &= ~Flag;
}

internal inline void
MakeEntityNonSpatial(sim_entity *Entity)
{
  AddFlags(Entity, EntityFlag_Nonspatial);
  Entity->P = InvalidP;
}

internal inline void
MakeEntitySpatial(sim_entity *Entity, v3 P, v3 dP)
{
  ClearFlags(Entity, EntityFlag_Nonspatial);
  Entity->P  = P;
  Entity->dP = dP;
}

internal inline v3
GetEntityGroundPoint(sim_entity *Entity)
{
  v3 Result = Entity->P;
  return(Result);
}

internal inline r32
GetStairGround(sim_entity *Entity, v3 AtGroundPoint)
{
  Assert(Entity->Type == EntityType_Stairwell);
  
  rectangle2 RegionRect = RectCenterDim2(Entity->P.XY, Entity->WalkableDim);
  v2 Bary = V2Clamp01(V2GetBarycentric(RegionRect, AtGroundPoint.XY));
  r32 Result = Entity->P.Z + Bary.Y*Entity->WalkableHeight;

  return(Result);
}

