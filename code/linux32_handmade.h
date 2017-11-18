//TODO: Do we want this here?
#include "handmade_intrinsics.h"

typedef struct
{
  Display* Display;
  Visual*  Visual;
  Window   RootWindow;
  Window   Window;
  GC       GraphicsContext;
  s32      Screen;
  s32      ScreenDepth;
} DisplayInfo;

typedef struct
{
  XImage *BitmapInfo;
  Pixmap BitmapHandle;
  void   *BitmapMemory;

  int BitmapMemorySize;
  int BytesPerPixel;
  int Width;
  int Height;
  int Pitch;
} linux32_offscreen_buffer;

typedef struct
{
  void* CodeSO;
  time_t SOLastWriteTime;
  update_and_render* UpdateAndRender;

  bool32 IsValid;
} linux32_code;

typedef struct
{
  u64 TotalSize;
  void* MemoryBlock;

  FILE *RecordingHandle;
  int InputRecordingIndex;
  
  FILE *PlaybackHandle;
  int InputPlaybackIndex;
} linux32_state;
