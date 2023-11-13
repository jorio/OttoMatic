//
// input.h
//

#pragma once

		/* NEEDS */

#define KEYBINDING_MAX_KEYS					2
#define KEYBINDING_MAX_GAMEPAD_BUTTONS		2

#define NUM_SUPPORTED_MOUSE_BUTTONS			31
#define NUM_SUPPORTED_MOUSE_BUTTONS_PURESDL	(NUM_SUPPORTED_MOUSE_BUTTONS-2)
#define SDL_BUTTON_WHEELUP					(NUM_SUPPORTED_MOUSE_BUTTONS-2)		// make wheelup look like it's a button
#define SDL_BUTTON_WHEELDOWN				(NUM_SUPPORTED_MOUSE_BUTTONS-1)		// make wheeldown look like it's a button

#define NUM_MOUSE_SENSITIVITY_LEVELS		8
#define DEFAULT_MOUSE_SENSITIVITY_LEVEL		(NUM_MOUSE_SENSITIVITY_LEVELS/2)

typedef struct KeyBinding
{
	int16_t			key[KEYBINDING_MAX_KEYS];

	int8_t			mouseButton;

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

	kNeed_NULL = NUM_REMAPPABLE_NEEDS,

	kNeed_UIUp,
	kNeed_UIDown,
	kNeed_UILeft,
	kNeed_UIRight,
	kNeed_UIPrev,
	kNeed_UINext,
	kNeed_UIConfirm,
	kNeed_UIDelete,
	kNeed_UIStart,
	kNeed_UIBack,
	kNeed_UIPause,
	kNeed_TextEntry_Left,
	kNeed_TextEntry_Left2,
	kNeed_TextEntry_Right,
	kNeed_TextEntry_Right2,
	kNeed_TextEntry_Home,
	kNeed_TextEntry_End,
	kNeed_TextEntry_Bksp,
	kNeed_TextEntry_Del,
	kNeed_TextEntry_Done,
	kNeed_TextEntry_CharPP,
	kNeed_TextEntry_CharMM,
	kNeed_TextEntry_Space,

	NUM_CONTROL_NEEDS
};


//============================================================================================


void InitInput(void);
void UpdateInput(void);
void CaptureMouse(Boolean doCapture);
void EatMouseEvents(void);
Boolean GetNewKeyState(unsigned short sdlScanCode);
Boolean GetKeyState(unsigned short sdlScanCode);
Boolean GetNewNeedState(int needID);
Boolean GetNeedState(int needID);
Boolean FlushMouseButtonPress(uint8_t sdlButton);
Boolean UserWantsOut(void);
Boolean IsCmdQPressed(void);
Boolean GetCheatKeyCombo(void);
void Rumble(float strength, uint32_t ms);
float SnapAngle(float angle, float snap);

SDL_GameController* TryOpenController(bool showMessageOnFailure);
void OnJoystickRemoved(SDL_JoystickID which);

void SetMacLinearMouse(int linear);
