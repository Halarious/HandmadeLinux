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
  v3 Result = V3Add(Entity->P, V3(0.0f, 0.0f, -0.5f*Entity->Dim.Z));
  return(Result);
}

internal inline r32
GetStairGround(sim_entity *Entity, v3 AtGroundPoint)
{
  Assert(Entity->Type == EntityType_Stairwell);
  
  rectangle3 RegionRect = RectCenterDim(Entity->P, Entity->Dim);
  v3 Bary = V3Clamp01(GetBarycentric(RegionRect, AtGroundPoint));
  r32 Result = RegionRect.Min.Z + Bary.Y*Entity->WalkableHeight;

  return(Result);
}

