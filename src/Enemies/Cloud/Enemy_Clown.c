/****************************/
/*   ENEMY: CLOWN.C			*/
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

static ObjNode *MakeEnemy_Clown(long x, long z);
static void MoveClown(ObjNode *theNode);
static void MoveClown_Standing(ObjNode *theNode);
static void  MoveClown_Walk(ObjNode *theNode);
static void MoveClownOnSpline(ObjNode *theNode);
static void UpdateClown(ObjNode *theNode);
static void  MoveClown_GotHit(ObjNode *theNode);
static void  MoveClown_Attack(ObjNode *theNode);

static Boolean ClownHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean ClownHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean ClownGotPunched(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta);
static void KillClown(ObjNode *enemy);

static void SeeIfClownAttack(ObjNode *theNode, float angleToPlayer, float distToPlayer);
static Boolean HurtClown(ObjNode *enemy, float damage);
static void ClownHitByJumpJet(ObjNode *enemy);

static void ClownBlowBubble(ObjNode *clown);
static void MoveClownBubble(ObjNode *theNode);
static void PopClownBubble(ObjNode *theNode);
static Boolean BubbleHitByDart(ObjNode *weapon, ObjNode *bubble, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_CLOWNS					7

#define	CLOWN_SCALE					2.2f

#define	CLOWN_DAMAGE				.33f
#define	CLOWN_HEALTH				1.0f

#define	CLOWN_CHASE_DIST_MAX		1500.0f
#define	CLOWN_CHASE_DIST_MIN		300.0f
#define	CLOWN_DETACH_DIST			(CLOWN_CHASE_DIST_MAX / 2.0f)
#define	CLOWN_ATTACK_DIST			700.0f

#define	CLOWN_TARGET_OFFSET			50.0f

#define CLOWN_TURN_SPEED			3.0f
#define CLOWN_WALK_SPEED			400.0f



		/* ANIMS */


enum
{
	CLOWN_ANIM_STAND,
	CLOWN_ANIM_WALK,
	CLOWN_ANIM_ATTACK,
	CLOWN_ANIM_GETHIT
};


		/* JOINTS */

enum
{
	CLOWN_JOINT_WAND	= 	16
};


#define	MAX_BUBBLE_SCALE	70.0f
#define	MAX_BUBBLE_SPEED	300.0f



/*********************/
/*    VARIABLES      */
/*********************/

#define	AttackDelay			SpecialF[0]
#define	StandDelay			SpecialF[1]
#define	ButtTimer			SpecialF[4]

#define	BlowBubble			Flag[0]


#define	BubbleWobbleX		SpecialF[0]
#define	BubbleWobbleY		SpecialF[1]
#define	InflatePitch		SpecialF[2]


/************************ ADD CLOWN ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Clown(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_CLOWN] >= MAX_CLOWNS)
			return(false);
	}

	newObj = MakeEnemy_Clown(x,z);
	newObj->TerrainItemPtr = itemPtr;
	newObj->EnemyRegenerate = itemPtr->parm[3] & (1<<1);

	return(true);
}

/****************** MAKE CLOWN ENEMY *************************/

static ObjNode *MakeEnemy_Clown(long x, long z)
{
ObjNode	*newObj;

				/*******************************/
				/* MAKE DEFAULT SKELETON ENEMY */
				/*******************************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_CLOWN,x,z, CLOWN_SCALE, 0, MoveClown);

	SetSkeletonAnim(newObj->Skeleton, CLOWN_ANIM_STAND);



				/*******************/
				/* SET BETTER INFO */
				/*******************/

	newObj->Health 		= CLOWN_HEALTH;
	newObj->Damage 		= CLOWN_DAMAGE;
	newObj->Kind 		= ENEMY_KIND_CLOWN;

	newObj->AttackDelay = RandomFloat() * 4.0f;


				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,CLOWN_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= ClownHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= ClownHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] 		= ClownGotPunched;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= ClownHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_DART] 		= ClownHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLARE] 		= ClownHitByWeapon;
	newObj->HitByJumpJetHandler 						= ClownHitByJumpJet;

	newObj->HurtCallback = HurtClown;							// set hurt callback function



				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 8,false);


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_CLOWN]++;

	return(newObj);
}



