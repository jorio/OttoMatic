/****************************/
/*   	BUMPERCAR.C		    */
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

static void MoveEnemyBumperCar(ObjNode *theNode);
static void PlayerMoveBumperCar(ObjNode *player);
static void DoBumperCarCollision(ObjNode *theNode);
static void UpdateBumperCar(ObjNode *theNode);
static void DrawGeneratorBeams(ObjNode *theNode);
static void MoveBumperCarPowerPost(ObjNode *theNode);
static void MoveBumperCarGate(ObjNode *posts);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	BUMPERCAR_SCALE	1.8f

#define	CONTROL_SENSITIVITY_BUMPERCAR_TURN	2.9f
#define	CONTROL_SENSITIVITY_BUMPERCAR_ACCEL	900.0f

#define	MAX_BUMPERCAR_SPEED			1400.0f
#define	MAX_ENEMY_BUMPERCAR_SPEED	(MAX_BUMPERCAR_SPEED * .9f)

#define	BUMPERCAR_FRICTION	130.0f

#define	MAX_CARS	8						// NOTE:  don't change this since we do some random #'s with "&0x7" masking

#define	BUMPER_CAR_RADIUS	(110.0f * BUMPERCAR_SCALE)
#define	BUMPER_CAR_RADIUSX2	(BUMPER_CAR_RADIUS * 2.0f)


#define	MAX_GENERATORS		8

#define	MAX_AREAS			3

const OGLColorRGBA kDimmedPowerPostFilter = { .4f, .4f, .4f, 1.0f };

enum
{
	GENERATOR_MODE_ON,
	GENERATOR_MODE_HIT,
	GENERATOR_MODE_BLOWN
};


/*********************/
/*    VARIABLES      */
/*********************/

ObjNode	*gPlayerCar = nil;
short	gPlayerCarID;


#define	GeneratorID		Special[0]
#define	AreaNum			Special[2]

#define	CarID			Special[0]
#define	SeekCarID		Special[1]
#define	CarBeingDriven	Flag[0]
#define	SeekTimer		SpecialF[0]						// timer for enemies seeking a target

#define	BumperWobble	SpecialF[0]

static ObjNode	*gCarList[MAX_AREAS][MAX_CARS];

static OGLPoint3D	gGeneratorCoords[MAX_AREAS][MAX_GENERATORS];
static	Byte		gGeneratorBlownMode[MAX_AREAS][MAX_GENERATORS];

static	short		gNumGenerators[MAX_AREAS];

static	int			gNumBlownGenerators[MAX_AREAS];
Boolean		gBumperCarGateBlown[MAX_AREAS];


/************************ INIT BUMPER CARS *************************/

void InitBumperCars(void)
{
int						a,i;
TerrainItemEntryType	*itemPtr;
ObjNode					*newObj;


	for (a = 0; a < MAX_AREAS; a++)
	{
		gNumGenerators[a] = 0;
		gNumBlownGenerators[a] = 0;
		gBumperCarGateBlown[a] = false;

		for (i = 0; i < MAX_CARS; i++)
			gCarList[a][i] = nil;


		for (i = 0; i < MAX_GENERATORS; i++)
			gGeneratorBlownMode[a][i] = GENERATOR_MODE_ON;
	}

			/*********************************/
			/* FIND COORDS OF ALL GENERATORS */
			/*********************************/


	itemPtr = *gMasterItemList; 									// get pointer to data inside the LOCKED handle

	for (i= 0; i < gNumTerrainItems; i++)
	{
		if (itemPtr[i].type == MAP_ITEM_BUMPERCARPOWER)				// see if it's what we're looking for
		{
			short	id,area;
			float	x,z;

			id = itemPtr[i].parm[0];								// get generator ID
			if (id >= MAX_GENERATORS)
				DoFatalAlert("InitBumperCars: ID# >= MAX_GENERATORS");

			area = itemPtr[i].parm[1];								// get area #
			if (area > MAX_AREAS)
				DoFatalAlert("InitBumperCars: Area# >= MAX_AREAS");

			x = gGeneratorCoords[area][id].x = itemPtr[i].x;		// get coords
			z = gGeneratorCoords[area][id].z = itemPtr[i].y;
			gGeneratorCoords[area][id].y = GetTerrainY_Undeformed(x,z) + 700.0f;

			gNumGenerators[area]++;
		}
	}

		/*******************************************/
		/* CREATE DUMMY DRAW OBJECT FOR GENERATORS */
		/*******************************************/

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+2;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.scale 		= 1;
	gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOLIGHTING|STATUS_BIT_GLOW|STATUS_BIT_NOZWRITES;

	newObj = MakeNewObject(&gNewObjectDefinition);
	newObj->CustomDrawFunction = DrawGeneratorBeams;
}



/************************* ADD BUMPER CAR *********************************/

