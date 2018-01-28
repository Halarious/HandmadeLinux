
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
  PlayingSound->CurrentVolume = PlayingSound->TargetVolume = V2(1.0f, 1.0f);
  PlayingSound->dCurrentVolume = V2(0.0f, 0.0f);
  PlayingSound->ID = SoundID;

  PlayingSound->Next = AudioState->FirstPlayingSound;
  AudioState->FirstPlayingSound = PlayingSound;

  return(PlayingSound);
}

internal void
ChangeVolume(audio_state* AudioState, playing_sound* Sound,
	     r32 FadeDurationInSeconds, v2 Volume)
{
  if(FadeDurationInSeconds <= 0.0f)
    {
      Sound->CurrentVolume = Sound->TargetVolume = Volume;
    }
  else
    {
      r32 OneOverFade = 1.0f / FadeDurationInSeconds;
      Sound->TargetVolume = Volume;
      Sound->dCurrentVolume = V2MulS(OneOverFade,
				     V2Sub(Sound->TargetVolume, Sound->CurrentVolume));
    }
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

  r32 SecondsPerSample = 1.0f / (r32)SoundBuffer->SamplesPerSecond;
#define AudioStateOutputChannelCount 2
  
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
	  
	      v2 Volume  = PlayingSound->CurrentVolume;
	      v2 dVolume = V2MulS(SecondsPerSample, PlayingSound->dCurrentVolume);
		
	      Assert(PlayingSound->SamplesPlayed >= 0);

	      u32 SamplesToMix = TotalSamplesToMix;
	      u32 SamplesRemainingInSound = LoadedSound->SampleCount - PlayingSound->SamplesPlayed;
	      if(SamplesToMix > SamplesRemainingInSound)
		{
		  SamplesToMix = SamplesRemainingInSound;
		}

	      bool32 VolumeEnded[AudioStateOutputChannelCount] = {};
	      for(u32 ChannelIndex = 0;
		  ChannelIndex < ArrayCount(VolumeEnded);
		  ++ChannelIndex)
		{
		  if(dVolume.E[ChannelIndex] != 0.0f)
		    {
		      r32 DeltaVolume = (PlayingSound->TargetVolume.E[ChannelIndex]
					 - Volume.E[ChannelIndex]);
		      u32 VolumeSampleCount = (u32)((DeltaVolume / dVolume.E[ChannelIndex]) + 0.5f);
		      if(SamplesToMix > VolumeSampleCount)
			{
			  SamplesToMix = VolumeSampleCount;
			  VolumeEnded[ChannelIndex] = true;
			}
		    }
		}
	      	      
	      for(u32 SampleIndex = PlayingSound->SamplesPlayed;
		  SampleIndex < (PlayingSound->SamplesPlayed + SamplesRemainingInSound);
		  ++SampleIndex)
		{
		  r32 SampleValue = LoadedSound->Samples[0][SampleIndex];
		  *Dest0++ = Volume.E[0]*SampleValue;
		  *Dest1++ = Volume.E[1]*SampleValue;

		  Volume = V2Add(Volume, dVolume);
		}

	      PlayingSound->CurrentVolume = Volume;
	      
	      for(u32 ChannelIndex = 0;
		  ChannelIndex < ArrayCount(VolumeEnded);
		  ++ChannelIndex)
		{
		  if(VolumeEnded[ChannelIndex])
		    {
		      PlayingSound->CurrentVolume.E[ChannelIndex] =
			PlayingSound->TargetVolume.E[ChannelIndex];
		      PlayingSound->dCurrentVolume.E[ChannelIndex] = 0.0f;
		    }
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

