// OTTO MATIC SETTINGS
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

/****************************/
/*    EXTERNALS            */
/****************************/

#include "game.h"

extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	float					gFramesPerSecondFrac,gFramesPerSecond,gGlobalTransparency;
extern	OGLColorRGB				gGlobalColorFilter;
extern	SDL_Window				*gSDLWindow;
extern	Boolean					gAllowAudioKeys;
extern const KeyBinding			gDefaultKeyBindings[NUM_CONTROL_NEEDS];

/****************************/
/*    PROTOTYPES            */
/****************************/

typedef struct
{
	Byte*			valuePtr;
	int				labelStrID;
	void			(*callback)(void);
	unsigned int	nChoices;
	int				choiceStrID[8];
	float			y;
	bool			extraPadding;
} SettingEntry;


static void cb_SetLanguage(void);
static void cb_ConfigureControls(void);
static void cb_Back(void);


/****************************/
/*    CONSTANTS            */
/****************************/

enum
{
	SETTINGS_STATE_OFF,
	SETTINGS_STATE_FADE_IN,
	SETTINGS_STATE_READY,
	SETTINGS_STATE_CONTROLS_READY,
	SETTINGS_STATE_CONTROLS_AWAITING_PRESS,
	SETTINGS_STATE_FADE_OUT,
};

const static OGLColorRGB kTitleColor = {1.0, 1.0, 0.7};

static SettingEntry gSettingEntries[] =
{
	{
		&gGamePrefs.language,
		STR_LANGUAGE,
		cb_SetLanguage,
		MAX_LANGUAGES, {},
		0,
		false,
	},
	{
		&gGamePrefs.fullscreen,
		STR_FULLSCREEN,
		SetFullscreenModeFromPrefs,
		2, {STR_OFF, STR_ON},
		0,
		false,
	},
	/*
	{
		&gGamePrefs.antialiasing,
		STR_ANTIALIASING,
		NULL,
		2, {STR_OFF, STR_ON},
		0,
	},
	*/
	{
		&gGamePrefs.anaglyph,
		STR_ANAGLYPH,
		NULL,
		2, {STR_OFF, STR_ON},
		0,
		false,
	},
	// Configure controls
	{
		NULL,
		STR_CONFIGURE_CONTROLS,
		cb_ConfigureControls,
		0, {},
		0,
		true,
	},
	// BACK
	{
		NULL,
		STR_BACK,
		cb_Back,
		0, {},
		0,
		true,
	},
};

/****************************/
/*    VARIABLES            */
/****************************/

// Main settings menu
static int gHighlightedSettingRow = 0;
static int gNumSettingRows = sizeof(gSettingEntries) / sizeof(gSettingEntries[0]);

// Keybindings menu
static int gHighlightedKeybindingRow = 0;
static int gHighlightedKeybindingColumn = 0;
static int gNumKeybindingRows = NUM_CONTROL_NEEDS + 2;		// 2 extra row for reset & back buttons

static float gSettingsFadeAlpha = 0;
static int gSettingsState = SETTINGS_STATE_OFF;

#pragma mark -

/***************************************************************/
/*                       CALLBACKS                             */
/***************************************************************/

static void cb_SetLanguage(void)
{
	LoadLanguageStrings(gGamePrefs.language);
}

static void cb_ConfigureControls(void)
{
	gSettingsState = SETTINGS_STATE_CONTROLS_READY;
}

static void cb_Back(void)
{
	switch (gSettingsState)
	{
		case SETTINGS_STATE_CONTROLS_AWAITING_PRESS:
			gSettingsState = SETTINGS_STATE_CONTROLS_READY;
			break;

		case SETTINGS_STATE_CONTROLS_READY:
			gSettingsState = SETTINGS_STATE_READY;
			break;

		default:
			gSettingsState = SETTINGS_STATE_FADE_OUT;
			break;
	}
}

#pragma mark -

/***************************************************************/
/*                       NAVIGATION                            */
/***************************************************************/

