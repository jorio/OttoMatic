/****************************/
/*   ENEMY: SQUOOSHY.C	*/
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLPoint3D				gCoord;
extern	short					gNumEnemies;
extern	float					gFramesPerSecondFrac,gGravity;
extern	OGLVector3D			gDelta;
extern	signed char			gNumEnemyOfKind[];
extern	u_long		gAutoFadeStatusBits;
extern	PlayerInfoType	gPlayerInfo;
extern	SparkleType	gSparkles[];
extern	NewParticleGroupDefType	gNewParticleGroupDef;
extern	SpriteType	*gSpriteGroupList[MAX_SPRITE_GROUPS];

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveSquooshy(ObjNode *theNode);
static ObjNode *MakeEnemy_Squooshy(long x, long z);
static void MoveSquooshy_Standing(ObjNode *theNode);
static void  MoveSquooshy_Jump(ObjNode *theNode);
static void UpdateSquooshy(ObjNode *theNode);
static void  MoveSquooshy_GetHit(ObjNode *theNode);
static Boolean SquooshyEnemyHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void DoSquooshyShoot(ObjNode *enemy);
static void  MoveSquooshy_Death(ObjNode *theNode);
static void MoveSquooshyBullet(ObjNode *theNode);
static Boolean HurtSquooshy(ObjNode *enemy, float damage);
static Boolean KillSquooshy(ObjNode *enemy);
static void SquooshyHitByJumpJet(ObjNode *enemy);
static Boolean SquooshyHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean SquooshyGotPunched(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta);
static void  MoveSquooshy_Land(ObjNode *theNode);
static void  MoveSquooshy_Shoot(ObjNode *theNode);
static Boolean SquooshyEnemyHitByFreeze(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void MoveSquooshy_Frozen(ObjNode *theNode);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_SQUOOSHYS					5

#define	SQUOOSHY_SCALE					2.5f

#define	SQUOOSHY_CHASE_DIST				3000.0f
#define	SQUOOSHY_ATTACK_DIST			1000.0f

#define	SQUOOSHY_TARGET_OFFSET			100.0f

#define SQUOOSHY_TURN_SPEED			1.4f

#define	BULLET_SPEED				1000.0f

		/* ANIMS */
enum
{
	SQUOOSHY_ANIM_STAND,
	SQUOOSHY_ANIM_JUMP,
	SQUOOSHY_ANIM_GETHIT,
	SQUOOSHY_ANIM_LAND,
	SQUOOSHY_ANIM_SHOOT,
	SQUOOSHY_ANIM_DEATH
};

#define	SQUOOSHY_JOINT_HEAD		1


/*********************/
/*    VARIABLES      */
/*********************/

#define	HasJumped		Flag[0]
#define IsShooting		Flag[1]
#define	DelayToShoot	SpecialF[0]
#define	ShotSpacing		SpecialF[1]
#define	ShotCounter		Special[0]
#define	ButtTimer		SpecialF[3]
#define	FrozenTimer		SpecialF[2]

/************************ ADD SQUOOSHY ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Squooshy(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_SQUOOSHY] >= MAX_SQUOOSHYS)
			return(false);
	}


	newObj = MakeEnemy_Squooshy(x,z);
	newObj->TerrainItemPtr = itemPtr;
	newObj->EnemyRegenerate = itemPtr->parm[3] & (1<<1);

	return(true);
}

/*************** MAKE ENEMY: SQUOOSHY ************************/

static ObjNode *MakeEnemy_Squooshy(long x, long z)
{
ObjNode	*newObj;

				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/
				
	newObj = MakeEnemySkeleton(SKELETON_TYPE_SQUOOSHY,x,z, SQUOOSHY_SCALE, 0, MoveSquooshy);

	SetSkeletonAnim(newObj->Skeleton, SQUOOSHY_ANIM_STAND);
	

				/*******************/
				/* SET BETTER INFO */
				/*******************/
			
	newObj->Health 		= 1.0;
	newObj->Damage 		= .1;
	newObj->Kind 		= ENEMY_KIND_SQUOOSHY;
	
	newObj->FrozenTimer	= 0;			
	
		
	
				/* SET COLLISION INFO */
				
	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,SQUOOSHY_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */
				
	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= SquooshyEnemyHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FREEZE] 		= SquooshyEnemyHitByFreeze;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= SquooshyHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] 		= SquooshyGotPunched;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= SquooshyEnemyHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLARE] 		= SquooshyEnemyHitByWeapon;
	
	newObj->HitByJumpJetHandler 						= SquooshyHitByJumpJet;
	
	

	newObj->HurtCallback = HurtSquooshy;							// set hurt callback function

	

				/* MAKE SHADOW */
				
	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 19, 7,false);
	
		
	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_SQUOOSHY]++;
	
	return(newObj);
}



