/****************************/
/*   ENEMY: MUTANT.C	*/
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

static ObjNode *MakeEnemy_Mutant(long x, long z);
static void CreateMutantGlow(ObjNode *alien);
static void UpdateMutantGlow(ObjNode *alien);
static void MoveMutant(ObjNode *theNode);
static void MoveMutant_Standing(ObjNode *theNode);
static void  MoveMutant_Walk(ObjNode *theNode);
static void MoveMutantOnSpline(ObjNode *theNode);
static void UpdateMutant(ObjNode *theNode);
static void  MoveMutant_Grow(ObjNode *theNode);
static void  MoveMutant_GotHit(ObjNode *theNode);
static void  MoveMutant_Attack(ObjNode *theNode);

static Boolean MutantHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean MutantHitByStunPulse(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean MutantGotPunched(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta);
static void KillMutant(ObjNode *enemy);

static void SeeIfMutantAttack(ObjNode *theNode, float angleToPlayer, float distToPlayer);
static Boolean HurtMutant(ObjNode *enemy, float damage);
static void MutantHitByJumpJet(ObjNode *enemy);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_MUTANTS					8

#define	MUTANT_SCALE				1.2f

#define	MUTANT_DAMAGE				.33f

#define	MUTANT_CHASE_DIST_MAX		1500.0f
#define	MUTANT_CHASE_DIST_MIN		300.0f
#define	MUTANT_DETACH_DIST			(MUTANT_CHASE_DIST_MAX / 2.0f)
#define	MUTANT_ATTACK_DIST			340.0f

#define	MUTANT_TARGET_OFFSET		50.0f

#define MUTANT_TURN_SPEED			3.0f
#define MUTANT_WALK_SPEED			280.0f



		/* ANIMS */


enum
{
	MUTANT_ANIM_STAND,
	MUTANT_ANIM_WALK,
	MUTANT_ANIM_ATTACK,
	MUTANT_ANIM_GETHIT,
	MUTANT_ANIM_GROW
};


		/* JOINTS */

enum
{
	MUTANT_JOINT_LEFTHAND	= 	10
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	ButtTimer			SpecialF[4]

#define	CheckSlap			Flag[0]


/************************ ADD MUTANT ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Mutant(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_MUTANT] >= MAX_MUTANTS)
			return(false);
	}

	newObj = MakeEnemy_Mutant(x,z);
	newObj->TerrainItemPtr = itemPtr;
	newObj->EnemyRegenerate = itemPtr->parm[3] & (1<<1);

	return(true);
}

/****************** MAKE MUTANT ENEMY *************************/

static ObjNode *MakeEnemy_Mutant(long x, long z)
{
ObjNode	*newObj;

				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_MUTANT,x,z, MUTANT_SCALE, 0, MoveMutant);

	SetSkeletonAnim(newObj->Skeleton, MUTANT_ANIM_STAND);



				/*******************/
				/* SET BETTER INFO */
				/*******************/

	newObj->Health 		= 1.0;
	newObj->Damage 		= MUTANT_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_MUTANT;

				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,MUTANT_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= MutantHitByStunPulse;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= MutantHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] 		= MutantGotPunched;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= MutantHitByStunPulse;
	newObj->HitByJumpJetHandler 						= MutantHitByJumpJet;

	newObj->HurtCallback = HurtMutant;							// set hurt callback function



				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 8,false);

	CreateMutantGlow(newObj);

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_MUTANT]++;

	return(newObj);
}


/******************* GROW MUTANT ***********************/
//
// Called from RadiateMutant() when saucer makes this tomato grow
//

void GrowMutant(float x, float z)
{
ObjNode	*newObj;

	newObj = MakeEnemy_Mutant(x, z);
	if (newObj)
	{
		newObj->Scale.x = newObj->Scale.y = newObj->Scale.z = .08f;											// start shrunken
		newObj->Rot.y = CalcYAngleFromPointToPoint(0, x, z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);		// aim at player

		SetSkeletonAnim(newObj->Skeleton, MUTANT_ANIM_GROW);
		UpdateObjectTransforms(newObj);
	}

}


/********************* MOVE MUTANT **************************/

