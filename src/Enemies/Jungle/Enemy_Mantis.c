/****************************/
/*   ENEMY: MANTIS.C	*/
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

static void MoveMantis(ObjNode *theNode);
static void MoveMantis_Standing(ObjNode *theNode);
static void  MoveMantis_Walk(ObjNode *theNode);
static void MoveMantisOnSpline(ObjNode *theNode);
static void UpdateMantis(ObjNode *theNode);
static void  MoveMantis_Spew(ObjNode *theNode);
static void  MoveMantis_GotHit(ObjNode *theNode);
static void  MoveMantis_Death(ObjNode *theNode);

static Boolean MantisHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void MantisHitByJumpJet(ObjNode *enemy);
static void KillMantis(ObjNode *enemy);

static Boolean HurtMantis(ObjNode *enemy, float damage);

static void SeeIfMantisAttack(ObjNode *theNode, float angleToPlayer, float distToPlayer);
static void MoveMantisBullet(ObjNode *theNode);
static void MantisShoot(ObjNode *enemy);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_MANTISS					8

#define	MANTIS_SCALE					2.1f

#define	MANTIS_CHASE_DIST_MAX		2500.0f
#define	MANTIS_DETACH_DIST			(MANTIS_CHASE_DIST_MAX / 3.0f)

#define	MANTIS_ATTACK_DIST			1600.0f

#define	MANTIS_TARGET_OFFSET		100.0f

#define MANTIS_TURN_SPEED			1.3f
#define MANTIS_WALK_SPEED			280.0f


		/* ANIMS */


enum
{
	MANTIS_ANIM_STAND,
	MANTIS_ANIM_WALK,
	MANTIS_ANIM_SPEW,
	MANTIS_ANIM_GOTHIT,
	MANTIS_ANIM_DEATH
};

#define	MANTIS_JOINT_HEAD		6

#define	BULLET_SPEED				1000.0f

/*********************/
/*    VARIABLES      */
/*********************/

#define	SpewTimer		SpecialF[0]
#define	ButtTimer		SpecialF[1]
#define	AttackDelay		SpecialF[2]
#define	ShotSpacing		SpecialF[3]
#define	BurnTimer		SpecialF[4]

#define	BulletBounceCount	Special[0]

/************************ ADD MANTIS ENEMY *************************/

Boolean AddEnemy_Mantis(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;
int		i;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_MANTIS] >= MAX_MANTISS)
			return(false);
	}



				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_MANTIS,x,z, MANTIS_SCALE, 0, MoveMantis);
	newObj->TerrainItemPtr = itemPtr;
	newObj->EnemyRegenerate = itemPtr->parm[3] & (1<<1);

	SetSkeletonAnim(newObj->Skeleton, MANTIS_ANIM_STAND);


				/*******************/
				/* SET BETTER INFO */
				/*******************/

	newObj->Health 		= 1.0;
	newObj->Damage 		= .1;
	newObj->Kind 		= ENEMY_KIND_MANTIS;

	newObj->AttackDelay = 0;

				/* SET COLLISION INFO */

	SetObjectCollisionBounds(newObj, 200,-500,-120,120,120,-120);
	CalcNewTargetOffsets(newObj,MANTIS_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	for (i = 0; i < NUM_WEAPON_TYPES; i++)
		newObj->HitByWeaponHandler[i] = MantisHitByWeapon;
	newObj->HitByJumpJetHandler = MantisHitByJumpJet;

	newObj->HurtCallback = HurtMantis;							// set hurt callback function



				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 15, 15,false);


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_MANTIS]++;
	return(true);
}

/********************* MOVE MANTIS **************************/

static void MoveMantis(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveMantis_Standing,
					MoveMantis_Walk,
					MoveMantis_Spew,
					MoveMantis_GotHit,
					MoveMantis_Death,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE MANTIS: STANDING ******************************/

static void  MoveMantis_Standing(ObjNode *theNode)
{
float	angle,dist;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

				/* TURN TOWARDS ME */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, MANTIS_TURN_SPEED, true);



				/* SEE IF CHASE */

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
	if (dist < MANTIS_CHASE_DIST_MAX)
	{
		MorphToSkeletonAnim(theNode->Skeleton, MANTIS_ANIM_WALK, 4);
	}

			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;

				/* SEE IF DO ATTACK */

	SeeIfMantisAttack(theNode, angle, dist);

	UpdateMantis(theNode);
}




