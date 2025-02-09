/****************************/
/*    NEW GAME - MAIN 		*/
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void InitArea(void);
static void CleanupLevel(void);
static void PlayArea(void);
static void PlayGame(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	NORMAL_GRAVITY	5200.0f


#define	DEFAULT_ANAGLYPH_R	0xd8
#define	DEFAULT_ANAGLYPH_G	0x90
#define	DEFAULT_ANAGLYPH_B	0xff


static const short kLevelSongs[NUM_LEVELS] =
{
	[LEVEL_NUM_FARM]		= SONG_FARM,
	[LEVEL_NUM_BLOB]		= SONG_SLIME,
	[LEVEL_NUM_BLOBBOSS]	= SONG_SLIMEBOSS,
	[LEVEL_NUM_CLOUD]		= SONG_CLOUD,
	[LEVEL_NUM_APOCALYPSE]	= SONG_APOCALYPSE,
	[LEVEL_NUM_JUNGLE]		= SONG_JUNGLE,
	[LEVEL_NUM_JUNGLEBOSS]	= SONG_JUNGLEBOSS,
	[LEVEL_NUM_FIREICE]		= SONG_FIREICE,
	[LEVEL_NUM_SAUCER]		= SONG_SAUCER,
	[LEVEL_NUM_BRAINBOSS]	= SONG_BRAINBOSS,
};


/****************************/
/*    VARIABLES             */
/****************************/

Boolean				gG4 = true;
Boolean				gIsInGame = false;
Boolean				gSkipFluff = false;

float				gGravity = NORMAL_GRAVITY;

Byte				gDebugMode = 0;				// 0 == none, 1 = fps, 2 = all

uint32_t			gAutoFadeStatusBits;

OGLSetupOutputType		*gGameViewInfoPtr = nil;

PrefsType			gGamePrefs;

OGLVector3D			gWorldSunDirection = { .5, -.35, .8};		// also serves as lense flare vector
OGLColorRGBA		gFillColor1 = { .9, .9, 0.85, 1};
OGLColorRGBA		gApocalypseColor = {.6, .6, .7,1};
OGLColorRGBA		gFireIceColor = {.7, .6, .6,1};

uint32_t			gGameFrameNum = 0;

Boolean				gPlayingFromSavedGame = false;
Boolean				gGameOver = false;
Boolean				gLevelCompleted = false;
float				gLevelCompletedCoolDownTimer = 0;

int					gLevelNum = 0;

short				gBestCheckpointNum;
OGLPoint2D			gBestCheckpointCoord;
float				gBestCheckpointAim;

uint32_t			gScore;
uint32_t			gLoadedScore;




//======================================================================================
//======================================================================================
//======================================================================================



/****************** TOOLBOX INIT  *****************/
//
// Note: the preferences are loaded in Main.cpp
//

void ToolBoxInit(void)
{
	SetFullscreenMode(true);

	OGL_Boot();

 	InitInput();

	LoadLocalizedStrings(gGamePrefs.language);

	MyFlushEvents();
}


/********** INIT DEFAULT PREFS ******************/

void InitDefaultPrefs(void)
{
	SDL_memset(&gGamePrefs, 0, sizeof(gGamePrefs));

	gGamePrefs.language						= GetBestLanguageIDFromSystemLocale();
	gGamePrefs.fullscreen					= true;
	gGamePrefs.vsync						= true;
	gGamePrefs.uiCentering					= false;
	gGamePrefs.uiScaleLevel					= DEFAULT_UI_SCALE_LEVEL;
	gGamePrefs.autoAlignCamera				= true;
	gGamePrefs.displayNumMinus1				= 0;
	gGamePrefs.antialiasingLevel			= 0;
	gGamePrefs.music						= true;
	gGamePrefs.playerRelControls			= false;
	gGamePrefs.mouseControlsOtto			= true;
	gGamePrefs.snappyCameraControl			= true;
	gGamePrefs.mouseSensitivityLevel		= DEFAULT_MOUSE_SENSITIVITY_LEVEL;
	gGamePrefs.anaglyphMode					= ANAGLYPH_OFF;
	gGamePrefs.anaglyphCalibrationRed		= DEFAULT_ANAGLYPH_R;
	gGamePrefs.anaglyphCalibrationGreen		= DEFAULT_ANAGLYPH_G;
	gGamePrefs.anaglyphCalibrationBlue		= DEFAULT_ANAGLYPH_B;
	gGamePrefs.doAnaglyphChannelBalancing	= true;
	gGamePrefs.gamepadRumble				= true;

	for (int i = 0; i < NUM_REMAPPABLE_NEEDS; i++)
	{
		gGamePrefs.remappableKeys[i] = kDefaultKeyBindings[i];
	}
}






#pragma mark -

/******************** PLAY GAME ************************/

static void PlayGame(void)
{
			/***********************/
			/* GAME INITIALIZATION */
			/***********************/

	gDoDeathExit = false;

	InitPlayerInfo_Game();					// init player info for entire game
	InitHelpMessages();						// init all help messages


			/*********************************/
			/* PLAY THRU LEVELS SEQUENTIALLY */
			/*********************************/

	for (;gLevelNum < 10; gLevelNum++)
	{
				/* KICK OFF MUSIC */

		PlaySong(kLevelSongs[gLevelNum], 0);

				/* DO LEVEL INTRO */

		DoLevelIntro();

		MyFlushEvents();


	        /* LOAD ALL OF THE ART & STUFF */

		InitArea();


			/***********/
	        /* PLAY IT */
	        /***********/

		PlayArea();

			/* CLEANUP LEVEL */

		MyFlushEvents();
		CleanupLevel();


			/***************/
			/* SEE IF LOST */
			/***************/

		if (gGameOver)									// bail out if game has ended
		{
			DoLoseScreen();
			break;
		}


		/* DO END-LEVEL BONUS SCREEN */

		DoBonusScreen();
	}


			/**************/
			/* SEE IF WON */
			/**************/

	if (gLevelNum == 10)
	{
		DoWinScreen();
	}


			/* DO HIGH SCORES */

	NewScore();
}




/**************** PLAY AREA ************************/

static void PlayArea(void)
{

			/* PREP STUFF */

	CaptureMouse(true);
	UpdateInput();
	CalcFramesPerSecond();
	CalcFramesPerSecond();
	gDisableHiccupTimer = true;
	gIsInGame = true;

	MakeFadeEvent(true, 1.0);

		/******************/
		/* MAIN GAME LOOP */
		/******************/

	while(true)
	{
				/* GET CONTROL INFORMATION FOR THIS FRAME */
				//
				// Also gathers frame rate info for the net clients.
				//


		UpdateInput();									// read local keys

				/* MOVE OBJECTS */

		MoveEverything();


			/* UPDATE THE TERRAIN */

		DoPlayerTerrainUpdate(gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z);


			/* DRAW IT ALL */


		OGL_DrawScene(DrawObjects);



			/**************/
			/* MISC STUFF */
			/**************/

			/* SEE IF PAUSED */

		if (GetNewNeedState(kNeed_UIPause) || IsCmdQPressed())
		{
			CaptureMouse(false);
			DoPaused();
			CaptureMouse(true);
		}

		CalcFramesPerSecond();

		gGameFrameNum++;

#if !_DEBUG
		if (GetKeyState(SDL_SCANCODE_GRAVE))							// cheat key
#endif
		{
			if (GetNewKeyState(SDL_SCANCODE_F9))						// goto checkpoint
			{
				DoWarpCheat();
			}
			else if (GetNewKeyState(SDL_SCANCODE_F10))					// win level cheat
			{
				break;
			}
		}

				/* SEE IF TRACK IS COMPLETED */

		if (gGameOver)													// if we need immediate abort, then bail now
			break;

		if (gLevelCompleted)
		{
			gLevelCompletedCoolDownTimer -= gFramesPerSecondFrac;		// game is done, but wait for cool-down timer before bailing
			if (gLevelCompletedCoolDownTimer <= 0.0f)
				break;
		}

		gDisableHiccupTimer = false;									// reenable this after the 1st frame

	}

	gIsInGame = false;

	OGL_FadeOutScene(DrawObjects, PausedUpdateCallback);


	CaptureMouse(false);

}


/****************** DRAW AREA EXTRA STUFF *******************************/

static void DrawAreaExtraStuff(ObjNode* theNode)
{
	(void) theNode;

			/* DRAW MISC */

	DrawShards();												// draw shards
	DrawVaporTrails();											// draw vapor trails
	DrawLensFlare();											// draw lens flare
	DrawInfobar();												// draw infobar last
	DrawDeathExit();											// draw death exit stuff
}


/******************** MOVE EVERYTHING ************************/

void MoveEverything(void)
{
float	fps = gFramesPerSecondFrac;

	MoveVaporTrails();
	MoveObjects();
	MoveSplineObjects();
	MoveShards();
	UpdateCamera();								// update camera

	/* LEVEL SPECIFIC UPDATES */

	switch(gLevelNum)
	{
		case	LEVEL_NUM_BLOB:
				MO_Object_OffsetUVs(gBG3DGroupList[MODEL_GROUP_LEVELSPECIFIC][SLIME_ObjType_BumperBubble], fps * .3, 0);	// scroll bumper bubble
				MO_Geometry_OffserUVs(MODEL_GROUP_LEVELSPECIFIC, SLIME_ObjType_BlobArrow, 0, fps * -.4, 0);	// scroll arrow texture
				break;

		case	LEVEL_NUM_BLOBBOSS:
				gSpinningPlatformRot += SPINNING_PLATFORM_SPINSPEED * fps;
				if (gSpinningPlatformRot > PI2)																			// wrap back rather than getting bigger and bigger
					gSpinningPlatformRot -= PI2;

				MO_Geometry_OffserUVs(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_Tube_Bent, 2,  	0, -fps);			// scroll pipe slime texutre ooze
				MO_Geometry_OffserUVs(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_Tube_Loop1, 2,  	0, fps);
				MO_Geometry_OffserUVs(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_Tube_Loop2, 1,  	0, fps);
				MO_Geometry_OffserUVs(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_Tube_Straight1,1, 0, -fps);
				MO_Geometry_OffserUVs(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_Tube_Straight2,1, 0, -fps);

				MO_Geometry_OffserUVs(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_TubeSegment,1, 0, -fps);
				break;

		case	LEVEL_NUM_CLOUD:
				UpdateZigZagSlats();
				break;

	}

}


/***************** INIT AREA ************************/

static void InitArea(void)
{
OGLSetupInputType	viewDef;
DeformationType		defData;



			/* INIT SOME PRELIM STUFF */

	gGameFrameNum 		= 0;
	gGameOver 			= false;
	gLevelCompleted 	= false;
	gBestCheckpointNum	= -1;


	gPlayerInfo.objNode = nil;
	gPlayerInfo.didCheat = false;
	gPlayerInfo.isTeleporting = false;




			/*************/
			/* MAKE VIEW */
			/*************/



			/* SETUP VIEW DEF */

	OGL_NewViewDef(&viewDef);

	viewDef.camera.hither 			= 50;
	viewDef.camera.yon 				= (SUPERTILE_ACTIVE_RANGE * SUPERTILE_SIZE * TERRAIN_POLYGON_SIZE) * .95f;
	viewDef.camera.fov 				= GAME_FOV;


	switch(gLevelNum)
	{
		case	LEVEL_NUM_BLOB:
				viewDef.camera.yon				*= .6f;								// bring in the yon
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= .8;
				viewDef.view.clearColor.g 		= .6;
				viewDef.view.clearColor.b		= .8;
				viewDef.view.clearBackBuffer	= true;
				viewDef.styles.fogStart			= viewDef.camera.yon * .1f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * .95f;
				gDrawLensFlare = true;
				break;


		case	LEVEL_NUM_BLOBBOSS:
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= .17;
				viewDef.view.clearColor.g 		= .05;
				viewDef.view.clearColor.b		= .29;
				viewDef.view.clearBackBuffer	= true;
				viewDef.styles.fogStart			= viewDef.camera.yon * .4f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * 1.1f;
				gDrawLensFlare = false;
				break;

		case	LEVEL_NUM_APOCALYPSE:
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= 0;
				viewDef.view.clearColor.g 		= 0;
				viewDef.view.clearColor.b		= 0;
				viewDef.view.clearBackBuffer	= true;
				viewDef.styles.fogStart			= viewDef.camera.yon * .15f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * 1.05f;
				gDrawLensFlare = false;
				break;

		case	LEVEL_NUM_CLOUD:
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= .686;
				viewDef.view.clearColor.g 		= .137;
				viewDef.view.clearColor.b		= .431;
				viewDef.view.clearBackBuffer	= false;
				viewDef.styles.fogStart			= viewDef.camera.yon * .3f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * .8f;
				gDrawLensFlare = false;
				break;


		case	LEVEL_NUM_JUNGLE:
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= .6;
				viewDef.view.clearColor.g 		= .6;
				viewDef.view.clearColor.b		= .3;
				viewDef.view.clearBackBuffer	= false;
				viewDef.styles.fogStart			= viewDef.camera.yon * .4f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * 1.1f;
				gDrawLensFlare = false;
				break;

		case	LEVEL_NUM_FIREICE:
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= 0;
				viewDef.view.clearColor.g 		= 0;
				viewDef.view.clearColor.b		= 0;
				viewDef.view.clearBackBuffer	= false;
				viewDef.styles.fogStart			= viewDef.camera.yon * .4f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * 1.1f;
				gDrawLensFlare = false;
				break;

		case	LEVEL_NUM_SAUCER:
				viewDef.camera.yon				*= .8f;						// bring it in a little for this

				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= .2;
				viewDef.view.clearColor.g 		= .4;
				viewDef.view.clearColor.b		= .7;
				viewDef.view.clearBackBuffer	= true;
				viewDef.styles.fogStart			= viewDef.camera.yon * .2f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * .9f;
				gDrawLensFlare = false;
				break;

		case	LEVEL_NUM_BRAINBOSS:
				viewDef.camera.yon				*= .7f;								// bring in the yon
				viewDef.styles.useFog			= true;
				viewDef.view.clearColor.r 		= .1; //.4;
				viewDef.view.clearColor.g 		= 0; //.7;
				viewDef.view.clearColor.b		= 0; //.2;
				viewDef.view.clearBackBuffer	= true;
				viewDef.styles.fogStart			= viewDef.camera.yon * .6f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * 1.0f;
				gDrawLensFlare = false;
				break;


		default:
				viewDef.styles.useFog			= false;
				viewDef.view.clearColor.r 		= .1;
				viewDef.view.clearColor.g 		= .5;
				viewDef.view.clearColor.b		= .1;
				viewDef.view.clearBackBuffer	= false;
				viewDef.styles.fogStart			= viewDef.camera.yon * .8f;
				viewDef.styles.fogEnd			= viewDef.camera.yon * 1.0f;
				gDrawLensFlare = true;
	}

		/**************/
		/* SET LIGHTS */
		/**************/

	switch(gLevelNum)
	{
		case	LEVEL_NUM_BLOBBOSS:
				gWorldSunDirection.x 	= .5;
				gWorldSunDirection.y 	= -.35;
				gWorldSunDirection.z 	= .8;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;
				viewDef.lights.ambientColor.r 		= .2;
				viewDef.lights.ambientColor.g 		= .2;
				viewDef.lights.ambientColor.b 		= .2;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0] 		= gFillColor1;
				break;

		case	LEVEL_NUM_APOCALYPSE:
				gWorldSunDirection.x 	= .5;
				gWorldSunDirection.y 	= -.6;
				gWorldSunDirection.z 	= .8;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;
				viewDef.lights.ambientColor.r 		= .2;
				viewDef.lights.ambientColor.g 		= .2;
				viewDef.lights.ambientColor.b 		= .2;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0] 		= gApocalypseColor;
				break;



		case	LEVEL_NUM_JUNGLE:
				gWorldSunDirection.x 	= .5;
				gWorldSunDirection.y 	= -.8;
				gWorldSunDirection.z 	= .8;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;
				viewDef.lights.ambientColor.r 		= .3;
				viewDef.lights.ambientColor.g 		= .3;
				viewDef.lights.ambientColor.b 		= .3;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0] 		= gFillColor1;
				break;

		case	LEVEL_NUM_JUNGLEBOSS:
				gWorldSunDirection.x 	= .1;
				gWorldSunDirection.y 	= -.5;
				gWorldSunDirection.z 	= -1;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;
				viewDef.lights.ambientColor.r 		= .3;
				viewDef.lights.ambientColor.g 		= .3;
				viewDef.lights.ambientColor.b 		= .3;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0] 		= gFillColor1;
				break;

		case	LEVEL_NUM_FIREICE:
				gWorldSunDirection.x 	= .5;
				gWorldSunDirection.y 	= -1;
				gWorldSunDirection.z 	= 0;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;
				viewDef.lights.ambientColor.r 		= .3;
				viewDef.lights.ambientColor.g 		= .3;
				viewDef.lights.ambientColor.b 		= .2;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0]			= gFireIceColor;
				break;

		case	LEVEL_NUM_SAUCER:
				gWorldSunDirection.x 	= .5;
				gWorldSunDirection.y 	= -.35;
				gWorldSunDirection.z 	= -.8;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;
				viewDef.lights.ambientColor.r 		= .3;
				viewDef.lights.ambientColor.g 		= .25;
				viewDef.lights.ambientColor.b 		= .25;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0] 		= gFillColor1;
				break;

		case	LEVEL_NUM_BRAINBOSS:
				gWorldSunDirection.x 	= -.5;
				gWorldSunDirection.y 	= -.7;
				gWorldSunDirection.z 	= .8;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;
				viewDef.lights.ambientColor.r 		= .3;
				viewDef.lights.ambientColor.g 		= .3;
				viewDef.lights.ambientColor.b 		= .3;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0] 		= gFillColor1;
				break;

		default:

				gWorldSunDirection.x = .5;
				gWorldSunDirection.y = -.35;
				gWorldSunDirection.z = .8;
				OGLVector3D_Normalize(&gWorldSunDirection,&gWorldSunDirection);

				viewDef.lights.numFillLights 		= 1;
				viewDef.lights.ambientColor.r 		= .4;
				viewDef.lights.ambientColor.g 		= .4;
				viewDef.lights.ambientColor.b 		= .36;
				viewDef.lights.fillDirection[0] 	= gWorldSunDirection;
				viewDef.lights.fillColor[0] 		= gFillColor1;
	}


			/*********************/
			/* SET ANAGLYPH INFO */
			/*********************/

	if (gGamePrefs.anaglyphMode != ANAGLYPH_OFF)
	{
		if (gGamePrefs.anaglyphMode == ANAGLYPH_MONO)
		{
			viewDef.lights.ambientColor.r 		+= .1f;					// make a little brighter
			viewDef.lights.ambientColor.g 		+= .1f;
			viewDef.lights.ambientColor.b 		+= .1f;
		}

		gAnaglyphFocallength	= 300.0f;
		gAnaglyphEyeSeparation 	= 40.0f;
	}




	OGL_SetupWindow(&viewDef);


			/**********************/
			/* SET AUTO-FADE INFO */
			/**********************/

	switch(gLevelNum)
	{
		case	LEVEL_NUM_BLOB:
				gAutoFadeStartDist = 0;						// no auto fade here - we gots fog
				break;

		case	LEVEL_NUM_APOCALYPSE:
				gAutoFadeStartDist	= gGameViewInfoPtr->yon * .75;
				gAutoFadeEndDist	= gGameViewInfoPtr->yon * .85f;
				break;

		case	LEVEL_NUM_SAUCER:
				gAutoFadeStartDist	= gGameViewInfoPtr->yon * .90;
				gAutoFadeEndDist	= gGameViewInfoPtr->yon * 1.0f;
				break;

		default:
				gAutoFadeStartDist	= gGameViewInfoPtr->yon * .80;
				gAutoFadeEndDist	= gGameViewInfoPtr->yon * .90f;

	}


	if (!gG4)												// bring it in a little for slow machines
	{
		gAutoFadeStartDist *= .85f;
		gAutoFadeEndDist *= .85f;
	}

	gAutoFadeRange_Frac	= 1.0f / (gAutoFadeEndDist - gAutoFadeStartDist);

	if (gAutoFadeStartDist != 0.0f)
		gAutoFadeStatusBits = STATUS_BIT_AUTOFADE;
	else
		gAutoFadeStatusBits = 0;



			/**********************/
			/* LOAD ART & TERRAIN */
			/**********************/
			//
			// NOTE: only call this *after* draw context is created!
			//

	LoadLevelArt();
	InitInfobar();
	gAlienSaucer = nil;

			/* INIT OTHER MANAGERS */

	InitEnemyManager();
	InitHumans();
	InitEffects();
	InitVaporTrails();
	InitItemsManager();
	InitSky();


			/* DRAW SHARDS, THE INFOBAR, ETC. AFTER ALL GAMEPLAY OBJECTS */

	NewObjectDefinitionType drawExtraDef =
	{
		.scale = 1,
		.genre = CUSTOM_GENRE,
		.flags = STATUS_BIT_DONTCULL,
		.slot = DRAWEXTRA_SLOT,
		.drawCall = DrawAreaExtraStuff
	};
	MakeNewObject(&drawExtraDef);



			/****************/
			/* INIT SPECIAL */
			/****************/

	gGravity = NORMAL_GRAVITY;					// assume normal gravity
	gTileSlipperyFactor = 0.0f;					// assume normal slippery

	switch(gLevelNum)
	{
		case	LEVEL_NUM_BLOB:

				gTileSlipperyFactor = .1f;

				/* INIT DEFORMATIONS */

				if (gG4)										// only do this on a G4 since we need the horsepower
				{
					defData.type 				= DEFORMATION_TYPE_JELLO;
					defData.amplitude 			= 15;
					defData.radius 				= 0;
					defData.speed 				= 200;
					defData.origin.x			= gTerrainUnitWidth;
					defData.origin.y			= 0;
					defData.oneOverWaveLength 	= 1.0f / 300.0f;
					NewSuperTileDeformation(&defData);

					defData.type 				= DEFORMATION_TYPE_JELLO;
					defData.amplitude 			= 10;
					defData.radius 				= 0;
					defData.speed 				= 300;
					defData.origin.x			= 0;
					defData.origin.y			= 0;
					defData.oneOverWaveLength 	= 1.0f / 200.0f;
					NewSuperTileDeformation(&defData);
				}
				break;

		case	LEVEL_NUM_BLOBBOSS:

				gGravity = NORMAL_GRAVITY*3/4;						// low gravity

					/* MAKE THE BLOB BOSS MACHINE */

				MakeBlobBossMachine();


				if (gG4)										// only do this on a G4 since we need the horsepower
				{
					/* INIT DEFORMATIONS */

					defData.type 				= DEFORMATION_TYPE_JELLO;
					defData.amplitude 			= 90;
					defData.radius 				= 0;
					defData.speed 				= 900;
					defData.origin.x			= gTerrainUnitWidth;
					defData.origin.y			= 0;
					defData.oneOverWaveLength 	= 1.0f / 300.0f;
					NewSuperTileDeformation(&defData);

					defData.type 				= DEFORMATION_TYPE_JELLO;
					defData.amplitude 			= 60;
					defData.radius 				= 0;
					defData.speed 				= 600;
					defData.origin.x			= 0;
					defData.origin.y			= 0;
					defData.oneOverWaveLength 	= 1.0f / 400.0f;
					NewSuperTileDeformation(&defData);
				}
				break;

		case	LEVEL_NUM_APOCALYPSE:
				InitZipLines();
				InitTeleporters();
				InitSpacePods();
				break;

		case	LEVEL_NUM_CLOUD:
				InitBumperCars();
				break;

		case	LEVEL_NUM_JUNGLEBOSS:
				InitJungleBossStuff();
				break;

		case	LEVEL_NUM_FIREICE:
				InitZipLines();
				break;

		case	LEVEL_NUM_BRAINBOSS:
				InitBrainBoss();
				break;

	}

		/* INIT THE PLAYER & RELATED STUFF */

	PrimeWater();								// NOTE:  must do this before items get added since some items may be on the water
	InitPlayersAtStartOfLevel();				// NOTE:  this will also cause the initial items in the start area to be created

	PrimeSplines();
	PrimeFences();

			/* INIT CAMERAS */

	InitCamera();

	SDL_HideCursor();							// do this again to be sure!
 }


