/****************************/
/*        WINDOWS           */
/* (c)2005 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode	*gCurrentNode,*gFirstNodePtr;
extern	float	gFramesPerSecondFrac;
extern	float					g2DLogicalWidth;
extern	float					g2DLogicalHeight;
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
static void DrawFadePane(ObjNode* theNode, const OGLSetupOutputType* setupInfo);


/****************************/
/*    CONSTANTS             */
/****************************/

#define		DO_GAMMA	1


/**********************/
/*     VARIABLES      */
/**********************/

u_long			gDisplayVRAM = 0;
u_long			gVRAMAfterBuffers = 0;


float		gGammaFadePercent = 1.0;

int				gGameWindowWidth, gGameWindowHeight;


/****************  INIT WINDOW STUFF *******************/

void InitWindowStuff(void)
{
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

	SetInfobarSpriteState(false);

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
		glTexCoord2f(0,1); glVertex3f(0, 0, 0);
		glTexCoord2f(1,1); glVertex3f(g2DLogicalWidth, 0, 0);
		glTexCoord2f(1,0); glVertex3f(g2DLogicalWidth, g2DLogicalHeight, 0);
		glTexCoord2f(0,0); glVertex3f(0, g2DLogicalHeight, 0);
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

	if (gGammaFadePercent != 1.0f)
	{
		gGammaFadePercent = 1.0f;

		SOURCE_PORT_MINOR_PLACEHOLDER(); // CGSetDisplayTransferByTable(0, 256, gOriginalRedTable, gOriginalGreenTable, gOriginalBlueTable);
	}
#endif
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
	newObj->CustomDrawFunction = DrawFadePane;

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
}

/******************** DRAW FADE PANE *********************/

static void DrawFadePane(ObjNode* theNode, const OGLSetupOutputType* setupInfo)
{
	OGL_PushState();

	SetInfobarSpriteState(false);

	glColor4f(0, 0, 0, 1.0f - gGammaFadePercent);

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBegin(GL_QUADS);
	glVertex3f(0,					0,					0);
	glVertex3f(g2DLogicalWidth,		0,					0);
	glVertex3f(g2DLogicalWidth,		g2DLogicalHeight,	0);
	glVertex3f(0,					g2DLogicalHeight,	0);
	glEnd();
	glDisable(GL_BLEND);

	OGL_PopState();
}

/************************** ENTER 2D *************************/

void Enter2D(void)
{
#if !__APPLE__ && !_WIN32		// Linux: work around game window sent to background after showing a dialog box
	SDL_SetWindowFullscreen(gSDLWindow, false);
	SDL_HideWindow(gSDLWindow);
	SDL_PumpEvents();
#endif
}


/************************** EXIT 2D *************************/

void Exit2D(void)
{
#if !__APPLE__ && !_WIN32		// Linux: work around game window sent to background after showing a dialog box
	SDL_PumpEvents();
	SDL_ShowWindow(gSDLWindow);
	SetFullscreenModeFromPrefs();
#endif
}


/******************* DO LOCK PIXELS **************/

void DoLockPixels(GWorldPtr world)
{
PixMapHandle pm;

	pm = GetGWorldPixMap(world);
	if (LockPixels(pm) == false)
		DoFatalAlert("PixMap Went Bye,Bye?!");
}



/******************* SET FULLSCREEN MODE FROM PREFS **************/

void SetFullscreenModeFromPrefs(void)
{
	SDL_SetWindowFullscreen(
			gSDLWindow,
			gGamePrefs.fullscreen? SDL_WINDOW_FULLSCREEN_DESKTOP: 0);
}
