/****************************/
/*        WINDOWS           */
/* (c)2005 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/



extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode	*gCurrentNode,*gFirstNodePtr;
extern	float	gFramesPerSecondFrac;
extern	short	gPrefsFolderVRefNum,gCurrentSong;
extern	long	gPrefsFolderDirID;
extern	Boolean				gMuteMusicFlag;
extern	PrefsType			gGamePrefs;
extern	Boolean			gSongPlayingFlag;
extern	SDL_GLContext		gAGLContext;
extern	SDL_Window			*gSDLWindow;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveFadeEvent(ObjNode *theNode);

#if 0	// srcport rm
static void CreateDisplayModeList(void);
pascal void ModeListCallback(void *userData, DMListIndexType a, DMDisplayModeListEntryPtr displaymodeInfo);
pascal void ShowVideoMenuSelection (DialogPtr dlogPtr, short item);
static void CalcVRAMAfterBuffers(void);
static void GetDisplayVRAM(void);
static pascal OSStatus DoScreenModeDialog_EventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData);
static void BuildResolutionMenu(void);
#endif


/****************************/
/*    CONSTANTS             */
/****************************/

#define		DO_GAMMA	1

typedef struct
{
	int		rezH,rezV;
	int		hz;
}VideoModeType;


#define	MAX_VIDEO_MODES		50


/**********************/
/*     VARIABLES      */
/**********************/

u_long			gDisplayVRAM = 0;
u_long			gVRAMAfterBuffers = 0;


#if 0	// srcport rm
WindowRef 		gDialogWindow = nil;
EventHandlerUPP gWinEvtHandler;
#endif


static short				gNumVideoModes = 0;
static VideoModeType		gVideoModeList[MAX_VIDEO_MODES];


float		gGammaFadePercent = 1.0;

int				gGameWindowWidth, gGameWindowHeight;

short			g2DStackDepth = 0;


Boolean			gPlayFullScreen;
#if 0	// srcport - awaiting reimplementation
static	CGDirectDisplayID	gCGDisplayID;

CGGammaValue gOriginalRedTable[256];
CGGammaValue gOriginalGreenTable[256];
CGGammaValue gOriginalBlueTable[256];


static CGGammaValue gGammaRedTable[256];
static CGGammaValue gGammaGreenTable[256];
static CGGammaValue gGammaBlueTable[256];
#endif


float		gGammaTweak = 1.0f;


/****************  INIT WINDOW STUFF *******************/

void InitWindowStuff(void)
{
#if 0		// srcport - awaiting reimplementation
Rect				r;
float				w,h;
CFDictionaryRef 	refDisplayMode = 0;
CGTableCount 		sampleCount;


	gPlayFullScreen = true;				//!gGamePrefs.playInWindow;

	if (gPlayFullScreen)
	{

		CGDisplayCapture(gCGDisplayID);

				/* FIND BEST MATCH */

		refDisplayMode = CGDisplayBestModeForParametersAndRefreshRate(gCGDisplayID, gGamePrefs.depth,
																	gGamePrefs.screenWidth, gGamePrefs.screenHeight,
																	gGamePrefs.hz,
																	NULL);
		if (refDisplayMode == nil)
			DoFatalAlert("InitWindowStuff: CGDisplayBestModeForParameters failed!");


				/* SWITCH TO IT */

		CGDisplaySwitchToMode (gCGDisplayID, refDisplayMode);


					/* GET GDEVICE & INFO */

		DMGetGDeviceByDisplayID ((DisplayIDType)gCGDisplayID, &gGDevice, false);

		w = gGamePrefs.screenWidth;
		h = gGamePrefs.screenHeight;

		r.top  		= (short) ((**gGDevice).gdRect.top + ((**gGDevice).gdRect.bottom - (**gGDevice).gdRect.top) / 2);	  	// h center
		r.top  		-= (short) (h / 2);
		r.left  	= (short) ((**gGDevice).gdRect.left + ((**gGDevice).gdRect.right - (**gGDevice).gdRect.left) / 2);		// v center
		r.left  	-= (short) (w / 2);
		r.right 	= (short) (r.left + w);
		r.bottom 	= (short) (r.top + h);




					/* GET ORIGINAL GAMMA TABLE */

		CGGetDisplayTransferByTable(gCGDisplayID, 256, gOriginalRedTable, gOriginalGreenTable, gOriginalBlueTable, &sampleCount);

	}


				/********************/
				/* INIT SOME WINDOW */
				/********************/
	else
	{
		int			w;
		gGDevice = GetMainDevice();

				/* SIZE WINDOW TO HALF SCREEN SIZE */

		w = (**gGDevice).gdRect.right;				// get width/height of display
		h = (**gGDevice).gdRect.bottom;

		r.left = r.top = 0;
		r.right = w / 2;
		r.bottom = h / 2;

		if (r.right < 640)			// keep minimum at 640 wide
		{
			r.right = 640;
			r.bottom = 480;
		}

		gGameWindow = NewCWindow(nil, &r, "", false, plainDBox, (WindowPtr)-1L, false, 0);


		gGameWindowGrafPtr = GetWindowPort(gGameWindow);

				/* MOVE WINDOW TO CENTER OF SCREEN */

		MoveWindow(gGameWindow, r.right/2, r.bottom/2, true);
		ShowWindow(gGameWindow);
	}



	gGameWindowWidth = r.right - r.left;
	gGameWindowHeight = r.bottom - r.top;

#endif
}


