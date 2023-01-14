/****************************/
/*   ENEMY: HAMMERBOT.C		*/
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

static void MoveHammerBot(ObjNode *theNode);
static ObjNode *MakeHammerBot(float x, float z);
static void AlignHammerBotParts(ObjNode *theNode);
static void MoveHammerBot_Chase(ObjNode *theNode);
static void MoveHammerBot_Wait(ObjNode *theNode);
static void DoHammerBotCollisionDetect(ObjNode *theNode);
static void UpdateHammerBot(ObjNode *theNode);
static void MoveHammerBotOnSpline(ObjNode *theNode);
static Boolean HammerBotHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void HammerBotHitByJumpJet(ObjNode *enemy);
static Boolean HammerBotHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean HurtHammerBot(ObjNode *enemy, float damage);
static void KillHammerBot(ObjNode *enemy);
static void MoveHammerBot_Hammer(ObjNode *theNode);
static void MoveHammerBot_Reset(ObjNode *theNode);
static void UpdateHammerMarch(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_HAMMERBOTS		11

#define	HAMMERBOT_SCALE		2.5f

#define	HAMMERBOT_CHASE_DIST	1500.0f
#define	HAMMERBOT_CHASE_SPEED	350.0f

#define	HAMMERBOT_DETACH_DIST	1000.0f

#define	HAMMERBOT_DAMAGE		.2f
#define	HAMMERBOT_HEALTH		1.8f

#define HAMMERBOT_TURN_SPEED	1.8f


enum
{
	HAMMERBOT_MODE_WAIT,
	HAMMERBOT_MODE_CHASE,
	HAMMERBOT_MODE_HAMMER,
	HAMMERBOT_MODE_RESET
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	WaitDelay	SpecialF[1]
#define	ChompIndex	SpecialF[2]


/************************* ADD HAMMERBOT *********************************/

Boolean AddHammerBot(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*body;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_HAMMERBOT] >= MAX_HAMMERBOTS)
			return(false);
	}

	body = MakeHammerBot(x,z);

	body->TerrainItemPtr = itemPtr;								// keep ptr to item list
	body->EnemyRegenerate = itemPtr->parm[3] & (1<<1);

	if (body->EnemyRegenerate)					// assume this is the ice saucer hammer, so give it lots of health
		body->Health = 20.0f;



	AlignHammerBotParts(body);

	gNumEnemies++;
	gNumEnemyOfKind[body->Kind]++;

	return(true);													// item was added
}

/****************** MAKE HAMMERBOT ***********************/

static ObjNode *MakeHammerBot(float x, float z)
{
ObjNode	*body,*hammer,*wheels;
float	q;
				/******************/
				/* MAKE MAIN BODY */
				/******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FIREICE_ObjType_HammerBot_Body;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 381;
	gNewObjectDefinition.moveCall 	= MoveHammerBot;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= HAMMERBOT_SCALE;
	body = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	body->Mode 			= HAMMERBOT_MODE_WAIT;
	body->WaitDelay 	= 0;
	body->Damage 		= HAMMERBOT_DAMAGE;

	body->ChompIndex 	= RandomFloat()*PI2;


			/* SET COLLISION STUFF */

	body->CType 		= CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_ENEMY|CTYPE_AUTOTARGETWEAPON;
	body->CBits			= CBITS_ALLSOLID;
	q 					= 60.0f * HAMMERBOT_SCALE;
	SetObjectCollisionBounds(body, 200.0f, -33.0f * HAMMERBOT_SCALE, -q,q,q,-q);



				/* SET WEAPON HANDLERS */

	body->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= HammerBotHitByWeapon;
	body->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= HammerBotHitBySuperNova;
	body->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= HammerBotHitByWeapon;
	body->HitByWeaponHandler[WEAPON_TYPE_DART] 		= HammerBotHitByWeapon;
	body->HitByWeaponHandler[WEAPON_TYPE_FLARE] 		= HammerBotHitByWeapon;
	body->HitByJumpJetHandler 						= HammerBotHitByJumpJet;

	body->HurtCallback = HurtHammerBot;							// set hurt callback function


				/***************/
				/* MAKE HAMMER */
				/***************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_HammerBot_Hammer;
	gNewObjectDefinition.slot		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	hammer = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	body->ChainNode = hammer;


			/***************/
			/* MAKE WHEELS */
			/***************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_HammerBot_Wheels;

	wheels = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	hammer->ChainNode = wheels;


	return(body);
}


/********************** MOVE HAMMERBOT ***********************/

static void MoveHammerBot(ObjNode *theNode)
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

		case	HAMMERBOT_MODE_WAIT:
				MoveHammerBot_Wait(theNode);
				break;


				/* CHASE MODE */

		case	HAMMERBOT_MODE_CHASE:
				MoveHammerBot_Chase(theNode);
				break;


				/* HAMMER MODE */

		case	HAMMERBOT_MODE_HAMMER:
				MoveHammerBot_Hammer(theNode);
				break;

				/* RESET */

		case	HAMMERBOT_MODE_RESET:
				MoveHammerBot_Reset(theNode);
				break;

	}


		/* COLLISION DETECT */

	DoHammerBotCollisionDetect(theNode);


		/**********/
		/* UPDATE */
		/**********/

	UpdateHammerBot(theNode);



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


