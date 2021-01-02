/****************************/
/*   	  INPUT.C	   	    */
/* (c)2003 Pangea Software  */
/* By Brian Greenstone      */
/****************************/

#include <mach/mach_port.h>

/***************/
/* EXTERNALS   */
/***************/

extern	AGLContext		gAGLContext;
extern	float			gFramesPerSecondFrac,gFramesPerSecond,gScratchF;
extern	PrefsType			gGamePrefs;


/**********************/
/*     PROTOTYPES     */
/**********************/

static pascal OSStatus MyMouseEventHandler(EventHandlerCallRef eventhandler, EventRef pEventRef, void *userdata);
static void Install_MouseEventHandler(void);
static void UpdateMouseDeltas(void);

static void MyInitHID(void);
static void FindHIDDevices(mach_port_t masterPort, io_iterator_t *hidObjectIterator);
static void ParseAllHIDDevices(io_iterator_t hidObjectIterator);
static const char *GetCFStringFromObject(CFStringRef object);
static long GetCFNumberFromObject(CFNumberRef object);
static void MyCFStringShow(CFStringRef object);
static void RecurseDictionaryElement(CFDictionaryRef dictionary,CFStringRef key);
static void CFArrayParse(CFArrayRef object);
static void AddHIDElement(CFDictionaryRef dictionary);
static void CFArrayCallback(const void * value, void * parameter);
static void CreateHIDDeviceInterface(io_object_t hidDevice, IOHIDDeviceInterface ***hidDeviceInterface);
static void AddHIDDeviceToList(io_object_t hidDevice, CFMutableDictionaryRef properties, long usagePage, long usage, long locationID);
static const char *CreateElementNameString(long usagePage, long usage);

static void BuildNeedsMenu (WindowRef window);
static void BuildHIDDeviceMenu (WindowRef window);
static void BuildHIDElementMenu (WindowRef window);
static void BuildMenuFromCStrings(WindowRef window, SInt32 controlSig, SInt32 id, char *textList[], short numItems, short defaultSelection);
static pascal OSStatus MyHIDWindowEventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData);
static EventLoopTimerUPP InstallTimerEvent (void);
static pascal void IdleTimerCallback (EventLoopTimerRef inTimer, void* userData);
static void DeviceMenuWasSelected(int deviceNum);
static void NeedsMenuWasSelected(int	needNum);
static void ElementMenuWasSelected(int elementNum);
static void HideCalibrationControls(void);
static void ShowCalibrationControls(void);
static void ResetAllDefaultControls(void);

static void ResetElementCalibration(short deviceNum, short elementNum);
static long CalibrateElementValue(long inValue, short deviceNum, short elementNum, long elementType);
static void PollAllHIDInputNeeds(void);

static short FindMatchingDevice(long vendorID, long productID, Boolean hasDuplicate, long locationID);

