/****************************/
/*   	MAINMENU SCREEN.C	*/
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupMainMenuScreen(void);
static void FreeMainMenuScreen(void);
static Boolean DoMainMenuControl(void);
static void MoveSaturn(ObjNode *theNode);
static void MoveStar(ObjNode *theNode);
static void SpawnSaucer(void);
static void MoveMenuSaucer(ObjNode *theNode);
static void MoveOttoLogo(ObjNode *logo);
static void StartCompanyLogos(void);
static void StartGameLogo(void);
static void MoveCompanyLogo(ObjNode *glow);
static void MoveMenuIcon(ObjNode *theNode);
static void CalcIconMatrix(ObjNode *theNode);
static void MakeOttoIcon(void);
static void MakeHelpIcon(void);
static void MakeOptionsIcon(void);
static void MakeHighScoresIcon(void);
static void DoCredits(void);
static void DoHighScores(void);
static void DoHelp(void);
static void MakeCreditsIcon(void);
static void MakeSavedGameIcon(void);
static void MakeExitIcon(void);
static void DrawOttoLogo(ObjNode* theNode);
static void MakeIconString(void);
static void MoveIconString(ObjNode *theNode);



/****************************/
/*    CONSTANTS             */
/****************************/


enum
{
	MAINMENU_ObjType_SaturnSphere,
	MAINMENU_ObjType_SaturnRing,
	MAINMENU_ObjType_SaturnGlow,
	MAINMENU_ObjType_RingGlow,
	MAINMENU_ObjType_HelpIcon,
	MAINMENU_ObjType_OptionsIcon,
	MAINMENU_ObjType_HighScoresIcon,
	MAINMENU_ObjType_CreditsIcon,
	MAINMENU_ObjType_SavedGameIcon,
	MAINMENU_ObjType_ExitIcon,

	MAINMENU_ObjType_Star,
	MAINMENU_ObjType_Saucer,
	MAINMENU_ObjType_Logo,
	MAINMENU_ObjType_LogoGlow,
	MAINMENU_ObjType_LogoNoPlanet,

	MAINMENU_ObjType_PangeaLogo,
	MAINMENU_ObjType_PangeaLogoGlow,
	MAINMENU_ObjType_AspyrLogo,
	MAINMENU_ObjType_AspyrLogoGlow,

	MAINMENU_ObjType_CreditsText,
	MAINMENU_ObjType_CreditsGlow,
	MAINMENU_ObjType_HelpText,
	MAINMENU_ObjType_HelpGlow
};


enum
{
	SELECT_PLAY,
	SELECT_LOADSAVEDGAME,
	SELECT_HELP,
	SELECT_QUIT,
	SELECT_CREDITS,
	SELECT_HIGHSCORES,
	SELECT_OPTIONS,

	NUM_SELECTIONS
};

#define	ICON_PLACEMENT_RADIUS	120.0f

#define	DO_ASPYR		0

#define	SAUCER_LIFE				8.0f
#define	SAUCER_FADE				1.0f

#define	SpecialSaucerLife		SpecialF[0]

/*********************/
/*    VARIABLES      */
/*********************/


static ObjNode *gSaturn,*gLogoObj;

static int		gSelection;
static float 	gTargetRot;

static float	gSaucerDelay;

static	short	gLogoAmbience;
static	float	gAmbienceVolume = FULL_CHANNEL_VOLUME;

static	Boolean	gCanSpawnSaucers = false;

static	Boolean	gDoCompanyLogos = true;

static	float	gMenuLogoFadeAlpha;

static 	Boolean	gFadeInText,gTextDone,gFadeInIconString,gHideIconString;


/********************** DO MAINMENU SCREEN ***********************/

void DoMainMenuScreen(void)
{
			/* SETUP */

	SetupMainMenuScreen();
	MakeFadeEvent(true, 1.0);

				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	UpdateInput();


	while(true)
	{
		/* CHECK USER CONTROL */

		UpdateInput();
		if (DoMainMenuControl())					// returns true if want to start game
			break;


			/* DRAW STUFF */

		CalcFramesPerSecond();
		MoveObjects();
		OGL_DrawScene(DrawObjects);

			/* SEE IF SPAWN SAUCER */

		if (gCanSpawnSaucers)
		{
			gSaucerDelay -= gFramesPerSecondFrac;
			if (gSaucerDelay <= 0.0f)
			{
				gSaucerDelay = 3.0f + RandomFloat() * 3.0f;
				SpawnSaucer();
			}
		}
	}


			/* CLEANUP */

	OGL_FadeOutScene(DrawObjects, NULL);
	FreeMainMenuScreen();

	gDoCompanyLogos = false;						// dont do these again
}


/****************** DRAW OTTO LOGO ***********************/

static void DrawOttoLogo(ObjNode* theNode)
{
	(void) theNode;

			/* SET STATE */

	OGL_PushState();

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	OGL_DisableLighting();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, g2DLogicalWidth, g2DLogicalHeight, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(g2DLogicalWidth-130,50,0);
	glScalef(.25,-.25,.25);

			/* DRAW IT */

	gGlobalTransparency = (.7f + RandomFloat() * .3f) * gMenuLogoFadeAlpha;
	if (gGlobalTransparency > 0.0f)
	{
		gGlobalTransparency = gMenuLogoFadeAlpha;
		MO_DrawObject(gBG3DGroupList[MODEL_GROUP_MAINMENU][MAINMENU_ObjType_LogoNoPlanet]);
	}

	gGlobalTransparency = 1;
	OGL_PopState();
}


/********************* SETUP MAINMENU SCREEN **********************/

