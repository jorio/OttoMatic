/****************************/
/*   	BONUS SCREEN.C		*/
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond,gGlobalTransparency;
extern	FSSpec		gDataSpec;
extern	Boolean		gNetGameInProgress,gGameOver, gNoCarControls;
extern	KeyMap gKeyMap,gNewKeys;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean		gSongPlayingFlag,gOSX,gDisableAnimSounds;
extern	PrefsType	gGamePrefs;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	PlayerInfoType	gPlayerInfo;
extern	int				gNumHumansRescuedTotal,gLevelNum;
extern	SparkleType	gSparkles[MAX_SPARKLES];
extern	OGLPoint3D	gCoord;
extern	OGLVector3D	gDelta;
extern	u_long			gScore,gGlobalMaterialFlags;
extern	short	gNumHumansRescuedOfType[NUM_HUMAN_TYPES];
extern	float			gRocketScaleAdjust,gGammaFadePercent;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupBonusScreen(void);
static void FreeBonusScreen(void);
static void DrawBonusCallback(OGLSetupOutputType *info);
static void DrawHumanBonus(OGLSetupOutputType *info);
static void DrawInventoryBonus(OGLSetupOutputType *info);
static void DrawBonus(OGLSetupOutputType *info);
static void DrawInventoryQuantity(OGLSetupOutputType *info);
static void MoveBonusHuman(ObjNode *theNode);
static void MoveBonusHuman_WalkOff(ObjNode *theNode);
static void CreateBonusHumans(void);
static void StartBonusTeleport(ObjNode *human);
static void UpdateBonusTeleport(ObjNode *human);
static void CreateInventoryIcons(void);
static void MoveStar(ObjNode *theNode);
static void MoveBonusRocket(ObjNode *rocket);
static void DoHumansBonusTally(void);
static void DoInventoryBonusTally(void);
static void DoBonusShipDissolve(void);
static void MoveBonusInventoryIcon(ObjNode *theNode);
static void DoTotalBonusTally(void);
static void DrawBonusToScore(OGLSetupOutputType *info);
static void DrawScore(OGLSetupOutputType *info);
static int DoSaveGamePrompt(void);
static int NavigateSaveMenu(void);
static void DoTractorBeam(void);
static void InitBonusTractorBeam(void);
static void MoveBonusTractorBeam(ObjNode *beam);
static void MoveBonusRocket_TractorBeam(ObjNode *rocket);
static void MoveGlowDisc(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	SAVE_TEXT_SIZE		30.0f

#define	POINTS_HUMAN		4000
#define	POINTS_INVENTORY	150

#define	HUMAN_MARGIN	700.0f
#define	HUMAN_SPACING	160.0f

enum
{
	BONUS_ObjType_Star,
	BONUS_ObjType_Rocket,
	BONUS_ObjType_TeleportRing,
	BONUS_ObjType_RocketInside,
	BONUS_ObjType_GlowDiscs,

	BONUS_ObjType_PulseGun,
	BONUS_ObjType_SuperNova,
	BONUS_ObjType_FreezeGun,
	BONUS_ObjType_FlameGun,
	BONUS_ObjType_FlareGun,
	BONUS_ObjType_Dart,
	BONUS_ObjType_Growth,

	BONUS_ObjType_TractorBeam
};



enum
{
	BONUS_SObjType_BonusText,
	BONUS_SObjType_BonusGlow,

	BONUS_SObjType_InventoryText,
	BONUS_SObjType_InventoryGlow,

	BONUS_SObjType_TotalBonusText,
	BONUS_SObjType_TotalBonusGlow,

	BONUS_SObjType_ScoreText,
	BONUS_SObjType_ScoreGlow
};


enum
{
	SHOW_SCORE_MODE_OFF,
	SHOW_SCORE_MODE_HUMANBONUS,
	SHOW_SCORE_MODE_INVENTORY,
	SHOW_SCORE_MODE_ADDTOSCORE
};


enum
{
	SAVE_MENU_SELECTION_NONE,
	SAVE_MENU_SELECTION_SAVE,
	SAVE_MENU_SELECTION_CONTINUE,
};


#define DIGIT_SPACING 	30.0f
#define	DIGIT_SPACING_Q	20.0f						// for inventory quantity number

#define	INVENTORY_COUNT_DELAY	.1f

/*********************/
/*    VARIABLES      */
/*********************/

static const short	gWeaponToIconTable[] =
{
	BONUS_ObjType_PulseGun,
	BONUS_ObjType_FreezeGun,
	BONUS_ObjType_FlameGun,
	BONUS_ObjType_Growth,
	BONUS_ObjType_FlareGun,
	0,									// skip fist
	BONUS_ObjType_SuperNova,
	BONUS_ObjType_Dart,
};


static const float gWeaponIconScale[] =
{
	1.0,	//	BONUS_ObjType_PulseGun,
	1.0,	//	BONUS_ObjType_FreezeGun,
	1.0,	//	BONUS_ObjType_FlameGun,
	.2,		//	BONUS_ObjType_Growth,
	1.0,	//	BONUS_ObjType_FlareGun,
	0,												// skip fist
	1.1,	//	BONUS_ObjType_SuperNova,
	.4,		//	BONUS_ObjType_Dart,
};


static const float gWeaponIconYOff[] =
{
	0.0,	//	BONUS_ObjType_PulseGun,
	0.0,	//	BONUS_ObjType_FreezeGun,
	0.0,	//	BONUS_ObjType_FlameGun,
	-30,	//	BONUS_ObjType_Growth,
	0.0,	//	BONUS_ObjType_FlareGun,
	0.0,												// skip fist
	-50.0,	//	BONUS_ObjType_SuperNova,
	0.0,	//	BONUS_ObjType_Dart,
};


float	gBonusTimer;

#define	InvisibleDelay	SpecialF[0]
#define	RingOffset		SpecialF[0]

#define	Quantity		Special[0]

static ObjNode *gInteriorObj;

static	Byte	gShowScoreMode;

static	u_long	gBonusTotal, gBonus, gInventoryQuantity;

static	float	gScoreAlpha,gInventoryQuantityAlpha,gInventoryCountTimer;

static ObjNode	*gSaveIcons[2];
static	short	gSaveMenuSelection;

static	short	gPointBeepChannel;

static	float	gBonusBeamTimer;


/********************** DO BONUS SCREEN **************************/

void DoBonusScreen(void)
{
	GammaFadeOut();

			/* SETUP */

	SetupBonusScreen();
	MakeFadeEvent(true, 1.0);


		/* DO SHIP DISSOLVE */

	if (gLevelNum != LEVEL_NUM_BLOB)
		DoBonusShipDissolve();


			/* DO HUMANS BONUS */

	DoHumansBonusTally();
	gBonusTotal += gBonus;

			/* DO INVENTORY BONUS */

	DoInventoryBonusTally();
	gBonusTotal += gBonus;

			/* ADD BONUS TO TOTAL */

	DoTotalBonusTally();

			/* DO SAVE GAME */

	if (gLevelNum < LEVEL_NUM_BRAINBOSS)		// dont save on last level
	{
		int saveMenuSelection = DoSaveGamePrompt();

		if (saveMenuSelection == SAVE_MENU_SELECTION_SAVE)
		{
			DoFileScreen(FILE_SCREEN_TYPE_SAVE, DrawBonusCallback);
		}
	}


		/* DO TRACTOR BEAM FOR JUNGLE-BOSS */

	if (gLevelNum == LEVEL_NUM_JUNGLE)
		DoTractorBeam();



			/* CLEANUP */

	GammaFadeOut();
	FreeBonusScreen();
}



/********************* SETUP BONUS SCREEN **********************/

static void SetupBonusScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
ObjNode				*newObj;
int					i;
const static OGLVector3D	fillDirection1 = { 1, 0, -.3 };


	gShowScoreMode = SHOW_SCORE_MODE_OFF;
	gBonusTotal = gBonus = 0;
	gPointBeepChannel = -1;

	gRocketScaleAdjust = .625f;

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= 1.0;
	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= 5000;

	if (gLevelNum != LEVEL_NUM_BLOB)
	{
		viewDef.camera.from.x		= -100;
		viewDef.camera.from.z		= 800;
		viewDef.camera.from.y		= 0;
		viewDef.camera.to.y 		= 450.0f;
	}
	else
	{
		viewDef.camera.from.x		= 0;
		viewDef.camera.from.z		= 800;
		viewDef.camera.from.y		= 0;
		viewDef.camera.to.y 		= 0.0f;
	}

	viewDef.lights.ambientColor.r = .2;
	viewDef.lights.ambientColor.g = .2;
	viewDef.lights.ambientColor.b = .15;
	viewDef.lights.fillDirection[0] = fillDirection1;


			/*********************/
			/* SET ANAGLYPH INFO */
			/*********************/

	if (gGamePrefs.anaglyph)
	{
		if (!gGamePrefs.anaglyphColor)
		{
			viewDef.lights.ambientColor.r 		+= .1f;					// make a little brighter
			viewDef.lights.ambientColor.g 		+= .1f;
			viewDef.lights.ambientColor.b 		+= .1f;
		}

		gAnaglyphFocallength	= 80.0f;
		gAnaglyphEyeSeparation 	= 12.0f;
	}


	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/

	InitSparkles();

			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:bonus.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_BONUS, gGameViewInfoPtr);


			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_FARMER, gGameViewInfoPtr);
	LoadASkeleton(SKELETON_TYPE_BEEWOMAN, gGameViewInfoPtr);
	LoadASkeleton(SKELETON_TYPE_SCIENTIST, gGameViewInfoPtr);
	LoadASkeleton(SKELETON_TYPE_SKIRTLADY, gGameViewInfoPtr);


			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:particle.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_PARTICLES, gGameViewInfoPtr);
	BlendAllSpritesInGroup(SPRITE_GROUP_PARTICLES);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:helpfont.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FONT, gGameViewInfoPtr);
	BlendAllSpritesInGroup(SPRITE_GROUP_FONT);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:bonus.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_BONUS, gGameViewInfoPtr);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:spheremap.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS, gGameViewInfoPtr);


				/* LOAD AUDIO */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Bonus.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_BONUS);


	if (gLevelNum != LEVEL_NUM_BLOB)
	{
				/**************/
				/* MAKE STARS */
				/**************/

		for (i = 0; i < 60; i++)
		{
			static OGLColorRGBA colors[] =
			{
				1,1,1,1,			// white
				1,.6,.6,1,			// red
				.6,.6,1,1,			// blue
				.7,.7,8,1,			// grey

			};

			gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
			gNewObjectDefinition.type 		= BONUS_ObjType_Star;
			gNewObjectDefinition.coord.x 	= RandomFloat2() * 800.0f;
			gNewObjectDefinition.coord.y 	= 300.0f + RandomFloat2() * 600.0f;
			gNewObjectDefinition.coord.z 	= -500;
			gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES | STATUS_BIT_GLOW | STATUS_BIT_NOTEXTUREWRAP |
											STATUS_BIT_NOZWRITES | STATUS_BIT_DONTCULL | STATUS_BIT_NOLIGHTING;
			gNewObjectDefinition.slot 		= 50;
			gNewObjectDefinition.moveCall 	= MoveStar;
			gNewObjectDefinition.rot 		= 0;
			gNewObjectDefinition.scale 	    = .1f + RandomFloat() * .2f;
			newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			newObj->SpecialF[0] = RandomFloat() * PI;

			newObj->ColorFilter = colors[MyRandomLong()&0x3];

			newObj->Delta.y = -(5.0f + RandomFloat() * 15.0f);
		}



				/***************/
				/* MAKE ROCKET */
				/***************/

		gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
		gNewObjectDefinition.type 		= BONUS_ObjType_Rocket;
		gNewObjectDefinition.coord.x 	= 0;
		gNewObjectDefinition.coord.y 	= 200;
		gNewObjectDefinition.coord.z 	= -800;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= 500;
		gNewObjectDefinition.moveCall 	= MoveBonusRocket;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= .5;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_BONUS,BONUS_ObjType_Rocket, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);

		MakeRocketFlame(newObj);
	}

			/*****************/
			/* MAKE INTERIOR */
			/*****************/

	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
	gNewObjectDefinition.type 		= BONUS_ObjType_RocketInside;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.coord.y 	= -300;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 3.0;
	gInteriorObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	if (gLevelNum != LEVEL_NUM_BLOB)
		gInteriorObj->ColorFilter.a = 0;

		/* MAKE GLOW DISCS */

	gNewObjectDefinition.type 		= BONUS_ObjType_GlowDiscs;
	gNewObjectDefinition.flags 		= STATUS_BIT_GLOW | STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= MoveGlowDisc;
	gInteriorObj->ChainNode = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	gInteriorObj->ChainNode->ColorFilter.a = gInteriorObj->ColorFilter.a * .99f;

	PlaySong(SONG_BONUS, true);

}



