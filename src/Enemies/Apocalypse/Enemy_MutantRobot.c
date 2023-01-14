/****************************/
/*   ENEMY: MUTANT ROBOT.C	*/
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

static void MoveMutantRobot(ObjNode *theNode);
static void MoveMutantRobot_Standing(ObjNode *theNode);
static void  MoveMutantRobot_Run(ObjNode *theNode);
static void  MoveMutantRobot_Walk(ObjNode *theNode);
static void  MoveMutantRobot_Death(ObjNode *theNode);
static void MoveMutantRobotOnSpline(ObjNode *theNode);
static void UpdateMutantRobot(ObjNode *theNode);
static void  MoveMutantRobot_GotHit(ObjNode *theNode);
static void  MoveMutantRobot_Attack(ObjNode *theNode);

static Boolean MutantRobotHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean MutantRobotHitByStunPulse(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean MutantRobotGotPunched(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta);
static void KillMutantRobot(ObjNode *enemy);

static Boolean SeeIfMutantRobotAttack(ObjNode *theNode, float angleToPlayer, float distToPlayer);
static Boolean HurtMutantRobot(ObjNode *enemy, float damage);
static void MutantRobotHitByJumpJet(ObjNode *enemy);

static void MoveMutantRobotBullet(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_MUTANTROBOTS					8

#define	MUTANTROBOT_SCALE					1.9f

#define	MUTANTROBOT_CHASE_DIST_MAX		1500.0f
#define	MUTANTROBOT_CHASE_DIST_MIN		300.0f
#define	MUTANTROBOT_DETACH_DIST			(MUTANTROBOT_CHASE_DIST_MAX / 2.0f)
#define	MUTANTROBOT_ATTACK_DIST			400.0f

#define	MUTANTROBOT_TARGET_OFFSET			50.0f

#define MUTANTROBOT_TURN_SPEED			3.0f
#define MUTANTROBOT_WALK_SPEED			280.0f
#define MUTANTROBOT_RUN_SPEED			700.0f

#define	WAVE_SPEED						1200.0f

#define	BRAIN_WAVE_DAMAGE				.08f

		/* ANIMS */


enum
{
	MUTANTROBOT_ANIM_STAND,
	MUTANTROBOT_ANIM_WALK,
	MUTANTROBOT_ANIM_DEATH,
	MUTANTROBOT_ANIM_ATTACK,
	MUTANTROBOT_ANIM_GETHIT,
	MUTANTROBOT_ANIM_RUN,
	MUTANTROBOT_ANIM_HEADBUTT
};


		/* JOINTS */

enum
{
	MUTANTROBOT_JOINT_ANTENNA	= 	4,
	MUTANTROBOT_JOINT_RTARM		= 	15
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	AttackDuration		SpecialF[1]
#define	DelayUntilAttack	SpecialF[2]
#define	ButtTimer			SpecialF[4]



static const OGLPoint3D 	gMuzzleTipOff = {0,9,-32};
static const OGLVector3D	gMuzzleTipAim = {0,-10,-13};

/************************ ADD MUTANTROBOT ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_MutantRobot(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_MUTANTROBOT] >= MAX_MUTANTROBOTS)
			return(false);
	}



				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_MUTANTROBOT,x,z, MUTANTROBOT_SCALE, 0, MoveMutantRobot);
	newObj->TerrainItemPtr = itemPtr;
	newObj->EnemyRegenerate = itemPtr->parm[3] & (1<<1);

	SetSkeletonAnim(newObj->Skeleton, MUTANTROBOT_ANIM_STAND);


				/*******************/
				/* SET BETTER INFO */
				/*******************/

	newObj->Health 		= 1.0;
	newObj->Damage 		= .1;
	newObj->Kind 		= ENEMY_KIND_MUTANTROBOT;

				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,MUTANTROBOT_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= MutantRobotHitByStunPulse;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= MutantRobotHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] 		= MutantRobotGotPunched;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= MutantRobotHitByStunPulse;
	newObj->HitByJumpJetHandler 						= MutantRobotHitByJumpJet;

	newObj->HurtCallback = HurtMutantRobot;							// set hurt callback function



				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 8,false);


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_MUTANTROBOT]++;
	return(true);
}

