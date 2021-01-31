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

const static OGLColorRGB kTitleColor = {1.0, 1.0, 0.7};


/****************************/
/*    VARIABLES            */
/****************************/

static float gSettingsFadeAlpha = 0;
static int gSettingsState = FILE_SCREEN_STATE_OFF;

static int gFileScreenType = FILE_SCREEN_TYPE_LOAD;
static void (*gFileScreenBackgroundDrawRoutine)(OGLSetupOutputType *) = nil;
static int gFileScreenHighlightedRow = 0;
static int gFileScreenNumRows = NUM_SAVE_SLOTS;
static bool gFileScreenExitedWithLegalSlot = false;

static char gFileScreenLabels[NUM_SAVE_SLOTS][64];

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
/*                       RENDERING                             */
/***************************************************************/

static void DrawText(
		const char* s,
		float x,
		float y,
		float xs,
		float ys,
		OGLSetupOutputType *info)
{
	const float cw = SCORE_TEXT_SPACING * xs /* * gs*/;

	for (const char* c = s; *c; c++)
	{
		if (*c == ' ')
		{
			x += cw * .5f;
			continue;
		}

		int texNum = CharToSprite(toupper(*c));				// get texture #
		if (texNum == -1)
		{
			texNum = CharToSprite('?');
		}
		if (texNum != -1)
		{
			DrawInfobarSprite2_Scaled(
					x,
					y,
					SCORE_TEXT_SPACING * xs * 2.0f,
					SCORE_TEXT_SPACING * ys * 2.0f,
					SPRITE_GROUP_FONT,
					texNum,
					info);
		}

		x += cw;
	}
}

static void SetSettingColor(bool highlight)
{
	if (highlight)											// highlight
	{
		float rf = .7f + RandomFloat() * .29f;
		gGlobalColorFilter = (OGLColorRGB){rf, rf, rf};
	}
	else													// no highlight
	{
		gGlobalColorFilter = (OGLColorRGB){.3, .5, .2};
	}
}

static void DrawFileScreen(OGLSetupOutputType *info)
{
	gFileScreenBackgroundDrawRoutine(info);

	/* SET STATE */

	OGL_PushState();

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	OGL_DisableLighting();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 640, 480, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);						// make glow

	gGlobalTransparency = gSettingsFadeAlpha;

	/*****************/
	/* DRAW THE TEXT */
	/*****************/

	{
		float y = 70.0f;

		// Draw title
		gGlobalColorFilter = kTitleColor;
		int titleStringID = gFileScreenType == FILE_SCREEN_TYPE_LOAD ? STR_LOAD_GAME : STR_SAVE_GAME;
		DrawText(GetLanguageString(titleStringID), 150, y, 1.33f, 2.0f, info);
		y += SCORE_TEXT_SPACING * 5.0f;

		// Draw rows
		for (int saveID = 0; saveID < gFileScreenNumRows; saveID++)
		{
			SetSettingColor(gFileScreenHighlightedRow == saveID);

			DrawText(gFileScreenLabels[saveID], 150, y, .66f, 1.0f, info);

			y += SCORE_TEXT_SPACING * 1.5f;
		}
	}

	/***********/
	/* CLEANUP */
	/***********/

	gGlobalTransparency = 1;
	OGL_PopState();
}

#pragma mark -

/***************************************************************/
/*                       MAIN LOOP                             */
/***************************************************************/

bool DoFileScreen(int fileScreenType, void (*backgroundDrawRoutine)(OGLSetupOutputType *))
{
	gFileScreenType = fileScreenType;
	gFileScreenBackgroundDrawRoutine = backgroundDrawRoutine;
	gFileScreenExitedWithLegalSlot = false;

	for (int i = 0; i < NUM_SAVE_SLOTS; i++)
	{
		SaveGameType saveData;
		if (!LoadSaveGameStruct(i, &saveData))
		{
			snprintf(gFileScreenLabels[i], sizeof(gFileScreenLabels[i]), "%s", GetLanguageString(STR_EMPTY_SLOT));
			continue;
		}

		time_t timestamp = (time_t) saveData.timestamp;
		struct tm *timeinfo = localtime(&timestamp);

		snprintf(gFileScreenLabels[i], sizeof(gFileScreenLabels[i]),
			"%d %s %d  -  %s %d  -  %s %d",
			timeinfo->tm_mday,
			GetLanguageString(timeinfo->tm_mon + STR_JANUARY),
			1900 + timeinfo->tm_year,
			GetLanguageString(STR_LEVEL),
			saveData.realLevel+1,
			GetLanguageString(STR_SCORE),
			saveData.score
			);
	}

	gAllowAudioKeys = false;					// dont interfere with keyboard binding

//	PlayEffect(MyRandomLong()&1? EFFECT_ACCENTDRONE1: EFFECT_ACCENTDRONE2);

	gSettingsState = FILE_SCREEN_STATE_FADE_IN;
	gSettingsFadeAlpha = 0;


	/* MAKE DARKEN PANE */

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOZWRITES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG|STATUS_BIT_NOTEXTUREWRAP|
										STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+100;
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
		OGL_DrawScene(gGameViewInfoPtr, DrawFileScreen);
	}


	/* CLEANUP */

	DeleteObject(pane);
	UpdateInput();

	SavePrefs();

	MyFlushEvents();

	gAllowAudioKeys = true;

	gFileScreenBackgroundDrawRoutine = nil;

	return gFileScreenExitedWithLegalSlot;
}
