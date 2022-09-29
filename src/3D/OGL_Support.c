/****************************/
/*   OPENGL SUPPORT.C	    */
/*   By Brian Greenstone    */
/* (c)2001 Pangea Software  */
/* (c)2022 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void OGL_InitFont(void);
static void OGL_CreateDrawContext(void);
static void OGL_DisposeDrawContext(void);
static void OGL_InitDrawContext(OGLViewDefType *viewDefPtr);
static void OGL_SetStyles(OGLSetupInputType *setupDefPtr);
static void OGL_CreateLights(OGLLightDefType *lightDefPtr);

static void ColorBalanceRGBForAnaglyph(uint32_t *rr, uint32_t *gg, uint32_t *bb);
static void	ConvertTextureToGrey(void *imageMemory, short width, short height, GLint srcFormat, GLint dataType);
static void	ConvertTextureToColorAnaglyph(void *imageMemory, short width, short height, GLint srcFormat, GLint dataType);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	STATE_STACK_SIZE	20



/*********************/
/*    VARIABLES      */
/*********************/

float					gAnaglyphFocallength	= 150.0f;
float					gAnaglyphEyeSeparation 	= 40.0f;
Byte					gAnaglyphPass;
u_char					gAnaglyphGreyTable[255];


SDL_GLContext	gAGLContext = nil;


OGLMatrix4x4	gViewToFrustumMatrix,gWorldToViewMatrix,gWorldToFrustumMatrix;
OGLMatrix4x4	gWorldToWindowMatrix,gFrustumToWindowMatrix;

float	gCurrentAspectRatio = 1;
float	g2DLogicalWidth		= 640.0f;
float	g2DLogicalHeight	= 480.0f;


Boolean		gStateStack_Lighting[STATE_STACK_SIZE];
Boolean		gStateStack_CullFace[STATE_STACK_SIZE];
Boolean		gStateStack_DepthTest[STATE_STACK_SIZE];
Boolean		gStateStack_Normalize[STATE_STACK_SIZE];
Boolean		gStateStack_Texture2D[STATE_STACK_SIZE];
Boolean		gStateStack_Blend[STATE_STACK_SIZE];
Boolean		gStateStack_Fog[STATE_STACK_SIZE];
GLboolean	gStateStack_DepthMask[STATE_STACK_SIZE];
GLint		gStateStack_BlendDst[STATE_STACK_SIZE];
GLint		gStateStack_BlendSrc[STATE_STACK_SIZE];
GLfloat		gStateStack_Color[STATE_STACK_SIZE][4];

int			gStateStackIndex = 0;

int			gPolysThisFrame;
int			gVRAMUsedThisFrame = 0;

Boolean		gMyState_Lighting;

static ObjNode* gDebugText;


/******************** OGL BOOT *****************/
//
// Initialize my OpenGL stuff.
//

void OGL_Boot(void)
{
short	i;
float	f;

		/* GENERATE ANAGLYPH GREY CONVERSION TABLE */
		//
		// This makes an intensity curve to brighten things up, but sometimes
		// it washes them out.
		//

	f = 0;
	for (i = 0; i < 255; i++)
	{
		gAnaglyphGreyTable[i] = i; //sin(f) * 255.0f;
		f += (PI/2.0) / 255.0f;
	}


		/* CREATE DRAW CONTEXT THAT WILL BE USED THROUGHOUT THE GAME */

	OGL_CreateDrawContext();
}


/******************** OGL SHUTDOWN *****************/

void OGL_Shutdown(void)
{
	OGL_DisposeDrawContext();
}


/*********************** OGL: NEW VIEW DEF **********************/
//
// fills a view def structure with default values.
//

void OGL_NewViewDef(OGLSetupInputType *viewDef)
{
const OGLColorRGBA		clearColor = {0,0,0,1};
const OGLPoint3D			cameraFrom = { 0, 0, 0.0 };
const OGLPoint3D			cameraTo = { 0, 0, -1 };
const OGLVector3D			cameraUp = { 0.0, 1.0, 0.0 };
const OGLColorRGBA			ambientColor = { .3, .3, .3, 1 };
const OGLColorRGBA			fillColor = { 1.0, 1.0, 1.0, 1 };
static OGLVector3D			fillDirection1 = { 1, 0, -1 };
static OGLVector3D			fillDirection2 = { -1, -.3, -.3 };


	OGLVector3D_Normalize(&fillDirection1, &fillDirection1);
	OGLVector3D_Normalize(&fillDirection2, &fillDirection2);

	viewDef->view.clearColor 		= clearColor;
	viewDef->view.clip.left 	= 0;
	viewDef->view.clip.right 	= 0;
	viewDef->view.clip.top 		= 0;
	viewDef->view.clip.bottom 	= 0;
	viewDef->view.clearBackBuffer = true;

	viewDef->camera.from			= cameraFrom;
	viewDef->camera.to 				= cameraTo;
	viewDef->camera.up 				= cameraUp;
	viewDef->camera.hither 			= 10;
	viewDef->camera.yon 			= 4000;
	viewDef->camera.fov 			= 1.1;

	viewDef->styles.usePhong 		= false;

	viewDef->styles.useFog			= false;
	viewDef->styles.fogStart		= viewDef->camera.yon * .5f;
	viewDef->styles.fogEnd			= viewDef->camera.yon;
	viewDef->styles.fogDensity		= 1.0;
	viewDef->styles.fogMode			= GL_LINEAR;

	viewDef->styles.redFont			= false;

	viewDef->lights.ambientColor 	= ambientColor;
	viewDef->lights.numFillLights 	= 1;
	viewDef->lights.fillDirection[0] = fillDirection1;
	viewDef->lights.fillDirection[1] = fillDirection2;
	viewDef->lights.fillColor[0] 	= fillColor;
	viewDef->lights.fillColor[1] 	= fillColor;
}


