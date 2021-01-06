/****************************/
/*      FILE ROUTINES       */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/


extern	short			gMainAppRezFile,gCurrentSong;
extern	short			gNumTerrainItems;
extern	short			gPrefsFolderVRefNum;
extern	long			gPrefsFolderDirID,gNumPaths;
extern	long			gTerrainTileWidth,gTerrainTileDepth,gTerrainUnitWidth,gTerrainUnitDepth,gNumUniqueSuperTiles;
extern	long			gNumSuperTilesDeep,gNumSuperTilesWide;
extern	FSSpec			gDataSpec;
extern	u_long			gScore,gLoadedScore;
extern	float			gDemoVersionTimer;
extern  u_short			**gTileDataHandle;
extern	float			**gMapYCoords,**gMapYCoordsOriginal;
extern	Byte			**gMapSplitMode;
extern	TerrainItemEntryType 	**gMasterItemList;
extern	SuperTileGridType **gSuperTileTextureGrid;
extern	FenceDefType	*gFenceList;
extern	long			gNumFences,gNumSplines,gNumWaterPatches;
extern	SplineDefType	**gSplineList;
extern	int				gLevelNum;
extern	TileAttribType	**gTileAttribList;
extern	u_short			**gTileGrid;
extern	MOMaterialObject	*gSuperTileTextureObjects[MAX_SUPERTILE_TEXTURES];
extern	PrefsType			gGamePrefs;
extern	SDL_GLContext		gAGLContext;
extern	Boolean			gSongPlayingFlag,gLowMemMode,gMuteMusicFlag,gMuteMusicFlag,gLoadedDrawSprocket,gOSX;
//extern	Movie				gSongMovie;
extern	WaterDefType	**gWaterListHandle, *gWaterList;
extern	PlayerInfoType	gPlayerInfo;
extern	Boolean			gPlayingFromSavedGame,gG4;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec, int skeletonType, OGLSetupOutputType *setupInfo);
static void ReadDataFromPlayfieldFile(FSSpec *specPtr, OGLSetupOutputType *setupInfo);
static void	ConvertTexture16To16(u_short *textureBuffer, int width, int height);

#if 0	// srcport rm
static OSErr GetFileWithNavServices(const FSSpecPtr defaultLocationfssPtr, FSSpec *documentFSSpec);
pascal void myEventProc(NavEventCallbackMessage callBackSelector, NavCBRecPtr callBackParms,
						NavCallBackUserData callBackUD);
pascal Boolean myFilterProc(AEDesc*theItem,void*info, NavCallBackUserData callBackUD, NavFilterModes filterMode);
static OSErr PutFileWithNavServices(NavReplyRecord *reply, FSSpec *outSpec);


static Boolean FindSharedLib(ConstStrFileNameParam libName, FSSpec *spec);
static UInt32	GetFileVersion(FSSpec *spec);
#endif


/****************************/
/*    CONSTANTS             */
/****************************/

#define	BASE_PATH_TILE		900					// tile # of 1st path tile

#define	PICT_HEADER_SIZE	512

#define	SKELETON_FILE_VERS_NUM	0x0110			// v1.1

#define	SAVE_GAME_VERSION	0x0100		// 1.0

		/* SAVE GAME */

typedef struct
{
	u_long		version;
	u_long		score;
	short		realLevel;
	short		numLives;
	float		health;
	float		jumpJet;
}SaveGameType;


		/* PLAYFIELD HEADER */
		// READ IN FROM FILE!

typedef struct
{
	NumVersion	version;							// version of file
	int32_t		numItems;							// # items in map
	int32_t		mapWidth;							// width of map
	int32_t		mapHeight;							// height of map
	int32_t		numTilePages;						// # tile pages
	int32_t		numTilesInList;						// # extracted tiles in list
	float		tileSize;							// 3D unit size of a tile
	float		minY,maxY;							// min/max height values
	int32_t		numSplines;							// # splines
	int32_t		numFences;							// # fences
	int32_t		numUniqueSuperTiles;				// # unique supertile
	int32_t		numWaterPatches;                    // # water patches
	int32_t		numCheckpoints;						// # checkpoints
}PlayfieldHeaderType;


		/* FENCE STRUCTURE IN FILE */
		//
		// note: we copy this data into our own fence list
		//		since the game uses a slightly different
		//		data structure.
		//

typedef	struct
{
	int32_t		x,z;
}FencePointType;


typedef struct
{
	uint16_t		type;				// type of fence
	int16_t			numNubs;			// # nubs in fence
	int32_t			junk1;//FencePointType	**nubList;			// handle to nub list
	Rect			bBox;				// bounding box of fence area
}FileFenceDefType;

/**********************/
/*     VARIABLES      */
/**********************/


float	g3DTileSize, g3DMinY, g3DMaxY;

static 	FSSpec		gSavedGameSpec;

static	FSSpec	gApplicationFSSpec;								// spec of this application



/****************** SET DEFAULT DIRECTORY ********************/
//
// This function needs to be called for OS X because OS X doesnt automatically
// set the default directory to the application directory.
//

void SetDefaultDirectory(void)
{
	SOURCE_PORT_MINOR_PLACEHOLDER();
#if 0
ProcessSerialNumber serial;
ProcessInfoRec info;
WDPBRec wpb;
OSErr	iErr;

	serial.highLongOfPSN = 0;
	serial.lowLongOfPSN = kCurrentProcess;


	info.processInfoLength = sizeof(ProcessInfoRec);
	info.processName = NULL;
	info.processAppSpec = &gApplicationFSSpec;

	iErr = GetProcessInformation(&serial, & info);

	wpb.ioVRefNum = gApplicationFSSpec.vRefNum;
	wpb.ioWDDirID = gApplicationFSSpec.parID;
	wpb.ioNamePtr = NULL;

	iErr = PBHSetVolSync(&wpb);
#endif
}



/******************* LOAD SKELETON *******************/
//
// Loads a skeleton file & creates storage for it.
//
// NOTE: Skeleton types 0..NUM_CHARACTERS-1 are reserved for player character skeletons.
//		Skeleton types NUM_CHARACTERS and over are for other skeleton entities.
//
// OUTPUT:	Ptr to skeleton data
//

SkeletonDefType *LoadSkeletonFile(short skeletonType, OGLSetupOutputType *setupInfo)
{
QDErr		iErr;
short		fRefNum;
FSSpec		fsSpec;
SkeletonDefType	*skeleton;
const Str63	fileNames[MAX_SKELETON_TYPES] =
{
	":Skeletons:Otto.skeleton",
	":Skeletons:Farmer.skeleton",
	":Skeletons:BrainAlien.skeleton",
	":Skeletons:Onion.skeleton",
	":Skeletons:Corn.skeleton",
	":Skeletons:Tomato.skeleton",
	":Skeletons:Blob.skeleton",
	":Skeletons:SlimeTree.skeleton",
	":Skeletons:BeeWoman.skeleton",
	":Skeletons:Squooshy.skeleton",
	":Skeletons:Flamester.skeleton",
	":Skeletons:GiantLizard.skeleton",
	":Skeletons:VenusFlytrap.skeleton",
	":Skeletons:Mantis.skeleton",
	":Skeletons:Turtle.skeleton",
	":Skeletons:PodWorm.skeleton",
	":Skeletons:Mutant.skeleton",
	":Skeletons:MutantRobot.skeleton",
	":Skeletons:Scientist.skeleton",
	":Skeletons:PitcherPlant.skeleton",
	":Skeletons:Clown.skeleton",
	":Skeletons:ClownFish.skeleton",
	":Skeletons:StrongMan.skeleton",
	":Skeletons:SkirtLady.skeleton",
	":Skeletons:IceCube.skeleton",
	":Skeletons:EliteBrainAlien.skeleton",
};


	if (skeletonType < MAX_SKELETON_TYPES)
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, fileNames[skeletonType], &fsSpec);
	else
		DoFatalAlert("LoadSkeleton: Unknown skeletonType!");


			/* OPEN THE FILE'S REZ FORK */

	fRefNum = FSpOpenResFile(&fsSpec,fsRdPerm);
	if (fRefNum == -1)
	{
		iErr = ResError();
		DoAlert("Error opening Skel Rez file");
		ShowSystemErr(iErr);
	}

	UseResFile(fRefNum);
	if (ResError())
		DoFatalAlert("Error using Rez file!");


			/* ALLOC MEMORY FOR SKELETON INFO STRUCTURE */

	skeleton = (SkeletonDefType *)AllocPtr(sizeof(SkeletonDefType));
	if (skeleton == nil)
		DoFatalAlert("Cannot alloc SkeletonInfoType");


			/* READ SKELETON RESOURCES */

	ReadDataFromSkeletonFile(skeleton,&fsSpec,skeletonType,setupInfo);
	PrimeBoneData(skeleton);

			/* CLOSE REZ FILE */

	CloseResFile(fRefNum);
	UseResFile(gMainAppRezFile);

	return(skeleton);
}


