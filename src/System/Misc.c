/****************************/
/*      MISC ROUTINES       */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/


extern	short		gMainAppRezFile;
extern	Boolean		gOSX,gG4;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	int			gPolysThisFrame;
extern	SDL_GLContext		gAGLContext;
extern	AGLDrawable		gAGLWin;
extern	FSSpec				gDataSpec;


/****************************/
/*    CONSTANTS             */
/****************************/

#define		ERROR_ALERT_ID		401

#define	DEFAULT_FPS			10

#define	USE_MALLOC		1

/**********************/
/*     VARIABLES      */
/**********************/

short	gPrefsFolderVRefNum;
long	gPrefsFolderDirID;

u_long 	seed0 = 0, seed1 = 0, seed2 = 0;

float	gFramesPerSecond, gFramesPerSecondFrac;

int		gNumPointers = 0;

unsigned char	gRegInfo[64];

Boolean	gGameIsRegistered = false;
Boolean	gSerialWasVerified = false;

Boolean	gLowMemMode = false;

Str255  gSerialFileName = ":OttoMatic:Info";

Boolean	gLittleSnitch = false;


/**********************/
/*     PROTOTYPES     */
/**********************/


/****************** DO SYSTEM ERROR ***************/

void ShowSystemErr(long err)
{
Str255		numStr;

	Enter2D();

	if (gAGLContext)
		aglSetDrawable(gAGLContext, nil);			// diable gl for dialog

	GammaOn();
	MyFlushEvents();
	UseResFile(gMainAppRezFile);
	NumToString(err, numStr);
	DoAlert (numStr);


	Exit2D();

	CleanQuit();
}

/****************** DO SYSTEM ERROR : NONFATAL ***************/
//
// nonfatal
//
void ShowSystemErr_NonFatal(long err)
{
Str255		numStr;

	Enter2D();

	if (gAGLContext)
		aglSetDrawable(gAGLContext, nil);			// diable gl for dialog

		GammaOn();
	MyFlushEvents();
	NumToString(err, numStr);
	DoAlert (numStr);

	Exit2D();

}


/*********************** DO ALERT *******************/

void DoAlert(Str255 s)
{
	GammaOn();

	Enter2D();

	if (gAGLContext)
		aglSetDrawable(gAGLContext, nil);			// diable gl for dialog

	MyFlushEvents();
	UseResFile(gMainAppRezFile);
	MyFlushEvents();
	ParamText(s,NIL_STRING,NIL_STRING,NIL_STRING);
	NoteAlert(ERROR_ALERT_ID,nil);

	Exit2D();

	HideCursor();
}


/*********************** DO ALERT NUM *******************/

void DoAlertNum(int n)
{
	GammaOn();

	Enter2D();

	NoteAlert(n,nil);

	Exit2D();

}



/*********************** DO FATAL ALERT *******************/

void DoFatalAlert(Str255 s)
{
OSErr	iErr;

	GammaOn();

	Enter2D();

	UseResFile(gMainAppRezFile);

	ParamText(s,NIL_STRING,NIL_STRING,NIL_STRING);
	iErr = NoteAlert(ERROR_ALERT_ID,nil);


	Exit2D();
	CleanQuit();
}



/************ CLEAN QUIT ***************/

void CleanQuit(void)
{
static Boolean	beenHere = false;

	if (!beenHere)
	{
		beenHere = true;

#if DEMO
		DeleteAllObjects();
#endif

		ShutdownSound();								// cleanup sound stuff

		DisposeTerrain();								// dispose of any memory allocated by terrain manager
		DisposeAllBG3DContainers();						// nuke all models
		DisposeAllSpriteGroups();						// nuke all sprites

		if (gGameViewInfoPtr)							// see if need to dispose this
			OGL_DisposeWindowSetup(&gGameViewInfoPtr);

#if DEMO
		GammaFadeOut();
		ShowDemoQuitScreen();
#endif

	}

	GameScreenToBlack();
	CleanupDisplay();								// unloads Draw Sprocket


	UseResFile(gMainAppRezFile);

	InitCursor();
	MyFlushEvents();

	SavePrefs();							// save prefs before bailing

	ExitToShell();
}



