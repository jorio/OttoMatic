// INPUT.C
// (c)2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include "game.h"
#include "mousesmoothing.h"
#include "killmacmouseacceleration.h"


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

#define JOYSTICK_DEAD_ZONE .1f
#define JOYSTICK_DEAD_ZONE_SQUARED (JOYSTICK_DEAD_ZONE*JOYSTICK_DEAD_ZONE)

/***************/
/* EXTERNALS   */
/***************/

extern	SDL_GLContext		gAGLContext;
extern	SDL_Window		*gSDLWindow;
extern	float			gFramesPerSecondFrac,gFramesPerSecond,gScratchF;
extern	PrefsType			gGamePrefs;

/**********************/
/*     PROTOTYPES     */
/**********************/

SDL_GameController	*gSDLController = NULL;
SDL_JoystickID		gSDLJoystickInstanceID = -1;		// ID of the joystick bound to gSDLController

Byte				gRawKeyboardState[SDL_NUM_SCANCODES];
bool				gAnyNewKeysPressed = false;
char				gTextInput[SDL_TEXTINPUTEVENT_TEXT_SIZE];

Byte				gNeedStates[NUM_CONTROL_NEEDS];

bool				gEatMouse = false;

OGLVector2D			gCameraControlDelta;


static OGLVector2D GetThumbStickVector(bool rightStick);


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
[kNeed_PrevWeapon	] = {0,						0,						0,					SDL_CONTROLLER_BUTTON_LEFTSHOULDER, },
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

	gTextInput[0] = '\0';


			/**********************/
			/* DO SDL MAINTENANCE */
			/**********************/

	MouseSmoothing_StartFrame();

	SDL_PumpEvents();
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				ExitToShell();			// throws Pomme::QuitRequest
				return;

			case SDL_WINDOWEVENT:
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_CLOSE:
						ExitToShell();	// throws Pomme::QuitRequest
						return;

					/*
					case SDL_WINDOWEVENT_RESIZED:
						QD3D_OnWindowResized(event.window.data1, event.window.data2);
						break;
					*/

					case SDL_WINDOWEVENT_FOCUS_LOST:
#if __APPLE__
						// On Mac, always restore system mouse accel if cmd-tabbing away from the game
						RestoreMacMouseAcceleration();
#endif
						break;

					case SDL_WINDOWEVENT_FOCUS_GAINED:
#if __APPLE__
						// On Mac, kill mouse accel when focus is regained only if the game has captured the mouse
						if (SDL_GetRelativeMouseMode())
							KillMacMouseAcceleration();
#endif
						break;
				}
				break;

				case SDL_TEXTINPUT:
					memcpy(gTextInput, event.text.text, sizeof(gTextInput));
					_Static_assert(sizeof(gTextInput) == sizeof(event.text.text), "size mismatch: gTextInput/event.text.text");
					break;

				case SDL_MOUSEMOTION:
					if (!gEatMouse)
					{
						MouseSmoothing_OnMouseMotion(&event.motion);
					}
					break;

				case SDL_JOYDEVICEADDED:	 // event.jdevice.which is the joy's INDEX (not an instance id!)
					TryOpenController(false);
					break;

				case SDL_JOYDEVICEREMOVED:	// event.jdevice.which is the joy's UNIQUE INSTANCE ID (not an index!)
					OnJoystickRemoved(event.jdevice.which);
					break;
		}
	}

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

		if (gSDLController && kb->gamepadButton != SDL_CONTROLLER_BUTTON_INVALID)
			downNow |= 0 != SDL_GameControllerGetButton(gSDLController, kb->gamepadButton);

		UpdateKeyState(&gNeedStates[i], downNow);
	}


	// -------------------------------------------


			/****************************/
			/* SET PLAYER AXIS CONTROLS */
			/****************************/

	gPlayerInfo.analogControlX = 											// assume no control input
	gPlayerInfo.analogControlZ = 0.0f;

			/* FIRST CHECK ANALOG AXES */

	if (gSDLController)
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

	int mdx, mdy;
	MouseSmoothing_GetDelta(&mdx, &mdy);
	float mouseDX = mdx * 0.015f;								// scale down deltas for our use
	float mouseDY = mdy * 0.015f;
	mouseDX *= 10;
	mouseDY *= 10;

	mouseDX = CLAMP(mouseDX, -1.0f, 1.0f);						// keep values pinned
	mouseDY = CLAMP(mouseDY, -1.0f, 1.0f);

	if (fabsf(mouseDX) > fabsf(gPlayerInfo.analogControlX))		// is the mouse delta better than what we've got from the other devices?
		gPlayerInfo.analogControlX = mouseDX;

	if (fabsf(mouseDY) > fabsf(gPlayerInfo.analogControlZ))		// is the mouse delta better than what we've got from the other devices?
		gPlayerInfo.analogControlZ = mouseDY;




			/* UPDATE SWIVEL CAMERA */

	gCameraControlDelta.x = 0;
	gCameraControlDelta.y = 0;

	if (gSDLController)
	{
		OGLVector2D rsVec = GetThumbStickVector(true);
		gCameraControlDelta.x -= rsVec.x * 1.0f;
		gCameraControlDelta.y += rsVec.y * 1.0f;
	}

	if (GetNeedState(kNeed_CameraLeft))
		gCameraControlDelta.x -= 1.0f;

	if (GetNeedState(kNeed_CameraRight))
		gCameraControlDelta.x += 1.0f;
}