/************* READ DATA FROM SKELETON FILE *******************/
//
// Current rez file is set to the file.
//

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec, int skeletonType, OGLSetupOutputType *setupInfo)
{
Handle				hand;
int					i,k,j;
long				numJoints,numAnims,numKeyframes;
AnimEventType		*animEventPtr;
JointKeyframeType	*keyFramePtr;
SkeletonFile_Header_Type	*headerPtr;
short				version;
AliasHandle				alias;
OSErr					iErr;
FSSpec					target;
Boolean					wasChanged;
OGLPoint3D				*pointPtr;
SkeletonFile_AnimHeader_Type	*animHeaderPtr;


			/************************/
			/* READ HEADER RESOURCE */
			/************************/

	hand = GetResource('Hedr',1000);
	if (hand == nil)
		DoFatalAlert("ReadDataFromSkeletonFile: Error reading header resource!");
	headerPtr = (SkeletonFile_Header_Type *)*hand;
	version = SwizzleShort(&headerPtr->version);
	if (version != SKELETON_FILE_VERS_NUM)
		DoFatalAlert("Skeleton file has wrong version #");

	numAnims = skeleton->NumAnims = SwizzleShort(&headerPtr->numAnims);			// get # anims in skeleton
	numJoints = skeleton->NumBones = SwizzleShort(&headerPtr->numJoints);		// get # joints in skeleton
	ReleaseResource(hand);

	if (numJoints > MAX_JOINTS)										// check for overload
		DoFatalAlert("ReadDataFromSkeletonFile: numJoints > MAX_JOINTS");


				/*************************************/
				/* ALLOCATE MEMORY FOR SKELETON DATA */
				/*************************************/

	AllocSkeletonDefinitionMemory(skeleton);



		/********************************/
		/* 	LOAD THE REFERENCE GEOMETRY */
		/********************************/

	alias = (AliasHandle)GetResource(rAliasType,1001);				// alias to geometry BG3D file
	if (alias != nil)
	{
		iErr = ResolveAlias(fsSpec, alias, &target, &wasChanged);	// try to resolve alias
		if (!iErr)
			LoadBonesReferenceModel(&target,skeleton, skeletonType, setupInfo);
		else
			DoFatalAlert("ReadDataFromSkeletonFile: Cannot find Skeleton's 3DMF file!");
		ReleaseResource((Handle)alias);
	}
	else
		DoFatalAlert("ReadDataFromSkeletonFile: file is missing the Alias resource");



		/***********************************/
		/*  READ BONE DEFINITION RESOURCES */
		/***********************************/

	for (i=0; i < numJoints; i++)
	{
		File_BoneDefinitionType	*bonePtr;
		u_short					*indexPtr;

			/* READ BONE DATA */

		hand = GetResource('Bone',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading Bone resource!");
		HLock(hand);
		bonePtr = (File_BoneDefinitionType *)*hand;

			/* COPY BONE DATA INTO ARRAY */

		skeleton->Bones[i].parentBone = SwizzleLong(&bonePtr->parentBone);					// index to previous bone
		skeleton->Bones[i].coord.x = SwizzleFloat(&bonePtr->coord.x);						// absolute coord (not relative to parent!)
		skeleton->Bones[i].coord.y = SwizzleFloat(&bonePtr->coord.y);
		skeleton->Bones[i].coord.z = SwizzleFloat(&bonePtr->coord.z);
		skeleton->Bones[i].numPointsAttachedToBone = SwizzleUShort(&bonePtr->numPointsAttachedToBone);		// # vertices/points that this bone has
		skeleton->Bones[i].numNormalsAttachedToBone = SwizzleUShort(&bonePtr->numNormalsAttachedToBone);	// # vertex normals this bone has
		ReleaseResource(hand);

			/* ALLOC THE POINT & NORMALS SUB-ARRAYS */

		skeleton->Bones[i].pointList = (u_short *)AllocPtr(sizeof(u_short) * (int)skeleton->Bones[i].numPointsAttachedToBone);
		if (skeleton->Bones[i].pointList == nil)
			DoFatalAlert("ReadDataFromSkeletonFile: AllocPtr/pointList failed!");

		skeleton->Bones[i].normalList = (u_short *)AllocPtr(sizeof(u_short) * (int)skeleton->Bones[i].numNormalsAttachedToBone);
		if (skeleton->Bones[i].normalList == nil)
			DoFatalAlert("ReadDataFromSkeletonFile: AllocPtr/normalList failed!");

			/* READ POINT INDEX ARRAY */

		hand = GetResource('BonP',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading BonP resource!");
		HLock(hand);
		indexPtr = (u_short *)(*hand);

			/* COPY POINT INDEX ARRAY INTO BONE STRUCT */

		for (j=0; j < skeleton->Bones[i].numPointsAttachedToBone; j++)
			skeleton->Bones[i].pointList[j] = SwizzleUShort(&indexPtr[j]);
		ReleaseResource(hand);


			/* READ NORMAL INDEX ARRAY */

		hand = GetResource('BonN',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading BonN resource!");
		HLock(hand);
		indexPtr = (u_short *)(*hand);

			/* COPY NORMAL INDEX ARRAY INTO BONE STRUCT */

		for (j=0; j < skeleton->Bones[i].numNormalsAttachedToBone; j++)
			skeleton->Bones[i].normalList[j] = SwizzleUShort(&indexPtr[j]);
		ReleaseResource(hand);

	}


		/*******************************/
		/* READ POINT RELATIVE OFFSETS */
		/*******************************/
		//
		// The "relative point offsets" are the only things
		// which do not get rebuilt in the ModelDecompose function.
		// We need to restore these manually.

	hand = GetResource('RelP', 1000);
	if (hand == nil)
		DoFatalAlert("Error reading RelP resource!");
	HLock(hand);
	pointPtr = (OGLPoint3D *)*hand;

	i = GetHandleSize(hand) / sizeof(OGLPoint3D);
	if (i != skeleton->numDecomposedPoints)
		DoFatalAlert("# of points in Reference Model has changed!");
	else
		for (i = 0; i < skeleton->numDecomposedPoints; i++)
		{
			skeleton->decomposedPointList[i].boneRelPoint.x = SwizzleFloat(&pointPtr[i].x);
			skeleton->decomposedPointList[i].boneRelPoint.y = SwizzleFloat(&pointPtr[i].y);
			skeleton->decomposedPointList[i].boneRelPoint.z = SwizzleFloat(&pointPtr[i].z);
		}

	ReleaseResource(hand);


			/*********************/
			/* READ ANIM INFO   */
			/*********************/

	for (i=0; i < numAnims; i++)
	{
				/* READ ANIM HEADER */

		hand = GetResource('AnHd',1000+i);
		if (hand == nil)
			DoFatalAlert("Error getting anim header resource");
		HLock(hand);
		animHeaderPtr = (SkeletonFile_AnimHeader_Type *)*hand;

		skeleton->NumAnimEvents[i] = SwizzleShort(&animHeaderPtr->numAnimEvents);			// copy # anim events in anim
		ReleaseResource(hand);

			/* READ ANIM-EVENT DATA */

		hand = GetResource('Evnt',1000+i);
		if (hand == nil)
			DoFatalAlert("Error reading anim-event data resource!");
		animEventPtr = (AnimEventType *)*hand;
		for (j=0;  j < skeleton->NumAnimEvents[i]; j++)
		{
			skeleton->AnimEventsList[i][j] = *animEventPtr++;							// copy whole thing
			skeleton->AnimEventsList[i][j].time = SwizzleShort(&skeleton->AnimEventsList[i][j].time);	// then swizzle the 16-bit short value
		}
		ReleaseResource(hand);


			/* READ # KEYFRAMES PER JOINT IN EACH ANIM */

		hand = GetResource('NumK',1000+i);									// read array of #'s for this anim
		if (hand == nil)
			DoFatalAlert("Error reading # keyframes/joint resource!");
		for (j=0; j < numJoints; j++)
			skeleton->JointKeyframes[j].numKeyFrames[i] = (*hand)[j];
		ReleaseResource(hand);
	}


	for (j=0; j < numJoints; j++)
	{
				/* ALLOC 2D ARRAY FOR KEYFRAMES */

		Alloc_2d_array(JointKeyframeType,skeleton->JointKeyframes[j].keyFrames,	numAnims,MAX_KEYFRAMES);

		if ((skeleton->JointKeyframes[j].keyFrames == nil) || (skeleton->JointKeyframes[j].keyFrames[0] == nil))
			DoFatalAlert("ReadDataFromSkeletonFile: Error allocating Keyframe Array.");

					/* READ THIS JOINT'S KF'S FOR EACH ANIM */

		for (i=0; i < numAnims; i++)
		{
			numKeyframes = skeleton->JointKeyframes[j].numKeyFrames[i];					// get actual # of keyframes for this joint
			if (numKeyframes > MAX_KEYFRAMES)
				DoFatalAlert("Error: numKeyframes > MAX_KEYFRAMES");

					/* READ A JOINT KEYFRAME */

			hand = GetResource('KeyF',1000+(i*100)+j);
			if (hand == nil)
				DoFatalAlert("Error reading joint keyframes resource!");
			keyFramePtr = (JointKeyframeType *)*hand;
			for (k = 0; k < numKeyframes; k++)												// copy this joint's keyframes for this anim
			{
				skeleton->JointKeyframes[j].keyFrames[i][k].tick				= SwizzleLong(&keyFramePtr->tick);
				skeleton->JointKeyframes[j].keyFrames[i][k].accelerationMode	= SwizzleLong(&keyFramePtr->accelerationMode);
				skeleton->JointKeyframes[j].keyFrames[i][k].coord.x				= SwizzleFloat(&keyFramePtr->coord.x);
				skeleton->JointKeyframes[j].keyFrames[i][k].coord.y				= SwizzleFloat(&keyFramePtr->coord.y);
				skeleton->JointKeyframes[j].keyFrames[i][k].coord.z				= SwizzleFloat(&keyFramePtr->coord.z);
				skeleton->JointKeyframes[j].keyFrames[i][k].rotation.x			= SwizzleFloat(&keyFramePtr->rotation.x);
				skeleton->JointKeyframes[j].keyFrames[i][k].rotation.y			= SwizzleFloat(&keyFramePtr->rotation.y);
				skeleton->JointKeyframes[j].keyFrames[i][k].rotation.z			= SwizzleFloat(&keyFramePtr->rotation.z);
				skeleton->JointKeyframes[j].keyFrames[i][k].scale.x				= SwizzleFloat(&keyFramePtr->scale.x);
				skeleton->JointKeyframes[j].keyFrames[i][k].scale.y				= SwizzleFloat(&keyFramePtr->scale.y);
				skeleton->JointKeyframes[j].keyFrames[i][k].scale.z				= SwizzleFloat(&keyFramePtr->scale.z);

				keyFramePtr++;
			}
			ReleaseResource(hand);
		}
	}

}

#pragma mark -



/******************** LOAD PREFS **********************/
//
// Load in standard preferences
//

OSErr LoadPrefs(PrefsType *prefBlock)
{
OSErr		iErr;
short		refNum;
FSSpec		file;
long		count;

				/*************/
				/* READ FILE */
				/*************/

#if DEMO
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":OttoMatic:DemoPreferences5", &file);
#else
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":OttoMatic:Preferences5", &file);
#endif
	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);
	if (iErr)
		return(iErr);

	count = sizeof(PrefsType);
	iErr = FSRead(refNum, &count,  (Ptr)prefBlock);		// read data from file
	if (iErr)
	{
		FSClose(refNum);
		return(iErr);
	}

	FSClose(refNum);

			/****************/
			/* VERIFY PREFS */
			/****************/

	if ((gGamePrefs.depth != 16) && (gGamePrefs.depth != 32))
		goto err;

		/* THEY'RE GOOD, SO ALSO RESTORE THE HID CONTROL SETTINGS */

	RestoreHIDControlSettings(&gGamePrefs.controlSettings);


	return(noErr);

err:
	InitDefaultPrefs();
	return(noErr);
}



