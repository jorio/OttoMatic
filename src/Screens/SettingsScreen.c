// OTTO MATIC SETTINGS
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

/****************************/
/*    EXTERNALS            */
/****************************/

#include "game.h"

/***************************************************************/
/*                       CALLBACKS                             */
/***************************************************************/

static void cb_SetLanguage(void)
{
	LoadLocalizedStrings(gGamePrefs.language);
	LayoutCurrentMenuAgain();
}

static void cb_SetRumble(void)
{
	if (gGamePrefs.gamepadRumble)
	{
		Rumble(1.0f, 500);
	}
}

static void cb_ResetKeyBindings(void)
{
	for (int i = 0; i < NUM_REMAPPABLE_NEEDS; i++)
	{
		memcpy(gGamePrefs.remappableKeys[i].key, kDefaultKeyBindings[i].key, sizeof(gGamePrefs.remappableKeys[i].key));
	}

	MyFlushEvents();
	PlayEffect(EFFECT_FLAREEXPLODE);
	LayoutCurrentMenuAgain();
}

static void cb_ResetPadBindings(void)
{
	for (int i = 0; i < NUM_REMAPPABLE_NEEDS; i++)
	{
		memcpy(gGamePrefs.remappableKeys[i].gamepad, kDefaultKeyBindings[i].gamepad, sizeof(gGamePrefs.remappableKeys[i].gamepad));
	}

	MyFlushEvents();
	PlayEffect(EFFECT_FLAREEXPLODE);
	LayoutCurrentMenuAgain();
}

static void cb_ResetMouseBindings(void)
{
	for (int i = 0; i < NUM_REMAPPABLE_NEEDS; i++)
	{
		gGamePrefs.remappableKeys[i].mouseButton = kDefaultKeyBindings[i].mouseButton;
	}

	MyFlushEvents();
	PlayEffect(EFFECT_FLAREEXPLODE);
	LayoutCurrentMenuAgain();
}

static const char* GenerateGamepadLabel(void)
{
	if (gSDLController)
		return SDL_GameControllerName(gSDLController);
	else
		return Localize(STR_NO_GAMEPAD_DETECTED);
}

static uint8_t GenerateNumDisplays(void)
{
	int numDisplays = SDL_GetNumVideoDisplays();
	return ClampInt(numDisplays, 1, 255);
}

static const char* GenerateDisplayName(char* buf, int bufSize, Byte value)
{
	snprintf(buf, bufSize, "%s %d", Localize(STR_DISPLAY), 1 + (int)value);
	return buf;
}

static const char* GenerateCurrentLanguageName(char* buf, int bufSize, Byte value)
{
	return Localize(STR_LANGUAGE_NAME);
}

static void cb_ChangeAnaglyphMode(void)
{
	gAnaglyphPass = 0;
	for (int i = 0; i < 4; i++)
	{
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glClearColor(0,0,0,1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		SDL_GL_SwapWindow(gSDLWindow);
	}
}

/***************************************************************/
/*                     MENU DEFINITIONS                        */
/***************************************************************/

static const MenuItem gKeybindingMenu[] =
{
	{.type = kMenuItem_Title, .text = STR_CONFIGURE_KEYBOARD},
	{.type = kMenuItem_Subtitle, .text = STR_CONFIGURE_KEYBOARD_HELP},
	{.type = kMenuItem_Spacer},

	{ .type = kMenuItem_KeyBinding, .kb = kNeed_Forward },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_Backward },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_TurnLeft },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_TurnRight },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_Jump },

	{ .type = kMenuItem_Spacer },

	{ .type = kMenuItem_KeyBinding, .kb = kNeed_Shoot },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_PunchPickup },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_PrevWeapon },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_NextWeapon },

	{ .type = kMenuItem_Spacer },

	{ .type = kMenuItem_KeyBinding, .kb = kNeed_CameraLeft },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_CameraRight },
	{ .type = kMenuItem_KeyBinding, .kb = kNeed_CameraMode },

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Action,
		.text = STR_RESET_KEYBINDINGS,
		.action = { .callback = cb_ResetKeyBindings },
	},

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Action,
		.text = STR_BACK,
		.action = { .callback = MenuCallback_Back },
	},

	{ .type = kMenuItem_END_SENTINEL }
};

static const MenuItem gGamepadMenu[] =
{
	{.type = kMenuItem_Title, .text = STR_CONFIGURE_GAMEPAD},
	{.type = kMenuItem_Subtitle, .generateText = GenerateGamepadLabel },
	{.type = kMenuItem_Subtitle, .text = STR_CONFIGURE_GAMEPAD_HELP},
	{.type = kMenuItem_Spacer },

	{ .type = kMenuItem_PadBinding, .kb = kNeed_Jump },
	{ .type = kMenuItem_PadBinding, .kb = kNeed_Shoot },
	{ .type = kMenuItem_PadBinding, .kb = kNeed_PunchPickup },
	{ .type = kMenuItem_PadBinding, .kb = kNeed_PrevWeapon },
	{ .type = kMenuItem_PadBinding, .kb = kNeed_NextWeapon },
	{ .type = kMenuItem_PadBinding, .kb = kNeed_CameraMode },

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Action,
		.text = STR_RESET_KEYBINDINGS,
		.action = { .callback = cb_ResetPadBindings },
	},

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Cycler,
		.text = STR_GAMEPAD_RUMBLE,
		.cycler =
		{
			.callback = cb_SetRumble,
			.valuePtr = &gGamePrefs.gamepadRumble,
			.numChoices = 2,
			.choices = {STR_OFF, STR_ON},
		},
	},

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Action,
		.text = STR_BACK,
		.action = { .callback = MenuCallback_Back },
	},

	{ .type = kMenuItem_END_SENTINEL }
};

