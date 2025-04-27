// INPUT.C
// (c)2023 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include "game.h"
#include "mousesmoothing.h"


/***************/
/* CONSTANTS   */
/***************/

enum
{
	KEYSTATE_ACTIVE_BIT		= 0b001,
	KEYSTATE_CHANGE_BIT		= 0b010,
	KEYSTATE_IGNORE_BIT		= 0b100,

	KEYSTATE_OFF			= 0b000,
	KEYSTATE_PRESSED		= KEYSTATE_ACTIVE_BIT | KEYSTATE_CHANGE_BIT,
	KEYSTATE_HELD			= KEYSTATE_ACTIVE_BIT,
	KEYSTATE_UP				= KEYSTATE_OFF | KEYSTATE_CHANGE_BIT,
	KEYSTATE_IGNOREHELD		= KEYSTATE_OFF | KEYSTATE_IGNORE_BIT,
};

#define kJoystickDeadZone				(33 * 32767 / 100)
#define kJoystickDeadZone_UI			(66 * 32767 / 100)
#define kJoystickDeadZoneFrac			(kJoystickDeadZone / 32767.0f)
#define kJoystickDeadZoneFracSquared	(kJoystickDeadZoneFrac * kJoystickDeadZoneFrac)

#define kDefaultSnapAngle				OGLMath_DegreesToRadians(10)

/***************/
/* EXTERNALS   */
/***************/

/**********************/
/*     PROTOTYPES     */
/**********************/

typedef uint8_t KeyState;

Boolean				gUserPrefersGamepad = false;
SDL_Gamepad			*gSDLGamepad = NULL;

static KeyState		gMouseButtonState[NUM_SUPPORTED_MOUSE_BUTTONS + 2];
static KeyState		gRawKeyboardState[SDL_SCANCODE_COUNT];
static KeyState		gNeedStates[NUM_CONTROL_NEEDS];

char				gTextInput[64];

static Boolean		gEatMouse = false;
Boolean				gMouseMotionNow = false;

OGLVector2D			gCameraControlDelta;



/**********************/
/* STATIC FUNCTIONS   */
/**********************/

static OGLVector2D GetThumbStickVector(bool rightStick);

static inline void UpdateKeyState(KeyState* state, bool downNow)
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

		case KEYSTATE_IGNOREHELD:
			*state = downNow ? KEYSTATE_IGNOREHELD : KEYSTATE_OFF;
			break;
	}
}

/**********************/
/* IMPLEMENTATION     */
/**********************/

void InitInput(void)
{
	// Open a connected gamepad on startup.
	TryOpenGamepad(true);
}

