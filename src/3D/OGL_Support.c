/****************************/
/*   OPENGL SUPPORT.C	    */
/* (c)2001 Pangea Software  */
/*   By Brian Greenstone    */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"

//#include <AGL/aglmacro.h> // srcport rm


extern int				gNumObjectNodes,gNumPointers;
extern	MOMaterialObject	*gMostRecentMaterial;
extern	short			gNumSuperTilesDrawn,gNumActiveParticleGroups,gNumFencesDrawn,gNumTerrainDeformations,gNumWaterDrawn,gNumEnemies;
extern	PlayerInfoType	gPlayerInfo;
extern	float			gFramesPerSecond,gCameraStartupTimer,gScratchF;
extern	Byte			gDebugMode;
extern	Boolean			gOSX;
extern	u_long			gGlobalMaterialFlags;
extern	PrefsType			gGamePrefs;
extern	int				gGameWindowWidth,gGameWindowHeight,gScratch,gNumSparkles;
extern	CGrafPtr				gDisplayContextGrafPtr;

/****************************/
/*    PROTOTYPES            */
/****************************/


static void OGL_CreateDrawContext(OGLViewDefType *viewDefPtr);
static void OGL_SetStyles(OGLSetupInputType *setupDefPtr);
static void OGL_CreateLights(OGLLightDefType *lightDefPtr);
static void OGL_InitFont(void);
static void OGL_FreeFont(void);

static void ColorBalanceRGBForAnaglyph(u_long *rr, u_long *gg, u_long *bb);
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


AGLDrawable		gAGLWin;
AGLContext		gAGLContext = nil;

static GLuint 			gFontList;


OGLMatrix4x4	gViewToFrustumMatrix,gWorldToViewMatrix,gWorldToFrustumMatrix;
OGLMatrix4x4	gWorldToWindowMatrix,gFrustumToWindowMatrix;

float	gCurrentAspectRatio = 1;


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
static int			gMinRAM = 100000000;

Boolean		gMyState_Lighting;


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

	viewDef->lights.ambientColor 	= ambientColor;
	viewDef->lights.numFillLights 	= 1;
	viewDef->lights.fillDirection[0] = fillDirection1;
	viewDef->lights.fillDirection[1] = fillDirection2;
	viewDef->lights.fillColor[0] 	= fillColor;
	viewDef->lights.fillColor[1] 	= fillColor;
}


/************** SETUP OGL WINDOW *******************/

void OGL_SetupWindow(OGLSetupInputType *setupDefPtr, OGLSetupOutputType **outputHandle)
{
OGLSetupOutputType	*outputPtr;

	HideCursor();		// do this just as a safety precaution to make sure no cursor lingering around

			/* ALLOC MEMORY FOR OUTPUT DATA */

	outputPtr = (OGLSetupOutputType *)AllocPtr(sizeof(OGLSetupOutputType));
	if (outputPtr == nil)
		DoFatalAlert("OGL_SetupWindow: AllocPtr failed");


				/* SETUP */

	OGL_CreateDrawContext(&setupDefPtr->view);
	OGL_SetStyles(setupDefPtr);
	OGL_CreateLights(&setupDefPtr->lights);


				/* PASS BACK INFO */

	outputPtr->drawContext 		= gAGLContext;
	outputPtr->clip 			= setupDefPtr->view.clip;
	outputPtr->hither 			= setupDefPtr->camera.hither;			// remember hither/yon
	outputPtr->yon 				= setupDefPtr->camera.yon;
	outputPtr->useFog 			= setupDefPtr->styles.useFog;
	outputPtr->clearBackBuffer 	= setupDefPtr->view.clearBackBuffer;

	outputPtr->isActive = true;											// it's now an active structure

	outputPtr->lightList = setupDefPtr->lights;							// copy lights

	outputPtr->fov = setupDefPtr->camera.fov;					// each camera will have its own fov so we can change it for special effects
	OGL_UpdateCameraFromTo(outputPtr, &setupDefPtr->camera.from, &setupDefPtr->camera.to);

	*outputHandle = outputPtr;											// return value to caller
}


