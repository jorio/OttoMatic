/****************************/
/*   	HIGHSCORES.C    	*/
/* (c)1996-99 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

//#include <AGL/aglmacro.h> // srcport rm

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	float			gFramesPerSecond,gFramesPerSecondFrac,gGlobalTransparency;
extern	short	gPrefsFolderVRefNum;
extern	long	gPrefsFolderDirID;
extern	FSSpec	gDataSpec;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	u_long			gScore,gGlobalMaterialFlags,gLoadedScore;
extern	Boolean			gPlayingFromSavedGame,gAllowAudioKeys;
extern	SDL_GLContext		gAGLContext;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void SetupScoreScreen(void);
static void FreeScoreScreen(void);
static void DrawHighScoresCallback(OGLSetupOutputType *info);
static void DrawScoreVerbage(OGLSetupOutputType *info);
static void DrawHighScoresAndCursor(OGLSetupOutputType *info);
static void SetHighScoresSpriteState(void);
static void StartEnterName(void);
static void MoveHighScoresCyc(ObjNode *theNode);
static Boolean IsThisScoreInList(u_long score);
static short AddNewScore(u_long newScore);
static void SaveHighScores(void);

/***************************/
/*    CONSTANTS            */
/***************************/

enum
{
	HIGHSCORES_SObjType_Cyc
};


enum
{
	HIGHSCORES_SObjType_ScoreText,
	HIGHSCORES_SObjType_ScoreTextGlow,
	HIGHSCORES_SObjType_EnterNameText,
	HIGHSCORES_SObjType_EnterNameGlow
};


#define MYSCORE_DIGIT_SPACING 	30.0f


/***************************/
/*    VARIABLES            */
/***************************/

static Str32	gHighScoresFileName = ":OttoMatic:HighScores";

HighScoreType	gHighScores[NUM_SCORES];

static	float	gFinalScoreTimer,gFinalScoreAlpha, gCursorFlux = 0;

static	short	gNewScoreSlot,gCursorIndex;

static	Boolean	gDrawScoreVerbage,gExitHighScores;


/*********************** NEW SCORE ***********************************/

void NewScore(void)
{
	if (gScore == 0)
		return;

	gAllowAudioKeys = false;					// dont interfere with name editing

			/* INIT */


	LoadHighScores();										// make sure current scores are loaded
	SetupScoreScreen();										// setup OGL
	MakeFadeEvent(true, 1.0);


			/* LOOP */

	CalcFramesPerSecond();
	UpdateInput();

	while(!gExitHighScores)
	{

		CalcFramesPerSecond();
		UpdateInput();
		MoveObjects();
		OGL_DrawScene(gGameViewInfoPtr, DrawHighScoresCallback);

				/*****************************/
				/* SEE IF USER ENTERING NAME */
				/*****************************/

		if (!gDrawScoreVerbage)
		{
			SOURCE_PORT_MINOR_PLACEHOLDER(); // TODO: reimplement for SDL input
#if 0
			EventRecord 	theEvent;

			GetNextEvent(keyDownMask|autoKeyMask, &theEvent);							// poll event queue
			if ((theEvent.what == keyDown) || (theEvent.what == autoKeyMask))			// see if key pressed
			{
				char	theChar = theEvent.message & charCodeMask;						// extract key
				int		i;

				switch(theChar)
				{
					case	CHAR_RETURN:
							gExitHighScores = true;
							break;

					case	CHAR_LEFT:
							if (gCursorIndex > 0)
								gCursorIndex--;
							break;

					case	CHAR_RIGHT:
							if (gCursorIndex < (MAX_NAME_LENGTH-1))
								gCursorIndex++;
							else
								gCursorIndex = MAX_NAME_LENGTH-1;
							break;

					case	CHAR_DELETE:
							if (gCursorIndex > 0)
							{
								gCursorIndex--;
								for (i = gCursorIndex+1; i < MAX_NAME_LENGTH; i++)
									gHighScores[gNewScoreSlot].name[i] = gHighScores[gNewScoreSlot].name[i+1];
								gHighScores[gNewScoreSlot].name[MAX_NAME_LENGTH] = ' ';
							}
							break;

					default:
							if (gCursorIndex < MAX_NAME_LENGTH)								// dont add anything more if maxxed out now
							{
								if ((theChar >= 'a') && (theChar <= 'z'))					// see if convert lower case to upper case a..z
									theChar = 'A' + (theChar-'a');
								gHighScores[gNewScoreSlot].name[gCursorIndex+1] = theChar;
								gCursorIndex++;
							}
				}

			}
#endif
		}
	}


		/* CLEANUP */

	if (gNewScoreSlot != -1)						// if a new score was added then update the high scores file
		SaveHighScores();

	FreeScoreScreen();

	GammaFadeOut();


	gAllowAudioKeys = true;
}