Boolean AddBumperCar(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
short	id,area;
int		playerCar = itemPtr->parm[3] & 1;

	id = itemPtr->parm[0];											// get car ID #

	if (!gG4)														// on slow machines, add fewer cars
	{
		if (id > 3)
			return(true);

	}

	area = itemPtr->parm[1];										// get area #

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	if (playerCar)
		gNewObjectDefinition.type 	= CLOUD_ObjType_BumperCar;
	else
		gNewObjectDefinition.type 	= CLOUD_ObjType_ClownBumperCar;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= PLAYER_SLOT-2;								// move car before player
	gNewObjectDefinition.moveCall 	= MoveEnemyBumperCar;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= BUMPERCAR_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;							// keep ptr to item list

	newObj->BoundingSphereRadius =  newObj->BBox.max.x * BUMPERCAR_SCALE;	// set correct bounding sphere for fence collision

				/* ADD TO CAR LIST */

	newObj->AreaNum = area;										// save area #

	newObj->CarID = id;											// set car ID
	if (id >= MAX_CARS)
		DoFatalAlert("AddBumperCar: illegal car ID");

	if (gCarList[area][id] != nil)
		DoFatalAlert("AddBumperCar: duplicate car ID#!");
	gCarList[area][newObj->CarID] = newObj;



			/* PLAYER'S CAR IS A TRIGGER */

	if (playerCar)												// if its the player's car then setup as a trigger
	{
		newObj->CType 			= CTYPE_TRIGGER|CTYPE_BLOCKCAMERA;
		newObj->CBits			= CBITS_ALLSOLID;
		newObj->TriggerSides 	= SIDE_BITS_TOP;				// side(s) to activate it
		newObj->Kind		 	= TRIGTYPE_BUMPERCAR;
		CreateCollisionBoxFromBoundingBox(newObj,.9,.8);

		newObj->MoveCall 		= PlayerMoveBumperCar;					// change move call
	}

			/* CLOWN CAR */
	else
	{
		newObj->CType 			= CTYPE_BLOCKCAMERA;
		newObj->CBits			= CBITS_ALLSOLID;
		CreateCollisionBoxFromBoundingBox(newObj,.9,.8);
	}


	return(true);													// item was added
}

/************** DO TRIGGER - BUMPER CAR ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_BumperCar(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
#pragma unused (sideBits)



	theNode->CarBeingDriven = true;
	gPlayerCar = theNode;
	gPlayerCarID = theNode->CarID;

	SetSkeletonAnim(whoNode->Skeleton, PLAYER_ANIM_BUMPERCAR);
	AlignPlayerInBumperCar(whoNode);

	return(true);
}


/***************** STOP BUMPER CARS **********************/

static void StopBumperCars(void)
{
int	a,i;

			/* STOP ALL THE SOUNDS */

	for (a = 0; a < MAX_AREAS; a++)
		for (i = 0; i < MAX_CARS; i++)
		{
			if (gCarList[a][i])
			{
				StopAChannel(&gCarList[a][i]->EffectChannel);
			}
		}

	if (gPlayerCar)
	{
		gPlayerCar->CarBeingDriven = false;
		gPlayerCar = nil;
	}
}



/******************** MOVE ENEMY BUMPER CAR *****************/

static void MoveEnemyBumperCar(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*targetCar;


	if (!gPlayerCar || gBumperCarGateBlown[theNode->AreaNum])	// dont do anything if player isnt riding or if gate is blown
	{
not_active:
		if (TrackTerrainItem(theNode))							// just check to see if it's gone
		{
			gCarList[theNode->AreaNum][theNode->CarID] = nil;	// remove from car list
			DeleteObject(theNode);
			return;
		}

		if (theNode->EffectChannel != -1)						// kill sound if not moving
			StopAChannel(&theNode->EffectChannel);
		return;
	}
	else
	if (gPlayerCar->AreaNum != theNode->AreaNum)				// must be in same area
		goto not_active;


	GetObjectInfo(theNode);


	/**************************/
	/* SEE IF SEEK NEW TARGET */
	/**************************/

	theNode->SeekTimer -= fps;
	if ((theNode->SeekTimer <= 0.0f) || (gCarList[theNode->AreaNum][theNode->SeekCarID] == nil))
	{
		theNode->SeekTimer = 5.0f + RandomFloat() * 5.0f;			// set new random seek time

				/* SEEK PLAYER */

		if (gPlayerCar && ((MyRandomLong() & 0xf) < 0x9))			// only if player is driving and our odds are good
		{
			theNode->SeekCarID = gPlayerCarID;
		}

			/* SEEK RANDOM CAR */

		else
		{
			theNode->SeekCarID = MyRandomLong()&0x7;				// car 0..7
		}
	}

		/**********************/
		/* MOVE TOWARD TARGET */
		/**********************/

	targetCar = gCarList[theNode->AreaNum][theNode->SeekCarID];
	if (targetCar)
	{
		float	angle;

			/* DO TURNING AI */

		angle =  TurnObjectTowardTarget(theNode, &gCoord, targetCar->Coord.x, targetCar->Coord.z, PI, false);


			/* APPLY FRICTION TO DELTAS */

		ApplyFrictionToDeltas(BUMPERCAR_FRICTION, &gDelta);


		if (SeeIfLineSegmentHitsFence(&gCoord, &targetCar->Coord, nil, nil, nil))		// dont accel if there's a fence between us
		{
			theNode->SeekTimer = 0;														// clear this so will choose new target next time
		}
		else
		{

				/* ACCELERATE IF AIMED AT TARGET */

			if (angle < (PI/3))
			{
				float r = theNode->Rot.y;
				theNode->AccelVector.x = -sin(r);
				theNode->AccelVector.y = -cos(r);

				gDelta.x += theNode->AccelVector.x * (CONTROL_SENSITIVITY_BUMPERCAR_ACCEL * fps);
				gDelta.z += theNode->AccelVector.y * (CONTROL_SENSITIVITY_BUMPERCAR_ACCEL * fps);


							/* CALC SPEED */

				VectorLength2D(theNode->Speed2D, gDelta.x, gDelta.z);					// calc 2D speed value

				if (theNode->Speed2D > MAX_ENEMY_BUMPERCAR_SPEED)						// see if limit top speed
				{
					float	tweak;

					tweak = 1.0f / (theNode->Speed2D / MAX_ENEMY_BUMPERCAR_SPEED);

					gDelta.x *= tweak;
					gDelta.z *= tweak;

					theNode->Speed2D = MAX_ENEMY_BUMPERCAR_SPEED;
				}
			}
		}

			/* MOVE IT */

		gCoord.x += gDelta.x * fps;
		gCoord.z += gDelta.z * fps;


			/* DO COLLISION DETECT */

		DoBumperCarCollision(theNode);



	}

		/* UPDATE */

	UpdateBumperCar(theNode);
}