/********************** FREE BONUS SCREEN **********************/

static void FreeBonusScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	DisposeSoundBank(SOUND_BANK_BONUS);
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
}

#pragma mark -


/**************** DO BONUS SHIP DISSOLVE ********************/

static void DoBonusShipDissolve(void)
{
	CalcFramesPerSecond();
	UpdateInput();

	while(gShowScoreMode == SHOW_SCORE_MODE_OFF)
	{
		float fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();
		OGL_DrawScene(gGameViewInfoPtr, DrawBonusCallback);

		OGL_MoveCameraFromTo(gGameViewInfoPtr, fps * 5.0f, 0,0, 0,fps * -80.0f,0);
	}
}



/**************** DO HUMANS BONUS TALLY ********************/

static void DoHumansBonusTally(void)
{
	gScoreAlpha = 0;
	gShowScoreMode = SHOW_SCORE_MODE_HUMANBONUS;

	CreateBonusHumans();

	CalcFramesPerSecond();
	UpdateInput();

	while(true)
	{
		float	fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();
		OGL_DrawScene(gGameViewInfoPtr, DrawBonusCallback);

		if (gGameViewInfoPtr->cameraPlacement.cameraLocation.x < 0.0f)				// keep @ 0 for icon alignment
			OGL_MoveCameraFrom(gGameViewInfoPtr, fps * 5.0f, 0, 0);	// slowly move camera

		gBonusTimer -= fps;
		if (gBonusTimer < 0.0f)
			break;

			/* FADE IN/OUT BONUS & SCORE */

		if (gBonusTimer > 1.0f)
		{
			gScoreAlpha += fps * 2.0f;
			if (gScoreAlpha > .99f)
				gScoreAlpha = .99f;
		}
		else
		{
			gScoreAlpha -= fps * 2.0f;
			if (gScoreAlpha < 0.0f)
				gScoreAlpha = 0;
		}
	}

}


