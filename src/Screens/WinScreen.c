/****************************/
/*   	WIN SCREEN.C		*/
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

static void FreeWinScreen(void);
static void SetupWinScreen(void);
static void DrawWinCallback(void);

static void StartTabloid(void);
static void DrawTabloid(ObjNode *theNode);
static void MoveTabloid(ObjNode *theNode);

static void DoTheEnd(void);
static void MoveTheEndPane(ObjNode *theNode);
static void MoveTheEndText(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	WIN_ObjType_Background,
	WIN_ObjType_Rocket,
	WIN_ObjType_Cyc,
	WIN_ObjType_Tabloid
};


#define	WIN_TEXT_SIZE		30.0f

/*********************/
/*    VARIABLES      */
/*********************/

static ObjNode *gTabloid = nil;


/********************** DO WIN SCREEN **************************/

void DoWinScreen(void)
{
float	timer = 90.0f;
float	delayToTabloid = 6.0f;

			/* SETUP */

	SetupWinScreen();
	MakeFadeEvent(true, 1.0);

			/********/
			/* LOOP */
			/********/

	while(true)
	{
			/* MOVE */

		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();


			/* DRAW */

		OGL_DrawScene(DrawWinCallback);


			/* CHECK TIMERS */

		timer -= gFramesPerSecondFrac;					// see if done w/ this screen
		if (timer <= 0.0f)
			break;

		if (!gTabloid)
		{
			delayToTabloid -= gFramesPerSecondFrac;			// see if bring up tabloid
			if (delayToTabloid <= 0.0f)
				StartTabloid();
		}
		else
		if (UserWantsOut())
			break;

	}

			/****************/
			/* DO "THE END" */
			/****************/

	DoTheEnd();



			/* CLEANUP */

	OGL_FadeOutScene(DrawWinCallback, NULL);
	FreeWinScreen();
}


/******************** DO THE END ************************/

static void DoTheEnd(void)
{
ObjNode	*text,*glow,*pane;
const OGLVector3D scale = {500,500/4,1};
const OGLVector3D scale2 = {140,140,1};
float	timer;

			/****************/
			/* MAKE SPRITES */
			/****************/

			/* DARKEN PANE */

	gNewObjectDefinition.group 		= SPRITE_GROUP_LOSE;
	gNewObjectDefinition.type 		= WIN_SObjType_BlackOut;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	gNewObjectDefinition.moveCall 	= MoveTheEndPane;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1;
	pane = MakeSpriteObject(&gNewObjectDefinition);
	pane->ColorFilter.a = 0;



			/* GLOW */

	gNewObjectDefinition.type 		= WIN_SObjType_TheEnd_Glow;
	gNewObjectDefinition.coord.x 	= -scale.x/2;
	gNewObjectDefinition.coord.y 	= -scale.y/2 - 100;
	gNewObjectDefinition.flags 		= STATUS_BIT_GLOW;
	gNewObjectDefinition.moveCall 	= MoveTheEndText;
	glow = MakeSpriteObject(&gNewObjectDefinition);

	glow->Scale = scale;
	glow->SpecialF[0] = glow->ColorFilter.a = 0;


			/* TEXT */

	gNewObjectDefinition.type 		= WIN_SObjType_TheEnd_Text;
	gNewObjectDefinition.flags 		= 0;
	text = MakeSpriteObject(&gNewObjectDefinition);

	text->Scale = scale;
	text->SpecialF[0] = text->ColorFilter.a = 0;


			/* Q-GLOW */

	gNewObjectDefinition.type 		= WIN_SObjType_QGlow;
	gNewObjectDefinition.coord.x 	= -scale2.x/2;
	gNewObjectDefinition.coord.y 	= -scale2.y/2 + 40;
	gNewObjectDefinition.flags 		= STATUS_BIT_GLOW;
	gNewObjectDefinition.moveCall 	= MoveTheEndText;
	glow = MakeSpriteObject(&gNewObjectDefinition);

	glow->Scale = scale2;
	glow->SpecialF[0] = glow->ColorFilter.a = -2;


			/* Q-TEXT */

	gNewObjectDefinition.type 		= WIN_SObjType_QText;
	gNewObjectDefinition.flags 		= 0;
	text = MakeSpriteObject(&gNewObjectDefinition);

	text->Scale = scale2;
	text->SpecialF[0] = text->ColorFilter.a = -2;




		/*************************/
		/* SHOW IN ANIMATED LOOP */
		/*************************/

	timer = 30.0f;

	while((timer-=gFramesPerSecondFrac) > 0.0f)
	{
		UpdateInput();
		if (UserWantsOut())
			break;

			/* DRAW STUFF */

		CalcFramesPerSecond();
		MoveObjects();
		OGL_DrawScene(DrawWinCallback);
	}

}