/******************* PLAYER MOVE BUMPER CAR *****************************/

static void PlayerMoveBumperCar(ObjNode *car)
{
float	r;
float	fps = gFramesPerSecondFrac;

			/*****************************************/
			/* JUST TRACK IT IF NOT BEING DRIVEN YET */
			/*****************************************/

	if (gPlayerInfo.objNode->Skeleton->AnimNum != PLAYER_ANIM_BUMPERCAR)		// double-check that player is in there
		car->CarBeingDriven = false;

	if (!car->CarBeingDriven)
	{
		if (gPlayerCar == car)
			gPlayerCar = nil;													// make sure this is nil if not driving

		if (gBumperCarGateBlown[car->CarID])									// only make car go away if gate blown
		{
			if (TrackTerrainItem(car))
			{
				gCarList[car->AreaNum][car->CarID] = nil;								// remove from car list
				DeleteObject(car);
				return;
			}
		}

			/* RESET TO ORIGINAL POSITION IF INVISIBLE */

		if (car->StatusBits & STATUS_BIT_ISCULLED)
		{
			car->Delta.x = car->Delta.z = 0;
			car->Coord = car->InitCoord;
			UpdateObjectTransforms(car);
			CalcObjectBoxFromNode(car);
		}

		return;
	}

				/***********************/
				/* CAR IS BEING DRIVEN */
				/***********************/

	GetObjectInfo(car);

	if (gBumperCarGateBlown[car->AreaNum])				// if power blown, then no control
	{
		gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;
	}
	else
	{
		if (gPlayerInfo.analogControlX || gPlayerInfo.analogControlZ)	// if player is attempting some control then reset this timer
		{
			gTimeSinceLastThrust = 0;
			gForceCameraAlignment = true;								// keep camera behind the car!
			gCameraUserRotY = 0;										// ... and reset user rot
		}
	}

			/* SNAP ANALOG ANGLE AGGRESSIVELY */

	{
		float angle = atan2f(gPlayerInfo.analogControlZ, gPlayerInfo.analogControlX);
		float magnitude = sqrtf(gPlayerInfo.analogControlZ*gPlayerInfo.analogControlZ + gPlayerInfo.analogControlX*gPlayerInfo.analogControlX);

		angle = SnapAngle(angle, OGLMath_DegreesToRadians(45/2.0f));

		gPlayerInfo.analogControlX = cosf(angle) * magnitude;
		gPlayerInfo.analogControlZ = sinf(angle) * magnitude;
	}


			/* CHECK USER CONTROLS FOR ROTATION */

	if (gPlayerInfo.analogControlX != 0.0f)																// see if spin
	{
		car->DeltaRot.y -= gPlayerInfo.analogControlX * fps * CONTROL_SENSITIVITY_BUMPERCAR_TURN;			// use x to rotate
		if (car->DeltaRot.y > PI2)																			// limit spin speed
			car->DeltaRot.y = PI2;
		else
		if (car->DeltaRot.y < -PI2)
			car->DeltaRot.y = -PI2;
	}

			/* APPLY FRICTION TO SPIN */
	else
	{
		if (car->DeltaRot.y > 0.0f)
		{
			car->DeltaRot.y -= 4.0f * fps;
			if (car->DeltaRot.y < 0.0f)
				car->DeltaRot.y = 0.0f;
		}
		else
		if (car->DeltaRot.y < 0.0f)
		{
			car->DeltaRot.y += 4.0f * fps;
			if (car->DeltaRot.y > 0.0f)
				car->DeltaRot.y = 0.0f;
		}
	}


			/* APPLY FRICTION TO DELTAS */

	ApplyFrictionToDeltas(BUMPERCAR_FRICTION, &gDelta);


				/* DO ROTATION */

	r = car->Rot.y += car->DeltaRot.y * fps;


				/* DO ACCELERATION */

	car->AccelVector.x = sin(r);
	car->AccelVector.y = cos(r);

	gDelta.x += car->AccelVector.x * (CONTROL_SENSITIVITY_BUMPERCAR_ACCEL * gPlayerInfo.analogControlZ * fps);
	gDelta.z += car->AccelVector.y * (CONTROL_SENSITIVITY_BUMPERCAR_ACCEL * gPlayerInfo.analogControlZ * fps);


				/* CALC SPEED */

	VectorLength2D(car->Speed2D, gDelta.x, gDelta.z);					// calc 2D speed value

	if (car->Speed2D > MAX_BUMPERCAR_SPEED)						// see if limit top speed
	{
		float	tweak;

		tweak = 1.0f / (car->Speed2D / MAX_BUMPERCAR_SPEED);

		gDelta.x *= tweak;
		gDelta.z *= tweak;

		car->Speed2D = MAX_BUMPERCAR_SPEED;
	}

			/* MOVE IT */

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;


		/* DO COLLISION DETECT */

	DoBumperCarCollision(car);


		/***************************/
		/* SEE IF PLAYER WANTS OUT */
		/***************************/

	if (GetNewNeedState(kNeed_Jump))
	{
		UpdateInput();				// NOTE:  we do this to clear out the jump key since the player's jump move call will be called on same frame and we don't want to jump-jet accidentally
		MorphToSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_JUMP, 5.0f);
		gDoJumpJetAtApex = false;
		gPlayerInfo.objNode->Delta.y = 2000.0f;
		gPlayerInfo.objNode->Coord.y = gCoord.y + 130.0f;

		StopBumperCars();

	}


		/* UPDATE */
		//
		// note: the player is then updated during the player's move call
		//

	UpdateBumperCar(car);

}


