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

static void DisplayPicture_Draw(OGLSetupOutputType *info);


/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

MOPictureObject 	*gBackgoundPicture = nil;


/********************** DISPLAY PICTURE **************************/
//
// Displays a picture using OpenGL until the user clicks the mouse or presses any key.
// If showAndBail == true, then show it and bail out
//

void DisplayPicture(FSSpec *spec)
{
OGLSetupInputType	viewDef;
float	timeout = 40.0f;


			/* SETUP VIEW */

	OGL_NewViewDef(&viewDef);

	viewDef.camera.hither 			= 10;
	viewDef.camera.yon 				= 3000;
	viewDef.view.clearColor.r 		= 0;
	viewDef.view.clearColor.g 		= 0;
	viewDef.view.clearColor.b		= 0;
	viewDef.styles.useFog			= false;

	OGL_SetupWindow(&viewDef, &gGameViewInfoPtr);


			/* CREATE BACKGROUND OBJECT */

	gBackgoundPicture = MO_CreateNewObjectOfType(MO_TYPE_PICTURE, (u_long)gGameViewInfoPtr, spec);
	if (!gBackgoundPicture)
		DoFatalAlert("DisplayPicture: MO_CreateNewObjectOfType failed");



		/***********/
		/* SHOW IT */
		/***********/


	MakeFadeEvent(true, 1.0);
	UpdateInput();
	CalcFramesPerSecond();

					/* MAIN LOOP */

		while(!Button())
		{
			CalcFramesPerSecond();
			MoveObjects();
			OGL_DrawScene(gGameViewInfoPtr, DisplayPicture_Draw);

			UpdateInput();
			if (AreAnyNewKeysPressed())
				break;

			timeout -= gFramesPerSecondFrac;
			if (timeout < 0.0f)
				break;
		}


			/* CLEANUP */

	DeleteAllObjects();
	MO_DisposeObjectReference(gBackgoundPicture);
	DisposeAllSpriteGroups();


			/* FADE OUT */

	GammaFadeOut();


	OGL_DisposeWindowSetup(&gGameViewInfoPtr);


}


/***************** DISPLAY PICTURE: DRAW *******************/

static void DisplayPicture_Draw(OGLSetupOutputType *info)
{
	MO_DrawObject(gBackgoundPicture, info);
	DrawObjects(info);
}


#pragma mark -

/************** DO LEGAL SCREEN *********************/

void DoLegalScreen(void)
{
FSSpec	spec;

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Images:Title", &spec);

	DisplayPicture(&spec);

}