/********************** MOVE MANTIS: WALKING ******************************/

static void  MoveMantis_Walk(ObjNode *theNode)
{
float		r,fps,angle,dist;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, MANTIS_TURN_SPEED, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * MANTIS_WALK_SPEED;
	gDelta.z = -cos(r) * MANTIS_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


				/* SEE IF STAND */

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
	if (dist >= MANTIS_CHASE_DIST_MAX)
		MorphToSkeletonAnim(theNode->Skeleton, MANTIS_ANIM_STAND, 4);


				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == MANTIS_ANIM_WALK)
		theNode->Skeleton->AnimSpeed = MANTIS_WALK_SPEED * .005f;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;

				/* SEE IF DO ATTACK */

	SeeIfMantisAttack(theNode, angle, dist);


	UpdateMantis(theNode);
}


/********************** MOVE MANTIS: SPEW ******************************/

static void  MoveMantis_Spew(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	ApplyFrictionToDeltas(2000.0,&gDelta);


			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;

			/* SHOOT SPEW */

	MantisShoot(theNode);


			/* SEE IF DONE */

	theNode->SpewTimer -= fps;
	if (theNode->SpewTimer <= 0.0f)
	{
		MorphToSkeletonAnim(theNode->Skeleton, MANTIS_ANIM_STAND, 2);
	}

	UpdateMantis(theNode);
}


/********************** MOVE MANTIS: GOT HIT ******************************/

static void  MoveMantis_GotHit(ObjNode *theNode)
{
	if (theNode->Speed3D > 500.0f)
		TurnObjectTowardTarget(theNode, &gCoord, gCoord.x - gDelta.x, gCoord.z - gDelta.z, 10, false);	// aim with motion

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(1200.0,&gDelta);

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;			// add gravity

	MoveEnemy(theNode);


				/* SEE IF DONE */

	theNode->ButtTimer -= gFramesPerSecondFrac;
	if (theNode->ButtTimer <= 0.0)
	{
		MorphToSkeletonAnim(theNode->Skeleton, MANTIS_ANIM_STAND, 3);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateMantis(theNode);
}


/********************** MOVE MANTIS: DEATH ******************************/

static void  MoveMantis_Death(ObjNode *theNode)
{
			/* SEE IF GONE */

	if (theNode->StatusBits & STATUS_BIT_ISCULLED)		// if was culled on last frame and is far enough away, then delete it
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) > 1000.0f)
		{
			DeleteEnemy(theNode);
			return;
		}
	}


				/* MOVE IT */

	if (theNode->Speed3D > 500.0f)
		TurnObjectTowardTarget(theNode, &gCoord, gCoord.x - gDelta.x, gCoord.z - gDelta.z, 10, false);	// aim with motion

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)		// if on ground, add friction
		ApplyFrictionToDeltas(1500.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;		// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEATH_ENEMY_COLLISION_CTYPES, true))
		return;


				/* UPDATE */

	UpdateMantis(theNode);


}


//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME MANTIS ENEMY *************************/

Boolean PrimeEnemy_Mantis(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;
int				i;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */

	newObj = MakeEnemySkeleton(SKELETON_TYPE_MANTIS,x,z, MANTIS_SCALE,0,nil);


	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;

	SetSkeletonAnim(newObj->Skeleton, MANTIS_ANIM_WALK);
	newObj->Skeleton->AnimSpeed = 2.0f;

				/* SET BETTER INFO */

	newObj->InitCoord 		= newObj->Coord;							// remember where started
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveMantisOnSpline;					// set move call
	newObj->Health 			= 1.0;
	newObj->Damage 			= 0;
	newObj->Kind 			= ENEMY_KIND_MANTIS;


				/* SET COLLISION INFO */

	SetObjectCollisionBounds(newObj, 200,-500,-120,120,120,-120);
	CalcNewTargetOffsets(newObj,MANTIS_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	for (i = 0; i < NUM_WEAPON_TYPES; i++)
		newObj->HitByWeaponHandler[i] = MantisHitByWeapon;
	newObj->HitByJumpJetHandler = MantisHitByJumpJet;

	newObj->HurtCallback = HurtMantis;							// set hurt callback function



				/* MAKE SHADOW & GLOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 15, 15, false);



			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE MANTIS ON SPLINE ***************************/

static void MoveMantisOnSpline(ObjNode *theNode)
{
Boolean isVisible;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 70);
	GetObjectCoordOnSpline(theNode);


			/* UPDATE STUFF IF VISIBLE */

	if (isVisible)
	{

		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BottomOff;	// calc y coord
		UpdateObjectTransforms(theNode);											// update transforms
		UpdateShadow(theNode);

				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))					// just do this to see if explosions hurt
			return;


					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < MANTIS_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveMantis);

	}
}