#pragma mark -


/******************** MY RANDOM LONG **********************/
//
// My own random number generator that returns a LONG
//
// NOTE: call this instead of MyRandomShort if the value is going to be
//		masked or if it just doesnt matter since this version is quicker
//		without the 0xffff at the end.
//

unsigned long MyRandomLong(void)
{
  return seed2 ^= (((seed1 ^= (seed2>>5)*1568397607UL)>>7)+
                   (seed0 = (seed0+1)*3141592621UL))*2435386481UL;
}


/************************* RANDOM RANGE *************************/
//
// THE RANGE *IS* INCLUSIVE OF MIN AND MAX
//

u_short	RandomRange(unsigned short min, unsigned short max)
{
u_short		qdRdm;											// treat return value as 0-65536
u_long		range, t;

	qdRdm = MyRandomLong();
	range = max+1 - min;
	t = (qdRdm * range)>>16;	 							// now 0 <= t <= range

	return( t+min );
}



/************** RANDOM FLOAT ********************/
//
// returns a random float between 0 and 1
//

float RandomFloat(void)
{
unsigned long	r;
float	f;

	r = MyRandomLong() & 0xfff;
	if (r == 0)
		return(0);

	f = (float)r;							// convert to float
	f = f * (1.0f/(float)0xfff);			// get # between 0..1
	return(f);
}


/************** RANDOM FLOAT 2 ********************/
//
// returns a random float between -1 and +1
//

float RandomFloat2(void)
{
unsigned long	r;
float	f;

	r = MyRandomLong() & 0xfff;
	if (r == 0)
		return(0);

	f = (float)r;							// convert to float
	f = f * (2.0f/(float)0xfff);			// get # between 0..2
	f -= 1.0f;								// get -1..+1
	return(f);
}



/**************** SET MY RANDOM SEED *******************/

void SetMyRandomSeed(unsigned long seed)
{
	seed0 = seed;
	seed1 = 0;
	seed2 = 0;

}

/**************** INIT MY RANDOM SEED *******************/

void InitMyRandomSeed(void)
{
	seed0 = 0x2a80ce30;
	seed1 = 0;
	seed2 = 0;
}


#pragma mark -


/******************* FLOAT TO STRING *******************/

void FloatToString(float num, Str255 string)
{
Str255	sf;
long	i,f;

	i = num;						// get integer part


	f = (fabs(num)-fabs((float)i)) * 10000;		// reduce num to fraction only & move decimal --> 5 places

	if ((i==0) && (num < 0))		// special case if (-), but integer is 0
	{
		string[0] = 2;
		string[1] = '-';
		string[2] = '0';
	}
	else
		NumToString(i,string);		// make integer into string

	NumToString(f,sf);				// make fraction into string

	string[++string[0]] = '.';		// add "." into string

	if (f >= 1)
	{
		if (f < 1000)
			string[++string[0]] = '0';	// add 1000's zero
		if (f < 100)
			string[++string[0]] = '0';	// add 100's zero
		if (f < 10)
			string[++string[0]] = '0';	// add 10's zero
	}

	for (i = 0; i < sf[0]; i++)
	{
		string[++string[0]] = sf[i+1];	// copy fraction into string
	}
}

/*********************** STRING TO FLOAT *************************/

