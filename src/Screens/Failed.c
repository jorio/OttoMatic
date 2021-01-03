/****************************/
/*   	FAILED.C			*/
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	FSSpec		gDataSpec;
extern	Boolean		gNetGameInProgress,gGameOver, gNoCarControls;
extern	KeyMap gKeyMap,gNewKeys;
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

static void BuildFailedMenu(const Str32 s);
static Boolean NavigateFailedMenu(void);
static void FreeFailedMenu(void);
static void MoveFailedObject(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	FAILED_ICON_SCALE	.4f

#define	LINE_SPACING	.12f



/*********************/
/*    VARIABLES      */
/*********************/

static short	gFailedMenuSelection;

static ObjNode	*gFailedIcons[2];

Str63	gMenuStrings[2] =
{
	{"TRY AGAIN"},
	{"RETIRE"},
};


static Boolean	gTryAgain;

static const OGLColorRGBA gFailedMenuHiliteColor = {.3,.5,.2,1};

/********************** DO FAILED **************************/
//
// Called at end of race when draw context is still visible and active
//
// return true if try again
//

Boolean DoFailedMenu(const Str31	headerString)
{
	BuildFailedMenu(headerString);


				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	UpdateInput();
	gNoCarControls = true;									// nobody has control during this

	while(true)
	{
			/* SEE IF MAKE SELECTION */

		if (NavigateFailedMenu())
			break;

			/* MOVE STUFF */

		MoveEverything();


			/* DRAW STUFF */

		CalcFramesPerSecond();
		UpdateInput();
		DoPlayerTerrainUpdate(gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z);	// need to call this to keep supertiles active
		OGL_DrawScene(gGameViewInfoPtr, DrawArea);
	}

	FreeFailedMenu();

	return(gTryAgain);
}


/**************************** BUILD FAILED MENU *********************************/

static void BuildFailedMenu(const Str32 s)
{
short	i;
ObjNode	*obj;

	gFailedMenuSelection = 0;

	gTryAgain = true;

			/* BUILD NEW TEXT STRINGS */

	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= -.6;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= MoveFailedObject;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = FAILED_ICON_SCALE * .8f;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;

	obj = MakeFontStringObject(s, &gNewObjectDefinition, gGameViewInfoPtr, true);		// title
	obj->ColorFilter.a = 0;

	gNewObjectDefinition.coord.y 	-= LINE_SPACING * 1.5f;

	gNewObjectDefinition.scale 	    = FAILED_ICON_SCALE;
	for (i = 0; i < 2; i++)
	{
		Str255	s;

#if 1
		s[0] = sprintf(s+1, "SRCPORT TODO GETINDSTRING %d %d", 3000 + gGamePrefs.language, i + 1);
		SOURCE_PORT_MINOR_PLACEHOLDER();
#else
		GetIndString(s, 3000 + gGamePrefs.language, i + 1);
#endif

		gFailedIcons[i] = MakeFontStringObject(s, &gNewObjectDefinition, gGameViewInfoPtr, true);
		gFailedIcons[i]->ColorFilter.a = 0;
		gNewObjectDefinition.coord.y 	-= LINE_SPACING;
	}
}

/****************** MOVE FAILED OBJECT ************************/

static void MoveFailedObject(ObjNode *theNode)
{
	theNode->ColorFilter.a += gFramesPerSecondFrac * 2.0f;			// fade in
	if (theNode->ColorFilter.a > 1.0f)
		theNode->ColorFilter.a = 1.0f;
}



/******************** FREE FAILED MENU **********************/
//
// Free objects of current menu
//

static void FreeFailedMenu(void)
{
int		i;

	for (i = 0; i < 2; i++)
	{
		DeleteObject(gFailedIcons[i]);
	}
}


/*********************** NAVIGATE FAILED MENU **************************/

static Boolean NavigateFailedMenu(void)
{
short	i;


		/* SEE IF CHANGE SELECTION */

	if (GetNewKeyState(KEY_UP) && (gFailedMenuSelection > 0))
	{
//		PlayEffect(EFFECT_SELECTCLICK);
		gFailedMenuSelection--;
	}
	else
	if (GetNewKeyState(KEY_DOWN) && (gFailedMenuSelection < 1))
	{
//		PlayEffect(EFFECT_SELECTCLICK);
		gFailedMenuSelection++;
	}

		/* SET APPROPRIATE FRAMES FOR ICONS */

	for (i = 0; i < 2; i++)
	{
		if (i != gFailedMenuSelection)
		{
			gFailedIcons[i]->ColorFilter.r = 										// normal
			gFailedIcons[i]->ColorFilter.g =
			gFailedIcons[i]->ColorFilter.b =
			gFailedIcons[i]->ColorFilter.a = 1.0;
		}
		else
		{
			gFailedIcons[i]->ColorFilter = gFailedMenuHiliteColor;										// hilite
		}

	}


			/***************************/
			/* SEE IF MAKE A SELECTION */
			/***************************/

	if (GetNewKeyState(KEY_SPACE) || GetNewKeyState(KEY_RETURN))
	{
		switch(gFailedMenuSelection)
		{
			case	0:								// try again
					gTryAgain = true;
					gGameOver = false;

					break;

			case	1:								// retire
					gTryAgain = false;
					break;

		}
		return(true);
	}


	return(false);
}
















