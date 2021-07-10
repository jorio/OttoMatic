/****************************/
/*   ENEMY: ICECUBE.C		*/
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	SplineDefType			**gSplineList;
extern	OGLPoint3D				gCoord;
extern	short					gNumEnemies,gNumCollisions;
extern	float					gFramesPerSecondFrac;
extern	OGLVector3D			gDelta;
extern	signed char			gNumEnemyOfKind[];
extern	u_long		gAutoFadeStatusBits;
extern	PlayerInfoType	gPlayerInfo;
extern	SparkleType	gSparkles[];
extern	Boolean				gPlayerHasLanded,gPlayerIsDead;
extern	SpriteType	*gSpriteGroupList[MAX_SPRITE_GROUPS];
extern	NewParticleGroupDefType	gNewParticleGroupDef;
extern	CollisionRec	gCollisionList[];

/****************************/
/*    PROTOTYPES            */
/****************************/

static ObjNode *MakeEnemy_IceCube(float x, float z);
static void MoveIceCube(ObjNode *theNode);
static void MoveIceCube_Standing(ObjNode *theNode);
static void  MoveIceCube_Walk(ObjNode *theNode);
static void MoveIceCubeOnSpline(ObjNode *theNode);
static void UpdateIceCube(ObjNode *theNode);
static void  MoveIceCube_GotHit(ObjNode *theNode);
static void  MoveIceCube_ThrowIce(ObjNode *theNode);

static Boolean IceCubeHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean IceCubeHitByStunPulse(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean IceCubeHitByFreeze(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean IceCubeHitByFlame(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void KillIceCube(ObjNode *enemy);
static Boolean IceCubeGotPunched(ObjNode *fist, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta);

static void SeeIfIceCubeAttack(ObjNode *theNode, float angleToPlayer, float distToPlayer);
static Boolean HurtIceCube(ObjNode *enemy, float damage);
static void IceCubeHitByJumpJet(ObjNode *enemy);

static void CreateIceCubeSparkles(ObjNode *theNode);
static void UpdateIceCubeSparkles(ObjNode *theNode);
static void SetIceCubeCollisionBox(ObjNode *theNode, Boolean atInit);

static void IceCubeThrowIce(ObjNode *enemy);
static void ReleaseIcicle(ObjNode *enemy, ObjNode *icicle);
static void MoveIcicle(ObjNode *theNode);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_ICECUBES					8

#define	ICECUBE_SCALE_NORMAL			2.5f
#define	ICECUBE_SCALE_MIN				(ICECUBE_SCALE_NORMAL * .3f)
#define	ICECUBE_SCALE_MAX				(ICECUBE_SCALE_NORMAL * 1.3f)

#define	ICECUBE_DAMAGE				.33f
#define	ICECUBE_HEALTH				1.2f

#define	ICECUBE_CHASE_DIST_MAX		2500.0f
#define	ICECUBE_CHASE_DIST_MIN		150.0f
#define	ICECUBE_DETACH_DIST			1000.0f

#define	ICECUBE_THROWICE_DIST_MAX	2000.0f
#define	ICECUBE_THROWICE_DIST_MIN	700.0f

#define	ICECUBE_TARGET_OFFSET			60.0f

#define ICECUBE_TURN_SPEED			3.0f
#define ICECUBE_WALK_SPEED			280.0f



		/* ANIMS */


enum
{
	ICECUBE_ANIM_STAND,
	ICECUBE_ANIM_WALK,
	ICECUBE_ANIM_THROWICE,
	ICECUBE_ANIM_GETHIT
};


		/* JOINTS */

enum
{
	ICECUBE_JOINT_HEAD		= 3,
	ICECUBE_JOINT_RIGHTHAND	= 	15
};


#define	MAX_BUBBLE_SCALE	70.0f
#define	MAX_BUBBLE_SPEED	300.0f


/*********************/
/*    VARIABLES      */
/*********************/

#define	ThrowNow			Flag[0]

#define	AttackDelay			SpecialF[0]
#define	StandDelay			SpecialF[1]
#define	ButtTimer			SpecialF[4]

ObjNode	*gPlayerGrabbedByThisIceCube = nil;

/************************ ADD ICECUBE ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_IceCube(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_ICECUBE] >= MAX_ICECUBES)
			return(false);
	}

	newObj = MakeEnemy_IceCube(x,z);
	newObj->TerrainItemPtr = itemPtr;
	newObj->EnemyRegenerate = itemPtr->parm[3] & (1<<1);

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_ICECUBE]++;

	return(true);
}

/****************** MAKE ICECUBE ENEMY *************************/

static ObjNode *MakeEnemy_IceCube(float x, float z)
{
ObjNode	*newObj;

				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_ICECUBE,x,z, ICECUBE_SCALE_NORMAL, 0, MoveIceCube);

	SetSkeletonAnim(newObj->Skeleton, ICECUBE_ANIM_STAND);

	newObj->StatusBits |= STATUS_BIT_NOLIGHTING;

	newObj->ColorFilter.a = .99;

				/*******************/
				/* SET BETTER INFO */
				/*******************/

	newObj->Health 		= ICECUBE_HEALTH;
	newObj->Damage 		= ICECUBE_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_ICECUBE;

	newObj->AttackDelay = RandomFloat() * 4.0f;


				/* SET COLLISION INFO */

	SetIceCubeCollisionBox(newObj, true);
	CalcNewTargetOffsets(newObj,ICECUBE_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] 		= IceCubeGotPunched;
	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= IceCubeHitByStunPulse;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FREEZE] 		= IceCubeHitByFreeze;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= IceCubeHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= IceCubeHitByFlame;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLARE] 		= IceCubeHitByFlame;
	newObj->HitByJumpJetHandler 						= IceCubeHitByJumpJet;

	newObj->HurtCallback = HurtIceCube;							// set hurt callback function



				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 6,false);

	CreateIceCubeSparkles(newObj);


	return(newObj);
}