float StringToFloat(Str255 textStr)
{
short	i;
short	length;
Byte	mode = 0;
long	integer = 0;
long	mantissa = 0,mantissaSize = 0;
float	f;
float	tens[8] = {1,10,100,1000,10000,100000,1000000,10000000};
char	c;
float	sign = 1;												// assume positive

	length = textStr[0];										// get string length

	if (length== 0)												// quick check for empty
		return(0);


			/* SCAN THE NUMBER */

	for (i = 1; i <= length; i++)
	{
		c  = textStr[i];										// get this char

		if (c == '-')											// see if negative
		{
			sign = -1;
			continue;
		}
		else
		if (c == '.')											// see if hit the decimal
		{
			mode = 1;
			continue;
		}
		else
		if ((c < '0') || (c > '9'))								// skip all but #'s
			continue;


		if (mode == 0)
			integer = (integer * 10) + (c - '0');
		else
		{
			mantissa = (mantissa * 10) + (c - '0');
			mantissaSize++;
		}
	}

			/* BUILT A FLOAT FROM IT */

	f = (float)integer + ((float)mantissa/tens[mantissaSize]);
	f *= sign;

	return(f);
}





#pragma mark -

/****************** ALLOC HANDLE ********************/

Handle	AllocHandle(long size)
{
Handle	hand;
OSErr	err;

	hand = NewHandle(size);							// alloc in APPL
	if (hand == nil)
	{
		DoAlert("AllocHandle: using temp mem");
		hand = TempNewHandle(size,&err);			// try TEMP mem
		if (hand == nil)
		{
			DoAlert("AllocHandle: failed!");
			return(nil);
		}
		else
			return(hand);
	}
	return(hand);

}



/****************** ALLOC PTR ********************/

void *AllocPtr(long size)
{
Ptr	pr;
u_long	*cookiePtr;

	size += 16;								// make room for our cookie & whatever else (also keep to 16-byte alignment!)

#if USE_MALLOC
	pr = malloc(size);
#else
	pr = NewPtr(size);
#endif
	if (pr == nil)
		DoFatalAlert("AllocPtr: NewPtr failed");

	cookiePtr = (u_long *)pr;

	*cookiePtr++ = 'FACE';
	*cookiePtr++ = 'PTR2';
	*cookiePtr++ = 'PTR3';
	*cookiePtr = 'PTR4';

	pr += 16;

	gNumPointers++;

	return(pr);
}


/****************** ALLOC PTR CLEAR ********************/

void *AllocPtrClear(long size)
{
Ptr	pr;
u_long	*cookiePtr;

	size += 16;								// make room for our cookie & whatever else (also keep to 16-byte alignment!)

#if USE_MALLOC
	pr = calloc(1, size);
#else
	pr = NewPtrClear(size);						// alloc in Application
#endif

	if (pr == nil)
		DoFatalAlert("AllocPtr: NewPtr failed");

	cookiePtr = (u_long *)pr;

	*cookiePtr++ = 'FACE';
	*cookiePtr++ = 'PTC2';
	*cookiePtr++ = 'PTC3';
	*cookiePtr = 'PTC4';

	pr += 16;

	gNumPointers++;

	return(pr);
}


/***************** SAFE DISPOSE PTR ***********************/

void SafeDisposePtr(Ptr ptr)
{
u_long	*cookiePtr;

	ptr -= 16;					// back up to pt to cookie

	cookiePtr = (u_long *)ptr;

	if (*cookiePtr != 'FACE')
		DoFatalAlert("SafeSafeDisposePtr: invalid cookie!");

	*cookiePtr = 0;

#if USE_MALLOC
	free(ptr);
#else
	DisposePtr(ptr);
#endif

	gNumPointers--;
}



#pragma mark -

/******************* COPY P STRING ********************/

void CopyPString(Str255 from, Str255 to)
{
short	i,n;

	n = from[0];			// get length

	for (i = 0; i <= n; i++)
		to[i] = from[i];

}


/***************** P STRING TO C ************************/

void PStringToC(char *pString, char *cString)
{
Byte	pLength,i;

	pLength = pString[0];

	for (i=0; i < pLength; i++)					// copy string
		cString[i] = pString[i+1];

	cString[pLength] = 0x00;					// add null character to end of c string
}


/***************** DRAW C STRING ********************/