#pragma mark -


/**************** GAMMA FADE OUT *************************/
static void CheckGLError(const char* file, const int line)
{
	static char buf[256];
	GLenum error = glGetError();
	if (error != 0)
	{
		snprintf(buf, 256, "OpenGL Error %d in %s:%d", error, file, line);
		DoFatalAlert(buf);
	}
}

#define CHECK_GL_ERROR() CheckGLError(__func__, __LINE__)


void GammaFadeOut(void)
{
#if DO_GAMMA

	SDL_GLContext currentContext = SDL_GL_GetCurrentContext();
	if (!currentContext)
	{
		printf("%s: no GL context; skipping fade out\n", __func__);
		return;
	}

	SDL_GL_MakeCurrent(gSDLWindow, gAGLContext);

	int windowWidth, windowHeight;
	SDL_GetWindowSize(gSDLWindow, &windowWidth, &windowHeight);
	int width4rem = windowWidth % 4;
	int width4ceil = windowWidth - width4rem + (width4rem == 0? 0: 4);

	GLint textureWidth = width4ceil;
	GLint textureHeight = windowHeight;
	char* textureData = AllocPtr(textureWidth * textureHeight * 3);

	//SDL_GL_SwapWindow(gSDLWindow);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, textureWidth);
	glReadPixels(0, 0, textureWidth, textureHeight, GL_BGR, GL_UNSIGNED_BYTE, textureData);
	CHECK_GL_ERROR();

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, textureWidth, textureHeight, 0, GL_BGR, GL_UNSIGNED_BYTE, textureData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

#if 0
	FILE* TGAfile = fopen("/tmp/FadeCapture.tga", "wb");
	uint8_t TGAhead[] = {0,0, 2, 0,0,0,0,0, 0,0,0,0, textureWidth&0xFF, (textureWidth>>8)&0xFF, textureHeight&0xFF, (textureHeight>>8)&0xFF, 24, 0<<5};
	fwrite(TGAhead, 1, sizeof(TGAhead), TGAfile);
	fwrite(textureData, textureHeight, 3*textureWidth, TGAfile);
	fclose(TGAfile);
#endif

	OGL_PushState();

	SetInfobarSpriteState();

	const float fadeDuration = .25f;
	Uint32 startTicks = SDL_GetTicks();
	Uint32 endTicks = startTicks + fadeDuration * 1000.0f;

	for (Uint32 ticks = startTicks; ticks <= endTicks; ticks = SDL_GetTicks())
	{
		gGammaFadePercent = 1.0f - ((ticks - startTicks) / 1000.0f / fadeDuration);
		if (gGammaFadePercent < 0.0f)
			gGammaFadePercent = 0.0f;

		SDL_GL_MakeCurrent(gSDLWindow, gAGLContext);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture);

		glColor4f(gGammaFadePercent, gGammaFadePercent, gGammaFadePercent, 1.0f);

		glBegin(GL_QUADS);
		glTexCoord2f(0,1); glVertex3f(   0,   0,0);
		glTexCoord2f(1,1); glVertex3f( 640,   0,0);
		glTexCoord2f(1,0); glVertex3f( 640, 480,0);
		glTexCoord2f(0,0); glVertex3f(   0, 480,0);
		glEnd();
		SDL_GL_SwapWindow(gSDLWindow);
		CHECK_GL_ERROR();

		SDL_Delay(15);
	}

	SafeDisposePtr(textureData);
	glDeleteTextures(1, &texture);

	OGL_PopState();

	gGammaFadePercent = 0;