/******************** SET ICECUBE COLLISION BOX ************************/
//
// This gets called @ init and whenever the ice cube guy changes scale
//

static void SetIceCubeCollisionBox(ObjNode *theNode, Boolean atInit)
{
float	s = theNode->Scale.x;
float	s2 = s * 35.0f;

			/* WE DO THIS MANUALLY SINCE WE DON'T WANT TO SET "OLD" INFO ALL THE TIME */

	theNode->NumCollisionBoxes = 1;					// 1 collision box
	theNode->TopOff 	= 100.0f * s;
	theNode->BottomOff 	= -100.0f * s;
	theNode->LeftOff 	= -s2;
	theNode->RightOff 	= s2;
	theNode->FrontOff 	= s2;
	theNode->BackOff 	= -s2;

	CalcObjectBoxFromNode(theNode);

	if (atInit)								// only set old info if initing, otherwise, keep old since dont want to hoze collision on next frame
		KeepOldCollisionBoxes(theNode);

}



/********************* MOVE ICECUBE **************************/

static void MoveIceCube(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveIceCube_Standing,
					MoveIceCube_Walk,
					MoveIceCube_ThrowIce,
					MoveIceCube_GotHit,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE ICECUBE: STANDING ******************************/

static void  MoveIceCube_Standing(ObjNode *theNode)
{
float	angleToTarget,dist;
float	fps = gFramesPerSecondFrac;


	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

				/* TURN TOWARDS ME */

	angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, ICECUBE_TURN_SPEED, true);
	dist = OGLPoint3D_Distance(&gPlayerInfo.coord, &gCoord);


				/* SEE IF CHASE */


	theNode->StandDelay -= fps;								// see if in stand delay
	if (theNode->StandDelay <= 0.0f)
	{
		if (!IsBottomlessPitInFrontOfEnemy(theNode->Rot.y))
		{
			if ((dist < ICECUBE_CHASE_DIST_MAX) && (dist > ICECUBE_CHASE_DIST_MIN))
			{
				if (!SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE))		// dont chase thru walls
				{
					MorphToSkeletonAnim(theNode->Skeleton, ICECUBE_ANIM_WALK, 4);
				}
			}
		}
	}


			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);

				/* SEE IF DO ATTACK */

	SeeIfIceCubeAttack(theNode, angleToTarget, dist);



				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateIceCube(theNode);
}




