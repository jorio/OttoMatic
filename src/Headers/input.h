//
// input.h
//

#pragma once

		/* NEEDS */

#define KEYBINDING_MAX_KEYS					2
#define KEYBINDING_MAX_GAMEPAD_BUTTONS		2

typedef struct KeyBinding
{
	int16_t			key[KEYBINDING_MAX_KEYS];

	struct
	{
		int8_t		type;
		int8_t		id;
	} mouse;

	struct
	{
		int8_t		type;
		int8_t		id;
	} gamepad[KEYBINDING_MAX_GAMEPAD_BUTTONS];
} KeyBinding;

enum
{
	kInputTypeUnbound = 0,
	kInputTypeButton,
	kInputTypeAxisPlus,
	kInputTypeAxisMinus,
};

enum
{
	kNeed_Forward,
	kNeed_Backward,
	kNeed_TurnLeft,
	kNeed_TurnRight,
	kNeed_PrevWeapon,
	kNeed_NextWeapon,
	kNeed_Shoot,
	kNeed_PunchPickup,
	kNeed_Jump,
	kNeed_CameraMode,
	kNeed_CameraLeft,
	kNeed_CameraRight,

	NUM_REMAPPABLE_NEEDS,

	// ^^^ REMAPPABLE
	// --------------------------------------------------------
	//              NON-REMAPPABLE vvv

	kNeed_UIUp = NUM_REMAPPABLE_NEEDS,
	kNeed_UIDown,
	kNeed_UILeft,
	kNeed_UIRight,
	kNeed_UIPrev,
	kNeed_UINext,
	kNeed_UIConfirm,
	kNeed_UIStart,
	kNeed_UIBack,
	kNeed_UIPause,

	NUM_CONTROL_NEEDS
};


//============================================================================================


void InitInput(void);
void UpdateInput(void);
void CaptureMouse(Boolean doCapture);
Boolean GetNewKeyState(unsigned short sdlScanCode);
Boolean GetKeyState(unsigned short sdlScanCode);
Boolean GetNewNeedState(int needID);
Boolean GetNeedState(int needID);
Boolean UserWantsOut(void);

SDL_GameController* TryOpenController(bool showMessageOnFailure);
void OnJoystickRemoved(SDL_JoystickID which);