#endif
}

/********************** GAMMA ON *********************/

void GammaOn(void)
{
#if DO_GAMMA

	if (!gPlayFullScreen)
		return;

	if (gGammaFadePercent != 1.0f)
	{
		gGammaFadePercent = 1.0f;

		SOURCE_PORT_MINOR_PLACEHOLDER(); // CGSetDisplayTransferByTable(0, 256, gOriginalRedTable, gOriginalGreenTable, gOriginalBlueTable);
	}
#endif
}



/********************** GAMMA OFF *********************/

void GammaOff(void)
{
#if DO_GAMMA

	if (!gPlayFullScreen)
		return;

	if (gGammaFadePercent != 0.0f)
	{
		int			i;

		gGammaFadePercent = 0.0f;

#if 1
		SOURCE_PORT_MINOR_PLACEHOLDER();
#else
	    for (i = 0; i < 256 ; i++)
	    {
	        gGammaRedTable[i] 	= gOriginalRedTable[i] * gGammaFadePercent * gGammaTweak;
	        gGammaGreenTable[i] = gOriginalGreenTable[i] * gGammaFadePercent * gGammaTweak;
	        gGammaBlueTable[i] 	= gOriginalBlueTable[i] * gGammaFadePercent * gGammaTweak;
	    }

		CGSetDisplayTransferByTable( 0, 256, gGammaRedTable, gGammaGreenTable, gGammaBlueTable);
#endif
	}
#endif
}




/****************** CLEANUP DISPLAY *************************/

void CleanupDisplay(void)
{
}


/******************** MAKE FADE EVENT *********************/
//
// INPUT:	fadeIn = true if want fade IN, otherwise fade OUT.
//

void MakeFadeEvent(Boolean fadeIn, float fadeSpeed)
{
ObjNode	*newObj;
ObjNode		*thisNodePtr;

		/* SCAN FOR OLD FADE EVENTS STILL IN LIST */

	thisNodePtr = gFirstNodePtr;

	while (thisNodePtr)
	{
		if (thisNodePtr->MoveCall == MoveFadeEvent)
		{
			thisNodePtr->Flag[0] = fadeIn;								// set new mode
			return;
		}
		thisNodePtr = thisNodePtr->NextNode;							// next node
	}


		/* MAKE NEW FADE EVENT */

	gNewObjectDefinition.genre = EVENT_GENRE;
	gNewObjectDefinition.flags = 0;
	gNewObjectDefinition.slot = SLOT_OF_DUMB + 1000;
	gNewObjectDefinition.moveCall = MoveFadeEvent;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->Flag[0] = fadeIn;

	newObj->Speed = fadeSpeed;
}


/***************** MOVE FADE EVENT ********************/