static void SetupMainMenuScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
static OGLColorRGBA			ambientColor = { .03, .03, .03, 1 };
short				i;
ObjNode				*newObj;
static OGLVector3D			fillDirection1 = { -1, 0, -1 };

	OGLVector3D_Normalize(&fillDirection1, &fillDirection1);

	gSelection = SELECT_PLAY;
	gTargetRot = 0;
	gSaucerDelay = 0;
	gSaturn = nil;
	gLogoAmbience = -1;
	gMenuLogoFadeAlpha = -1;
	gHideIconString = false;

			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.view.clearBackBuffer = true;

	viewDef.camera.fov 			= .9;
	viewDef.camera.hither 		= 30;
	viewDef.camera.yon 			= 6000;
	viewDef.camera.from.z		= 800;
	viewDef.camera.from.y		= 0;

	viewDef.lights.ambientColor 	= ambientColor;
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

		gAnaglyphFocallength	= 300.0f;
		gAnaglyphEyeSeparation 	= 30.0f;
	}


	OGL_SetupWindow(&viewDef);


				/************/
				/* LOAD ART */
				/************/

			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:mainmenu.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_MAINMENU);

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_MAINMENU, MAINMENU_ObjType_HelpIcon,
							 -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_MAINMENU, MAINMENU_ObjType_OptionsIcon,
							 -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_MAINMENU, MAINMENU_ObjType_HighScoresIcon,
							 -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_MAINMENU, MAINMENU_ObjType_CreditsIcon,
							 -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);


	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL);


			/* LOAD SPRITES */

	LoadSpriteGroup(SPRITE_GROUP_SPHEREMAPS, SPHEREMAP_SObjType_COUNT, "spheremap");
	

			/* INIT EFFECT SYSTEMS */
	
	InitEffects();


			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_OTTO);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_OTTO,
								 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

	LoadASkeleton(SKELETON_TYPE_FARMER);
	LoadASkeleton(SKELETON_TYPE_BEEWOMAN);
	LoadASkeleton(SKELETON_TYPE_CLOWN);
	LoadASkeleton(SKELETON_TYPE_MANTIS);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_MANTIS,
								 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

	LoadASkeleton(SKELETON_TYPE_BRAINALIEN);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_BRAINALIEN,
								 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);



				/* LOAD AUDIO */

	LoadSoundBank(SOUNDBANK_MENU);

	PlaySong(SONG_THEME, kPlaySong_PreloadFlag);		// preload the song to avoid hiccup when starting playback later



			/**************/
			/* MAKE STARS */
			/**************/

	for (i = 0; i < 60*2; i++)
	{
		static const OGLColorRGBA colors[] =
		{
			{1.0f, 1.0f, 1.0f, 1.0f},			// white
			{1.0f, 0.6f, 0.6f, 1.0f},			// red
			{0.6f, 0.6f, 1.0f, 1.0f},			// blue
			{0.7f, 0.7f, 1.0f, 1.0f},			// grey
		};

		gNewObjectDefinition.group 		= MODEL_GROUP_MAINMENU;
		gNewObjectDefinition.type 		= MAINMENU_ObjType_Star;
		gNewObjectDefinition.coord.x 	= RandomFloat2() * 800.0f * 2;
		gNewObjectDefinition.coord.y 	= RandomFloat2() * 600.0f;
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

		newObj->Delta.x = 5.0f + RandomFloat() * 15.0f;
	}


			/* MAKE CORNER LOGO */

	NewObjectDefinitionType cornerLogoDef =
	{
		.genre = CUSTOM_GENRE,
		.slot = DRAWEXTRA_SLOT,
		.scale = 1,
		.flags = STATUS_BIT_DONTCULL,
		.drawCall = DrawOttoLogo
	};
	MakeNewObject(&cornerLogoDef);


				/* START W/ GAME LOGO */

	if (gDoCompanyLogos && !gCommandLine.skipFluff)
		StartCompanyLogos();
	else
		StartGameLogo();
}


/********************* START COMPANY LOGOS ******************************/

static void StartCompanyLogos(void)
{
ObjNode	*pangea;

			/* PANGEA LOGO GLOW */

	gNewObjectDefinition.group 		= MODEL_GROUP_MAINMENU;
	gNewObjectDefinition.type 		= MAINMENU_ObjType_PangeaLogoGlow;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= -100;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOTEXTUREWRAP|STATUS_BIT_NOLIGHTING|
									STATUS_BIT_GLOW|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOZWRITES;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB + 300;
	gNewObjectDefinition.moveCall 	= MoveCompanyLogo;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1.0;
	gLogoObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	gLogoObj->ColorFilter.a = .99;


			/* PANGEA LOGO */

	gNewObjectDefinition.type 		= MAINMENU_ObjType_PangeaLogo;
	gNewObjectDefinition.coord.z 	+= 1.0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOTEXTUREWRAP|
										STATUS_BIT_NOLIGHTING|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOZWRITES;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.slot++;
	pangea = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	gLogoObj->ChainNode = pangea;





	gLogoAmbience = PlayEffect(EFFECT_LOGOAMBIENCE);
	gAmbienceVolume = FULL_CHANNEL_VOLUME;

	gCanSpawnSaucers = false;
}


/********************* START GAME LOGO ******************************/

static void StartGameLogo(void)
{
ObjNode	*newObj2;

			/**************/
			/* MAKE TITLE */
			/**************/

			/* LOGO GLOW */

	gNewObjectDefinition.group 		= MODEL_GROUP_MAINMENU;
	gNewObjectDefinition.type 		= MAINMENU_ObjType_LogoGlow;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= -200;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOTEXTUREWRAP|STATUS_BIT_NOLIGHTING|STATUS_BIT_GLOW|STATUS_BIT_NOZWRITES;
	gNewObjectDefinition.slot 		= 400;
	gNewObjectDefinition.moveCall 	= MoveOttoLogo;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = .13;
	gLogoObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	gLogoObj->ColorFilter.a = 0;


			/* LOGO */

	gNewObjectDefinition.type 		= MAINMENU_ObjType_Logo;
	gNewObjectDefinition.coord.z 	-= 1.0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOTEXTUREWRAP|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOZWRITES;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	newObj2 = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj2->ColorFilter.a = 0;

	gLogoObj->ChainNode = newObj2;


	PlaySong(SONG_THEME, 0);

	gCanSpawnSaucers = true;
}


/********************** FREE MAINMENU ART **********************/

static void FreeMainMenuScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeSoundBank(SOUNDBANK_MENU);
	DisposeEffects();
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	OGL_DisposeWindowSetup();
	Pomme_FlushPtrTracking(true);
}

#pragma mark -


/************************* BUILD MENU *****************************/

