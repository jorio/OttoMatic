/****************************/
/*   ENEMY: TOMATO.C	*/
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

static ObjNode *MakeEnemy_Tomato(long x, long z);
static void MoveTomato(ObjNode *theNode);
static void MoveTomato_Standing(ObjNode *theNode);
static void  MoveTomato_Walking(ObjNode *theNode);
static void MoveTomatoOnSpline(ObjNode *theNode);
static void UpdateTomato(ObjNode *theNode);
static void  MoveTomato_Roll(ObjNode *theNode);
static void  MoveTomato_Jump(ObjNode *theNode);
static void  MoveTomato_Grow(ObjNode *theNode);

static void CreateTomatoSparkles(ObjNode *theNode);
static void UpdateTomatoSparkles(ObjNode *theNode);
static Boolean TomatoGotPunched(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta);
static void TomatoHitByJumpJet(ObjNode *enemy);
static Boolean TomatoHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean TomatoEnemyHitByStunPulse(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean HurtTomato(ObjNode *enemy, float damage);
static void CheckIfTomatoLandedOnPlayer(ObjNode *theNode);
static Boolean KillTomato(ObjNode *enemy);
static void MoveTomatoSlice(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_TOMATOS					8

#define	TOMATO_SCALE					1.3f

#define	TOMATO_CHASE_DIST			2000.0f
#define	TOMATO_DETACH_DIST			(TOMATO_CHASE_DIST / 2.0f)
#define	TOMATO_CEASE_DIST			(TOMATO_CHASE_DIST + 500.0f)
#define	TOMATO_ATTACK_DIST			1200.0f

#define	TOMATO_TARGET_OFFSET			200.0f

#define TOMATO_TURN_SPEED			2.4f
#define TOMATO_WALK_SPEED			400.0f

#define	TOMATO_JUMP_DY				2000.0f

#define	TOMATO_KNOCKDOWN_SPEED		1700.0f

#define	TOMATO_DAMAGE				.25f


		/* ANIMS */


enum
{
	TOMATO_ANIM_STAND,
	TOMATO_ANIM_WALK,
	TOMATO_ANIM_ROLL,
	TOMATO_ANIM_JUMP,
	TOMATO_ANIM_GROW
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	JumpNow			Flag[0]
#define	IsInAir	Flag[1]
#define	DelayUntilAttack	SpecialF[2]
#define	ButtTimer		SpecialF[3]


/************************ ADD TOMATO ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Tomato(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_TOMATO] >= MAX_TOMATOS)
			return(false);
	}


	newObj = MakeEnemy_Tomato(x,z);
	newObj->TerrainItemPtr = itemPtr;
	newObj->EnemyRegenerate = itemPtr->parm[3] & (1<<1);

	return(true);
}


/************************ MAKE TOMATO ENEMY *************************/

static ObjNode *MakeEnemy_Tomato(long x, long z)
{
ObjNode	*newObj;

				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_TOMATO,x,z, TOMATO_SCALE, 0, MoveTomato);


	SetSkeletonAnim(newObj->Skeleton, TOMATO_ANIM_STAND);


				/*******************/
				/* SET BETTER INFO */
				/*******************/

	newObj->StatusBits |= STATUS_BIT_ROTXZY;

	newObj->Health 		= 1.0;
	newObj->Damage 		= .1;
	newObj->Kind 		= ENEMY_KIND_TOMATO;


				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,TOMATO_TARGET_OFFSET);

				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] = TomatoEnemyHitByStunPulse;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] = TomatoHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] = TomatoGotPunched;
	newObj->HitByJumpJetHandler = TomatoHitByJumpJet;

	newObj->HurtCallback = HurtTomato;							// set hurt callback function


				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 8,false);
	CreateTomatoSparkles(newObj);

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_TOMATO]++;

	return(newObj);
}


/******************* GROW TOMATO ***********************/
//
// Called from RadiateSprout() when saucer makes this tomato grow
//

void GrowTomato(float x, float z)
{
ObjNode	*newObj;

	newObj = MakeEnemy_Tomato(x, z);
	if (newObj)
	{
		newObj->Scale.x = newObj->Scale.y = newObj->Scale.z = .08f;											// start shrunken
		newObj->Rot.y = CalcYAngleFromPointToPoint(0, x, z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);		// aim at player

		SetSkeletonAnim(newObj->Skeleton, TOMATO_ANIM_GROW);
		UpdateObjectTransforms(newObj);
	}

}


