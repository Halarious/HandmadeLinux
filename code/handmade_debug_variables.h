
typedef struct
{
  debug_state* State;
  memory_arena* Arena;

  debug_variable* Group;
} debug_variable_definition_context; 

internal debug_variable* 
DEBUGAddVariable(debug_variable_definition_context* Context, debug_variable_type Type, char* Name)
{
  debug_variable* Var = PushStruct(Context->Arena, debug_variable);  
  Var->Type = Type;
  Var->Name = (char*) PushCopy(Context->Arena, StringLength(Name) + 1, Name);
  Var->Next = 0;
    
  debug_variable* Group = Context->Group;
  Var->Parent = Group;

  if(Group)
    {
      if(Group->Group.LastChild)
	{
	  Group->Group.LastChild = Group->Group.LastChild->Next = Var;
	}
      else
	{
	  Group->Group.LastChild = Group->Group.FirstChild = Var;
	}
    }
  return(Var);
}

internal debug_variable*
DEBUGBeginVariableGroup(debug_variable_definition_context* Context, char* Name)
{
  debug_variable* Group = DEBUGAddVariable(Context, DebugVariable_Group, Name);
  Group->Group.Expanded = false;
  Group->Group.FirstChild = Group->Group.LastChild = 0;

  Context->Group = Group;

  return(Group);
}

internal debug_variable*
DEBUGAddBoolVariable(debug_variable_definition_context* Context, char* Name, bool32 Value)
{
  debug_variable* Var = DEBUGAddVariable(Context, DebugVariable_Bool32, Name);
  Var->Bool32 = Value;

  return(Var);
}

internal debug_variable*
DEBUGAddReal32Variable(debug_variable_definition_context* Context, char* Name, r32 Value)
{
  debug_variable* Var = DEBUGAddVariable(Context, DebugVariable_Real32, Name);
  Var->Real32 = Value;

  return(Var);
}

internal debug_variable*
DEBUGAddV4Variable(debug_variable_definition_context* Context, char* Name, v4 Value)
{
  debug_variable* Var = DEBUGAddVariable(Context, DebugVariable_V4, Name);
  Var->Vector4 = Value;

  return(Var);
}

internal void
DEBUGEndVariableGroup(debug_variable_definition_context* Context)
{
  Assert(Context->Group);
  
  Context->Group = Context->Group->Parent;
}

internal void
DEBUGCreateVariables(debug_state* DebugState)
{
  debug_variable_definition_context Context = {};
  Context.State = DebugState;
  Context.Arena = &DebugState->DebugArena;
  Context.Group = DEBUGBeginVariableGroup(&Context, "Root");


#define DEBUG_VARIABLE_LISTING_BOOL(Name) DEBUGAddBoolVariable(&Context, #Name, DEBUGUI_##Name)
#define DEBUG_VARIABLE_LISTING_REAL32(Name) DEBUGAddReal32Variable(&Context, #Name, DEBUGUI_##Name)
#define DEBUG_VARIABLE_LISTING_V4(Name) DEBUGAddV4Variable(&Context, #Name, DEBUGUI_##Name)

  DEBUGBeginVariableGroup(&Context, "Ground Chunks");
  DEBUG_VARIABLE_LISTING_BOOL(GroundChunkCheckerboards);
  DEBUG_VARIABLE_LISTING_BOOL(GroundChunkOutlines);
  DEBUG_VARIABLE_LISTING_BOOL(RecomputeGroundChunksOnExeChange);
  DEBUGEndVariableGroup(&Context);

  DEBUGBeginVariableGroup(&Context, "Particles");
  DEBUG_VARIABLE_LISTING_BOOL(ParticleTest);
  DEBUG_VARIABLE_LISTING_BOOL(ParticleGrid);
  DEBUGEndVariableGroup(&Context);

  DEBUGBeginVariableGroup(&Context, "Renderer");
  {
    DEBUG_VARIABLE_LISTING_BOOL(TestWeirdDrawBufferSize);
    DEBUG_VARIABLE_LISTING_BOOL(ShowLightingSamples);
  
    DEBUGBeginVariableGroup(&Context, "Camera");
    {
      DEBUG_VARIABLE_LISTING_BOOL(UseDebugCamera);
      DEBUG_VARIABLE_LISTING_REAL32(DebugCameraDistance);
      DEBUG_VARIABLE_LISTING_BOOL(UseRoomBasedCamera);
    }
    DEBUGEndVariableGroup(&Context);
  }
  DEBUGEndVariableGroup(&Context);
  
  DEBUG_VARIABLE_LISTING_BOOL(FamiliarFollowsHero);
  DEBUG_VARIABLE_LISTING_BOOL(UseSpaceOutlines);
  DEBUG_VARIABLE_LISTING_V4(FauxV4);

#undef DEBUG_VARIABLE_LISTING_BOOL
#undef DEBUG_VARIABLE_LISTING_REAL32
#undef DEBUG_VARIABLE_LISTING_V4
   
  DebugState->RootGroup = Context.Group;
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