/**************** DO INVENTORY BONUS TALLY ********************/

static void DoInventoryBonusTally(void)
{
	CreateInventoryIcons();

	gBonus = 0;

	gScoreAlpha = 0;
	gInventoryQuantityAlpha = 0;

	gShowScoreMode = SHOW_SCORE_MODE_INVENTORY;

	CalcFramesPerSecond();
	UpdateInput();

	while(true)
	{
		float	fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();
		OGL_DrawScene(gGameViewInfoPtr, DrawBonusCallback);

		gBonusTimer -= fps;
		if (gBonusTimer < 0.0f)
			break;

			/* FADE IN/OUT BONUS & SCORE */

		if (gBonusTimer > 1.0f)
		{
			gScoreAlpha += fps * 2.0f;
			if (gScoreAlpha > .99f)
				gScoreAlpha = .99f;
		}
		else
		{
			gScoreAlpha -= fps * 2.0f;
			if (gScoreAlpha < 0.0f)
				gScoreAlpha = 0;
		}
	}

}


/**************** DO TOTAL BONUS TALLY ********************/

static void DoTotalBonusTally(void)
{
Boolean	fadeIn = true;
float	tick;

	gBonus = gBonusTotal;								// set this so correct value displays at bottom of screen
	gScoreAlpha = 0;
	gInventoryQuantityAlpha = 0;

	gShowScoreMode = SHOW_SCORE_MODE_ADDTOSCORE;

	tick = 1.0f;
	gBonusTimer = 2.0;										// used as delay when done counting bonus

	CalcFramesPerSecond();
	UpdateInput();

	while(true)
	{
		float	fps = gFramesPerSecondFrac;

		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();
		OGL_DrawScene(gGameViewInfoPtr, DrawBonusCallback);



				/* SEE IF DONE */

		if (gBonusTotal == 0)							// if all out then tick down timer
		{
			StopAChannel(&gPointBeepChannel);
			gBonusTimer -= fps;
			if (gBonusTimer < 0.0f)
				break;

			if (gBonusTimer < .6f)						// see if start text fadeout
				fadeIn = false;
		}
		else
		{
					/* SEE IF ADD TO SCORE */

			tick -= fps;
			if (tick <= 0.0f)
			{
				tick = .1;

				if (gBonusTotal >= 951)							// tick up by N unless less than N remaining
				{
					gScore += 951;
					gBonusTotal -= 951;
				}
				else
				{
					gScore += gBonusTotal;
					gBonusTotal = 0;
				}
				gBonus = gBonusTotal;							// keep these the same so that correct thing displays on screen @ bottom

						/* KEEP AUDIO GOING */

				if (gPointBeepChannel == -1)
					gPointBeepChannel = PlayEffect(EFFECT_POINTBEEP);

			}
		}




			/* FADE IN/OUT BONUS & SCORE */

		if (fadeIn)
		{
			gScoreAlpha += fps * 2.0f;
			if (gScoreAlpha > .99f)
				gScoreAlpha = .99f;
		}
		else
		{
			gScoreAlpha -= fps * 2.0f;
			if (gScoreAlpha < 0.0f)
				gScoreAlpha = 0;
		}
	}

}


