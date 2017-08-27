#define InvalidP V3(100000.0f, 100000.0f, 100000.0f)

internal inline bool32
IsSet(sim_entity *Entity, u32 Flag)
{
  bool32 Result = Entity->Flags & Flag;
  return(Result);
}

internal inline void
AddFlag(sim_entity *Entity, u32 Flag)
{
  Entity->Flags |= Flag;
}

internal inline void
ClearFlag(sim_entity *Entity, u32 Flag)
{
  Entity->Flags &= ~Flag;
}

internal inline void
MakeEntityNonSpatial(sim_entity *Entity)
{
  AddFlag(Entity, EntityFlag_Nonspatial);
  Entity->P = InvalidP;
}

internal inline void
MakeEntitySpatial(sim_entity *Entity, v3 P, v3 dP)
{
  ClearFlag(Entity, EntityFlag_Nonspatial);
  Entity->P  = P;
  Entity->dP = dP;
}
