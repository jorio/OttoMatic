/****************************/
/*   ENEMY: BRAIN ALIEN.C	*/
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

static void MoveBrainAlien_Standing(ObjNode *theNode);
static void  MoveBrainAlien_Run(ObjNode *theNode);
static void  MoveBrainAlien_Walk(ObjNode *theNode);
static void  MoveBrainAlien_Death(ObjNode *theNode);
static void MoveBrainAlienOnSpline(ObjNode *theNode);
static void UpdateBrainAlien(ObjNode *theNode);
static void  MoveBrainAlien_BrainAttack(ObjNode *theNode);
static void  MoveBrainAlien_GotHit(ObjNode *theNode);
static void  MoveBrainAlien_HeadButt(ObjNode *theNode);
static void MoveBrainAlien_Frozen(ObjNode *theNode);

static void ShootBrainWave(ObjNode *enemy);
static void KillBrainAlien(ObjNode *enemy);

static void SeeIfBrainAlienAttack(ObjNode *theNode, float angleToPlayer, float distToPlayer);
static void MoveBrainWave(ObjNode *theNode);
static void MoveBrainWave_Dissipate(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_BRAINALIENS					7

#define	BRAINALIEN_SCALE					1.7f

#define	BRAINALIEN_CHASE_DIST_MAX		1500.0f
#define	BRAINALIEN_CHASE_DIST_MIN		300.0f
#define	BRAINALIEN_DETACH_DIST			(BRAINALIEN_CHASE_DIST_MAX / 2.0f)
#define	BRAINALIEN_ATTACK_DIST			1400.0f
#define	HEAD_BUTT_ATTACK_DIST			180.0f

#define	BRAINALIEN_TARGET_OFFSET			50.0f

#define BRAINALIEN_TURN_SPEED			3.0f
#define BRAINALIEN_WALK_SPEED			280.0f
#define BRAINALIEN_RUN_SPEED			700.0f

#define	WAVE_SPEED						1200.0f

#define	BRAIN_WAVE_DAMAGE				.08f
#define	BRAINALIEN_HEALTH				.7f

		/* ANIMS */


enum
{
	BRAINALIEN_ANIM_STAND,
	BRAINALIEN_ANIM_WALK,
	BRAINALIEN_ANIM_DEATH,
	BRAINALIEN_ANIM_BRAINATTACK,
	BRAINALIEN_ANIM_GETHIT,
	BRAINALIEN_ANIM_RUN,
	BRAINALIEN_ANIM_HEADBUTT
};


		/* JOINTS */

enum
{
	BRAINALIEN_JOINT_HEAD	= 	4
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	WaveSpacer			SpecialF[0]
#define	AttackDuration		SpecialF[1]
#define	DelayUntilAttack	SpecialF[2]
#define	FrozenTimer			SpecialF[3]
#define	ButtTimer			SpecialF[4]


/************************ ADD BRAINALIEN ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_BrainAlien(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_BRAINALIEN] >= MAX_BRAINALIENS)
			return(false);
	}

	newObj = MakeBrainAlien(x, z);

	newObj->TerrainItemPtr = itemPtr;

	return(true);
}

/************************* MAKE BRAINALIEN ****************************/

ObjNode *MakeBrainAlien(float x, float z)
{
ObjNode	*newObj;

				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_BRAINALIEN,x,z, BRAINALIEN_SCALE, 0, MoveBrainAlien);

	SetSkeletonAnim(newObj->Skeleton, BRAINALIEN_ANIM_STAND);


				/*******************/
				/* SET BETTER INFO */
				/*******************/

	newObj->Health 		= BRAINALIEN_HEALTH;
	newObj->Damage 		= .1;
	newObj->Kind 		= ENEMY_KIND_BRAINALIEN;

	newObj->FrozenTimer	= 0;

				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,BRAINALIEN_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= BrainAlienHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FREEZE] 		= BrainAlienHitByFreeze;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= BrainAlienHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] 		= BrainAlienGotPunched;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= BrainAlienHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLARE] 		= BrainAlienHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_DART] 		= BrainAlienHitByWeapon;
	newObj->HitByJumpJetHandler 						= BrainAlienHitByJumpJet;

	newObj->HurtCallback = HurtBrainAlien;							// set hurt callback function



				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 8,false);

	CreateBrainAlienGlow(newObj);

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_BRAINALIEN]++;



	return(newObj);

}