/******************** SAVE PREFS **********************/

void SavePrefs(void)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;

		/* GET THE CURRENT CONTROL SETTINGS */

#if 0 // srcport rm
	if (!gHIDInitialized)								// can't save prefs unless HID is initialized!
		return;
#endif

	BuildHIDControlSettings(&gGamePrefs.controlSettings);


				/* CREATE BLANK FILE */

#if DEMO
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":OttoMatic:DemoPreferences5", &file);
#else
	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, ":OttoMatic:Preferences5", &file);
#endif
	FSpDelete(&file);															// delete any existing file
	iErr = FSpCreate(&file, 'Otto', 'Pref', smSystemScript);					// create blank file
	if (iErr)
		return;

				/* OPEN FILE */

	iErr = FSpOpenDF(&file, fsRdWrPerm, &refNum);
	if (iErr)
	{
		FSpDelete(&file);
		return;
	}

				/* WRITE DATA */

	count = sizeof(PrefsType);
	FSWrite(refNum, &count, &gGamePrefs);
	FSClose(refNum);
}

#pragma mark -




/**************** DRAW PICTURE INTO GWORLD ***********************/
//
// Uses Quicktime to load any kind of picture format file and draws
// it into the GWorld
//
//
// INPUT: myFSSpec = spec of image file
//
// OUTPUT:	theGWorld = gworld contining the drawn image.
//

OSErr DrawPictureIntoGWorld(FSSpec *myFSSpec, GWorldPtr *theGWorld, short depth)
{
	printf("DrawPictureIntoGWorld: %s\n", myFSSpec->cName);
	SOURCE_PORT_PLACEHOLDER();
	return unimpErr;
#if 0
OSErr						iErr;
GraphicsImportComponent		gi;
Rect						r;
ComponentResult				result;
PixMapHandle 				hPixMap;
FInfo						fndrInfo;

			/* VERIFY FILE - DEBUG */

	iErr = FSpGetFInfo(myFSSpec, &fndrInfo);
	if (iErr)
	{
		switch(iErr)
		{
			case	fnfErr:
					DoFatalAlert("DrawPictureIntoGWorld:  file is missing");
					break;

			default:
					DoFatalAlert("DrawPictureIntoGWorld:  file is invalid");
		}
	}


			/* PREP IMPORTER COMPONENT */

	result = GetGraphicsImporterForFile(myFSSpec, &gi);		// load importer for this image file
	if (result != noErr)
	{
		DoAlert("DrawPictureIntoGWorld: GetGraphicsImporterForFile failed!  You do not have Quicktime properly installed, reinstall Quicktime and do a FULL install.");
		return(result);
	}
	if (GraphicsImportGetBoundsRect(gi, &r) != noErr)		// get dimensions of image
		DoFatalAlert("DrawPictureIntoGWorld: GraphicsImportGetBoundsRect failed!");


			/* KEEP MUSIC PLAYING */

	if (gSongPlayingFlag && (!gMuteMusicFlag))
		SOURCE_PORT_MINOR_PLACEHOLDER(); //MoviesTask(gSongMovie, 0);


			/* MAKE GWORLD */

	iErr = NewGWorld(theGWorld, depth, &r, nil, nil, 0);					// try app mem
	if (iErr)
	{
		DoAlert("DrawPictureIntoGWorld: using temp mem");
		iErr = NewGWorld(theGWorld, depth, &r, nil, nil, useTempMem);		// try sys mem
		if (iErr)
		{
			DoAlert("DrawPictureIntoGWorld: MakeMyGWorld failed");
			return(1);
		}
	}

	if (depth == 32)
	{
		hPixMap = GetGWorldPixMap(*theGWorld);				// get gworld's pixmap
		(**hPixMap).cmpCount = 4;							// we want full 4-component argb (defaults to only rgb)
	}


			/* DRAW INTO THE GWORLD */

	DoLockPixels(*theGWorld);
	GraphicsImportSetGWorld(gi, *theGWorld, nil);			// set the gworld to draw image into
	GraphicsImportSetQuality(gi,codecLosslessQuality);		// set import quality

	result = GraphicsImportDraw(gi);						// draw into gworld
	CloseComponent(gi);										// cleanup
	if (result != noErr)
	{
		DoAlert("DrawPictureIntoGWorld: GraphicsImportDraw failed!");
		ShowSystemErr(result);
		DisposeGWorld (*theGWorld);
		*theGWorld= nil;
		return(result);
	}
	return(noErr);
#endif
}


#pragma mark -

/************************** LOAD LEVEL ART ***************************/

void LoadLevelArt(OGLSetupOutputType *setupInfo)
{
FSSpec	spec;

const char*	terrainFiles[NUM_LEVELS] =
{
	":Terrain:EarthFarm.ter",
	":Terrain:BlobWorld.ter",
	":Terrain:BlobBoss.ter",
	":Terrain:Apocalypse.ter",
	":Terrain:Cloud.ter",
	":Terrain:Jungle.ter",
	":Terrain:JungleBoss.ter",
	":Terrain:FireIce.ter",
	":Terrain:Saucer.ter",
	":Terrain:BrainBoss.ter",
};

const char*	levelModelFiles[NUM_LEVELS] =
{
	":Models:level1_farm.bg3d",
	":Models:level2_slime.bg3d",
	":Models:level3_blobboss.bg3d",
	":Models:level4_apocalypse.bg3d",
	":Models:level5_cloud.bg3d",
	":Models:level6_jungle.bg3d",
	":Models:level6_jungle.bg3d",
	":Models:level8_fireice.bg3d",
	":Models:level9_saucer.bg3d",
	":Models:level10_brainboss.bg3d",
};

const char*	levelSpriteFiles[NUM_LEVELS] =
{
	":Sprites:level1_farm.sprites",
	":Sprites:level2_slime.sprites",
	"",
	":Sprites:level4_apocalypse.sprites",
	":Sprites:level5_cloud.sprites",
	":Sprites:level6_jungle.sprites",
	":Sprites:level6_jungle.sprites",
	":Sprites:level8_fireice.sprites",
	"",
	":Sprites:level10_brainboss.sprites",
};


const char*	levelSoundFiles[NUM_LEVELS] =
{
	":Audio:Farm.sounds",
	":Audio:Slime.sounds",
	":Audio:Slime.sounds",
	":Audio:Apocalypse.sounds",
	":Audio:Cloud.sounds",
	":Audio:Jungle.sounds",
	":Audio:Jungle.sounds",
	":Audio:FireIce.sounds",
	":Audio:Saucer.sounds",
	":Audio:BrainBoss.sounds",
};



				/* LOAD AUDIO */


	if (levelSoundFiles[gLevelNum][0] > 0)
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, levelSoundFiles[gLevelNum], &spec);
		LoadSoundBank(&spec, SOUND_BANK_LEVELSPECIFIC);
	}


			/* LOAD BG3D GEOMETRY */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, levelModelFiles[gLevelNum], &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC, setupInfo);

	if (gG4)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_PowerupOrb,
									 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
	}



			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_OTTO, setupInfo);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_OTTO,
								 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

	LoadASkeleton(SKELETON_TYPE_BRAINALIEN, setupInfo);

	if (gG4)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_BRAINALIEN,
									 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
	}


	LoadASkeleton(SKELETON_TYPE_FARMER, setupInfo);
	LoadASkeleton(SKELETON_TYPE_BEEWOMAN, setupInfo);