/***************** UPDATE BUMPER CAR ***********************/

static void UpdateBumperCar(ObjNode *theNode)
{
			/* NO AUDIO IF NO POWER */

	if (gBumperCarGateBlown[theNode->AreaNum])
	{
		StopAChannel(&theNode->EffectChannel);					// make sure no audio if no power
	}
	else
	{

				/* UPDATE AUDIO ONLY IF PLAYER IS DRIVING */

		if (gPlayerCar)
		{
			if (theNode->EffectChannel == -1)
				theNode->EffectChannel = PlayEffect3D(EFFECT_BUMPERHUM, &gCoord);
			else
			{
				float q = theNode->Speed2D / MAX_BUMPERCAR_SPEED;

				ChangeChannelRate(theNode->EffectChannel, 0x10000 + (float)0x8000 * q);
				Update3DSoundChannel(EFFECT_BUMPERHUM, &theNode->EffectChannel, &gCoord);
			}
		}
	}

	UpdateObject(theNode);
}


/******************* DO BUMPER CAR COLLISION **************************/

static void DoBumperCarCollision(ObjNode *theNode)
{
int		i,hitCount = 0;
ObjNode	*otherCar;
float	dist,speed;
OGLVector2D	vec2to1,move1,move2,bounce,bounce2;
float	otherX,otherZ;
u_long	ctype;
short	area = theNode->AreaNum;

			/************************/
			/* DO CAR-CAR COLLISION */
			/************************/


	for (i = 0; i < MAX_CARS; i++)
	{
		otherCar = gCarList[area][i];

		if ((otherCar == theNode) || (otherCar == nil))							// don't check against self or if blank
			continue;

		otherX = otherCar->Coord.x;
		otherZ = otherCar->Coord.z;

		dist = CalcDistance(gCoord.x, gCoord.z, otherX, otherZ);				// calc dist to the car

					/*****************************/
					/* SEE IF THEY HAVE COLLIDED */
					/*****************************/

		if (dist < BUMPER_CAR_RADIUSX2)
		{
						/* CALC OUR VECTORS */

			vec2to1.x = gCoord.x - otherX;										// calc vector from other to us
			vec2to1.y = gCoord.z - otherZ;
			FastNormalizeVector2D(vec2to1.x, vec2to1.y, &vec2to1, false);

			FastNormalizeVector2D(gDelta.x, gDelta.z, &move1, true);			// calc this and other motion vectors
			FastNormalizeVector2D(otherCar->Delta.x, otherCar->Delta.z, &move2, true);


					/* FIRST JUST MOVE SO IT'S NOT OVERLAPPING */

			gCoord.x = otherX + vec2to1.x * (BUMPER_CAR_RADIUSX2 * 1.01f);	// move me back so I'm not overlapping the other car (plus a little extra)
			gCoord.z = otherZ + vec2to1.y * (BUMPER_CAR_RADIUSX2 * 1.01f);


						/* CALC REFLECTION VECTORS */

			ReflectVector2D(&move1, &vec2to1, &bounce);							// reflect the motion vector of #1
			vec2to1.x = -vec2to1.x;												// flip reflection axis vector
			vec2to1.y = -vec2to1.y;
			ReflectVector2D(&move2, &vec2to1, &bounce2);						// reflect the motion vector of #2

			speed = (theNode->Speed2D * .5f) + (otherCar->Speed2D * .5f);			// swap some energy


					/* BOUNCE 1ST OBJECT (OUR CAR) */

			move1.x += bounce.x;												// average in bounce vector
			move1.y += bounce.y;
			move1.x -= bounce2.x;												// also average in the reverse boucne vector of #2
			move1.y -= bounce2.y;
			FastNormalizeVector2D(move1.x, move1.y, &move1, true);


			gDelta.x = move1.x * speed;											// calc new deltas from the deflected vector
			gDelta.z = move1.y * speed;


					/* BOUNCE HIT OBJECT OFF OF OUR CAR */

			move2.x += bounce2.x;												// average the bounce and the original move
			move2.y += bounce2.y;
			move2.x -= bounce.x;												// also average in the reverse boucne vector of #1
			move2.y -= bounce.y;
			FastNormalizeVector2D(move2.x, move2.y, &move1, true);

			otherCar->Delta.x = move2.x * speed;								// calc new deltas from the deflected vector
			otherCar->Delta.z = move2.y * speed;


			VectorLength2D(theNode->Speed2D, gDelta.x, gDelta.z);						// calc 2D speed value
			VectorLength2D(otherCar->Speed2D, otherCar->Delta.x, otherCar->Delta.z);

			if (hitCount == 0)
			{
				if (theNode->Speed2D > 100.0f)
				{
					float q = (theNode->Speed2D + otherCar->Speed2D) / (MAX_BUMPERCAR_SPEED * 2.0f);
					PlayEffect_Parms3D(EFFECT_BUMPERHIT, &gCoord, NORMAL_CHANNEL_RATE + (q * (float)0x3000), q);

					if (theNode == gPlayerCar || otherCar == gPlayerCar)
						Rumble(0.5f, 250);
				}
			}

					/* GOTTA RE-CHECK THEM ALL AGAIN */

			if (++hitCount < 3)
				i = -1;															// reset loop counter (-1 will go to 0 @ end of loop)
		}
	}


			/* HANDLE GENERAL COLLISION */

	if (theNode == gPlayerCar)
		ctype = CTYPE_FENCE|CTYPE_MISC|CTYPE_TRIGGER2;
	else
		ctype = CTYPE_FENCE|CTYPE_MISC;

	if (HandleCollisions(theNode, ctype, -.9f))
	{
		theNode->DeltaRot.y *= .7f;
	}
}


