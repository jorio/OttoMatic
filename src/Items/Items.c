/****************************/
/*   		ITEMS.C		    */
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

static void DrawSlimeFlow(ObjNode *slime);
static void RadiateSprout(ObjNode *theNode);
static void MoveSprout(ObjNode *theNode);
static void MoveWindmill(ObjNode *base);

static void DrawCyclorama(ObjNode *theNode);

static void MoveSlimeMech_Boiler(ObjNode *theNode);
static void MoveSlimePipe_Valve(ObjNode *theNode);
static void MoveMechOnAStick(ObjNode *theNode);
static void MoveSlimePipe(ObjNode *theNode);

static void MovePlatformOnSpline(ObjNode *theNode);
static void MoveCloudTunnel(ObjNode *tube);

static void MoveGreenSteam(ObjNode *theNode);


static void RadiateGrave(ObjNode *theNode);

static void MovePointySlimeTree(ObjNode *theNode);

/****************************/
/*    CONSTANTS             */
/****************************/

#define	WINDMILL_SCALE	4.0f

#define	SLIME_TREE_SCALE	6.0f

/*********************/
/*    VARIABLES      */
/*********************/


#define	OozeColor	Special[0]


/********************* INIT ITEMS MANAGER *************************/

void InitItemsManager(void)
{
	InitZigZagSlats();

	gSpinningPlatformRot= 0;

	CreateCyclorama();

	gPlayerRocketSled = nil;
	gSaucerIceBounds = nil;
	gNumIceCracks = 0;
	gIceCracked = false;
}


/************************* CREATE CYCLORAMA *********************************/

void CreateCyclorama(void)
{
ObjNode	*newObj;

	switch(gLevelNum)
	{
		case	LEVEL_NUM_BLOB:
		case	LEVEL_NUM_SAUCER:
		case	LEVEL_NUM_BRAINBOSS:
				break;

		default:
				gNewObjectDefinition.group	= MODEL_GROUP_LEVELSPECIFIC;
				gNewObjectDefinition.type 	= 0;						// cyc is always 1st model in level bg3d files
				gNewObjectDefinition.coord.x = 0;
				gNewObjectDefinition.coord.y = 0;
				gNewObjectDefinition.coord.z = 0;
				gNewObjectDefinition.flags 	= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG;
				gNewObjectDefinition.slot 	= TERRAIN_SLOT+1;					// draw after terrain for better performance since terrain blocks much of the pixels
				gNewObjectDefinition.moveCall = nil;
				gNewObjectDefinition.rot 	= 0;
				gNewObjectDefinition.scale 	= gGameViewInfoPtr->yon * .995f / 100.0f;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->CustomDrawFunction = DrawCyclorama;
				break;
	}
}


/********************** DRAW CYCLORAMA *************************/

static void DrawCyclorama(ObjNode *theNode)
{
OGLPoint3D cameraCoord = gGameViewInfoPtr->cameraPlacement.cameraLocation;

		/* UPDATE CYCLORAMA COORD INFO */

	theNode->Coord = cameraCoord;
	UpdateObjectTransforms(theNode);



			/* DRAW THE OBJECT */

	MO_DrawObject(theNode->BaseGroup);
}

#pragma mark -






/************************* ADD BARN *********************************/

Boolean AddBarn(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FARM_ObjType_Barn;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 70;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= itemPtr->parm[0] * (PI/2);
	gNewObjectDefinition.scale 		= 1.5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_IMPENETRABLE|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj,.8,1);

	return(true);													// item was added
}


/************************* ADD SILO *********************************/

Boolean AddSilo(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FARM_ObjType_Silo;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
	gNewObjectDefinition.slot 		= 90;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2.5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj,.7,1);

	return(true);													// item was added
}


/************************* ADD PHONE POLE *********************************/

Boolean AddPhonePole(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FARM_ObjType_PhonePole;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 90;
	gNewObjectDefinition.moveCall 	= MoveStaticObject3;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * PI;
	gNewObjectDefinition.scale 		= 4.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj, 1000, 0, -40, 40, 40, -40);

	return(true);													// item was added
}


#pragma mark -

/************************* ADD SPROUT *********************************/
//
// Vegetable sprouts
//

Boolean AddSprout(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		type = itemPtr->parm[0];			// get sprout type

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FARM_ObjType_CornSprout + type;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_SAUCERTARGET;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveSprout;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Kind = type;											// remember which sprout type

			/* SET COLLISION STUFF */

//	newObj->CType 			= CTYPE_MISC;
//	newObj->CBits			= CBITS_ALLSOLID;
//	CreateCollisionBoxFromBoundingBox(newObj,.7,1);

			/* SET SAUCER INFO */

	newObj->SaucerTargetType = SAUCER_TARGET_TYPE_RADIATE;
	newObj->SaucerAbductHandler = RadiateSprout;


	return(true);													// item was added
}


/********************** MOVE SPROUT *************************/

static void MoveSprout(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

		/* KEEP ALIGNED ON TERRAIN */

	RotateOnTerrain(theNode, 0, nil);							// set transform matrix
	SetObjectTransformMatrix(theNode);


		/* TRY TO GET THE SAUCER TO COME AFTER THIS */

	if (gNumEnemies < gMaxEnemies)				// dont call if there are too many enemies already
		CallAlienSaucer(theNode);
}


/********************* RADIATE SPROUT ************************/
//
// Called once when saucer starts to radiate this.
//

static void RadiateSprout(ObjNode *theNode)
{

	switch(theNode->Kind)
	{
		case	0:			// corn
				GrowCorn(theNode->Coord.x, theNode->Coord.z);
				break;

		case	1:			// onion
				GrowOnion(theNode->Coord.x, theNode->Coord.z);
				break;

		case	2:			// tomato
				GrowTomato(theNode->Coord.x, theNode->Coord.z);
				break;

	}


	DeleteObject(theNode);						// delete the sprout (but it will come back later)
}


