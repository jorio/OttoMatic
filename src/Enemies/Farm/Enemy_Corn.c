/****************************/
/*   ENEMY: CORN.C	*/
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

static void MoveCorn(ObjNode *theNode);
static ObjNode *MakeEnemy_Corn(long x, long z);
static void MoveCorn_Standing(ObjNode *theNode);
static void  MoveCorn_Walking(ObjNode *theNode);
static void  MoveCorn_Grow(ObjNode *theNode);
static void MoveCornOnSpline(ObjNode *theNode);
static void UpdateCorn(ObjNode *theNode);
static void  MoveCorn_GetHit(ObjNode *theNode);
static void CreateCornSparkles(ObjNode *theNode);
static void UpdateCornSparkles(ObjNode *theNode);
static Boolean CornEnemyHitByStunPulse(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void DoCornShoot(ObjNode *enemy);
static void ShootCornKernel(ObjNode *enemy);
static void MoveCornKernel(ObjNode *theNode);
static void PopKernel(ObjNode *theNode, Boolean fromTrigger);
static Boolean HurtCorn(ObjNode *enemy, float damage);
static Boolean KillCorn(ObjNode *enemy);
static void CornHitByJumpJet(ObjNode *enemy);
static Boolean CornHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean CornGotPunched(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_CORNS					8

#define	CORN_SCALE					1.7f
#define	CORN_SCALE_MIN				0.3f

#define	CORN_CHASE_DIST				3000.0f
#define	CORN_DETACH_DIST			(CORN_CHASE_DIST/2.0f)
#define	CORN_ATTACK_DIST			2200.0f

#define	CORN_TARGET_OFFSET			100.0f

#define CORN_TURN_SPEED			2.4f
#define CORN_WALK_SPEED			300.0f

#define	KERNEL_DAMAGE			.15f


		/* ANIMS */


enum
{
	CORN_ANIM_STAND,
	CORN_ANIM_WALK,
	CORN_ANIM_GETHIT,
	CORN_ANIM_GROW
};


/*********************/
/*    VARIABLES      */
/*********************/

#define IsShooting		Flag[1]
#define	DelayToShoot	SpecialF[0]
#define	ShotSpacing		SpecialF[1]
#define	ShotCounter		Special[0]
#define	ButtTimer		SpecialF[3]



/************************ ADD CORN ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Corn(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_CORN] >= MAX_CORNS)
			return(false);
	}


	newObj = MakeEnemy_Corn(x,z);
	newObj->TerrainItemPtr = itemPtr;
	newObj->EnemyRegenerate = itemPtr->parm[3] & (1<<1);

	return(true);
}

/*************** MAKE ENEMY: CORN ************************/

static ObjNode *MakeEnemy_Corn(long x, long z)
{
ObjNode	*newObj;

				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_CORN,x,z, CORN_SCALE, 0, MoveCorn);

	SetSkeletonAnim(newObj->Skeleton, CORN_ANIM_STAND);


				/*******************/
				/* SET BETTER INFO */
				/*******************/

	newObj->Health 		= 1.0;
	newObj->Damage 		= .1;
	newObj->Kind 		= ENEMY_KIND_CORN;



				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,CORN_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] = CornEnemyHitByStunPulse;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] = CornHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] = CornGotPunched;
	newObj->HitByJumpJetHandler 				= CornHitByJumpJet;

	newObj->HurtCallback = HurtCorn;							// set hurt callback function



				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 8,false);

	CreateCornSparkles(newObj);

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_CORN]++;

	return(newObj);
}

/******************* GROW CORN ***********************/
//
// Called from RadiateSprout() when saucer makes this grow
//

void GrowCorn(float x, float z)
{
ObjNode	*newObj;

	newObj = MakeEnemy_Corn(x, z);
	if (newObj)
	{
		newObj->Scale.x = newObj->Scale.y = newObj->Scale.z = .08f;											// start shrunken
		newObj->Rot.y = CalcYAngleFromPointToPoint(0, x, z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);		// aim at player

		SetSkeletonAnim(newObj->Skeleton, CORN_ANIM_GROW);
		UpdateObjectTransforms(newObj);
	}
}


/********************* MOVE CORN **************************/

