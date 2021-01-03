/****************************/
/*   	MISCSCREENS.C	    */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	FSSpec		gDataSpec;
extern	KeyMap gKeyMap,gNewKeys;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	Boolean		gSongPlayingFlag,gDisableAnimSounds,gOSX;
extern	PrefsType	gGamePrefs;
extern	OGLPoint3D	gCoord;
extern	int			gLevelNum;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	OGLVector3D	gDelta;
extern	short		gMainAppRezFile;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DisplayPicture_Draw(OGLSetupOutputType *info);
//static pascal OSStatus DoGameSettingsDialog_EventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData);


/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

MOPictureObject 	*gBackgoundPicture = nil;

OGLSetupOutputType	*gScreenViewInfoPtr = nil;

static	Boolean	gLanguageChanged = false;


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



#pragma mark -

#if DEMO

/****************** DO DEMO EXPIRED SCREEN **************************/

void DoDemoExpiredScreen(void)
{
FSSpec	spec;

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Images:DemoExpired", &spec);
	DisplayPicture(&spec);
	CleanQuit();
}


/*************** SHOW DEMO QUIT SCREEN **********************/

void ShowDemoQuitScreen(void)
{
FSSpec	spec;

	SaveDemoTimer();						// make sure to save this before we bail

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Images:DemoQuit", &spec);
	DisplayPicture(&spec);

}


#endif




/********************* DO LEVEL CHEAT DIALOG **********************/

Boolean DoLevelCheatDialog(void)
{
	SOURCE_PORT_PLACEHOLDER();
#if 0	// srcport: awaiting reimplementation
DialogPtr 		myDialog;
short			itemHit;
Boolean			dialogDone = false;

	Enter2D();
//	InitCursor();
	myDialog = GetNewDialog(132,nil,MOVE_TO_FRONT);
	if (!myDialog)
	{
		DoAlert("DoLevelCheatDialog: GetNewDialog failed!");
		ShowSystemErr(ResError());
	}

	AutoSizeDialog(myDialog);



//	GammaFadeIn();


				/* DO IT */

	MyFlushEvents();
	while(!dialogDone)
	{
		ModalDialog(nil, &itemHit);
		switch (itemHit)
		{
			case 	1:									// hit Quit
					CleanQuit();

			default:									// selected a level
					gLevelNum = (itemHit - 2);
					dialogDone = true;
		}
	}
	DisposeDialog(myDialog);
	Exit2D();
//	HideCursor();
	GammaFadeOut();
	GameScreenToBlack();

#endif
	return(false);
}

#if 1

#pragma mark -

/********************** DO GAME SETTINGS DIALOG ************************/

