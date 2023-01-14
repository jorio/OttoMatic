/****************************/
/*   ENEMY: GIANTLIZARD.C	*/
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

static void MoveGiantLizard(ObjNode *theNode);
static void MoveGiantLizard_Standing(ObjNode *theNode);
static void  MoveGiantLizard_Walk(ObjNode *theNode);
static void MoveGiantLizardOnSpline(ObjNode *theNode);
static void UpdateGiantLizard(ObjNode *theNode);
static void  MoveGiantLizard_Breathe(ObjNode *theNode);
static void  MoveGiantLizard_Eat(ObjNode *theNode);
static void  MoveGiantLizard_Gobble(ObjNode *theNode);
static void  MoveGiantLizard_Throw(ObjNode *theNode);
static void  MoveGiantLizard_GotHit(ObjNode *theNode);
static void  MoveGiantLizard_Death(ObjNode *theNode);
static void  MoveGiantLizard_Angry(ObjNode *theNode);

static void KillGiantLizard(ObjNode *enemy);

static Boolean HurtGiantLizard(ObjNode *enemy, float damage);

static void SeeIfGiantLizardAttack(ObjNode *theNode, float angleToPlayer, float distToPlayer);
static void GiantLizardShootFlames(ObjNode *enemy);
static void MoveLizardFlame(ObjNode *theNode);
static void MakeLizardFlamePoof(ObjNode *flame);

static void AlignPlayerInLizardGrasp(ObjNode *enemy);
static Boolean GiantLizardHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void GiantLizardHitByJumpJet(ObjNode *enemy);


/****************************/
/*    CONSTANTS             */
/****************************/

#if !VIP_ENEMIES
#define	MAX_GIANTLIZARDS					8
#endif

#define	GIANTLIZARD_SCALE					2.5f

#define	GIANTLIZARD_CHASE_DIST_MAX		2500.0f
#define	GIANTLIZARD_DETACH_DIST			(GIANTLIZARD_CHASE_DIST_MAX / 3.0f)

#define	GIANTLIZARD_FLAME_DIST_MAX			1400.0f
#define	GIANTLIZARD_FLAME_DIST_MIN			700.0f

#define	GIANTLIZARD_EAT_DIST				700.0f

#define	GIANTLIZARD_TARGET_OFFSET		100.0f

#define GIANTLIZARD_TURN_SPEED			1.3f
#define GIANTLIZARD_WALK_SPEED			280.0f


		/* ANIMS */


enum
{
	GIANTLIZARD_ANIM_STAND,
	GIANTLIZARD_ANIM_WALK,
	GIANTLIZARD_ANIM_BREATE,
	GIANTLIZARD_ANIM_EAT,
	GIANTLIZARD_ANIM_GOBBLE,
	GIANTLIZARD_ANIM_THROW,
	GIANTLIZARD_ANIM_GETHIT,
	GIANTLIZARD_ANIM_DEATH,
	GIANTLIZARD_ANIM_ANGRY
};


enum
{
	GIANTLIZARD_JOINT_HEAD = 7

};


/*********************/
/*    VARIABLES      */
/*********************/

#define	GobbleTimer		SpecialF[0]
#define	ButtTimer		SpecialF[2]
#define	AngryTimer		SpecialF[4]
#define	RoarSpacing		SpecialF[5]

#define	BreatheFire		Flag[0]
#define	ThrowPlayerFlag	Flag[0]
#define	GrippingPlayer	Flag[1]

static const OGLPoint3D gJawOff = {0,-50,-60};


/************************ ADD GIANTLIZARD ENEMY *************************/

Boolean AddEnemy_GiantLizard(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;
int		i;

#if !VIP_ENEMIES
	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_GIANTLIZARD] >= MAX_GIANTLIZARDS)
			return(false);
	}
#endif



				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_GIANTLIZARD,x,z, GIANTLIZARD_SCALE, 0, MoveGiantLizard);
	newObj->TerrainItemPtr = itemPtr;
	newObj->EnemyRegenerate = itemPtr->parm[3] & (1<<1);

	SetSkeletonAnim(newObj->Skeleton, GIANTLIZARD_ANIM_STAND);


				/*******************/
				/* SET BETTER INFO */
				/*******************/

	newObj->Health 		= 1.0;
	newObj->Damage 		= .1;
	newObj->Kind 		= ENEMY_KIND_GIANTLIZARD;

	newObj->BreatheFire = false;

				/* SET COLLISION INFO */

	SetObjectCollisionBounds(newObj, 160,-500,-200,200,200,-200);
	CalcNewTargetOffsets(newObj,GIANTLIZARD_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	for (i = 0; i < NUM_WEAPON_TYPES; i++)
		newObj->HitByWeaponHandler[i] = GiantLizardHitByWeapon;
	newObj->HitByJumpJetHandler = GiantLizardHitByJumpJet;

	newObj->HurtCallback = HurtGiantLizard;							// set hurt callback function



				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 20, 35,false);


