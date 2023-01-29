/****************************/
/*   	WIN/LOSESCREEN.C	*/
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

static void FreeLoseScreen(void);
static void SetupLoseScreen(void);
static void DrawGameOverLogo(ObjNode* theNode);
static void UpdateConveyorBelt(void);
static ObjNode *PutNewHumanOnBelt(float xoff);
static void PrimeInitialHumansOnBelt(void);
static void StartTransformationEffect(short index);
static void MoveTransformingAlien(ObjNode *theNode);
static void UpdateTransformEffect(void);
static void MoveWarpEffect(ObjNode *theNode);



/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	LOSE_ObjType_Background,
	LOSE_ObjType_Warp
};



#define	MAX_HUMANS_ON_BELT		20

enum
{
	BELT_MODE_MOVE,
	BELT_MODE_STOP
};


#define	HUMAN_SPACING	220.0f

#define	HUMANS_IN_LINE	10


#define	LOSE_TEXT_SIZE		30.0f

/*********************/
/*    VARIABLES      */
/*********************/

static	int		gNumHumansOnBelt;
static	ObjNode	*gHumansOnBelt[MAX_HUMANS_ON_BELT];

static	Byte	gBeltMode,gBeltIndex,gHeadHuman;
static	float	gBeltTimer;

static	short	gTransformingHumanIndex;
static	float	gTransformFade,gTransformTimer;
static	Boolean	gIsTransforming;
static	ObjNode	*gTransformingAlien;



/********************** DO LOSE SCREEN **************************/

void DoLoseScreen(void)
{
float	timer = 43.0f;

			/* SETUP */

	SetupLoseScreen();
	MakeFadeEvent(true, 1.0);

			/********/
			/* LOOP */
			/********/

	do
	{
			/* MOVE */

		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();

		/* UPDATE CONVEYOR BELT */

		UpdateConveyorBelt();

			/* UPDATE TRANSFORM */

		if (gIsTransforming)
			UpdateTransformEffect();


			/* DRAW */

		OGL_DrawScene(DrawObjects);

		timer -= gFramesPerSecondFrac;
		if (timer <= 0.0f)
			break;

	} while (!UserWantsOut());


			/* CLEANUP */

	OGL_FadeOutScene(DrawObjects, NULL);
	FreeLoseScreen();
}



/********************* SETUP LOSE SCREEN **********************/

static void SetupLoseScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
ObjNode			*newObj;
int				i,j;
static const OGLVector3D	fillDirection1 = { -1, -.2, -.6 };

	PlaySong(SONG_LOSESONG, kPlaySong_PlayOnceFlag);


			/* INIT HUMANS */

	gNumHumansOnBelt = 0;
	for (i = 0; i < MAX_HUMANS_ON_BELT; i++)
		gHumansOnBelt[i] = nil;

	gBeltMode = BELT_MODE_STOP;
	gBeltTimer = 0.0f;
	gBeltIndex = 0;
	gHeadHuman = 0;
	gIsTransforming = false;


			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= 1.1;
	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= 3000;

	viewDef.camera.from.x		= 300;
	viewDef.camera.from.z		= 600;
	viewDef.camera.from.y		= 250;
	viewDef.camera.to.y 			= 150.0f;

	viewDef.lights.fillDirection[0] = fillDirection1;

	viewDef.styles.fogStart = 1200;
	viewDef.styles.fogEnd = 1450;
	viewDef.styles.useFog = true;


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

		gAnaglyphFocallength	= 350.0f;
		gAnaglyphEyeSeparation 	= 25.0f;
	}

	OGL_SetupWindow(&viewDef);


				/************/
				/* LOAD ART */
				/************/

	InitEffects();

				/* LOAD AUDIO */

	LoadSoundBank(SOUNDBANK_LOSE);

			/* LOAD SPRITES */