/********************* SETUP SCORE SCREEN **********************/

static void SetupScoreScreen(void)
{
FSSpec				spec;
OGLSetupInputType	viewDef;
ObjNode				*newObj;

	PlaySong(SONG_HIGHSCORE, true);

	gDrawScoreVerbage = true;
	gExitHighScores = false;
	gFinalScoreAlpha = 1.0f;


		/* IF THIS WAS A SAVED GAME AND SCORE HASN'T CHANGED AND IS ALREADY IN LIST THEN DON'T ADD TO HIGH SCORES */

	if (gPlayingFromSavedGame && (gScore == gLoadedScore) && IsThisScoreInList(gScore))
	{
		gFinalScoreTimer = 4.0f;
		gNewScoreSlot = -1;
	}

			/* NOT SAVED GAME OR A BETTER SCORE THAN WAS LOADED OR ISN'T IN LIST YET */
	else
	{
		gNewScoreSlot = AddNewScore(gScore);				// try to add new score to high scores list
		if (gNewScoreSlot == -1)							// -1 if not added
			gFinalScoreTimer = 4.0f;
		else
			gFinalScoreTimer = 3.0f;
	}



			/**************/
			/* SETUP VIEW */
			/**************/

	OGL_NewViewDef(&viewDef);

	viewDef.camera.fov 			= 1.9;
	viewDef.camera.hither 		= 20;
	viewDef.camera.yon 			= 5000;

	viewDef.camera.from.x		= 0;
	viewDef.camera.from.z		= 800;
	viewDef.camera.from.y		= 0;


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

		gAnaglyphFocallength	= 180.0f;
		gAnaglyphEyeSeparation 	= 10.0f;
	}

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


				/************/
				/* LOAD ART */
				/************/

	InitSparkles();

			/* LOAD MODELS */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:highscores.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_HIGHSCORES, gGameViewInfoPtr);


			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_OTTO, gGameViewInfoPtr);


			/* LOAD SPRITES */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:particle.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_PARTICLES, gGameViewInfoPtr);
	BlendAllSpritesInGroup(SPRITE_GROUP_PARTICLES);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:helpfont.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FONT, gGameViewInfoPtr);
	BlendAllSpritesInGroup(SPRITE_GROUP_FONT);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:highscores.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_HIGHSCORES, gGameViewInfoPtr);



				/* LOAD AUDIO */

//	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:bonus.sounds", &spec);
//	LoadSoundBank(&spec, SOUND_BANK_BONUS);



			/************/
			/* MAKE CYC */
			/************/

	gNewObjectDefinition.group 		= MODEL_GROUP_HIGHSCORES;
	gNewObjectDefinition.type 		= HIGHSCORES_SObjType_Cyc;
	gNewObjectDefinition.coord		= viewDef.camera.from;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG;
	gNewObjectDefinition.slot 		= 10;
	gNewObjectDefinition.moveCall 	= MoveHighScoresCyc;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);



}




/********************** FREE SCORE SCREEN **********************/

static void FreeScoreScreen(void)
{
	MyFlushEvents();
	DeleteAllObjects();
	FreeAllSkeletonFiles(-1);
	DisposeAllSpriteGroups();
	DisposeAllBG3DContainers();
	DisposeSoundBank(SOUND_BANK_BONUS);
	OGL_DisposeWindowSetup(&gGameViewInfoPtr);
}



/***************** DRAW HIGHSCORES CALLBACK *******************/

static void DrawHighScoresCallback(OGLSetupOutputType *info)
{
	DrawObjects(info);
	DrawSparkles(info);											// draw light sparkles


			/* DRAW SPRITES */

	OGL_PushState();

	SetHighScoresSpriteState();

	if (gDrawScoreVerbage)
		DrawScoreVerbage(info);
	else
		DrawHighScoresAndCursor(info);


	OGL_PopState();
	gGlobalMaterialFlags = 0;
	gGlobalTransparency = 1.0;
}


/********************* SET HIGHSCORES SPRITE STATE ***************/

static void SetHighScoresSpriteState(void)
{
SDL_GLContext agl_ctx = gAGLContext;

	OGL_DisableLighting();
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);								// no z-buffer

	gGlobalMaterialFlags = BG3D_MATERIALFLAG_CLAMP_V|BG3D_MATERIALFLAG_CLAMP_U;	// clamp all textures


			/* INIT MATRICES */

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 640, 480, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