static void MoveFadeEvent(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	speed = theNode->Speed * fps;

			/* SEE IF FADE IN */

	if (theNode->Flag[0])
	{
		gGammaFadePercent += speed;
		if (gGammaFadePercent >= 1.0f)										// see if @ 100%
		{
			gGammaFadePercent = 1.0f;
			DeleteObject(theNode);
		}
	}

			/* FADE OUT */
	else
	{
		gGammaFadePercent -= speed;
		if (gGammaFadePercent <= 0.0f)													// see if @ 0%
		{
			gGammaFadePercent = 0;
			DeleteObject(theNode);
		}
	}


	if (gPlayFullScreen)
	{
#if 1
		SOURCE_PORT_MINOR_PLACEHOLDER();
#else
		int			i;

	    for (i = 0; i < 256 ; i++)
	    {
	        gGammaRedTable[i] 	= gOriginalRedTable[i] * gGammaFadePercent * gGammaTweak;
	        gGammaGreenTable[i] = gOriginalGreenTable[i] * gGammaFadePercent * gGammaTweak;
	        gGammaBlueTable[i] 	= gOriginalBlueTable[i] * gGammaFadePercent * gGammaTweak;
	    }

		CGSetDisplayTransferByTable( 0, 256, gGammaRedTable, gGammaGreenTable, gGammaBlueTable);
#endif
	}

}



/************************ GAME SCREEN TO BLACK ************************/

void GameScreenToBlack(void)
{
	SOURCE_PORT_MINOR_PLACEHOLDER();
}


/************************** ENTER 2D *************************/
//
// For OS X - turn off DSp when showing 2D
//

void Enter2D(void)
{
	InitCursor();
	MyFlushEvents();

	g2DStackDepth++;
	if (g2DStackDepth > 1)						// see if already in 2D
	{
		GammaOn();
		return;
	}

	if (gPlayFullScreen)
	{
		GammaOff();

#if 1
		SOURCE_PORT_MINOR_PLACEHOLDER();
#else
		if (gAGLContext)
		{
			glFlush();
			glFinish();

			aglSetDrawable(gAGLContext, nil);		// diable GL so our dialogs will show up
			glFlush();
			glFinish();
		}

			/* NEED TO UN-CAPTURE THE CG DISPLAY */

		CGDisplayRelease(gCGDisplayID);
#endif
	}

	GammaOn();

}


/************************** EXIT 2D *************************/
//
// For OS X - turn ON DSp when NOT 2D
//

void Exit2D(void)
{
	g2DStackDepth--;
	if (g2DStackDepth > 0)			// don't exit unless on final exit
		return;

	HideCursor();

#if 1
	SOURCE_PORT_MINOR_PLACEHOLDER();
#else
	if (gPlayFullScreen)
	{
//		if (gAGLContext)
		{
			CGDisplayCapture(gCGDisplayID);
			if (gAGLContext)
				aglSetFullScreen(gAGLContext, 0, 0, 0, 0);		//re-enable GL
		}
	}
#endif

}


#pragma mark -

#if 0		// srcport rm -- unused in Otto Matic
/*********************** DUMP GWORLD 2 **********************/
//
//    copies to a destination RECT
//

void DumpGWorld2(GWorldPtr thisWorld, WindowPtr thisWindow,Rect *destRect)
{
PixMapHandle pm;
GDHandle		oldGD;
GWorldPtr		oldGW;
Rect			r;

	DoLockPixels(thisWorld);

	GetGWorld (&oldGW,&oldGD);
	pm = GetGWorldPixMap(thisWorld);
	if ((pm == nil) | (*pm == nil) )
		DoAlert("PixMap Handle or Ptr = Null?!");

	SetPort(GetWindowPort(thisWindow));

	ForeColor(blackColor);
	BackColor(whiteColor);

	GetPortBounds(thisWorld, &r);

	CopyBits((BitMap *)*pm, GetPortBitMapForCopyBits(GetWindowPort(thisWindow)),
			 &r,
			 destRect,
			 srcCopy, 0);

	SetGWorld(oldGW,oldGD);								// restore gworld
}
#endif


/******************* DO LOCK PIXELS **************/

