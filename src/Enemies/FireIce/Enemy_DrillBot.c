/****************************/
/*   ENEMY: DRILLBOT.C		*/
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	SplineDefType			**gSplineList;
extern	OGLPoint3D				gCoord;
extern	short					gNumEnemies;
extern	float					gFramesPerSecondFrac,gFramesPerSecond,gCurrentMaxSpeed;
extern	OGLVector3D			gDelta;
extern	signed char			gNumEnemyOfKind[];
extern	u_long		gAutoFadeStatusBits;
extern	PlayerInfoType	gPlayerInfo;
extern	Boolean				gPlayerIsDead;
extern	SparkleType	gSparkles[MAX_SPARKLES];


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveDrillBot(ObjNode *theNode);
static ObjNode *MakeDrillBot(float x, float z);
static void AlignDrillBotParts(ObjNode *theNode);
static void MoveDrillBot_Chase(ObjNode *theNode);
static void MoveDrillBot_Ram(ObjNode *theNode);
static void MoveDrillBot_Wait(ObjNode *theNode);
static void DoDrillBotCollisionDetect(ObjNode *theNode);
static void UpdateDrillBot(ObjNode *theNode);
static void MoveDrillBotOnSpline(ObjNode *theNode);
static Boolean DrillBotHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void DrillBotHitByJumpJet(ObjNode *enemy);
static Boolean DrillBotHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean HurtDrillBot(ObjNode *enemy, float damage);
static void KillDrillBot(ObjNode *enemy);
static void SeeIfDrillPlayer(ObjNode *body);
static void ThrowPlayerOffDrill(ObjNode *body, ObjNode *player);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_DRILLBOTS			9

#define	DRILLBOT_SCALE			1.5f

#define	DRILLBOT_CHASE_DIST		3000.0f
#define	DRILLBOT_CHASE_SPEED	240.0f

#define	DRILLBOT_DETACH_DIST	1000.0f

#define	DRILLBOT_HEALTH		1.5f
#define	DRILLBOT_DAMAGE		.2f

#define	DRILLBOT_RAM_DIST	500.0f
#define	DRILLBOT_RAM_SPEED	900.0f

#define DRILLBOT_TURN_SPEED	1.6f
#define	RAM_DURATION		1.3f
#define	WAIT_DELAY			1.5f

#define	DRILLBOT_TARGET_OFFSET	50.0f

enum
{
	DRILLBOT_MODE_WAIT,
	DRILLBOT_MODE_CHASE,
	DRILLBOT_MODE_RAM
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	RamTimer	SpecialF[0]
#define	WaitDelay	SpecialF[1]

#define	PlayerSlide	SpecialF[0]
#define	DrillTimer	SpecialF[1]

static	ObjNode	*gWhoIsDrillingPlayer;

/************************* ADD DRILLBOT *********************************/

Boolean AddDrillBot(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*body;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_DRILLBOT] >= MAX_DRILLBOTS)
			return(false);
	}

	body = MakeDrillBot(x,z);

	body->TerrainItemPtr = itemPtr;								// keep ptr to item list
	body->EnemyRegenerate = itemPtr->parm[3] & (1<<1);

	AlignDrillBotParts(body);

	gNumEnemies++;
	gNumEnemyOfKind[body->Kind]++;
	return(true);													// item was added
}

/****************** MAKE DRILLBOT ***********************/

