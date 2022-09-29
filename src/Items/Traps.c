/****************************/
/*   	TRAPS.C			    */
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

static void MoveFallingCrystal_Underground(ObjNode *theNode);
static void MoveFallingCrystal_Growing(ObjNode *theNode);
static void MoveFallingCrystal_Falling(ObjNode *theNode);
static void MoveSnowball(ObjNode *theNode);

static void MoveSoapBubble(ObjNode *bubble);
static void MoveInertBubble(ObjNode *bubble);
static void MoveSoapBubbleWithPlayer(ObjNode *bubble);
static void SetFallingCrystalCollisionBoxes(ObjNode *theNode);
static void ShatterCrystal(ObjNode *crystal);
static void MoveBubblePump(ObjNode *pump);
static void MakeSoapBubble(float x, float y, float z);

static void MoveMagnetMonsterOnSpline(ObjNode *theNode);
static void DrawMagnetMonsterProp(ObjNode *theNode);

static void MoveCrunchDoor(ObjNode *bottom);
static void MoveCrunchDoorTop(ObjNode *top);
static Boolean CrunchDoorHitBySuperNova(ObjNode *weapon, ObjNode *door, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void MoveManhole(ObjNode *theNode);
static void MakeManholeBlast(void);
static void MoveManholeBlast(ObjNode *theNode);

static void MoveProximityMine(ObjNode *theNode);
static Boolean ProximityMineHitByWeapon(ObjNode *weapon, ObjNode *mine, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void ExplodeProximityMine(ObjNode *mine);
static void MoveShockwave(ObjNode *theNode);
static void MoveConeBlast(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_PODS				10

#define	CRYSTAL_BEGIN_DIST		1000.0f

#define	FALLING_CRYSTAL_SCALE	2.5f

#define	SOAP_BUBBLE_SCALE		1.6f

#define	SNOWBALL_SCALE_START	1.5f
#define	SNOWBALL_SCALE_END		8.0f

enum
{
	PLUNGER_MODE_WAIT,
	PLUNGER_MODE_DOWN,
	PLUNGER_MODE_UP
};


enum
{
	CRUNCH_DOOR_MODE_CLOSED,
	CRUNCH_DOOR_MODE_OPENING,
	CRUNCH_DOOR_MODE_OPENED,
	CRUNCH_DOOR_MODE_CLOSING
};


enum
{
	MANHOLE_MODE_WAIT,
	MANHOLE_MODE_FLY,
	MANHOLE_MODE_SPIN
};

/*********************/
/*    VARIABLES      */
/*********************/

ObjNode	*gSoapBubble;

ObjNode	*gMagnetMonsterList[MAX_MAGNET_MONSTERS];

#define	PlungerMode			Special[0]
#define	SoapBubbleFree		Flag[0]


#define	OneWay				Flag[0]
#define	IsPowered			Flag[1]

#pragma mark -

/************************* ADD FALLING CRYSTAL *********************************/

Boolean AddFallingCrystal(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SLIME_ObjType_FallingCrystal_Blue + itemPtr->parm[1];
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_HIDDEN;		// start hidden!
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-1;
	gNewObjectDefinition.moveCall 	= MoveFallingCrystal_Underground;
	gNewObjectDefinition.rot 	= (float)itemPtr->parm[0] * (PI2/8.0f);			// use given rot
	gNewObjectDefinition.scale 		= FALLING_CRYSTAL_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;												// keep ptr to item list

	newObj->Flag[0] = itemPtr->parm[3] & 1;						// remember if aim at player when grow

	newObj->ColorFilter.a = .9;									// slightly transparent


	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_ALLSOLID;

	newObj->Rot.x = -PI/12.0f;									// tilt it just a little
	newObj->Coord.y -= newObj->BBox.max.y * FALLING_CRYSTAL_SCALE + 30.0f;				// hide underground

	UpdateObjectTransforms(newObj);

	SetFallingCrystalCollisionBoxes(newObj);					// set collision boxes
	KeepOldCollisionBoxes(newObj);

	return(true);												// item was added
}


/***************** SET FALLING CRYSTAL COLLISION BOXES ********************/

static void SetFallingCrystalCollisionBoxes(ObjNode *theNode)
{
static const OGLPoint3D	 baseOff = {0, 0, 40.4f};
static const OGLVector3D up = {0,1,0};
OGLVector3D				v;
OGLPoint3D				base;
float					w,w2,x,y,z,s;
int						numBoxes,i;
CollisionBoxType		*box;

			/* CALC VECTOR OF TILT & BASE POINT */

	OGLVector3D_Transform(&up, &theNode->BaseTransformMatrix, &v);
	OGLPoint3D_Transform(&baseOff, &theNode->BaseTransformMatrix, &base);


			/* CALC SIZE INFO */

	s = theNode->Scale.x;
	w = theNode->BBox.max.x * s * .7f;								// get the cubic size of each collision box
	w2 = w * 2.0f;

	numBoxes = (theNode->BBox.max.y * s / w2) + .5f;				// calc # collision boxes needed (round up)
	if (numBoxes > MAX_COLLISION_BOXES)
		numBoxes = MAX_COLLISION_BOXES;

	theNode->NumCollisionBoxes = numBoxes;						// set # boxes
	box = &theNode->CollisionBoxes[0];							// point to boxes



			/* SET ALL BOXES */

	x = base.x + v.x * w;						// move up to center 1st box
	y = base.y + v.y * w;
	z = base.z + v.z * w;


	for (i = 0; i < numBoxes; i++)
	{
		box[i].top 		= y + w;
		box[i].bottom 	= y - w;
		box[i].left 	= x - w;
		box[i].right 	= x + w;
		box[i].front 	= z + w;
		box[i].back 	= z - w;

		x += v.x * w2;
		y += v.y * w2;
		z += v.z * w2;

	}
}


/******************** MOVE FALLING CRYSTAL: UNDERGROUND **************************/

static void MoveFallingCrystal_Underground(ObjNode *theNode)
{
				/* TRACK IT */

	if (TrackTerrainItem(theNode))
	{
		DeleteObject(theNode);
		return;
	}


			/* SEE IF PLAYER CLOSE ENOUGH TO START GROWING */

	if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < CRYSTAL_BEGIN_DIST)
	{
		theNode->MoveCall = MoveFallingCrystal_Growing;
		theNode->StatusBits &= ~STATUS_BIT_HIDDEN;

		if (theNode->Flag[0])							// see if aim at player
		{
			theNode->Rot.y 	= CalcYAngleFromPointToPoint(0, theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
			UpdateObjectTransforms(theNode);
		}

		theNode->Delta.x = 0;
		theNode->Delta.y = 1;
		theNode->Delta.z = 0;
		OGLVector3D_Transform(&theNode->Delta, &theNode->BaseTransformMatrix, &theNode->Delta);
		theNode->Delta.x *= 550.0f;
		theNode->Delta.y *= 550.0f;
		theNode->Delta.z *= 550.0f;


	}
}


/******************** MOVE FALLING CRYSTAL: GROWING **************************/

static void MoveFallingCrystal_Growing(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	y;
				/* TRACK IT */

	if (TrackTerrainItem(theNode))
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	y = GetTerrainY(gCoord.x, gCoord.z);
	if (gCoord.y > y)
	{
		gCoord.y = y;
		gDelta.x = gDelta.y = gDelta.z = 0;
		theNode->MoveCall = MoveFallingCrystal_Falling;
		PlayEffect_Parms3D(EFFECT_CRYSTALCRACK, &gCoord, NORMAL_CHANNEL_RATE / 2, 1.0);
	}

	UpdateObject(theNode);

	SetFallingCrystalCollisionBoxes(theNode);
}



/******************** MOVE FALLING CRYSTAL: FALLING **************************/

static void MoveFallingCrystal_Falling(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		i;

				/* TRACK IT */

	if (TrackTerrainItem(theNode))
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);

			/* KEEL OVER */

	theNode->DeltaRot.x -= fps * 1.6f;
	theNode->Rot.x += theNode->DeltaRot.x * fps;

	if (theNode->Rot.x <= -PI/2)							// see if down
	{
		ShatterCrystal(theNode);
		return;
	}

	UpdateObject(theNode);
	SetFallingCrystalCollisionBoxes(theNode);


		/***********************/
		/* SEE IF CRUSH PLAYER */
		/***********************/

	if (gPlayerInfo.invincibilityTimer <= 0.0f)								// dont check if player is invincible
	{
		for (i = 0; i < theNode->NumCollisionBoxes; i++)
		{
			if (DoSimpleBoxCollisionAgainstPlayer(theNode->CollisionBoxes[i].top, theNode->CollisionBoxes[i].bottom,
												 theNode->CollisionBoxes[i].left, theNode->CollisionBoxes[i].right,
												theNode->CollisionBoxes[i].front, theNode->CollisionBoxes[i].back))
			{
				if (gPlayerInfo.objNode->Skeleton->AnimNum != PLAYER_ANIM_ACCORDIAN)
				{
					CrushPlayer();
					break;
				}
			}
		}
	}
}


/*************************** SHATTER CRYSTAL **************************************/

static void ShatterCrystal(ObjNode *crystal)
{
	ExplodeGeometry(crystal, 400, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .9);

	PlayEffect3D(EFFECT_CRYSTALCRASH, &crystal->Coord);

	DeleteObject(crystal);
}


#pragma mark -


/************************* ADD INERT SOAP BUBBLE *********************************/

Boolean AddInertBubble(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
float	y,ty;
Boolean	onWater;

	ty = GetTerrainY(x,z);

	onWater = GetWaterY(x, z, &y);					// see if on H2o
	if ((ty > y) || (!onWater))
		return(true);								// bad bubble cuz not on H2O


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SLIME_ObjType_HalfSoapBubble;
	gNewObjectDefinition.slot 		= WATER_SLOT+1;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_AIMATCAMERA|STATUS_BIT_GLOW|
									STATUS_BIT_NOFOG|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOZWRITES|STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.moveCall 	= MoveInertBubble;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->ColorFilter.a = .9;


	newObj->SpecialF[0] = RandomFloat2()*PI2;					// randomize wobble
	newObj->SpecialF[1] = RandomFloat2()*PI2;


	return(true);												// item was added
}


/************************ MOVE INERT BUBBLE **************************/

static void MoveInertBubble(ObjNode *bubble)
{
float	fps = gFramesPerSecondFrac;
float	s;
				/* TRACK IT */

	if (TrackTerrainItem(bubble))
	{
		DeleteObject(bubble);
		return;
	}

			/* MAKE WOBBLE */

	s = bubble->Scale.z;

	bubble->SpecialF[0] += fps * 10.0f;
	bubble->Scale.y = s + sin(bubble->SpecialF[0]) * .06f;

	bubble->SpecialF[1] -= fps * 14.0f;
	bubble->Scale.x = s + cos(bubble->SpecialF[1]) * .06f;

	UpdateObjectTransforms(bubble);
}

/*********************** ADD BUBBLE PUMP *********************/

Boolean AddBubblePump(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*pump, *plunger;
float	r = (float)itemPtr->parm[0] * (PI2/4.0f);

				/***************************/
				/* MAKE THE PUMP MECHANISM */
				/***************************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SLIME_ObjType_BubblePump;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.moveCall 	= MoveBubblePump;
	gNewObjectDefinition.rot 		= r;
	gNewObjectDefinition.scale 		= 2.0;
	pump = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	pump->TerrainItemPtr = itemPtr;								// keep ptr to item list

	pump->CType = CTYPE_MISC;
	pump->CBits = CBITS_ALLSOLID;
	SetObjectCollisionBounds(pump, 40,0,-40,40,40,-40);


			/********************/
			/* MAKE THE PLUNGER */
			/********************/

	gNewObjectDefinition.type 		= SLIME_ObjType_BubblePumpPlunger;
	gNewObjectDefinition.moveCall 	= nil;
	plunger = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	plunger->PlungerMode = PLUNGER_MODE_WAIT;


			/* SET COLLISION STUFF */

	plunger->CType 			= CTYPE_TRIGGER|CTYPE_MISC|CTYPE_BLOCKSHADOW|CTYPE_MPLATFORM;
	plunger->CBits			= CBITS_ALLSOLID;
	plunger->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	plunger->Kind		 	= TRIGTYPE_BUBBLEPUMP;

	CreateCollisionBoxFromBoundingBox_Rotated(plunger, 1,1);

	pump->ChainNode = plunger;
	plunger->ChainHead = pump;


	return(true);
}



/************** DO TRIGGER - BUBBLE PUMP ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_BubblePump(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
ObjNode	*body;

#pragma unused (sideBits, whoNode)

	if (theNode->PlungerMode != PLUNGER_MODE_WAIT)		// only if ready
		return(true);

	gDelta.x *= .2f;									// friction to keep from sliding
	gDelta.z *= .2f;
	gDelta.y = 400.0f;

	theNode->PlungerMode = PLUNGER_MODE_DOWN;

	body = theNode->ChainHead;							// get pump from plunger
	MakeSoapBubble(body->Coord.x, body->Coord.y + 130.0f, body->Coord.z);

	PlayEffect3D(EFFECT_AIRPUMP, &gCoord);
	return(true);
}


/*********************** MOVE BUBBLE PUMP ************************/

static void MoveBubblePump(ObjNode *pump)
{
ObjNode *plunger = pump->ChainNode;

			/* SEE IF GONE */

	if (TrackTerrainItem(pump))
	{
		DeleteObject(pump);
		return;
	}


		/* UPDATE PLUNGER */

	GetObjectInfo(plunger);

	switch(plunger->PlungerMode)
	{
		case	PLUNGER_MODE_DOWN:
				gDelta.y = -80.0f;
				gCoord.y += gDelta.y * gFramesPerSecondFrac;
				if (gCoord.y <= (pump->Coord.y - 50.0f))				// see if all the way down
				{
					gCoord.y = pump->Coord.y - 50.0f;
					plunger->PlungerMode = PLUNGER_MODE_UP;
				}
				break;

		case	PLUNGER_MODE_UP:
				gDelta.y = 80.0f;
				gCoord.y += gDelta.y * gFramesPerSecondFrac;
				if (gCoord.y >= plunger->InitCoord.y)				// see if all the way up
				{
					gCoord.y = plunger->InitCoord.y;
					plunger->PlungerMode = PLUNGER_MODE_WAIT;
					gDelta.y = 0;
				}
				break;

	}


	UpdateObject(plunger);
}



/************************ MAKE SOAP BUBBLE *************************/

static void MakeSoapBubble(float x, float y, float z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SLIME_ObjType_SoapBubble;
	gNewObjectDefinition.slot 		= WATER_SLOT+1;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_AIMATCAMERA|STATUS_BIT_GLOW|
									STATUS_BIT_NOFOG|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOZWRITES|STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.moveCall 	= MoveSoapBubble;
	gNewObjectDefinition.scale 		= SOAP_BUBBLE_SCALE * .1f;
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->ColorFilter.a = .9;

	newObj->SpecialF[0] = RandomFloat2()*PI2;					// randomize wobble
	newObj->SpecialF[1] = RandomFloat2()*PI2;

	newObj->SoapBubbleFree = false;								// still growing, not free yet

	newObj->Health = 15;										// set life timer before pop


				/* MAKE SHADOW */

	AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 10,10, false);

	CreateCollisionBoxFromBoundingBox_Maximized(newObj);
}


/************************ MOVE SOAP BUBBLE **************************/

static void MoveSoapBubble(ObjNode *bubble)
{
float	fps = gFramesPerSecondFrac;
float	s,avgX,avgY,avgZ;

				/* TRACK IT */

	if (TrackTerrainItem(bubble))
	{
		DeleteObject(bubble);
		return;
	}

	GetObjectInfo(bubble);

			/* GROW TO FULL SIZE */

	s = bubble->Scale.z;								// use z as scale basis since it doesnt wobble
	if (s < SOAP_BUBBLE_SCALE)
	{
		s += fps;										// grow
		if (s >= SOAP_BUBBLE_SCALE)						// see if full size
		{
			s = SOAP_BUBBLE_SCALE;
			bubble->SoapBubbleFree = true;				// its free now

			gDelta.x = RandomFloat2() * 80.0f;			// set random drift
			gDelta.z = RandomFloat2() * 80.0f;
			gDelta.y = 100.0f + RandomFloat() * 100.0f;

			CreateCollisionBoxFromBoundingBox_Maximized(bubble);		// set collision box now that @ full size
		}
		bubble->Scale.z = s;
	}

			/***************/
			/* MAKE WOBBLE */
			/***************/

	bubble->SpecialF[0] += fps * 10.0f;
	bubble->Scale.y = s + sin(bubble->SpecialF[0]) * .06f;

	bubble->SpecialF[1] -= fps * 14.0f;
	bubble->Scale.x = s + cos(bubble->SpecialF[1]) * .06f;


			/**************/
			/* FREE FLOAT */
			/**************/

	if (bubble->SoapBubbleFree)
	{
		gDelta.y -= 60.0f * fps;								// add slight gravity to it

		gCoord.x += gDelta.x * fps;
		gCoord.y += gDelta.y * fps;
		gCoord.z += gDelta.z * fps;

		HandleCollisions(bubble, CTYPE_MISC|CTYPE_TERRAIN|CTYPE_FENCE, -.5);	// do collision just to bounce it



		bubble->Health -= fps;									// see if pop
		if (bubble->Health <= 0.0f)
		{
			PopSoapBubble(bubble);
			return;
		}
	}

			/* STILL GROWING */
	else
	{
		gCoord.y += 140.0f * fps;								// move up as it grows
	}

			/**********************/
			/* SEE IF GRAB PLAYER */
			/**********************/

	if (CalcDistance3D(gCoord.x, gCoord.y, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.y, gPlayerInfo.coord.z) < 150.0f)
	{
		MorphToSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_BUBBLE, 8.0);

				/* MOVE BUBBLE & PLAYER TOGETHER @ AVERAGE POINT */

		avgX = (gCoord.x + gPlayerInfo.coord.x) * .5f;
		avgY = (gCoord.y + gPlayerInfo.coord.y) * .5f;
		avgZ = (gCoord.z + gPlayerInfo.coord.z) * .5f;

		gCoord.x = gPlayerInfo.objNode->Coord.x = avgX;
		gCoord.y = gPlayerInfo.objNode->Coord.y = avgY;
		gCoord.z = gPlayerInfo.objNode->Coord.z = avgZ;

		bubble->MoveCall = MoveSoapBubbleWithPlayer;

		bubble->Health = 40.0f;							// reset life timer

		gSoapBubble = bubble;
	}

	UpdateObject(bubble);
}