static void DoHIDSucksDialog(void);
static pascal OSStatus HIDSucksEventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData);
static void UpdateKeyMap(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	DEVICE_NAME_MAX_LENGTH		256
#define	ELEMENT_NAME_MAX_LENGTH		128

enum { // control enums from nib
	kCntlOkElement 		= 1,
    kCntlPopUpDevice	= 2,
    kCntlPopUpNeeds		= 3,
    kCntlPopUpElement	= 4,

    kCntlActiveCheckBox	= 13,

    kCntlCalibrationBox= 15,

    kCntlTextRawValueElement 		= 27,
    kCntlRawValueScale 				= 28,
    kCntlCalibratedValueScale 		= 29,

    kCntlReverse = 32,

    kCntlTextScaleValueElement			= 33,
    kCntlUserScaleValueGraphicElement 	= 34


};



typedef struct
{
	IOHIDElementCookie	cookie;
	long				elementType;
	long				usagePage, usage;
	long				min,max,scaledMin,scaledMax;
	char				name[ELEMENT_NAME_MAX_LENGTH];

	Boolean				reverseAxis;
	long				calibrationMin, calibrationMax, calibrationCenter;
}HIDElementType;


typedef struct
{
	Boolean			isActive;

	IOHIDDeviceInterface 	**hidDeviceInterface;

	char			deviceName[DEVICE_NAME_MAX_LENGTH];
	long			usagePage, usage;
	long			vendorID, productID, locationID;
	Boolean			hasDuplicate;
	short			numElements;
	HIDElementType	elements[MAX_HID_ELEMENTS];
}HIDDeviceType;




#define	AXIS_RANGE	128.0f				// calibrate all axis values to be in +/- AXIS_RANGE values (-128 to +128)


/**********************/
/*     VARIABLES      */
/**********************/

static EventHandlerUPP gConfigWinEvtHandler;


Boolean	gHIDInitialized = false;

static	EventHandlerUPP			gMouseEventHandlerUPP = nil;
static	EventHandlerRef			gMouseEventHandlerRef = 0;

static	long					gMouseDeltaX = 0;
static	long					gMouseDeltaY = 0;

static	float					gReadMouseDeltasTimer = 0;

Boolean		gMouseButtonState = false, gMouseNewButtonState = false;

KeyMapByteArray gKeyMap,gNewKeys,gOldKeys;



			/**************/
			/* NEEDS LIST */
			/**************/

InputNeedType	gControlNeeds[NUM_CONTROL_NEEDS] =
{
	{										// kNeed_TurnLeft_Key
		"Turn Left (Key)",
		kHIDUsage_KeyboardLeftArrow,
	},

	{										// kNeed_TurnRight_Key
		"Turn Right (Key)",
		kHIDUsage_KeyboardRightArrow,
	},

	{										// kNeed_Forward
		"Move Forward (Key)",
		kHIDUsage_KeyboardUpArrow,
	},

	{										// kNeed_Backward
		"Move Backwards (Key)",
		kHIDUsage_KeyboardDownArrow,
	},

	{										// kNeed_XAxis
		"X-Axis",
		0,
	},

	{										// kNeed_YAxis
		"Y-Axis",
		0,
	},

	{										// kNeed_NextWeapon
		"Next Weapon",
		kHIDUsage_KeyboardLeftShift,
	},


	{										// kNeed_Shoot
		"Shoot",
		kHIDUsage_KeyboardLeftGUI,
	},

	{										// kNeed_PunchPickup
		"Punch/Pickup/Drop",
		kHIDUsage_KeyboardLeftAlt,
	},

	{										// kNeed_Jump
		"Jump",
		kHIDUsage_KeyboardSpacebar,
	},


	{										// kNeed_CameraMode
		"Camera Mode",
		kHIDUsage_KeyboardTab,
	},

	{										// kNeed_CameraLeft
		"Camera Swing Left",
		kHIDUsage_KeyboardComma,
	},

	{										// kNeed_CameraRight
		"Camera Swing Right",
		kHIDUsage_KeyboardPeriod,
	},




};



		/* HID */

static mach_port_t 	gHID_MasterPort 			= nil;

static char *gStringBuffer = nil;


static	short			gNumHIDDevices = 0;
static 	HIDDeviceType	gHIDDeviceList[MAX_HID_DEVICES];

static	MenuRef gRestoreMenu = nil;
static WindowRef gHIDConfigWindow = nil;


static	int gSelectedDeviceNum 	= 0; 	// 0...n
static	int	gSelectedNeed 		= 0;	// 0...n
static	int	gSelectedElement 	= 0;

static	Boolean	gShowCalibration;
static 	long	gCurrentCalibrationValue;

static	EventLoopTimerRef gTimer = NULL;


/************************* INIT INPUT *********************************/

void InitInput(void)
{
short	i;

	for (i = 0; i < NUM_CONTROL_NEEDS; i++)							// init current control values
		gControlNeeds[i].oldValue = gControlNeeds[i].value = 0;

	Install_MouseEventHandler();									// install our carbon even mouse handler
	MyInitHID();


}


/********************** UPDATE INPUT ******************************/
//
// This is the master input update function which reads all input values including those from HID devices, GetKeys(), and the Mouse.
//

void UpdateInput(void)
{
float	analog, mouseDX, mouseDY;

	UpdateKeyMap();												// always read real keyboard anyway via GetKeys()
	UpdateMouseDeltas();							// get mouse deltas


			/****************************************/
			/* UPDATE ALL THE NEEDS CONTROLS VALUES */
			/****************************************/

	PollAllHIDInputNeeds();



			/****************************/
			/* SET PLAYER AXIS CONTROLS */
			/****************************/

	gPlayerInfo.analogControlX = 											// assume no control input
	gPlayerInfo.analogControlZ = 0.0f;

			/* FIRST CHECK ANALOG AXES */

					/* PLAYER 1 */

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

			/* NEXT CHECK THE DIGITAL KEYS */


	if (gControlNeeds[kNeed_TurnLeft_Key].value)							// is Left Key pressed?
		gPlayerInfo.analogControlX = -1.0f;
	else
	if (gControlNeeds[kNeed_TurnRight_Key].value)						// is Right Key pressed?
		gPlayerInfo.analogControlX = 1.0f;


	if (gControlNeeds[kNeed_Forward].value)							// is Up Key pressed?
		gPlayerInfo.analogControlZ = -1.0f;
	else
	if (gControlNeeds[kNeed_Backward].value)						// is Down Key pressed?
		gPlayerInfo.analogControlZ = 1.0f;



		/* AND FINALLY SEE IF MOUSE DELTAS ARE BEST */
		//
		// The mouse is only used for Player 1
		//

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

}




//=================================================================================================================
//=================================================================================================================
//=================================================================================================================
//=================================================================================================================
//=================================================================================================================


#pragma mark ======= HID INITIALIZATION ============



/************************ MY INIT HID *************************/

static void MyInitHID(void)
{
io_iterator_t 	hidObjectIterator 	= nil;
IOReturn 		ioReturnValue 		= kIOReturnSuccess;

	if (gHIDInitialized)
		DoFatalAlert("\pMyInitHID: HID already initialized!");


	gNumHIDDevices = 0;											// nothing in our device list yet

		/* GET A PORT TO INIT COMMUNICATION WITH I/O KIT */

	ioReturnValue = IOMasterPort(bootstrap_port, &gHID_MasterPort);
	if (ioReturnValue != kIOReturnSuccess)
		DoFatalAlert("\pMyInitHID: IOMasterPort failed!");


			/* CREATE AN ITERATION LIST OF HID DEVICES */

	FindHIDDevices(gHID_MasterPort, &hidObjectIterator);
	if (hidObjectIterator == nil)
		DoFatalAlert("\pMyInitHID:  no HID Devices found!");


			/***************************************************/
			/* PARSE ALL THE HID DEVICES IN THE ITERATION LIST */
			/***************************************************/
			//
			// This builds our master list of desired HID devices and
			// all associated elements.
			//

	ParseAllHIDDevices(hidObjectIterator);


			/* DISPOSE OF THE ITERATION LIST */

	IOObjectRelease(hidObjectIterator);


	gHIDInitialized = true;


			/* SET THE DEFAULT CONTROL SETTINGS */

	ResetAllDefaultControls();

}


/********************* SHUTDOWN HID ****************************/
//
// Called when exiting the app.  It should dispose of all the HID stuff
// that's allocated.
//

void ShutdownHID(void)
{
short	i;

	if (!gHIDInitialized)
		return;

			/* DISPOSE OF ALL THE OPEN INTERFACES */

	for (i = 0; i < gNumHIDDevices; i++)
	{
		IOHIDDeviceInterface **interface = gHIDDeviceList[i].hidDeviceInterface;	// get interface reference from our list

		(*interface)->close(interface);												// close & release interface
		(*interface)->Release(interface);
	}



			/* FREE MASTER PORT */

	if (gHID_MasterPort)
		mach_port_deallocate(mach_task_self(), gHID_MasterPort);


	gHIDInitialized = false;

}


/******************* FIND HID DEVICES ************************/
//
// Creates an iterator list object which contains all of the HID devices
//

static void FindHIDDevices(mach_port_t masterPort, io_iterator_t *hidObjectIterator)
{
CFMutableDictionaryRef 	hidMatchDictionary;
IOReturn 				ioReturnValue = kIOReturnSuccess;


		/* CREATE A DICTIONARY FOR HID DEVICES */

	hidMatchDictionary = IOServiceMatching(kIOHIDDeviceKey);			// "kIOHIDDeviceKey" tells it to look for HID devices


		/* FILL THE DICTIONARY WITH THE HID DEVICES */
		//
		// NOTE: IOServiceGetMatchingServices consumes a reference to the dictionary,
		// so we don't need to release the dictionary ref.
		//

	ioReturnValue = IOServiceGetMatchingServices(masterPort, hidMatchDictionary, hidObjectIterator);


				/* VERIFY THAT WE FOUND ANYTHING */

	if ((ioReturnValue != kIOReturnSuccess) || (*hidObjectIterator == nil))
	{
		DoAlert("\pFindHIDDevices: No matching HID class devices found!");
		DoHIDSucksDialog();
		ExitToShell();
	}
}


/********************** PARSE ALL HID DEVICES ************************/
//
// This iterates thru all of the HID devices returned in our dictionary iterator.
//

static void ParseAllHIDDevices(io_iterator_t hidObjectIterator)
{
io_object_t 			hidDevice;
IOReturn 				ioReturnValue;
kern_return_t 			result;
CFMutableDictionaryRef 	properties;
CFTypeRef 				object;
long 					usagePage, usage, locationID;
Boolean					allowThisDevice;

		/***************************************************/
		/* ITERATE THRU ALL OF THE HID OBJECTS IN THE LIST */
		/***************************************************/
		//
		// NOTE:  On Mac OS 10.2.8 we get lots of errors from the CFDictionaryGetValue() calls.
		//		It often gets these on the keyboard device, so to try and recover from this,
		//		we're going to assume that a device is the keyboard if running 10.2
		//

	while ((hidDevice = IOIteratorNext(hidObjectIterator)))					// get the HID device
	{
			/* CONVERT THE HID'S PROPERTIES INTO A CORE-FOUNDATION DICTIONARY */
			//
			// We do this so that we can use various "CF" functions to get data easily.
			//

		result = IORegistryEntryCreateCFProperties(hidDevice, &properties,	kCFAllocatorDefault, kNilOptions);
		if ((result != KERN_SUCCESS) || (properties == nil))
			continue;
//		{
//			DoAlert("\pParseAllHIDDevices: IORegistryEntryCreateCFProperties failed!");
//			DoHIDSucksDialog();
//			ExitToShell();
//		}

				/* GET THE USAGE PAGE & USAGE VALUES OF THIS HID DEVICE */

		object = CFDictionaryGetValue(properties, CFSTR(kIOHIDPrimaryUsagePageKey));		// get the Usage Page object
		if (!object)
		{
				goto error;
		}
		else
		{
			if (!CFNumberGetValue(object, kCFNumberLongType, &usagePage))					// extract the Usage Page value
			{
				goto error;
			}
		}

		object = CFDictionaryGetValue(properties, CFSTR(kIOHIDPrimaryUsageKey));			// get the Usage object
		if (!object)
		{
				goto error;
		}
		else
		{
			if (!CFNumberGetValue(object, kCFNumberLongType, &usage))						// extract the Usage value
			{
					goto error;
			}
		}


		object = CFDictionaryGetValue(properties, CFSTR(kIOHIDLocationIDKey));				// get the Location ID object
		if (!object)
			locationID = 0xdeadbeef;
		else
		{
			if (!CFNumberGetValue(object, kCFNumberLongType, &locationID))						// extract the LocationID value
				locationID = 0xdeadbeef;
		}

				/***********************************************/
				/* SEE IF THIS IS A DEVICE WE'RE INTERESTED IN */
				/***********************************************/

		allowThisDevice = false;										// assume we don't want this HID device

		switch(usagePage)
		{
			case	kHIDPage_GenericDesktop:
					switch(usage)
					{
						case	kHIDUsage_GD_Joystick:
						case	kHIDUsage_GD_GamePad:
						case	kHIDUsage_GD_Keyboard:
						case	kHIDUsage_GD_MultiAxisController:
								allowThisDevice = true;
								break;
					}
					break;

			case	kHIDPage_KeyboardOrKeypad:
					allowThisDevice = true;
					break;

		}


				/***********************************/
				/* ADD THIS HID DEVICE TO OUR LIST */
				/***********************************/
				//
				// ADD DEVICE & ELEMENTS TO LIST & OPEN INTERFACE TO IT
				//

		if (allowThisDevice)
		{
			AddHIDDeviceToList(hidDevice, properties, usagePage, usage, locationID);
		}



			/* RELEASE THE HID OBJECT */
error:
		CFRelease(properties);											// free the CF dictionary we created

		ioReturnValue = IOObjectRelease(hidDevice);
		if (ioReturnValue != kIOReturnSuccess)
			DoFatalAlert("\pParseAllHIDDevices: IOObjectRelease failed!");
	}

}


/********************** ADD HID DEVICE TO LIST ***************************/
//
// Gathers information about this HID device.
//
// INPUT:  properties = CF dictionary that we got earlier which contains all the info we need to know.
//

static void AddHIDDeviceToList(io_object_t hidDevice, CFMutableDictionaryRef properties, long usagePage, long usage, long locationID)
{
CFTypeRef object;
const char	*deviceName;
const char hidSucks[] = "Unnamed Device";
short		d, i;
Boolean		duplicates;
long		vendorID, productID;

	if (gNumHIDDevices >= MAX_HID_DEVICES)									// see if the list is alredy full
		return;

	d = gNumHIDDevices;


		/*********************************/
		/* GATHER SOME INFO THAT WE NEED */
		/*********************************/

			/* GET PRODUCT NAME */

	object = CFDictionaryGetValue(properties, CFSTR(kIOHIDProductKey));		// get the Product object
	if (!object)
	{
//		CFShow(properties);		//---------
//		DoAlert("\pAddHIDDeviceToList: CFDictionaryGetValue failed! (kIOHIDProductKey) This is a known bug in OS X, but ** Rebooting your computer should fix this. **");
//		DoHIDSucksDialog();
//		ExitToShell();

		deviceName = hidSucks;

	}
	else
		deviceName = GetCFStringFromObject(object);								// get ptr to name C string


			/* GET VENDOR & PRODUCT ID'S */
			//
			// For unknown reasons the product and vendor ID keys will fail
			// randomly on Powerbook keyboards.
			//

	object = CFDictionaryGetValue(properties, CFSTR(kIOHIDVendorIDKey));	// get the Vendor ID object
	if (!object)
	{
		vendorID = usagePage;												// set some bogus vendorID
//		DoAlert("\pAddHIDDeviceToList: CFDictionaryGetValue failed! (kIOHIDVendorIDKey)");
	}
	else
	if (!CFNumberGetValue(object, kCFNumberLongType, &vendorID))			// extract the Vendor ID value
	{
		DoAlert("\pAddHIDDeviceToList: CFNumberGetValue failed!");
		DoHIDSucksDialog();
		ExitToShell();
	}

	object = CFDictionaryGetValue(properties, CFSTR(kIOHIDProductIDKey));	// get the Product ID object
	if (!object)
	{
		productID = usage;
//		DoAlert("\pAddHIDDeviceToList: CFDictionaryGetValue failed! (kIOHIDProductIDKey)");
	}
	else
	if (!CFNumberGetValue(object, kCFNumberLongType, &productID))			// extract the Product ID value
	{
		DoAlert("\pAddHIDDeviceToList: CFNumberGetValue failed!");
		DoHIDSucksDialog();
		ExitToShell();
	}

		/* SCAN THE EXISTING DEVICE LIST TO SEE IF THERE ARE OTHER IDENTICAL DEVICES */

	duplicates = false;
	for (i = 0; i < d; i++)													// scan thru list of devices already in list
	{
		if ((gHIDDeviceList[i].vendorID == vendorID) && (gHIDDeviceList[i].productID == productID))	// is this a match?
		{
			gHIDDeviceList[i].hasDuplicate = true;
			duplicates = true;
		}
	}


			/***********************/
			/* SAVE INFO INTO LIST */
			/***********************/

	gHIDDeviceList[d].usagePage 	= usagePage;							// keep usage Page
	gHIDDeviceList[d].usage 		= usage;								// keep usage
	gHIDDeviceList[d].vendorID 		= vendorID;								// keep vendor ID
	gHIDDeviceList[d].productID		= productID;							// keep product ID
	gHIDDeviceList[d].locationID	= locationID;							// keep location ID
	gHIDDeviceList[d].hasDuplicate 	= duplicates;							// there are more than 1 of this device
	gHIDDeviceList[d].numElements	 = 0;									// no elements added yet

	strncpy(gHIDDeviceList[d].deviceName, deviceName, DEVICE_NAME_MAX_LENGTH);	// copy device name string


			/* ONLY KEYBOARDS ARE ACTIVE BY DEFAULT */

	if ((usagePage == kHIDPage_KeyboardOrKeypad) ||
		((usagePage == kHIDPage_GenericDesktop) && (usage == kHIDUsage_GD_Keyboard)))
		gHIDDeviceList[d].isActive = true;
	else
		gHIDDeviceList[d].isActive = false;


			/****************************/
			/* RECURSIVELY ADD ELEMENTS */
			/****************************/

	RecurseDictionaryElement(properties, CFSTR(kIOHIDElementKey));



				/* OPEN AN INTERFACE TO THIS DEVICE */

	CreateHIDDeviceInterface(hidDevice, &gHIDDeviceList[d].hidDeviceInterface);


	gNumHIDDevices++;
}


/********************* CREATE HID DEVICE INTERFACE *************************/
//
// Opens an interface to the input hidDevice.
// Remember to close this interface when we're done with it (like when we quit or re-init the HID stuff).
//

static void CreateHIDDeviceInterface(io_object_t hidDevice, IOHIDDeviceInterface ***hidDeviceInterface)
{
IOCFPlugInInterface 	**plugInInterface;
HRESULT 				plugInResult;
SInt32 					score = 0;
IOReturn 				ioReturnValue;


	ioReturnValue = IOCreatePlugInInterfaceForService(hidDevice, kIOHIDDeviceUserClientTypeID,	kIOCFPlugInInterfaceID,
													&plugInInterface,	&score);
	if (ioReturnValue != kIOReturnSuccess)
	{
		DoAlert("\pCreateHIDDeviceInterface: IOCreatePlugInInterfaceForService failed!");
		DoHIDSucksDialog();
		ExitToShell();
	}



	// Call a method of the intermediate plug-in to create the device interface

	plugInResult = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID), (LPVOID)hidDeviceInterface);
	if (plugInResult != S_OK)
	{
		DoAlert("\pCreateHIDDeviceInterface: Couldn't create HID class device interface");
		DoHIDSucksDialog();
		ExitToShell();
	}

	(*plugInInterface)->Release(plugInInterface);


			/* OPEN THE INTERFACE */

	ioReturnValue = (**hidDeviceInterface)->open(*hidDeviceInterface, 0);
	if (ioReturnValue != kIOReturnSuccess)
	{
		DoAlert("\pCreateHIDDeviceInterface: Couldn't open device interface");
		DoHIDSucksDialog();
		ExitToShell();
	}
}