/********************* MOVE TOMATO **************************/

static void MoveTomato(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveTomato_Standing,
					MoveTomato_Walking,
					MoveTomato_Roll,
					MoveTomato_Jump,
					MoveTomato_Grow
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);
	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE TOMATO: STANDING ******************************/

static void  MoveTomato_Standing(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

				/* TURN TOWARDS ME */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, TOMATO_TURN_SPEED, true);



				/* SEE IF CHASE */

	float dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
	if (dist < TOMATO_CHASE_DIST)
	{
		MorphToSkeletonAnim(theNode->Skeleton, TOMATO_ANIM_WALK, 8);
	}

			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;



	UpdateTomato(theNode);
}




/********************** MOVE TOMATO: WALKING ******************************/

static void  MoveTomato_Walking(ObjNode *theNode)
{
float		r,fps,dist,angleToPlayer;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	angleToPlayer = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, TOMATO_TURN_SPEED, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * TOMATO_WALK_SPEED;
	gDelta.z = -cos(r) * TOMATO_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == TOMATO_ANIM_WALK)
		theNode->Skeleton->AnimSpeed = TOMATO_WALK_SPEED * .008f;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


			/* CHECK RANGE */

	dist = CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
	if (dist > TOMATO_CEASE_DIST)										// see if stop chasing
	{
		MorphToSkeletonAnim(theNode->Skeleton, TOMATO_ANIM_STAND, 6);
		gDelta.x = gDelta.z = 0;
	}
			/* SEE IF ATTACK */

	else
	if ((dist < TOMATO_ATTACK_DIST) && (angleToPlayer < PI/6))			// see if attack
	{
		if (theNode->DelayUntilAttack <= 0.0f)
		{
			MorphToSkeletonAnim(theNode->Skeleton, TOMATO_ANIM_JUMP, 9);
			gDelta.x = gDelta.z = 0;
			theNode->JumpNow = false;
			theNode->IsInAir = false;
			theNode->DelayUntilAttack = 5;									// set delay until it'll do this again

		}
	}

	UpdateTomato(theNode);
}


/********************** MOVE TOMATO: GET HIT ******************************/

static void  MoveTomato_Roll(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(40.0,&gDelta);

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;			// add gravity

	MoveEnemy(theNode);

				/* UPDATE ROTATIONS */

	theNode->Rot.x += gFramesPerSecondFrac * theNode->Speed3D * .01f;					// make spin

	theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, gCoord.x, gCoord.z, theNode->OldCoord.x, theNode->OldCoord.z);


				/* SEE IF DONE */

	theNode->ButtTimer -= gFramesPerSecondFrac;
	if (theNode->ButtTimer <= 0.0)
	{
		MorphToSkeletonAnim(theNode->Skeleton, TOMATO_ANIM_STAND, 3);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateTomato(theNode);
}


/********************** MOVE TOMATO: JUMP ******************************/

static void  MoveTomato_Jump(ObjNode *theNode)
{
			/* SEE IF JUMP NOW */

	if (theNode->JumpNow)
	{
		PlayEffect3D(EFFECT_TOMATOJUMP, &gCoord);

		theNode->JumpNow = false;
		theNode->IsInAir = true;
		gDelta.y = TOMATO_JUMP_DY;
		gDelta.x = (gPlayerInfo.coord.x - gCoord.x) * 1.0f;
		gDelta.z = (gPlayerInfo.coord.z - gCoord.z) * 1.0f;
	}


			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);

	if (theNode->IsInAir)
		theNode->Rot.x -= gFramesPerSecondFrac * theNode->Speed3D * .005f;					// make spin


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;



			/*****************/
			/* SEE IF LANDED */
			/*****************/

	if (theNode->IsInAir)
	{
		if (theNode->StatusBits & STATUS_BIT_ONGROUND)
		{
			MorphToSkeletonAnim(theNode->Skeleton, TOMATO_ANIM_ROLL, 8);
			theNode->ButtTimer = 1.0f;

					/* MAKE DEFORMATION WAVE */

			if (theNode->StatusBits & STATUS_BIT_ONGROUND)		// if landed on terrain then do deformation wave
			{
				DeformationType		defData;

				defData.type 				= DEFORMATION_TYPE_RADIALWAVE;
				defData.amplitude 			= 150.0f;
				defData.radius 				= 0;
				defData.speed 				= 2800;
				defData.origin.x			= gCoord.x;
				defData.origin.y			= gCoord.z;
				defData.oneOverWaveLength 	= 1.0f / 280.0f;
				defData.radialWidth			= 400.0f;
				defData.decayRate			= 100.0f;
				NewSuperTileDeformation(&defData);
			}

					/* SEE IF LANDED ON PLAYER */

			CheckIfTomatoLandedOnPlayer(theNode);
		}
	}

	UpdateTomato(theNode);
}