/********************** MOVE SOAP BUBBLE WITH PLAYER **************************/

static void MoveSoapBubbleWithPlayer(ObjNode *bubble)
{
float	fps = gFramesPerSecondFrac;
float	s;

			/* CHECK TIMER & USER ABORT */

	bubble->Health -= fps;
	if ((bubble->Health <= 0.0f)
		|| GetNewNeedState(kNeed_Jump)
		|| GetNewNeedState(kNeed_Shoot)
		|| GetNewNeedState(kNeed_PunchPickup))
	{
		PopSoapBubble(bubble);
		return;
	}
	else
	if (bubble->Health < 4.0f)			// visual warning before pop
		bubble->ColorFilter.g = bubble->ColorFilter.b = bubble->Health * .25f;	// fade to red

			/* MAKE WOBBLE */

	s = SOAP_BUBBLE_SCALE;

	bubble->SpecialF[0] += fps * 10.0f;
	bubble->Scale.y = s + sin(bubble->SpecialF[0]) * .06f;

	bubble->SpecialF[1] -= fps * 14.0f;
	bubble->Scale.x = s + cos(bubble->SpecialF[1]) * .06f;

}


/***************** POP SOAP BUBBLE ****************************/

void PopSoapBubble(ObjNode *bubble)
{
long					pg,i;
OGLVector3D				delta;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
float					x,y,z;

			/*************************/
			/* MAKE BUBBLE PARTICLES */
			/*************************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
	gNewParticleGroupDef.gravity				= 1000;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 7;
	gNewParticleGroupDef.decayRate				=  .3;
	gNewParticleGroupDef.fadeRate				= .7;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_Bubble;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		x = bubble->Coord.x;
		y = bubble->Coord.y;
		z = bubble->Coord.z;

		for (i = 0; i < 50; i++)
		{
			delta.x = RandomFloat2() * 400.0f;
			delta.y = RandomFloat2() * 400.0f;
			delta.z = RandomFloat2() * 400.0f;

			pt.x = x + delta.x * .15f;
			pt.y = y + delta.y * .15f;
			pt.z = z + delta.z * .15f;

			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= 1.0f + RandomFloat() * .5f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA;
			if (AddParticleToGroup(&newParticleDef))
				break;
		}
	}



			/* FINAL STUFF */

	PlayEffect3D(EFFECT_BUBBLEPOP, &bubble->Coord);

	if (bubble == gSoapBubble)			// see if popped the one we were riding
	{
		gSoapBubble = nil;
		MorphToSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_FALL, 5.0);
	}

	DeleteObject(bubble);
}