#if !VIP_ENEMIES	// these are important enemies, so they don't count towards the total enemy count
	gNumEnemies++;
#endif
	gNumEnemyOfKind[ENEMY_KIND_GIANTLIZARD]++;
	return(true);
}

/********************* MOVE GIANTLIZARD **************************/

static void MoveGiantLizard(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveGiantLizard_Standing,
					MoveGiantLizard_Walk,
					MoveGiantLizard_Breathe,
					MoveGiantLizard_Eat,
					MoveGiantLizard_Gobble,
					MoveGiantLizard_Throw,
					MoveGiantLizard_GotHit,
					MoveGiantLizard_Death,
					MoveGiantLizard_Angry
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE GIANTLIZARD: STANDING ******************************/

static void  MoveGiantLizard_Standing(ObjNode *theNode)
{
float	dist;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);


				/* SEE IF CHASE */

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
	if (dist < GIANTLIZARD_CHASE_DIST_MAX
		&& !SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE | CTYPE_BLOCKRAYS))		// don't start chasing thru walls (spawn at item #202 to test)
	{
		MorphToSkeletonAnim(theNode->Skeleton, GIANTLIZARD_ANIM_WALK, 2);
	}

			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateGiantLizard(theNode);
}




/********************** MOVE GIANTLIZARD: WALKING ******************************/

static void  MoveGiantLizard_Walk(ObjNode *theNode)
{
float		r,fps,angle,dist;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, GIANTLIZARD_TURN_SPEED, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * GIANTLIZARD_WALK_SPEED;
	gDelta.z = -cos(r) * GIANTLIZARD_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


				/* SEE IF STAND */

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
	if (dist >= GIANTLIZARD_CHASE_DIST_MAX)
		MorphToSkeletonAnim(theNode->Skeleton, GIANTLIZARD_ANIM_STAND, 4);


				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == GIANTLIZARD_ANIM_WALK)
		theNode->Skeleton->AnimSpeed = GIANTLIZARD_WALK_SPEED * .005f;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;

				/* SEE IF DO ATTACK */

	SeeIfGiantLizardAttack(theNode, angle, dist);


	UpdateGiantLizard(theNode);
}

/********************** MOVE GIANTLIZARD: BREATHE ******************************/

static void  MoveGiantLizard_Breathe(ObjNode *theNode)
{
			/* MOVE */

	ApplyFrictionToDeltas(3000.0,&gDelta);
	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, GIANTLIZARD_TURN_SPEED/5, false);

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;

				/* BREATHE FIRE */

	if (theNode->BreatheFire)
	{
		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect3D(EFFECT_FIREBREATH, &gCoord);
		else
			Update3DSoundChannel(EFFECT_FIREBREATH, &theNode->EffectChannel, &gCoord);

		GiantLizardShootFlames(theNode);
	}

				/* SEE IF ABORT */
	else
	if (gPlayerInfo.scaleRatio <= 1.0f)							// only test abort if play is normal size
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < GIANTLIZARD_FLAME_DIST_MIN)	// abort if player too close
		{
			MorphToSkeletonAnim(theNode->Skeleton, GIANTLIZARD_ANIM_EAT, 2);	// eat instead
		}
	}



				/* SEE IF DONE */

	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, GIANTLIZARD_ANIM_WALK, 1);
		theNode->BreatheFire = false;
	}


	UpdateGiantLizard(theNode);
}


/********************** MOVE GIANTLIZARD: EAT ******************************/