void DrawCString(char *string)
{
	while(*string != 0x00)
		DrawChar(*string++);
}


/******************* VERIFY SYSTEM ******************/

void VerifySystem(void)
{
OSErr	iErr;
long		createdDirID, cpuSpeed;
NumVersion	vers;

	SetDefaultDirectory();							// be sure to get the default directory


			/* VERIFY & MAKE FSSPEC FOR DATA FOLDER */

#if DEMO
	iErr = FSMakeFSSpec(0, 0, ":DemoData:Images", &gDataSpec);
#else
	iErr = FSMakeFSSpec(0, 0, ":Data:Images", &gDataSpec);
#endif
	if (iErr)
	{
		DoAlertNum(133);
		ExitToShell();
	}

	gG4 = true;

	if (!gG4)																// if not G4, check processor speed to see if on really fast G3
	{
		iErr = Gestalt(gestaltProcClkSpeed,&cpuSpeed);
		if (iErr != noErr)
			DoFatalAlert("VerifySystem: gestaltProcClkSpeed failed!");

		if ((cpuSpeed/1000000) >= 600)										// must be at least 600mhz G3 for us to treat it like a G4
			gG4 = true;
	}


		/* DETERMINE IF RUNNING ON OS X */

	iErr = Gestalt(gestaltSystemVersion,(long *)&vers);
	if (iErr != noErr)
		DoFatalAlert("VerifySystem: gestaltSystemVersion failed!");

	if (vers.stage >= 0x10)													// see if at least OS 10
	{
		gOSX = true;
		if ((vers.stage == 0x10) && (vers.nonRelRev < 0x10))				// must be at least OS 10.1 !!!
			DoFatalAlert("This game requires OS 10.1 or later to run on OS X.  Either upgrade to 10.1 or run the game on OS 9.");
	}
	else
	{
		gOSX = false;
		if (vers.stage == 8)						// check for 8.6 also
			if (vers.nonRelRev < 0x60)
				DoFatalAlert("This game requires at least OS 8.6 with all the updates.");
	}

		/* REQUIRE CARBONLIB 1.2 */

	iErr = Gestalt(gestaltCarbonVersion,(long *)&vers);
	if (iErr)
	{
		ShowSystemErr_NonFatal(iErr);
		goto carbonerr;
	}
	if (vers.stage == 1)
	{
		if (vers.nonRelRev < 0x20)
		{
carbonerr:
			DoFatalAlert("This application requires CarbonLib 1.2 or newer.  Run Software Update, or install it from the Otto Matic CD.");
		}
	}


#if !DEMO
			/* CHECK TIME-BOMB */
	{
//		unsigned long secs;
//		DateTimeRec	d;
//
//		GetDateTime(&secs);
//		SecondsToDate(secs, &d);
//
//		if ((d.year > 2001) ||
//			((d.year == 2001) && (d.month > 11)))
//		{
//			DoFatalAlert("Sorry, but this beta has expired");
//		}
	}
#endif


			/* CHECK PREFERENCES FOLDER */

	iErr = FindFolder(kOnSystemDisk,kPreferencesFolderType,kDontCreateFolder,			// locate the folder
					&gPrefsFolderVRefNum,&gPrefsFolderDirID);
	if (iErr != noErr)
		DoAlert("Warning: Cannot locate the Preferences folder.");

	iErr = DirCreate(gPrefsFolderVRefNum,gPrefsFolderDirID,"OttoMatic",&createdDirID);		// make folder in there



		/* CHECK MEMORY */
	{
		u_long	mem;
		iErr = Gestalt(gestaltPhysicalRAMSize,(long *)&mem);
		if (iErr == noErr)
		{
					/* CHECK FOR LOW-MEMORY SITUATIONS */

			mem /= 1024;
			mem /= 1024;
			if (mem <= 64)						// see if have only 64 MB of real RAM or less installed
			{
				u_long	vmAttr;

						/* MUST HAVE VM ON */

				Gestalt(gestaltVMAttr,(long *)&vmAttr);	// get VM attribs to see if its ON
				if (!(vmAttr & (1 << gestaltVMPresent)))
				{
					DoFatalAlert("This game needs at least 96MB of real RAM to run, however, turn on Virtual Memory, reboot your computer, and it might work.");
				}
				gLowMemMode = true;
			}
		}
	}




		/***********************************/
		/* SEE IF LITTLE-SNITCH IS RUNNING */
		/***********************************/


	gLittleSnitch = false;

#if ((DEMO == 0) && (OEM == 0))

	{
		ProcessSerialNumber psn = {kNoProcess, kNoProcess};
		ProcessInfoRec	info;
		Str255		s;

		info.processName = s;
		info.processInfoLength = sizeof(ProcessInfoRec);
		info.processAppSpec = nil;

		while(GetNextProcess(&psn) == noErr)
		{
			char	pname[256];
			char	*matched;

			iErr = GetProcessInformation(&psn, &info);
			if (iErr)
				break;

			p2cstrcpy(pname, &s[0]);					// convert pstring to cstring

			matched = strstr (pname, "Snitc");			// does "Snitc" appear anywhere in the process name?
			if (matched != nil)
			{
				gLittleSnitch = true;
				break;
			}
		}
	}
#endif



		/***************************************/
		/* SEE IF QUICKEN SCHEDULER IS RUNNING */
		/***************************************/

	{
		ProcessSerialNumber psn = {kNoProcess, kNoProcess};
		ProcessInfoRec	info;
		short			i;
		Str255		s;
		const char snitch[] = "Quicken Scheduler";

		info.processName = s;
		info.processInfoLength = sizeof(ProcessInfoRec);
		info.processAppSpec = nil;

		while(GetNextProcess(&psn) == noErr)
		{
			iErr = GetProcessInformation(&psn, &info);
			if (iErr)
				break;

			if (s[0] != snitch[0])					// see if string matches
				goto next_process2;

			for (i = 1; i <= s[0]; i++)
			{
				if (s[i] != snitch[i])
					goto next_process2;
			}

			DoAlert("IMPORTANT:  Quicken Scheduler is known to cause certain keyboard access functions in OS X to malfunction.  If the keyboard does not appear to be working in this game, quit Quicken Scheduler to fix it.");

next_process2:;
		}
	}

}