/******************** GET CF STRING FROM OBJECT **************************/

static const char *GetCFStringFromObject(CFStringRef object)
{
const char	*c;
char	dummyString[] = "Unknown Name Device";

			/* ATTEMPT TO GET THE STRING FROM THE POINTER */

	c = CFStringGetCStringPtr(object, CFStringGetSystemEncoding());


			/* IF THAT FAILES THEN EXTRACT IT INTO A BUFFER */

	if (c == nil)
	{
		CFIndex bufferSize = CFStringGetLength(object) + 1;					// get size of string

		if (gStringBuffer)													// if there was an existing string buffer, nuke it.
			SafeDisposePtr(gStringBuffer);

		gStringBuffer = AllocPtr(bufferSize);

		if (CFStringGetCString(object, gStringBuffer, bufferSize, CFStringGetSystemEncoding()))
		{
			c = gStringBuffer;
		}

				/* IF THAT FAILS TOO THEN JUST POINT TO A DUMMY STRING */

		else
		{
			c = dummyString;
		}
	}


	return	c;
}



/****************** RECURSE DICTIONARY ELEMENT *************************/

static void RecurseDictionaryElement(CFDictionaryRef dictionary,CFStringRef key)
{
CFTypeRef 	object;
CFTypeID 	type;

	object = CFDictionaryGetValue(dictionary, key);			// get the desired key value from the dictionary

	if (object)												// if the data exists then deal with it
	{
		type = CFGetTypeID(object);							// what is the data type?

		if (type == CFArrayGetTypeID())
			CFArrayParse(object);
		else
		if (type == CFDictionaryGetTypeID())
			AddHIDElement(object);
		else
		if (type == CFNumberGetTypeID())
			GetCFNumberFromObject(object);
		else
		if (type == CFStringGetTypeID())
			MyCFStringShow(object);
	}
}



/******************* MY CFSTRING SHOW *********************/

static void MyCFStringShow(CFStringRef object)
{
const char * c = CFStringGetCStringPtr(object, CFStringGetSystemEncoding());

	if (c)
	{
//		printf(�%s�, c);
	}
	else
	{
		CFIndex bufferSize = CFStringGetLength(object) + 1;
		char * buffer = (char *)malloc(bufferSize);
		if (buffer)
		{
			if (CFStringGetCString(object,	buffer,	bufferSize,	CFStringGetSystemEncoding()))
			{
//				printf(�%s�, buffer);
			}
			free(buffer);
		}
	}
}


/********************* CF ARRAY PARSE *************************/

static void CFArrayParse(CFArrayRef object)
{
CFRange range;

	range.location = 0;
	range.length = CFArrayGetCount(object);

		/* SCAN EACH ELEMENT OF THE ARRAY */
		//
		// Calling this will cause the callback function to be called once
		// for each element of the array.
		//

	CFArrayApplyFunction(object, range, CFArrayCallback, 0);
}


/******************** CF ARRAY CALLBACK *********************/
//
// This callback function is called once for each element in the array which was
// passed to CFArrayApplyFunction() above.
//

static void CFArrayCallback(const void * object, void * parameter)
{
#pragma unused	(parameter)

	if (CFGetTypeID(object) != CFDictionaryGetTypeID())		// we're only looking for dictionary types in the array.  Skip anything else
		return;

			/* TRY TO ADD THIS ELEMENT TO THE HID DEVICE */

	AddHIDElement(object);
}




/****************** ADD HID ELEMENT *************************/
//
// The input object is a HID Element, but not all elements are buttons and things.
// So, we first check to see if this element is anything we care about.  And since the
// element can have sub-elements, we recurse with kIOHIDElementKey at the end.
//

static void AddHIDElement(CFDictionaryRef dictionary)
{
CFTypeRef 			object;
IOHIDElementCookie	cookie;
long				elementType, usagePage, usage;
long				min,max,scaledMin,scaledMax;
const char 			*elementName;
short				d, e;

		/*******************************************************/
		/* FIRST DETERMINE IF THIS IS AN ELEMENT WE CARE ABOUT */
		/*******************************************************/


	object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementTypeKey));	// lookup the Type of this element
	if (!object)
		goto skip_element;

	elementType = GetCFNumberFromObject(object);						// get Type value

	switch(elementType)
	{
		case	kIOHIDElementTypeInput_Misc:							// misc (tend to be joystick/thumbsticks) are good
		case	kIOHIDElementTypeInput_Button:							// buttons are good
		case	kIOHIDElementTypeInput_Axis:							// axes are good
				break;

		default:														// anything else we don't care about
				goto skip_element;
	}


		/*************************************************************/
		/* THIS IS AN ELEMENT WE LIKE, SO EXTRACT THE IMPORTANT DATA */
		/*************************************************************/


			/* GET THE ELEMENT'S COOKIE */

	object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementCookieKey));		// lookup the Cookie
	cookie = (IOHIDElementCookie)GetCFNumberFromObject(object);						// get Cookie value


			/* GET USAGE */

	object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementUsagePageKey));	// lookup the Usage Page
	usagePage = GetCFNumberFromObject(object);										// get Usage Page value

	if (usagePage == kHIDPage_PID)  // don't do PID elements
		goto skip_element;
	if (usagePage == kHIDPage_LEDs)  // don't do LED elements (These SHOULD be output only but aren't always)
		goto skip_element;

	object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementUsageKey));		// lookup the Usage
	usage = GetCFNumberFromObject(object);											// get Usage value


			/* GET MIN/MAX */

	object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementMinKey));
	min = GetCFNumberFromObject(object);
	object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementMaxKey));
	max = GetCFNumberFromObject(object);

	object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementScaledMinKey));
	scaledMin = GetCFNumberFromObject(object);
	object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementScaledMaxKey));
	scaledMax = GetCFNumberFromObject(object);


			/* GET NAME STRING */

	object = CFDictionaryGetValue(dictionary, CFSTR(kIOHIDElementNameKey));			// try to get the element name, but most devices return NIL
	if (object)
		elementName = GetCFStringFromObject(object);								// get ptr to the built-in name string

	if ((object == nil) || (elementName == nil))
	{
		elementName = CreateElementNameString(usagePage, usage);
		if (elementName == nil)														// if we get nil then skip it (keyboards return some bogus info)
			goto skip_element;
	}



		/*********************/
		/* SAVE ELEMENT INFO */
		/*********************/

	d = gNumHIDDevices;
	e = gHIDDeviceList[d].numElements;

	if (e >= MAX_HID_ELEMENTS)					// see if we're already full
	{
		goto skip_element;
	}


	gHIDDeviceList[d].elements[e].elementType =	elementType;
	gHIDDeviceList[d].elements[e].cookie	=	cookie;
	gHIDDeviceList[d].elements[e].usagePage	=	usagePage;
	gHIDDeviceList[d].elements[e].usage		=	usage;
	gHIDDeviceList[d].elements[e].min		=	min;
	gHIDDeviceList[d].elements[e].max		=	max;
	gHIDDeviceList[d].elements[e].scaledMin	=	scaledMin;
	gHIDDeviceList[d].elements[e].scaledMax	=	scaledMax;

	strncpy(gHIDDeviceList[d].elements[e].name, elementName, ELEMENT_NAME_MAX_LENGTH);	// copy device name string

	ResetElementCalibration(d, e);



	gHIDDeviceList[d].numElements++;


			/**********************/
			/* SEE IF SUB-RECURSE */
			/**********************/

skip_element:
	RecurseDictionaryElement(dictionary, CFSTR(kIOHIDElementKey));
}


/********************** CREATE ELEMENT NAME STRING ****************************/
//
// Since most devices return nil for their name strings, we need to fabricate names of our own.
// Using the usagePage and usage info, we can pretty much figure out something.
//

