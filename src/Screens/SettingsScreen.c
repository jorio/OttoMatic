// OTTO MATIC SETTINGS
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

/****************************/
/*    EXTERNALS            */
/****************************/

extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	float					gFramesPerSecondFrac,gFramesPerSecond,gGlobalTransparency;
extern	OGLColorRGB				gGlobalColorFilter;
extern	SDL_Window				*gSDLWindow;

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
	SETTINGS_STATE_FADE_OUT,
};

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
	/*
	{
		&gGamePrefs.anaglyph,
		STR_ANAGLYPH,
		NULL,
		2, {STR_OFF, STR_ON},
		0,
		false,
	},
	*/
	/*
	// Configure controls
	{
		NULL,
		STR_CONFIGURE_CONTROLS,
		cb_ConfigureControls,
		0, {},
		0,
		true,
	},
	*/
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

static int gSettingsMenuSelection = 0;
static int gSettingsMenuCount = 0;

static float gSettingsFadeAlpha = 0;
static int gSettingsState = SETTINGS_STATE_OFF;


/****************************/
/*    CALLBACKS            */
/****************************/

static void cb_SetLanguage(void)
{
	LoadLanguageStrings(gGamePrefs.language);
}

static void cb_ConfigureControls(void)
{
	DoAlert("Implement me");
}

static void cb_Back(void)
{
	gSettingsState = SETTINGS_STATE_FADE_OUT;
}

/***************************************************************/


static unsigned int PositiveModulo(int value, unsigned int m)
{
	int mod = value % (int) m;
	if (mod < 0)
	{
		mod += m;
	}
	return mod;
}

static void NavigateSettings(void)
{
	UpdateInput();

	bool valueActivated = false;
	int valueDelta = 0;

	if (GetNewKeyState(SDL_SCANCODE_ESCAPE))
	{
		cb_Back();
	}
	else if (GetNewKeyState(SDL_SCANCODE_UP) && (gSettingsMenuSelection > 0))
	{
		gSettingsMenuSelection--;
		PlayEffect(EFFECT_WEAPONCLICK);
	}
	else if (GetNewKeyState(SDL_SCANCODE_DOWN) && (gSettingsMenuSelection < gSettingsMenuCount - 1))
	{
		gSettingsMenuSelection++;
		PlayEffect(EFFECT_WEAPONCLICK);
	}


	const SettingEntry* entry = &gSettingEntries[gSettingsMenuSelection];
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


//	if (valueActivated || valueDelta)
//		MyFlushEvents();
}

static void DrawSettingsText(int stringID, float x, float y, OGLSetupOutputType *info)
{
	const float xs = .66f;

	for (const char* c = GetLanguageString(stringID); *c; c++)
	{
		float cw = SCORE_TEXT_SPACING * xs;

		int texNum = CharToSprite(*c);				// get texture #
		if (texNum != -1)
		{
			DrawInfobarSprite2_Scaled(
					x + cw/2.0f,
					y,
					SCORE_TEXT_SPACING * 2.0f * xs,
					SCORE_TEXT_SPACING * 2.0f,
					SPRITE_GROUP_FONT,
					texNum,
					info);
		}
		x += cw;
	}
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

	float y = 120;

	for (int settingID = 0; settingID < gSettingsMenuCount; settingID++)
	{
		const SettingEntry* entry = &gSettingEntries[settingID];

		if (entry->extraPadding)								// add extra padding above if needed
		{
			y += SCORE_TEXT_SPACING * 2;
		}

		if (gSettingsMenuSelection == settingID)				// highlight
		{
			float rf = .7f + RandomFloat() * .29f;
			gGlobalColorFilter = (OGLColorRGB){rf, rf, rf};
		}
		else													// no highlight
		{
			gGlobalColorFilter = (OGLColorRGB){.3, .5, .2};
		}

		DrawSettingsText(entry->labelStrID, 150, y, info);		// draw text

		if (entry->valuePtr)
		{
			int valueStrID = 0;
			if (entry->valuePtr == &gGamePrefs.language)
				valueStrID = STR_LANGUAGE_NAME;
			else
				valueStrID = entry->choiceStrID[*entry->valuePtr];
			DrawSettingsText(valueStrID, 350, y, info);
		}

		y += SCORE_TEXT_SPACING * 1.5f;
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

void DoSettingsOverlay(void)
{
	PlayEffect(MyRandomLong()&1? EFFECT_ACCENTDRONE1: EFFECT_ACCENTDRONE2);

	gSettingsMenuCount = sizeof(gSettingEntries) / sizeof(gSettingEntries[0]);
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
}