/********************* MOVE MUTANTROBOT **************************/

static void MoveMutantRobot(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveMutantRobot_Standing,
					MoveMutantRobot_Walk,
					MoveMutantRobot_Death,
					MoveMutantRobot_Attack,
					MoveMutantRobot_GotHit,
					MoveMutantRobot_Run,
					nil,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE MUTANTROBOT: STANDING ******************************/

static void  MoveMutantRobot_Standing(ObjNode *theNode)
{
float	angleToTarget,dist;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

				/* TURN TOWARDS ME */

	angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, MUTANTROBOT_TURN_SPEED, true);



				/* SEE IF CHASE */

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
	if ((dist < MUTANTROBOT_CHASE_DIST_MAX) && (dist > MUTANTROBOT_CHASE_DIST_MIN))
	{
		if (MyRandomLong()&1)										// randomly choose walk or run
			MorphToSkeletonAnim(theNode->Skeleton, MUTANTROBOT_ANIM_RUN, 4);
		else
			MorphToSkeletonAnim(theNode->Skeleton, MUTANTROBOT_ANIM_WALK, 4);
	}

			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


				/* SEE IF DO ATTACK */

	SeeIfMutantRobotAttack(theNode, angleToTarget, dist);


	UpdateMutantRobot(theNode);
}


/********************** MOVE MUTANTROBOT: ATTACK ******************************/

static void  MoveMutantRobot_Attack(ObjNode *theNode)
{
float	angleToTarget,fps = gFramesPerSecondFrac;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

				/* TURN TOWARDS ME */

	angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, MUTANTROBOT_TURN_SPEED, true);

			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


				/* MAINTAIN ATTACK */

	theNode->DelayUntilAttack -= fps;
	if (theNode->DelayUntilAttack <= 0.0f)
	{
		if (!SeeIfMutantRobotAttack(theNode, angleToTarget, 0))
			MorphToSkeletonAnim(theNode->Skeleton, MUTANTROBOT_ANIM_STAND, 4);
	}

				/* SEE IF DONE */

	theNode->AttackDuration -= fps;
	if (theNode->AttackDuration <= 0.0f)
		MorphToSkeletonAnim(theNode->Skeleton, MUTANTROBOT_ANIM_STAND, 4);


	UpdateMutantRobot(theNode);
}



/********************** MOVE MUTANTROBOT: WALKING ******************************/

static void  MoveMutantRobot_Walk(ObjNode *theNode)
{
float		r,fps,angle,dist;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, MUTANTROBOT_TURN_SPEED, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * MUTANTROBOT_WALK_SPEED;
	gDelta.z = -cos(r) * MUTANTROBOT_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


				/* SEE IF STAND */

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
	if ((dist >= MUTANTROBOT_CHASE_DIST_MAX) || (dist <= MUTANTROBOT_CHASE_DIST_MIN))
		MorphToSkeletonAnim(theNode->Skeleton, MUTANTROBOT_ANIM_STAND, 4);


				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == MUTANTROBOT_ANIM_WALK)
		theNode->Skeleton->AnimSpeed = MUTANTROBOT_WALK_SPEED * .008f;


				/* SEE IF DO ATTACK */

	SeeIfMutantRobotAttack(theNode, angle, dist);

				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;

	UpdateMutantRobot(theNode);
}


/********************** MOVE MUTANTROBOT: RUN ******************************/