/********************* MOVE BRAINALIEN **************************/

void MoveBrainAlien(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveBrainAlien_Standing,
					MoveBrainAlien_Walk,
					MoveBrainAlien_Death,
					MoveBrainAlien_BrainAttack,
					MoveBrainAlien_GotHit,
					MoveBrainAlien_Run,
					MoveBrainAlien_HeadButt,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

			/* SEE IF FROZEN */

	if (theNode->FrozenTimer > 0.0f)
		MoveBrainAlien_Frozen(theNode);
	else
		myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE BRAINALIEN: STANDING ******************************/

static void  MoveBrainAlien_Standing(ObjNode *theNode)
{
float	angleToTarget,dist;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)									// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

				/* TURN TOWARDS ME */

	angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, BRAINALIEN_TURN_SPEED, true);



				/* SEE IF CHASE */

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);

	if (theNode->Kind == ENEMY_KIND_ELITEBRAINALIEN)					// elites are always running
		MorphToSkeletonAnim(theNode->Skeleton, BRAINALIEN_ANIM_RUN, 4);
	else
	if (!IsBottomlessPitInFrontOfEnemy(theNode->Rot.y))
	{
		if ((dist < BRAINALIEN_CHASE_DIST_MAX) && (dist > BRAINALIEN_CHASE_DIST_MIN))
		{
			if (MyRandomLong()&1)													// randomly choose walk or run
				MorphToSkeletonAnim(theNode->Skeleton, BRAINALIEN_ANIM_RUN, 4);
			else
				MorphToSkeletonAnim(theNode->Skeleton, BRAINALIEN_ANIM_WALK, 4);
		}
	}
			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;									// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


				/* SEE IF DO ATTACK */

	SeeIfBrainAlienAttack(theNode, angleToTarget, dist);


	UpdateBrainAlien(theNode);
}




/********************** MOVE BRAINALIEN: WALKING ******************************/

static void  MoveBrainAlien_Walk(ObjNode *theNode)
{
float		r,fps,angle,dist;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, BRAINALIEN_TURN_SPEED, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * BRAINALIEN_WALK_SPEED;
	gDelta.z = -cos(r) * BRAINALIEN_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


				/* SEE IF STAND */

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);

	if (IsBottomlessPitInFrontOfEnemy(r))							// check for bottomless pits on Cloud level
	{
		MorphToSkeletonAnim(theNode->Skeleton, BRAINALIEN_ANIM_STAND, 4);
	}
	else
	{
		if ((dist >= BRAINALIEN_CHASE_DIST_MAX) || (dist <= BRAINALIEN_CHASE_DIST_MIN))
		{
			if (theNode->Kind == ENEMY_KIND_BRAINALIEN)
				MorphToSkeletonAnim(theNode->Skeleton, BRAINALIEN_ANIM_STAND, 4);
		}
	}

				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == BRAINALIEN_ANIM_WALK)
		theNode->Skeleton->AnimSpeed = BRAINALIEN_WALK_SPEED * .01f;


				/* SEE IF DO ATTACK */

	SeeIfBrainAlienAttack(theNode, angle, dist);

				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateBrainAlien(theNode);
}


/********************** MOVE BRAINALIEN: RUN ******************************/

static void  MoveBrainAlien_Run(ObjNode *theNode)
{
float		r,fps,angle,dist;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, BRAINALIEN_TURN_SPEED, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * BRAINALIEN_RUN_SPEED;
	gDelta.z = -cos(r) * BRAINALIEN_RUN_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


				/* SEE IF STAND */

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
	if (IsBottomlessPitInFrontOfEnemy(r))							// check for bottomless pits on Cloud level
	{
		MorphToSkeletonAnim(theNode->Skeleton, BRAINALIEN_ANIM_STAND, 4);
	}
	else
	if (theNode->Kind == ENEMY_KIND_BRAINALIEN)		// elites keep going, but regulars can stop
	{
		if ((dist >= BRAINALIEN_CHASE_DIST_MAX) || (dist <= BRAINALIEN_CHASE_DIST_MIN))
			MorphToSkeletonAnim(theNode->Skeleton, BRAINALIEN_ANIM_STAND, 4);
	}

				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == BRAINALIEN_ANIM_RUN)
		theNode->Skeleton->AnimSpeed = BRAINALIEN_RUN_SPEED * .003f;


				/* SEE IF DO ATTACK */

	SeeIfBrainAlienAttack(theNode, angle, dist);

				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateBrainAlien(theNode);
}