#if !DEMO
	LoadASkeleton(SKELETON_TYPE_SCIENTIST, setupInfo);
	LoadASkeleton(SKELETON_TYPE_SKIRTLADY, setupInfo);
#endif

			/* LOAD LEVEL-SPECIFIC SKELETONS & APPLY REFLECTION MAPS */

	switch(gLevelNum)
	{
		case	LEVEL_NUM_FARM:
				LoadASkeleton(SKELETON_TYPE_ONION, setupInfo);
				if (gG4)
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_ONION,
												 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);

				LoadASkeleton(SKELETON_TYPE_CORN, setupInfo);
				if (gG4)
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_CORN,
												 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);


				LoadASkeleton(SKELETON_TYPE_TOMATO, setupInfo);
				if (gG4)
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_TOMATO,
												 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);


				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, FARM_ObjType_MetalGate,
											 	0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				break;

		case	LEVEL_NUM_BLOB:
				LoadASkeleton(SKELETON_TYPE_BLOB, setupInfo);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_BLOB,
											 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				LoadASkeleton(SKELETON_TYPE_SQUOOSHY, setupInfo);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_SQUOOSHY,
											 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				LoadASkeleton(SKELETON_TYPE_SLIMETREE, setupInfo);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_SLIMETREE,
											 0, 0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SetContainerMaterialFlags(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_SLIMETREE, 0, 1,			// no wrapping on the leaves
									 BG3D_MATERIALFLAG_CLAMP_U|BG3D_MATERIALFLAG_CLAMP_V);


				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SLIME_ObjType_BumperBubble,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SLIME_ObjType_CrystalCluster_Red,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SLIME_ObjType_CrystalCluster_Blue,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SLIME_ObjType_FallingCrystal_Red,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SLIME_ObjType_FallingCrystal_Blue,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SLIME_ObjType_BlobChunk,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SLIME_ObjType_SlimeTube_FancyJ,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SLIME_ObjType_SlimeTube_J,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SLIME_ObjType_SlimeTube_M,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SLIME_ObjType_SlimeTube_FancyShort,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SLIME_ObjType_SlimeTube_T,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SLIME_ObjType_SlimeTube_Grate,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SLIME_ObjType_SlimeTube_Valve,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				break;

		case	LEVEL_NUM_BLOBBOSS:
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_Tube_Bent,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_Tube_Loop1,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_Tube_Loop2,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_Tube_Straight1,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_Tube_Straight2,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_Blobule,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, BLOBBOSS_ObjType_BlobDroplet,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				break;

		case	LEVEL_NUM_APOCALYPSE:
				LoadASkeleton(SKELETON_TYPE_PODWORM, setupInfo);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_PODWORM,
											 0, 0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				LoadASkeleton(SKELETON_TYPE_MUTANT, setupInfo);
				LoadASkeleton(SKELETON_TYPE_MUTANTROBOT, setupInfo);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_MUTANTROBOT,
											 0, 0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);

				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, APOCALYPSE_ObjType_CrashedRocket,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, APOCALYPSE_ObjType_CrashedSaucer,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);
				break;


		case	LEVEL_NUM_CLOUD:
				LoadASkeleton(SKELETON_TYPE_CLOWN, setupInfo);
				LoadASkeleton(SKELETON_TYPE_CLOWNFISH, setupInfo);
				LoadASkeleton(SKELETON_TYPE_STRONGMAN, setupInfo);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_CLOWNFISH,
											 0, 0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				if (gG4)
				{
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, CLOUD_ObjType_BumperCar,
												 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, CLOUD_ObjType_ClownBumperCar,
												 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				}
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, CLOUD_ObjType_ClownBubble,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, CLOUD_ObjType_Cannon,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, CLOUD_ObjType_BrassPost,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_GreenSheen);
				break;


		case	LEVEL_NUM_JUNGLE:
				LoadASkeleton(SKELETON_TYPE_GIANTLIZARD, setupInfo);
				LoadASkeleton(SKELETON_TYPE_FLYTRAP, setupInfo);
				LoadASkeleton(SKELETON_TYPE_MANTIS, setupInfo);
				if (gG4)
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_MANTIS,
											 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				LoadASkeleton(SKELETON_TYPE_TURTLE, setupInfo);
				if (gG4)
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_TURTLE,
											 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				break;

		case	LEVEL_NUM_JUNGLEBOSS:
				LoadASkeleton(SKELETON_TYPE_TURTLE, setupInfo);
				LoadASkeleton(SKELETON_TYPE_FLYTRAP, setupInfo);
				LoadASkeleton(SKELETON_TYPE_PITCHERPLANT, setupInfo);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_PITCHERPLANT,
											 0, 0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_GreenSheen);

				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_TURTLE,
											 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				if (gG4)
				{
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, JUNGLE_ObjType_PitcherPod_Pod,
												 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_GreenSheen);
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, JUNGLE_ObjType_PitcherPod_Stem,
												 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_GreenSheen);
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, JUNGLE_ObjType_TentacleGenerator,
												 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_GreenSheen);

					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, JUNGLE_ObjType_LeafPlatform0,
												 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_GreenSheen);
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, JUNGLE_ObjType_LeafPlatform1,
												 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_GreenSheen);
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, JUNGLE_ObjType_LeafPlatform2,
												 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_GreenSheen);
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, JUNGLE_ObjType_LeafPlatform3,
												 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_GreenSheen);
				}
				break;

		case	LEVEL_NUM_FIREICE:
				LoadASkeleton(SKELETON_TYPE_FLAMESTER, setupInfo);
				LoadASkeleton(SKELETON_TYPE_ICECUBE, setupInfo);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_ICECUBE,
											 0, 0, MULTI_TEXTURE_COMBINE_MODULATE, SPHEREMAP_SObjType_Tundra);
				LoadASkeleton(SKELETON_TYPE_SQUOOSHY, setupInfo);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_SQUOOSHY,
											 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, FIREICE_ObjType_SwingerBot_Body,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);

				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, FIREICE_ObjType_JawsBot_Body,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);


				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, FIREICE_ObjType_Icicle,
											 	-1, MULTI_TEXTURE_COMBINE_MODULATE, SPHEREMAP_SObjType_Tundra);


				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, FIREICE_ObjType_Blobule,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, FIREICE_ObjType_BlobDroplet,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, FIREICE_ObjType_Saucer,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sea);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, FIREICE_ObjType_Hatch,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sea);

				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, FIREICE_ObjType_Peoplecicle,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sea);

				break;



		case	LEVEL_NUM_SAUCER:
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SAUCER_ObjType_Saucer,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Sea);

				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SAUCER_ObjType_Dish,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SAUCER_ObjType_PeopleHut,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SAUCER_ObjType_RailGun,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SAUCER_ObjType_TurretBase,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SAUCER_ObjType_Turret,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, SAUCER_ObjType_Beemer,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				break;

		case	LEVEL_NUM_BRAINBOSS:
				LoadASkeleton(SKELETON_TYPE_ELITEBRAINALIEN, setupInfo);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_ELITEBRAINALIEN,
											 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, BRAINBOSS_ObjType_LeftBrain,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, BRAINBOSS_ObjType_BrainPort,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				break;

	}

	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_TeleportBase,
								 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);





			/* LOAD SPRITES */

	if (levelSpriteFiles[gLevelNum][0] > 0)
	{
		FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, levelSpriteFiles[gLevelNum], &spec);
		LoadSpriteFile(&spec, SPRITE_GROUP_LEVELSPECIFIC, setupInfo);
	}

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:infobar.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_INFOBAR, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:fence.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FENCES, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:global.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_GLOBAL, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:spheremap.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS, setupInfo);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:helpfont.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FONT, setupInfo);


			/* LOAD TERRAIN */
			//
			// must do this after creating the view!
			//

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, terrainFiles[gLevelNum], &spec);
	LoadPlayfield(&spec, setupInfo);

}