static void  MoveMutantRobot_Run(ObjNode *theNode)
{
float		r,fps,angle,dist;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, MUTANTROBOT_TURN_SPEED, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * MUTANTROBOT_RUN_SPEED;
	gDelta.z = -cos(r) * MUTANTROBOT_RUN_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


				/* SEE IF STAND */

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
	if ((dist >= MUTANTROBOT_CHASE_DIST_MAX) || (dist <= MUTANTROBOT_CHASE_DIST_MIN))
		MorphToSkeletonAnim(theNode->Skeleton, MUTANTROBOT_ANIM_STAND, 4);


				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == MUTANTROBOT_ANIM_RUN)
		theNode->Skeleton->AnimSpeed = MUTANTROBOT_RUN_SPEED * .003f;


				/* SEE IF DO ATTACK */

	SeeIfMutantRobotAttack(theNode, angle, dist);

				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateMutantRobot(theNode);
}


/********************** MOVE MUTANTROBOT: GOT HIT ******************************/

static void  MoveMutantRobot_GotHit(ObjNode *theNode)
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
		MorphToSkeletonAnim(theNode->Skeleton, MUTANTROBOT_ANIM_STAND, 3);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateMutantRobot(theNode);
}





/********************** MOVE MUTANTROBOT: DEATH ******************************/

static void  MoveMutantRobot_Death(ObjNode *theNode)
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

	UpdateMutantRobot(theNode);


}



//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME MUTANTROBOT ENEMY *************************/

Boolean PrimeEnemy_MutantRobot(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */

	newObj = MakeEnemySkeleton(SKELETON_TYPE_MUTANTROBOT,x,z, MUTANTROBOT_SCALE, 0, nil);


	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;

	SetSkeletonAnim(newObj->Skeleton, MUTANTROBOT_ANIM_WALK);
	newObj->Skeleton->AnimSpeed = 2.0f;

				/* SET BETTER INFO */

	newObj->InitCoord 		= newObj->Coord;							// remember where started
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		-= newObj->BottomOff;
	newObj->SplineMoveCall 	= MoveMutantRobotOnSpline;					// set move call
	newObj->Health 			= 1.0;
	newObj->Damage 			= 0;
	newObj->Kind 			= ENEMY_KIND_MUTANTROBOT;

				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,MUTANTROBOT_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= MutantRobotHitByStunPulse;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= MutantRobotHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] 		= MutantRobotGotPunched;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= MutantRobotHitByStunPulse;
	newObj->HitByJumpJetHandler 						= MutantRobotHitByJumpJet;

	newObj->HurtCallback = HurtMutantRobot;							// set hurt callback function


				/* MAKE SHADOW  */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 8, false);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE MUTANTROBOT ON SPLINE ***************************/

static void MoveMutantRobotOnSpline(ObjNode *theNode)
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

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < MUTANTROBOT_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveMutantRobot);

	}
}


/***************** UPDATE MUTANTROBOT ************************/

static void UpdateMutantRobot(ObjNode *theNode)
{
static OGLPoint3D	off = {18,5,0};
int		particleGroup,magicNum,i;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;
float				fps = gFramesPerSecondFrac;

	UpdateEnemy(theNode);


			/*************************/
			/* MAKE BROKEN ARM SPARK */
			/*************************/

	theNode->ParticleTimer -= fps;
	if (theNode->ParticleTimer <= 0.0f)										// see if time to make particles
	{
		theNode->ParticleTimer += .01f;

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= 400;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 30;
			groupDef.decayRate				=  0;
			groupDef.fadeRate				= 1.2;
			groupDef.particleTextureNum		= PARTICLE_SObjType_WhiteSpark3;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}
		if (particleGroup != -1)
		{
			FindCoordOnJoint(theNode, MUTANTROBOT_JOINT_RTARM, &off, &p);

			d.x = RandomFloat2() * 100.0f;
			d.y = RandomFloat2() * 100.0f;
			d.z = RandomFloat2() * 100.0f;

			newParticleDef.groupNum		= particleGroup;
			newParticleDef.where		= &p;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= 1.0;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= 1.0f;
			AddParticleToGroup(&newParticleDef);
		}
	}


			/*************************/
			/* UPDATE MUZZLE SPARKLE */
			/*************************/

	i = theNode->Sparkles[0];								// get sparkle index
	if (i != -1)
	{
		FindCoordOnJoint(theNode, MUTANTROBOT_JOINT_ANTENNA, &gMuzzleTipOff, &gSparkles[i].where);

		gSparkles[i].color.a -= fps * 4.0f;						// fade out
		if (gSparkles[i].color.a <= 0.0f)
		{
			DeleteSparkle(i);
			theNode->Sparkles[0] = -1;
		}
	}
}