void DoLockPixels(GWorldPtr world)
{
PixMapHandle pm;

	pm = GetGWorldPixMap(world);
	if (LockPixels(pm) == false)
		DoFatalAlert("PixMap Went Bye,Bye?!");
}


#pragma mark -

/********************* DO SCREEN MODE DIALOG *************************/

void DoScreenModeDialog(void)
{
#if 1
	SOURCE_PORT_PLACEHOLDER();
#else
OSErr			err;
EventHandlerRef	ref;
EventTypeSpec	list[] = { { kEventClassCommand,  kEventProcessCommand } };
ControlID 		idControl;
ControlRef 		control;
int				i;
const char		*rezNames[MAX_LANGUAGES] =
{
	"VideoMode_English",
	"VideoMode_French",
	"VideoMode_German",
	"VideoMode_Spanish",
	"VideoMode_Italian",
	"VideoMode_Swedish",
};

	if (gGamePrefs.language > LANGUAGE_SWEDISH)					// verify that this is legit
		gGamePrefs.language = LANGUAGE_ENGLISH;


				/* GET LIST OF VIDEO MODES FOR USER TO CHOOSE FROM */

	CreateDisplayModeList();


				/**********************************************/
				/* DETERMINE IF NEED TO DO THIS DIALOG OR NOT */
				/**********************************************/

	UpdateInput();
	UpdateInput();
	if (GetKeyState(KEY_OPTION) || gGamePrefs.showScreenModeDialog)			// see if force it
		goto do_it;

					/* VERIFY THAT CURRENT MODE IS ALLOWED */

	for (i=0; i < gNumVideoModes; i++)
	{
		if ((gVideoModeList[i].rezH == gGamePrefs.screenWidth) &&					// if its a match then bail
			(gVideoModeList[i].rezV == gGamePrefs.screenHeight) &&
			(gVideoModeList[i].hz == gGamePrefs.hz))
			{
				CalcVRAMAfterBuffers();
				return;
			}
	}

			/* BAD MODE, SO RESET TO SOMETHING LEGAL AS DEFAULT */

	gGamePrefs.screenWidth = gVideoModeList[0].rezH;
	gGamePrefs.screenHeight = gVideoModeList[0].rezV;
	gGamePrefs.hz = gVideoModeList[0].hz;



    		/***************/
    		/* INIT DIALOG */
    		/***************/
do_it:

	InitCursor();

				/* CREATE WINDOW FROM THE NIB */

    err = CreateWindowFromNib(gNibs, CFStringCreateWithCString(nil, rezNames[gGamePrefs.language],
    						kCFStringEncodingMacRoman), &gDialogWindow);
	if (err)
		DoFatalAlert("DoScreenModeDialog: CreateWindowFromNib failed!");


			/* CREATE NEW WINDOW EVENT HANDLER */

    gWinEvtHandler = NewEventHandlerUPP(DoScreenModeDialog_EventHandler);
    InstallWindowEventHandler(gDialogWindow, gWinEvtHandler, GetEventTypeCount(list), list, 0, &ref);


			/* SET "DON'T SHOW" CHECKBOX */

    idControl.signature = 'nosh';
    idControl.id 		= 0;
    GetControlByID(gDialogWindow, &idControl, &control);
	SetControlValue(control, !gGamePrefs.showScreenModeDialog);


			/* SET "16/32" BUTTONS */

    idControl.signature = 'bitd';
    idControl.id 		= 0;
    GetControlByID(gDialogWindow, &idControl, &control);
	SetControlValue(control, (gGamePrefs.depth == 32) + 1);

//	if (gDisplayVRAM <= 0x2000000)				// if <= 32MB VRAM...
//	{
//		DisableControl(control);
//	}



			/* SET STEREO BUTTONS*/

    idControl.signature = '3dmo';
    idControl.id 		= 0;
    GetControlByID(gDialogWindow, &idControl, &control);
	SetControlValue(control, gGamePrefs.anaglyph + 1);


			/* SET ANAGLYPH COLOR/BW BUTTONS */

    idControl.signature = '3dco';
    idControl.id 		= 0;
    GetControlByID(gDialogWindow, &idControl, &control);
	SetControlValue(control, gGamePrefs.anaglyphColor + 1);



	BuildResolutionMenu();


			/**********************/
			/* PROCESS THE DIALOG */
			/**********************/

    ShowWindow(gDialogWindow);
	RunAppModalLoopForWindow(gDialogWindow);


			/*********************/
			/* GET RESULT VALUES */
			/*********************/

			/* GET "DON'T SHOW" CHECKBOX */

    idControl.signature = 'nosh';
    idControl.id 		= 0;
    GetControlByID(gDialogWindow, &idControl, &control);
	gGamePrefs.showScreenModeDialog = !GetControlValue(control);


			/* GET "16/32" BUTTONS */

    idControl.signature = 'bitd';
    idControl.id 		= 0;
    GetControlByID(gDialogWindow, &idControl, &control);
	if (GetControlValue(control) == 1)
		gGamePrefs.depth = 16;
	else
		gGamePrefs.depth = 32;


			/* GET RESOLUTION MENU */

    idControl.signature = 'rezm';
    idControl.id 		= 0;
    GetControlByID(gDialogWindow, &idControl, &control);
	i = GetControlValue(control);
	gGamePrefs.screenWidth = gVideoModeList[i-1].rezH;
	gGamePrefs.screenHeight = gVideoModeList[i-1].rezV;
	gGamePrefs.hz = gVideoModeList[i-1].hz;


			/* GET STEREO BUTTONS*/

    idControl.signature = '3dmo';
    idControl.id 		= 0;
    GetControlByID(gDialogWindow, &idControl, &control);
	gGamePrefs.anaglyph = GetControlValue(control) - 1;


			/* GET ANAGLYPH COLOR/BW BUTTONS */

    idControl.signature = '3dco';
    idControl.id 		= 0;
    GetControlByID(gDialogWindow, &idControl, &control);
	if (GetControlValue(control) == 2)
		gGamePrefs.anaglyphColor = true;
	else
		gGamePrefs.anaglyphColor = false;


				/***********/
				/* CLEANUP */
				/***********/

	DisposeEventHandlerUPP (gWinEvtHandler);
	DisposeWindow (gDialogWindow);



	CalcVRAMAfterBuffers();
	SavePrefs();
#endif
}


