/****************************/
/*   	LEVELINTROS.C		*/
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

static void SetupIntroScreen(void);
static void FreeIntroScreen(void);
static void CreateIntroSaucers(void);
static void MoveIntroSaucer(ObjNode *topObj);
static void MovePlanet(ObjNode *theNode);
static void MoveStar(ObjNode *theNode);
static void CreateIntroSaucer2(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	SAUCER_SCALE	1.0f

#define	ICESAUCER_SCALE	1.8f


/******************* LEVELINTRO *************************/

enum
{
	INTRO_ObjType_EnemySaucer_Top,
	INTRO_ObjType_IceSaucer,

	INTRO_ObjType_Star,
	INTRO_ObjType_PlanetGlow,

	INTRO_ObjType_Earth,
	INTRO_ObjType_EarthClouds,

	INTRO_ObjType_Blob,
	INTRO_ObjType_BlobClouds,

	INTRO_ObjType_Apoc,
	INTRO_ObjType_ApocClouds,

	INTRO_ObjType_Cloud,
	INTRO_ObjType_CloudClouds,

	INTRO_ObjType_Jungle,
	INTRO_ObjType_JungleClouds,

	INTRO_ObjType_Fire,
	INTRO_ObjType_FireClouds,

	INTRO_ObjType_Saucer,
	INTRO_ObjType_SaucerClouds,

	INTRO_ObjType_X,
	INTRO_ObjType_XClouds
};


/*********************/
/*    VARIABLES      */
/*********************/


static float	gIntroTimer;


/******************* DO LEVEL INTRO **************************/

void DoLevelIntro(void)
{
float	oldTime,maxTime = 11.0f;

	if (gCommandLine.skipFluff)
		return;

	switch (gLevelNum)										// level special cases
	{
		case	LEVEL_NUM_BLOBBOSS:
		case	LEVEL_NUM_JUNGLEBOSS:
				return;										// these levels don't have an intro, so bail now.

		case	LEVEL_NUM_BRAINBOSS:
				maxTime = 5.0f;								// shorter intro time
				break;
	}


	gIntroTimer = 0;

			/* SETUP */

	SetupIntroScreen();
	MakeFadeEvent(true, 1.0);

				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	UpdateInput();

	while(true)
	{
			/* DRAW STUFF */

		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();
		OGL_DrawScene(DrawObjects);

		if (UserWantsOut())
			break;


				/* CHECK TIMER */

		oldTime = gIntroTimer;
		gIntroTimer += gFramesPerSecondFrac;
		if ((gIntroTimer > maxTime) && (oldTime <= maxTime))
		{
			MakeFadeEvent(false, 1.0);
		}

		if (gGammaFadeFrac <= 0.0f)
			break;
	}


			/* CLEANUP */

	if (gGammaFadeFrac > 0.0f)
	{
		float backupGamma = gGammaFadeFrac;
		OGL_FadeOutScene(DrawObjects, NULL);
		gGammaFadeFrac = backupGamma;
	}

	FreeIntroScreen();
}


/***************** DRAW INTRO CALLBACK *******************/


static void MoveLevelName(ObjNode* theNode)
{
	float alpha = theNode->ColorFilter.a;

	if (gLevelNum == LEVEL_NUM_BRAINBOSS)							// comes in sooner on Brain Boss level
	{
		if (gIntroTimer > 1.0f)
			alpha += gFramesPerSecondFrac *.6f;
	}
	else
	{
		if (gIntroTimer > 3.0f)
			alpha += gFramesPerSecondFrac *.6f;
	}

	if (alpha > 1.0f)
		alpha = 1.0f;

	theNode->ColorFilter.a = alpha;
}


/********************* SETUP INTRO SCREEN **********************/

static void SetupIntroScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
ObjNode	*planet,*glow,*newObj,*clouds;
int		i;

static const OGLColorRGBA glowColor[] =
{
	{.7,.7,1.,.99},						// earth
	{.7,.3,1.,.99},						// slime
	{.7,.7,1.,.99},						// slime boss
	{.7,.7,.7,.80},						// apoc
	{.7,.2,1.,.99},						// cloud
	{.9,.7,.6,.99},						// jungle
	{.7,.7,1.,.99},						// jungle boss
	{.7,.7,.1,.99},						// fire & ice
	{.5,.7,1.,.99},						// saucer
	{1.,1.,.9,.99},						// brain boss
};

static OGLColorRGBA			ambientColor = { .03, .03, .03, 1 };

const Byte	terra[] =
{
	INTRO_ObjType_Earth,				// earth
	INTRO_ObjType_Blob,					// slime
	INTRO_ObjType_Blob,					// slime boss
	INTRO_ObjType_Apoc,					// apoc
	INTRO_ObjType_Cloud,				// cloud
	INTRO_ObjType_Jungle,				// jungle
	INTRO_ObjType_Jungle,				// jungle boss
	INTRO_ObjType_Fire,					// fire & ice
	INTRO_ObjType_Saucer,				// saucer
	INTRO_ObjType_X,					// brain boss
};

const Byte	cloud[] =
{
	INTRO_ObjType_EarthClouds,			// earth
	INTRO_ObjType_BlobClouds,			// slime
	INTRO_ObjType_BlobClouds,			// slime boss
	INTRO_ObjType_ApocClouds,			// apoc
	INTRO_ObjType_CloudClouds,			// cloud
	INTRO_ObjType_JungleClouds,			// jungle
	INTRO_ObjType_JungleClouds,			// jungle boss
	INTRO_ObjType_FireClouds,			// fire & ice
	INTRO_ObjType_SaucerClouds,			// saucer
	INTRO_ObjType_XClouds,				// brain boss
};



			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.lights.ambientColor 	= ambientColor;
	viewDef.lights.fillDirection[0].x *= -1.0f;

	viewDef.camera.fov 		= 1.4;
	viewDef.camera.hither 	= 400;
	viewDef.camera.yon 		= 15000;

	viewDef.styles.redFont = true;

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

		gAnaglyphFocallength	= 300.0f;
		gAnaglyphEyeSeparation 	= 70.0f;
	}



	OGL_SetupWindow(&viewDef);


				/************/
				/* LOAD ART */
				/************/


			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:spheremap.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS);

			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:LevelIntro.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELINTRO);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELINTRO, INTRO_ObjType_IceSaucer,
								 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sea);


	InitSparkles();
	InitParticleSystem();


			/**************/
			/* MAKE TITLE */
			/**************/

	gNewObjectDefinition.scale 	    = 1.00f;
	gNewObjectDefinition.coord.x 	= -310;
	gNewObjectDefinition.coord.y 	= 200;
	gNewObjectDefinition.coord.z 	= 0.0f;
	gNewObjectDefinition.moveCall	= MoveLevelName;
	ObjNode* levelName = TextMesh_New(Localize(STR_LEVEL_1 + gLevelNum), 0, &gNewObjectDefinition);
	levelName->ColorFilter.a = 0;


			/**************/
			/* MAKE STARS */
			/**************/

	for (i = 0; i < 170; i++)
	{
		OGLMatrix4x4	m;
		OGLPoint3D		p;
		static const OGLColorRGBA colors[] =
		{
			{1,1,1,1},			// white
			{1,.6,.6,1},		// red
			{.6,.6,1,1},		// blue
			{.7,.7,8,1},		// grey
		};

		OGLMatrix4x4_SetRotateAboutPoint(&m, &viewDef.camera.from, RandomFloat()*PI2,RandomFloat()*PI2,RandomFloat()*PI2);
		p.x = p.y =0;
		p.z = viewDef.camera.yon * .9f;
		OGLPoint3D_Transform(&p,&m,&gNewObjectDefinition.coord);

		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;
		gNewObjectDefinition.type 		= INTRO_ObjType_Star;
		gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES | STATUS_BIT_GLOW | STATUS_BIT_NOTEXTUREWRAP |
										STATUS_BIT_NOZWRITES | STATUS_BIT_DONTCULL | STATUS_BIT_NOLIGHTING | STATUS_BIT_AIMATCAMERA;
		gNewObjectDefinition.slot 		= 200;
		gNewObjectDefinition.moveCall 	= MoveStar;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 	    = 4.0f + RandomFloat() * 3.0f;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->SpecialF[0] = RandomFloat() * PI;

		newObj->ColorFilter = colors[MyRandomLong()&0x3];

	}


			/***************/
			/* MAKE PLANET */
			/***************/

				/* EARTH SPHERE */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;
	gNewObjectDefinition.type 		= terra[gLevelNum];
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= -viewDef.camera.yon * .8f;
	gNewObjectDefinition.flags 		= STATUS_BIT_ROTYZX | STATUS_BIT_DONTCULL | STATUS_BIT_UVTRANSFORM;
	gNewObjectDefinition.slot 		= 50;
	gNewObjectDefinition.moveCall 	= MovePlanet;
	gNewObjectDefinition.rot 		= PI2/3;
	gNewObjectDefinition.scale 	    = 60;
	planet = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* EARTH CLOUDS */

	gNewObjectDefinition.type 		= cloud[gLevelNum];
	gNewObjectDefinition.flags 		= STATUS_BIT_ROTYZX | STATUS_BIT_DONTCULL | STATUS_BIT_UVTRANSFORM |
									STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOZWRITES;
	if (gLevelNum == LEVEL_NUM_APOCALYPSE)
		gNewObjectDefinition.flags |= STATUS_BIT_GLOW;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	clouds = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	clouds->ColorFilter.a = .95;

			/* GLOW */

	gNewObjectDefinition.type 		= INTRO_ObjType_PlanetGlow;
	gNewObjectDefinition.flags 		= STATUS_BIT_ROTYZX | STATUS_BIT_DONTCULL | STATUS_BIT_UVTRANSFORM |
									STATUS_BIT_GLOW | STATUS_BIT_NOZWRITES;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.scale 	    *= 1.3f;
	glow = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	glow->ColorFilter = glowColor[gLevelNum];

	planet->ChainNode = glow;

	glow->ChainNode = clouds;


			/**************/
			/* MAKE SHIPS */
			/**************/

	switch(gLevelNum)
	{
		case	LEVEL_NUM_SAUCER:								// show ice saucer
				CreateIntroSaucer2();
				break;

		case	LEVEL_NUM_BRAINBOSS:							// no ships
				break;

		default:
				CreateIntroSaucers();

	}
}

