/****************************/
/*   ENEMY: JAWSBOT.C		*/
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

static void MoveJawsBot(ObjNode *theNode);
static ObjNode *MakeJawsBot(float x, float z);
static void AlignJawsBotParts(ObjNode *theNode);
static void MoveJawsBot_Chase(ObjNode *theNode);
static void MoveJawsBot_Wait(ObjNode *theNode);
static void DoJawsBotCollisionDetect(ObjNode *theNode);
static void UpdateJawsBot(ObjNode *theNode);
static void MoveJawsBotOnSpline(ObjNode *theNode);
static Boolean JawsBotHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void JawsBotHitByJumpJet(ObjNode *enemy);
static Boolean JawsBotHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean HurtJawsBot(ObjNode *enemy, float damage);
static void KillJawsBot(ObjNode *enemy);
static void SeeIfJawsChompPlayer(ObjNode *body);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_JAWSBOTS		6

#define	JAWSBOT_SCALE		15.0f

#define	JAWSBOT_CHASE_DIST	2000.0f
#define	JAWSBOT_CHASE_SPEED	350.0f

#define	JAWSBOT_DETACH_DIST	1000.0f

#define	JAWSBOT_DAMAGE		.2
#define	JAWSBOT_HEALTH		1.0f

#define JAWSBOT_TURN_SPEED	2.0f

#define	JAWSBOT_TARGET_OFFSET	60.0f

enum
{
	JAWSBOT_MODE_WAIT,
	JAWSBOT_MODE_CHASE
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	ChompIndex	SpecialF[2]


/************************* ADD JAWSBOT *********************************/

Boolean AddJawsBot(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*body;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_JAWSBOT] >= MAX_JAWSBOTS)
			return(false);
	}


	body = MakeJawsBot(x,z);

	body->TerrainItemPtr = itemPtr;								// keep ptr to item list
	body->EnemyRegenerate = itemPtr->parm[3] & (1<<1);

	AlignJawsBotParts(body);

	gNumEnemies++;
	gNumEnemyOfKind[body->Kind]++;
	return(true);													// item was added
}

/****************** MAKE JAWSBOT ***********************/

static ObjNode *MakeJawsBot(float x, float z)
{
ObjNode	*body,*jaw,*wheels,*prev;
int	i;
float	q;
				/******************/
				/* MAKE MAIN BODY */
				/******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FIREICE_ObjType_JawsBot_Body;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 381;
	gNewObjectDefinition.moveCall 	= MoveJawsBot;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= JAWSBOT_SCALE;
	body = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	body->Mode 			= JAWSBOT_MODE_WAIT;
	body->Damage 		= JAWSBOT_DAMAGE;

	body->ChompIndex 	= RandomFloat()*PI2;


			/* SET COLLISION STUFF */

	body->CType 		= CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_ENEMY|CTYPE_AUTOTARGETWEAPON;
	body->CBits			= CBITS_ALLSOLID;
	q = body->BBox.max.x * JAWSBOT_SCALE;
	SetObjectCollisionBounds(body, 10.0f * JAWSBOT_SCALE, 0, -q,q,q,-q);

	CalcNewTargetOffsets(body,JAWSBOT_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	body->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= JawsBotHitByWeapon;
	body->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= JawsBotHitBySuperNova;
	body->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= JawsBotHitByWeapon;
	body->HitByWeaponHandler[WEAPON_TYPE_DART] 			= JawsBotHitByWeapon;
	body->HitByWeaponHandler[WEAPON_TYPE_FLARE] 		= JawsBotHitByWeapon;
	body->HitByJumpJetHandler 							= JawsBotHitByJumpJet;

	body->HurtCallback = HurtJawsBot;							// set hurt callback function



				/************/
				/* MAKE JAW */
				/*************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_JawsBot_Jaw;
	gNewObjectDefinition.slot		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	jaw = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	body->ChainNode = jaw;


			/***************/
			/* MAKE WHEELS */
			/***************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_JawsBot_Wheels;
	gNewObjectDefinition.moveCall 	= nil;

	prev = jaw;
	for (i = 0; i < 3; i++)
	{
		wheels = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		prev->ChainNode = wheels;
		prev = wheels;
	}


	return(body);
}


/********************** MOVE JAWSBOT ***********************/

static void MoveJawsBot(ObjNode *theNode)
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

		case	JAWSBOT_MODE_WAIT:
				MoveJawsBot_Wait(theNode);
				break;


				/* CHASE MODE */

		case	JAWSBOT_MODE_CHASE:
				MoveJawsBot_Chase(theNode);
				break;

	}


		/* COLLISION DETECT */

	DoJawsBotCollisionDetect(theNode);


		/**********/
		/* UPDATE */
		/**********/

	UpdateJawsBot(theNode);



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


/******************** MOVE JAWSBOT:  WAIT *****************************/

static void MoveJawsBot_Wait(ObjNode *theNode)
{
float	speed,r,fps = gFramesPerSecondFrac;



			/* DECELERATE */

	theNode->Speed2D -= 2300.0f * fps;
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

	if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < JAWSBOT_CHASE_DIST)
		theNode->Mode = JAWSBOT_MODE_CHASE;
}