/*************************** DO SAVE GAME ********************************/

static int DoSaveGamePrompt(void)
{
short	i;

	gSaveMenuSelection = 0;

			/**********************/
			/* BUILD TEXT STRINGS */
			/**********************/

	gNewObjectDefinition.coord.x 	= 0;		// FONTSTRING_GENRE objects are centered
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = SAVE_TEXT_SIZE;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;

	for (i = 0; i < 2; i++)
	{
		const char* cstr = GetLanguageString(i + STR_SAVE_GAME);

		gSaveIcons[i] = MakeFontStringObject(cstr, &gNewObjectDefinition, gGameViewInfoPtr, true);
		gSaveIcons[i]->ColorFilter.a = 0;
		gNewObjectDefinition.coord.y 	+= SAVE_TEXT_SIZE * .7f;
	}

				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	UpdateInput();

	int saveMenuOutcome = SAVE_MENU_SELECTION_NONE;

	while(true)
	{
		DoSoundMaintenance();

			/* SEE IF MAKE SELECTION */

		saveMenuOutcome = NavigateSaveMenu();
		if (saveMenuOutcome != SAVE_MENU_SELECTION_NONE)
			break;

			/* DRAW STUFF */

		CalcFramesPerSecond();
		UpdateInput();
		OGL_DrawScene(gGameViewInfoPtr, DrawObjects);


			/* FADE IN TEXT */

		for (i = 0; i < 2; i++)
		{
			gSaveIcons[i]->ColorFilter.a += gFramesPerSecondFrac * 3.0f;
			if (gSaveIcons[i]->ColorFilter.a > 1.0f)
				gSaveIcons[i]->ColorFilter.a = 1.0;
		}


	}


			/* CLEANUP */

	for (i = 0; i < 2; i++)
		DeleteObject(gSaveIcons[i]);


	return saveMenuOutcome;
}


/*********************** NAVIGATE SAVE MENU **************************/

static int NavigateSaveMenu(void)
{
static const OGLColorRGBA noHiliteColor = {.3,.5,.2,1};

		/* SEE IF CHANGE SELECTION */

	if (GetNewKeyState(SDL_SCANCODE_UP) && (gSaveMenuSelection > 0))
	{
		gSaveMenuSelection--;
	}
	else
	if (GetNewKeyState(SDL_SCANCODE_DOWN) && (gSaveMenuSelection < 1))
	{
		gSaveMenuSelection++;
	}

		/* SET APPROPRIATE FRAMES FOR ICONS */

	for (int i = 0; i < 2; i++)
	{
		if (i == gSaveMenuSelection)
		{
			gSaveIcons[i]->ColorFilter.r = 										// normal
			gSaveIcons[i]->ColorFilter.g =
			gSaveIcons[i]->ColorFilter.b =
			gSaveIcons[i]->ColorFilter.a = .7f + RandomFloat() * .29f;
		}
		else
		{
			gSaveIcons[i]->ColorFilter = noHiliteColor;							// hilite
		}

	}


			/***************************/
			/* SEE IF MAKE A SELECTION */
			/***************************/

	if (GetNewKeyState(SDL_SCANCODE_RETURN) || GetNewKeyState(SDL_SCANCODE_SPACE))
	{
		switch(gSaveMenuSelection)
		{
			case	0:								// SAVE GAME
					return SAVE_MENU_SELECTION_SAVE;

			case	1:								// CONTINUE
					return SAVE_MENU_SELECTION_CONTINUE;
		}
	}



	return SAVE_MENU_SELECTION_NONE;
}


#pragma mark -

/**************** DO TRACTOR BEAM ********************/

static void DoTractorBeam(void)
{
	GammaFadeOut();

	InitBonusTractorBeam();


	CalcFramesPerSecond();
	UpdateInput();

	MakeFadeEvent(true, 1.0);

	while(gBonusBeamTimer < 12.0f)
	{

		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();
		OGL_DrawScene(gGameViewInfoPtr, DrawObjects);
	}

}


/***************** INIT BONUS TRACTOR BEAM **************************/

static void InitBonusTractorBeam(void)
{
const OGLPoint3D from = {0,-150,500};
const OGLPoint3D to = {0,0,-800};
ObjNode	*newObj;

	DeleteObject(gInteriorObj);										// delete the interior

	gBonusBeamTimer = 0;


			/* MAKE ROCKET */

	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
	gNewObjectDefinition.type 		= BONUS_ObjType_Rocket;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= -300;
	gNewObjectDefinition.coord.z 	= -800;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 500;
	gNewObjectDefinition.moveCall 	= MoveBonusRocket_TractorBeam;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_BONUS,BONUS_ObjType_Rocket, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);

	MakeRocketFlame(newObj);


			/* MAKE TRACTOR BEAM */

	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
	gNewObjectDefinition.type 		= BONUS_ObjType_TractorBeam;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.z 	= -800;
	gNewObjectDefinition.coord.y	= -1000;
	gNewObjectDefinition.flags 		= STATUS_BIT_GLOW | STATUS_BIT_NOLIGHTING | STATUS_BIT_NOZWRITES | STATUS_BIT_KEEPBACKFACES | STATUS_BIT_HIDDEN;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= MoveBonusTractorBeam;
	gNewObjectDefinition.rot 		= PI;
	gNewObjectDefinition.scale 		= 9.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* SET CAMERA */

	OGL_UpdateCameraFromTo(gGameViewInfoPtr, &from, &to);
}

/***************** MOVE BONUS ROCKET: TRACTOR BEAM ********************/