#pragma mark -

/************************* ADD CORN STALK *********************************/

Boolean AddCornStalk(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FARM_ObjType_CornStalk;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 388;
	gNewObjectDefinition.moveCall 	= MoveStaticObject2;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj,.7,1);

	return(true);													// item was added
}

/************************* ADD BIG LEAF PLANT *********************************/

Boolean AddBigLeafPlant(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FARM_ObjType_BigLeafPlant;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 364;
	gNewObjectDefinition.moveCall 	= MoveStaticObject2;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2.5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj,.7,1);

	return(true);													// item was added
}

#pragma mark -

/************************* ADD BASIC PLANT *********************************/

Boolean AddBasicPlant(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		type = itemPtr->parm[0];
uint32_t flags = gAutoFadeStatusBits;

	switch(type)
	{
		case	0:				// FARM TREE
				gNewObjectDefinition.type 		= FARM_ObjType_Tree;
				gNewObjectDefinition.scale 		= 4.5f + RandomFloat() * 1.5f;
				flags |= STATUS_BIT_KEEPBACKFACES;
				break;

		case	1:				// JUNGLE FERN
				gNewObjectDefinition.type 		= JUNGLE_ObjType_Fern;
				gNewObjectDefinition.scale 		= 2.5f + RandomFloat() * 1.0f;
				flags |= STATUS_BIT_KEEPBACKFACES;
				break;

		case	2:				// JUNGLE FLOWER
				gNewObjectDefinition.type 		= JUNGLE_ObjType_Flower;
				gNewObjectDefinition.scale 		= 2.0f + RandomFloat() * .5f;
				flags |= STATUS_BIT_KEEPBACKFACES;
				break;

		case	3:				// JUNGLE LEAFY PLANT
				gNewObjectDefinition.type 		= JUNGLE_ObjType_LeafyPlant;
				gNewObjectDefinition.scale 		= 4.0f + RandomFloat() * 1.0f;
				flags |= STATUS_BIT_KEEPBACKFACES;
				break;

		case	4:				// TOPIARY ?
				gNewObjectDefinition.type 		= CLOUD_ObjType_Topiary0;
				gNewObjectDefinition.scale 		= 4.0;
				break;

		case	5:				// TOPIARY SPHERE
				gNewObjectDefinition.type 		= CLOUD_ObjType_Topiary1;
				gNewObjectDefinition.scale 		= 4.0;
				break;

		case	6:				// TOPIARY TRIANGLE
				gNewObjectDefinition.type 		= CLOUD_ObjType_Topiary2;
				gNewObjectDefinition.scale 		= 4.0;
				break;

		case	7:				// PINE TREE
				gNewObjectDefinition.type 		= FIREICE_ObjType_PineTree;
				gNewObjectDefinition.scale 		= 2.0;
				break;

		case	8:				// SNOW TREE
				gNewObjectDefinition.type 		= FIREICE_ObjType_SnowTree;
				gNewObjectDefinition.scale 		= 2.0;
				break;

		default:
				DoFatalAlert("AddBasicPlant: bad type");
	}

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x, z, gNewObjectDefinition.group, gNewObjectDefinition.type, 1.0);		// pass bogus scale of .x since we don't want true bbox of object just some small inner area
	gNewObjectDefinition.flags 		= flags;
	gNewObjectDefinition.slot 		= 60;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;

	switch(type)
	{
		case	1:													// GIANT FERN
				SetObjectCollisionBounds(newObj, 1000, 0, -160, 160, 160, -160);
				break;

		default:
				SetObjectCollisionBounds(newObj, 1000, 0, -40, 40, 40, -40);
	}

			/* SPECIAL FLAGS */

	switch(type)
	{
		case	0:				// FARM TREE
				BG3D_SetContainerMaterialFlags(gNewObjectDefinition.group, gNewObjectDefinition.type, 1,	// no wrapping on the leaves
												BG3D_MATERIALFLAG_CLAMP_U|BG3D_MATERIALFLAG_CLAMP_V);
				break;

		default:
				BG3D_SetContainerMaterialFlags(gNewObjectDefinition.group, gNewObjectDefinition.type, -1,	// no wrapping anywhere
												BG3D_MATERIALFLAG_CLAMP_U|BG3D_MATERIALFLAG_CLAMP_V);
	}



	return(true);													// item was added
}


/************************* ADD METAL TUB *********************************/

Boolean AddMetalTub(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FARM_ObjType_MetalTub;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 192;
	gNewObjectDefinition.moveCall 	= MoveStaticObject3;
	gNewObjectDefinition.rot 		= 0;;
	gNewObjectDefinition.scale 		= 5.0f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj,1,1);

	return(true);													// item was added
}

/************************* ADD OUTHOUSE *********************************/

Boolean AddOutHouse(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FARM_ObjType_OutHouse;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 304;
	gNewObjectDefinition.moveCall 	= MoveStaticObject2;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI/2);
	gNewObjectDefinition.scale 		= 2.3f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj,1,1);

	return(true);													// item was added
}

/************************* ADD ROCK *********************************/

Boolean AddRock(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		type = itemPtr->parm[0];			// get sprout type

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FARM_ObjType_Rock_Small + type;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 155;
	gNewObjectDefinition.moveCall 	= MoveStaticObject2;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= 2.0 + sin(gNewObjectDefinition.coord.x);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj,1,1);

	return(true);													// item was added
}

/************************* ADD HAY *********************************/