/************** SETUP OGL WINDOW *******************/

void OGL_SetupWindow(OGLSetupInputType *setupDefPtr)
{
	GAME_ASSERT_MESSAGE(gGameViewInfoPtr == NULL, "gGameViewInfoPtr is already active");

	SDL_ShowCursor(0);	// do this just as a safety precaution to make sure no cursor lingering around

			/* ALLOC MEMORY FOR OUTPUT DATA */

	gGameViewInfoPtr = (OGLSetupOutputType *) AllocPtr(sizeof(OGLSetupOutputType));
	if (gGameViewInfoPtr == nil)
		DoFatalAlert("OGL_SetupWindow: AllocPtr failed");


				/* SETUP */

	OGL_InitDrawContext(&setupDefPtr->view);
	OGL_CheckError();

	OGL_SetStyles(setupDefPtr);
	OGL_CheckError();

	OGL_CreateLights(&setupDefPtr->lights);
	OGL_CheckError();

	SDL_GL_SetSwapInterval(gCommandLine.vsync);



				/* PASS BACK INFO */

//	gGameViewInfoPtr->drawContext 		= gAGLContext;
	gGameViewInfoPtr->clip 				= setupDefPtr->view.clip;
	gGameViewInfoPtr->hither 			= setupDefPtr->camera.hither;			// remember hither/yon
	gGameViewInfoPtr->yon 				= setupDefPtr->camera.yon;
	gGameViewInfoPtr->useFog 			= setupDefPtr->styles.useFog;
	gGameViewInfoPtr->clearBackBuffer 	= setupDefPtr->view.clearBackBuffer;

	gGameViewInfoPtr->isActive = true;											// it's now an active structure

	gGameViewInfoPtr->lightList = setupDefPtr->lights;							// copy lights

	gGameViewInfoPtr->fov = setupDefPtr->camera.fov;							// each camera will have its own fov so we can change it for special effects
	OGL_UpdateCameraFromTo(&setupDefPtr->camera.from, &setupDefPtr->camera.to);

	TextMesh_InitMaterial(setupDefPtr->styles.redFont);
	OGL_InitFont();
}


/***************** OGL_DisposeWindowSetup ***********************/
//
// Disposes of all data created by OGL_SetupWindow
//

void OGL_DisposeWindowSetup(void)
{
	GAME_ASSERT(gGameViewInfoPtr);						// see if this setup exists

			/* KILL FONT MATERIAL */

	TextMesh_DisposeMaterial();

		/* FREE MEMORY & NIL POINTER */

	gGameViewInfoPtr->isActive = false;						// now inactive
	SafeDisposePtr((Ptr) gGameViewInfoPtr);
	gGameViewInfoPtr = nil;
}



/**************** OGL: CREATE DRAW CONTEXT *********************/

static void OGL_CreateDrawContext(void)
{
	GAME_ASSERT_MESSAGE(!gAGLContext, "GL context already exists");
	GAME_ASSERT_MESSAGE(gSDLWindow, "Window must be created before the DC!");

			/* CREATE AGL CONTEXT & ATTACH TO WINDOW */

	gAGLContext = SDL_GL_CreateContext(gSDLWindow);
	GAME_ASSERT_MESSAGE(gAGLContext, SDL_GetError());
	GAME_ASSERT(glGetError() == GL_NO_ERROR);

			/* ACTIVATE CONTEXT */

	int mkc = SDL_GL_MakeCurrent(gSDLWindow, gAGLContext);
	GAME_ASSERT_MESSAGE(mkc == 0, SDL_GetError());


			/* GET OPENGL EXTENSIONS */
			//
			// On Mac/Linux, we only need to do this once.
			// But on Windows, we must do it whenever we create a draw context.
			//

	OGL_InitFunctions();


			/* SEE IF SUPPORT 1024x1024 TEXTURES */

	GLint maxTexSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
	if (maxTexSize < 1024)
		DoFatalAlert("Your video card cannot do 1024x1024 textures, so it is below the game's minimum system requirements.");
}


/**************** OGL: NUKE DRAW CONTEXT *********************/
//
// Do this when QUITTING the game!
// The game reuses the same draw context for all scenes!
//

static void OGL_DisposeDrawContext(void)
{
	if (!gAGLContext)
	{
		return;
	}

	SDL_GL_MakeCurrent(gSDLWindow, NULL);		// make context not current
	SDL_GL_DeleteContext(gAGLContext);			// nuke context
	gAGLContext = nil;
}


/**************** OGL: INIT DRAW CONTEXT *********************/

static void OGL_InitDrawContext(OGLViewDefType* viewDefPtr)
{
			/* FIX FOG FOR FOR B&W ANAGLYPH */
			//
			// The NTSC luminance standard where grayscale = .299r + .587g + .114b
			//

	if (gGamePrefs.anaglyph)
	{
		if (gGamePrefs.anaglyphColor)
		{
			uint32_t	r,g,b;

			r = viewDefPtr->clearColor.r * 255.0f;
			g = viewDefPtr->clearColor.g * 255.0f;
			b = viewDefPtr->clearColor.b * 255.0f;

			ColorBalanceRGBForAnaglyph(&r, &g, &b);

			viewDefPtr->clearColor.r = (float)r / 255.0f;
			viewDefPtr->clearColor.g = (float)g / 255.0f;
			viewDefPtr->clearColor.b = (float)b / 255.0f;

		}
		else
		{
			float	f;

			f = viewDefPtr->clearColor.r * .299;
			f += viewDefPtr->clearColor.g * .587;
			f += viewDefPtr->clearColor.b * .114;

			viewDefPtr->clearColor.r =
			viewDefPtr->clearColor.g =
			viewDefPtr->clearColor.b = f;
		}
	}


				/* SET VARIOUS STATE INFO */


	glEnable(GL_DEPTH_TEST);								// use z-buffer

	{
		GLfloat	color[] = {1,1,1,1};									// set global material color to white
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
	}

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

  	glEnable(GL_NORMALIZE);



				/* CLEAR BACK BUFFER ENTIRELY */

	glClearColor(0,0,0, 1.0);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(gSDLWindow);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(viewDefPtr->clearColor.r, viewDefPtr->clearColor.g, viewDefPtr->clearColor.b, 1.0);

}



