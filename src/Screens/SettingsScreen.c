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

static void LayOutMainSettingsScreen(void);
static void LayOutKeybindingScreen(void);

#define		SpecialHighlightType	Special[0]
#define		SpecialHighlightRow		Special[1]
#define		SpecialHighlightColumn	Special[2]


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

static const OGLColorRGBA kTitleColor = {1.0f, 1.0f, 0.7f, 1.0f};
static const OGLColorRGBA kInactiveColor = {.3f, .5f, .2f, 1.0f};

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
	{
		&gGamePrefs.anaglyph,
		STR_ANAGLYPH,
		NULL,
		2, {STR_OFF, STR_ON},
		0,
		false,
	},
	{
		NULL,
		STR_CONFIGURE_CONTROLS,
		cb_ConfigureControls,
		0, {},
		0,
		true,
	},
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

static ObjNode* gChainHead = nil;

#pragma mark -

/***************************************************************/
/*                       CALLBACKS                             */
/***************************************************************/

static void cb_SetLanguage(void)
{
	LoadLocalizedStrings(gGamePrefs.language);
}

static void cb_ConfigureControls(void)
{
	gSettingsState = SETTINGS_STATE_CONTROLS_READY;
	PlayEffect(EFFECT_MENUCHANGE);
	LayOutKeybindingScreen();
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
			PlayEffect(EFFECT_MENUCHANGE);
			LayOutMainSettingsScreen();
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

		if (gSettingsState == SETTINGS_STATE_READY)
		{
			LayOutMainSettingsScreen();
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
				LayOutKeybindingScreen();
			}
		}

		if (valueActivated)
		{
			MyFlushEvents();
			PlayEffect(EFFECT_MENUCHANGE);
			gSettingsState = SETTINGS_STATE_CONTROLS_AWAITING_PRESS;
			LayOutKeybindingScreen();
		}
	}
	else if (valueActivated && gHighlightedKeybindingRow == NUM_CONTROL_NEEDS + 0)		// reset
	{
		MyFlushEvents();
		PlayEffect(EFFECT_FLAREEXPLODE);
		memcpy(gGamePrefs.keys, gDefaultKeyBindings, sizeof(gDefaultKeyBindings));
		_Static_assert(sizeof(gDefaultKeyBindings) == sizeof(gGamePrefs.keys), "size mismatch: default keybindings / prefs keybindings");
		LayOutKeybindingScreen();
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
		LayOutKeybindingScreen();
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
				LayOutKeybindingScreen();
				break;
			}
		}
	}
}

#pragma mark -

/***************************************************************/
/*                          LAYOUT                             */
/***************************************************************/

static void NukeTextNodes()
{
	if (gChainHead)
	{
		DeleteObject(gChainHead);
		gChainHead = nil;
	}
}

static void MoveText(ObjNode* node)
{
	if (gSettingsState == SETTINGS_STATE_OFF)
	{
		DeleteObject(node);
		return;
	}

	int effect = 0;
	float intensity = 1;

	if (node->SpecialHighlightType == 1)						// it's a selectable node
	{
		if (node->SpecialHighlightRow == gHighlightedSettingRow)
			effect = 2;
		else
			effect = 1;
	}
	else if (node->SpecialHighlightType == 2)
	{
		if (node->SpecialHighlightRow == gHighlightedKeybindingRow &&
			(node->SpecialHighlightColumn < 0 || node->SpecialHighlightColumn == gHighlightedKeybindingColumn))
		{
			if (gSettingsState == SETTINGS_STATE_CONTROLS_AWAITING_PRESS)
				effect = 3;
			else
				effect = 2;
		}
		else
			effect = 1;
	}

	if (effect == 1)
		node->ColorFilter = kInactiveColor;
	else if (effect == 2)
	{
		float rf = .7f + RandomFloat() * .29f;
		node->ColorFilter = (OGLColorRGBA){rf, rf, rf, 1};
	}
	else if (effect == 3)
	{
		node->SpecialF[0] += gFramesPerSecondFrac * 10.0f;
		intensity = (1.0f + sinf(node->SpecialF[0])) * 0.5f;
		//node->ColorFilter = (OGLColorRGBA){1,1,1,intensity};
	}

	node->ColorFilter.a = gSettingsFadeAlpha * intensity;
}

static void LayOutMainSettingsScreen(void)
{
	NukeTextNodes();

	gNewObjectDefinition.coord		= (OGLPoint3D){-170, -160, 0};
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.moveCall 	= MoveText;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = 2.0f;
	gNewObjectDefinition.slot 		= SPRITE_SLOT;
	ObjNode* title = TextMesh_New(Localize(STR_SETTINGS), 0, &gNewObjectDefinition);
	title->ColorFilter = kTitleColor;

	gChainHead = title;
	gNewObjectDefinition.autoChain = gChainHead;

	gNewObjectDefinition.coord.y += SCORE_TEXT_SPACING * 5.0f;
	gNewObjectDefinition.scale		= .75f;

	// Draw setting rows
	for (int settingID = 0; settingID < gNumSettingRows; settingID++)
	{
		const SettingEntry* entry = &gSettingEntries[settingID];

		if (entry->extraPadding)								// add extra padding above if needed
		{
			gNewObjectDefinition.coord.y += SCORE_TEXT_SPACING * 1.5f;
		}

		gNewObjectDefinition.coord.x = -170;
		ObjNode* captionNode = TextMesh_New(Localize(entry->labelStrID), 0, &gNewObjectDefinition);

		captionNode->SpecialHighlightType = 1;
		captionNode->SpecialHighlightRow = settingID;

		if (entry->valuePtr)
		{
			int valueStrID = 0;
			if (entry->valuePtr == &gGamePrefs.language)
				valueStrID = STR_LANGUAGE_NAME;
			else
				valueStrID = entry->choiceStrID[*entry->valuePtr];

			gNewObjectDefinition.coord.x = 30;
			ObjNode* valueNode = TextMesh_New(Localize(valueStrID), 0, &gNewObjectDefinition);
			valueNode->SpecialHighlightType = 1;
			valueNode->SpecialHighlightRow = settingID;
		}

		gNewObjectDefinition.coord.y += SCORE_TEXT_SPACING * 1.5f;
	}

	gNewObjectDefinition.autoChain = nil;
}