#pragma mark -

/************************ PRIME MAGNET MONSTER *************************/

Boolean PrimeMagnetMonster(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj, *prop;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/***************/
				/* MAKE OBJECT */
				/***************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SLIME_ObjType_MagnetMonster;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_ONSPLINE;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 4.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;


				/* SET BETTER INFO */

	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveMagnetMonsterOnSpline;		// set move call


			/* SET SOME CUSTOM STUFF */

	newObj->MagnetMonsterID = itemPtr->parm[0];					// remember ID#

	GAME_ASSERT_MESSAGE(newObj->MagnetMonsterID < MAX_MAGNET_MONSTERS, "Too many magnet monsters!");
	gMagnetMonsterList[newObj->MagnetMonsterID] = newObj;		// keep in a list

	newObj->MagnetMonsterMoving = false;
	newObj->MagnetMonsterResetToStart = false;
	newObj->MagnetMonsterWatchPlayerRespawn = false;
	newObj->MagnetMonsterSpeed = 0;
	newObj->MagnetMonsterInitialPlacement = placement;			// remember where started


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	AddToSplineObjectList(newObj, true);



			/******************/
			/* MAKE PROPELLER */
			/******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SLIME_ObjType_MagnetMonsterProp;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_ROTXZY;
	gNewObjectDefinition.slot		= WATER_SLOT + 1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 4.0;
	prop = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	prop->CustomDrawFunction = DrawMagnetMonsterProp;				// override and use custom draw function

	newObj->ChainNode = prop;


			/* DETACH FROM LINKED LIST */

	DetachObject(newObj, true);

	return(true);
}