/******************** REGULATE SPEED ***************/

void RegulateSpeed(short fps)
{
u_long	n;
static u_long oldTick = 0;

	n = 60 / fps;
	while ((TickCount() - oldTick) < n) {}			// wait for n ticks
	oldTick = TickCount();							// remember current time
}


/************* COPY PSTR **********************/

void CopyPStr(ConstStr255Param	inSourceStr, StringPtr	outDestStr)
{
short	dataLen = inSourceStr[0] + 1;

	BlockMoveData(inSourceStr, outDestStr, dataLen);
	outDestStr[0] = dataLen - 1;
}



#pragma mark -



/************** CALC FRAMES PER SECOND *****************/
//
// This version uses UpTime() which is only available on PCI Macs.
//

void CalcFramesPerSecond(void)
{
AbsoluteTime currTime,deltaTime;
static AbsoluteTime time = {0,0};
Nanoseconds	nano;

slow_down:
	currTime = UpTime();

	deltaTime = SubAbsoluteFromAbsolute(currTime, time);
	nano = AbsoluteToNanoseconds(deltaTime);

	gFramesPerSecond = 1000000.0f / (float)nano.lo;
	gFramesPerSecond *= 1000.0f;

	if (gFramesPerSecond > 100)					// keep from going over 100fps (there were problems in 2.0 of frame rate precision loss)
		goto slow_down;

	if (gFramesPerSecond < DEFAULT_FPS)			// (avoid divide by 0's later)
		gFramesPerSecond = DEFAULT_FPS;
	gFramesPerSecondFrac = 1.0f/gFramesPerSecond;		// calc fractional for multiplication


	time = currTime;	// reset for next time interval
}