/********************** MOVE ICECUBE: WALKING ******************************/

static void  MoveIceCube_Walk(ObjNode *theNode)
{
float		r,fps,angle,dist,maxSpeed,scaleFactor;

	theNode->Skeleton->AnimSpeed = .5f;							// slow down the walk anim

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, ICECUBE_TURN_SPEED, true);

	scaleFactor = theNode->Scale.x * (1.0f/ICECUBE_SCALE_NORMAL);
	maxSpeed = ICECUBE_WALK_SPEED * scaleFactor;						// scale speed with size-scale

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * maxSpeed;
	gDelta.z = -cos(r) * maxSpeed;
	gDelta.y -= ENEMY_GRAVITY*fps;										// add gravity
	MoveEnemy(theNode);


				/****************/
				/* SEE IF STAND */
				/****************/

	dist = OGLPoint3D_Distance(&gPlayerInfo.coord, &gCoord);


		/* STOP WALKING IF BOTTOMLESS PIT IN FRONT OF US */

	if (IsBottomlessPitInFrontOfEnemy(r))
	{
		MorphToSkeletonAnim(theNode->Skeleton, ICECUBE_ANIM_STAND, 4);
	}
	else
	{
		if ((dist >= ICECUBE_CHASE_DIST_MAX) || (dist <= ICECUBE_CHASE_DIST_MIN))
			MorphToSkeletonAnim(theNode->Skeleton, ICECUBE_ANIM_STAND, 4);
	}


				/* SEE IF DO ATTACK */

	SeeIfIceCubeAttack(theNode, angle, dist);

				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;

	UpdateIceCube(theNode);
}


/********************** MOVE ICECUBE: THROW ICE ******************************/

static void  MoveIceCube_ThrowIce(ObjNode *theNode)
{
float			s;
ObjNode			*icicle = theNode->ChainNode;
OGLMatrix4x4	m,m2,rm;

			/**********************/
			/* DO BASIC FUNCTIONS */
			/**********************/

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)				// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);



				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


			/* SEE IF DONE */

	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, ICECUBE_ANIM_STAND, 3);
	}

	UpdateIceCube(theNode);


			/*********************/
			/* UPDATE THE ICICLE */
			/*********************/

	if (icicle)														// if nil then already thrown
	{
					/* SEE IF THROW THE ICICLE NOW */

		if (theNode->ThrowNow)
		{
			theNode->ThrowNow = false;
			ReleaseIcicle(theNode, icicle);
		}
		else
		{

						/* MAKE ICICLE GROW */

			s = icicle->Scale.x += gFramesPerSecondFrac * 1.4f;
			if (s > 1.0f)												// this is scale ratio, real scale is set in matrix below
				s = 1.0f;
			icicle->Scale.x = icicle->Scale.y = icicle->Scale.z = s;

					/* ALIGN IT IN ENEMY'S GRASP */

			OGLMatrix4x4_SetScale(&m, s,s,s);										// set scale matrix
			OGLMatrix4x4_SetRotate_Y(&rm, PI/2);									// rotate into position for hand
			OGLMatrix4x4_Multiply(&m, &rm, &m2);
			m2.value[M03] = 0;														// offset translation for hand alignment
			m2.value[M13] = -20;
			m2.value[M23] = 0;

			FindJointFullMatrix(theNode,ICECUBE_JOINT_RIGHTHAND,&m);				// calc matrix of joint
			OGLMatrix4x4_Multiply(&m2, &m, &icicle->BaseTransformMatrix);
			SetObjectTransformMatrix(icicle);
		}
	}


}


