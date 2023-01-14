//
// sobjtypes.h
//

#pragma once

enum
{
	SPRITE_GROUP_SPHEREMAPS 		= 	0,
	SPRITE_GROUP_INFOBAR			=	1,
	SPRITE_GROUP_PARTICLES			=	3,
	SPRITE_GROUP_BONUS				= 	4,
	SPRITE_GROUP_HIGHSCORES			=	4,
	SPRITE_GROUP_LOSE				=	4,
	SPRITE_GROUP_FENCES				= 	5,
	SPRITE_GROUP_GLOBAL				= 	6,
	SPRITE_GROUP_LEVELSPECIFIC		= 	8,
	
	MAX_SPRITE_GROUPS
};

		/* GLOBAL SPRITES */
		
enum
{
	GLOBAL_SObjType_Shadow_Circular,

	GLOBAL_SObjType_FrozenBrainAlien,

	GLOBAL_SObjType_Water,
	GLOBAL_SObjType_Soap,
	GLOBAL_SObjType_GreenWater,
	GLOBAL_SObjType_OilyWater,
	GLOBAL_SObjType_JungleWater,
	GLOBAL_SObjType_Mud,
	GLOBAL_SObjType_RadioactiveWater,
	GLOBAL_SObjType_Lava,
	
	GLOBAL_SObjType_AtomicNucleus_Red,
	GLOBAL_SObjType_AtomicNucleus_Green,
	GLOBAL_SObjType_AtomicNucleus_Blue,
	GLOBAL_SObjType_AtomicRing_Red,
	GLOBAL_SObjType_AtomicRing_Green,
	GLOBAL_SObjType_AtomicRing_Blue,
	GLOBAL_SObjType_NovaCharge,
	GLOBAL_SObjType_MagnetRay,

	GLOBAL_SObjType_COUNT,
};





		/* SPHEREMAP SPRITES */
		
enum
{
	SPHEREMAP_SObjType_Blue,
	SPHEREMAP_SObjType_Sea,
	SPHEREMAP_SObjType_DarkDusk,
	SPHEREMAP_SObjType_Medow,
	SPHEREMAP_SObjType_GreenSheen,
	SPHEREMAP_SObjType_DarkYosemite,
	SPHEREMAP_SObjType_Red,
	SPHEREMAP_SObjType_Tundra,
	SPHEREMAP_SObjType_COUNT,
};



		/* PARTICLE SPRITES */
		
enum
{
	PARTICLE_SObjType_WhiteSpark,
	PARTICLE_SObjType_WhiteSpark2,
	PARTICLE_SObjType_WhiteSpark3,
	PARTICLE_SObjType_WhiteSpark4,
	PARTICLE_SObjType_GreySmoke,
	PARTICLE_SObjType_RedGlint,
	PARTICLE_SObjType_Splash,
	PARTICLE_SObjType_SnowFlakes,
	PARTICLE_SObjType_Fire,
	PARTICLE_SObjType_GreenFumes,
	PARTICLE_SObjType_BlackSmoke,
	PARTICLE_SObjType_RedFumes,
	PARTICLE_SObjType_RedSpark,
	PARTICLE_SObjType_BlueSpark,
	PARTICLE_SObjType_GreenSpark,
	PARTICLE_SObjType_BlueGlint,
	PARTICLE_SObjType_PurpleRing,
	PARTICLE_SObjType_WhiteGlow,
	PARTICLE_SObjType_Bubble,
	PARTICLE_SObjType_GreenGlint,
	
	PARTICLE_SObjType_LensFlare0,
	PARTICLE_SObjType_LensFlare1,
	PARTICLE_SObjType_LensFlare2,
	PARTICLE_SObjType_LensFlare3,
	
	PARTICLE_SObjType_RocketFlame0,
	PARTICLE_SObjType_RocketFlame1,
	PARTICLE_SObjType_RocketFlame2,
	PARTICLE_SObjType_RocketFlame3,
	PARTICLE_SObjType_RocketFlame4,
	PARTICLE_SObjType_RocketFlame5,
	PARTICLE_SObjType_RocketFlame6,
	PARTICLE_SObjType_RocketFlame7,

	PARTICLE_SObjType_COUNT,
};


		/* BONUS SPRITES */

enum
{
	BONUS_SObjType_BonusText,
	BONUS_SObjType_BonusGlow,

	BONUS_SObjType_InventoryText,
	BONUS_SObjType_InventoryGlow,

	BONUS_SObjType_TotalBonusText,
	BONUS_SObjType_TotalBonusGlow,

	BONUS_SObjType_ScoreText,
	BONUS_SObjType_ScoreGlow,

	BONUS_SObjType_COUNT,
};


		/* HIGH SCORE SPRITES */

enum
{
	HIGHSCORES_SObjType_ScoreText,
	HIGHSCORES_SObjType_ScoreTextGlow,
	HIGHSCORES_SObjType_EnterNameText,
	HIGHSCORES_SObjType_EnterNameGlow,
	HIGHSCORES_SObjType_COUNT,
};


		/* GAME OVER SPRITES */

enum
{
	LOSE_SObjType_GameOver,
	WIN_SObjType_TheEnd_Text,
	WIN_SObjType_TheEnd_Glow,
	WIN_SObjType_BlackOut,
	WIN_SObjType_QText,
	WIN_SObjType_QGlow,
	GAMEOVER_SObjType_COUNT,
};


		/* FENCE SPRITES */