Boolean AddHay(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		type = itemPtr->parm[0];			// get sprout type

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FARM_ObjType_HayBrick + type;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 46;
	gNewObjectDefinition.moveCall 	= MoveStaticObject2;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI/2);
	gNewObjectDefinition.scale 		= 1.3;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,1);

	return(true);													// item was added
}

#pragma mark -

/************************* ADD FENCE POST *********************************/

Boolean AddFencePost(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		type = itemPtr->parm[0];			// get post type
static const float scale[] =
{
	1.2,
	.9,
	4.0,
	1.3,
	4.0,
	3.5,
	3.5,
	4.0				// neuron post

};

static const short types[] =
{
	FARM_ObjType_WoodPost,
	FARM_ObjType_MetalPost,

	JUNGLE_ObjType_WoodPost,

	APOCALYPSE_ObjType_CrunchPost,

	CLOUD_ObjType_BrassPost,

	FIREICE_ObjType_RockPost,
	FIREICE_ObjType_IcePost,

	BRAINBOSS_ObjType_FencePost,
};

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= types[type];
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits; // | STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 		= 40;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= scale[type];
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC | CTYPE_BLOCKCAMERA | CTYPE_BLOCKRAYS;

	if (type != 1)			// hack for 1.0.2 update to fix tractor-barn metal fence go-thru problem
		newObj->CType |= CTYPE_IMPENETRABLE;

	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj,1,1);

	return(true);													// item was added
}

/************************* ADD WINDMILL *********************************/

Boolean AddWindmill(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj,*prop;
float	r;

					/**********************/
					/* MAKE WINDMILL BASE */
					/**********************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FARM_ObjType_Windmill;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_KEEPBACKFACES | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= 100;
	gNewObjectDefinition.moveCall 	= MoveWindmill;
	gNewObjectDefinition.rot 		= r = (float)itemPtr->parm[0] * (PI/2);
	gNewObjectDefinition.scale 		= WINDMILL_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj,1,1);


					/**********************/
					/* MAKE WINDMILL PROP */
					/**********************/

	gNewObjectDefinition.type 		= FARM_ObjType_Propeller;
	gNewObjectDefinition.coord.x 	+= sin(r) * 53.0f * WINDMILL_SCALE;
	gNewObjectDefinition.coord.z 	+= cos(r) * 53.0f * WINDMILL_SCALE;
	gNewObjectDefinition.coord.y 	+= 372.0f * WINDMILL_SCALE;
	gNewObjectDefinition.moveCall 	= nil;
	prop = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->ChainNode = prop;


	return(true);													// item was added
}


/*************************** MOVE WINDMILL *****************************/

static void MoveWindmill(ObjNode *base)
{
ObjNode *prop = base->ChainNode;

	if (TrackTerrainItem(base))							// just check to see if it's gone
	{
		DeleteObject(base);
		return;
	}

	prop->Rot.z += gFramesPerSecondFrac;

	UpdateObjectTransforms(prop);
}


#pragma mark -

/************************* ADD SLIME PIPE *********************************/

