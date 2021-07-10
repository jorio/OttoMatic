// OTTO MATIC FILE SELECT SCREEN
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include "game.h"
#include <time.h>

/****************************/
/*    EXTERNALS            */
/****************************/

extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	float					gFramesPerSecondFrac,gFramesPerSecond,gGlobalTransparency;
extern	float					g2DLogicalWidth;
extern	float					g2DLogicalHeight;
extern	OGLColorRGB				gGlobalColorFilter;
extern	SDL_Window				*gSDLWindow;
extern	Boolean					gAllowAudioKeys;
extern	short	gPrefsFolderVRefNum;
extern	long	gPrefsFolderDirID;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void cb_Back(void);

/****************************/
/*    CONSTANTS            */
/****************************/

enum
{
	FILE_SCREEN_STATE_OFF,
	FILE_SCREEN_STATE_FADE_IN,
	FILE_SCREEN_STATE_READY,
	FILE_SCREEN_STATE_FADE_OUT,
};

static const OGLColorRGBA kTitleColor = {1.0f, 1.0f, 0.7f, 1.0f};
static const OGLColorRGBA kInactiveColor = {0.3f, 0.5f, 0.2f, 1.0f};


/****************************/
/*    VARIABLES            */
/****************************/

static float gSettingsFadeAlpha = 0;
static int gSettingsState = FILE_SCREEN_STATE_OFF;

static int gFileScreenType = FILE_SCREEN_TYPE_LOAD;
static int gFileScreenHighlightedRow = 0;
static int gFileScreenNumRows = NUM_SAVE_SLOTS;
static bool gFileScreenExitedWithLegalSlot = false;

#pragma mark -

/***************************************************************/
/*                       CALLBACKS                             */
/***************************************************************/

static void cb_Back(void)
{
	switch (gSettingsState)
	{
		case FILE_SCREEN_STATE_READY:
			gSettingsState = FILE_SCREEN_STATE_FADE_OUT;
			break;

		default:
			gSettingsState = FILE_SCREEN_STATE_FADE_OUT;
			break;
	}
}

static void MoveFileScreenText(ObjNode* node)
{
	if (gSettingsState == FILE_SCREEN_STATE_OFF)
	{
		DeleteObject(node);
		return;
	}

	if (node->Flag[0])										// it's a selectable node
	{
		if (node->Special[0] == gFileScreenHighlightedRow)
		{
			float rf = .7f + RandomFloat() * .29f;
			node->ColorFilter = (OGLColorRGBA){rf, rf, rf, 1};
		}
		else													// no highlight
		{
			node->ColorFilter = kInactiveColor;
		}
	}

	node->ColorFilter.a = gSettingsFadeAlpha;
}

#pragma mark -

/***************************************************************/
/*                       NAVIGATION                            */
/***************************************************************/

static void NavigateFileMenu(void)
{
	UpdateInput();

	bool valueActivated = false;

	if (GetNewKeyState(SDL_SCANCODE_ESCAPE))
	{
		cb_Back();
	}
	else if (GetNewKeyState(SDL_SCANCODE_UP))
	{
		gFileScreenHighlightedRow = PositiveModulo(gFileScreenHighlightedRow - 1, gFileScreenNumRows);
		PlayEffect(EFFECT_WEAPONCLICK);
	}
	else if (GetNewKeyState(SDL_SCANCODE_DOWN))
	{
		gFileScreenHighlightedRow = PositiveModulo(gFileScreenHighlightedRow + 1, gFileScreenNumRows);
		PlayEffect(EFFECT_WEAPONCLICK);
	}
	else if (GetNewKeyState(SDL_SCANCODE_SPACE) || GetNewKeyState(SDL_SCANCODE_RETURN))
	{
		valueActivated = true;
	}

	if (valueActivated)
	{
		if (gFileScreenType == FILE_SCREEN_TYPE_SAVE)
		{
			PlayEffect(EFFECT_WEAPONCLICK);
			SaveGame(gFileScreenHighlightedRow);
			gFileScreenExitedWithLegalSlot = true;
			cb_Back();
		}
		else
		{
			if (LoadSavedGame(gFileScreenHighlightedRow))
			{
				//PlayEffect(EFFECT_WEAPONCLICK);
				gFileScreenExitedWithLegalSlot = true;
				cb_Back();
			}
		}

		MyFlushEvents();
	}
}

#pragma mark -

/***************************************************************/
/*                       MAIN LOOP                             */
/***************************************************************/

