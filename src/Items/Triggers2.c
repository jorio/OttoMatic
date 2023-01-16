/****************************/
/*    	TRIGGERS 2	        */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/*******************/
/*   PROTOTYPES    */
/*******************/

static void MoveJungleGate(ObjNode *theNode);
static Boolean	JungleGate_HitByWeaponHandler(ObjNode *weaponObj, ObjNode *gate, OGLPoint3D *where, OGLVector3D *delta);
static void MoveTurtlePlatform(ObjNode *theNode);
static void MoveLeafPlatform(ObjNode *theNode);
static void MoveLeafPlatformShadow(ObjNode *theNode);
static void MoveDebrisGate(ObjNode *theNode);
static Boolean DebrisGate_HitByWeaponHandler(ObjNode *weaponObj, ObjNode *gate, OGLPoint3D *where, OGLVector3D *delta);
static void MoveShockwave(ObjNode *theNode);
static void MoveConeBlast(ObjNode *theNode);
static Boolean ChainReactingMineHitByWeapon(ObjNode *weapon, ObjNode *mine, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void MoveTrapDoor(ObjNode *theNode);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	JUNGLEGATE_SCALE	3.0f

#define	DEBRISGATE_SCALE	2.5f

enum
{
	TRAP_DOOR_MODE_WAIT,
	TRAP_DOOR_MODE_OPEN
};

/**********************/
/*     VARIABLES      */
/**********************/

#define	ShimmeyTimer		SpecialF[0]

#define	WobbleIndex			SpecialF[0]
#define	TimeSinceStepped	SpecialF[1]


/************************* ADD JUNGLE GATE *********************************/

Boolean AddJungleGate(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		i;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= JUNGLE_ObjType_Gate;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveJungleGate;

	if (itemPtr->parm[0] == 1)
		gNewObjectDefinition.rot 		= PI/2;
	else
		gNewObjectDefinition.rot 		= 0;

	gNewObjectDefinition.scale 		= JUNGLEGATE_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_BLOCKRAYS|CTYPE_IMPENETRABLE;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_JUNGLEGATE;

	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1.1, 1);

			/* COLLISION CALLBACKS */


	for (i = 0; i < NUM_WEAPON_TYPES; i++)								// all weapons call this
		newObj->HitByWeaponHandler[i] = JungleGate_HitByWeaponHandler;

	return(true);													// item was added
}


/******************* MOVE JUNGLE GATE ************************/

static void MoveJungleGate(ObjNode *theNode)
{
float	off,r;

		/* SEE IF OUT OF RANGE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

		/* SEE IF MAKE SHIMMEY */

	if (theNode->ShimmeyTimer > 0.0f)
	{
		off = cos(theNode->ShimmeyTimer	* 20.0f) * (theNode->ShimmeyTimer * 10.0f);

		r = theNode->Rot.y;

		theNode->Coord.x = theNode->InitCoord.x + sin(r) * off;
		theNode->Coord.z = theNode->InitCoord.z + cos(r) * off;
		UpdateObjectTransforms(theNode);

		theNode->ShimmeyTimer -= gFramesPerSecondFrac;
		if (theNode->ShimmeyTimer < 0.0f)
			theNode->ShimmeyTimer = 0;
	}
}




/********************* JUNGLE GATE:  HIT BY POW ***********************/

static Boolean	JungleGate_HitByWeaponHandler(ObjNode *weaponObj, ObjNode *gate, OGLPoint3D *where, OGLVector3D *delta)
{
#pragma unused(where, delta)

				/* THE GIANT'S FIST CAN BUST THE GATE */

	if (!weaponObj)
		goto nope;

	if ((weaponObj->Kind == WEAPON_TYPE_FIST) && (gPlayerInfo.scaleRatio > 1.0f))
		DoTrig_JungleGate(gate, nil, 0);
	else
	{
nope:
		if (gate->ShimmeyTimer == 0.0f)
			gate->ShimmeyTimer = 1.1f;
	}

	return(true);			// stop weapon
}