static void MoveMutant(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveMutant_Standing,
					MoveMutant_Walk,
					MoveMutant_Attack,
					MoveMutant_GotHit,
					MoveMutant_Grow,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE MUTANT: STANDING ******************************/

static void  MoveMutant_Standing(ObjNode *theNode)
{
float	angleToTarget,dist;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

				/* TURN TOWARDS ME */

	angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, MUTANT_TURN_SPEED, true);



				/* SEE IF CHASE */

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
	if ((dist < MUTANT_CHASE_DIST_MAX) && (dist > MUTANT_CHASE_DIST_MIN))
	{
		MorphToSkeletonAnim(theNode->Skeleton, MUTANT_ANIM_WALK, 4);
	}

			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


				/* SEE IF DO ATTACK */

	SeeIfMutantAttack(theNode, angleToTarget, dist);


	UpdateMutant(theNode);
}




/********************** MOVE MUTANT: WALKING ******************************/

static void  MoveMutant_Walk(ObjNode *theNode)
{
float		r,fps,angle,dist;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, MUTANT_TURN_SPEED, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * MUTANT_WALK_SPEED;
	gDelta.z = -cos(r) * MUTANT_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


				/* SEE IF STAND */

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
	if ((dist >= MUTANT_CHASE_DIST_MAX) || (dist <= MUTANT_CHASE_DIST_MIN))
		MorphToSkeletonAnim(theNode->Skeleton, MUTANT_ANIM_STAND, 4);


				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == MUTANT_ANIM_WALK)
		theNode->Skeleton->AnimSpeed = MUTANT_WALK_SPEED * .01f;


				/* SEE IF DO ATTACK */

	SeeIfMutantAttack(theNode, angle, dist);

				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;

	UpdateMutant(theNode);
}


/********************** MOVE MUTANT: ATTACK ******************************/

static void  MoveMutant_Attack(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

				/* TURN TOWARDS ME */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, MUTANT_TURN_SPEED, true);


			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

			/* SEE IF HIT PLAYER */

	if (theNode->CheckSlap)
	{
		OGLPoint3D	hitPt;
		long					pg,i;
		OGLVector3D				delta;
		OGLPoint3D				pt;
		NewParticleDefType		newParticleDef;
		float					x,y,z;

		theNode->CheckSlap = false;

		FindCoordOfJoint(theNode, MUTANT_JOINT_LEFTHAND, &hitPt);

		if (DoSimpleBoxCollisionAgainstPlayer(hitPt.y + 50.0f, hitPt.y - 50.0f, hitPt.x-50.0f, hitPt.x+50.0f,
										hitPt.z + 50.0f, hitPt.z - 50.0f))
		{
			PlayEffect3D(EFFECT_PLAYERCRUSH, &hitPt);

			PlayerGotHit(theNode, 0);				// hurt the player
		}


				/*************************/
				/* MAKE RADIOACTIVE POOF */
				/*************************/

		gNewParticleGroupDef.magicNum				= 0;
		gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
		gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
		gNewParticleGroupDef.gravity				= 40;
		gNewParticleGroupDef.magnetism				= 0;
		gNewParticleGroupDef.baseScale				= 40.0;
		gNewParticleGroupDef.decayRate				=  -1.5;
		gNewParticleGroupDef.fadeRate				= .7;
		gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_GreenFumes;
		gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
		gNewParticleGroupDef.dstBlend				= GL_ONE;

		pg = NewParticleGroup(&gNewParticleGroupDef);
		if (pg != -1)
		{
			x = hitPt.x;
			y = hitPt.y;
			z =	hitPt.z;

			for (i = 0; i < 15; i++)
			{
				pt.x = x + RandomFloat2() * 20.0f;
				pt.y = y + RandomFloat2() * 20.0f;
				pt.z = z + RandomFloat2() * 20.0f;

				delta.x = RandomFloat2() * 150.0f;
				delta.y = RandomFloat2() * 100.0f;
				delta.z = RandomFloat2() * 150.0f;


				newParticleDef.groupNum		= pg;
				newParticleDef.where		= &pt;
				newParticleDef.delta		= &delta;
				newParticleDef.scale		= 1.0f + RandomFloat2() * .2f;
				newParticleDef.rotZ			= RandomFloat() * PI2;
				newParticleDef.rotDZ		= RandomFloat2() * 4.0f;
				newParticleDef.alpha		= FULL_ALPHA;
				AddParticleToGroup(&newParticleDef);
			}
		}


	}

			/* SEE IF DONE */

	if (theNode->Skeleton->AnimHasStopped)
		MorphToSkeletonAnim(theNode->Skeleton, MUTANT_ANIM_STAND, 7);

	UpdateMutant(theNode);
}