/********************** MOVE ICECUBE: GOT HIT ******************************/

static void  MoveIceCube_GotHit(ObjNode *theNode)
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
		MorphToSkeletonAnim(theNode->Skeleton, ICECUBE_ANIM_STAND, 3);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateIceCube(theNode);
}





//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME ICECUBE ENEMY *************************/

Boolean PrimeEnemy_IceCube(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE ENEMY */

	newObj = MakeEnemy_IceCube(x, z);
	SetSkeletonAnim(newObj->Skeleton, ICECUBE_ANIM_WALK);

				/* SET SPLINE INFO */


	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;

	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveIceCubeOnSpline;				// set move call
	newObj->MoveCall 		= nil;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE ICECUBE ON SPLINE ***************************/

static void MoveIceCubeOnSpline(ObjNode *theNode)
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
		UpdateIceCubeSparkles(theNode);

				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))					// just do this to see if explosions hurt
			return;


					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		dist = OGLPoint3D_Distance(&gPlayerInfo.coord, &theNode->Coord);
		if (dist < ICECUBE_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveIceCube);

	}
}


/***************** UPDATE ICECUBE ************************/

static void UpdateIceCube(ObjNode *theNode)
{
	float	s;

	UpdateEnemy(theNode);

		/* GROW BACK FROM SHRINK */

	s = theNode->Scale.x;
	if (s < ICECUBE_SCALE_NORMAL)
	{
		s += gFramesPerSecondFrac * .1f;				// grow by some amount
		if (s > ICECUBE_SCALE_NORMAL)					// check if @ normal
			s = ICECUBE_SCALE_NORMAL;

		theNode->Scale.x =
		theNode->Scale.y =
		theNode->Scale.z = s;

		UpdateObjectTransforms(theNode);
		SetIceCubeCollisionBox(theNode, false);			// update collision box based on new scale
	}


		/* UPDATE SPARKLES */

	UpdateIceCubeSparkles(theNode);

}


#pragma mark -

/************ ICECUBE HIT BY STUN PULSE *****************/
//
// Called when weapon is moved and this handler is called after a collision.
//
// Also used for Flame weapon!
//

static Boolean IceCubeHitByStunPulse(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord)


			/* HURT IT */

	HurtIceCube(enemy, weapon->Damage * .25f);


			/* GIVE MOMENTUM */

	enemy->Delta.x += weaponDelta->x * .4f;
	enemy->Delta.y += weaponDelta->y * .4f  + 200.0f;
	enemy->Delta.z += weaponDelta->z * .4f;

	return(true);			// stop weapon
}


/************ ICECUBE HIT BY FREEZE *****************/
//
// Freeze gun actually makes him stronger
//

static Boolean IceCubeHitByFreeze(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)
float	s;

		/* MAKE SCALE GROW */

	s = enemy->Scale.x;
	if (s < ICECUBE_SCALE_MAX)
	{
		s += .15f;										// grow by some amount
		if (s > ICECUBE_SCALE_MAX)						// check if @ max
			s = ICECUBE_SCALE_MAX;

		enemy->Scale.x =
		enemy->Scale.y =
		enemy->Scale.z = s;

		UpdateObjectTransforms(enemy);
		SetIceCubeCollisionBox(enemy, false);			// update collision box based on new scale

		enemy->Health = ICECUBE_HEALTH;					// reset health
	}


	return(true);			// stop weapon
}

/************ ICECUBE HIT BY FLAME *****************/

static Boolean IceCubeHitByFlame(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)
float	s;

//	enemy->Health -= .2f;
//	if (enemy->Health <= 0.0f)
//	{
//		KillIceCube(enemy);
//		return(true);
//	}

		/* MAKE SCALE SHRINK */

	s = enemy->Scale.x;
	if (s > ICECUBE_SCALE_MIN)
	{
		s -= .25f;										// shrink by some amount
		if (s < ICECUBE_SCALE_MIN)						// check if @ min
		{
			KillIceCube(enemy);
			return(true);
		}

		enemy->Scale.x =
		enemy->Scale.y =
		enemy->Scale.z = s;

		UpdateObjectTransforms(enemy);
		SetIceCubeCollisionBox(enemy, false);			// update collision box based on new scale

		ExplodeGeometry(enemy, 200, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 3, 1.0);		// shed some ice
	}

	return(true);			// stop weapon
}