/************** RESET MAGNET MONSTER TO INITIAL POSITION ON SPLINE **************/

static void ResetMagnetMonsterPosition(ObjNode* theNode)
{
	theNode->SplinePlacement = theNode->MagnetMonsterInitialPlacement;
	theNode->MagnetMonsterMoving = false;
	theNode->MagnetMonsterResetToStart = false;
	theNode->MagnetMonsterWatchPlayerRespawn = false;
	theNode->MagnetMonsterSpeed = 0;
	GetObjectCoordOnSpline(theNode);
	SetSplineAim(theNode);
	UpdateObjectTransforms(theNode);																// update transforms

	if (theNode->EffectChannel != -1)
		StopAChannelIfEffectNum(&theNode->EffectChannel, EFFECT_SLIMEBOAT);
}


/******************** MOVE MAGNET MONSTER ON SPLINE ***************************/

static void MoveMagnetMonsterOnSpline(ObjNode *theNode)
{
Boolean isVisible;
float	fps = gFramesPerSecondFrac;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/*************************/
		/* MOVE ALONG THE SPLINE */
		/*************************/

	if (theNode->MagnetMonsterMoving)
	{
				/* ALWAYS RESET MY POSITION AFTER PLAYER DIES & RESPAWNS */

		if (gPlayerIsDead)
		{
			theNode->MagnetMonsterWatchPlayerRespawn = true;	// wait on player to respawn before resetting position
		}
		else if (theNode->MagnetMonsterWatchPlayerRespawn)		// if player just respawned, reset position
		{
			ResetMagnetMonsterPosition(theNode);
			return;
		}


				/* UPDATE SOUND */

		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect3D(EFFECT_SLIMEBOAT, &theNode->Coord);
		else
			Update3DSoundChannel(EFFECT_SLIMEBOAT, &theNode->EffectChannel, &theNode->Coord);


		theNode->MagnetMonsterSpeed += fps * 250.0f;			// increase speed
		if (theNode->MagnetMonsterSpeed > 460.0f)				// see if @ max speed
			theNode->MagnetMonsterSpeed = 460.0f;

		if (theNode->MagnetMonsterResetToStart)					// see if want to stop when back @ start index
		{
			float	oldPlacement = theNode->SplinePlacement;
			IncreaseSplineIndex(theNode, theNode->MagnetMonsterSpeed);

			// TODO: this condition seems buggy -- first magnet monster in level 2 seems to loop around placement 0.5?
			if ((theNode->SplinePlacement >= theNode->MagnetMonsterInitialPlacement) && (oldPlacement < theNode->MagnetMonsterInitialPlacement))
			{
				ResetMagnetMonsterPosition(theNode);
				return;
			}
		}
		else													// just keep moving normally
		{
			IncreaseSplineIndex(theNode, theNode->MagnetMonsterSpeed);
		}

		GetObjectCoordOnSpline(theNode);
	}

			/* NOT MOVING, SO BE SURE SOUND IS OFF */
	else
	{
		if (theNode->EffectChannel != -1)
			StopAChannelIfEffectNum(&theNode->EffectChannel, EFFECT_SLIMEBOAT);
	}


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/

	if (isVisible)
	{
		ObjNode *prop;
		static const OGLPoint3D propOff = {0, -19, 107};


					/* UPDATE MONSTER */

		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		GetWaterY(theNode->Coord.x, theNode->Coord.z, &theNode->Coord.y);								// get water y coord
		theNode->Coord.y += 60.0f;
		UpdateObjectTransforms(theNode);																// update transforms


					/* UPDATE PROPELLER & SPRAY */

		prop = theNode->ChainNode;														// get prop obj
		OGLPoint3D_Transform(&propOff, &theNode->BaseTransformMatrix, &prop->Coord);	// set prop coords

		if (theNode->MagnetMonsterMoving)
		{
			prop->Rot.z -= fps * 14.0f;													// spin propeller
			SprayWater(theNode, prop->Coord.x, prop->Coord.y, prop->Coord.z);			// spray H2o
		}

		prop->Rot.y = theNode->Rot.y;													// match yrot

		UpdateObjectTransforms(prop);
	}

			/**********************************************************/
			/* NOT VISIBLE, SO SEE IF NEED TO RESET TO START POSITION */
			/**********************************************************/
	else
	{
		if (theNode->MagnetMonsterResetToStart)					// see if want to stop when back @ start index
		{
			ResetMagnetMonsterPosition(theNode);
		}
	}
}


/*********************** DRAW MAGNET MONSTER PROPELLER ****************************/