/**************** OGL: SET STYLES ****************/

static void OGL_SetStyles(OGLSetupInputType *setupDefPtr)
{
OGLStyleDefType *styleDefPtr = &setupDefPtr->styles;


	glEnable(GL_CULL_FACE);									// activate culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);									// CCW is front face

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);		// set default blend func
	glDisable(GL_BLEND);									// but turn it off by default

//	glHint(GL_TRANSFORM_HINT_APPLE, GL_FASTEST);	// srcport rm

	glDisable(GL_RESCALE_NORMAL);

    glHint(GL_FOG_HINT, GL_NICEST);		// pixel accurate fog?

	OGL_CheckError();

			/* ENABLE ALPHA CHANNELS */

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_NOTEQUAL, 0);	// draw any pixel who's Alpha != 0


		/* SET FOG */

	glHint(GL_FOG_HINT, GL_FASTEST);

	if (styleDefPtr->useFog)
	{
		glFogi(GL_FOG_MODE, styleDefPtr->fogMode);
		glFogf(GL_FOG_DENSITY, styleDefPtr->fogDensity);
		glFogf(GL_FOG_START, styleDefPtr->fogStart);
		glFogf(GL_FOG_END, styleDefPtr->fogEnd);
		glFogfv(GL_FOG_COLOR, (float *)&setupDefPtr->view.clearColor);
		glEnable(GL_FOG);
	}
	else
		glDisable(GL_FOG);

	OGL_CheckError();
}




/********************* OGL: CREATE LIGHTS ************************/
//
// NOTE:  The Projection matrix must be the identity or lights will be transformed.
//

static void OGL_CreateLights(OGLLightDefType *lightDefPtr)
{
GLfloat	ambient[4];

	OGL_EnableLighting();


			/************************/
			/* CREATE AMBIENT LIGHT */
			/************************/

	ambient[0] = lightDefPtr->ambientColor.r;
	ambient[1] = lightDefPtr->ambientColor.g;
	ambient[2] = lightDefPtr->ambientColor.b;
	ambient[3] = 1;
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);			// set scene ambient light


			/**********************/
			/* CREATE FILL LIGHTS */
			/**********************/

	for (int i = 0; i < lightDefPtr->numFillLights; i++)
	{
		static GLfloat lightamb[4] = { 0.0, 0.0, 0.0, 1.0 };
		GLfloat lightVec[4];
		GLfloat	diffuse[4];

					/* SET FILL DIRECTION */

		OGLVector3D_Normalize(&lightDefPtr->fillDirection[i], &lightDefPtr->fillDirection[i]);
		lightVec[0] = -lightDefPtr->fillDirection[i].x;		// negate vector because OGL is stupid
		lightVec[1] = -lightDefPtr->fillDirection[i].y;
		lightVec[2] = -lightDefPtr->fillDirection[i].z;
		lightVec[3] = 0;									// when w==0, this is a directional light, if 1 then point light
		glLightfv(GL_LIGHT0+i, GL_POSITION, lightVec);


					/* SET COLOR */

		glLightfv(GL_LIGHT0+i, GL_AMBIENT, lightamb);

		diffuse[0] = lightDefPtr->fillColor[i].r;
		diffuse[1] = lightDefPtr->fillColor[i].g;
		diffuse[2] = lightDefPtr->fillColor[i].b;
		diffuse[3] = 1;

		glLightfv(GL_LIGHT0+i, GL_DIFFUSE, diffuse);


		glEnable(GL_LIGHT0+i);								// enable the light
	}




			/* KILL OTHER LIGHTS THAT MIGHT STILL BE ACTIVE FROM PREVIOUS SCENE */

	for (int i = lightDefPtr->numFillLights; i < MAX_FILL_LIGHTS; i++)
	{
		glDisable(GL_LIGHT0 + i);
	}
}

#pragma mark -

/******************* OGL DRAW SCENE *********************/

void OGL_DrawScene(void (*drawRoutine)(void))
{
	GAME_ASSERT(gGameViewInfoPtr);						// make sure it's legit
	GAME_ASSERT(gGameViewInfoPtr->isActive);

	int makeCurrentRC = SDL_GL_MakeCurrent(gSDLWindow, gAGLContext);		// make context active
	GAME_ASSERT_MESSAGE(makeCurrentRC == 0, SDL_GetError());


	if (gGammaFadeFrac <= 0)							// if we just finished fading out and haven't started fading in yet, just show black
	{
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		SDL_GL_SwapWindow(gSDLWindow);					// end render loop
		return;
	}


			/* INIT SOME STUFF */

	if (gGamePrefs.anaglyph)
	{
		gAnaglyphPass = 0;
		PrepAnaglyphCameras();
	}


	if (gDebugMode)
	{
		int depth = 32;
		gVRAMUsedThisFrame = gGameWindowWidth * gGameWindowHeight * (depth / 8);	// backbuffer size
		gVRAMUsedThisFrame += gGameWindowWidth * gGameWindowHeight * 2;				// z-buffer size
		gVRAMUsedThisFrame += gGameWindowWidth * gGameWindowHeight * (depth / 8);	// display size
	}


	gPolysThisFrame 	= 0;										// init poly counter
	gMostRecentMaterial = nil;
	gGlobalMaterialFlags = 0;
	SetColor4f(1,1,1,1);

				/*****************/
				/* CLEAR BUFFERS */
				/*****************/

				/* MAKE SURE GREEN CHANNEL IS CLEAR */
				//
				// Bringing up dialogs can write into green channel, so always be sure it's clear
				//

	if (gGameViewInfoPtr->clearBackBuffer || (gDebugMode == 3))
	{
		if (gGamePrefs.anaglyph)
		{
			if (gGamePrefs.anaglyphColor)
				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);		// make sure clearing Red/Green/Blue channels
			else
				glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE);		// make sure clearing Red/Blue channels
		}
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}
	else
		glClear(GL_DEPTH_BUFFER_BIT);


			/*************************/
			/* SEE IF DOING ANAGLYPH */
			/*************************/

