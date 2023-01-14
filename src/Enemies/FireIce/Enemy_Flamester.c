/****************************/
/*   ENEMY: FLAMESTER.C	*/
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

static ObjNode *MakeEnemy_Flamester(float x, float z);
static void CreateFlamesterGlow(ObjNode *alien);
static void UpdateFlamesterGlow(ObjNode *alien);
static void MoveFlamester(ObjNode *theNode);
static void MoveFlamester_Standing(ObjNode *theNode);
static void  MoveFlamester_Walk(ObjNode *theNode);
static void MoveFlamesterOnSpline(ObjNode *theNode);
static void UpdateFlamester(ObjNode *theNode);
static void SetFlamesterCollisionBox(ObjNode *theNode, Boolean atInit);
static Boolean FlamesterHitByFlame(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean FlamesterHitByFreeze(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void KillFlamester(ObjNode *enemy);

static Boolean HurtFlamester(ObjNode *enemy, float damage);

static void SeeIfFlamesterAttack(ObjNode *theNode, float distToPlayer);
static void  MoveFlamester_Attack(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_FLAMESTERS					8

#define	FLAMESTER_SCALE_NORMAL			2.1f
#define	FLAMESTER_SCALE_MIN				(FLAMESTER_SCALE_NORMAL * .3f)
#define	FLAMESTER_SCALE_MAX				(FLAMESTER_SCALE_NORMAL * 1.3f)

#define	FLAMESTER_CHASE_DIST_MAX		3000.0f
#define	FLAMESTER_DETACH_DIST			900.0f
#define	FLAMESTER_ATTACK_DIST			600.0f
#define	ATTACK_DURATION					7.0f

#define	FLAMESTER_TARGET_OFFSET			150.0f

#define FLAMESTER_TURN_SPEED			3.0f
#define FLAMESTER_WALK_SPEED			280.0f

#define	FLAMESTER_HEALTH				1.0f
#define	FLAMESTER_DAMAGE				.2f


		/* ANIMS */


enum
{
	FLAMESTER_ANIM_STAND,
	FLAMESTER_ANIM_WALK,
	FLAMESTER_ANIM_ATTACK
};



/*********************/
/*    VARIABLES      */
/*********************/

#define	AttackTimer		SpecialF[0]
#define	SpewLavaTimer	SpecialF[1]


/************************ ADD FLAMESTER ENEMY *************************/

Boolean AddEnemy_Flamester(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_FLAMESTER] >= MAX_FLAMESTERS)
			return(false);
	}


	newObj = MakeEnemy_Flamester(x,z);
	newObj->TerrainItemPtr = itemPtr;
	newObj->EnemyRegenerate = itemPtr->parm[3] & (1<<1);

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_FLAMESTER]++;

	return(true);
}

/********************** MAKE FLAMESTER ************************/

static ObjNode *MakeEnemy_Flamester(float x, float z)
{
ObjNode	*newObj;
				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_FLAMESTER,x,z, FLAMESTER_SCALE_NORMAL, 0, MoveFlamester);

	SetSkeletonAnim(newObj->Skeleton, FLAMESTER_ANIM_STAND);


				/*******************/
				/* SET BETTER INFO */
				/*******************/

	newObj->Health 		= FLAMESTER_HEALTH;
	newObj->Damage 		= FLAMESTER_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_FLAMESTER;


				/* SET COLLISION INFO */

	SetFlamesterCollisionBox(newObj, true);
	CalcNewTargetOffsets(newObj,FLAMESTER_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= FlamesterHitByFlame;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLARE] 		= FlamesterHitByFlame;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FREEZE] 		= FlamesterHitByFreeze;

	newObj->HurtCallback = HurtFlamester;							// set hurt callback function




				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 8,false);

	CreateFlamesterGlow(newObj);

	return(newObj);
}


/******************** SET FLAMESTER COLLISION BOX ************************/
//
// This gets called @ init and whenever the ice cube guy changes scale
//