/********************** CREATE INTRO SAUCERS ***********************/

static void CreateIntroSaucers(void)
{
ObjNode	*top;
int		n,r,c;
float	x,z;

	n = 0;
	z = 1000;
	for (r = 0; r < 4; r++)
	{
		x = 130.0f + r / 2.0f * -900.0f;

		for (c = 0; c < (r+1); c++)
		{
			gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;
			gNewObjectDefinition.type 		= INTRO_ObjType_EnemySaucer_Top;
			gNewObjectDefinition.coord.x 	= x + RandomFloat2() * 100.0f;
			gNewObjectDefinition.coord.y 	= -2000;
			gNewObjectDefinition.coord.z 	= z + RandomFloat2() * 100.0f;
			gNewObjectDefinition.flags 		= STATUS_BIT_NOZBUFFER;		// don't let saucers clip into earth
			gNewObjectDefinition.slot 		= 300;						// draw on top of stars b/c no zbuffer
			gNewObjectDefinition.moveCall 	= MoveIntroSaucer;
			gNewObjectDefinition.rot 		= RandomFloat()*PI2;
			gNewObjectDefinition.scale 		= SAUCER_SCALE;
			top = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			top->SpecialF[0] = RandomFloat2() * PI2;

			top->Kind = n++;

			x += 900.0f;
		}

		z += 900.0f;
	}
}