#pragma mark -

/******************* ALIGN PLAYER IN BUMPER CAR **********************/
//
// NOTE: player's coords are loaded into gCoord
//

void AlignPlayerInBumperCar(ObjNode *player)
{
	GAME_ASSERT(gPlayerCar);

	gCoord.x = gPlayerCar->Coord.x;
	gCoord.y = gPlayerCar->Coord.y + 90.0f;
	gCoord.z = gPlayerCar->Coord.z;


	player->Rot.y = gPlayerCar->Rot.y;
}


/****************** PLAYER TOUCHED ELECTRIC FLOOR ******************/

void PlayerTouchedElectricFloor(ObjNode *theNode)
{
	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_ZAPPED)			// see if already zapped
	{
		gExplodePlayerAfterElectrocute = true;
		return;
	}

			/* SPARKS */

	MakeSparkExplosion(gCoord.x, gCoord.y, gCoord.z, 200.0f, 1.0, PARTICLE_SObjType_WhiteSpark,0);

	if (theNode->Skeleton->AnimNum != PLAYER_ANIM_ZAPPED)
	{
		SetSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_ZAPPED);
		theNode->ZappedTimer = 1.7f;
		gExplodePlayerAfterElectrocute = true;
	}

	PlayEffect3D(EFFECT_ZAP, &gCoord);


			/* DISCHARGE A SUPERNOVA */

	if (gDischargeTimer <= 0.0f)
	{
		ObjNode	*novaObj;

		gNewObjectDefinition.genre		= CUSTOM_GENRE;
		gNewObjectDefinition.coord 		= gCoord;
		gNewObjectDefinition.flags 		= STATUS_BIT_NOZWRITES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG|STATUS_BIT_NOTEXTUREWRAP|
											STATUS_BIT_GLOW|STATUS_BIT_KEEPBACKFACES;
		gNewObjectDefinition.slot 		= SPRITE_SLOT-1;
		gNewObjectDefinition.moveCall 	= nil;
		novaObj = MakeNewObject(&gNewObjectDefinition);
		novaObj->CustomDrawFunction = DrawSuperNovaDischarge;

		gPlayerInfo.superNovaCharge = .6;
		CreateSuperNovaStatic(novaObj, gCoord.x, gCoord.y, gCoord.z, MAX_SUPERNOVA_DISCHARGES, MAX_SUPERNOVA_DIST);

		gDischargeTimer = 1.7;
	}
}


