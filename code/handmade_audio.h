
typedef struct playing_sound playing_sound;
struct playing_sound
{
  v2 CurrentVolume;
  v2 dCurrentVolume;
  v2 TargetVolume;
  
  sound_id ID;
  s32 SamplesPlayed;
  playing_sound* Next;
};

typedef struct
{
  memory_arena *PermArena;
  playing_sound *FirstPlayingSound;
  playing_sound *FirstFreePlayingSound;
} audio_state; 