/******************** MOVE JAWSBOT:  CHASE *****************************/

static void MoveJawsBot_Chase(ObjNode *theNode)
{
float	speed,r;
float	fps = gFramesPerSecondFrac;

		/* ACCEL TO CHASE SPEED */

	theNode->Speed2D += JAWSBOT_CHASE_SPEED * fps;
	if (theNode->Speed2D > JAWSBOT_CHASE_SPEED)
		theNode->Speed2D = JAWSBOT_CHASE_SPEED;
	speed = theNode->Speed2D;

			/* MOVE TOWARD PLAYER */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, JAWSBOT_TURN_SPEED, false);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * speed;
	gDelta.z = -cos(r) * speed;
	gDelta.y -= 2000.0f * fps;						// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y += gDelta.y * fps;
}



/********************* ALIGN JAWSBOT PARTS ************************/

static void AlignJawsBotParts(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	speed,oldCI;
ObjNode	*jaw,*wheel;
OGLMatrix4x4	m;
int				i;
static const OGLPoint3D wheelOffs[3] =
{
	{0,3,-6},
	{0,3,0},
	{0,3,6},
};
static const OGLPoint3D jawOff = {0,4.4,-10.0};

	speed = theNode->Speed2D = CalcVectorLength(&theNode->Delta);			// calc 2D speed


			/******************/
			/* UPDATE THE JAW */
			/******************/

	jaw = theNode->ChainNode;

	oldCI = jaw->ChompIndex;

	jaw->ChompIndex += (150.0f + speed) * fps * .04f;				// udpate the jaw chomping while we're here
	if (jaw->ChompIndex > PI2)
		jaw->ChompIndex -= PI2;
	jaw->Rot.x = sin(jaw->ChompIndex) * -.6f;


	OGLMatrix4x4_SetRotate_X(&m, jaw->Rot.x);						// set rotation matrix
	m.value[M03] = jawOff.x;										// insert translation
	m.value[M13] = jawOff.y;
	m.value[M23] = jawOff.z;
	OGLMatrix4x4_Multiply(&m, &theNode->BaseTransformMatrix, &jaw->BaseTransformMatrix);
	SetObjectTransformMatrix(jaw);

	jaw->Coord = theNode->Coord;

	if (jaw->ChompIndex < oldCI)							// if looped back, then chomp sound
		PlayEffect_Parms3D(EFFECT_METALHIT, &gCoord, NORMAL_CHANNEL_RATE * 3/2, .6);	// make clank sound


			/*****************/
			/* UPDATE WHEELS */
			/*****************/

	wheel = jaw->ChainNode;											// get wheel 1st obj

	i = 0;
	while(wheel != nil)
	{
		wheel->Rot.x -= speed * .02f * fps;							// spin the wheel while we're here

		OGLMatrix4x4_SetRotate_X(&m, wheel->Rot.x);					// set rotation matrix
		m.value[M03] = wheelOffs[i].x;								// insert translation
		m.value[M13] = wheelOffs[i].y;
		m.value[M23] = wheelOffs[i].z;
		OGLMatrix4x4_Multiply(&m, &theNode->BaseTransformMatrix, &wheel->BaseTransformMatrix);
		SetObjectTransformMatrix(wheel);

		wheel->Coord = theNode->Coord;

		wheel = wheel->ChainNode;									// next wheel
		i++;
	}

}


/***************** DO JAWSBOT COLLISION DETECT **************************/