void UpdateInput(void)
{
	gTextInput[0] = '\0';
	gMouseMotionNow = false;
	gEatMouse = false;

			/**********************/
			/* DO SDL MAINTENANCE */
			/**********************/

	MouseSmoothing_StartFrame();

	int mouseWheelDelta = 0;

	SDL_PumpEvents();
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_EVENT_QUIT:
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				ExitToShell();			// throws Pomme::QuitRequest
				return;

			case SDL_EVENT_WINDOW_RESIZED:
				// QD3D_OnWindowResized(event.window.data1, event.window.data2);
				break;

			case SDL_EVENT_WINDOW_FOCUS_LOST:
				// On Mac, always restore system mouse accel if cmd-tabbing away from the game
				SetMacLinearMouse(0);
				gEatMouse = true;
				break;

			case SDL_EVENT_WINDOW_FOCUS_GAINED:
				// On Mac, kill mouse accel when focus is regained only if the game has captured the mouse
				if (SDL_GetWindowRelativeMouseMode(gSDLWindow))
					SetMacLinearMouse(1);
				gEatMouse = true;
				break;

			case SDL_EVENT_TEXT_INPUT:
				SDL_snprintf(gTextInput, sizeof(gTextInput), "%s", event.text.text);
				break;

			case SDL_EVENT_MOUSE_MOTION:
				if (!gEatMouse)
				{
					gMouseMotionNow = true;
					MouseSmoothing_OnMouseMotion(&event.motion);
				}
				break;

			case SDL_EVENT_MOUSE_WHEEL:
				if (!gEatMouse)
				{
					mouseWheelDelta += event.wheel.y;
					mouseWheelDelta += event.wheel.x;
				}
				break;

			case SDL_EVENT_GAMEPAD_ADDED:
				TryOpenGamepad(false);
				break;

			case SDL_EVENT_GAMEPAD_REMOVED:
				OnJoystickRemoved(event.gdevice.which);
				break;

			case SDL_EVENT_KEY_DOWN:
				gUserPrefersGamepad = false;
				break;

			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
			case SDL_EVENT_GAMEPAD_BUTTON_UP:
				gUserPrefersGamepad = true;
				break;
		}
	}

	// --------------------------------------------
	// Refresh the state of each individual mouse button,
	// including wheelup/wheeldown which we are exposed to the game as buttons

	if (gEatMouse)
	{
		MouseSmoothing_ResetState();
		SDL_memset(gMouseButtonState, KEYSTATE_OFF, sizeof(gMouseButtonState));
	}
	else
	{
		uint32_t mouseButtons = SDL_GetMouseState(NULL, NULL);

		// Actual mouse buttons
		for (int i = 1; i < NUM_SUPPORTED_MOUSE_BUTTONS_PURESDL; i++)
		{
			bool downNow = mouseButtons & SDL_BUTTON_MASK(i);
			UpdateKeyState(&gMouseButtonState[i], downNow);
		}

		// Wheel up/wheel down fake buttons
		UpdateKeyState(&gMouseButtonState[SDL_BUTTON_WHEELUP],		mouseWheelDelta > 0);
		UpdateKeyState(&gMouseButtonState[SDL_BUTTON_WHEELDOWN],	mouseWheelDelta < 0);
	}

	// --------------------------------------------
	// Refresh the state of each individual keyboard key

	int numkeys = 0;
	const bool* keystate = SDL_GetKeyboardState(&numkeys);

	{
		int minNumKeys = numkeys < SDL_SCANCODE_COUNT ? numkeys : SDL_SCANCODE_COUNT;

		for (int i = 0; i < minNumKeys; i++)
			UpdateKeyState(&gRawKeyboardState[i], keystate[i]);

		// fill out the rest
		for (int i = minNumKeys; i < SDL_SCANCODE_COUNT; i++)
			UpdateKeyState(&gRawKeyboardState[i], false);
	}

	// --------------------------------------------
	// Parse Alt+Enter

	if (GetNewKeyState(SDL_SCANCODE_RETURN)
		&& (GetKeyState(SDL_SCANCODE_LALT) || GetKeyState(SDL_SCANCODE_RALT)))
	{
		gGamePrefs.fullscreen = gGamePrefs.fullscreen ? 0 : 1;
		SetFullscreenMode(false);

		gRawKeyboardState[SDL_SCANCODE_RETURN] = KEYSTATE_IGNOREHELD;
	}

	// --------------------------------------------
	// Update need states

	for (int i = 0; i < NUM_CONTROL_NEEDS; i++)
	{
		const KeyBinding* kb = (i < NUM_REMAPPABLE_NEEDS) ? &gGamePrefs.remappableKeys[i] : &kDefaultKeyBindings[i];

		bool downNow = false;

		for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
			if (kb->key[j] && kb->key[j] < numkeys)
				downNow |= gRawKeyboardState[kb->key[j]] & KEYSTATE_ACTIVE_BIT;

		downNow |= gMouseButtonState[kb->mouseButton] & KEYSTATE_ACTIVE_BIT;

		if (gSDLGamepad)
		{
			int16_t deadZone = i >= NUM_REMAPPABLE_NEEDS
							   ? kJoystickDeadZone_UI
							   : kJoystickDeadZone;

			for (int j = 0; j < KEYBINDING_MAX_GAMEPAD_BUTTONS; j++)
			{
				switch (kb->gamepad[j].type)
				{
					case kInputTypeButton:
						downNow |= 0 != SDL_GetGamepadButton(gSDLGamepad, kb->gamepad[j].id);
						break;

					case kInputTypeAxisPlus:
						downNow |= SDL_GetGamepadAxis(gSDLGamepad, kb->gamepad[j].id) > deadZone;
						break;

					case kInputTypeAxisMinus:
						downNow |= SDL_GetGamepadAxis(gSDLGamepad, kb->gamepad[j].id) < -deadZone;
						break;

					default:
						break;
				}
			}
		}

		UpdateKeyState(&gNeedStates[i], downNow);
	}


	// -------------------------------------------


			/****************************/
			/* SET PLAYER AXIS CONTROLS */
			/****************************/

	gPlayerInfo.analogControlX = 											// assume no control input
	gPlayerInfo.analogControlZ = 0.0f;

			/* FIRST CHECK ANALOG AXES */

	if (gSDLGamepad)
	{
		OGLVector2D thumbVec = GetThumbStickVector(false);
		if (thumbVec.x != 0 || thumbVec.y != 0)
		{
			gPlayerInfo.analogControlX = thumbVec.x;
			gPlayerInfo.analogControlZ = thumbVec.y;
		}
	}

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

	const float mouseSensitivityFrac = (float)gGamePrefs.mouseSensitivityLevel / NUM_MOUSE_SENSITIVITY_LEVELS;
	int mdx, mdy;
	MouseSmoothing_GetDelta(&mdx, &mdy);


	if (gGamePrefs.mouseControlsOtto)
	{
		float mouseDX = mdx * mouseSensitivityFrac * .1f;
		float mouseDY = mdy * mouseSensitivityFrac * .1f;

		mouseDX = ClampFloat(mouseDX, -1.0f, 1.0f);					// keep values pinned
		mouseDY = ClampFloat(mouseDY, -1.0f, 1.0f);

		if (fabsf(mouseDX) > fabsf(gPlayerInfo.analogControlX))		// is the mouse delta better than what we've got from the other devices?
			gPlayerInfo.analogControlX = mouseDX;

		if (fabsf(mouseDY) > fabsf(gPlayerInfo.analogControlZ))		// is the mouse delta better than what we've got from the other devices?
			gPlayerInfo.analogControlZ = mouseDY;
	}


			/* UPDATE SWIVEL CAMERA */

	gCameraControlDelta.x = 0;
	gCameraControlDelta.y = 0;

	if (gSDLGamepad)
	{
		OGLVector2D rsVec = GetThumbStickVector(true);
		gCameraControlDelta.x -= rsVec.x * 1.0f;
		gCameraControlDelta.y += rsVec.y * 1.0f;
	}

	if (GetNeedState(kNeed_CameraLeft))
		gCameraControlDelta.x -= 1.0f;

	if (GetNeedState(kNeed_CameraRight))
		gCameraControlDelta.x += 1.0f;

	if (!gGamePrefs.mouseControlsOtto)
		gCameraControlDelta.x -= mdx * mouseSensitivityFrac * 0.04f;


	// --------------------------------------------

			/*******************/
			/* CHECK FOR CMD+Q */
			/*******************/
			// When in-game, take a different path (see PlayArea)

	if ((!gIsInGame || gGamePaused) && IsCmdQPressed())
	{
		CleanQuit();
	}
}

