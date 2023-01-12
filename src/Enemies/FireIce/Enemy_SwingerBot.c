/****************************/
/*   ENEMY: SWINGERBOT.C		*/
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

static void MoveSwingerBot(ObjNode *theNode);
static ObjNode *MakeSwingerBot(float x, float z);
static void AlignSwingerBotParts(ObjNode *body);
static void MoveSwingerBot_Chase(ObjNode *theNode);
static void MoveSwingerBot_Ram(ObjNode *theNode);
static void MoveSwingerBot_Wait(ObjNode *theNode);
static void DoSwingerBotCollisionDetect(ObjNode *theNode);
static void UpdateSwingerBot(ObjNode *theNode);
static void MoveSwingerBotOnSpline(ObjNode *theNode);
static Boolean SwingerBotHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void SwingerBotHitByJumpJet(ObjNode *enemy);
static Boolean SwingerBotHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean HurtSwingerBot(ObjNode *enemy, float damage);
static void KillSwingerBot(ObjNode *enemy);
static void SeeIfMaceHitsPlayer(ObjNode *body);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_SWINGERBOTS			6

#define	SWINGERBOT_SCALE		1.0f

#define	SWINGERBOT_CHASE_DIST	2000.0f
#define	SWINGERBOT_CHASE_SPEED	400.0f

#define	SWINGERBOT_DETACH_DIST	1100.0f

#define	SWINGERBOT_DAMAGE		.4f
#define	SWINGERBOT_HEALTH		1.5f

#define	SWINGERBOT_RAM_DIST		900.0f
#define	SWINGERBOT_RAM_SPEED	700.0f

#define SWINGERBOT_TURN_SPEED	2.0f
#define	RAM_DURATION			2.5f
#define	WAIT_DELAY				1.0f

#define	SWINGERBOT_TARGET_OFFSET	50.0f

enum
{
	SWINGERBOT_MODE_WAIT,
	SWINGERBOT_MODE_CHASE,
	SWINGERBOT_MODE_RAM
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	RamTimer	SpecialF[0]
#define	WaitDelay	SpecialF[1]


/************************* ADD SWINGERBOT *********************************/

Boolean AddSwingerBot(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*body;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_SWINGERBOT] >= MAX_SWINGERBOTS)
			return(false);
	}


	body = MakeSwingerBot(x,z);

	body->TerrainItemPtr = itemPtr;								// keep ptr to item list
	body->EnemyRegenerate = itemPtr->parm[3] & (1<<1);

	AlignSwingerBotParts(body);

	gNumEnemies++;
	gNumEnemyOfKind[body->Kind]++;
	return(true);													// item was added
}

/****************** MAKE SWINGERBOT ***********************/

