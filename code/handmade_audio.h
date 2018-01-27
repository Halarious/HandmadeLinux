
typedef struct playing_sound playing_sound;
struct playing_sound
{
  r32 Volume[2];
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