#pragma mark -

/************ MUTANT ROBOT HIT BY STUN PULSE *****************/
//
// Called when weapon is moved and this handler is called after a collision.
//
// Also used for Flame weapon!
//

static Boolean MutantRobotHitByStunPulse(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord)


			/* HURT IT */

	HurtMutantRobot(enemy, .15);


			/* GIVE MOMENTUM */

	enemy->Delta.x += weaponDelta->x * .4f;
	enemy->Delta.y += weaponDelta->y * .4f  + 200.0f;
	enemy->Delta.z += weaponDelta->z * .4f;

	return(true);			// stop weapon
}





/************ MUTANT ROBOT GOT PUNCHED *****************/
//
// returns true if want to disable punch after this
//

static Boolean MutantRobotGotPunched(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta)
{
float	r;

#pragma unused (weapon, fistCoord, weaponDelta)


			/* HURT IT */

	HurtMutantRobot(enemy, .15);


			/* GIVE MOMENTUM */

	r = gPlayerInfo.objNode->Rot.y;

	enemy->Delta.x = -sin(r) * 1000.0f;
	enemy->Delta.z = -cos(r) * 1000.0f;
	enemy->Delta.y = 500.0f;

	return(true);
}


/************ MUTANT ROBOT GOT HIT BY JUMP JET *****************/

static void MutantRobotHitByJumpJet(ObjNode *enemy)
{
float	r;

		/* HURT IT */

	HurtMutantRobot(enemy, .5);


			/* GIVE MOMENTUM */

	r = gPlayerInfo.objNode->Rot.y;
	enemy->Delta.x = -sin(r) * 1000.0f;
	enemy->Delta.z = -cos(r) * 1000.0f;
	enemy->Delta.y = 500.0f;
}

/************** MUTANT ROBOT GOT HIT BY SUPERNOVA *****************/

static Boolean MutantRobotHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	KillMutantRobot(enemy);

	return(false);
}




/*********************** HURT MUTANT ROBOT ***************************/

static Boolean HurtMutantRobot(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveMutantRobot);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillMutantRobot(enemy);
		return(true);
	}
	else
	{
					/* GO INTO HIT ANIM */

		if (enemy->Skeleton->AnimNum != MUTANTROBOT_ANIM_GETHIT)
			MorphToSkeletonAnim(enemy->Skeleton, MUTANTROBOT_ANIM_GETHIT, 5);

		enemy->ButtTimer = 3.0f;
	}
	return(false);
}


/****************** KILL MUTANT ROBOT ***********************/

static void KillMutantRobot(ObjNode *enemy)
{
	SpewAtoms(&enemy->Coord, 1,1,1, false);
	ExplodeGeometry(enemy, 400, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .6);
	PlayEffect3D(EFFECT_SHATTER, &enemy->Coord);

	if (!enemy->EnemyRegenerate)
		enemy->TerrainItemPtr = nil;							// dont ever come back

	DeleteEnemy(enemy);
}


#pragma mark -

/************ SEE IF MUTANT ROBOT ATTACK *********************/
//
// returns true if is attacking
//

