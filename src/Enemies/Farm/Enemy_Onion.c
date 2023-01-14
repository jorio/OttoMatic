/****************************/
/*   ENEMY: ONION.C	*/
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

static ObjNode *MakeEnemy_Onion(long x, long z);
static void MoveOnion(ObjNode *theNode);
static void MoveOnion_Standing(ObjNode *theNode);
static void  MoveOnion_Walking(ObjNode *theNode);
static void MoveOnionOnSpline(ObjNode *theNode);
static void UpdateOnion(ObjNode *theNode);
static void  MoveOnion_GetHit(ObjNode *theNode);
static void  MoveOnion_Attack(ObjNode *theNode);
static void  MoveOnion_Jiggle(ObjNode *theNode);
static void  MoveOnion_Grow(ObjNode *theNode);

static void CreateOnionSparkles(ObjNode *theNode);
static void UpdateOnionSparkles(ObjNode *theNode);
static void OnionHitByJumpJet(ObjNode *enemy);
static Boolean OnionHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean OnionEnemyHitByStunPulse(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void AlignPlayerInOnionGrasp(ObjNode *onion);
static Boolean OnionGotPunched(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta);
static Boolean HurtOnion(ObjNode *enemy, float damage);
static Boolean KillOnion(ObjNode *enemy);
static void MoveOnionSlice(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_ONIONS					8

#define	ONION_SCALE					2.0f
#define	ONION_SCALE_MIN				0.7f

#define	ONION_CHASE_DIST			3000.0f
#define	ONION_DETACH_DIST			(ONION_CHASE_DIST / 2.0f)
#define	ONION_ATTACK_DIST			600.0f
#define	ONION_CEASE_DIST			3500.0f

#define	ONION_TARGET_OFFSET			200.0f

#define ONION_TURN_SPEED			2.4f
#define ONION_WALK_SPEED			300.0f


#define	ONION_KNOCKDOWN_SPEED		1700.0f

#define	SLAP_DAMAGE					.3f

		/* ANIMS */


enum
{
	ONION_ANIM_STAND,
	ONION_ANIM_WALK,
	ONION_ANIM_GETHIT,
	ONION_ANIM_ATTACK,
	ONION_ANIM_JIGGLE,
	ONION_ANIM_GROW
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	GrippingPlayer	Flag[1]
#define	DelayUntilAttack	SpecialF[2]
#define	ButtTimer		SpecialF[3]


/************************ ADD ONION ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Onion(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_ONION] >= MAX_ONIONS)
			return(false);
	}

	newObj = MakeEnemy_Onion(x, z);
	newObj->TerrainItemPtr = itemPtr;
	newObj->EnemyRegenerate = itemPtr->parm[3] & (1<<1);

	SetSkeletonAnim(newObj->Skeleton, ONION_ANIM_STAND);

	return(true);
}


/************************ MAKE ONION ENEMY *************************/

static ObjNode *MakeEnemy_Onion(long x, long z)
{
ObjNode	*newObj;

				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_ONION,x,z, ONION_SCALE, 0, MoveOnion);

	SetSkeletonAnim(newObj->Skeleton, ONION_ANIM_STAND);


				/*******************/
				/* SET BETTER INFO */
				/*******************/

	newObj->Health 		= 1.0;
	newObj->Damage 		= .1;
	newObj->Kind 		= ENEMY_KIND_ONION;

	newObj->DelayUntilAttack = 0;

				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,ONION_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] = OnionEnemyHitByStunPulse;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] = OnionHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] = OnionGotPunched;
	newObj->HitByJumpJetHandler = OnionHitByJumpJet;

	newObj->HurtCallback = HurtOnion;							// set hurt callback function



				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 8,false);

	CreateOnionSparkles(newObj);

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_ONION]++;

	return(newObj);
}


/******************* GROW ONION ***********************/
//
// Called from RadiateSprout() when saucer makes this it grow
//

void GrowOnion(float x, float z)
{
ObjNode	*newObj;

	newObj = MakeEnemy_Onion(x, z);
	if (newObj)
	{
		newObj->Scale.x = newObj->Scale.y = newObj->Scale.z = .08f;											// start shrunken
		newObj->Rot.y = CalcYAngleFromPointToPoint(0, x, z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);		// aim at player

		SetSkeletonAnim(newObj->Skeleton, ONION_ANIM_GROW);
		UpdateObjectTransforms(newObj);
	}

}