static void BuildMenu(void)
{
ObjNode	*ring,*glow,*ringGlow;

	PlaySong(SONG_THEME, 0);		// if theme not already going, then do it (no-op otherwise)

	gCanSpawnSaucers = true;

			/****************/
			/* SATURN MODEL */
			/****************/

				/* SPHERE */

	gNewObjectDefinition.group 		= MODEL_GROUP_MAINMENU;
	gNewObjectDefinition.type 		= MAINMENU_ObjType_SaturnSphere;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_ROTYZX | STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 5;
	gNewObjectDefinition.moveCall 	= MoveSaturn;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 2.5;
	gSaturn = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	gSaturn->ColorFilter.a = 0;

			/* RING */

	gNewObjectDefinition.type 		= MAINMENU_ObjType_SaturnRing;
	gNewObjectDefinition.flags 		|= STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.coord.z 	-= 200.0f;
	gNewObjectDefinition.moveCall 	= nil;
	ring = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	gSaturn->ChainNode = ring;


			/* GLOW */

	gNewObjectDefinition.type 		= MAINMENU_ObjType_SaturnGlow;
	gNewObjectDefinition.flags 		|= STATUS_BIT_GLOW | STATUS_BIT_NOZWRITES;
	gNewObjectDefinition.slot 		= 350;
	gNewObjectDefinition.scale 	    *= 1.3f;
	glow = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	glow->ColorFilter.a = 0;

	ring->ChainNode = glow;

			/* RING GLOW */

	gNewObjectDefinition.type 		= MAINMENU_ObjType_RingGlow;
	gNewObjectDefinition.slot		= ring->Slot-1;
	ringGlow = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	ringGlow->ColorFilter.a = 0;

	glow->ChainNode = ringGlow;


		/*****************/
		/* ICONS ON RING */
		/*****************/

	MakeOttoIcon();
	MakeHelpIcon();
	MakeOptionsIcon();
	MakeHighScoresIcon();
	MakeCreditsIcon();
	MakeSavedGameIcon();
	MakeExitIcon();


			/* STRING */

	MakeIconString();
}

/******************** MAKE ICON STRING ********************/

static void MakeIconString(void)
{
ObjNode	*newObj;

			/* BUILD NEW TEXT STRINGS */

	gNewObjectDefinition.coord		= (OGLPoint3D) {0, 180, 0};
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= MoveIconString;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 1;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	newObj = TextMesh_New(Localize(STR_NEW_GAME + gSelection), 1, &gNewObjectDefinition);		// title
	newObj->ColorFilter.a = 0;

	gFadeInIconString = true;
}

/*************** MOVE ICON STRING ********************/

static void MoveIconString(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	if (gSaturn->ColorFilter.a < 1.0f)					// nothing until saturn is ready
		return;


		/* FADE IN */

	if (gFadeInIconString)
	{
		theNode->ColorFilter.a += fps * 1.8f;
		if (theNode->ColorFilter.a > 1.0f)
			theNode->ColorFilter.a = 1.0f;
	}
	else
	{
		theNode->ColorFilter.a -= fps * 1.8f;
		if (theNode->ColorFilter.a <= 0.0f)
		{
			DeleteObject(theNode);
			MakeIconString();
		}
	}

	if (gHideIconString)
	{
		theNode->StatusBits |= STATUS_BIT_HIDDEN;
	}
	else
	{
		if (theNode->StatusBits & STATUS_BIT_HIDDEN)	// if going from hidden to not, then reset alpha so will fade in
		{
			//theNode->StatusBits &= ~STATUS_BIT_HIDDEN;
			//theNode->ColorFilter.a = 0;

			// Recreate the object so it appears in the correct language after backing out from the settings screen
			DeleteObject(theNode);
			MakeIconString();
		}
	}

}



/************** MAKE OTTO ICON **************************/

static void MakeOttoIcon(void)
{
float	r;
short	i;
ObjNode	*icon;

		/* PUT OTTO ON THERE */

	gNewObjectDefinition.type 		= SKELETON_TYPE_OTTO;
	gNewObjectDefinition.animNum	= PLAYER_ANIM_SITONLEDGE;
	r = PI - (SELECT_PLAY / NUM_SELECTIONS) * PI2;							// calc relative rot of icon
	gNewObjectDefinition.coord.x 	= -sin(r) * (ICON_PLACEMENT_RADIUS + 18);
	gNewObjectDefinition.coord.z 	= -cos(r) * (ICON_PLACEMENT_RADIUS + 18);
	gNewObjectDefinition.coord.y 	= 41;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= PLAYER_SLOT;
	gNewObjectDefinition.moveCall	= MoveMenuIcon;
	gNewObjectDefinition.rot 		= r;
	gNewObjectDefinition.scale 		= .8;
	icon = MakeNewSkeletonObject(&gNewObjectDefinition);

	icon->ColorFilter.a = 0;
	icon->Kind = SELECT_PLAY;

	CalcIconMatrix(icon);

				/* TORSO SPARKLE */

	i = icon->Sparkles[0] = GetFreeSparkle(icon);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL | SPARKLE_FLAG_TRANSFORMWITHOWNER;
		gSparkles[i].where.x = 0;
		gSparkles[i].where.y = -95;
		gSparkles[i].where.z = 15;

		gSparkles[i].color.r = .7;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = .3;
		gSparkles[i].color.a = 0;

		gSparkles[i].scale = 80.0f;
		gSparkles[i].separation = 30.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark4;
	}


			/* AND HIS HANDS */

	gPlayerInfo.currentWeaponType = WEAPON_TYPE_FIST;

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_OttoLeftHand;
	gNewObjectDefinition.slot		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	gPlayerInfo.leftHandObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);



			/* RIGHT HAND */

	gNewObjectDefinition.type 		= GLOBAL_ObjType_OttoRightHand;
	gPlayerInfo.rightHandObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	gPlayerInfo.rightHandObj->Kind = WEAPON_TYPE_FIST;			// set weapon type since this gets passed to weapon handlers

	UpdateRobotHands(icon);

}



/************** MAKE HELP ICON **************************/