static const char *CreateElementNameString(long usagePage, long usage)
{
const char *c = "Unnamed Element";
const char *buttonNames[30] =
{
	"Button 1",	"Button 2",	"Button 3",	"Button 4",	"Button 5",	"Button 6",	"Button 7",	"Button 8",	"Button 9",	"Button 10",
	"Button 11","Button 12","Button 13","Button 14","Button 15","Button 16","Button 17","Button 18","Button 19","Button 20",
	"Button 21","Button 22","Button 23","Button 24","Button 25","Button 26","Button 27","Button 28","Button 29","Button 30",
};

	switch(usagePage)
	{
			/**************************/
			/* GENERIC DESKTOP DEVICE */
			/**************************/

		case	kHIDPage_GenericDesktop:
				switch(usage)
				{
					case	kHIDUsage_GD_X:
							c = "X-Axis";
							break;

					case	kHIDUsage_GD_Y:
							c = "Y-Axis";
							break;

					case	kHIDUsage_GD_Z:
							c = "Z-Axis";
							break;

					case	kHIDUsage_GD_Rx:
							c = "Rotate X-Axis";
							break;

					case	kHIDUsage_GD_Ry:
							c = "Rotate Y-Axis";
							break;

					case	kHIDUsage_GD_Rz:
							c = "Rotate Z-Axis";
							break;

					case	kHIDUsage_GD_Slider:
							c = "Slider";
							break;

					case	kHIDUsage_GD_Dial:
							c = "Dial";
							break;

					case	kHIDUsage_GD_Wheel:
							c = "Wheel";
							break;

					case	kHIDUsage_GD_Hatswitch:
							c = "Hat Switch";
							break;

					case	kHIDUsage_GD_Start:
							c = "Start";
							break;

					case	kHIDUsage_GD_Select:
							c = "Select";
							break;

					case	kHIDUsage_GD_DPadUp:
							c = "D-Pad Up";
							break;

					case	kHIDUsage_GD_DPadDown:
							c = "D-Pad Down";
							break;

					case	kHIDUsage_GD_DPadRight:
							c = "D-Pad Right";
							break;

					case	kHIDUsage_GD_DPadLeft:
							c = "D-Pad Left";
							break;

				}
				break;


			/***********/
			/* BUTTONS */
			/***********/

		case	kHIDPage_Button:
				if (usage < 30)
					c = buttonNames[usage-1];
				else
					c = "Button";
				break;

			/*******************/
			/* KEYBOARD DEVICE */
			/*******************/

		case	kHIDPage_KeyboardOrKeypad:

				switch(usage)
				{
					case	kHIDUsage_KeyboardA:
							c = "A";
							break;
					case	kHIDUsage_KeyboardB:
							c = "B";
							break;
					case	kHIDUsage_KeyboardC:
							c = "C";
							break;
					case	kHIDUsage_KeyboardD:
							c = "D";
							break;
					case	kHIDUsage_KeyboardE:
							c = "E";
							break;
					case	kHIDUsage_KeyboardF:
							c = "F";
							break;
					case	kHIDUsage_KeyboardG:
							c = "G";
							break;
					case	kHIDUsage_KeyboardH:
							c = "H";
							break;
					case	kHIDUsage_KeyboardI:
							c = "I";
							break;
					case	kHIDUsage_KeyboardJ:
							c = "J";
							break;
					case	kHIDUsage_KeyboardK:
							c = "K";
							break;
					case	kHIDUsage_KeyboardL:
							c = "L";
							break;
					case	kHIDUsage_KeyboardM:
							c = "M";
							break;
					case	kHIDUsage_KeyboardN:
							c = "N";
							break;
					case	kHIDUsage_KeyboardO:
							c = "O";
							break;
					case	kHIDUsage_KeyboardP:
							c = "P";
							break;
					case	kHIDUsage_KeyboardQ:
							c = "Q";
							break;
					case	kHIDUsage_KeyboardR:
							c = "R";
							break;
					case	kHIDUsage_KeyboardS:
							c = "S";
							break;
					case	kHIDUsage_KeyboardT:
							c = "T";
							break;
					case	kHIDUsage_KeyboardU:
							c = "U";
							break;
					case	kHIDUsage_KeyboardV:
							c = "V";
							break;
					case	kHIDUsage_KeyboardW:
							c = "W";
							break;
					case	kHIDUsage_KeyboardX:
							c = "X";
							break;
					case	kHIDUsage_KeyboardY:
							c = "Y";
							break;
					case	kHIDUsage_KeyboardZ:
							c = "Z";
							break;

					case	kHIDUsage_Keyboard1:
							c = "1";
							break;
					case	kHIDUsage_Keyboard2:
							c = "2";
							break;
					case	kHIDUsage_Keyboard3:
							c = "3";
							break;
					case	kHIDUsage_Keyboard4:
							c = "4";
							break;
					case	kHIDUsage_Keyboard5:
							c = "5";
							break;
					case	kHIDUsage_Keyboard6:
							c = "6";
							break;
					case	kHIDUsage_Keyboard7:
							c = "7";
							break;
					case	kHIDUsage_Keyboard8:
							c = "8";
							break;
					case	kHIDUsage_Keyboard9:
							c = "9";
							break;
					case	kHIDUsage_Keyboard0:
							c = "0";
							break;

					case	kHIDUsage_KeyboardReturnOrEnter:
							c = "Return";
							break;
					case	kHIDUsage_KeyboardEscape:
							c = "ESC";
							break;
					case	kHIDUsage_KeyboardDeleteOrBackspace:
							c = "Delete";
							break;
					case	kHIDUsage_KeyboardTab:
							c = "Tab";
							break;
					case	kHIDUsage_KeyboardSpacebar:
							c = "Spacebar";
							break;
					case	kHIDUsage_KeyboardHyphen:
							c = " - ";
							break;
					case	kHIDUsage_KeyboardEqualSign:
							c = "=";
							break;
					case	kHIDUsage_KeyboardOpenBracket:
							c = "[";
							break;
					case	kHIDUsage_KeyboardCloseBracket:
							c = "]";
							break;

					case	kHIDUsage_KeyboardBackslash:
							c = "Backslash";
							break;
					case	kHIDUsage_KeyboardNonUSPound:
							c = "Pound";
							break;
					case	kHIDUsage_KeyboardSemicolon:
							c = ";";
							break;
					case	kHIDUsage_KeyboardQuote:
							c = "Quote";
							break;
					case	kHIDUsage_KeyboardGraveAccentAndTilde:
							c = "~ (tilde)";
							break;

					case	kHIDUsage_KeyboardComma:
							c = ",";
							break;
					case	kHIDUsage_KeyboardPeriod:
							c = ".";
							break;
					case	kHIDUsage_KeyboardSlash:
							c = "/";
							break;
					case	kHIDUsage_KeyboardCapsLock:
							c = "CAPSLOCK";
							break;

					case	kHIDUsage_KeyboardF1:
							c = "F1";
							break;
					case	kHIDUsage_KeyboardF2:
							c = "F2";
							break;
					case	kHIDUsage_KeyboardF3:
							c = "F3";
							break;
					case	kHIDUsage_KeyboardF4:
							c = "F4";
							break;
					case	kHIDUsage_KeyboardF5:
							c = "F5";
							break;
					case	kHIDUsage_KeyboardF6:
							c = "F6";
							break;
					case	kHIDUsage_KeyboardF7:
							c = "F7";
							break;
					case	kHIDUsage_KeyboardF8:
							c = "F8";
							break;
					case	kHIDUsage_KeyboardF9:
							c = "F9";
							break;
					case	kHIDUsage_KeyboardF10:
							c = "F10";
							break;
					case	kHIDUsage_KeyboardF11:
							c = "F11";
							break;
					case	kHIDUsage_KeyboardF12:
							c = "F12";
							break;
					case	kHIDUsage_KeyboardF13:
					case	kHIDUsage_KeyboardPrintScreen:
							c = "F13";
							break;
					case	kHIDUsage_KeyboardF14:
					case	kHIDUsage_KeyboardScrollLock:
							c = "F14";
							break;
					case	kHIDUsage_KeyboardF15:
					case	kHIDUsage_KeyboardPause:
							c = "F15";
							break;

					case	kHIDUsage_KeyboardInsert:
							c = "Insert";
							break;
					case	kHIDUsage_KeyboardHome:
							c = "Home";
							break;
					case	kHIDUsage_KeyboardPageUp:
							c = "Page Up";
							break;
					case	kHIDUsage_KeyboardDeleteForward:
							c = "Del";
							break;
					case	kHIDUsage_KeyboardEnd:
							c = "End";
							break;
					case	kHIDUsage_KeyboardPageDown:
							c = "Page Down";
							break;

					case	kHIDUsage_KeyboardRightArrow:
							c = "Right Arrow";
							break;
					case	kHIDUsage_KeyboardLeftArrow:
							c = "Left Arrow";
							break;
					case	kHIDUsage_KeyboardDownArrow:
							c = "Down Arrow";
							break;

					case	kHIDUsage_KeyboardRightControl:		// ** for some reason kHIDUsage_KeyboardRightControl appears to really be the down arrow, but only on 10.2.6, not 10.3
							c = nil;
							break;

					case	kHIDUsage_KeyboardUpArrow:
							c = "Up Arrow";
							break;

					case	kHIDUsage_KeypadNumLock:
							c = "Num Lock / Clear";
							break;
					case	kHIDUsage_KeypadSlash:
							c = "Keypad Slash /";
							break;
					case	kHIDUsage_KeypadAsterisk:
							c = "Keypad Asterisk *";
							break;
					case	kHIDUsage_KeypadHyphen:
							c = "Keypad Hyphen -";
							break;
					case	kHIDUsage_KeypadPlus:
							c = "Keypad Plus +";
							break;
					case	kHIDUsage_KeypadEnter:
							c = "Keypad Enter";
							break;

					case	kHIDUsage_Keypad1:
							c = "Keypad 1";
							break;
					case	kHIDUsage_Keypad2:
							c = "Keypad 2";
							break;
					case	kHIDUsage_Keypad3:
							c = "Keypad 3";
							break;
					case	kHIDUsage_Keypad4:
							c = "Keypad 4";
							break;
					case	kHIDUsage_Keypad5:
							c = "Keypad 5";
							break;
					case	kHIDUsage_Keypad6:
							c = "Keypad 6";
							break;
					case	kHIDUsage_Keypad7:
							c = "Keypad 7";
							break;
					case	kHIDUsage_Keypad8:
							c = "Keypad 8";
							break;
					case	kHIDUsage_Keypad9:
							c = "Keypad 9";
							break;
					case	kHIDUsage_Keypad0:
							c = "Keypad 0";
							break;

					case	kHIDUsage_KeypadPeriod:
							c = "Keypad Period .";
							break;
					case	kHIDUsage_KeyboardNonUSBackslash:
							c = "Keypad Backslash";
							break;

					case	kHIDUsage_KeypadEqualSign:
							c = "Keypad Equal =";
							break;
					case	kHIDUsage_KeyboardHelp:
							c = "Help";
							break;
					case	kHIDUsage_KeypadComma:
							c = "Keypad Comma ,";
							break;

					case	kHIDUsage_KeyboardReturn:
							c = "Return";
							break;

					case	kHIDUsage_KeyboardLeftControl:
							c = "CTRL";
							break;
					case	kHIDUsage_KeyboardLeftShift:
							c = "Shift";
							break;
					case	kHIDUsage_KeyboardLeftAlt:
							c = "Option";
							break;
					case	kHIDUsage_KeyboardLeftGUI:
							c = "Apple/Command";
							break;

#if 0											// NOTE:  HID returns these "right" keys twice even though they don't exist at all!  HID sucks....
					case	kHIDUsage_KeyboardRightControl:
							c = "Right CTRL";
							break;
					case	kHIDUsage_KeyboardRightShift:
							c = "Right Shift";
							break;
					case	kHIDUsage_KeyboardRightAlt:
							c = "Right Option";
							break;
					case	kHIDUsage_KeyboardRightGUI:
							c = "Right Apple/Command";
							break;
#endif

					default:
							c = nil;									// key not supported so pass back nil so that we skip the element
				}
				break;

	}

	return(c);
}



/******************** GET CF NUMBER FROM OBJECT ********************/

static long GetCFNumberFromObject(CFNumberRef object)
{
long number;

	if (object == nil)											// see if was passed something bad
	{
		DoAlert("\pGetCFNumberFromObject: object == nil");
		DoHIDSucksDialog();
		ExitToShell();
	}

	if (CFNumberGetValue(object, kCFNumberLongType, &number))
	{
		return(number);
	}

	return(0);
}



#pragma mark -
#pragma mark ======== HID USER CONFIGURATION ===========


/*********************** DO INPUT CONFIG DIALOG ********************************/

void DoInputConfigDialog(void)
{
ControlID 	idControl;
ControlRef 	control;
OSStatus		err;
EventHandlerRef	ref;
EventTypeSpec	list[] = {   { kEventClassWindow, kEventWindowClose },
                             { kEventClassWindow, kEventWindowDrawContent },
                             { kEventClassControl, kEventControlClick },
                             { kEventClassCommand,  kEventProcessCommand } };
const char		*rezNames[MAX_LANGUAGES] =
{
	"HIDConfigWindow_English",
	"HIDConfigWindow_French",
	"HIDConfigWindow_German",
	"HIDConfigWindow_Spanish",
	"HIDConfigWindow_Italian",
	"HIDConfigWindow_Swedish",
};


    		/* DON'T DO DIALOG IF THERE ARE NO DEVICES */

	if (gNumHIDDevices == 0)
	{
		DoAlert("\pOS X says you don't have any input devices.  If this is not true, try rebooting.");
		DoHIDSucksDialog();
		return;
	}


    Enter2D();


    		/***************/
    		/* INIT DIALOG */
    		/***************/


				/* CREATE WINDOW FROM THE NIB */

    err = CreateWindowFromNib(gNibs, CFStringCreateWithCString(nil, rezNames[gGamePrefs.language],
    						kCFStringEncodingMacRoman), &gHIDConfigWindow);
	if (err)
		DoFatalAlert("\pDoInputConfigDialog: CreateWindowFromNib failed!");


			/* SET DIALOG MENUS */

	gRestoreMenu = nil;
	BuildNeedsMenu(gHIDConfigWindow);
	BuildHIDDeviceMenu(gHIDConfigWindow);
	BuildHIDElementMenu(gHIDConfigWindow);

	DeviceMenuWasSelected(gSelectedDeviceNum);		// call this to prime all the menu selections




			/*****************/
			/* HANDLE EVENTS */
			/*****************/

			/* CREATE NEW WINDOW EVENT HANDLER */

    gConfigWinEvtHandler = NewEventHandlerUPP(MyHIDWindowEventHandler);
    InstallWindowEventHandler(gHIDConfigWindow, gConfigWinEvtHandler, GetEventTypeCount (list), list, 0, &ref);

    ShowWindow(gHIDConfigWindow);



			/* SET DONT-USE-HID CHECKBOX */

    idControl.signature = 'nhid';
    idControl.id 		= 0;
    GetControlByID(gHIDConfigWindow, &idControl, &control);
	SetControlValue(control, gGamePrefs.dontUseHID);

			/* INSTALL A TIMER EVENT */

	InstallEventLoopTimer(GetCurrentEventLoop(), 0, 0.1, InstallTimerEvent(), nil, &gTimer);


			/* START THE EVENT LOOP */

    RunAppModalLoopForWindow(gHIDConfigWindow);


			/***********/
			/* CLEANUP */
			/***********/

    if (gTimer)												// uninstall the event timer
    {
        RemoveEventLoopTimer(gTimer);
		gTimer = nil;
	}

	DisposeWindow(gHIDConfigWindow);						// dispose of the window
	gHIDConfigWindow = nil;
	DisposeEventHandlerUPP(gConfigWinEvtHandler);


	Exit2D();


				/* SAVE SETTINGS */

	SavePrefs();
}