/********************** MOVE TOMATO: GROW ******************************/

static void  MoveTomato_Grow(ObjNode *theNode)
{
float	scale;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

		/* MAKE GROW */

	scale = theNode->Scale.x += gFramesPerSecondFrac;

	if (scale >= TOMATO_SCALE)
		scale = TOMATO_SCALE;

	theNode->Scale.x =
	theNode->Scale.y =
	theNode->Scale.z = scale;

	if (DoEnemyCollisionDetect(theNode,0, true))			// just do ground
		return;


			/* SEE IF DONE GROWING */

	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, TOMATO_ANIM_STAND, 2);
		gDelta.y = 700.0f;
	}

	UpdateTomato(theNode);
}




//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME TOMATO ENEMY *************************/

Boolean PrimeEnemy_Tomato(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */

	newObj = MakeEnemySkeleton(SKELETON_TYPE_TOMATO,x,z, TOMATO_SCALE, 0, nil);


	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;

	SetSkeletonAnim(newObj->Skeleton, TOMATO_ANIM_WALK);


				/* SET BETTER INFO */

	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		-= newObj->BottomOff;
	newObj->SplineMoveCall 	= MoveTomatoOnSpline;				// set move call
	newObj->Health 			= 1.0;
	newObj->Damage 			= 0;
	newObj->Kind 			= ENEMY_KIND_TOMATO;

//	BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_SKELETONBASE + gNewObjectDefinition.type,
//								 0, -1, MULTI_TEXTURE_COMBINE_ADD, 2);					// set this model to be sphere mapped


				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,TOMATO_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] = TomatoEnemyHitByStunPulse;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] = TomatoHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] = TomatoGotPunched;
	newObj->HitByJumpJetHandler = TomatoHitByJumpJet;

	newObj->HurtCallback = HurtTomato;							// set hurt callback function


	newObj->InitCoord = newObj->Coord;							// remember where started

				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 8, false);

	CreateTomatoSparkles(newObj);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE TOMATO ON SPLINE ***************************/

static void MoveTomatoOnSpline(ObjNode *theNode)
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
		UpdateTomatoSparkles(theNode);

				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))					// just do this to see if explosions hurt
			return;


					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < TOMATO_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveTomato);

	}


}




#pragma mark -






/***************** UPDATE TOMATO ************************/

static void UpdateTomato(ObjNode *theNode)
{
	theNode->DelayUntilAttack -= gFramesPerSecondFrac;

	if ((theNode->Skeleton->AnimNum != TOMATO_ANIM_ROLL) &&		// see if need to fix x-rot
		(theNode->Skeleton->AnimNum != TOMATO_ANIM_JUMP))
		theNode->Rot.x = 0;

	UpdateEnemy(theNode);
	UpdateTomatoSparkles(theNode);

	theNode->BottomOff = theNode->BBox.min.y;					// use the bbox as the collision bottom
}


/*************** CHECK IF TOMATO LANDED ON PLAYER ************************/

static void CheckIfTomatoLandedOnPlayer(ObjNode *theNode)
{
int		i;
ObjNode	*hitObj;

#pragma unused(theNode)

	if (gPlayerInfo.invincibilityTimer > 0.0f)				// dont bother if player is invincible
		return;

	for (i=0; i < gNumCollisions; i++)
	{
		if ((gCollisionList[i].type == COLLISION_TYPE_OBJ) && (gCollisionList[i].sides & SIDE_BITS_BOTTOM))
		{
			hitObj = gCollisionList[i].objectPtr;				// get ObjNode of this collision
			if (hitObj->CType & CTYPE_PLAYER)					// see if player
			{
				if (gPlayerInfo.objNode->Skeleton->AnimNum != PLAYER_ANIM_ACCORDIAN)
				{
					CrushPlayer();
//					MorphToSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_ACCORDIAN, 5);
//					PlayerLoseHealth(TOMATO_DAMAGE, PLAYER_DEATH_TYPE_EXPLODE);
//					gPlayerInfo.objNode->AccordianTimer = 2.0;
//					PlayEffect3D(EFFECT_PLAYERCRUSH, &gPlayerInfo.coord);
				}
			}
		}
	}
}