/********************* MOVE SQUOOSHY **************************/

static void MoveSquooshy(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveSquooshy_Standing,
					MoveSquooshy_Jump,
					MoveSquooshy_GetHit,
					MoveSquooshy_Land,
					MoveSquooshy_Shoot,
					MoveSquooshy_Death
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);
	
			/* SEE IF FROZEN */
			
	if (theNode->FrozenTimer > 0.0f)
		MoveSquooshy_Frozen(theNode);
	else	
		myMoveTable[theNode->Skeleton->AnimNum](theNode);	
}



/********************** MOVE SQUOOSHY: STANDING ******************************/

static void  MoveSquooshy_Standing(ObjNode *theNode)
{
float	angleToTarget,dist;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

				/* TURN TOWARDS ME */
				
	angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, SQUOOSHY_TURN_SPEED, true);			

		
				/************************/
				/* SEE IF JUMP OR SHOOT */
				/************************/

	if (angleToTarget < (PI/3))
	{
		dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);
		
					/* SEE IF SHOOT */
					
		if (dist < SQUOOSHY_ATTACK_DIST)
		{
			MorphToSkeletonAnim(theNode->Skeleton, SQUOOSHY_ANIM_SHOOT, 6);
			theNode->ShotSpacing = 1.0;
		}
		
					/* SEE IF JUMP */
		else			
		if (dist < SQUOOSHY_CHASE_DIST)
		{					
			MorphToSkeletonAnim(theNode->Skeleton, SQUOOSHY_ANIM_JUMP, 8);
			theNode->HasJumped = false;
		}
	}
			/* MOVE */
				
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);
				

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;



	UpdateSquooshy(theNode);		
}




/********************** MOVE SQUOOSHY: JUMP ******************************/

static void  MoveSquooshy_Jump(ObjNode *theNode)
{
float		r,fps;

	fps = gFramesPerSecondFrac;

	r = theNode->Rot.y;

			/*******************/
			/* SEE IF JUMP NOW */
			/*******************/

	if ((theNode->Skeleton->AnimHasStopped) && (!theNode->HasJumped))
	{			
		theNode->HasJumped = true;
		
		gDelta.y = 2000.0f;
		gDelta.x = -sin(r) * 600.0f;
		gDelta.z = -cos(r) * 600.0f;
	}
	else
	if (!theNode->HasJumped)						// do friction if hasnt jumped yet
		ApplyFrictionToDeltas(2000.0,&gDelta);



			/* MOVE IT */
			
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);
			

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;




			/*****************/
			/* SEE IF LANDED */
			/*****************/
		
	if (gDelta.y <= 0.0f)
	{			
		if ((theNode->HasJumped) && (theNode->StatusBits & STATUS_BIT_ONGROUND))
		{
			MorphToSkeletonAnim(theNode->Skeleton, SQUOOSHY_ANIM_LAND, 7);	
		}	
	}		
				
	UpdateSquooshy(theNode);		
}


/********************** MOVE SQUOOSHY: LAND ******************************/

static void  MoveSquooshy_Land(ObjNode *theNode)
{

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

			/* MOVE */
				
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);
				

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;

	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, SQUOOSHY_ANIM_STAND, 4);
	
	}

	UpdateSquooshy(theNode);		
}



/********************** MOVE SQUOOSHY: GET HIT ******************************/

static void  MoveSquooshy_GetHit(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(400.0,&gDelta);
		
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;			// add gravity

	MoveEnemy(theNode);


				/* SEE IF DONE */
			
	theNode->ButtTimer -= gFramesPerSecondFrac;
	if (theNode->ButtTimer <= 0.0)
	{
		MorphToSkeletonAnim(theNode->Skeleton, SQUOOSHY_ANIM_STAND, 3);
	}


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateSquooshy(theNode);
}


/********************** MOVE SQUOOSHY: SHOOT ******************************/