static void DrawMagnetMonsterProp(ObjNode *theNode)
{
OGLPoint3D	targetCoord,points[10*2];
float		dx,dy,dz,x,y,z,u;
int			i,j;
OGLTextureCoord	uvs[10*2];
static MOTriangleIndecies triangles[9*2] =
{
	{{ 0, 1, 2}},	{{ 2, 1, 3}},
	{{ 2, 3, 4}},	{{ 4, 3, 5}},
	{{ 4, 5, 6}},	{{ 6, 5, 7}},
	{{ 6, 7, 8}},	{{ 8, 7, 9}},
	{{ 8, 9,10}},	{{10, 9,11}},
	{{10,11,12}},	{{12,11,13}},
	{{12,13,14}},	{{14,13,15}},
	{{14,15,16}},	{{16,15,17}},
	{{16,17,18}},	{{18,17,19}},
};

MOVertexArrayData	mesh;

			/* DRAW THE PROPELLER OBJECT NORMALLY */

	if (theNode->BaseGroup)
	{
		MO_DrawObject(theNode->BaseGroup);
	}


			/************************/
			/* DRAW MAGNETIC EFFECT */
			/************************/

	if ((gPlayerInfo.objNode->Skeleton->AnimNum != PLAYER_ANIM_PICKUPANDHOLDMAGNET) || (!gPlayerInfo.objNode->IsHoldingPickup) || (gTargetPickup == nil))
		return;


	OGL_PushState();								// keep state

	targetCoord = theNode->Coord;									// get coord of propeller
	targetCoord.y += 20.0f;

	x = gTargetPickup->Coord.x;										// get coord of magnet
	y = gTargetPickup->Coord.y;
	z = gTargetPickup->Coord.z;


		/* BUILD STATIC SECTION POINTS */

	dx = (targetCoord.x - x) / 9.0f;								// calc vector from L to R
	dy = (targetCoord.y - y) / 9.0f;
	dz = (targetCoord.z - z) / 9.0f;


	u = 0;

	j = 0;
	for (i = 0; i < 10; i++)												// build the point list
	{
		float yoff;

		if (i > 0)
			yoff = RandomFloat2() * 20.0f;
		else
			yoff = 0;

		points[j].x = x;
		points[j].y = y + yoff + 9.0f;
		points[j].z = z;
		uvs[j].u = u;
		uvs[j].v = 1;
		j++;
		points[j].x = x;
		points[j].y = y + yoff -9.0f;
		points[j].z = z;
		uvs[j].u = u;
		uvs[j].v = 0;
		j++;

		x += dx;
		y += dy;
		z += dz;
		u += 1.0f / 9.0f;

	}


			/******************/
			/* BUILD THE MESH */
			/******************/

	mesh.numMaterials 	= -1;
	mesh.numPoints 		= 10*2;
	mesh.numTriangles 	= 9*2;
	mesh.points			= (OGLPoint3D *)points;
	mesh.normals		= nil;
	mesh.uvs[0]			= (OGLTextureCoord *)uvs;
	mesh.colorsByte		= nil;
	mesh.colorsFloat	= nil;
	mesh.triangles		= triangles;


			/***********/
			/* DRAW IT */
			/***********/

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_CULL_FACE);
	OGL_DisableLighting();


	gGlobalTransparency = .3;
	MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_MagnetRay].materialObject);
	MO_DrawGeometry_VertexArray(&mesh);
	gGlobalTransparency = 1.0f;


	OGL_PopState();								// keep state

}


#pragma mark -

/************************* ADD CRUNCH DOOR *********************************/

Boolean AddCrunchDoor(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*bottom,*top;

					/***************/
					/* MAKE BOTTOM */
					/***************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= APOCALYPSE_ObjType_CrunchDoor_Bottom;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY_Undeformed(x,z) - 30.0f;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveCrunchDoor;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI/4.0f);
	gNewObjectDefinition.scale 		= 5.0;
	bottom = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	bottom->TerrainItemPtr = itemPtr;							// keep ptr to item list

	bottom->CType 			= CTYPE_MISC|CTYPE_LIGHTNINGROD|CTYPE_BLOCKRAYS;
	bottom->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(bottom, 	1, .7);

	bottom->OneWay = (itemPtr->parm[3] & 1);					// see if its a 1-way door

	bottom->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= CrunchDoorHitBySuperNova;

					/************/
					/* MAKE TOP */
					/************/

	gNewObjectDefinition.type 		= APOCALYPSE_ObjType_CrunchDoor_Top;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= MoveCrunchDoorTop;
	top = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	top->Mode = CRUNCH_DOOR_MODE_CLOSED;						// init mode
	top->Timer = 2.0;

	bottom->ChainNode = top;									// link w/ bottom
	top->ChainHead = bottom;

	top->CType 			= CTYPE_MISC|CTYPE_IMPENETRABLE|CTYPE_BLOCKCAMERA|CTYPE_BLOCKRAYS;
	top->CBits			= CBITS_ALLSOLID;
	top->TriggerSides 	= SIDE_BITS_BOTTOM;				// side(s) to activate it
	top->Kind		 	= TRIGTYPE_CRUNCHDOOR;
	CreateCollisionBoxFromBoundingBox_Rotated(top, 	1, 1);



	return(true);
}

/************** DO TRIGGER - CRUNCH DOOR ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_CrunchDoor(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
#pragma unused(theNode, whoNode, sideBits)

	KillPlayer(PLAYER_DEATH_TYPE_EXPLODE);
	return(true);
}


/************** CRUNCH DOOR HIT BY SUPERNOVA *****************/

static Boolean CrunchDoorHitBySuperNova(ObjNode *weapon, ObjNode *door, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
ObjNode	*top = door->ChainNode;
int	i,j;
static const OGLPoint3D eyes[] =
{
	{-35, 89, 0},
	{35, 89,0},
};

#pragma unused(weapon, weaponCoord, weaponDelta)

	if (door->IsPowered)
		return(false);


			/* ONE-WAY DOORS CAN ONLY BE ACTIVATED FROM THE CORRECT SIDE */

	if (door->OneWay)									// if one-way door, then see if player is in front of it
	{
		OGLVector2D	v1,v2;
		float		r = door->Rot.y;

		v1.x = gPlayerInfo.coord.x - door->Coord.x;		// vector to player
		v1.y = gPlayerInfo.coord.z - door->Coord.z;
		FastNormalizeVector2D(v1.x, v1.y, &v1, true);
		v2.x = -sin(r);									// vector of aim
		v2.y = -cos(r);

		if (OGLVector2D_Dot(&v1,&v2) < 0.0f)			// if player is behind, then dont do anything
		{
			DisplayHelpMessage(HELP_MESSAGE_DOORSIDE, 4.0f, true);
			return(false);
		}
	}


				/*  ACTIVATE THE DOOR */

	door->IsPowered = true;

	DisableHelpType(HELP_MESSAGE_NEEDELECTRICITY);


				/* MAKE SPARKLES */

	for (j = 0; j < 2; j++)
	{
		i = top->Sparkles[j] = GetFreeSparkle(top);				// get free sparkle slot
		if (i != -1)
		{
			gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_TRANSFORMWITHOWNER;
			gSparkles[i].where = eyes[j];

			gSparkles[i].color.r = 1;
			gSparkles[i].color.g = 1;
			gSparkles[i].color.b = 1;
			gSparkles[i].color.a = 1;

			gSparkles[i].scale = 60.0f;

			gSparkles[i].separation = 80.0f;

			gSparkles[i].textureNum = PARTICLE_SObjType_GreenSpark;
		}
	}

	return(false);
}


/*********************** MOVE CRUNCH DOOR ************************/

