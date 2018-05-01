
timed_block
ConstructTimedBlock(int Counter, char* FileName, int LineNumber, char* FunctionName, int HitCount)
{
  debug_record* Record = DebugRecordArray + Counter;
  Record->FileName = FileName;
  Record->FunctionName = FunctionName;
  Record->LineNumber = LineNumber;
  Record->CycleCount -= __builtin_readcyclecounter();
  Record->HitCount += HitCount;
  
  timed_block Result = {Record};
  return(Result);
}

void
DestructTimedBlock(timed_block Block)
{
  Block.Record->CycleCount += __builtin_readcyclecounter();
}