#pragma mark -

/******************* LOAD PLAYFIELD *******************/

void LoadPlayfield(FSSpec *specPtr, OGLSetupOutputType *setupInfo)
{


			/* READ PLAYFIELD RESOURCES */

	ReadDataFromPlayfieldFile(specPtr, setupInfo);




				/***********************/
				/* DO ADDITIONAL SETUP */
				/***********************/

	CreateSuperTileMemoryList();				// allocate memory for the supertile geometry
	CalculateSplitModeMatrix();					// precalc the tile split mode matrix
	InitSuperTileGrid();						// init the supertile state grid

	BuildTerrainItemList();						// build list of items & find player start coords
}


/********************** READ DATA FROM PLAYFIELD FILE ************************/

static void ReadDataFromPlayfieldFile(FSSpec *specPtr, OGLSetupOutputType *setupInfo)
{
Handle					hand;
PlayfieldHeaderType		**header;
long					row,col,j,i,size;
float					yScale;
float					*src;
short					fRefNum;
OSErr					iErr;
Ptr						tempBuffer16 = nil,tempBuffer24 = nil, tempBuffer32 = nil;

				/* OPEN THE REZ-FORK */

	fRefNum = FSpOpenResFile(specPtr,fsRdPerm);
	GAME_ASSERT_MESSAGE(fRefNum != -1, "FSpOpenResFile failed");
	UseResFile(fRefNum);


			/************************/
			/* READ HEADER RESOURCE */
			/************************/

	hand = GetResource('Hedr',1000);
	GAME_ASSERT_MESSAGE(hand, "Error reading header resource!");

	header = (PlayfieldHeaderType **)hand;
	gNumTerrainItems		= SwizzleLong(&(**header).numItems);
	gTerrainTileWidth		= SwizzleLong(&(**header).mapWidth);
	gTerrainTileDepth		= SwizzleLong(&(**header).mapHeight);
	g3DTileSize				= SwizzleFloat(&(**header).tileSize);
	g3DMinY					= SwizzleFloat(&(**header).minY);
	g3DMaxY					= SwizzleFloat(&(**header).maxY);
	gNumSplines				= SwizzleLong(&(**header).numSplines);
	gNumFences				= SwizzleLong(&(**header).numFences);
	gNumWaterPatches		= SwizzleLong(&(**header).numWaterPatches);
	gNumUniqueSuperTiles	= SwizzleLong(&(**header).numUniqueSuperTiles);
	ReleaseResource(hand);

	if ((gTerrainTileWidth % SUPERTILE_SIZE) != 0)		// terrain must be non-fractional number of supertiles in w/h
		DoFatalAlert("ReadDataFromPlayfieldFile: terrain width not a supertile multiple");
	if ((gTerrainTileDepth % SUPERTILE_SIZE) != 0)
		DoFatalAlert("ReadDataFromPlayfieldFile: terrain depth not a supertile multiple");


				/* CALC SOME GLOBALS HERE */

	gTerrainTileWidth = (gTerrainTileWidth/SUPERTILE_SIZE)*SUPERTILE_SIZE;		// round size down to nearest supertile multiple
	gTerrainTileDepth = (gTerrainTileDepth/SUPERTILE_SIZE)*SUPERTILE_SIZE;
	gTerrainUnitWidth = gTerrainTileWidth*TERRAIN_POLYGON_SIZE;					// calc world unit dimensions of terrain
	gTerrainUnitDepth = gTerrainTileDepth*TERRAIN_POLYGON_SIZE;
	gNumSuperTilesDeep = gTerrainTileDepth/SUPERTILE_SIZE;						// calc size in supertiles
	gNumSuperTilesWide = gTerrainTileWidth/SUPERTILE_SIZE;


			/**************************/
			/* TILE RELATED RESOURCES */
			/**************************/


			/* READ TILE ATTRIBUTES */

	hand = GetResource('Atrb',1000);
	GAME_ASSERT_MESSAGE(hand, "Error reading tile attrib resource!");
	{
		DetachResource(hand);
		HLockHi(hand);
		gTileAttribList = (TileAttribType **)hand;
	}


			/*******************************/
			/* SUPERTILE RELATED RESOURCES */
			/*******************************/

			/* READ SUPERTILE GRID MATRIX */

	if (gSuperTileTextureGrid)														// free old array
		Free_2d_array(gSuperTileTextureGrid);
	Alloc_2d_array(SuperTileGridType, gSuperTileTextureGrid, gNumSuperTilesDeep, gNumSuperTilesWide);

	hand = GetResource('STgd',1000);												// load grid from rez
	GAME_ASSERT_MESSAGE(hand, "Error reading supertile rez resource!");
	{																							// copy rez into 2D array
		SuperTileGridType *src = (SuperTileGridType *)*hand;

		for (row = 0; row < gNumSuperTilesDeep; row++)
			for (col = 0; col < gNumSuperTilesWide; col++)
			{
				gSuperTileTextureGrid[row][col].isEmpty = src->isEmpty;
				gSuperTileTextureGrid[row][col].superTileID = SwizzleUShort(&src->superTileID);
				src++;
			}
		ReleaseResource(hand);
	}


			/*******************************/
			/* MAP LAYER RELATED RESOURCES */
			/*******************************/


			/* READ MAP DATA LAYER */
			//
			// The only thing we need this for is a quick way to
			// look up tile attributes
			//

	{
		u_short	*src;

		hand = GetResource('Layr',1000);
		GAME_ASSERT_MESSAGE(hand, "Error reading map layer rez");
		{
			if (gTileGrid)														// free old array
				Free_2d_array(gTileGrid);
			Alloc_2d_array(u_short, gTileGrid, gTerrainTileDepth, gTerrainTileWidth);

			src = (u_short *)*hand;
			for (row = 0; row < gTerrainTileDepth; row++)
			{
				for (col = 0; col < gTerrainTileWidth; col++)
				{
					gTileGrid[row][col] = SwizzleUShort(src);
					src++;
				}
			}
			ReleaseResource(hand);
		}
	}


			/* READ HEIGHT DATA MATRIX */

	yScale = TERRAIN_POLYGON_SIZE / g3DTileSize;											// need to scale original geometry units to game units

	Alloc_2d_array(float, gMapYCoords, gTerrainTileDepth+1, gTerrainTileWidth+1);			// alloc 2D array for map
	Alloc_2d_array(float, gMapYCoordsOriginal, gTerrainTileDepth+1, gTerrainTileWidth+1);	// and the copy of it

	hand = GetResource('YCrd',1000);
	GAME_ASSERT_MESSAGE(hand, "Error reading height data resource!");
	{
		src = (float *)*hand;
		for (row = 0; row <= gTerrainTileDepth; row++)
			for (col = 0; col <= gTerrainTileWidth; col++)
				gMapYCoordsOriginal[row][col] = gMapYCoords[row][col] = SwizzleFloat(src++) * yScale;
		ReleaseResource(hand);
	}

				/**************************/
				/* ITEM RELATED RESOURCES */
				/**************************/

				/* READ ITEM LIST */

	hand = GetResource('Itms',1000);
	GAME_ASSERT_MESSAGE(hand, "Error reading itemlist resource!");
	{
		TerrainItemEntryType   *rezItems;

		DetachResource(hand);							// lets keep this data around
		HLockHi(hand);									// LOCK this one because we have the lookup table into this
		gMasterItemList = (TerrainItemEntryType **)hand;
		rezItems = (TerrainItemEntryType *)*hand;

					/* CONVERT COORDINATES */

		for (i = 0; i < gNumTerrainItems; i++)
		{
			(*gMasterItemList)[i].x = SwizzleULong(&rezItems[i].x) * MAP2UNIT_VALUE;								// convert coordinates
			(*gMasterItemList)[i].y = SwizzleULong(&rezItems[i].y) * MAP2UNIT_VALUE;

			(*gMasterItemList)[i].type = SwizzleUShort(&rezItems[i].type);
			(*gMasterItemList)[i].parm[0] = rezItems[i].parm[0];
			(*gMasterItemList)[i].parm[1] = rezItems[i].parm[1];
			(*gMasterItemList)[i].parm[2] = rezItems[i].parm[2];
			(*gMasterItemList)[i].parm[3] = rezItems[i].parm[3];
			(*gMasterItemList)[i].flags = SwizzleUShort(&rezItems[i].flags);
		}
	}




			/****************************/
			/* SPLINE RELATED RESOURCES */
			/****************************/

			/* READ SPLINE LIST */

	hand = GetResource('Spln',1000);
	if (hand)
	{
		File_SplineDefType* splinePtr = (File_SplineDefType *)*hand;

		DetachResource(hand);
		HLockHi(hand);
		gSplineList = (SplineDefType **) NewHandleClear(gNumSplines * sizeof(SplineDefType));

		for (i = 0; i < gNumSplines; i++)
		{
			(*gSplineList)[i].numNubs = SwizzleShort(&splinePtr[i].numNubs);
			(*gSplineList)[i].numPoints = SwizzleLong(&splinePtr[i].numPoints);
			(*gSplineList)[i].numItems = SwizzleShort(&splinePtr[i].numItems);

			(*gSplineList)[i].bBox.top = SwizzleShort(&splinePtr[i].bBox.top);
			(*gSplineList)[i].bBox.bottom = SwizzleShort(&splinePtr[i].bBox.bottom);
			(*gSplineList)[i].bBox.left = SwizzleShort(&splinePtr[i].bBox.left);
			(*gSplineList)[i].bBox.right = SwizzleShort(&splinePtr[i].bBox.right);
		}

	}
	else
		gNumSplines = 0;

			/* READ SPLINE POINT LIST */

	for (i = 0; i < gNumSplines; i++)
	{
		SplineDefType	*spline = &(*gSplineList)[i];									// point to Nth spline

		// (Bugdom's) Level 2's spline #16 has 0 points. Skip the byteswapping, but do alloc an empty handle, which the game expects.
		GAME_ASSERT(spline->numPoints != 0);

		hand = GetResource('SpPt',1000+i);
		GAME_ASSERT_MESSAGE(hand, "can't get spline points rez");
		{
			SplinePointType	*ptList = (SplinePointType *)*hand;

			DetachResource(hand);
			HLockHi(hand);
			(*gSplineList)[i].pointList = (SplinePointType **)hand;

			for (j = 0; j < spline->numPoints; j++)			// swizzle
			{
				(*spline->pointList)[j].x = SwizzleFloat(&ptList[j].x);
				(*spline->pointList)[j].z = SwizzleFloat(&ptList[j].z);
			}
		}
	}


			/* READ SPLINE ITEM LIST */

	for (i = 0; i < gNumSplines; i++)
	{
		SplineDefType	*spline = &(*gSplineList)[i];									// point to Nth spline

		hand = GetResource('SpIt',1000+i);
		GAME_ASSERT_MESSAGE(hand, "ReadDataFromPlayfieldFile: cant get spline items rez");
		{
			SplineItemType	*itemList = (SplineItemType *)*hand;

			DetachResource(hand);
			HLockHi(hand);
			(*gSplineList)[i].itemList = (SplineItemType **)hand;

			for (j = 0; j < spline->numItems; j++)			// swizzle
			{
				(*spline->itemList)[j].placement = SwizzleFloat(&itemList[j].placement);
				(*spline->itemList)[j].type	= SwizzleUShort(&itemList[j].type);
				(*spline->itemList)[j].flags	= SwizzleUShort(&itemList[j].flags);
			}
		}
	}

			/****************************/
			/* FENCE RELATED RESOURCES */
			/****************************/

			/* READ FENCE LIST */

	hand = GetResource('Fenc',1000);
	if (hand)
	{
		FileFenceDefType *inData;

		gFenceList = (FenceDefType *)AllocPtr(sizeof(FenceDefType) * gNumFences);	// alloc new ptr for fence data
		GAME_ASSERT(gFenceList);

		inData = (FileFenceDefType *)*hand;								// get ptr to input fence list

		for (i = 0; i < gNumFences; i++)								// copy data from rez to new list
		{
			gFenceList[i].type 		= SwizzleUShort(&inData[i].type);
			gFenceList[i].numNubs 	= SwizzleShort(&inData[i].numNubs);
			gFenceList[i].nubList 	= nil;
			gFenceList[i].sectionVectors = nil;
			gFenceList[i].sectionNormals = nil;
		}
		ReleaseResource(hand);
	}
	else
		gNumFences = 0;


			/* READ FENCE NUB LIST */

	for (i = 0; i < gNumFences; i++)
	{
		hand = GetResource('FnNb',1000+i);					// get rez
		GAME_ASSERT_MESSAGE(hand, "cant get fence nub rez");
		HLock(hand);
		{
   			FencePointType *fileFencePoints = (FencePointType *)*hand;

			gFenceList[i].nubList = (OGLPoint3D *)AllocPtr(sizeof(FenceDefType) * gFenceList[i].numNubs);	// alloc new ptr for nub array
			GAME_ASSERT(gFenceList[i].nubList);

			for (j = 0; j < gFenceList[i].numNubs; j++)		// convert x,z to x,y,z
			{
				gFenceList[i].nubList[j].x = SwizzleLong(&fileFencePoints[j].x);
				gFenceList[i].nubList[j].z = SwizzleLong(&fileFencePoints[j].z);
				gFenceList[i].nubList[j].y = 0;
			}
			ReleaseResource(hand);
		}
	}


			/****************************/
			/* WATER RELATED RESOURCES */
			/****************************/

			/* READ WATER LIST */

	hand = GetResource('Liqd',1000);
	if (hand)
	{
		DetachResource(hand);
		HLockHi(hand);
		gWaterListHandle = (WaterDefType **)hand;
		gWaterList = *gWaterListHandle;

		for (i = 0; i < gNumWaterPatches; i++)						// swizzle
		{
			gWaterList[i].type = SwizzleUShort(&gWaterList[i].type);
			gWaterList[i].flags = SwizzleULong(&gWaterList[i].flags);
			gWaterList[i].height = SwizzleLong(&gWaterList[i].height);
			gWaterList[i].numNubs = SwizzleShort(&gWaterList[i].numNubs);

			gWaterList[i].hotSpotX = SwizzleFloat(&gWaterList[i].hotSpotX);
			gWaterList[i].hotSpotZ = SwizzleFloat(&gWaterList[i].hotSpotZ);

			gWaterList[i].bBox.top = SwizzleShort(&gWaterList[i].bBox.top);
			gWaterList[i].bBox.bottom = SwizzleShort(&gWaterList[i].bBox.bottom);
			gWaterList[i].bBox.left = SwizzleShort(&gWaterList[i].bBox.left);
			gWaterList[i].bBox.right = SwizzleShort(&gWaterList[i].bBox.right);

			for (j = 0; j < gWaterList[i].numNubs; j++)
			{
				gWaterList[i].nubList[j].x = SwizzleFloat(&gWaterList[i].nubList[j].x);
				gWaterList[i].nubList[j].y = SwizzleFloat(&gWaterList[i].nubList[j].y);
			}
		}


	}
	else
		gNumWaterPatches = 0;


			/* CLOSE REZ FILE */

	CloseResFile(fRefNum);
	UseResFile(gMainAppRezFile);




		/********************************************/
		/* READ SUPERTILE IMAGE DATA FROM DATA FORK */
		/********************************************/


				/* ALLOC BUFFERS */

	size = SUPERTILE_TEXMAP_SIZE * SUPERTILE_TEXMAP_SIZE * 2;						// calc size of supertile 16-bit texture
	tempBuffer16 = AllocPtr(size);
	if (tempBuffer16 == nil)
		DoFatalAlert("ReadDataFromPlayfieldFile: AllocPtr failed!");

	tempBuffer24 = AllocPtr(SUPERTILE_TEXMAP_SIZE * SUPERTILE_TEXMAP_SIZE * 3);		// alloc for 24bit pixels
	if (tempBuffer24 == nil)
		DoFatalAlert("ReadDataFromPlayfieldFile: AllocPtr failed!");

	tempBuffer32 = AllocPtr(SUPERTILE_TEXMAP_SIZE * SUPERTILE_TEXMAP_SIZE * 4);		// alloc for 32bit pixels
	if (tempBuffer32 == nil)
		DoFatalAlert("ReadDataFromPlayfieldFile: AllocPtr failed!");



				/* OPEN THE DATA FORK */

	iErr = FSpOpenDF(specPtr, fsRdPerm, &fRefNum);
	if (iErr)
		DoFatalAlert("ReadDataFromPlayfieldFile: FSpOpenDF failed!");



	for (i = 0; i < gNumUniqueSuperTiles; i++)
	{
		long	width,height;
		MOMaterialData	matData;
		int		x,y;
		u_short	*src,*dest;


				/* READ THE SIZE OF THE NEXT COMPRESSED SUPERTILE TEXTURE */

		int32_t compressedSize = FSReadBELong(fRefNum);


				/* READ & DECOMPRESS IT */

		long decompressedSize = LZSS_Decode(fRefNum, tempBuffer16, compressedSize);
		GAME_ASSERT_MESSAGE(decompressedSize == size, "LZSS_Decode size is wrong!");

				/* IF LOW MEM MODE THEN SHRINK THE TERRAIN TEXTURES IN HALF */

		if (gLowMemMode)
		{
			dest = src = (u_short *)tempBuffer16;

			for (y = 0; y < SUPERTILE_TEXMAP_SIZE; y+=2)
			{
				for (x = 0; x < SUPERTILE_TEXMAP_SIZE; x+=2)
				{
					*dest++ = src[x];
				}
				src += SUPERTILE_TEXMAP_SIZE*2;
			}

			width = SUPERTILE_TEXMAP_SIZE/2;
			height = SUPERTILE_TEXMAP_SIZE/2;
		}
		else
		{
			width = SUPERTILE_TEXMAP_SIZE;
			height = SUPERTILE_TEXMAP_SIZE;
		}


				/**************************/
				/* CREATE MATERIAL OBJECT */
				/**************************/


			/* USE PACKED PIXEL TYPE */

		ConvertTexture16To16((uint16_t *)tempBuffer16, width, height);
		matData.pixelSrcFormat 	= GL_BGRA_EXT;
		matData.pixelDstFormat 	= GL_RGBA;
		matData.textureName[0] 	= OGL_TextureMap_Load(tempBuffer16, width, height,
												 GL_BGRA_EXT, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV);

			/* INIT NEW MATERIAL DATA */

		matData.setupInfo				= setupInfo;								// remember which draw context this material is assigned to
		matData.flags 					= 	BG3D_MATERIALFLAG_CLAMP_U|
											BG3D_MATERIALFLAG_CLAMP_V|
											BG3D_MATERIALFLAG_TEXTURED;

		switch(gLevelNum)
		{
			case	LEVEL_NUM_BLOBBOSS:
					if (gG4)
					{
						matData.flags |= BG3D_MATERIALFLAG_MULTITEXTURE;
						matData.envMapNum	= SPHEREMAP_SObjType_Red;
					}
					break;

		}

		matData.multiTextureMode		= MULTI_TEXTURE_MODE_REFLECTIONSPHERE;
		matData.multiTextureCombine		= MULTI_TEXTURE_COMBINE_ADD;
		matData.diffuseColor.r			= 1;
		matData.diffuseColor.g			= 1;
		matData.diffuseColor.b			= 1;
		matData.diffuseColor.a			= 1;
		matData.numMipmaps				= 1;										// 1 texture
		matData.width					= width;
		matData.height					= height;
		matData.texturePixels[0] 		= nil;										// the original pixels are gone (or will be soon)
		gSuperTileTextureObjects[i] 	= MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);		// create the new object
	}

			/* CLOSE THE FILE */

	FSClose(fRefNum);
	if (tempBuffer16)
		SafeDisposePtr(tempBuffer16);
	if (tempBuffer24)
		SafeDisposePtr(tempBuffer24);
	if (tempBuffer32)
		SafeDisposePtr(tempBuffer32);
}