/************** DO TRIGGER - JUNGLE GATE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_JungleGate(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
#pragma unused(sideBits)
int		i;
ObjNode	*newObj;
float	dx,dz;



	if (whoNode)				// this is nil if called from weapon handler above
	{
		if (whoNode->Skeleton->AnimNum != PLAYER_ANIM_GIANTJUMPJET)		// only trigger if GIANT jump-jetted into this
		{
			if (theNode->ShimmeyTimer == 0.0f)
				theNode->ShimmeyTimer = .5f;							// make shimmy instead
			return(true);
		}

		dx = whoNode->Delta.x;
		dz = whoNode->Delta.z;
	}
	else
	{
		dx = dz = 0;
	}

			/**************/
			/* BLOW IT UP */
			/**************/

	for (i = 0; i < 9; i++)
	{
		OGLPoint3D	coord;
		static const short types[] =
		{
			JUNGLE_ObjType_GateChunk0,
			JUNGLE_ObjType_GateChunk0,
			JUNGLE_ObjType_GateChunk1,
			JUNGLE_ObjType_GateChunk2
		};

		coord.x = RandomFloat2() * 200.0f;
		coord.y = RandomFloat() * 500.0f;
		coord.z = 0;
		OGLPoint3D_Transform(&coord, &theNode->BaseTransformMatrix, &gNewObjectDefinition.coord);

		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= types[MyRandomLong() & 0x3];
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
		gNewObjectDefinition.rot 		= theNode->Rot.y;
		gNewObjectDefinition.moveCall 	= MoveWoodenGateChunk;
		gNewObjectDefinition.scale 		= theNode->Scale.x;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->DeltaRot.x = RandomFloat2() * PI2;
		newObj->DeltaRot.y = RandomFloat2() * PI2;
		newObj->DeltaRot.z = RandomFloat2() * PI2;

		newObj->Delta.x = dx * .2f;
		newObj->Delta.y = RandomFloat() * 600.0f;
		newObj->Delta.z = dz * .2f;

		CreateCollisionBoxFromBoundingBox_Maximized(newObj);
	}

	PlayEffect3D(EFFECT_BIGDOORSMASH, &theNode->Coord);



			/* DELETE THE GATE */

	theNode->TerrainItemPtr = nil;								// never come back
	DeleteObject(theNode);

	return(true);
}



#pragma mark -

/************************* ADD TURTLE PLATFORM *********************************/

Boolean AddTurtlePlatform(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

					/* MAIN OBJECT */

	gNewObjectDefinition.type 		= SKELETON_TYPE_TURTLE;
	gNewObjectDefinition.animNum 	= 0;
	gNewObjectDefinition.coord.x 	= x;

	if (!GetWaterY(x, z, &gNewObjectDefinition.coord.y))			// get water y
		gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveTurtlePlatform;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/8.0f);
	gNewObjectDefinition.scale 		= 2.5;
	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_MISC|CTYPE_BLOCKSHADOW;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;						// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_TURTLE;

			/* SET COLLISION BASED ON ROT */

	if (itemPtr->parm[0] & 1)
	{
		CreateCollisionBoxFromBoundingBox(newObj, .8,1);
	}
	else
		CreateCollisionBoxFromBoundingBox_Rotated(newObj,.8,1);


	newObj->WobbleIndex = RandomFloat() * PI2;					// random wobble index
	newObj->TimeSinceStepped = 0;

	return(true);
}


/******************* MOVE TURTLE PLATFORM *************************/

static void MoveTurtlePlatform(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

		/* SEE IF OUT OF RANGE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);



	theNode->TimeSinceStepped += fps;
	if (theNode->TimeSinceStepped > 1.0f)
	{
			/* SEE IF UNSTEP */

		if (theNode->Skeleton->AnimNum == 1)
			MorphToSkeletonAnim(theNode->Skeleton, 0, 4.0);

				/* WOBBLE */

		theNode->WobbleIndex += fps * PI2;
		gCoord.y = theNode->InitCoord.y + sin(theNode->WobbleIndex) * 10.0f;

	}

	UpdateObject(theNode);
}


/************** DO TRIGGER - TURTLE PLATFORM ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_TurtlePlatform(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
#pragma unused(whoNode,sideBits)

	if (theNode->Skeleton->AnimNum != 1)
		MorphToSkeletonAnim(theNode->Skeleton, 1, 4.0);

	theNode->TimeSinceStepped = 0;
	return(true);
}


#pragma mark -

/************************* ADD SMASHABLE *********************************/

Boolean AddSmashable(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
short	type = itemPtr->parm[0];

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= JUNGLE_ObjType_Hut + type;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI2/8.0f);

	gNewObjectDefinition.scale 		= 3.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_SMASHABLE;

	CreateCollisionBoxFromBoundingBox_Rotated(newObj, .7, 1);

	return(true);													// item was added
}


