
#define DEBUG_MAX_VARIABLE_STACK_DEPTH 64

typedef struct
{
  debug_state* State;
  memory_arena* Arena;

  u32 GroupDepth;
  debug_variable* GroupStack[DEBUG_MAX_VARIABLE_STACK_DEPTH];
} debug_variable_definition_context; 

internal debug_variable*
DEBUGAddVariable(debug_state* DebugState, debug_variable_type Type, char* Name)
{
  debug_variable* Var = PushStruct(&DebugState->DebugArena, debug_variable);  
  Var->Type = Type;
  Var->Name = (char*) PushCopy(&DebugState->DebugArena, StringLength(Name) + 1, Name);

  return(Var);
}

internal void
DEBUGAddVariableToGroup(debug_state* DebugState, debug_variable* Group, debug_variable* Add)
{
    debug_variable_link* Link = PushStruct(&DebugState->DebugArena, debug_variable_link);
    DLIST_INSERT(&Group->VarGroup, Link);

    Link->Var = Add;
}

internal void
DEBUGAddVariableToDefaultGroup(debug_variable_definition_context* Context, debug_variable* Var)
{
  debug_variable* Parent = Context->GroupStack[Context->GroupDepth];
  if(Parent)
    {
      DEBUGAddVariableToGroup(Context->State, Parent, Var);  
    }
}

internal debug_variable* 
DEBUGAddVariableWithContext(debug_variable_definition_context* Context, debug_variable_type Type, char* Name)
{
  debug_variable* Var = DEBUGAddVariable(Context->State, Type, Name);
  DEBUGAddVariableToDefaultGroup(Context, Var);
    
  return(Var);
}

internal debug_variable*
DEBUGAddRootGroup(debug_state* DebugState, char* Name)
{
  debug_variable* Group = DEBUGAddVariable(DebugState, DebugVariable_VarGroup, Name);    
  DLIST_INIT(&Group->VarGroup);

  return(Group);
}

internal debug_variable*
DEBUGBeginVariableGroup(debug_variable_definition_context* Context, char* Name)
{
  debug_variable* Group = DEBUGAddRootGroup(Context->State, Name);
  DEBUGAddVariableToDefaultGroup(Context, Group);
  
  Assert(Context->GroupDepth < (ArrayCount(Context->GroupStack) - 1))
  Context->GroupStack[++Context->GroupDepth] = Group;
  
  return(Group);
}

internal debug_variable*
DEBUGAddBoolVariable(debug_variable_definition_context* Context, char* Name, bool32 Value)
{
  debug_variable* Var = DEBUGAddVariableWithContext(Context, DebugVariable_Bool32, Name);
  Var->Bool32 = Value;

  return(Var);
}

internal debug_variable*
DEBUGAddReal32Variable(debug_variable_definition_context* Context, char* Name, r32 Value)
{
  debug_variable* Var = DEBUGAddVariableWithContext(Context, DebugVariable_Real32, Name);
  Var->Real32 = Value;

  return(Var);
}

internal debug_variable*
DEBUGAddV4Variable(debug_variable_definition_context* Context, char* Name, v4 Value)
{
  debug_variable* Var = DEBUGAddVariableWithContext(Context, DebugVariable_V4, Name);
  Var->Vector4 = Value;

  return(Var);
}

internal debug_variable*
DEBUGAddBitmapVariable(debug_variable_definition_context* Context, char* Name, bitmap_id Value)
{
  debug_variable* Var = DEBUGAddVariableWithContext(Context, DebugVariable_BitmapDisplay, Name);
  Var->BitmapDisplay.ID = Value;
  
  return(Var);
}

internal void
DEBUGEndVariableGroup(debug_variable_definition_context* Context)
{
  Assert(Context->GroupDepth > 0);
  --Context->GroupDepth;
}

internal void
DEBUGCreateVariables(debug_variable_definition_context* Context)
{
  
#define DEBUG_VARIABLE_LISTING_BOOL(Name) DEBUGAddBoolVariable(Context, #Name, DEBUGUI_##Name)
#define DEBUG_VARIABLE_LISTING_REAL32(Name) DEBUGAddReal32Variable(Context, #Name, DEBUGUI_##Name)
#define DEBUG_VARIABLE_LISTING_V4(Name) DEBUGAddV4Variable(Context, #Name, DEBUGUI_##Name)
#define DEBUG_VARIABLE_LISTING_BITMAP(Name) DEBUGAddBitmapVariable(Context, #Name, DEBUGUI_##Name)

  DEBUGBeginVariableGroup(Context, "Entities");
  DEBUG_VARIABLE_LISTING_BOOL(DrawEntityOutlines);
  DEBUGEndVariableGroup(Context);  
  
  DEBUGBeginVariableGroup(Context, "Ground Chunks");
  DEBUG_VARIABLE_LISTING_BOOL(GroundChunkCheckerboards);
  DEBUG_VARIABLE_LISTING_BOOL(GroundChunkOutlines);
  DEBUG_VARIABLE_LISTING_BOOL(RecomputeGroundChunksOnExeChange);
  DEBUGEndVariableGroup(Context);

  DEBUGBeginVariableGroup(Context, "Particles");
  DEBUG_VARIABLE_LISTING_BOOL(ParticleTest);
  DEBUG_VARIABLE_LISTING_BOOL(ParticleGrid);
  DEBUGEndVariableGroup(Context);

  DEBUGBeginVariableGroup(Context, "Renderer");
  {
    DEBUG_VARIABLE_LISTING_BOOL(TestWeirdDrawBufferSize);
    DEBUG_VARIABLE_LISTING_BOOL(ShowLightingSamples);
  
    DEBUGBeginVariableGroup(Context, "Camera");
    {
      DEBUG_VARIABLE_LISTING_BOOL(UseDebugCamera);
      DEBUG_VARIABLE_LISTING_REAL32(DebugCameraDistance);
      DEBUG_VARIABLE_LISTING_BOOL(UseRoomBasedCamera);
    }
    DEBUGEndVariableGroup(Context);
  }
  DEBUGEndVariableGroup(Context);
  
  DEBUG_VARIABLE_LISTING_BOOL(FamiliarFollowsHero);
  DEBUG_VARIABLE_LISTING_BOOL(UseSpaceOutlines);
  DEBUG_VARIABLE_LISTING_V4(FauxV4);

#undef DEBUG_VARIABLE_LISTING_BOOL
#undef DEBUG_VARIABLE_LISTING_REAL32
#undef DEBUG_VARIABLE_LISTING_V4
#undef DEBUG_VARIABLE_LISTING_BITMAP
}

/*

#define DEBUGUI_GroundChunkCheckerboards 0
#define DEBUGUI_GroundChunkOutlines 0 
#define DEBUGUI_RecomputeGroundChunksOnExeChange 0
#define DEBUGUI_ParticleTest 0
#define DEBUGUI_ParticleGrid 0
#define DEBUGUI_TestWeirdDrawBufferSize 0
#define DEBUGUI_ShowLightingSamples 0
#define DEBUGUI_UseDebugCamera 0
#define DEBUGUI_UseRoomBasedCamera 0
#define DEBUGUI_FamiliarFollowsHero 0
#define DEBUGUI_UseSpaceOutlines 0

 */


