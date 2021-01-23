//
// input.h
//

#ifndef __INPUT_H
#define __INPUT_H


		/* NEEDS */

typedef struct KeyBinding
{
	int16_t		key1;
	int16_t		key2;
	int16_t		mouseButton;
	int16_t		gamepadButton;
} KeyBinding;

enum
{
	kNeed_Forward,
	kNeed_Backward,
	kNeed_TurnLeft,
	kNeed_TurnRight,
	kNeed_NextWeapon,
	kNeed_Shoot,
	kNeed_PunchPickup,
	kNeed_Jump,
	kNeed_CameraMode,
	kNeed_CameraLeft,
	kNeed_CameraRight,
	
	NUM_CONTROL_NEEDS
};


//============================================================================================


void InitInput(void);
void UpdateInput(void);
void CaptureMouse(Boolean doCapture);
Boolean GetNewKeyState(unsigned short sdlScanCode);
Boolean GetKeyState(unsigned short sdlScanCode);
Boolean AreAnyNewKeysPressed(void);
Boolean GetNewNeedState(int needID);
Boolean GetNeedState(int needID);

void DoInputConfigDialog(void);




#endif