/************** DO TRIGGER - SMASHABLE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_Smashable(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{

#pragma unused(sideBits,whoNode)

	if (gPlayerInfo.scaleRatio <= 1.0f)
		return(true);

	ExplodeGeometry(theNode, 800, SHARD_MODE_BOUNCE, 1, .5);

	theNode->TerrainItemPtr = nil;								// never come back
	DeleteObject(theNode);

	PlayEffect3D(EFFECT_BIGDOORSMASH, &theNode->Coord);

	return(true);
}


#pragma mark -




/************** ADD LEAF PLATFORM ***********************/

Boolean AddLeafPlatform(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj,*shadowObj;

					/* MAKE LEAF PLATFORM */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= JUNGLE_ObjType_LeafPlatform0 + itemPtr->parm[0];
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY_Undeformed(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveLeafPlatform;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[1] * (PI2 / 4.0f);
	gNewObjectDefinition.scale 		= 4.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_TRIGGER | CTYPE_MISC | CTYPE_BLOCKCAMERA | CTYPE_BLOCKSHADOW
								| CTYPE_MPLATFORM | CTYPE_MPLATFORM_FREEJUMP;		// helps keep Otto grounded
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= CBITS_TOP;							// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_LEAFPLATFORM;

	CreateCollisionBoxFromBoundingBox_Rotated(newObj, .9,1.0);
	switch(itemPtr->parm[1])
	{
		case	0:
				newObj->CollisionBoxes[0].front -= 350.0f;
				break;
		case	1:
				newObj->CollisionBoxes[0].right -= 350.0f;
				break;
		case	2:
				newObj->CollisionBoxes[0].back += 350.0f;
				break;
		case	3:
				newObj->CollisionBoxes[0].left += 350.0f;
				break;
	}
	CalcObjectBoxFromNode(newObj);
	KeepOldCollisionBoxes(newObj);


	newObj->WobbleIndex = RandomFloat2()*PI2;


				/* MAKE SHADOW */

	gNewObjectDefinition.type 		= JUNGLE_ObjType_LeafPlatformShadow;
	gNewObjectDefinition.coord.y 	= FindHighestCollisionAtXZ(x,z, CTYPE_TERRAIN | CTYPE_WATER) + 3.0f;
	gNewObjectDefinition.slot		= WATER_SLOT+1;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOZWRITES | STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.moveCall 	= MoveLeafPlatformShadow;
	shadowObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->ChainNode = shadowObj;

	return(true);
}

/********************** MOVE LEAF PLATFORM ***********************/

static void MoveLeafPlatform(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
OGLPoint3D	p;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	theNode->WobbleIndex += fps * 10.0f;
	theNode->Rot.x = sin(theNode->WobbleIndex) * theNode->SpecialF[1];
	theNode->SpecialF[1] -= fps * .02f;
	if (theNode->SpecialF[1] <= 0.015f)
	{
		theNode->SpecialF[1] = 0.015f;
	}
	UpdateObjectTransforms(theNode);


			/* PIN OTTO TO PLATFORM AS WE WOBBLE */

	// Since we're an MPlatform, Otto will copy our deltas.
	// So, pin Otto to the platform with a large negative delta Y.
	// This ensures that Otto stays firmly grounded (at high
	// framerates) even as the leaf wobbles towards the floor.
	theNode->Delta.y = -300;


			/* UPDATE COLLISION TOP */

	p.x = 0;
	p.y = theNode->BBox.max.y;
	p.z = (theNode->BBox.max.z + theNode->BBox.min.z) * .5f;
	OGLPoint3D_Transform(&p, &theNode->BaseTransformMatrix, &p);
	theNode->CollisionBoxes[0].top = p.y - 5.0f;
	theNode->CollisionBoxes[0].bottom = theNode->CollisionBoxes[0].top - 80.0f;
}


/************** DO TRIGGER - LEAF PLATFORM ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_LeafPlatform(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
#pragma unused(whoNode, sideBits)

	if (gDelta.y < -700.0f)
	{
		if (gPlayerInfo.scaleRatio > 1.0f)			// if giant lands on this then crush
		{
			theNode->SpecialF[1] = .3f;
			theNode->WobbleIndex = PI;
			return(false);
		}
		else
		{
			theNode->SpecialF[1] = .07f;
			theNode->WobbleIndex = PI;
		}
	}

	return(true);
}


/******************** MOVE LEAF PLATFORM SHADOW **********************/

static void MoveLeafPlatformShadow(ObjNode *theNode)
{
float	x,y,z,r;

				/* CALC POINT UNDER LEAF */

	r = theNode->Rot.y;
	x = theNode->Coord.x - sin(r) * 300.0f;
	z = theNode->Coord.x - cos(r) * 300.0f;


				/* CALC Y COORD THERE */

	y = FindHighestCollisionAtXZ(x,z, CTYPE_TERRAIN | CTYPE_WATER) + 10.0f;

	if (y != theNode->Coord.y)
		UpdateObjectTransforms(theNode);


}


#pragma mark -

/************************* ADD DEBRIS GATE *********************************/

Boolean AddDebrisGate(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		i;
short	type = itemPtr->parm[0];

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= APOCALYPSE_ObjType_DebrisGate_Intact + type;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveDebrisGate;

	if (itemPtr->parm[1] == 1)
		gNewObjectDefinition.rot 		= PI/2;
	else
		gNewObjectDefinition.rot 		= 0;

	gNewObjectDefinition.scale 		= DEBRISGATE_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* SET COLLISION STUFF */

	if (type == 1)
	{
		newObj->CType 			= CTYPE_TRIGGER|CTYPE_MISC|CTYPE_IMPENETRABLE|CTYPE_BLOCKCAMERA|CTYPE_BLOCKRAYS;
		newObj->CBits			= CBITS_ALLSOLID;
		newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
		newObj->Kind		 	= TRIGTYPE_DEBRISGATE;
		CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1.1, 1);

		for (i = 0; i < NUM_WEAPON_TYPES; i++)								// all weapons call this
			newObj->HitByWeaponHandler[i] = DebrisGate_HitByWeaponHandler;
	}
	else
	{

	}



	return(true);													// item was added
}