/********************* MOVE ONION **************************/

static void MoveOnion(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveOnion_Standing,
					MoveOnion_Walking,
					MoveOnion_GetHit,
					MoveOnion_Attack,
					MoveOnion_Jiggle,
					MoveOnion_Grow,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);
	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE ONION: STANDING ******************************/

static void  MoveOnion_Standing(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

				/* TURN TOWARDS ME */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, ONION_TURN_SPEED, true);



				/* SEE IF CHASE */

	float dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
	if (dist < ONION_CHASE_DIST)
	{
		MorphToSkeletonAnim(theNode->Skeleton, ONION_ANIM_WALK, 8);
	}

			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;



	UpdateOnion(theNode);
}




/********************** MOVE ONION: WALKING ******************************/

static void  MoveOnion_Walking(ObjNode *theNode)
{
float		r,fps,dist,angleToPlayer;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * ONION_WALK_SPEED;
	gDelta.z = -cos(r) * ONION_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);

	angleToPlayer = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, ONION_TURN_SPEED, true);


				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == ONION_ANIM_WALK)
		theNode->Skeleton->AnimSpeed = ONION_WALK_SPEED * .004f;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


			/* CHECK RANGE */

	dist = CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
	if (dist > ONION_CEASE_DIST)										// see if stop chasing
	{
		MorphToSkeletonAnim(theNode->Skeleton, ONION_ANIM_STAND, 6);
		gDelta.x = gDelta.z = 0;
	}
	else
	if ((dist < ONION_ATTACK_DIST) && (angleToPlayer < PI/6))			// see if attack
	{
		if (theNode->DelayUntilAttack <= 0.0f)
		{
			if (!SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE|CTYPE_BLOCKRAYS|CTYPE_IMPENETRABLE))	// dont grab if there's a fence between us
			{
				MorphToSkeletonAnim(theNode->Skeleton, ONION_ANIM_ATTACK, 9);
				gDelta.x = gDelta.z = 0;
			}
		}
	}

	UpdateOnion(theNode);
}


/********************** MOVE ONION: GET HIT ******************************/

static void  MoveOnion_GetHit(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(400.0,&gDelta);

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;			// add gravity

	MoveEnemy(theNode);


				/* SEE IF DONE */

	theNode->ButtTimer -= gFramesPerSecondFrac;
	if (theNode->ButtTimer <= 0.0)
	{
		MorphToSkeletonAnim(theNode->Skeleton, ONION_ANIM_STAND, 3);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateOnion(theNode);
}

/********************** MOVE ONION: ATTACK ******************************/

static void  MoveOnion_Attack(ObjNode *theNode)
{
float	dist,angle;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


				/* SEE IF STRIKE IS AT BOTTOM */

	if (theNode->Skeleton->AnimHasStopped)
	{
		angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, 0, false);
		dist = CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);

				/*****************************************/
				/* SEE IF PLAYER CAN BE GRABBED BY ONION */
				/*****************************************/

		if (!SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE|CTYPE_BLOCKRAYS|CTYPE_IMPENETRABLE))	// dont grab if there's a fence between us
		{
			if ((angle < .55f) && (dist < ONION_ATTACK_DIST) &&
				(gPlayerInfo.objNode->Skeleton->AnimNum != PLAYER_ANIM_GRABBED) && (gPlayerInfo.invincibilityTimer <= 0.0f))
			{
				theNode->GrippingPlayer = true;
				theNode->DelayUntilAttack = 5;									// set delay until it'll do this again
				SetSkeletonAnim(theNode->Skeleton, ONION_ANIM_JIGGLE);					// make jiggle
				SetSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_GRABBED);	// put player into grabbed mode
			}
			else
				MorphToSkeletonAnim(theNode->Skeleton, ONION_ANIM_STAND, 3);
		}
		else
			MorphToSkeletonAnim(theNode->Skeleton, ONION_ANIM_STAND, 3);

	}


	UpdateOnion(theNode);
}


/********************** MOVE ONION: JIGGLE ******************************/