static void SetFlamesterCollisionBox(ObjNode *theNode, Boolean atInit)
{

	CreateCollisionBoxFromBoundingBox_Update(theNode, 1,1);


	if (atInit)								// only set old info if initing, otherwise, keep old since dont want to hoze collision on next frame
		KeepOldCollisionBoxes(theNode);

}


/********************* MOVE FLAMESTER **************************/

static void MoveFlamester(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveFlamester_Standing,
					MoveFlamester_Walk,
					MoveFlamester_Attack
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE FLAMESTER: STANDING ******************************/

static void  MoveFlamester_Standing(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

				/* TURN TOWARDS ME */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, FLAMESTER_TURN_SPEED, true);



				/* SEE IF CHASE */

	float dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
	if (dist < FLAMESTER_CHASE_DIST_MAX)
	{
		MorphToSkeletonAnim(theNode->Skeleton, FLAMESTER_ANIM_WALK, 4);
	}

			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);

				/* SEE IF DO ATTACK */

	SeeIfFlamesterAttack(theNode, dist);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


	UpdateFlamester(theNode);
}




/********************** MOVE FLAMESTER: WALKING ******************************/

static void  MoveFlamester_Walk(ObjNode *theNode)
{
float		r,fps,dist;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, FLAMESTER_TURN_SPEED, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * FLAMESTER_WALK_SPEED;
	gDelta.z = -cos(r) * FLAMESTER_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


				/* SEE IF STAND */

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
	if (dist >= FLAMESTER_CHASE_DIST_MAX)
		MorphToSkeletonAnim(theNode->Skeleton, FLAMESTER_ANIM_STAND, 4);


				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == FLAMESTER_ANIM_WALK)
		theNode->Skeleton->AnimSpeed = FLAMESTER_WALK_SPEED * .005f;

				/* SEE IF DO ATTACK */

	SeeIfFlamesterAttack(theNode, dist);

				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateFlamester(theNode);
}

/********************** MOVE FLAMESTER: ATTACK ******************************/

static void  MoveFlamester_Attack(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;


	gDelta.x = gDelta.z = 0;

			/* SPIN */

	if (theNode->AttackTimer < (ATTACK_DURATION/2))				// see if slow or speed
	{
		theNode->DeltaRot.y -= fps * 4.0f;
		if (theNode->DeltaRot.y < 0.0f)
			theNode->DeltaRot.y = 0.0f;
	}
	else
	{
		theNode->DeltaRot.y += fps * 4.0f;
		if (theNode->DeltaRot.y > 50.0f)
			theNode->DeltaRot.y = 50.0f;
	}
	theNode->Rot.y += theNode->DeltaRot.y * fps;


			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

			/* SEE IF SPEW LAVA BOULDER */

	theNode->SpewLavaTimer -= fps;
	if (theNode->SpewLavaTimer <= 0.0f)
	{
		theNode->SpewLavaTimer = .3f + RandomFloat() * .5f;
		SpewALavaBoulder(&gCoord, true);
	}


			/* SEE IF DONE */

	theNode->AttackTimer -= fps;
	if (theNode->AttackTimer <= 0.0f)
	{
		MorphToSkeletonAnim(theNode->Skeleton, FLAMESTER_ANIM_STAND, 2);
	}

	UpdateFlamester(theNode);
}



//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME FLAMESTER ENEMY *************************/

Boolean PrimeEnemy_Flamester(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE OBJECT */

	newObj = MakeEnemy_Flamester(x,z);

	SetSkeletonAnim(newObj->Skeleton, FLAMESTER_ANIM_WALK);


				/* SET SPLINE INFO */


	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;

	newObj->InitCoord 		= newObj->Coord;							// remember where started
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveFlamesterOnSpline;					// set move call
	newObj->MoveCall		= nil;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE FLAMESTER ON SPLINE ***************************/

static void MoveFlamesterOnSpline(ObjNode *theNode)
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
		UpdateFlamesterGlow(theNode);												// update glow

				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))					// just do this to see if explosions hurt
			return;


					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < FLAMESTER_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveFlamester);

		BurnSkeleton(theNode, 60);
	}
}


/***************** UPDATE FLAMESTER ************************/

