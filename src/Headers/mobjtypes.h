//
// mobjtypes.h
//

#pragma once

enum
{
	MODEL_GROUP_GLOBAL		=	0,
	MODEL_GROUP_LEVELSPECIFIC =	1,
	MODEL_GROUP_LEVELINTRO =	1,
	MODEL_GROUP_WINNERS		= 	2,
	MODEL_GROUP_BONUS		= 	2,
	MODEL_GROUP_MAINMENU	= 	2,
	MODEL_GROUP_HIGHSCORES	=	2,
	MODEL_GROUP_LOSESCREEN	=	2,
	MODEL_GROUP_WINSCREEN	=	2,
	
	MODEL_GROUP_SKELETONBASE				// skeleton files' models are attached here
};




/******************* GLOBAL *************************/

enum
{
	GLOBAL_ObjType_Ripple,
	GLOBAL_ObjType_PowerupOrb,
	GLOBAL_ObjType_PowerupOrb_Top,
	GLOBAL_ObjType_PowerupOrb_Bottom,
	
	GLOBAL_ObjType_OttoLeftHand,
	GLOBAL_ObjType_OttoRightHand,
	GLOBAL_ObjType_OttoLeftFist,
	GLOBAL_ObjType_OttoRightFist,
	GLOBAL_ObjType_PulseGunHand,
	GLOBAL_ObjType_FreezeGunHand,
	GLOBAL_ObjType_FlameGunHand,
	GLOBAL_ObjType_GrowthHand,
	GLOBAL_ObjType_FlareGunHand,
	
							// weapons all go here
	GLOBAL_ObjType_StunPulseGun,
	GLOBAL_ObjType_SuperNovaBattery,
	GLOBAL_ObjType_FreezeGun,
	GLOBAL_ObjType_FlameGun,
	GLOBAL_ObjType_FlareGun,
	GLOBAL_ObjType_DartPOW,
	GLOBAL_ObjType_FreeLifePOW,
	
	GLOBAL_ObjType_StunPulseBullet,
	GLOBAL_ObjType_FreezeBullet,
	GLOBAL_ObjType_FlameBullet,
	
	GLOBAL_ObjType_EnemySaucer_Top,
	GLOBAL_ObjType_EnemySaucer_Bottom,
	GLOBAL_ObjType_WhiteBeam,
	GLOBAL_ObjType_BlueSpiral,
	
	GLOBAL_ObjType_Rocket,
	GLOBAL_ObjType_RocketDoor,
	
	GLOBAL_ObjType_TeleportRing,
	
	GLOBAL_ObjType_BrainWave,
	
	GLOBAL_ObjType_TeleportBase,
	GLOBAL_ObjType_TeleportDish,
	
	GLOBAL_ObjType_DeathExitLeft,
	GLOBAL_ObjType_DeathExitRight,
	
	GLOBAL_ObjType_SleepZ
};


/******************* FARM *************************/

enum
{
	FARM_ObjType_Cyclorama,

	FARM_ObjType_Barn,
	FARM_ObjType_Silo,
	FARM_ObjType_WoodGate,
	FARM_ObjType_WoodGateChunk_Post,
	FARM_ObjType_WoodGateChunk_Slat,
	FARM_ObjType_MetalGate,
	FARM_ObjType_MetalGateChunk_V,
	FARM_ObjType_MetalGateChunk_H,
	FARM_ObjType_WoodPost,
	FARM_ObjType_MetalPost,
	
	FARM_ObjType_Tractor,
	FARM_ObjType_BackLeftWheel,
	FARM_ObjType_BackRightWheel,
	FARM_ObjType_FrontLeftWheel,
	FARM_ObjType_FrontRightWheel,
	
	FARM_ObjType_CornStalk,
	FARM_ObjType_CornSprout,
	FARM_ObjType_OnionSprout,
	FARM_ObjType_TomatoSprout,
	FARM_ObjType_BigLeafPlant,
	