/*********************** CONVERT TEXTURE; 16 TO 16 ***********************************/
//
// Simply flips Y since OGL Textures are screwey
//

static void	ConvertTexture16To16(u_short *textureBuffer, int width, int height)
{
int		x,y;
u_short	pixel,*bottom;
u_short	*dest;
Boolean	blackOpaq;

	blackOpaq = (gLevelNum != LEVEL_NUM_CLOUD);			// make black transparent on cloud

	bottom = textureBuffer + ((height - 1) * width);

	for (y = 0; y < height / 2; y++)
	{
		dest = bottom;

		for (x = 0; x < width; x++)
		{
			pixel = textureBuffer[x];						// get 16bit pixel from top
#if __BIG_ENDIAN__
			if ((pixel & 0x7fff) || blackOpaq)				// check/set alpha
				pixel |= 0x8000;
			else
				pixel &= 0x7fff;
#else
			pixel = SwizzleUShort(&pixel);

			if ((pixel & 0x7fff) || blackOpaq)				// check/set alpha
				pixel |= 0x8000;
			else
				pixel &= 0x7fff;
#endif

			textureBuffer[x] = bottom[x];					// copy bottom to top
#if __BIG_ENDIAN__
			if ((textureBuffer[x] & 0x7fff) || blackOpaq)				// check/set alpha
				textureBuffer[x] |= 0x8000;
			else
				textureBuffer[x] &= 0x7fff;
#else
			textureBuffer[x] = SwizzleUShort(&textureBuffer[x]);
			if ((textureBuffer[x] & 0x7fff) || blackOpaq)				// check/set alpha
				textureBuffer[x] |= 0x8000;
			else
				textureBuffer[x] &= 0x7fff;
#endif

			bottom[x] = pixel;								// save top into bottom
		}

		textureBuffer += width;
		bottom -= width;
	}
}