static void MoveBonusRocket_TractorBeam(ObjNode *rocket)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*flame;
u_long	vol;

	GetObjectInfo(rocket);

	rocket->Rot.y -= fps * .5f;

	if (gBonusBeamTimer > 2.0f)							// if beam on, then shake ship and drag down
	{
		rocket->SpecialF[0] += fps * 15.0f;
		gCoord.x = rocket->InitCoord.x + sin(rocket->SpecialF[0]) * 12.0f;

					/* DRAG DOWN */

		gDelta.y -= 50.0f * fps;
		if (gDelta.y < -300.0f)
			gDelta.y = -300.0f;
		gCoord.y += gDelta.y * fps;
	}

	UpdateObject(rocket);


		/* UPDATE FLAME */

	flame = rocket->ChainNode;
	if (flame)
	{
		flame->Coord = gCoord;
		UpdateObjectTransforms(flame);
	}

			/* UPDATE AUDIO */

	vol = gGammaFadePercent * FULL_CHANNEL_VOLUME / 2;
	if (rocket->EffectChannel == -1)
		rocket->EffectChannel = PlayEffect_Parms(EFFECT_BONUSROCKET, vol, vol, NORMAL_CHANNEL_RATE);
	else
		ChangeChannelVolume(rocket->EffectChannel, vol, vol);

}


/****************** MOVE BONUS TRACTOR BEAM ***************************/

static void MoveBonusTractorBeam(ObjNode *beam)
{
float	fps = gFramesPerSecondFrac;
float	oldTime = gBonusBeamTimer;

			/* SEE IF TURN ON BEAM */

	gBonusBeamTimer += fps;
	if ((oldTime < 2.0f) && (gBonusBeamTimer >= 2.0f))
	{
		beam->StatusBits &= ~STATUS_BIT_HIDDEN;
		beam->EffectChannel = PlayEffect_Parms(EFFECT_BONUSTRACTORBEAM, FULL_CHANNEL_VOLUME * 2, FULL_CHANNEL_VOLUME * 2, NORMAL_CHANNEL_RATE * 3/2);
	}

			/* SEE IF FADE OUT */

	else
	if ((oldTime < 10.0f) && (gBonusBeamTimer >= 10.0f))
		MakeFadeEvent(false, 1.0);


	GetObjectInfo(beam);

			/* UPDATE BEAM */

	beam->ColorFilter.a = .99f - RandomFloat()*.1f;					// flicker
	beam->Scale.x = beam->Scale.z = 9.0f - RandomFloat() * .4f;

	TurnObjectTowardTarget(beam, &gCoord, gGameViewInfoPtr->cameraPlacement.cameraLocation.x, gGameViewInfoPtr->cameraPlacement.cameraLocation.z, 100, false);	// aim at camera
	UpdateObjectTransforms(beam);

	MO_Object_OffsetUVs(beam->BaseGroup, 0, fps * 2.0f);
}



#pragma mark -


/***************** DRAW BONUS CALLBACK *******************/

static void DrawBonusCallback(OGLSetupOutputType *info)
{
	DrawObjects(info);
	DrawSparkles(info);											// draw light sparkles

	switch(gShowScoreMode)
	{
				/* DRAW HUMAN BONUS */

		case	SHOW_SCORE_MODE_HUMANBONUS:
				DrawHumanBonus(info);
				break;

				/* DRAW INVENTORY BONUS */

		case	SHOW_SCORE_MODE_INVENTORY:
				DrawInventoryBonus(info);
				break;

				/* DRAW TOTALS */

		case	SHOW_SCORE_MODE_ADDTOSCORE:
				DrawBonusToScore(info);
				break;

	}
}


/************************ DRAW HUMAN BONUS *************************/

static void DrawHumanBonus(OGLSetupOutputType *info)
{

		/************/
		/* SET TAGS */
		/************/

	OGL_PushState();

	SetInfobarSpriteState(true);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);						// make glow


			/**********************/
			/* DRAW BONUS VERBAGE */
			/**********************/

			/* DRAW GLOW */

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	gGlobalTransparency = (.7f + RandomFloat()*.1f) * gScoreAlpha;
	DrawInfobarSprite2(-150, 95, 300, SPRITE_GROUP_BONUS, BONUS_SObjType_BonusGlow, info);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			/* DRAW TEXT */

	gGlobalTransparency = gScoreAlpha;
	DrawInfobarSprite2(-150, 95, 300, SPRITE_GROUP_BONUS, BONUS_SObjType_BonusText, info);


			/************************/
			/* DRAW THE BONUS VALUE */
			/************************/

	DrawBonus(info);



			/***********/
			/* CLEANUP */
			/***********/

	OGL_PopState();
	gGlobalMaterialFlags = 0;
	gGlobalTransparency = 1.0;

}



/************************ DRAW INVENTORY BONUS *************************/

static void DrawInventoryBonus(OGLSetupOutputType *info)
{
		/************/
		/* SET TAGS */
		/************/

	OGL_PushState();

	SetInfobarSpriteState(true);


	glBlendFunc(GL_SRC_ALPHA, GL_ONE);						// make glow


			/**********************/
			/* DRAW BONUS VERBAGE */
			/**********************/

			/* DRAW GLOW */

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	gGlobalTransparency = (.7f + RandomFloat()*.1f) * gScoreAlpha;
	DrawInfobarSprite2(-150, 95, 300, SPRITE_GROUP_BONUS, BONUS_SObjType_InventoryGlow, info);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			/* DRAW TEXT */

	gGlobalTransparency = gScoreAlpha;
	DrawInfobarSprite2(-150, 95, 300, SPRITE_GROUP_BONUS, BONUS_SObjType_InventoryText, info);


			/************************/
			/* DRAW THE BONUS VALUE */
			/************************/

	DrawBonus(info);



			/************************/
			/* DRAW WEAPON QUANTITY */
			/************************/

	if (gInventoryQuantityAlpha != 0.0f)
	{
		DrawInventoryQuantity(info);
	}


			/***********/
			/* CLEANUP */
			/***********/

	OGL_PopState();
	gGlobalMaterialFlags = 0;
	gGlobalTransparency = 1.0;

}



