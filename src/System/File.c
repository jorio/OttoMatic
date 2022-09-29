/****************************/
/*      FILE ROUTINES       */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


#include "game.h"
#include <time.h>


/***************/
/* EXTERNALS   */
/***************/

/****************************/
/*    PROTOTYPES            */
/****************************/

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec, int skeletonType);
static void ReadDataFromPlayfieldFile(FSSpec *specPtr);
static void	ConvertTexture16To16(uint16_t *textureBuffer, int width, int height);
static inline void Blit16(
		const char*			src,
		int					srcWidth,
		int					srcHeight,
		int					srcRectX,
		int					srcRectY,
		int					srcRectWidth,
		int					srcRectHeight,
		char* 				dst,
		int 				dstWidth,
		int 				dstHeight,
		int					dstRectX,
		int					dstRectY);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	SKELETON_FILE_VERS_NUM	0x0110			// v1.1

#define	SAVE_GAME_VERSION	0x0200		// 2.0
#define SAVE_PATH_FORMAT ":OttoMatic:save_%02d.ottosave"

#define PREFS_HEADER_LENGTH 16
#define PREFS_FILE_NAME ":OttoMatic:Preferences6"
const char PREFS_HEADER_STRING[PREFS_HEADER_LENGTH+1] = "OttoMaticPrefs06";		// Bump this every time prefs struct changes -- note: this will reset user prefs


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



/******************* LOAD SKELETON *******************/
//
// Loads a skeleton file & creates storage for it.
//
// NOTE: Skeleton types 0..NUM_CHARACTERS-1 are reserved for player character skeletons.
//		Skeleton types NUM_CHARACTERS and over are for other skeleton entities.
//
// OUTPUT:	Ptr to skeleton data
//

SkeletonDefType *LoadSkeletonFile(short skeletonType)
{
QDErr		iErr;
short		fRefNum;
FSSpec		fsSpec;
SkeletonDefType	*skeleton;
const char *fileNames[MAX_SKELETON_TYPES] =
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

	ReadDataFromSkeletonFile(skeleton,&fsSpec,skeletonType);
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

static void ReadDataFromSkeletonFile(SkeletonDefType *skeleton, FSSpec *fsSpec, int skeletonType)
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
			LoadBonesReferenceModel(&target,skeleton, skeletonType);
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
char		header[PREFS_HEADER_LENGTH + 1];
PrefsType	prefBuffer;

	CheckPreferencesFolder();

				/*************/
				/* READ FILE */
				/*************/

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, PREFS_FILE_NAME, &file);
	iErr = FSpOpenDF(&file, fsRdPerm, &refNum);
	if (iErr)
		return(iErr);

				/* CHECK FILE LENGTH */

	long eof = 0;
	GetEOF(refNum, &eof);

	if (eof != sizeof(PrefsType) + PREFS_HEADER_LENGTH)
	{
		goto fileIsCorrupt;
	}

				/* READ HEADER */

	count = PREFS_HEADER_LENGTH;
	iErr = FSRead(refNum, &count, header);
	header[PREFS_HEADER_LENGTH] = '\0';
	if (iErr
		|| count != PREFS_HEADER_LENGTH
		|| 0 != strncmp(header, PREFS_HEADER_STRING, PREFS_HEADER_LENGTH))
	{
		goto fileIsCorrupt;
	}

			/* READ PREFS STRUCT */

	count = sizeof(PrefsType);
	iErr = FSRead(refNum, &count, (Ptr)&prefBuffer);		// read data from file
	if (iErr || count != sizeof(PrefsType))
	{
		goto fileIsCorrupt;
	}

	FSClose(refNum);

			/* NUKE NON-REMAPPABLE KEYBINDINGS TO DEFAULTS */

	for (int i = NUM_REMAPPABLE_NEEDS; i < NUM_CONTROL_NEEDS; i++)
	{
		prefBuffer.keys[i] = kDefaultKeyBindings[i];
	}

			/* PREFS ARE OK */

	*prefBlock = prefBuffer;
	return noErr;

fileIsCorrupt:
	printf("Prefs appear corrupt.\n");
	FSClose(refNum);
	return badFileFormat;
}



/******************** SAVE PREFS **********************/