do_anaglyph:

	if (gGamePrefs.anaglyph)
	{
				/* SET COLOR MASK */

		if (gAnaglyphPass == 0)
		{
			glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
		}
		else
		{
			if (gGamePrefs.anaglyphColor)
				glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);
			else
				glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
			glClear(GL_DEPTH_BUFFER_BIT);
		}

		CalcAnaglyphCameraOffset(gAnaglyphPass);
	}
	else
	{
		gAnaglyphPass = 0;
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);		// this lets us hot-switch between anaglyph and non-anaglyph in the settings
	}


				/* SET VIEWPORT */

	{
		int x, y, w, h;
		OGL_GetCurrentViewport(&x, &y, &w, &h);
		glViewport(x, y, w, h);
		gCurrentAspectRatio = (float) w / (float) (h == 0? 1: h);

		// Compute logical width & height for 2D elements
		g2DLogicalHeight = 480.0f;
		if (gCurrentAspectRatio < 4.0f/3.0f)
			g2DLogicalWidth = 640.0f;
		else
			g2DLogicalWidth = 480.0f * gCurrentAspectRatio;
	}


			/* GET UPDATED GLOBAL COPIES OF THE VARIOUS MATRICES */

	OGL_Camera_SetPlacementAndUpdateMatrices();


			/* CALL INPUT DRAW FUNCTION */

	if (drawRoutine != nil)
		drawRoutine();


			/***********************************/
			/* SEE IF DO ANOTHER ANAGLYPH PASS */
			/***********************************/

	if (gGamePrefs.anaglyph)
	{
		gAnaglyphPass++;
		if (gAnaglyphPass == 1)
			goto do_anaglyph;
	}


		/**************************/
		/* SEE IF SHOW DEBUG INFO */
		/**************************/

	if (GetNewKeyState(SDL_SCANCODE_F8))
	{
		if (++gDebugMode > 3)
			gDebugMode = 0;

		if (gDebugMode == 3)								// see if show wireframe
			glPolygonMode(GL_FRONT_AND_BACK ,GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK ,GL_FILL);
	}

				/* SHOW BASIC DEBUG INFO */

	if (!gDebugText)
	{
		// no-op
	}
	else if (gDebugMode > 0)
	{
		char debugString[1024];
		snprintf(
			debugString,
			sizeof(debugString),
			"fps:\t\t%d\n"
			"tris:\t\t%d\n"
			"\n"
			"input x:\t%.3f\n"
			"input y:\t%.3f\n"
			"input a:\t%.0f\u00b0\n"
			"\n"
			"player x:\t%.0f\n"
			"player z:\t%.0f\n"
			"\n"
			"enemies:\t%d\n"
			"t-defs:\t%d\n"
			"sparkles:\t%d\n"
			"h2o:\t\t%d\n"
			"ground?\t%c\n"
			"\n"
			"vram:\t\t%dK\n"
#if 0
			"ptrs:\t\t%d\n"
			"ptr mem:\t%ldK\n"
#endif
			"nodes:\t%d\n"
			"\n"
			"time since last thrust:\t%.3f\n"
			"force cam align?\t\t%c\n"
			"auto rotate cam?\t\t%c\n"
			"cam user rot:\t\t%.3f\n"
			"cam ctrl dX:\t\t%.3f\n"
			,
			(int)(gFramesPerSecond+.5f),
			gPolysThisFrame,
			gPlayerInfo.analogControlX,
			gPlayerInfo.analogControlZ,
			(180/PI) * ( atan2f(gPlayerInfo.analogControlZ,gPlayerInfo.analogControlX) ),
			gPlayerInfo.coord.x,
			gPlayerInfo.coord.z,
			gNumEnemies,
			gNumTerrainDeformations,
			gNumSparkles,
			gNumWaterDrawn,
			gPlayerInfo.objNode && (gPlayerInfo.objNode->StatusBits & STATUS_BIT_ONGROUND)? 'Y': 'N',
			gVRAMUsedThisFrame/1024,
#if 0
			gNumPointers,
			gMemAllocatedInPtrs/1024,
#endif
			gNumObjectNodes,
			gTimeSinceLastThrust,
			gForceCameraAlignment? 'Y': 'N',
			gAutoRotateCamera? 'Y': 'N',
			gCameraUserRotY,
			gCameraControlDelta.x
		);
		TextMesh_Update(debugString, 0, gDebugText);
		gDebugText->StatusBits &= ~STATUS_BIT_HIDDEN;
	}
	else
	{
		gDebugText->StatusBits |= STATUS_BIT_HIDDEN;
	}



            /**************/
			/* END RENDER */
			/**************/

           /* SWAP THE BUFFS */

	SDL_GL_SwapWindow(gSDLWindow);					// end render loop


	if (gGamePrefs.anaglyph)
		RestoreCamerasFromAnaglyph();

}


