//
// input.h
//

#ifndef __INPUT_H
#define __INPUT_H


		/* NEEDS */
		
enum
{
	kNeed_TurnLeft	= 0,
	kNeed_TurnRight,
	kNeed_Forward,
	kNeed_Backward,
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
Boolean GetNewKeyState(unsigned short sdlScanCode);
Boolean GetKeyState(unsigned short sdlScanCode);
Boolean AreAnyNewKeysPressed(void);
Boolean GetNewNeedState(int needID);
Boolean GetNeedState(int needID);

void DoInputConfigDialog(void);




#endif



