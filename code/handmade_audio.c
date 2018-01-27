
internal playing_sound*
PlaySound(audio_state* AudioState, sound_id SoundID)
{
  if(!AudioState->FirstFreePlayingSound)
    {
      AudioState->FirstFreePlayingSound = PushStruct(AudioState->PermArena, playing_sound);   
      AudioState->FirstFreePlayingSound->Next = 0;
    }
  
  playing_sound* PlayingSound = AudioState->FirstFreePlayingSound;
  AudioState->FirstFreePlayingSound = PlayingSound->Next;

  PlayingSound->SamplesPlayed = 0;
  PlayingSound->Volume[0] = 1.0f;
  PlayingSound->Volume[1] = 1.0f;
  PlayingSound->ID = SoundID;

  PlayingSound->Next = AudioState->FirstPlayingSound;
  AudioState->FirstPlayingSound = PlayingSound;

  return(PlayingSound);
}

internal void
OutputPlayingSounds(audio_state* AudioState,
		    sound_output_buffer* SoundBuffer,
		    assets* Assets,
		    memory_arena* TempArena)
{  
  temporary_memory MixerMemory = BeginTemporaryMemory(TempArena);
  
  r32* RealChannel0 = PushArray(TempArena, SoundBuffer->SampleCount, r32);
  r32* RealChannel1 = PushArray(TempArena, SoundBuffer->SampleCount, r32);

  {
    r32* Dest0 = RealChannel0;
    r32* Dest1 = RealChannel1;
    for(u32 SampleIndex = 0;
	SampleIndex < SoundBuffer->SampleCount;
	++SampleIndex)
      {
	*Dest0++ = 0.0f;
	*Dest1++ = 0.0f;
      }
  }

  for(playing_sound** PlayingSoundPtr = &AudioState->FirstPlayingSound;
      *PlayingSoundPtr;
      )
    {
      playing_sound* PlayingSound = *PlayingSoundPtr;
      bool32 SoundFinished = false;

      u32 TotalSamplesToMix = SoundBuffer->SampleCount;
      r32* Dest0 = RealChannel0;
      r32* Dest1 = RealChannel1;

      while(TotalSamplesToMix && !SoundFinished)
	{
	  loaded_sound* LoadedSound = GetSound(Assets, PlayingSound->ID);
	  if(LoadedSound)
	    {
	      asset_sound_info* Info = GetSoundInfo(Assets, PlayingSound->ID);
	      PrefetchSound(Assets, Info->NextIDToPlay);
	  
	      r32 Volume0 = PlayingSound->Volume[0];
	      r32 Volume1 = PlayingSound->Volume[1];

	      Assert(PlayingSound->SamplesPlayed >= 0);

	      u32 SamplesToMix = TotalSamplesToMix;
	      u32 SamplesRemainingInSound = LoadedSound->SampleCount - PlayingSound->SamplesPlayed;
	      if(SamplesToMix > SamplesRemainingInSound)
		{
		  SamplesToMix = SamplesRemainingInSound;
		}
	      
	      for(u32 SampleIndex = PlayingSound->SamplesPlayed;
		  SampleIndex < (PlayingSound->SamplesPlayed + SamplesRemainingInSound);
		  ++SampleIndex)
		{
		  r32 SampleValue = LoadedSound->Samples[0][SampleIndex];
		  *Dest0++ = Volume0*SampleValue;
		  *Dest1++ = Volume1*SampleValue;
		}

	      Assert(TotalSamplesToMix >= SamplesToMix);
	      PlayingSound->SamplesPlayed += SamplesToMix;	      
	      TotalSamplesToMix -= SamplesToMix;
	      
	      if(((u32)PlayingSound->SamplesPlayed == LoadedSound->SampleCount))
		{
		  if(IsSoundIDValid(Info->NextIDToPlay))
		    {
		      PlayingSound->ID = Info->NextIDToPlay;
		      PlayingSound->SamplesPlayed = 0;
		    }
		  else
		    {
		      SoundFinished = true;
		    }
		}
	      else
		{
		  Assert(TotalSamplesToMix == 0);
		}
	    }
	  else
	    {
	      LoadSound(Assets, PlayingSound->ID);
	      break;
	    }
	}
       
      if(SoundFinished)
	{
	  *PlayingSoundPtr = PlayingSound->Next;
	  PlayingSound->Next = AudioState->FirstFreePlayingSound;
	  AudioState->FirstFreePlayingSound = PlayingSound;
	}
      else
	{
	  PlayingSoundPtr = &PlayingSound->Next;
	}
    }

  {
    r32* Source0 = RealChannel0;
    r32* Source1 = RealChannel1;
	  
    s16* SampleOut = SoundBuffer->Samples;
    for(u32 SampleIndex = 0;
	SampleIndex < SoundBuffer->SampleCount;
	++SampleIndex)
      {
	*SampleOut++ = (s16)(*Source0++ + 0.5f);
	*SampleOut++ = (s16)(*Source1++ + 0.5f);
      }
  }
  
  EndTemporaryMemory(MixerMemory);
}

internal void
InitializeAudioState(audio_state* AudioState, memory_arena* Arena)
{
  AudioState->PermArena = Arena;
  AudioState->FirstPlayingSound = 0;
  AudioState->FirstFreePlayingSound = 0;
}