#pragma mark -


/************************ NAV SERVICES:  EVENT PROC *****************************/

#if 0	// srcport rm
pascal void myEventProc(NavEventCallbackMessage callBackSelector, NavCBRecPtr callBackParms,
						NavCallBackUserData callBackUD)
{

#pragma unused (callBackUD)


	switch (callBackSelector)
	{
		case	kNavCBEvent:
				switch (((callBackParms->eventData).eventDataParms).event->what)
				{
					case 	updateEvt:
							break;
				}
				break;
	}
}





/******************** NAV SERVICES: GET DOCUMENT ***********************/

static OSErr GetFileWithNavServices(const FSSpecPtr defaultLocationfssPtr, FSSpec *documentFSSpec)
{
NavDialogOptions 	dialogOptions;
AEDesc 				defaultLocation;
NavEventUPP 		eventProc 	= NewNavEventUPP(myEventProc);
NavObjectFilterUPP filterProc 	= nil; //NewNavObjectFilterUPP(myFilterProc);
OSErr 				anErr 		= noErr;

#if CUSTOM
NavDialogRef				navRef;
NavDialogCreationOptions	createOption;
#endif
			/* Specify default options for dialog box */

	anErr = NavGetDefaultDialogOptions(&dialogOptions);
	if (anErr == noErr)
	{
			/* Adjust the options to fit our needs */

		dialogOptions.dialogOptionFlags |= kNavSelectDefaultLocation;	// Set default location option
		dialogOptions.dialogOptionFlags ^= kNavAllowPreviews;			// Clear preview option
		dialogOptions.dialogOptionFlags ^= kNavAllowMultipleFiles;		// Clear multiple files option


				/* make descriptor for default location */

		anErr = AECreateDesc(typeFSS,defaultLocationfssPtr, sizeof(*defaultLocationfssPtr), &defaultLocation);
//		if (anErr ==noErr)
		{
			/* Get 'open'resource.  A nil handle being returned is OK, this simply means no automatic file filtering. */

			static NavTypeList			typeList = {'Otto', 0, 1, 'OSav'};	// set types to filter

			NavTypeListPtr 	typeListPtr = &typeList;	// = (NavTypeListHandle)GetResource('open',128);
			NavReplyRecord 		reply;


			/* Call NavGetFile() with specified options and declare our app-defined functions and type list */

			anErr = NavGetFile(&defaultLocation, &reply, &dialogOptions, eventProc, nil, filterProc, &typeListPtr,nil);
			if ((anErr == noErr) && (reply.validRecord))
			{
					/* Deal with multiple file selection */

				long 	count;

				anErr = AECountItems(&(reply.selection),&count);


					/* Set up index for file list */

				if (anErr == noErr)
				{
					long i;

					for (i = 1; i <= count; i++)
					{
						AEKeyword 	theKeyword;
						DescType 	actualType;
						Size 		actualSize;

						/* Get a pointer to selected file */

						anErr = AEGetNthPtr(&(reply.selection), i, typeFSS,&theKeyword, &actualType,
											documentFSSpec, sizeof(FSSpec), &actualSize);
					}
				}


				/* Dispose of NavReplyRecord,resources,descriptors */

				anErr = NavDisposeReply(&reply);
			}

			(void)AEDisposeDesc(&defaultLocation);
		}
	}


		/* CLEAN UP */

	if (eventProc)
	{
		DisposeNavEventUPP(eventProc);
		eventProc = nil;
	}
	if (filterProc)
	{
		DisposeNavObjectFilterUPP(filterProc);
		filterProc = nil;
	}

#if CUSTOM
	NavDialogDispose(navRef);
#endif
	return anErr;
}


/********************** PUT FILE WITH NAV SERVICES *************************/

static OSErr PutFileWithNavServices(NavReplyRecord *reply, FSSpec *outSpec)
{
OSErr 				anErr 			= noErr;
NavDialogOptions 	dialogOptions;
OSType 				fileTypeToSave 	='PSav';
OSType 				creatorType 	='Otto';
NavEventUPP 		eventProc = nil; //NewNavEventUPP(myEventProc);
Str255				name = "Otto Saved Game";

	anErr = NavGetDefaultDialogOptions(&dialogOptions);
	if (anErr == noErr)
	{
		CopyPStr(name, dialogOptions.savedFileName);					// set default name

		anErr = NavPutFile(nil, reply, &dialogOptions, nil, fileTypeToSave, creatorType, nil);
		if ((anErr == noErr) && (reply->validRecord))
		{
			AEKeyword	theKeyword;
			DescType 	actualType;
			Size 		actualSize;

			anErr = AEGetNthPtr(&(reply->selection),1,typeFSS, &theKeyword,&actualType, outSpec, sizeof(FSSpec), &actualSize);
		}
	}

	if (eventProc)
	{
		DisposeNavEventUPP(eventProc);
		eventProc = nil;
	}

	return anErr;
}






#pragma mark -

/******************** GET LIBRARY VERSION *******************/
//
// Returns the version # of the input extension/library (used to get OpenGL version #)
//

