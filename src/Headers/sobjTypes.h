//
// sobjtypes.h
//

#ifndef SOBJ_H
#define SOBJ_H

enum
{
	SPRITE_GROUP_SPHEREMAPS 		= 	0,
	SPRITE_GROUP_INFOBAR			=	1,
	SPRITE_GROUP_LEVELINTRO			=	1,
	SPRITE_GROUP_MODESCREEN			=	2,
	SPRITE_GROUP_PARTICLES			=	3,
	SPRITE_GROUP_BONUS				= 	4,
	SPRITE_GROUP_HIGHSCORES			=	4,
	SPRITE_GROUP_LOSE				=	4,
	SPRITE_GROUP_FENCES				= 	5,
	SPRITE_GROUP_GLOBAL				= 	6,
	SPRITE_GROUP_FONT				= 	7,
	SPRITE_GROUP_LEVELSPECIFIC		= 	8,
	
	MAX_SPRITE_GROUPS
};

		/* GLOBAL SPRITES */
		
enum
{
	GLOBAL_SObjType_Shadow_Circular,
	GLOBAL_SObjType_Shadow_Triangular,
	
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
	
	GLOBAL_SObjType_FrozenBrainAlien
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
	SPHEREMAP_SObjType_Tundra
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
	PARTICLE_SObjType_RocketFlame7
};

/******************* HELPTEXT SOBJTYPES *************************/

enum
{
	HELPTEXT_SObjType_Comma,
	HELPTEXT_SObjType_Dash,
	HELPTEXT_SObjType_Period,
	HELPTEXT_SObjType_QuestionMark,
	HELPTEXT_SObjType_ExclamationMark,
	HELPTEXT_SObjType_Apostrophe,
	
	HELPTEXT_SObjType_UU,
	HELPTEXT_SObjType_OO,
	HELPTEXT_SObjType_AA,
	HELPTEXT_SObjType_AO,
	HELPTEXT_SObjType_NN,
	HELPTEXT_SObjType_EE,
	HELPTEXT_SObjType_Ax,
	HELPTEXT_SObjType_Ox,
	HELPTEXT_SObjType_Oa,
	
	HELPTEXT_SObjType_0,
	HELPTEXT_SObjType_1,
	HELPTEXT_SObjType_2,
	HELPTEXT_SObjType_3,
	HELPTEXT_SObjType_4,
	HELPTEXT_SObjType_5,
	HELPTEXT_SObjType_6,
	HELPTEXT_SObjType_7,
	HELPTEXT_SObjType_8,
	HELPTEXT_SObjType_9,
	HELPTEXT_SObjType_A,
	HELPTEXT_SObjType_B,
	HELPTEXT_SObjType_C,
	HELPTEXT_SObjType_D,
	HELPTEXT_SObjType_E,
	HELPTEXT_SObjType_F,
	HELPTEXT_SObjType_G,
	HELPTEXT_SObjType_H,
	HELPTEXT_SObjType_I,
	HELPTEXT_SObjType_J,
	HELPTEXT_SObjType_K,
	HELPTEXT_SObjType_L,
	HELPTEXT_SObjType_M,
	HELPTEXT_SObjType_N,
	HELPTEXT_SObjType_O,
	HELPTEXT_SObjType_P,
	HELPTEXT_SObjType_Q,
	HELPTEXT_SObjType_R,
	HELPTEXT_SObjType_S,
	HELPTEXT_SObjType_T,
	HELPTEXT_SObjType_U,
	HELPTEXT_SObjType_V,
	HELPTEXT_SObjType_W,
	HELPTEXT_SObjType_X,
	HELPTEXT_SObjType_Y,
	HELPTEXT_SObjType_Z,
	
	HELPTEXT_SObjType_Cursor

};




/******************* FARM LEVEL *************************/

enum
{
	FARM_SObjType_Sky
};



/******************* SLIME LEVEL *************************/

enum
{
	SLIME_SObjType_Sky,
	SLIME_SObjType_GreenSlime,
	SLIME_SObjType_OilSlime,
	SLIME_SObjType_OrangeSlime
};


/******************* JUNGLE LEVEL *************************/

enum
{
	JUNGLE_SObjType_Sky,
	JUNGLE_SObjType_FrozenFlyTrap,
	JUNGLE_SObjType_Vine
};


/******************* APOCALYPSE LEVEL *************************/

enum
{
	APOCALYPSE_SObjType_Sky,
	APOCALYPSE_SObjType_Rope
};

/******************* CLOUD LEVEL *************************/

enum
{
	CLOUD_SObjType_Sky,
	CLOUD_SObjType_BlueBeam,
	CLOUD_SObjType_Cloud
};



/******************* FIRE&ICE LEVEL *************************/

enum
{
	FIREICE_SObjType_Sky,
	FIREICE_SObjType_Rope,
	FIREICE_SObjType_FrozenSquooshy
};


/******************* BRAIN BOSS LEVEL *************************/

enum
{
	BRAINBOSS_SObjType_Sky,
	BRAINBOSS_SObjType_Static1,
	BRAINBOSS_SObjType_Static2,
	BRAINBOSS_SObjType_Static3,
	BRAINBOSS_SObjType_Static4,
	
	BRAINBOSS_SObjType_RedZap
};


#endif