/********************** MOVE MUTANT: GOT HIT ******************************/

static void  MoveMutant_GotHit(ObjNode *theNode)
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
		MorphToSkeletonAnim(theNode->Skeleton, MUTANT_ANIM_STAND, 3);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateMutant(theNode);
}



/********************** MOVE MUTANT: GROW ******************************/

static void  MoveMutant_Grow(ObjNode *theNode)
{
float	scale;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

		/* MAKE GROW */

	scale = theNode->Scale.x += gFramesPerSecondFrac * .4f;

			/* SEE IF DONE GROWING */

	if (scale >= MUTANT_SCALE)
	{
		scale = MUTANT_SCALE;
		MorphToSkeletonAnim(theNode->Skeleton, MUTANT_ANIM_STAND, 2);
		gDelta.y = 700.0f;
	}

	theNode->Scale.x =
	theNode->Scale.y =
	theNode->Scale.z = scale;

	if (DoEnemyCollisionDetect(theNode,0, true))			// just do ground
		return;

	UpdateMutant(theNode);
}


//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME MUTANT ENEMY *************************/

Boolean PrimeEnemy_Mutant(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */

	newObj = MakeEnemySkeleton(SKELETON_TYPE_MUTANT,x,z, MUTANT_SCALE, 0, nil);


	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;

	SetSkeletonAnim(newObj->Skeleton, MUTANT_ANIM_WALK);
	newObj->Skeleton->AnimSpeed = 2.0f;

				/* SET BETTER INFO */

	newObj->InitCoord 		= newObj->Coord;							// remember where started
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		-= newObj->BottomOff;
	newObj->SplineMoveCall 	= MoveMutantOnSpline;					// set move call
	newObj->Health 			= 1.0;
	newObj->Damage 			= MUTANT_DAMAGE;
	newObj->Kind 			= ENEMY_KIND_MUTANT;

				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,MUTANT_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= MutantHitByStunPulse;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= MutantHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] 		= MutantGotPunched;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= MutantHitByStunPulse;
	newObj->HitByJumpJetHandler 						= MutantHitByJumpJet;

	newObj->HurtCallback = HurtMutant;							// set hurt callback function


				/* MAKE SHADOW & GLOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 8, false);

	CreateMutantGlow(newObj);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);											// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE MUTANT ON SPLINE ***************************/

static void MoveMutantOnSpline(ObjNode *theNode)
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

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BBox.min.y;	// calc y coord
		UpdateObjectTransforms(theNode);											// update transforms
		UpdateShadow(theNode);
		UpdateMutantGlow(theNode);													// update glow

				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))					// just do this to see if explosions hurt
			return;


					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < MUTANT_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveMutant);

	}
}


/***************** UPDATE MUTANT ************************/

static void UpdateMutant(ObjNode *theNode)
{
	UpdateEnemy(theNode);
	UpdateMutantGlow(theNode);
}


#pragma mark -

/************ MUTANT HIT BY STUN PULSE *****************/
//
// Called when weapon is moved and this handler is called after a collision.
//
// Also used for Flame weapon!
//

static Boolean MutantHitByStunPulse(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon,	weaponCoord)


			/* HURT IT */

	HurtMutant(enemy, .25);


			/* GIVE MOMENTUM */

	enemy->Delta.x += weaponDelta->x * .4f;
	enemy->Delta.y += weaponDelta->y * .4f  + 200.0f;
	enemy->Delta.z += weaponDelta->z * .4f;

	return(true);			// stop weapon
}





/************ MUTANT GOT PUNCHED *****************/
//
// returns true if want to disable punch after this
//

static Boolean MutantGotPunched(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta)
{
float	r;

#pragma unused (weapon,	fistCoord, weaponDelta)


			/* HURT IT */

	HurtMutant(enemy, .25);


			/* GIVE MOMENTUM */

	r = gPlayerInfo.objNode->Rot.y;

	enemy->Delta.x = -sin(r) * 1000.0f;
	enemy->Delta.z = -cos(r) * 1000.0f;
	enemy->Delta.y = 500.0f;


	return(true);
}


