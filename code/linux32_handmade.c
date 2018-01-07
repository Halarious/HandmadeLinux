#include "handmade_platform.h"

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "linux32_handmade.h"

global_variable bool32 GlobalRunning = true;
global_variable bool32 GlobalPause   = false;
global_variable linux32_offscreen_buffer GlobalOffscreenBuffer = {};

internal void
InitScreenBuffer(DisplayInfo DisplayInfo,
		 linux32_offscreen_buffer *Buffer,
		 u32 Width, u32 Height)
{  
  Buffer->BitmapInfo = XCreateImage(DisplayInfo.Display, 
				    DisplayInfo.Visual, 
				    DisplayInfo.ScreenDepth, 
				    ZPixmap, 
				    0, 0, 
				    Width, 
				    Height, 
				    32, 0);
  Buffer->BitmapHandle = XCreatePixmap(DisplayInfo.Display, 
				       DisplayInfo.Window, 
				       Width, Height, 
				       DisplayInfo.ScreenDepth);
      
  Buffer->Width  = Width;
  Buffer->Height = Height;
  
  Buffer->BitmapInfo->width          = Buffer->Width;
  Buffer->BitmapInfo->height         = Buffer->Height;
  Buffer->BitmapInfo->format         = ZPixmap;
  Buffer->BitmapInfo->bits_per_pixel = 32;
  Buffer->BytesPerPixel              = (Buffer->BitmapInfo->bits_per_pixel)/8;
  Buffer->BitmapInfo->bytes_per_line = Width*Buffer->BytesPerPixel;
  
  Buffer->BitmapMemorySize  = Buffer->Width*Buffer->Height*Buffer->BytesPerPixel;
  Buffer->BitmapMemory      = mmap(0, Buffer->BitmapMemorySize,
				   PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
				   -1, 0);
  Buffer->BitmapInfo->data  = (char*)(Buffer->BitmapMemory);
  Buffer->Pitch             = Width*Buffer->BytesPerPixel;
}

internal struct timespec
SubtractTimeValues(struct timespec EndTime, struct timespec BeginTime)
{
  struct timespec Result = {};

  s64 BeginTimeNanos = BeginTime.tv_nsec;
  s64 EndTimeNanos   = EndTime.tv_nsec;
  if(BeginTimeNanos > EndTimeNanos)
    {
      Result.tv_nsec  = (time_t)(1000000000L - (BeginTimeNanos - EndTimeNanos));
      if(EndTime.tv_sec)
	{
	  EndTime.tv_sec -= 1;
	}
    }
  else
    {
      Result.tv_nsec = EndTimeNanos - BeginTimeNanos;
    }
  
  Result.tv_sec = EndTime.tv_sec - BeginTime.tv_sec;

  return(Result);
}

internal void
Linux32BeginRecordingInput(linux32_state *Linux32State, int InputRecordingIndex)
{
  Linux32State->InputRecordingIndex = InputRecordingIndex;

  char* Filename = "handmade.hmi";
  Linux32State->RecordingHandle =
    fopen(Filename, "w");

  size_t BytesToWrite = (size_t) Linux32State->TotalSize;
  Assert(Linux32State->TotalSize == BytesToWrite);
  size_t BytesWritten;
  BytesWritten =
    fwrite(Linux32State->MemoryBlock, BytesToWrite, 1,
	   Linux32State->RecordingHandle);
}

internal void
Linux32EndRecordingInput(linux32_state *Linux32State)
{
  fclose(Linux32State->RecordingHandle);
  Linux32State->InputRecordingIndex = 0;
}

internal void
Linux32BeginInputPlayback(linux32_state *Linux32State, int InputPlaybackIndex)
{
  Linux32State->InputPlaybackIndex = InputPlaybackIndex;
 
  char* Filename = "handmade.hmi";
  Linux32State->PlaybackHandle =
    fopen(Filename, "r");

  size_t BytesToRead = (size_t) Linux32State->TotalSize;
  Assert(Linux32State->TotalSize == BytesToRead);
  size_t BytesRead;
  BytesRead =
    fread( Linux32State->MemoryBlock, BytesToRead, 1,
	   Linux32State->RecordingHandle);  
}