bool DoFileScreen(int fileScreenType, void (*backgroundDrawRoutine)(OGLSetupOutputType *))
{
Str255 label;

	gFileScreenType = fileScreenType;
	gFileScreenExitedWithLegalSlot = false;

	gNewObjectDefinition.coord		= (OGLPoint3D){-170, -160, 0};
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= MoveFileScreenText;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 2.0f;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	ObjNode* title = TextMesh_New(
			Localize(fileScreenType == FILE_SCREEN_TYPE_LOAD ? STR_LOAD_GAME : STR_SAVE_GAME), 0,
			&gNewObjectDefinition);
	title->ColorFilter = kTitleColor;
	gNewObjectDefinition.coord.y 	+= 64;

	gNewObjectDefinition.scale 	    = .66f;
	for (int i = 0; i < NUM_SAVE_SLOTS; i++)
	{
		SaveGameType saveData;

		if (!LoadSaveGameStruct(i, &saveData))
		{
			snprintf(label, sizeof(label), "%s", Localize(STR_EMPTY_SLOT));
		}
		else
		{
			time_t timestamp = (time_t) saveData.timestamp;
			struct tm *timeinfo = localtime(&timestamp);

			snprintf(label, sizeof(label),
					"%s %d (%d %s %d)",
					Localize(STR_LEVEL),
					saveData.realLevel+1,
					timeinfo->tm_mday,
					Localize(timeinfo->tm_mon + STR_JANUARY),
					1900 + timeinfo->tm_year);
		}

		ObjNode* item = TextMesh_New(label, 0, &gNewObjectDefinition);
		item->Flag[0] = true;	// mark it as selectable
		item->Special[0] = i;

		gNewObjectDefinition.coord.y 	+= 20;
	}

	gAllowAudioKeys = false;					// dont interfere with keyboard binding

//	PlayEffect(MyRandomLong()&1? EFFECT_ACCENTDRONE1: EFFECT_ACCENTDRONE2);

	gSettingsState = FILE_SCREEN_STATE_FADE_IN;
	gSettingsFadeAlpha = 0;


	/* MAKE DARKEN PANE */

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOZWRITES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG|STATUS_BIT_NOTEXTUREWRAP|
										STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+100-1;
	gNewObjectDefinition.moveCall 	= nil;
	ObjNode* pane = MakeNewObject(&gNewObjectDefinition);
	pane->CustomDrawFunction = DrawDarkenPane;
	pane->ColorFilter.r = 0;
	pane->ColorFilter.g = 0;
	pane->ColorFilter.b = 0;
	pane->ColorFilter.a = 0;


	/*************************/
	/* SHOW IN ANIMATED LOOP */
	/*************************/

	while (gSettingsState != FILE_SCREEN_STATE_OFF)
	{
		switch (gSettingsState)
		{
			case FILE_SCREEN_STATE_FADE_IN:
				pane->ColorFilter.a += gFramesPerSecondFrac * 2.0f;
				if (pane->ColorFilter.a > .9f)
					pane->ColorFilter.a = .9f;

				gSettingsFadeAlpha += gFramesPerSecondFrac * 2.0f;
				if (gSettingsFadeAlpha >= 1.0f)
				{
					gSettingsFadeAlpha = 1.0f;
					gSettingsState = FILE_SCREEN_STATE_READY;
				}
				break;

			case FILE_SCREEN_STATE_FADE_OUT:
				pane->ColorFilter.a -= gFramesPerSecondFrac * 2.0f;
				if (pane->ColorFilter.a < 0.0f)
					pane->ColorFilter.a = 0;

				gSettingsFadeAlpha -= gFramesPerSecondFrac * 2.0f;
				if (gSettingsFadeAlpha <= 0.0f)
				{
					gSettingsFadeAlpha = 0.0f;
					gSettingsState = FILE_SCREEN_STATE_OFF;
				}
				break;

			case FILE_SCREEN_STATE_READY:
				NavigateFileMenu();
				break;

			default:
				break;
		}

			/* DRAW STUFF */

		CalcFramesPerSecond();
		MoveObjects();
		OGL_DrawScene(gGameViewInfoPtr, backgroundDrawRoutine);
	}


	/* CLEANUP */

	DeleteObject(pane);
	UpdateInput();

	SavePrefs();

	MyFlushEvents();

	gAllowAudioKeys = true;

	return gFileScreenExitedWithLegalSlot;
}
