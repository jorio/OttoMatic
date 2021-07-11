// OTTO MATIC LEVEL SELECT SCREEN
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include "game.h"
#include <time.h>

/****************************/
/*    EXTERNALS            */
/****************************/

/****************************/
/*    PROTOTYPES            */
/****************************/

static void cb_Back(void);

/****************************/
/*    CONSTANTS            */
/****************************/

enum
{
	LEVEL_CHEAT_STATE_OFF,
	LEVEL_CHEAT_STATE_FADE_IN,
	LEVEL_CHEAT_STATE_READY,
	LEVEL_CHEAT_STATE_FADE_OUT,
};

static const OGLColorRGBA kTitleColor = {1.0f, 1.0f, 0.7f, 1.0f};
static const OGLColorRGBA kInactiveColor = {0.3f, 0.5f, 0.2f, 1.0f};


/****************************/
/*    VARIABLES            */
/****************************/

static float gSettingsFadeAlpha = 0;
static int gSettingsState = LEVEL_CHEAT_STATE_OFF;

static int gMenuHighlightedRow = 0;
static int gMenuNumRows = NUM_LEVELS;
static bool gMenuExitedWithLegalRow = false;

#pragma mark -

/***************************************************************/
/*                       CALLBACKS                             */
/***************************************************************/

static void cb_Back(void)
{
	switch (gSettingsState)
	{
		case LEVEL_CHEAT_STATE_READY:
			gSettingsState = LEVEL_CHEAT_STATE_FADE_OUT;
			break;

		default:
			gSettingsState = LEVEL_CHEAT_STATE_FADE_OUT;
			break;
	}
}

static void MoveText(ObjNode* node)
{
	if (gSettingsState == LEVEL_CHEAT_STATE_OFF)
	{
		DeleteObject(node);
		return;
	}

	if (node->Flag[0])										// it's a selectable node
	{
		if (node->Special[0] == gMenuHighlightedRow)
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

static void NavigateMenu(void)
{
	UpdateInput();

	bool valueActivated = false;

	if (GetNewKeyState(SDL_SCANCODE_ESCAPE))
	{
		cb_Back();
	}
	else if (GetNewKeyState(SDL_SCANCODE_UP))
	{
		gMenuHighlightedRow = PositiveModulo(gMenuHighlightedRow - 1, gMenuNumRows);
		PlayEffect(EFFECT_WEAPONCLICK);
	}
	else if (GetNewKeyState(SDL_SCANCODE_DOWN))
	{
		gMenuHighlightedRow = PositiveModulo(gMenuHighlightedRow + 1, gMenuNumRows);
		PlayEffect(EFFECT_WEAPONCLICK);
	}
	else if (GetNewKeyState(SDL_SCANCODE_SPACE) || GetNewKeyState(SDL_SCANCODE_RETURN))
	{
		valueActivated = true;
	}

	if (valueActivated)
	{
		PlayEffect(EFFECT_MENUCHANGE);
		gMenuExitedWithLegalRow = true;
		cb_Back();

		MyFlushEvents();
	}
}

#pragma mark -

/***************************************************************/
/*                       MAIN LOOP                             */
/***************************************************************/

int DoLevelCheatDialog(void (*backgroundDrawRoutine)(OGLSetupOutputType *))
{
Str255 label;

	gMenuExitedWithLegalRow = false;

	gNewObjectDefinition.coord		= (OGLPoint3D){-170, -160, 0};
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= MoveText;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 2.0f;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	ObjNode* title = TextMesh_New(Localize(STR_LEVEL_CHEAT), 0,&gNewObjectDefinition);
	title->ColorFilter = kTitleColor;
	gNewObjectDefinition.coord.y 	+= 64;

	gNewObjectDefinition.scale 	    = .66f;
	for (int i = 0; i < NUM_LEVELS; i++)
	{
		snprintf(label, sizeof(label), "%d. %s", i + 1, Localize(STR_LEVEL_1 + i));

		ObjNode* item = TextMesh_New(label, 0, &gNewObjectDefinition);
		item->Flag[0] = true;	// mark it as selectable
		item->Special[0] = i;

		gNewObjectDefinition.coord.y 	+= 20;
	}

	gSettingsState = LEVEL_CHEAT_STATE_FADE_IN;
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

	while (gSettingsState != LEVEL_CHEAT_STATE_OFF)
	{
		switch (gSettingsState)
		{
			case LEVEL_CHEAT_STATE_FADE_IN:
				pane->ColorFilter.a += gFramesPerSecondFrac * 2.0f;
				if (pane->ColorFilter.a > .9f)
					pane->ColorFilter.a = .9f;

				gSettingsFadeAlpha += gFramesPerSecondFrac * 2.0f;
				if (gSettingsFadeAlpha >= 1.0f)
				{
					gSettingsFadeAlpha = 1.0f;
					gSettingsState = LEVEL_CHEAT_STATE_READY;
				}
				break;

			case LEVEL_CHEAT_STATE_FADE_OUT:
				pane->ColorFilter.a -= gFramesPerSecondFrac * 2.0f;
				if (pane->ColorFilter.a < 0.0f)
					pane->ColorFilter.a = 0;

				gSettingsFadeAlpha -= gFramesPerSecondFrac * 2.0f;
				if (gSettingsFadeAlpha <= 0.0f)
				{
					gSettingsFadeAlpha = 0.0f;
					gSettingsState = LEVEL_CHEAT_STATE_OFF;
				}
				break;

			case LEVEL_CHEAT_STATE_READY:
				NavigateMenu();
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
	MyFlushEvents();

	if (gMenuExitedWithLegalRow)
	{
		return gMenuHighlightedRow;
	}
	else
	{
		return -1;
	}
}