//	LoadSpriteGroup(SPRITE_GROUP_PARTICLES, PARTICLE_SObjType_COUNT, "particle");
	LoadSpriteGroup(SPRITE_GROUP_SPHEREMAPS, SPHEREMAP_SObjType_COUNT, "spheremap");
	LoadSpriteGroup(SPRITE_GROUP_LOSE, GAMEOVER_SObjType_COUNT, "lose");
	BlendAllSpritesInGroup(SPRITE_GROUP_PARTICLES);


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:losescreen.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LOSESCREEN);


			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_FARMER);
	LoadASkeleton(SKELETON_TYPE_BEEWOMAN);
	LoadASkeleton(SKELETON_TYPE_SCIENTIST);
	LoadASkeleton(SKELETON_TYPE_SKIRTLADY);
	LoadASkeleton(SKELETON_TYPE_BRAINALIEN);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_BRAINALIEN,
								 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);



		/****************/
		/* BUILD MODELS */
		/****************/

				/* BACKGROUND */

	gNewObjectDefinition.group 		= MODEL_GROUP_LOSESCREEN;
	gNewObjectDefinition.type 		= LOSE_ObjType_Background;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG;
	gNewObjectDefinition.slot 		= 50;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* POST SPARKLES */

	for (j = 0; j < 2; j++)
	{
		static const OGLPoint3D	where[2] =
		{
			{0, 175.0f * 2.0f, 65.0f * 2.0f},
			{0, 175.0f * 2.0f, -65.0f * 2.0f},
		};

		i = newObj->Sparkles[j] = GetFreeSparkle(newObj);
		if (i != -1)
		{
			gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_RANDOMSPIN|SPARKLE_FLAG_FLICKER;
			gSparkles[i].where = where[j];

			gSparkles[i].color.r = 1;
			gSparkles[i].color.g = 1;
			gSparkles[i].color.b = 1;
			gSparkles[i].color.a = .8;

			gSparkles[i].scale = 60.0f;

			gSparkles[i].separation = 100.0f;

			gSparkles[i].textureNum = PARTICLE_SObjType_RedGlint;
		}
	}

			/* HUMANS */

	PrimeInitialHumansOnBelt();


			/* LOGO */

	NewObjectDefinitionType gameOverLogoDef =
	{
		.genre = CUSTOM_GENRE,
		.slot = DRAWEXTRA_SLOT,
		.scale = 1,
		.flags = STATUS_BIT_DONTCULL,
		.drawCall = DrawGameOverLogo,
	};
	MakeNewObject(&gameOverLogoDef);
}



/********************** FREE LOSE SCREEN **********************/

static void FreeLoseScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeEffects();
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	DisposeSoundBank(SOUNDBANK_LOSE);
	OGL_DisposeWindowSetup();
	Pomme_FlushPtrTracking(true);
}

/***************** DRAW GAME OVER LOGO *******************/

static void DrawGameOverLogo(ObjNode* theNode)
{
	(void) theNode;

	OGL_PushState();

	SetInfobarSpriteState(true);

				/* DRAW GAME OVER */

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	DrawInfobarSprite2(-200, -230, 400, SPRITE_GROUP_LOSE, LOSE_SObjType_GameOver);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	OGL_PopState();
}

#pragma mark -


/*********************** UPDATE CONVEYOR BELT *****************************/

static void UpdateConveyorBelt(void)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*human;
int		i;

	switch(gBeltMode)
	{
		case	BELT_MODE_STOP:
				gBeltTimer -= fps;										// see if time to move again
				if (gBeltTimer <= 0.0f)
				{
					gBeltMode = BELT_MODE_MOVE;
					PutNewHumanOnBelt(0);
					PlayEffect_Parms(EFFECT_CONVEYORBELT, FULL_CHANNEL_VOLUME/4, FULL_CHANNEL_VOLUME * 3/4, NORMAL_CHANNEL_RATE);

				}
				break;

		case	BELT_MODE_MOVE:

					/* MOVE ALL HUMANS FORWARD */

				for (i = 0; i < MAX_HUMANS_ON_BELT; i++)
				{
					human = gHumansOnBelt[i];
					if (human)
					{
						human->Coord.x += 200.0f * fps;
						if (human->Coord.x > 700.0f)						// see if scrolled off
						{
							DeleteObject(human);
							gHumansOnBelt[i] = nil;
							continue;
						}

						UpdateObjectTransforms(human);
					}
				}

						/* SEE IF HEAD HUMAN IS IN PLACE */

				human = gHumansOnBelt[gHeadHuman];
				if (human)
				{
					if (human->Coord.x >= 0.0f)								// see if @ transformer
					{
						gBeltMode = BELT_MODE_STOP;							// stop to do transformation
						gBeltTimer = 1.2f;

						human->StatusBits |= STATUS_BIT_NOZWRITES;			// do this so will transform nicer
						StartTransformationEffect(gHeadHuman);						// start to transform human

						if (++gHeadHuman >= MAX_HUMANS_ON_BELT)				// set next head human
							gHeadHuman = 0;

					}
				}

				break;



	}
}


/****************** PRIME INITIAL HUMANS ON BELT ****************************/

static void PrimeInitialHumansOnBelt(void)
{
int		i;
float	xoff;
ObjNode	*human;

	xoff = HUMAN_SPACING * HUMANS_IN_LINE - HUMAN_SPACING;
	for (i = 0; i < (HUMANS_IN_LINE-1); i++)
	{
		human = PutNewHumanOnBelt(xoff);

		if (human)
		{
			SetSkeletonAnimTime(human->Skeleton, RandomFloat());	// set random time index so all of these are not in sync

		}

		xoff -= HUMAN_SPACING;
	}
}



/********************* PUT NEW HUMAN ON BELT ****************************/

