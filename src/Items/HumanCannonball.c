/****************************/
/*   	ZIPLINE.C		    */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


#include "3dmath.h"
//#include <AGL/aglmacro.h> // srcport rm

extern	float				gFramesPerSecondFrac,gFramesPerSecond,gCurrentAspectRatio,gSpinningPlatformRot;
extern	OGLPoint3D			gCoord;
extern	OGLVector3D			gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLBoundingBox 		gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	OGLSetupOutputType	*gGameViewInfoPtr;
extern	u_long				gAutoFadeStatusBits,gGlobalMaterialFlags;
extern	Boolean				gAreaCompleted;
extern	PlayerInfoType		gPlayerInfo;
extern	const float gWaterHeights[NUM_LEVELS][6];
extern	int					gLevelNum;
extern	SplineDefType	**gSplineList;
extern	SparkleType	gSparkles[MAX_SPARKLES];
extern	short				gNumEnemies,gNumTerrainItems;
extern	SpriteType	*gSpriteGroupList[];
extern	AGLContext		gAGLContext;
extern	TerrainItemEntryType 	**gMasterItemList;
extern	float			gCameraUserRotY;
extern	ObjNode			*gFirstNodePtr;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveCannon(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	CANNON_SCALE	1.9f


/*********************/
/*    VARIABLES      */
/*********************/

#define	FuseTimer	SpecialF[0]
#define	FuseIsLit	Flag[0]



/************************* ADD CANNON *********************************/

Boolean AddCannon(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj,*step;

					/***************/
					/* MAKE CANNON */
					/***************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOUD_ObjType_Cannon;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 70;
	gNewObjectDefinition.moveCall 	= MoveCannon;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2 / 4.0f);
	gNewObjectDefinition.scale 		= CANNON_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;							// keep ptr to item list



			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj,.9,.8);


					/*****************/
					/* MAKE PEDESTAL */
					/*****************/

	gNewObjectDefinition.type 		= CLOUD_ObjType_Pedestal;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	step = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->ChainNode = step;


			/* SET COLLISION STUFF */

	step->CType 		= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	step->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(step,1,1);



	return(true);													// item was added
}


/******************** MOVE CANNON *****************/

static void MoveCannon(ObjNode *theNode)
{
ObjNode	*player = gPlayerInfo.objNode;
float	r;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}


			/* SEE IF FIRE CANNON */

	if (theNode->FuseIsLit)
		theNode->FuseTimer -= gFramesPerSecondFrac;

	if (player->Skeleton->AnimNum == PLAYER_ANIM_CLIMBINTO)		// see if player is in cannon
	{
		if (theNode->FuseTimer <= 0.0f)
		{
					/* MOVE PLAYER TO CURRENT POINT */

			FindCoordOfJoint(player, 0, &player->Coord);
			player->Coord.y += 40.0f;
			SetSkeletonAnim(player->Skeleton, PLAYER_ANIM_SHOOTFROMCANNON);
			r = player->Rot.y = theNode->Rot.y;
			player->Rot.x = PI/6;
			UpdateObjectTransforms(player);

			player->Delta.y = 900;
			player->Delta.x = -sin(r) * 1600.0f;
			player->Delta.z = -cos(r) * 1600.0f;

			MakePuff(&player->Coord, 100.0, PARTICLE_SObjType_BlackSmoke, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, .5);

			PlayEffect3D(EFFECT_CANNONFIRE, &theNode->Coord);

			gCameraUserRotY = 0;									// swing camera to back
		}
	}

				/* PLAYER NOT IN CANNON */
	else
	{
		float	x,z;

		if (IsPlayerInPositionToEnterCannon(&x, &z))				// see if should do help message
			DisplayHelpMessage(HELP_MESSAGE_INTOCANNON,1.0, true);
	}
}



/****************************** IS PLAYER IN POSITION TO ENTER CANNON *************************/

ObjNode *IsPlayerInPositionToEnterCannon(float *outX, float *outZ)
{
ObjNode	*thisNodePtr = gFirstNodePtr;
float	r,x,z;

	do
	{
		if ((thisNodePtr->Genre == DISPLAY_GROUP_GENRE)	&&					// is this a cannon obj?
			(thisNodePtr->Group == MODEL_GROUP_LEVELSPECIFIC) &&
			(thisNodePtr->Type == CLOUD_ObjType_Cannon))
		{
			/* CALC HOT-SPOT IN FRONT OF CANNON */

			r = thisNodePtr->Rot.y;
			x = thisNodePtr->Coord.x - (sin(r) * (240.0f * CANNON_SCALE));
			z = thisNodePtr->Coord.z - (cos(r) * (240.0f * CANNON_SCALE));

			/* SEE IF PLAYER CLOSE ENOUGH TO IT */

			if (CalcDistance(x,z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < 80.0f)
			{
				*outX = x;									// pass back the tip
				*outZ = z;
				return(thisNodePtr);
			}
		}


		thisNodePtr = thisNodePtr->NextNode;				// next node
	}
	while (thisNodePtr != nil);

	return(nil);

}


/***************** START CANNON FUSE *************************/

void StartCannonFuse(ObjNode *theNode)
{
	theNode->FuseIsLit = true;
	theNode->FuseTimer = 3.0f;




}