/********************** BUILD NEEDS MENU ***********************/
//
// Do this only once, when the config window is created.
// It scans the Input Sprocket Needs list and builds a pop-up menu from it
//

static void BuildNeedsMenu (WindowRef window)
{
int		j;
char 	*menuStrings[NUM_CONTROL_NEEDS];

	for (j = 0; j < NUM_CONTROL_NEEDS; j++)									// iterate through all of the needs
		menuStrings[j] = &gControlNeeds[j].name[0];							// point to this string

	BuildMenuFromCStrings(window, 'hidm', kCntlPopUpNeeds, menuStrings, NUM_CONTROL_NEEDS, 0);
}

/********************** BUILD HID DEVICE MENU ***********************/

static void BuildHIDDeviceMenu (WindowRef window)
{
short	i;
char 	*menuStrings[MAX_HID_DEVICES];

			/* BUILD A LIST OF PTRS TO THE DEVICE NAMES */

	for (i = 0; i < gNumHIDDevices; i++)
		menuStrings[i] = &gHIDDeviceList[i].deviceName[0];							// point to this string

	BuildMenuFromCStrings(window, 'hidm', kCntlPopUpDevice, menuStrings, gNumHIDDevices, 0);

}


/*********************** BUILD HID ELEMENT MENU ********************************/
//
// builds mneu of elements of current device
//

static void BuildHIDElementMenu (WindowRef window)
{
short	i, numElements;
char 	*menuStrings[MAX_HID_ELEMENTS];

	menuStrings[0] = "< NOT USED >";												// the first menu item is the blank "unassigned" item

	numElements = gHIDDeviceList[gSelectedDeviceNum].numElements;					// get # elements in this device
	for (i = 0; i < numElements; i++)												// then add on the real element items
		menuStrings[i+1] = &gHIDDeviceList[gSelectedDeviceNum].elements[i].name[0];	// point to this string

	BuildMenuFromCStrings(window, 'hidm', kCntlPopUpElement, menuStrings, numElements+1, 0);
}


/********************** BUILD MENU FROM C STRINGS ***************************/

static void BuildMenuFromCStrings(WindowRef window, SInt32 controlSig, SInt32 id, char *textList[], short numItems, short defaultSelection)
{
MenuHandle 	hMenu;
Size 		tempSize;
ControlID 	idControl;
ControlRef 	control;
short 		i;
OSStatus 	err = noErr;
MenuItemIndex	menuItemIndex;

    		/* GET MENU DATA */

    idControl.signature = controlSig;
    idControl.id 		= id;
    err = GetControlByID(window, &idControl, &control);
	if (err)
		DoFatalAlert("\pBuildMenuFromCStrings: GetControlByID failed!");

    GetControlData(control, kControlMenuPart, kControlPopupButtonMenuHandleTag, sizeof (MenuHandle), &hMenu, &tempSize);

    	/* REMOVE ALL ITEMS FROM EXISTING MENU */

    err = DeleteMenuItems(hMenu, 1, CountMenuItems (hMenu));
	if (err)
		DoFatalAlert("\pBuildMenuFromCStrings: DeleteMenuItems failed!");


			/* ADD NEW ITEMS TO THE MENU */

    for (i = 0; i < numItems; i++)
    {
		CFStringRef cfString;

		cfString = CFStringCreateWithCString (nil, textList[i], kCFStringEncodingMacRoman);     	// convert our C string into a CF String

		err = AppendMenuItemTextWithCFString(hMenu, cfString, 0, 0, &menuItemIndex);			// append to menu
		if (err)
			DoFatalAlert("\pBuildMenuFromCStrings: AppendMenuItemTextWithCFString failed!");

		CFRelease(cfString);																	// dispose of the ref to the CF String
	}


			/* FORCE UPDATE OF MENU EXTENTS */

    SetControlMaximum(control, numItems);

	if (defaultSelection >= numItems)					// make sure default isn't over
		defaultSelection = 0;

	SetControlValue(control, defaultSelection+1);

}



/********************** MY HID WINDOW EVENT HANDLER ************************/
//
// main window event handling
//

static pascal OSStatus MyHIDWindowEventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData)
{
#pragma unused (myHandler, userData)
OSStatus			result = eventNotHandledErr;
ControlID 			idControl;
ControlRef 			control;
HICommand 			command;


	switch(GetEventKind(event))
	{
				/* PROCESS COMMAND */

		case	kEventProcessCommand:
				GetEventParameter (event, kEventParamDirectObject, kEventParamHICommand, NULL, sizeof(command), NULL, &command);
				switch(command.commandID)
				{
							/* HIT OK BUTTON */

					case	kHICommandOK:
							QuitAppModalLoopForWindow(gHIDConfigWindow);
							break;

							/* HIT RESET BUTTON */

					case	'rest':
							ResetAllDefaultControls();
							DeviceMenuWasSelected(gSelectedDeviceNum);		// call this to cause all menus to update with the resetted info
							break;

							/* CALIBRATION MIN BUTTON */

					case	'smin':
							gHIDDeviceList[gSelectedDeviceNum].elements[gSelectedElement].calibrationMin = gCurrentCalibrationValue;
							break;

							/* CALIBRATION CENTER BUTTON */

					case	'setc':
							gHIDDeviceList[gSelectedDeviceNum].elements[gSelectedElement].calibrationCenter = gCurrentCalibrationValue;
							break;

							/* CALIBRATION MAX BUTTON */

					case	'smax':
							gHIDDeviceList[gSelectedDeviceNum].elements[gSelectedElement].calibrationMax = gCurrentCalibrationValue;
							break;

							/* CALIBRATION REVERSE AXIS */

					case	'flip':
						    idControl.signature = 'hidm';
						    idControl.id 		= kCntlReverse;
						    GetControlByID(gHIDConfigWindow, &idControl, &control);
							gHIDDeviceList[gSelectedDeviceNum].elements[gSelectedElement].reverseAxis = GetControlValue(control);
							break;


							/* TOGGLE ISACTIVE */

					case	'IsAc':
						    idControl.signature = 'hidm';
						    idControl.id 		= kCntlActiveCheckBox;
						    GetControlByID(gHIDConfigWindow, &idControl, &control);
							gHIDDeviceList[gSelectedDeviceNum].isActive = GetControlValue(control);
							break;

							/* TOGGLE DONT USE HID */

					case	'nhid':
						    idControl.signature = 'nhid';
						    idControl.id 		= 0;
						    GetControlByID(gHIDConfigWindow, &idControl, &control);
							gGamePrefs.dontUseHID = GetControlValue(control);
							break;


							/* HELP / HID SUCKS */

					case	'hidx':
							DoHIDSucksDialog();
							break;

							/* HELP / DONT USE HIT ? */

					case	'nhlp':
							DoAlert("\pOnly check the Don't use HID for Input checkbox if your keyboard is not working with it unchecked.  In this mode your custom control configuration is ignored and you cannot use input devices.  Only the default keyboard setup works.");
							break;


							/************************/
							/* SELECTED A MENU ITEM */
							/************************/

					default:

							if (command.attributes & kHICommandFromMenu)			// see if this command is from a menu
							{
								MenuRef	hMenu;
								Size	tempSize;

										/* SELECTED ITEM IN NEEDS MENU */

							    idControl.signature = 'hidm';
							    idControl.id 		= kCntlPopUpNeeds;
							    GetControlByID(gHIDConfigWindow, &idControl, &control);
							    GetControlData(control, kControlMenuPart, kControlPopupButtonMenuRefTag, sizeof (MenuRef), &hMenu, &tempSize);

								if (hMenu == command.menu.menuRef)
								{
									NeedsMenuWasSelected(command.menu.menuItemIndex - 1);
									break;
								}


									/* SELECTED ITEM IN DEVICES MENU */

							    idControl.id = kCntlPopUpDevice;
							    GetControlByID(gHIDConfigWindow, &idControl, &control);
							    GetControlData(control, kControlMenuPart, kControlPopupButtonMenuRefTag, sizeof (MenuRef), &hMenu, &tempSize);

								if (hMenu == command.menu.menuRef)
								{
									DeviceMenuWasSelected(command.menu.menuItemIndex - 1);
									break;
								}

									/* SELECTED ITEM IN ELEMENTS MENU */

								ElementMenuWasSelected(command.menu.menuItemIndex - 2);
							}
				}
				break;
    }
    return (result);
}



/********************** DEVICE WAS SELECTED **************************/
//
// Called when user manipulates the Devices menu.
//

static void DeviceMenuWasSelected(int deviceNum)
{
ControlID 	idControl;
ControlRef 	control;

	gSelectedDeviceNum = deviceNum;									// assign as the selected device #
	gSelectedElement = -1;											// default to the NONE


				/* UPDATE ELEMENT MENUS */

	if (gHIDConfigWindow)
	{
		BuildHIDElementMenu(gHIDConfigWindow);

		/* RE-SELECT THE SELECTED NEED TO CAUSE ELEMENTS MENU TO SHOW THE RIGHT ITEM */

		NeedsMenuWasSelected(gSelectedNeed);
	}

			/* SET ACTIVE CHECKBOX */


    idControl.signature = 'hidm';
    idControl.id 		= kCntlActiveCheckBox;
    GetControlByID(gHIDConfigWindow, &idControl, &control);
	SetControlValue(control, gHIDDeviceList[deviceNum].isActive);
}


/********************** NEEDS MENU WAS SELECTED **************************/
//
// Called when user manipulates the Needs or Devices menus.
//

static void NeedsMenuWasSelected(int needNum)
{
short		elementNum;
ControlID 	idControl;
ControlRef 	control;

	gSelectedNeed = needNum;


			/* GET THE ELEMENT INDEX FOR THIS NEED WITH THE CURRENTLY SELECTED DEVICE */

	elementNum = gControlNeeds[needNum].elementInfo[gSelectedDeviceNum].elementNum;


					/* UPDATE THE ELEMENTS MENU TO SHOW OUR SELECTION */

	ElementMenuWasSelected(elementNum);

	idControl.signature = 'hidm';
	idControl.id 		= kCntlPopUpElement;
	GetControlByID(gHIDConfigWindow, &idControl, &control);

	SetControlValue(control, elementNum+2);
}


