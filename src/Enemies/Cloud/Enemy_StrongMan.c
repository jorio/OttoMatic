/****************************/
/*   ENEMY: STRONGMAN.C			*/
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

static ObjNode *MakeEnemy_StrongMan(long x, long z);
static void MoveStrongMan(ObjNode *theNode);
static void MoveStrongMan_Standing(ObjNode *theNode);
static void  MoveStrongMan_Walk(ObjNode *theNode);
static void MoveStrongManOnSpline(ObjNode *theNode);
static void UpdateStrongMan(ObjNode *theNode);
static void  MoveStrongMan_GotHit(ObjNode *theNode);
static void  MoveStrongMan_GrabPlayer(ObjNode *theNode);

static Boolean StrongManHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean StrongManHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void KillStrongMan(ObjNode *enemy);

static void SeeIfStrongManAttack(ObjNode *theNode, float angleToPlayer, float distToPlayer);
static Boolean HurtStrongMan(ObjNode *enemy, float damage);
static void StrongManHitByJumpJet(ObjNode *enemy);
static void AlignPlayerInStrongManGrasp(ObjNode *onion);
static void StrongManReleasePlayer(ObjNode *theNode);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_STRONGMANS					6

#define	STRONGMAN_SCALE					2.0f

#define	STRONGMAN_DAMAGE				.33f

#define	STRONGMAN_CHASE_DIST_MAX		2500.0f
#define	STRONGMAN_CHASE_DIST_MIN		150.0f
#define	STRONGMAN_DETACH_DIST			(STRONGMAN_CHASE_DIST_MAX / 3.0f)

#define	STRONGMAN_GRAB_PLAYER_DIST		150.0f

#define	STRONGMAN_TARGET_OFFSET			60.0f

#define STRONGMAN_TURN_SPEED			3.0f
#define STRONGMAN_WALK_SPEED			280.0f



		/* ANIMS */


enum
{
	STRONGMAN_ANIM_STAND,
	STRONGMAN_ANIM_WALK,
	STRONGMAN_ANIM_GRABPLAYER,
	STRONGMAN_ANIM_GETHIT
};


		/* JOINTS */

enum
{
	STRONGMAN_JOINT_WAND	= 	16
};


#define	MAX_BUBBLE_SCALE	70.0f
#define	MAX_BUBBLE_SPEED	300.0f


/*********************/
/*    VARIABLES      */
/*********************/

#define	AttackDelay			SpecialF[0]
#define	StandDelay			SpecialF[1]
#define	ButtTimer			SpecialF[4]

#define	ReleasePlayer		Flag[0]

ObjNode	*gPlayerGrabbedByThisStrongMan = nil;

/************************ ADD STRONGMAN ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_StrongMan(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_STRONGMAN] >= MAX_STRONGMANS)
			return(false);
	}

	newObj = MakeEnemy_StrongMan(x,z);
	newObj->TerrainItemPtr = itemPtr;
	newObj->EnemyRegenerate = itemPtr->parm[3] & (1<<1);

	return(true);
}

/****************** MAKE STRONGMAN ENEMY *************************/

static ObjNode *MakeEnemy_StrongMan(long x, long z)
{
ObjNode	*newObj;

				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_STRONGMAN,x,z, STRONGMAN_SCALE, 0, MoveStrongMan);

	SetSkeletonAnim(newObj->Skeleton, STRONGMAN_ANIM_STAND);



				/*******************/
				/* SET BETTER INFO */
				/*******************/

	newObj->Health 		= 2.5;
	newObj->Damage 		= STRONGMAN_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_STRONGMAN;

	newObj->AttackDelay = RandomFloat() * 4.0f;


				/* SET COLLISION INFO */

	SetObjectCollisionBounds(newObj, 200, -200, -60, 60, 60,-60);
	CalcNewTargetOffsets(newObj,STRONGMAN_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= StrongManHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= StrongManHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= StrongManHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_DART] 		= StrongManHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLARE] 		= StrongManHitByWeapon;
	newObj->HitByJumpJetHandler 						= StrongManHitByJumpJet;

	newObj->HurtCallback = HurtStrongMan;							// set hurt callback function



				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 6,false);


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_STRONGMAN]++;

	return(newObj);
}



