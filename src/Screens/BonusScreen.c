/****************************/
/*   	BONUS SCREEN.C		*/
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

static void SetupBonusScreen(void);
static void FreeBonusScreen(void);
static void DrawBonusCallback(void);
static void DrawHumanBonus(void);
static void DrawInventoryBonus(void);
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
static void DrawBonusToScore(void);
static int DoSaveGamePrompt(void);
static void DoTractorBeam(void);
static void InitBonusTractorBeam(void);
static void MoveBonusTractorBeam(ObjNode *beam);
static void MoveBonusRocket_TractorBeam(ObjNode *rocket);
static void MoveGlowDisc(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	POINTS_HUMAN		4000
#define	POINTS_INVENTORY	150

#define	HUMAN_SPACING	160.0f

#define STARS_LOW_Y		(-1000.0f)
#define STARS_HIGH_Y	(2000.0f)

static const RectF gInteriorStarZones[] =
{
	{.top=150, .bottom=-50, .left=-650, .right=-480},
	{.top=800, .bottom=600, .left=-650, .right=-480},

	{.top=150, .bottom=-50, .left=-200, .right=-20},
	{.top=700, .bottom=500, .left=-200, .right=-20},

	{.top=150, .bottom=-50, .left=220, .right=400},
	{.top=700, .bottom=500, .left=220, .right=400},

	{.top=250, .bottom=-50, .left=-1250, .right=-950},
	{.top=250, .bottom=-50, .left=700, .right=1550},
};

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


static float	gBonusTimer;
static float	gBonusFastForward = 1.0f;

#define	HumanNumberInSequence	Special[0]
#define	InvisibleDelay	SpecialF[0]
#define	RingOffset		SpecialF[0]

#define	Quantity		Special[0]

static ObjNode *gInteriorObj;

static	Byte	gShowScoreMode;

static	int			gNumHumansShownSoFar = 0;
static	uint32_t	gBonusTotal;
static	uint32_t	gBonus;
static	uint32_t	gInventoryQuantity;

static	float	gScoreAlpha,gInventoryQuantityAlpha;

static	short	gPointBeepChannel;

static	float	gBonusBeamTimer;


/********************** DO BONUS SCREEN **************************/

void DoBonusScreen(void)
{
			/* SETUP */

	gBonusFastForward = 1;
	SetupBonusScreen();
	MakeFadeEvent(true, 1.0);


		/* DO SHIP DISSOLVE */

	if (gLevelNum != LEVEL_NUM_BLOB)
		DoBonusShipDissolve();


			/* DO HUMANS BONUS */

	DoHumansBonusTally();
	gBonusTotal += gBonus;

			/* DO INVENTORY BONUS */

	if (!gPlayerInfo.didCheat)
	{
		DoInventoryBonusTally();
		gBonusTotal += gBonus;
	}

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

			/* FADE OUT */

	OGL_FadeOutScene(DrawBonusCallback, NULL);

		/* DO TRACTOR BEAM FOR JUNGLE-BOSS */

	if (gLevelNum == LEVEL_NUM_JUNGLE)
	{
		gShowScoreMode = SHOW_SCORE_MODE_OFF;		// reset stars

		DoTractorBeam();
		// Tractor beam does its own async fadeout
	}



			/* CLEANUP */

	FreeBonusScreen();
	gShowScoreMode = SHOW_SCORE_MODE_OFF;
}



/********************* SETUP BONUS SCREEN **********************/

static void SetupBonusScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
ObjNode				*newObj;
static const OGLVector3D	fillDirection1 = { 1, 0, -.3 };

static const OGLColorRGBA	starColors[] =
{
	{1.0f, 1.0f, 1.0f, 1.0f},			// white
	{1.0f, 0.6f, 0.6f, 1.0f},			// red
	{0.6f, 0.6f, 1.0f, 1.0f},			// blue
	{0.7f, 0.7f, 1.0f, 1.0f},			// grey
};


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

	if (gGamePrefs.anaglyphMode != ANAGLYPH_OFF)
	{
		if (gGamePrefs.anaglyphMode == ANAGLYPH_MONO)
		{
			viewDef.lights.ambientColor.r 		+= .1f;					// make a little brighter
			viewDef.lights.ambientColor.g 		+= .1f;
			viewDef.lights.ambientColor.b 		+= .1f;
		}

		gAnaglyphFocallength	= 80.0f;
		gAnaglyphEyeSeparation 	= 12.0f;
	}


	OGL_SetupWindow(&viewDef);


				/************/
				/* LOAD ART */
				/************/

	InitEffects();

			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:bonus.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_BONUS);


			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_FARMER);
	LoadASkeleton(SKELETON_TYPE_BEEWOMAN);
	LoadASkeleton(SKELETON_TYPE_SCIENTIST);
	LoadASkeleton(SKELETON_TYPE_SKIRTLADY);


			/* LOAD SPRITES */