	FARM_ObjType_Tree,
	FARM_ObjType_PhonePole,
	FARM_ObjType_Windmill,
	FARM_ObjType_Propeller,
	
	FARM_ObjType_MetalTub,
	FARM_ObjType_OutHouse,
	
	FARM_ObjType_Rock_Small,
	FARM_ObjType_Rock_Medium,
	FARM_ObjType_Rock_Large,
	
	FARM_ObjType_HayBrick,
	FARM_ObjType_HayCylinder,
	
	FARM_ObjType_CornKernel,
	FARM_ObjType_PopCorn,
	FARM_ObjType_OnionSlice,
	FARM_ObjType_OnionStalk,
	FARM_ObjType_TomatoSlice
};


/******************* SLIME *************************/

enum
{
	SLIME_ObjType_SlimeTube_FancyJ,
	SLIME_ObjType_SlimeTube_J,
	SLIME_ObjType_SlimeTube_M,
	SLIME_ObjType_SlimeTube_FancyShort,
	SLIME_ObjType_SlimeTube_T,
	SLIME_ObjType_SlimeTube_Grate,
	SLIME_ObjType_SlimeTube_Valve,
	
	SLIME_ObjType_Mech_OnAStick_Pole,
	SLIME_ObjType_Mech_OnAStick_Top,
	SLIME_ObjType_Mech_Boiler,

	SLIME_ObjType_CrystalCluster_Blue,
	SLIME_ObjType_CrystalCluster_Red,
	SLIME_ObjType_CrystalColumn_Blue,
	SLIME_ObjType_CrystalColumn_Red,
	
	SLIME_ObjType_FallingCrystal_Blue,
	SLIME_ObjType_FallingCrystal_Red,
	SLIME_ObjType_BumperBubble,
	SLIME_ObjType_SoapBubble,
	SLIME_ObjType_HalfSoapBubble,
	SLIME_ObjType_BubblePump,
	SLIME_ObjType_BubblePumpPlunger,
	
	SLIME_ObjType_MagnetMonster,
	SLIME_ObjType_MagnetMonsterProp,
	SLIME_ObjType_Magnet,
	SLIME_ObjType_FallingSlimePlatform_Small,
	SLIME_ObjType_FallingSlimePlatform_Large,
	SLIME_ObjType_SlimeTree_Big,
	SLIME_ObjType_SlimeTree_Small,
	
	SLIME_ObjType_BlobChunk,
	SLIME_ObjType_BlobArrow
};



/******************* BLOB BOSS *************************/

enum
{
	BLOBBOSS_ObjType_Cyclorama,

	BLOBBOSS_ObjType_BarPlatform_Blue,
	BLOBBOSS_ObjType_BarPlatform_Grey,
	BLOBBOSS_ObjType_BarPlatform_Red,
	BLOBBOSS_ObjType_BarPlatform_Yellow,

	BLOBBOSS_ObjType_CircularPlatform_Blue,
	BLOBBOSS_ObjType_CircularPlatform_Grey,
	BLOBBOSS_ObjType_CircularPlatform_Red,
	BLOBBOSS_ObjType_CircularPlatform_Yellow,

	BLOBBOSS_ObjType_CogPlatformDown_Grey,
	BLOBBOSS_ObjType_CogPlatformDown_Blue,
	BLOBBOSS_ObjType_CogPlatformDown_Red,
	BLOBBOSS_ObjType_CogPlatformDown_Yellow,
	BLOBBOSS_ObjType_CogPlatformUp_Blue,
	BLOBBOSS_ObjType_CogPlatformUp_Grey,
	BLOBBOSS_ObjType_CogPlatformUp_Red,
	BLOBBOSS_ObjType_CogPlatformUp_Yellow,

	BLOBBOSS_ObjType_XPlatformUp_Blue,
	BLOBBOSS_ObjType_XPlatformUp_Grey,
	BLOBBOSS_ObjType_XPlatformUp_Red,
	BLOBBOSS_ObjType_XPlatformUp_Yellow,
	