/********************* MOVE CLOWN **************************/

static void MoveClown(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveClown_Standing,
					MoveClown_Walk,
					MoveClown_Attack,
					MoveClown_GotHit,
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);

	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}



/********************** MOVE CLOWN: STANDING ******************************/

static void  MoveClown_Standing(ObjNode *theNode)
{
float	angleToTarget,dist;
float	fps = gFramesPerSecondFrac;

	theNode->BlowBubble = false;							// make sure this is reset

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

				/* TURN TOWARDS ME */

	angleToTarget = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, CLOWN_TURN_SPEED, true);
	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);


				/* SEE IF CHASE */


	theNode->StandDelay -= fps;								// see if in stand delay
	if (theNode->StandDelay <= 0.0f)
	{
		if (!IsBottomlessPitInFrontOfEnemy(theNode->Rot.y))
		{
			if ((dist < CLOWN_CHASE_DIST_MAX) && (dist > CLOWN_CHASE_DIST_MIN))
			{
				MorphToSkeletonAnim(theNode->Skeleton, CLOWN_ANIM_WALK, 4);
			}
		}
	}


			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;


				/* SEE IF DO ATTACK */

	SeeIfClownAttack(theNode, angleToTarget, dist);


	UpdateClown(theNode);
}




/********************** MOVE CLOWN: WALKING ******************************/

static void  MoveClown_Walk(ObjNode *theNode)
{
float		r,fps,angle,dist;

	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, CLOWN_TURN_SPEED, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * CLOWN_WALK_SPEED;
	gDelta.z = -cos(r) * CLOWN_WALK_SPEED;
	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	MoveEnemy(theNode);

				/****************/
				/* SEE IF STAND */
				/****************/

	dist = CalcQuickDistance(gPlayerInfo.coord.x, gPlayerInfo.coord.z, gCoord.x, gCoord.z);

		/* STOP WALKING IF BOTTOMLESS PIT IN FRONT OF US */

	if (IsBottomlessPitInFrontOfEnemy(r))
	{
		MorphToSkeletonAnim(theNode->Skeleton, CLOWN_ANIM_STAND, 4);
	}
	else
	{
		if ((dist >= CLOWN_CHASE_DIST_MAX) || (dist <= CLOWN_CHASE_DIST_MIN))
			MorphToSkeletonAnim(theNode->Skeleton, CLOWN_ANIM_STAND, 4);
	}

				/* UPDATE ANIM SPEED */

	if (theNode->Skeleton->AnimNum == CLOWN_ANIM_WALK)
		theNode->Skeleton->AnimSpeed = CLOWN_WALK_SPEED * .01f;


				/* SEE IF DO ATTACK */

	SeeIfClownAttack(theNode, angle, dist);

				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;

	UpdateClown(theNode);
}


/********************** MOVE CLOWN: ATTACK ******************************/

static void  MoveClown_Attack(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if on ground, add friction
		ApplyFrictionToDeltas(2000.0,&gDelta);

				/* TURN TOWARDS ME */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, CLOWN_TURN_SPEED, true);


			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*gFramesPerSecondFrac;				// add gravity
	MoveEnemy(theNode);


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, false))
		return;

	UpdateClown(theNode);


			/* SEE IF BLOW BUBBLE */

	if (theNode->BlowBubble)
	{
		ClownBlowBubble(theNode);
	}
}


/********************** MOVE CLOWN: GOT HIT ******************************/

static void  MoveClown_GotHit(ObjNode *theNode)
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
		MorphToSkeletonAnim(theNode->Skeleton, CLOWN_ANIM_STAND, 3);
	}


				/* DO ENEMY COLLISION */

	if (DoEnemyCollisionDetect(theNode,DEFAULT_ENEMY_COLLISION_CTYPES, true))
		return;


	UpdateClown(theNode);
}




//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME CLOWN ENEMY *************************/