static void NavigateSettings(void)
{
	UpdateInput();

	bool valueActivated = false;
	int valueDelta = 0;

	if (GetNewKeyState(SDL_SCANCODE_ESCAPE))
	{
		cb_Back();
	}
	else if (GetNewKeyState(SDL_SCANCODE_UP))
	{
		gHighlightedSettingRow = PositiveModulo(gHighlightedSettingRow - 1, gNumSettingRows);
		PlayEffect(EFFECT_WEAPONCLICK);
	}
	else if (GetNewKeyState(SDL_SCANCODE_DOWN))
	{
		gHighlightedSettingRow = PositiveModulo(gHighlightedSettingRow + 1, gNumSettingRows);
		PlayEffect(EFFECT_WEAPONCLICK);
	}

	const SettingEntry* entry = &gSettingEntries[gHighlightedSettingRow];
	bool activateWithArrows = entry->valuePtr != nil;

	valueActivated = GetNewKeyState(SDL_SCANCODE_SPACE) || GetNewKeyState(SDL_SCANCODE_RETURN);

	if (valueActivated || (activateWithArrows && GetNewKeyState(SDL_SCANCODE_RIGHT)))
	{
		valueDelta = 1;
	}
	else if (activateWithArrows && GetNewKeyState(SDL_SCANCODE_LEFT))
	{
		valueDelta = -1;
	}
	else
	{
		valueDelta = 0;
	}

	if (valueDelta != 0)
	{

		if (entry->valuePtr)
		{
			PlayEffect(EFFECT_MENUCHANGE);
			unsigned int value = (unsigned int) *entry->valuePtr;
			value = PositiveModulo(value + valueDelta, entry->nChoices);
			*entry->valuePtr = value;
		}

		if (entry->callback)
		{
			entry->callback();
		}
	}

	if (valueActivated || valueDelta)
		MyFlushEvents();
}

static void NavigateKeybindings(void)
{
	UpdateInput();

	if (GetNewKeyState(SDL_SCANCODE_ESCAPE))
	{
		cb_Back();
	}
	else if (GetNewKeyState(SDL_SCANCODE_UP))
	{
		gHighlightedKeybindingRow = PositiveModulo(gHighlightedKeybindingRow - 1, gNumKeybindingRows);
		PlayEffect(EFFECT_WEAPONCLICK);
	}
	else if (GetNewKeyState(SDL_SCANCODE_DOWN))
	{
		gHighlightedKeybindingRow = PositiveModulo(gHighlightedKeybindingRow + 1, gNumKeybindingRows);
		PlayEffect(EFFECT_WEAPONCLICK);
	}
	else if (GetNewKeyState(SDL_SCANCODE_LEFT) && (gHighlightedKeybindingColumn > 0))
	{
		gHighlightedKeybindingColumn--;
		PlayEffect(EFFECT_WEAPONCLICK);
	}
	else if (GetNewKeyState(SDL_SCANCODE_RIGHT) && (gHighlightedKeybindingColumn < 1))
	{
		gHighlightedKeybindingColumn++;
		PlayEffect(EFFECT_WEAPONCLICK);
	}


	bool valueActivated = GetNewKeyState(SDL_SCANCODE_SPACE) || GetNewKeyState(SDL_SCANCODE_RETURN);
	bool valueNuked = GetNewKeyState(SDL_SCANCODE_DELETE) || GetNewKeyState(SDL_SCANCODE_BACKSPACE);

	if (gHighlightedKeybindingRow < NUM_CONTROL_NEEDS)
	{
		KeyBinding* entry = &gGamePrefs.keys[gHighlightedKeybindingRow];
		int16_t* entryKey = gHighlightedKeybindingColumn == 0 ? &entry->key1 : &entry->key2;

		if (valueNuked)
		{
			MyFlushEvents();
			if (*entryKey != 0)
			{
				PlayEffect(EFFECT_FLAREEXPLODE);
				*entryKey = 0;
			}
		}

		if (valueActivated)
		{
			MyFlushEvents();
			PlayEffect(EFFECT_MENUCHANGE);
			gSettingsState = SETTINGS_STATE_CONTROLS_AWAITING_PRESS;
		}
	}
	else if (valueActivated && gHighlightedKeybindingRow == NUM_CONTROL_NEEDS + 0)		// reset
	{
		MyFlushEvents();
		PlayEffect(EFFECT_FLAREEXPLODE);
		memcpy(gGamePrefs.keys, gDefaultKeyBindings, sizeof(gDefaultKeyBindings));
		_Static_assert(sizeof(gDefaultKeyBindings) == sizeof(gGamePrefs.keys), "size mismatch: default keybindings / prefs keybindings");
	}
	else if (valueActivated && gHighlightedKeybindingRow == NUM_CONTROL_NEEDS + 1)		// back
	{
		MyFlushEvents();
		PlayEffect(EFFECT_WEAPONCLICK);
		cb_Back();
	}
}