static void MakeHelpIcon(void)
{
float	r;
ObjNode	*icon;

		/* PUT BEEHIVE LADY ON THERE */

	gNewObjectDefinition.type 		= SKELETON_TYPE_BEEWOMAN;
	gNewObjectDefinition.animNum	= 4;
	r = PI - ((float)SELECT_HELP / (float)NUM_SELECTIONS) * PI2;							// calc relative rot of icon
	gNewObjectDefinition.coord.x 	= -sin(r) * (ICON_PLACEMENT_RADIUS + 12.0);
	gNewObjectDefinition.coord.z 	= -cos(r) * (ICON_PLACEMENT_RADIUS + 12.0);
	gNewObjectDefinition.coord.y 	= 25;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall	= MoveMenuIcon;
	gNewObjectDefinition.rot 		= r;
	gNewObjectDefinition.scale 		= 1.0;
	icon = MakeNewSkeletonObject(&gNewObjectDefinition);

	icon->ColorFilter.a = 0;
	icon->Kind = SELECT_HELP;

	CalcIconMatrix(icon);


		/* ALSO PUT THE HELP ICON THERE */

	gNewObjectDefinition.group 		= MODEL_GROUP_MAINMENU;
	gNewObjectDefinition.type 		= MAINMENU_ObjType_HelpIcon;
	gNewObjectDefinition.coord.x 	= -sin(r) * (ICON_PLACEMENT_RADIUS + 25.0f);
	gNewObjectDefinition.coord.z 	= -cos(r) * (ICON_PLACEMENT_RADIUS + 25.0f);
	gNewObjectDefinition.coord.y 	= 25;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL ;
	gNewObjectDefinition.moveCall 	= MoveMenuIcon;
	gNewObjectDefinition.rot 		= r + PI;
	gNewObjectDefinition.scale 		= 1.8;
	icon = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	icon->ColorFilter.a = 0;
	icon->Kind = SELECT_HELP;

	CalcIconMatrix(icon);

}

/************** MAKE OPTIONS ICON **************************/

static void MakeOptionsIcon(void)
{
float	r;
ObjNode	*icon;

		/* PUT BEEHIVE LADY ON THERE */

	gNewObjectDefinition.type 		= SKELETON_TYPE_BRAINALIEN;
	gNewObjectDefinition.animNum	= 7;
	r = PI - ((float)SELECT_OPTIONS / (float)NUM_SELECTIONS) * PI2;							// calc relative rot of icon
	gNewObjectDefinition.coord.x 	= -sin(r + .1f) * (ICON_PLACEMENT_RADIUS + 15);
	gNewObjectDefinition.coord.z 	= -cos(r + .1f) * (ICON_PLACEMENT_RADIUS + 15);
	gNewObjectDefinition.coord.y 	= 29;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall	= MoveMenuIcon;
	gNewObjectDefinition.rot 		= r;
	gNewObjectDefinition.scale 		= 1.0;
	icon = MakeNewSkeletonObject(&gNewObjectDefinition);

	icon->ColorFilter.a = 0;
	icon->Kind = SELECT_OPTIONS;

	CalcIconMatrix(icon);

//	CreateBrainAlienGlow(icon);


		/* ALSO PUT THE OPTIONS ICON THERE */

	r -= .13f;
	gNewObjectDefinition.group 		= MODEL_GROUP_MAINMENU;
	gNewObjectDefinition.type 		= MAINMENU_ObjType_OptionsIcon;
	gNewObjectDefinition.coord.x 	= -sin(r) * (ICON_PLACEMENT_RADIUS + 16);
	gNewObjectDefinition.coord.z 	= -cos(r) * (ICON_PLACEMENT_RADIUS + 16);
	gNewObjectDefinition.coord.y 	= 20;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.moveCall 	= MoveMenuIcon;
	gNewObjectDefinition.rot 		= r + PI;
	gNewObjectDefinition.scale 		= 1.9;
	icon = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	icon->ColorFilter.a = 0;
	icon->Kind = SELECT_OPTIONS;

	CalcIconMatrix(icon);

}

/************** MAKE HIGH SCORES ICON **************************/

static void MakeHighScoresIcon(void)
{
float	r;
ObjNode	*icon;

		/* PUT MANTIS ON THERE */

	gNewObjectDefinition.type 		= SKELETON_TYPE_MANTIS;
	gNewObjectDefinition.animNum	= 5;
	r = PI - ((float)SELECT_HIGHSCORES / (float)NUM_SELECTIONS) * PI2;							// calc relative rot of icon
	gNewObjectDefinition.coord.x 	= -sin(r) * (ICON_PLACEMENT_RADIUS + 5);
	gNewObjectDefinition.coord.z 	= -cos(r) * (ICON_PLACEMENT_RADIUS + 5);
	gNewObjectDefinition.coord.y 	= 43;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall	= MoveMenuIcon;
	gNewObjectDefinition.rot 		= r;
	gNewObjectDefinition.scale 		= .6;
	icon = MakeNewSkeletonObject(&gNewObjectDefinition);

	icon->ColorFilter.a = 0;
	icon->Kind = SELECT_HIGHSCORES;

	CalcIconMatrix(icon);


		/* ALSO PUT THE HIGHSCORES ICON THERE */

	gNewObjectDefinition.group 		= MODEL_GROUP_MAINMENU;
	gNewObjectDefinition.type 		= MAINMENU_ObjType_HighScoresIcon;
	gNewObjectDefinition.coord.x 	= -sin(r) * (ICON_PLACEMENT_RADIUS + 15.0f);
	gNewObjectDefinition.coord.z 	= -cos(r) * (ICON_PLACEMENT_RADIUS + 15.0f);
	gNewObjectDefinition.coord.y 	= 25;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL ;
	gNewObjectDefinition.moveCall 	= MoveMenuIcon;
	gNewObjectDefinition.rot 		= r + PI;
	gNewObjectDefinition.scale 		= .55;
	icon = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	icon->ColorFilter.a = 0;
	icon->Kind = SELECT_HIGHSCORES;

	CalcIconMatrix(icon);

}

/************** MAKE CREDITS ICON **************************/

static void MakeCreditsIcon(void)
{
float	r;
ObjNode	*icon;

	r = PI - ((float)SELECT_CREDITS / (float)NUM_SELECTIONS) * PI2;							// calc relative rot of icon


		/* PUT SKELETON ON THERE */

	gNewObjectDefinition.type 		= SKELETON_TYPE_CLOWN;
	gNewObjectDefinition.animNum	= 4;
	gNewObjectDefinition.coord.x 	= -sin(r-.2f) * (ICON_PLACEMENT_RADIUS + 11);
	gNewObjectDefinition.coord.z 	= -cos(r-.2f) * (ICON_PLACEMENT_RADIUS + 11);
	gNewObjectDefinition.coord.y 	= 45;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall	= MoveMenuIcon;
	gNewObjectDefinition.rot 		= r;
	gNewObjectDefinition.scale 		= 1.0;
	icon = MakeNewSkeletonObject(&gNewObjectDefinition);

	icon->ColorFilter.a = 0;
	icon->Kind = SELECT_CREDITS;

	CalcIconMatrix(icon);


		/* ALSO PUT THE CREDITS ICON THERE */

	gNewObjectDefinition.group 		= MODEL_GROUP_MAINMENU;
	gNewObjectDefinition.type 		= MAINMENU_ObjType_CreditsIcon;
	gNewObjectDefinition.coord.x 	= -sin(r) * ICON_PLACEMENT_RADIUS;
	gNewObjectDefinition.coord.z 	= -cos(r) * ICON_PLACEMENT_RADIUS;
	gNewObjectDefinition.coord.y 	= 20;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL ;
	gNewObjectDefinition.moveCall 	= MoveMenuIcon;
	gNewObjectDefinition.rot 		= r + PI;
	gNewObjectDefinition.scale 		= .7;
	icon = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	icon->ColorFilter.a = 0;
	icon->Kind = SELECT_CREDITS;

	CalcIconMatrix(icon);

}