/********************** MOVE BRAINALIEN: GOT HIT ******************************/

static void  MoveBrainAlien_GotHit(ObjNode *theNode)
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
		MorphToSkeletonAnim(theNode->Skeleton, BRAINALIEN_ANIM_STAND, 3);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateBrainAlien(theNode);
}


/********************** MOVE BRAINALIEN: BRAIN ATTACK ******************************/

static void  MoveBrainAlien_BrainAttack(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(3000.0,&gDelta);


			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


			/* SHOOT BRAIN WAVE */

	theNode->WaveSpacer -= fps;
	if (theNode->WaveSpacer <= 0.0f)
	{
		theNode->WaveSpacer += .2f;
		ShootBrainWave(theNode);
	}



				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


				/* SEE IF STOP */

	theNode->AttackDuration -= fps;
	if (theNode->AttackDuration <= 0.0f)
		MorphToSkeletonAnim(theNode->Skeleton, BRAINALIEN_ANIM_STAND, 8);

	UpdateBrainAlien(theNode);
}

/********************** MOVE BRAINALIEN: HEAD BUTT ******************************/

static void  MoveBrainAlien_HeadButt(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
static const OGLPoint3D headOff = {0,40,-20};
OGLPoint3D	headCoord;


			/* MOVE */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, BRAINALIEN_TURN_SPEED, false);
	ApplyFrictionToDeltas(3000.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

				/* SEE IF HEAD HIT PLAYER */

	FindCoordOnJoint(theNode, BRAINALIEN_JOINT_HEAD, &headOff, &headCoord);			// calc coord of forehead

	if (DoSimpleBoxCollision(headCoord.y + 30.0f, headCoord.y - 30.0f, headCoord.x - 30.0f, headCoord.x + 30.0f,
							headCoord.z + 30.0f, headCoord.z - 30.0f, CTYPE_PLAYER))
	{
		if (gPlayerInfo.invincibilityTimer <= 0.0f)
			PlayEffect3D(EFFECT_HEADTHUD, &headCoord);

		theNode->Damage = .1;									// set damage of headbutt
		PlayerGotHit(theNode, 0);			// hurt the player

	}


				/* SEE IF STOP */

	if (theNode->Skeleton->AnimHasStopped)
		MorphToSkeletonAnim(theNode->Skeleton, BRAINALIEN_ANIM_STAND, 6);

	UpdateBrainAlien(theNode);
}



/********************** MOVE BRAINALIEN: DEATH ******************************/

static void  MoveBrainAlien_Death(ObjNode *theNode)
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

	UpdateBrainAlien(theNode);


}



/********************* MOVE BRAIN ALIEN: FROZEN **************************/

static void MoveBrainAlien_Frozen(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->Skeleton->AnimSpeed = 0;							// make sure animation is frozen solid

				/* MOVE IT */

	ApplyFrictionToDeltas(1100.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*fps;								// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEATH_ENEMY_COLLISION_CTYPES, true))
		return;


				/* UPDATE */

	UpdateBrainAlien(theNode);

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



//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME BRAINALIEN ENEMY *************************/

Boolean PrimeEnemy_BrainAlien(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */

	newObj = MakeEnemySkeleton(SKELETON_TYPE_BRAINALIEN,x,z, BRAINALIEN_SCALE, 0, nil);


	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;

	SetSkeletonAnim(newObj->Skeleton, BRAINALIEN_ANIM_WALK);
	newObj->Skeleton->AnimSpeed = 2.0f;

				/* SET BETTER INFO */

	newObj->InitCoord 		= newObj->Coord;							// remember where started
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		-= newObj->BottomOff;
	newObj->SplineMoveCall 	= MoveBrainAlienOnSpline;					// set move call
	newObj->Health 			= BRAINALIEN_HEALTH;
	newObj->Damage 			= 0;
	newObj->Kind 			= ENEMY_KIND_BRAINALIEN;

	newObj->FrozenTimer		= 0.0f;

				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,BRAINALIEN_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= BrainAlienHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FREEZE] 		= BrainAlienHitByFreeze;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= BrainAlienHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] 		= BrainAlienGotPunched;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= BrainAlienHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLARE] 		= BrainAlienHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_DART] 		= BrainAlienHitByWeapon;
	newObj->HitByJumpJetHandler 						= BrainAlienHitByJumpJet;

	newObj->HurtCallback = HurtBrainAlien;							// set hurt callback function



				/* MAKE SHADOW & GLOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 8, false);

	CreateBrainAlienGlow(newObj);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE BRAINALIEN ON SPLINE ***************************/