static void DoJawsBotCollisionDetect(ObjNode *theNode)
{
			/* HANDLE THE BASIC STUFF */

	HandleCollisions(theNode, CTYPE_TERRAIN|CTYPE_MISC|CTYPE_TRIGGER2|CTYPE_PLAYER, 0);


				/* CHECK FENCE COLLISION */

	DoFenceCollision(theNode);

}


/******************** UPDATE JAWSBOT *****************/

static void UpdateJawsBot(ObjNode *theNode)
{
	UpdateObject(theNode);
	AlignJawsBotParts(theNode);

	SeeIfJawsChompPlayer(theNode);
}


//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME JAWSBOT ENEMY *************************/

Boolean PrimeEnemy_JawsBot(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/***************/
				/* MAKE OBJECT */
				/***************/

	newObj = MakeJawsBot(x,z);

	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;


				/* SET BETTER INFO */

	newObj->InitCoord 		= newObj->Coord;					// remember where started
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		-= newObj->BottomOff;
	newObj->MoveCall		= nil;
	newObj->SplineMoveCall 	= MoveJawsBotOnSpline;				// set move call
	newObj->Health 			= JAWSBOT_HEALTH;
	newObj->Damage 			= JAWSBOT_DAMAGE;
	newObj->Kind 			= ENEMY_KIND_JAWSBOT;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE JAWSBOT ON SPLINE ***************************/

static void MoveJawsBotOnSpline(ObjNode *theNode)
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
		if (dist < JAWSBOT_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveJawsBot);

					/* KEEP ALL THE PARTS ALIGNED */

		AlignJawsBotParts(theNode);

	}
}


#pragma mark -

/************ JAWSBOT HIT BY STUN PULSE *****************/
//
// Called when weapon is moved and this handler is called after a collision.
//
// Also used for Flame weapon!
//

static Boolean JawsBotHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weaponCoord)


			/* HURT IT */

	HurtJawsBot(enemy, weapon->Damage);


			/* GIVE MOMENTUM */

	enemy->Delta.x += weaponDelta->x * .4f;
	enemy->Delta.y += weaponDelta->y * .4f  + 200.0f;
	enemy->Delta.z += weaponDelta->z * .4f;

	return(true);			// stop weapon
}




/************ JAWSBOT GOT HIT BY JUMP JET *****************/

static void JawsBotHitByJumpJet(ObjNode *enemy)
{
float	r;

		/* HURT IT */

	HurtJawsBot(enemy, 1.0);


			/* GIVE MOMENTUM */

	r = gPlayerInfo.objNode->Rot.y;
	enemy->Delta.x = -sin(r) * 1000.0f;
	enemy->Delta.z = -cos(r) * 1000.0f;
	enemy->Delta.y = 500.0f;
}

/************** JAWSBOT GOT HIT BY SUPERNOVA *****************/

static Boolean JawsBotHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	KillJawsBot(enemy);

	return(false);
}




/*********************** HURT JAWSBOT ***************************/

static Boolean HurtJawsBot(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveJawsBot);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillJawsBot(enemy);
		return(true);
	}
	else
	{

	}

	return(false);
}


/****************** KILL JAWSBOT ***********************/