/***************** OGL_DisposeWindowSetup ***********************/
//
// Disposes of all data created by OGL_SetupWindow
//

void OGL_DisposeWindowSetup(OGLSetupOutputType **dataHandle)
{
OGLSetupOutputType	*data;

	data = *dataHandle;
	if (data == nil)												// see if this setup exists
		DoFatalAlert("OGL_DisposeWindowSetup: data == nil");

			/* KILL DEBUG FONT */

	OGL_FreeFont();

  	aglSetCurrentContext(nil);								// make context not current
   	aglSetDrawable(data->drawContext, nil);
	aglDestroyContext(data->drawContext);					// nuke the AGL context


		/* FREE MEMORY & NIL POINTER */

	data->isActive = false;									// now inactive
	SafeDisposePtr((Ptr)data);
	*dataHandle = nil;

	gAGLContext = nil;
}




/**************** OGL: CREATE DRAW CONTEXT *********************/

static void OGL_CreateDrawContext(OGLViewDefType *viewDefPtr)
{
AGLPixelFormat 	fmt;
GLboolean      mkc, ok;
GLint          attribWindow[]	= {AGL_RGBA, AGL_DOUBLEBUFFER, AGL_DEPTH_SIZE, 32, AGL_ALL_RENDERERS, AGL_ACCELERATED, AGL_NO_RECOVERY, AGL_NONE};
GLint          attrib32bit[] 	= {AGL_RGBA, AGL_FULLSCREEN, AGL_DOUBLEBUFFER, AGL_DEPTH_SIZE, 32, AGL_ALL_RENDERERS, AGL_ACCELERATED, AGL_NO_RECOVERY, AGL_NONE};
GLint          attrib16bit[] 	= {AGL_RGBA, AGL_FULLSCREEN, AGL_DOUBLEBUFFER, AGL_DEPTH_SIZE, 32, AGL_ALL_RENDERERS, AGL_ACCELERATED, AGL_NO_RECOVERY, AGL_NONE};
GLint          attrib2[] 		= {AGL_RGBA, AGL_FULLSCREEN, AGL_DOUBLEBUFFER, AGL_DEPTH_SIZE, 16, AGL_ALL_RENDERERS, AGL_NONE};
AGLContext agl_ctx;
GLint			maxTexSize;
static char			*s;

			/* FIX FOG FOR FOR B&W ANAGLYPH */
			//
			// The NTSC luminance standard where grayscale = .299r + .587g + .114b
			//

	if (gGamePrefs.anaglyph)
	{
		if (gGamePrefs.anaglyphColor)
		{
			u_long	r,g,b;

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

			/***********************/
			/* CHOOSE PIXEL FORMAT */
			/***********************/

			/* PLAY IN WINDOW */

	if (!gPlayFullScreen)
	{
		fmt = aglChoosePixelFormat(&gGDevice, 1, attribWindow);
	}

			/* FULL-SCREEN */
	else
	{
		if (gGamePrefs.depth == 32)
			fmt = aglChoosePixelFormat(&gGDevice, 1, attrib32bit);						// 32-bit display
		else
			fmt = aglChoosePixelFormat(&gGDevice, 1, attrib16bit);						// 16-bit display
	}

			/* BACKUP PLAN IF ERROR */

	if ((fmt == NULL) || (aglGetError() != AGL_NO_ERROR))
	{
		fmt = aglChoosePixelFormat(&gGDevice, 1, attrib2);							// try being less stringent
		if ((fmt == NULL) || (aglGetError() != AGL_NO_ERROR))
		{
			DoFatalAlert("aglChoosePixelFormat failed!  OpenGL could not initialize your video card for 3D.  Check that your video card meets the game's minimum system requirements.");
		}
	}


			/* CREATE AGL CONTEXT & ATTACH TO WINDOW */

	gAGLContext = aglCreateContext(fmt, nil);
	if ((gAGLContext == nil) || (aglGetError() != AGL_NO_ERROR))
		DoFatalAlert("OGL_CreateDrawContext: aglCreateContext failed!");

	agl_ctx = gAGLContext;

	if (gPlayFullScreen)
	{
		gAGLWin = nil;
		aglEnable (gAGLContext, AGL_FS_CAPTURE_SINGLE);
		aglSetFullScreen(gAGLContext, 0, 0, 0, 0);
	}
	else
	{
		gAGLWin = (AGLDrawable)gGameWindowGrafPtr;
		ok = aglSetDrawable(gAGLContext, gAGLWin);
		if ((!ok) || (aglGetError() != AGL_NO_ERROR))
		{
			if (aglGetError() == AGL_BAD_ALLOC)
			{
				gGamePrefs.showScreenModeDialog	= true;
				SavePrefs();
				DoFatalAlert("Not enough VRAM for the selected video mode.  Please try again and select a different mode.");
			}
			else
				DoFatalAlert("OGL_CreateDrawContext: aglSetDrawable failed!");
		}
	}


			/* ACTIVATE CONTEXT */

	mkc = aglSetCurrentContext(gAGLContext);
	if ((mkc == nil) || (aglGetError() != AGL_NO_ERROR))
		return;


			/* NO LONGER NEED PIXEL FORMAT */

	aglDestroyPixelFormat(fmt);



				/* SET VARIOUS STATE INFO */


	glEnable(GL_DEPTH_TEST);								// use z-buffer

	{
		GLfloat	color[] = {1,1,1,1};									// set global material color to white
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color);
	}

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

  	glEnable(GL_NORMALIZE);



 		/***************************/
		/* GET OPENGL CAPABILITIES */
 		/***************************/

	s = (char *)glGetString(GL_EXTENSIONS);					// get extensions list



			/* INIT DEBUG FONT */

	OGL_InitFont();


			/* SEE IF SUPPORT 1024x1024 TEXTURES */

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
	if (maxTexSize < 1024)
		DoFatalAlert("Your video card cannot do 1024x1024 textures, so it is below the game's minimum system requirements.");


				/* CLEAR BACK BUFFER ENTIRELY */

	glClearColor(0,0,0, 1.0);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT);
	aglSwapBuffers(gAGLContext);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(viewDefPtr->clearColor.r, viewDefPtr->clearColor.g, viewDefPtr->clearColor.b, 1.0);

}



