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
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		memcpy(gGamePrefs.keys[i].key, kDefaultKeyBindings[i].key, sizeof(gGamePrefs.keys[i].key));
	}

	MyFlushEvents();
	PlayEffect(EFFECT_FLAREEXPLODE);
	LayoutCurrentMenuAgain();
}

static void cb_ResetPadBindings(void)
{
	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		memcpy(gGamePrefs.keys[i].gamepad, kDefaultKeyBindings[i].gamepad, sizeof(gGamePrefs.keys[i].gamepad));
	}

	MyFlushEvents();
	PlayEffect(EFFECT_FLAREEXPLODE);
	LayoutCurrentMenuAgain();
}

/***************************************************************/
/*                     MENU DEFINITIONS                        */
/***************************************************************/

static const MenuItem gKeybindingMenu[] =
{
	{.type = kMenuItem_Title, .text = STR_CONFIGURE_KEYBOARD},

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

	{
		.type = kMenuItem_Cycler,
		.text = STR_LANGUAGE,
		.cycler =
		{
			.callback = cb_SetLanguage,
			.valuePtr = &gGamePrefs.language,
			.numChoices = MAX_LANGUAGES,
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
		.text = STR_ANAGLYPH,
		.cycler =
		{
			.callback = NULL,
			.valuePtr = &gGamePrefs.anaglyph,
			.numChoices = 2,
			.choices = {STR_OFF, STR_ON},
		},
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

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Submenu,
		.text = STR_CONFIGURE_KEYBOARD,
		.submenu = {.menu = gKeybindingMenu},
	},

	{
		.type = kMenuItem_Submenu,
		.text = STR_CONFIGURE_GAMEPAD,
		.submenu = {.menu = gGamepadMenu},
	},

	{ .type = kMenuItem_Spacer },

	{
		.type = kMenuItem_Action,
		.text = STR_BACK,
		.action = { .callback = MenuCallback_Back },
	},

	{ .type = kMenuItem_END_SENTINEL }
};

#pragma mark -

/***************************************************************/
/*                          RUNNER                             */
/***************************************************************/

void DoSettingsOverlay(void (*updateRoutine)(void),
					   void (*backgroundDrawRoutine)(OGLSetupOutputType *))
{
	gAllowAudioKeys = false;					// dont interfere with keyboard binding

//	PlayEffect(MyRandomLong()&1? EFFECT_ACCENTDRONE1: EFFECT_ACCENTDRONE2);

	PrefsType gPreviousPrefs = gGamePrefs;

	StartMenu(gSettingsMenu, nil, updateRoutine, backgroundDrawRoutine);

	// Save prefs if any changes
	if (0 != memcmp(&gGamePrefs, &gPreviousPrefs, sizeof(gGamePrefs)))
		SavePrefs();

	gAllowAudioKeys = true;
}