static ObjNode *PutNewHumanOnBelt(float xoff)
{
static const short	skels[4] =
{
	SKELETON_TYPE_FARMER,
	SKELETON_TYPE_BEEWOMAN,
	SKELETON_TYPE_SCIENTIST,
	SKELETON_TYPE_SKIRTLADY
};

ObjNode	*newObj;

	gNewObjectDefinition.type 		= skels[MyRandomLong()&0x3];
	gNewObjectDefinition.animNum	= 0;
	gNewObjectDefinition.coord.x 	= -HUMANS_IN_LINE * HUMAN_SPACING + xoff;
	gNewObjectDefinition.coord.y 	= 20;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 205;
	gNewObjectDefinition.moveCall	= nil;
	gNewObjectDefinition.rot 		= -PI/2;
	gNewObjectDefinition.scale 		= 2.0;
	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

	newObj->Coord.y 	-= newObj->BBox.min.y;
	UpdateObjectTransforms(newObj);

	newObj->CType = CTYPE_HUMAN;


			/* ADD TO LIST */

	if (gHumansOnBelt[gBeltIndex] != nil)							// make sure this slot free
		DoFatalAlert("PutNewHumanOnBelt: slot overflow!");
	gHumansOnBelt[gBeltIndex++] = newObj;
	if (gBeltIndex >= MAX_HUMANS_ON_BELT)
		gBeltIndex = 0;

	return(newObj);
}




#pragma mark -


/************************ START TRANSFORMATION EFFECT *****************************/

static void StartTransformationEffect(short index)
{
ObjNode	*newObj;
int		i;

	gTransformingHumanIndex = index;
	gTransformFade = 0;
	gIsTransforming = true;
	gTransformTimer = .9f;


				/********************/
				/* MAKE BRAIN ALIEN */
				/********************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_BRAINALIEN;
	gNewObjectDefinition.animNum 	= 0;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 20;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 400;
	gNewObjectDefinition.moveCall 	= MoveTransformingAlien;
	gNewObjectDefinition.rot 		= -PI/2;
	gNewObjectDefinition.scale 		= 1.7;

	gTransformingAlien = MakeNewSkeletonObject(&gNewObjectDefinition);

	gTransformingAlien->Coord.y 	-= gTransformingAlien->BBox.min.y;
	UpdateObjectTransforms(gTransformingAlien);

	gTransformingAlien->ColorFilter.a = 0;						// start faded out


	CreateBrainAlienGlow(gTransformingAlien);



				/********************/
				/* MAKE WARP EFFECT */
				/********************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LOSESCREEN;
	gNewObjectDefinition.type 		= LOSE_ObjType_Warp;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 200;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_GLOW | STATUS_BIT_KEEPBACKFACES | STATUS_BIT_NOZWRITES | STATUS_BIT_ROTZXY;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveWarpEffect;
	gNewObjectDefinition.rot 		= PI/2;
	gNewObjectDefinition.scale 	    = .2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->ColorFilter.a = .99;
	newObj->Timer = 1.0f;

				/* SPARKLE */

	i = newObj->Sparkles[0] = GetFreeSparkle(newObj);
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_RANDOMSPIN|SPARKLE_FLAG_FLICKER;
		gSparkles[i].where = gNewObjectDefinition.coord;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = .8;

		gSparkles[i].scale = 200.0f;

		gSparkles[i].separation = 200.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark4;
	}

	PlayEffect_Parms(EFFECT_TRANSFORM, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME/2, NORMAL_CHANNEL_RATE);

}


/******************** UPDATE TRANSFORM EFFECT **************************/

static void UpdateTransformEffect(void)
{
ObjNode	*human;

			/* SEE IF STOP */

	gTransformTimer -= gFramesPerSecondFrac;
	if (gTransformTimer <= 0.0f)
	{
		gIsTransforming = false;
		gTransformingAlien->MoveCall = UpdateBrainAlienGlow;
		return;
	}


				/* FADE */

	gTransformFade += gFramesPerSecondFrac * 1.5f;
	if (gTransformFade >= 1.0f)
	{
		gTransformFade = 1.0f;
	}


			/* SET HUMAN FADE VALUE */

	human = gHumansOnBelt[gTransformingHumanIndex];						// get human
	if (human->CType & CTYPE_HUMAN)										// make sure human and not alien
	{
		human->ColorFilter.a = 1.0f - gTransformFade;
		if (human->ColorFilter.a <= 0.0f)								// fade out
		{
			DeleteObject(human);										// delete the human
			gHumansOnBelt[gTransformingHumanIndex] = gTransformingAlien;	// put the alien in its place
		}
	}
}



/*********** MOVE TRANSFORMING ALIEN *********************/

static void MoveTransformingAlien(ObjNode *theNode)
{
	theNode->ColorFilter.a = gTransformFade;

	UpdateBrainAlienGlow(theNode);

}


/********************* MOVE WARP EFFECT **********************/

static void MoveWarpEffect(ObjNode *theNode)
{
float	s = theNode->Scale.x;
float	fps = gFramesPerSecondFrac;

	theNode->Timer -= fps;
	if (theNode->Timer < 0.0f)
	{
		DeleteObject(theNode);
		return;
	}


	s += fps * 25.0f;
	if (s > 2.1f)
		s = .4f;

	theNode->Scale.x =
	theNode->Scale.y =
	theNode->Scale.z = s;

	theNode->Rot.z = RandomFloat()*PI2;

	UpdateObjectTransforms(theNode);



}