static Boolean SeeIfMutantRobotAttack(ObjNode *theNode, float angleToPlayer, float distToPlayer)
{
ObjNode	*newObj;
int				i;
OGLPoint3D	muzzleCoord;
OGLVector3D	muzzleVector;
OGLMatrix4x4	m;

	if (!gPlayerHasLanded)
		return(false);

	if ((distToPlayer > MUTANTROBOT_ATTACK_DIST) || (angleToPlayer > (PI/3.0f)))
		return(false);

			/* DON'T ATTACK THRU THINGS */

	if (SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE|CTYPE_BLOCKRAYS))
		return(false);



	theNode->DelayUntilAttack = .2f;

	if (theNode->Skeleton->AnimNum != MUTANTROBOT_ANIM_ATTACK)
	{
		MorphToSkeletonAnim(theNode->Skeleton, MUTANTROBOT_ANIM_ATTACK, 6);
		theNode->AttackDuration = 3.0;
	}

		/* CALC COORD & VECTOR OF MUZZLE */

	FindJointFullMatrix(theNode,MUTANTROBOT_JOINT_ANTENNA,&m);
	OGLPoint3D_Transform(&gMuzzleTipOff, &m, &muzzleCoord);
	OGLVector3D_Transform(&gMuzzleTipAim, &m, &muzzleVector);

	PlayEffect3D(EFFECT_MUTANTROBOTSHOOT, &muzzleCoord);


		/*********************/
		/* MAKE MUZZLE FLASH */
		/*********************/

	if (theNode->Sparkles[0] != -1)							// see if delete existing sparkle
	{
		DeleteSparkle(theNode->Sparkles[0]);
		theNode->Sparkles[0] = -1;
	}

	i = theNode->Sparkles[0] = GetFreeSparkle(theNode);		// make new sparkle
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
		gSparkles[i].where = muzzleCoord;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 50.0f;
		gSparkles[i].separation = 30.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_BlueSpark;
	}


			/************************/
			/* CREATE WEAPON OBJECT */
			/************************/

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_StunPulseBullet;
	gNewObjectDefinition.coord		= muzzleCoord;
	gNewObjectDefinition.flags 		= STATUS_BIT_USEALIGNMENTMATRIX|STATUS_BIT_GLOW|STATUS_BIT_NOZWRITES|
									STATUS_BIT_NOFOG|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= theNode->Slot-1;			// dont move until next frame
	gNewObjectDefinition.moveCall 	= MoveMutantRobotBullet;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .6;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


	newObj->Kind = WEAPON_TYPE_STUNPULSE;

	newObj->ColorFilter.a = .99;			// do this just to turn on transparency so it'll glow

	newObj->Delta.x = muzzleVector.x * 1500.0f;
	newObj->Delta.y = muzzleVector.y * 1500.0f;
	newObj->Delta.z = muzzleVector.z * 1500.0f;

	newObj->Health = 1.0f;

				/* COLLISION */

	newObj->CType = CTYPE_HURTME;
	newObj->CBits = CBITS_TOUCHABLE;
	SetObjectCollisionBounds(newObj, 50,-50,-50,50,50,-50);
	newObj->Damage = .1f;


			/* SET THE ALIGNMENT MATRIX */

	SetAlignmentMatrix(&newObj->AlignmentMatrix, &muzzleVector);

	return(true);
}


/******************* MOVE MUTANT ROBOT BULLET ***********************/

static void MoveMutantRobotBullet(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* SEE IF GONE */

	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}


	GetObjectInfo(theNode);

			/* MOVE IT */

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/***********************/
			/* SEE IF HIT ANYTHING */
			/***********************/

	if (gCoord.y <= GetTerrainY(gCoord.x, gCoord.z))					// see if hit ground
		goto explode_weapon;

	if (DoSimpleBoxCollision(gCoord.y + 40.0f, gCoord.y - 40.0f, gCoord.x - 40.0f, gCoord.x + 40.0f,
							gCoord.z + 40.0f, gCoord.z - 40.0f, CTYPE_MISC))

	{
explode_weapon:
		ExplodeStunPulse(theNode);
		return;
	}




	UpdateObject(theNode);
}

