void SavePrefs(void)
{
FSSpec				file;
OSErr				iErr;
short				refNum;
long				count;

	CheckPreferencesFolder();

				/* CREATE BLANK FILE */

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, PREFS_FILE_NAME, &file);
	FSpDelete(&file);															// delete any existing file
	iErr = FSpCreate(&file, 'Otto', 'Pref', smSystemScript);					// create blank file
	if (iErr)
		return;

				/* OPEN FILE */

	iErr = FSpOpenDF(&file, fsWrPerm, &refNum);
	if (iErr)
	{
		FSpDelete(&file);
		return;
	}

			/* WRITE HEADER */

	count = PREFS_HEADER_LENGTH;
	iErr = FSWrite(refNum, &count, (Ptr) PREFS_HEADER_STRING);
	GAME_ASSERT(iErr == noErr);
	GAME_ASSERT(count == PREFS_HEADER_LENGTH);

				/* WRITE DATA */

	count = sizeof(PrefsType);
	FSWrite(refNum, &count, (Ptr) &gGamePrefs);
	FSClose(refNum);
}

#pragma mark -

/************************** LOAD LEVEL ART ***************************/

void LoadLevelArt(void)
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





				/* LOAD AUDIO */


	LoadSoundBank(kLevelSoundBanks[gLevelNum]);


			/* LOAD BG3D GEOMETRY */

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Models:global.bg3d", &spec);
	ImportBG3D(&spec, MODEL_GROUP_GLOBAL);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, levelModelFiles[gLevelNum], &spec);
	ImportBG3D(&spec, MODEL_GROUP_LEVELSPECIFIC);

	if (gG4)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_PowerupOrb,
									 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
	}



			/* LOAD SKELETONS */

	LoadASkeleton(SKELETON_TYPE_OTTO);
	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_OTTO,
								 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

	LoadASkeleton(SKELETON_TYPE_BRAINALIEN);

	if (gG4)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_BRAINALIEN,
									 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
	}


	LoadASkeleton(SKELETON_TYPE_FARMER);
	LoadASkeleton(SKELETON_TYPE_BEEWOMAN);
	LoadASkeleton(SKELETON_TYPE_SCIENTIST);
	LoadASkeleton(SKELETON_TYPE_SKIRTLADY);

			/* LOAD LEVEL-SPECIFIC SKELETONS & APPLY REFLECTION MAPS */

	switch(gLevelNum)
	{
		case	LEVEL_NUM_FARM:
				LoadASkeleton(SKELETON_TYPE_ONION);
				if (gG4)
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_ONION,
												 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);

				LoadASkeleton(SKELETON_TYPE_CORN);
				if (gG4)
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_CORN,
												 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);


				LoadASkeleton(SKELETON_TYPE_TOMATO);
				if (gG4)
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_TOMATO,
												 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);


				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, FARM_ObjType_MetalGate,
											 	0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				break;

		case	LEVEL_NUM_BLOB:
				LoadASkeleton(SKELETON_TYPE_BLOB);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_BLOB,
											 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				LoadASkeleton(SKELETON_TYPE_SQUOOSHY);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_SQUOOSHY,
											 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);

				LoadASkeleton(SKELETON_TYPE_SLIMETREE);
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
				LoadASkeleton(SKELETON_TYPE_PODWORM);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_PODWORM,
											 0, 0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				LoadASkeleton(SKELETON_TYPE_MUTANT);
				LoadASkeleton(SKELETON_TYPE_MUTANTROBOT);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_MUTANTROBOT,
											 0, 0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);

				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, APOCALYPSE_ObjType_CrashedRocket,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_LEVELSPECIFIC, APOCALYPSE_ObjType_CrashedSaucer,
											 	-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);
				break;


		case	LEVEL_NUM_CLOUD:
				LoadASkeleton(SKELETON_TYPE_CLOWN);
				LoadASkeleton(SKELETON_TYPE_CLOWNFISH);
				LoadASkeleton(SKELETON_TYPE_STRONGMAN);
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
				LoadASkeleton(SKELETON_TYPE_GIANTLIZARD);
				LoadASkeleton(SKELETON_TYPE_FLYTRAP);
				LoadASkeleton(SKELETON_TYPE_MANTIS);
				if (gG4)
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_MANTIS,
											 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				LoadASkeleton(SKELETON_TYPE_TURTLE);
				if (gG4)
					BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_TURTLE,
											 0, -1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);
				break;

		case	LEVEL_NUM_JUNGLEBOSS:
				LoadASkeleton(SKELETON_TYPE_TURTLE);
				LoadASkeleton(SKELETON_TYPE_FLYTRAP);
				LoadASkeleton(SKELETON_TYPE_PITCHERPLANT);
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
				LoadASkeleton(SKELETON_TYPE_FLAMESTER);
				LoadASkeleton(SKELETON_TYPE_ICECUBE);
				BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + SKELETON_TYPE_ICECUBE,
											 0, 0, MULTI_TEXTURE_COMBINE_MODULATE, SPHEREMAP_SObjType_Tundra);
				LoadASkeleton(SKELETON_TYPE_SQUOOSHY);
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
				LoadASkeleton(SKELETON_TYPE_ELITEBRAINALIEN);
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
		LoadSpriteFile(&spec, SPRITE_GROUP_LEVELSPECIFIC);
	}

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:infobar.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_INFOBAR);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:fence.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_FENCES);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:global.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_GLOBAL);

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Sprites:spheremap.sprites", &spec);
	LoadSpriteFile(&spec, SPRITE_GROUP_SPHEREMAPS);


			/* LOAD TERRAIN */
			//
			// must do this after creating the view!
			//

	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, terrainFiles[gLevelNum], &spec);
	LoadPlayfield(&spec);

}