static ObjNode *MakeSwingerBot(float x, float z)
{
ObjNode	*body,*pivot,*treads,*smallGear, *mace1, *mace2;
float	q;
				/******************/
				/* MAKE MAIN BODY */
				/******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FIREICE_ObjType_SwingerBot_Body;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 381;
	gNewObjectDefinition.moveCall 	= MoveSwingerBot;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= SWINGERBOT_SCALE;
	body = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	body->Mode 			= SWINGERBOT_MODE_WAIT;
	body->WaitDelay 	= 0;
	body->Damage 		= SWINGERBOT_DAMAGE;
	body->Health		= SWINGERBOT_HEALTH;

			/* SET COLLISION STUFF */

	body->CType 		= CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_ENEMY|CTYPE_AUTOTARGETWEAPON;
	body->CBits			= CBITS_ALLSOLID;
	q = body->BBox.max.x * SWINGERBOT_SCALE;
	SetObjectCollisionBounds(body, body->BBox.max.y * SWINGERBOT_SCALE, 0, -q,q,q,-q);

	CalcNewTargetOffsets(body,SWINGERBOT_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	body->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= SwingerBotHitByWeapon;
	body->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= SwingerBotHitBySuperNova;
	body->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= SwingerBotHitByWeapon;
	body->HitByWeaponHandler[WEAPON_TYPE_FLARE] 		= SwingerBotHitByWeapon;
	body->HitByJumpJetHandler 							= SwingerBotHitByJumpJet;

	body->HurtCallback = HurtSwingerBot;							// set hurt callback function


				/***************/
				/* MAKE TREADS */
				/***************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_SwingerBot_Treads;
	gNewObjectDefinition.slot		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	treads = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	body->ChainNode = treads;
	treads->StatusBits |= STATUS_BIT_UVTRANSFORM;					// treads do uv animation

			/*******************/
			/* MAKE SMALL GEAR */
			/*******************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_SwingerBot_SmallGear;
	smallGear = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	treads->ChainNode = smallGear;


			/**************/
			/* MAKE PIVOT */
			/**************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_SwingerBot_Pivot;
	pivot = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	smallGear->ChainNode = pivot;


			/**************/
			/* MAKE MACES */
			/**************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_SwingerBot_Mace;
	mace1 = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	pivot->ChainNode = mace1;

	mace2 = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	mace1->ChainNode = mace2;



	return(body);
}


/********************** MOVE SWINGERBOT ***********************/

static void MoveSwingerBot(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))
	{
		DeleteEnemy(theNode);
		return;
	}

		/* MOVE IT */

	GetObjectInfo(theNode);

	switch(theNode->Mode)
	{
				/* WAITING MODE */

		case	SWINGERBOT_MODE_WAIT:
				MoveSwingerBot_Wait(theNode);
				break;


				/* CHASE MODE */

		case	SWINGERBOT_MODE_CHASE:
				MoveSwingerBot_Chase(theNode);
				break;


				/* RAM MODE */

		case	SWINGERBOT_MODE_RAM:
				MoveSwingerBot_Ram(theNode);
				break;

	}


		/* COLLISION DETECT */

	DoSwingerBotCollisionDetect(theNode);


		/**********/
		/* UPDATE */
		/**********/

	UpdateSwingerBot(theNode);



	/*********************/
	/* SEE IF HIT PLAYER */
	/*********************/

	if (OGLPoint3D_Distance(&gCoord, &gPlayerInfo.coord) < (theNode->BBox.max.z * theNode->Scale.x + gPlayerInfo.objNode->BBox.max.x))
	{
		OGLVector2D	v1,v2;
		float		r,a;

				/* SEE IF PLAYER IN FRONT */

		r = theNode->Rot.y;							// calc tractor aim vec
		v1.x = -sin(r);
		v1.y = -cos(r);

		v2.x = gPlayerInfo.coord.x - gCoord.x;
		v2.y = gPlayerInfo.coord.z - gCoord.z;
		FastNormalizeVector2D(v2.x, v2.y, &v2, true);

		a = acos(OGLVector2D_Dot(&v1,&v2));			// calc angle between

		if (a < (PI/2))
		{
			PlayerGotHit(theNode, 0);
		}
	}
}


/******************** MOVE SWINGERBOT:  WAIT *****************************/

static void MoveSwingerBot_Wait(ObjNode *theNode)
{
float	speed,r,fps = gFramesPerSecondFrac;

			/* DECELERATE */

	theNode->Speed2D -= 1000.0f * fps;
	if (theNode->Speed2D < 0.0f)
		theNode->Speed2D = 0;
	speed = theNode->Speed2D;

			/* MOVE TOWARD PLAYER */

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * speed;
	gDelta.z = -cos(r) * speed;
	gDelta.y -= 2000.0f * fps;						// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y += gDelta.y * fps;


			/* SEE IF CHASE */

	theNode->WaitDelay -= fps;							// see if ready to chase
	if (theNode->WaitDelay <= 0.0f)
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < SWINGERBOT_CHASE_DIST)
		{
			theNode->Mode = SWINGERBOT_MODE_CHASE;
		}
	}
}



/******************** MOVE SWINGERBOT:  CHASE *****************************/