/************** MAKE SAVED GAME ICON **************************/

static void MakeSavedGameIcon(void)
{
float	r;
ObjNode	*icon;

	r = PI - ((float)SELECT_LOADSAVEDGAME / (float)NUM_SELECTIONS) * PI2;							// calc relative rot of icon

		/* PUT FARMER ON THERE */

	gNewObjectDefinition.type 		= SKELETON_TYPE_FARMER;
	gNewObjectDefinition.animNum	= 4;
	gNewObjectDefinition.coord.x 	= -sin(r-.13f) * ICON_PLACEMENT_RADIUS;
	gNewObjectDefinition.coord.z 	= -cos(r-.13f) * ICON_PLACEMENT_RADIUS;
	gNewObjectDefinition.coord.y 	= 32;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall	= MoveMenuIcon;
	gNewObjectDefinition.rot 		= r + PI/2;
	gNewObjectDefinition.scale 		= 1.15;
	icon = MakeNewSkeletonObject(&gNewObjectDefinition);

	icon->ColorFilter.a = 0;
	icon->Kind = SELECT_LOADSAVEDGAME;

	CalcIconMatrix(icon);

		/* ALSO PUT THE ICON THERE */

	gNewObjectDefinition.group 		= MODEL_GROUP_MAINMENU;
	gNewObjectDefinition.type 		= MAINMENU_ObjType_SavedGameIcon;
	gNewObjectDefinition.coord.x 	= -sin(r+.1f) * ICON_PLACEMENT_RADIUS;
	gNewObjectDefinition.coord.z 	= -cos(r+.1f) * ICON_PLACEMENT_RADIUS;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL;
	gNewObjectDefinition.moveCall 	= MoveMenuIcon;
	gNewObjectDefinition.rot 		= r + PI;
	gNewObjectDefinition.scale 		= .6;
	icon = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	icon->ColorFilter.a = 0;
	icon->Kind = SELECT_LOADSAVEDGAME;

	CalcIconMatrix(icon);

}



/************** MAKE EXIT ICON **************************/

static void MakeExitIcon(void)
{
float	r;
ObjNode	*icon;


	r = PI - ((float)SELECT_QUIT / (float)NUM_SELECTIONS) * PI2;							// calc relative rot of icon


	gNewObjectDefinition.group 		= MODEL_GROUP_MAINMENU;
	gNewObjectDefinition.type 		= MAINMENU_ObjType_ExitIcon;
	gNewObjectDefinition.coord.x 	= -sin(r) * (ICON_PLACEMENT_RADIUS + 15);
	gNewObjectDefinition.coord.z 	= -cos(r) * (ICON_PLACEMENT_RADIUS + 15);
	gNewObjectDefinition.coord.y 	= 32;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL | STATUS_BIT_KEEPBACKFACES |
									STATUS_BIT_GLOW | STATUS_BIT_NOZWRITES | STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.moveCall 	= MoveMenuIcon;
	gNewObjectDefinition.rot 		= r;
	gNewObjectDefinition.scale 		= .93;
	gNewObjectDefinition.slot 		= 800;
	icon = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	icon->ColorFilter.a = 0;
	icon->Kind = SELECT_QUIT;

	CalcIconMatrix(icon);

}



/********************** CALC ICON MATRIX *********************/

static void CalcIconMatrix(ObjNode *theNode)
{
OGLMatrix4x4	m,m2,m3;
float			s;

				/* SET SCALE MATRIX */

	s = theNode->Scale.x / gSaturn->Scale.x;						// to adjust from saturn scale to otto's
	OGLMatrix4x4_SetScale(&m, s, s, s);

			/* SET ROTATION MATRIX */

	OGLMatrix4x4_SetRotate_Y(&m2, theNode->Rot.y);
	OGLMatrix4x4_Multiply(&m, &m2, &m3);

			/* CALC LOCATION ON RIM */

//	r = PI + (theNode->Kind / NUM_SELECTIONS) * PI2;
	OGLMatrix4x4_SetTranslate(&m, theNode->Coord.x, theNode->Coord.y, theNode->Coord.z);
	OGLMatrix4x4_Multiply(&m3, &m, &m2);


			/* MULTIPLY MATRICES */

	OGLMatrix4x4_Multiply(&m2, &gSaturn->BaseTransformMatrix, &theNode->BaseTransformMatrix);
	SetObjectTransformMatrix(theNode);
}


/******************** MOVE MENU ICON ***************************/

static void MoveMenuIcon(ObjNode *theNode)
{
int	i;
OGLMatrix4x4	m;

			/* FADE IN */

	if (theNode->ColorFilter.a < 1.0f)
	{
		theNode->ColorFilter.a += gFramesPerSecondFrac;
		if (theNode->ColorFilter.a > 1.0f)
			theNode->ColorFilter.a = 1.0f;
	}

	CalcIconMatrix(theNode);

			/* UPDATE STUFF */

	switch(theNode->Kind)
	{
		case	SELECT_PLAY:
				UpdateRobotHands(theNode);

				i = theNode->Sparkles[0];								// fade in sparkle too
				if (i != -1)
					gSparkles[i].color.a = theNode->ColorFilter.a * .8f;
				break;

		case	SELECT_QUIT:
				if (theNode->ColorFilter.a >= 1.0f)
					theNode->ColorFilter.a = .99f;
				theNode->Rot.z -= gFramesPerSecondFrac * PI;
				OGLMatrix4x4_SetRotate_Z(&m, theNode->Rot.z);
				OGLMatrix4x4_Multiply(&m, &theNode->BaseTransformMatrix, &theNode->BaseTransformMatrix);
				SetObjectTransformMatrix(theNode);
				break;
	}
}