/********************* DRAW SCORE VERBAGE ****************************/

static void DrawScoreVerbage(OGLSetupOutputType *info)
{
Str32	s;
int		texNum,n,i;
float	x;
SDL_GLContext agl_ctx = gAGLContext;

				/* SEE IF DONE */

	gFinalScoreTimer -= gFramesPerSecondFrac;
	if (gFinalScoreTimer <= 0.0f)
	{
		if (gNewScoreSlot != -1)							// see if bail or if let player enter name for high score
		{
			StartEnterName();
		}
		else
			gExitHighScores = true;
		return;
	}
	if (gFinalScoreTimer < 1.0f)							// fade out
		gFinalScoreAlpha = gFinalScoreTimer;


			/****************************/
			/* DRAW BONUS TOTAL VERBAGE */
			/****************************/

			/* DRAW GLOW */

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	gGlobalTransparency = (.7f + RandomFloat()*.1f) * gFinalScoreAlpha;
	DrawInfobarSprite2(320-150, 170, 300, SPRITE_GROUP_HIGHSCORES, HIGHSCORES_SObjType_ScoreTextGlow, info);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			/* DRAW TEXT */

	gGlobalTransparency = gFinalScoreAlpha;
	DrawInfobarSprite2(320-150, 170, 300, SPRITE_GROUP_HIGHSCORES, HIGHSCORES_SObjType_ScoreText, info);


			/**************/
			/* DRAW SCORE */
			/**************/

	NumToString(gScore, s);
	n = s[0];										// get str len

	x = 320.0f - ((float)n / 2.0f) * MYSCORE_DIGIT_SPACING - (MYSCORE_DIGIT_SPACING/2);	// calc starting x

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	for (i = 1; i <= n; i++)
	{
		texNum = CharToSprite(s[i]);				// get texture #

		gGlobalTransparency = (.9f + RandomFloat()*.099f) * gFinalScoreAlpha;
		DrawInfobarSprite2(x, 230, MYSCORE_DIGIT_SPACING * 1.9f, SPRITE_GROUP_FONT, texNum, info);
		x += MYSCORE_DIGIT_SPACING;
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	gGlobalTransparency = 1.0f;
}


/****************** DRAW HIGH SCORES AND CURSOR ***********************/

static void DrawHighScoresAndCursor(OGLSetupOutputType *info)
{
SDL_GLContext agl_ctx = gAGLContext;
float	y,cursorY,cursorX;
int		i,j,n;
Str32	s;

	gFinalScoreAlpha += gFramesPerSecondFrac;						// fade in
	if (gFinalScoreAlpha > .99f)
		gFinalScoreAlpha = .99f;


 	gCursorFlux += gFramesPerSecondFrac * 10.0f;


			/****************************/
			/* DRAW ENTER NAME VERBAGE */
			/****************************/

			/* DRAW GLOW */

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	gGlobalTransparency = (.7f + RandomFloat()*.1f) * gFinalScoreAlpha;
	DrawInfobarSprite2(320-250, 10, 500, SPRITE_GROUP_HIGHSCORES, HIGHSCORES_SObjType_EnterNameGlow, info);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			/* DRAW TEXT */

	gGlobalTransparency = gFinalScoreAlpha;
	DrawInfobarSprite2(320-250, 10, 500, SPRITE_GROUP_HIGHSCORES, HIGHSCORES_SObjType_EnterNameText, info);


	gGlobalTransparency = gFinalScoreAlpha;


			/*****************/
			/* DRAW THE TEXT */
			/*****************/

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);						// make glow

	y = 120;
	for (i = 0; i < NUM_SCORES; i++)
	{
		if (i == gNewScoreSlot)								// see if cursor will go on this line
		{
			cursorY = y;
			cursorX = 150.0f + (SCORE_TEXT_SPACING * gCursorIndex);
		}

				/* DRAW NAME */

		DrawScoreText(gHighScores[i].name, 150,y,info);

				/* DRAW SCORE */

		NumToString(gHighScores[i].score, s);	// convert score to a text string
		if (s[0] < SCORE_DIGITS)				// pad 0's
		{
			n = SCORE_DIGITS-s[0];
			BlockMove(&s[1],&s[1+n], 20);		// shift existing data over

			for (j = 0; j < n; j++)				// pad with 0's
				s[1+j] = '0';

			s[0] = SCORE_DIGITS;
		}
		DrawScoreText(s, 350,y,info);

		y += SCORE_TEXT_SPACING * 1.3f;
	}

		/*******************/
		/* DRAW THE CURSOR */
		/*******************/

	if (gCursorIndex < MAX_NAME_LENGTH)						// dont draw if off the right side
	{
		gGlobalTransparency = (.3f + ((sin(gCursorFlux) + 1.0f) * .5f) * .699f) * gFinalScoreAlpha;
		DrawInfobarSprite2(cursorX, cursorY, SCORE_TEXT_SPACING * 1.9f, SPRITE_GROUP_FONT, HELPTEXT_SObjType_Cursor, info);
	}



			/***********/
			/* CLEANUP */
			/***********/

	gGlobalTransparency = 1;
}



