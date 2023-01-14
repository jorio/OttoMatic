/****************************/
/*   	ICESAUCER.C		    */
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

static void MoveIceSaucer_UnderIce(ObjNode *saucer);
static void MoveIceCrack(ObjNode *theNode);
static void MovePlayerIntoSaucer(ObjNode *player);
static void MoveIceSaucer_LiftOff(ObjNode *saucer);
static void MoveSaucerIce(ObjNode *ice);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	ICE_SAUCER_SCALE	2.7f

enum
{
	HATCH_MODE_OPEN,
	HATCH_MODE_CLOSE,
	HATCH_MODE_SEAL
};


/*********************/
/*    VARIABLES      */
/*********************/


CollisionBoxType *gSaucerIceBounds = nil;

int		gNumIceCracks;

Boolean		gIceCracked;
Boolean		gPlayerEnterSaucer;


/************************* ADD ICE SAUCER *********************************/

Boolean AddIceSaucer(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*ice, *hatch;
float	r;

			/*********************/
			/* CREATE THE SAUCER */
			/*********************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FIREICE_ObjType_Saucer;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + 100.0f;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB - 20;
	gNewObjectDefinition.moveCall 	= MoveIceSaucer_UnderIce;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= ICE_SAUCER_SCALE;
	gPlayerSaucer = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	gPlayerSaucer->TerrainItemPtr = itemPtr;							// keep ptr to item list

			/* SET COLLISION INFO */

	gPlayerSaucer->CType 			= CTYPE_MISC | CTYPE_BLOCKCAMERA | CTYPE_BLOCKSHADOW;
	gPlayerSaucer->CBits			= CBITS_ALLSOLID;

	CreateCollisionBoxFromBoundingBox(gPlayerSaucer, .45,1);				// make hatch collision zone

	r = gPlayerSaucer->BBox.max.x * ICE_SAUCER_SCALE * .9f;					// make the lower collision zone
	AddCollisionBoxToObject(gPlayerSaucer, 35.0f * ICE_SAUCER_SCALE, -100, -r, r, r, -r);


			/********************/
			/* CREATE THE HATCH */
			/********************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_Hatch;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	hatch = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	gPlayerSaucer->ChainNode = hatch;

	gPlayerEnterSaucer = false;

				/************************/
				/* CREATE THE ICE PATCH */
				/************************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_SaucerIce;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + 400.0f;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOLIGHTING | STATUS_BIT_KEEPBACKFACES | STATUS_BIT_NOFOG;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= MoveSaucerIce;
	gNewObjectDefinition.scale 		= 20.0;
	ice = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	ice->CType 			= CTYPE_MISC | CTYPE_BLOCKCAMERA | CTYPE_BLOCKSHADOW;
	ice->CBits			= CBITS_TOP;

	r = ice->BBox.max.x * ice->Scale.x;
	SetObjectCollisionBounds(ice, 1, -1000, -r, r, r, -r);

	gSaucerIceBounds = &ice->CollisionBoxes[0];					// keep global ptr to bounds info



	hatch->ChainNode = ice;


	return(true);													// item was added
}


/******************** MOVE SAUCER ICE *********************/
//
// This just checks to be sure player isn't under ice - hack
//

static void MoveSaucerIce(ObjNode *ice)
{
CollisionBoxType *boxPtr = &ice->CollisionBoxes[0];
ObjNode	*player = gPlayerInfo.objNode;
float	x = gPlayerInfo.coord.x;
float	z = gPlayerInfo.coord.z;

			/* SEE IF OUT OF BOUNDS */

	if (x < boxPtr->left)
		return;
	if (x > boxPtr->right)
		return;
	if (z < boxPtr->back)
		return;
	if (z > boxPtr->front)
		return;

	if (player->Coord.y < ice->Coord.y)			// see if under it
	{
		player->Coord.y = (ice->Coord.y - player->BBox.min.y) + 10.0f;
		player->Delta.y = 0;
	}

}



/******************* MOVE ICE SAUCER: UNDER ICE **********************/
//
// Just sit around waiting for something to happen
//

static void MoveIceSaucer_UnderIce(ObjNode *saucer)
{
ObjNode	*hatch;
float	fps = gFramesPerSecondFrac;
float	dist = 10000000.0f;

	if (!gIceCracked)										// only check if gone if ice not cracked
	{
#if 0
		if (TrackTerrainItem(saucer))							// just check to see if it's gone
		{
			DeleteObject(saucer);
			gSaucerIceBounds = nil;
		}
#endif
		return;
	}

	hatch = saucer->ChainNode;								// get hatch object

	if (hatch->Mode != HATCH_MODE_SEAL)
	{
				/* IF PLAYER CLOSE TO HATCH, THEN OPEN THE HATCH */

		dist = CalcDistance(hatch->Coord.x, hatch->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
		if (dist < 1200.0f)
		{
			if (hatch->Mode != HATCH_MODE_OPEN)
			{
				if (gPlayerInfo.fuel < 1.0f)								// see if have enough fuel to leave
					DisplayHelpMessage(HELP_MESSAGE_NOTENOUGHFUELTOLEAVE, 1.0, true);
				else
				{
					PlayEffect3D(EFFECT_SAUCERHATCH, &hatch->Coord);
					hatch->Mode = HATCH_MODE_OPEN;
				}
			}
		}
		else
		{
			if (hatch->Mode != HATCH_MODE_CLOSE)
			{
				PlayEffect3D(EFFECT_SAUCERHATCH, &hatch->Coord);
				hatch->Mode = HATCH_MODE_CLOSE;
			}
		}
	}

			/* MOVE HATCH TO POSITION */

	fps = gFramesPerSecondFrac;
	switch(hatch->Mode)
	{
		case	HATCH_MODE_OPEN:
				hatch->Delta.y += 400.0f * fps;
				hatch->Coord.y += hatch->Delta.y * fps;
				if (hatch->Coord.y > (saucer->Coord.y + 250.0f))
				{
					hatch->Coord.y = saucer->Coord.y + 250.0f;
					hatch->Delta.y = 0;

							/* NOW CHECK IF PLAYER ENTERS */

					if (!gPlayerEnterSaucer)								// if not entering yet, then check
					{
						if (dist < 220.0f)									// see if player on hatch area
						{
							ObjNode *player = gPlayerInfo.objNode;

							MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_FALL, 4);
							player->MoveCall = MovePlayerIntoSaucer;
							gPlayerInfo.invincibilityTimer = 20.0f;
							gPlayerEnterSaucer = true;
							gFreezeCameraFromXZ = true;
							gFreezeCameraFromY = true;
						}
					}
				}
				break;

		case	HATCH_MODE_CLOSE:
				hatch->Delta.y -= 300.0f * fps;
				hatch->Coord.y += hatch->Delta.y * fps;
				if (hatch->Coord.y < saucer->Coord.y)
				{
					hatch->Coord.y = saucer->Coord.y;
					hatch->Delta.y = 0;
				}
				break;

		case	HATCH_MODE_SEAL:
				hatch->Delta.y -= 200.0f * fps;
				hatch->Coord.y += hatch->Delta.y * fps;
				if (hatch->Coord.y < saucer->Coord.y)
				{
					hatch->Coord.y = saucer->Coord.y;
					saucer->MoveCall = MoveIceSaucer_LiftOff;
					HidePlayer(gPlayerInfo.objNode);
					saucer->EffectChannel = PlayEffect3D(EFFECT_SAUCER, &saucer->Coord);
				}
				break;

	}

	UpdateObjectTransforms(hatch);
}


/********************* MOVE PLAYER INTO SAUCER ************************/

static void MovePlayerIntoSaucer(ObjNode *player)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(player);

	gDelta.y -= gGravity * fps;

	gDelta.x = (gPlayerSaucer->Coord.x - gCoord.x) * 2.0f;				// move toward center of hatch
	gDelta.z = (gPlayerSaucer->Coord.z - gCoord.z) * 2.0f;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	if (gCoord.y < gPlayerSaucer->Coord.y)
	{
		gCoord.y = gPlayerSaucer->Coord.y;
		gDelta.y = 0;
		gPlayerSaucer->ChainNode->Mode = HATCH_MODE_SEAL;
		PlayEffect3D(EFFECT_SAUCERHATCH, &gPlayerSaucer->Coord);
		player->MoveCall = nil;
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_STAND, 6);
	}

	UpdatePlayer_Robot(player);
}