void CaptureMouse(Boolean doCapture)
{
	SDL_PumpEvents();	// Prevent SDL from thinking mouse buttons are stuck as we switch into relative mode
	SDL_SetRelativeMouseMode(doCapture ? SDL_TRUE : SDL_FALSE);
	SDL_ShowCursor(doCapture ? 0 : 1);
//	ClearMouseState();
//	EatMouseEvents();

#if __APPLE__
	if (doCapture)
        KillMacMouseAcceleration();
    else
        RestoreMacMouseAcceleration();
#endif
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

#pragma mark -

/****************************** SDL JOYSTICK FUNCTIONS ********************************/

SDL_GameController* TryOpenController(bool showMessage)
{
	if (gSDLController)
	{
		printf("Already have a valid controller.\n");
		return gSDLController;
	}

	if (SDL_NumJoysticks() == 0)
	{
		return NULL;
	}

	for (int i = 0; gSDLController == NULL && i < SDL_NumJoysticks(); ++i)
	{
		if (SDL_IsGameController(i))
		{
			gSDLController = SDL_GameControllerOpen(i);
			gSDLJoystickInstanceID = SDL_JoystickGetDeviceInstanceID(i);
		}
	}

	if (!gSDLController)
	{
		printf("Joystick(s) found, but none is suitable as an SDL_GameController.\n");
		if (showMessage)
		{
			char messageBuf[1024];
			snprintf(messageBuf, sizeof(messageBuf),
					 "The game does not support your controller yet (\"%s\").\n\n"
					 "You can play with the keyboard and mouse instead. Sorry!",
					 SDL_JoystickNameForIndex(0));
			SDL_ShowSimpleMessageBox(
					SDL_MESSAGEBOX_WARNING,
					"Controller not supported",
					messageBuf,
					gSDLWindow);
		}
		return NULL;
	}

	printf("Opened joystick %d as controller: %s\n", gSDLJoystickInstanceID, SDL_GameControllerName(gSDLController));

	/*
	gSDLHaptic = SDL_HapticOpenFromJoystick(SDL_GameControllerGetJoystick(gSDLController));
	if (!gSDLHaptic)
		printf("This joystick can't do haptic.\n");
	else
		printf("This joystick can do haptic!\n");
	*/

	return gSDLController;
}

void OnJoystickRemoved(SDL_JoystickID which)
{
	if (NULL == gSDLController)		// don't care, I didn't open any controller
		return;

	if (which != gSDLJoystickInstanceID)	// don't care, this isn't the joystick I'm using
		return;

	printf("Current joystick was removed: %d\n", which);

	// Nuke reference to this controller+joystick
	SDL_GameControllerClose(gSDLController);
	gSDLController = NULL;
	gSDLJoystickInstanceID = -1;

	// Try to open another joystick if any is connected.
	TryOpenController(false);
}

static OGLVector2D GetThumbStickVector(bool rightStick)
{
	Sint16 dxRaw = SDL_GameControllerGetAxis(gSDLController, rightStick ? SDL_CONTROLLER_AXIS_RIGHTX : SDL_CONTROLLER_AXIS_LEFTX);
	Sint16 dyRaw = SDL_GameControllerGetAxis(gSDLController, rightStick ? SDL_CONTROLLER_AXIS_RIGHTY : SDL_CONTROLLER_AXIS_LEFTY);

	float dx = dxRaw / 32767.0f;
	float dy = dyRaw / 32767.0f;

	float magnitudeSquared = dx*dx + dy*dy;
	if (magnitudeSquared < JOYSTICK_DEAD_ZONE_SQUARED)
		return (OGLVector2D) { 0, 0 };
	else
		return (OGLVector2D) { dx, dy };
}