/************ MUTANT GOT HIT BY JUMP JET *****************/

static void MutantHitByJumpJet(ObjNode *enemy)
{
float	r;

		/* HURT IT */

	HurtMutant(enemy, 1.0);


			/* GIVE MOMENTUM */

	r = gPlayerInfo.objNode->Rot.y;
	enemy->Delta.x = -sin(r) * 1000.0f;
	enemy->Delta.z = -cos(r) * 1000.0f;
	enemy->Delta.y = 500.0f;
}

/************** MUTANT GOT HIT BY SUPERNOVA *****************/

static Boolean MutantHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	KillMutant(enemy);

	return(false);
}




/*********************** HURT MUTANT ***************************/

static Boolean HurtMutant(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveMutant);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillMutant(enemy);
		return(true);
	}
	else
	{
					/* GO INTO HIT ANIM */

		if (enemy->Skeleton->AnimNum != MUTANT_ANIM_GETHIT)
			MorphToSkeletonAnim(enemy->Skeleton, MUTANT_ANIM_GETHIT, 8);

		enemy->ButtTimer = 1.0f;
	}
	return(false);
}


/****************** KILL MUTANT ***********************/

static void KillMutant(ObjNode *enemy)
{
	float	x = enemy->Coord.x;
	float	y = enemy->Coord.y;
	float	z = enemy->Coord.z;

	SpewAtoms(&enemy->Coord, 0,2, 1, false);

	MakeSparkExplosion(x, y, z, 400, 1.0, PARTICLE_SObjType_GreenSpark,0);
	MakeSparkExplosion(x, y, z, 300, 1.0, PARTICLE_SObjType_BlueSpark,0);
	MakePuff(&enemy->Coord, 80, PARTICLE_SObjType_GreenFumes, GL_SRC_ALPHA, GL_ONE, .5);

	if (!enemy->EnemyRegenerate)
		enemy->TerrainItemPtr = nil;							// dont ever come back

	DeleteEnemy(enemy);
}


#pragma mark -

/************ SEE IF MUTANT ATTACK *********************/

static void SeeIfMutantAttack(ObjNode *theNode, float angleToPlayer, float distToPlayer)
{
	if (!gPlayerHasLanded)
		return;

	if (gPlayerInfo.invincibilityTimer > 0.0f)
		return;

	if (distToPlayer > MUTANT_ATTACK_DIST)
		return;

	if (angleToPlayer > (PI/2))
		return;

	if (gPlayerInfo.objNode->Skeleton->AnimNum == PLAYER_ANIM_RIDEZIP)		// don't attack if on zipline (avoid a weird death bug)
		return;


			/* DON'T ATTACK THRU THINGS */

	if (SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE|CTYPE_BLOCKRAYS))
		return;


	MorphToSkeletonAnim(theNode->Skeleton, MUTANT_ANIM_ATTACK, 5.0);

	theNode->CheckSlap = false;

	PlayEffect3D(EFFECT_MUTANTGROWL, &gCoord);
}



#pragma mark -


/*************** CREATE MUTANT GLOW ****************/

static void CreateMutantGlow(ObjNode *alien)
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
			gSparkles[s].color.r = .7;
			gSparkles[s].color.g = 1;
			gSparkles[s].color.b = .5;
			gSparkles[s].color.a = .6;

			gSparkles[s].scale = 150.0f;
			gSparkles[s].separation = -100.0f;

			gSparkles[s].textureNum = PARTICLE_SObjType_WhiteGlow;
		}
	}
}


/**************** UPDATE MUTANT GLOW *******************/

static void UpdateMutantGlow(ObjNode *alien)
{
int	numJoints,i,s;



	numJoints = alien->Skeleton->skeletonDefinition->NumBones;
	if (numJoints > MAX_NODE_SPARKLES)				// make sure we don't overflow
		numJoints = MAX_NODE_SPARKLES;

	for (i = 0; i < numJoints; i++)
	{
		s = alien->Sparkles[i];
		if (s != -1)
		{
			FindCoordOfJoint(alien, i, &gSparkles[s].where);
			gSparkles[s].scale = (50.0f + RandomFloat()*10.0f) * 2.0f; //scale[i];
		}
	}
}
