/******************** MOVE HAMMERBOT:  WAIT *****************************/

static void MoveHammerBot_Wait(ObjNode *theNode)
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


			/* UPDATE HAMMER ANIM */

	UpdateHammerMarch(theNode);



			/* SEE IF CHASE */

	theNode->WaitDelay -= fps;							// see if ready to chase
	if (theNode->WaitDelay <= 0.0f)
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < HAMMERBOT_CHASE_DIST)
		{
			theNode->Mode = HAMMERBOT_MODE_CHASE;
		}
	}
}



/******************** MOVE HAMMERBOT:  CHASE *****************************/

static void MoveHammerBot_Chase(ObjNode *theNode)
{
float	speed, r;
float	fps = gFramesPerSecondFrac;
float	targetX,targetY,targetZ,sinR,cosR;

		/* ACCEL TO CHASE SPEED */

	theNode->Speed2D += HAMMERBOT_CHASE_SPEED * fps;
	if (theNode->Speed2D > HAMMERBOT_CHASE_SPEED)
		theNode->Speed2D = HAMMERBOT_CHASE_SPEED;
	speed = theNode->Speed2D;

			/* MOVE TOWARD PLAYER */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, HAMMERBOT_TURN_SPEED, false);

	r = theNode->Rot.y;
	sinR = -sin(r);
	cosR = -cos(r);
	gDelta.x = sinR * speed;
	gDelta.z = cosR * speed;
	gDelta.y -= 2000.0f * fps;						// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y += gDelta.y * fps;

			/* UPDATE HAMMER ANIM */

	UpdateHammerMarch(theNode);


 			/**************************/
			/* SEE IF READY TO HAMMER */
 			/**************************/

	if (gPlayerInfo.invincibilityTimer <= 0.0f)							// dont hammer if player invincible
	{
			/* CALC TARGET SPOT IN FRONT OF HAMMERBOT */

		targetX = gCoord.x + sinR * (179.0f * HAMMERBOT_SCALE);
		targetZ = gCoord.z + cosR * (179.0f * HAMMERBOT_SCALE);
		targetY = gCoord.y;

				/* SEE IF PLAYER IS IN TARGET ZONE */

		if (DoSimpleBoxCollisionAgainstPlayer(targetY + 200.0f, targetY, targetX - 70.0f, targetX + 70.0f,
											targetZ + 70.0f, targetZ - 70.0f))
		{
			theNode->Mode = HAMMERBOT_MODE_HAMMER;
		}
	}
}

/******************** MOVE HAMMERBOT:  HAMMER *****************************/

