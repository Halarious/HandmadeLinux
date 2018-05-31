
typedef struct
{
  debug_state* State;
  memory_arena* Arena;

  debug_variable* Group;
  u32 VarCount;
  debug_variable* Vars[64];
} debug_variable_definition_context; 

internal debug_variable*
DEBUGAddVariable(debug_state* DebugState, debug_variable_type Type, char* Name)
{
  debug_variable* Var = PushStruct(&DebugState->DebugArena, debug_variable);  
  Var->Type = Type;
  Var->Name = (char*) PushCopy(&DebugState->DebugArena, StringLength(Name) + 1, Name);

  return(Var);
}

internal debug_variable* 
DEBUGAddVariableWithContext(debug_variable_definition_context* Context, debug_variable_type Type, char* Name)
{
  Assert(Context->VarCount < ArrayCount(Context->Vars));
  
  debug_variable* Var = DEBUGAddVariable(Context->State, Type, Name);
  Context->Vars[Context->VarCount++] = Var;
  
  return(Var);
}

internal debug_variable*
DEBUGBeginVariableGroup(debug_variable_definition_context* Context, char* Name)
{
  debug_variable* Group = DEBUGAddVariableWithContext(Context, DebugVariable_VarArray, Name);    
  Group->VarArray.Count = 0;

  
  
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
  Assert(Context->Group);
  
  Context->Group = Context->Group->Parent;
}

internal void
DEBUGCreateVariables(debug_variable_definition_context* Context)
{

  debug_variable* UseDebugCamRef = 0;
  
#define DEBUG_VARIABLE_LISTING_BOOL(Name) DEBUGAddBoolVariable(Context, #Name, DEBUGUI_##Name)
#define DEBUG_VARIABLE_LISTING_REAL32(Name) DEBUGAddReal32Variable(Context, #Name, DEBUGUI_##Name)
#define DEBUG_VARIABLE_LISTING_V4(Name) DEBUGAddV4Variable(Context, #Name, DEBUGUI_##Name)
#define DEBUG_VARIABLE_LISTING_BITMAP(Name) DEBUGAddBitmapVariable(Context, #Name, DEBUGUI_##Name)

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
      UseDebugCamRef = DEBUG_VARIABLE_LISTING_BOOL(UseDebugCamera);
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