/***************** UPDATE MANTIS ************************/

static void UpdateMantis(ObjNode *theNode)
{


	UpdateEnemy(theNode);

			/* UPDATE BURN */


	if (theNode->BurnTimer > 0.0f)
	{
		theNode->BurnTimer -= gFramesPerSecondFrac;
		BurnSkeleton(theNode, 50.0f);
	}
}




#pragma mark -



/************ MANTIS HIT BY WEAPON *****************/
//
// returns true if want to disable punch/weapon after this
//

static Boolean MantisHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
float	r;

#pragma unused (weaponCoord, weaponDelta)

		/* SEE IF SUPERNOVA */

	if (weapon == nil)
		KillMantis(enemy);

		/* NOT SUPERNOVA */

	else
	{
		switch(weapon->Kind)
		{
			case	WEAPON_TYPE_FLAME:
					enemy->BurnTimer = 3.0;
					if (gPlayerInfo.scaleRatio <= 1.0f)					// minimal damage if player is normal size
						HurtMantis(enemy, .09f);
					else
						HurtMantis(enemy, .3f);
					break;

			case	WEAPON_TYPE_STUNPULSE:
					if (gPlayerInfo.scaleRatio > 1.0f)
					{
						HurtMantis(enemy, .2);

						r = gPlayerInfo.objNode->Rot.y;					// give momentum
						enemy->Delta.x = -sin(r) * 1000.0f;
						enemy->Delta.z = -cos(r) * 1000.0f;
						enemy->Delta.y = 500.0f;
					}
					break;

			case	WEAPON_TYPE_FIST:
					if (gPlayerInfo.scaleRatio > 1.0f)
					{
						HurtMantis(enemy, .3);

						r = gPlayerInfo.objNode->Rot.y;					// give momentum
						enemy->Delta.x = -sin(r) * 1000.0f;
						enemy->Delta.z = -cos(r) * 1000.0f;
						enemy->Delta.y = 500.0f;
					}
					break;
		}
	}

	return(true);
}


/************** MANTIS HIT BY BY JUMPJET *****************/

static void MantisHitByJumpJet(ObjNode *enemy)
{
	if (gPlayerInfo.scaleRatio > 1.0f)
		KillMantis(enemy);
}




/*********************** HURT MANTIS ***************************/

static Boolean HurtMantis(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveMantis);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillMantis(enemy);
		return(true);
	}
	else
	{
					/* GO INTO HIT ANIM */

		if (enemy->Skeleton->AnimNum != MANTIS_ANIM_GOTHIT)
			MorphToSkeletonAnim(enemy->Skeleton, MANTIS_ANIM_GOTHIT, 5);

		enemy->ButtTimer = 3.0f;
	}
	return(false);
}


/****************** KILL MANTIS ***********************/

static void KillMantis(ObjNode *enemy)
{
int		i;

	enemy->CType = CTYPE_MISC;

	if (enemy->Skeleton->AnimNum != MANTIS_ANIM_DEATH)
	{
		MorphToSkeletonAnim(enemy->Skeleton, MANTIS_ANIM_DEATH, 5);
		SpewAtoms(&enemy->Coord, 0,1, 2, false);
	}

			/* DISABLE THE HANDLERS */

	for (i = 0; i < NUM_WEAPON_TYPES; i++)
		enemy->HitByWeaponHandler[i] = nil;
	enemy->HitByJumpJetHandler 	= nil;

	if (!enemy->EnemyRegenerate)
		enemy->TerrainItemPtr = nil;							// dont ever come back
}


#pragma mark -

/************ SEE IF MANTIS ATTACK *********************/