internal void
Linux32EndInputPlayback(linux32_state *Linux32State)
{
  fclose(Linux32State->PlaybackHandle);
  Linux32State->InputPlaybackIndex = 0;
}

internal void
Linux32RecordInput(linux32_state *Linux32State, input *NewInput)
{
  ssize_t BytesWritten;
  BytesWritten = fwrite(NewInput, sizeof(*NewInput), 1,
			Linux32State->RecordingHandle);
}

internal void
Linux32PlaybackInput(linux32_state *Linux32State, input *NewInput)
{
  ssize_t BytesRead;
  BytesRead = fread(NewInput, sizeof(*NewInput), 1, Linux32State->RecordingHandle);
  if(BytesRead == 0)
    {
      int PlaybackIndex = Linux32State->InputPlaybackIndex;
      Linux32EndInputPlayback(Linux32State);
      Linux32BeginInputPlayback(Linux32State, PlaybackIndex);
      fread(NewInput, sizeof(*NewInput), 1,
	    Linux32State->RecordingHandle);
      
    }
}

internal void
Linux32ProcessKeyboardMessage(button_state* NewState,
			      bool32 IsDown)
{
  if(NewState->EndedDown != IsDown)
    {
      NewState->EndedDown = IsDown;
      ++NewState->HalfTransitionCount;
    }
}

internal void
HandleEvents(XEvent *Event, linux32_state *Linux32State,
	     controller_input *KeyboardController)
{
  int EventType = Event->type;
  switch(EventType)
    {
    case KeyRelease:
    case KeyPress:
      {
	bool32 IsDown = (EventType == KeyPress);

	KeySym KeySym;
	char TranslatedChar[8];
	//TODO: Gathering from some info this could be way worse than
	//      the windows equivalent Casey is using (maybe?), so it
	//      could be beneficial to look into something better although
	//      the, very little, research so far has been discouraging
	XLookupString(&Event->xkey,
		      TranslatedChar, sizeof(TranslatedChar),
		      &KeySym, 0);
	switch(KeySym)
	  {
	  case 'w':
	    {
	      Linux32ProcessKeyboardMessage(&KeyboardController->MoveUp,
					    IsDown);
	    } break;
	  case 'a':
	    {
	      Linux32ProcessKeyboardMessage(&KeyboardController->MoveLeft,
					    IsDown);
	    } break;
	  case 's':
	    {
	      Linux32ProcessKeyboardMessage(&KeyboardController->MoveDown,
					    IsDown);
	    } break;
	  case 'd':
	    {
	      Linux32ProcessKeyboardMessage(&KeyboardController->MoveRight,
					    IsDown);
	    } break;
	    case XK_Up:
	    {
	      Linux32ProcessKeyboardMessage(&KeyboardController->ActionUp,
					    IsDown);
	    } break;
	    case XK_Down:
	    {
	      Linux32ProcessKeyboardMessage(&KeyboardController->ActionDown,
					    IsDown);
	    } break;
	    case XK_Left:
	    {
	      Linux32ProcessKeyboardMessage(&KeyboardController->ActionLeft,
					    IsDown);
	    } break;
	    case XK_Right:
	    {
	      Linux32ProcessKeyboardMessage(&KeyboardController->ActionRight,
					    IsDown);
	    } break;
	    case XK_space:
	    {
	      Linux32ProcessKeyboardMessage(&KeyboardController->Start,
					    IsDown);
	    } break;
	  case 'L':
	    {
	      if(IsDown)
		{
		  if(Linux32State->InputRecordingIndex == 0)
		    {
		      Linux32BeginRecordingInput(Linux32State, 1);
		    }
		  else
		    {
		      Linux32EndRecordingInput(Linux32State);
		      Linux32BeginInputPlayback(Linux32State, 1);
		    }
		}
	    } break;
	  case 'P':
	    {
	      if(IsDown)
		{
		  GlobalPause = !GlobalPause;
		}
	    } break;
	  default:
	    {
	    } break;
	  }
      } break;
    default:
      {
      }
    }
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
  if(LoadedFile->Contents)
    {
      munmap(LoadedFile->Contents, LoadedFile->ContentsSize);
      LoadedFile->Contents     = 0;
      LoadedFile->ContentsSize = 0;
    }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
  loaded_file LoadedFile = {};
  
  int FileDescriptor = open(Filename, O_RDONLY);
  if(FileDescriptor != -1)
    {
      //TODO Stat returns off_t as the type for the size, apparently
      //     this is defined to be a signed integer but I have no clue
      //     if this supports sizes more than 4GB or what is going on
      //     here. There is an fstat64 but not sure if it is implicit
      //     now. Also why signed??? 
      struct stat FileInfo;
      if(fstat(FileDescriptor, &FileInfo) != -1)
	{
	  u64 FileSize = FileInfo.st_size;
	  LoadedFile.Contents = mmap(0, FileSize, PROT_READ|PROT_WRITE,
				     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	  if(LoadedFile.Contents != MAP_FAILED)
	    {
	      ssize_t BytesRead = read(FileDescriptor,
				       LoadedFile.Contents, FileSize);
	      if(BytesRead != -1  &&
		 BytesRead == FileSize)
		{
		  LoadedFile.ContentsSize = FileSize;
		}
	      else
		{
		  DEBUGPlatformFreeFileMemory(Thread, &LoadedFile);
		}
	    }
	  close(FileDescriptor);
	}
    }
  
  return(LoadedFile);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
  bool32 Result = false;
  
  int FileDescriptor = open(Filename, O_WRONLY);
  if(FileDescriptor != -1)
    {
      size_t BytesWritten;
      BytesWritten = write(FileDescriptor, Memory, MemorySize);
      if(BytesWritten != -1 )
	{
	  Result = (BytesWritten == MemorySize);
	}
      else
	{
	  
	}
      
      close(FileDescriptor);
    }

  return(Result);
}

int CopyFile(const char *from, const char *to)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
        return -1;

    fd_to = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_to < 0)
        goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0)
    {
        if (close(fd_to) < 0)
        {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        return 0;
    }

  out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return -1;
}