void CaptureMouse(Boolean doCapture)
{
	SDL_PumpEvents();	// Prevent SDL from thinking mouse buttons are stuck as we switch into relative mode
	SDL_SetWindowMouseGrab(gSDLWindow, doCapture);
	SDL_SetWindowRelativeMouseMode(gSDLWindow, doCapture);
	if (doCapture)
		SDL_HideCursor();
	else
		SDL_ShowCursor();
//	ClearMouseState();
	EatMouseEvents();
	SetMacLinearMouse(doCapture);
}

void EatMouseEvents(void)
{
	gEatMouse = true;
}

Boolean GetNewKeyState(unsigned short sdlScanCode)
{
	return gRawKeyboardState[sdlScanCode] == KEYSTATE_PRESSED;
}

Boolean GetKeyState(unsigned short sdlScanCode)
{
	return 0 != (gRawKeyboardState[sdlScanCode] & KEYSTATE_ACTIVE_BIT);
}

Boolean UserWantsOut(void)
{
	return GetNewNeedState(kNeed_UIConfirm)
		|| GetNewNeedState(kNeed_UIBack)
		|| GetNewNeedState(kNeed_UIStart)
		|| FlushMouseButtonPress(SDL_BUTTON_LEFT);
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

Boolean FlushMouseButtonPress(uint8_t sdlButton)
{
	GAME_ASSERT(sdlButton < NUM_SUPPORTED_MOUSE_BUTTONS);
	Boolean gotPress = KEYSTATE_PRESSED == gMouseButtonState[sdlButton];
	if (gotPress)
		gMouseButtonState[sdlButton] = KEYSTATE_HELD;
	return gotPress;
}

Boolean IsCmdQPressed(void)
{
#if __APPLE__
	return (GetKeyState(SDL_SCANCODE_LGUI) || GetKeyState(SDL_SCANCODE_RGUI))
		&& GetNewKeyState(SDL_GetScancodeFromKey(SDLK_Q, NULL));
#else
	// on non-mac systems, alt-f4 is handled by the system
	return false;
#endif
}

Boolean GetCheatKeyCombo(void)
{
	return (GetKeyState(SDL_SCANCODE_B) && GetKeyState(SDL_SCANCODE_R) && GetKeyState(SDL_SCANCODE_I))
		|| (GetKeyState(SDL_SCANCODE_C) && GetKeyState(SDL_SCANCODE_M) && GetKeyState(SDL_SCANCODE_R));
}

#pragma mark -

/****************************** SDL JOYSTICK FUNCTIONS ********************************/

SDL_Gamepad* TryOpenGamepad(bool showMessage)
{
	int numJoysticks = 0;
	int numJoysticksAlreadyInUse = 0;

	SDL_JoystickID* joysticks = SDL_GetJoysticks(&numJoysticks);
	SDL_Gamepad* newGamepad = NULL;

	for (int i = 0; i < numJoysticks; ++i)
	{
		SDL_JoystickID joystickID = joysticks[i];

		// Usable as an SDL_Gamepad?
		if (!SDL_IsGamepad(joystickID))
		{
			continue;
		}

		// Already in use?
		if (gSDLGamepad && SDL_GetGamepadID(gSDLGamepad) == joystickID)
		{
			numJoysticksAlreadyInUse++;
			continue;
		}

		// Use this one
		newGamepad = SDL_OpenGamepad(joystickID);
		if (newGamepad)
		{
			gSDLGamepad = newGamepad;
			break;
		}
	}

	if (newGamepad)
	{
		// OK
	}
	else if (numJoysticksAlreadyInUse == numJoysticks)
	{
		// No-op; All joysticks already in use (or there might be zero joysticks)
	}
	else
	{
		SDL_Log("%d joysticks found, but none is suitable as an SDL_Gamepad.", numJoysticks);
		if (showMessage)
		{
			char messageBuf[1024];
			SDL_snprintf(messageBuf, sizeof(messageBuf),
						 "The game does not support your controller yet (\"%s\").\n\n"
						 "You can play with the keyboard and mouse instead. Sorry!",
						 SDL_GetJoystickNameForID(joysticks[0]));
			SDL_ShowSimpleMessageBox(
				SDL_MESSAGEBOX_WARNING,
				"Controller not supported",
				messageBuf,
				gSDLWindow);
		}
	}

	SDL_free(joysticks);

	return newGamepad;
}

void Rumble(float strength, uint32_t ms)
{
	if (NULL == gSDLGamepad || !gGamePrefs.gamepadRumble)
		return;

	// TODO: Read global rumble intensity off prefs
	float rumbleIntensityFrac = 1;

	SDL_RumbleGamepad(
		gSDLGamepad,
		(Uint16)(strength * 65535),
		(Uint16)(strength * 65535),
		(Uint32)((float)ms * rumbleIntensityFrac));
}

void OnJoystickRemoved(SDL_JoystickID which)
{
	if (NULL == gSDLGamepad)		// don't care, I didn't open any controller
		return;

	if (which != SDL_GetGamepadID(gSDLGamepad))	// don't care, this isn't the joystick I'm using
		return;

	SDL_Log("Current joystick was removed: %d", which);

	// Nuke reference to this SDL_Gamepad
	SDL_CloseGamepad(gSDLGamepad);
	gSDLGamepad = NULL;

	// Try to open another joystick if any is connected.
	TryOpenGamepad(false);
}

float SnapAngle(float angle, float snap)
{
	if (angle >= -snap && angle <= snap)							// east (-0...0)
		return 0;
	else if (angle >= PI/2 - snap && angle <= PI/2 + snap)			// south
		return PI/2;
	else if (angle >= PI - snap || angle <= -PI + snap)				// west (180...-180)
		return PI;
	else if (angle >= -PI/2 - snap && angle <= -PI/2 + snap)		// north
		return -PI/2;
	else
		return angle;
}

static OGLVector2D GetThumbStickVector(bool rightStick)
{
	Sint16 dxRaw = SDL_GetGamepadAxis(gSDLGamepad, rightStick ? SDL_GAMEPAD_AXIS_RIGHTX : SDL_GAMEPAD_AXIS_LEFTX);
	Sint16 dyRaw = SDL_GetGamepadAxis(gSDLGamepad, rightStick ? SDL_GAMEPAD_AXIS_RIGHTY : SDL_GAMEPAD_AXIS_LEFTY);

	float dx = dxRaw / 32767.0f;
	float dy = dyRaw / 32767.0f;

	float magnitudeSquared = dx*dx + dy*dy;

	if (magnitudeSquared < kJoystickDeadZoneFracSquared)
	{
		return (OGLVector2D) { 0, 0 };
	}
	else
	{
		float magnitude;

		if (magnitudeSquared > 1.0f)
		{
			// Cap magnitude -- what's returned by the controller actually lies within a square
			magnitude = 1.0f;
		}
		else
		{
			magnitude = sqrtf(magnitudeSquared);

			// Avoid magnitude bump when thumbstick is pushed past dead zone:
			// Bring magnitude from [kJoystickDeadZoneFrac, 1.0] to [0.0, 1.0].
			magnitude = (magnitude - kJoystickDeadZoneFrac) / (1.0f - kJoystickDeadZoneFrac);
		}

		float angle = atan2f(dy, dx);

		angle = SnapAngle(angle, kDefaultSnapAngle);

		return (OGLVector2D) { cosf(angle) * magnitude, sinf(angle) * magnitude };
	}
}