/********************** OGL: GET CURRENT VIEWPORT ********************/
//
// Remember that with OpenGL, the bottom of the screen is y==0, so some of this code
// may look upside down.
//

void OGL_GetCurrentViewport(int *x, int *y, int *w, int *h)
{
int	t,b,l,r;

	SDL_GL_GetDrawableSize(gSDLWindow, &gGameWindowWidth, &gGameWindowHeight);

	t = gGameViewInfoPtr->clip.top;
	b = gGameViewInfoPtr->clip.bottom;
	l = gGameViewInfoPtr->clip.left;
	r = gGameViewInfoPtr->clip.right;

	*x = l;
	*y = t;
	*w = gGameWindowWidth-l-r;
	*h = gGameWindowHeight-t-b;
}


#pragma mark -


/***************** OGL TEXTUREMAP LOAD **************************/

GLuint OGL_TextureMap_Load(void *imageMemory, int width, int height,
							GLint srcFormat,  GLint destFormat, GLint dataType)
{
GLuint	textureName;


	if (gGamePrefs.anaglyph)
	{
		if (gGamePrefs.anaglyphColor)
			ConvertTextureToColorAnaglyph(imageMemory, width, height, srcFormat, dataType);
		else
			ConvertTextureToGrey(imageMemory, width, height, srcFormat, dataType);
	}

			/* GET A UNIQUE TEXTURE NAME & INITIALIZE IT */

	glGenTextures(1, &textureName);
	if (OGL_CheckError())
		DoFatalAlert("OGL_TextureMap_Load: glGenTextures failed!");

	glBindTexture(GL_TEXTURE_2D, textureName);				// this is now the currently active texture
	if (OGL_CheckError())
		DoFatalAlert("OGL_TextureMap_Load: glBindTexture failed!");

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D,
				0,										// mipmap level
				destFormat,								// format in OpenGL
				width,									// width in pixels
				height,									// height in pixels
				0,										// border
				srcFormat,								// what my format is
				dataType,								// size of each r,g,b
				imageMemory);							// pointer to the actual texture pixels

			/* SEE IF RAN OUT OF MEMORY WHILE COPYING TO OPENGL */

	if (OGL_CheckError())
		DoFatalAlert("OGL_TextureMap_Load: glTexImage2D failed!");


				/* SET THIS TEXTURE AS CURRENTLY ACTIVE FOR DRAWING */

	OGL_Texture_SetOpenGLTexture(textureName);

	return(textureName);
}

/***************** OGL TEXTUREMAP LOAD FROM TGA **********************/

GLuint OGL_TextureMap_LoadTGA(const char* path, int flags, int* outWidth, int* outHeight)
{
FSSpec					spec;
uint8_t*				pixelData = nil;
TGAHeader				header;
OSErr					err;

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, path, &spec);

			/* LOAD RAW ARGB DATA FROM TGA FILE */

	err = ReadTGA(&spec, &pixelData, &header, true);
	GAME_ASSERT(err == noErr);

	GAME_ASSERT(header.bpp == 32);
	GAME_ASSERT(header.imageType == TGA_IMAGETYPE_CONVERTED_ARGB);

			/* PRE-PROCESS IMAGE */

	int internalFormat = GL_RGB;

	if (flags & kLoadTextureFlags_GrayscaleIsAlpha)
	{
		for (int p = 0; p < 4 * header.width * header.height; p += 4)
		{
			// put Blue into Alpha & leave map white
			pixelData[p+0] = pixelData[p+3];	// put blue into alpha
			pixelData[p+1] = 255;
			pixelData[p+2] = 255;
			pixelData[p+3] = 255;
		}
		internalFormat = GL_RGBA;
	}
	else if (flags & kLoadTextureFlags_KeepOriginalAlpha)
	{
		internalFormat = GL_RGBA;
	}
	else
	{
		internalFormat = GL_RGB;
	}

			/* LOAD TEXTURE */

	GLuint glTextureName = OGL_TextureMap_Load(
			pixelData,
			header.width,
			header.height,
			GL_BGRA,
			internalFormat,
			GL_UNSIGNED_INT_8_8_8_8
			);

			/* CLEAN UP */

	DisposePtr((Ptr) pixelData);

	if (outWidth)
		*outWidth = header.width;
	if (outHeight)
		*outHeight = header.height;

	return glTextureName;
}

/******************** CONVERT TEXTURE TO GREY **********************/
//
// The NTSC luminance standard where grayscale = .299r + .587g + .114b
//