static void MoveCrunchDoor(ObjNode *bottom)
{
				/* TRACK IT */

	if (!bottom->IsPowered)											// never delete once powered
	{
		if (TrackTerrainItem(bottom))
		{
			DeleteObject(bottom);
			return;
		}
	}


			/* DO HELP */

	if (CalcQuickDistance(bottom->Coord.x, bottom->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < HELP_BEACON_RANGE)
	{
		if (bottom->OneWay)									// if one-way door, then see if player is in front of it
		{
			OGLVector2D	v1,v2;
			float		r = bottom->Rot.y;

			v1.x = gPlayerInfo.coord.x - bottom->Coord.x;	// vector to player
			v1.y = gPlayerInfo.coord.z - bottom->Coord.z;
			FastNormalizeVector2D(v1.x, v1.y, &v1, true);
			v2.x = -sin(r);									// vector of aim
			v2.y = -cos(r);

			if (OGLVector2D_Dot(&v1,&v2) < 0.0f)			// if player is behind, then tell about it
				DisplayHelpMessage(HELP_MESSAGE_DOORSIDE, 4.0f, true);
			else
				DisplayHelpMessage(HELP_MESSAGE_NEEDELECTRICITY, 4.0f, true);
		}
		else
			DisplayHelpMessage(HELP_MESSAGE_NEEDELECTRICITY, 4.0f, true);
	}
}


/******************* MOVE CRUNCH DOOR TOP ************************/

static void MoveCrunchDoorTop(ObjNode *top)
{
ObjNode	*bottom = top->ChainHead;
float	fps = gFramesPerSecondFrac;

	top->CType &= ~CTYPE_TRIGGER;						// assume not a trigger

	GetObjectInfo(top);

	switch(top->Mode)
	{
				/**********/
				/* CLOSED */
				/**********/

		case	CRUNCH_DOOR_MODE_CLOSED:
				if (!bottom->IsPowered)								// dont do anything if no power to it
					break;

				if (bottom->OneWay)									// if one-way door, then see if player is in front of it
				{
					OGLVector2D	v1,v2;
					float		r = bottom->Rot.y;

					v1.x = gPlayerInfo.coord.x - gCoord.x;			// vector to player
					v1.y = gPlayerInfo.coord.z - gCoord.z;
					FastNormalizeVector2D(v1.x, v1.y, &v1, true);
					v2.x = -sin(r);									// vector of aim
					v2.y = -cos(r);

					if (OGLVector2D_Dot(&v1,&v2) < 0.0f)			// if player is behind, then dont do anything
						break;
				}

				top->Timer -= fps;									// dec timer to see if should open
				if (top->Timer <= 0.0f)
				{
					top->Mode = CRUNCH_DOOR_MODE_OPENING;
					gDelta.y = 500.0f;
				}
				break;

				/***********/
				/* OPENING */
				/***********/

		case	CRUNCH_DOOR_MODE_OPENING:
				gCoord.y += gDelta.y * fps;						// move up
				if (gCoord.y > (bottom->Coord.y + 400.0f))		// see if @ top of opening
				{
					top->Mode = CRUNCH_DOOR_MODE_OPENED;		// keep opened for a while
					top->Timer = 2.0;
					gDelta.y = 0;

					bottom->OneWay = false;						// no longer a 1-way door
				}
				else
				{
					gDelta.y -= fps * 2500.0f;					// make it jerky by slowing delta & restarting
					if (gDelta.y <= 0.0f)
					{
						gDelta.y = 500.0f;
						PlayEffect3D(EFFECT_DOORCLANKOPEN, &gCoord);
					}
				}
				break;


				/**********/
				/* OPENED */
				/**********/

		case	CRUNCH_DOOR_MODE_OPENED:
				top->Timer -= fps;					// dec timer to see if should close
				if (top->Timer <= 0.0f)
				{
					top->Mode = CRUNCH_DOOR_MODE_CLOSING;
					gDelta.y = 0.0f;
				}
				break;


				/***********/
				/* CLOSING */
				/***********/

		case	CRUNCH_DOOR_MODE_CLOSING:
				top->CType |= CTYPE_TRIGGER;				// trigger only while closing
				gDelta.y -= 6500.0f * fps;
				gCoord.y += gDelta.y * fps;					// move it
				if (gCoord.y <= bottom->Coord.y)			// see if hit bottom
				{
					gCoord.y = bottom->Coord.y;
					gDelta.y *= -.5f;						// try to bounce it
					if (fabs(gDelta.y) < 200.0f)			// if slow then just stop
					{
						gDelta.y = 0;
						top->Mode = CRUNCH_DOOR_MODE_CLOSED;		// keep closed for a while
						top->Timer = 3.0;
					}
					else
						PlayEffect3D(EFFECT_DOORCLANKCLOSE, &gCoord);

				}
				break;
	}

	UpdateObject(top);
}


#pragma mark -


/************************* ADD MANHOLE *********************************/

Boolean AddManhole(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= APOCALYPSE_ObjType_Manhole;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY_Undeformed(x,z) + 300.0f;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_ROTZXY;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveManhole;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;							// keep ptr to item list

	newObj->Rot.x += PI/2;										// rot flat
	UpdateObjectTransforms(newObj);

	newObj->Mode = MANHOLE_MODE_WAIT;
	newObj->Timer = RandomFloat() * 4.0f;				// set random timer

	return(true);
}


/************************* MOVE MANHOLE **********************************/

static void MoveManhole(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	dx, dy, q = PI/2;
OGLBoundingBox	bbox;
float	y,bottom;


				/* TRACK IT */

	if (TrackTerrainItem(theNode))
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);


			/***********/
			/* MOVE IT */
			/***********/

	gDelta.y -= 4500.0f * fps;
	gCoord.y += gDelta.y * fps;



	switch(theNode->Mode)
	{
		case	MANHOLE_MODE_WAIT:
				theNode->Timer -= fps;						// see if blow
				if (theNode->Timer <= 0.0f)
				{
					theNode->Timer = 3.0;
					theNode->Mode = MANHOLE_MODE_FLY;
					gDelta.y = 3000.0f;
					MakeManholeBlast();
				}
				break;


		case	MANHOLE_MODE_FLY:
				theNode->Rot.x += 15.0f * fps;					// spin
				if (theNode->Rot.x >= PI2)						// wrap rot instead if making it get big
					theNode->Rot.x -= PI2;

				if (theNode->StatusBits & STATUS_BIT_ONGROUND)
				{
					theNode->Mode = MANHOLE_MODE_SPIN;
					if (theNode->Rot.x > PI)					// keep from rotating over 180 degrees!
						theNode->Rot.x -= PI;
					PlayEffect3D(EFFECT_MANHOLEROLL, &gCoord);
				}
				break;



		default:
						/****************/
						/* DO COIN SPIN */
						/****************/

				dx = q - theNode->Rot.x;
				if (dx != 0.0f)
				{
							/* SPIN ON Y & Z */

					theNode->Rot.y += dy = (q - fabs(dx)) * -25.0f * fps;
					theNode->Rot.z += dy * .8f;							// partially counter-rot on z to let wobble w/o spin on y

							/* ROTATE X TOWARD FLAT */

					if (dx < 0.0f)
					{
						theNode->Rot.x -= fps * .4f;
						if (theNode->Rot.x < q)
						{
							theNode->Rot.x = q;
							theNode->Mode = MANHOLE_MODE_WAIT;
						}
					}
					else
					{
						theNode->Rot.x += fps * .4f;
						if (theNode->Rot.x > q)
						{
							theNode->Rot.x = q;
							theNode->Mode = MANHOLE_MODE_WAIT;
						}
					}
				}


				break;
	}


			/***************************/
			/* UPDATE BBOX & COLLISION */
			/***************************/

	theNode->Coord = gCoord;
	UpdateObjectTransforms(theNode);							// update the matrix

	OGLBoundingBox_Transform(&gObjectGroupBBoxList[theNode->Group][theNode->Type],
							&theNode->BaseTransformMatrix,
							&bbox);

	y = GetTerrainY(gCoord.x, gCoord.z);						// get terrain y
	bottom = bbox.min.y;										// get bottom most point on manhole

	if ((bottom <= y) && (gDelta.y < 0.0f))
	{
		dy = gCoord.y - bottom;									// get dist from center to bottom
		gCoord.y = y + dy;										// keep flush on ground
		gDelta.y = -100.0f;

		theNode->StatusBits |= STATUS_BIT_ONGROUND;
	}
	else
		theNode->StatusBits &= ~STATUS_BIT_ONGROUND;			// not on ground


	UpdateObject(theNode);
}