/********************** CREATE INTRO SAUCER 2 ***********************/

static void CreateIntroSaucer2(void)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELINTRO;
	gNewObjectDefinition.type 		= INTRO_ObjType_IceSaucer;
	gNewObjectDefinition.coord.x 	= 130;
	gNewObjectDefinition.coord.y 	= -2000;
	gNewObjectDefinition.coord.z 	= 1000;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOZBUFFER;		// don't let saucers clip into earth
	gNewObjectDefinition.slot 		= 300;						// draw on top of stars b/c no zbuffer
	gNewObjectDefinition.moveCall 	= MoveIntroSaucer;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= ICESAUCER_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
}




/********************** FREE INTRO ART **********************/

static void FreeIntroScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	DisposeParticleSystem();
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	OGL_DisposeWindowSetup();
	Pomme_FlushPtrTracking(true);
}

#pragma mark -


/************ MOVE INTRO SAUCER ************************/

static void MoveIntroSaucer(ObjNode *topObj)
{
float	fps = gFramesPerSecondFrac;
float	r;

	GetObjectInfo(topObj);

			/* MAKE WOBBLE */

	topObj->SpecialF[0] += fps * 3.0f;
	r = sin(topObj->SpecialF[0]) * .1f;
	topObj->Rot.z = r;

			/* MOVE IT */

	gCoord.z -= 800.0f * fps;

			/* MAKE IT SPIN */

	topObj->Rot.y -= fps * 3.0f;


	UpdateObject(topObj);


			/* UPDATE CAMERA */

	if (topObj->Kind == 0)
	{
		OGL_UpdateCameraFromTo(&gGameViewInfoPtr->cameraPlacement.cameraLocation, &gCoord);

	}


			/* UPDATE AUDIO */

	if (topObj->EffectChannel != -1)
	{
		Update3DSoundChannel(EFFECT_SAUCER, &topObj->EffectChannel, &topObj->Coord);
	}
	else
		topObj->EffectChannel = PlayEffect_Parms3D(EFFECT_SAUCER,  &topObj->Coord, NORMAL_CHANNEL_RATE + (MyRandomLong()&0x1ff), .2);
}




/********************* MOVE PLANET ******************************/

static void MovePlanet(ObjNode *theNode)
{
ObjNode	*glow = theNode->ChainNode;
ObjNode	*clouds = glow->ChainNode;

	theNode->TextureTransformU -= gFramesPerSecondFrac * .01f;

	if (clouds)
		clouds->TextureTransformU -= gFramesPerSecondFrac * .015f;

}


/************************ MOVE STAR *************************/

static void MoveStar(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->SpecialF[0] += fps * 2.0f;

	theNode->ColorFilter.a = .3f + (sin(theNode->SpecialF[0]) + 1.0f) * .5f;

}