static void LayOutKeybindingScreen(void)
{
	NukeTextNodes();

	gNewObjectDefinition.coord		= (OGLPoint3D){-170, -160, 0};
	gNewObjectDefinition.flags		= 0;
	gNewObjectDefinition.moveCall	= MoveText;
	gNewObjectDefinition.rot		= 0;
	gNewObjectDefinition.scale		= 2.0f;
	gNewObjectDefinition.slot		= SPRITE_SLOT;
	ObjNode* title = TextMesh_New(Localize(STR_CONFIGURE_CONTROLS), 0, &gNewObjectDefinition);
	title->ColorFilter = kTitleColor;

	gChainHead = title;
	gNewObjectDefinition.autoChain = gChainHead;

	gNewObjectDefinition.coord.y += SCORE_TEXT_SPACING * 5.0f;
	gNewObjectDefinition.scale		= .6f;

	// Draw binding text
	for (int bindingID = 0; bindingID < NUM_CONTROL_NEEDS; bindingID++)
	{
		const char* caption = Localize(STR_KEYBINDING_DESCRIPTION_0 + bindingID);
		const char* value1 = "---";
		const char* value2 = "---";

		if (gGamePrefs.keys[bindingID].key1 != 0)
			value1 = SDL_GetScancodeName(gGamePrefs.keys[bindingID].key1);

		if (gGamePrefs.keys[bindingID].key2 != 0)
			value2 = SDL_GetScancodeName(gGamePrefs.keys[bindingID].key2);

		if (gHighlightedKeybindingRow == bindingID && gSettingsState == SETTINGS_STATE_CONTROLS_AWAITING_PRESS)
		{
			if (gHighlightedKeybindingColumn == 0)
				value1 = Localize(STR_PRESS);
			else
				value2 = Localize(STR_PRESS);
		}

		gNewObjectDefinition.coord.x = -170;
		ObjNode* captionNode = TextMesh_New(caption, 0, &gNewObjectDefinition);
		captionNode->ColorFilter = kInactiveColor;

		gNewObjectDefinition.coord.x = 0;
		ObjNode* valueNode1 = TextMesh_NewEmpty(256, &gNewObjectDefinition);
		TextMesh_Update(value1, 0, valueNode1);
		valueNode1->SpecialHighlightType	= 2;
		valueNode1->SpecialHighlightRow		= bindingID;
		valueNode1->SpecialHighlightColumn	= 0;

		gNewObjectDefinition.coord.x = 120;
		ObjNode* valueNode2 = TextMesh_NewEmpty(256, &gNewObjectDefinition);
		TextMesh_Update(value2, 0, valueNode2);
		valueNode2->SpecialHighlightType	= 2;
		valueNode2->SpecialHighlightRow		= bindingID;
		valueNode2->SpecialHighlightColumn	= 1;

		gNewObjectDefinition.coord.y += SCORE_TEXT_SPACING * 1.5f;
	}

	// Reset button
	gNewObjectDefinition.coord.x = -170;
	gNewObjectDefinition.coord.y += SCORE_TEXT_SPACING * 1.5f;
	ObjNode* resetButton = TextMesh_New(Localize(STR_RESET_KEYBINDINGS), 0, &gNewObjectDefinition);
	resetButton->SpecialHighlightType	= 2;
	resetButton->SpecialHighlightRow	= gNumKeybindingRows - 2;
	resetButton->SpecialHighlightColumn	= -1;	// ignore column to highlight

	// Back button
	gNewObjectDefinition.coord.y += SCORE_TEXT_SPACING * 1.5f;
	ObjNode* backButton = TextMesh_New(Localize(STR_BACK), 0, &gNewObjectDefinition);
	backButton->SpecialHighlightType	= 2;
	backButton->SpecialHighlightRow		= gNumKeybindingRows - 1;
	backButton->SpecialHighlightColumn	= -1;	// ignore column to highlight

	gNewObjectDefinition.autoChain = nil;
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
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+100-1;
	gNewObjectDefinition.moveCall 	= nil;
	ObjNode* pane = MakeNewObject(&gNewObjectDefinition);
	pane->CustomDrawFunction = DrawDarkenPane;
	pane->ColorFilter.r = 0;
	pane->ColorFilter.g = 0;
	pane->ColorFilter.b = 0;
	pane->ColorFilter.a = 0;


	LayOutMainSettingsScreen();


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
		OGL_DrawScene(gGameViewInfoPtr, DrawMainMenuCallback);
	}


	/* CLEANUP */

	DeleteObject(pane);
	UpdateInput();

	SavePrefs();

	MyFlushEvents();

	gAllowAudioKeys = true;
	gChainHead = nil;
}