Boolean PrimeEnemy_Clown(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/* MAKE DEFAULT SKELETON ENEMY */

	newObj = MakeEnemySkeleton(SKELETON_TYPE_CLOWN,x,z, CLOWN_SCALE, 0, nil);


	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;

	SetSkeletonAnim(newObj->Skeleton, CLOWN_ANIM_WALK);
	newObj->Skeleton->AnimSpeed = 2.0f;

				/* SET BETTER INFO */

	newObj->InitCoord 		= newObj->Coord;							// remember where started
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		-= newObj->BottomOff;
	newObj->SplineMoveCall 	= MoveClownOnSpline;					// set move call
	newObj->Health 			= CLOWN_HEALTH;
	newObj->Damage 			= CLOWN_DAMAGE;
	newObj->Kind 			= ENEMY_KIND_CLOWN;


				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,CLOWN_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= ClownHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= ClownHitBySuperNova;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] 		= ClownGotPunched;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= ClownHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_DART] 		= ClownHitByWeapon;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FLARE] 		= ClownHitByWeapon;
	newObj->HitByJumpJetHandler 						= ClownHitByJumpJet;

	newObj->HurtCallback = HurtClown;							// set hurt callback function


				/* MAKE SHADOW & GLOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 8, false);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE CLOWN ON SPLINE ***************************/

static void MoveClownOnSpline(ObjNode *theNode)
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

				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))					// just do this to see if explosions hurt
			return;


					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < CLOWN_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveClown);

	}
}


/***************** UPDATE CLOWN ************************/

static void UpdateClown(ObjNode *theNode)
{
	UpdateEnemy(theNode);
}


#pragma mark -

/************ CLOWN HIT BY STUN PULSE *****************/
//
// Called when weapon is moved and this handler is called after a collision.
//
// Also used for Flame weapon!
//

static Boolean ClownHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord)


			/* HURT IT */

	HurtClown(enemy, weapon->Damage);


			/* GIVE MOMENTUM */

	enemy->Delta.x += weaponDelta->x * .4f;
	enemy->Delta.y += weaponDelta->y * .4f  + 200.0f;
	enemy->Delta.z += weaponDelta->z * .4f;

	return(true);			// stop weapon
}




/************ CLOWN GOT PUNCHED *****************/
//
// returns true if want to disable punch after this
//

static Boolean ClownGotPunched(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta)
{
float	r;

#pragma unused (weapon, fistCoord, weaponDelta)

			/* HURT IT */

	HurtClown(enemy, .25);


			/* GIVE MOMENTUM */

	r = gPlayerInfo.objNode->Rot.y;

	enemy->Delta.x = -sin(r) * 1000.0f;
	enemy->Delta.z = -cos(r) * 1000.0f;
	enemy->Delta.y = 500.0f;

	return(true);
}


/************ CLOWN GOT HIT BY JUMP JET *****************/

static void ClownHitByJumpJet(ObjNode *enemy)
{
float	r;

		/* HURT IT */

	HurtClown(enemy, 1.0);


			/* GIVE MOMENTUM */

	r = gPlayerInfo.objNode->Rot.y;
	enemy->Delta.x = -sin(r) * 1000.0f;
	enemy->Delta.z = -cos(r) * 1000.0f;
	enemy->Delta.y = 500.0f;
}

/************** CLOWN GOT HIT BY SUPERNOVA *****************/

static Boolean ClownHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	KillClown(enemy);

	return(false);
}




/*********************** HURT CLOWN ***************************/

static Boolean HurtClown(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveClown);


			/* SEE IF POP BUBBLE BEING BLOWN */

	if (enemy->BlowBubble)
	{
		ObjNode	*bubble = enemy->ChainNode;

		if (bubble)
		{
			enemy->ChainNode = nil;
			PopClownBubble(bubble);
		}

		enemy->BlowBubble = false;
	}

				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillClown(enemy);
		return(true);
	}
	else
	{
					/* GO INTO HIT ANIM */

		if (enemy->Skeleton->AnimNum != CLOWN_ANIM_GETHIT)
			MorphToSkeletonAnim(enemy->Skeleton, CLOWN_ANIM_GETHIT, 8);

		enemy->ButtTimer = 1.0f;
	}
	return(false);
}


/****************** KILL CLOWN ***********************/