static const MenuItem gMouseMenu[] =
{
	{.type = kMenuItem_Title, .text = STR_CONFIGURE_MOUSE},
	{.type = kMenuItem_Spacer},

	{
		.type = kMenuItem_Cycler,
		.text = STR_MOUSE_CONTROL_TYPE,
		.cycler =
		{
			.valuePtr = &gGamePrefs.mouseControlsOtto,
			.numChoices = 2,
			.choices = {STR_MOUSE_CONTROLS_CAMERA, STR_MOUSE_CONTROLS_OTTO},
		},
	},

	{
		.type = kMenuItem_Cycler,
		.text = STR_MOUSE_SENSITIVITY,
		.cycler =
		{
			.valuePtr = &gGamePrefs.mouseSensitivityLevel,
			.numChoices = NUM_MOUSE_SENSITIVITY_LEVELS,
			.choices =
			{
				STR_MOUSE_SENSITIVITY_1,
				STR_MOUSE_SENSITIVITY_2,
				STR_MOUSE_SENSITIVITY_3,
				STR_MOUSE_SENSITIVITY_4,
				STR_MOUSE_SENSITIVITY_5,
				STR_MOUSE_SENSITIVITY_6,
				STR_MOUSE_SENSITIVITY_7,
				STR_MOUSE_SENSITIVITY_8,
			},
		},
	},

	{ .type = kMenuItem_Spacer },

	{ .type = kMenuItem_MouseBinding, .kb = kNeed_Shoot },
	{ .type = kMenuItem_MouseBinding, .kb = kNeed_Jump },
	{ .type = kMenuItem_MouseBinding, .kb = kNeed_PunchPickup },
	{ .type = kMenuItem_MouseBinding, .kb = kNeed_PrevWeapon },
	{ .type = kMenuItem_MouseBinding, .kb = kNeed_NextWeapon },
	{ .type = kMenuItem_MouseBinding, .kb = kNeed_CameraMode },
	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Action,
		.text = STR_RESET_KEYBINDINGS,
		.action = { .callback = cb_ResetMouseBindings },
	},

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Action,
		.text = STR_BACK,
		.action = { .callback = MenuCallback_Back },
	},

	{ .type = kMenuItem_END_SENTINEL }
};

static const MenuItem gSettingsMenu[] =
{
	{.type = kMenuItem_Title, .text = STR_SETTINGS},
	{.type = kMenuItem_Spacer},


	{
		.type = kMenuItem_Submenu,
		.text = STR_CONFIGURE_KEYBOARD,
		.submenu = {.menu = gKeybindingMenu},
	},

	{
		.type = kMenuItem_Submenu,
		.text = STR_CONFIGURE_MOUSE,
		.submenu = {.menu = gMouseMenu},
	},

	{
		.type = kMenuItem_Submenu,
		.text = STR_CONFIGURE_GAMEPAD,
		.submenu = {.menu = gGamepadMenu},
	},

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Cycler,
		.text = STR_MUSIC,
		.cycler =
		{
			.callback = EnforceMusicPausePref,
			.valuePtr = &gGamePrefs.music,
			.numChoices = 2,
			.choices = {STR_OFF, STR_ON},
		},
	},

	{
		.type = kMenuItem_Cycler,
		.text = STR_AUTO_ALIGN_CAMERA,
		.cycler =
		{
			.valuePtr = &gGamePrefs.autoAlignCamera,
			.numChoices = 2,
			.choices = {STR_OFF, STR_ON},
		},
	},

#if _DEBUG
	{
		.type = kMenuItem_Cycler,
		.rawText = "Tank controls",
		.cycler =
		{
			.valuePtr = &gGamePrefs.playerRelControls,
			.numChoices = 2,
			.choices = {STR_OFF, STR_ON},
		},
	},
