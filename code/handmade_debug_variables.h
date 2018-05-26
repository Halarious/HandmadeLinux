
typedef struct
{
  debug_state* State;
  memory_arena* Arena;

  debug_variable_reference* Group;
} debug_variable_definition_context; 

internal debug_variable*
DEBUGAddUnreferencedVariable(debug_state* DebugState, debug_variable_type Type, char* Name)
{
  debug_variable* Var = PushStruct(&DebugState->DebugArena, debug_variable);  
  Var->Type = Type;
  Var->Name = (char*) PushCopy(&DebugState->DebugArena, StringLength(Name) + 1, Name);

  return(Var);
}

internal debug_variable_reference* 
DEBUGAddVariableReference(debug_state* DebugState, debug_variable_reference* GroupRef, debug_variable* Var)
{
  debug_variable_reference* Ref = PushStruct(&DebugState->DebugArena, debug_variable_reference);  
  Ref->Var = Var;
  Ref->Next = 0;

  Ref->Parent = GroupRef;
  debug_variable* Group = Ref->Parent ? Ref->Parent->Var : 0;
  
  if(Group)
    {
      if(Group->Group.LastChild)
	{
	  Group->Group.LastChild = Group->Group.LastChild->Next = Ref;
	}
      else
	{
	  Group->Group.LastChild = Group->Group.FirstChild = Ref;
	}
    }

  return(Ref);
}

internal debug_variable_reference* 
DEBUGAddVariableReferenceWithContext(debug_variable_definition_context* Context, debug_variable* Var)
{
  debug_variable_reference* Ref = DEBUGAddVariableReference(Context->State, Context->Group, Var);  

  return(Ref);
}

internal debug_variable_reference* 
DEBUGAddVariable(debug_variable_definition_context* Context, debug_variable_type Type, char* Name)
{
  debug_variable* Var = DEBUGAddUnreferencedVariable(Context->State, Type, Name);
  debug_variable_reference* Ref = DEBUGAddVariableReferenceWithContext(Context, Var);

  return(Ref);
}

internal debug_variable*
DEBUGAddRootGroupInternal(debug_state* DebugState, char* Name)
{
  debug_variable* Group = DEBUGAddUnreferencedVariable(DebugState, DebugVariable_Group, Name);

  Group->Group.Expanded = true;
  Group->Group.FirstChild = Group->Group.LastChild = 0;
  
  return(Group);
}

internal debug_variable_reference*
DEBUGAddRootGroup(debug_state* DebugState, char* Name)
{
  debug_variable_reference* GroupRef =
    DEBUGAddVariableReference(DebugState, 0,
			      DEBUGAddRootGroupInternal(DebugState, Name));
  return(GroupRef);
}

internal debug_variable_reference*
DEBUGBeginVariableGroup(debug_variable_definition_context* Context, char* Name)
{
  debug_variable_reference* Group =
    DEBUGAddVariableReferenceWithContext(Context,
					 DEBUGAddRootGroupInternal(Context->State, Name));    
  Group->Var->Group.Expanded = false;

  Context->Group = Group;

  return(Group);
}

internal debug_variable_reference*
DEBUGAddBoolVariable(debug_variable_definition_context* Context, char* Name, bool32 Value)
{
  debug_variable_reference* Ref = DEBUGAddVariable(Context, DebugVariable_Bool32, Name);
  Ref->Var->Bool32 = Value;

  return(Ref);
}

internal debug_variable_reference*
DEBUGAddReal32Variable(debug_variable_definition_context* Context, char* Name, r32 Value)
{
  debug_variable_reference* Ref = DEBUGAddVariable(Context, DebugVariable_Real32, Name);
  Ref->Var->Real32 = Value;

  return(Ref);
}

internal debug_variable_reference*
DEBUGAddV4Variable(debug_variable_definition_context* Context, char* Name, v4 Value)
{
  debug_variable_reference* Ref = DEBUGAddVariable(Context, DebugVariable_V4, Name);
  Ref->Var->Vector4 = Value;

  return(Ref);
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

  debug_variable_reference* UseDebugCamRef = 0;
  
#define DEBUG_VARIABLE_LISTING_BOOL(Name) DEBUGAddBoolVariable(Context, #Name, DEBUGUI_##Name)
#define DEBUG_VARIABLE_LISTING_REAL32(Name) DEBUGAddReal32Variable(Context, #Name, DEBUGUI_##Name)
#define DEBUG_VARIABLE_LISTING_V4(Name) DEBUGAddV4Variable(Context, #Name, DEBUGUI_##Name)

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

  DEBUGAddVariableReferenceWithContext(Context, UseDebugCamRef->Var);
#undef DEBUG_VARIABLE_LISTING_BOOL
#undef DEBUG_VARIABLE_LISTING_REAL32
#undef DEBUG_VARIABLE_LISTING_V4
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