/************ ICECUBE GOT PUNCHED *****************/
//
// returns true if want to disable punch after this
//

static Boolean IceCubeGotPunched(ObjNode *fist, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta)
{
float	r;

#pragma unused(fistCoord, weaponDelta)

			/* HURT IT */

	HurtIceCube(enemy, fist->Damage);


			/* GIVE MOMENTUM */

	r = gPlayerInfo.objNode->Rot.y;

	enemy->Delta.x = -sin(r) * 1000.0f;
	enemy->Delta.z = -cos(r) * 1000.0f;
	enemy->Delta.y = 500.0f;

	return(true);
}


/************ ICECUBE GOT HIT BY JUMP JET *****************/

static void IceCubeHitByJumpJet(ObjNode *enemy)
{
float	r;

		/* HURT IT */

	HurtIceCube(enemy, 1.0);


			/* GIVE MOMENTUM */

	r = gPlayerInfo.objNode->Rot.y;
	enemy->Delta.x = -sin(r) * 1000.0f;
	enemy->Delta.z = -cos(r) * 1000.0f;
	enemy->Delta.y = 500.0f;
}

/************** ICECUBE GOT HIT BY SUPERNOVA *****************/

static Boolean IceCubeHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	KillIceCube(enemy);

	return(false);
}




/*********************** HURT ICECUBE ***************************/

static Boolean HurtIceCube(ObjNode *enemy, float damage)
{
		/* SEE IF STOP THROW ICICLE */

	if (enemy->ChainNode)
	{
		DeleteObject(enemy->ChainNode);
		enemy->ChainNode = nil;
	}


			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveIceCube);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillIceCube(enemy);
		return(true);
	}
	else
	{
					/* GO INTO HIT ANIM */

		if (enemy->Skeleton->AnimNum != ICECUBE_ANIM_GETHIT)
			MorphToSkeletonAnim(enemy->Skeleton, ICECUBE_ANIM_GETHIT, 8);

		enemy->ButtTimer = 1.0f;
	}
	return(false);
}


/****************** KILL ICECUBE ***********************/

static void KillIceCube(ObjNode *enemy)
{
	SpewAtoms(&enemy->Coord, 2,1, 0, false);
	ExplodeGeometry(enemy, 300, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .4);
	PlayEffect3D(EFFECT_SHATTER, &enemy->Coord);

	if (!enemy->EnemyRegenerate)
		enemy->TerrainItemPtr = nil;							// dont ever come back

	DeleteEnemy(enemy);
}




#pragma mark -

/*************** CREATE ICECUBE SPARKLES ********************/

static void CreateIceCubeSparkles(ObjNode *theNode)
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
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 70.0f;
		gSparkles[i].separation = 20.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_RedGlint;
	}
}


/*************** UPDATE ICECUBE SPARKLES ********************/

static void UpdateIceCubeSparkles(ObjNode *theNode)
{
short			i;
float			r,aimX,aimZ;
static const OGLPoint3D	leftEye = {-12,6,-40};
static const OGLPoint3D	rightEye = {12,6,-40};

	r = theNode->Rot.y;
	aimX = -sin(r);
	aimZ = -cos(r);

		/* UPDATE RIGHT EYE */

	i = theNode->Sparkles[0];												// get sparkle index
	if (i != -1)
	{
		FindCoordOnJoint(theNode, ICECUBE_JOINT_HEAD, &rightEye, &gSparkles[i].where);		// calc coord of right eye
		gSparkles[i].aim.x = aimX;											// update aim vector
		gSparkles[i].aim.z = aimZ;
	}


		/* UPDATE LEFT EYE */

	i = theNode->Sparkles[1];												// get sparkle index
	if (i != -1)
	{
		FindCoordOnJoint(theNode, ICECUBE_JOINT_HEAD, &leftEye, &gSparkles[i].where);		// calc coord of left eye
		gSparkles[i].aim.x = aimX;											// update aim vector
		gSparkles[i].aim.z = aimZ;
	}

}