static void MoveCorn(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveCorn_Standing,
					MoveCorn_Walking,
					MoveCorn_GetHit,
					MoveCorn_Grow,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);
	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE CORN: STANDING ******************************/

static void  MoveCorn_Standing(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

				/* TURN TOWARDS ME */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, CORN_TURN_SPEED, true);



				/* SEE IF CHASE */

	float dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
	if (dist < CORN_CHASE_DIST)
	{
		MorphToSkeletonAnim(theNode->Skeleton, CORN_ANIM_WALK, 8);
	}

			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;



	UpdateCorn(theNode);
}




/********************** MOVE CORN: WALKING ******************************/

static void  MoveCorn_Walking(ObjNode *theNode)
{
float		r,fps;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, CORN_TURN_SPEED, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * CORN_WALK_SPEED;
	gDelta.z = -cos(r) * CORN_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == CORN_ANIM_WALK)
		theNode->Skeleton->AnimSpeed = CORN_WALK_SPEED * .007f;


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	DoCornShoot(theNode);

	UpdateCorn(theNode);
}


/********************** MOVE CORN: GET HIT ******************************/

static void  MoveCorn_GetHit(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(400.0,&gDelta);

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;			// add gravity

	MoveEnemy(theNode);


				/* SEE IF DONE */

	theNode->ButtTimer -= gFramesPerSecondFrac;
	if (theNode->ButtTimer <= 0.0)
	{
		MorphToSkeletonAnim(theNode->Skeleton, CORN_ANIM_STAND, 3);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateCorn(theNode);
}

/********************** MOVE CORN: GROW ******************************/

static void  MoveCorn_Grow(ObjNode *theNode)
{
float	scale;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

		/* MAKE GROW */

	scale = theNode->Scale.x += gFramesPerSecondFrac;

	if (scale >= CORN_SCALE)								// done growing?
	{
		FinalizeSkeletonObjectScale(theNode, CORN_SCALE, CORN_ANIM_STAND);		// make final collision box so corn won't drop underground

		MorphToSkeletonAnim(theNode->Skeleton, CORN_ANIM_STAND, 2);

		gDelta.y = 700.0f;									// bump upwards
	}
	else
	{
		theNode->Scale = (OGLVector3D) {scale, scale, scale};
	}

	if (DoEnemyCollisionDetect(theNode,0, true))			// just do ground
		return;

	UpdateCorn(theNode);
}


//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME CORN ENEMY *************************/

Boolean PrimeEnemy_Corn(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */

	newObj = MakeEnemySkeleton(SKELETON_TYPE_CORN,x,z, CORN_SCALE, 0, nil);


	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;

	SetSkeletonAnim(newObj->Skeleton, CORN_ANIM_WALK);


				/* SET BETTER INFO */

	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		-= newObj->BottomOff;
	newObj->SplineMoveCall 	= MoveCornOnSpline;				// set move call
	newObj->Health 			= 1.0;
	newObj->Damage 			= 0;
	newObj->Kind 			= ENEMY_KIND_CORN;


				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,CORN_TARGET_OFFSET);

				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] = CornEnemyHitByStunPulse;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] = CornHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] 	= CornGotPunched;
	newObj->HitByJumpJetHandler 					= CornHitByJumpJet;

	newObj->HurtCallback = HurtCorn;							// set hurt callback function


	newObj->InitCoord = newObj->Coord;							// remember where started

				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 8, false);

	CreateCornSparkles(newObj);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);									// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE CORN ON SPLINE ***************************/

static void MoveCornOnSpline(ObjNode *theNode)
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
		UpdateCornSparkles(theNode);

				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))					// just do this to see if explosions hurt
			return;

					/* TRY SHOOTING */

		DoCornShoot(theNode);

					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < CORN_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveCorn);

	}
}




#pragma mark -




/***************** UPDATE CORN ************************/

static void UpdateCorn(ObjNode *theNode)
{
	UpdateEnemy(theNode);
	UpdateCornSparkles(theNode);
}


#pragma mark -



/*************** CREATE CORN SPARKLES ********************/

static void CreateCornSparkles(ObjNode *theNode)
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

		gSparkles[i].scale = 170.0f;
		gSparkles[i].separation = 10.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark3;
	}
}


/*************** UPDATE CORN SPARKLES ********************/