/******************* MOVE ICE SAUCER: LIFTOFF **********************/

static void MoveIceSaucer_LiftOff(ObjNode *saucer)
{
float fps = gFramesPerSecondFrac;

	GetObjectInfo(saucer);

			/* SPIN SAUCER */

	saucer->DeltaRot.y += fps * .5f;
	if (saucer->DeltaRot.y > PI)
		saucer->DeltaRot.y = PI;

	saucer->Rot.y += saucer->DeltaRot.y * fps;


		/* LIFTOFF */

	gDelta.y += 70.0f * fps;
	gCoord.y += gDelta.y * fps;

	gPlayerInfo.coord.y = gPlayerInfo.objNode->Coord.y = gCoord.y;			// set player y so that camera knows where to look

	UpdateObject(saucer);


			/* SEE IF GONE */

	if (!gLevelCompleted)
	{
		if (gCoord.y > (GetTerrainY(gCoord.x, gCoord.z) + 6000.0f))
		{
			gLevelCompleted = true;
			gLevelCompletedCoolDownTimer = 0;
		}
	}

		/* UPDATE HATCH */

	saucer->ChainNode->Rot.y = saucer->Rot.y;
	saucer->ChainNode->Coord = gCoord;			// also align the hatch
	UpdateObjectTransforms(saucer->ChainNode);


		/* UPDATE AUDIO */

	if (saucer->EffectChannel != -1)
		Update3DSoundChannel(EFFECT_SAUCER, &saucer->EffectChannel, &saucer->Coord);

}