//	LoadSpriteGroup(SPRITE_GROUP_PARTICLES, PARTICLE_SObjType_COUNT, "particle");
	LoadSpriteGroup(SPRITE_GROUP_BONUS, BONUS_SObjType_COUNT, "bonus");
	LoadSpriteGroup(SPRITE_GROUP_SPHEREMAPS, SPHEREMAP_SObjType_COUNT, "spheremap");

	BlendAllSpritesInGroup(SPRITE_GROUP_PARTICLES);


				/* LOAD AUDIO */

	LoadSoundBank(SOUNDBANK_BONUS);


	if (gLevelNum != LEVEL_NUM_BLOB)
	{
				/**************/
				/* MAKE STARS */
				/**************/

		for (int i = 0; i < 60*5; i++)
		{
			gNewObjectDefinition.group 		= MODEL_GROUP_BONUS;
			gNewObjectDefinition.type 		= BONUS_ObjType_Star;
			gNewObjectDefinition.coord.x 	= RandomFloatRange(-1600.0f, 2000.0f);
			gNewObjectDefinition.coord.y 	= RandomFloatRange(STARS_LOW_Y, STARS_HIGH_Y);
			gNewObjectDefinition.coord.z 	= -500;
			gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES | STATUS_BIT_GLOW | STATUS_BIT_NOTEXTUREWRAP |
											STATUS_BIT_NOZWRITES | STATUS_BIT_DONTCULL | STATUS_BIT_NOLIGHTING;
			gNewObjectDefinition.slot 		= 50;
			gNewObjectDefinition.moveCall 	= MoveStar;
			gNewObjectDefinition.rot 		= 0;
			gNewObjectDefinition.scale 	    = .1f + RandomFloat() * .2f;
			newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			newObj->Timer = RandomFloat() * PI;
			newObj->Mode = 0;
			newObj->SpecialF[0] = STARS_LOW_Y;
			newObj->SpecialF[1] = STARS_HIGH_Y;
			newObj->SpecialF[2] = 1;

			newObj->ColorFilter = starColors[MyRandomLong()&0x3];

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

		/* MAKE DENSER STARS VIEWABLE FROM INSIDE WINDOWS */

	int numInteriorStarZones = (int) (sizeof(gInteriorStarZones) / sizeof(gInteriorStarZones[0]));

	for (int starRect = 0; starRect < numInteriorStarZones; starRect++)
	{
		RectF r = gInteriorStarZones[starRect];
		float width = r.right - r.left;
		float height = r.top - r.bottom;
		float area = width * height;

		for (int i = 0; i <= (int)(area/4000); i++)
		{
			NewObjectDefinitionType def =
			{
				.group		= MODEL_GROUP_BONUS,
				.type		= BONUS_ObjType_Star,
				.coord		= {RandomFloatRange(r.left, r.right), RandomFloatRange(r.bottom, r.top), -500},
				.flags		= STATUS_BIT_KEEPBACKFACES | STATUS_BIT_GLOW | STATUS_BIT_NOTEXTUREWRAP |
								STATUS_BIT_NOZWRITES | STATUS_BIT_DONTCULL | STATUS_BIT_NOLIGHTING,
				.slot		= 50,
				.moveCall	= MoveStar,
				.rot		= 0,
				.scale		= .15f + RandomFloat() * .2f,
			};
			newObj = MakeNewDisplayGroupObject(&def);

			newObj->Timer = RandomFloat() * PI;
			newObj->Mode = 1;
			newObj->SpecialF[0] = r.bottom;
			newObj->SpecialF[1] = r.top;
			newObj->SpecialF[2] = 0;
			newObj->ColorFilter = starColors[MyRandomLong()&0x3];
			newObj->Delta.y = -(5.0f + RandomFloat() * 15.0f);
		}
	}

	PlaySong(SONG_BONUS, 0);

}



/********************** FREE BONUS SCREEN **********************/

static void FreeBonusScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeEffects();
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	DisposeSoundBank(SOUNDBANK_BONUS);
	OGL_DisposeWindowSetup();
	Pomme_FlushPtrTracking(true);
}



/**************** CHECK FAST FORWARD KEY ********************/