static void  MoveGiantLizard_Eat(ObjNode *theNode)
{


			/* MOVE */

	ApplyFrictionToDeltas(2000.0,&gDelta);
	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, GIANTLIZARD_TURN_SPEED/5, false);

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


			/*********************/
			/* SEE IF EAT PLAYER */
			/*********************/

	if (theNode->Skeleton->AnimHasStopped)
	{
		OGLPoint3D	intersect;
		Boolean		overTop;
		float		fenceTop;

		if (!SeeIfLineSegmentHitsFence(&gPlayerInfo.coord, &gCoord, &intersect, &overTop, &fenceTop))	// dont grab if there's a fence between us
		{
			OGLPoint3D	p;

			FindCoordOnJoint(theNode, GIANTLIZARD_JOINT_HEAD, &gJawOff, &p);			// get coord of mouth
			if (CalcQuickDistance(p.x, p.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < 200.0f)
			{
				theNode->GrippingPlayer = true;
				theNode->GobbleTimer = 4.0;												// set duration of gobble
				SetSkeletonAnim(theNode->Skeleton, GIANTLIZARD_ANIM_GOBBLE);			// make gobble
				SetSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_GRABBED);	// put player into grabbed mode
			}
			else
				MorphToSkeletonAnim(theNode->Skeleton, GIANTLIZARD_ANIM_WALK, 1);		// dont eat, so keep chasing
		}
		else
			MorphToSkeletonAnim(theNode->Skeleton, GIANTLIZARD_ANIM_WALK, 1);		// dont eat, so keep chasing

	}


	UpdateGiantLizard(theNode);
}



/********************** MOVE GIANTLIZARD: GOBBLE ******************************/

static void  MoveGiantLizard_Gobble(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	ApplyFrictionToDeltas(2000.0,&gDelta);


			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


			/* ALIGN PLAYER IN GRASP */

	AlignPlayerInLizardGrasp(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
	{
		return;
	}


				/* SEE IF DONE */

	theNode->GobbleTimer -= fps;
	if (theNode->GobbleTimer <= 0.0f)
	{
		MorphToSkeletonAnim(theNode->Skeleton, GIANTLIZARD_ANIM_THROW, 3);
		theNode->ThrowPlayerFlag = false;
	}


	UpdateGiantLizard(theNode);

}



/********************** MOVE GIANTLIZARD: THROW ******************************/

static void  MoveGiantLizard_Throw(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	ApplyFrictionToDeltas(2000.0,&gDelta);


			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	if (theNode->GrippingPlayer)
	{
		AlignPlayerInLizardGrasp(theNode);


			/* SEE IF THROW PLAYER */

		if (theNode->ThrowPlayerFlag)					// see if throw player now
		{
			OGLMatrix4x4	m;
			ObjNode			*player = gPlayerInfo.objNode;
			const OGLVector3D	throwVec = {0,-.2,-1};

			MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_THROWN, 7.0);
			PlayEffect_Parms3D(EFFECT_THROWNSWOOSH, &player->Coord, NORMAL_CHANNEL_RATE, 2.0);

			FindJointMatrixAtFlagEvent(theNode, GIANTLIZARD_JOINT_HEAD, &m);			// calc throw vector @ exact time of throw flag
			OGLVector3D_Transform(&throwVec, &m, &player->Delta);
			player->Delta.x *= 3200.0f;
			player->Delta.y = 0; //3200.0f;
			player->Delta.z *= 3200.0f;
			gCurrentMaxSpeed = 20000;

			theNode->GrippingPlayer = false;
		}
	}


			/* SEE IF DONE */

	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, GIANTLIZARD_ANIM_STAND, 1);
	}

	UpdateGiantLizard(theNode);
}





/********************** MOVE GIANT LIZARD: GOT HIT ******************************/

static void  MoveGiantLizard_GotHit(ObjNode *theNode)
{
	if (theNode->Speed3D > 500.0f)
		TurnObjectTowardTarget(theNode, &gCoord, gCoord.x - gDelta.x, gCoord.z - gDelta.z, 1, false);	// aim with motion

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(1200.0,&gDelta);

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;			// add gravity

	MoveEnemy(theNode);


				/* SEE IF DONE */

	theNode->ButtTimer -= gFramesPerSecondFrac;
	if (theNode->ButtTimer <= 0.0)
	{
		MorphToSkeletonAnim(theNode->Skeleton, GIANTLIZARD_ANIM_STAND, 3);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateGiantLizard(theNode);
}


/********************** MOVE GIANTLIZARD: DEATH ******************************/

static void  MoveGiantLizard_Death(ObjNode *theNode)
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

	if (DoEnemyCollisionDetect(theNode,DEATH_ENEMY_COLLISION_CTYPES, false))
		return;


				/* UPDATE */

	UpdateGiantLizard(theNode);

}