static ObjNode *MakeDrillBot(float x, float z)
{
ObjNode	*body,*drill,*wheels,*prev;
int		i,j;
float	q;
				/******************/
				/* MAKE MAIN BODY */
				/******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FIREICE_ObjType_DrillBot_Body;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 381;
	gNewObjectDefinition.moveCall 	= MoveDrillBot;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= DRILLBOT_SCALE;
	body = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	body->Mode 			= DRILLBOT_MODE_WAIT;
	body->WaitDelay 	= 0;
	body->Damage 		= DRILLBOT_DAMAGE;
	body->Health 		= DRILLBOT_HEALTH;


			/* SET COLLISION STUFF */

	body->CType 		= CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_ENEMY|CTYPE_AUTOTARGETWEAPON;
	body->CBits			= CBITS_ALLSOLID;
	q = body->BBox.max.x * DRILLBOT_SCALE;
	SetObjectCollisionBounds(body, body->BBox.max.y * DRILLBOT_SCALE, -10.0f * DRILLBOT_SCALE, -q,q,q,-q);

	CalcNewTargetOffsets(body,DRILLBOT_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	body->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= DrillBotHitByWeapon;
	body->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= DrillBotHitBySuperNova;
	body->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= DrillBotHitByWeapon;
	body->HitByWeaponHandler[WEAPON_TYPE_DART] 			= DrillBotHitByWeapon;
	body->HitByWeaponHandler[WEAPON_TYPE_FLARE] 		= DrillBotHitByWeapon;
	body->HitByJumpJetHandler 							= DrillBotHitByJumpJet;

	body->HurtCallback = HurtDrillBot;							// set hurt callback function


				/************/
				/* MAKE DRILL */
				/*************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_DrillBot_Drill;
	gNewObjectDefinition.slot		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	drill = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	body->ChainNode = drill;


			/***************/
			/* MAKE WHEELS */
			/***************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_DrillBot_Wheels;
	gNewObjectDefinition.moveCall 	= nil;

	prev = drill;
	for (i = 0; i < 2; i++)
	{
		wheels = MakeNewDisplayGroupObject(&gNewObjectDefinition);
		prev->ChainNode = wheels;
		prev = wheels;
	}


		/****************/
		/* ADD SPARKLES */
		/****************/

	for (j = 0; j < 2; j++)
	{
		const OGLPoint3D	sparkles[2] = {-77,112,-40,  77,112,-40};

		i = body->Sparkles[j] = GetFreeSparkle(body);				// get free sparkle slot
		if (i != -1)
		{
			gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_FLICKER|SPARKLE_FLAG_TRANSFORMWITHOWNER;
			gSparkles[i].where = sparkles[j];

			gSparkles[i].color.r = 1;
			gSparkles[i].color.g = 1;
			gSparkles[i].color.b = 1;
			gSparkles[i].color.a = 1;

			gSparkles[i].scale = 150.0f;
			gSparkles[i].separation = 40.0f;

			gSparkles[i].textureNum = PARTICLE_SObjType_GreenGlint;
		}
	}

	return(body);
}


/********************** MOVE DRILLBOT ***********************/

static void MoveDrillBot(ObjNode *theNode)
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

		case	DRILLBOT_MODE_WAIT:
				MoveDrillBot_Wait(theNode);
				break;


				/* CHASE MODE */

		case	DRILLBOT_MODE_CHASE:
				MoveDrillBot_Chase(theNode);
				break;


				/* RAM MODE */

		case	DRILLBOT_MODE_RAM:
				MoveDrillBot_Ram(theNode);
				break;

	}


		/* COLLISION DETECT */

	DoDrillBotCollisionDetect(theNode);


		/**********/
		/* UPDATE */
		/**********/

	UpdateDrillBot(theNode);



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


/******************** MOVE DRILLBOT:  WAIT *****************************/

static void MoveDrillBot_Wait(ObjNode *theNode)
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

	theNode->WaitDelay -= fps;							// see if ready to chase
	if (theNode->WaitDelay <= 0.0f)
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < DRILLBOT_CHASE_DIST)
		{
			theNode->Mode = DRILLBOT_MODE_CHASE;
		}
	}
}



/******************** MOVE DRILLBOT:  CHASE *****************************/

static void MoveDrillBot_Chase(ObjNode *theNode)
{
float	speed,angle,r,fps = gFramesPerSecondFrac;

		/* ACCEL TO CHASE SPEED */

	theNode->Speed2D += DRILLBOT_CHASE_SPEED * fps;
	if (theNode->Speed2D > DRILLBOT_CHASE_SPEED)
		theNode->Speed2D = DRILLBOT_CHASE_SPEED;
	speed = theNode->Speed2D;

			/* MOVE TOWARD PLAYER */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, DRILLBOT_TURN_SPEED, false);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * speed;
	gDelta.z = -cos(r) * speed;
	gDelta.y -= 2000.0f * fps;						// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y += gDelta.y * fps;


			/* SEE IF READY TO RAM */

	if (angle < PI/4)
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < DRILLBOT_RAM_DIST)
		{
			theNode->Mode = DRILLBOT_MODE_RAM;
			theNode->RamTimer = RAM_DURATION;
		}
	}
}


/******************** MOVE DRILLBOT:  RAM *****************************/