/********************* MOVE STRONGMAN **************************/

static void MoveStrongMan(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveStrongMan_Standing,
					MoveStrongMan_Walk,
					MoveStrongMan_GrabPlayer,
					MoveStrongMan_GotHit,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE STRONGMAN: STANDING ******************************/

static void  MoveStrongMan_Standing(ObjNode *theNode)
{
float	angleToTarget,dist;
float	fps = gFramesPerSecondFrac;


	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

				/* TURN TOWARDS ME */

	angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, STRONGMAN_TURN_SPEED, true);
	dist = OGLPoint3D_Distance(&gPlayerInfo.coord, &gCoord);


				/* SEE IF CHASE */


	theNode->StandDelay -= fps;								// see if in stand delay
	if (theNode->StandDelay <= 0.0f)
	{
		if (!IsBottomlessPitInFrontOfEnemy(theNode->Rot.y))
		{
			if ((dist < STRONGMAN_CHASE_DIST_MAX) && (dist > STRONGMAN_CHASE_DIST_MIN))
			{
				if (!SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE))		// dont chase thru walls
				{
					MorphToSkeletonAnim(theNode->Skeleton, STRONGMAN_ANIM_WALK, 4);
				}
			}
		}
	}


			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);

				/* SEE IF DO ATTACK */

	SeeIfStrongManAttack(theNode, angleToTarget, dist);



				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateStrongMan(theNode);
}




/********************** MOVE STRONGMAN: WALKING ******************************/

static void  MoveStrongMan_Walk(ObjNode *theNode)
{
float		r,fps,angle,dist;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, STRONGMAN_TURN_SPEED, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * STRONGMAN_WALK_SPEED;
	gDelta.z = -cos(r) * STRONGMAN_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


				/****************/
				/* SEE IF STAND */
				/****************/

	dist = OGLPoint3D_Distance(&gPlayerInfo.coord, &gCoord);


		/* STOP WALKING IF BOTTOMLESS PIT IN FRONT OF US */

	if (IsBottomlessPitInFrontOfEnemy(r))
	{
		MorphToSkeletonAnim(theNode->Skeleton, STRONGMAN_ANIM_STAND, 4);
	}
	else
	{
		if ((dist >= STRONGMAN_CHASE_DIST_MAX) || (dist <= STRONGMAN_CHASE_DIST_MIN))
			MorphToSkeletonAnim(theNode->Skeleton, STRONGMAN_ANIM_STAND, 4);
	}

				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == STRONGMAN_ANIM_WALK)
		theNode->Skeleton->AnimSpeed = STRONGMAN_WALK_SPEED * .01f;


				/* SEE IF DO ATTACK */

	SeeIfStrongManAttack(theNode, angle, dist);

				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;

	UpdateStrongMan(theNode);
}


/********************** MOVE STRONGMAN: GRAB PLAYER ******************************/

static void  MoveStrongMan_GrabPlayer(ObjNode *theNode)
{
			/* SEE IF RELEASE PLAYER */

	if (theNode->ReleasePlayer)
	{
		StrongManReleasePlayer(theNode);
	}

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)				// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);


			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);


			/* ALIGN PLAYER IN GRASP */

	AlignPlayerInStrongManGrasp(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


			/* SEE IF DONE */

	if (theNode->Skeleton->AnimHasStopped)
		MorphToSkeletonAnim(theNode->Skeleton, STRONGMAN_ANIM_STAND, 3);

	UpdateStrongMan(theNode);


}


/********************** MOVE STRONGMAN: GOT HIT ******************************/

static void  MoveStrongMan_GotHit(ObjNode *theNode)
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
		MorphToSkeletonAnim(theNode->Skeleton, STRONGMAN_ANIM_STAND, 3);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateStrongMan(theNode);
}



//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME STRONGMAN ENEMY *************************/