static void UpdateCornSparkles(ObjNode *theNode)
{
short			i;
float			r,aimX,aimZ;
static const OGLPoint3D	leftEye = {-27,10,-60};
static const OGLPoint3D	rightEye = {27,10,-60};

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

/************ CORN HIT BY STUN PULSE *****************/
//
// Called when weapon is moved and this handler is called after a collision.
//

static Boolean CornEnemyHitByStunPulse(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

			/* HURT IT */

	if (HurtCorn(enemy, .5))
		return(true);




			/* GIVE MOMENTUM */

	enemy->Delta.x += weaponDelta->x * .6f;
	enemy->Delta.y += weaponDelta->y * .6f;
	enemy->Delta.z += weaponDelta->z * .6f;

	return(true);
}

/************ CORN GOT PUNCHED *****************/
//
// returns true if want to disable punch after this
//

static Boolean CornGotPunched(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta)
{
float	r;

#pragma unused (weapon, fistCoord, weaponDelta)

			/* HURT IT */

	if (HurtCorn(enemy, .2))
		return(true);


			/* GIVE MOMENTUM */

	r = gPlayerInfo.objNode->Rot.y;
	enemy->Delta.x = -sin(r) * 500.0f;
	enemy->Delta.z = -cos(r) * 500.0f;
	enemy->Delta.y = 300.0f;

	return(true);
}

/************** CORN GOT HIT BY SUPERNOVA  *****************/

static Boolean CornHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	KillCorn(enemy);

	return(false);
}

/************** CORN GOT HIT BY JUMP JET *****************/

static void CornHitByJumpJet(ObjNode *enemy)
{
	KillCorn(enemy);
}




/*********************** HURT CORN ***************************/

static Boolean HurtCorn(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveCorn);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		return (KillCorn(enemy));
	}
	else
	{
					/* FINALIZE SCALE & COLLISION BOX */
					//
					// If we don't do this, a miniature corn will keep the giant collision box
					// that was reserved for it to grow, and it'll hover above the ground.
					//

		if (enemy->Skeleton->AnimNum == CORN_ANIM_GROW)
		{
			float finalScale = MaxFloat(CORN_SCALE_MIN, enemy->Scale.y);
			FinalizeSkeletonObjectScale(enemy, finalScale, CORN_ANIM_STAND);
		}

					/* GO INTO HIT ANIM */

		if (enemy->Skeleton->AnimNum != CORN_ANIM_GETHIT)
		{
			MorphToSkeletonAnim(enemy->Skeleton, CORN_ANIM_GETHIT, 5);
		}

		enemy->ButtTimer = 3.0f;
	}

	return(false);
}


/****************** KILL CORN *********************/
//
// OUTPUT: true if ObjNode was deleted
//

static Boolean KillCorn(ObjNode *enemy)
{
ObjNode	*newObj;
int		i;
float	y;

	SpewAtoms(&enemy->Coord, 0,1, 3, false);
	PlayEffect3D(EFFECT_CORNCRUNCH, &enemy->Coord);

			/* EXPLODE POPCORN */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FARM_ObjType_PopCorn;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MovePopcorn;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.0;

	y = GetTerrainY(enemy->Coord.x, enemy->Coord.z);

	for (i = 0; i < 20; i++)
	{
		gNewObjectDefinition.coord.x 	= enemy->Coord.x + RandomFloat2() * 70.0f;
		gNewObjectDefinition.coord.z 	= enemy->Coord.z + RandomFloat2() * 70.0f;
		gNewObjectDefinition.coord.y 	= y + RandomFloat() * 250.0f;

		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Delta.x = RandomFloat2() * 700.0f;
		newObj->Delta.z = RandomFloat2() * 700.0f;
		newObj->Delta.y = 600.0f + RandomFloat2() * 1000.0f;

		newObj->DeltaRot.x = RandomFloat2() * PI2;
		newObj->DeltaRot.y = RandomFloat2() * PI2;
		newObj->DeltaRot.z = RandomFloat2() * PI2;

		newObj->Health = 3.0f + RandomFloat() * 2.0f;

		CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	}


			/* EXPLODE GEOMETRY */

	ExplodeGeometry(enemy, 300, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 2, 1.0);


	DeleteEnemy(enemy);
	return(true);
}




