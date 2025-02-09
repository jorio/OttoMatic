/****************************/
/*      MISC ROUTINES       */
/* By Brian Greenstone      */
/* (c)2002 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MIN_FPS				15			// Platform physics are unstable under 15 FPS
#define MAX_FPS				500

/**********************/
/*     VARIABLES      */
/**********************/

short	gPrefsFolderVRefNum;
long	gPrefsFolderDirID;

static uint32_t seed0 = 0x2a80ce30, seed1 = 0, seed2 = 0;

float	gFramesPerSecond, gFramesPerSecondFrac;



/**********************/
/*     PROTOTYPES     */
/**********************/



/*********************** DO ALERT *******************/

void DoAlert(const char* format, ...)
{
	Enter2D();

	char message[1024];
	va_list args;
	va_start(args, format);
	SDL_vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Game Alert: %s", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GAME_FULL_NAME, message, gSDLWindow);
}


/*********************** DO FATAL ALERT *******************/

void DoFatalAlert(const char* format, ...)
{
	if (gSDLWindow)
		SDL_SetWindowFullscreen(gSDLWindow, 0);

	char message[1024];
	va_list args;
	va_start(args, format);
	SDL_vsnprintf(message, sizeof(message), format, args);
	va_end(args);

	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Game Fatal Alert: %s", message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GAME_FULL_NAME, message, gSDLWindow);
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

	SDL_ShowCursor();
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

uint32_t MyRandomLong(void)
{
  return seed2 ^= (((seed1 ^= (seed2>>5)*1568397607UL)>>7)+
                   (seed0 = (seed0+1)*3141592621UL))*2435386481UL;
}


/************************* RANDOM RANGE *************************/
//
// THE RANGE *IS* INCLUSIVE OF MIN AND MAX
//

uint16_t	RandomRange(unsigned short min, unsigned short max)
{
uint16_t		qdRdm;											// treat return value as 0-65536
uint32_t		range, t;

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

void SetMyRandomSeed(uint32_t seed)
{
	seed0 = seed;
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


/************** CALC FRAMES PER SECOND *****************/

void CalcFramesPerSecond(void)
{
	static uint64_t performanceFrequency = 0;
	static uint64_t prevTime = 0;
	uint64_t currTime;

	if (performanceFrequency == 0)
	{
		performanceFrequency = SDL_GetPerformanceFrequency();
	}

slow_down:
	currTime = SDL_GetPerformanceCounter();
	uint64_t deltaTime = currTime - prevTime;

	if (deltaTime <= 0)
	{
		gFramesPerSecond = MIN_FPS;						// avoid divide by 0
	}
	else
	{
		gFramesPerSecond = performanceFrequency / (float)(deltaTime);

		if (gFramesPerSecond > MAX_FPS)					// keep from going over 100fps (there were problems in 2.0 of frame rate precision loss)
		{
			if (gFramesPerSecond - MAX_FPS > 1000)		// try to sneak in some sleep if we have 1 ms to spare
			{
				SDL_Delay(1);
			}
			goto slow_down;
		}

		if (gFramesPerSecond < MIN_FPS)					// (avoid divide by 0's later)
		{
			gFramesPerSecond = MIN_FPS;
		}
	}

	// In debug builds, speed up with BACKTICK+KP_PLUS or LT on gamepad
#if _DEBUG
	if (GetKeyState(SDL_SCANCODE_KP_PLUS))
#else
	if (GetKeyState(SDL_SCANCODE_GRAVE) && GetKeyState(SDL_SCANCODE_KP_PLUS))
#endif
	{
		gFramesPerSecond = MIN_FPS;
	}
#if _DEBUG
	else if (gSDLGamepad)
	{
		float analogSpeedUp = SDL_GetGamepadAxis(gSDLGamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) / 32767.0f;
		if (analogSpeedUp > .25f)
		{
			gFramesPerSecond = MIN_FPS;
		}
	}
#endif

	gFramesPerSecondFrac = 1.0f / gFramesPerSecond;		// calc fractional for multiplication

	prevTime = currTime;								// reset for next time interval
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
	// Flush input events
	SDL_FlushEvents(SDL_EVENT_KEY_DOWN, SDL_EVENT_FINGER_CANCELED);
}


#pragma mark -




/********************* SWIZZLE SHORT **************************/

int16_t SwizzleShort(const int16_t *shortPtr)
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

uint16_t SwizzleUShort(const uint16_t *shortPtr)
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

int32_t SwizzleLong(const int32_t *longPtr)
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

uint32_t SwizzleULong(const uint32_t *longPtr)
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

uint64_t SwizzleULong64(const uint64_t *u64Ptr)
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

float SwizzleFloat(const float *floatPtr)
{
	const void* blob = floatPtr;

	uint32_t theLong = SwizzleULong((const uint32_t *) blob);

	blob = &theLong;

	return *((const float *) blob);
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