enum
{
	FENCE_TYPE_FARMWOOD,
	FENCE_TYPE_CORNSTALK,
	FENCE_TYPE_CHICKENWIRE,
	FENCE_TYPE_METALFARM,

	FENCE_TYPE_PINKCRYSTAL,
	FENCE_TYPE_MECH,
	FENCE_TYPE_SLIMETREE,
	FENCE_TYPE_BLUECRYSTAL,
	FENCE_TYPE_MECH2,
	FENCE_TYPE_JUNGLEWOOD,
	FENCE_TYPE_JUNGLEFERN,

	FENCE_TYPE_LAMP,
	FENCE_TYPE_RUBBLE,
	FENCE_TYPE_CRUNCH,

	FENCE_TYPE_FUN,
	FENCE_TYPE_HEDGE,
	FENCE_TYPE_LINE,
	FENCE_TYPE_TENT,

	FENCE_TYPE_LAVAFENCE,
	FENCE_TYPE_ROCKFENCE,

	FENCE_TYPE_NEURONFENCE,

	FENCE_TYPE_SAUCER,
	FENCE_TYPE_COUNT,
};


/******************* INFOBAR SOBJTYPES *************************/

enum
{
	INFOBAR_SObjType_PulseGun,
	INFOBAR_SObjType_FreezeGun,
	INFOBAR_SObjType_FlameGun,
	INFOBAR_SObjType_Fist,
	INFOBAR_SObjType_SuperNova,
	INFOBAR_SObjType_GrowVial,
	INFOBAR_SObjType_FlareGun,
	INFOBAR_SObjType_DartPOW,

	INFOBAR_SObjType_PulseGunGlow,
	INFOBAR_SObjType_FreezeGlow,
	INFOBAR_SObjType_FlameGlow,
	INFOBAR_SObjType_FistGlow,
	INFOBAR_SObjType_SuperNovaGlow,
	INFOBAR_SObjType_GrowVialGlow,
	INFOBAR_SObjType_FlareGlow,
	INFOBAR_SObjType_DartGlow,

	INFOBAR_SObjType_LeftGirder,
	INFOBAR_SObjType_RightGirder,
	INFOBAR_SObjType_OttoHead,
	INFOBAR_SObjType_HealthMeter,
	INFOBAR_SObjType_FuelMeter,
	INFOBAR_SObjType_JumpJetMeter,
	INFOBAR_SObjType_MeterBack,
	INFOBAR_SObjType_WeaponDisplay,
	INFOBAR_SObjType_RocketIcon,

	INFOBAR_SObjType_0,
	INFOBAR_SObjType_1,
	INFOBAR_SObjType_2,
	INFOBAR_SObjType_3,
	INFOBAR_SObjType_4,
	INFOBAR_SObjType_5,
	INFOBAR_SObjType_6,
	INFOBAR_SObjType_7,
	INFOBAR_SObjType_8,
	INFOBAR_SObjType_9,

	INFOBAR_SObjType_BeamCupLeft,
	INFOBAR_SObjType_BeamCupRight,
	INFOBAR_SObjType_TeleportBeam,
	INFOBAR_SObjType_DestructoBeam,

	INFOBAR_SObjType_HumanFrame,
	INFOBAR_SObjType_HumanFrame2,
	INFOBAR_SObjType_Farmer,
	INFOBAR_SObjType_BeeWoman,
	INFOBAR_SObjType_Scientist,
	INFOBAR_SObjType_SkirtLady,

	INFOBAR_SObjType_COUNT,
};


/******************* FARM LEVEL *************************/

enum
{
	FARM_SObjType_Sky,
	FARM_SObjType_COUNT,
};



/******************* SLIME LEVEL *************************/

enum
{
	SLIME_SObjType_Sky,
	SLIME_SObjType_GreenSlime,
	SLIME_SObjType_OilSlime,
	SLIME_SObjType_OrangeSlime,
	SLIME_SObjType_COUNT,
};


/******************* JUNGLE LEVEL *************************/

enum
{
	JUNGLE_SObjType_Sky,
	JUNGLE_SObjType_FrozenFlyTrap,
	JUNGLE_SObjType_Vine,
	JUNGLE_SObjType_COUNT,
};


/******************* APOCALYPSE LEVEL *************************/

enum
{
	APOCALYPSE_SObjType_Sky,
	APOCALYPSE_SObjType_Rope,
	APOCALYPSE_SObjType_COUNT,
};

/******************* CLOUD LEVEL *************************/

enum
{
	CLOUD_SObjType_Sky,
	CLOUD_SObjType_BlueBeam,
	CLOUD_SObjType_Cloud,
	CLOUD_SObjType_COUNT,
};



/******************* FIRE&ICE LEVEL *************************/

enum
{
	FIREICE_SObjType_Sky,
	FIREICE_SObjType_Rope,
	FIREICE_SObjType_FrozenSquooshy,
	FIREICE_SObjType_COUNT,
};


/******************* BRAIN BOSS LEVEL *************************/

enum
{
	BRAINBOSS_SObjType_Sky,
	BRAINBOSS_SObjType_Static1,
	BRAINBOSS_SObjType_Static2,
	BRAINBOSS_SObjType_Static3,
	BRAINBOSS_SObjType_Static4,
	
	BRAINBOSS_SObjType_RedZap,
	BRAINBOSS_SObjType_COUNT,
};

