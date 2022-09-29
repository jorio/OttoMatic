/****************************/
/*      MISC ROUTINES       */
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"

/****************************/
/*    CONSTANTS             */
/****************************/

#define	DEFAULT_FPS			10

/**********************/
/*     VARIABLES      */
/**********************/

short	gPrefsFolderVRefNum;
long	gPrefsFolderDirID;

u_long 	seed0 = 0, seed1 = 0, seed2 = 0;

float	gFramesPerSecond, gFramesPerSecondFrac;



/**********************/
/*     PROTOTYPES     */
/**********************/


/****************** DO SYSTEM ERROR ***************/

void ShowSystemErr(long err)
{
Str255		numStr;

	Enter2D();

	snprintf(numStr, sizeof(numStr), "System error: %ld", err);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Otto Matic", numStr, gSDLWindow);

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

	snprintf(numStr, sizeof(numStr), "System error (non-fatal): %ld", err);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Otto Matic", numStr, gSDLWindow);

	Exit2D();
}


/*********************** DO ALERT *******************/

void DoAlert(const char* s)
{
	Enter2D();

	printf("OTTO MATIC Alert: %s\n", s);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Otto Matic", s, gSDLWindow);

	Exit2D();
	SDL_ShowCursor(0);
}


/*********************** DO FATAL ALERT *******************/

void DoFatalAlert(const char* s)
{
	Enter2D();

	printf("OTTO MATIC Fatal Alert: %s\n", s);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Otto Matic", s, gSDLWindow);

	Exit2D();
	CleanQuit();
}


/*********************** DO ASSERT *******************/

void DoAssert(const char* msg, const char* file, int line)
{
	Enter2D();
	printf("GAME ASSERTION FAILED: %s - %s:%d\n", msg, file, line);
	static char alertbuf[1024];
	snprintf(alertbuf, 1024, "%s\n%s:%d", msg, file, line);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Otto Matic: Assertion Failed!", alertbuf, /*gSDLWindow*/ nil);
	Exit2D();
	ExitToShell();
}


/************ CLEAN QUIT ***************/