	BLOBBOSS_ObjType_MovingPlatform_Blue,
	BLOBBOSS_ObjType_MovingPlatform_Red,
	BLOBBOSS_ObjType_FallingSlimePlatform_Small,
	
	BLOBBOSS_ObjType_BeamEnd,
	BLOBBOSS_ObjType_Base,
	BLOBBOSS_ObjType_Casing,
	BLOBBOSS_ObjType_TubeSegment,
	BLOBBOSS_ObjType_Horn,
	BLOBBOSS_ObjType_Blobule,
	BLOBBOSS_ObjType_BlobDroplet,
	
	BLOBBOSS_ObjType_Tube_Bent,
	BLOBBOSS_ObjType_Tube_Loop1,
	BLOBBOSS_ObjType_Tube_Loop2,
	BLOBBOSS_ObjType_Tube_Straight1,
	BLOBBOSS_ObjType_Tube_Straight2
};

/******************* APOCALYPSE *************************/

enum
{
	APOCALYPSE_ObjType_Cyclorama,

	APOCALYPSE_ObjType_CrunchDoor_Bottom,
	APOCALYPSE_ObjType_CrunchDoor_Top,
	APOCALYPSE_ObjType_CrunchPost,
	
	APOCALYPSE_ObjType_Manhole,
	
	APOCALYPSE_ObjType_SpacePod,
	
	APOCALYPSE_ObjType_Teleporter,
	APOCALYPSE_ObjType_TeleporterZap,
	APOCALYPSE_ObjType_Console0,
	APOCALYPSE_ObjType_Console1,
	APOCALYPSE_ObjType_Console2,
	APOCALYPSE_ObjType_Console3,
	APOCALYPSE_ObjType_Console4,
	APOCALYPSE_ObjType_Console5,
	APOCALYPSE_ObjType_Console6,
	APOCALYPSE_ObjType_Console7,
	APOCALYPSE_ObjType_Console8,
	APOCALYPSE_ObjType_Console9,
	APOCALYPSE_ObjType_Console10,
	APOCALYPSE_ObjType_Console11,
	
	APOCALYPSE_ObjType_ZipLinePost,
	APOCALYPSE_ObjType_ZipLinePully,
	
	APOCALYPSE_ObjType_ProximityMine,
	APOCALYPSE_ObjType_ExplodingCylinder,

	APOCALYPSE_ObjType_LampPost,
	APOCALYPSE_ObjType_LampPost_Broken,
	APOCALYPSE_ObjType_LampPost_Ruined,

	APOCALYPSE_ObjType_DebrisGate_Open,
	APOCALYPSE_ObjType_DebrisGate_Blocked,
	APOCALYPSE_ObjType_DebrisGate_Debris0,
	APOCALYPSE_ObjType_DebrisGate_Debris1,
	APOCALYPSE_ObjType_DebrisGate_Debris2,
	APOCALYPSE_ObjType_DebrisGate_Debris3,
	APOCALYPSE_ObjType_DebrisGate_Debris4,
	
	APOCALYPSE_ObjType_GraveStone,
	APOCALYPSE_ObjType_GraveStone2,
	APOCALYPSE_ObjType_GraveStone3,
	APOCALYPSE_ObjType_GraveStone4,
	APOCALYPSE_ObjType_GraveStone5,
	
	APOCALYPSE_ObjType_CrashedRocket,
	APOCALYPSE_ObjType_CrashedSaucer,
	
	APOCALYPSE_ObjType_ConeBlast,
	APOCALYPSE_ObjType_Shockwave,
	
	APOCALYPSE_ObjType_Rubble0,
	APOCALYPSE_ObjType_Rubble1,
	APOCALYPSE_ObjType_Rubble2,
	APOCALYPSE_ObjType_Rubble3,
	APOCALYPSE_ObjType_Rubble4,
	APOCALYPSE_ObjType_Rubble5,
	APOCALYPSE_ObjType_Rubble6,
	
	APOCALYPSE_ObjType_TeleporterMap0
};