/********************* MAKE MANHOLE BLAST *****************************/

static void MakeManholeBlast(void)
{
ObjNode	*newObj;

			/* MAKE THE STEAM EVENT */

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord.x 	= gCoord.x;
	gNewObjectDefinition.coord.z 	= gCoord.z;
	gNewObjectDefinition.coord.y 	= GetTerrainY_Undeformed(gCoord.x,gCoord.z);
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+1;
	gNewObjectDefinition.moveCall 	= MoveManholeBlast;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->ParticleTimer = 0;

	newObj->Health = 1.2;
	newObj->Damage = .2;

	PlayEffect3D(EFFECT_MANHOLEBLAST, &gCoord);


			/* SEE IF HIT PLAYER */

	if (DoSimpleBoxCollisionAgainstPlayer(gCoord.y+40.0f, gCoord.y-40.0f, gCoord.x-80.0f, gCoord.x+80.0f,gCoord.z+80.0f, gCoord.z-80.0f))
	{
		ObjNode	*player = gPlayerInfo.objNode;
		PlayerGotHit(newObj, 0);
		player->Delta.x = player->Delta.z = 0;
		player->Delta.y = 2000.0f;
	}
}


/****************** MOVE MANHOLE BLAST *************************/

static void MoveManholeBlast(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
int		particleGroup,magicNum,i;
OGLVector3D			d;
OGLPoint3D			p;
float		x,y,z;

	theNode->Health -= fps;													// see if done
	if (theNode->Health <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}


	theNode->ParticleTimer -= fps;
	if (theNode->ParticleTimer <= 0.0f)										// see if time to make particles
	{
		theNode->ParticleTimer += .03f;

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= 0;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 90;
			groupDef.decayRate				=  -5.0;
			groupDef.fadeRate				= 4.0;
			groupDef.particleTextureNum		= PARTICLE_SObjType_GreenFumes;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			x = theNode->Coord.x;
			y = theNode->Coord.y;
			z = theNode->Coord.z;

			for (i = 0; i < 5; i++)
			{
				p.x = x + RandomFloat2() * 40.0f;
				p.y = y - RandomFloat() * 60.0f;
				p.z = z + RandomFloat2() * 40.0f;

				d.x = d.z = 0;
				d.y = 3500.0f + RandomFloat2() * 500.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= theNode->Health;
				newParticleDef.rotZ			= 0;
				newParticleDef.rotDZ		= 0;
				newParticleDef.alpha		= .9f;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}


#pragma mark -


/************************* ADD PROXIMITY MINE *********************************/

Boolean AddProximityMine(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		i;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= APOCALYPSE_ObjType_ProximityMine;
	gNewObjectDefinition.scale 		= .6;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y	= GetTerrainY(x,z) -
									(gObjectGroupBBoxList[gNewObjectDefinition.group][gNewObjectDefinition.type].min.y * gNewObjectDefinition.scale);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 522;
	gNewObjectDefinition.moveCall 	= MoveProximityMine;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;							// keep ptr to item list


				/* COLLISION INFO */

	newObj->CType = CTYPE_MISC;
	newObj->CBits = CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Maximized(newObj);

	newObj->SpecialF[0] = RandomFloat()*PI2;						// random wobble index

	for (i = 0; i < NUM_WEAPON_TYPES; i++)							// set all weapon handlers
		newObj->HitByWeaponHandler[i] = ProximityMineHitByWeapon;


				/* SPARKLES */

	i = newObj->Sparkles[0] = GetFreeSparkle(newObj);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_TRANSFORMWITHOWNER;
		gSparkles[i].where.x = 0;
		gSparkles[i].where.y = 97.0f;
		gSparkles[i].where.z = 0;
		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 0;
		gSparkles[i].color.a = 1;
		gSparkles[i].scale = 30.0f;
		gSparkles[i].separation = 20.0f;
		gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark2;
	}

	AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 4,4, false);

	return(true);
}


/********************* MOVE PROXIMITY MINE *********************/

static void MoveProximityMine(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	d;
int		i;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);

			/* WOBBLE */

	theNode->SpecialF[0] += fps * 1.4f;
	gCoord.y = theNode->InitCoord.y + (1.0f + sin(theNode->SpecialF[0])) * 20.0f;
	theNode->Rot.y += 2.0f * fps;

	UpdateObject(theNode);


		/* CHECK SPARKLE COLOR */

	d = OGLPoint3D_Distance(&gCoord, &gPlayerInfo.coord);

	i = theNode->Sparkles[0];
	if (i != -1)
	{
		if (d < 900.0f)
		{
			gSparkles[i].color.r = 1;
			gSparkles[i].color.g = 0;
			gSparkles[i].color.b = 0;
		}
		else
		{
			gSparkles[i].color.r = 1;
			gSparkles[i].color.g = 1;
			gSparkles[i].color.b = 0;
		}
	}

		/* SEE IF GO BOOM */

	if (d < 320.0f)
	{
		ExplodeProximityMine(theNode);
		return;
	}

}

/********************* PROXIMITY MINE HIT BY WEAPON ********************/

static Boolean ProximityMineHitByWeapon(ObjNode *weapon, ObjNode *mine, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon,weaponCoord,weaponDelta)

	ExplodeProximityMine(mine);

	return(true);
}


/************** EXPLODE PROXIMITY MINE *********************/