static void MoveHammerBot_Hammer(ObjNode *theNode)
{
float	speed,r,fps = gFramesPerSecondFrac;
ObjNode	*hammer;
static const OGLPoint3D headOff = {0,179,-34};
OGLPoint3D	headPt;

			/* DECELERATE */

	theNode->Speed2D -= 5000.0f * fps;
	if (theNode->Speed2D < 0.0f)
		theNode->Speed2D = 0;
	speed = theNode->Speed2D;

			/* MOVE */

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * speed;
	gDelta.z = -cos(r) * speed;
	gDelta.y -= 1000.0f * fps;						// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y += gDelta.y * fps;

		/****************/
		/* SWING HAMMER */
		/****************/

	hammer = theNode->ChainNode;

	OGLPoint3D_Transform(&headOff, &hammer->BaseTransformMatrix, &headPt);			// calc coord of hammer's head

		/* DO THE SWING */

	if (hammer->Rot.x > -PI/2)
	{
		hammer->Rot.x -= fps * 8.0f;
		if (hammer->Rot.x <= -PI/2)													// see if at 90 degree hit
		{
			hammer->Rot.x = -PI/2;
			theNode->Delta.y = 1000.0f;												// pop up a little when hammer hits
			PlayEffect_Parms3D(EFFECT_METALHIT, &gCoord, NORMAL_CHANNEL_RATE, 1.0);	// make clank sound
		}

				/* SEE IF POUNDED PLAYER */

		if (DoSimpleBoxCollisionAgainstPlayer(headPt.y + 50.0f, headPt.y, headPt.x - 40.0f, headPt.x + 40.0f,
										headPt.z + 40.0f, headPt.z - 40.0f))
		{
			CrushPlayer();
			MakeSparkExplosion(headPt.x, headPt.y, headPt.z, 200.0f, .6, PARTICLE_SObjType_WhiteSpark4,0);
			theNode->Mode = HAMMERBOT_MODE_RESET;
			theNode->Timer = 0;														// time to keep hammer down when impact
			PlayEffect_Parms3D(EFFECT_METALHIT, &gCoord, NORMAL_CHANNEL_RATE, 1.0);	// make clank sound
		}
	}

			/* RESET AFTER HIT */
	else
	{
		theNode->Mode = HAMMERBOT_MODE_RESET;
		theNode->Timer = .3f + RandomFloat();						// time to keep hammer down when impact

		SeeIfCrackSaucerIce(&headPt);
	}


}


/******************** MOVE HAMMERBOT:  RESET *****************************/

static void MoveHammerBot_Reset(ObjNode *theNode)
{
float	speed,r,fps = gFramesPerSecondFrac;
ObjNode	*hammer;

			/* DECELERATE */

	theNode->Speed2D -= 5000.0f * fps;
	if (theNode->Speed2D < 0.0f)
		theNode->Speed2D = 0;
	speed = theNode->Speed2D;

			/* MOVE */

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * speed;
	gDelta.z = -cos(r) * speed;
	gDelta.y -= 1000.0f * fps;						// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y += gDelta.y * fps;

		/*************************/
		/* MOVE HAMMER BACK TO 0 */
		/*************************/

	theNode->Timer -= fps;							// keep down for a sec before moving
	if (theNode->Timer <= 0.0f)
	{
		hammer = theNode->ChainNode;

		hammer->Rot.x += fps * 1.5f;
		if (hammer->Rot.x >= 0.0f)
		{
			hammer->Rot.x = 0;
			hammer->ChompIndex = 0;												// reset so will line up on other anims
			theNode->Mode = HAMMERBOT_MODE_WAIT;
		}
	}
}





/********************* ALIGN HAMMERBOT PARTS ************************/

static void AlignHammerBotParts(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	speed;
ObjNode	*hammer,*wheel;
OGLMatrix4x4	m;

	speed = theNode->Speed2D = CalcVectorLength(&theNode->Delta);			// calc 2D speed


			/*********************/
			/* UPDATE THE HAMMER */
			/*********************/

	hammer = theNode->ChainNode;



	OGLMatrix4x4_SetRotate_X(&m, hammer->Rot.x);						// set rotation matrix
	OGLMatrix4x4_Multiply(&m, &theNode->BaseTransformMatrix, &hammer->BaseTransformMatrix);
	SetObjectTransformMatrix(hammer);

	hammer->Coord = theNode->Coord;

			/*****************/
			/* UPDATE WHEELS */
			/*****************/

	wheel = hammer->ChainNode;											// get wheel 1st obj

	wheel->Rot.x -= speed * .01f * fps;							// spin the wheel while we're here

	OGLMatrix4x4_SetRotate_X(&m, wheel->Rot.x);					// set rotation matrix
	OGLMatrix4x4_Multiply(&m, &theNode->BaseTransformMatrix, &wheel->BaseTransformMatrix);
	SetObjectTransformMatrix(wheel);

	wheel->Coord = theNode->Coord;

}


/***************** DO HAMMERBOT COLLISION DETECT **************************/

static void DoHammerBotCollisionDetect(ObjNode *theNode)
{
			/* HANDLE THE BASIC STUFF */

	HandleCollisions(theNode, CTYPE_TERRAIN|CTYPE_MISC|CTYPE_TRIGGER2|CTYPE_PLAYER, 0);


				/* CHECK FENCE COLLISION */

	DoFenceCollision(theNode);

}