Boolean AddSlimePipe(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*pipe,*slime,*slime2;
float	r,y,s,x2,z2;
int		type = itemPtr->parm[0];


				/*************/
				/* MAKE PIPE */
				/*************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SLIME_ObjType_SlimeTube_FancyJ + type;
	gNewObjectDefinition.scale 		= s = 2.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= y = GetMinTerrainY(x, z, gNewObjectDefinition.group, gNewObjectDefinition.type, gNewObjectDefinition.scale);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
	gNewObjectDefinition.slot 		= 88;
	gNewObjectDefinition.moveCall 	= MoveSlimePipe;
	gNewObjectDefinition.rot 		= r = (float)itemPtr->parm[1] * (PI2/8);
	pipe = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	pipe->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	pipe->CType 		= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	pipe->CBits			= CBITS_ALLSOLID;


				/**************/
				/* MAKE SLIME */
				/**************/

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOLIGHTING|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_CLIPALPHA|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= pipe->Rot.y;

	switch(type)
	{
				/* FANCY TUBE */

		case	0:
				SetObjectCollisionBounds(pipe, 	pipe->BBox.max.y * s, pipe->BBox.min.y * s,				// set collision on pipe
												pipe->BBox.min.x * .9f * s,pipe->BBox.max.x * .8f * s,
												pipe->BBox.max.x * .9f * s,pipe->BBox.min.x * .8f * s);

				x -= sin(r) * (242.0f * s);
				z -= cos(r) * (242.0f * s);
				y = GetTerrainY(x,z);							// bottom y

				gNewObjectDefinition.coord.x 	= x;
				gNewObjectDefinition.coord.z 	= z;
				gNewObjectDefinition.coord.y 	= y;
				slime = MakeNewObject(&gNewObjectDefinition);
				break;

				/* NORMAL TUBE */
				/* SHORT TUBE */

		case	1:
		case	3:
				SetObjectCollisionBounds(pipe, 	pipe->BBox.max.y * s, pipe->BBox.min.y * s,				// set collision on pipe
												pipe->BBox.min.x * .9f * s,pipe->BBox.max.x * .9f * s,
												pipe->BBox.max.x * .9f * s,pipe->BBox.min.x * .9f * s);

				x -= sin(r) * (161.0f * s);
				z -= cos(r) * (161.0f * s);
				y = GetTerrainY(x,z);

				gNewObjectDefinition.coord.x 	= x;
				gNewObjectDefinition.coord.z 	= z;
				gNewObjectDefinition.coord.y 	= y;
				slime = MakeNewObject(&gNewObjectDefinition);
				break;

				/* M TUBE */

		case	2:
				r += PI/2;

				x2 = sin(r) * (161.0 * s);
				z2 = cos(r) * (161.0 * s);
				SetObjectCollisionBounds(pipe, 	600, 0,	x2 - 55.0f * s, x2 + 55.0f,	z2 + 55.0f * s, z2 - 55.0f * s);

				pipe->NumCollisionBoxes++;
				pipe->CollisionBoxes[1].left = x - (x2 + 50.0f * s);
				pipe->CollisionBoxes[1].right = x - (x2 - 50.0f * s);
				pipe->CollisionBoxes[1].top = y + 600.0f;
				pipe->CollisionBoxes[1].bottom = y;
				pipe->CollisionBoxes[1].front = z - (z2 - 50.0f * s);
				pipe->CollisionBoxes[1].back = z - (z2 + 50.0f * s);
				KeepOldCollisionBoxes(pipe);


				gNewObjectDefinition.coord.x 	= x;									// make slime
				gNewObjectDefinition.coord.z 	= z;
				gNewObjectDefinition.coord.y 	= y;
				slime = MakeNewObject(&gNewObjectDefinition);
				break;

				/* T TUBE */

		case	4:
				SetObjectCollisionBounds(pipe, 	pipe->BBox.max.y * s, pipe->BBox.min.y * s,				// set collision on pipe
												pipe->BBox.min.z * .9f * s,pipe->BBox.max.z * .9f * s,
												pipe->BBox.max.z * .9f * s,pipe->BBox.min.z * .9f * s);

				r += PI/2;
				y = GetTerrainY(x,z);

							/* A */

				gNewObjectDefinition.coord.x 	= x - sin(r) * (162.0f * s);
				gNewObjectDefinition.coord.z 	= z - cos(r) * (162.0f * s);
				gNewObjectDefinition.coord.y 	= y;
				slime = MakeNewObject(&gNewObjectDefinition);
				slime->Kind = type;
				slime->CustomDrawFunction = DrawSlimeFlow;
				slime->OozeColor = itemPtr->parm[2];	// save ooze color
				pipe->ChainNode = slime;

							/* B */

				gNewObjectDefinition.coord.x 	= x + sin(r) * (162.0f * s);
				gNewObjectDefinition.coord.z 	= z + cos(r) * (162.0f * s);
				gNewObjectDefinition.coord.y 	= y;
				slime2 = MakeNewObject(&gNewObjectDefinition);
				slime2->Kind = type;
				slime2->CustomDrawFunction = DrawSlimeFlow;
				slime2->OozeColor = itemPtr->parm[2];	// save ooze color
				slime->ChainNode = slime2;
				return(true);
				break;

				/* GRATE TUBE */

		case	5:
				SetObjectCollisionBounds(pipe, 	pipe->BBox.max.y, pipe->BBox.min.y,				// set collision on pipe
												pipe->BBox.min.x * .9f,pipe->BBox.max.x * .9f,
												pipe->BBox.max.x * .9f,pipe->BBox.min.x * .9f);

				x -= sin(r) * (242.0f * s);
				z -= cos(r) * (242.0f * s);
				y = GetTerrainY(x,z);							// bottom y

				gNewObjectDefinition.coord.x 	= x;
				gNewObjectDefinition.coord.z 	= z;
				gNewObjectDefinition.coord.y 	= y;
				slime = MakeNewObject(&gNewObjectDefinition);
				break;

				/* VALVE TUBE */

		case	6:
				pipe->MoveCall = MoveSlimePipe_Valve;
				r += PI/2;

				x2 = sin(r) * (157.0 * s);
				z2 = cos(r) * (157.0 * s);
				SetObjectCollisionBounds(pipe, 	600, 0,	x2 - 40.0f, x2 + 40.0f,	z2 + 40.0f, z2 - 40.0f);

				pipe->NumCollisionBoxes++;
				pipe->CollisionBoxes[1].left = x - (x2 + 39.0f);
				pipe->CollisionBoxes[1].right = x - (x2 - 39.0f);
				pipe->CollisionBoxes[1].top = y + 600.0f;
				pipe->CollisionBoxes[1].bottom = y;
				pipe->CollisionBoxes[1].front = z - (z2 - 39.0f);
				pipe->CollisionBoxes[1].back = z - (z2 + 39.0f);

				KeepOldCollisionBoxes(pipe);
				return(true);

		default:
				DoFatalAlert("AddSlimePipe: illegal slime type (parm 0)");
				return false;
	}

	slime->Kind = type;						// save pipe type
	slime->OozeColor = itemPtr->parm[2];	// save ooze color
	slime->CustomDrawFunction = DrawSlimeFlow;

	pipe->ChainNode = slime;

	return(true);													// item was added
}


/******************* MOVE SLIME PIPE **************************/

static void MoveSlimePipe(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	if (theNode->EffectChannel == -1)
		theNode->EffectChannel = PlayEffect3D(EFFECT_SLIMEPIPES, &theNode->Coord);
	else
		Update3DSoundChannel(EFFECT_SLIMEPIPES, &theNode->EffectChannel, &theNode->Coord);

}


/********** MOVE SLIME PIPE: VALVE ***************/