#pragma mark -

/********************* DO CORN SHOOT ****************************/

static void DoCornShoot(ObjNode *enemy)
{
float	fps = gFramesPerSecondFrac;

			/*********************************/
			/* UPDATE CURRENT SHOOTING SPREE */
			/*********************************/

	if (enemy->IsShooting)
	{
			/* SEE IF TIME TO FIRE OFF A KERNEL */

		enemy->ShotSpacing -= fps;
		if (enemy->ShotSpacing <= 0.0f)
		{
			enemy->ShotSpacing += .4f;								// reset spacing timer

			ShootCornKernel(enemy);


				/* SEE IF ALL DONE */

			enemy->ShotCounter++;
			if (enemy->ShotCounter >= 3)
				enemy->IsShooting = false;
		}
	}

			/********************************/
			/* SEE IF SHOULD START SHOOTING */
			/********************************/
	else
	{
		enemy->DelayToShoot -= fps;							// check timer
		if (enemy->DelayToShoot <= 0.0f)
		{
				/* SEE IF AIMED AT PLAYER */

			if (TurnObjectTowardTarget(enemy, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, 0, true) < (PI/3.0f))
			{
				/* SEE IF IN RANGE */

				if (CalcDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < CORN_ATTACK_DIST)
				{
						/* START SHOOTING */

					enemy->IsShooting = true;
					enemy->DelayToShoot = 4.0f;
					enemy->ShotCounter = 0;
					enemy->ShotSpacing = 0;
				}
			}
		}
	}
}


/*************************** SHOOT CORN KERNEL *****************************/

static void ShootCornKernel(ObjNode *enemy)
{
ObjNode	*newObj;
float	f;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FARM_ObjType_CornKernel;
	gNewObjectDefinition.coord.x 	= gCoord.x;
	gNewObjectDefinition.coord.y 	= gCoord.y + 230.0f;
	gNewObjectDefinition.coord.z 	= gCoord.z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveCornKernel;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_MISC;
	newObj->CBits			= CBITS_ALLSOLID|CBITS_ALWAYSTRIGGER;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_CORNKERNEL;

	CreateCollisionBoxFromBoundingBox(newObj, 1, 1);

			/* SET BULLET INFO */

	newObj->Damage = KERNEL_DAMAGE;

	f = CalcDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
	f *= .8f;

	newObj->Delta.x = -sin(enemy->Rot.y) * f + (RandomFloat2() * 200.0f);
	newObj->Delta.z = -cos(enemy->Rot.y) * f + (RandomFloat2() * 200.0f);
	newObj->Delta.y = 100.0f;

	newObj->DeltaRot.x = RandomFloat2() * PI2;
	newObj->DeltaRot.y = RandomFloat2() * PI2;
	newObj->DeltaRot.z = RandomFloat2() * PI2;

	AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 3,3, true);


	newObj->Health = 4.0f + RandomFloat() * 3.0f;

	PlayEffect3D(EFFECT_SHOOTCORN, &newObj->Coord);
}


/******************* MOVE CORN KERNEL ************************/

static void MoveCornKernel(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;


		/* SEE IF OUT OF RANGE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

			/* SEE IF AUTO-POP */

	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
		PopKernel(theNode, false);


	GetObjectInfo(theNode);


			/* MOVE IT */

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// do friction if landed
	{
		ApplyFrictionToDeltasXZ(2000.0,&gDelta);
		ApplyFrictionToRotation(10.0,&theNode->DeltaRot);
	}

	gDelta.y -= 1000.0f * fps;								// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.x += theNode->DeltaRot.x * fps;
	theNode->Rot.y += theNode->DeltaRot.y * fps;
	theNode->Rot.z += theNode->DeltaRot.z * fps;


		/* HANDLE COLLISIONS */

	HandleCollisions(theNode, CTYPE_MISC|CTYPE_TERRAIN|CTYPE_FENCE, -.3);

	UpdateObject(theNode);


}


/************** DO TRIGGER - CORN KERNEL ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_CornKernel(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
#pragma unused (whoNode, sideBits)

	PopKernel(theNode, true);
	return(true);
}


/****************** POP KERNEL ************************/