/****************** DO SCREEN MODE DIALOG EVENT HANDLER *************************/
//
// main window event handling
//

#if 0	// srcport rm
static pascal OSStatus DoScreenModeDialog_EventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData)
{
#pragma unused (myHandler, userData)
OSStatus		result = eventNotHandledErr;
HICommand 		command;

	switch(GetEventKind(event))
	{

				/*******************/
				/* PROCESS COMMAND */
				/*******************/

		case	kEventProcessCommand:
				GetEventParameter (event, kEventParamDirectObject, kEventParamHICommand, NULL, sizeof(command), NULL, &command);
				switch(command.commandID)
				{
							/* "OK" BUTTON */

					case	'ok  ':
		                    QuitAppModalLoopForWindow(gDialogWindow);
							break;

							/* "QUIT" BUTTON */

					case	'quit':
		                    ExitToShell();
							break;


				}
				break;
    }

    return (result);
}



/****************** BUILD RESOLUTION MENU *************************/

static void BuildResolutionMenu(void)
{
Str255 		menuStrings[MAX_VIDEO_MODES];
short		i, defaultItem = 1;
Str32		s,t;

	for (i=0; i < gNumVideoModes; i++)
	{
				/* BUILD MENU ITEM STRING */

		NumToString(gVideoModeList[i].rezH, s);			// horiz rez
		s[s[0]+1] = 'x';								// "x"
		s[0]++;

		NumToString(gVideoModeList[i].rezV, t);			// vert rez
		BlockMove(&t[1], &s[s[0]+1], t[0]);
		s[0] += t[0];

		s[s[0]+1] = ' ';								// " "
		s[0]++;

		NumToString(gVideoModeList[i].hz, t);			// refresh hz
		BlockMove(&t[1], &s[s[0]+1], t[0]);
		s[0] += t[0];

		s[s[0]+1] = 'h';								// "hz"
		s[s[0]+2] = 'z';
		s[0] += 2;

		BlockMove(&s[0], &menuStrings[i][0], s[0]+1);

			/* IS THIS THE CURRENT SETTING? */

		if ((gVideoModeList[i].rezH == gGamePrefs.screenWidth) &&
			(gVideoModeList[i].rezV == gGamePrefs.screenHeight) &&
			(gVideoModeList[i].hz == gGamePrefs.hz))
		{
			defaultItem = i;
		}
	}


			/* BUILD THE MENU FROM THE STRING LIST */

	BuildControlMenu (gDialogWindow, 'rezm', 0, &menuStrings[0], gNumVideoModes, defaultItem);
}