static void KillClown(ObjNode *enemy)
{

	float	x = enemy->Coord.x;
	float	y = enemy->Coord.y;
	float	z = enemy->Coord.z;

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

/************ SEE IF CLOWN ATTACK *********************/

static void SeeIfClownAttack(ObjNode *theNode, float angleToPlayer, float distToPlayer)
{
	if (!gPlayerHasLanded)
		return;

	if (gPlayerInfo.invincibilityTimer > 0.0f)
		return;

	theNode->AttackDelay -= gFramesPerSecondFrac;
	if (theNode->AttackDelay > 0.0f)
		return;

	if (distToPlayer > CLOWN_ATTACK_DIST)
		return;

	if (angleToPlayer > (PI/2))
		return;

			/* DON'T ATTACK THRU THINGS */

	if (SeeIfLineSegmentHitsAnything(&gCoord, &gPlayerInfo.coord, nil, CTYPE_FENCE|CTYPE_BLOCKRAYS))
		return;


	MorphToSkeletonAnim(theNode->Skeleton, CLOWN_ANIM_ATTACK, 5.0);

}


/************************ CLOWN BLOW BUBBLE **************************/

static void ClownBlowBubble(ObjNode *clown)
{
ObjNode	*bubble = clown->ChainNode;
static const OGLPoint3D	wandOff = {-6,-6,-26};
OGLPoint3D		wandPt;
OGLMatrix4x4	m;
static const OGLVector3D wandAim = {17, 1.563, 3.8};
OGLVector3D		v;
float			s;
Boolean			detach;


			/************************************/
			/* SEE IF NEED TO CREATE THE BUBBLE */
			/************************************/

	if (bubble == nil)
	{
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= CLOUD_ObjType_ClownBubble;
		gNewObjectDefinition.coord		= clown->Coord;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits  | STATUS_BIT_GLOW;
		gNewObjectDefinition.slot 		= SLOT_OF_DUMB - 1;
		gNewObjectDefinition.moveCall 	= nil;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= 1;
		bubble = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		clown->ChainNode = bubble;

		bubble->ColorFilter.a = .5f;
		bubble->InflatePitch = NORMAL_CHANNEL_RATE;
		bubble->Damage = .15;
		bubble->Health = 4.0;


	}

				/* SEE IF ABORT BUBBLE */

	if (clown->Skeleton->AnimNum != CLOWN_ANIM_ATTACK)
	{
		PopClownBubble(bubble);
		return;
	}



			/* INFLATE BUBBLE */

	s = bubble->Scale.x =
	bubble->Scale.y =
	bubble->Scale.z += 50.0f * gFramesPerSecondFrac;

	if (s > MAX_BUBBLE_SCALE)												// see if full size
	{
		bubble->Scale.x =
		bubble->Scale.y =
		bubble->Scale.z = MAX_BUBBLE_SCALE;
		detach = true;
	}
	else
		detach = false;



			/* INCREASE PITCH */

	if (bubble->EffectChannel == -1)
		bubble->EffectChannel = PlayEffect3D(EFFECT_INFLATE, &bubble->Coord);
	else
	{
		bubble->InflatePitch += (float)NORMAL_CHANNEL_RATE * (gFramesPerSecondFrac * .5f);
		ChangeChannelRate(bubble->EffectChannel, bubble->InflatePitch);
		Update3DSoundChannel(EFFECT_INFLATE, &bubble->EffectChannel, &bubble->Coord);
	}



				/* CALC PT WHERE BUBBLE SHOULD BE */

	FindJointFullMatrix(clown,CLOWN_JOINT_WAND,&m);				// calc matrix
	OGLPoint3D_Transform(&wandOff, &m, &wandPt);				// calc point of wand center
	OGLVector3D_Transform(&wandAim, &m, &v);					// calc vector away from wand


	bubble->Coord.x = wandPt.x + v.x * s;						// move bubble so tangent with wand
	bubble->Coord.y = wandPt.y + v.y * s;
	bubble->Coord.z = wandPt.z + v.z * s;

	UpdateObjectTransforms(bubble);


			/*****************/
			/* SEE IF DETACH */
			/*****************/

	if (detach)
	{
		StopAChannel(&bubble->EffectChannel);					// stop inflate effect


		clown->ChainNode = nil;									// detach bubble from clown
		MorphToSkeletonAnim(clown->Skeleton, CLOWN_ANIM_STAND, 2.0);
		bubble->MoveCall = MoveClownBubble;
		clown->AttackDelay = RandomFloat() * 4.0f;

		bubble->BubbleWobbleX = RandomFloat() * PI2;
		bubble->BubbleWobbleY = RandomFloat() * PI2;

		bubble->Delta.y = 100.0f;

		CreateCollisionBoxFromBoundingBox(bubble, 1,1);

		clown->StandDelay = 3.0f;								// clown won't move for 3 secs to give bubble time to clear out

		clown->BlowBubble = false;


					/* ACTIVATE COLLISION */

		bubble->HitByWeaponHandler[WEAPON_TYPE_DART] = BubbleHitByDart;		// bubble can now be popped by weapon
		bubble->CType = CTYPE_MISC;
		bubble->CBits = CBITS_TOUCHABLE;
		CreateCollisionBoxFromBoundingBox(bubble,.9,.9);
	}
}


/************************ MOVE CLOWN BUBBLE *****************************/

static void MoveClownBubble(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
OGLVector2D	v;

				/* SEE IF EXHAUSTED */

	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		PopClownBubble(theNode);
		return;
	}



	GetObjectInfo(theNode);


				/* UPDATE WOBBLE */

	theNode->BubbleWobbleX += fps * 6.0f;
	theNode->BubbleWobbleY += fps * 5.0f;

	theNode->Scale.x = MAX_BUBBLE_SCALE + sin(theNode->BubbleWobbleX) * (MAX_BUBBLE_SCALE * .1f);
	theNode->Scale.y = MAX_BUBBLE_SCALE + sin(theNode->BubbleWobbleY) * (MAX_BUBBLE_SCALE * .1f);

	theNode->Rot.y += fps * .5f;

	gDelta.y -= 50.0f * fps;										// gravity


				/* FLOAT TOWARD PLAYER */

	v.x = gPlayerInfo.coord.x - gCoord.x;							// calc vector to player
	v.y = gPlayerInfo.coord.z - gCoord.z;
	FastNormalizeVector2D(v.x,v.y, &v, false);

	gDelta.x += v.x * (200.0f * fps);								// accelerate toward player
	gDelta.z += v.y * (200.0f * fps);

	VectorLength2D(theNode->Speed2D, gDelta.x, gDelta.z);			// calc 2D speed value
	if (theNode->Speed2D > MAX_BUBBLE_SPEED)									// see if limit top speed
	{
		float	tweak = theNode->Speed2D / MAX_BUBBLE_SPEED;
		gDelta.x /= tweak;
		gDelta.z /= tweak;
		theNode->Speed2D = MAX_BUBBLE_SPEED;
	}

	gCoord.x += gDelta.x * fps;										// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	UpdateObject(theNode);


			/***********************/
			/* SEE IF HIT ANYTHING */
			/***********************/

	if (HandleCollisions(theNode, CTYPE_FENCE | CTYPE_TERRAIN | CTYPE_MISC | CTYPE_PLAYER, 0))
	{
		int		i;

				/* SEE IF HIT PLAYER */

		for (i = 0; i < gNumCollisions; i++)
		{
			if (gCollisionList[i].objectPtr ==  gPlayerInfo.objNode)
			{
				PlayerGotHit(theNode, 0);
			}
		}

		PopClownBubble(theNode);									// pop bubble
	}
}



/************************** POP CLOWN BUBBLE ******************************/

static void PopClownBubble(ObjNode *theNode)
{
	PlayEffect3D(EFFECT_CLOWNBUBBLEPOP, &theNode->Coord);	// play pop
	ExplodeGeometry(theNode, 200, SHARD_MODE_FROMORIGIN, 1, 3.0);
	DeleteObject(theNode);
}


/******************** BUBBLE HIT BY DART ***************************/

static Boolean BubbleHitByDart(ObjNode *weapon, ObjNode *bubble, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weaponDelta, weapon, weaponCoord)

	PopClownBubble(bubble);

	return(true);			// stop weapon
}