#pragma mark -

/****************** DO MAIN MENU CONTROL **********************/
//
// returns true if user wants to start playing
//

static Boolean DoMainMenuControl(void)
{
			/***************/
			/* HANDLE MENU */
			/***************/

	if (gSaturn)
	{
			/* SPIN LEFT */

		if (GetNewNeedState(kNeed_UILeft) || GetNewNeedState(kNeed_UINext))
		{
			PlayEffect(EFFECT_MENUCHANGE);
			gTargetRot -= PI2 / (float)NUM_SELECTIONS;
			gSelection--;
			if (gSelection < 0)
				gSelection = NUM_SELECTIONS-1;
			gFadeInIconString = false;
		}
				/* SPIN RIGHT */

		else
		if (GetNewNeedState(kNeed_UIRight) || GetNewNeedState(kNeed_UIPrev))
		{
			PlayEffect(EFFECT_MENUCHANGE);
			gTargetRot += PI2 / (float)NUM_SELECTIONS;
			gSelection++;
			if (gSelection == NUM_SELECTIONS)
				gSelection = 0;
			gFadeInIconString = false;
		}

				/* MAKE SELECTION */
		else
		if (GetNewNeedState(kNeed_UIConfirm) || GetNewNeedState(kNeed_UIStart))
		{
			gHideIconString = true;

			switch(gSelection)
			{
				case	SELECT_PLAY:
						gLevelNum = 0;							// start on Level 0 if not loading from saved game

						if (GetKeyState(SDL_SCANCODE_F10) ||	// see if do Level cheat
							(GetNeedState(kNeed_UIBack) && GetNeedState(kNeed_UIStart)))
						{
							int cheatLevel = DoLevelCheatDialog(DrawObjects);
							if (cheatLevel < 0)
								break;
							gLevelNum = cheatLevel;
						}
						else
						{
							for (int i = 0; i < 10; i++)		// see if do level cheat by holding down number key
							{
								if (GetKeyState(SDL_SCANCODE_1 + i))	// scancodes 1,2,...,9,0 are contiguous
									gLevelNum = i;
							}
						}

						gPlayingFromSavedGame = false;
						return(true);

				case	SELECT_HELP:
						DoHelp();
						break;

				case	SELECT_HIGHSCORES:
						DoHighScores();
						break;

				case	SELECT_OPTIONS:
						DoSettingsOverlay(nil, DrawObjects);
						break;

				case	SELECT_CREDITS:
						DoCredits();
						break;

				case	SELECT_LOADSAVEDGAME:
						if (DoFileScreen(FILE_SCREEN_TYPE_LOAD, DrawObjects))
						{
							gPlayingFromSavedGame = true;
							return true;
						}
						break;

				case	SELECT_QUIT:
						CleanQuit();
						break;
			}

			gHideIconString = false;
		}
	}
		/***************/
		/* ABORT INTRO */
		/***************/
	else
	{
		if (UserWantsOut())
		{
			DeleteObject(gLogoObj);
			StopAChannel(&gLogoAmbience);
			BuildMenu();
		}
	}

	return(false);
}



/************************* MOVE SATURN ***********************/

static void MoveSaturn(ObjNode *theNode)
{
float	rot,s;
float	fps = gFramesPerSecondFrac;
ObjNode	*ring = theNode->ChainNode;
ObjNode	*glow = ring->ChainNode;
ObjNode	*ringGlow = glow->ChainNode;
OGLMatrix4x4	m;

	gMenuLogoFadeAlpha += gFramesPerSecondFrac * .5f;
	if (gMenuLogoFadeAlpha > 1.0f)
		gMenuLogoFadeAlpha = 1.0;

			/* FADE IN */

	if (theNode->ColorFilter.a < 1.0f)
	{
		theNode->ColorFilter.a += fps;
		if (theNode->ColorFilter.a > 1.0f)
			theNode->ColorFilter.a = 1.0f;
	}


				/* UPDATE ROTATION */

	rot = theNode->Rot.y;													// get current y rot

	if (gTargetRot != rot)
	{
		if (rot < gTargetRot)
		{
			rot += fps * 2.0f;
			if (rot > gTargetRot)
				rot = gTargetRot;
		}
		else
		{
			rot -= fps * 2.0f;
			if (rot < gTargetRot)
				rot = gTargetRot;
		}

		theNode->Rot.y = rot;												// update it
	}


			/* WOBBLE ON Z */

	theNode->SpecialF[0] += fps * .5f;
	gSaturn->Rot.z = -PI/12.0f + sin(theNode->SpecialF[0]) * .1f;
	theNode->SpecialF[1] += fps * .3f;
	gSaturn->Rot.x = PI/11.0f + sin(theNode->SpecialF[1]) * .05f;

	UpdateObjectTransforms(theNode);


			/* UPDATE RING ALSO */

	ring->ColorFilter.a = theNode->ColorFilter.a;
	ring->BaseTransformMatrix = theNode->BaseTransformMatrix;
	SetObjectTransformMatrix(ring);


			/* UPDATE GLOW ALSO */

	glow->ColorFilter.a = theNode->ColorFilter.a * .99f;

	ringGlow->ColorFilter.a = theNode->ColorFilter.a * .6f;
	ringGlow->BaseTransformMatrix = theNode->BaseTransformMatrix;

	s = 1.3f + sin(ringGlow->SpecialF[0] += fps * 11.0f) * .005f;		// undulate a little
	OGLMatrix4x4_SetScale(&m, s,s,s);
	OGLMatrix4x4_Multiply(&m, &ringGlow->BaseTransformMatrix, &ringGlow->BaseTransformMatrix);
	SetObjectTransformMatrix(ringGlow);
}



/************************ MOVE STAR *************************/

static void MoveStar(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->SpecialF[0] += fps * 2.0f;

	theNode->ColorFilter.a = .3f + (sin(theNode->SpecialF[0]) + 1.0f) * .5f;

	theNode->Coord.x += theNode->Delta.x * fps;
	if (theNode->Coord.x > 1600.0f)
		theNode->Coord.x -= 2*1600.0f;

	UpdateObjectTransforms(theNode);
}



/************************ SPAWN SAUCER ****************************/

