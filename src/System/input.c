// INPUT.C
// (c)2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

/***************/
/* CONSTANTS   */
/***************/

enum
{
	KEYSTATE_OFF		= 0b00,
	KEYSTATE_UP			= 0b01,

	KEYSTATE_PRESSED	= 0b10,
	KEYSTATE_HELD		= 0b11,

	KEYSTATE_ACTIVE_BIT	= 0b10,
};

/***************/
/* EXTERNALS   */
/***************/

extern	SDL_GLContext		gAGLContext;
extern	float			gFramesPerSecondFrac,gFramesPerSecond,gScratchF;
extern	PrefsType			gGamePrefs;

/**********************/
/*     PROTOTYPES     */
/**********************/

Byte				gRawKeyboardState[SDL_NUM_SCANCODES];
bool				gAnyNewKeysPressed = false;

Byte				gNeedStates[NUM_CONTROL_NEEDS];

			/**************/
			/* NEEDS LIST */
			/**************/

#if __APPLE__
	#define DEFAULT_SHOOT_SC1 SDL_SCANCODE_LGUI
	#define DEFAULT_SHOOT_SC2 SDL_SCANCODE_RGUI
#else
	#define DEFAULT_SHOOT_SC1 SDL_SCANCODE_LCTRL
	#define DEFAULT_SHOOT_SC2 SDL_SCANCODE_RCTRL
#endif