#pragma mark -

/************************ ADD TIRE BUMPER STRIP ****************************/

Boolean AddTireBumperStrip(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode* newObj = nil;
ObjNode* prev = nil;
short	numTires = itemPtr->parm[0];
float	aim = (float)itemPtr->parm[1] * (PI2/4.0f);
float	tx,tz,s,diameter,radius;
short	i;

	tx = x;
	tz = z;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOUD_ObjType_TireBumper;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 200;
	gNewObjectDefinition.rot 		= 0;
	s =gNewObjectDefinition.scale 		= 2.0;

	for (i = 0; i < numTires; i++)
	{
		gNewObjectDefinition.coord.x 	= tx;
		gNewObjectDefinition.coord.z 	= tz;
		gNewObjectDefinition.slot++;

		if (i == 0)
			gNewObjectDefinition.moveCall = MoveStaticObject;
		else
			gNewObjectDefinition.moveCall = nil;

		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		if (i == 0)														// first one is special
		{
			newObj->TerrainItemPtr = itemPtr;							// keep ptr to item list

			newObj->CType 			= CTYPE_MISC | CTYPE_BLOCKSHADOW;
			newObj->CBits			= CBITS_ALLSOLID;

			radius = newObj->BBox.max.x * s;
			diameter = radius * 2.0f;									// calc diameter of each tire

			switch(itemPtr->parm[1])
			{
				case	0:
						SetObjectCollisionBounds(newObj,  newObj->BBox.max.y * s, -100,
												-radius, radius,
												radius, -diameter * (float)numTires + radius);
						break;

				case	1:
						SetObjectCollisionBounds(newObj,  newObj->BBox.max.y * s, -100,
												-diameter * (float)numTires + radius, radius,
												radius, -radius);
						break;

				case	2:
						SetObjectCollisionBounds(newObj,  newObj->BBox.max.y * s, -100,
												-radius, radius,
												radius, diameter * (float)numTires + radius);
						break;

				case	3:
						SetObjectCollisionBounds(newObj,  newObj->BBox.max.y * s, -100,
												-radius, diameter * (float)numTires + radius,
												radius, -radius);
						break;
			}

		}
		else
			prev->ChainNode = newObj;



				/* NEXT COORD */

		tx -= sin(aim) * 230.0f;
		tz -= cos(aim) * 230.0f;

		prev = newObj;
	}


	return(true);
}


#pragma mark -


/************************* ADD BUMPERCAR POWER POST *********************************/

Boolean AddBumperCarPowerPost(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj,*bumper;
int		i;
short	area = itemPtr->parm[1];
short	id = itemPtr->parm[0];

				/***********************/
				/* MAKE GENERATOR POST */
				/***********************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOUD_ObjType_Generator;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveBumperCarPowerPost;
	gNewObjectDefinition.rot 		= 0;

	gNewObjectDefinition.scale 		= 1.1;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->GeneratorID = id;										// save ID #
	newObj->AreaNum = area;

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;			// note: don't set to trigger quite yet (do that after lower bumper is blown off)
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_BUMPERCARPOWERPOST;
	CreateCollisionBoxFromBoundingBox(newObj, .5, 1);

	if (gGeneratorBlownMode[area][id] == GENERATOR_MODE_HIT)		// it's an active trigger if lower bumper is gone
		newObj->CType |= CTYPE_TRIGGER2;

			/*********************************************/
			/* IF NOT BLOWN, MAKE SPARKLE & LOWER BUMPER */
			/*********************************************/

	if (gGeneratorBlownMode[area][id] != GENERATOR_MODE_BLOWN)
	{
		i = newObj->Sparkles[0] = GetFreeSparkle(newObj);				// get free sparkle slot
		if (i != -1)
		{
			gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_FLICKER;
			gSparkles[i].where.x = x;
			gSparkles[i].where.y = gNewObjectDefinition.coord.y + newObj->TopOff;
			gSparkles[i].where.z = z;

			gSparkles[i].color.r = 1;
			gSparkles[i].color.g = 1;
			gSparkles[i].color.b = 1;
			gSparkles[i].color.a = 1;

			gSparkles[i].scale = 300.0f;
			gSparkles[i].separation = 40.0f;

			gSparkles[i].textureNum = PARTICLE_SObjType_BlueGlint;
		}
	}
				/* IS BLOWN, SO DIM */
	else
	{
		newObj->ColorFilter = kDimmedPowerPostFilter;					// make dim
	}

	if (gGeneratorBlownMode[area][id] == GENERATOR_MODE_ON)
	{

			/*********************/
			/* MAKE LOWER BUMPER */
			/*********************/

		gNewObjectDefinition.type 		= CLOUD_ObjType_GeneratorBumper;
		gNewObjectDefinition.coord.y 	+= 80.0f;
		gNewObjectDefinition.slot++;
		gNewObjectDefinition.moveCall 	= nil;
		bumper = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->ChainNode = bumper;
		bumper->ChainHead = newObj;

		bumper->GeneratorID = id;										// save ID #
		bumper->AreaNum = area;

		bumper->BumperWobble = RandomFloat()*PI2;

				/* SET COLLISION STUFF */

		bumper->CType 			= CTYPE_TRIGGER2|CTYPE_MISC|CTYPE_BLOCKCAMERA;
		bumper->CBits			= CBITS_ALLSOLID;
		bumper->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
		bumper->Kind		 	= TRIGTYPE_BUMPERCARPOWERPOST;

		CreateCollisionBoxFromBoundingBox(bumper, 1, 1);

	}

	return(true);											// item was added
}


