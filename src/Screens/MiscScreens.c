/****************************/
/*   	MISCSCREENS.C	    */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DisplayPicture_Draw(void);


/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

static MOPictureObject 	*gBackgroundPicture = nil;


/********************** DISPLAY PICTURE **************************/
//
// Displays a picture using OpenGL until the user clicks the mouse or presses any key.
//

void DisplayPicture(const char* path, float timeout)
{
OGLSetupInputType	viewDef;


			/* SETUP VIEW */

	OGL_NewViewDef(&viewDef);

	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.view.clearColor.r 		= 1;
	viewDef.view.clearColor.g 		= 1;
	viewDef.view.clearColor.b		= 0;
	viewDef.styles.useFog			= false;

	OGL_SetupWindow(&viewDef);


			/* CREATE BACKGROUND OBJECT */

	gBackgroundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, 0, (void*) path);
	GAME_ASSERT(gBackgroundPicture);



		/***********/
		/* SHOW IT */
		/***********/


	MakeFadeEvent(true, 1.0);
	UpdateInput();
	CalcFramesPerSecond();

					/* MAIN LOOP */

		do
		{
			CalcFramesPerSecond();
			MoveObjects();
			OGL_DrawScene(DisplayPicture_Draw);

			UpdateInput();

			timeout -= gFramesPerSecondFrac;
			if (timeout < 0.0f)
				break;
		} while (!UserWantsOut());


			/* FADE OUT */

	OGL_FadeOutScene(DisplayPicture_Draw, NULL);


			/* CLEANUP */

	DeleteAllObjects();
	MO_DisposeObjectReference(gBackgroundPicture);
	DisposeAllSpriteGroups();




	OGL_DisposeWindowSetup();
}


/***************** DISPLAY PICTURE: DRAW *******************/

static void DisplayPicture_Draw(void)
{
	MO_DrawObject(gBackgroundPicture);
	DrawObjects();
}


#pragma mark -

/************** DO LEGAL SCREEN *********************/

void DoLegalScreen(void)
{
	DisplayPicture(":system:legal.tga", 8.0f);
	Pomme_FlushPtrTracking(true);
}