#pragma mark -

/******************* LOAD PLAYFIELD *******************/

void LoadPlayfield(FSSpec *specPtr)
{


			/* READ PLAYFIELD RESOURCES */

	ReadDataFromPlayfieldFile(specPtr);




				/***********************/
				/* DO ADDITIONAL SETUP */
				/***********************/

	CreateSuperTileMemoryList();				// allocate memory for the supertile geometry
	CalculateSplitModeMatrix();					// precalc the tile split mode matrix
	InitSuperTileGrid();						// init the supertile state grid

	BuildTerrainItemList();						// build list of items & find player start coords
}


/********************** READ DATA FROM PLAYFIELD FILE ************************/

static void ReadDataFromPlayfieldFile(FSSpec *specPtr)
{
Handle					hand;
PlayfieldHeaderType		**header;
float					yScale;
short					fRefNum;
OSErr					iErr;

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

		for (int row = 0; row < gNumSuperTilesDeep; row++)
			for (int col = 0; col < gNumSuperTilesWide; col++)
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
			for (int row = 0; row < gTerrainTileDepth; row++)
			{
				for (int col = 0; col < gTerrainTileWidth; col++)
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
		float* src = (float *)*hand;
		for (int row = 0; row <= gTerrainTileDepth; row++)
			for (int col = 0; col <= gTerrainTileWidth; col++)
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

		for (int i = 0; i < gNumTerrainItems; i++)
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

		for (int i = 0; i < gNumSplines; i++)
		{
			(*gSplineList)[i].numNubs = SwizzleShort(&splinePtr[i].numNubs);
			(*gSplineList)[i].numPoints = SwizzleLong(&splinePtr[i].numPoints);
			(*gSplineList)[i].numItems = SwizzleShort(&splinePtr[i].numItems);

			(*gSplineList)[i].bBox.top = SwizzleShort(&splinePtr[i].bBox.top);
			(*gSplineList)[i].bBox.bottom = SwizzleShort(&splinePtr[i].bBox.bottom);
			(*gSplineList)[i].bBox.left = SwizzleShort(&splinePtr[i].bBox.left);
			(*gSplineList)[i].bBox.right = SwizzleShort(&splinePtr[i].bBox.right);
		}

		ReleaseResource(hand);
		hand = nil;
	}
	else
		gNumSplines = 0;

			/* READ SPLINE POINT LIST */

	for (int i = 0; i < gNumSplines; i++)
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

			for (int j = 0; j < spline->numPoints; j++)			// swizzle
			{
				(*spline->pointList)[j].x = SwizzleFloat(&ptList[j].x);
				(*spline->pointList)[j].z = SwizzleFloat(&ptList[j].z);
			}
		}
	}


			/* READ SPLINE ITEM LIST */

	for (int i = 0; i < gNumSplines; i++)
	{
		SplineDefType	*spline = &(*gSplineList)[i];									// point to Nth spline

		hand = GetResource('SpIt',1000+i);
		GAME_ASSERT_MESSAGE(hand, "ReadDataFromPlayfieldFile: cant get spline items rez");
		{
			SplineItemType	*itemList = (SplineItemType *)*hand;

			DetachResource(hand);
			HLockHi(hand);
			(*gSplineList)[i].itemList = (SplineItemType **)hand;

			for (int j = 0; j < spline->numItems; j++)			// swizzle
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

		for (int i = 0; i < gNumFences; i++)							// copy data from rez to new list
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

	for (int i = 0; i < gNumFences; i++)
	{
		hand = GetResource('FnNb',1000+i);					// get rez
		GAME_ASSERT_MESSAGE(hand, "cant get fence nub rez");
		HLock(hand);
		{
   			FencePointType *fileFencePoints = (FencePointType *)*hand;

			gFenceList[i].nubList = (OGLPoint3D *)AllocPtr(sizeof(FenceDefType) * gFenceList[i].numNubs);	// alloc new ptr for nub array
			GAME_ASSERT(gFenceList[i].nubList);

			for (int j = 0; j < gFenceList[i].numNubs; j++)		// convert x,z to x,y,z
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

		for (int i = 0; i < gNumWaterPatches; i++)						// swizzle
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

			for (int j = 0; j < gWaterList[i].numNubs; j++)
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

	int size = SQUARED(SUPERTILE_TEXMAP_SIZE) * sizeof(uint16_t);						// calc size of supertile 16-bit texture

	int seamlessCanvasSize = SQUARED(SUPERTILE_TEXMAP_SIZE + 2) * sizeof(uint16_t);		// seamless texture needs 1px border around supertile image

	Ptr allImages = AllocPtrClear(gNumUniqueSuperTiles * size);							// all supertile images from .ter data fork
	Ptr canvas = AllocPtrClear(seamlessCanvasSize);										// we'll assemble the final supertile texture in there

	memset(gSuperTileTextureObjects, 0, sizeof(gSuperTileTextureObjects));				// clear all supertile texture pointers


				/* READ ALL SUPERTILE IMAGES FROM DATA FORK */

	iErr = FSpOpenDF(specPtr, fsRdPerm, &fRefNum);
	GAME_ASSERT_MESSAGE(!iErr, "Couldn't open terrain data fork!");

	for (int i = 0; i < gNumUniqueSuperTiles; i++)
	{
		Ptr image = allImages + i * size;

				/* READ THE SIZE OF THE NEXT COMPRESSED SUPERTILE TEXTURE */

		int32_t compressedSize = FSReadBELong(fRefNum);

				/* READ & DECOMPRESS IT */

		long decompressedSize = LZSS_Decode(fRefNum, image, compressedSize);
		GAME_ASSERT_MESSAGE(decompressedSize == size, "LZSS_Decode size is wrong!");

				/* CONVERT PIXEL FORMAT */

		ConvertTexture16To16((uint16_t*) image, SUPERTILE_TEXMAP_SIZE, SUPERTILE_TEXMAP_SIZE);
	}


				/* CREATE SUPERTILE MATERIALS */

	for (int row = 0; row < gNumSuperTilesDeep; row++)
	for (int col = 0; col < gNumSuperTilesWide; col++)
	{
		int unique = gSuperTileTextureGrid[row][col].superTileID;
		if (unique == 0)																// if 0 then it's a blank
		{
			continue;
		}

		GAME_ASSERT_MESSAGE(gSuperTileTextureObjects[unique] == NULL, "supertile isn't unique!");

				/******************************/
				/* ASSEMBLE SUPERTILE TEXTURE */
				/******************************/

#define TILEIMAGE(col, row) (allImages + gSuperTileTextureGrid[(row)][(col)].superTileID * size)

		int cw, ch;	// canvas width & height

		if (!gG4)	// No seamless texturing if we're in low-detail mode (requires NPOT textures)
		{
			cw = SUPERTILE_TEXMAP_SIZE;
			ch = SUPERTILE_TEXMAP_SIZE;
			memcpy(canvas, TILEIMAGE(col, row), size);
		}
		else		// Do seamless texturing
		{
			int tw = SUPERTILE_TEXMAP_SIZE;		// supertile width & height
			int th = SUPERTILE_TEXMAP_SIZE;
			cw = tw + 2;
			ch = th + 2;

			// Clear canvas to black
			memset(canvas, 0, seamlessCanvasSize);

			// Blit supertile image to middle of canvas
			Blit16(TILEIMAGE(col, row), tw, th, 0, 0, tw, th, canvas, cw, ch, 1, 1);

			// Scan for neighboring supertiles
			bool hasN = row > 0;
			bool hasS = row < gNumSuperTilesDeep-1;
			bool hasW = col > 0;
			bool hasE = col < gNumSuperTilesWide-1;

			// Stitch edges from neighboring supertiles on each side and copy 1px corners from diagonal neighbors
			//                       srcBuf                   sW  sH    sX    sY  rW  rH  dstBuf  dW  dH    dX    dY
			if (hasN)         Blit16(TILEIMAGE(col  , row-1), tw, th,    0, th-1, tw,  1, canvas, cw, ch,    1,    0);
			if (hasS)         Blit16(TILEIMAGE(col  , row+1), tw, th,    0,    0, tw,  1, canvas, cw, ch,    1, ch-1);
			if (hasW)         Blit16(TILEIMAGE(col-1, row  ), tw, th, tw-1,    0,  1, th, canvas, cw, ch,    0,    1);
			if (hasE)         Blit16(TILEIMAGE(col+1, row  ), tw, th,    0,    0,  1, th, canvas, cw, ch, cw-1,    1);
			if (hasE && hasN) Blit16(TILEIMAGE(col+1, row-1), tw, th,    0, th-1,  1,  1, canvas, cw, ch, cw-1,    0);
			if (hasW && hasN) Blit16(TILEIMAGE(col-1, row-1), tw, th, tw-1, th-1,  1,  1, canvas, cw, ch,    0,    0);
			if (hasW && hasS) Blit16(TILEIMAGE(col-1, row+1), tw, th, tw-1,    0,  1,  1, canvas, cw, ch,    0, ch-1);
			if (hasE && hasS) Blit16(TILEIMAGE(col+1, row+1), tw, th,    0,    0,  1,  1, canvas, cw, ch, cw-1, ch-1);
		}

#undef TILEIMAGE


				/**************************/
				/* CREATE MATERIAL OBJECT */
				/**************************/

		MOMaterialData	matData;

			/* USE PACKED PIXEL TYPE */

		matData.pixelSrcFormat 	= GL_BGRA_EXT;
		matData.pixelDstFormat 	= GL_RGBA;
		matData.textureName[0] 	= OGL_TextureMap_Load(canvas, cw, ch, GL_BGRA_EXT, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV);

			/* INIT NEW MATERIAL DATA */

		matData.setupInfo				=	gGameViewInfoPtr;						// remember which draw context this material is assigned to
		matData.flags 					= 	BG3D_MATERIALFLAG_CLAMP_U|
											BG3D_MATERIALFLAG_CLAMP_V|
											BG3D_MATERIALFLAG_TEXTURED;

		if (gG4 && gLevelNum == LEVEL_NUM_BLOBBOSS)
		{
			matData.flags |= BG3D_MATERIALFLAG_MULTITEXTURE;
			matData.envMapNum	= SPHEREMAP_SObjType_Red;
		}

		matData.multiTextureMode		= MULTI_TEXTURE_MODE_REFLECTIONSPHERE;
		matData.multiTextureCombine		= MULTI_TEXTURE_COMBINE_ADD;
		matData.diffuseColor			= (OGLColorRGBA) {1,1,1,1};
		matData.numMipmaps				= 1;										// 1 texture
		matData.width					= cw;
		matData.height					= ch;
		matData.texturePixels[0] 		= nil;										// the original pixels are gone (or will be soon)
		gSuperTileTextureObjects[unique]= MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);		// create the new object
	}

			/* CLOSE THE FILE AND CLEAN UP */

	FSClose(fRefNum);
	SafeDisposePtr(allImages);
	SafeDisposePtr(canvas);
}


/*********************** CONVERT TEXTURE; 16 TO 16 ***********************************/
//
// Converts big-endian 1-5-5-5 to native endianness and cleans up the alpha bit.
//

static void	ConvertTexture16To16(uint16_t *textureBuffer, int width, int height)
{
	bool blackOpaq = (gLevelNum != LEVEL_NUM_CLOUD);		// make black transparent on cloud

	for (int p = 0; p < width*height; p++)
	{
		uint16_t pixel = SwizzleUShort(textureBuffer);

		if (blackOpaq || (pixel & 0x7fff))
			pixel |= 0x8000;
		else
			pixel &= 0x7fff;

		*textureBuffer = pixel;

		textureBuffer++;
	}
}


/****************** COPY REGION BETWEEN 16-BIT PIXEL BUFFERS **********************/

static inline void Blit16(
		const char*			src,
		int					srcWidth,
		int					srcHeight,
		int					srcRectX,
		int					srcRectY,
		int					srcRectWidth,
		int					srcRectHeight,
		char*				dst,
		int 				dstWidth,
		int 				dstHeight,
		int					dstRectX,
		int					dstRectY
)
{
	const int bytesPerPixel = 2;

	GAME_ASSERT(srcRectX + srcRectWidth <= srcWidth);
	GAME_ASSERT(srcRectY + srcRectHeight <= srcHeight);
	GAME_ASSERT(dstRectX + srcRectWidth <= dstWidth);
	GAME_ASSERT(dstRectY + srcRectHeight <= dstHeight);

	src += bytesPerPixel * (srcRectX + srcWidth * srcRectY);
	dst += bytesPerPixel * (dstRectX + dstWidth * dstRectY);

	for (int row = 0; row < srcRectHeight; row++)
	{
		memcpy(dst, src, bytesPerPixel * srcRectWidth);
		src += bytesPerPixel * srcWidth;
		dst += bytesPerPixel * dstWidth;
	}
}


#pragma mark -


/***************************** SAVE GAME ********************************/
//
// Returns true if saving was successful
//

bool SaveGame(int saveSlot)
{
SaveGameType	saveData;
FSSpec			spec;
OSErr			err;
short			refNum;
Str255			saveFilePath;

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

	saveData.timestamp		= time(NULL);
	saveData.timestamp		= SwizzleULong64(&saveData.timestamp);

		/*******************/
		/* DO NAV SERVICES */
		/*******************/

	snprintf(saveFilePath, sizeof(saveFilePath), SAVE_PATH_FORMAT, saveSlot);

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, saveFilePath, &spec);

	err = FSpCreate(&spec, 'Otto', 'OSav', 0);
	if (err != noErr)
	{
		DoAlert("Couldn't create save file.");
		return false;
	}

	err = FSpOpenDF(&spec, fsWrPerm, &refNum);

	if (err != noErr)
	{
		DoAlert("Couldn't open file for writing.");
		return false;
	}

	long count = sizeof(SaveGameType);
	err = FSWrite(refNum, &count, (Ptr) &saveData);
	FSClose(refNum);

	if (count != sizeof(SaveGameType) || err != noErr)
	{
		DoAlert("Couldn't write saved game file.");
		return false;
	}

	return true;
}


/***************************** LOAD SAVED GAME ********************************/

bool LoadSaveGameStruct(int saveSlot, SaveGameType* saveData)
{
FSSpec			spec;
OSErr			err;
short			refNum;
Str255			saveFilePath;

				/* GET FILE WITH NAVIGATION SERVICES */

	snprintf(saveFilePath, sizeof(saveFilePath), SAVE_PATH_FORMAT, saveSlot);

	FSMakeFSSpec(gPrefsFolderVRefNum, gPrefsFolderDirID, saveFilePath, &spec);
	err = FSpOpenDF(&spec, fsRdPerm, &refNum);

	if (err != noErr)
	{
		return false;
	}

	long count = sizeof(SaveGameType);
	err = FSRead(refNum, &count, (Ptr) saveData);
	FSClose(refNum);

	if (count != sizeof(SaveGameType) || err != noErr)
	{
		return false;
	}

	saveData->score		= SwizzleULong(&saveData->score);
	saveData->realLevel	= SwizzleShort(&saveData->realLevel);
	saveData->numLives	= SwizzleShort(&saveData->numLives);
	saveData->health	= SwizzleFloat(&saveData->health);
	saveData->jumpJet	= SwizzleFloat(&saveData->jumpJet);
	saveData->timestamp	= SwizzleULong64(&saveData->timestamp);

	return true;
}

bool LoadSavedGame(int saveSlot)
{
SaveGameType	saveData;

				/* GET FILE WITH NAVIGATION SERVICES */

	if (!LoadSaveGameStruct(saveSlot, &saveData))
		return false;

			/**********************/
			/* USE SAVE GAME DATA */
			/**********************/

	gLoadedScore = gScore = saveData.score;
	gLevelNum			= saveData.realLevel;
	gPlayerInfo.lives 	= saveData.numLives;
	gPlayerInfo.health	= saveData.health;
	gPlayerInfo.jumpJet	= saveData.jumpJet;

	return true;
}