/******************* MOVE DEBRIS GATE ************************/

static void MoveDebrisGate(ObjNode *theNode)
{
float	off,r;

		/* SEE IF OUT OF RANGE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

		/* SEE IF MAKE SHIMMEY */

	if (theNode->ShimmeyTimer > 0.0f)
	{
		off = cos(theNode->ShimmeyTimer	* 20.0f) * (theNode->ShimmeyTimer * 10.0f);

		r = theNode->Rot.y;

		theNode->Coord.x = theNode->InitCoord.x + sin(r) * off;
		theNode->Coord.z = theNode->InitCoord.z + cos(r) * off;
		UpdateObjectTransforms(theNode);

		theNode->ShimmeyTimer -= gFramesPerSecondFrac;
		if (theNode->ShimmeyTimer < 0.0f)
			theNode->ShimmeyTimer = 0;
	}
}


/********************* DEBRIS GATE:  HIT BY POW ***********************/

static Boolean DebrisGate_HitByWeaponHandler(ObjNode *weaponObj, ObjNode *gate, OGLPoint3D *where, OGLVector3D *delta)
{
#pragma unused (weaponObj, where, delta)

	if (gate->ShimmeyTimer == 0.0f)
		gate->ShimmeyTimer = 1.1f;

	return(true);			// stop weapon
}

/************** DO TRIGGER - DEBRIS GATE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_DebrisGate(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
#pragma unused (sideBits, whoNode)
int		i;
ObjNode	*newObj;

static const short type[] =
{
	APOCALYPSE_ObjType_DebrisGate_Debris0,
	APOCALYPSE_ObjType_DebrisGate_Debris1,
	APOCALYPSE_ObjType_DebrisGate_Debris2,
	APOCALYPSE_ObjType_DebrisGate_Debris3,
	APOCALYPSE_ObjType_DebrisGate_Debris4
};

static const OGLPoint3D coord[] =
{
	{263, 101, 0},
	{120, 41, 0},
	{24, 94, 0},
	{-119, 95, 0},
	{-30, 171, 0},
};


	if (whoNode->Skeleton->AnimNum != PLAYER_ANIM_JUMPJET)		// only trigger if jump-jetted into this
		return(true);

			/**************/
			/* BLOW IT UP */
			/**************/

	for (i = 0; i < 5; i++)
	{
		OGLPoint3D_Transform(&coord[i], &theNode->BaseTransformMatrix, &gNewObjectDefinition.coord);
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= type[i];
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
		gNewObjectDefinition.rot 		= theNode->Rot.y;
		gNewObjectDefinition.moveCall 	= MoveWoodenGateChunk;
		gNewObjectDefinition.scale 		= theNode->Scale.x;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->DeltaRot.x = RandomFloat2() * PI2;
		newObj->DeltaRot.y = RandomFloat2() * PI2;
		newObj->DeltaRot.z = RandomFloat2() * PI2;

		newObj->Delta.x = whoNode->Delta.x * .2f;
		newObj->Delta.y = whoNode->Delta.y + 1200.0f;
		newObj->Delta.z = whoNode->Delta.z * .2f;

		CreateCollisionBoxFromBoundingBox_Maximized(newObj);
	}

	PlayEffect3D(EFFECT_DEBRISSMASH, &theNode->Coord);


			/* DELETE THE GATE */

	theNode->TerrainItemPtr = nil;								// never come back
	DeleteObject(theNode);

	return(true);
}