static void MoveSwingerBot_Chase(ObjNode *theNode)
{
float	speed,angle,r,fps = gFramesPerSecondFrac;

		/* ACCEL TO CHASE SPEED */

	theNode->Speed2D += SWINGERBOT_CHASE_SPEED * fps;
	if (theNode->Speed2D > SWINGERBOT_CHASE_SPEED)
		theNode->Speed2D = SWINGERBOT_CHASE_SPEED;
	speed = theNode->Speed2D;

			/* MOVE TOWARD PLAYER */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, SWINGERBOT_TURN_SPEED, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * speed;
	gDelta.z = -cos(r) * speed;
	gDelta.y -= 1000.0f * fps;						// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y += gDelta.y * fps;


			/* SEE IF READY TO RAM */

	if (angle < PI/2)
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < SWINGERBOT_RAM_DIST)
		{
			theNode->Mode = SWINGERBOT_MODE_RAM;
			theNode->RamTimer = RAM_DURATION;
		}
	}
}


/******************** MOVE SWINGERBOT:  RAM *****************************/

static void MoveSwingerBot_Ram(ObjNode *theNode)
{
float	speed,r,fps = gFramesPerSecondFrac;


		/* ACCEL TO CHASE SPEED */

	theNode->Speed2D += SWINGERBOT_RAM_SPEED * fps;
	if (theNode->Speed2D > SWINGERBOT_RAM_SPEED)
		theNode->Speed2D = SWINGERBOT_RAM_SPEED;
	speed = theNode->Speed2D;

			/* MOVE TOWARD PLAYER */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, SWINGERBOT_TURN_SPEED, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * speed;
	gDelta.z = -cos(r) * speed;
	gDelta.y -= 1000.0f * fps;						// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y += gDelta.y * fps;


			/* SEE IF DONE */

	theNode->RamTimer -= fps;
	if (theNode->RamTimer <= 0.0f)
	{
		theNode->Mode = SWINGERBOT_MODE_WAIT;
		theNode->WaitDelay = WAIT_DELAY;
	}
}




/********************* ALIGN SWINGERBOT PARTS ************************/

static void AlignSwingerBotParts(ObjNode *body)
{
ObjNode	*mace1,*mace2,*pivot,*smallGear,*treads;
float	fps = gFramesPerSecondFrac;
float	speed,spin;
OGLMatrix4x4	m;

				/* GET ALL THE PARTS */

	treads 		= body->ChainNode;
	smallGear 	= treads->ChainNode;
	pivot 		= smallGear->ChainNode;
	mace1		= pivot->ChainNode;
	mace2		= mace1->ChainNode;


	speed = body->Speed2D = CalcVectorLength(&body->Delta);			// calc 2D speed
	spin = .3f + speed * .002f;

			/*********************/
			/* UPDATE THE TREADS */
			/*********************/

	treads->BaseTransformMatrix = body->BaseTransformMatrix;		// align perfectly w/ body
	SetObjectTransformMatrix(treads);
	treads->Coord = body->Coord;

	treads->TextureTransformV += speed * .002f * fps;


			/*************************/
			/* UPDATE THE SMALL GEAR */
			/*************************/

	smallGear->Rot.y += (fps * spin) * 10.0f;						// spin the gear

	OGLMatrix4x4_SetRotate_Y(&m, smallGear->Rot.y);					// set rotation matrix
	m.value[M03] = 0;												// insert translation
	m.value[M13] = 65.0f;
	m.value[M23] = -77;
	OGLMatrix4x4_Multiply(&m, &body->BaseTransformMatrix, &smallGear->BaseTransformMatrix);
	SetObjectTransformMatrix(smallGear);

	smallGear->Coord = body->Coord;


			/********************/
			/* UPDATE THE PIVOT */
			/********************/

	pivot->Rot.y -= (fps * spin) * 6.0f;							// spin the gear

	OGLMatrix4x4_SetRotate_Y(&m, pivot->Rot.y);						// set rotation matrix
	OGLMatrix4x4_Multiply(&m, &body->BaseTransformMatrix, &pivot->BaseTransformMatrix);
	SetObjectTransformMatrix(pivot);

	pivot->Coord = body->Coord;



			/****************/
			/* UPDATE MACES */
			/****************/

	spin *= .5f;							// tweak this for rotation

			/* MACE 1 */

	mace1->Rot.z = -spin;
	if (mace1->Rot.z < (-PI/2))
		mace1->Rot.z = -PI/2;

	OGLMatrix4x4_SetRotate_Z(&m, mace1->Rot.z);					// set rotation matrix
	m.value[M03] = -140.0f;										// insert translation
	m.value[M13] = 206.0;
	m.value[M23] = 0;
	OGLMatrix4x4_Multiply(&m, &pivot->BaseTransformMatrix, &mace1->BaseTransformMatrix);
	SetObjectTransformMatrix(mace1);
	mace1->Coord = body->Coord;

			/* MACE 2 */

	mace2->Rot.z = spin;
	if (mace2->Rot.z > (PI/2))
		mace2->Rot.z = PI/2;

	OGLMatrix4x4_SetRotate_Z(&m, mace2->Rot.z);					// set rotation matrix
	m.value[M03] = 140.0f;										// insert translation
	m.value[M13] = 206.0;
	m.value[M23] = 0;
	OGLMatrix4x4_Multiply(&m, &pivot->BaseTransformMatrix, &mace2->BaseTransformMatrix);
	SetObjectTransformMatrix(mace2);
	mace2->Coord = body->Coord;

}