static void  MoveOnion_Jiggle(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);


			/* ALIGN PLAYER IN GRASP */

	AlignPlayerInOnionGrasp(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


				/* SEE IF DONE */

	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, ONION_ANIM_STAND, 3);
		SetSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_FLATTENED);
		PlayEffect3D(EFFECT_PLAYERCRUSH, &gPlayerInfo.coord);

		gPlayerInfo.knockDownTimer = 1.5f;
		gPlayerInfo.objNode->Delta.x = 0;
		gPlayerInfo.objNode->Delta.z = 0;
		theNode->GrippingPlayer = false;
		gPlayerInfo.invincibilityTimer = 10.0f;					// set this while flattened so I cant be bothered - it gets reset @ end of anim

		PlayerLoseHealth(SLAP_DAMAGE, PLAYER_DEATH_TYPE_EXPLODE);

	}




	UpdateOnion(theNode);
}


/********************** MOVE ONION: GROW ******************************/

static void  MoveOnion_Grow(ObjNode *theNode)
{
float	scale;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

		/* MAKE GROW */

	scale = theNode->Scale.x += gFramesPerSecondFrac;

	if (scale >= ONION_SCALE)								// don't overgrow
		scale = ONION_SCALE;

	theNode->Scale = (OGLVector3D) {scale, scale, scale};

	if (DoEnemyCollisionDetect(theNode,0, true))			// just do ground
		return;


			/* SEE IF DONE GROWING */

	if (theNode->Skeleton->AnimHasStopped)
	{
		FinalizeSkeletonObjectScale(theNode, ONION_SCALE, ONION_ANIM_STAND);

		MorphToSkeletonAnim(theNode->Skeleton, ONION_ANIM_STAND, 2);
		gDelta.y = 700.0f;
	}

	UpdateOnion(theNode);
}


/************** ALIGN PLAYER IN ONION GRASP ********************/

static void AlignPlayerInOnionGrasp(ObjNode *onion)
{
OGLMatrix4x4	m,m2,m3;
float		scale;

			/* CALC SCALE MATRIX */

	scale = PLAYER_DEFAULT_SCALE/onion->Scale.y;							// to adjust from onion's scale to player's
	OGLMatrix4x4_SetScale(&m2, scale, scale, scale);


			/* CALC TRANSLATE MATRIX */

	OGLMatrix4x4_SetTranslate(&m3, 0,60,30);
	OGLMatrix4x4_Multiply(&m2, &m3, &m);


			/* GET ALIGNMENT MATRIX */

	FindJointFullMatrix(onion, 10, &m2);									// get joint's matrix


	OGLMatrix4x4_Multiply(&m, &m2, &gPlayerInfo.objNode->BaseTransformMatrix);
	SetObjectTransformMatrix(gPlayerInfo.objNode);


			/* FIND COORDS */

	FindCoordOfJoint(onion, 10, &gPlayerInfo.objNode->Coord);
}





//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME ONION ENEMY *************************/

Boolean PrimeEnemy_Onion(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */

	newObj = MakeEnemySkeleton(SKELETON_TYPE_ONION,x,z, ONION_SCALE, 0, nil);
	if (newObj == nil)
		return(false);


	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;

	SetSkeletonAnim(newObj->Skeleton, ONION_ANIM_WALK);


				/* SET BETTER INFO */

	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		-= newObj->BottomOff;
	newObj->SplineMoveCall 	= MoveOnionOnSpline;				// set move call
	newObj->Health 			= 1.0;
	newObj->Damage 			= 0;
	newObj->Kind 			= ENEMY_KIND_ONION;


				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,ONION_TARGET_OFFSET);

				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] = OnionEnemyHitByStunPulse;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] = OnionHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] = OnionGotPunched;
	newObj->HitByJumpJetHandler = OnionHitByJumpJet;

	newObj->HurtCallback = HurtOnion;							// set hurt callback function



	newObj->InitCoord = newObj->Coord;							// remember where started

				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 8, false);

	CreateOnionSparkles(newObj);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE ONION ON SPLINE ***************************/

static void MoveOnionOnSpline(ObjNode *theNode)
{
Boolean isVisible;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 30);
	GetObjectCoordOnSpline(theNode);


			/* UPDATE STUFF IF VISIBLE */

	if (isVisible)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BottomOff;	// calc y coord
		UpdateObjectTransforms(theNode);																// update transforms
		UpdateShadow(theNode);
		UpdateOnionSparkles(theNode);

				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))					// just do this to see if explosions hurt
			return;

					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < ONION_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveOnion);

	}


}