static void SpawnSaucer(void)
{
ObjNode	*newObj;
float	dx;

	gNewObjectDefinition.group 		= MODEL_GROUP_MAINMENU;
	gNewObjectDefinition.type 		= MAINMENU_ObjType_Saucer;

	if (MyRandomLong() & 0x1)
	{
		gNewObjectDefinition.coord.x 	= -1200;
		dx = 500.0f;
	}
	else
	{
		gNewObjectDefinition.coord.x 	= 1200;
		dx = -500.0f;
	}

	gNewObjectDefinition.coord.y 	= RandomFloat2()*700.0f;
	gNewObjectDefinition.coord.z 	= -100;
	gNewObjectDefinition.flags 		= STATUS_BIT_ROTYZX;
	gNewObjectDefinition.slot 		= 200;
	gNewObjectDefinition.moveCall 	= MoveMenuSaucer;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = .4;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->Delta.x = dx;

	newObj->SpecialSaucerLife = SAUCER_LIFE;
}


/********************** MOVE MENU SAUCER *****************************/

static void MoveMenuSaucer(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	float life = theNode->SpecialSaucerLife -= fps;

	if (life <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}

	if (life < SAUCER_FADE)								// fade out
		theNode->ColorFilter.a = life / SAUCER_FADE;
	else if (life > SAUCER_LIFE - SAUCER_FADE)			// fade in
		theNode->ColorFilter.a = 1.0f - (life - (SAUCER_LIFE - SAUCER_FADE)) / SAUCER_FADE;
	else
		theNode->ColorFilter.a = 1.0f;

	GetObjectInfo(theNode);

	gCoord.z -= fps * 200.0f;
	gCoord.x += fps * gDelta.x;

	theNode->Rot.y += fps * 3.0f;

	UpdateObject(theNode);
}


/********************** MOVE OTTO LOGO ****************************/

static void MoveOttoLogo(ObjNode *glow)
{
float fps = gFramesPerSecondFrac;
ObjNode	*logo = glow->ChainNode;


			/* UPDATE LOGO */

	logo->Coord.z += fps * 120.0f;							// move forward

	if (logo->Coord.z > 500.0f)
	{
		if (logo->ColorFilter.a == 1.0)						// see if just now starting fade
		{
			BuildMenu();									// build the menu
		}

		logo->ColorFilter.a -= fps * .8f;					// fade out
		if (logo->ColorFilter.a <= 0.0f)
		{
			DeleteObject(glow);
			return;
		}

	}
	else													// fade in
	{
		logo->ColorFilter.a += fps * .6f;
		if (logo->ColorFilter.a > 1.0f)
			logo->ColorFilter.a = 1.0f;

	}


	UpdateObjectTransforms(logo);


			/* UPDATE GLOW */

	glow->Coord.z = logo->Coord.z - 1.0f;
	glow->Scale.x =
	glow->Scale.y =
	glow->Scale.z = logo->Scale.x + RandomFloat() * .01f;

	glow->ColorFilter.a = logo->ColorFilter.a * .99f;

	UpdateObjectTransforms(glow);
}

/********************** MOVE COMPANY LOGO ****************************/

static void MoveCompanyLogo(ObjNode *glow)
{
float fps = gFramesPerSecondFrac;
ObjNode	*logo = glow->ChainNode;

			/***************/
			/* PANGEA LOGO */
			/***************/

			/* UPDATE PANGEA LOGO */

	logo->Coord.z += fps * 130.0f;							// move forward

	if (logo->Coord.z > 400.0f)
	{
		logo->ColorFilter.a -= fps * .6f;					// fade out
		if (logo->ColorFilter.a <= 0.0f)
		{
			logo->ColorFilter.a = 0;
			DeleteObject(glow);
			StartGameLogo();								// build the menu
			return;
		}

		if (gLogoAmbience != -1)
		{
			gAmbienceVolume -= fps * (float)FULL_CHANNEL_VOLUME * .8f;
			if (gAmbienceVolume < 0.0f)
				gAmbienceVolume = 0;
			ChangeChannelVolume(gLogoAmbience, gAmbienceVolume, gAmbienceVolume);
		}

	}

	UpdateObjectTransforms(logo);


			/* UPDATE GLOW */

	glow->Coord.z = logo->Coord.z - 1.0f;

	glow->ColorFilter.a = (logo->ColorFilter.a - RandomFloat() * .2) * .99f;
	if (glow->ColorFilter.a < 0.0f)
		glow->ColorFilter.a = 0.0f;

	UpdateObjectTransforms(glow);

}



#pragma mark -

/********************** DO HELP ***********************/

static void DoHelp(void)
{
ObjNode	*glow, *text, *pane;

	if (MyRandomLong()&1)
		PlayEffect(EFFECT_ACCENTDRONE1);
	else
		PlayEffect(EFFECT_ACCENTDRONE2);

	gFadeInText = true;
	gTextDone = false;

		/* DARKEN PANE */

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOZWRITES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG|STATUS_BIT_NOTEXTUREWRAP|
										STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.slot 		= 900;
	gNewObjectDefinition.moveCall 	= MoveCredits;
	pane = MakeNewObject(&gNewObjectDefinition);
	pane->CustomDrawFunction = DrawDarkenPane;
	pane->Scale.y = 480;
	pane->ColorFilter = (OGLColorRGBA) {0, 0, 0, 0};


			/* GLOW */

	gNewObjectDefinition.group 		= MODEL_GROUP_MAINMENU;
	gNewObjectDefinition.type 		= MAINMENU_ObjType_HelpGlow;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= DARKEN_PANE_Z + 4;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOTEXTUREWRAP|STATUS_BIT_NOLIGHTING|STATUS_BIT_GLOW|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOZWRITES;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = .7;
	glow = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	glow->ColorFilter.a = 0;
	pane->ChainNode = glow;

			/* TEXT */

	gNewObjectDefinition.type 		= MAINMENU_ObjType_HelpText;
	gNewObjectDefinition.coord.z 	+= 1.0;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOTEXTUREWRAP|STATUS_BIT_NOLIGHTING|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOZWRITES;
	gNewObjectDefinition.slot++;
	text = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	text->ColorFilter.a = 0;

	glow->ChainNode = text;




		/*************************/
		/* SHOW IN ANIMATED LOOP */
		/*************************/

	while(!gTextDone)
	{
		UpdateInput();
		if (UserWantsOut())
			gFadeInText = false;

			/* DRAW STUFF */

		CalcFramesPerSecond();
		MoveObjects();
		OGL_DrawScene(DrawObjects);
	}


		/* CLEANUP */
	DeleteObject(pane);
	UpdateInput();
}