#pragma mark -

/*************** CREATE TOMATO SPARKLES ********************/

static void CreateTomatoSparkles(ObjNode *theNode)
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


/*************** UPDATE TOMATO SPARKLES ********************/

static void UpdateTomatoSparkles(ObjNode *theNode)
{
short			i;
float			r,aimX,aimZ;
static const OGLPoint3D	leftEye = {-30,10,-60};
static const OGLPoint3D	rightEye = {30,10,-60};

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

/************ TOMATO HIT BY STUN PULSE *****************/
//
// Called when weapon is moved and this handler is called after a collision.
//

static Boolean TomatoEnemyHitByStunPulse(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord)


			/* HURT IT */

	if (HurtTomato(enemy, .5))
		return(true);


			/* GIVE MOMENTUM */

	enemy->Delta.x += weaponDelta->x * .6f;
	enemy->Delta.y += weaponDelta->y * .6f;
	enemy->Delta.z += weaponDelta->z * .6f;

	return(true);
}

/************ TOMATO GOT PUNCHED *****************/
//
// returns true if want to disable punch after this
//

static Boolean TomatoGotPunched(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta)
{
float	r;

#pragma unused (weapon, fistCoord, weaponDelta)

			/* HURT IT */

	if (HurtTomato(enemy, .5))
		return(true);


			/* GIVE MOMENTUM */

	r = gPlayerInfo.objNode->Rot.y;
	enemy->Delta.x = -sin(r) * 1000.0f;
	enemy->Delta.z = -cos(r) * 1000.0f;
	enemy->Delta.y = 500.0f;

	return(true);
}

/************** TOMATO GOT HIT BY SUPERNOVA *****************/

static Boolean TomatoHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	KillTomato(enemy);

	return(false);
}


/************** TOMATO GOT HIT BY JUMP JET *****************/

static void TomatoHitByJumpJet(ObjNode *enemy)
{
	KillTomato(enemy);
}



/*********************** HURT TOMATO ***************************/
//
// OUTPUT: true if ObjNode was deleted
//

static Boolean HurtTomato(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveTomato);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		return (KillTomato(enemy));
	}
	else
	{
					/* GO INTO HIT ANIM */

		if (enemy->Skeleton->AnimNum != TOMATO_ANIM_ROLL)
			MorphToSkeletonAnim(enemy->Skeleton, TOMATO_ANIM_ROLL, 5);

		enemy->ButtTimer = 3.0f;
	}

	return(false);
}


/****************** KILL TOMATO *********************/
//
// OUTPUT: true if ObjNode was deleted
//

static Boolean KillTomato(ObjNode *enemy)
{

int	i;
ObjNode	*newObj;

	SpewAtoms(&enemy->Coord, 0,1, 2, false);

	PlayEffect3D(EFFECT_TOMATOSPLAT, &enemy->Coord);



				/* SET DEFAULT STUFF */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FARM_ObjType_TomatoSlice;
	gNewObjectDefinition.coord.x 	= enemy->Coord.x;
	gNewObjectDefinition.coord.y 	= enemy->Coord.y + enemy->BottomOff + 30.0f;
	gNewObjectDefinition.coord.z 	= enemy->Coord.z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveTomatoSlice;
	gNewObjectDefinition.rot 		= 0;


			/****************/
			/* MAKE  SLICES */
			/****************/

	for (i = 0; i < 5; i++)
	{
		gNewObjectDefinition.scale 		= .5f + RandomFloat2() * .1f;

		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Delta.y = 400.0f + RandomFloat() * 400.0f;
		newObj->Delta.x = RandomFloat2() * 500.0f;
		newObj->Delta.z = RandomFloat2() * 500.0f;

		newObj->DeltaRot.x = RandomFloat2() * 5.0f;
		newObj->DeltaRot.y = RandomFloat2() * 10.0f;
		newObj->DeltaRot.z = RandomFloat2() * 5.0f;

		gNewObjectDefinition.coord.y += 30.0f;
	}

			/* EXPLODE GEOMETRY */

	ExplodeGeometry(enemy, 300, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 2, .8);


	DeleteEnemy(enemy);
	return(true);
}


/*********************** MOVE TOMATO SLICE **********************/

static void MoveTomatoSlice(ObjNode *theNode)
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