static void MoveDrillBot_Ram(ObjNode *theNode)
{
float	speed,r,fps = gFramesPerSecondFrac;


		/* ACCEL TO CHASE SPEED */

	theNode->Speed2D += DRILLBOT_RAM_SPEED * fps;
	if (theNode->Speed2D > DRILLBOT_RAM_SPEED)
		theNode->Speed2D = DRILLBOT_RAM_SPEED;
	speed = theNode->Speed2D;

			/* MOVE TOWARD PLAYER */

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * speed;
	gDelta.z = -cos(r) * speed;
	gDelta.y -= 2000.0f * fps;						// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y += gDelta.y * fps;


			/* SEE IF DONE */

	theNode->RamTimer -= fps;
	if (theNode->RamTimer <= 0.0f)
	{
		theNode->Mode = DRILLBOT_MODE_WAIT;
		theNode->WaitDelay = WAIT_DELAY;
	}
}




/********************* ALIGN DRILLBOT PARTS ************************/

static void AlignDrillBotParts(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	speed;
ObjNode	*drill,*wheel;
OGLMatrix4x4	m;
int				i;
static const OGLPoint3D wheelOffs[2] =
{
	0,32,-45,
	0,32,45,
};

	speed = theNode->Speed2D = CalcVectorLength(&theNode->Delta);			// calc 2D speed


			/********************/
			/* UPDATE THE DRILL */
			/********************/

	drill = theNode->ChainNode;

	drill->Rot.z -= fps * 20.0f;


	OGLMatrix4x4_SetRotate_Z(&m, drill->Rot.z);						// set rotation matrix
	m.value[M03] = 0;												// insert translation
	m.value[M13] = 59.0f;
	m.value[M23] = 0;
	OGLMatrix4x4_Multiply(&m, &theNode->BaseTransformMatrix, &drill->BaseTransformMatrix);
	SetObjectTransformMatrix(drill);

	drill->Coord = theNode->Coord;

			/*****************/
			/* UPDATE WHEELS */
			/*****************/

	wheel = drill->ChainNode;											// get wheel 1st obj

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



		/* UPDATE THE AUDIO WHILE WE'RE HERE */

	if (drill->DrillTimer > 0.0f)					// see which drill whine to play
	{
		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_DRILLBOTWHINEHI, &theNode->Coord, NORMAL_CHANNEL_RATE * 3/2, 1.5);
		else
			Update3DSoundChannel(EFFECT_DRILLBOTWHINEHI, &theNode->EffectChannel, &theNode->Coord);
	}
	else
	{
		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect3D(EFFECT_DRILLBOTWHINE, &theNode->Coord);
		else
			Update3DSoundChannel(EFFECT_DRILLBOTWHINE, &theNode->EffectChannel, &theNode->Coord);
	}
}


/***************** DO DRILLBOT COLLISION DETECT **************************/

static void DoDrillBotCollisionDetect(ObjNode *theNode)
{
			/* HANDLE THE BASIC STUFF */

	HandleCollisions(theNode, CTYPE_TERRAIN|CTYPE_MISC|CTYPE_TRIGGER2|CTYPE_PLAYER, 0);


				/* CHECK FENCE COLLISION */

	DoFenceCollision(theNode);

}


/******************** UPDATE DRILLBOT *****************/

static void UpdateDrillBot(ObjNode *theNode)
{
	UpdateObject(theNode);
	AlignDrillBotParts(theNode);

	SeeIfDrillPlayer(theNode);
}


//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME DRILLBOT ENEMY *************************/

Boolean PrimeEnemy_DrillBot(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/***************/
				/* MAKE OBJECT */
				/***************/

	newObj = MakeDrillBot(x,z);

	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;


				/* SET BETTER INFO */

	newObj->InitCoord 		= newObj->Coord;					// remember where started
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		-= newObj->BottomOff;
	newObj->MoveCall		= nil;
	newObj->SplineMoveCall 	= MoveDrillBotOnSpline;				// set move call
	newObj->Health 			= 1.0;
	newObj->Damage 			= DRILLBOT_DAMAGE;
	newObj->Kind 			= ENEMY_KIND_DRILLBOT;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);									// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE DRILLBOT ON SPLINE ***************************/

static void MoveDrillBotOnSpline(ObjNode *theNode)
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
		if (dist < DRILLBOT_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveDrillBot);

					/* KEEP ALL THE PARTS ALIGNED */

		AlignDrillBotParts(theNode);

	}
}