/********************** BUILD CONTROL MENU ***************************/
//
// builds menu of id, based on text list of numitems number of strings
//

void BuildControlMenu(WindowRef window, SInt32 controlSig, SInt32 id, Str255 textList[], short numItems, short defaultSelection)
{
MenuHandle 	hMenu;
Size 		tempSize;
ControlID 	idControl;
ControlRef 	control;
short 		i;
OSStatus 	err = noErr;

    		/* GET MENU DATA */

    idControl.signature = controlSig;
    idControl.id 		= id;
    GetControlByID(window, &idControl, &control);
    GetControlData(control, kControlMenuPart, kControlPopupButtonMenuHandleTag, sizeof (MenuHandle), &hMenu, &tempSize);

    	/* REMOVE ALL ITEMS FROM EXISTING MENU */

    err = DeleteMenuItems(hMenu, 1, CountMenuItems (hMenu));


			/* ADD NEW ITEMS TO THE MENU */

    for (i = 0; i < numItems; i++)
        AppendMenu(hMenu, textList[i]);


			/* FORCE UPDATE OF MENU EXTENTS */

    SetControlMaximum(control, numItems);

	if (defaultSelection >= numItems)					// make sure default isn't over
		defaultSelection = 0;

	SetControlValue(control, defaultSelection+1);

}



#pragma mark -

/********************* CREATE DISPLAY MODE LIST *************************/