static void SeeIfMantisAttack(ObjNode *theNode, float angleToPlayer, float distToPlayer)
{
	if ((gPlayerInfo.invincibilityTimer > 0.0f) || (!gPlayerHasLanded))
		return;

			/* DON'T ATTACK THRU THINGS */

	if (SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE|CTYPE_BLOCKRAYS))
		return;


	theNode->AttackDelay -= gFramesPerSecondFrac;								// see if ready
	if (theNode->AttackDelay <= 0.0f)
	{
		if ((distToPlayer < MANTIS_ATTACK_DIST) && (angleToPlayer < PI))		// see if player is set for it
		{
			MorphToSkeletonAnim(theNode->Skeleton, MANTIS_ANIM_SPEW, 3);
			theNode->SpewTimer = 3.0;

			theNode->AttackDelay = 2.5;
		}
	}
}


/******************** MANTIS SHOOT ******************************/

static void MantisShoot(ObjNode *enemy)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*newObj;
static const OGLPoint3D	muzzleOff = {0,20,-60};
static const OGLVector3D muzzleAim = {0,.1,-1};
OGLVector3D	aim;
OGLMatrix4x4	m;

			/* SEE IF TIME TO FIRE OFF A SHOT */

	enemy->ShotSpacing -= fps;
	if (enemy->ShotSpacing <= 0.0f)
	{
		enemy->ShotSpacing += .1f + RandomFloat() * .2f;						// reset spacing timer

		PlayEffect3D(EFFECT_MANTISSPIT, &gCoord);

			/* CREATE BLOBULE */

		FindJointFullMatrix(enemy, MANTIS_JOINT_HEAD, &m);
		OGLPoint3D_Transform(&muzzleOff, &m, &gNewObjectDefinition.coord);	// calc start coord
		OGLVector3D_Transform(&muzzleAim, &m, &aim);						// calc delta/aim vector

		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= JUNGLE_ObjType_AcidDrop;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_USEALIGNMENTMATRIX;
		gNewObjectDefinition.slot 		= 502;
		gNewObjectDefinition.moveCall 	= MoveMantisBullet;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= .9;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Damage 			= .1;

		newObj->BulletBounceCount	 = 0;


			/* SET COLLISION */

		newObj->CType 			= CTYPE_HURTME;
		newObj->CBits			= CBITS_TOUCHABLE;
		CreateCollisionBoxFromBoundingBox(newObj, 1, 1);

		newObj->Delta.x = aim.x * BULLET_SPEED;
		newObj->Delta.y = aim.y * BULLET_SPEED;
		newObj->Delta.z = aim.z * BULLET_SPEED;

		newObj->DeltaRot.x = RandomFloat2() * PI2;
		newObj->DeltaRot.y = RandomFloat2() * PI2;
		newObj->DeltaRot.z = RandomFloat2() * PI2;

				/* SET THE ALIGNMENT MATRIX */

		SetAlignmentMatrix(&newObj->AlignmentMatrix, &aim);

		AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 4, 4, false);

	}
}


/******************* MOVE MANTIS BULLET ********************/

static void MoveMantisBullet(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
OGLVector3D	aim;


	GetObjectInfo(theNode);

			/* MOVE IT */

	gDelta.y -= 1600.0f * fps;
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


		/***********************/
		/* SEE IF HIT ANYTHING */
		/***********************/

	if (HandleCollisions(theNode, CTYPE_MISC|CTYPE_TERRAIN|CTYPE_FENCE, -.5f))
	{
		ExplodeGeometry(theNode, 200, SHARD_MODE_FROMORIGIN, 1, .5);			// impact splat

		MakePuff(&gCoord, 7.0, PARTICLE_SObjType_GreenFumes, GL_SRC_ALPHA, GL_ONE, 1.2);

		if (theNode->BulletBounceCount == 0)
			PlayEffect3D(EFFECT_ACIDSIZZLE, &gCoord);

		theNode->BulletBounceCount++;
		if (theNode->BulletBounceCount > 3)
		{
			DeleteObject(theNode);
			return;
		}
	}


		/* SET NEW ALIGNMENT & UPDATE */


	FastNormalizeVector(gDelta.x, gDelta.y, gDelta.z, &aim);
	SetAlignmentMatrix(&theNode->AlignmentMatrix, &aim);
	UpdateObject(theNode);
}


















