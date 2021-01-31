/****************************/
/*   	PAUSED.C			*/
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	FSSpec		gDataSpec;
extern	Boolean		gGameOver,gMuteMusicFlag;
extern	KeyMap 		gKeyMap,gNewKeys;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean		gSongPlayingFlag,gResetSong,gDisableAnimSounds;
extern	PrefsType	gGamePrefs;
extern	OGLPoint3D	gCoord;
extern	MOPictureObject 	*gBackgoundPicture;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	PlayerInfoType	gPlayerInfo;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void BuildPausedMenu(void);
static Boolean NavigatePausedMenu(void);
static void FreePausedMenu(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	PAUSED_TEXT_SIZE	30.0f


/*********************/
/*    VARIABLES      */
/*********************/

static short	gPausedMenuSelection;

static ObjNode	*gPausedIcons[3];


static const OGLColorRGBA gPausedMenuNoHiliteColor = {1,1,.3,1};



/********************** DO PAUSED **************************/

void DoPaused(void)
{
short	i;
Boolean	oldMute = gMuteMusicFlag;


	if (!gMuteMusicFlag)							// see if pause music
		ToggleMusic();

	BuildPausedMenu();


				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	UpdateInput();

	while(true)
	{
			/* SEE IF MAKE SELECTION */

		if (NavigatePausedMenu())
			break;

			/* DRAW STUFF */

		CalcFramesPerSecond();
		UpdateInput();
		DoPlayerTerrainUpdate(gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z);							// need to call this to keep supertiles active
		OGL_DrawScene(gGameViewInfoPtr, DrawArea);


			/* FADE IN TEXT */

		for (i = 0; i < 3; i++)
		{
			gPausedIcons[i]->ColorFilter.a += gFramesPerSecondFrac * 3.0f;
			if (gPausedIcons[i]->ColorFilter.a > 1.0f)
				gPausedIcons[i]->ColorFilter.a = 1.0;
		}


	}

	FreePausedMenu();


	if (!oldMute)									// see if restart music
		ToggleMusic();

}


/**************************** BUILD PAUSED MENU *********************************/

static void BuildPausedMenu(void)
{
short	i;

	gPausedMenuSelection = 0;



			/* BUILD NEW TEXT STRINGS */

	gNewObjectDefinition.coord.x 	= 320;
	gNewObjectDefinition.coord.y 	= 240;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = PAUSED_TEXT_SIZE;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;

	for (i = 0; i < 3; i++)
	{
		const char* cstr = GetLanguageString(i + STR_RESUME_GAME);

		gPausedIcons[i] = MakeFontStringObject(cstr, &gNewObjectDefinition, gGameViewInfoPtr, true);
		gPausedIcons[i]->ColorFilter.a = 0;
		gNewObjectDefinition.coord.y 	+= PAUSED_TEXT_SIZE * .7f;
	}
}


/******************* MOVE PAUSED ICONS **********************/
//
// Called only when done to fade them out
//

static void MovePausedIcons(ObjNode *theNode)
{
	theNode->ColorFilter.a -= gFramesPerSecondFrac * 3.0f;
	if (theNode->ColorFilter.a < 0.0f)
		DeleteObject(theNode);

}



/******************** FREE PAUSED MENU **********************/
//
// Free objects of current menu
//

static void FreePausedMenu(void)
{
int		i;

	for (i = 0; i < 3; i++)
	{
		gPausedIcons[i]->MoveCall = MovePausedIcons;
	}
}


/*********************** NAVIGATE PAUSED MENU **************************/

static Boolean NavigatePausedMenu(void)
{
short	i;
Boolean	continueGame = false;


		/* SEE IF CHANGE SELECTION */

	if (GetNewKeyState(SDL_SCANCODE_UP) && (gPausedMenuSelection > 0))
	{
		gPausedMenuSelection--;
		PlayEffect(EFFECT_WEAPONCLICK);
	}
	else
	if (GetNewKeyState(SDL_SCANCODE_DOWN) && (gPausedMenuSelection < 2))
	{
		gPausedMenuSelection++;
		PlayEffect(EFFECT_WEAPONCLICK);
	}

		/* SET APPROPRIATE FRAMES FOR ICONS */

	for (i = 0; i < 3; i++)
	{
		if (i == gPausedMenuSelection)
		{
			gPausedIcons[i]->ColorFilter.r = 										// normal
			gPausedIcons[i]->ColorFilter.g =
			gPausedIcons[i]->ColorFilter.b =
			gPausedIcons[i]->ColorFilter.a = .8f + RandomFloat() * .19f;
		}
		else
		{
			gPausedIcons[i]->ColorFilter = gPausedMenuNoHiliteColor;										// hilite
		}

	}


			/***************************/
			/* SEE IF MAKE A SELECTION */
			/***************************/

	if (GetNewKeyState(SDL_SCANCODE_RETURN) || GetNewKeyState(SDL_SCANCODE_SPACE))
	{
//		PlayEffect(EFFECT_SELECTCLICK);
		switch(gPausedMenuSelection)
		{
			case	0:								// RESUME
					continueGame = true;
					break;

			case	1:								// EXIT
					gGameOver = true;
					continueGame = true;
					break;

			case	2:								// QUIT
					CleanQuit();
					break;

		}
	}


			/*****************************/
			/* SEE IF CANCEL A SELECTION */
			/*****************************/

	else
	if (GetNewKeyState(SDL_SCANCODE_ESCAPE))
	{
		continueGame = true;
	}


	return(continueGame);
}
















