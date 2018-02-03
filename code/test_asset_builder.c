
#include "test_asset_builder.h"

FILE* Out = 0;

internal void
BeginAssetType(assets* Assets, asset_type_id TypeID)
{
  Assert(Assets->DEBUGAssetType == 0);
  Assets->DEBUGAssetType = Assets->AssetTypes + TypeID;
  Assets->DEBUGAssetType->TypeID = TypeID;
  Assets->DEBUGAssetType->FirstAssetIndex = Assets->AssetCount;
  Assets->DEBUGAssetType->OnePastLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;
}

internal bitmap_id
AddBitmapAsset(assets* Assets, char* Filename, r32 AlignPercentageX, r32 AlignPercentageY)
{
  Assert(Assets->DEBUGAssetType);
  Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

  bitmap_id Result = {Assets->DEBUGAssetType->OnePastLastAssetIndex++};
  asset* Asset = Assets->Assets + Result.Value;
  Asset->FirstTagIndex = Assets->TagCount;
  Asset->OnePastLastTagIndex = Asset->FirstTagIndex;
  Asset->Bitmap.Filename = Filename;
  Asset->Bitmap.AlignPercentage[0] = AlignPercentageX;
  Asset->Bitmap.AlignPercentage[1] = AlignPercentageY;
  
  Assets->DEBUGAsset = Asset;

  return(Result);
}

internal sound_id
AddSoundAsset(assets* Assets, char* Filename, u32 FirstSampleIndex, u32 SampleCount)
{
  Assert(Assets->DEBUGAssetType);
  Assert(Assets->DEBUGAssetType->OnePastLastAssetIndex < ArrayCount(Assets->Assets));

  sound_id Result = {Assets->DEBUGAssetType->OnePastLastAssetIndex++};
  asset* Asset = Assets->Assets + Result.Value;
  Asset->FirstTagIndex = Assets->TagCount;
  Asset->OnePastLastTagIndex = Asset->FirstTagIndex;
  Asset->Sound.Filename = Filename;
  Asset->Sound.FirstSampleIndex = FirstSampleIndex;
  Asset->Sound.SampleCount = SampleCount;
  Asset->Sound.NextIDToPlay.Value = 0;

  Assets->DEBUGAsset = Asset;
  
  return(Result);
}

internal void
AddTag(assets* Assets, asset_tag_id ID, r32 Value)
{
  Assert(Assets->DEBUGAsset);
  
  ++Assets->DEBUGAsset->OnePastLastTagIndex;
  hha_tag* Tag = Assets->Tags + Assets->TagCount++;

  Tag->ID = ID;
  Tag->Value = Value;
}

internal void
EndAssetType(assets* Assets)
{
  Assert(Assets->DEBUGAssetType);
  Assets->AssetCount = Assets->DEBUGAssetType->OnePastLastAssetIndex;
  Assets->DEBUGAssetType = 0;
  Assets->DEBUGAsset = 0;
}

