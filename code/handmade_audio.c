
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
  PlayingSound->dSample = 1.0f;

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
ChangePitch(audio_state* AudioState, playing_sound* Sound,
	    r32 dSample)
{
  Sound->dSample = dSample;
}

internal void
OutputPlayingSounds(audio_state* AudioState,
		    sound_output_buffer* SoundBuffer,
		    assets* Assets,
		    memory_arena* TempArena)
{  
  temporary_memory MixerMemory = BeginTemporaryMemory(TempArena);

  u32 SampleCountAlign4 = Align4(SoundBuffer->SampleCount);
  u32 SampleCount4 = SampleCountAlign4 / 4;
  
  __m128* RealChannel0 = PushArrayA(TempArena, SampleCount4, __m128, 16);
  __m128* RealChannel1 = PushArrayA(TempArena, SampleCount4, __m128, 16);

  r32 SecondsPerSample = 1.0f / (r32)SoundBuffer->SamplesPerSecond;
#define AudioStateOutputChannelCount 2

  __m128 Zero = _mm_set1_ps(0.0f);;
  {
    __m128* Dest0 = RealChannel0;
    __m128* Dest1 = RealChannel1;
    for(u32 SampleIndex = 0;
	SampleIndex < SampleCount4;
	++SampleIndex)
      {
	_mm_store_ps((r32*)Dest0++, Zero);
	_mm_store_ps((r32*)Dest1++, Zero);
      }
  }

  for(playing_sound** PlayingSoundPtr = &AudioState->FirstPlayingSound;
      *PlayingSoundPtr;
      )
    {
      playing_sound* PlayingSound = *PlayingSoundPtr;
      bool32 SoundFinished = false;

      u32 TotalSamplesToMix = SoundBuffer->SampleCount;
      r32* Dest0 = (r32*)RealChannel0;
      r32* Dest1 = (r32*)RealChannel1;

      while(TotalSamplesToMix && !SoundFinished)
	{
	  loaded_sound* LoadedSound = GetSound(Assets, PlayingSound->ID);
	  if(LoadedSound)
	    {
	      asset_sound_info* Info = GetSoundInfo(Assets, PlayingSound->ID);
	      PrefetchSound(Assets, Info->NextIDToPlay);
	  
	      v2 Volume  = PlayingSound->CurrentVolume;
	      v2 dVolume = V2MulS(SecondsPerSample, PlayingSound->dCurrentVolume);
	      r32 dSample = PlayingSound->dSample;
		
	      Assert(PlayingSound->SamplesPlayed >= 0);

	      u32 SamplesToMix = TotalSamplesToMix;
	      r32 RealSamplesRemainingInSound =
		(LoadedSound->SampleCount - RoundReal32ToInt32(PlayingSound->SamplesPlayed)) / dSample;
	      u32 SamplesRemainingInSound = RoundReal32ToInt32(RealSamplesRemainingInSound);
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

	      r32 SamplePosition = PlayingSound->SamplesPlayed;
	      for(u32 LoopIndex = 0;
		  LoopIndex < SamplesToMix;
		  ++LoopIndex)
		{
#if 0
		  u32 SampleIndex = FloorReal32ToInt32(SamplePosition);
		  r32 Frac = SamplePosition - (r32)SampleIndex;
		  r32 Sample0 = (r32)LoadedSound->Samples[0][SampleIndex];
		  r32 Sample1 = (r32)LoadedSound->Samples[0][SampleIndex + 1];
		  r32 SampleValue = Lerp(Sample0, Frac, Sample1);
#else
		  u32 SampleIndex = RoundReal32ToInt32(SamplePosition);
		  r32 SampleValue = LoadedSound->Samples[0][SampleIndex];
#endif		  
		  
		  *Dest0++ = Volume.E[0]*SampleValue;
		  *Dest1++ = Volume.E[1]*SampleValue;

		  Volume = V2Add(Volume, dVolume);
		  SamplePosition += dSample;
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
	      PlayingSound->SamplesPlayed = SamplePosition;	      
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
    __m128* Source0 = RealChannel0;
    __m128* Source1 = RealChannel1;
    
    __m128i* SampleOut = (__m128i*)SoundBuffer->Samples;
    Assert(SampleCount4 > 0);
    for(u32 SampleIndex = 0;
	SampleIndex < SampleCount4;
	++SampleIndex)
      {
	__m128 S0 = _mm_load_ps((r32*)Source0++);
	__m128 S1 = _mm_load_ps((r32*)Source1++);

	__m128i L = _mm_cvtps_epi32(S0);
	__m128i R = _mm_cvtps_epi32(S1);
	
	__m128 LR0 = _mm_unpacklo_epi32(L, R);
	__m128 LR1 = _mm_unpackhi_epi32(L, R);
	
	__m128i S01 = _mm_packs_epi32(LR0, LR1);

	*SampleOut++ = S01;
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