internal time_t
Linux32GetLastWriteTime(char* Filename)
{
  time_t Result = 0;

  struct stat FileAttributes;
  if(!stat(Filename, &FileAttributes))
    {
      //NOTE: These fields are available from linux 2.6+, for earlier
      //      versions there are back-compatible fields but we can say
      //      we don't support earlier than 2.6. The man pages say this
      //      is a struct timespec but it seems on th current machine it
      //      is treated as time_t
      Result = FileAttributes.st_mtime;
    }
  
  return(Result);
}

internal linux32_code
Linux32LoadCode(char* SourceSOName, char* TempSOName, char* LockFilename)
{
  linux32_code Result = {};
  if(access(LockFilename , F_OK) == -1)
    {
      Result.SOLastWriteTime =  
	Linux32GetLastWriteTime(SourceSOName);
  
      CopyFile(SourceSOName, TempSOName);
      Result.CodeSO = dlopen(TempSOName, RTLD_LAZY);
      if(Result.CodeSO)
	{
	  Result.UpdateAndRender = (update_and_render*)
	    dlsym(Result.CodeSO, "UpdateAndRender");
	  Result.IsValid = (Result.UpdateAndRender != 0);
	}
    }
  
  if(!Result.IsValid)
    {
      Result.UpdateAndRender = UpdateAndRenderStub;
    }
  
  return Result;
}

internal void
Linux32UnloadCode(linux32_code* Code)
{
  if(Code->CodeSO)
    {
      dlclose(Code->CodeSO);
      Code->CodeSO = 0;
    }

  Code->IsValid = false;
  Code->UpdateAndRender = UpdateAndRenderStub;
}

internal void
CatStrings(size_t SourceACount, char* SourceA,
	  size_t SourceBCount, char* SourceB,
	  size_t DestCount,    char* Dest)
{
  for(int Index = 0;
      Index < SourceACount;
      ++Index)
    {
      *Dest++ = *SourceA++;
    }
  for(int Index = 0;
      Index < SourceBCount;
      ++Index)
    {
      *Dest++ = *SourceB++;
    }

  *Dest++ = '\0';
}