/****************** MOVE THE END PANE *********************/

static void MoveTheEndPane(ObjNode *theNode)
{
	theNode->ColorFilter.a += gFramesPerSecondFrac * .6f;
	if (theNode->ColorFilter.a > .7f)
		theNode->ColorFilter.a = .7f;

	theNode->Coord.x = -g2DLogicalWidth*.5f;
	theNode->Coord.y = -g2DLogicalHeight*.5f;
	theNode->Scale.x = g2DLogicalWidth;
	theNode->Scale.y = g2DLogicalHeight;
	UpdateObjectTransforms(theNode);
}


/*************** MOVE THE END TEXT ***********************/

static void MoveTheEndText(ObjNode *theNode)
{
	theNode->SpecialF[0] += gFramesPerSecondFrac * .6f;
	if (theNode->SpecialF[0] > 1.0f)
		theNode->SpecialF[0] = 1.0f;

	if (theNode->StatusBits & STATUS_BIT_GLOW)
		theNode->ColorFilter.a = theNode->SpecialF[0] * .7f + RandomFloat()*.29f;
	else
		theNode->ColorFilter.a = theNode->SpecialF[0];
}


/********************* SETUP WIN SCREEN **********************/

static void SetupWinScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
ObjNode			*newObj;
static const OGLVector3D	fillDirection1 = { -1, -.2, -.6 };
int				i;
static const short	skels[8] =
{
	SKELETON_TYPE_FARMER,
	SKELETON_TYPE_BEEWOMAN,
	SKELETON_TYPE_SCIENTIST,
	SKELETON_TYPE_SKIRTLADY,
	SKELETON_TYPE_FARMER,
	SKELETON_TYPE_BEEWOMAN,
	SKELETON_TYPE_SCIENTIST,
	SKELETON_TYPE_SKIRTLADY,
};

static const Byte anim[8] =
{
	5,
	5,
	4,
	4,
	5,
	5,
	4,
	4
};