static void KillJawsBot(ObjNode *enemy)
{
int		i;
ObjNode	*newObj;
static const OGLPoint3D	wheelOff[4] =
{
	{-10.8f,	0.0f,	-8.3f},			// front left wheel
	{10.8f,		0.0f,	-8.3f},			// front right wheel
	{-10.8f,	0.0f,	8.3f},			// back left wheel
	{10.0f,		0.0f,	8.3f},			// back right wheel
};
OGLPoint3D	wheelPts[4];

static const OGLPoint3D	jawOffs[2] =
{
	{0.0f, 12.0f, -17.1f},
	{0.0f, 4.2f, 19.0f},
};


			/*******************/
			/* CREATE 4 WHEELS */
			/*******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;						// set basic info
	gNewObjectDefinition.type 		= FIREICE_ObjType_JawsBot_Chunk_Wheel;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveEnemyRobotChunk;
	gNewObjectDefinition.rot 		= enemy->Rot.y;
	gNewObjectDefinition.scale 		= JAWSBOT_SCALE;

	OGLPoint3D_TransformArray(wheelOff, &enemy->BaseTransformMatrix, wheelPts, 4);		// calc coords of wheels

	for (i = 0; i < 4; i++)																// make the 4 wheel objects
	{
		gNewObjectDefinition.coord	 	= wheelPts[i];
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Delta.x = RandomFloat2() * 600.0f;
		newObj->Delta.z = RandomFloat2() * 600.0f;
		newObj->Delta.y = 1300.0f;

		newObj->DeltaRot.x = RandomFloat2() * 9.0f;
		newObj->DeltaRot.z = RandomFloat2() * 9.0f;
	}


			/****************/
			/* CREATE JAWS  */
			/****************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_JawsBot_Chunk_Jaw;
	OGLPoint3D_TransformArray(jawOffs, &enemy->BaseTransformMatrix, wheelPts, 2);		// calc coords of jaws

	for (i = 0; i < 2; i++)																// make the 4 wheel objects
	{
		gNewObjectDefinition.coord	 	= wheelPts[i];
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Delta.x = RandomFloat2() * 600.0f;
		newObj->Delta.z = RandomFloat2() * 600.0f;
		newObj->Delta.y = 1300.0f;

		newObj->DeltaRot.x = RandomFloat2() * 9.0f;
		newObj->DeltaRot.z = RandomFloat2() * 9.0f;

		gNewObjectDefinition.type++;
	}




			/****************************/
			/* EXPLODE BODY INTO SHARDS */
			/****************************/

	ExplodeGeometry(enemy, 400, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .4);

	MakeSparkExplosion(enemy->Coord.x, enemy->Coord.y, enemy->Coord.z, 250, 2.0, PARTICLE_SObjType_BlueSpark,0);
	MakeSparkExplosion(enemy->Coord.x, enemy->Coord.y, enemy->Coord.z, 180, 2.2, PARTICLE_SObjType_WhiteSpark4,0);

	PlayEffect3D(EFFECT_ROBOTEXPLODE, &enemy->Coord);

			/* ATOMS */

	SpewAtoms(&enemy->Coord, 1,1,1, false);


			/* DELETE THE OLD STUFF */

	if (!enemy->EnemyRegenerate)
		enemy->TerrainItemPtr = nil;							// dont ever come back

	DeleteEnemy(enemy);
}


/*********************** SEE IF JAWS CHOMP PLAYER *****************************/

static void SeeIfJawsChompPlayer(ObjNode *body)
{
ObjNode			*player;
static const OGLPoint3D off = {0,5,-18};
OGLPoint3D		jawPt;
OGLVector2D		v;
float			throwFactor;

	if (gPlayerInfo.invincibilityTimer > 0.0f)							// cant get hit if invincible
		return;

				/* CALC JAW HOT-SPOT */

	OGLPoint3D_Transform(&off, &body->BaseTransformMatrix, &jawPt);


			/* SEE IF HIT PLAYER */

	if (DoSimpleBoxCollisionAgainstPlayer(jawPt.y+70.0f, jawPt.y-70.0f, jawPt.x-70.0f, jawPt.x+70.0f, jawPt.z+70.0f, jawPt.z-70.0f))
	{
		player = gPlayerInfo.objNode;											// get player objNode

		PlayerGotHit(nil, JAWSBOT_DAMAGE);										// hurt player
		SetSkeletonAnim(player->Skeleton, PLAYER_ANIM_THROWN);
		PlayEffect_Parms3D(EFFECT_THROWNSWOOSH, &player->Coord, NORMAL_CHANNEL_RATE, 2.0);
		player->StatusBits &= ~STATUS_BIT_ONGROUND;

				/* CALC VECTOR TO THROW PLAYER */

		v.x = player->Coord.x - jawPt.x;
		v.y = player->Coord.z - jawPt.z;
		FastNormalizeVector2D(v.x, v.y, &v, true);

		throwFactor = (body->Speed2D + 400.0f) * 1.5f;

		player->Delta.x = v.x * throwFactor;
		player->Delta.z = v.y * throwFactor;

		player->Delta.y = 1800.0f;												// throw up high

		gCurrentMaxSpeed = CalcVectorLength(&player->Delta);					// set this so we can go flyin'


				/* GOODIES */

		PlayEffect_Parms3D(EFFECT_METALHIT2, &jawPt, NORMAL_CHANNEL_RATE, 1.0);	// make clank sound
		MakeSparkExplosion(jawPt.x, jawPt.y, jawPt.z, 300.0f, .8, PARTICLE_SObjType_WhiteSpark3, 0);
	}
}











