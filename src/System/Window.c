/****************************/
/*        WINDOW            */
/* By Brian Greenstone      */
/* (c)2005 Pangea Software  */
/* (c)2021 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveFadeEvent(ObjNode *theNode);
static void DrawFadePane(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/


/**********************/
/*     VARIABLES      */
/**********************/

float		gGammaFadePercent = 1.0;

int				gGameWindowWidth, gGameWindowHeight;


/****************  INIT WINDOW STUFF *******************/

void InitWindowStuff(void)
{
		/* SHOW A COUPLE BLACK FRAMES BEFORE WE BEGIN */

	OGLSetupInputType viewDef;
	OGL_NewViewDef(&viewDef);
	OGL_SetupWindow(&viewDef);
	for (int i = 0; i < 45; i++)
	{
		OGL_DrawScene(nil);
		UpdateInput();  // will flush SDL events
	}
	OGL_DisposeWindowSetup();
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
	if (gCommandLine.skipFluff)
		return;

	SDL_GLContext currentContext = SDL_GL_GetCurrentContext();
	if (!currentContext)
	{
#if _DEBUG
		printf("%s: no GL context; skipping fade out\n", __func__);
#endif
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

	float u1 = 0;
	float v1 = 0;
	float u2 = (float)windowWidth / textureWidth;
	float v2 = (float)windowHeight / textureHeight;

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
		glTexCoord2f(u1,v2); glVertex3f(0, 0, 0);
		glTexCoord2f(u2,v2); glVertex3f(g2DLogicalWidth, 0, 0);
		glTexCoord2f(u2,v1); glVertex3f(g2DLogicalWidth, g2DLogicalHeight, 0);
		glTexCoord2f(u1,v1); glVertex3f(0, g2DLogicalHeight, 0);
		glEnd();
		SDL_GL_SwapWindow(gSDLWindow);
		CHECK_GL_ERROR();

		SDL_Delay(15);
	}

	SafeDisposePtr(textureData);
	glDeleteTextures(1, &texture);

	OGL_PopState();

	gGammaFadePercent = 0;
}

/********************** GAMMA ON *********************/

void GammaOn(void)
{
	if (gCommandLine.skipFluff)
		return;

	if (gGammaFadePercent != 1.0f)
	{
		gGammaFadePercent = 1.0f;
	}
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

static void DrawFadePane(ObjNode* theNode)
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
	// Linux: work around game window sent to background after showing a dialog box
	// Windows: work around alert box appearing behind game
#if !__APPLE__
	SDL_SetWindowFullscreen(gSDLWindow, false);
	SDL_HideWindow(gSDLWindow);
	SDL_PumpEvents();
#endif
}


/************************** EXIT 2D *************************/

void Exit2D(void)
{
#if !__APPLE__
	SDL_PumpEvents();
	SDL_ShowWindow(gSDLWindow);
	SetFullscreenModeFromPrefs();
#endif
}


/******************* SET FULLSCREEN MODE FROM PREFS **************/

void SetFullscreenModeFromPrefs(void)
{
	if (!gGamePrefs.fullscreen)
	{
		SDL_SetWindowFullscreen(gSDLWindow, 0);
		SDL_SetWindowPosition(gSDLWindow,
							  SDL_WINDOWPOS_CENTERED_DISPLAY(gGamePrefs.preferredDisplay),
							  SDL_WINDOWPOS_CENTERED_DISPLAY(gGamePrefs.preferredDisplay));
	}
	else
	{
		// We must switch back to windowed mode for the preferred monitor to take effect
		SDL_SetWindowFullscreen(gSDLWindow, 0);
		SDL_SetWindowPosition(gSDLWindow,
							  SDL_WINDOWPOS_CENTERED_DISPLAY(gGamePrefs.preferredDisplay),
							  SDL_WINDOWPOS_CENTERED_DISPLAY(gGamePrefs.preferredDisplay));

		if (gCommandLine.fullscreenWidth <= 0 || gCommandLine.fullscreenHeight <= 0)
		{
			// No custom fullscreen resolution specified.
			// Enter windowed fullscreen mode.
			SDL_SetWindowFullscreen(gSDLWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
		else
		{
			SDL_DisplayMode mode =
			{
					.w = gCommandLine.fullscreenWidth,
					.h = gCommandLine.fullscreenHeight,
					.refresh_rate = gCommandLine.fullscreenRefreshRate,
					.format = SDL_PIXELFORMAT_UNKNOWN,
					.driverdata = NULL,
			};
			SDL_SetWindowDisplayMode(gSDLWindow, &mode);
			SDL_SetWindowFullscreen(gSDLWindow, SDL_WINDOW_FULLSCREEN);
		}
	}

	EatMouseEvents();
}