static void	ConvertTextureToGrey(void *imageMemory, short width, short height, GLint srcFormat, GLint dataType)
{
long	x,y;
float	r,g,b;
uint32_t	a,q,rq,bq;
uint32_t   redCal = gGamePrefs.anaglyphCalibrationRed;
uint32_t   blueCal =  gGamePrefs.anaglyphCalibrationBlue;


	if (dataType == GL_UNSIGNED_INT_8_8_8_8_REV)
	{
		uint32_t	*pix32 = (uint32_t *)imageMemory;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				uint32_t	pix = pix32[x];

				r = (float)((pix >> 16) & 0xff) / 255.0f * .299f;
				g = (float)((pix >> 8) & 0xff) / 255.0f * .586f;
				b = (float)(pix & 0xff) / 255.0f * .114f;
				a = (pix >> 24) & 0xff;


				q = (r + g + b) * 255.0f;									// pass thru the brightness curve
				if (q > 0xff)
					q = 0xff;
				q = gAnaglyphGreyTable[q];

				rq = (q * redCal) / 0xff;									// balance the red & blue
				bq = (q * blueCal) / 0xff;

				pix = (a << 24) | (rq << 16) | (q << 8) | bq;
				pix32[x] = pix;
			}
			pix32 += width;
		}
	}

	else
	if ((dataType == GL_UNSIGNED_BYTE) && (srcFormat == GL_RGBA))
	{
		uint32_t	*pix32 = (uint32_t *)imageMemory;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				uint32_t	pix = SwizzleULong(&pix32[x]);

				r = (float)((pix >> 24) & 0xff) / 255.0f * .299f;
				g = (float)((pix >> 16) & 0xff) / 255.0f * .586f;
				b = (float)((pix >> 8)  & 0xff) / 255.0f * .114f;
				a = pix & 0xff;

				q = (r + g + b) * 255.0f;									// pass thru the brightness curve
				if (q > 0xff)
					q = 0xff;
				q = gAnaglyphGreyTable[q];

				rq = (q * redCal) / 0xff;									// balance the red & blue
				bq = (q * blueCal) / 0xff;

				pix = (rq << 24) | (q << 16) | (bq << 8) | a;
				pix32[x] = SwizzleULong(&pix);

			}
			pix32 += width;
		}
	}
	else
	if (dataType == GL_UNSIGNED_SHORT_1_5_5_5_REV)
	{
		uint16_t	*pix16 = (uint16_t *)imageMemory;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				uint16_t	pix = pix16[x]; //SwizzleUShort(&pix16[x]);

				r = (float)((pix >> 10) & 0x1f) / 31.0f * .299f;
				g = (float)((pix >> 5) & 0x1f) / 31.0f * .586f;
				b = (float)(pix & 0x1f) / 31.0f * .114f;
				a = pix & 0x8000;

				q = (r + g + b) * 255.0f;								// pass thru the brightness curve
				if (q > 0xff)
					q = 0xff;
				q = gAnaglyphGreyTable[q];

				rq = (q * redCal) / 0xff;									// balance the red & blue
				bq = (q * blueCal) / 0xff;

				q = (float)q / 8.0f;
				if (q > 0x1f)
					q = 0x1f;

				rq = (float)rq / 8.0f;
				if (rq > 0x1f)
					rq = 0x1f;
				bq = (float)bq / 8.0f;
				if (bq > 0x1f)
					bq = 0x1f;

				pix = a | (rq << 10) | (q << 5) | bq;
				pix16[x] = pix; //SwizzleUShort(&pix);

			}
			pix16 += width;
		}
	}
}


/******************* COLOR BALANCE RGB FOR ANAGLYPH *********************/

void ColorBalanceRGBForAnaglyph(uint32_t *rr, uint32_t *gg, uint32_t *bb)
{
uint32_t	r,g,b;
float	d;
float   lumR, lumGB, ratio;
const Boolean	allowChannelBalancing = true;

	r = *rr;
	g = *gg;
	b = *bb;


				/* ADJUST FOR USER CALIBRATION */

	r = r * gGamePrefs.anaglyphCalibrationRed / 255;
	b = b * gGamePrefs.anaglyphCalibrationBlue / 255;
	g = g * gGamePrefs.anaglyphCalibrationGreen / 255;


				/* DO LUMINOSITY CHANNEL BALANCING */

	if (allowChannelBalancing && gGamePrefs.doAnaglyphChannelBalancing)
	{
		float   fr, fg, fb;

		fr = r;
		fg = g;
		fb = b;

		lumR = fr * .299f;
		lumGB = fg * .587f + fb * .114f;

		lumR += 1.0f;
		lumGB += 1.0f;


			/* BALANCE BLUE */

		ratio = lumR / lumGB;
		ratio *= 1.5f;
		d = fb * ratio;
		if (d > fb)
		{
			b = d;
			if (b > 0xff)
				b = 0xff;
		}

			/* SMALL BALANCE ON GREEN */

		ratio *= .8f;
		d = fg * ratio;
		if (d > fg)
		{
			g = d;
			if (g > 0xff)
				g = 0xff;
		}

			/* BALANCE RED */

		ratio = lumGB / lumR;
		ratio *= .4f;
		d = fr * ratio;
		if (d > fr)
		{
			r = d;
			if (r > 0xff)
				r = 0xff;
		}

	}



	*rr = r;
	*gg = g;
	*bb = b;
}




/******************** CONVERT TEXTURE TO COLOR ANAGLYPH **********************/


static void	ConvertTextureToColorAnaglyph(void *imageMemory, short width, short height, GLint srcFormat, GLint dataType)
{
long	x,y;
uint32_t	r,g,b;
uint32_t	a;

	if (dataType == GL_UNSIGNED_INT_8_8_8_8_REV)
	{
		uint32_t	*pix32 = (uint32_t *)imageMemory;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				a = ((pix32[x] >> 24) & 0xff);
				r = ((pix32[x] >> 16) & 0xff);
				g = ((pix32[x] >> 8) & 0xff);
				b = ((pix32[x] >> 0) & 0xff);

				ColorBalanceRGBForAnaglyph(&r, &g, &b);

				pix32[x] = (a << 24) | (r << 16) | (g << 8) | b;

			}
			pix32 += width;
		}
	}
	else
	if ((dataType == GL_UNSIGNED_BYTE) && (srcFormat == GL_RGBA))
	{
		uint32_t	*pix32 = (uint32_t *)imageMemory;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				uint32_t	pix = SwizzleULong(&pix32[x]);

				a = ((pix >> 0) & 0xff);
				r = ((pix >> 24) & 0xff);
				g = ((pix >> 16) & 0xff);
				b = ((pix >> 8) & 0xff);

				ColorBalanceRGBForAnaglyph(&r, &g, &b);

				pix = (r << 24) | (g << 16) | (b << 8) | a;
				pix32[x] = SwizzleULong(&pix);
			}
			pix32 += width;
		}
	}
	else
	if (dataType == GL_UNSIGNED_SHORT_1_5_5_5_REV)
	{
		u_short	*pix16 = (u_short *)imageMemory;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				r = ((pix16[x] >> 10) & 0x1f) << 3;			// load 5 bits per channel & convert to 8 bits
				g = ((pix16[x] >> 5) & 0x1f) << 3;
				b = (pix16[x] & 0x1f) << 3;
				a = pix16[x] & 0x8000;

				ColorBalanceRGBForAnaglyph(&r, &g, &b);

				r >>= 3;
				g >>= 3;
				b >>= 3;

				pix16[x] = a | (r << 10) | (g << 5) | b;

			}
			pix16 += width;
		}
	}

}