/********************** DO HIGH SCORES ***********************/

static void DoHighScores(void)
{
ObjNode	*pane;

	PlaySong(SONG_HIGHSCORE, 0);

			/* LOAD HIGH SCORES */

	LoadHighScores();


	gFadeInText = true;
	float gScoreFadeAlpha = 0;


		/* MAKE DARKEN PANE */

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOZWRITES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG|STATUS_BIT_NOTEXTUREWRAP|
										STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+100-1;
	gNewObjectDefinition.moveCall 	= nil;
	pane = MakeNewObject(&gNewObjectDefinition);
	pane->CustomDrawFunction = DrawDarkenPane;
	pane->Scale.y = 480;
	pane->ColorFilter = (OGLColorRGBA){0,0,0,0};

	SetupHighScoreTableObjNodes(pane, 0);


		/*************************/
		/* SHOW IN ANIMATED LOOP */
		/*************************/

	while(true)
	{
		UpdateInput();
		if (UserWantsOut())
			gFadeInText = false;

			/*  FADE IN */

		if (gFadeInText)
		{
			pane->ColorFilter.a += gFramesPerSecondFrac * 2.0f;
			if (pane->ColorFilter.a > .9f)
				pane->ColorFilter.a = .9f;

			gScoreFadeAlpha += gFramesPerSecondFrac * 2.0f;
			if (gScoreFadeAlpha > 1.0f)
				gScoreFadeAlpha = 1.0f;
		}

			/* FADE OUT */
		else
		{
			pane->ColorFilter.a -= gFramesPerSecondFrac * 2.0f;
			if (pane->ColorFilter.a < 0.0f)
				pane->ColorFilter.a = 0;

			gScoreFadeAlpha -= gFramesPerSecondFrac * 2.0f;
			if (gScoreFadeAlpha < 0.0f)
			{
				gScoreFadeAlpha = 0.0f;
				break;
			}
		}


		for (ObjNode* text = pane->ChainNode; text; text = text->ChainNode)
		{
			text->ColorFilter.a = gScoreFadeAlpha;
		}

			/* DRAW STUFF */

		CalcFramesPerSecond();
		MoveObjects();
		OGL_DrawScene(DrawObjects);
	}


		/* CLEANUP */
	DeleteObject(pane);
	UpdateInput();
	PlaySong(SONG_THEME, 0);
}








#pragma mark -

/********************** DO CREDITS ***********************/

static void DoCredits(void)
{
static const MenuItem kCreditsMenu[] =
{
	{ .type = kMenuItem_Title, .text = STR_CREDITS },
	{ .type = kMenuItem_Spacer },
	{ .type = kMenuItem_Label, .text = STR_CREDITS_GAME_DESIGN },
	{ .type = kMenuItem_Label, .text = -1, .rawText = "      Brian Greenstone" },
	{ .type = kMenuItem_Label, .text = -1, .rawText = "      Duncan Knarr" },
	{ .type = kMenuItem_Spacer },
	{ .type = kMenuItem_Label, .text = STR_CREDITS_PROGRAMMING },
	{ .type = kMenuItem_Label, .text = -1, .rawText = "      Brian Greenstone" },
	{ .type = kMenuItem_Spacer },
	{ .type = kMenuItem_Label, .text = STR_CREDITS_ARTWORK },
	{ .type = kMenuItem_Label, .text = -1, .rawText = "      Duncan Knarr" },
	{ .type = kMenuItem_Spacer },
	{ .type = kMenuItem_Label, .text = STR_CREDITS_MUSIC },
	{ .type = kMenuItem_Label, .text = -1, .rawText = "      Aleksander Dimitrijevic" },
	{ .type = kMenuItem_Spacer },
	{ .type = kMenuItem_Label, .text = STR_CREDITS_ADDITIONAL_PROGRAMMING },
	{ .type = kMenuItem_Label, .text = -1, .rawText = "      Iliyas Jorio" },
	{ .type = kMenuItem_END_SENTINEL },
};

	if (MyRandomLong()&1)
		PlayEffect(EFFECT_ACCENTDRONE1);
	else
		PlayEffect(EFFECT_ACCENTDRONE2);

	MenuStyle creditsMenuStyle = kDefaultMenuStyle;
	creditsMenuStyle.isInteractive = false;

	StartMenu(kCreditsMenu, &creditsMenuStyle, nil, DrawObjects);
}



/************************ MOVE CREDITS **************************/

void MoveCredits(ObjNode *pane)
{
ObjNode	*glow = pane->ChainNode;
ObjNode *mainText = glow->ChainNode;

			/* FADE IN */

	if (gFadeInText)
	{
		for (ObjNode* text = glow->ChainNode; text; text = text->ChainNode)
		{
			text->ColorFilter.a += gFramesPerSecondFrac * 1.4f;
			if (text->ColorFilter.a > 1)
				text->ColorFilter.a = 1;
		}
	}

		/* FADE OUT */

	else
	{
		for (ObjNode* text = glow->ChainNode; text; text = text->ChainNode)
		{
			text->ColorFilter.a -= gFramesPerSecondFrac * 1.4f;
			if (text->ColorFilter.a <= 0.0f)
			{
				text->ColorFilter.a = 0.0f;
				gTextDone = true;
			}
		}
	}


	glow->ColorFilter.a = (.8f + RandomFloat() * .2f) * (mainText->ColorFilter.a * .7f);
	pane->ColorFilter.a = mainText->ColorFilter.a * .8f;
}




/********************** DRAW DARKEN PANE *****************************/

void DrawDarkenPane(ObjNode *theNode)
{
	OGL_PushState();
	SetInfobarSpriteState(true);

	glDisable(GL_TEXTURE_2D);
	SetColor4fv(&theNode->ColorFilter.r);
	glEnable(GL_BLEND);

	glBegin(GL_QUADS);
	glVertex3f(-g2DLogicalWidth,	-theNode->Scale.y,	0);
	glVertex3f(g2DLogicalWidth,		-theNode->Scale.y,	0);
	glVertex3f(g2DLogicalWidth,		theNode->Scale.y,	0);
	glVertex3f(-g2DLogicalWidth,	theNode->Scale.y,	0);
	glEnd();

	OGL_PopState();
}

