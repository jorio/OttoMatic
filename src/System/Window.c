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

static void MoveFadePane(ObjNode *theNode);
static void DrawFadePane(ObjNode *theNode);

#define FaderFrameCounter	Special[0]


/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	kFaderMode_FadeOut,
	kFaderMode_FadeIn,
	kFaderMode_Done,
};

/**********************/
/*     VARIABLES      */
/**********************/

float		gGammaFadeFrac = 1.0;

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


/***************** FREEZE-FRAME FADE OUT ********************/

void OGL_FadeOutScene(void (*drawCall)(void), void (*moveCall)(void))
{
	if (gCommandLine.skipFluff)
		return;

	ObjNode* fader = MakeFadeEvent(false, 3.0f);

	long pFaderFrameCount = fader->FaderFrameCounter;

	while (fader->Mode != kFaderMode_Done)
	{
		CalcFramesPerSecond();
		UpdateInput();

		if (moveCall)
		{
			moveCall();
		}

		// Force fader object to move even if MoveObjects was skipped
		if (fader->FaderFrameCounter == pFaderFrameCount)	// fader wasn't moved by moveCall
		{
			MoveFadePane(fader);
			pFaderFrameCount = fader->FaderFrameCounter;
		}

		OGL_DrawScene(drawCall);
	}

	// Draw one more blank frame
	gGammaFadeFrac = 0;
	CalcFramesPerSecond();
	UpdateInput();
	OGL_DrawScene(drawCall);

#if 0
	if (gGameView->fadeSound)
	{
		FadeSound(0);
		KillSong();
		StopAllEffectChannels();
		FadeSound(1);		// restore sound volume for new playback
	}
#endif
}


/********************** GAMMA ON *********************/

void GammaOn(void)
{
	if (gCommandLine.skipFluff)
		return;

	if (gGammaFadeFrac != 1.0f)
	{
		gGammaFadeFrac = 1.0f;
	}
}



/******************** MAKE FADE EVENT *********************/
//
// INPUT:	fadeIn = true if want fade IN, otherwise fade OUT.
//

ObjNode* MakeFadeEvent(Boolean fadeIn, float fadeSpeed)
{
ObjNode	*newObj;
ObjNode		*thisNodePtr;

		/* SCAN FOR OLD FADE EVENTS STILL IN LIST */

	thisNodePtr = gFirstNodePtr;

	while (thisNodePtr)
	{
		if (thisNodePtr->MoveCall == MoveFadePane)
		{
			thisNodePtr->Mode = fadeIn;									// set new mode
			thisNodePtr->Speed = fadeSpeed;
			return thisNodePtr;
		}
		thisNodePtr = thisNodePtr->NextNode;							// next node
	}


		/* MAKE NEW FADE EVENT */

	gGammaFadeFrac = fadeIn? 0: 1;

	NewObjectDefinitionType def =
	{
		.genre = CUSTOM_GENRE,
		.flags = 0,
		.slot = FADEPANE_SLOT,
		.moveCall = MoveFadePane,
		.scale = 1,
	};
	newObj = MakeNewObject(&def);
	newObj->CustomDrawFunction = DrawFadePane;

	newObj->Mode = fadeIn ? kFaderMode_FadeIn : kFaderMode_FadeOut;
	newObj->FaderFrameCounter = 0;
	newObj->Speed = fadeSpeed;

	return newObj;
}


/***************** MOVE FADE EVENT ********************/

static void MoveFadePane(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	speed = theNode->Speed * fps;

			/* SEE IF FADE IN */

	if (theNode->Mode == kFaderMode_FadeIn)
	{
		gGammaFadeFrac += speed;
		if (gGammaFadeFrac >= 1.0f)				// see if @ 100%
		{
			gGammaFadeFrac = 1;
			theNode->Mode = kFaderMode_Done;
			DeleteObject(theNode);				// nuke it if fading in
		}
	}

			/* FADE OUT */

	else if (theNode->Mode == kFaderMode_FadeOut)
	{
		gGammaFadeFrac -= speed;
		if (gGammaFadeFrac <= 0.0f)				// see if @ 0%
		{
			gGammaFadeFrac = 0;
			theNode->Mode = kFaderMode_Done;
			theNode->MoveCall = NULL;			// DON'T nuke the fader pane if fading out -- but don't run this again
		}
	}
}

/******************** DRAW FADE PANE *********************/

static void DrawFadePane(ObjNode* theNode)
{
	OGL_PushState();

	SetInfobarSpriteState(false);

	glColor4f(0, 0, 0, 1.0f - gGammaFadeFrac);

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

	SDL_GL_SetSwapInterval(gGamePrefs.vsync);

	EatMouseEvents();
}