static void MoveSlimePipe_Valve(ObjNode *theNode)
{
int		i;
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	if (!gG4)					// only steam w/ G4 to save cycles on G3's
		return;

		/**************/
		/* MAKE STEAM */
		/**************/

	theNode->ParticleTimer -= fps;											// see if add smoke
	if (theNode->ParticleTimer <= 0.0f)
	{
		theNode->ParticleTimer += .05f;										// reset timer

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= 0;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 20;
			groupDef.decayRate				=  -.3f;
			groupDef.fadeRate				= .3;
			groupDef.particleTextureNum		= PARTICLE_SObjType_GreySmoke;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			float	r = theNode->Rot.y;
			float	s = theNode->Scale.x;
			float	x,y,z;

			x = theNode->Coord.x - sin(r) * (s * 75.0f);
			z = theNode->Coord.z - cos(r) * (s * 75.0f);
			y = theNode->Coord.y + (s * 303.0f);

			for (i = 0; i < 2; i++)
			{
				p.x = x + RandomFloat2() * 40.0f;
				p.y = y + RandomFloat2() * 30.0f;
				p.z = z + RandomFloat2() * 40.0f;

				d.x = RandomFloat2() * 20.0f;
				d.y = 150.0f + RandomFloat() * 40.0f;
				d.z = RandomFloat2() * 20.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat() * PI2;
				newParticleDef.rotDZ		= RandomFloat2();
				newParticleDef.alpha		= .7;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}


}



/********************** DRAW SLIME FLOW *****************************/

static void DrawSlimeFlow(ObjNode *slime)
{
float	v,r,s,y2;
int		tubeType;
OGLMatrix4x4				m;
OGLPoint3D					tc[4];
static const OGLVector3D 	up = {0,1,0};
static OGLPoint3D		coords[4] =
{
	{-30,30,0},
	{30,30,0},
	{30,-30,0},
	{-30,-30,0},
};
static const float heights[] =
{
	389,				// fancy
	385,				// J
	296,				// M
	207,				// short
	219,				// T
	383,				// grate
};

	v = slime->SpecialF[0] += gFramesPerSecondFrac * 1.9f;

	tubeType = slime->Kind;							// get tube type


			/* CALC COORDS OF SLIME */

	s = slime->Scale.x;

	y2 = heights[tubeType] * s;							// top y

	r = 45.0f * s;


			/* SUBMIT TEXTURE */

	gGlobalMaterialFlags = BG3D_MATERIALFLAG_CLAMP_U;
	MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_LEVELSPECIFIC][SLIME_SObjType_GreenSlime + slime->OozeColor].materialObject);


			/* CALC COORDS OF VERTICES */

	coords[0].x = -r * 1.2f;	coords[0].y = 0;
	coords[1].x = r * 1.2f;	coords[1].y = 0;
	coords[2].x = r;	coords[2].y = y2;
	coords[3].x = -r;	coords[3].y = y2;

	SetLookAtMatrixAndTranslate(&m, &up, &slime->Coord, &gGameViewInfoPtr->cameraPlacement.cameraLocation);		// aim at camera & translate
	OGLPoint3D_TransformArray(&coords[0], &m, tc, 4);



			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,v);		glVertex3fv(&tc[0].x);
	glTexCoord2f(1,v);		glVertex3fv(&tc[1].x);
	glTexCoord2f(1,1+v);	glVertex3fv(&tc[2].x);
	glTexCoord2f(0,1+v);	glVertex3fv(&tc[3].x);
	glEnd();

	gGlobalMaterialFlags = 0;
}

/************************* ADD SLIME MECH *********************************/

Boolean AddSlimeMech(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj,*top;
int		type = itemPtr->parm[0];
float	s;

	switch(type)
	{
				/*******************/
				/* MECH ON A STICK */
				/*******************/

		case	0:

						/* POLE */

				gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
				gNewObjectDefinition.type 		= SLIME_ObjType_Mech_OnAStick_Pole;
				gNewObjectDefinition.coord.x 	= x;
				gNewObjectDefinition.coord.z 	= z;
				gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
				gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
				gNewObjectDefinition.slot 		= 221;
				gNewObjectDefinition.moveCall 	= MoveMechOnAStick;
				gNewObjectDefinition.rot 		= 0;
				gNewObjectDefinition.scale 		= s = 2.5f;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


						/* SET COLLISION STUFF */

				newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
				newObj->CBits			= CBITS_ALLSOLID;
				CreateCollisionBoxFromBoundingBox(newObj,1,1);

						/* TOP */

				gNewObjectDefinition.type 		= SLIME_ObjType_Mech_OnAStick_Top;
				gNewObjectDefinition.moveCall 	= nil;
				gNewObjectDefinition.rot 		= RandomFloat() * PI2;
				gNewObjectDefinition.scale 		= s = 2.5f;
				top = MakeNewDisplayGroupObject(&gNewObjectDefinition);
				newObj->ChainNode = top;
				break;


				/**********/
				/* BOILER */
				/**********/

		case	1:
				gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
				gNewObjectDefinition.type 		= SLIME_ObjType_Mech_Boiler;
				gNewObjectDefinition.coord.x 	= x;
				gNewObjectDefinition.coord.z 	= z;
				gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
				gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
				gNewObjectDefinition.slot 		= 221;
				gNewObjectDefinition.moveCall 	= MoveSlimeMech_Boiler;
				gNewObjectDefinition.rot 		= RandomFloat() * PI2;
				gNewObjectDefinition.scale 		= s = 2.5f;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

						/* SET COLLISION STUFF */

				newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
				newObj->CBits			= CBITS_ALLSOLID;
				CreateCollisionBoxFromBoundingBox(newObj,.5,1);
				break;

	}

	return(true);													// item was added
}


/************************* MOVE MECH ON A STICK ***************************/

static void MoveMechOnAStick(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*top = theNode->ChainNode;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	top->Rot.y -= fps * 2.0f;
	UpdateObjectTransforms(top);
}


/************************* MOVE SLIME MECH: BOILER ***************************/