#pragma mark -




/***************** UPDATE ONION ************************/

static void UpdateOnion(ObjNode *theNode)
{
	theNode->DelayUntilAttack -= gFramesPerSecondFrac;

	UpdateEnemy(theNode);

	UpdateOnionSparkles(theNode);

	if (theNode->Skeleton->AnimNum != ONION_ANIM_JIGGLE)		// verify that player is freed if not jiggling
	{
		if (theNode->GrippingPlayer)
		{
			SetPlayerStandAnim(gPlayerInfo.objNode, 8);
		}
	}

}


#pragma mark -



/*************** CREATE ONION SPARKLES ********************/

static void CreateOnionSparkles(ObjNode *theNode)
{
short	i,j;

	for (j = 0; j < 2; j++)
	{

				/* CREATE EYE LIGHTS */

		i = theNode->Sparkles[j] = GetFreeSparkle(theNode);				// get free sparkle slot
		if (i == -1)
			return;

		gSparkles[i].flags = 0;
		gSparkles[i].where.x = theNode->Coord.x;
		gSparkles[i].where.y = theNode->Coord.y;
		gSparkles[i].where.z = theNode->Coord.z;

		gSparkles[i].aim.x =
		gSparkles[i].aim.y =
		gSparkles[i].aim.z = 1;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 0;
		gSparkles[i].color.b = 0;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 190.0f;
		gSparkles[i].separation = 10.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark3;
	}
}


/*************** UPDATE ONION SPARKLES ********************/

static void UpdateOnionSparkles(ObjNode *theNode)
{
short			i;
float			r,aimX,aimZ;
static const OGLPoint3D	leftEye = {-18,4,-38};
static const OGLPoint3D	rightEye = {18,4,-38};

	r = theNode->Rot.y;
	aimX = -sin(r);
	aimZ = -cos(r);

		/* UPDATE RIGHT EYE */

	i = theNode->Sparkles[0];												// get sparkle index
	if (i != -1)
	{
		FindCoordOnJoint(theNode, 0, &rightEye, &gSparkles[i].where);		// calc coord of right eye
		gSparkles[i].aim.x = aimX;											// update aim vector
		gSparkles[i].aim.z = aimZ;
	}


		/* UPDATE LEFT EYE */

	i = theNode->Sparkles[1];												// get sparkle index
	if (i != -1)
	{
		FindCoordOnJoint(theNode, 0, &leftEye, &gSparkles[i].where);		// calc coord of left eye
		gSparkles[i].aim.x = aimX;											// update aim vector
		gSparkles[i].aim.z = aimZ;
	}

}

#pragma mark -

/************ ONION HIT BY STUN PULSE *****************/
//
// Called when weapon is moved and this handler is called after a collision.
//

static Boolean OnionEnemyHitByStunPulse(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord)

			/* HURT IT */

	if (HurtOnion(enemy, .5))
		return(true);


	if (enemy->Skeleton->AnimNum != ONION_ANIM_GETHIT)
		MorphToSkeletonAnim(enemy->Skeleton, ONION_ANIM_GETHIT, 5);


			/* GIVE MOMENTUM */

	enemy->Delta.x += weaponDelta->x * .6f;
	enemy->Delta.y += weaponDelta->y * .6f;
	enemy->Delta.z += weaponDelta->z * .6f;

	return(true);
}

/************ ONION GOT PUNCHED *****************/
//
// returns true if want to disable punch after this
//

static Boolean OnionGotPunched(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta)
{
float	r;

#pragma unused (fistCoord, weapon, weaponDelta)

			/* HURT IT */

	if (HurtOnion(enemy, .5))
		return(true);


			/* GIVE MOMENTUM */

	r = gPlayerInfo.objNode->Rot.y;
	enemy->Delta.x = -sin(r) * 1000.0f;
	enemy->Delta.z = -cos(r) * 1000.0f;
	enemy->Delta.y = 500.0f;

	return(true);
}

/************** ONION GOT HIT BY SUPERNOVA *****************/

static Boolean OnionHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	KillOnion(enemy);

	return(false);
}