/******************* CLOUD *************************/

enum
{
	CLOUD_ObjType_Cyclorama,
	CLOUD_ObjType_Post0,
	CLOUD_ObjType_Post1,
	CLOUD_ObjType_Post2,
	CLOUD_ObjType_Post3,
	CLOUD_ObjType_CloudPlatform,
	CLOUD_ObjType_CloudTunnel_Frame,
	CLOUD_ObjType_CloudTunnel_Tube,
	
	CLOUD_ObjType_Cannon,
	CLOUD_ObjType_Pedestal,
	
	CLOUD_ObjType_BumperCar,
	CLOUD_ObjType_ClownBumperCar,
	CLOUD_ObjType_TireBumper,
	CLOUD_ObjType_Generator,
	CLOUD_ObjType_GeneratorBumper,
	CLOUD_ObjType_BumperGatePosts,
	CLOUD_ObjType_BumperGateBeams,
	
	CLOUD_ObjType_FishBomb,
	CLOUD_ObjType_ConeBlast,
	CLOUD_ObjType_Shockwave,
	
	CLOUD_ObjType_ClownBubble,
	
	CLOUD_ObjType_Balloon,
	CLOUD_ObjType_BalloonString,
	
	CLOUD_ObjType_Topiary0,
	CLOUD_ObjType_Topiary1,
	CLOUD_ObjType_Topiary2,
	
	CLOUD_ObjType_RocketSled,
	
	CLOUD_ObjType_TrapDoor,
	CLOUD_ObjType_ZigZag_Blue,
	CLOUD_ObjType_ZigZag_Red,
	
	CLOUD_ObjType_BrassPost
};


/******************* JUNGLE *************************/

enum
{
	JUNGLE_ObjType_Cyclorama,
	
	JUNGLE_ObjType_Gate,
	JUNGLE_ObjType_GateChunk0,
	JUNGLE_ObjType_GateChunk1,
	JUNGLE_ObjType_GateChunk2,
	
	JUNGLE_ObjType_Hut,
	JUNGLE_ObjType_FireRing,
	JUNGLE_ObjType_AcidDrop,

	JUNGLE_ObjType_GrowthPOW,
	
	JUNGLE_ObjType_LeafPlatform0,
	JUNGLE_ObjType_LeafPlatform1,
	JUNGLE_ObjType_LeafPlatform2,
	JUNGLE_ObjType_LeafPlatform3,
	JUNGLE_ObjType_LeafPlatformShadow,
	
	JUNGLE_ObjType_Fern,
	JUNGLE_ObjType_Flower,
	JUNGLE_ObjType_LeafyPlant,
	
	JUNGLE_ObjType_WoodPost,
	
	JUNGLE_ObjType_TentacleGenerator,
	JUNGLE_ObjType_PitcherPod_Stem,
	JUNGLE_ObjType_PitcherPod_Pod,
	JUNGLE_ObjType_PitcherPod_DeadPod,
	JUNGLE_ObjType_PitcherPod_Pollen,
	JUNGLE_ObjType_PitcherPlant_Grass,
	JUNGLE_ObjType_PitcherPlant_Dead,
	
	JUNGLE_ObjType_TractorBeam,
	JUNGLE_ObjType_TractorBeamPost
};



/******************* FIREICE *************************/

enum
{
	FIREICE_ObjType_Cyclorama,
	FIREICE_ObjType_LavaPillar_Full,
	FIREICE_ObjType_LavaPillar_Segment,
	FIREICE_ObjType_Boulder,
	FIREICE_ObjType_SnowBall,
		
	FIREICE_ObjType_Peoplecicle,
	FIREICE_ObjType_SaucerIce,
	FIREICE_ObjType_Saucer,
	FIREICE_ObjType_Hatch,
	FIREICE_ObjType_IceCrack,
		