static void CheckFastForward(void)
{
	if (GetNeedState(kNeed_Jump))
		gBonusFastForward = 3.0f;
	else
		gBonusFastForward = 1.0f;
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
		OGL_DrawScene(DrawBonusCallback);

		OGL_MoveCameraFromTo(fps * 5.0f, 0,0, 0,fps * -80.0f,0);
	}
}


#pragma mark -

/**************** MOVE TALLY TEXT ********************/

static void MoveTallyText(ObjNode* theNode)
{
	theNode->ColorFilter.a = (.9f + RandomFloat()*.099f) * gScoreAlpha;

	int targetTally;
	switch (theNode->Special[0])
	{
		case 0:
			targetTally = gBonus;
			break;
		case 1:
			targetTally = gScore;
			break;
		case 2:
			targetTally = gInventoryQuantity;
			theNode->ColorFilter.a *= gInventoryQuantityAlpha;
			break;
		case 3:
			targetTally = gNumHumansInLevel - gNumHumansShownSoFar;
			if (gNumHumansShownSoFar < gNumHumansRescuedTotal)
				theNode->ColorFilter.a = 0;
			else
			{
				theNode->Timer += gFramesPerSecondFrac * 2.0f;
				theNode->Timer = ClampFloat(theNode->Timer, 0, 1);
				theNode->ColorFilter.a *= theNode->Timer;
			}
			break;
		default:
			targetTally = -1;
	}

	if (targetTally >= 0
		&& targetTally != theNode->Special[1])
	{
		theNode->Special[1] = targetTally;

		char text[128];

		switch (theNode->Special[0])
		{
			case 0:
			case 1:
			case 2:
				snprintf(text, sizeof(text), "%d", targetTally);
				break;

			case 3:
				if (targetTally <= 0)
					snprintf(text, sizeof(text), "%s", Localize(STR_ALL_HUMANS_RESCUED));
				else if (targetTally == 1)
					snprintf(text, sizeof(text), "%s", Localize(STR_1_HUMAN_MISSING));
				else
					snprintf(text, sizeof(text), Localize(STR_N_HUMANS_MISSING), targetTally);
				break;
		}

		TextMesh_Update(text, kTextMeshAlignCenter, theNode);
	}
}


/**************** CREATE TALLY TEXT ********************/
//
// type 0: text contents will track gBonus
// type 1: text contents will track gScore
// type 2: text contents will track gInventoryQuantity
//

static ObjNode* CreateTallyText(int type, float y)
{
	gNewObjectDefinition.coord		= (OGLPoint3D){0, y, 0};
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= MoveTallyText;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1.5f;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	ObjNode* tallyText = TextMesh_NewEmpty(16, &gNewObjectDefinition);
	tallyText->Special[0] = type;
	tallyText->Special[1] = -1;		// Special[1]: currently displaying number

	return tallyText;
}


#pragma mark -

/**************** DO HUMANS BONUS TALLY ********************/