/***************** DO SWINGERBOT COLLISION DETECT **************************/

static void DoSwingerBotCollisionDetect(ObjNode *theNode)
{
			/* HANDLE THE BASIC STUFF */

	HandleCollisions(theNode, CTYPE_TERRAIN|CTYPE_MISC|CTYPE_TRIGGER2, 0);


				/* CHECK FENCE COLLISION */

	DoFenceCollision(theNode);

}


/******************** UPDATE SWINGERBOT *****************/

static void UpdateSwingerBot(ObjNode *theNode)
{
	UpdateObject(theNode);
	AlignSwingerBotParts(theNode);

	SeeIfMaceHitsPlayer(theNode);


			/* UPDATE THE SOUND */

	if (theNode->EffectChannel == -1)
		theNode->EffectChannel = PlayEffect3D(EFFECT_SWINGERDRONE, &theNode->Coord);
	else
		Update3DSoundChannel(EFFECT_SWINGERDRONE, &theNode->EffectChannel, &theNode->Coord);
}


//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME SWINGERBOT ENEMY *************************/

Boolean PrimeEnemy_SwingerBot(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/***************/
				/* MAKE OBJECT */
				/***************/

	newObj = MakeSwingerBot(x,z);

	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;


				/* SET BETTER INFO */

	newObj->InitCoord 		= newObj->Coord;					// remember where started
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		-= newObj->BottomOff;
	newObj->MoveCall		= nil;
	newObj->SplineMoveCall 	= MoveSwingerBotOnSpline;				// set move call
	newObj->Health 			= 1.0;
	newObj->Damage 			= SWINGERBOT_DAMAGE;
	newObj->Kind 			= ENEMY_KIND_SWINGERBOT;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE SWINGERBOT ON SPLINE ***************************/

static void MoveSwingerBotOnSpline(ObjNode *theNode)
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

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BottomOff;	// calc y coord
		UpdateObjectTransforms(theNode);											// update transforms
		UpdateShadow(theNode);

				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))					// just do this to see if explosions hurt
			return;


					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		dist = OGLPoint3D_Distance(&gPlayerInfo.coord, &theNode->Coord);
		if (dist < SWINGERBOT_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveSwingerBot);

					/* KEEP ALL THE PARTS ALIGNED */

		AlignSwingerBotParts(theNode);

	}
}


#pragma mark -

/************ SWINGERBOT HIT BY STUN PULSE *****************/
//
// Called when weapon is moved and this handler is called after a collision.
//
// Also used for Flame weapon!
//

static Boolean SwingerBotHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weaponCoord)


			/* HURT IT */

	HurtSwingerBot(enemy, weapon->Damage);


			/* GIVE MOMENTUM */

	enemy->Delta.x += weaponDelta->x * .4f;
	enemy->Delta.y += weaponDelta->y * .4f  + 200.0f;
	enemy->Delta.z += weaponDelta->z * .4f;

	return(true);			// stop weapon
}





/************ SWINGERBOT GOT HIT BY JUMP JET *****************/