void GetLibVersion(ConstStrFileNameParam libName, NumVersion *version)
{
	FSSpec				spec;
	UInt32				versCode;
//	char				versStr[32];

	if (FindSharedLib(libName, &spec))
	{
		versCode = GetFileVersion(&spec);
//		VersionCodeToText(versCode, versStr);

//		printf("Shared lib '%#s' found!\n", libName);
//		printf("    Filename is '%#s'\n", spec.name);
//		printf("    Version code is: 0x%08X\n", versCode);
//		printf("    Version str  is: %s\n", versStr);
	}
	else
	{
		DoAlert("GetLibVersion: lib not found");
	}

	version->majorRev 		= (versCode & 0xff000000) >> 24;
	version->minorAndBugRev = (versCode & 0x00ff0000) >> 16;
	version->stage 			= 0;
	version->nonRelRev 		= 0;
}


static UInt32	GetFileVersion(FSSpec *spec)
{
	SInt16						fRefNum, saveRefNum;
	Handle						versH;
	UInt32						versCode;

	saveRefNum = CurResFile();
	fRefNum = FSpOpenResFile(spec, fsRdPerm);
	if (fRefNum == -1)
		return 0;

	if ((versH = Get1Resource('vers', 1)) == NULL)
		versH = Get1Resource('vers', 2);

	if (versH)
		versCode = *(UInt32 *) (*versH);
	else
		versCode = 0;

	CloseResFile(fRefNum);
	UseResFile(saveRefNum);

	return versCode;
}

#if 0
static void VersionCodeToText(UInt32 versCode, char *versStr)
{
	char			temp[128];

	if (!versCode) {
		versStr[0] = '?';
		versStr[1] = 0;
	}
	else {
		sprintf(versStr, "%d.%d", (versCode & 0x0F000000) >> 24, (versCode & 0x00F00000) >> 20);
		if (versCode & 0x000F0000) {
			sprintf(temp, ".%d", (versCode & 0x000F0000) >> 16);
			strcat(versStr, temp);
		}
		if ((versCode & 0x0000FFFF) != 0x8000) {
			char		c;

			switch (versCode & 0xFF00) {
				case 0x8000:
					c = 'f';
					break;
				case 0x6000:
					c = 'b';
					break;
				case 0x4000:
					c = 'a';
					break;
				default:
					c = 'd';
					break;
			}
			sprintf(temp, "%c%d", c, (versCode & 0xFF));
			strcat(versStr, temp);
		}
	}
}
#endif

static Boolean MatchSharedLib(ConstStrFileNameParam libName, FSSpec *spec)
{
	SInt16						fRefNum, saveRefNum, i;
	Boolean						result;
	CFragResourcePtr			*cfrg;
	CFragResourceMemberPtr		member;

	saveRefNum = CurResFile();
	fRefNum = FSpOpenResFile(spec, fsRdPerm);
	if (fRefNum == -1)
		return false;

	result = false;
	cfrg = (CFragResourcePtr *) Get1Resource(kCFragResourceType, kCFragResourceID);
	if (cfrg && (**cfrg).memberCount) {
		member = &(**cfrg).firstMember;
		for (i=0; i<(**cfrg).memberCount; i++) {
			if (EqualString(libName, member->name, false, true)) {
				result = true;
				break;
			}
			member = (CFragResourceMemberPtr) (((char *) member) + member->memberSize);
		}
	}

	CloseResFile(fRefNum);
	UseResFile(saveRefNum);
	return result;
}


static Boolean FindSharedLib(ConstStrFileNameParam libName, FSSpec *spec)
{
	SInt16						extVRefNum;
	SInt32						extDirID;
	StrFileName					fName;
	SInt32						fIndex;
	OSErr						err;
	CInfoPBRec					cpb;
	HFileInfo					*fpb = (HFileInfo *) &cpb;

	if (FindFolder(kOnSystemDisk, kExtensionFolderType, kDontCreateFolder, &extVRefNum, &extDirID) != noErr)
		return false;

	if (FSMakeFSSpec(extVRefNum, extDirID, libName, spec) == noErr)
		return true;

	fpb->ioVRefNum = extVRefNum;
	fpb->ioNamePtr = fName;
	fIndex = 1;
	do {
		fpb->ioDirID = extDirID;
		fpb->ioFDirIndex = fIndex;

		err = PBGetCatInfoSync(&cpb);
		if (err == noErr) {
			if (!(fpb->ioFlAttrib & 16) && fpb->ioFlFndrInfo.fdType == kCFragLibraryFileType) {
				FSMakeFSSpec(extVRefNum, extDirID, fName, spec);
				if (MatchSharedLib(libName, spec))
					return true;
			}
			fIndex ++;
		}
	} while (err == noErr);

	return false;
}
#endif


#pragma mark -


/***************************** SAVE GAME ********************************/
//
// Returns true if saving was successful
//

Boolean SaveGame(void)
{
	SOURCE_PORT_PLACEHOLDER();
	return false;
#if 0
SaveGameType	saveData;
short			fRefNum;
FSSpec			*specPtr;
NavReplyRecord	navReply;
long			count;
Boolean			success = false;

	Enter2D();


	InitCursor();
	MyFlushEvents();

			/*************************/
			/* CREATE SAVE GAME DATA */
			/*************************/

	saveData.version		= SAVE_GAME_VERSION;				// save file version #
	saveData.version		= SwizzleULong(&saveData.version);

	saveData.score 			= gScore;
	saveData.score			= SwizzleULong(&saveData.score);

	saveData.realLevel		= gLevelNum+1;						// save @ beginning of next level
	saveData.realLevel		= SwizzleShort(&saveData.realLevel);

	saveData.numLives 		= gPlayerInfo.lives;
	saveData.numLives		= SwizzleShort(&saveData.numLives);

	saveData.health			= gPlayerInfo.health;
	saveData.health			= SwizzleFloat(&saveData.health);

	saveData.jumpJet		= gPlayerInfo.jumpJet;
	saveData.jumpJet		= SwizzleFloat(&saveData.jumpJet);

		/*******************/
		/* DO NAV SERVICES */
		/*******************/

	if (PutFileWithNavServices(&navReply, &gSavedGameSpec))
		goto bail;
	specPtr = &gSavedGameSpec;
	if (navReply.replacing)										// see if delete old
		FSpDelete(specPtr);


				/* CREATE & OPEN THE REZ-FORK */

	if (FSpCreate(specPtr,'Otto','OSav',nil) != noErr)
	{
		DoAlert("Error creating Save file");
		goto bail;
	}

	FSpOpenDF(specPtr,fsRdWrPerm, &fRefNum);
	if (fRefNum == -1)
	{
		DoAlert("Error opening Save file");
		goto bail;
	}


				/* WRITE TO FILE */

	count = sizeof(SaveGameType);
	if (FSWrite(fRefNum, &count, (Ptr)&saveData) != noErr)
	{
		DoAlert("Error writing Save file");
		FSClose(fRefNum);
		goto bail;
	}

			/* CLOSE FILE */

	FSClose(fRefNum);


			/* CLEANUP NAV SERVICES */

	NavCompleteSave(&navReply, kNavTranslateInPlace);

	success = true;

bail:
	NavDisposeReply(&navReply);
	HideCursor();
	Exit2D();

	return(success);
#endif
}


/***************************** LOAD SAVED GAME ********************************/

Boolean LoadSavedGame(void)
{
	SOURCE_PORT_PLACEHOLDER();
	return false;
#if 0
SaveGameType	saveData;
short			fRefNum;
long			count;
Boolean			success = false;
//short			oldSong;

//	oldSong = gCurrentSong;							// turn off playing music to get around bug in OS X
//	KillSong();

	Enter2D();

	InitCursor();
	MyFlushEvents();


				/* GET FILE WITH NAVIGATION SERVICES */

	if (GetFileWithNavServices(nil, &gSavedGameSpec) != noErr)
		goto bail;


				/* OPEN THE REZ-FORK */

	FSpOpenDF(&gSavedGameSpec,fsRdPerm, &fRefNum);
	if (fRefNum == -1)
	{
		DoAlert("Error opening Save file");
		goto bail;
	}

				/* READ FROM FILE */

	count = sizeof(SaveGameType);
	if (FSRead(fRefNum, &count, &saveData) != noErr)
	{
		DoAlert("Error reading Save file");
		FSClose(fRefNum);
		goto bail;
	}

			/* CLOSE FILE */

	FSClose(fRefNum);


			/**********************/
			/* USE SAVE GAME DATA */
			/**********************/

	gLoadedScore = gScore = SwizzleULong(&saveData.score);

	gLevelNum			= SwizzleShort(&saveData.realLevel);

	gPlayerInfo.lives 	= SwizzleShort(&saveData.numLives);
	gPlayerInfo.health	= SwizzleFloat(&saveData.health);
	gPlayerInfo.jumpJet	= SwizzleFloat(&saveData.jumpJet);

	success = true;


bail:
	Exit2D();
	HideCursor();


//	if (!success)								// user cancelled, so start song again before returning
//		PlaySong(oldSong, true);

	return(success);
#endif
}


#pragma mark -