/****************** OGL: TEXTURE SET OPENGL TEXTURE **************************/
//
// Sets the current OpenGL texture using glBindTexture et.al. so any textured triangles will use it.
//

void OGL_Texture_SetOpenGLTexture(GLuint textureName)
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if (OGL_CheckError())
		DoFatalAlert("OGL_Texture_SetOpenGLTexture: glPixelStorei failed!");

	glBindTexture(GL_TEXTURE_2D, textureName);
	if (OGL_CheckError())
		DoFatalAlert("OGL_Texture_SetOpenGLTexture: glBindTexture failed!");


	glGetError();

	glEnable(GL_TEXTURE_2D);
}



#pragma mark -

/*************** OGL_MoveCameraFromTo ***************/

void OGL_MoveCameraFromTo(float fromDX, float fromDY, float fromDZ, float toDX, float toDY, float toDZ)
{

			/* SET CAMERA COORDS */

	gGameViewInfoPtr->cameraPlacement.cameraLocation.x += fromDX;
	gGameViewInfoPtr->cameraPlacement.cameraLocation.y += fromDY;
	gGameViewInfoPtr->cameraPlacement.cameraLocation.z += fromDZ;

	gGameViewInfoPtr->cameraPlacement.pointOfInterest.x += toDX;
	gGameViewInfoPtr->cameraPlacement.pointOfInterest.y += toDY;
	gGameViewInfoPtr->cameraPlacement.pointOfInterest.z += toDZ;

	UpdateListenerLocation();
}


/*************** OGL_MoveCameraFrom ***************/

void OGL_MoveCameraFrom(float fromDX, float fromDY, float fromDZ)
{

			/* SET CAMERA COORDS */

	gGameViewInfoPtr->cameraPlacement.cameraLocation.x += fromDX;
	gGameViewInfoPtr->cameraPlacement.cameraLocation.y += fromDY;
	gGameViewInfoPtr->cameraPlacement.cameraLocation.z += fromDZ;

	UpdateListenerLocation();
}



/*************** OGL_UpdateCameraFromTo ***************/

void OGL_UpdateCameraFromTo(const OGLPoint3D *from, const OGLPoint3D *to)
{
static const OGLVector3D up = {0,1,0};

	gGameViewInfoPtr->cameraPlacement.upVector			= up;
	gGameViewInfoPtr->cameraPlacement.cameraLocation 	= *from;
	gGameViewInfoPtr->cameraPlacement.pointOfInterest 	= *to;

	UpdateListenerLocation();
}

/*************** OGL_UpdateCameraFromToUp ***************/

void OGL_UpdateCameraFromToUp(OGLPoint3D *from, OGLPoint3D *to, OGLVector3D *up)
{
	gGameViewInfoPtr->cameraPlacement.upVector			= *up;
	gGameViewInfoPtr->cameraPlacement.cameraLocation	= *from;
	gGameViewInfoPtr->cameraPlacement.pointOfInterest	= *to;

	UpdateListenerLocation();
}



/************** OGL: CAMERA SET PLACEMENT & UPDATE MATRICES **********************/
//
// This is called by OGL_DrawScene to initialize all of the view matrices,
// and to extract the current view matrices used for culling et.al.
//

void OGL_Camera_SetPlacementAndUpdateMatrices(void)
{
float	aspect;
int		temp, w, h, i;
OGLLightDefType	*lights;

	OGL_GetCurrentViewport(&temp, &temp, &w, &h);
	aspect = (float)w/(float)h;

			/* INIT PROJECTION MATRIX */

	glMatrixMode(GL_PROJECTION);

			/* SETUP FOR ANAGLYPH STEREO 3D CAMERA */

	if (gGamePrefs.anaglyph)
	{
		float	left, right;
		float	halfFOV = gGameViewInfoPtr->fov * .5f;
		float	znear 	= gGameViewInfoPtr->hither;
	   	float	wd2     = znear * tan(halfFOV);
		float	ndfl    = znear / gAnaglyphFocallength;

		if (gAnaglyphPass == 0)
		{
			left  = - aspect * wd2 + 0.5f * gAnaglyphEyeSeparation * ndfl;
			right =   aspect * wd2 + 0.5f * gAnaglyphEyeSeparation * ndfl;
		}
		else
		{
			left  = - aspect * wd2 - 0.5f * gAnaglyphEyeSeparation * ndfl;
			right =   aspect * wd2 - 0.5f * gAnaglyphEyeSeparation * ndfl;
		}

		glLoadIdentity();
		glFrustum(left, right, -wd2, wd2, gGameViewInfoPtr->hither, gGameViewInfoPtr->yon);
		glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat*) &gViewToFrustumMatrix.value[0]);
	}

			/* SETUP STANDARD PERSPECTIVE CAMERA */
	else
	{
		OGL_SetGluPerspectiveMatrix(
				&gViewToFrustumMatrix,
				gGameViewInfoPtr->fov,
				aspect,
				gGameViewInfoPtr->hither,
				gGameViewInfoPtr->yon);
		glLoadMatrixf((const GLfloat*) &gViewToFrustumMatrix.value[0]);
	}



			/* INIT MODELVIEW MATRIX */

	glMatrixMode(GL_MODELVIEW);
	OGL_SetGluLookAtMatrix(
			&gWorldToViewMatrix,
			&gGameViewInfoPtr->cameraPlacement.cameraLocation,
			&gGameViewInfoPtr->cameraPlacement.pointOfInterest,
			&gGameViewInfoPtr->cameraPlacement.upVector);
	glLoadMatrixf((const GLfloat*) &gWorldToViewMatrix.value[0]);



		/* UPDATE LIGHT POSITIONS */

	lights =  &gGameViewInfoPtr->lightList;						// point to light list
	for (i=0; i < lights->numFillLights; i++)
	{
		GLfloat lightVec[4];

		lightVec[0] = -lights->fillDirection[i].x;			// negate vector because OGL is stupid
		lightVec[1] = -lights->fillDirection[i].y;
		lightVec[2] = -lights->fillDirection[i].z;
		lightVec[3] = 0;									// when w==0, this is a directional light, if 1 then point light
		glLightfv(GL_LIGHT0+i, GL_POSITION, lightVec);
	}


			/* GET VARIOUS CAMERA MATRICES */

	OGLMatrix4x4_Multiply(&gWorldToViewMatrix, &gViewToFrustumMatrix, &gWorldToFrustumMatrix);

	OGLMatrix4x4_GetFrustumToWindow(&gFrustumToWindowMatrix);
	OGLMatrix4x4_Multiply(&gWorldToFrustumMatrix, &gFrustumToWindowMatrix, &gWorldToWindowMatrix);

	UpdateListenerLocation();
}