static void SwingerBotHitByJumpJet(ObjNode *enemy)
{
float	r;

		/* HURT IT */

	HurtSwingerBot(enemy, 1.0);


			/* GIVE MOMENTUM */

	r = gPlayerInfo.objNode->Rot.y;
	enemy->Delta.x = -sin(r) * 1000.0f;
	enemy->Delta.z = -cos(r) * 1000.0f;
	enemy->Delta.y = 500.0f;
}

/************** SWINGERBOT GOT HIT BY SUPERNOVA *****************/

static Boolean SwingerBotHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	KillSwingerBot(enemy);

	return(false);
}




/*********************** HURT SWINGERBOT ***************************/

static Boolean HurtSwingerBot(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveSwingerBot);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillSwingerBot(enemy);
		return(true);
	}
	else
	{

	}

	return(false);
}


/****************** KILL SWINGERBOT ***********************/

static void KillSwingerBot(ObjNode *enemy)
{
int		i;
ObjNode	*newObj;
static const OGLPoint3D	treadOffs[2] =
{
	{-92,0,-30},		// left wheel
	{92,0,30},			// right wheel
};
OGLPoint3D	pts[2];

static const OGLPoint3D	gearOffs[2] =
{
	{0, 74, 0},			// big gear
	{0, 74, -78},		// small gear
};

static const OGLPoint3D	maceOffs[2] =
{
	{-140, 110, 0},
	{140, 110, 0},
};


			/*******************/
			/* CREATE 2 TREADS */
			/*******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;						// set basic info
	gNewObjectDefinition.type 		= FIREICE_ObjType_SwingerBot_Chunk_Tread;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveEnemyRobotChunk;
	gNewObjectDefinition.rot 		= enemy->Rot.y;
	gNewObjectDefinition.scale 		= SWINGERBOT_SCALE;

	OGLPoint3D_TransformArray(treadOffs, &enemy->BaseTransformMatrix, pts, 2);		// calc coords of treads

	for (i = 0; i < 2; i++)																// make the treads objects
	{
		gNewObjectDefinition.coord	 	= pts[i];
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Delta.x = RandomFloat2() * 600.0f;
		newObj->Delta.z = RandomFloat2() * 600.0f;
		newObj->Delta.y = 1600.0f;

		newObj->DeltaRot.x = RandomFloat2() * 9.0f;
		newObj->DeltaRot.z = RandomFloat2() * 9.0f;
	}


			/****************/
			/* CREATE GEARS */
			/****************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_SwingerBot_Chunk_BigGear;
	OGLPoint3D_TransformArray(gearOffs, &enemy->BaseTransformMatrix, pts, 2);		// calc coords of gears

	for (i = 0; i < 2; i++)															// make the objects
	{
		gNewObjectDefinition.coord	 	= pts[i];
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Delta.x = RandomFloat2() * 600.0f;
		newObj->Delta.z = RandomFloat2() * 600.0f;
		newObj->Delta.y = 1900.0f;

		newObj->DeltaRot.x = RandomFloat2() * 9.0f;
		newObj->DeltaRot.z = RandomFloat2() * 9.0f;

		gNewObjectDefinition.type++;
	}

			/****************/
			/* CREATE MACES */
			/****************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_SwingerBot_Chunk_BigGear;
	OGLPoint3D_TransformArray(maceOffs, &enemy->BaseTransformMatrix, pts, 2);		// calc coords of gears

	for (i = 0; i < 2; i++)															// make the objects
	{
		gNewObjectDefinition.coord	 	= pts[i];
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Delta.x = RandomFloat2() * 800.0f;
		newObj->Delta.z = RandomFloat2() * 800.0f;
		newObj->Delta.y = 1300.0f;

		newObj->DeltaRot.x = RandomFloat2() * 6.0f;
		newObj->DeltaRot.z = RandomFloat2() * 6.0f;
	}


			/****************/
			/* CREATE PIVOT */
			/****************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_SwingerBot_Chunk_Pivot;
	gNewObjectDefinition.coord	 	= enemy->Coord;
	gNewObjectDefinition.coord.y 	+= 100.0f;

	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->Delta.x = RandomFloat2() * 800.0f;
	newObj->Delta.z = RandomFloat2() * 800.0f;
	newObj->Delta.y = 2000.0f;

	newObj->DeltaRot.x = RandomFloat2() * 6.0f;
	newObj->DeltaRot.z = RandomFloat2() * 6.0f;


			/****************************/
			/* EXPLODE BODY INTO SHARDS */
			/****************************/

	ExplodeGeometry(enemy, 400, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .4);

	MakeSparkExplosion(enemy->Coord.x, enemy->Coord.y, enemy->Coord.z, 250, 2.0, PARTICLE_SObjType_GreenSpark,0);
	MakeSparkExplosion(enemy->Coord.x, enemy->Coord.y, enemy->Coord.z, 180, 2.2, PARTICLE_SObjType_WhiteSpark4,0);

	PlayEffect3D(EFFECT_ROBOTEXPLODE, &enemy->Coord);

			/* ATOMS */

	SpewAtoms(&enemy->Coord, 1,1,1, false);


			/* DELETE THE OLD STUFF */

	if (!enemy->EnemyRegenerate)
		enemy->TerrainItemPtr = nil;							// dont ever come back

	DeleteEnemy(enemy);
}