static void MoveBrainAlienOnSpline(ObjNode *theNode)
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
		UpdateBrainAlienGlow(theNode);												// update glow

				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))					// just do this to see if explosions hurt
			return;


					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < BRAINALIEN_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveBrainAlien);

	}
}


/***************** UPDATE BRAINALIEN ************************/

static void UpdateBrainAlien(ObjNode *theNode)
{
	UpdateEnemy(theNode);
	UpdateBrainAlienGlow(theNode);

			/* UPDATE SOUND */

	if (theNode->Skeleton->AnimNum == BRAINALIEN_ANIM_BRAINATTACK)
	{
		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_BRAINWAVE, &gCoord, NORMAL_CHANNEL_RATE, 1.0);
		else
			Update3DSoundChannel(EFFECT_BRAINWAVE, &theNode->EffectChannel, &gCoord);
	}
	else
		StopAChannel(&theNode->EffectChannel);					// make sure off since not doing brain wave attack


}


#pragma mark -

/************ BRAIN ALIEN HIT BY STUN PULSE *****************/
//
// Called when weapon is moved and this handler is called after a collision.
//
// Also used for Flame weapon!
//

Boolean BrainAlienHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weaponCoord)


			/* HURT IT */

	HurtBrainAlien(enemy, weapon->Damage);


			/* GIVE MOMENTUM */

	enemy->Delta.x += weaponDelta->x * .4f;
	enemy->Delta.y += weaponDelta->y * .4f  + 200.0f;
	enemy->Delta.z += weaponDelta->z * .4f;

	return(true);			// stop weapon
}


/************ BRAIN ALIEN HIT BY FREEZE *****************/

Boolean BrainAlienHitByFreeze(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

			/* CHANGE THE TEXTURE */

	enemy->Skeleton->overrideTexture = gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_FrozenBrainAlien].materialObject;


			/* STOP MOMENTUM */

	enemy->Delta.x =
	enemy->Delta.y =
	enemy->Delta.z = 0.0f;

	enemy->FrozenTimer = 4.0;			// set freeze timer

	return(true);			// stop weapon
}





/************ BRAIN ALIEN GOT PUNCHED *****************/
//
// returns true if want to disable punch after this
//

Boolean BrainAlienGotPunched(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta)
{
float	r;

#pragma unused (fistCoord, weaponDelta, weapon)

			/********************************/
			/* SEE IF PUNCHING FROZEN ALIEN */
			/********************************/

	if (enemy->FrozenTimer > 0.0f)
	{
		KillBrainAlien(enemy);
	}

			/**********************************/
			/* NOT FROZEN, SO HANDLE NORMALLY */
			/**********************************/
	else
	{

				/* HURT IT */

		HurtBrainAlien(enemy, .4);


				/* GIVE MOMENTUM */

		r = gPlayerInfo.objNode->Rot.y;

		enemy->Delta.x = -sin(r) * 1000.0f;
		enemy->Delta.z = -cos(r) * 1000.0f;
		enemy->Delta.y = 500.0f;
	}

	return(true);
}


/************ BRAIN ALIEN GOT HIT BY JUMP JET *****************/

void BrainAlienHitByJumpJet(ObjNode *enemy)
{
float	r;

		/* HURT IT */

	HurtBrainAlien(enemy, 1.0);


			/* GIVE MOMENTUM */

	r = gPlayerInfo.objNode->Rot.y;
	enemy->Delta.x = -sin(r) * 1000.0f;
	enemy->Delta.z = -cos(r) * 1000.0f;
	enemy->Delta.y = 500.0f;
}