Boolean PrimeEnemy_StrongMan(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */

	newObj = MakeEnemySkeleton(SKELETON_TYPE_STRONGMAN,x,z, STRONGMAN_SCALE, 0, nil);


	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;

	SetSkeletonAnim(newObj->Skeleton, STRONGMAN_ANIM_WALK);
	newObj->Skeleton->AnimSpeed = 2.0f;

				/* SET BETTER INFO */

	newObj->InitCoord 		= newObj->Coord;							// remember where started
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		-= newObj->BottomOff;
	newObj->SplineMoveCall 	= MoveStrongManOnSpline;					// set move call
	newObj->Health 			= 1.0;
	newObj->Damage 			= STRONGMAN_DAMAGE;
	newObj->Kind 			= ENEMY_KIND_STRONGMAN;

				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,STRONGMAN_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= StrongManHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= StrongManHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= StrongManHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_DART] 		= StrongManHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLARE] 		= StrongManHitByWeapon;
	newObj->HitByJumpJetHandler 						= StrongManHitByJumpJet;

	newObj->HurtCallback = HurtStrongMan;							// set hurt callback function


				/* MAKE SHADOW & GLOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 6, false);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE STRONGMAN ON SPLINE ***************************/

static void MoveStrongManOnSpline(ObjNode *theNode)
{
Boolean isVisible;
float	dist;

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

				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))					// just do this to see if explosions hurt
			return;


					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		dist = OGLPoint3D_Distance(&gPlayerInfo.coord, &theNode->Coord);
		if (dist < STRONGMAN_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveStrongMan);

	}
}


/***************** UPDATE STRONGMAN ************************/

static void UpdateStrongMan(ObjNode *theNode)
{
	if (theNode->Skeleton->AnimNum != STRONGMAN_ANIM_GRABPLAYER)		// make sure isn't holding player if not in this anim
		StrongManReleasePlayer(theNode);

	UpdateEnemy(theNode);
}


#pragma mark -

/************ STRONGMAN HIT BY STUN PULSE *****************/
//
// Called when weapon is moved and this handler is called after a collision.
//
// Also used for Flame weapon!
//

static Boolean StrongManHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused(weaponCoord)


			/* HURT IT */

	HurtStrongMan(enemy, weapon->Damage);


			/* GIVE MOMENTUM */

	enemy->Delta.x += weaponDelta->x * .4f;
	enemy->Delta.y += weaponDelta->y * .4f  + 200.0f;
	enemy->Delta.z += weaponDelta->z * .4f;

	return(true);			// stop weapon
}


/************ STRONGMAN GOT HIT BY JUMP JET *****************/

static void StrongManHitByJumpJet(ObjNode *enemy)
{
float	r;

		/* HURT IT */

	HurtStrongMan(enemy, 1.0);


			/* GIVE MOMENTUM */

	r = gPlayerInfo.objNode->Rot.y;
	enemy->Delta.x = -sin(r) * 1000.0f;
	enemy->Delta.z = -cos(r) * 1000.0f;
	enemy->Delta.y = 500.0f;
}

/************** STRONGMAN GOT HIT BY SUPERNOVA *****************/

static Boolean StrongManHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	KillStrongMan(enemy);

	return(false);
}




/*********************** HURT STRONGMAN ***************************/

static Boolean HurtStrongMan(ObjNode *enemy, float damage)
{
	StrongManReleasePlayer(enemy);

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveStrongMan);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillStrongMan(enemy);
		return(true);
	}
	else
	{
					/* GO INTO HIT ANIM */

		if (enemy->Skeleton->AnimNum != STRONGMAN_ANIM_GETHIT)
			MorphToSkeletonAnim(enemy->Skeleton, STRONGMAN_ANIM_GETHIT, 8);

		enemy->ButtTimer = 1.0f;
	}
	return(false);
}


/****************** KILL STRONGMAN ***********************/