#pragma mark -




/************************* ADD CHAINREACTING MINE *********************************/

Boolean AddChainReactingMine(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
float	s;
int		i;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= APOCALYPSE_ObjType_ExplodingCylinder;
	gNewObjectDefinition.scale 		= s = .6;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y	= GetTerrainY(x,z) - (gObjectGroupBBoxList[MODEL_GROUP_LEVELSPECIFIC][APOCALYPSE_ObjType_ExplodingCylinder].min.y * s);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
	gNewObjectDefinition.slot 		= 524;
	gNewObjectDefinition.moveCall 	= MoveStaticObject3;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;							// keep ptr to item list


				/* COLLISION INFO */

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_MISC;
	newObj->CBits 			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;					// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_CHAINREACTINGMINE;
	CreateCollisionBoxFromBoundingBox_Maximized(newObj);

	for (i = 0; i < NUM_WEAPON_TYPES; i++)							// set all weapon handlers
		newObj->HitByWeaponHandler[i] = ChainReactingMineHitByWeapon;


				/* SPARKLE */

	i = newObj->Sparkles[0] = GetFreeSparkle(newObj);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
		gSparkles[i].where.x = newObj->Coord.x;
		gSparkles[i].where.y = newObj->Coord.y + (newObj->BBox.max.y * s) + 10.0f;
		gSparkles[i].where.z = newObj->Coord.z;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 120.0f;

		gSparkles[i].separation = 40.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_RedSpark;
	}


	return(true);
}

/********************* CHAIN REACTING MINE HIT BY WEAPON ********************/

static Boolean ChainReactingMineHitByWeapon(ObjNode *weapon, ObjNode *mine, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weaponCoord,weaponDelta)

	DoTrig_ChainReactingMine(mine, weapon, 0);

	return(true);
}


/************** DO TRIGGER - CHAINREACTING MINE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_ChainReactingMine(ObjNode *mine, ObjNode *whoNode, Byte sideBits)
{
long					pg,i;
OGLVector3D				d;
OGLPoint3D				pt,where;
NewParticleDefType		newParticleDef;
float					x,y,z;
ObjNode					*newObj;
DeformationType		defData;

#pragma unused(whoNode, sideBits)

	where = mine->Coord;
	x = where.x;
	y = where.y;
	z = where.z;

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


	ExplodeGeometry(mine, 500, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, 1.0);
	DeleteObject(mine);
	PlayEffect_Parms3D(EFFECT_MINEEXPLODE, &gCoord, NORMAL_CHANNEL_RATE, 2.0);

	return(true);
}



/***************** MOVE SHOCKWAVE ***********************/

