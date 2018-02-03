
#include <stdio.h>
#include <stdlib.h>
#include "handmade_platform.h"
#include "handmade_asset_type_id.h"

FILE* Out = 0;

typedef struct
{
  char* Filename;
  r32 AlignPercentage[2];
} asset_bitmap_info;

typedef struct
{
  char* Filename;
  u32 FirstSampleIndex;
  u32 SampleCount;
  u32 NextIDToPlay;
} asset_sound_info;

typedef struct
{
  u32 ID;
  r32 Value;
} asset_tag;

typedef struct
{
  u64 DataOffset;
  u32 FirstTagIndex;
  u32 OnePastLastTagIndex;
} asset;

typedef struct
{
  u32 FirstAssetIndex;
  u32 OnePastLastAssetIndex;
} asset_type;

typedef struct
{
  char* Filename;
  r32 Alignment[2];
  
} bitmap_asset;

#define VERY_LARGE_NUMBER 4096

u32 BitmapCount;
u32 SoundCount;
u32 TagCount;
u32 AssetCount;
asset_bitmap_info BitmapInfos[VERY_LARGE_NUMBER];
asset_sound_info SoundInfos[VERY_LARGE_NUMBER];
asset_tag Tags[VERY_LARGE_NUMBER];
asset Assets[VERY_LARGE_NUMBER];
  
asset_type AssetTypes[Asset_Count];

u32 DEBUGUsedBitmapCount;
u32 DEBUGUsedSoundCount;
u32 DEBUGUsedAssetCount;
u32 DEBUGUsedTagCount;
asset_type* DEBUGAssetType;
asset* DEBUGAsset;

internal void
BeginAssetType(asset_type_id TypeID)
{
  Assert(DEBUGAssetType == 0);
  DEBUGAssetType = AssetTypes + TypeID;
  DEBUGAssetType->FirstAssetIndex = DEBUGUsedAssetCount;
  DEBUGAssetType->OnePastLastAssetIndex = DEBUGAssetType->FirstAssetIndex;
}

internal void
AddBitmapAsset(char* Filename, r32 AlignPercentageX, r32 AlignPercentageY)
{
  Assert(DEBUGAssetType);
  Assert(DEBUGAssetType->OnePastLastAssetIndex < AssetCount);

  asset* Asset = Assets + DEBUGAssetType->OnePastLastAssetIndex++;
  Asset->FirstTagIndex = DEBUGUsedTagCount;
  Asset->OnePastLastTagIndex = Asset->FirstTagIndex;
  Asset->SlotID = DEBUGAddBitmapInfo(Filename, AlignPercentage).Value;

  /*

    internal bitmap_id
    DEBUGAddBitmapInfo(char* Filename, v2 AlignPercentage)
    {
    Assert(Assets->DEBUGUsedAssetCount < Assets->BitmapCount);
  
    bitmap_id ID = {Assets->DEBUGUsedAssetCount++};

    asset_bitmap_info* Info = Assets->BitmapInfos + ID.Value;
    Info->Filename = PushString(&Assets->Arena, Filename);
    Info->AlignPercentage = AlignPercentage;
  
    return(ID);
    }

   */
  
  Assets->DEBUGAsset = Asset;
}

internal asset*
AddSoundAsset(char* Filename, u32 FirstSampleIndex, u32 SampleCount)
{
  Assert(DEBUGAssetType);
  Assert(DEBUGAssetType->OnePastLastAssetIndex < AssetCount);

  asset* Asset = Assets + DEBUGAssetType->OnePastLastAssetIndex++;
  FirstTagIndex = DEBUGUsedTagCount;
  OnePastLastTagIndex = Asset->FirstTagIndex;
  SlotID = DEBUGAddSoundInfo(Filename, FirstSampleIndex, SampleCount).Value;
  /*

    internal sound_id
    DEBUGAddSoundInfo(char* Filename, u32 FirstSampleIndex, u32 SampleCount)
    {
    Assert(DEBUGUsedSoundCount < SoundCount);
  
    sound_id ID = {DEBUGUsedAssetCount++};

    asset_sound_info* Info = SoundInfos + ID.Value;
    Info->Filename = PushString(&Arena, Filename);
    Info->FirstSampleIndex = FirstSampleIndex;
    Info->SampleCount = SampleCount;
    Info->NextIDToPlay.Value = 0;
  
    return(ID);
    }

   */
  Assets->DEBUGAsset = Asset;

  return(Asset);
}

internal void
AddTag(assets* Assets, asset_tag_id ID, r32 Value)
{
  Assert(Assets->DEBUGAsset);
  
  ++Assets->DEBUGAsset->OnePastLastTagIndex;
  asset_tag* Tag = Assets->Tags + Assets->DEBUGUsedTagCount++;

  Tag->ID = ID;
  Tag->Value = Value;
}