/********************** ELEMENT MENU WAS SELECTED **************************/
//
// Called when user manipulates the Elements menu.
//
// INPUT:  elementNum == the index into the device's element list of -1 if none.
//

static void ElementMenuWasSelected(int elementNum)
{
long	elementType;

		/* MAKE CURRENTLY SELECTED & ASSIGN TO THE NEED */

	gSelectedElement = elementNum;
	gControlNeeds[gSelectedNeed].elementInfo[gSelectedDeviceNum].elementNum = elementNum;

		/* SEE IF HIDE/SHOW THE CALIBRATION CONTROLS */

	if (elementNum == -1)																// if not assigned then hide calibration
		goto hide_calibration;

	elementType = gHIDDeviceList[gSelectedDeviceNum].elements[elementNum].elementType;	// get type
	switch (elementType)
	{
		case	kIOHIDElementTypeInput_Axis:
		case	kIOHIDElementTypeInput_Misc:
				ShowCalibrationControls();
				break;

		default:
hide_calibration:
				HideCalibrationControls();
	}

}


/******************* HIDE CALIBRATION CONTROLS *************************/
//
// The calibration controls are all grouped in a Group Box, so we only need to hide/show that
// single control to cause all includedd controls to hide/show.
//

static void HideCalibrationControls(void)
{
ControlID 	idControl;
ControlRef 	control;

	idControl.signature = 'hidm';
	idControl.id 		= kCntlCalibrationBox;
	GetControlByID(gHIDConfigWindow, &idControl, &control);

	HideControl(control);


	gShowCalibration = false;
}


/******************* SHOW CALIBRATION CONTROLS *************************/
//
// The calibration controls are all grouped in a Group Box, so we only need to hide/show that
// single control to cause all includedd controls to hide/show.
//

static void ShowCalibrationControls(void)
{
ControlID 	idControl;
ControlRef 	control;
long	min,max;

			/*************************************************/
			/* UPDATE THE MIN/MAX VALUES OF THE THERMOMETERS */
			/*************************************************/

					/* RAW */

	min = gHIDDeviceList[gSelectedDeviceNum].elements[gSelectedElement].min;
	max = gHIDDeviceList[gSelectedDeviceNum].elements[gSelectedElement].max;

	idControl.signature = 'hidm';
	idControl.id 		= kCntlRawValueScale;
	GetControlByID(gHIDConfigWindow, &idControl, &control);				// get the thermometer control

	SetControlMinimum(control, min);
	SetControlMaximum(control, max);


					/* CALIBRATED */

	min = -AXIS_RANGE;
	max = AXIS_RANGE;

	idControl.signature = 'hidm';
	idControl.id 		= kCntlCalibratedValueScale;
	GetControlByID(gHIDConfigWindow, &idControl, &control);				// get the thermometer control

	SetControlMinimum(control, min);
	SetControlMaximum(control, max);






			/* UPDATE THE REVERSE CHECKBOX */

    idControl.signature = 'hidm';
    idControl.id 		= kCntlReverse;
    GetControlByID(gHIDConfigWindow, &idControl, &control);
	SetControlValue(control, gHIDDeviceList[gSelectedDeviceNum].elements[gSelectedElement].reverseAxis);


			/* MAKE THE CALIBRATION GROUP BOX VISIBLE */

	idControl.signature = 'hidm';
	idControl.id 		= kCntlCalibrationBox;
	GetControlByID(gHIDConfigWindow, &idControl, &control);

	ShowControl(control);

	gShowCalibration = true;
}




#pragma mark -

/********************** INSTALL TIMER CALLBACK ************************/
//
// Installs a timer event which will get called so that we can do updates and stuff during this dialog
//

static EventLoopTimerUPP InstallTimerEvent(void)
{
static EventLoopTimerUPP	sTimerUPP = NULL;

	if (sTimerUPP == NULL)
		sTimerUPP = NewEventLoopTimerUPP (IdleTimerCallback);

	return sTimerUPP;
}


/******************** IDLE TIMER CALLBACK ****************************/
//
// This function is called by the Event Loop.  Here we check if the user
// has pressed anything of interest, and we can update our calibration info.
//

static pascal void IdleTimerCallback (EventLoopTimerRef inTimer, void* userData)
{
short					d;
short					e, numElements;
long					elementType;
IOHIDElementCookie		cookie;
HRESULT 				result;
IOHIDEventStruct 		hidEvent;
IOHIDDeviceInterface 	**hidDeviceInterface;
ControlID 				idControl;
ControlRef 				control;

#pragma unused (inTimer, userData)

	d = gSelectedDeviceNum;
	hidDeviceInterface = gHIDDeviceList[d].hidDeviceInterface;						// get hid interface for the current device

			/***************************************************/
			/* SCAN THE CURRENT DEVICE FOR ANY PRESSED BUTTONS */
			/***************************************************/

	numElements = gHIDDeviceList[d].numElements;									// get # elements in this device

	for (e = 0; e < numElements; e++)												// test each element
	{
				/* ONLY LOOK FOR BUTTONS */

		elementType = gHIDDeviceList[d].elements[e].elementType;					// get type
		if (elementType == kIOHIDElementTypeInput_Button)
		{
			cookie = gHIDDeviceList[d].elements[e].cookie;							// get cookie

					/* READ THE VALUE FROM THE ELEMENT */
					//
					// Note:  	I've found that if any device is plugged in after a reboot, yet
					// 			no buttons have been pressed, then getElementValue() will return
					//			an error the first time.  Therefore, we need to handle errors
					//			gracefully by just setting the value to 0.
					//

			result = (*hidDeviceInterface)->getElementValue(hidDeviceInterface,	cookie, &hidEvent);		// poll this element
			if (result)
				hidEvent.value = 0;

					/* SEE IF THIS IS PRESSED */
			else
			if (hidEvent.value > 0)													// is the button pressed?
			{
				ElementMenuWasSelected(e);											// select this Element

				idControl.signature = 'hidm';										// update the pop-up menu
				idControl.id 		= kCntlPopUpElement;
				GetControlByID(gHIDConfigWindow, &idControl, &control);
				SetControlValue(control, e+2);
				break;
			}
		}
	}



			/**********************************/
			/* UPDATE CALIBRATION INFORMATION */
			/**********************************/

	if (gShowCalibration)
	{
        char 		text[256];
        long		calibratedValue;

		cookie = gHIDDeviceList[d].elements[gSelectedElement].cookie;							// get cookie

				/* GET THE CURRENT VALUE OF THE AXIS */

		result = (*hidDeviceInterface)->getElementValue(hidDeviceInterface,	cookie, &hidEvent);		// poll this element
		if (result)
		{
			DoAlert("\pIdleTimerCallback: getElementValue failed!");
			DoHIDSucksDialog();
			ExitToShell();
		}

		gCurrentCalibrationValue = hidEvent.value;							// extract its value


				/* UPDATE THE RAW THERMOMETER */

		idControl.signature = 'hidm';
		idControl.id 		= kCntlRawValueScale;
		GetControlByID(gHIDConfigWindow, &idControl, &control);				// get the thermometer control

		SetControlValue(control, gCurrentCalibrationValue);
		Draw1Control (control);


			/* UPDATE THE CALIBRATED THERMOMETER */

		idControl.signature = 'hidm';
		idControl.id 		= kCntlCalibratedValueScale;
		GetControlByID(gHIDConfigWindow, &idControl, &control);				// get the thermometer control

		calibratedValue = CalibrateElementValue(gCurrentCalibrationValue, d, gSelectedElement,
												gHIDDeviceList[d].elements[gSelectedElement].elementType);

		SetControlValue(control, calibratedValue);
		Draw1Control (control);



			/* UPDATE THE RAW VALUE DISPLAY */

		idControl.signature = 'hidm';
		idControl.id 		= kCntlTextRawValueElement;
		GetControlByID(gHIDConfigWindow, &idControl, &control);				// get the thermometer control

        sprintf (text, "%3ld", gCurrentCalibrationValue);
        SetControlData (control, kControlNoPart, kControlStaticTextTextTag, strlen(text), text);
		Draw1Control (control);
	}
}

#pragma mark -

/****************** RESET ELEMENT CALIBRATION ***********************/

static void ResetElementCalibration(short deviceNum, short elementNum)
{
long	min, max;

	min = gHIDDeviceList[deviceNum].elements[elementNum].min;
	max = gHIDDeviceList[deviceNum].elements[elementNum].max;



	gHIDDeviceList[deviceNum].elements[elementNum].reverseAxis 		= false;
	gHIDDeviceList[deviceNum].elements[elementNum].calibrationMin 	= min;
	gHIDDeviceList[deviceNum].elements[elementNum].calibrationMax 	= max;
	gHIDDeviceList[deviceNum].elements[elementNum].calibrationCenter = (min+max)/2;
}


/******************* RESET ALL DEFAULT CONTROLS **************************/
//
// This sets the control configuration to our desired default settings
//

static void ResetAllDefaultControls(void)
{
short	n, d, keyboardDevice, e;

		/****************************************************************************/
		/* FIRST GO THRU THE DEVICE LIST AND DEACTIVATE EVERYTHING BUT THE KEYBOARD */
		/****************************************************************************/

	keyboardDevice = -1;

	for (d = 0; d < gNumHIDDevices; d++)
	{
		switch(gHIDDeviceList[d].usagePage)
		{
			case	kHIDPage_GenericDesktop:
					switch(gHIDDeviceList[d].usage)
					{
						case	kHIDUsage_GD_Keyboard:
								gHIDDeviceList[d].isActive = true;
								keyboardDevice = d;
								break;

						default:
								gHIDDeviceList[d].isActive = false;

					}
					break;

			case	kHIDPage_KeyboardOrKeypad:
					gHIDDeviceList[d].isActive = true;
					keyboardDevice = d;
					break;

			default:
					gHIDDeviceList[d].isActive = false;
		}
	}


			/*********************************************/
			/* NEXT RESET ALL THE INFO IN THE NEEDS LIST */
			/*********************************************/

	for (n = 0; n < NUM_CONTROL_NEEDS; n++)
	{
    	for (d = 0; d < MAX_HID_DEVICES; d++)
    	{
    		gControlNeeds[n].elementInfo[d].elementNum 			= -1;			// set as UNASSIGNED
    		gControlNeeds[n].elementInfo[d].elementCurrentValue	= 0;
    	}
	}


			/*************************************/
			/* NEXT SET OUR DEFAULT KEY CONTROLS */
			/*************************************/

	if (keyboardDevice == -1)											// verify that we have a keyboard
	{
		DoAlert("\pCannot initialize default game controls because no keyboard was found.  Try rebooting.");
		DoHIDSucksDialog();
		return;
	}


			/* SCAN NEEDS LIST AND SET KEY ELEMENTS */

	for (n = 0; n < NUM_CONTROL_NEEDS; n++)
	{
		long	defaultKey = gControlNeeds[n].defaultKeyboardKey;	// get the default keyboard key (usage) value for this need


			/* SEARCH FOR THIS KEY IN THE KEYBOARD ELEMENT LIST */

		for (e = 0; e < gHIDDeviceList[keyboardDevice].numElements; e++)
		{
			if (gHIDDeviceList[keyboardDevice].elements[e].usage == defaultKey)	// is this the key element we're looking for?
			{
				gControlNeeds[n].elementInfo[keyboardDevice].elementNum = e;	// set this need to use this element of the keyboard device
				break;
			}
		}
	}
}