/*********************** SEE IF MACE HITS PLAYER *****************************/

static void SeeIfMaceHitsPlayer(ObjNode *body)
{
ObjNode			*mace[2],*pivot,*smallGear,*treads,*player;
static const OGLPoint3D off = {0,-118,0};
OGLPoint3D		ballPt;
OGLVector2D		v;
OGLMatrix3x3	m;
float			throwFactor;
int				i;

				/* GET THE MACES */

	treads 		= body->ChainNode;
	smallGear 	= treads->ChainNode;
	pivot 		= smallGear->ChainNode;
	mace[0]		= pivot->ChainNode;
	mace[1]		= mace[0]->ChainNode;


		/************************************/
		/* CHECK BOTH MACES FOR A COLLISION */
		/************************************/

	for (i = 0; i < 2; i++)
	{

				/* CALC BALL HOT-SPOT */

		OGLPoint3D_Transform(&off, &mace[i]->BaseTransformMatrix, &ballPt);


				/* SEE IF HIT PLAYER */

		if (DoSimpleBoxCollisionAgainstPlayer(ballPt.y+40.0f, ballPt.y-40.0f, ballPt.x-40.0f, ballPt.x+40.0f, ballPt.z+40.0f, ballPt.z-40.0f))
		{
			player = gPlayerInfo.objNode;											// get player objNode

			player->Rot.y = CalcYAngleFromPointToPoint(player->Rot.y,body->Coord.x,body->Coord.z,player->Coord.x, player->Coord.z);		// aim player direction of blast

			Boolean didHurt = PlayerGotHit(nil, SWINGERBOT_DAMAGE);					// hurt player
			SetSkeletonAnim(player->Skeleton, PLAYER_ANIM_THROWN);

					/* CALC VECTOR TO THROW PLAYER */

			v.x = player->Coord.x - body->Coord.x;
			v.y = player->Coord.z - body->Coord.z;
			FastNormalizeVector2D(v.x, v.y, &v, true);

			OGLMatrix3x3_SetRotate(&m, PI/3);										// rot 90 degrees to give lateral-side-swipe motion
			OGLVector2D_Transform(&v, &m, &v);

			throwFactor = (mace[i]->Speed2D + 400.0f) * 5.0f;

			player->Delta.x = v.x * throwFactor;
			player->Delta.z = v.y * throwFactor;
			player->Delta.y = 700.0f;

			gCurrentMaxSpeed = CalcVectorLength(&player->Delta);						// set this so we can go flyin'

					/* GOODIES */

			if (didHurt)															// only do feedback if actually inflicted damage
			{
				PlayEffect_Parms3D(EFFECT_THROWNSWOOSH, &player->Coord, NORMAL_CHANNEL_RATE, 2.0);
				PlayEffect_Parms3D(EFFECT_METALHIT2, &ballPt, NORMAL_CHANNEL_RATE * 2/3, 1.0);	// make clank sound
				MakeSparkExplosion(ballPt.x, ballPt.y, ballPt.z, 300.0f, .5, PARTICLE_SObjType_WhiteSpark3,0);
			}

			break;
		}
	}

}