static void MoveShockwave(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	x,y,z,w;
short	n,i;

	theNode->Scale.x = theNode->Scale.y = theNode->Scale.z += 9.0f * fps;

	theNode->ColorFilter.a -= fps * 2.2f;
	if (theNode->ColorFilter.a <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}

	UpdateObjectTransforms(theNode);


			/***********************/
			/* SEE IF HIT ANYTHING */
			/***********************/

	x = theNode->Coord.x;
	y = theNode->Coord.y;
	z = theNode->Coord.z;
	w = theNode->Scale.x * theNode->BBox.max.x;

			/* CHECK TO SEE IF HIT PLAYER */

	if (DoSimpleBoxCollisionAgainstPlayer(y+10, y-10, x - w, x + w,	z + w, z - w))
	{
		ObjNode	*player = gPlayerInfo.objNode;

		player->Rot.y = CalcYAngleFromPointToPoint(player->Rot.y,x,z,player->Coord.x, player->Coord.z);		// aim player direction of blast
		PlayerGotHit(nil, theNode->Damage);																			// get hit
		player->Delta.x *= 2.0f;
		player->Delta.z *= 2.0f;
	}

			/* CHECK TO SEE IF HIT ENEMY */

	if (DoSimpleBoxCollision(y+10, y-10, x - w, x + w,	z + w, z - w, CTYPE_ENEMY))
	{
		for (i = 0; i < gNumCollisions; i++)
		{
			ObjNode	*enemy = gCollisionList[i].objectPtr;

			enemy->Rot.y = CalcYAngleFromPointToPoint(enemy->Rot.y,x,z,enemy->Coord.x, enemy->Coord.z);		// aim enemy direction of blast
			enemy->Delta.x *= 2.0f;
			enemy->Delta.z *= 2.0f;

			if (enemy->HurtCallback)												// call enemy's hurt function
				enemy->HurtCallback(enemy, 1.0);

		}
	}


		/* SEE IF HIT ANOTHER CHAIN REACTING MINE */

	n = DoSimpleBoxCollision(y+10, y-10, x - w, x + w,	z + w, z - w, CTYPE_MISC | CTYPE_TRIGGER);
	for (i = 0; i < n; i++)
	{
		ObjNode	*obj = gCollisionList[i].objectPtr;						// get hit obj

		if (obj->Kind != TRIGTYPE_CHAINREACTINGMINE)					// see if it's a mine
			continue;

		DoTrig_ChainReactingMine(obj, theNode, 0);						// trigger the mine to explode
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


/************************* ADD TRAP DOOR *********************************/

Boolean AddTrapDoor(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
float	x2,z2;

	x /= TERRAIN_POLYGON_SIZE;
	z /= TERRAIN_POLYGON_SIZE;

	x2 = (float)x * TERRAIN_POLYGON_SIZE + (TERRAIN_POLYGON_SIZE/2.0f);
	z2 = (float)z * TERRAIN_POLYGON_SIZE;


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOUD_ObjType_TrapDoor;
	gNewObjectDefinition.coord.x 	= x2;
	gNewObjectDefinition.coord.y 	= GetTerrainY_Undeformed(x2,z2-10.0f);			// get height of terrain in back of this a ways (the hole is a pit)
	gNewObjectDefinition.coord.z 	= z2;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveTrapDoor;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= TERRAIN_POLYGON_SIZE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_MISC;
	newObj->CBits			= CBITS_TOP|CBITS_ALWAYSTRIGGER;
	newObj->TriggerSides 	= SIDE_BITS_TOP;						// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_TRAPDOOR;

	CreateCollisionBoxFromBoundingBox(newObj, 1, 1);


	newObj->Mode = TRAP_DOOR_MODE_WAIT;


	return(true);
}


/***************** MOVE TRAP DOOR ******************/

static void MoveTrapDoor(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

		/* SEE IF OUT OF RANGE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	switch(theNode->Mode)
	{
		case	TRAP_DOOR_MODE_WAIT:
				break;

		case	TRAP_DOOR_MODE_OPEN:
				if (theNode->Rot.x < (PI/2))
					theNode->DeltaRot.x += fps * 6.0f;
				else
				if (theNode->Rot.x > (PI/2))
					theNode->DeltaRot.x -= fps * 6.0f;

				theNode->Rot.x += theNode->DeltaRot.x * fps;

				if (theNode->Rot.x > (PI * 2/3))
				{
					theNode->Rot.x = PI*2/3;
					theNode->DeltaRot.x = theNode->DeltaRot.x * -.8f;
				}

				UpdateObjectTransforms(theNode);
				break;

	}

}


/************** DO TRIGGER - TRAP DOOR ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_TrapDoor(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
#pragma unused (whoNode, sideBits)

	if (theNode->Mode != TRAP_DOOR_MODE_OPEN)
	{
		theNode->DeltaRot.x = 0;
		theNode->Mode = TRAP_DOOR_MODE_OPEN;

		theNode->CType = 0;

		PlayEffect3D(EFFECT_TRAPDOOR, &theNode->Coord);
	}

	return(true);
}