#pragma mark -
#pragma mark ========= HID DEVICE POLLING ===========


/******************** POLL ALL INPUT NEEDS ***********************/
//
// This is the main input update function which should be called once per loop.
// It scans the Needs list and updates the status of all of the Elements assigned to each Need.
//

static void PollAllHIDInputNeeds(void)
{
short					n, d, e;
IOHIDElementCookie		cookie;
IOHIDDeviceInterface 	**hidDeviceInterface;
IOHIDEventStruct 		hidEvent;
long					bestValue, value, elementType;
HRESULT 				result;

	if (!gHIDInitialized)
		return;


			/* SEE IF USER HAS HID DISABLED FOR MANUAL GETKEYS() KEYBOARD READS */
			//
			// This is a hack I had to put in so that people who couldn't get the game to work with HID could still use GetKeys() to do it
			//

	if (gGamePrefs.dontUseHID)
	{
		for (n = 0; n < NUM_CONTROL_NEEDS; n++)
		{
			u_long value;

			gControlNeeds[n].oldValue = gControlNeeds[n].value;				// remember what the old value was before we update it

			switch(n)
			{
				case	0:								// kNeed_TurnLeft_Key
						gControlNeeds[n].value = value= GetKeyState(KEY_LEFT);
						break;


				case	1:								// kNeed_TurnRight_Key
						gControlNeeds[n].value = value = GetKeyState(KEY_RIGHT);
						break;

				case	2:								// kNeed_Forward
						gControlNeeds[n].value = value = GetKeyState(KEY_UP);
						break;

				case	3:								// kNeed_Backward
						gControlNeeds[n].value = value = GetKeyState(KEY_DOWN);
						break;

				case	6:								// kNeed_Kick
						gControlNeeds[n].value = value = GetKeyState(KEY_APPLE);
						break;

				case	7:								// kNeed_PickupDrop
						gControlNeeds[n].value = value = GetKeyState(KEY_OPTION);
						break;

				case	8:								// kNeed_AutoWalk
						gControlNeeds[n].value = value = GetKeyState(KEY_SHIFT);
						break;

				case	9:								// kNeed_Jump
						gControlNeeds[n].value = value = GetKeyState(KEY_SPACE);
						break;

				case	10:								// kNeed_LaunchBuddy
						gControlNeeds[n].value = value = GetKeyState(KEY_TAB);
						break;

				case	11:								// kNeed_CameraMode
						gControlNeeds[n].value = value = GetKeyState(KEY_C);
						break;

				case	12:								// kNeed_CameraLeft
						gControlNeeds[n].value = value = GetKeyState(KEY_COMMA);
						break;

				case	13:								// kNeed_CameraRight
						gControlNeeds[n].value = value = GetKeyState(KEY_PERIOD);
						break;


				default:
						value = 0;


			}

			if (value && (gControlNeeds[n].oldValue == 0))			// is this a new button press?
				gControlNeeds[n].newButtonPress	= true;
			else
				gControlNeeds[n].newButtonPress	= false;

		}
		return;
	}



			/******************************************/
			/* SCAN THRU ALL OF THE NEEDS IN OUR LIST */
			/******************************************/

	for (n = 0; n < NUM_CONTROL_NEEDS; n++)
	{
		gControlNeeds[n].oldValue = gControlNeeds[n].value;				// remember what the old value was before we update it

		bestValue = 0;													// init the best value


				/* SCAN THRU ALL OF THE DEVICES */

		for (d = 0; d < gNumHIDDevices; d++)
		{
			if (gHIDDeviceList[d].isActive == false)					// skip any inactive devices
				continue;


				/*********************************************/
				/* UPDATE THE STATUS OF THE ASSIGNED ELEMENT */
				/*********************************************/


			e					= gControlNeeds[n].elementInfo[d].elementNum;			// get the index into the device's Element list
			if (e == -1)																// if it's UNDEFINED then skip it
				continue;

			elementType 		= gHIDDeviceList[d].elements[e].elementType;			// get element type
			hidDeviceInterface 	= gHIDDeviceList[d].hidDeviceInterface;					// get hid interface for this device
			cookie				= gHIDDeviceList[d].elements[e].cookie;					// get cookie


						/* GET ELEMENT'S VALUE */

			result = (*hidDeviceInterface)->getElementValue(hidDeviceInterface,	cookie, &hidEvent);		// poll this element
			if (result != noErr)
				value = hidEvent.value = 0;						// NOTE:  HID is buggy as hell!  Sometimes devices return errors until a button has been pressed
			else
				value = hidEvent.value;

						/* CALIBRATE THE VALUE */

			value = CalibrateElementValue(value, d, e, elementType);


						/* SEE IF IT'S THE BEST VALUE SO FAR */

			if (abs(value) > abs(bestValue))
				bestValue = value;
		}


				/* SPECIAL CHECK FOR MOUSE BUTTON FIRE */

		if (n == kNeed_Shoot)
		{
			if (gMouseButtonState)
				bestValue = 1;
		}

				/*************************************/
				/* SAVE THE BEST VALUE AS THE RESULT */
				/*************************************/

		gControlNeeds[n].value = bestValue;

		if ((bestValue != 0) && (gControlNeeds[n].oldValue == 0))			// is this a new button press?
			gControlNeeds[n].newButtonPress	= true;
		else
			gControlNeeds[n].newButtonPress	= false;
	}

}


/********************** CALIBRATE ELEMENT VALUE ***************************/

static long CalibrateElementValue(long inValue, short deviceNum, short elementNum, long elementType)
{
long	calibratedValue, dist;
long	calMin, calMax, calCenter;
long	tolerance, range;
float	scale;

				/* BUTTONS DON'T NEED CALIBRATION */

	if (elementType == kIOHIDElementTypeInput_Button)
		return(inValue);


				/* GET OUR CALIBRATION VALUES FOR THIS ELEMENT */

	calMin = gHIDDeviceList[deviceNum].elements[elementNum].calibrationMin;
	calMax = gHIDDeviceList[deviceNum].elements[elementNum].calibrationMax;
	calCenter = gHIDDeviceList[deviceNum].elements[elementNum].calibrationCenter;


				/* CALC DIST FROM CENTER & CHECK "NULL" ZONE */
				//
				// Most joysticks/thumbsticks have horrible accuracy, and "center" can vary wildly from jiggle to jiggle.
				// So, for our application we're going to just snap any values that are "close" to center all the way to center.
				//


	dist = inValue - calCenter;											// this makes the center == 0 with left < 0 and right > 0

	tolerance = ((calMax-calCenter) / 10);								// tolerance == 1/nth of dist between center and max
	if (abs(dist) < tolerance)											// if within tolerance then just snap value to center, 0
		dist = 0;


					/* CALIBRATE FOR LEFT OF CENTER */

	if (dist < 0)
	{
		range = calCenter - calMin;										// calc calibrated range on left of zero
		if (range < 1)													// avoid any accidental divides by zero
			range = 1;

		scale = (float)dist / (float)range;								// calc scale factor
		if (scale < -1.0f)												// make sure -1 to 0 (can go outside this if inValue is outside of calibrated range)
			scale = -1.0f;
		else
		if (scale > 0.0f)
			scale = 0.0f;
	}

					/* CALIBRATE FOR RIGHT OF CENTER */
	else
	if (dist > 0)
	{
		range = calMax - calCenter;										// calc calibrated range on left of zero
		if (range < 1)													// avoid any accidental divides by zero
			range = 1;

		scale = (float)dist / (float)range;								// calc scale factor
		if (scale < 0.0f)												// make sure 0 to +1 (can go outside this if inValue is outside of calibrated range)
			scale = 0.0f;
		else
		if (scale > 1.0f)
			scale = 1.0f;
	}
				/* CALIBRATE EXACTLY TO CENTER */
	else
		return(0);


				/* SCALE TO -AXIS_RANGE TO +AXIS_RANGE */
				//
				// Different devices will have different min/max axis values.
				// So, we'll calibrate all of ours to be +/- AXIS_RANGE
				//

	calibratedValue	= scale * AXIS_RANGE;

	if (gHIDDeviceList[deviceNum].elements[elementNum].reverseAxis)			// see if reverse it
		calibratedValue = -calibratedValue;


	return(calibratedValue);
}



#pragma mark -
#pragma mark ========== HID SETTINGS ===========


/********************* BUILD HID CONTROL SETTINGS **************************/
//
// This call builds a database of information for saving the current control settings.
//

void BuildHIDControlSettings(HIDControlSettingsType *settings)
{
long	d, e, n;

	if (!gHIDInitialized)										// verify that we've got legit data to build
	{
		DoFatalAlert("\pBuildHIDControlSettings: HID isn't initialized!");
	}


				/***************************/
				/* SAVE DEVICE INFORMATION */
				/***************************/

			/* SAVE SOME CONSTANTS SO WE DONT SCREW UP LATER IF WE CHANGE THEM */
			//
			// If we save a certain amount of data, but change any of the constants later then
			// when we try to read the data we're not going to be able to decypher it because
			// the arrays won't match up.  So, when reading this data back later, we need to
			// compare these saved constants with their current values.  If they don't match
			// then we cannot read in this data and we must just toss it out and make the user
			// reset their settings.
			//


	settings->maxNeeds 		= NUM_CONTROL_NEEDS;
	settings->maxDevices 	= MAX_HID_DEVICES;
	settings->maxElement 	= MAX_HID_ELEMENTS;


		/* REMEMBER WHICH DEVICES ARE SET TO ACTIVE */

	settings->numDevices = gNumHIDDevices;								// remember how many HID devices we're building data for

	for (d = 0; d < MAX_HID_DEVICES; d++)
		settings->isActive[d] = gHIDDeviceList[d].isActive;


		/* REMEMBER VENDOR & PRODUCT ID'S & TWIN COUNT FOR THE DEVICES */

	for (d = 0; d < MAX_HID_DEVICES; d++)
	{
		settings->vendorID[d]		= gHIDDeviceList[d].vendorID;
		settings->productID[d]		= gHIDDeviceList[d].productID;
		settings->hasDuplicate[d] 	= gHIDDeviceList[d].hasDuplicate;
		settings->locationID[d] 	= gHIDDeviceList[d].locationID;
	}


			/* REMEMBER CALIBRATION INFORMATION */

	for (d = 0; d < MAX_HID_DEVICES; d++)
	{
		for (e = 0; e < MAX_HID_ELEMENTS; e++)
		{
			settings->calibrationMin[d][e] 		= gHIDDeviceList[d].elements[e].calibrationMin;
			settings->calibrationMax[d][e] 		= gHIDDeviceList[d].elements[e].calibrationMax;
			settings->calibrationCenter[d][e] 	= gHIDDeviceList[d].elements[e].calibrationCenter;
			settings->reverseAxis[d][e] 		= gHIDDeviceList[d].elements[e].reverseAxis;
		}
	}



				/**************************/
				/* SAVE NEEDS INFORMATION */
				/**************************/


	for (n = 0; n < NUM_CONTROL_NEEDS; n++)
	{
		for (d = 0; d < MAX_HID_DEVICES; d++)							// save information for each device
		{
				/* GET ELEMENT # THAT THIS NEED USES & SAVE IT */

			e = gControlNeeds[n].elementInfo[d].elementNum;				// which element # does this Need use in this Device?
			settings->needsElementNum[n][d] = e;
		}
	}
}