void
DisplayBufferInWindow(DisplayInfo DisplayInfo, linux32_offscreen_buffer* Buffer)
{
  //NOTE TODO: This is here because we want a Y-is-up coordinate system
  //           and for now I see no other way (this could still be more
  //           efficent though).

  char* FlipBuffer = (char*)mmap(0, Buffer->BitmapMemorySize,
				 PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
				 -1, 0);
  for(s32 Y = (Buffer->Height - 1);
      Y >= 0;
      --Y)
    {
      void* MemoryLine = (((char*)Buffer->BitmapMemory)
			  + Y * Buffer->Pitch);
      void* BufferMemoryLine = (FlipBuffer
				+ ((Buffer->Height - 1) - Y) * Buffer->Pitch); 
      memmove(BufferMemoryLine, MemoryLine, Buffer->Pitch);
    }
  Buffer->BitmapInfo->data = FlipBuffer;
  
  XPutImage(DisplayInfo.Display,
	    GlobalOffscreenBuffer.BitmapHandle, 
	    DisplayInfo.GraphicsContext,
	    GlobalOffscreenBuffer.BitmapInfo,
	    0, 0, //NOTE Source
	    0, 0, //NOTE Dest
	    GlobalOffscreenBuffer.Width,
	    GlobalOffscreenBuffer.Height);

}

internal void
HandleDebugCycleCounters(memory* Memory)
{
  #if HANDMADE_INTERNAL
  printf("DEBUG CYCLE COUNTS: \n");
  for(s32 CounterIndex = 0;
      CounterIndex < ArrayCount(Memory->Counters);
      ++CounterIndex)
    {
      debug_cycle_counter* Counter = Memory->Counters + CounterIndex;

      if(Counter->HitCount)
	{
	  printf("\t%d: %luc %uh %luc/h\n",
		 CounterIndex,
		 Counter->CycleCount,
		 Counter->HitCount,
		 Counter->CycleCount / Counter->HitCount);

	  Counter->CycleCount = 0;
	  Counter->HitCount = 0;
	}
    }
  #endif
}

typedef struct
{
  char* StringToPrint;
} work_queue_entry;

global_variable u32 volatile EntryCompletionCount = 0;
global_variable u32 volatile NextEntryToDo = 0;
global_variable u32 volatile EntryCount;
work_queue_entry Entries[256];

#define CompletePastWritesBeforeFutureWrites asm volatile("" ::: "memory"); _mm_sfence();
#define CompletePastReadsBeforeFutureReads   asm volatile("" ::: "memory");

internal void
PushString(sem_t* SemaphoreHandle, char* String)
{
  Assert(EntryCount < ArrayCount(Entries));

  work_queue_entry* Entry = Entries + EntryCount;
  Entry->StringToPrint = String;

  CompletePastWritesBeforeFutureWrites;
  
  ++EntryCount;

  sem_post(SemaphoreHandle);
}

typedef struct
{
  sem_t* SemaphoreHandle;
  u32    LogicalThreadIndex;
} linux32_thread_info;

#if COMPILER_MSVC
#elif COMPILER_LLVM
#else
#endif

void*
ThreadProc(void* lpParameter)
{
  linux32_thread_info* ThreadInfo = (linux32_thread_info*) lpParameter;

  for(;;)
    {
      if(NextEntryToDo < EntryCount)
	{
	  u32 EntryIndex = __sync_add_and_fetch(&NextEntryToDo, 1) - 1;
	  CompletePastReadsBeforeFutureReads;
	  work_queue_entry* Entry = Entries + EntryIndex;

	  char Buffer[256];
	  sprintf(Buffer, "Thread %u: %s\n", ThreadInfo->LogicalThreadIndex, Entry->StringToPrint);
	  
	  puts(Buffer);

	  __sync_add_and_fetch(&EntryCompletionCount, 1);
	}
      else
	{
	  sem_wait(ThreadInfo->SemaphoreHandle);
	}
    }

  return(0);
}