static void  MoveSquooshy_Shoot(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

				/* TURN TOWARDS ME */
				
	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, SQUOOSHY_TURN_SPEED, true);			

	DoSquooshyShoot(theNode);

	
			/* MOVE */
				
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);
				

				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;

	if (theNode->Skeleton->AnimHasStopped)
		MorphToSkeletonAnim(theNode->Skeleton, SQUOOSHY_ANIM_STAND, 5);

	UpdateSquooshy(theNode);		
}


/********************* MOVE SQUOOSHY: FROZEN **************************/

static void MoveSquooshy_Frozen(ObjNode *theNode)
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
			
	UpdateSquooshy(theNode);		

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

/********************** MOVE SQUOOSHY: DEATH ******************************/

static void  MoveSquooshy_Death(ObjNode *theNode)
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
		ApplyFrictionToDeltas(2500.0,&gDelta);
	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;		// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */
				
	if (DoEnemyCollisionDetect(theNode,DEATH_ENEMY_COLLISION_CTYPES, true))
		return;


				/* UPDATE */
			
	UpdateSquooshy(theNode);		
	
	
}



#pragma mark -




/***************** UPDATE SQUOOSHY ************************/

static void UpdateSquooshy(ObjNode *theNode)
{
	UpdateEnemy(theNode);
}




#pragma mark -

/************ SQUOOSHY HIT BY STUN PULSE *****************/
//
// Called when weapon is moved and this handler is called after a collision.
//

static Boolean SquooshyEnemyHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon,	weaponCoord)
	
			/* HURT IT */
			
	if (HurtSquooshy(enemy, weapon->Damage))
		return(true);

	
	

			/* GIVE MOMENTUM */

	enemy->Delta.x += weaponDelta->x * .6f;
	enemy->Delta.y += weaponDelta->y * .6f;
	enemy->Delta.z += weaponDelta->z * .6f;
	
	return(true);
}

/************ SQUOOSHY HIT BY FREEZE *****************/

static Boolean SquooshyEnemyHitByFreeze(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon,	weaponCoord, weaponDelta)

			/* CHANGE THE TEXTURE */
				
	enemy->Skeleton->overrideTexture = gSpriteGroupList[SPRITE_GROUP_LEVELSPECIFIC][FIREICE_SObjType_FrozenSquooshy].materialObject;


			/* STOP MOMENTUM */

	enemy->Delta.x =
	enemy->Delta.y =
	enemy->Delta.z = 0.0f;

	enemy->FrozenTimer = 4.0;			// set freeze timer

	return(true);			// stop weapon
}



/************ SQUOOSHY GOT PUNCHED *****************/
//
// returns true if want to disable punch after this
//

static Boolean SquooshyGotPunched(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta)
{
float	r;

#pragma unused (fistCoord, weapon, weaponDelta)

			/********************************/
			/* SEE IF PUNCHING FROZEN ALIEN */
			/********************************/
			
	if (enemy->FrozenTimer > 0.0f)
	{
		KillSquooshy(enemy);
	}
	
			/**********************************/
			/* NOT FROZEN, SO HANDLE NORMALLY */
			/**********************************/
	else
	{
		
				/* HURT IT */
				
		if (HurtSquooshy(enemy, .2))
			return(true);


				/* GIVE MOMENTUM */

		r = gPlayerInfo.objNode->Rot.y;
		enemy->Delta.x = -sin(r) * 500.0f;
		enemy->Delta.z = -cos(r) * 500.0f;
		enemy->Delta.y = 300.0f;
	}


	return(true);	
}

/************** SQUOOSHY GOT HIT BY SUPERNOVA  *****************/

static Boolean SquooshyHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon,	weaponCoord, weaponDelta)
	
	KillSquooshy(enemy);
	
	return(false);
}

/************** SQUOOSHY GOT HIT BY JUMP JET *****************/

static void SquooshyHitByJumpJet(ObjNode *enemy)
{
	KillSquooshy(enemy);
}




/*********************** HURT SQUOOSHY ***************************/

static Boolean HurtSquooshy(ObjNode *enemy, float damage)
{


				/* HURT ENEMY & SEE IF KILL */
				
	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		return (KillSquooshy(enemy));
	}
	else
	{
					/* GO INTO HIT ANIM */
				
		if (enemy->Skeleton->AnimNum != SQUOOSHY_ANIM_GETHIT)
			MorphToSkeletonAnim(enemy->Skeleton, SQUOOSHY_ANIM_GETHIT, 5);

		enemy->ButtTimer = 3.0f;
	}	
	
	return(false);
}