/********************** MOVE GIANT LIZARD: ANGRY ******************************/

static void  MoveGiantLizard_Angry(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	ApplyFrictionToDeltas(1200.0,&gDelta);

	gDelta.y -= ENEMY_GRAVITY*fps;			// add gravity

	MoveEnemy(theNode);

				/* SEE IF DONE */

	theNode->AngryTimer -= fps;
	if (theNode->AngryTimer <= 0.0)
		MorphToSkeletonAnim(theNode->Skeleton, GIANTLIZARD_ANIM_STAND, 3);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateGiantLizard(theNode);
}

//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME GIANTLIZARD ENEMY *************************/

Boolean PrimeEnemy_GiantLizard(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */

	newObj = MakeEnemySkeleton(SKELETON_TYPE_GIANTLIZARD,x,z, GIANTLIZARD_SCALE,0,nil);


	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;

	SetSkeletonAnim(newObj->Skeleton, GIANTLIZARD_ANIM_WALK);
	newObj->Skeleton->AnimSpeed = 2.0f;

				/* SET BETTER INFO */

	newObj->InitCoord 		= newObj->Coord;							// remember where started
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveGiantLizardOnSpline;					// set move call
	newObj->Health 			= 1.0;
	newObj->Damage 			= 0;
	newObj->Kind 			= ENEMY_KIND_GIANTLIZARD;

	newObj->BreatheFire = false;


				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,GIANTLIZARD_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	for (int i = 0; i < NUM_WEAPON_TYPES; i++)
		newObj->HitByWeaponHandler[i] = GiantLizardHitByWeapon;
	newObj->HitByJumpJetHandler = GiantLizardHitByJumpJet;

	newObj->HurtCallback = HurtGiantLizard;							// set hurt callback function



				/* MAKE SHADOW & GLOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 20, 35, false);



			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE GIANTLIZARD ON SPLINE ***************************/

static void MoveGiantLizardOnSpline(ObjNode *theNode)
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

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < GIANTLIZARD_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveGiantLizard);

	}
}


/***************** UPDATE GIANTLIZARD ************************/

static void UpdateGiantLizard(ObjNode *theNode)
{
	theNode->RoarSpacing -= gFramesPerSecondFrac;				// dec the roar effect spacing timer

	UpdateEnemy(theNode);

}


/************** ALIGN PLAYER IN LIZARD GRASP ********************/

static void AlignPlayerInLizardGrasp(ObjNode *enemy)
{
OGLMatrix4x4	m,m2,m3;
float		scale;

			/* CALC SCALE MATRIX */

	scale = PLAYER_DEFAULT_SCALE/GIANTLIZARD_SCALE;					// to adjust from onion's scale to player's
	OGLMatrix4x4_SetScale(&m2, scale, scale, scale);


			/* CALC TRANSLATE MATRIX */

	OGLMatrix4x4_SetTranslate(&m3, 0, -30.0f, -70.0f);
	OGLMatrix4x4_Multiply(&m2, &m3, &m);


			/* GET ALIGNMENT MATRIX */

	FindJointFullMatrix(enemy, GIANTLIZARD_JOINT_HEAD, &m2);									// get joint's matrix


	OGLMatrix4x4_Multiply(&m, &m2, &gPlayerInfo.objNode->BaseTransformMatrix);
	SetObjectTransformMatrix(gPlayerInfo.objNode);


			/* FIND COORDS */

	FindCoordOnJoint(enemy, GIANTLIZARD_JOINT_HEAD, &gJawOff, &gPlayerInfo.objNode->Coord);			// get coord of mouth
}





#pragma mark -




/************ GIANT LIZARD HIT BY WEAPON *****************/
//
// returns true if want to disable punch/weapon after this
//

static Boolean GiantLizardHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
float	r;