/******************** UPDATE HAMMERBOT *****************/

static void UpdateHammerBot(ObjNode *theNode)
{
	UpdateObject(theNode);
	AlignHammerBotParts(theNode);
}

/**************** UPDATE HAMMER MARCH *******************/

static void UpdateHammerMarch(ObjNode *theNode)
{
ObjNode	*hammer = theNode->ChainNode;
float	speed = theNode->Speed2D;
float	oldRotX,newRotX;

	oldRotX = hammer->Rot.x;										// get rot before we move it

	hammer->ChompIndex += (150.0f + speed) * gFramesPerSecondFrac * .015f;
	hammer->Rot.x = newRotX = sin(hammer->ChompIndex) * .6f;

		/* SEE IF MAKE SQUEAK */

	if ((oldRotX * newRotX) < 0.0f)				// if the signs of the 2 rots are different then crossed the center line
	{
		if (newRotX < 0.0f)
			PlayEffect_Parms3D(EFFECT_HAMMERSQUEAK, &hammer->Coord, NORMAL_CHANNEL_RATE, 1.0);
		else
			PlayEffect_Parms3D(EFFECT_HAMMERSQUEAK, &hammer->Coord, NORMAL_CHANNEL_RATE * 2/3, 1.0);
	}

}


//===============================================================================================================
//===============================================================================================================
//===============================================================================================================



#pragma mark -

/************************ PRIME HAMMERBOT ENEMY *************************/

Boolean PrimeEnemy_HammerBot(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/***************/
				/* MAKE OBJECT */
				/***************/

	newObj = MakeHammerBot(x,z);

	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;


				/* SET BETTER INFO */

	newObj->InitCoord 		= newObj->Coord;					// remember where started
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		-= newObj->BottomOff;
	newObj->MoveCall		= nil;
	newObj->SplineMoveCall 	= MoveHammerBotOnSpline;				// set move call
	newObj->Health 			= HAMMERBOT_HEALTH;
	newObj->Damage 			= HAMMERBOT_DAMAGE;
	newObj->Kind 			= ENEMY_KIND_HAMMERBOT;


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);

	return(true);
}


/******************** MOVE HAMMERBOT ON SPLINE ***************************/

static void MoveHammerBotOnSpline(ObjNode *theNode)
{
Boolean isVisible;
float	dist;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 140);
	GetObjectCoordOnSpline(theNode);


			/* UPDATE STUFF IF VISIBLE */

	if (isVisible)
	{

		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BottomOff;	// calc y coord
		UpdateObjectTransforms(theNode);															// update transforms
		UpdateShadow(theNode);

				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))					// just do this to see if explosions hurt
			return;


					/* SEE IF LEAVE SPLINE TO CHASE PLAYER */

		dist = OGLPoint3D_Distance(&gPlayerInfo.coord, &theNode->Coord);
		if (dist < HAMMERBOT_DETACH_DIST)
			DetachEnemyFromSpline(theNode, MoveHammerBot);


					/* KEEP ALL THE PARTS ALIGNED */

		UpdateHammerMarch(theNode);
		AlignHammerBotParts(theNode);
	}
}


#pragma mark -

/************ HAMMERBOT HIT BY STUN PULSE *****************/
//
// Called when weapon is moved and this handler is called after a collision.
//
// Also used for Flame weapon!
//

static Boolean HammerBotHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weaponCoord)


			/* HURT IT */

	HurtHammerBot(enemy, weapon->Damage);


			/* GIVE MOMENTUM */

	enemy->Delta.x += weaponDelta->x * .4f;
	enemy->Delta.y += weaponDelta->y * .4f  + 200.0f;
	enemy->Delta.z += weaponDelta->z * .4f;

	return(true);			// stop weapon
}





/************ HAMMERBOT GOT HIT BY JUMP JET *****************/

static void HammerBotHitByJumpJet(ObjNode *enemy)
{
float	r;

		/* HURT IT */

	HurtHammerBot(enemy, 1.0);


			/* GIVE MOMENTUM */

	r = gPlayerInfo.objNode->Rot.y;
	enemy->Delta.x = -sin(r) * 1000.0f;
	enemy->Delta.z = -cos(r) * 1000.0f;
	enemy->Delta.y = 500.0f;
}