/************** BRAIN ALIEN GOT HIT BY SUPERNOVA *****************/

Boolean BrainAlienHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	KillBrainAlien(enemy);

	return(false);
}




/*********************** HURT BRAIN ALIEN ***************************/

Boolean HurtBrainAlien(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveBrainAlien);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillBrainAlien(enemy);
		return(true);
	}
	else
	{
					/* GO INTO HIT ANIM */

		if (enemy->Skeleton->AnimNum != BRAINALIEN_ANIM_GETHIT)
			MorphToSkeletonAnim(enemy->Skeleton, BRAINALIEN_ANIM_GETHIT, 5);

		enemy->ButtTimer = 3.0f;
	}

	return(false);
}


/****************** KILL BRAIN ALIEN ***********************/

static void KillBrainAlien(ObjNode *enemy)
{
OGLPoint3D	where = enemy->Coord;							// make copy of coord
int	i;
			/*******************************/
			/* IF FROZEN THEN JUST SHATTER */
			/*******************************/

	if (enemy->FrozenTimer > 0.0f)
	{
		ExplodeGeometry(enemy, 400, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 2, .6);
		PlayEffect3D(EFFECT_SHATTER, &enemy->Coord);
		if (!enemy->EnemyRegenerate)
			enemy->TerrainItemPtr = nil;							// dont ever come back
		DeleteEnemy(enemy);
	}

			/**************/
			/* NOT FROZEN */
			/**************/
	else
	{
		enemy->CType = CTYPE_MISC;
		enemy->TopOff = 10;

		if (enemy->Skeleton->AnimNum != BRAINALIEN_ANIM_DEATH)
		{
			MorphToSkeletonAnim(enemy->Skeleton, BRAINALIEN_ANIM_DEATH, 5);
			PlayEffect3D(EFFECT_BRAINDIE, &enemy->Coord);
		}


				/* DISABLE THE HANDLERS */

		for (i = 0; i < NUM_WEAPON_TYPES; i++)
			enemy->HitByWeaponHandler[i] = nil;
		enemy->HitByJumpJetHandler 	= nil;
	}

			/* SPEW ATOMS */

	switch(gLevelNum)
	{
		case	LEVEL_NUM_BRAINBOSS:
				if (MyRandomLong()&0x7)
					SpewAtoms(&where, 0,0, 2, false);
				else
					SpewAtoms(&where, 1,0, 1, false);
				break;

		default:
				SpewAtoms(&where, 0,1, 2, false);
	}
}


#pragma mark -

/************ SEE IF BRAIN ALIEN ATTACK *********************/

static void SeeIfBrainAlienAttack(ObjNode *theNode, float angleToPlayer, float distToPlayer)
{
	if (!gPlayerHasLanded)
		return;

	if (gPlayerInfo.invincibilityTimer > 0.0f)
		return;

			/* DON'T ATTACK THRU THINGS */

	if (SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE|CTYPE_BLOCKRAYS))
		return;


			/*************/
			/* HEAD BUTT */
			/*************/

	if ((distToPlayer < HEAD_BUTT_ATTACK_DIST) && (angleToPlayer < (PI/4.0f)))
	{
		MorphToSkeletonAnim(theNode->Skeleton, BRAINALIEN_ANIM_HEADBUTT, 7);
		PlayEffect3D(EFFECT_HEADSWOOSH, &gCoord);
	}

			/**************/
			/* BRAIN WAVE */
			/**************/

	else
	{
		theNode->DelayUntilAttack -= gFramesPerSecondFrac;
		if (theNode->DelayUntilAttack <= 0.0f)
		{
			if (theNode->Kind == ENEMY_KIND_ELITEBRAINALIEN)		// quicker attacks if elite dudes
				theNode->DelayUntilAttack = 1.5f;
			else
				theNode->DelayUntilAttack = 2.5f;
			if (angleToPlayer < (PI/6.0f))
			{
				if (distToPlayer < BRAINALIEN_ATTACK_DIST)
				{
					MorphToSkeletonAnim(theNode->Skeleton, BRAINALIEN_ANIM_BRAINATTACK, 5);
					theNode->WaveSpacer = .3f;
					theNode->AttackDuration = 1.5f;
				}
			}
		}
	}
}