/****************** KILL SQUOOSHY *********************/
//
// OUTPUT: true if ObjNode was deleted
//

static Boolean KillSquooshy(ObjNode *enemy)
{
int	i;
			/*******************************/
			/* IF FROZEN THEN JUST SHATTER */
			/*******************************/
			
	if (enemy->FrozenTimer > 0.0f)
	{
		SpewAtoms(&enemy->Coord, 0,1, 2, false);
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
					
		if (enemy->Skeleton->AnimNum != SQUOOSHY_ANIM_DEATH)
		{
			MorphToSkeletonAnim(enemy->Skeleton, SQUOOSHY_ANIM_DEATH, 5);
			SpewAtoms(&enemy->Coord, 2,1, 0, false);
		}



				/* DISABLE THE HANDLERS */
				
		for (i = 0; i < NUM_WEAPON_TYPES; i++)
			enemy->HitByWeaponHandler[i] = nil;
		enemy->HitByJumpJetHandler 	= nil;
	}		
		
	return(true);
}


#pragma mark -

/********************* SEE IF SQUOOSHY SHOOT ****************************/

static void DoSquooshyShoot(ObjNode *enemy)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*newObj;
static const OGLPoint3D	muzzleOff = {0,-25,-60};
static const OGLVector3D muzzleAim = {0,-.2,-1};
OGLVector3D	aim;
OGLMatrix4x4	m;

			/* SEE IF TIME TO FIRE OFF A SHOT */
			
	enemy->ShotSpacing -= fps;
	if (enemy->ShotSpacing <= 0.0f)
	{
		enemy->ShotSpacing += .3f;						// reset spacing timer
			
			/* CREATE BLOBULE */

		FindJointFullMatrix(enemy, SQUOOSHY_JOINT_HEAD, &m);
		OGLPoint3D_Transform(&muzzleOff, &m, &gNewObjectDefinition.coord);	// calc start coord
		OGLVector3D_Transform(&muzzleAim, &m, &aim);						// calc delta/aim vector
			
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;	
		gNewObjectDefinition.type 		= FIREICE_ObjType_Blobule;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
		gNewObjectDefinition.slot 		= 502;
		gNewObjectDefinition.moveCall 	= MoveSquooshyBullet;	
		gNewObjectDefinition.rot 		= 0;	
		gNewObjectDefinition.scale 		= .9;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Damage 			= .1;

		newObj->CType 			= CTYPE_HURTME;
		newObj->CBits			= CBITS_TOUCHABLE;

		CreateCollisionBoxFromBoundingBox(newObj, 1, 1);
			
		newObj->Delta.x = aim.x * BULLET_SPEED;
		newObj->Delta.y = aim.y * BULLET_SPEED;
		newObj->Delta.z = aim.z * BULLET_SPEED;
			
		newObj->DeltaRot.x = RandomFloat2() * PI2;
		newObj->DeltaRot.y = RandomFloat2() * PI2;
		newObj->DeltaRot.z = RandomFloat2() * PI2;
			
		AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 4, 4, false);
		
		PlayEffect3D(EFFECT_SQUOOSHYSHOOT, &gNewObjectDefinition.coord);
		
	}
}


/******************* MOVE SQUOOSHY BULLET ************************/

static void MoveSquooshyBullet(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;


		/* SEE IF OUT OF RANGE */
		
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);
	

			/* MOVE IT */
	
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// do friction if landed
	{
		ApplyFrictionToDeltasXZ(2000.0,&gDelta);		
		ApplyFrictionToRotation(10.0,&theNode->DeltaRot);
	}
			
	gDelta.y -= 2000.0f * fps;								// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;
	
	theNode->Rot.x += theNode->DeltaRot.x * fps;
	theNode->Rot.y += theNode->DeltaRot.y * fps;
	theNode->Rot.z += theNode->DeltaRot.z * fps;
	
	
		/* HANDLE COLLISIONS */
			
	if (HandleCollisions(theNode, CTYPE_MISC|CTYPE_TERRAIN|CTYPE_FENCE, -.5))
	{
		MakeSplatter(&gCoord,FIREICE_ObjType_BlobDroplet);
		PlayEffect3D(EFFECT_SPLATHIT, &gCoord);
		DeleteObject(theNode);
		return;
	}
				
	UpdateObject(theNode);
}

