void DoGameSettingsDialog(void)
{
	SOURCE_PORT_PLACEHOLDER();
#if 0	// srcport: awaiting reimplementation
OSErr			err;
EventTypeSpec	list[] = { { kEventClassCommand,  kEventProcessCommand } };
WindowRef 		dialogWindow = nil;
EventHandlerUPP winEvtHandler;
ControlID 		idControl;
ControlRef 		control;
EventHandlerRef	ref;

const char		*rezNames[MAX_LANGUAGES] =
{
	"Settings_English",
	"Settings_French",
	"Settings_German",
	"Settings_Spanish",
	"Settings_Italian",
	"Settings_Swedish"
};


	if (gGamePrefs.language >= MAX_LANGUAGES)		// verify prefs for the hell of it.
		InitDefaultPrefs();

	Enter2D();
	InitCursor();
	MyFlushEvents();


    		/***************/
    		/* INIT DIALOG */
    		/***************/

do_again:

	gLanguageChanged  = false;

			/* CREATE WINDOW FROM THE NIB */

    err = CreateWindowFromNib(gNibs,CFStringCreateWithCString(nil, rezNames[gGamePrefs.language],
    						kCFStringEncodingMacRoman), &dialogWindow);
	if (err)
		DoFatalAlert("DoGameSettingsDialog: CreateWindowFromNib failed!");

			/* CREATE NEW WINDOW EVENT HANDLER */

    winEvtHandler = NewEventHandlerUPP(DoGameSettingsDialog_EventHandler);
    InstallWindowEventHandler(dialogWindow, winEvtHandler, GetEventTypeCount(list), list, dialogWindow, &ref);


			/* SET "SHOW VIDEO" CHECKBOX */

    idControl.signature = 'vido';
    idControl.id 		= 0;
    GetControlByID(dialogWindow, &idControl, &control);
	SetControlValue(control, gGamePrefs.showScreenModeDialog);


			/* SET "CONTROL RELATIVE" BUTTONS */

    idControl.signature = 'relc';
    idControl.id 		= 0;
    GetControlByID(dialogWindow, &idControl, &control);
	SetControlValue(control, gGamePrefs.playerRelControls + 1);


			/* SET LANGUAGE MENU */

    idControl.signature = 'lang';
    idControl.id 		= 0;
    GetControlByID(dialogWindow, &idControl, &control);
	SetControlValue(control, gGamePrefs.language + 1);



			/**********************/
			/* PROCESS THE DIALOG */
			/**********************/

    ShowWindow(dialogWindow);
	RunAppModalLoopForWindow(dialogWindow);


			/*********************/
			/* GET RESULT VALUES */
			/*********************/

			/* GET "SHOW VIDEO" CHECKBOX */

    idControl.signature = 'vido';
    idControl.id 		= 0;
    GetControlByID(dialogWindow, &idControl, &control);
	gGamePrefs.showScreenModeDialog = GetControlValue(control);


			/* GET LANGUAGE */

    idControl.signature = 'lang';
    idControl.id 		= 0;
    GetControlByID(dialogWindow, &idControl, &control);
	gGamePrefs.language = GetControlValue(control) - 1;


			/* GET RELATIVE BUTTONS */

    idControl.signature = 'relc';
    idControl.id 		= 0;
    GetControlByID(dialogWindow, &idControl, &control);
	gGamePrefs.playerRelControls = GetControlValue(control) - 1;

				/***********/
				/* CLEANUP */
				/***********/

	DisposeEventHandlerUPP (winEvtHandler);
	DisposeWindow (dialogWindow);

			/* IF IT WAS JUST A LANGUAGE CHANGE THEN GO BACK TO THE DIALOG */

	if (gLanguageChanged)
		goto do_again;


	HideCursor();
	Exit2D();
	SavePrefs();

	CalcFramesPerSecond();				// reset this so things dont go crazy when we return
	CalcFramesPerSecond();
#endif
}


/****************** DO GAME SETTINGS DIALOG EVENT HANDLER *************************/

#if 0	// srcport: awaiting reimplementation
static pascal OSStatus DoGameSettingsDialog_EventHandler(EventHandlerCallRef myHandler, EventRef event, void* userData)
{
#pragma unused (myHandler, userData)
OSStatus			result = eventNotHandledErr;
HICommand 			command;

	switch(GetEventKind(event))
	{

				/*******************/
				/* PROCESS COMMAND */
				/*******************/

		case	kEventProcessCommand:
				GetEventParameter (event, kEventParamDirectObject, kEventParamHICommand, NULL, sizeof(command), NULL, &command);
				switch(command.commandID)
				{
							/* OK BUTTON */

					case	kHICommandOK:
		                    QuitAppModalLoopForWindow((WindowRef) userData);
		                    break;


							/* CONFIGURE INPUT BUTTON */

					case	'cnfg':
							DoInputConfigDialog();
							break;


							/* CLEAR HIGH SCORES BUTTON */

					case	'hisc':
							ClearHighScores();
							break;


							/* LANGUAGE */

					case	'lang':
							gLanguageChanged = true;
		                    QuitAppModalLoopForWindow((WindowRef) userData);
							break;

				}
				break;
    }

    return (result);
}
#endif

#else

/********************* DO GAME SETTINGS DIALOG *********************/