int
main(int ArgC, char** Args)
{
  assets Assets_;
  assets* Assets = &Assets_;
  
  Assets->TagCount = 1;
  Assets->AssetCount = 1;
  Assets->DEBUGAssetType = 0;
  Assets->DEBUGAsset = 0;
  
  BeginAssetType(Assets, Asset_Shadow);
  AddBitmapAsset(Assets, "../data/test/test_hero_shadow.bmp",
		 0.5f, 0.156602029f);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Tree);
  AddBitmapAsset(Assets, "../data/test2/tree00.bmp",
		 0.493827164f, 0.295652182f);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Sword);
  AddBitmapAsset(Assets, "../data/test2/rock03.bmp",
		 0.5f, 0.65625f);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Grass);
  AddBitmapAsset(Assets, "../data/test2/grass00.bmp",
		 0.5f, 0.5f);
  AddBitmapAsset(Assets, "../data/test2/grass01.bmp",
		 0.5f, 0.5f);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Stone);
  AddBitmapAsset(Assets, "../data/test2/ground00.bmp", 0.5f, 0.5f);
  AddBitmapAsset(Assets, "../data/test2/ground01.bmp", 0.5f, 0.5f);
  AddBitmapAsset(Assets, "../data/test2/ground02.bmp", 0.5f, 0.5f);
  AddBitmapAsset(Assets, "../data/test2/ground03.bmp", 0.5f, 0.5f);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Tuft);
  AddBitmapAsset(Assets, "../data/test2/tuft00.bmp", 0.5f, 0.5f);
  AddBitmapAsset(Assets, "../data/test2/tuft01.bmp", 0.5f, 0.5f);
  AddBitmapAsset(Assets, "../data/test2/tuft02.bmp", 0.5f, 0.5f);
  EndAssetType(Assets);

  r32 Angle_Right = 0.0f*Tau32;
  r32 Angle_Back = 0.25f*Tau32;
  r32 Angle_Left = 0.5f*Tau32;
  r32 Angle_Front = 0.75f*Tau32;

  r32 HeroAlignX = 0.5f;
  r32 HeroAlignY = 0.156682029f;
  
  BeginAssetType(Assets, Asset_Head);
  AddBitmapAsset(Assets, "../data/test/test_hero_right_head.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Right);
  AddBitmapAsset(Assets, "../data/test/test_hero_back_head.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Back);
  AddBitmapAsset(Assets, "../data/test/test_hero_left_head.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Left);
  AddBitmapAsset(Assets, "../data/test/test_hero_front_head.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Front);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Cape);
  AddBitmapAsset(Assets, "../data/test/test_hero_right_cape.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, 0.0f);
  AddBitmapAsset(Assets, "../data/test/test_hero_back_cape.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Back);
  AddBitmapAsset(Assets, "../data/test/test_hero_left_cape.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Left);
  AddBitmapAsset(Assets, "../data/test/test_hero_front_cape.bmp",HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Front);
  EndAssetType(Assets);

  BeginAssetType(Assets, Asset_Torso);
  AddBitmapAsset(Assets, "../data/test/test_hero_right_torso.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, 0.0f);
  AddBitmapAsset(Assets, "../data/test/test_hero_back_torso.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Back);
  AddBitmapAsset(Assets, "../data/test/test_hero_left_torso.bmp", HeroAlignX, HeroAlignY);
  AddTag(Assets, Tag_FacingDirection, Angle_Left);
  AddBitmapAsset(Assets, "../data/test/test_hero_front_torso.bmp", HeroAlignX, HeroAlignY);
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
  sound_id LastMusic = {0};
  for(u32 FirstSampleIndex = 0;
      FirstSampleIndex < TotalMusicSampleCount;
      FirstSampleIndex += OneMusicChunk)
    {
      u32 SampleCount = TotalMusicSampleCount - FirstSampleIndex;
      if(SampleCount > OneMusicChunk)
	{
	  SampleCount = OneMusicChunk;
	}
      sound_id ThisMusic = AddSoundAsset(Assets, "../data/test3/music_test.wav", FirstSampleIndex, SampleCount);
      if(LastMusic.Value)
	{
	  Assets->Assets[LastMusic.Value].Sound.NextIDToPlay = ThisMusic;
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
      hha_header Header = {};
      Header.MagicValue = HHA_MAGIC_VALUE;
      Header.Version = HHA_VERSION;
      Header.TagCount = Assets->TagCount;
      Header.AssetTypeCount = Asset_Count;
      Header.AssetCount = Assets->AssetCount;
      
      u32 TagArraySize = Header.TagCount*sizeof(hha_tag);
      u32 AssetTypesArraySize = Header.AssetTypeCount*sizeof(hha_asset_type);
      u32 AssetsArraySize = Header.AssetCount*sizeof(hha_asset);
      
      Header.Tags = sizeof(Header);
      Header.AssetTypes = Header.Tags + TagArraySize;
      Header.Assets = Header.AssetTypes + AssetTypesArraySize;

      fwrite(&Header, sizeof(Header), 1, Out);
      fwrite(Assets->Tags, TagArraySize, 1, Out);
      fwrite(Assets->AssetTypes, AssetTypesArraySize, 1, Out);
      //fwrite(&AssetArray, AssetsArraySize, 1, Out);
      
      fclose(Out);
    }
  else
    {
      printf("Error: Couldn't open the file!");
    }
  
}