#pragma mark -

/************ DRILLBOT HIT BY STUN PULSE *****************/
//
// Called when weapon is moved and this handler is called after a collision.
//
// Also used for Flame weapon!
//

static Boolean DrillBotHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weaponCoord)


			/* HURT IT */

	HurtDrillBot(enemy, weapon->Damage);


			/* GIVE MOMENTUM */

	enemy->Delta.x += weaponDelta->x * .4f;
	enemy->Delta.y += weaponDelta->y * .4f  + 200.0f;
	enemy->Delta.z += weaponDelta->z * .4f;

	return(true);			// stop weapon
}




/************ DRILLBOT GOT HIT BY JUMP JET *****************/

static void DrillBotHitByJumpJet(ObjNode *enemy)
{
float	r;

		/* HURT IT */

	HurtDrillBot(enemy, 1.0);


			/* GIVE MOMENTUM */

	r = gPlayerInfo.objNode->Rot.y;
	enemy->Delta.x = -sin(r) * 1000.0f;
	enemy->Delta.z = -cos(r) * 1000.0f;
	enemy->Delta.y = 500.0f;
}

/************** DRILLBOT GOT HIT BY SUPERNOVA *****************/

static Boolean DrillBotHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	KillDrillBot(enemy);

	return(false);
}




/*********************** HURT DRILLBOT ***************************/

static Boolean HurtDrillBot(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveDrillBot);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillDrillBot(enemy);
		return(true);
	}
	else
	{

	}

	return(false);
}


/****************** KILL DRILLBOT ***********************/

static void KillDrillBot(ObjNode *enemy)
{
int		i;
ObjNode	*newObj;
static const OGLPoint3D	wheelOff[4] =
{
	-104.0,				// front left wheel
	32.0,
	-47.0,

	104.0,				// front right wheel
	32.0 ,
	-47.0,

	-104.0,				// back left wheel
	32.0,
	47.0,

	104.0,				// back rightwheel
	32.0,
	47.0,
};
OGLPoint3D	wheelPts[4];

static const OGLPoint3D	drillOff = {0, 71, -271};


		/* THROW PLAYER IN CASE WAS BEING DRILLED WHEN BOT GOT KILLED */

	ThrowPlayerOffDrill(enemy, gPlayerInfo.objNode);


			/*******************/
			/* CREATE 4 WHEELS */
			/*******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;						// set basic info
	gNewObjectDefinition.type 		= FIREICE_ObjType_DrillBot_Chunk_Wheel;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveEnemyRobotChunk;
	gNewObjectDefinition.rot 		= enemy->Rot.y;
	gNewObjectDefinition.scale 		= DRILLBOT_SCALE;

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
			/* CREATE DRILL */
			/****************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_DrillBot_Chunk_Drill;
	OGLPoint3D_Transform(&drillOff, &enemy->BaseTransformMatrix, &gNewObjectDefinition.coord);		// calc coord of drill
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->Delta.x = RandomFloat2() * 600.0f;
	newObj->Delta.z = RandomFloat2() * 600.0f;
	newObj->Delta.y = 1000.0f;

	newObj->DeltaRot.x = RandomFloat2() * 9.0f;
	newObj->DeltaRot.z = RandomFloat2() * 9.0f;



			/****************************/
			/* EXPLODE BODY INTO SHARDS */
			/****************************/

	ExplodeGeometry(enemy, 400, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .4);

	MakeSparkExplosion(enemy->Coord.x, enemy->Coord.y, enemy->Coord.z, 250, 2.0, PARTICLE_SObjType_BlueSpark,0);
	MakeSparkExplosion(enemy->Coord.x, enemy->Coord.y, enemy->Coord.z, 180, 2.2, PARTICLE_SObjType_GreenSpark,0);

	PlayEffect3D(EFFECT_ROBOTEXPLODE, &enemy->Coord);



			/* ATOMS */

	SpewAtoms(&enemy->Coord, 1,1,1, false);


			/* DELETE THE OLD STUFF */

	if (!enemy->EnemyRegenerate)
		enemy->TerrainItemPtr = nil;							// dont ever come back

	DeleteEnemy(enemy);


}


#pragma mark -

/***************************** SEE IF DRILL PLAYER ******************************/

