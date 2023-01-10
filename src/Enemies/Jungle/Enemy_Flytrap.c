/****************************/
/*   ENEMY: FLYTRAP.C	*/
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

static void MoveFlyTrap(ObjNode *theNode);
static void MoveFlyTrap_Wait(ObjNode *theNode);
static void MoveFlyTrap_Frozen(ObjNode *theNode);
static void  MoveFlyTrap_Eat(ObjNode *theNode);
static void  MoveFlyTrap_Gobble(ObjNode *theNode);
static void MoveFlyTrap_Flaming(ObjNode *theNode);

static Boolean FlyTrapHitByFreeze(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean FlyTrapHitByFire(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void AlignPlayerInFlyTrapGrasp(ObjNode *enemy);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_FLYTRAPS					12

#define	FLYTRAP_SCALE					2.6f

#define	FLYTRAP_ATTACK_DIST			1000.0f



		/* ANIMS */


enum
{
	FLYTRAP_ANIM_WAIT,
	FLYTRAP_ANIM_EAT,
	FLYTRAP_ANIM_GOBBLE,
	FLYTRAP_ANIM_FLAMING
};

#define	FLYTRAP_JOINT_HEAD	11

/*********************/
/*    VARIABLES      */
/*********************/

#define	FrozenTimer		SpecialF[2]

#define	CheckEatFlag	Flag[0]
#define	ThrowPlayerFlag	Flag[0]
#define	GrippingPlayer	Flag[1]

static const OGLPoint3D gJawOff = {0,0,0};


/************************ ADD FLYTRAP ENEMY *************************/

Boolean AddEnemy_FlyTrap(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

#if !VIP_ENEMIES
	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_FLYTRAP] >= MAX_FLYTRAPS)
			return(false);
	}
#endif



				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_FLYTRAP,x,z, FLYTRAP_SCALE, (float)itemPtr->parm[0] * PI2/8, MoveFlyTrap);
	newObj->TerrainItemPtr = itemPtr;

	newObj->Skeleton->CurrentAnimTime = newObj->Skeleton->MaxAnimTime * RandomFloat();		// set random time index so all of these are not in sync


				/*******************/
				/* SET BETTER INFO */
				/*******************/

	newObj->Health 		= 1.0;
	newObj->Damage 		= .1;
	newObj->Kind 		= ENEMY_KIND_FLYTRAP;

	newObj->FrozenTimer	= 0;
	newObj->CheckEatFlag = false;

				/* SET COLLISION INFO */

	SetObjectCollisionBounds(newObj, newObj->BBox.max.y, newObj->BBox.min.y, -100,100,100,-100);


				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_FREEZE] 		= FlyTrapHitByFreeze;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= FlyTrapHitByFire;

	if (gLevelNum == LEVEL_NUM_JUNGLEBOSS)				// dont auto target these on boss level since cant be killed
		newObj->CType &= ~CTYPE_AUTOTARGETWEAPON;


#if !VIP_ENEMIES
	gNumEnemies++;
#endif
	gNumEnemyOfKind[ENEMY_KIND_FLYTRAP]++;
	return(true);
}

/********************* MOVE FLYTRAP **************************/

static void MoveFlyTrap(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveFlyTrap_Wait,
					MoveFlyTrap_Eat,
					MoveFlyTrap_Gobble,
					MoveFlyTrap_Flaming
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}


			/* SEE IF FROZEN */

	if (theNode->FrozenTimer > 0.0f)
		MoveFlyTrap_Frozen(theNode);
	else
		myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE FLYTRAP: WAIT ******************************/

static void  MoveFlyTrap_Wait(ObjNode *theNode)
{
float	angle,dist;


			/* SEE IF ATTACK */

	if (gPlayerInfo.invincibilityTimer > 0.0f)				// dont attack if I'm invincible
		return;

	angle = TurnObjectTowardTarget(theNode, &theNode->Coord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, 0, false);
	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, theNode->Coord.x, theNode->Coord.z);

	if ((dist < FLYTRAP_ATTACK_DIST) && (angle < (PI/3.0f)))
	{
		MorphToSkeletonAnim(theNode->Skeleton, FLYTRAP_ANIM_EAT, 2);
	}
}





/********************* MOVE FLYTRAP: FROZEN **************************/

static void MoveFlyTrap_Frozen(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->Skeleton->AnimSpeed = 0;							// make sure animation is frozen solid


				/* UPDATE */


	theNode->FrozenTimer -= fps;								// dec the frozen timer & see if thawed
	if (theNode->FrozenTimer <= 0.0f)
	{
		theNode->FrozenTimer = 0.0f;
		theNode->Skeleton->AnimSpeed = 1;
		ExplodeGeometry(theNode, 300, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 2, 1.0);	// shatter frozen shell
		PlayEffect3D(EFFECT_SHATTER, &theNode->Coord);
		theNode->Skeleton->overrideTexture = nil;				// don't override the texture any more
	}

}


/********************** MOVE FLYTRAP: EAT ******************************/