static void MoveSlimeMech_Boiler(ObjNode *theNode)
{
int		i;
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	if (!gG4)					// only steam w/ G4 to save cycles on G3's
		return;

		/**************/
		/* MAKE STEAM */
		/**************/

	theNode->ParticleTimer -= fps;											// see if add smoke
	if (theNode->ParticleTimer <= 0.0f)
	{
		theNode->ParticleTimer += .05f;										// reset timer

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= 0;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 25;
			groupDef.decayRate				=  -.2f;
			groupDef.fadeRate				= .2;
			groupDef.particleTextureNum		= PARTICLE_SObjType_BlackSmoke;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			float	r = theNode->Rot.y;
			float	s = theNode->Scale.x;
			float	x,y,z;

			x = theNode->Coord.x + sin(r) * (s * 77.0f);
			z = theNode->Coord.z + cos(r) * (s * 77.0f);
			y = theNode->Coord.y + (s * 588.0f);

			for (i = 0; i < 2; i++)
			{
				p.x = x + RandomFloat2() * 40.0f;
				p.y = y + RandomFloat2() * 30.0f;
				p.z = z + RandomFloat2() * 40.0f;

				d.x = RandomFloat2() * 20.0f;
				d.y = 150.0f + RandomFloat() * 40.0f;
				d.z = RandomFloat2() * 20.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat() * PI2;
				newParticleDef.rotDZ		= RandomFloat2();
				newParticleDef.alpha		= .7;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}

#pragma mark -

/************************* ADD BASIC CRYSTAL *********************************/

Boolean AddBasicCrystal(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
float	s;
int		type = itemPtr->parm[0];


	if (type > 3)
		DoFatalAlert("AddBasicCrystal: illegal crystal type (parm 0)");

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SLIME_ObjType_CrystalCluster_Blue + type;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-10;
	gNewObjectDefinition.moveCall 	= MoveStaticObject2;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/8.0f);
	gNewObjectDefinition.scale 		= s = 3.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->ColorFilter.a = .9;				// slightly transparent

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;

	switch(type)
	{
					/* CLUSTER */
		case	0:
		case	1:
				SetObjectCollisionBounds(newObj, 	newObj->BBox.max.y * s, 0,
													-70.0f * s, 70.0f * s,
													70.0f * s, -70.0f * s);
				break;

					/* COLUMN */
		default:
				CreateCollisionBoxFromBoundingBox(newObj, .7, 1);
				break;
	}



	return(true);												// item was added
}


/************************* ADD SLIME TREE *********************************/

Boolean AddSlimeTree(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;


	switch(itemPtr->parm[0])
	{
			/* ANIMATING SLIME TREE */

		case	0:
				gNewObjectDefinition.type 		= SKELETON_TYPE_SLIMETREE;
				gNewObjectDefinition.animNum 	= 0;
				gNewObjectDefinition.coord.x 	= x;
				gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
				gNewObjectDefinition.coord.z 	= z;
				gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
				gNewObjectDefinition.slot 		= 227;
				gNewObjectDefinition.moveCall 	= MoveStaticObject3;
				gNewObjectDefinition.rot 		= RandomFloat()*PI2;
				gNewObjectDefinition.scale 		= SLIME_TREE_SCALE;
				newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

				newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


				newObj->CType 			= CTYPE_MISC;
				newObj->CBits			= CBITS_ALLSOLID;

				SetObjectCollisionBounds(newObj, 	newObj->BBox.max.y, 0,
													-100.0f, 100.0f,
													100.0f, -100.0f);

				newObj->Skeleton->AnimSpeed += RandomFloat2() * .2f;
				break;


				/* BIG STILL SLIME TREE */

		case	1:
				gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
				gNewObjectDefinition.type 		= SLIME_ObjType_SlimeTree_Big;
				gNewObjectDefinition.coord.x 	= x;
				gNewObjectDefinition.coord.z 	= z;
				gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
				gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
				gNewObjectDefinition.slot 		= 301;
				gNewObjectDefinition.moveCall 	= MovePointySlimeTree;
				gNewObjectDefinition.rot 		= RandomFloat()*PI2;
				gNewObjectDefinition.scale 		= SLIME_TREE_SCALE;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

				newObj->CType 			= CTYPE_MISC;
				newObj->CBits			= CBITS_ALLSOLID;

				SetObjectCollisionBounds(newObj, 	newObj->BBox.max.y * SLIME_TREE_SCALE, 0,
													-15.0f * SLIME_TREE_SCALE, 15.0f * SLIME_TREE_SCALE,
													15.0f * SLIME_TREE_SCALE, -15.0f * SLIME_TREE_SCALE);
				break;

				/* SMALL STILL SLIME TREE */

		case	2:
				gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
				gNewObjectDefinition.type 		= SLIME_ObjType_SlimeTree_Small;
				gNewObjectDefinition.coord.x 	= x;
				gNewObjectDefinition.coord.z 	= z;
				gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
				gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
				gNewObjectDefinition.slot 		= 300;
				gNewObjectDefinition.moveCall 	= MovePointySlimeTree;
				gNewObjectDefinition.rot 		= RandomFloat()*PI2;
				gNewObjectDefinition.scale 		= SLIME_TREE_SCALE;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

				newObj->CType 			= CTYPE_MISC;
				newObj->CBits			= CBITS_ALLSOLID;

				SetObjectCollisionBounds(newObj, 	newObj->BBox.max.y * gNewObjectDefinition.scale, 0,
													-60.0f, 60.0f,
													60.0f, -60.0f);
				break;
	}

	return(true);
}


/********************* MOVE POINTY SLIME TREE *************************/

static void MovePointySlimeTree(ObjNode *theNode)
{
static const OGLPoint3D	smallPts[7] =
{
	{-35.5,	23.9,	9.9},
	{-25.3,	37.4,	-36.8},
	{29.2,	37.4,	-34.5},
	{48.7,	35.0,	16.6},
	{0.3,	37.4,	44.7},
	{-19.3,	58.3,	9.8},
	{14.2,	50.2,	8.7},
};

static const OGLPoint3D	bigPts[5] =
{
	{-1.5,	73.9,	120.5},
	{-95.5,	50.3,	17.8},
	{-54.5,	71.2,	-65.5},
	{49.1,	49.3,	-54.0},
	{81.8,	71.2,	30.0},
};

OGLPoint3D	tPts[7];

			/*********************/
			/* DO BASIC TRACKING */
			/*********************/

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - (theNode->BBox.min.y * theNode->Scale.y);

	UpdateObjectTransforms(theNode);

	if (theNode->NumCollisionBoxes > 0)
		CalcObjectBoxFromNode(theNode);


		/***********************************/
		/* SEE IF ANY POINTS POPPED BUBBLE */
		/***********************************/

	if ((gPlayerInfo.objNode->Skeleton->AnimNum != PLAYER_ANIM_BUBBLE) || (!gSoapBubble))		// see if riding bubble
		return;


	float x = theNode->Coord.x;
	float z = theNode->Coord.z;

			/* SEE IF ANYWHERE IN RANGE */

	if (CalcQuickDistance(x,z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) > 600.0f)
		return;

	switch(theNode->Type)
	{
				/* DO SMALL TREE */

		case	SLIME_ObjType_SlimeTree_Small:
				OGLPoint3D_TransformArray(&smallPts[0], &theNode->BaseTransformMatrix, &tPts[0], 7);		// transform pts

				for (int i = 0; i < 7; i++)
				{
					if (OGLPoint3D_Distance(&tPts[i], &gPlayerInfo.coord) < 130.0f)
					{
						PopSoapBubble(gSoapBubble);
						break;
					}
				}
				break;




				/* DO BIG TREE */

		case	SLIME_ObjType_SlimeTree_Big:
				OGLPoint3D_TransformArray(&bigPts[0], &theNode->BaseTransformMatrix, &tPts[0], 5);		// transform pts
				for (int i = 0; i < 5; i++)
				{
					if (OGLPoint3D_Distance(&tPts[i], &gPlayerInfo.coord) < 130.0f)
					{
						PopSoapBubble(gSoapBubble);
						break;
					}
				}
				break;
	}

}





#pragma mark -

/************************ PRIME MOVING PLATFORM *************************/

Boolean PrimeMovingPlatform(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement,s;
int				type = itemPtr->parm[0];

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/***************/
				/* MAKE OBJECT */
				/***************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BLOBBOSS_ObjType_MovingPlatform_Blue + type;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + 500.0f;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_ONSPLINE;
	gNewObjectDefinition.slot 		= PLAYER_SLOT - 4;				// good idea to move this before the player
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= s = 2.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;


				/* SET BETTER INFO */

	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MovePlatformOnSpline;		// set move call

	newObj->CType 			= CTYPE_MISC | CTYPE_MPLATFORM | CTYPE_BLOCKCAMERA | CTYPE_BLOCKSHADOW;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 	1, 1);



			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	AddToSplineObjectList(newObj, false);


			/* DETACH FROM LINKED LIST */

	DetachObject(newObj, true);

	return(true);
}