	FIREICE_ObjType_JawsBot_Body,
	FIREICE_ObjType_JawsBot_Jaw,
	FIREICE_ObjType_JawsBot_Wheels,
	FIREICE_ObjType_JawsBot_Chunk_Jaw,
	FIREICE_ObjType_JawsBot_Chunk_Jaw2,
	FIREICE_ObjType_JawsBot_Chunk_Wheel,

	FIREICE_ObjType_HammerBot_Body,
	FIREICE_ObjType_HammerBot_Hammer,
	FIREICE_ObjType_HammerBot_Wheels,
	FIREICE_ObjType_HammerBot_Chunk_Body,
	FIREICE_ObjType_HammerBot_Chunk_Hammer,
	FIREICE_ObjType_HammerBot_Chunk_Wheel,

	FIREICE_ObjType_DrillBot_Body,
	FIREICE_ObjType_DrillBot_Drill,
	FIREICE_ObjType_DrillBot_Wheels,
	FIREICE_ObjType_DrillBot_Chunk_Drill,
	FIREICE_ObjType_DrillBot_Chunk_Wheel,
	
	FIREICE_ObjType_SwingerBot_Body,
	FIREICE_ObjType_SwingerBot_Treads,
	FIREICE_ObjType_SwingerBot_Pivot,
	FIREICE_ObjType_SwingerBot_SmallGear,
	FIREICE_ObjType_SwingerBot_Mace,
	FIREICE_ObjType_SwingerBot_Chunk_BigGear,
	FIREICE_ObjType_SwingerBot_Chunk_SmallGear,
	FIREICE_ObjType_SwingerBot_Chunk_Mace,
	FIREICE_ObjType_SwingerBot_Chunk_Pivot,
	FIREICE_ObjType_SwingerBot_Chunk_Tread,

	
	FIREICE_ObjType_Icicle,

	FIREICE_ObjType_ZipLinePost,
	FIREICE_ObjType_ZipLinePully,
	
	FIREICE_ObjType_Stone_Blance,
	FIREICE_ObjType_Stone_Monolith,
	FIREICE_ObjType_Stone_Henge,
	FIREICE_ObjType_Stone_Balance2,
	FIREICE_ObjType_Stone_Donut,
	FIREICE_ObjType_Stone_Claw,
	FIREICE_ObjType_Stone_BlanceIce,
	FIREICE_ObjType_Stone_MonolithIce,
	FIREICE_ObjType_Stone_HengeIce,
	FIREICE_ObjType_Stone_Balance2Ice,
	FIREICE_ObjType_Stone_DonutIce,
	FIREICE_ObjType_Stone_ClawIce,
	
	FIREICE_ObjType_PineTree,
	FIREICE_ObjType_SnowTree,
	
	FIREICE_ObjType_LavaPlatform,
	
	FIREICE_ObjType_RocketSled,
	
	FIREICE_ObjType_RockPost,
	FIREICE_ObjType_IcePost,
	
	FIREICE_ObjType_Blobule,
	FIREICE_ObjType_BlobDroplet

	

};

/******************* SAUCER *************************/

enum
{
	SAUCER_ObjType_Saucer,
	SAUCER_ObjType_PeopleHut,

	SAUCER_ObjType_Beemer,
	SAUCER_ObjType_BeemerBeam,
	
	SAUCER_ObjType_RailGun,
	SAUCER_ObjType_RailGunBeam,
	
	SAUCER_ObjType_TurretBase,
	SAUCER_ObjType_Turret,
	
	SAUCER_ObjType_DishBase,
	SAUCER_ObjType_Dish	
};

/******************* BRAIN BOSS *************************/

enum
{
	BRAINBOSS_ObjType_Cyc,
	BRAINBOSS_ObjType_BrainCore,
	BRAINBOSS_ObjType_LeftBrain,
	BRAINBOSS_ObjType_RightBrain,
	BRAINBOSS_ObjType_NeuronStrand,
	BRAINBOSS_ObjType_BrainPort,
	BRAINBOSS_ObjType_FencePost
};

/******************* HIGH SCORES *************************/

enum
{
	HIGHSCORES_ObjType_Cyc,
};