static void DoHumansBonusTally(void)
{
	gScoreAlpha = 0;
	gShowScoreMode = SHOW_SCORE_MODE_HUMANBONUS;

	gNumHumansShownSoFar = 0;

	CreateBonusHumans();
	ObjNode* tallyText = CreateTallyText(0, 150);

	ObjNode* tallyText2 = CreateTallyText(3, 180);
	tallyText2->Scale.x *= 0.66f;
	tallyText2->Scale.y *= 0.66f;
	tallyText2->Scale.z *= 0.66f;
	UpdateObjectTransforms(tallyText2);

	CalcFramesPerSecond();
	UpdateInput();

	while(true)
	{
		float	fps = gFramesPerSecondFrac * gBonusFastForward;

		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();
		OGL_DrawScene(DrawBonusCallback);

		if (gGameViewInfoPtr->cameraPlacement.cameraLocation.x < 0.0f)				// keep @ 0 for icon alignment
			OGL_MoveCameraFrom(fps * 5.0f, 0, 0);	// slowly move camera

		CheckFastForward();

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

	DeleteObject(tallyText);
	DeleteObject(tallyText2);
}


/**************** DO INVENTORY BONUS TALLY ********************/

static void DoInventoryBonusTally(void)
{
	CreateInventoryIcons();
	ObjNode* tallyTextQuantity = CreateTallyText(2, -10);
	ObjNode* tallyTextBonus = CreateTallyText(0, 150);

	gBonus = 0;

	gScoreAlpha = 0;
	gInventoryQuantityAlpha = 0;

	gShowScoreMode = SHOW_SCORE_MODE_INVENTORY;

	CalcFramesPerSecond();
	UpdateInput();

	while(true)
	{
		float	fps = gFramesPerSecondFrac * gBonusFastForward;

		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();
		OGL_DrawScene(DrawBonusCallback);

		CheckFastForward();

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

	DeleteObject(tallyTextQuantity);
	DeleteObject(tallyTextBonus);
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

	ObjNode* tallyTextBonus = CreateTallyText(0, 150);
	ObjNode* tallyTextTotal = CreateTallyText(1, 20);

	CalcFramesPerSecond();
	UpdateInput();

	while(true)
	{
		float	fps = gFramesPerSecondFrac * gBonusFastForward;

		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();
		OGL_DrawScene(DrawBonusCallback);

		CheckFastForward();



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


	DeleteObject(tallyTextBonus);
	DeleteObject(tallyTextTotal);

}


/*************************** DO SAVE GAME ********************************/

static int DoSaveGamePrompt(void)
{
static const OGLColorRGBA noHiliteColor = {.3,.5,.2,1};

	static const MenuItem savePrompt[] =
	{
		{.type=kMenuItem_Pick,		.text=STR_SAVE_GAME,				.pick=SAVE_MENU_SELECTION_SAVE},
		{.type=kMenuItem_Pick,		.text=STR_CONTINUE_WITHOUT_SAVING,	.pick=SAVE_MENU_SELECTION_CONTINUE},
		{.type=kMenuItem_END_SENTINEL},
	};

	MenuStyle menuStyle = kDefaultMenuStyle;
	menuStyle.centeredText = true;
	menuStyle.asyncFadeOut = false;
	menuStyle.fadeInSpeed = 10;
	menuStyle.inactiveColor = noHiliteColor;
	menuStyle.standardScale = 1.0f;
	menuStyle.rowHeight = 24;
	menuStyle.darkenPane = false;
	menuStyle.playMenuChangeSounds = false;

	return StartMenu(savePrompt, &menuStyle, nil, DrawObjects);
}


#pragma mark -

/**************** DO TRACTOR BEAM ********************/

static void DoTractorBeam(void)
{
	InitBonusTractorBeam();


	CalcFramesPerSecond();
	UpdateInput();

	MakeFadeEvent(true, 1.0);

	while(gBonusBeamTimer < 12.0f)
	{

		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();
		OGL_DrawScene(DrawObjects);
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

	OGL_UpdateCameraFromTo(&from, &to);
}

/***************** MOVE BONUS ROCKET: TRACTOR BEAM ********************/

static void MoveBonusRocket_TractorBeam(ObjNode *rocket)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*flame;
uint32_t	vol;

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

	vol = gGammaFadeFrac * FULL_CHANNEL_VOLUME / 2;
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

static void DrawBonusCallback(void)
{
	DrawObjects();

	switch(gShowScoreMode)
	{
				/* DRAW HUMAN BONUS */

		case	SHOW_SCORE_MODE_HUMANBONUS:
				DrawHumanBonus();
				break;

				/* DRAW INVENTORY BONUS */

		case	SHOW_SCORE_MODE_INVENTORY:
				DrawInventoryBonus();
				break;

				/* DRAW TOTALS */

		case	SHOW_SCORE_MODE_ADDTOSCORE:
				DrawBonusToScore();
				break;

	}
}


/************************ DRAW HUMAN BONUS *************************/

static void DrawHumanBonus(void)
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
			//
			// Note: actual tally is drawn as a text ObjNode
			//

			/* DRAW GLOW */

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	gGlobalTransparency = (.7f + RandomFloat()*.1f) * gScoreAlpha;
	DrawInfobarSprite2(-150, 95, 300, SPRITE_GROUP_BONUS, BONUS_SObjType_BonusGlow);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			/* DRAW TEXT */

	gGlobalTransparency = gScoreAlpha;
	DrawInfobarSprite2(-150, 95, 300, SPRITE_GROUP_BONUS, BONUS_SObjType_BonusText);



			/***********/
			/* CLEANUP */
			/***********/

	OGL_PopState();
	gGlobalMaterialFlags = 0;
	gGlobalTransparency = 1.0;

}



/************************ DRAW INVENTORY BONUS *************************/

static void DrawInventoryBonus(void)
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
	DrawInfobarSprite2(-150, 95, 300, SPRITE_GROUP_BONUS, BONUS_SObjType_InventoryGlow);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			/* DRAW TEXT */

	gGlobalTransparency = gScoreAlpha;
	DrawInfobarSprite2(-150, 95, 300, SPRITE_GROUP_BONUS, BONUS_SObjType_InventoryText);


			/***********/
			/* CLEANUP */
			/***********/

	OGL_PopState();
	gGlobalMaterialFlags = 0;
	gGlobalTransparency = 1.0;

}


/************************ DRAW BONUS TO SCORE *************************/

static void DrawBonusToScore(void)
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
	DrawInfobarSprite2(-150, 95, 300, SPRITE_GROUP_BONUS, BONUS_SObjType_TotalBonusGlow);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			/* DRAW TEXT */

	gGlobalTransparency = gScoreAlpha;
	DrawInfobarSprite2(-150, 95, 300, SPRITE_GROUP_BONUS, BONUS_SObjType_TotalBonusText);



			/**************/
			/* DRAW SCORE */
			/**************/

			/* DRAW GLOW */

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	gGlobalTransparency = (.7f + RandomFloat()*.1f) * gScoreAlpha;
	DrawInfobarSprite2(-150, -35, 300, SPRITE_GROUP_BONUS, BONUS_SObjType_ScoreGlow);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			/* DRAW TEXT */

	gGlobalTransparency = gScoreAlpha;
	DrawInfobarSprite2(-150, -35, 300, SPRITE_GROUP_BONUS, BONUS_SObjType_ScoreText);



			/***********/
			/* CLEANUP */
			/***********/

	OGL_PopState();
	gGlobalMaterialFlags = 0;
	gGlobalTransparency = 1.0;
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
static float gInventoryCountTimer = 0;
float	fps = gFramesPerSecondFrac * gBonusFastForward;

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
				gInventoryQuantityAlpha = 0;
				DeleteObject(theNode);
				return;
			}
		}


				/* SPIN THE ICON */

		theNode->Rot.y +=  fps * PI2 / gBonusFastForward;
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
				gInventoryCountTimer += INVENTORY_COUNT_DELAY;		// reset timer
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

	if (gNumHumansRescuedTotal == gNumHumansInLevel)
		gBonusTimer += 2;
	else if (gNumHumansRescuedTotal != 0)
		gBonusTimer += 1;

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
			if (j & 1)												// alternate left/right
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
			newObj->HumanNumberInSequence = j;

			d += 1.25f;
			j++;
		}
	}
}