/************************* START ENTER NAME **********************************/

static void StartEnterName(void)
{
	gFinalScoreAlpha = 0;
	gDrawScoreVerbage = false;
	gCursorIndex = 0;
	MyFlushEvents();
}



/************************ MOVE HIGH SCORES CYC *******************************/

static void MoveHighScoresCyc(ObjNode *theNode)
{
	theNode->Rot.y += gFramesPerSecondFrac * .07f;
	theNode->Rot.z += gFramesPerSecondFrac * .05f;

	UpdateObjectTransforms(theNode);
}


#pragma mark -


/*********************** LOAD HIGH SCORES ********************************/

void LoadHighScores(void)
{
OSErr				iErr;
short				refNum;
FSSpec				file;
long				count;

				/* OPEN FILE */

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gHighScoresFileName, &file);
	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);
	if (iErr == fnfErr)
		ClearHighScores();
	else
	if (iErr)
		DoFatalAlert("LoadHighScores: Error opening High Scores file!");
	else
	{
		count = sizeof(HighScoreType) * NUM_SCORES;
		iErr = FSRead(refNum, &count,  &gHighScores[0]);								// read data from file
		if (iErr)
		{
			FSClose(refNum);
			FSpDelete(&file);												// file is corrupt, so delete
			return;
		}
		FSClose(refNum);
	}
}


/************************ SAVE HIGH SCORES ******************************/

static void SaveHighScores(void)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;

				/* CREATE BLANK FILE */

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gHighScoresFileName, &file);
	FSpDelete(&file);															// delete any existing file
	iErr = FSpCreate(&file, 'Otto', 'Skor', smSystemScript);					// create blank file
	if (iErr)
		goto err;


				/* OPEN FILE */

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, gHighScoresFileName, &file);
	iErr = FSpOpenDF(&file, fsRdWrPerm, &refNum);
	if (iErr)
	{
err:
		DoAlert("Unable to Save High Scores file!");
		return;
	}

				/* WRITE DATA */

	count = sizeof(HighScoreType) * NUM_SCORES;
	FSWrite(refNum, &count, &gHighScores[0]);
	FSClose(refNum);

}


/**************** CLEAR HIGH SCORES **********************/

void ClearHighScores(void)
{
short				i,j;
char				blank[MAX_NAME_LENGTH] = "EMPTY----------";


			/* INIT SCORES */

	for (i=0; i < NUM_SCORES; i++)
	{
		gHighScores[i].name[0] = MAX_NAME_LENGTH;
		for (j=0; j < MAX_NAME_LENGTH; j++)
			gHighScores[i].name[j+1] = blank[j];
		gHighScores[i].score = 0;
	}

	SaveHighScores();
}


/*************************** ADD NEW SCORE ****************************/
//
// Returns high score slot that score was inserted to or -1 if none
//

static short AddNewScore(u_long newScore)
{
short	slot,i;

			/* FIND INSERT SLOT */

	for (slot=0; slot < NUM_SCORES; slot++)
	{
		if (newScore > gHighScores[slot].score)
			goto	got_slot;
	}
	return(-1);


got_slot:
			/* INSERT INTO LIST */

	for (i = NUM_SCORES-1; i > slot; i--)						// make hole
		gHighScores[i] = gHighScores[i-1];
	gHighScores[slot].score = newScore;							// set score in structure
	gHighScores[slot].name[0] = MAX_NAME_LENGTH;				// clear name
	for (i = 1; i <= MAX_NAME_LENGTH; i++)
		gHighScores[slot].name[i] = ' ';						// clear to all spaces
	return(slot);
}


#pragma mark -

/****************** IS THIS SCORE IN LIST *********************/
//
// Returns True if this score value is anywhere in the high scores already
//

static Boolean IsThisScoreInList(u_long score)
{
short	slot;

	for (slot=0; slot < NUM_SCORES; slot++)
	{
		if (gHighScores[slot].score == score)
			return(true);
	}

	return(false);
}