int
main(int ArgCount, char** Arguments)
{
  linux32_state Linux32State = {};

  linux32_thread_info ThreadInfo[4];
  sem_t SemaphoreHandle;
  
  u32 InitialCount = 0;
  u32 ThreadCount  = ArrayCount(ThreadInfo); 
  u32 SemaphoreResult = sem_init (&SemaphoreHandle, 0, InitialCount);

  Assert(!SemaphoreResult);
  
  for(u32 Index = 0;
      Index < ArrayCount(ThreadInfo);
      ++Index)
    {
      linux32_thread_info* Info = ThreadInfo + Index;
      Info->SemaphoreHandle    = &SemaphoreHandle;
      Info->LogicalThreadIndex = Index; 

      pthread_t ThreadID;
      pthread_create(&ThreadID, 0, &ThreadProc, Info);
    }

  PushString(&SemaphoreHandle, "String A0\n");
  PushString(&SemaphoreHandle, "String A1\n");
  PushString(&SemaphoreHandle, "String A2\n");
  PushString(&SemaphoreHandle, "String A3\n");
  PushString(&SemaphoreHandle, "String A4\n");
  PushString(&SemaphoreHandle, "String A5\n");
  PushString(&SemaphoreHandle, "String A6\n");
  PushString(&SemaphoreHandle, "String A7\n");
  PushString(&SemaphoreHandle, "String A8\n");
  PushString(&SemaphoreHandle, "String A9\n");
  PushString(&SemaphoreHandle, "String A10\n");

  sleep(5);
  
  PushString(&SemaphoreHandle, "String B0\n");
  PushString(&SemaphoreHandle, "String B1\n");
  PushString(&SemaphoreHandle, "String B2\n");
  PushString(&SemaphoreHandle, "String B3\n");
  PushString(&SemaphoreHandle, "String B4\n");
  PushString(&SemaphoreHandle, "String B5\n");
  PushString(&SemaphoreHandle, "String B6\n");
  PushString(&SemaphoreHandle, "String B7\n");
  PushString(&SemaphoreHandle, "String B8\n");
  PushString(&SemaphoreHandle, "String B9\n");
  PushString(&SemaphoreHandle, "String B10\n");

  while(EntryCount != EntryCompletionCount);
  
  char ELFFilename[PATH_MAX];
  ssize_t SizeOfFilename = readlink("/proc/self/exe", ELFFilename, sizeof(ELFFilename));  
  char* OnePastLastSlash = ELFFilename;
  for(char* Scan = ELFFilename;
      *Scan;
      ++Scan)
    {
      if(*Scan == '/')
	{
	  OnePastLastSlash = Scan + 1;
	}
    }

  char SourceCodeSOFilename[] = "handmade.so";
  char SourceCodeSOFullPath[PATH_MAX];
  CatStrings(OnePastLastSlash - ELFFilename, ELFFilename,
	     sizeof(SourceCodeSOFilename) - 1, SourceCodeSOFilename,
	     sizeof(SourceCodeSOFullPath),     SourceCodeSOFullPath);

  char TempCodeSOFilename[] = "handmade_temp.so";
  char TempCodeSOFullPath[PATH_MAX];
  CatStrings(OnePastLastSlash - ELFFilename, ELFFilename,
	     sizeof(TempCodeSOFilename) - 1, TempCodeSOFilename,
	     sizeof(TempCodeSOFullPath),     TempCodeSOFullPath);  

  char LockFilename[] = "lock.tmp";
  char LockFullPath[PATH_MAX];
  CatStrings(OnePastLastSlash - ELFFilename, ELFFilename,
	     sizeof(LockFilename) - 1, LockFilename,
	     sizeof(LockFullPath),     LockFullPath);  
  
  DisplayInfo DisplayInfo = {};
  DisplayInfo.Display         = XOpenDisplay(NULL);
  DisplayInfo.Screen          = DefaultScreen(DisplayInfo.Display);
  int w = 960;//DisplayWidth(DisplayInfo.Display, DisplayInfo.Screen)/2;
  int h = 540;//DisplayHeight(DisplayInfo.Display, DisplayInfo.Screen)/2;
  DisplayInfo.RootWindow      = RootWindow(DisplayInfo.Display, DisplayInfo.Screen);
  DisplayInfo.Visual          = DefaultVisual(DisplayInfo.Display, DisplayInfo.Screen);
  DisplayInfo.ScreenDepth     = DefaultDepth(DisplayInfo.Display, DisplayInfo.Screen); 
  DisplayInfo.GraphicsContext = DefaultGC(DisplayInfo.Display, DisplayInfo.Screen);
  DisplayInfo.Window          = XCreateWindow(DisplayInfo.Display,
					      DisplayInfo.RootWindow,
					      0, 0,
					      w, h, 0,
					      DisplayInfo.ScreenDepth, 
					      InputOutput,
					      DisplayInfo.Visual,
					      0, 0);
  XColor Color;
  XParseColor(DisplayInfo.Display,
	      DefaultColormap(DisplayInfo.Display, 0), "#000000",
	      &Color);
  XAllocColor(DisplayInfo.Display,
	      DefaultColormap(DisplayInfo.Display, 0),
	      &Color);
  XSetWindowBackground(DisplayInfo.Display,
		       DisplayInfo.Window,
		       Color.pixel);
  
  //TODO: How to reliably query this on Linux Xlib? Similar problem to
  //      Caseys' on Win
  int MonitorRefreshHz = 60;
  XRRScreenConfiguration *ScreenConfig = XRRGetScreenInfo(DisplayInfo.Display,
							  DisplayInfo.RootWindow);
  short Linux32RefreshRate = XRRConfigCurrentRate(ScreenConfig);
  if(Linux32RefreshRate > 0)
    {
      MonitorRefreshHz = Linux32RefreshRate;
    }
  r32 GameUpdateHz     = MonitorRefreshHz / 2.0f; 
  r32 TargetSecondsPerFrame = 1.0f / (r32)GameUpdateHz;
  //TODO Apparently Windows is a unsigned value, so we need to
  //     check for any errors in the XCreateWindow call differently
  if(DisplayInfo.Window != BadAlloc)
    {
      Atom wmDelete = XInternAtom(DisplayInfo.Display,
				  "WM_DELETE_WINDOW", True);
      XSetWMProtocols (DisplayInfo.Display, DisplayInfo.Window,
		       &wmDelete, 1);
      XStoreName      (DisplayInfo.Display, DisplayInfo.Window,
		       "Handmade Window");
      XSelectInput(DisplayInfo.Display, DisplayInfo.Window,
		   KeyPressMask    | KeyReleaseMask    | 
		   ButtonPressMask | ButtonReleaseMask |
		   PointerMotionMask);
      XMapWindow(DisplayInfo.Display, DisplayInfo.Window);
      
#if HANDMADE_INTERNAL
      void* BaseAddress = (void*) Terabytes(2);
#else
      void* BaseAddress = 0;
#endif

      memory Memory = {};
      Memory.PermanentStorageSize = Megabytes(256);
      Memory.TransientStorageSize = Gigabytes((u64)1);
      Memory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
      Memory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
      Memory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

      Linux32State.TotalSize   = Memory.PermanentStorageSize + Memory.TransientStorageSize;
      Linux32State.MemoryBlock = mmap(BaseAddress, Linux32State.TotalSize,
				     PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
				     -1, 0);
      Memory.PermanentStorage = Linux32State.MemoryBlock;
      Memory.TransientStorage = (u8*)Memory.PermanentStorage + Memory.PermanentStorageSize;

      if(Memory.PermanentStorage != MAP_FAILED)
	{  
	  InitScreenBuffer(DisplayInfo, &GlobalOffscreenBuffer,
			   w, h);
	  input Input[2] = {};
	  input* NewInputState = &Input[0];
	  input* OldInputState = &Input[1];
	  
	  linux32_code Code = Linux32LoadCode(SourceCodeSOFullPath,
					      TempCodeSOFullPath,
					      LockFullPath);
	  
	  //TODO Might want to change this to RDTSC using an asm
	  //     instruction, we seem to not be getting _super_ precise
	  //     but it may also be unnecessary for this purpose. It even
	  //     looks like it's not the greatest idea to use rdtsc for actual
	  //     time calculations because of inconsistent clock rate and
	  //     whatnot.
	  struct timespec LastTime;
	  clock_gettime(CLOCK_MONOTONIC, &LastTime);
	  //TODO: This is LLVM only obviously
	  s64 LastCycleCount = __builtin_readcyclecounter();
	  while(GlobalRunning)
	    {
	      NewInputState->dtForFrame = TargetSecondsPerFrame;
	      NewInputState->ExecutableReloaded = false;
	      
	      time_t NewSOLastWriteTime = Linux32GetLastWriteTime(SourceCodeSOFullPath);
	      if(NewSOLastWriteTime != Code.SOLastWriteTime)
		{
		  Linux32UnloadCode(&Code);
		  Code = Linux32LoadCode(SourceCodeSOFullPath,
					 TempCodeSOFullPath,
					 LockFullPath);
		  NewInputState->ExecutableReloaded = true;
		}
	      
	      controller_input *OldKeyboardController = GetController(OldInputState, 0);
	      controller_input *NewKeyboardController = GetController(NewInputState, 0);
	      controller_input ZeroController = {};
	      *NewKeyboardController = ZeroController;
	      for(int ButtonIndex = 0;
		  ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
		  ++ButtonIndex)
		{
		  NewKeyboardController->Buttons[ButtonIndex].EndedDown =
		    OldKeyboardController->Buttons[ButtonIndex].EndedDown;
		}
	      
	      while(XPending(DisplayInfo.Display))
		{
		  XEvent Event = {};
		  XNextEvent(DisplayInfo.Display, &Event);
		  HandleEvents(&Event, &Linux32State, NewKeyboardController);
		}
	      
	      if(!GlobalPause)
		{
		  {
		    u32    MaskReturn;
		    int    IgnoredInt;
		    Window IgnoredWindow;
		    XQueryPointer(DisplayInfo.Display, DisplayInfo.Window,
				  &IgnoredWindow,         //Window *root_return,
				  &IgnoredWindow,         //Window *child_return,
				  &NewInputState->MouseX, //int *root_x_return,
				  &NewInputState->MouseY, //int *root_y_return,
				  &IgnoredInt,		  //int *win_x_return,
				  &IgnoredInt,            //int *win_y_return,
				  &MaskReturn);

		    Linux32ProcessKeyboardMessage(&NewInputState->MouseButtons[0],
						  MaskReturn & Button1Mask);
		    Linux32ProcessKeyboardMessage(&NewInputState->MouseButtons[1],
						  MaskReturn & Button2Mask);
		    Linux32ProcessKeyboardMessage(&NewInputState->MouseButtons[2],
						  MaskReturn & Button3Mask);
		    Linux32ProcessKeyboardMessage(&NewInputState->MouseButtons[3],
						  MaskReturn & Button4Mask);
		    Linux32ProcessKeyboardMessage(&NewInputState->MouseButtons[4],
						  MaskReturn & Button5Mask);

		  }
		  
		  thread_context Thread = {};

		  offscreen_buffer Buffer = {};
		  Buffer.Memory = GlobalOffscreenBuffer.BitmapMemory;
		  Buffer.Width  = GlobalOffscreenBuffer.Width;
		  Buffer.Height = GlobalOffscreenBuffer.Height;
		  Buffer.Pitch  = GlobalOffscreenBuffer.Pitch;

		  if(Linux32State.InputRecordingIndex)
		    {
		      Linux32RecordInput(&Linux32State, NewInputState);
		    }
		  if(Linux32State.InputPlaybackIndex)
		    {
		      Linux32PlaybackInput(&Linux32State, NewInputState);
		    }
		  if(Code.UpdateAndRender)
		    {
		      Code.UpdateAndRender(&Thread, &Memory, NewInputState, &Buffer);
		      HandleDebugCycleCounters(&Memory);
		    }
		  
		  DisplayBufferInWindow(DisplayInfo, &GlobalOffscreenBuffer);

		  struct timespec EndTime; 
		  clock_gettime(CLOCK_MONOTONIC, &EndTime);

		  struct timespec ElapsedTime = SubtractTimeValues(EndTime, LastTime);
		  r32 ElapsedTimeSeconds      = (r32)ElapsedTime.tv_nsec / 1000000000.0f; 
		  //TODO See what happens if for some reason we miss (a) frame(s),
		  //     and what is expected as a framerate if it even matters.
		  if(ElapsedTimeSeconds < TargetSecondsPerFrame)
		    {
		      //TODO The code here sleeps for 1ms less than it should so we guarantee the spinlock for a bit and hit
		      //     very close to 33.33ms of wait time, like in Caseys' example. Win sleep is a bit different (DWORD for ms time)
		      //     and it would appear that linux is more precise but I couldn't figure out why I was getting like .10+ ms
		      //     off all he time while using the sleep (granularity give back nano precision so it shouldn't happen),
		      //     but for now it is as it is and later we should do something about this to make it sane and get rid of
		      //     the spinlock
		      r32 Epsilon = 1000000.0f;
		      
		      struct timespec SleepTime = {};
		      struct timespec RemainingSleepTime = {};
		      r32 SleepTimeInSeconds = TargetSecondsPerFrame - ElapsedTimeSeconds;
		      SleepTime.tv_sec  = FloorReal32ToInt32(SleepTimeInSeconds);
		      SleepTime.tv_nsec = ((SleepTimeInSeconds - SleepTime.tv_sec) * 1000000000.0f) - Epsilon;
		      clock_nanosleep(CLOCK_MONOTONIC, 0, &SleepTime, &RemainingSleepTime);

		      if(RemainingSleepTime.tv_sec ||
			 RemainingSleepTime.tv_nsec)
			{
			  printf("Missed sleep\n\t");
			}
		      
		      while(ElapsedTimeSeconds < TargetSecondsPerFrame)
			{		      
			  clock_gettime(CLOCK_MONOTONIC, &EndTime);
			  ElapsedTime      = SubtractTimeValues(EndTime, LastTime);
			  ElapsedTimeSeconds = (r32)ElapsedTime.tv_nsec/1000000000.0f; 
			}
		    }
		  else
		    {
		      printf("Missed a frame\n\t");
		    }
		  
		  
		  clock_gettime(CLOCK_MONOTONIC, &EndTime);	    
#if 0
		  s64 EndCycleCount = __builtin_readcyclecounter();	      
		  s64 CyclesElapsed = EndCycleCount - LastCycleCount;
		  LastCycleCount = EndCycleCount;

		  ElapsedTime = SubtractTimeValues(EndTime, LastTime);
		  printf("%.2fms/f, %uMc/f \n", (r32)ElapsedTime.tv_nsec/1000000.0f, CyclesElapsed / (1000*1000));
#endif

		  LastTime = EndTime;
		  
		    
		  //TODO: Find a way to stop flickering caused by clearing the screen
		  //      to blackness
		  int OffsetX = 10;
		  int OffsetY = 10;
		  //XClearWindow(DisplayInfo.Display, DisplayInfo.Window);
		  XClearArea(DisplayInfo.Display, DisplayInfo.Window,
			     0, 0, w, OffsetY, false);
		  XClearArea(DisplayInfo.Display, DisplayInfo.Window,
			     0, OffsetY + Buffer.Height,
			     w, h, false);
		  XClearArea(DisplayInfo.Display, DisplayInfo.Window,
			     0, 0, OffsetX, h, false);
		  XClearArea(DisplayInfo.Display, DisplayInfo.Window,
			     OffsetX + Buffer.Width, 0, w, h, false);
		  XCopyArea(DisplayInfo.Display,
			    GlobalOffscreenBuffer.BitmapHandle,
			    DisplayInfo.Window,
			    DisplayInfo.GraphicsContext,
			    0, 0,
			    GlobalOffscreenBuffer.Width,
			    GlobalOffscreenBuffer.Height,
			    OffsetX, OffsetY);

		  input *tempInput = OldInputState;
		  OldInputState    = NewInputState;
		  NewInputState    = tempInput;
		}
	    }
	}
      else
	{
	  //TODO: Logging
	}
    }
  else
    {
      //TODO: Logging
    }

  return(0);
}