int
main(int ArgC, char** Args)
{

  BeginAssetType(Assets, Asset_Shadow);
  AddBitmapAsset(Assets, "../data/test/test_hero_shadow.bmp",
		 V2(0.5f, 0.156602029f));
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Tree);
  AddBitmapAsset(Assets, "../data/test2/tree00.bmp",
		 V2(0.493827164f, 0.295652182f));
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Sword);
  AddBitmapAsset(Assets, "../data/test2/rock03.bmp",
		 V2(0.5f, 0.65625f));
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Grass);
  AddBitmapAsset(Assets, "../data/test2/grass00.bmp",
		 V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "../data/test2/grass01.bmp",
		 V2(0.5f, 0.5f));
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Stone);
  AddBitmapAsset(Assets, "../data/test2/ground00.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "../data/test2/ground01.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "../data/test2/ground02.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "../data/test2/ground03.bmp", V2(0.5f, 0.5f));
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Tuft);
  AddBitmapAsset(Assets, "../data/test2/tuft00.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "../data/test2/tuft01.bmp", V2(0.5f, 0.5f));
  AddBitmapAsset(Assets, "../data/test2/tuft02.bmp", V2(0.5f, 0.5f));
  EndAssetType(Assets);

  r32 Angle_Right = 0.0f*Tau32;
  r32 Angle_Back = 0.25f*Tau32;
  r32 Angle_Left = 0.5f*Tau32;
  r32 Angle_Front = 0.75f*Tau32;

  v2 HeroAlign = {0.5f, 0.156682029f};
  
  BeginAssetType(Assets, Asset_Head);
  AddBitmapAsset(Assets, "../data/test/test_hero_right_head.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Right);
  AddBitmapAsset(Assets, "../data/test/test_hero_back_head.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Back);
  AddBitmapAsset(Assets, "../data/test/test_hero_left_head.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Left);
  AddBitmapAsset(Assets, "../data/test/test_hero_front_head.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Front);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Cape);
  AddBitmapAsset(Assets, "../data/test/test_hero_right_cape.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, 0.0f);
  AddBitmapAsset(Assets, "../data/test/test_hero_back_cape.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Back);
  AddBitmapAsset(Assets, "../data/test/test_hero_left_cape.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Left);
  AddBitmapAsset(Assets, "../data/test/test_hero_front_cape.bmp",HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Front);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Torso);
  AddBitmapAsset(Assets, "../data/test/test_hero_right_torso.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, 0.0f);
  AddBitmapAsset(Assets, "../data/test/test_hero_back_torso.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Back);
  AddBitmapAsset(Assets, "../data/test/test_hero_left_torso.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Left);
  AddBitmapAsset(Assets, "../data/test/test_hero_front_torso.bmp", HeroAlign);
  AddTag(Assets, Tag_FacingDirection, Angle_Front);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Bloop);
  AddSoundAsset(Assets, "../data/test3/bloop_00.wav", 0, 0);
  AddSoundAsset(Assets, "../data/test3/bloop_01.wav", 0, 0);
  AddSoundAsset(Assets, "../data/test3/bloop_02.wav", 0, 0);
  AddSoundAsset(Assets, "../data/test3/bloop_03.wav", 0, 0);
  AddSoundAsset(Assets, "../data/test3/bloop_04.wav", 0, 0);
  EndAssetType(Assets);
  
  BeginAssetType(Assets, Asset_Crack);
  AddSoundAsset(Assets, "../data/test3/crack_00.wav", 0, 0);
  EndAssetType(Assets);
  
  BeginAssetType(Assets, Asset_Drop);
  AddSoundAsset(Assets, "../data/test3/drop_00.wav", 0, 0);
  EndAssetType(Assets);
  
  BeginAssetType(Assets, Asset_Glide);
  AddSoundAsset(Assets, "../data/test3/glide_00.wav", 0, 0);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Music);
  u32 OneMusicChunk = 10*48000;
  u32 TotalMusicSampleCount = 7468095;
  asset* LastMusic = 0;
  for(u32 FirstSampleIndex = 0;
      FirstSampleIndex < TotalMusicSampleCount;
      FirstSampleIndex += OneMusicChunk)
    {
      u32 SampleCount = TotalMusicSampleCount - FirstSampleIndex;
      if(SampleCount > OneMusicChunk)
	{
	  SampleCount = OneMusicChunk;
	}
      asset* ThisMusic = AddSoundAsset(Assets, "../data/test3/music_test.wav", FirstSampleIndex, SampleCount);
      if(LastMusic)
	{
	  Assets->SoundInfos[LastMusic->SlotID].NextIDToPlay.Value = ThisMusic->SlotID;
	}
      LastMusic = ThisMusic;
    }
  EndAssetType(Assets);
    
  BeginAssetType(Assets, Asset_Puhp);
  AddSoundAsset(Assets, "../data/test3/puhp_00.wav", 0, 0);
  AddSoundAsset(Assets, "../data/test3/puhp_01.wav", 0, 0);
  EndAssetType(Assets);

  Out = fopen("test.hha", "wb");
  if(Out)
    {
      

      fclose(Out);
    }
  
}