void CleanQuit(void)
{
static Boolean	beenHere = false;

	if (!beenHere)
	{
		beenHere = true;

		ShutdownSound();								// cleanup sound stuff

		DisposeTerrain();								// dispose of any memory allocated by terrain manager
		DisposeAllBG3DContainers();						// nuke all models
		DisposeAllSpriteGroups();						// nuke all sprites

		if (gGameViewInfoPtr)							// see if need to dispose this
			OGL_DisposeWindowSetup();

		OGL_Shutdown();

		TextMesh_DisposeMetrics();
	}

	UseResFile(gMainAppRezFile);

	SDL_ShowCursor(1);
	MyFlushEvents();

//	SavePrefs();							// save prefs before bailing

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


/************** RANDOM FLOAT ********************/
//
// returns a random float between a and b
// You must ensure that a < b !!!
//

float RandomFloatRange(float a, float b)
{
	return a + (b-a) * RandomFloat();
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


/**************** POSITIVE MODULO *******************/

int PositiveModulo(int value, unsigned int m)
{
	int mod = value % (int) m;
	if (mod < 0)
	{
		mod += m;
	}
	return mod;
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
	return NewPtr(size);
}


/****************** ALLOC PTR CLEAR ********************/

void *AllocPtrClear(long size)
{
	return NewPtrClear(size);
}


/***************** SAFE DISPOSE PTR ***********************/

void SafeDisposePtr(Ptr ptr)
{
	DisposePtr(ptr);
}



/******************* CHECK PREFERENCES FOLDER ******************/

void CheckPreferencesFolder(void)
{
OSErr	iErr;
long		createdDirID;


			/* CHECK PREFERENCES FOLDER */

	iErr = FindFolder(kOnSystemDisk,kPreferencesFolderType,kDontCreateFolder,			// locate the folder
					&gPrefsFolderVRefNum,&gPrefsFolderDirID);
	if (iErr != noErr)
		DoAlert("Warning: Cannot locate the Preferences folder.");

	iErr = DirCreate(gPrefsFolderVRefNum,gPrefsFolderDirID,"OttoMatic",&createdDirID);		// make folder in there
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


/************** CALC FRAMES PER SECOND *****************/
//
// This version uses UpTime() which is only available on PCI Macs.
//

void CalcFramesPerSecond(void)
{
	static UnsignedWide time;
	UnsignedWide currTime;
	unsigned long deltaTime;

slow_down:
	Microseconds(&currTime);
	deltaTime = currTime.lo - time.lo;

	gFramesPerSecond = 1000000.0f / deltaTime;

#if 0
AbsoluteTime currTime,deltaTime;
static AbsoluteTime time = {0,0};
Nanoseconds	nano;

slow_down:
	currTime = UpTime();

	deltaTime = SubAbsoluteFromAbsolute(currTime, time);
	nano = AbsoluteToNanoseconds(deltaTime);

	gFramesPerSecond = 1000000.0f / (float)nano.lo;
	gFramesPerSecond *= 1000.0f;
#endif

	if (gFramesPerSecond > 100)					// keep from going over 100fps (there were problems in 2.0 of frame rate precision loss)
		goto slow_down;

	if (gFramesPerSecond < DEFAULT_FPS)			// (avoid divide by 0's later)
		gFramesPerSecond = DEFAULT_FPS;

	if (GetKeyState(SDL_SCANCODE_GRAVE) && GetKeyState(SDL_SCANCODE_KP_PLUS))		// debug speed-up with `+KP_PLUS
		gFramesPerSecond = 10;

#if _DEBUG
	if (gSDLController)
	{
		float analogSpeedUp = SDL_GameControllerGetAxis(gSDLController, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 32767.0f;
		if (analogSpeedUp > .25f)
			gFramesPerSecond = 10;
	}
#endif

	gFramesPerSecondFrac = 1.0f/gFramesPerSecond;		// calc fractional for multiplication

//printf("FPS: %f\n", gFramesPerSecond);

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
	SDL_FlushEvents(SDL_KEYDOWN, SDL_JOYBUTTONUP);
	SDL_FlushEvents(SDL_CONTROLLERAXISMOTION, SDL_CONTROLLERBUTTONUP);
}


#pragma mark -




/********************* SWIZZLE SHORT **************************/

int16_t SwizzleShort(int16_t *shortPtr)
{
int16_t	theShort = *shortPtr;

#if __LITTLE_ENDIAN__

	Byte	b1 = theShort & 0xff;
	Byte	b2 = (theShort & 0xff00) >> 8;

	theShort = (b1 << 8) | b2;
#endif

	return(theShort);
}


/********************* SWIZZLE USHORT **************************/

uint16_t SwizzleUShort(uint16_t *shortPtr)
{
uint16_t	theShort = *shortPtr;

#if __LITTLE_ENDIAN__

	Byte	b1 = theShort & 0xff;
	Byte	b2 = (theShort & 0xff00) >> 8;

	theShort = (b1 << 8) | b2;
#endif

	return(theShort);
}



/********************* SWIZZLE LONG **************************/

int32_t SwizzleLong(int32_t *longPtr)
{
int32_t	theLong = *longPtr;

#if __LITTLE_ENDIAN__

	int32_t	b1 = theLong & 0xff;
	int32_t	b2 = (theLong & 0xff00) >> 8;
	int32_t	b3 = (theLong & 0xff0000) >> 16;
	int32_t	b4 = (theLong & 0xff000000) >> 24;

	theLong = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;

#endif

	return(theLong);
}


/********************* SWIZZLE U LONG **************************/

uint32_t SwizzleULong(uint32_t *longPtr)
{
uint32_t	theLong = *longPtr;

#if __LITTLE_ENDIAN__

	uint32_t	b1 = theLong & 0xff;
	uint32_t	b2 = (theLong & 0xff00) >> 8;
	uint32_t	b3 = (theLong & 0xff0000) >> 16;
	uint32_t	b4 = (theLong & 0xff000000) >> 24;

	theLong = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;

#endif

	return(theLong);
}



/********************* SWIZZLE U LONG **************************/

uint64_t SwizzleULong64(uint64_t *u64Ptr)
{
	uint64_t	u64 = *u64Ptr;

#if __LITTLE_ENDIAN__
	uint64_t	b1 = (u64 >>  0) & 0xff;
	uint64_t	b2 = (u64 >>  8) & 0xff;
	uint64_t	b3 = (u64 >> 16) & 0xff;
	uint64_t	b4 = (u64 >> 24) & 0xff;
	uint64_t	b5 = (u64 >> 32) & 0xff;
	uint64_t	b6 = (u64 >> 40) & 0xff;
	uint64_t	b7 = (u64 >> 48) & 0xff;
	uint64_t	b8 = (u64 >> 56) & 0xff;

	u64	= (b1 << 56)
		| (b2 << 48)
		| (b3 << 40)
		| (b4 << 32)
		| (b5 << 24)
		| (b6 << 16)
		| (b7 << 8)
		| (b8);
#endif

	return u64;
}





/********************* SWIZZLE FLOAT **************************/

float SwizzleFloat(float *floatPtr)
{
float	*theFloat;
uint32_t	bytes = SwizzleULong((uint32_t *)floatPtr);

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


/************************ READ INTEGER THEN BYTESWAP ********************/

int16_t FSReadBEShort(short refNum)
{
	int16_t result = 0;
	long count = sizeof(result);
	OSErr err = FSRead(refNum, &count, (Ptr) &result);
	GAME_ASSERT(err == noErr);
	return SwizzleShort(&result);
}

uint16_t FSReadBEUShort(short refNum)
{
	uint16_t result = 0;
	long count = sizeof(result);
	OSErr err = FSRead(refNum, &count, (Ptr) &result);
	GAME_ASSERT(err == noErr);
	return SwizzleUShort(&result);
}

int32_t FSReadBELong(short refNum)
{
	int32_t result = 0;
	long count = sizeof(result);
	OSErr err = FSRead(refNum, &count, (Ptr) &result);
	GAME_ASSERT(err == noErr);
	return SwizzleLong(&result);
}

uint32_t FSReadBEULong(short refNum)
{
	uint32_t result = 0;
	long count = sizeof(result);
	OSErr err = FSRead(refNum, &count, (Ptr) &result);
	GAME_ASSERT(err == noErr);
	return SwizzleULong(&result);
}

float FSReadBEFloat(short refNum)
{
	float result = 0;
	long count = sizeof(result);
	OSErr err = FSRead(refNum, &count, (Ptr) &result);
	GAME_ASSERT(err == noErr);
	return SwizzleFloat(&result);
}