static void CreateDisplayModeList(void)
{
int					i, j, numDeviceModes;
CFArrayRef 			modeList;
CFDictionaryRef 	mode;
long				width, height, dep;
double				hz;

			/* MAKE SURE IT SHOWS 640X480 */

	gVideoModeList[0].rezH = 640;
	gVideoModeList[0].rezV = 480;
	gVideoModeList[0].hz = 60;
	gNumVideoModes = 1;

				/* GET MAIN MONITOR ID & VRAM FOR IT */

	gCGDisplayID = CGMainDisplayID();
	GetDisplayVRAM();										// calc the amount of vram on this device

				/* GET LIST OF MODES FOR THIS MONITOR */

	modeList = CGDisplayAvailableModes(gCGDisplayID);
	if (modeList == nil)
		DoFatalAlert("CreateDisplayModeList: CGDisplayAvailableModes failed!");

	numDeviceModes = CFArrayGetCount(modeList);


				/*********************/
				/* EXTRACT MODE INFO */
				/*********************/

	for (i = 0; i < numDeviceModes; i++)
    {
    	CFNumberRef	number;
    	Boolean		skip;

        mode = CFArrayGetValueAtIndex( modeList, i );				  	// Pull the mode dictionary out of the CFArray
		if (mode == nil)
			DoFatalAlert("CreateDisplayModeList: CFArrayGetValueAtIndex failed!");

		number = CFDictionaryGetValue(mode, kCGDisplayWidth);			// get width
		CFNumberGetValue(number, kCFNumberLongType, &width) ;

		number = CFDictionaryGetValue(mode, kCGDisplayHeight);	 		// get height
		CFNumberGetValue(number, kCFNumberLongType, &height);

		number = CFDictionaryGetValue(mode, kCGDisplayBitsPerPixel);	// get depth
		CFNumberGetValue(number, kCFNumberLongType, &dep);
		if (dep != 32)
			continue;

		number = CFDictionaryGetValue(mode, kCGDisplayRefreshRate);		// get refresh rate
		if (number == nil)
			hz = 85;
		else
		{
			CFNumberGetValue(number, kCFNumberDoubleType, &hz);
			if (hz == 0)												// LCD's tend to return 0 since there's no real refresh rate
				hz = 60;
		}

				/* IN LOW-VRAM SITUATIONS, DON'T ALLOW LARGE MODES */

#if 0
		if (gDisplayVRAM <= 0x4000000)				// if <= 64MB VRAM...
		{
			if (width > 1280)
				continue;
		}

		if (gDisplayVRAM <= 0x2000000)				// if <= 32MB VRAM...
		{
			if (width > 1024)
				continue;
		}
#endif

					/***************************/
					/* SEE IF ADD TO MODE LIST */
					/***************************/

					/* SEE IF IT'S A VALID MODE FOR US */

		if (CFDictionaryGetValue(mode, kCGDisplayModeUsableForDesktopGUI) != kCFBooleanTrue)	// is it a displayable rez?
			continue;


						/* SEE IF ALREADY IN LIST */

		skip = false;
		for (j = 0; j < gNumVideoModes; j++)
		{
			if ((gVideoModeList[j].rezH == width) &&
				(gVideoModeList[j].rezV == height) &&
				(gVideoModeList[j].hz == hz))
			{
				skip = true;
				break;
			}
		}

		if (!skip)
		{
				/* THIS REZ NOT IN LIST YET, SO ADD */

			if (gNumVideoModes < MAX_VIDEO_MODES)
			{
				gVideoModeList[gNumVideoModes].rezH = width;
				gVideoModeList[gNumVideoModes].rezV = height;
				gVideoModeList[gNumVideoModes].hz = hz;
				gNumVideoModes++;
			}
		}
	}
}

/******************** GET DISPLAY VRAM ***********************/

static void GetDisplayVRAM(void)
{
io_service_t	port;
CFTypeRef		classCode;

			/* SEE HOW MUCH VRAM WE HAVE */

	port = CGDisplayIOServicePort(gCGDisplayID);
	classCode = IORegistryEntryCreateCFProperty(port, CFSTR(kIOFBMemorySizeKey), kCFAllocatorDefault, kNilOptions);

	if (CFGetTypeID(classCode) == CFNumberGetTypeID())
	{
		CFNumberGetValue(classCode, kCFNumberSInt32Type, &gDisplayVRAM);
	}
	else
		gDisplayVRAM = 0x8000000;				// if failed, then just assume 128MB VRAM to be safe


			/* SET SOME PREFS IF THINGS ARE LOOKING LOW */

	if (gDisplayVRAM <= 0x1000000)				// if <= 16MB VRAM...
	{
		gGamePrefs.depth = 16;
	}

//	if (gDisplayVRAM < 0x1000000)				// if < 16MB VRAM...
//	{
//		DoFatalAlert("This game requires at least 32MB of VRAM, and you appear to have much less than that.  You will need to install a newer video card with more VRAM to play this game.");
//	}


	SavePrefs();
}


/*********************** CALC VRAM AFTER BUFFERS ***********************/
//
// CALC HOW MUCH VRAM REMAINS AFTER OUR BUFFERS
//

static void CalcVRAMAfterBuffers(void)
{
int	bufferSpace;

	bufferSpace = gGamePrefs.screenWidth * gGamePrefs.screenHeight * 2 * 2;		// calc main pixel/z buffers @ 16-bit
	if (gGamePrefs.depth == 32)
		bufferSpace *= 2;

	gVRAMAfterBuffers = gDisplayVRAM - bufferSpace;
}



#endif