#pragma mark -

/************ SEE IF ICECUBE ATTACK *********************/

static void SeeIfIceCubeAttack(ObjNode *theNode, float angleToPlayer, float distToPlayer)
{
	if (!gPlayerHasLanded)
		return;

	if (gPlayerIsDead)
		return;

	if (gPlayerInfo.invincibilityTimer > 0.0f)
		return;

	theNode->AttackDelay -= gFramesPerSecondFrac;
	if (theNode->AttackDelay > 0.0f)
		return;

	if (angleToPlayer > PI/2)
		return;

		/***************************/
		/* SEE WHICH ATTACK TO USE */
		/***************************/

		/* SEE IF IN THROW-ICE RANGE */

	if ((distToPlayer >= ICECUBE_THROWICE_DIST_MIN) && (distToPlayer <= ICECUBE_THROWICE_DIST_MAX))
	{
		IceCubeThrowIce(theNode);
	}

			/* SEE IF IN BREATH ICE RANGE */
	else
	if (distToPlayer < ICECUBE_THROWICE_DIST_MIN)
	{


	}
}


/*********************** ICE CUBE THROW ICE ******************************/

static void IceCubeThrowIce(ObjNode *enemy)
{
ObjNode	*icicle;

			/* START ENEMY THROWING */

	MorphToSkeletonAnim(enemy->Skeleton, ICECUBE_ANIM_THROWICE, 4);
	enemy->ThrowNow = false;


			/*****************/
			/* CREATE ICICLE */
			/*****************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FIREICE_ObjType_Icicle;
	gNewObjectDefinition.coord		= enemy->Coord;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits  | STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB - 1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .1;
	icicle = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	enemy->ChainNode = icicle;

	icicle->ColorFilter.a = .8;
	icicle->Damage = .1;
}


/********************* RELEASE ICICLE ***************************/
//
// Called when player releases it during the throw
//

static void ReleaseIcicle(ObjNode *enemy, ObjNode *icicle)
{
float	r = enemy->Rot.y;
const OGLPoint3D handOff = {0,-20,0};

	enemy->ChainNode = nil;							// enemy no longer owns this

	icicle->MoveCall = MoveIcicle;

	FindCoordOnJointAtFlagEvent(enemy, ICECUBE_JOINT_RIGHTHAND, &handOff, &icicle->Coord);

	icicle->Scale = enemy->Scale;									// set true scale

	icicle->Rot.y = r;

	icicle->Delta.x = -sin(r) * 1400.0f;
	icicle->Delta.z = -cos(r) * 1400.0f;
	icicle->Delta.y = 200.0f;

	UpdateObjectTransforms(icicle);

		/* SET COLLISION INFO */

	icicle->CType = CTYPE_HURTME;
	icicle->CBits = CBITS_TOUCHABLE;
	CreateCollisionBoxFromBoundingBox_Rotated(icicle, 1, 1);

}


/*********************** MOVE ICICLE *******************************/

static void MoveIcicle(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	gDelta.y -= 1300.0f * fps;						// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.x -= fps * .5f;					// slowly tilt down

			/* SEE IF HIT ANYTHING */

	if (HandleCollisions(theNode, CTYPE_FENCE | CTYPE_TERRAIN | CTYPE_MISC, 0))
	{
		ExplodeGeometry(theNode, 300, SHARD_MODE_BOUNCE, 1, .4);
		PlayEffect_Parms3D(EFFECT_SHATTER, &theNode->Coord, NORMAL_CHANNEL_RATE * 3/2, .7f);
		DeleteObject(theNode);
		return;
	}

	UpdateObject(theNode);
}