static void SeeIfDrillPlayer(ObjNode *body)
{
float	 fps = gFramesPerSecondFrac;
ObjNode	*player = gPlayerInfo.objNode;
ObjNode	*drill = body->ChainNode;
static const OGLPoint3D drillPtOffs[2] =
{
	0,0,-471,							// tip
	0,0,-141							// base
};
OGLPoint3D	drillPts[2];
float		f,oneMinusF;


			/* CALC TIP & BASE OF DRILL */

	OGLPoint3D_TransformArray(&drillPtOffs[0], &drill->BaseTransformMatrix,	&drillPts[0], 2);



			/**************************************************/
			/* IF ALREADY BEING DRILLED THEN JUST UPDATE THAT */
			/**************************************************/

	if (player->Skeleton->AnimNum == PLAYER_ANIM_DRILLED)
	{
		OGLPoint3D	oldCoord;

		if (gWhoIsDrillingPlayer != body)				// see if this is the drillbot doing the drilling
			return;

				/* SEE IF DONE */

		drill->DrillTimer -= fps;
		if (drill->DrillTimer <= 0.0f)
		{
			ThrowPlayerOffDrill(body, player);
			return;
		}

drilled:
		player->Rot.z = -drill->Rot.z;
		player->Rot.y = body->Rot.y + PI;

				/* CALC PT ON DRILL TO PUT PLAYER */

		drill->PlayerSlide += fps * 3.0f;
		if (drill->PlayerSlide > PI2)
			drill->PlayerSlide -= PI2;

		f = (1.0f + sin(drill->PlayerSlide)) * .5f;				// get ratio 0..1
		oneMinusF = 1.0f - f;

		player->Coord.x = (drillPts[0].x * f) + (drillPts[1].x * oneMinusF);
		player->Coord.y = (drillPts[0].y * f) + (drillPts[1].y * oneMinusF);
		player->Coord.z = (drillPts[0].z * f) + (drillPts[1].z * oneMinusF);

		gPlayerInfo.invincibilityTimer = 2.0f;					// keep this set during the drill


				/* MAKE SURE NOT DRILLING THRU FENCE */

		oldCoord = gCoord;
		gCoord = player->Coord;						// set global coord to play for this collision call
		if (DoFenceCollision(player))
		{
			ThrowPlayerOffDrill(body, player);
		}
		gCoord = oldCoord;							// reset global coord to drillbot

	}



			/**********************************/
			/* SEE IF SHOULD DRILL PLAYER NOW */
			/**********************************/
	else
	{
		if (gPlayerInfo.invincibilityTimer > 0.0f)					// not if player invincible
			return;

		if (SeeIfLineSegmentHitsPlayer(&drillPts[0], &drillPts[1]))
		{
			PlayerGotHit(nil, DRILLBOT_DAMAGE);										// hurt player
			if (!gPlayerIsDead)
			{
				MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_DRILLED, 8);
				player->StatusBits |= STATUS_BIT_ROTZXY;
				gWhoIsDrillingPlayer = body;
				drill->PlayerSlide = PI;
				drill->DrillTimer = 3.0f;
				goto drilled;
			}
		}
	}
}


/********************** THROW PLAYER OFF DRILL ***************************/

static void ThrowPlayerOffDrill(ObjNode *body, ObjNode *player)
{
OGLVector2D		v;
OGLMatrix3x3	m;

	if (player->Skeleton->AnimNum != PLAYER_ANIM_DRILLED)
		return;

	SetSkeletonAnim(player->Skeleton, PLAYER_ANIM_THROWN);
	PlayEffect_Parms3D(EFFECT_THROWNSWOOSH, &player->Coord, NORMAL_CHANNEL_RATE, 2.0);

			/* CALC VECTOR TO THROW PLAYER */

	v.x = player->Coord.x - body->Coord.x;
	v.y = player->Coord.z - body->Coord.z;
	FastNormalizeVector2D(v.x, v.y, &v, true);

	OGLMatrix3x3_SetRotate(&m, PI/3);										// rot 90 degrees to give lateral-side-swipe motion
	OGLVector2D_Transform(&v, &m, &v);

	player->Delta.x = v.x * 1000.0f;
	player->Delta.z = v.y * 1000.0f;
	player->Delta.y = 2200.0f;

	gCurrentMaxSpeed = CalcVectorLength(&player->Delta);					// set this so we can go flyin'

	player->StatusBits &= ~STATUS_BIT_ROTZXY;
	player->Rot.z = 0;
}