/********************* IS POWER OF 2 ****************************/

Boolean IsPowerOf2(int num)
{
int		i;

	i = 2;
	do
	{
		if (i == num)				// see if this power of 2 matches
			return(true);
		i *= 2;						// next power of 2
	}while(i <= num);				// search until power is > number

	return(false);
}

#pragma mark-

/******************* MY FLUSH EVENTS **********************/
//
// This call makes damed sure that there are no events anywhere in any event queue.
//

void MyFlushEvents(void)
{
EventRecord 	theEvent;

	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());

#if 1
			/* POLL EVENT QUEUE TO BE SURE THINGS ARE FLUSHED OUT */

	while (GetNextEvent(mDownMask|mUpMask|keyDownMask|keyUpMask|autoKeyMask, &theEvent));


	FlushEvents (everyEvent, REMOVE_ALL_EVENTS);
	FlushEventQueue(GetMainEventQueue());
#endif
}


#pragma mark -




/********************* SWIZZLE SHORT **************************/

short SwizzleShort(short *shortPtr)
{
short	theShort = *shortPtr;

#if __LITTLE_ENDIAN__

	Byte	b1 = theShort & 0xff;
	Byte	b2 = (theShort & 0xff00) >> 8;

	theShort = (b1 << 8) | b2;
#endif

	return(theShort);
}


/********************* SWIZZLE USHORT **************************/

u_short SwizzleUShort(u_short *shortPtr)
{
u_short	theShort = *shortPtr;

#if __LITTLE_ENDIAN__

	Byte	b1 = theShort & 0xff;
	Byte	b2 = (theShort & 0xff00) >> 8;

	theShort = (b1 << 8) | b2;
#endif

	return(theShort);
}



/********************* SWIZZLE LONG **************************/

long SwizzleLong(long *longPtr)
{
long	theLong = *longPtr;

#if __LITTLE_ENDIAN__

	Byte	b1 = theLong & 0xff;
	Byte	b2 = (theLong & 0xff00) >> 8;
	Byte	b3 = (theLong & 0xff0000) >> 16;
	Byte	b4 = (theLong & 0xff000000) >> 24;

	theLong = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;

#endif

	return(theLong);
}


/********************* SWIZZLE U LONG **************************/

u_long SwizzleULong(u_long *longPtr)
{
u_long	theLong = *longPtr;

#if __LITTLE_ENDIAN__

	Byte	b1 = theLong & 0xff;
	Byte	b2 = (theLong & 0xff00) >> 8;
	Byte	b3 = (theLong & 0xff0000) >> 16;
	Byte	b4 = (theLong & 0xff000000) >> 24;

	theLong = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;

#endif

	return(theLong);
}



/********************* SWIZZLE FLOAT **************************/

float SwizzleFloat(float *floatPtr)
{
float	*theFloat;
u_long	bytes = SwizzleULong((u_long *)floatPtr);

	theFloat = (float *)&bytes;

	return(*theFloat);
}


/************************ SWIZZLE POINT 3D ********************/

void SwizzlePoint3D(OGLPoint3D *pt)
{
	pt->x = SwizzleFloat(&pt->x);
	pt->y = SwizzleFloat(&pt->y);
	pt->z = SwizzleFloat(&pt->z);

}


/************************ SWIZZLE VECTOR 3D ********************/

void SwizzleVector3D(OGLVector3D *pt)
{
	pt->x = SwizzleFloat(&pt->x);
	pt->y = SwizzleFloat(&pt->y);
	pt->z = SwizzleFloat(&pt->z);

}

/************************ SWIZZLE UV ********************/

void SwizzleUV(OGLTextureCoord *pt)
{
	pt->u = SwizzleFloat(&pt->u);
	pt->v = SwizzleFloat(&pt->v);

}