/************** HAMMERBOT GOT HIT BY SUPERNOVA *****************/

static Boolean HammerBotHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	KillHammerBot(enemy);

	return(false);
}




/*********************** HURT HAMMERBOT ***************************/

static Boolean HurtHammerBot(ObjNode *enemy, float damage)
{

			/* SEE IF REMOVE FROM SPLINE */

	if (enemy->StatusBits & STATUS_BIT_ONSPLINE)
		DetachEnemyFromSpline(enemy, MoveHammerBot);


				/* HURT ENEMY & SEE IF KILL */

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillHammerBot(enemy);
		return(true);
	}
	else
	{

	}

	return(false);
}


/****************** KILL HAMMERBOT ***********************/

static void KillHammerBot(ObjNode *enemy)
{
int		i;
ObjNode	*newObj;
static const OGLPoint3D	wheelOff[2] =
{
	{-40,0,0},			// left wheel
	{40,0,0},			// right wheel
};
OGLPoint3D	wheelPts[2];

static const OGLPoint3D	hammerOff = {0, 90, -60};
static const OGLPoint3D	bodyOff = {0, 17, -39};


			/*******************/
			/* CREATE 2 WHEELS */
			/*******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;						// set basic info
	gNewObjectDefinition.type 		= FIREICE_ObjType_HammerBot_Chunk_Wheel;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveEnemyRobotChunk;
	gNewObjectDefinition.rot 		= enemy->Rot.y + PI;
	gNewObjectDefinition.scale 		= HAMMERBOT_SCALE;

	OGLPoint3D_TransformArray(wheelOff, &enemy->BaseTransformMatrix, wheelPts, 2);		// calc coords of wheels

	for (i = 0; i < 2; i++)																// make the 4 wheel objects
	{
		gNewObjectDefinition.coord	 	= wheelPts[i];
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Delta.x = RandomFloat2() * 600.0f;
		newObj->Delta.z = RandomFloat2() * 600.0f;
		newObj->Delta.y = 1300.0f;

		newObj->DeltaRot.x = RandomFloat2() * 9.0f;
		newObj->DeltaRot.z = RandomFloat2() * 9.0f;

		gNewObjectDefinition.rot  -= PI;
	}


			/****************/
			/* CREATE HAMMER */
			/****************/

	gNewObjectDefinition.rot 		= enemy->Rot.y;
	gNewObjectDefinition.type 		= FIREICE_ObjType_HammerBot_Chunk_Hammer;
	OGLPoint3D_Transform(&hammerOff, &enemy->BaseTransformMatrix, &gNewObjectDefinition.coord);		// calc coord of drill
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->Delta.x = RandomFloat2() * 600.0f;
	newObj->Delta.z = RandomFloat2() * 600.0f;
	newObj->Delta.y = 1000.0f;

	newObj->DeltaRot.x = RandomFloat2() * 9.0f;
	newObj->DeltaRot.z = RandomFloat2() * 9.0f;


			/***************/
			/* CREATE BODY */
			/***************/

	gNewObjectDefinition.type 		= FIREICE_ObjType_HammerBot_Chunk_Body;
	OGLPoint3D_Transform(&bodyOff, &enemy->BaseTransformMatrix, &gNewObjectDefinition.coord);		// calc coord of drill
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->Delta.x = RandomFloat2() * 600.0f;
	newObj->Delta.z = RandomFloat2() * 600.0f;
	newObj->Delta.y = 1000.0f;

	newObj->DeltaRot.x = RandomFloat2() * 9.0f;
	newObj->DeltaRot.z = RandomFloat2() * 9.0f;




	MakeSparkExplosion(enemy->Coord.x, enemy->Coord.y, enemy->Coord.z, 350, 2.5, PARTICLE_SObjType_WhiteSpark4,0);
	MakeSparkExplosion(enemy->Coord.x, enemy->Coord.y, enemy->Coord.z, 280, 2.2, PARTICLE_SObjType_RedSpark,0);

	PlayEffect3D(EFFECT_ROBOTEXPLODE, &enemy->Coord);

			/* ATOMS */

	SpewAtoms(&enemy->Coord, 1,1,1, false);


			/* DELETE THE OLD STUFF */

	if (!enemy->EnemyRegenerate)
		enemy->TerrainItemPtr = nil;							// dont ever come back

	DeleteEnemy(enemy);

}










