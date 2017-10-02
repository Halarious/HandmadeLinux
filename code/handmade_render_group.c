
internal render_group*
AllocateRenderGroup(memory_arena *Arena, u32 MaxPushBufferSize,
		    r32 MetersToPixels)
{
  render_group* Result = PushStruct(Arena, render_group);
  Result->PushBufferBase = PushSize(Arena, MaxPushBufferSize);

  Result->DefaultBasis = PushStruct(Arena, render_basis);
  Result->DefaultBasis->P = V3(0.0f, 0.0f, 0.0f);
  Result->MetersToPixels = MetersToPixels;
  Result->Count = 0;
  
  Result->MaxPushBufferSize = MaxPushBufferSize;
  Result->PushBufferSize = 0;
  
  return(Result);
}