/******************** MOVE PLATFORM ON SPLINE ***************************/

static void MovePlatformOnSpline(ObjNode *theNode)
{
Boolean isVisible;
float	oldX, oldZ;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	oldX = theNode->Coord.x;
	oldZ = theNode->Coord.z;
	IncreaseSplineIndex(theNode, 100);
	GetObjectCoordOnSpline(theNode);

	theNode->Delta.x = (theNode->Coord.x - oldX) * gFramesPerSecond;	// set deltas that we just moved
	theNode->Delta.z = (theNode->Coord.z - oldZ) * gFramesPerSecond;

			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/

	if (isVisible)
	{
		UpdateObjectTransforms(theNode);					// update transforms
		CalcObjectBoxFromNode(theNode);						// calc current collision box
	}

}



/************************* ADD BLOB BOSS TUBE *********************************/

Boolean AddBlobBossTube(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*pipe;
int		type = itemPtr->parm[0];
float	s;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BLOBBOSS_ObjType_Tube_Bent + type;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
	gNewObjectDefinition.slot 		= 88;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * PI/2;
	gNewObjectDefinition.scale 		= s = 3.0;
	pipe = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	pipe->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	pipe->CType 		= CTYPE_MISC;
	pipe->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(pipe,1,1);

	return(true);
}


#pragma mark -

/************** ADD SCAFFOLDING POST ***********************/


Boolean AddScaffoldingPost(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
short	type = itemPtr->parm[0];

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOUD_ObjType_Post0 + type;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY_Undeformed(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 300;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 10.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType = CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj, 1,1);


			/* MAKE SPARKLE */

#if 1
	if (type == 3)
	{
		short	i = newObj->Sparkles[0] = GetFreeSparkle(newObj);				// get free sparkle slot
		if (i != -1)
		{
			gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_RANDOMSPIN|SPARKLE_FLAG_FLICKER;
			gSparkles[i].where.x = newObj->Coord.x;
			gSparkles[i].where.y = newObj->Coord.y + 160.0f;
			gSparkles[i].where.z = newObj->Coord.z;

			gSparkles[i].color.r = 1;
			gSparkles[i].color.g = 1;
			gSparkles[i].color.b = 1;
			gSparkles[i].color.a = 1;

			gSparkles[i].scale = 60.0f;

			gSparkles[i].separation = 20.0f;

			gSparkles[i].textureNum = PARTICLE_SObjType_GreenSpark;
		}
	}
#endif
	return(true);
}



/******************** ADD CLOUD TUNNEL ***********************/