#pragma mark -


/******************** OGL: CHECK ERROR ********************/

GLenum _OGL_CheckError(const char* file, const int line)
{
	GLenum error = glGetError();
	if (error != 0)
	{
		static char buf[256];
		snprintf(buf, 256, "OpenGL Error 0x%x in %s:%d", error, file, line);
		DoFatalAlert(buf);
	}
	return error;
}



#pragma mark -


/********************* PUSH STATE **************************/

void OGL_PushState(void)
{
int	i;

		/* PUSH MATRIES WITH OPENGL */

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glMatrixMode(GL_MODELVIEW);										// in my code, I keep modelview matrix as the currently active one all the time.


		/* SAVE OTHER INFO */

	i = gStateStackIndex++;											// get stack index and increment

	if (i >= STATE_STACK_SIZE)
		DoFatalAlert("OGL_PushState: stack overflow");

	gStateStack_Lighting[i] = gMyState_Lighting;
	gStateStack_CullFace[i] = glIsEnabled(GL_CULL_FACE);
	gStateStack_DepthTest[i] = glIsEnabled(GL_DEPTH_TEST);
	gStateStack_Normalize[i] = glIsEnabled(GL_NORMALIZE);
	gStateStack_Texture2D[i] = glIsEnabled(GL_TEXTURE_2D);
	gStateStack_Fog[i] 		= glIsEnabled(GL_FOG);
	gStateStack_Blend[i] 	= glIsEnabled(GL_BLEND);

	glGetFloatv(GL_CURRENT_COLOR, &gStateStack_Color[i][0]);

	glGetIntegerv(GL_BLEND_SRC, &gStateStack_BlendSrc[i]);
	glGetIntegerv(GL_BLEND_DST, &gStateStack_BlendDst[i]);
	glGetBooleanv(GL_DEPTH_WRITEMASK, &gStateStack_DepthMask[i]);
}


/********************* POP STATE **************************/

void OGL_PopState(void)
{
int		i;

		/* RETREIVE OPENGL MATRICES */

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

		/* GET OTHER INFO */

	i = --gStateStackIndex;												// dec stack index

	if (i < 0)
		DoFatalAlert("OGL_PopState: stack underflow!");

	if (gStateStack_Lighting[i])
		OGL_EnableLighting();
	else
		OGL_DisableLighting();


	if (gStateStack_CullFace[i])
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);


	if (gStateStack_DepthTest[i])
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	if (gStateStack_Normalize[i])
		glEnable(GL_NORMALIZE);
	else
		glDisable(GL_NORMALIZE);

	if (gStateStack_Texture2D[i])
		glEnable(GL_TEXTURE_2D);
	else
		glDisable(GL_TEXTURE_2D);

	if (gStateStack_Blend[i])
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);

	if (gStateStack_Fog[i])
		glEnable(GL_FOG);
	else
		glDisable(GL_FOG);

	glDepthMask(gStateStack_DepthMask[i]);
	glBlendFunc(gStateStack_BlendSrc[i], gStateStack_BlendDst[i]);

	glColor4fv(&gStateStack_Color[i][0]);

}


/******************* OGL ENABLE LIGHTING ****************************/

void OGL_EnableLighting(void)
{
	gMyState_Lighting = true;
	glEnable(GL_LIGHTING);
}

/******************* OGL DISABLE LIGHTING ****************************/

void OGL_DisableLighting(void)
{
	gMyState_Lighting = false;
	glDisable(GL_LIGHTING);
}


#pragma mark -

/************************** OGL_INIT FONT **************************/

static void OGL_InitFont(void)
{
	NewObjectDefinitionType newObjDef;
	memset(&newObjDef, 0, sizeof(newObjDef));
	newObjDef.flags = STATUS_BIT_HIDDEN;
	newObjDef.slot = DEBUGOVERLAY_SLOT;
	newObjDef.scale = 0.45f;
	newObjDef.coord = (OGLPoint3D) { -320, -100, 0 };
	gDebugText = TextMesh_NewEmpty(2048, &newObjDef);
}