static void KillStrongMan(ObjNode *enemy)
{
float	x = enemy->Coord.x;
float	y = enemy->Coord.y;
float	z = enemy->Coord.z;

	StrongManReleasePlayer(enemy);


	SpewAtoms(&enemy->Coord, 0,1, 2, false);

	MakeSparkExplosion(x, y, z, 400, 1.0, PARTICLE_SObjType_GreenSpark,0);
	MakeSparkExplosion(x, y, z, 300, 1.0, PARTICLE_SObjType_BlueSpark,0);
	MakePuff(&enemy->Coord, 80, PARTICLE_SObjType_GreenFumes, GL_SRC_ALPHA, GL_ONE, .5);

	PlayEffect3D(EFFECT_CONFETTIBOOM, &enemy->Coord);

	if (!enemy->EnemyRegenerate)
		enemy->TerrainItemPtr = nil;							// dont ever come back

	DeleteEnemy(enemy);
}


#pragma mark -

/************ SEE IF STRONGMAN ATTACK *********************/

static void SeeIfStrongManAttack(ObjNode *theNode, float angleToPlayer, float distToPlayer)
{
	if (!gPlayerHasLanded)
		return;

	if (gPlayerIsDead)
		return;

	if (gPlayerInfo.invincibilityTimer > 0.0f)
		return;

	if (gPlayerInfo.objNode->Skeleton->AnimNum == PLAYER_ANIM_GRABBEDBYSTRONGMAN)	// see if already grabbed
		return;


	theNode->AttackDelay -= gFramesPerSecondFrac;
	if (theNode->AttackDelay > 0.0f)
		return;



			/* DON'T ATTACK THRU THINGS */

	if (SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE|CTYPE_BLOCKRAYS))
		return;


			/**********************/
			/* SEE IF GRAB PLAYER */
			/**********************/

	if ((distToPlayer < STRONGMAN_GRAB_PLAYER_DIST) && (angleToPlayer < (PI/3)))
	{
		MorphToSkeletonAnim(theNode->Skeleton, STRONGMAN_ANIM_GRABPLAYER, 9);
		theNode->ReleasePlayer = false;

		MorphToSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_GRABBEDBYSTRONGMAN, 8);

		gPlayerGrabbedByThisStrongMan = theNode;
	}


}


/************** ALIGN PLAYER IN STRONGMAN GRASP ********************/

static void AlignPlayerInStrongManGrasp(ObjNode *enemy)
{
OGLMatrix4x4	m,m2,m3;
float		scale;

	if (enemy->ReleasePlayer)						// dont align if not holding
		return;

			/* CALC ROT MATRIX */

	OGLMatrix4x4_SetRotate_Y(&m, PI);									// rotate 180


			/* CALC SCALE MATRIX */

	scale = PLAYER_DEFAULT_SCALE/STRONGMAN_SCALE;							// to adjust from onion's scale to player's
	OGLMatrix4x4_SetScale(&m2, scale, scale, scale);
	OGLMatrix4x4_Multiply(&m, &m2, &m2);								// apply rot


			/* CALC TRANSLATE MATRIX */

	OGLMatrix4x4_SetTranslate(&m3, 0 ,30, -35);
	OGLMatrix4x4_Multiply(&m2, &m3, &m);


			/* GET ALIGNMENT MATRIX */

	FindJointFullMatrix(enemy, 1, &m2);									// get joint's matrix

	OGLMatrix4x4_Multiply(&m, &m2, &gPlayerInfo.objNode->BaseTransformMatrix);
	SetObjectTransformMatrix(gPlayerInfo.objNode);


			/* FIND COORDS */

	FindCoordOfJoint(enemy, 1, &gPlayerInfo.objNode->Coord);
}


/********************* STRONGMAN RELEASE PLAYER ****************************/

static void StrongManReleasePlayer(ObjNode *theNode)
{
	if (gPlayerGrabbedByThisStrongMan != theNode)									// must be this guy
		return;

	if (gPlayerInfo.objNode->Skeleton->AnimNum == PLAYER_ANIM_GRABBEDBYSTRONGMAN)	// verify player is grabbed
	{
		PlayerGotHit(theNode, 0);
		MorphToSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_GOTHIT, 7);	// make sure in this position
		PlayEffect_Parms3D(EFFECT_METALLAND, &gCoord, NORMAL_CHANNEL_RATE, 1.6);

		gPlayerGrabbedByThisStrongMan = nil;
	}
}