/************************ MOVE BUMPER CAR POWER POST **************************/

static void MoveBumperCarPowerPost(ObjNode *theNode)
{
ObjNode	*bumper = theNode->ChainNode;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

			/* UPDATE HUM */

	if (gGeneratorBlownMode[theNode->AreaNum][theNode->GeneratorID] == GENERATOR_MODE_BLOWN)	// dont hum if blown
		StopAChannel(&theNode->EffectChannel);
	else
	{
		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect3D(EFFECT_BUMPERPOLEHUM, &theNode->Coord);
		else
		{
			Update3DSoundChannel(EFFECT_BUMPERPOLEHUM, &theNode->EffectChannel, &theNode->Coord);
		}
	}

		/* UPDATE LOWER BUMPER WOBBLE */

	if (bumper)
	{
		bumper->BumperWobble += gFramesPerSecondFrac * 10.0f;

		bumper->Scale.x =
		bumper->Scale.y =
		bumper->Scale.z = theNode->Scale.x + (sin(bumper->BumperWobble) * .1f);

		UpdateObjectTransforms(bumper);
	}
}


/************** DO TRIGGER - BUMPER CAR POWER POST ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_BumperCarPowerPost(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
short	area = theNode->AreaNum;

#pragma unused(sideBits,whoNode)


	if (whoNode->Speed2D < (MAX_BUMPERCAR_SPEED * .95f))			// don't trigger unless going near full speed
	{
		if (whoNode->Speed2D > 150.0f)								// but make a sound if fast enough
			PlayEffect3D(EFFECT_BUMPERPOLETAP, &theNode->Coord);
		return(true);
	}

			/************************/
			/* SEE IF RAMMED BUMPER */
			/************************/

	MakeSparkExplosion(theNode->Coord.x, theNode->Coord.y + 70.0f, theNode->Coord.z, 300.0f, 1, PARTICLE_SObjType_RedSpark,0);

	if (theNode->Type == CLOUD_ObjType_GeneratorBumper)
	{
		ObjNode	*post = theNode->ChainHead;

		PlayEffect3D(EFFECT_BUMPERPOLEBREAK, &theNode->Coord);	// player power down effect


		post->ChainNode = nil;									// detach from the actual generator post
		post->CType |= CTYPE_TRIGGER2;							// activate post as a trigger now

		ExplodeGeometry(theNode, 400, SHARD_MODE_UPTHRUST|SHARD_MODE_HEAVYGRAVITY|SHARD_MODE_FROMORIGIN, 1, .7);
		DeleteObject(theNode);

		gGeneratorBlownMode[area][theNode->GeneratorID] = GENERATOR_MODE_HIT;
	}

			/*******************/
			/* RAMMED THE POST */
			/*******************/
	else
	{
		int i = theNode->Sparkles[0];

		if (i != -1)
		{
			DeleteSparkle(i);
			theNode->Sparkles[0] = -1;
			theNode->CType &= ~CTYPE_TRIGGER2;					// don't trigger again
		}

		gGeneratorBlownMode[area][theNode->GeneratorID] = GENERATOR_MODE_BLOWN;

		StopAChannel(&theNode->EffectChannel);					// stop the hum

		PlayEffect3D(EFFECT_BUMPERPOLEOFF, &theNode->Coord);	// player power down effect

		theNode->ColorFilter = kDimmedPowerPostFilter;			 // make dim

				/* SEE IF ALL BLOWN */

		gNumBlownGenerators[area]++;
		if (gNumBlownGenerators[area] >= gNumGenerators[area])
			gBumperCarGateBlown[area] = true;
	}
	return(true);
}



/******************** DRAW GENERATOR BEAMS *************************/

