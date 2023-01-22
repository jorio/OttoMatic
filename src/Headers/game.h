#pragma once

		/* MY BUILD OPTIONS */

// Default to little-endian
#if !defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)
	#define __LITTLE_ENDIAN__ 1
#endif

#if _MSC_VER
	#define _Static_assert static_assert
#endif

#ifdef __cplusplus
extern "C"
{
#endif

// If enabled, "VIP" enemies are always allowed to spawn and they don't count towards the global enemy budget.
// VIP enemy kinds are: GiantLizard and Flytrap.
#define VIP_ENEMIES 1

		/* HEADERS */

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>
#include <stdio.h>

#include "Pomme.h"

#include "pool.h"
#include "globals.h"
#include "structs.h"
#include "metaobjects.h"
#include "ogl_support.h"
#include "main.h"
#include "mobjtypes.h"
#include "misc.h"
#include "sound2.h"
#include "sobjtypes.h"
#include "sprites.h"
#include "sparkle.h"
#include "bg3d.h"
#include "camera.h"
#include "collision.h"
#include 	"input.h"
#include "file.h"
#include "window.h"
#include "player.h"
#include "terrain.h"
#include "humans.h"
#include "skeletonobj.h"
#include "skeletonanim.h"
#include "skeletonjoints.h"
#include	"infobar.h"
#include "triggers.h"
#include "effects.h"
#include "shards.h"
#include "bones.h"
#include "vaportrails.h"
#include "splineitems.h"
#include "mytraps.h"
#include "enemy.h"
#include "items.h"
#include "sky.h"
#include "water.h"
#include "fences.h"
#include "miscscreens.h"
#include "objects.h"
#include "lzss.h"
#include "3dmath.h"
#include "ogl_functions.h"
#include "localization.h"
#include "textmesh.h"
#include "tga.h"
#include "menu.h"

		/* EXTERNS */

extern	BG3DFileContainer		*gBG3DContainerList[];
extern	Boolean					gAllowAudioKeys;
extern	Boolean					gAutoRotateCamera;
extern	Boolean					gBrainBossDead;
extern	Boolean					gBumperCarGateBlown[];
extern	Boolean					gDisableAnimSounds;
extern	Boolean					gDisableHiccupTimer;
extern	Boolean					gDoDeathExit;
extern	Boolean					gDoJumpJetAtApex;
extern	Boolean					gDrawLensFlare;
extern	Boolean					gExplodePlayerAfterElectrocute;
extern	Boolean					gForceCameraAlignment;
extern	Boolean					gFreezeCameraFromXZ;
extern	Boolean					gFreezeCameraFromY;
extern	Boolean					gG4;
extern	Boolean					gGameOver;
extern	Boolean					gGamePaused;
extern	Boolean					gHelpMessageDisabled[NUM_HELP_MESSAGES];
extern	Boolean					gIceCracked;
extern	Boolean					gIsInGame;
extern	Boolean					gLevelCompleted;
extern	Boolean					gMouseMotionNow;
extern	Boolean					gMyState_Lighting;
extern	Boolean					gPlayerFellIntoBottomlessPit;
extern	Boolean					gPlayerHasLanded;
extern	Boolean					gPlayerIsDead;
extern	Boolean					gPlayingFromSavedGame;
extern	Boolean					gUserPrefersGamepad;
extern	Byte					**gMapSplitMode;
extern	Byte					gDebugMode;
extern	Byte					gHumansInSaucerList[];
extern	ChannelInfoType			gChannelInfo[];
extern	CollisionBoxType 		*gSaucerIceBounds;
extern	CollisionRec			gCollisionList[];
extern	CommandLineOptions		gCommandLine;
extern	FSSpec					gDataSpec;
extern	FenceDefType			*gFenceList;
extern	HighScoreType			gHighScores[];
extern	MOMaterialObject		*gMostRecentMaterial;
extern	MOMaterialObject		*gSuperTileTextureObjects[MAX_SUPERTILE_TEXTURES];
extern	MOVertexArrayData		**gLocalTriMeshesOfSkelType;
extern	MetaObjectPtr			gBG3DGroupList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	NewParticleGroupDefType	gNewParticleGroupDef;
extern	OGLBoundingBox			gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	OGLBoundingBox			gWaterBBox[];
extern	OGLColorRGB				gGlobalColorFilter;
extern	OGLMatrix4x4			*gCurrentObjMatrix;
extern	OGLMatrix4x4			gViewToFrustumMatrix;
extern	OGLMatrix4x4			gWorldToFrustumMatrix;
extern	OGLMatrix4x4			gWorldToViewMatrix;
extern	OGLMatrix4x4			gWorldToWindowMatrix;
extern	OGLPoint2D				gBestCheckpointCoord;
extern	OGLPoint2D				gRocketShipHotZone[4];
extern	OGLPoint3D				gCoord;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	OGLVector2D				gCameraControlDelta;
extern	OGLVector3D				gDelta;
extern	OGLVector3D				gRecentTerrainNormal;
extern	OGLVector3D				gWorldSunDirection;
extern	ObjNode					*gAlienSaucer;
extern	ObjNode					*gCurrentNode;
extern	ObjNode					*gCurrentZip;
extern	ObjNode					*gExitRocket;
extern	ObjNode					*gFirstNodePtr;
extern	ObjNode					*gMagnetMonsterList[MAX_MAGNET_MONSTERS];
extern	ObjNode					*gPlayerRocketSled;
extern	ObjNode					*gPlayerSaucer;
extern	ObjNode					*gSaucerTarget;
extern	ObjNode					*gSoapBubble;
extern	ObjNode					*gTargetPickup;
extern	ObjNode					*gTractorBeamObj;
extern	Pool 					*gParticleGroupPool;
extern	Pool					*gShardPool;
extern	Pool					*gSparklePool;
extern	PrefsType				gGamePrefs;
extern	SDL_GameController		*gSDLController;
extern	SDL_GLContext			gAGLContext;
extern	SDL_Window				*gSDLWindow;
extern	SparkleType				gSparkles[MAX_SPARKLES];
extern	SplineDefType			**gSplineList;
extern	SpriteType				*gSpriteGroupList[MAX_SPRITE_GROUPS];
extern	SuperTileGridType		**gSuperTileTextureGrid;
extern	SuperTileItemIndexType	**gSuperTileItemIndexGrid;
extern	SuperTileMemoryType		gSuperTileMemoryList[];
extern	SuperTileStatus			**gSuperTileStatusGrid;
extern	TerrainItemEntryType	**gMasterItemList;
extern	TileAttribType			**gTileAttribList;
extern	WaterDefType			**gWaterListHandle;
extern	WaterDefType			*gWaterList;
extern	char					gTextInput[SDL_TEXTINPUTEVENT_TEXT_SIZE];
extern	const KeyBinding		kDefaultKeyBindings[NUM_CONTROL_NEEDS];
extern	const MenuStyle			kDefaultMenuStyle;
extern	const OGLPoint3D		gPlayerMuzzleTipOff;
extern	const int				kLevelSoundBanks[NUM_LEVELS];
extern	float					gSkyAltitudeY;
extern	float					**gMapYCoords;
extern	float					**gMapYCoordsOriginal;
extern	float					g2DLogicalHeight;
extern	float					g2DLogicalWidth;
extern	float					gAutoFadeEndDist;
extern	float					gAutoFadeRange_Frac;
extern	float					gAutoFadeStartDist;
extern	float					gAutoRotateCameraSpeed;
extern	float					gBeamCharge;
extern	float					gBestCheckpointAim;
extern	float					gCameraDistFromMe;
extern	float					gCameraLookAtYOff;
extern	float					gCameraUserRotY;
extern	float					gCurrentAspectRatio;
extern	float					gCurrentMaxSpeed;
extern	float					gDeathTimer;
extern	float					gDischargeTimer;
extern	float					gFramesPerSecond;
extern	float					gFramesPerSecondFrac;
extern	float					gGlobalTransparency;
extern	float					gGravity;
extern	float					gHumanScaleRatio;
extern	float					gJumpJetWarningCooldown;
extern	float					gLevelCompletedCoolDownTimer;
extern	float					gMinHeightOffGround;
extern	float					gPlayerBottomOff;
extern	float					gPlayerToCameraAngle;
extern	float					gRocketScaleAdjust;
extern	float					gSpinningPlatformRot;
extern	float					gTargetMaxSpeed;
extern	float					gTileSlipperyFactor;
extern	float					gTimeSinceLastShoot;
extern	float					gTimeSinceLastThrust;
extern	int						gGameWindowHeight;
extern	int						gGameWindowWidth;
extern	int						gLevelNum;
extern	int						gMaxEnemies;
extern	int						gNumEnemies;
extern	int						gNumEnemyOfKind[NUM_ENEMY_KINDS];
extern	int						gNumHumansInLevel;
extern	int						gNumHumansInTransit;
extern	int						gNumHumansRescuedTotal;
extern	int						gNumIceCracks;
extern	int						gNumObjectNodes;
extern	int						gNumObjectsInBG3DGroupList[MAX_BG3D_GROUPS];
extern	int						gNumSpritesInGroupList[MAX_SPRITE_GROUPS];
extern	int						gPolysThisFrame;
extern	int						gVRAMUsedThisFrame;
extern	int						gNumFences;
extern	int						gNumSplines;
extern	int						gNumSuperTilesDeep;
extern	int						gNumSuperTilesWide;
extern	int						gNumUniqueSuperTiles;
extern	int						gNumWaterPatches;
extern	long					gPrefsFolderDirID;
extern	int						gTerrainTileDepth;
extern	int						gTerrainTileWidth;
extern	int						gTerrainUnitDepth;
extern	int						gTerrainUnitWidth;
extern	int						*gTerrainItemFileIDs;
extern	short					gBeamMode;
extern	short					gBeamModeSelected;
extern	short					gBestCheckpointNum;
extern	short					gDisplayedHelpMessage;
extern	int						gNumCollisions;
extern	int						gNumFencesDrawn;
extern	int						gNumHumansInSaucer;
extern	int						gNumHumansRescuedOfType[NUM_HUMAN_TYPES];
extern	int						gNumSuperTilesDrawn;
extern	int						gNumTerrainDeformations;
extern	int						gNumTerrainItems;
extern	int						gNumWaterDrawn;
extern	short					gPrefsFolderVRefNum;
extern	uint32_t				gAutoFadeStatusBits;
extern	uint32_t				gGameFrameNum;
extern	uint32_t				gGlobalMaterialFlags;
extern	uint32_t				gLoadedScore;
extern	uint32_t				gScore;
extern	uint16_t				**gTileGrid;
extern	uint16_t				gTileAttribFlags;

#ifdef __cplusplus
};
#endif