static const OGLPoint3D	humanPt[] =
{
	{1000,	0,	-400},
	{1100,	0,	-500},
	{900,	0,	-300},
	{950,	0,	-500},
	{1150,	0,	-700},
	{1200,	0,	-550},
	{1050,	0,	-450},
	{900,	0,	-200},
};

	gTabloid = nil;

	KillSong();

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.view.clearColor.r 		= .1;
	viewDef.view.clearColor.g 		= .5;
	viewDef.view.clearColor.b		= .1;

	viewDef.camera.fov 			= 1.1;
	viewDef.camera.hither 		= 100;
	viewDef.camera.yon 			= 6000;

	viewDef.camera.from.x		= 1200;
	viewDef.camera.from.z		= 150;
	viewDef.camera.from.y		= 250;

	viewDef.camera.to.x 		= 700.0f;
	viewDef.camera.to.y 		= 150.0f;
	viewDef.camera.to.z 		= -1000;

	viewDef.lights.fillDirection[0] = fillDirection1;

	OGL_SetupWindow(&viewDef);


				/************/
				/* LOAD ART */
				/************/

	InitSparkles();


			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:winscreen.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_WINSCREEN);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_WINSCREEN, WIN_ObjType_Rocket,
								 -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);


	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL);


			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:particle.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_PARTICLES);
	BlendAllSpritesInGroup(SPRITE_GROUP_PARTICLES);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:spheremap.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:lose.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_LOSE);


			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_OTTO);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_OTTO,
								 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

	LoadASkeleton(SKELETON_TYPE_FARMER);
	LoadASkeleton(SKELETON_TYPE_BEEWOMAN);
	LoadASkeleton(SKELETON_TYPE_SCIENTIST);
	LoadASkeleton(SKELETON_TYPE_SKIRTLADY);




		/****************/
		/* BUILD MODELS */
		/****************/

				/* BACKGROUND */

	gNewObjectDefinition.group 		= MODEL_GROUP_WINSCREEN;
	gNewObjectDefinition.type 		= WIN_ObjType_Background;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 50;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 3;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


				/* ROCKET */


	gNewObjectDefinition.group 		= MODEL_GROUP_WINSCREEN;
	gNewObjectDefinition.type 		= WIN_ObjType_Rocket;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= -800;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = .35;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


				/* CYC */

	gNewObjectDefinition.group 		= MODEL_GROUP_WINSCREEN;
	gNewObjectDefinition.type 		= WIN_ObjType_Cyc;
	gNewObjectDefinition.coord	 	= viewDef.camera.from;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG | STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 		= 2;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = viewDef.camera.yon / 100.0f * .98f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


		/* HUMANS */

	for (i = 0; i < 8; i++)
	{
		gNewObjectDefinition.type 		= skels[i];
		gNewObjectDefinition.animNum	= anim[i];
		gNewObjectDefinition.coord	 	= humanPt[i];
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= 205;
		gNewObjectDefinition.moveCall	= nil;
		gNewObjectDefinition.rot 		= PI/3 + RandomFloat2() * .2f;
		gNewObjectDefinition.scale 		= 1.0;
		newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

		newObj->Coord.y 	-= newObj->BBox.min.y;
		UpdateObjectTransforms(newObj);

		SetSkeletonAnimTime(newObj->Skeleton, RandomFloat());	// set random time index so all of these are not in sync
	}


			/* OTTO */

	gNewObjectDefinition.type 		= SKELETON_TYPE_OTTO;
	gNewObjectDefinition.animNum	= 0;
	gNewObjectDefinition.coord.x	= 500;
	gNewObjectDefinition.coord.y	= 0;
	gNewObjectDefinition.coord.z	= -400;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 205;
	gNewObjectDefinition.moveCall	= nil;
	gNewObjectDefinition.rot 		= -PI/2;
	gNewObjectDefinition.scale 		= 1.0;
	gPlayerInfo.objNode = newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

	newObj->Coord.y 	-= newObj->BBox.min.y;
	UpdateObjectTransforms(newObj);

	CreatePlayerSparkles(newObj);


			/* LEFT HAND */

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_OttoLeftHand;
	gNewObjectDefinition.slot		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	gPlayerInfo.leftHandObj 		= MakeNewDisplayGroupObject(&gNewObjectDefinition);



			/* RIGHT HAND */

	gNewObjectDefinition.type 	= GLOBAL_ObjType_OttoRightHand;
	gPlayerInfo.rightHandObj 	= MakeNewDisplayGroupObject(&gNewObjectDefinition);
	gPlayerInfo.rightHandObj->Kind = WEAPON_TYPE_FIST;			// set weapon type since this gets passed to weapon handlers



	PlaySong(SONG_WIN, kPlaySong_PlayOnceFlag);

}


/********************** FREE WIN SCREEN **********************/

static void FreeWinScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
//	DisposeSoundBank(SOUND_BANK_WIN);
	OGL_DisposeWindowSetup();
	Pomme_FlushPtrTracking(true);
}


/***************** DRAW WIN CALLBACK *******************/

static void DrawWinCallback(void)
{
	UpdateRobotHands(gPlayerInfo.objNode);
	UpdatePlayerSparkles(gPlayerInfo.objNode);

	DrawObjects();
}



#pragma mark -


/*********************** START TABLOID *************************/

static void StartTabloid(void)
{
ObjNode	*newObj;


	gNewObjectDefinition.group 		= MODEL_GROUP_WINSCREEN;
	gNewObjectDefinition.type 		= WIN_ObjType_Tabloid;
	gNewObjectDefinition.coord.x 	= 0;				// centered
	gNewObjectDefinition.coord.y 	= 0;				// centered
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES | STATUS_BIT_NOLIGHTING | STATUS_BIT_NOZBUFFER | STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveTabloid;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = .01;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->ColorFilter.a = 0;

	newObj->CustomDrawFunction = DrawTabloid;


	gTabloid = newObj;
}


/********************* MOVE TABLOID ***********************/

static void MoveTabloid(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	s;

	theNode->ColorFilter.a += fps;							// fade in
	if (theNode->ColorFilter.a > 1.0f)
		theNode->ColorFilter.a = 1.0f;

	s = theNode->Scale.x;									// zoom
	if (s < 1.0f)
	{
		s += fps * .6f;
		theNode->Scale.x =
		theNode->Scale.y =
		theNode->Scale.z = s;

		theNode->Rot.z += fps * 45.0f;							// spin
	}
	else
	{
		theNode->Rot.z = PI - (PI/30);


	}



	UpdateObjectTransforms(theNode);
}


/***************** DRAW TABLOID *************************/

static void DrawTabloid(ObjNode *theNode)
{
	OGL_PushState();

	SetInfobarSpriteState(true);

	MO_DrawObject(theNode->BaseGroup);

	OGL_PopState();
}

