static void DrawGeneratorBeams(ObjNode *theNode)
{
short	i,i2;
float	x,y,z,x2,y2,z2,u,u2,yo;
short	a;

#pragma unused(theNode)


	gGlobalTransparency = .99;

	for (a = 0; a < MAX_AREAS; a++)
	{
		x = gGeneratorCoords[a][0].x;									// get coords of 1st generator
		y = gGeneratorCoords[a][0].y;
		z = gGeneratorCoords[a][0].z;

		for (i = 0; i < gNumGenerators[a]; i++)
		{
					/* GET COORDS OF NEXT GENERATOR */

			if (i == (gNumGenerators[a]-1))							// see if loop back
				i2 = 0;
			else
				i2 = i+1;

			x2 = gGeneratorCoords[a][i2].x;							// get next coords
			y2 = gGeneratorCoords[a][i2].y;
			z2 = gGeneratorCoords[a][i2].z;


			if ((gGeneratorBlownMode[a][i] != GENERATOR_MODE_BLOWN) && (gGeneratorBlownMode[a][i2] != GENERATOR_MODE_BLOWN))	// if either post is blown then skip
			{
					/* DRAW A QUAD */

				yo = 20.0f + RandomFloat() * 10.0f;

				u = RandomFloat() * 5.0f;
				u2 = u + 5.0f;

				MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_LEVELSPECIFIC][CLOUD_SObjType_BlueBeam].materialObject);

				glBegin(GL_QUADS);
				glTexCoord2f(u,0);			glVertex3f(x,y+yo,z);
				glTexCoord2f(u,1);			glVertex3f(x,y-yo,z);
				glTexCoord2f(u2,1);			glVertex3f(x2,y2-yo, z2);
				glTexCoord2f(u2,0);			glVertex3f(x2,y2+yo,z2);
				glEnd();
			}

					/* NEXT ENDPOINT */
			x = x2;
			y = y2;
			z = z2;

		}
	}

	gGlobalTransparency = 1.0;

}


#pragma mark -

/************************* ADD BUMPERCAR GATE *********************************/

Boolean AddBumperCarGate(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*post,*beam;
int		i,j;
short	area = itemPtr->parm[1];

static const OGLPoint3D	sparkles[10] =
{
	{-165.0f,	217.5f,	0},
	{-165.0f,	177.5f,	0},
	{-165.0f,	137.5f,	0},
	{-165.0f,	97.5f, 	0},
	{-165.0f,	57.5f, 	0},

	{165.0f,	217.5f,	0},
	{165.0f,	177.5f,	0},
	{165.0f,	137.5f,	0},
	{165.0f,	97.5f,	0},
	{165.0f,	57.5f,	0},
};

				/**************/
				/* MAKE POSTS */
				/**************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOUD_ObjType_BumperGatePosts;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-20;
	gNewObjectDefinition.moveCall 	= MoveBumperCarGate;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 3.5;
	post = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	post->TerrainItemPtr = itemPtr;								// keep ptr to item list

	post->AreaNum = area;

			/* SET COLLISION STUFF */

	post->CType 		= CTYPE_MISC|CTYPE_IMPENETRABLE|CTYPE_BLOCKCAMERA;			// note: don't set to trigger quite yet (do that after lower bumper is blown off)
	post->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(post, 1.2, 1);


	if (gBumperCarGateBlown[area])
	{
			post->TopOff = 40.0f;								// lower so just a small bump
			CalcObjectBoxFromNode(post);
	}
	else
	{

				/*****************/
				/* MAKE SPARKLES */
				/*****************/

		for (j = 0; j < 10; j++)
		{
			i = post->Sparkles[j] = GetFreeSparkle(post);				// get free sparkle slot
			if (i != -1)
			{
				gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_FLICKER|SPARKLE_FLAG_TRANSFORMWITHOWNER;
				gSparkles[i].where = sparkles[j];

				gSparkles[i].color.r = 1;
				gSparkles[i].color.g = 1;
				gSparkles[i].color.b = 1;
				gSparkles[i].color.a = 1;

				gSparkles[i].scale = 200.0f;
				gSparkles[i].separation = 40.0f;

				gSparkles[i].textureNum = PARTICLE_SObjType_BlueGlint;
			}
		}


			/**************/
			/* MAKE BEAMS */
			/**************/

		gNewObjectDefinition.type 		= CLOUD_ObjType_BumperGateBeams;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_GLOW | STATUS_BIT_KEEPBACKFACES | STATUS_BIT_NOZWRITES | STATUS_BIT_UVTRANSFORM;
		gNewObjectDefinition.slot++;
		gNewObjectDefinition.moveCall 	= nil;
		beam = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		post->ChainNode = beam;


		beam->ColorFilter.a = .99;
	}


	return(true);											// item was added
}


/********************* MOVE BUMPER CAR GATE *****************************/

static void MoveBumperCarGate(ObjNode *posts)
{
ObjNode	*beams = posts->ChainNode;
int		j,i;

	if (!gPlayerCar)											// if cars are active then keep this around so enemy cars far away can't get thru holes
	{
		if (TrackTerrainItem(posts))							// just check to see if it's gone
		{
			DeleteObject(posts);
			return;
		}
	}

	if (beams)
	{
		beams->TextureTransformU += gFramesPerSecondFrac * 1.1;

		beams->ColorFilter.a =.2f +  RandomFloat() * .79f;

		if (gBumperCarGateBlown[posts->AreaNum])							// see if deactivate
		{
				/* TURN OFF SPARKELS */

			for (j = 0; j < 10; j++)
			{
				i = posts->Sparkles[j];
				if (i != -1)
				{
					DeleteSparkle(i);
					posts->Sparkles[j] = -1;
				}
			}

				/* NUKE BEAMS */

			DeleteObject(beams);
			posts->ChainNode = nil;


				/* CHANGE COLLISION */

			posts->TopOff = 40.0f;								// lower so just a small bump
			CalcObjectBoxFromNode(posts);
		}
	}


}