static void PopKernel(ObjNode *theNode, Boolean fromTrigger)
{
long					pg,i;
OGLVector3D				delta;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;

	theNode->Type = FARM_ObjType_PopCorn;
	ResetDisplayGroupObject(theNode);

	theNode->CType = 0;

	theNode->Delta.x = RandomFloat2() * 600.0f;
	theNode->Delta.y = 700.0f + RandomFloat() * 600.0f;
	theNode->Delta.z = RandomFloat2() * 600.0f;

	theNode->DeltaRot.x = RandomFloat2() * 10.0f;
	theNode->DeltaRot.y = RandomFloat2() * 10.0f;
	theNode->DeltaRot.z = RandomFloat2() * 10.0f;

	theNode->MoveCall = MovePopcorn;

	CreateCollisionBoxFromBoundingBox(theNode, 1, 1);

	theNode->Health = 5;


	PlayEffect3D(EFFECT_POPCORN, &theNode->Coord);


			/* ALSO PUT DOWN PARTICLES */

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
	gNewParticleGroupDef.gravity				= 900;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 20;
	gNewParticleGroupDef.decayRate				= 0;
	gNewParticleGroupDef.fadeRate				= 1.0;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_RedSpark;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		float	x = theNode->Coord.x;
		float	y = theNode->Coord.y;
		float	z = theNode->Coord.z;

		for (i = 0; i < 15; i++)
		{
			pt.x = x + RandomFloat2() * 30.0f;
			pt.y = y + RandomFloat2() * 20.0f;
			pt.z = z + RandomFloat2() * 30.0f;

			delta.x = RandomFloat2() * 300.0f;
			delta.y = RandomFloat() * 400.0f;
			delta.z = RandomFloat2() * 300.0f;

			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA;
			AddParticleToGroup(&newParticleDef);
		}
	}


			/*********************/
			/* SEE IF HIT PLAYER */
			/*********************/

	if (gPlayerInfo.invincibilityTimer <= 0.0f)
	{
		if (CalcDistance3D(theNode->Coord.x, theNode->Coord.y, theNode->Coord.z,
							gPlayerInfo.coord.x, gPlayerInfo.coord.y, gPlayerInfo.coord.z) < 190.0f)
		{
			ObjNode *player = gPlayerInfo.objNode;
			OGLVector3D	v;

			PlayerGotHit(theNode, 0);

			v.x = gPlayerInfo.coord.x - theNode->Coord.x;
			v.y = gPlayerInfo.coord.y - theNode->Coord.y;
			v.z = gPlayerInfo.coord.z - theNode->Coord.z;
			FastNormalizeVector(v.x, v.y, v.z, &v);

			if (fromTrigger)
			{
				gDelta.x = v.x * 2000.0f;
				gDelta.y = v.y * 2000.0f;
				gDelta.z = v.z * 2000.0f;
			}
			else
			{
				player->Delta.x = v.x * 2000.0f;
				player->Delta.y = v.y * 2000.0f;
				player->Delta.z = v.z * 2000.0f;
			}
		}
	}

}


/******************* MOVE POPCORN ************************/

void MovePopcorn(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;


		/* SEE IF OUT OF RANGE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
nuke:
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);


			/* MOVE IT */

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// do friction if landed
	{
		ApplyFrictionToDeltasXZ(400.0,&gDelta);
		ApplyFrictionToRotation(10.0,&theNode->DeltaRot);
	}

	gDelta.y -= 800.0f * fps;								// gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.x += theNode->DeltaRot.x * fps;
	theNode->Rot.y += theNode->DeltaRot.y * fps;
	theNode->Rot.z += theNode->DeltaRot.z * fps;


		/* HANDLE COLLISIONS */

	HandleCollisions(theNode, CTYPE_MISC|CTYPE_TERRAIN|CTYPE_FENCE|CTYPE_PLAYER, -.3);


			/* DECAY */

	theNode->Health -= fps;
	if (theNode->Health < 0.0f)
	{
		theNode->ColorFilter.a -= fps;
		if (theNode->ColorFilter.a <= 0.0f)
			goto nuke;
		if (theNode->ShadowNode)									// also fade shadow
			theNode->ShadowNode->ColorFilter.a = theNode->ColorFilter.a;
	}

	UpdateObject(theNode);
}