/*********************** SHOOT BRAIN WAVE ***************************/

static void ShootBrainWave(ObjNode *enemy)
{
ObjNode	*wave;
static const OGLPoint3D brainOff = {0,20,-30};

				/* CALC COORD ON BRAIN */

	FindCoordOnJoint(enemy, BRAINALIEN_JOINT_HEAD, &brainOff, &gNewObjectDefinition.coord);


				/* CREATE WEAPON OBJECT */

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_BrainWave;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOLIGHTING|
									STATUS_BIT_NOTEXTUREWRAP|STATUS_BIT_ROTXZY|STATUS_BIT_NOZWRITES|STATUS_BIT_GLOW;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveBrainWave;
	gNewObjectDefinition.rot 		= enemy->Rot.y;
	gNewObjectDefinition.scale 		= .3;
	wave = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	wave->Damage = BRAIN_WAVE_DAMAGE;
	wave->Health = .7f;

	wave->Delta.x = -sin(wave->Rot.y) * WAVE_SPEED;
	wave->Delta.z = -cos(wave->Rot.y) * WAVE_SPEED;

	wave->ColorFilter.a = .999f;

	AttachShadowToObject(wave, SHADOW_TYPE_CIRCULAR, 5, .5, false);

}


/******************** MOVE BRAIN WAVE **************************/

static void MoveBrainWave(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

			/* MOVE IT */

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.z = RandomFloat()*PI2;

	theNode->Scale.z += fps * .2f;
	theNode->Scale.x = theNode->Scale.z + RandomFloat()*.4f;
	theNode->Scale.y = theNode->Scale.z + RandomFloat()*.4f;


				/* COLLISION */

	if (HandleCollisions(theNode, CTYPE_MISC, 0))
		theNode->MoveCall = MoveBrainWave_Dissipate;
	if (gCoord.y < GetTerrainY(gCoord.x, gCoord.z))
		theNode->MoveCall = MoveBrainWave_Dissipate;




			/* DECAY IT */

	theNode->Health -= fps * 1.3f;
	if (theNode->Health < 0.0f)								// see if fading now
	{
		theNode->ColorFilter.a -= fps * 3.0f;
		if (theNode->ColorFilter.a <= 0.0f)					// is it gone?
		{
			DeleteObject(theNode);
			return;
		}
	}

	theNode->ColorFilter.g -= fps;
	if (theNode->ColorFilter.g < 0.0f)
		theNode->ColorFilter.g = 0;


		/* SEE IF HIT PLAYER */

	if (gPlayerInfo.invincibilityTimer <= 0.0f)
	{
		if (CalcDistance3D(gCoord.x, gCoord.y, gCoord.z,
						 gPlayerInfo.coord.x, gPlayerInfo.coord.y, gPlayerInfo.coord.z) < 100.0f)
		{
			PlayerGotHit(theNode, 0);
			theNode->MoveCall = MoveBrainWave_Dissipate;
		}
	}


			/* UPDATE */

	UpdateObject(theNode);
}


/************* MOVE BRAIN WAVE: DISSIPATE ******************/

static void MoveBrainWave_Dissipate(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->Scale.x = theNode->Scale.y = theNode->Scale.z += fps * 5.0f;

	theNode->ColorFilter.a -= fps * 3.0f;
	if (theNode->ColorFilter.a <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}

	UpdateObjectTransforms(theNode);
}


#pragma mark -


/*************** CREATE BRAIN ALIEN GLOW ****************/

void CreateBrainAlienGlow(ObjNode *alien)
{
int	numJoints,i,s;

	numJoints = alien->Skeleton->skeletonDefinition->NumBones;
	if (numJoints > MAX_NODE_SPARKLES)				// make sure we don't overflow
		numJoints = MAX_NODE_SPARKLES;

	for (i = 0; i < numJoints; i++)
	{
		s = alien->Sparkles[i] = GetFreeSparkle(alien);		// get free sparkle slot
		if (s != -1)
		{
			gSparkles[s].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
			gSparkles[s].where = alien->Coord;

			if (alien->Type == SKELETON_TYPE_ELITEBRAINALIEN)
			{
				gSparkles[s].color.r = 1;
				gSparkles[s].color.g = .7;
			}
			else
			{
				gSparkles[s].color.r = .7;
				gSparkles[s].color.g = 1;
			}

			gSparkles[s].color.b = .5;
			gSparkles[s].color.a = alien->ColorFilter.a * .6f;

			gSparkles[s].scale = 150.0f;
			gSparkles[s].separation = -50.0f;

			gSparkles[s].textureNum = PARTICLE_SObjType_WhiteGlow;
		}
	}
}


