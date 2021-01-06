//
// input.h
//

#ifndef __INPUT_H
#define __INPUT_H


		/* NEEDS */
		
enum
{
	kNeed_TurnLeft_Key	= 0,
	kNeed_TurnRight_Key,
	kNeed_Forward,
	kNeed_Backward,
	kNeed_XAxis,
	kNeed_YAxis,
	kNeed_NextWeapon,
	kNeed_Shoot,
	kNeed_PunchPickup,
	kNeed_Jump,
	kNeed_CameraMode,
	kNeed_CameraLeft,
	kNeed_CameraRight,
	
	NUM_CONTROL_NEEDS
};


		/* HID STUFF */
		
#define	MAX_HID_DEVICES				8			// max # of HID devices we'll allow
#define	MAX_HID_ELEMENTS			150			// max # of elements per HID device

typedef struct
{
	Byte		maxNeeds;						// copies of the code constants
	Byte		maxDevices;
	Byte		maxElement;

	Byte		numDevices;						// # devices we're saving info for
	
	Boolean		isActive[MAX_HID_DEVICES];		// flags indicating which HID devices in our list are set to Active by the user
	long		vendorID[MAX_HID_DEVICES];
	long		productID[MAX_HID_DEVICES];
	Boolean		hasDuplicate[MAX_HID_DEVICES];  // true if there might be another identical device
	long		locationID[MAX_HID_DEVICES];
	long		calibrationMin[MAX_HID_DEVICES][MAX_HID_ELEMENTS];
	long		calibrationMax[MAX_HID_DEVICES][MAX_HID_ELEMENTS];
	long		calibrationCenter[MAX_HID_DEVICES][MAX_HID_ELEMENTS];
	Boolean		reverseAxis[MAX_HID_DEVICES][MAX_HID_ELEMENTS];
	
	long		needsElementNum[NUM_CONTROL_NEEDS][MAX_HID_DEVICES];

}HIDControlSettingsType;


			/* NEEDS DATA TYPES */
			
typedef struct
{
	short				elementNum;						// index into device's element list
	long				elementCurrentValue;			// the current polled value of this element
}NeedElementInfoType;

typedef struct
{
	char					*name;							// name of the Need
	short					defaultKeyboardKey;				// default keyboard key
	NeedElementInfoType		elementInfo[MAX_HID_DEVICES];	// for each possible device, all the info about its assigned element
	long					value;							// current element value
	long					oldValue;						// element's value on previous frame
	Boolean					newButtonPress;					// true when value is > 0 and oldValue == 0
}InputNeedType;




			/* EXTERNS */
			
extern	InputNeedType	gControlNeeds[];
extern	Boolean			gMouseButtonState, gMouseNewButtonState, gHIDInitialized;




//============================================================================================


void InitInput(void);
void UpdateInput(void);
Boolean GetNewKeyState(unsigned short sdlScanCode);
Boolean GetKeyState(unsigned short sdlScanCode);
Boolean AreAnyNewKeysPressed(void);


void ShutdownHID(void);

void DoInputConfigDialog(void);

void BuildHIDControlSettings(HIDControlSettingsType *settings);
void RestoreHIDControlSettings(HIDControlSettingsType *settings);



#endif