void DoGameSettingsDialog(void)
{
DialogRef 		myDialog;
DialogItemType	itemHit;
ControlRef		ctrlHandle;
Boolean			dialogDone;


	if (gGamePrefs.language > LANGUAGE_SWEDISH)		// verify prefs for the hell of it.
		InitDefaultPrefs();

	Enter2D();



	InitCursor();

	MyFlushEvents();

	UseResFile(gMainAppRezFile);
	myDialog = GetNewDialog(1000 + gGamePrefs.language,nil,MOVE_TO_FRONT);
	if (myDialog == nil)
	{
		DoAlert("DoGameSettingsDialog: GetNewDialog failed!");
		ShowSystemErr(gGamePrefs.language);
	}


				/* SET CONTROL VALUES */

	GetDialogItemAsControl(myDialog,10,&ctrlHandle);			// camera relativity
	SetControlValue(ctrlHandle,!gGamePrefs.playerRelControls);
	GetDialogItemAsControl(myDialog,11,&ctrlHandle);
	SetControlValue(ctrlHandle,gGamePrefs.playerRelControls);

	GetDialogItemAsControl(myDialog,4,&ctrlHandle);				// language
	SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_ENGLISH);
	GetDialogItemAsControl(myDialog,5,&ctrlHandle);
	SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_FRENCH);
	GetDialogItemAsControl(myDialog,6,&ctrlHandle);
	SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_GERMAN);
	GetDialogItemAsControl(myDialog,7,&ctrlHandle);
	SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_SPANISH);
	GetDialogItemAsControl(myDialog,8,&ctrlHandle);
	SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_ITALIAN);
	GetDialogItemAsControl(myDialog,9,&ctrlHandle);
	SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_SWEDISH);



	AutoSizeDialog(myDialog);




				/* DO IT */

	dialogDone = false;
	while(dialogDone == false)
	{
		ModalDialog(nil, &itemHit);
		switch (itemHit)
		{
			case 	1:									// hit ok
					dialogDone = true;
					break;

			case	10:
			case	11:
					gGamePrefs.playerRelControls = (itemHit == 11);
					GetDialogItemAsControl(myDialog,10,&ctrlHandle);
					SetControlValue(ctrlHandle,!gGamePrefs.playerRelControls);
					GetDialogItemAsControl(myDialog,11,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.playerRelControls);
					break;

			case	4:
			case	5:
			case	6:
			case	7:
			case	8:
			case	9:
					gGamePrefs.language = LANGUAGE_ENGLISH + (itemHit - 4);
					GetDialogItemAsControl(myDialog,4,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_ENGLISH);
					GetDialogItemAsControl(myDialog,5,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_FRENCH);
					GetDialogItemAsControl(myDialog,6,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_GERMAN);
					GetDialogItemAsControl(myDialog,7,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_SPANISH);
					GetDialogItemAsControl(myDialog,8,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_ITALIAN);
					GetDialogItemAsControl(myDialog,9,&ctrlHandle);
					SetControlValue(ctrlHandle,gGamePrefs.language == LANGUAGE_SWEDISH);
					break;

			case	3:
					DoInputConfigDialog();
					InitCursor();
					break;


			case	15:
					ClearHighScores();
					break;


		}
	}

			/* CLEAN UP */

	DisposeDialog(myDialog);


	HideCursor();

	Exit2D();


	SavePrefs();

	CalcFramesPerSecond();				// reset this so things dont go crazy when we return
	CalcFramesPerSecond();



}

#endif


#pragma mark -



/************** DO HTTP MUTIPLE FAILURE WARNING ******************/
//
// This is called from ToolBoxInit() if our multiple attempts to read the UpdateData
// from the web site have failed.
//

void DoHTTPMultipleFailureWarning(void)
{
	DoAlert("Otto Matic needs to connect to the internet to get some information, but several attempts over many days have failed.");
	DoAlert("Please check that you do not have any firewall software blocking www.pangeasoft.net");
	DoFatalAlert("If you continue to have trouble, please contact support@pangeasoft.net");
}