#pragma unused (weaponCoord, weaponDelta)

			/* SEE IF SUPERNOVA */

	if (!weapon)
	{
		KillGiantLizard(enemy);
	}

			/* NOT SUPERNOVA */
	else
	{
		switch(weapon->Kind)
		{
			case	WEAPON_TYPE_FLAME:
			case	WEAPON_TYPE_STUNPULSE:
					if (enemy->Skeleton->AnimNum != GIANTLIZARD_ANIM_ANGRY)					// make lizard angry
					{
						MorphToSkeletonAnim(enemy->Skeleton, GIANTLIZARD_ANIM_ANGRY, 3);
						if (enemy->RoarSpacing <= 0.0f)										// see if do roar
						{
							PlayEffect3D(EFFECT_LIZARDROAR, &enemy->Coord);
							enemy->RoarSpacing = 2.0;
						}
					}
					enemy->AngryTimer = 2.0;

					if (gPlayerInfo.scaleRatio > 1.0f)					 					// hurt lizard
						HurtGiantLizard(enemy, .2f);
					else
						HurtGiantLizard(enemy, .07f);

					break;


			case	WEAPON_TYPE_FIST:
					if (gPlayerInfo.scaleRatio > 1.0f)
					{
						HurtGiantLizard(enemy, .3);

						r = gPlayerInfo.objNode->Rot.y;							// give momentum
						enemy->Delta.x = -sin(r) * 1000.0f;
						enemy->Delta.z = -cos(r) * 1000.0f;
						enemy->Delta.y = 500.0f;
					}
					break;
		}
	}

	return(true);
}


/************** GIANT LIZARD HIT BY BY JUMPJET *****************/

static void GiantLizardHitByJumpJet(ObjNode *enemy)
{
	if (gPlayerInfo.scaleRatio > 1.0f)
		KillGiantLizard(enemy);
}






/*********************** HURT GIANTLIZARD ***************************/

static Boolean HurtGiantLizard(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveGiantLizard);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillGiantLizard(enemy);
		return(true);
	}
	else
	if (damage > 1.0f)				// gotta be enough damage to cause him to fall
	{
					/* GO INTO HIT ANIM */

		if (enemy->Skeleton->AnimNum != GIANTLIZARD_ANIM_GETHIT)
			MorphToSkeletonAnim(enemy->Skeleton, GIANTLIZARD_ANIM_GETHIT, 5);

		enemy->ButtTimer = 3.0f;
	}
	return(false);
}


/****************** KILL GIANTLIZARD ***********************/