/********************* RESTORE HID CONTROL SETTINGS ***************************/

void RestoreHIDControlSettings(HIDControlSettingsType *settings)
{
long	d, e, n;
long	matchingDevice;

	if (!gHIDInitialized)												// verify that we've got legit data already
		DoFatalAlert("\pRestoreHIDControlSettings: HID isn't initialized!");

			/* VERIFY THAT WE'VE STILL GOT THE SAME CONSTANTS */

	if ((settings->maxNeeds != NUM_CONTROL_NEEDS) ||
		(settings->maxDevices != MAX_HID_DEVICES) ||
		(settings->maxElement != MAX_HID_ELEMENTS))
		return;															// bail out - this will leave the default settings in place


			/***************************/
			/* RESTORE DEVICE SETTINGS */
			/***************************/

	for (d = 0; d < settings->numDevices; d++)							// loop thru all the devices we saved in the settings data
	{
		matchingDevice = FindMatchingDevice(settings->vendorID[d], settings->productID[d], settings->hasDuplicate[d], settings->locationID[d]);
		if (matchingDevice == -1)
			continue;

					/* RESTORE "ISACTIVE" INFO */

		gHIDDeviceList[matchingDevice].isActive = settings->isActive[d];


				/* RESTORE CALIBRATION INFO FOR EACH ELEMENT IN THIS DEVICE */

		for (e = 0; e < gHIDDeviceList[matchingDevice].numElements; e++)
		{
			gHIDDeviceList[matchingDevice].elements[e].calibrationMin = settings->calibrationMin[d][e];
			gHIDDeviceList[matchingDevice].elements[e].calibrationMax = settings->calibrationMax[d][e];
			gHIDDeviceList[matchingDevice].elements[e].calibrationCenter = settings->calibrationCenter[d][e];
			gHIDDeviceList[matchingDevice].elements[e].reverseAxis = settings->reverseAxis[d][e];;
		}

	}


		/**************************************************************/
		/* TRY TO ASSIGN THE SAVED NEEDS ELEMENTS TO EXISTING DEVICES */
		/**************************************************************/

	for (n = 0; n < NUM_CONTROL_NEEDS; n++)
	{
		for (d = 0; d < MAX_HID_DEVICES; d++)							// save information for each device
		{
					/* DOES THIS DEVICE STILL EXIST? */

			matchingDevice = FindMatchingDevice(settings->vendorID[d], settings->productID[d], settings->hasDuplicate[d], settings->locationID[d]);
			if (matchingDevice == -1)
				continue;

					/* YEP, IT EXISTS, SO LETS SET OUR STUFF */

			e = settings->needsElementNum[n][d];							// get the element # we want to set it to

			gControlNeeds[n].elementInfo[matchingDevice].elementNum = e;	// set it in the matching device's field
		}
	}
}



/********************* FIND MATCHING DEVICE *********************************/
//
// Scans our existing device list looking for a match based on the input values.
//
// Returns -1 if no match found
//

static short FindMatchingDevice(long vendorID, long productID, Boolean hasDuplicate, long locationID)
{
short	d;

	for (d = 0; d < gNumHIDDevices; d++)
	{
		if (vendorID != gHIDDeviceList[d].vendorID)						// does vendor ID match?
			continue;

		if (productID != gHIDDeviceList[d].productID)					// does product ID match?
			continue;

		if (hasDuplicate)												// only care about location ID's if there's more than 1 of this same device
		{
			if (locationID != gHIDDeviceList[d].locationID)					// does location ID match?
				continue;
		}

		return(d);
	}

	return(-1);													// if it gets here then no match was found
}



#pragma mark -
#pragma mark =========== KEYMAP ===========

/**************** UPDATE KEY MAP *************/
//
// This reads the KeyMap and sets a bunch of new/old stuff.
//

static void UpdateKeyMap(void)
{
int		i;

	GetKeys((void *)gKeyMap);

			/* CALC WHICH KEYS ARE NEW THIS TIME */

	for (i = 0; i <  16; i++)
		gNewKeys[i] = (gOldKeys[i] ^ gKeyMap[i]) & gKeyMap[i];


			/* REMEMBER AS OLD MAP */

	for (i = 0; i <  16; i++)
		gOldKeys[i] = gKeyMap[i];



		/*****************************************************/
		/* WHILE WE'RE HERE LET'S DO SOME SPECIAL KEY CHECKS */
		/*****************************************************/

				/* SEE IF QUIT GAME */

	if (GetKeyState(KEY_Q) && GetKeyState(KEY_APPLE))			// see if real key quit
		CleanQuit();


}


/****************** GET KEY STATE ***********/

Boolean GetKeyState(unsigned short key)
{
	//return (BitTst(gKeyMap, key));

	return ( ( gKeyMap[key>>3] >> (key & 7) ) & 1);


//unsigned char *keyMap;

//	keyMap = (unsigned char *)&gKeyMap;
//	return ( ( keyMap[key>>3] >> (key & 7) ) & 1);
}


/****************** GET NEW KEY STATE ***********/

Boolean GetNewKeyState(unsigned short key)
{
//  return !!(BitTst(gNewKeys, (sizeof(KeyMapByteArray)*8) - key));

	return ( ( gNewKeys[key>>3] >> (key & 7) ) & 1);



//unsigned char *keyMap;

//	keyMap = (unsigned char *)&gNewKeys;
//	return ( ( keyMap[key>>3] >> (key & 7) ) & 1);
}


/***************** ARE ANY NEW KEYS PRESSED ****************/

Boolean AreAnyNewKeysPressed(void)
{
int		i;

	for (i = 0; i < 16; i++)
	{
		if (gNewKeys[i])
			return(true);
	}

	return(false);
}


#pragma mark -
#pragma mark ========== MOUSE DELTAS ============


/***************** UPDATE MOUSE DELTA *****************/
//
// Call this only once per frame.
//

static void UpdateMouseDeltas(void)
{
EventRecord   theEvent;

				/* UPDATE DELTAS */

	gReadMouseDeltasTimer -= gFramesPerSecondFrac;			// regulate the rate that we read mouse deltas so we don't go too fast and get just 0's
	if (gReadMouseDeltasTimer <= 0.0f)
	{
		gReadMouseDeltasTimer += .1f;						// read 10 times per second

		gMouseDeltaX = gMouseDeltaY = 0;		 			// assume no deltas will be gotten

		GetNextEvent(kEventMouseMoved, &theEvent);			// getting a Mouse Moved event should trigger the Mouse Event Handler below

		FlushEvents(everyEvent, 0);							// keep the event queue flushed of other key events and stuff
	}

		/* WHILE WE'RE HERE UPDATE THE MOUSE BUTTON STATE */

	if (Button())											// is mouse button down?
	{
		if (!gMouseButtonState)								// is this a new click?
			gMouseNewButtonState = gMouseButtonState = true;
		else
			gMouseNewButtonState = false;
	}
	else
	{
		gMouseButtonState = gMouseNewButtonState = false;
	}

}


/**************** MY MOUSE EVENT HANDLER *************************/
//
// Every time WaitNextEvent() is called this callback will be invoked.
//

static pascal OSStatus MyMouseEventHandler(EventHandlerCallRef eventhandler, EventRef pEventRef, void *userdata)
{
OSStatus			result = eventNotHandledErr;
OSStatus			theErr = noErr;
Point				qdPoint;
#pragma unused (eventhandler, userdata)

	switch (GetEventKind(pEventRef))
	{
		case	kEventMouseMoved:
		case	kEventMouseDragged:
				theErr = GetEventParameter(pEventRef, kEventParamMouseDelta, typeQDPoint,
										nil, sizeof(Point), nil, &qdPoint);

				gMouseDeltaX = qdPoint.h;
				gMouseDeltaY = qdPoint.v;

				result = noErr;
				break;

	}
     return(result);
}


/******************* INSTALL MOUSE EVENT HANDLER ***********************/

static void Install_MouseEventHandler(void)
{
EventTypeSpec			mouseEvents[] = {{kEventClassMouse, kEventMouseMoved},	{kEventClassMouse, kEventMouseDragged}};

	// Set up the handler to capture mouse movements.

	if (gMouseEventHandlerUPP == nil)
		gMouseEventHandlerUPP = NewEventHandlerUPP(MyMouseEventHandler);

	if (gMouseEventHandlerRef == 0)
	{
		//	make sure we start the deltas at 0 when we setup the handler

		gMouseDeltaX = 0;
		gMouseDeltaY = 0;

		//	install the handler

		InstallEventHandler(GetApplicationEventTarget(), gMouseEventHandlerUPP,	2, mouseEvents, nil, &gMouseEventHandlerRef);
	}
}

#if 0
/******************* REMOVE MOUSE EVENT HANDLER *************************/

static void Remove_MouseEventHandlers(void)
{

	//	if the handler has been installed, remove it

	if (gMouseEventHandlerRef != 0) {
		RemoveEventHandler(gMouseEventHandlerRef);
		gMouseEventHandlerRef = 0;

		DisposeEventHandlerUPP(gMouseEventHandlerUPP);
		gMouseEventHandlerUPP = nil;
	}

}
#endif

#pragma mark -


/*********************** DO HID SUCKS DIALOG ********************************/

static void DoHIDSucksDialog(void)
{
static WindowRef 		sucksDialogWindow = nil;
static EventHandlerUPP 	sucksWinEvtHandler;

OSStatus		err;
EventHandlerRef	ref;
EventTypeSpec	list[] = {   { kEventClassWindow, kEventWindowClose },
                             { kEventClassWindow, kEventWindowDrawContent },
                             { kEventClassControl, kEventControlClick },
                             { kEventClassCommand,  kEventProcessCommand } };

    Enter2D();

				/* CREATE WINDOW FROM THE NIB */

    err = CreateWindowFromNib(gNibs, CFSTR ("HIDSucks"), &sucksDialogWindow);
	if (err)
		DoFatalAlert("\pDoHIDSucksDialog: CreateWindowFromNib failed!");


			/* CREATE NEW WINDOW EVENT HANDLER */

    sucksWinEvtHandler = NewEventHandlerUPP(HIDSucksEventHandler);
    InstallWindowEventHandler(sucksDialogWindow, sucksWinEvtHandler, GetEventTypeCount (list), list, sucksDialogWindow, &ref);

    ShowWindow(sucksDialogWindow);
    BringToFront(sucksDialogWindow);
	SelectWindow(sucksDialogWindow);

			/* START THE EVENT LOOP */

    RunAppModalLoopForWindow(sucksDialogWindow);



	DisposeWindow(sucksDialogWindow);						// dispose of the window
	DisposeEventHandlerUPP(sucksWinEvtHandler);


	Exit2D();

}

/********************** HID SUCKS EVENT HANDLER ************************/
//
// main window event handling
//

static pascal OSStatus HIDSucksEventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData)
{
#pragma unused (myHandler)
OSStatus			result = eventNotHandledErr;
HICommand 			command;


	switch(GetEventKind(event))
	{
				/* PROCESS COMMAND */

		case	kEventProcessCommand:
				GetEventParameter (event, kEventParamDirectObject, kEventParamHICommand, NULL, sizeof(command), NULL, &command);
				switch(command.commandID)
				{
							/* HIT OK BUTTON */

					case	kHICommandOK:
							QuitAppModalLoopForWindow((WindowRef) userData);
							break;

				}
				break;
    }
    return result;
}