/**************** OGL: SET STYLES ****************/

static void OGL_SetStyles(OGLSetupInputType *setupDefPtr)
{
OGLStyleDefType *styleDefPtr = &setupDefPtr->styles;
AGLContext agl_ctx = gAGLContext;


	glEnable(GL_CULL_FACE);									// activate culling
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);									// CCW is front face
	glEnable(GL_DITHER);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);		// set default blend func
	glDisable(GL_BLEND);									// but turn it off by default

	glHint(GL_TRANSFORM_HINT_APPLE, GL_FASTEST);
	glDisable(GL_RESCALE_NORMAL);

    glHint(GL_FOG_HINT, GL_NICEST);		// pixel accurate fog?


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
}




/********************* OGL: CREATE LIGHTS ************************/
//
// NOTE:  The Projection matrix must be the identity or lights will be transformed.
//

static void OGL_CreateLights(OGLLightDefType *lightDefPtr)
{
int		i;
GLfloat	ambient[4];
AGLContext agl_ctx = gAGLContext;

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

	for (i=0; i < lightDefPtr->numFillLights; i++)
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

}

#pragma mark -

/******************* OGL DRAW SCENE *********************/

void OGL_DrawScene(OGLSetupOutputType *setupInfo, void (*drawRoutine)(OGLSetupOutputType *))
{
int	x,y,w,h;
AGLContext agl_ctx = setupInfo->drawContext;

	if (setupInfo == nil)										// make sure it's legit
		DoFatalAlert("OGL_DrawScene setupInfo == nil");
	if (!setupInfo->isActive)
		DoFatalAlert("OGL_DrawScene isActive == false");

  	aglSetCurrentContext(setupInfo->drawContext);			// make context active


			/* INIT SOME STUFF */

	if (gGamePrefs.anaglyph)
	{
		gAnaglyphPass = 0;
		PrepAnaglyphCameras();
	}


	if (gDebugMode)
	{
		gVRAMUsedThisFrame = gGameWindowWidth * gGameWindowHeight * (gGamePrefs.depth / 8);				// backbuffer size
		gVRAMUsedThisFrame += gGameWindowWidth * gGameWindowHeight * 2;										// z-buffer size
		gVRAMUsedThisFrame += gGamePrefs.screenWidth * gGamePrefs.screenHeight * (gGamePrefs.depth / 8);	// display size
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

	if (setupInfo->clearBackBuffer || (gDebugMode == 3))
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


				/* SET VIEWPORT */

	OGL_GetCurrentViewport(setupInfo, &x, &y, &w, &h);
	glViewport(x,y, w, h);
	gCurrentAspectRatio = (float)w/(float)h;


			/* GET UPDATED GLOBAL COPIES OF THE VARIOUS MATRICES */

	OGL_Camera_SetPlacementAndUpdateMatrices(setupInfo);


			/* CALL INPUT DRAW FUNCTION */

	if (drawRoutine != nil)
		drawRoutine(setupInfo);


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

	if (GetNewKeyState(KEY_F8))
	{
		if (++gDebugMode > 3)
			gDebugMode = 0;

		if (gDebugMode == 3)								// see if show wireframe
			glPolygonMode(GL_FRONT_AND_BACK ,GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK ,GL_FILL);
	}


				/* SHOW BASIC DEBUG INFO */

	if (gDebugMode > 0)
	{
		int		y = 100;
		int		mem = FreeMem();

		if (mem < gMinRAM)		// poll for lowest RAM free
			gMinRAM = mem;

		OGL_DrawString("input x:", 20,y);
		OGL_DrawFloat(gPlayerInfo.analogControlX, 100,y);
		y += 15;
		OGL_DrawString("input y:", 20,y);
		OGL_DrawFloat(gPlayerInfo.analogControlZ, 100,y);
		y += 15;

		OGL_DrawString("fps:", 20,y);
		OGL_DrawInt(gFramesPerSecond+.5f, 100,y);
		y += 15;

		OGL_DrawString("#tri:", 20,y);
		OGL_DrawInt(gPolysThisFrame, 100,y);
		y += 15;

		OGL_DrawString("#scratchI:", 20,y);
		OGL_DrawInt(gScratch, 100,y);
		y += 15;

#if 0
		OGL_DrawString("enemies:", 20,y);
		OGL_DrawInt(gNumEnemies, 100,y);
		y += 15;

		OGL_DrawString("player Y:", 20,y);
		OGL_DrawInt(gPlayerInfo.coord.y, 100,y);
		y += 15;


		OGL_DrawString("#free RAM:", 20,y);
		OGL_DrawInt(mem, 100,y);
		y += 15;

		OGL_DrawString("min RAM:", 20,y);
		OGL_DrawInt(gMinRAM, 100,y);
		y += 15;

		OGL_DrawString("used VRAM:", 20,y);
		OGL_DrawInt(gVRAMUsedThisFrame, 100,y);
		y += 15;

		OGL_DrawString("OGL Mem:", 20,y);
		OGL_DrawInt(glmGetInteger(GLM_CURRENT_MEMORY), 100,y);
		y += 15;


		OGL_DrawString("#scratchF:", 20,y);
		OGL_DrawFloat(gScratchF, 100,y);
		y += 15;

		OGL_DrawString("#t-defs:", 20,y);
		OGL_DrawInt(gNumTerrainDeformations, 100,y);
		y += 15;

		OGL_DrawString("#sparkles:", 20,y);
		OGL_DrawInt(gNumSparkles, 100,y);
		y += 15;

		if (gPlayerInfo.objNode)
		{
			OGL_DrawString("ground?:", 20,y);
			if (gPlayerInfo.objNode->StatusBits & STATUS_BIT_ONGROUND)
				OGL_DrawString("Y", 100,y);
			else
				OGL_DrawString("N", 100,y);
			y += 15;
		}


		OGL_DrawString("#H2O:", 20,y);
		OGL_DrawInt(gNumWaterDrawn, 100,y);
		y += 15;

		OGL_DrawString("#scratchI:", 20,y);
		OGL_DrawInt(gScratch, 100,y);
		y += 15;





//		OGL_DrawString("# pointers:", 20,y);
//		OGL_DrawInt(gNumPointers, 100,y);
//		y += 15;

#endif

	}



            /**************/
			/* END RENDER */
			/**************/

           /* SWAP THE BUFFS */

	aglSwapBuffers(setupInfo->drawContext);					// end render loop


	if (gGamePrefs.anaglyph)
		RestoreCamerasFromAnaglyph();

}


/********************** OGL: GET CURRENT VIEWPORT ********************/
//
// Remember that with OpenGL, the bottom of the screen is y==0, so some of this code
// may look upside down.
//

void OGL_GetCurrentViewport(const OGLSetupOutputType *setupInfo, int *x, int *y, int *w, int *h)
{
int	t,b,l,r;

	t = setupInfo->clip.top;
	b = setupInfo->clip.bottom;
	l = setupInfo->clip.left;
	r = setupInfo->clip.right;

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
AGLContext agl_ctx = gAGLContext;


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

/******************** CONVERT TEXTURE TO GREY **********************/
//
// The NTSC luminance standard where grayscale = .299r + .587g + .114b
//


static void	ConvertTextureToGrey(void *imageMemory, short width, short height, GLint srcFormat, GLint dataType)
{
long	x,y;
float	r,g,b;
u_long	a,q,rq,bq;
u_long   redCal = gGamePrefs.anaglyphCalibrationRed;
u_long   blueCal =  gGamePrefs.anaglyphCalibrationBlue;


	if (dataType == GL_UNSIGNED_INT_8_8_8_8_REV)
	{
		u_long	*pix32 = (u_long *)imageMemory;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				u_long	pix = pix32[x];

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
		u_long	*pix32 = (u_long *)imageMemory;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				u_long	pix = SwizzleULong(&pix32[x]);

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
		u_short	*pix16 = (u_short *)imageMemory;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				u_short	pix = pix16[x]; //SwizzleUShort(&pix16[x]);

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

void ColorBalanceRGBForAnaglyph(u_long *rr, u_long *gg, u_long *bb)
{
long	r,g,b;
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
u_long	r,g,b;
u_long	a;

	if (dataType == GL_UNSIGNED_INT_8_8_8_8_REV)
	{
		u_long	*pix32 = (u_long *)imageMemory;
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
		u_long	*pix32 = (u_long *)imageMemory;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				u_long	pix = SwizzleULong(&pix32[x]);

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
AGLContext agl_ctx = gAGLContext;

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

void OGL_MoveCameraFromTo(OGLSetupOutputType *setupInfo, float fromDX, float fromDY, float fromDZ, float toDX, float toDY, float toDZ)
{

			/* SET CAMERA COORDS */

	setupInfo->cameraPlacement.cameraLocation.x += fromDX;
	setupInfo->cameraPlacement.cameraLocation.y += fromDY;
	setupInfo->cameraPlacement.cameraLocation.z += fromDZ;

	setupInfo->cameraPlacement.pointOfInterest.x += toDX;
	setupInfo->cameraPlacement.pointOfInterest.y += toDY;
	setupInfo->cameraPlacement.pointOfInterest.z += toDZ;

	UpdateListenerLocation(setupInfo);
}


/*************** OGL_MoveCameraFrom ***************/

void OGL_MoveCameraFrom(OGLSetupOutputType *setupInfo, float fromDX, float fromDY, float fromDZ)
{

			/* SET CAMERA COORDS */

	setupInfo->cameraPlacement.cameraLocation.x += fromDX;
	setupInfo->cameraPlacement.cameraLocation.y += fromDY;
	setupInfo->cameraPlacement.cameraLocation.z += fromDZ;

	UpdateListenerLocation(setupInfo);
}



/*************** OGL_UpdateCameraFromTo ***************/

void OGL_UpdateCameraFromTo(OGLSetupOutputType *setupInfo, const OGLPoint3D *from, const OGLPoint3D *to)
{
static const OGLVector3D up = {0,1,0};

	setupInfo->cameraPlacement.upVector 		= up;
	setupInfo->cameraPlacement.cameraLocation 	= *from;
	setupInfo->cameraPlacement.pointOfInterest 	= *to;

	UpdateListenerLocation(setupInfo);
}

/*************** OGL_UpdateCameraFromToUp ***************/

void OGL_UpdateCameraFromToUp(OGLSetupOutputType *setupInfo, OGLPoint3D *from, OGLPoint3D *to, OGLVector3D *up)
{

	setupInfo->cameraPlacement.upVector 		= *up;
	setupInfo->cameraPlacement.cameraLocation 	= *from;
	setupInfo->cameraPlacement.pointOfInterest 	= *to;

	UpdateListenerLocation(setupInfo);
}



/************** OGL: CAMERA SET PLACEMENT & UPDATE MATRICES **********************/
//
// This is called by OGL_DrawScene to initialize all of the view matrices,
// and to extract the current view matrices used for culling et.al.
//

void OGL_Camera_SetPlacementAndUpdateMatrices(OGLSetupOutputType *setupInfo)
{
float	aspect;
OGLCameraPlacement	*placement;
int		temp, w, h, i;
OGLLightDefType	*lights;
AGLContext agl_ctx = gAGLContext;

	OGL_GetCurrentViewport(setupInfo, &temp, &temp, &w, &h);
	aspect = (float)w/(float)h;

			/* INIT PROJECTION MATRIX */

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();


			/* SETUP FOR ANAGLYPH STEREO 3D CAMERA */

	if (gGamePrefs.anaglyph)
	{
		float	left, right;
		float	halfFOV = setupInfo->fov * .5f;
		float	near 	= setupInfo->hither;
	   	float	wd2     = near * tan(halfFOV);
		float	ndfl    = near / gAnaglyphFocallength;

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

		glFrustum(left, right, -wd2, wd2, setupInfo->hither, setupInfo->yon);
	}

			/* SETUP STANDARD PERSPECTIVE CAMERA */
	else
	{
		gluPerspective (OGLMath_RadiansToDegrees(setupInfo->fov),	// fov
						aspect,					// aspect
						setupInfo->hither,		// hither
						setupInfo->yon);		// yon
	}



			/* INIT MODELVIEW MATRIX */

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	placement = &setupInfo->cameraPlacement;
	gluLookAt(placement->cameraLocation.x, placement->cameraLocation.y, placement->cameraLocation.z,
			placement->pointOfInterest.x, placement->pointOfInterest.y, placement->pointOfInterest.z,
			placement->upVector.x, placement->upVector.y, placement->upVector.z);


		/* UPDATE LIGHT POSITIONS */

	lights =  &setupInfo->lightList;						// point to light list
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

	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)&gWorldToViewMatrix);
	glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat *)&gViewToFrustumMatrix);
	OGLMatrix4x4_Multiply(&gWorldToViewMatrix, &gViewToFrustumMatrix, &gWorldToFrustumMatrix);

	OGLMatrix4x4_GetFrustumToWindow(setupInfo, &gFrustumToWindowMatrix);
	OGLMatrix4x4_Multiply(&gWorldToFrustumMatrix, &gFrustumToWindowMatrix, &gWorldToWindowMatrix);

	UpdateListenerLocation(setupInfo);
}



#pragma mark -


/**************** OGL BUFFER TO GWORLD ***********************/

GWorldPtr OGL_BufferToGWorld(Ptr buffer, int width, int height, int bytesPerPixel)
{
Rect			r;
GWorldPtr		gworld;
PixMapHandle	gworldPixmap;
long			gworldRowBytes,x,y,pixmapRowbytes;
Ptr				gworldPixelPtr;
unsigned long	*pix32Src,*pix32Dest;
unsigned short	*pix16Src,*pix16Dest;
OSErr			iErr;
long			pixelSize;

			/* CREATE GWORLD TO DRAW INTO */

	switch(bytesPerPixel)
	{
		case	2:
				pixelSize = 16;
				break;

		case	4:
				pixelSize = 32;
				break;
	}

	SetRect(&r,0,0,width,height);
	iErr = NewGWorld(&gworld,pixelSize, &r, nil, nil, 0);
	if (iErr)
		DoFatalAlert("OGL_BufferToGWorld: NewGWorld failed!");

	DoLockPixels(gworld);

	gworldPixmap = GetGWorldPixMap(gworld);
	LockPixels(gworldPixmap);

	gworldRowBytes = (**gworldPixmap).rowBytes & 0x3fff;					// get GWorld's rowbytes
	gworldPixelPtr = GetPixBaseAddr(gworldPixmap);							// get ptr to pixels

	pixmapRowbytes = width * bytesPerPixel;


			/* WRITE DATA INTO GWORLD */

	switch(pixelSize)
	{
		case	32:
				pix32Src = (unsigned long *)buffer;							// get 32bit pointers
				pix32Dest = (unsigned long *)gworldPixelPtr;
				for (y = 0; y <  height; y++)
				{
					for (x = 0; x < width; x++)
						pix32Dest[x] = pix32Src[x];

					pix32Dest += gworldRowBytes/4;							// next dest row
					pix32Src += pixmapRowbytes/4;
				}
				break;

		case	16:
				pix16Src = (unsigned short *)buffer;						// get 16bit pointers
				pix16Dest = (unsigned short *)gworldPixelPtr;
				for (y = 0; y <  height; y++)
				{
					for (x = 0; x < width; x++)
						pix16Dest[x] = pix16Src[x];

					pix16Dest += gworldRowBytes/2;							// next dest row
					pix16Src += pixmapRowbytes/2;
				}
				break;


		default:
				DoFatalAlert("OGL_BufferToGWorld: Only 32/16 bit textures supported right now.");

	}

	return(gworld);
}


/******************** OGL: CHECK ERROR ********************/

GLenum OGL_CheckError(void)
{
GLenum	err;
AGLContext agl_ctx = gAGLContext;


	err = glGetError();
	if (err != GL_NO_ERROR)
	{
		switch(err)
		{
			case	GL_INVALID_ENUM:
					DoAlert("OGL_CheckError: GL_INVALID_ENUM");
					DoFatalAlert("This typically means that you have a very old version of OpenGL installed.  Install OpenGL 1.2.1 or later.");
					break;

			case	GL_INVALID_VALUE:
					DoAlert("OGL_CheckError: GL_INVALID_VALUE");
					break;

			case	GL_INVALID_OPERATION:
					DoAlert("OGL_CheckError: GL_INVALID_OPERATION");
					break;

			case	GL_STACK_OVERFLOW:
					DoAlert("OGL_CheckError: GL_STACK_OVERFLOW");
					break;

			case	GL_STACK_UNDERFLOW:
					DoAlert("OGL_CheckError: GL_STACK_UNDERFLOW");
					break;

			case	GL_OUT_OF_MEMORY:
					DoAlert("OGL_CheckError: GL_OUT_OF_MEMORY  (increase your Virtual Memory setting!)");
					break;

			default:
					DoAlert("OGL_CheckError: some other error");
					ShowSystemErr_NonFatal(err);
		}
	}

	return(err);
}


#pragma mark -


/********************* PUSH STATE **************************/

void OGL_PushState(void)
{
int	i;
AGLContext agl_ctx = gAGLContext;

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
AGLContext agl_ctx = gAGLContext;

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
AGLContext agl_ctx = gAGLContext;

	gMyState_Lighting = true;
	glEnable(GL_LIGHTING);
}

/******************* OGL DISABLE LIGHTING ****************************/

void OGL_DisableLighting(void)
{
AGLContext agl_ctx = gAGLContext;

	gMyState_Lighting = false;
	glDisable(GL_LIGHTING);
}


#pragma mark -

/************************** OGL_INIT FONT **************************/

static void OGL_InitFont(void)
{
AGLContext agl_ctx = gAGLContext;

	gFontList = glGenLists(256);

    if (!aglUseFont(gAGLContext, kFontIDMonaco, bold, 9, 0, 256, gFontList))
		DoFatalAlert("OGL_InitFont: aglUseFont failed");
}


/******************* OGL_FREE FONT ***********************/

static void OGL_FreeFont(void)
{

AGLContext agl_ctx = gAGLContext;
	glDeleteLists(gFontList, 256);

}

/**************** OGL_DRAW STRING ********************/

void OGL_DrawString(Str255 s, GLint x, GLint y)
{

AGLContext agl_ctx = gAGLContext;

	OGL_PushState();

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 640, 0, 480, -10.0, 10.0);

	glDisable(GL_LIGHTING);

	glRasterPos2i(x, 480-y);

	glListBase(gFontList);
	glCallLists(s[0], GL_UNSIGNED_BYTE, &s[1]);

	OGL_PopState();

}

/**************** OGL_DRAW FLOAT ********************/

void OGL_DrawFloat(float f, GLint x, GLint y)
{

Str255	s;

	FloatToString(f,s);
	OGL_DrawString(s,x,y);

}



/**************** OGL_DRAW INT ********************/

void OGL_DrawInt(int f, GLint x, GLint y)
{

Str255	s;

	NumToString(f,s);
	OGL_DrawString(s,x,y);

}

#pragma mark -


/********************* OGL:  CHECK RENDERER **********************/
//
// Returns: true if renderer for the requested device complies, false otherwise
//

Boolean OGL_CheckRenderer (GDHandle hGD, long* vram)
{
AGLRendererInfo info, head_info;
GLint 			dAccel = 0;
Boolean			gotit = false;

			/**********************/
			/* GET FIRST RENDERER */
			/**********************/

	head_info = aglQueryRendererInfo(&hGD, 1);
	if(!head_info)
	{
		DoAlert("CheckRenderer: aglQueryRendererInfo failed");
		DoFatalAlert("This problem occurs if you have run the faulty MacOS 9.2.1 updater.  To fix, simply delete all Nvidia extensions and reboot.");
	}

		/*******************************************/
		/* SEE IF THERE IS AN ACCELERATED RENDERER */
		/*******************************************/

	info = head_info;

	while (info)
	{
		aglDescribeRenderer(info, AGL_ACCELERATED, &dAccel);

				/* GOT THE ACCELERATED RENDERER */

		if (dAccel)
		{
			gotit = true;

					/* GET VRAM */

			aglDescribeRenderer (info, AGL_TEXTURE_MEMORY, vram);




			break;
		}


				/* TRY NEXT ONE */

		info = aglNextRendererInfo(info);
	}



			/***********/
			/* CLEANUP */
			/***********/

	aglDestroyRendererInfo(head_info);

	return(gotit);
}