static void  MoveFlyTrap_Eat(ObjNode *theNode)
{
			/*********************/
			/* SEE IF EAT PLAYER */
			/*********************/

	if (theNode->Skeleton->AnimHasStopped)
		MorphToSkeletonAnim(theNode->Skeleton, FLYTRAP_ANIM_WAIT, 2);
	else
	if (theNode->CheckEatFlag && (gPlayerInfo.scaleRatio <= 1.0f))
	{
		OGLPoint3D	intersect;
		Boolean		overTop;
		float		fenceTop;

		if (!SeeIfLineSegmentHitsFence(&gPlayerInfo.coord, &theNode->Coord, &intersect, &overTop, &fenceTop))			// dont grab if there's a fence between us
		{
			OGLPoint3D	p;

			FindCoordOnJoint(theNode, FLYTRAP_JOINT_HEAD, &gJawOff, &p);			// get coord of mouth
			if (CalcQuickDistance(p.x, p.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < 200.0f)
			{
				theNode->GrippingPlayer = true;
				theNode->ThrowPlayerFlag = false;
				MorphToSkeletonAnim(theNode->Skeleton, FLYTRAP_ANIM_GOBBLE, 3);			// make gobble
				theNode->Skeleton->AnimSpeed = .6;
				SetSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_GRABBED2);	// put player into grabbed mode
			}
		}
	}
}


/********************** MOVE FLYTRAP: GOBBLE ******************************/

static void  MoveFlyTrap_Gobble(ObjNode *theNode)
{


	if (theNode->GrippingPlayer)
	{
		AlignPlayerInFlyTrapGrasp(theNode);


			/* SEE IF THROW PLAYER */

		if (theNode->ThrowPlayerFlag)					// see if throw player now
		{
			OGLMatrix4x4	m;
			ObjNode			*player = gPlayerInfo.objNode;
			const OGLVector3D	throwVec = {0,1,-1};

			MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_THROWN, 7.0);
			PlayEffect_Parms3D(EFFECT_THROWNSWOOSH, &player->Coord, NORMAL_CHANNEL_RATE, 2.0);

			FindJointMatrixAtFlagEvent(theNode, FLYTRAP_JOINT_HEAD, &m);			// calc throw vector @ exact time of throw flag
			OGLVector3D_Transform(&throwVec, &m, &player->Delta);
			player->Delta.x *= 2500.0f;
			player->Delta.y *= 2500.0f;
			player->Delta.z *= 2500.0f;
			gCurrentMaxSpeed = 15000;

			theNode->GrippingPlayer = false;
		}
	}


			/* SEE IF DONE */

	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, FLYTRAP_ANIM_WAIT, 2);
	}

}



/********************* MOVE FLYTRAP: FLAMING **************************/

static void MoveFlyTrap_Flaming(ObjNode *theNode)
{

	BurnSkeleton(theNode, 50.0f);


}





//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -



/************ FLYTRAP HIT BY FREEZE *****************/

static Boolean FlyTrapHitByFreeze(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	if (gPlayerInfo.scaleRatio <= 1.0f)							// harmless if player is normal size
		return(true);

			/* CHANGE THE TEXTURE */

	enemy->Skeleton->overrideTexture = gSpriteGroupList[SPRITE_GROUP_LEVELSPECIFIC][JUNGLE_SObjType_FrozenFlyTrap].materialObject;


	enemy->FrozenTimer = 4.0;			// set freeze timer

	return(true);			// stop weapon
}


/************ FLYTRAP HIT BY FIRE *****************/

static Boolean FlyTrapHitByFire(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	if (gPlayerInfo.scaleRatio <= 1.0f)							// harmless if player is normal size
		return(true);

	MorphToSkeletonAnim(enemy->Skeleton, FLYTRAP_ANIM_FLAMING, 2.0);	// do flaming anim

	return(true);			// stop weapon
}





/************** ALIGN PLAYER IN FLYTRAP GRASP ********************/

static void AlignPlayerInFlyTrapGrasp(ObjNode *enemy)
{
OGLMatrix4x4	m,m2,m3;
float		scale;

			/* CALC SCALE MATRIX */

	scale = PLAYER_DEFAULT_SCALE/FLYTRAP_SCALE;					// to adjust from onion's scale to player's
	OGLMatrix4x4_SetScale(&m2, scale, scale, scale);


			/* CALC TRANSLATE MATRIX */

	OGLMatrix4x4_SetTranslate(&m3, gJawOff.x, gJawOff.y, gJawOff.z);
	OGLMatrix4x4_Multiply(&m2, &m3, &m);


			/* GET ALIGNMENT MATRIX */

	FindJointFullMatrix(enemy, FLYTRAP_JOINT_HEAD, &m2);									// get joint's matrix


	OGLMatrix4x4_Multiply(&m, &m2, &gPlayerInfo.objNode->BaseTransformMatrix);
	SetObjectTransformMatrix(gPlayerInfo.objNode);


			/* FIND COORDS */

	FindCoordOnJoint(enemy, FLYTRAP_JOINT_HEAD, &gJawOff, &gPlayerInfo.objNode->Coord);			// get coord of mouth
}