const KeyBinding gDefaultKeyBindings[NUM_CONTROL_NEEDS] =
{
[kNeed_Forward		] = {SDL_SCANCODE_UP,		SDL_SCANCODE_W,			0,					SDL_CONTROLLER_BUTTON_DPAD_UP, },
[kNeed_Backward		] = {SDL_SCANCODE_DOWN,		SDL_SCANCODE_S,			0,					SDL_CONTROLLER_BUTTON_DPAD_DOWN, },
[kNeed_TurnLeft		] = {SDL_SCANCODE_LEFT,		SDL_SCANCODE_A,			0,					SDL_CONTROLLER_BUTTON_DPAD_LEFT, },
[kNeed_TurnRight	] = {SDL_SCANCODE_RIGHT,	SDL_SCANCODE_D,			0,					SDL_CONTROLLER_BUTTON_DPAD_RIGHT, },
[kNeed_NextWeapon	] = {SDL_SCANCODE_LSHIFT,	SDL_SCANCODE_RSHIFT,	SDL_BUTTON_MIDDLE,	SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, },
[kNeed_Shoot		] = {DEFAULT_SHOOT_SC1,		DEFAULT_SHOOT_SC2,		SDL_BUTTON_LEFT,	SDL_CONTROLLER_BUTTON_X, },
[kNeed_PunchPickup	] = {SDL_SCANCODE_LALT,		SDL_SCANCODE_RALT,		SDL_BUTTON_RIGHT,	SDL_CONTROLLER_BUTTON_B, },
[kNeed_Jump			] = {SDL_SCANCODE_SPACE,	0,						0,					SDL_CONTROLLER_BUTTON_A, },
[kNeed_CameraMode	] = {SDL_SCANCODE_TAB,		0,						0,					SDL_CONTROLLER_BUTTON_RIGHTSTICK, },
[kNeed_CameraLeft	] = {SDL_SCANCODE_COMMA,	0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
[kNeed_CameraRight	] = {SDL_SCANCODE_PERIOD,	0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
//[kNeed_Pause			] = { "Pause",				SDL_SCANCODE_ESCAPE,	0,						0,					SDL_CONTROLLER_BUTTON_START, },
//[kKey_ToggleMusic		] = { "Toggle Music",		SDL_SCANCODE_M,			0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
//[kKey_ToggleFullscreen	] = { "Toggle Fullscreen",	SDL_SCANCODE_F11,		0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
//[kKey_RaiseVolume		] = { "Raise Volume",		SDL_SCANCODE_EQUALS,	0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
//[kKey_LowerVolume		] = { "Lower Volume",		SDL_SCANCODE_MINUS,		0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
//[kKey_ZoomIn			] = { "Zoom In",			SDL_SCANCODE_2,			0,						0,					SDL_CONTROLLER_BUTTON_LEFTSHOULDER, },
//[kKey_ZoomOut			] = { "Zoom Out",			SDL_SCANCODE_1,			0,						0,					SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, },
//[kKey_UI_Confirm		] = { "DO_NOT_REBIND",		SDL_SCANCODE_RETURN,	SDL_SCANCODE_KP_ENTER,	0,					SDL_CONTROLLER_BUTTON_INVALID, },
//[kKey_UI_Cancel			] = { "DO_NOT_REBIND",		SDL_SCANCODE_ESCAPE,	0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
//[kKey_UI_Skip			] = { "DO_NOT_REBIND",		SDL_SCANCODE_SPACE,		0,						0,					SDL_CONTROLLER_BUTTON_INVALID, },
};

/**********************/
/* STATIC FUNCTIONS   */
/**********************/


static inline void UpdateKeyState(Byte* state, bool downNow)
{
	switch (*state)	// look at prev state
	{
		case KEYSTATE_HELD:
		case KEYSTATE_PRESSED:
			*state = downNow ? KEYSTATE_HELD : KEYSTATE_UP;
			break;
		case KEYSTATE_OFF:
		case KEYSTATE_UP:
		default:
			*state = downNow ? KEYSTATE_PRESSED : KEYSTATE_OFF;
			break;
	}
}

/**********************/
/* IMPLEMENTATION     */
/**********************/

void InitInput(void)
{
	SOURCE_PORT_MINOR_PLACEHOLDER();
}

void UpdateInput(void)
{
	SDL_PumpEvents();
	int numkeys = 0;
	const UInt8* keystate = SDL_GetKeyboardState(&numkeys);
	uint32_t mouseButtons = SDL_GetMouseState(NULL, NULL);

	gAnyNewKeysPressed = false;

	{
		int minNumKeys = numkeys < SDL_NUM_SCANCODES ? numkeys : SDL_NUM_SCANCODES;

		for (int i = 0; i < minNumKeys; i++)
		{
			UpdateKeyState(&gRawKeyboardState[i], keystate[i]);
			if (gRawKeyboardState[i] == KEYSTATE_PRESSED)
				gAnyNewKeysPressed = true;
		}

		// fill out the rest
		for (int i = minNumKeys; i < SDL_NUM_SCANCODES; i++)
			UpdateKeyState(&gRawKeyboardState[i], false);
	}

	// --------------------------------------------


	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		const KeyBinding* kb = &gGamePrefs.keys[i];

		bool downNow = false;

		if (kb->key1 && kb->key1 < numkeys)
			downNow |= 0 != keystate[kb->key1];

		if (kb->key2 && kb->key2 < numkeys)
			downNow |= 0 != keystate[kb->key2];

		if (kb->mouseButton)
			downNow |= 0 != (mouseButtons & SDL_BUTTON(kb->mouseButton));

//		if (gSDLController && kb->gamepadButton != SDL_CONTROLLER_BUTTON_INVALID)
//			downNow |= 0 != SDL_GameControllerGetButton(gSDLController, kb->gamepadButton);

		UpdateKeyState(&gNeedStates[i], downNow);
	}


	// -------------------------------------------


			/****************************/
			/* SET PLAYER AXIS CONTROLS */
			/****************************/

	gPlayerInfo.analogControlX = 											// assume no control input
	gPlayerInfo.analogControlZ = 0.0f;

			/* FIRST CHECK ANALOG AXES */

			/*
	analog = gControlNeeds[kNeed_XAxis].value;
	if (analog != 0.0f)														// is X-Axis being used?
	{
		analog *= 1.0f / AXIS_RANGE;										// convert to  -1.0 to 1.0 range
		gPlayerInfo.analogControlX = analog;
	}

	analog = gControlNeeds[kNeed_YAxis].value;
	if (analog != 0.0f)														// is Y-Axis being used?
	{
		analog *= 1.0f / AXIS_RANGE;										// convert to  -1.0 to 1.0 range
		gPlayerInfo.analogControlZ = analog;
	}
			 */

			/* NEXT CHECK THE DIGITAL KEYS */


	if (GetNeedState(kNeed_TurnLeft))							// is Left Key pressed?
		gPlayerInfo.analogControlX = -1.0f;
	else
	if (GetNeedState(kNeed_TurnRight))						// is Right Key pressed?
		gPlayerInfo.analogControlX = 1.0f;


	if (GetNeedState(kNeed_Forward))							// is Up Key pressed?
		gPlayerInfo.analogControlZ = -1.0f;
	else
	if (GetNeedState(kNeed_Backward))						// is Down Key pressed?
		gPlayerInfo.analogControlZ = 1.0f;



		/* AND FINALLY SEE IF MOUSE DELTAS ARE BEST */

		/*
	mouseDX = gMouseDeltaX * 0.015f;											// scale down deltas for our use
	mouseDY = gMouseDeltaY * 0.015f;

	if (mouseDX > 1.0f)											// keep x values pinned
		mouseDX = 1.0f;
	else
	if (mouseDX < -1.0f)
		mouseDX = -1.0f;

	if (fabs(mouseDX) > fabs(gPlayerInfo.analogControlX))		// is the mouse delta better than what we've got from the other devices?
		gPlayerInfo.analogControlX = mouseDX;


	if (mouseDY > 1.0f)											// keep y values pinned
		mouseDY = 1.0f;
	else
	if (mouseDY < -1.0f)
		mouseDY = -1.0f;

	if (fabs(mouseDY) > fabs(gPlayerInfo.analogControlZ))		// is the mouse delta better than what we've got from the other devices?
		gPlayerInfo.analogControlZ = mouseDY;
		 */
}

Boolean GetNewKeyState(unsigned short sdlScanCode)
{
	return gRawKeyboardState[sdlScanCode] == KEYSTATE_PRESSED;
}

Boolean GetKeyState(unsigned short sdlScanCode)
{
	return gRawKeyboardState[sdlScanCode] == KEYSTATE_PRESSED || gRawKeyboardState[sdlScanCode] == KEYSTATE_HELD;
}

Boolean AreAnyNewKeysPressed(void)
{
	return gAnyNewKeysPressed;
}

Boolean GetNeedState(int needID)
{
	GAME_ASSERT(needID < NUM_CONTROL_NEEDS);
	return 0 != (gNeedStates[needID] & KEYSTATE_ACTIVE_BIT);
}

Boolean GetNewNeedState(int needID)
{
	GAME_ASSERT(needID < NUM_CONTROL_NEEDS);
	return gNeedStates[needID] == KEYSTATE_PRESSED;
}

void DoInputConfigDialog(void)
{
	SOURCE_PORT_MINOR_PLACEHOLDER();
}