#endif

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Cycler,
		.text = STR_LANGUAGE,
		.cycler =
		{
			.callback = cb_SetLanguage,
			.valuePtr = &gGamePrefs.language,
			.numChoices = MAX_LANGUAGES,
			.generateChoiceString = GenerateCurrentLanguageName,
		},
	},

	{
		.type = kMenuItem_Cycler,
		.text= STR_UI_CENTERING,
		.cycler =
		{
			.valuePtr = &gGamePrefs.uiCentering,
			.numChoices = 2,
			.choices = {STR_OFF, STR_ON},
		},
	},
	{
		.type = kMenuItem_Cycler,
		.text = STR_UI_SCALE,
		.cycler =
		{
			.valuePtr = &gGamePrefs.uiScaleLevel,
			.numChoices = NUM_UI_SCALE_LEVELS,
			.choices =
			{
				STR_MOUSE_SENSITIVITY_1,
				STR_MOUSE_SENSITIVITY_2,
				STR_MOUSE_SENSITIVITY_3,
				STR_MOUSE_SENSITIVITY_4,
				STR_MOUSE_SENSITIVITY_5,
				STR_MOUSE_SENSITIVITY_6,
				STR_MOUSE_SENSITIVITY_7,
				STR_MOUSE_SENSITIVITY_8,
			},
		},
	},

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Cycler,
		.text = STR_FULLSCREEN,
		.cycler =
		{
			.callback = SetFullscreenModeFromPrefs,
			.valuePtr = &gGamePrefs.fullscreen,
			.numChoices = 2,
			.choices = {STR_OFF, STR_ON},
		},
	},

	{
		.type = kMenuItem_Cycler,
		.text = STR_VSYNC,
		.cycler =
		{
			.callback = SetFullscreenModeFromPrefs,
			.valuePtr = &gGamePrefs.vsync,
			.numChoices = 2,
			.choices = {STR_OFF, STR_ON},
		},
	},

	{
		.type = kMenuItem_Cycler,
		.text = STR_PREFERRED_DISPLAY,
		.cycler =
		{
			.callback = SetFullscreenModeFromPrefs,
			.valuePtr = &gGamePrefs.preferredDisplay,
			.generateNumChoices = GenerateNumDisplays,
			.generateChoiceString = GenerateDisplayName,
		},
	},

#if !(__APPLE__)
	{
		.type = kMenuItem_Cycler,
		.text = STR_ANTIALIASING,
		.cycler =
		{
			.valuePtr = &gGamePrefs.antialiasingLevel,
			.numChoices = 4,
			.choices = {STR_OFF, STR_MSAA_2X, STR_MSAA_4X, STR_MSAA_8X},
		}
	},
#endif

	{
		.type = kMenuItem_Cycler,
		.text = STR_ANAGLYPH,
		.cycler =
		{
			.callback = cb_ChangeAnaglyphMode,
			.valuePtr = &gGamePrefs.anaglyphMode,
			.numChoices = 3,
			.choices = {STR_OFF, STR_ON_COLOR, STR_ON_MONOCHROME},
		},
	},

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Action,
		.text = STR_BACK,
		.action = { .callback = MenuCallback_Back },
	},

	{ .type = kMenuItem_END_SENTINEL }
};

static const MenuItem kAntialiasingWarning[] =
{
	{ .type = kMenuItem_Label, .text = STR_ANTIALIASING_CHANGE_WARNING_1 },
	{ .type = kMenuItem_Label, .text = STR_ANTIALIASING_CHANGE_WARNING_2 },
	{ .type = kMenuItem_Spacer },
	{ .type = kMenuItem_Action, .text = STR_OK, .action = { .callback = MenuCallback_Back } },
	{ .type = kMenuItem_END_SENTINEL },
};

static const MenuItem kAnaglyphWarning[] =
{
	{ .type = kMenuItem_Label, .text = STR_ANAGLYPH_TOGGLE_WARNING_1 },
	{ .type = kMenuItem_Spacer },
	{ .type = kMenuItem_Label, .text = STR_ANAGLYPH_TOGGLE_WARNING_2 },
	{ .type = kMenuItem_Label, .text = STR_ANAGLYPH_TOGGLE_WARNING_3 },
	{ .type = kMenuItem_Spacer },
	{ .type = kMenuItem_Action, .text = STR_OK, .action = { .callback = MenuCallback_Back } },
	{ .type = kMenuItem_END_SENTINEL },
};

#pragma mark -

/***************************************************************/
/*                          RUNNER                             */
/***************************************************************/

void DoSettingsOverlay(void (*updateRoutine)(void),
					   void (*backgroundDrawRoutine)(void))
{
	gAllowAudioKeys = false;					// don't interfere with keyboard binding

//	PlayEffect(MyRandomLong()&1? EFFECT_ACCENTDRONE1: EFFECT_ACCENTDRONE2);

	PrefsType gPreviousPrefs = gGamePrefs;

	StartMenu(gSettingsMenu, nil, updateRoutine, backgroundDrawRoutine);

	// Save prefs if any changes
	if (0 != memcmp(&gGamePrefs, &gPreviousPrefs, sizeof(gGamePrefs)))
		SavePrefs();

	gAllowAudioKeys = true;

	// If user changed antialiasing setting, show warning
	if (gPreviousPrefs.antialiasingLevel != gGamePrefs.antialiasingLevel)
	{
		StartMenu(kAntialiasingWarning, nil, updateRoutine, backgroundDrawRoutine);
	}

	// If user changed anaglyph setting, show warning
	if (gPreviousPrefs.anaglyphMode != gGamePrefs.anaglyphMode)
	{
		StartMenu(kAnaglyphWarning, nil, updateRoutine, backgroundDrawRoutine);
	}
}