#pragma mark -

/***************** SEE IF CRACK SAUCER ICE ****************************/

void SeeIfCrackSaucerIce(OGLPoint3D *impactPt)
{
float	x = impactPt->x;
float	z = impactPt->z;


	if (!gSaucerIceBounds
		|| x < gSaucerIceBounds->left
		|| x > gSaucerIceBounds->right
		|| z > gSaucerIceBounds->front
		|| z < gSaucerIceBounds->back)
	{
		return;
	}


	PlayEffect_Parms3D(EFFECT_ICECRACK, impactPt, NORMAL_CHANNEL_RATE, 2.0);


			/* SEE IF KABOOM ICE */

	gNumIceCracks++;
	if (gNumIceCracks > 3)
	{
		gIceCracked = true;
		PlayEffect_Parms3D(EFFECT_SHATTER, impactPt, NORMAL_CHANNEL_RATE * 2/3, 2.0);

		gSaucerIceBounds = nil;							// hammers cant hit ice anymore

		ObjNode* ice = gPlayerSaucer->ChainNode->ChainNode;		// get ice object
		gPlayerSaucer->ChainNode->ChainNode = nil;		// saucer doesn't have ice

		ExplodeGeometry(ice, 400, SHARD_MODE_BOUNCE, 1, .5);
		DeleteObject(ice);
		gPlayerInfo.objNode->Delta.y = 100;			// kick up player a tad
	}
	else
	{
				/* MAKE CRACK GRAPHIC */

		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= FIREICE_ObjType_IceCrack;
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.z 	= z;
		gNewObjectDefinition.coord.y 	= gSaucerIceBounds->top + 3.0f;
		gNewObjectDefinition.flags 		= STATUS_BIT_NOLIGHTING | STATUS_BIT_KEEPBACKFACES | STATUS_BIT_NOFOG | STATUS_BIT_NOZWRITES;
		gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
		gNewObjectDefinition.moveCall 	= MoveIceCrack;
		gNewObjectDefinition.rot 		= RandomFloat() * PI2;
		gNewObjectDefinition.scale 		= 3.0f + RandomFloat() * 1.0f;
		MakeNewDisplayGroupObject(&gNewObjectDefinition);
	}
}


/******************* MOVE ICE CRACK *************************/
//
// These stay around until the ice sheet has been shattered and then they go away
//

static void MoveIceCrack(ObjNode *theNode)
{
	if (gIceCracked)
		DeleteObject(theNode);
}