static void UpdateFlamester(ObjNode *theNode)
{
	UpdateEnemy(theNode);
	UpdateFlamesterGlow(theNode);
	BurnSkeleton(theNode, 30.0f * theNode->Scale.x);
}


#pragma mark -



/************ FLAMESTER HIT BY FLAME *****************/
//
// Flame gun actually makes him stronger
//

static Boolean FlamesterHitByFlame(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)
float	s;

		/* MAKE SCALE GROW */

	s = enemy->Scale.x;
	if (s < FLAMESTER_SCALE_MAX)
	{
		s += .15f;										// grow by some amount
		if (s > FLAMESTER_SCALE_MAX)						// check if @ max
			s = FLAMESTER_SCALE_MAX;

		enemy->Scale.x =
		enemy->Scale.y =
		enemy->Scale.z = s;

		UpdateObjectTransforms(enemy);
		SetFlamesterCollisionBox(enemy, false);			// update collision box based on new scale

		enemy->Health = FLAMESTER_HEALTH;					// reset health
	}


	return(true);			// stop weapon
}


/************ FLAMESTER HIT BY FREEZE *****************/

static Boolean FlamesterHitByFreeze(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)
float	s;

		/* MAKE SCALE SHRINK */

	s = enemy->Scale.x;
	if (s > FLAMESTER_SCALE_MIN)
	{
		s -= .25f;										// shrink by some amount
		if (s < FLAMESTER_SCALE_MIN)						// check if @ min
		{
			KillFlamester(enemy);
			return(true);
		}

		enemy->Scale.x =
		enemy->Scale.y =
		enemy->Scale.z = s;

		UpdateObjectTransforms(enemy);
		SetFlamesterCollisionBox(enemy, false);			// update collision box based on new scale

	}

	return(true);			// stop weapon
}




/*********************** HURT FLAMESTER ***************************/

static Boolean HurtFlamester(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveFlamester);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillFlamester(enemy);
		return(true);
	}
	else
	{
					/* GO INTO HIT ANIM */

//		if (enemy->Skeleton->AnimNum != FLAMESTER_ANIM_GETHIT)
//			MorphToSkeletonAnim(enemy->Skeleton, FLAMESTER_ANIM_GETHIT, 5);

//		enemy->ButtTimer = 3.0f;
	}
	return(false);
}


/****************** KILL FLAMESTER ***********************/

static void KillFlamester(ObjNode *enemy)
{
	SpewAtoms(&enemy->Coord, 2,1, 0, false);

	if (!enemy->EnemyRegenerate)
		enemy->TerrainItemPtr = nil;							// dont ever come back

	DeleteEnemy(enemy);
}




#pragma mark -


/*************** CREATE FLAMESTER GLOW ****************/

static void CreateFlamesterGlow(ObjNode *alien)
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
			gSparkles[s].color.r = 1;
			gSparkles[s].color.g = .5;
			gSparkles[s].color.b = .2;
			gSparkles[s].color.a = .8;

			gSparkles[s].scale = 150.0f;
			gSparkles[s].separation = -50.0f;

			gSparkles[s].textureNum = PARTICLE_SObjType_WhiteGlow;
		}
	}
}


/**************** UPDATE FLAMESTER GLOW *******************/

static void UpdateFlamesterGlow(ObjNode *alien)
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
			gSparkles[s].scale = (100.0f + RandomFloat()*20.0f);
		}
	}
}



#pragma mark -

/************ SEE IF FLAMESTER ATTACK *********************/

static void SeeIfFlamesterAttack(ObjNode *theNode, float distToPlayer)
{
	if (!gPlayerHasLanded)
		return;

	if (gPlayerIsDead)
		return;

	if (gPlayerInfo.invincibilityTimer > 0.0f)
		return;

	if (distToPlayer < FLAMESTER_ATTACK_DIST)
	{
		MorphToSkeletonAnim(theNode->Skeleton, FLAMESTER_ANIM_ATTACK, 4);
		theNode->AttackTimer = ATTACK_DURATION;
		theNode->SpewLavaTimer = 1.0f;
		theNode->DeltaRot.y = 0;
	}
}