Boolean AddCloudTunnel(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*frame,*tube;

			/**************/
			/* MAKE FRAME */
			/**************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOUD_ObjType_CloudTunnel_Frame;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY_Undeformed(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI/2);
	gNewObjectDefinition.scale 		= 8.0;
	frame = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	frame->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/*************/
			/* MAKE TUBE */
			/*************/

	gNewObjectDefinition.type 		= CLOUD_ObjType_CloudTunnel_Tube;
	gNewObjectDefinition.moveCall 	= MoveCloudTunnel;
	gNewObjectDefinition.flags 		|= STATUS_BIT_KEEPBACKFACES | STATUS_BIT_GLOW | STATUS_BIT_NOZWRITES | STATUS_BIT_NOLIGHTING | STATUS_BIT_ROTZXY;
	tube = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	tube->ColorFilter.a = .7;

	frame->ChainNode = tube;
	return(true);
}


/************ MOVE CLOUD TUNNEL ************/

static void MoveCloudTunnel(ObjNode *tube)
{
	tube->Rot.z += gFramesPerSecondFrac * 8.0f;

	UpdateObjectTransforms(tube);

}


#pragma mark -

/************************* ADD LAMP POST *********************************/

Boolean AddLampPost(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		type = itemPtr->parm[0];


	if (type > 2)
		DoFatalAlert("AddLampPost: illegal type (parm 0)");

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= APOCALYPSE_ObjType_LampPost + type;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 141;
	gNewObjectDefinition.moveCall 	= MoveStaticObject2;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/4.0f);
	gNewObjectDefinition.scale 		= .8;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);



	return(true);												// item was added
}





/************************* ADD CRASHED SHIP *********************************/

Boolean AddCrashedShip(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		type = itemPtr->parm[0];


	if (type > 1)
		DoFatalAlert("AddCrashedShip: illegal type (parm 0)");

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= APOCALYPSE_ObjType_CrashedRocket + type;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
	gNewObjectDefinition.slot 		= 289;
	gNewObjectDefinition.moveCall 	= MoveStaticObject3;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/4.0f);
	gNewObjectDefinition.scale 		= 1.4;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;

	if (type == 0)
	{
		SetObjectCollisionBounds(newObj, 500, 0, -100, 100, 100, -100);
	}
	else
	{
		if (itemPtr->parm[0] & 1)
			SetObjectCollisionBounds(newObj, 500, 0, -550, 550, 170, -170);
		else
			SetObjectCollisionBounds(newObj, 500, 0, -170, 170, 550, -550);
	}

	return(true);												// item was added
}


/************************* ADD RUBBLE *********************************/

Boolean AddRubble(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		type = itemPtr->parm[0];


	if (type > 6)
		DoFatalAlert("AddRubble: illegal type (parm 0)");

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= APOCALYPSE_ObjType_Rubble0 + type;
	gNewObjectDefinition.scale 		= 2.0;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x, z, gNewObjectDefinition.group, gNewObjectDefinition.type, gNewObjectDefinition.scale);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 70;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI2/4.0f);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);

	return(true);												// item was added
}


/************************* ADD TELEPORTER MAP *********************************/

Boolean AddTeleporterMap(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		type = itemPtr->parm[0];


	if (type > 4)
		DoFatalAlert("AddTeleporterMap: illegal type (parm 0)");

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= APOCALYPSE_ObjType_TeleporterMap0; // + type;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 70;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI2/4.0f);
	gNewObjectDefinition.scale 		= 2.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);

	return(true);												// item was added
}

#pragma mark -

/********************* ADD GREEN STEAM *****************************/

Boolean AddGreenSteam(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-10;
	gNewObjectDefinition.moveCall 	= MoveGreenSteam;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->ParticleTimer = 0;

			/* SET COLLISION STUFF */

	newObj->CType 	= CTYPE_HURTME;
	newObj->CBits	= CBITS_TOUCHABLE;
	SetObjectCollisionBounds(newObj, 50, 0, -50, 50, 50, -50);

	newObj->Damage	= .15f;

	return(true);
}



/************************ MOVE GREEN STEAM ****************************/

static void MoveGreenSteam(ObjNode *theNode)
{
int		i;
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}


	theNode->ParticleTimer -= fps;
	if (theNode->ParticleTimer <= 0.0f)
	{
		theNode->ParticleTimer += 0.025;


		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= 170;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 30;
			groupDef.decayRate				= -.7;
			groupDef.fadeRate				= .2;
			groupDef.particleTextureNum		= PARTICLE_SObjType_GreenFumes;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			float	x = theNode->Coord.x;
			float	y = theNode->Coord.y;
			float	z = theNode->Coord.z;

			for (i = 0; i < 2; i++)
			{
				p.x = x + RandomFloat2() * 40.0f;
				p.y = y;
				p.z = z + RandomFloat2() * 40.0f;

				d.x = RandomFloat2() * 100.0f;
				d.y = 300.0f + RandomFloat() * 150.0f;
				d.z = RandomFloat2() * 100.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat()*PI2;
				newParticleDef.rotDZ		= RandomFloat2() * .1f;
				newParticleDef.alpha		= .6;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}


}

#pragma mark -

/************************* ADD GRAVE STONE *********************************/

Boolean AddGraveStone(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		type = itemPtr->parm[0];

				/*******************/
				/* MAKE GRAVESTONE */
				/*******************/

	if (type > 4)
		DoFatalAlert("AddGraveStone: illegal type (parm 0)");

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= APOCALYPSE_ObjType_GraveStone + type;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 289;
	gNewObjectDefinition.moveCall 	= MoveStaticObject2;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI2/4.0f);
	gNewObjectDefinition.scale 		= .6;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1, 1);

			/* SET SAUCER INFO */

	newObj->SaucerTargetType = SAUCER_TARGET_TYPE_RADIATE;
	newObj->SaucerAbductHandler = RadiateGrave;



	return(true);												// item was added
}




/********************* RADIATE GRAVE ************************/
//
// Called once when saucer starts to radiate this.
//

static void RadiateGrave(ObjNode *theNode)
{
	GrowMutant(theNode->Coord.x, theNode->Coord.z);
}