/********************* DRAW INVENTORY QUANTITY ****************************/

static void DrawInventoryQuantity(OGLSetupOutputType *info)
{
Str32	s;
int		texNum,n,i;
float	x;

	gGlobalTransparency = gInventoryQuantityAlpha;

	NumToString(gInventoryQuantity, s);
	n = s[0];										// get str len

	x = - ((float)n / 2.0f) * DIGIT_SPACING_Q - (DIGIT_SPACING_Q/2);	// calc starting x

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	for (i = 1; i <= n; i++)
	{
		texNum = CharToSprite(s[i]);				// get texture #

		gGlobalTransparency = (.9f + RandomFloat()*.099f) * gScoreAlpha;
		DrawInfobarSprite2(x, -10, DIGIT_SPACING_Q * 1.9f, SPRITE_GROUP_FONT, texNum, info);
		x += DIGIT_SPACING_Q;
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gGlobalTransparency = 1.0f;
}


/************************ DRAW BONUS TO SCORE *************************/

static void DrawBonusToScore(OGLSetupOutputType *info)
{
		/************/
		/* SET TAGS */
		/************/

	OGL_PushState();

	SetInfobarSpriteState(true);


	glBlendFunc(GL_SRC_ALPHA, GL_ONE);						// make glow


			/****************************/
			/* DRAW BONUS TOTAL VERBAGE */
			/****************************/

			/* DRAW GLOW */

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	gGlobalTransparency = (.7f + RandomFloat()*.1f) * gScoreAlpha;
	DrawInfobarSprite2(-150, 100, 300, SPRITE_GROUP_BONUS, BONUS_SObjType_TotalBonusGlow, info);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			/* DRAW TEXT */

	gGlobalTransparency = gScoreAlpha;
	DrawInfobarSprite2(-150, 100, 300, SPRITE_GROUP_BONUS, BONUS_SObjType_TotalBonusText, info);


			/************************/
			/* DRAW THE BONUS VALUE */
			/************************/

	DrawBonus(info);



			/**************/
			/* DRAW SCORE */
			/**************/

			/* DRAW GLOW */

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	gGlobalTransparency = (.7f + RandomFloat()*.1f) * gScoreAlpha;
	DrawInfobarSprite2(-150, -30, 300, SPRITE_GROUP_BONUS, BONUS_SObjType_ScoreGlow, info);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			/* DRAW TEXT */

	gGlobalTransparency = gScoreAlpha;
	DrawInfobarSprite2(-150, -30, 300, SPRITE_GROUP_BONUS, BONUS_SObjType_ScoreText, info);

			/* DRAW VALUE */

	DrawScore(info);



			/***********/
			/* CLEANUP */
			/***********/

	OGL_PopState();
	gGlobalMaterialFlags = 0;
	gGlobalTransparency = 1.0;

}




/********************* DRAW BONUS ****************************/

static void DrawBonus(OGLSetupOutputType *info)
{
Str32	s;
int		texNum,n,i;
float	x;

	NumToString(gBonus, s);
	n = s[0];										// get str len

	x = - ((float)n / 2.0f) * DIGIT_SPACING - (DIGIT_SPACING/2);	// calc starting x

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	for (i = 1; i <= n; i++)
	{
		texNum = CharToSprite(s[i]);				// get texture #

		gGlobalTransparency = (.9f + RandomFloat()*.099f) * gScoreAlpha;
		DrawInfobarSprite2(x, 150, DIGIT_SPACING * 1.9f, SPRITE_GROUP_FONT, texNum, info);
		x += DIGIT_SPACING;
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


/********************* DRAW SCORE ****************************/

static void DrawScore(OGLSetupOutputType *info)
{
Str32	s;
int		texNum,n,i;
float	x;

	NumToString(gScore, s);
	n = s[0];										// get str len

	x = - ((float)n / 2.0f) * DIGIT_SPACING - (DIGIT_SPACING/2);	// calc starting x

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	for (i = 1; i <= n; i++)
	{
		texNum = CharToSprite(s[i]);				// get texture #

		gGlobalTransparency = (.9f + RandomFloat()*.099f) * gScoreAlpha;
		DrawInfobarSprite2(x, 20, DIGIT_SPACING * 1.9f, SPRITE_GROUP_FONT, texNum, info);
		x += DIGIT_SPACING;
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}



#pragma mark -

/************************ CREATE INVENTORY ICONS *************************/

static void CreateInventoryIcons(void)
{
int		i;
short	type,quantity;
float	d;
ObjNode	*newObj;


	d = 1.0;									// MUST start with a non-zero delay or first one will be skipped

	for (i = 0; i < MAX_INVENTORY_SLOTS; i++)
	{
		type = gPlayerInfo.weaponInventory[i].type;					// get weapon type

//		if (i ==0) type = WEAPON_TYPE_STUNPULSE;	//---------

		if (type == NO_INVENTORY_HERE)								// skip if blank
			continue;
		if (type == WEAPON_TYPE_FIST)								// also skip fist
			continue;

		quantity = gPlayerInfo.weaponInventory[i].quantity;			// get quantity

//		if (i == 0)	quantity = 10;		//--------

		if (quantity == 0)											// skip if 0 (this shouldn't really ever happen)
			continue;


			/* MAKE OBJECT */

		gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
		gNewObjectDefinition.type 		= gWeaponToIconTable[type];
		gNewObjectDefinition.coord.x 	= 0;
		gNewObjectDefinition.coord.y 	= 100 + gWeaponIconYOff[type];
		gNewObjectDefinition.coord.z 	= 300;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= 300;
		gNewObjectDefinition.moveCall 	= MoveBonusInventoryIcon;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= 2.7f * gWeaponIconScale[type];
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->ColorFilter.a 	= 0;
		newObj->InvisibleDelay 	= d;
		newObj->Quantity		= quantity;
		d += (quantity * INVENTORY_COUNT_DELAY) + 1.0f;
	}

	gBonusTimer = d + 2.0f;
}



/****************** MOVE BONUS INVENTORY ICON **********************/

static void MoveBonusInventoryIcon(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* SEE IF SHOW NOW */

	if (theNode->InvisibleDelay > 0.0f)
	{
		theNode->InvisibleDelay -= fps;
		if (theNode->InvisibleDelay <= 0.0f)
			theNode->InvisibleDelay = 0;
	}

			/**************/
			/* SHOWING IT */
			/**************/
	else
	{

			/* FADE IN */

		if ((theNode->ColorFilter.a != 1.0f) && (theNode->Quantity > 0))
		{
			theNode->ColorFilter.a += fps * 2.0;
			if (theNode->ColorFilter.a > 1.0f)					// once opaque then start the tally
			{
				theNode->ColorFilter.a  = 1.0f;
				gInventoryCountTimer = INVENTORY_COUNT_DELAY;
			}
		}

			/* FADE OUT */

		if (theNode->Quantity == 0)
		{
			StopAChannel(&gPointBeepChannel);

			theNode->ColorFilter.a -= fps * 2.0;				// fade out and delete when gone
			if (theNode->ColorFilter.a <= 0.0f)
			{
				DeleteObject(theNode);
				return;
			}
		}


				/* SPIN THE ICON */

		theNode->Rot.y +=  fps * PI2;
		UpdateObjectTransforms(theNode);


				/*************************/
				/* UPDATE BONUS COUNTING */
				/*************************/

		if (theNode->ColorFilter.a == 1.0f)							// only count when done fading in
		{
					/* SEE IF TIME TO COUNT THIS */

			gInventoryCountTimer -= fps;							// check timer
			if (gInventoryCountTimer <= 0.0f)
			{
				gInventoryCountTimer = INVENTORY_COUNT_DELAY;		// reset timer
				theNode->Quantity--;								// dec quanitity
				gBonus += POINTS_INVENTORY;


						/* KEEP AUDIO GOING */

				if (gPointBeepChannel == -1)
					gPointBeepChannel = PlayEffect(EFFECT_POINTBEEP);
			}
		}

		gInventoryQuantityAlpha = theNode->ColorFilter.a;
		gInventoryQuantity = theNode->Quantity;
	}

}


#pragma mark -


/********************** CREATE BONUS HUMANS **************************/

static void CreateBonusHumans(void)
{
int		i,t,j;
ObjNode	*newObj;
float	d;
const short	humanSkeletons[NUM_HUMAN_TYPES] =
{
	SKELETON_TYPE_FARMER,
	SKELETON_TYPE_BEEWOMAN,
	SKELETON_TYPE_SCIENTIST,
	SKELETON_TYPE_SKIRTLADY
};

	gBonusTimer = gNumHumansRescuedTotal * 1.4f + 3.0f;

	d = 1.0;														// MUST start with a non-zero delay or first one will be skipped
	j = 0;

	for (t = 0; t < NUM_HUMAN_TYPES; t++)							// scan each type of human
	{
		for (i = 0; i < gNumHumansRescuedOfType[t]; i++)			// do for each of this human type
		{
				/***********************/
				/* MAKE HUMAN SKELETON */
				/***********************/

			gNewObjectDefinition.type 		= humanSkeletons[t];
			gNewObjectDefinition.animNum	= 0;
			if (j++ & 1)											// alternate left/right
				gNewObjectDefinition.coord.x = 200.0f;
			else
				gNewObjectDefinition.coord.x = -200.0f;
			gNewObjectDefinition.coord.y 	= -90;
			gNewObjectDefinition.coord.z 	= -50;
			gNewObjectDefinition.flags 		= 0;
			gNewObjectDefinition.slot 		= 100;
			gNewObjectDefinition.moveCall	= MoveBonusHuman;
			gNewObjectDefinition.rot 		= PI;
			gNewObjectDefinition.scale 		= 2.9;

			newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

			newObj->ColorFilter.a = 0;
			newObj->InvisibleDelay = d;

			d += 1.25f;
		}
	}
}


/****************** MOVE BONUS HUMAN **********************/

static void MoveBonusHuman(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* SEE IF FADE IN NOW */

	if (theNode->InvisibleDelay > 0.0f)
	{
		theNode->InvisibleDelay -= fps;
		if (theNode->InvisibleDelay <= 0.0f)
		{
				/* START TELEPORT */

			StartBonusTeleport(theNode);
			gBonus += POINTS_HUMAN;
		}
		else
			return;
	}

	if (theNode->ChainNode)				// if has rings...
		UpdateBonusTeleport(theNode);

}


/****************** MOVE BONUS HUMAN : WALK OFF **********************/

static void MoveBonusHuman_WalkOff(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* SEE IF WALKED OUT OF VIEW */

	if (theNode->StatusBits & STATUS_BIT_ISCULLED)
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);

		/* ACCEL FORWARD */

	gDelta.x += -sin(theNode->Rot.y) * 300.0f * fps;
	gDelta.z += -cos(theNode->Rot.y) * 300.0f * fps;
	theNode->Speed2D = CalcVectorLength(&gDelta);
	if (theNode->Speed2D > 600.0f)
	{
		float	q = 600.0f / theNode->Speed2D;
		gDelta.x *= q;
		gDelta.z *= q;
		theNode->Speed2D = 600.0f;
	}

			/* SET ANIM SPEED */

	theNode->Skeleton->AnimSpeed = theNode->Speed2D * .009f;

			/* MOVE IT */

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;

	UpdateObject(theNode);
}


#pragma mark -

/********************* START BONUS TELEPORT ***********************/

static void StartBonusTeleport(ObjNode *human)
{
ObjNode	*ring1,*ring2;
int		i;

		/***********************/
		/* MAKE TELEPORT RINGS */
		/***********************/

			/* RING 1 */

	gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
	gNewObjectDefinition.type 		= BONUS_ObjType_TeleportRing;
	gNewObjectDefinition.coord.x 	= human->Coord.x;
	gNewObjectDefinition.coord.z 	= human->Coord.z;
	gNewObjectDefinition.coord.y 	= human->Coord.y;
	gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES|STATUS_BIT_GLOW|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOZWRITES;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+20;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .6;
	ring1 = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	ring1->RingOffset = 0;
	ring1->ColorFilter.a = .99;
	ring1->Rot.x = RandomFloat2() * .3f;
	ring1->Rot.z = RandomFloat2() * .3f;
	human->ChainNode = ring1;


			/* RING 2 */

	ring2 = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	ring2->RingOffset = 0;
	ring2->ColorFilter.a = .99;
	ring2->Rot.x = RandomFloat2() * .3f;
	ring2->Rot.z = RandomFloat2() * .3f;
	ring1->ChainNode = ring2;


			/* MAKE SPARKLE */

	i = ring1->Sparkles[0] = GetFreeSparkle(ring1);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
		gSparkles[i].where = human->Coord;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 250.0f;
		gSparkles[i].separation = 100.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_BlueSpark;
	}


	PlayEffect(EFFECT_BONUSTELEPORT);
}


/******************** UPDATE BONUS TELEPORT *********************/

static void UpdateBonusTeleport(ObjNode *human)
{
float	fps = gFramesPerSecondFrac;
ObjNode *ring1,*ring2;
int		i;

			/* GET RINGS */

	ring1 = human->ChainNode;
	ring2 = ring1->ChainNode;


			/* MOVE RINGS */

	ring1->RingOffset += fps * 100.0f;
	ring1->Coord.x = human->Coord.x;
	ring1->Coord.y = human->Coord.y + ring1->RingOffset;
	ring1->Coord.z = human->Coord.z;
	ring1->Scale.x = ring1->Scale.y = ring1->Scale.z += fps * .3f;
	ring1->Rot.y += fps * PI2;

	ring2->RingOffset += fps * 100.0f;
	ring2->Coord.x = human->Coord.x;
	ring2->Coord.y = human->Coord.y - ring2->RingOffset;
	ring2->Coord.z = human->Coord.z;
	ring2->Scale.x = ring2->Scale.y = ring2->Scale.z -= fps * .3f;
	ring2->Rot.y -= fps * PI2;

	UpdateObjectTransforms(ring1);
	UpdateObjectTransforms(ring2);

			/* FADE IN HUMAN */

	human->ColorFilter.a += fps * .7f;
	if (human->ColorFilter.a >= 1.0f)			// see if done teleporting
	{
		human->ColorFilter.a = 1.0f;
		DeleteObject(ring1);					// delete rings & sparkle
		human->ChainNode = nil;
		human->MoveCall = MoveBonusHuman_WalkOff;
		MorphToSkeletonAnim(human->Skeleton, 1, 4);
		return;
	}

	ring1->ColorFilter.a = 1.0f - human->ColorFilter.a;
	ring2->ColorFilter.a = 1.0f - human->ColorFilter.a;

	if (human->ShadowNode)
		human->ShadowNode->ColorFilter.a = human->ColorFilter.a;


		/* UPDATE SPARKLE */

	i = ring1->Sparkles[0];
	if (i != -1)
	{
		gSparkles[i].color.a -= fps * 1.0f;
	}
}

#pragma mark -

/************************ MOVE STAR *************************/

static void MoveStar(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->SpecialF[0] += fps * 2.0f;

	theNode->ColorFilter.a = .3f + (sin(theNode->SpecialF[0]) + 1.0f) * .5f;

	theNode->Coord.y += theNode->Delta.y * fps;
	if (theNode->Coord.y > 850.0f)
		theNode->Coord.y = -850.0f;

	UpdateObjectTransforms(theNode);
}


/******************* MOVE BONUS ROCKET *****************************/

static void MoveBonusRocket(ObjNode *rocket)
{
float	fps = gFramesPerSecondFrac;
float	a;
ObjNode	*flame;
u_long	vol;

	GetObjectInfo(rocket);

	gCoord.y -= 100.0f * fps;
	gCoord.z += 260.0f * fps;

	rocket->Rot.y += fps * .5f;

	if (gCoord.z > 400.0f)													// fade when close
	{
		a = rocket->ColorFilter.a -= fps * 1.3f;							// fade out the ship
		if (rocket->ColorFilter.a <= 0.0f)
		{
			gInteriorObj->ColorFilter.a = 1.0f;								// make sure interior is full alpha
			DeleteObject(rocket);
			gShowScoreMode = SHOW_SCORE_MODE_HUMANBONUS;
			return;
		}
		else
			gInteriorObj->ColorFilter.a = 1.0f - rocket->ColorFilter.a;		// fade in the interior as ship fades out
	}


			/**********/
			/* UPDATE */
			/**********/

	UpdateObject(rocket);

		/* UPDATE FLAME */

	flame = rocket->ChainNode;
	if (flame)
	{
		flame->ColorFilter.a = rocket->ColorFilter.a;
		flame->Coord = gCoord;
		UpdateObjectTransforms(flame);
	}

	if (rocket->Sparkles[0] != -1)											// fade sparkle
	{
		gSparkles[rocket->Sparkles[0]].color.a = rocket->ColorFilter.a * .6f;
	}

			/* UPDATE AUDIO */

	vol = gGammaFadePercent * FULL_CHANNEL_VOLUME / 2;
	if (rocket->EffectChannel == -1)
		rocket->EffectChannel = PlayEffect_Parms(EFFECT_BONUSROCKET, vol, vol, NORMAL_CHANNEL_RATE);
	else
		ChangeChannelVolume(rocket->EffectChannel, vol, vol);
}



/******************* MOVE GLOW DISC **************************/

static void MoveGlowDisc(ObjNode *theNode)
{
	theNode->ColorFilter.a = (.5f + RandomFloat() * .3f) * gInteriorObj->ColorFilter.a;


}