static void ExplodeProximityMine(ObjNode *mine)
{
long					pg,i;
OGLVector3D				d;
OGLPoint3D				pt,where;
NewParticleDefType		newParticleDef;
float					x,y,z;
ObjNode					*newObj;
DeformationType		defData;

	where = mine->Coord;

		/*********************/
		/* FIRST MAKE SPARKS */
		/*********************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
	gNewParticleGroupDef.gravity				= 100;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 10;
	gNewParticleGroupDef.decayRate				=  .5;
	gNewParticleGroupDef.fadeRate				= 1.0;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_BlueSpark;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;
	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		x = where.x;
		y = where.y;
		z = where.z;

		for (i = 0; i < 20; i++)
		{
			d.y = RandomFloat2() * 300.0f;
			d.x = RandomFloat2() * 300.0f;
			d.z = RandomFloat2() * 300.0f;

			pt.x = x + d.x * 100.0f;
			pt.y = y + d.y * 100.0f;
			pt.z = z + d.z * 100.0f;



			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.5f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);
			AddParticleToGroup(&newParticleDef);
		}
	}

		/******************/
		/* MAKE SHOCKWAVE */
		/******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= APOCALYPSE_ObjType_Shockwave;
	gNewObjectDefinition.coord		= where;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG|STATUS_BIT_GLOW|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOZWRITES|STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+30;
	gNewObjectDefinition.moveCall 	= MoveShockwave;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	newObj->ColorFilter.a = .99;

	newObj->Damage = .2f;

		/*******************/
		/* MAKE CONE BLAST */
		/*******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= APOCALYPSE_ObjType_ConeBlast;
	gNewObjectDefinition.coord.x 	= where.x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(where.x, where.z);
	gNewObjectDefinition.coord.z 	= where.z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG|STATUS_BIT_GLOW|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOZWRITES;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+11;
	gNewObjectDefinition.moveCall 	= MoveConeBlast;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .3;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	newObj->ColorFilter.a = .99;


		/*************************/
		/* MAKE DEFORMATION WAVE */
		/*************************/

	defData.type 				= DEFORMATION_TYPE_RADIALWAVE;
	defData.amplitude 			= 100;
	defData.radius 				= 20;
	defData.speed 				= 3000;
	defData.origin.x			= where.x;
	defData.origin.y			= where.z;
	defData.oneOverWaveLength 	= 1.0f / 500.0f;
	defData.radialWidth			= 800.0f;
	defData.decayRate			= 300.0f;
	NewSuperTileDeformation(&defData);

	PlayEffect_Parms3D(EFFECT_MINEEXPLODE, &where, NORMAL_CHANNEL_RATE, 2.0);
	ExplodeGeometry(mine, 500, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, 1.0);

	mine->TerrainItemPtr = nil;
	DeleteObject(mine);
}



/***************** MOVE SHOCKWAVE ***********************/

static void MoveShockwave(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	x,y,z,w;


	theNode->Scale.x = theNode->Scale.y = theNode->Scale.z += 11.0f * fps;

	theNode->ColorFilter.a -= fps * 3.0f;
	if (theNode->ColorFilter.a <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}

	UpdateObjectTransforms(theNode);


			/* SEE IF HIT PLAYER */

	x = theNode->Coord.x;
	y = theNode->Coord.y;
	z = theNode->Coord.z;
	w = theNode->Scale.x * theNode->BBox.max.x;

	if (DoSimpleBoxCollisionAgainstPlayer(y+10, y-10, x - w, x + w,	z + w, z - w))
	{
		ObjNode	*player = gPlayerInfo.objNode;

		player->Rot.y = CalcYAngleFromPointToPoint(player->Rot.y,x,z,player->Coord.x, player->Coord.z);		// aim player direction of blast
		PlayerGotHit(nil, theNode->Damage);																			// get hit
		player->Delta.x *= 2.0f;
		player->Delta.z *= 2.0f;
//		player->Delta.y *= 2.0f;
	}
}


/***************** MOVE CONEBLAST ***********************/

static void MoveConeBlast(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->Scale.x = theNode->Scale.y = theNode->Scale.z += fps * 5.0f;

	theNode->ColorFilter.a -= fps * 1.5f;
	if (theNode->ColorFilter.a <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}

	UpdateObjectTransforms(theNode);
}


#pragma mark -

/******************** ADD SNOWBALL ******************************/

Boolean AddSnowball(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FIREICE_ObjType_SnowBall;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 95;
	gNewObjectDefinition.moveCall 	= MoveSnowball;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= SNOWBALL_SCALE_START;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;												// keep ptr to item list


	newObj->CType 			= CTYPE_MISC | CTYPE_HURTME;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj,.95,.95);

	newObj->Damage = .2;
	newObj->Health = 6.0f;

	newObj->Flag[0] = false;									// not moving yet

	return(true);												// item was added
}


/************************ MOVE SNOWBALL ****************************/

static void MoveSnowball(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	s;
int		i;
int		particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;

			/* SEE IF TRIGGERED YET */

	if (!theNode->Flag[0])
	{
		if (TrackTerrainItem(theNode))
		{
			DeleteObject(theNode);
			return;
		}

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < 1500.0f)
			theNode->Flag[0] = true;

		return;
	}


		/* SEE IF DECAYED */

	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		ExplodeGeometry(theNode, 350, SHARD_MODE_BOUNCE, 1, 1.0);
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);

			/* DO GRAVITY & ACCELERATION */

	gDelta.y -= 3000.0f * fps;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
		GetTerrainY(gCoord.x,gCoord.z);									// call this just to get the terrain normal

		gDelta.x += gRecentTerrainNormal.x * 5000.0f * fps;				// accel from terrain normal
		gDelta.z += gRecentTerrainNormal.z * 5000.0f * fps;
	}

			/* MOVE IT */

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	VectorLength2D(theNode->Speed2D, gDelta.x, gDelta.z);					// calc 2D speed value

	s = theNode->Scale.x;
	s += theNode->Speed2D * fps * .001f;
	if (s > SNOWBALL_SCALE_END)
		s = SNOWBALL_SCALE_END;

	theNode->Scale.x =
	theNode->Scale.y =
	theNode->Scale.z = s;
	CreateCollisionBoxFromBoundingBox_Update(theNode,.95,.95);


			/* DO COLLISION */

	HandleCollisions(theNode, CTYPE_MISC|CTYPE_TERRAIN, .1);


		/* SPIN IT */

	theNode->Rot.x -= theNode->Speed2D * fps * .008f;
	TurnObjectTowardTarget(theNode, &gCoord, gCoord.x + gDelta.x, gCoord.z + gDelta.z, PI2, false);


			/* UPDATE */

	UpdateObject(theNode);


		/*********************/
		/* LEAVE SNOW BEHIND */
		/*********************/

	theNode->ParticleTimer -= fps;											// see if add smoke
	if (theNode->ParticleTimer <= 0.0f)
	{
		theNode->ParticleTimer += .04f;										// reset timer

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_BOUNCE | PARTICLE_FLAGS_ALLAIM;
			groupDef.gravity				= 300;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 15.0;
			groupDef.decayRate				=  -.2f;
			groupDef.fadeRate				= .6;
			groupDef.particleTextureNum		= PARTICLE_SObjType_SnowFlakes;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;	//_MINUS_SRC_ALPHA;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			p.y = GetTerrainY(gCoord.x, gCoord.z);

			for (i = 0; i < 2; i++)
			{
				p.x = gCoord.x + RandomFloat2() * 30.0f;
				p.z = gCoord.z + RandomFloat2() * 30.0f;

				d.x = RandomFloat2() * 20.0f;
				d.y = 150.0f + RandomFloat() * 40.0f;
				d.z = RandomFloat2() * 20.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat() * PI2;
				newParticleDef.rotDZ		= RandomFloat2();
				newParticleDef.alpha		= .7;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}


}