static void KillGiantLizard(ObjNode *enemy)
{
int	i;

	enemy->CType = CTYPE_MISC;

	if (enemy->Skeleton->AnimNum != GIANTLIZARD_ANIM_DEATH)
	{
		MorphToSkeletonAnim(enemy->Skeleton, GIANTLIZARD_ANIM_DEATH, 5);
		enemy->BottomOff = 0;
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

/************ SEE IF GIANT LIZARD ATTACK *********************/

static void SeeIfGiantLizardAttack(ObjNode *theNode, float angleToPlayer, float distToPlayer)
{
	if ((gPlayerInfo.invincibilityTimer > 0.0f) || (!gPlayerHasLanded))
		return;

			/* DON'T ATTACK THRU THINGS */

	if (SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE|CTYPE_BLOCKRAYS))
		return;



			/********************************/
			/* CHECK ATTACKS FOR SMALL OTTO */
			/********************************/

	if (gPlayerInfo.scaleRatio <= 1.0f)
	{
				/* FLAME */

		if ((distToPlayer < GIANTLIZARD_FLAME_DIST_MAX) && (distToPlayer > GIANTLIZARD_FLAME_DIST_MIN) && (angleToPlayer < (PI/4.0f)))
		{
			PlayEffect3D(EFFECT_LIZARDINHALE, &gCoord);
			MorphToSkeletonAnim(theNode->Skeleton, GIANTLIZARD_ANIM_BREATE, 2);
			theNode->Skeleton->AnimSpeed = .5;
		}


				/* EAT */

		else
		if ((distToPlayer < GIANTLIZARD_EAT_DIST) && (angleToPlayer < (PI/4.0f)))
		{
			MorphToSkeletonAnim(theNode->Skeleton, GIANTLIZARD_ANIM_EAT, 2);
		}
	}


			/******************************/
			/* CHECK ATTACKS FOR BIG OTTO */
			/******************************/

	else
	{
				/* FLAME */

		if ((distToPlayer < GIANTLIZARD_FLAME_DIST_MAX) && (angleToPlayer < (PI/4.0f)))
		{
			PlayEffect3D(EFFECT_LIZARDINHALE, &gCoord);
			MorphToSkeletonAnim(theNode->Skeleton, GIANTLIZARD_ANIM_BREATE, 2);
			theNode->Skeleton->AnimSpeed = .5;
		}
	}
}


/********************** GIANT LIZARD SHOOT FLAMES **************************/

static void GiantLizardShootFlames(ObjNode *enemy)
{
OGLMatrix4x4	m;
const OGLVector3D headAim = {0,-.7,-1};
OGLVector3D		aim;
ObjNode		*flame;

			/* SEE IF SHOOT NOW */

	enemy->ParticleTimer -= gFramesPerSecondFrac;
	if (enemy->ParticleTimer > 0.0f)
		return;
	enemy->ParticleTimer += .06f;


			/* CALCULATE THE SHOOT VECTOR & COORD */

	FindJointFullMatrix(enemy, GIANTLIZARD_JOINT_HEAD, &m);
	OGLVector3D_Transform(&headAim, &m, &aim);
	OGLPoint3D_Transform(&gJawOff, &m, &gNewObjectDefinition.coord);


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= JUNGLE_ObjType_FireRing;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_USEALIGNMENTMATRIX | STATUS_BIT_GLOW|
									STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOZWRITES|STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-1;
	gNewObjectDefinition.moveCall 	= MoveLizardFlame;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .5f + RandomFloat() * .2f;
	flame = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	flame->ColorFilter.a = .6;
	flame->Health = RandomFloat() * .3f;						// where in alpha decay to kill it

		/* SET DELTAS */

	flame->Delta.x = aim.x * 2500.0f;
	flame->Delta.y = aim.y * 2500.0f;
	flame->Delta.z = aim.z * 2500.0f;

	flame->DeltaRot.z = RandomFloat2() * 10.0f;
	flame->Rot.z = RandomFloat2() * PI2;

				/* SET THE ALIGNMENT MATRIX */

	SetAlignmentMatrixWithZRot(&flame->AlignmentMatrix, &aim, flame->Rot.z);


		/* SET COLLISION */

	flame->Damage 			= .2;
	flame->CType 			= CTYPE_HURTME|CTYPE_FLAMING;
	flame->CBits			= CBITS_TOUCHABLE;

	CreateCollisionBoxFromBoundingBox_Maximized(flame);
}


/****************** MOVE LIZARD FLAME ******************************/

static void MoveLizardFlame(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
OGLVector3D	aim;

	GetObjectInfo(theNode);

			/* FADE */

	theNode->ColorFilter.a -= fps * .7f;
	if (theNode->ColorFilter.a <= theNode->Health)
	{
gone:
		MakeLizardFlamePoof(theNode);
		DeleteObject(theNode);
		return;
	}

			/* MOVE IT */

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Scale.x =
	theNode->Scale.y =
	theNode->Scale.z += fps * .9f;

	theNode->Rot.z += theNode->DeltaRot.z * fps;

		/* SEE IF HIT ANYTHING */

	if (HandleCollisions(theNode, CTYPE_MISC|CTYPE_TERRAIN|CTYPE_FENCE,0))
	{
		goto gone;
	}


		/* SET NEW ALIGNMENT & UPDATE */


	FastNormalizeVector(gDelta.x, gDelta.y, gDelta.z, &aim);
	SetAlignmentMatrixWithZRot(&theNode->AlignmentMatrix, &aim, theNode->Rot.z);
	UpdateObject(theNode);

}


/******************** MAKE LIZARD FLAME POOF *************************/

static void MakeLizardFlamePoof(ObjNode *flame)
{
long					pg,i;
OGLVector3D				delta;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
float					x,y,z;

#pragma unused (flame)

	x = gCoord.x;
	y = gCoord.y;
	z = gCoord.z;


	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_GRAVITOIDS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE|PARTICLE_FLAGS_DISPERSEIFBOUNCE;
	gNewParticleGroupDef.gravity				= -200;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 40;
	gNewParticleGroupDef.decayRate				=  -1.0;
	gNewParticleGroupDef.fadeRate				= .3;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_BlackSmoke;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < 6; i++)
		{
			pt.x = x + RandomFloat2() * 60.0f;
			pt.y = y + RandomFloat2() * 60.0f;
			pt.z = z + RandomFloat2() * 60.0f;

			delta.x = RandomFloat2() * 100.0f;
			delta.y = 200.0f + RandomFloat2() * 220.0f;
			delta.z = RandomFloat2() * 100.0f;


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= RandomFloat() * PI2;
			newParticleDef.rotDZ		= RandomFloat2() * 4.0f;
			newParticleDef.alpha		= .5;
			AddParticleToGroup(&newParticleDef);
		}
	}
}


