/************** ONION GOT HIT BY JUMPJET *****************/

static void OnionHitByJumpJet(ObjNode *enemy)
{
	KillOnion(enemy);
}


/*********************** HURT ONION ***************************/
//
// Returns true if onion deleted.
//

static Boolean HurtOnion(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveOnion);

		
			/* FINALIZE SCALE IF NOT DONE GROWING */
	
	if (enemy->Skeleton->AnimNum == ONION_ANIM_GROW)
	{
		float finalScale = MaxFloat(ONION_SCALE_MIN, enemy->Scale.y);
		FinalizeSkeletonObjectScale(enemy, finalScale, ONION_ANIM_STAND);
	}
	

				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		return (KillOnion(enemy));
	}
	else
	{
					/* GO INTO HIT ANIM */

		if (enemy->Skeleton->AnimNum != ONION_ANIM_GETHIT)
			MorphToSkeletonAnim(enemy->Skeleton, ONION_ANIM_GETHIT, 5);

		enemy->ButtTimer = 3.0f;
	}

	return(false);
}


/****************** KILL ONION *********************/
//
// OUTPUT: true if ObjNode was deleted
//

static Boolean KillOnion(ObjNode *enemy)
{
int	i;
ObjNode	*newObj;

	SpewAtoms(&enemy->Coord, 1,0, 2, false);

			/* RELEASE PLAYER */

	if (enemy->GrippingPlayer)
		SetPlayerStandAnim(gPlayerInfo.objNode, 8);


				/* SET DEFAULT STUFF */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.coord.x 	= enemy->Coord.x;
	gNewObjectDefinition.coord.y 	= enemy->Coord.y + enemy->BottomOff + 50.0f;
	gNewObjectDefinition.coord.z 	= enemy->Coord.z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveOnionSlice;
	gNewObjectDefinition.rot 		= 0;


			/*********************/
			/* MAKE ONION SLICES */
			/*********************/

	gNewObjectDefinition.type 		= FARM_ObjType_OnionSlice;

	for (i = 0; i < 5; i++)
	{
		gNewObjectDefinition.scale 		= .7f + RandomFloat2() * .1f;

		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Delta.y = 400.0f + RandomFloat() * 400.0f;
		newObj->Delta.x = RandomFloat2() * 500.0f;
		newObj->Delta.z = RandomFloat2() * 500.0f;

		newObj->DeltaRot.x = RandomFloat2() * 6.0f;
		newObj->DeltaRot.y = RandomFloat2() * 10.0f;
		newObj->DeltaRot.z = RandomFloat2() * 6.0f;

		gNewObjectDefinition.coord.y += 40.0f;
	}

			/* MAKE STALK */

	gNewObjectDefinition.scale 		= .7;
	gNewObjectDefinition.type 		= FARM_ObjType_OnionStalk;
	gNewObjectDefinition.coord.y 	+= 60.0f;

	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->Delta.y = 400.0f + RandomFloat() * 400.0f;
	newObj->Delta.x = RandomFloat2() * 500.0f;
	newObj->Delta.z = RandomFloat2() * 500.0f;

	newObj->DeltaRot.x = RandomFloat2() * 6.0f;
	newObj->DeltaRot.y = RandomFloat2() * 10.0f;
	newObj->DeltaRot.z = RandomFloat2() * 6.0f;


			/* EXPLODE GEOMETRY */

	ExplodeGeometry(enemy, 300, SHARD_MODE_BOUNCE, 2, .7);


	PlayEffect3D(EFFECT_ONIONSPLAT, &enemy->Coord);

			/* DELETE ORIGINAL ONION */

	DeleteEnemy(enemy);
	return(true);
}


/*********************** MOVE ONION SLICE **********************/

static void MoveOnionSlice(ObjNode *theNode)
{
float fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	gDelta.y -= 1000.0f * fps;				// gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.x += theNode->DeltaRot.x * fps;			// spin
	theNode->Rot.y += theNode->DeltaRot.y * fps;
	theNode->Rot.z += theNode->DeltaRot.z * fps;

			/* SEE IF FALLEN BELOW FLOOR */

	if (gCoord.y < (GetTerrainY(gCoord.x, gCoord.z) - 300.0f))
	{
		DeleteObject(theNode);
		return;
	}

	UpdateObject(theNode);
}