/****************** MOVE BONUS HUMAN **********************/

static void MoveBonusHuman(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac * gBonusFastForward;

			/* SEE IF FADE IN NOW */

	if (theNode->InvisibleDelay > 0.0f)
	{
		theNode->InvisibleDelay -= fps;
		if (theNode->InvisibleDelay <= 0.0f)
		{
				/* START TELEPORT */

			StartBonusTeleport(theNode);
			gNumHumansShownSoFar++;
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
float	fps = gFramesPerSecondFrac * MinFloat(1.5f, gBonusFastForward);

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
float	fps = gFramesPerSecondFrac * gBonusFastForward;
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

	if (theNode->Mode == 1
		&& gShowScoreMode == SHOW_SCORE_MODE_OFF)
	{
		theNode->StatusBits |= STATUS_BIT_HIDDEN;
		return;
	}
	else
	{
		theNode->StatusBits &= ~STATUS_BIT_HIDDEN;
	}

	float lowY = theNode->SpecialF[0];
	float highY = theNode->SpecialF[1];

	theNode->Timer += fps * 2.0f;

	theNode->SpecialF[2] += fps * 1.3f;
	theNode->SpecialF[2] = MinFloat(theNode->SpecialF[2], 1);

	theNode->ColorFilter.a = .3f + (sin(theNode->Timer) + 1.0f) * .5f;
	theNode->ColorFilter.a *= theNode->SpecialF[2];

	theNode->Coord.y += theNode->Delta.y * fps;
	if (theNode->Coord.y < lowY)
		theNode->Coord.y = highY;

	UpdateObjectTransforms(theNode);
}


/******************* MOVE BONUS ROCKET *****************************/

static void MoveBonusRocket(ObjNode *rocket)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*flame;
uint32_t	vol;

	GetObjectInfo(rocket);

	gCoord.y -= 100.0f * fps;
	gCoord.z += 260.0f * fps;

	rocket->Rot.y += fps * .5f;

	if (gCoord.z > 400.0f)													// fade when close
	{
		rocket->ColorFilter.a -= fps * 1.3f;								// fade out the ship
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

	vol = gGammaFadeFrac * FULL_CHANNEL_VOLUME / 2;
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