static void NavigateKeybinding_AwaitNewKey(void)
{
	GAME_ASSERT(gHighlightedKeybindingRow < NUM_CONTROL_NEEDS);

	UpdateInput();

	if (GetNewKeyState(SDL_SCANCODE_ESCAPE))
	{
		gSettingsState = SETTINGS_STATE_CONTROLS_READY;
		MyFlushEvents();
	}
	else
	{
		for (int i = 0; i < SDL_NUM_SCANCODES; i++)
		{
			if (GetNewKeyState(i))
			{
				PlayEffect(EFFECT_MENUCHANGE);
				KeyBinding* entry = &gGamePrefs.keys[gHighlightedKeybindingRow];
				int16_t* entryKey = gHighlightedKeybindingColumn == 0 ? &entry->key1 : &entry->key2;
				*entryKey = i;
				gSettingsState = SETTINGS_STATE_CONTROLS_READY;
				MyFlushEvents();
				break;
			}
		}
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

static void DrawMainSettingsScreen(OGLSetupOutputType* info)
{
	float y = 70.0f;

	// Draw title
	gGlobalColorFilter = kTitleColor;
	DrawText(GetLanguageString(STR_SETTINGS), 150, y, 1.33f, 2.0f, info);
	y += SCORE_TEXT_SPACING * 5.0f;

	// Draw setting rows
	for (int settingID = 0; settingID < gNumSettingRows; settingID++)
	{
		const SettingEntry* entry = &gSettingEntries[settingID];

		if (entry->extraPadding)								// add extra padding above if needed
		{
			y += SCORE_TEXT_SPACING * 1.5f;
		}

		SetSettingColor(gHighlightedSettingRow == settingID);

		DrawText(GetLanguageString(entry->labelStrID), 150, y, .66f, 1.0f, info);

		if (entry->valuePtr)
		{
			int valueStrID = 0;
			if (entry->valuePtr == &gGamePrefs.language)
				valueStrID = STR_LANGUAGE_NAME;
			else
				valueStrID = entry->choiceStrID[*entry->valuePtr];
			DrawText(GetLanguageString(valueStrID), 350, y, .66f, 1.0f, info);
		}

		y += SCORE_TEXT_SPACING * 1.5f;
	}
}

static void DrawKeybindingScreen(OGLSetupOutputType *info)
{
	float y = 70.0f;
	bool blinkFlux = SDL_GetTicks() % 500 < 300;

	// Draw title
	gGlobalColorFilter = kTitleColor;
	DrawText(GetLanguageString(STR_CONFIGURE_CONTROLS), 150, y, 1.33f, 2.0f, info);
	y += SCORE_TEXT_SPACING * 5.0f;

	// Draw binding text
	for (int bindingID = 0; bindingID < NUM_CONTROL_NEEDS; bindingID++)
	{
		bool isRowHighlighted = gHighlightedKeybindingRow == bindingID;

		const char* caption = GetLanguageString(STR_KEYBINDING_DESCRIPTION_0 + bindingID);
		const char* value1 = "---";
		const char* value2 = "---";

		if (gGamePrefs.keys[bindingID].key1 != 0)
			value1 = SDL_GetScancodeName(gGamePrefs.keys[bindingID].key1);

		if (gGamePrefs.keys[bindingID].key2 != 0)
			value2 = SDL_GetScancodeName(gGamePrefs.keys[bindingID].key2);

		if (isRowHighlighted && gSettingsState == SETTINGS_STATE_CONTROLS_AWAITING_PRESS)
		{
			if (gHighlightedKeybindingColumn == 0)
			{
				value1 = blinkFlux ? GetLanguageString(STR_PRESS) : "";
			}
			else
			{
				value2 = blinkFlux ? GetLanguageString(STR_PRESS) : "";
			}
		}

		SetSettingColor(isRowHighlighted);
		DrawText(caption, 150, y, .5f, 1.0f, info);
		SetSettingColor(isRowHighlighted && gHighlightedKeybindingColumn == 0);
		DrawText(value1, 320, y, .5f, 1.0f, info);
		SetSettingColor(isRowHighlighted && gHighlightedKeybindingColumn == 1);
		DrawText(value2, 400, y, .5f, 1.0f, info);

		y += SCORE_TEXT_SPACING * 1.5f;
	}

	// Draw reset button
	y += SCORE_TEXT_SPACING * 1.5f;
	SetSettingColor(gHighlightedKeybindingRow == gNumKeybindingRows - 2);
	DrawText(GetLanguageString(STR_RESET_KEYBINDINGS), 150, y, .66f, 1.0f, info);

	// Draw back button
	y += SCORE_TEXT_SPACING * 1.5f;
	SetSettingColor(gHighlightedKeybindingRow == gNumKeybindingRows - 1);
	DrawText(GetLanguageString(STR_BACK), 150, y, .66f, 1.0f, info);
}

static void DrawSettings(OGLSetupOutputType *info)
{
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

	if (gSettingsState == SETTINGS_STATE_CONTROLS_READY || gSettingsState == SETTINGS_STATE_CONTROLS_AWAITING_PRESS)
	{
		DrawKeybindingScreen(info);
	}
	else
	{
		DrawMainSettingsScreen(info);
	}

	/***********/
	/* CLEANUP */
	/***********/

	gGlobalTransparency = 1;
	OGL_PopState();
}


static void MenuBackgroundDrawRoutineWrapper(OGLSetupOutputType *info)
{
	DrawMainMenuCallback(info);
	DrawSettings(info);
}

#pragma mark -

/***************************************************************/
/*                       MAIN LOOP                             */
/***************************************************************/

void DoSettingsOverlay(void)
{
	gAllowAudioKeys = false;					// dont interfere with keyboard binding

	PlayEffect(MyRandomLong()&1? EFFECT_ACCENTDRONE1: EFFECT_ACCENTDRONE2);

	gSettingsState = SETTINGS_STATE_FADE_IN;
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

	while (gSettingsState != SETTINGS_STATE_OFF)
	{
		switch (gSettingsState)
		{
			case SETTINGS_STATE_FADE_IN:
				pane->ColorFilter.a += gFramesPerSecondFrac * 2.0f;
				if (pane->ColorFilter.a > .9f)
					pane->ColorFilter.a = .9f;

				gSettingsFadeAlpha += gFramesPerSecondFrac * 2.0f;
				if (gSettingsFadeAlpha >= 1.0f)
				{
					gSettingsFadeAlpha = 1.0f;
					gSettingsState = SETTINGS_STATE_READY;
				}
				break;

			case SETTINGS_STATE_FADE_OUT:
				pane->ColorFilter.a -= gFramesPerSecondFrac * 2.0f;
				if (pane->ColorFilter.a < 0.0f)
					pane->ColorFilter.a = 0;

				gSettingsFadeAlpha -= gFramesPerSecondFrac * 2.0f;
				if (gSettingsFadeAlpha <= 0.0f)
				{
					gSettingsFadeAlpha = 0.0f;
					gSettingsState = SETTINGS_STATE_OFF;
				}
				break;

			case SETTINGS_STATE_READY:
				NavigateSettings();
				break;

			case SETTINGS_STATE_CONTROLS_READY:
				NavigateKeybindings();
				break;

			case SETTINGS_STATE_CONTROLS_AWAITING_PRESS:
				NavigateKeybinding_AwaitNewKey();
				break;

			default:
				break;
		}

			/* DRAW STUFF */

		CalcFramesPerSecond();
		MoveObjects();
		OGL_DrawScene(gGameViewInfoPtr, MenuBackgroundDrawRoutineWrapper);
	}


	/* CLEANUP */

	DeleteObject(pane);
	UpdateInput();

	SavePrefs();

	MyFlushEvents();

	gAllowAudioKeys = true;
}