/**************** CLEANUP LEVEL **********************/

static void CleanupLevel(void)
{
	SDL_memset(gRocketShipHotZone, 0, sizeof(gRocketShipHotZone));

	StopAllEffectChannels();
 	EmptySplineObjectList();
	DisposeInfobar();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeVaporTrails();
	DisposeSuperTileMemoryList();
	DisposeTerrain();
	DisposeSky();
	DisposeEffects();
	DisposeAllSpriteGroups();
	DisposeFences();

	DisposeAllBG3DContainers();


	DisposeSoundBank(kLevelSoundBanks[gLevelNum]);

	OGL_DisposeWindowSetup();	// do this last!

	Pomme_FlushPtrTracking(true);

	gPlayerInfo.objNode = nil;
	gPlayerInfo.leftHandObj = nil;
	gPlayerInfo.rightHandObj = nil;

	gAlienSaucer = nil;
	gSaucerTarget = nil;
}

/************ CHEAT KEYS CHECKED AFTER LEGAL SCREEN ******************/

static void CheckBootCheats(void)
{
		/* SKIP FLUFF */

	if (GetKeyState(SDL_SCANCODE_F))
	{
		gSkipFluff = true;
	}

		/* TEST HIGH SCORE SCREEN: HOLD DOWN MINUS KEY AFTER LEGAL SCREEN */

	if (GetKeyState(SDL_SCANCODE_MINUS))
	{
		gScore = RandomRange(100, 65535);
		NewScore();
		return;
	}

		/* TEST WIN SCREEN */

	if (GetKeyState(SDL_SCANCODE_W))
	{
		DoWinScreen();
		return;
	}

		/* TEST LOSE SCREEN */

	if (GetKeyState(SDL_SCANCODE_L))
	{
		DoLoseScreen();
		return;
	}

		/* LEVEL CHEAT: HOLD DOWN NUMBER KEY AFTER LEGAL SCREEN */

	for (int i = 0; i < 10; i++)
	{
		if (GetKeyState(SDL_SCANCODE_1 + i))	// scancodes 1,2,...,9,0 are contiguous
		{
			gLevelNum = i;
			PlayGame();
			return;
		}
	}
}

/************************************************************/
/******************** PROGRAM MAIN ENTRY  *******************/
/************************************************************/


void GameMain(void)
{
				/**************/
				/* BOOT STUFF */
				/**************/

	ToolBoxInit();

			/* INIT SOME OF MY STUFF */

	InitSpriteManager();
	InitBG3DManager();
	InitObjectManager();
	InitWindowStuff();
	InitTerrainManager();
	InitSkeletonManager();
	InitSoundTools();
	TextMesh_LoadMetrics();


			/* INIT MORE MY STUFF */


	{
			/* INIT RANDOM SEED */

		unsigned long someLong;
		GetDateTime(&someLong);
		SetMyRandomSeed((uint32_t) someLong);
	}

	SDL_HideCursor();

	Pomme_FlushPtrTracking(false);

#if _DEBUG
	gDebugMode = 1;
#endif

		/* SHOW LEGAL SCREEN */

	DoLegalScreen();

		/* DO BOOT CHEATS */

	CheckBootCheats();

		/* MAIN LOOP */

	while(true)
	{
		MyFlushEvents();
		DoMainMenuScreen();

		PlayGame();
	}

}