/**************** UPDATE BRAIN ALIEN GLOW *******************/

void UpdateBrainAlienGlow(ObjNode *alien)
{
int	numJoints,i,s;
static const OGLPoint3D off[] =
{
	{0,		0,		0	},
	{0,		0,		0	},		// chest
	{0,		0,		-10	},		// neck 1
	{0,		13,		-13	},		// neck 2
	{0,		23,		-20	},		// head
	{0,		0,		0	},
	{0,		-10,	0	},		// left elbow
	{0,		-15,	0	},		// left hand
	{0,		0,		0	},		// rt hip
	{0,		0,		0	},		// rt knee
	{0,		0,		0	},		// left hip
	{0,		0,		0	},		// left knee
	{0,		-5,		-15	},		// rt foot
	{0,		-5,		-15	},		// left foot
	{0,		0,		0	},
	{0,		-10,	0	},		// rt elbow
	{0,		-15,	0	},		// rt hand
};

static const float scale[] =
{
	1.3,
	.7,		// chest
	.5,		// neck 1
	1.2,	// neck 2
	1.8,	// head
	1,
	.8,		// left elbow
	.8,		// left hand
	.4,		// rt hip
	.6,		// rt knee
	.4,		// left hip
	.6,		// left knee
	1,		// rt foot
	1,		// left foot
	1,
	.8,		// rt elbow
	.8,		// rt hand
};


	numJoints = alien->Skeleton->skeletonDefinition->NumBones;
	if (numJoints > MAX_NODE_SPARKLES)				// make sure we don't overflow
		numJoints = MAX_NODE_SPARKLES;

	for (i = 0; i < numJoints; i++)
	{
		s = alien->Sparkles[i];
		if (s != -1)
		{
			FindCoordOnJoint(alien, i, &off[i], &gSparkles[s].where);
			gSparkles[s].scale = (50.0f + RandomFloat()*10.0f) * scale[i];
			gSparkles[s].color.a = alien->ColorFilter.a * .6f;
		}
	}
}


#pragma mark -

/********************* MOVE ELITE BRAIN ALIEN: FROM PORTAL ***********************/

void MoveEliteBrainAlien_FromPortal(ObjNode *theNode)
{
float		r,fps,c,a;

	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);


	r = theNode->Rot.y;
	gDelta.x = -sin(r) * BRAINALIEN_WALK_SPEED;
	gDelta.z = -cos(r) * BRAINALIEN_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


				/* SEE IF DONE */

	theNode->Timer -= fps;
	if (theNode->Timer <= 0.0f)
	{
		theNode->MoveCall = MoveBrainAlien;
		MorphToSkeletonAnim(theNode->Skeleton, BRAINALIEN_ANIM_RUN, 4);

					/* SET WEAPON HANDLERS */

		theNode->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= BrainAlienHitByWeapon;
		theNode->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= BrainAlienHitBySuperNova;
		theNode->HitByWeaponHandler[WEAPON_TYPE_FIST] 		= BrainAlienGotPunched;
		theNode->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= BrainAlienHitByWeapon;
		theNode->HitByWeaponHandler[WEAPON_TYPE_FLARE] 		= BrainAlienHitByWeapon;

		theNode->HurtCallback = HurtBrainAlien;							// set hurt callback function
	}

				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


			/* UPDATE FADE-IN COLORING */

	c = theNode->ColorFilter.r += fps;
	if (c > 1.0f)
		c = 1.0f;

	a = theNode->ColorFilter.a += fps;
	if (a >= 1.0f)
		a = 1.0f;

	theNode->ColorFilter.r = theNode->ColorFilter.g = theNode->ColorFilter.b = c;
	theNode->ColorFilter.a = a;


	UpdateBrainAlien(theNode);
}















