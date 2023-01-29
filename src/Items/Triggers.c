/****************************/
/*    	TRIGGERS	        */
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

static Boolean DoTrig_WoodenGate(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static void MoveWoodenGate(ObjNode *theNode);
static Boolean	WoodenGate_HitByWeaponHandler(ObjNode *weaponObj, ObjNode *gate, OGLPoint3D *where, OGLVector3D *delta);

static Boolean DoTrig_MetalGate(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static void MoveMetalGate(ObjNode *theNode);
static Boolean	MetalGate_HitByWeaponHandler(ObjNode *weaponObj, ObjNode *gate, OGLPoint3D *where, OGLVector3D *delta);
static void MoveMetalGateChunk(ObjNode *theNode);

static Boolean DoTrig_Checkpoint(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
static void MoveCheckpoint(ObjNode *base);
static void MakeCheckPointSparkles(ObjNode *theNode);

static void MoveBumperBubble(ObjNode *theNode);
static Boolean DoTrig_BumperBubble(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

static void MoveFallingSlimePlatform(ObjNode *theNode);
static Boolean DoTrig_FallingSlimePlatform(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

static void MoveSpinningPlatform(ObjNode *theNode);
static Boolean DoTrig_SpinningPlatform(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);


/****************************/
/*    CONSTANTS             */
/****************************/


#define	BUMPERBUBBLE_SCALE			1.5f
#define	BUMPER_BUBBLE_SPIN_RADIUS	50.0f

#define	WOODFENCE_SCALE		1.5f
#define	METALFENCE_SCALE	1.3f




/**********************/
/*     VARIABLES      */
/**********************/

												// TRIGGER HANDLER TABLE
												//========================

Boolean	(*gTriggerTable[])(ObjNode *, ObjNode *, Byte) =
{
	DoTrig_WoodenGate,
	DoTrig_MetalGate,
	DoTrig_Human,
	DoTrig_CornKernel,
	DoTrig_Checkpoint,
	DoTrig_BumperBubble,
	DoTrig_FallingSlimePlatform,
	DoTrig_BubblePump,
	DoTrig_SpinningPlatform,
	DoTrig_JungleGate,
	DoTrig_TurtlePlatform,
	DoTrig_Smashable,
	DoTrig_LeafPlatform,
	DoTrig_PowerupPod,
	DoTrig_DebrisGate,
	DoTrig_ChainReactingMine,
	DoTrig_CrunchDoor,
	DoTrig_BumperCar,
	DoTrig_BumperCarPowerPost,
	DoTrig_RocketSled,
	DoTrig_TrapDoor,
	DoTrig_LavaPlatform
};


#define	ShimmeyTimer		SpecialF[0]
#define	BumperBubbleSpin	SpecialF[1]
#define	BumperWobble0		SpecialF[2]
#define	BumperWobble1		SpecialF[3]
#define	BumperWobble2		SpecialF[4]
#define	BumperWobble3		SpecialF[5]

#define	CheckpointSpinning 	Flag[0]
#define	CheckpointNum		Special[0]
#define	ReincarnationAim	SpecialF[0]

#define	FallDelay			SpecialF[0]

#define	SpinDirectionCCW	Flag[0]
#define	SpinOffset			SpecialF[0]

float	gSpinningPlatformRot = 0;


/******************** HANDLE TRIGGER ***************************/
//
// INPUT: triggerNode = ptr to trigger's node
//		  whoNode = who touched it?
//		  side = side bits from collision.  Which side (of hitting object) hit the trigger
//
// OUTPUT: true if we want to handle the trigger as a solid object
//
// NOTE:  Triggers cannot self-delete in their DoTrig_ calls!!!  Bad things will happen in the hander collision function!
//

Boolean HandleTrigger(ObjNode *triggerNode, ObjNode *whoNode, Byte side)
{
	if (triggerNode->CBits & (CBITS_TOUCHABLE|CBITS_ALWAYSTRIGGER))			// see if a non-solid trigger
	{
		return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
	}

			/* CHECK SIDES */
	else
	if (side & SIDE_BITS_BACK)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_FRONT)		// if my back hit, then must be front-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_FRONT)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_BACK)			// if my front hit, then must be back-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_LEFT)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_RIGHT)		// if my left hit, then must be right-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_RIGHT)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_LEFT)			// if my right hit, then must be left-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_TOP)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_BOTTOM)		// if my top hit, then must be bottom-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
	if (side & SIDE_BITS_BOTTOM)
	{
		if (triggerNode->TriggerSides & SIDE_BITS_TOP)			// if my bottom hit, then must be top-triggerable
			return(gTriggerTable[triggerNode->Kind](triggerNode,whoNode,side));	// call trigger's handler routine
		else
			return(true);
	}
	else
		return(true);											// assume it can be solid since didnt trigger
}





#pragma mark -

/************************* ADD WOODEN GATE *********************************/

Boolean AddWoodenGate(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		i;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FARM_ObjType_WoodGate;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveWoodenGate;

	if (itemPtr->parm[0] == 1)
		gNewObjectDefinition.rot 		= PI/2;
	else
		gNewObjectDefinition.rot 		= 0;

	gNewObjectDefinition.scale 		= WOODFENCE_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_MISC|CTYPE_IMPENETRABLE|CTYPE_BLOCKCAMERA|CTYPE_BLOCKRAYS;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_WOODENGATE;

	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1.5, 1);

			/* COLLISION CALLBACKS */


	for (i = 0; i < NUM_WEAPON_TYPES; i++)								// all weapons call this
		newObj->HitByWeaponHandler[i] = WoodenGate_HitByWeaponHandler;

	return(true);													// item was added
}


/******************* MOVE WOODEN GATE ************************/

static void MoveWoodenGate(ObjNode *theNode)
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




/********************* WOODEN GATE:  HIT BY POW ***********************/

static Boolean	WoodenGate_HitByWeaponHandler(ObjNode *weaponObj, ObjNode *gate, OGLPoint3D *where, OGLVector3D *delta)
{
#pragma unused (weaponObj, where, delta)

	if (gate->ShimmeyTimer == 0.0f)
		gate->ShimmeyTimer = 1.1f;

	PlayEffect3D(EFFECT_WOODDOORHIT, &gate->Coord);

	return(true);			// stop weapon
}


/************** DO TRIGGER - WOODEN GATE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_WoodenGate(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
ObjNode	*newObj;
int		i;
static const short type[] =
{
	FARM_ObjType_WoodGateChunk_Post,
	FARM_ObjType_WoodGateChunk_Post,
	FARM_ObjType_WoodGateChunk_Post,
	FARM_ObjType_WoodGateChunk_Post,

	FARM_ObjType_WoodGateChunk_Slat,
	FARM_ObjType_WoodGateChunk_Slat,
	FARM_ObjType_WoodGateChunk_Slat,
	FARM_ObjType_WoodGateChunk_Slat,
};

static const OGLPoint3D coord[] =
{
	{-393, 60, 0},
	{-45, 60, 0},
	{45, 60, 0},
	{393, 60, 0},

	{-210, 220, 0},
	{-210, 72, 0},
	{210, 220, 0},
	{210, 72, 0},
};

#pragma unused (sideBits)

	if (whoNode->Skeleton->AnimNum != PLAYER_ANIM_JUMPJET)		// only trigger if jump-jetted into this
	{
		if (theNode->ShimmeyTimer == 0.0f)
		{
			theNode->ShimmeyTimer = .5f;						// make shimmy instead
			PlayEffect3D(EFFECT_WOODDOORHIT, &theNode->Coord);
		}

		return(true);
	}


	DisableHelpType(HELP_MESSAGE_SMASH);				// disable this help message since the player can now do it
	DisableHelpType(HELP_MESSAGE_JUMPJET);

			/**************/
			/* BLOW IT UP */
			/**************/


	for (i = 0; i < 8; i++)
	{
		OGLPoint3D_Transform(&coord[i], &theNode->BaseTransformMatrix, &gNewObjectDefinition.coord);
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= type[i];
		gNewObjectDefinition.flags 		= STATUS_BIT_NOLIGHTING;
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


			/* DELETE THE GATE */

	theNode->TerrainItemPtr = nil;								// never come back
	DeleteObject(theNode);

	PlayEffect3D(EFFECT_WOODGATECRASH, &whoNode->Coord);

	return(true);
}


/***************** MOVE WOODEN GATE CHUNK **************************/

void MoveWoodenGateChunk(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	gDelta.y -= 2500.0f * fps;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

			/* SEE IF BOUNCE */

	if (theNode->Special[0] < 1)							// see if bounce
	{
		if ((gCoord.y + theNode->BottomOff) < GetTerrainY(gCoord.x, gCoord.z))
		{
			theNode->Special[0]++;
			gDelta.y *= -.7f;
			theNode->DeltaRot.x *= .6f;
			theNode->DeltaRot.y *= .6f;
			theNode->DeltaRot.z *= .6f;
		}
	}
	else
	{
		if ((gCoord.y + theNode->BBox.max.y) < GetTerrainY(gCoord.x, gCoord.z))
		{
			DeleteObject(theNode);
			return;
		}
	}

			/* SPIN */

	theNode->Rot.x += theNode->DeltaRot.x * fps;
	theNode->Rot.y += theNode->DeltaRot.y * fps;
	theNode->Rot.z += theNode->DeltaRot.z * fps;

	UpdateObject(theNode);
}



#pragma mark -

/************************* ADD METAL GATE *********************************/

Boolean AddMetalGate(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		i;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FARM_ObjType_MetalGate;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveMetalGate;

	if (itemPtr->parm[0] == 1)
		gNewObjectDefinition.rot 		= PI/2;
	else
		gNewObjectDefinition.rot 		= 0;

	gNewObjectDefinition.scale 		= METALFENCE_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_TRIGGER2|CTYPE_TRIGGER|CTYPE_MISC|CTYPE_IMPENETRABLE|CTYPE_BLOCKCAMERA|CTYPE_BLOCKRAYS;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_METALGATE;

	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 1.1, 1.5);

			/* COLLISION CALLBACKS */

	for (i = 0; i < NUM_WEAPON_TYPES; i++)								// all weapons call this
		newObj->HitByWeaponHandler[i] = MetalGate_HitByWeaponHandler;

	return(true);													// item was added
}


/******************* MOVE METAL GATE ************************/

static void MoveMetalGate(ObjNode *theNode)
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




/********************* METAL GATE:  HIT BY WEAPON ***********************/

static Boolean	MetalGate_HitByWeaponHandler(ObjNode *weaponObj, ObjNode *gate, OGLPoint3D *where, OGLVector3D *delta)
{
#pragma unused (weaponObj, delta, where)

	if (gate->ShimmeyTimer == 0.0f)
	{
		gate->ShimmeyTimer = 1.1f;

		PlayEffect3D(EFFECT_METALGATEHIT, &gate->Coord);
	}

	return(true);			// stop weapon
}


/************** DO TRIGGER - METAL GATE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_MetalGate(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
ObjNode	*newObj;
int		i;
static const short type[] =
{
	FARM_ObjType_MetalGateChunk_V,
	FARM_ObjType_MetalGateChunk_V,
	FARM_ObjType_MetalGateChunk_V,
	FARM_ObjType_MetalGateChunk_V,

	FARM_ObjType_MetalGateChunk_H,
	FARM_ObjType_MetalGateChunk_H,
	FARM_ObjType_MetalGateChunk_H,
	FARM_ObjType_MetalGateChunk_H,
};

static const OGLPoint3D coord[] =
{
	{-393, 60, 0},
	{-45, 60, 0},
	{45, 60, 0},
	{393, 60, 0},

	{-210, 220, 0},
	{-210, 72, 0},
	{210, 220, 0},
	{210, 72, 0},
};

#pragma unused (sideBits)

	if (whoNode->CType & CTYPE_PLAYER)						// player's cannot trigger this
	{
shim:
		if (theNode->ShimmeyTimer == 0.0f)
		{
			theNode->ShimmeyTimer = .5f;					// make shimmy instead
			PlayEffect3D(EFFECT_METALGATEHIT, &theNode->Coord);
		}
		return(true);
	}

	if (whoNode->Speed2D < 1200.0f)							// must be going fast enough
		goto shim;

			/**************/
			/* BLOW IT UP */
			/**************/

	for (i = 0; i < 8; i++)
	{
		OGLPoint3D_Transform(&coord[i], &theNode->BaseTransformMatrix, &gNewObjectDefinition.coord);
		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= type[i];
		gNewObjectDefinition.flags 		= STATUS_BIT_NOLIGHTING;
		gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
		gNewObjectDefinition.rot 		= theNode->Rot.y;
		gNewObjectDefinition.moveCall 	= MoveMetalGateChunk;
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


	PlayEffect3D(EFFECT_METALGATECRASH, &theNode->Coord);		// play crash sound


			/* DELETE THE GATE */

	theNode->TerrainItemPtr = nil;								// never come back
	DeleteObject(theNode);


			/* STOP THE TRACTOR */

	StopTractor(whoNode);


	return(true);
}


/***************** MOVE METAL GATE CHUNK **************************/

static void MoveMetalGateChunk(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	gDelta.y -= 2000.0f * fps;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

			/* SEE IF BOUNCE */

	if (theNode->Special[0] < 2)							// see if bounce
	{
		if ((gCoord.y + theNode->BottomOff) < GetTerrainY(gCoord.x, gCoord.z))
		{
			theNode->Special[0]++;
			gDelta.y *= -.7f;
			theNode->DeltaRot.x *= .6f;
			theNode->DeltaRot.y *= .6f;
			theNode->DeltaRot.z *= .6f;
		}
	}
	else
	{
		if ((gCoord.y + theNode->BBox.max.y) < GetTerrainY(gCoord.x, gCoord.z))
		{
del:
			DeleteObject(theNode);
			return;
		}
	}

			/* SPIN */

	theNode->Rot.x += theNode->DeltaRot.x * fps;
	theNode->Rot.y += theNode->DeltaRot.y * fps;
	theNode->Rot.z += theNode->DeltaRot.z * fps;

	theNode->ColorFilter.a -= fps * .5f;
	if (theNode->ColorFilter.a  <= 0.0f)
		goto del;

	UpdateObject(theNode);
}


#pragma mark -


/************************* ADD CHECKPOINT *********************************/

Boolean AddCheckpoint(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*base, *dish;
float	y;
					/* MAKE BASE */

	y = FindHighestCollisionAtXZ(x,z, CTYPE_MISC|CTYPE_TERRAIN|CTYPE_MPLATFORM);

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_TeleportBase;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= y + 3.0f;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveCheckpoint;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 5.0;
	base = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	base->TerrainItemPtr = itemPtr;								// keep ptr to item list

	base->ReincarnationAim = (float)itemPtr->parm[1] * (PI2/8.0f);

	base->CType 			= CTYPE_TRIGGER|CTYPE_MISC|CTYPE_BLOCKCAMERA;
	base->CBits			= CBITS_ALLSOLID;
	base->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	base->Kind		 	= TRIGTYPE_CHECKPOINT;

	CreateCollisionBoxFromBoundingBox(base, 1.1, 1);

	base->CheckpointNum = itemPtr->parm[0];						// remember checkpoint # 0..n

	if (base->CheckpointNum <= gBestCheckpointNum)				// see if this one has already been activated
	{
		base->CheckpointSpinning = true;
		MakeCheckPointSparkles(base);
		base->CType &= ~CTYPE_TRIGGER;					// dont allow trigger since alreay spinning
	}
	else
	{
		base->CheckpointSpinning = false;
	}


				/* MAKE DISH */

	gNewObjectDefinition.type 		= GLOBAL_ObjType_TeleportDish;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	dish = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	base->ChainNode = dish;

	AttachShadowToObject(base, GLOBAL_SObjType_Shadow_Circular, 5,5, false);


	return(true);													// item was added
}


/******************* MOVE CHECKPOINT ************************/

static void MoveCheckpoint(ObjNode *base)
{
ObjNode	*dish = base->ChainNode;
float	dx,dy,dz;
float	fps = gFramesPerSecondFrac;

		/* SEE IF OUT OF RANGE */

	if (TrackTerrainItem(base))							// just check to see if it's gone
	{
		DeleteObject(base);
		return;
	}

			/* SPIN DISH */

	if (base->CheckpointSpinning)
	{
		dish->Rot.y -= gFramesPerSecondFrac * 4.0f;

		if (base->EffectChannel == -1)
			base->EffectChannel = PlayEffect3D(EFFECT_CHECKPOINTLOOP, &base->Coord);
		else
			Update3DSoundChannel(EFFECT_CHECKPOINTLOOP, &base->EffectChannel, &base->Coord);
	}

	GetObjectInfo(base);



			/* MOVE */

	gDelta.y -= gGravity * fps;					// gravity

	dx = gDelta.x;
	dy = gDelta.y;
	dz = gDelta.z;

	if (base->MPlatform)						// see if factor in moving platform
	{
		ObjNode *plat = base->MPlatform;
		dx += plat->Delta.x;
		dy += plat->Delta.y;
		dz += plat->Delta.z;
	}

	gCoord.x += dx*fps;
	gCoord.y += dy*fps;
	gCoord.z += dz*fps;


			/* DO COLLISION */

	HandleCollisions(base, CTYPE_TERRAIN|CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TRIGGER2, 0);

			/* UPDATE */

	UpdateObject(base);
	dish->Coord = gCoord;
	UpdateObjectTransforms(dish);


		/* SEE IF DISPLAY HELP */

	if (!gHelpMessageDisabled[HELP_MESSAGE_CHECKPOINT])
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < HELP_BEACON_RANGE)
		{
			DisplayHelpMessage(HELP_MESSAGE_CHECKPOINT, .5f, true);
		}
	}

}


/************** DO TRIGGER - CHECKPOINT ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_Checkpoint(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{

#pragma unused (whoNode, sideBits)


			/* DO THE USUAL CHECKPOINT STUFF */

	theNode->CheckpointSpinning = true;
	theNode->CType = CTYPE_MISC;							// dont need to trigger anymore


	if (theNode->CheckpointNum >= gBestCheckpointNum)		// see if this is a better checkpoint than the previous
	{
		gBestCheckpointNum = theNode->CheckpointNum;
		gBestCheckpointCoord.x = gPlayerInfo.coord.x;		// set to coord on previous frame of animation
		gBestCheckpointCoord.y = gPlayerInfo.coord.z;

		gBestCheckpointAim = theNode->ReincarnationAim;
	}


			/* ATTACH SPARKLES */

	MakeCheckPointSparkles(theNode);


	DisableHelpType(HELP_MESSAGE_CHECKPOINT);

	PlayEffect3D(EFFECT_CHECKPOINTHIT, &theNode->Coord);

	return(true);
}


/******************** MAKE CHECKPOINT SPARKLES ******************************/

static void MakeCheckPointSparkles(ObjNode *theNode)
{
int		i;

			/* #1 */

	i = theNode->Sparkles[0] = GetFreeSparkle(theNode);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_TRANSFORMWITHOWNER;

		gSparkles[i].where.x = -7.4f;
		gSparkles[i].where.y = 21.0f;
		gSparkles[i].where.z = -.25f;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 30.0f;
		gSparkles[i].separation = 20.0f;
		gSparkles[i].textureNum = PARTICLE_SObjType_BlueSpark;
	}


			/* #2 */

	i = theNode->Sparkles[1] = GetFreeSparkle(theNode);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_TRANSFORMWITHOWNER;

		gSparkles[i].where.x = 4.125f;
		gSparkles[i].where.y = 21.0f;
		gSparkles[i].where.z = -6.68f;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 30.0f;
		gSparkles[i].separation = 20.0f;
		gSparkles[i].textureNum = PARTICLE_SObjType_BlueSpark;
	}

			/* #3 */

	i = theNode->Sparkles[2] = GetFreeSparkle(theNode);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_TRANSFORMWITHOWNER;

		gSparkles[i].where.x = 4.188f;
		gSparkles[i].where.y = 21.0f;
		gSparkles[i].where.z = 5.43f;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 30.0f;
		gSparkles[i].separation = 20.0f;
		gSparkles[i].textureNum = PARTICLE_SObjType_BlueSpark;
	}




}



#pragma mark -


/************************* ADD BUMPER BUBBLE *********************************/

Boolean AddBumperBubble(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;



					/* MAKE BASE */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SLIME_ObjType_BumperBubble;
	gNewObjectDefinition.scale 		= BUMPERBUBBLE_SCALE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x, z, gNewObjectDefinition.group, gNewObjectDefinition.type, gNewObjectDefinition.scale);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveBumperBubble;
	gNewObjectDefinition.rot 		= 0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType 		= CTYPE_TRIGGER|CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_BLOCKSHADOW;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_BUMPERBUBBLE;


	CreateCollisionBoxFromBoundingBox(newObj, .9, .95);

				/* SET SPIN */

	newObj->BumperBubbleSpin = RandomFloat() * PI2;

	newObj->Coord.x = newObj->InitCoord.x + sin(newObj->BumperBubbleSpin) * BUMPER_BUBBLE_SPIN_RADIUS;
	newObj->Coord.z = newObj->InitCoord.z + cos(newObj->BumperBubbleSpin) * BUMPER_BUBBLE_SPIN_RADIUS;

	return(true);													// item was added
}


/******************* MOVE BUMPER BUBBLE ************************/

static void MoveBumperBubble(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	s,r;

		/* SEE IF OUT OF RANGE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

			/* SPIN */

	r = theNode->BumperBubbleSpin += fps;

	theNode->Coord.x = theNode->InitCoord.x + sin(r) * BUMPER_BUBBLE_SPIN_RADIUS;
	theNode->Coord.z = theNode->InitCoord.z + cos(r) * BUMPER_BUBBLE_SPIN_RADIUS;


			/* WOBBLE */

	s = theNode->BumperWobble3;					// get wobble scaler

	theNode->BumperWobble0 += fps * 10.0f;
	theNode->Scale.y = BUMPERBUBBLE_SCALE + sin(theNode->BumperWobble0) * s;

	theNode->BumperWobble1 -= fps * 14.0f;
	theNode->Scale.x = BUMPERBUBBLE_SCALE + cos(theNode->BumperWobble1) * s;

	theNode->BumperWobble2 += fps * 18.0f;
	theNode->Scale.z = BUMPERBUBBLE_SCALE + sin(theNode->BumperWobble2 + .3f) * s;

	theNode->BumperWobble3 -= fps * .1f;
	if (theNode->BumperWobble3 < 0.0f)
		theNode->BumperWobble3 = 0.0;


		/* UPDATE */

	RotateOnTerrain(theNode, 0, nil);							// set transform matrix
	SetObjectTransformMatrix(theNode);
	CalcObjectBoxFromNode(theNode);
}


/************** DO TRIGGER - BUMPER BUBBLE ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_BumperBubble(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
			/* HIT TOP OF BUMPER */
			//
			// For landing on top we special case it so that we don't get bounced backward - better player control this way.
			//

	if (sideBits & SIDE_BITS_BOTTOM)
	{
		gDelta.y = 2200.0f;
	}

			/* HIT SIDE OF BUMPER */
	else
	{
		float footY = gCoord.y + whoNode->BottomOff;
		OGLVector3D	vec;

		vec.x = gCoord.x - theNode->Coord.x;						// calc vector to player's foot
		vec.y = fabs(footY - theNode->Coord.y);
		vec.z = gCoord.z - theNode->Coord.z;
		FastNormalizeVector(vec.x, vec.y, vec.z, &vec);

		gDelta.x = vec.x * 2500.0f;									// set player delta bounce
		gDelta.y = vec.y * 2500.0f;
		gDelta.z = vec.z * 2500.0f;
		VectorLength2D(gCurrentMaxSpeed, gDelta.x, gDelta.z);		// also let player's max speed to this fast
	}

	theNode->SpecialF[3] = .3f;

	PlayEffect3D(EFFECT_BUMPERBUBBLE, &theNode->Coord);				// play effect

	return(false);
}




#pragma mark -


/************************* ADD FALLING SLIME PLATFORM *********************************/

Boolean AddFallingSlimePlatform(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
float	s;
short	type = itemPtr->parm[0];
int		i;



	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	if (gLevelNum == LEVEL_NUM_BLOBBOSS)
	{
		type = 0;
		gNewObjectDefinition.type 	= BLOBBOSS_ObjType_FallingSlimePlatform_Small;
		gNewObjectDefinition.coord.y = GetTerrainY_Undeformed(x,z) + 400.0f;
	}
	else
	{
		gNewObjectDefinition.type 	= SLIME_ObjType_FallingSlimePlatform_Small + type;
		GetWaterY(x,z, &gNewObjectDefinition.coord.y);
	}

	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y	+= 200.0f;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveFallingSlimePlatform;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= s = 2.0f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_BLOCKSHADOW|CTYPE_IMPENETRABLE|CTYPE_IMPENETRABLE2;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= SIDE_BITS_TOP;						// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_FALLINGSLIMEPLATFORM;

	newObj->Mode			= FALLING_PLATFORM_MODE_WAIT;

	SetObjectCollisionBounds(newObj, 0, -1000,
							newObj->BBox.min.x * s, newObj->BBox.max.x * s,
							newObj->BBox.max.z * s, newObj->BBox.min.x * s);

			/*******************/
			/* ATTACH SPARKLES */
			/*******************/

	switch(type)
	{
		case	0:
				i = newObj->Sparkles[0] = GetFreeSparkle(newObj);				// get free sparkle slot
				if (i != -1)
				{
					gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_TRANSFORMWITHOWNER;

					gSparkles[i].where.x = -71.0f;
					gSparkles[i].where.y = 78.6f;
					gSparkles[i].where.z = 71.0f;

					gSparkles[i].color.r = 1;
					gSparkles[i].color.g = 1;
					gSparkles[i].color.b = 1;
					gSparkles[i].color.a = 1;

					gSparkles[i].scale = 30.0f;
					gSparkles[i].separation = 20.0f;
					gSparkles[i].textureNum = PARTICLE_SObjType_RedSpark;
				}

				i = newObj->Sparkles[1] = GetFreeSparkle(newObj);
				if (i != -1)
				{
					gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_TRANSFORMWITHOWNER;

					gSparkles[i].where.x = 72.5f;
					gSparkles[i].where.y = 78.6f;
					gSparkles[i].where.z = -72.5f;

					gSparkles[i].color.r = 1;
					gSparkles[i].color.g = 1;
					gSparkles[i].color.b = 1;
					gSparkles[i].color.a = 1;

					gSparkles[i].scale = 30.0f;
					gSparkles[i].separation = 20.0f;
					gSparkles[i].textureNum = PARTICLE_SObjType_RedSpark;
				}
				break;

		case	1:
				i = newObj->Sparkles[0] = GetFreeSparkle(newObj);
				if (i != -1)
				{
					gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_TRANSFORMWITHOWNER;

					gSparkles[i].where.x = -235.0f;
					gSparkles[i].where.y = 130.0f;
					gSparkles[i].where.z = -235.0f;

					gSparkles[i].color.r = 1;
					gSparkles[i].color.g = 1;
					gSparkles[i].color.b = 1;
					gSparkles[i].color.a = 1;

					gSparkles[i].scale = 30.0f;
					gSparkles[i].separation = 20.0f;
					gSparkles[i].textureNum = PARTICLE_SObjType_RedSpark;
				}

				i = newObj->Sparkles[1] = GetFreeSparkle(newObj);
				if (i != -1)
				{
					gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_TRANSFORMWITHOWNER;

					gSparkles[i].where.x = 236.5f;
					gSparkles[i].where.y = 130.0f;
					gSparkles[i].where.z = -235.0f;

					gSparkles[i].color.r = 0;
					gSparkles[i].color.g = 1;
					gSparkles[i].color.b = 0;
					gSparkles[i].color.a = 1;

					gSparkles[i].scale = 30.0f;
					gSparkles[i].separation = 20.0f;
					gSparkles[i].textureNum = PARTICLE_SObjType_RedSpark;
				}

				i = newObj->Sparkles[2] = GetFreeSparkle(newObj);
				if (i != -1)
				{
					gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_TRANSFORMWITHOWNER;

					gSparkles[i].where.x = 236.5f;
					gSparkles[i].where.y = 130.0f;
					gSparkles[i].where.z = 235.0f;

					gSparkles[i].color.r = 1;
					gSparkles[i].color.g = 1;
					gSparkles[i].color.b = 1;
					gSparkles[i].color.a = 1;

					gSparkles[i].scale = 30.0f;
					gSparkles[i].separation = 20.0f;
					gSparkles[i].textureNum = PARTICLE_SObjType_RedSpark;
				}

				i = newObj->Sparkles[3] = GetFreeSparkle(newObj);
				if (i != -1)
				{
					gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_TRANSFORMWITHOWNER;

					gSparkles[i].where.x = -235.5f;
					gSparkles[i].where.y = 130.0f;
					gSparkles[i].where.z = 235.0f;

					gSparkles[i].color.r = 1;
					gSparkles[i].color.g = 1;
					gSparkles[i].color.b = 1;
					gSparkles[i].color.a = 1;

					gSparkles[i].scale = 30.0f;
					gSparkles[i].separation = 20.0f;
					gSparkles[i].textureNum = PARTICLE_SObjType_RedSpark;
				}
				break;
	}


	return(true);													// item was added
}


/******************* MOVE FALLING SLIME PLATFORM ************************/

static void MoveFallingSlimePlatform(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

		/* SEE IF OUT OF RANGE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);

	switch(theNode->Mode)
	{
		case	FALLING_PLATFORM_MODE_FALL:

				theNode->FallDelay -= fps;								// see if still delayed
				if (theNode->FallDelay <= 0.0f)
				{
					gDelta.y -= 100.0f * fps;
					gCoord.y += gDelta.y * fps;

					if (gCoord.y < (theNode->InitCoord.y - 600.0f))		// see if reset
						theNode->Mode = FALLING_PLATFORM_MODE_RISE;

				}
				break;


		case	FALLING_PLATFORM_MODE_RISE:
				gDelta.y = 200.0f;
				gCoord.y += gDelta.y * fps;

				if (gCoord.y >= theNode->InitCoord.y)
				{
					gDelta.y = 0;
					theNode->Mode = FALLING_PLATFORM_MODE_WAIT;
					theNode->CType |= CTYPE_TRIGGER;		// its triggerable again
				}

				break;

	}

	UpdateObject(theNode);
}


/************** DO TRIGGER - FALLING SLIME PLATFORM ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_FallingSlimePlatform(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
#pragma unused (sideBits, whoNode, theNode)

	return(true);
}

#pragma mark -


/************************* ADD SPINNING PLATFORM *********************************/

Boolean AddSpinningPlatform(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
float	s,y,r;
int		type = itemPtr->parm[0];
CollisionBoxType *boxPtr;
int		i,c;

static const OGLPoint3D barLights[4] =
{
	{-167.6, 244.5, -570.1},
	{168.8, 244.5, -570.1},
	{168.8, 244.5, 569.0},
	{-167.6, 244.5, 569.0},
};

static const OGLPoint3D xLights[8] =
{
	{-167.6, 244.5, -570.1},
	{168.8, 244.5, -570.1},
	{168.8, 244.5, 569.0},
	{-167.6, 244.5, 569.0},

	{-569.0, 244.8, -167.4},
	{570.1, 244.8, -167.4},
	{570.1, 244.8, 167.4},
	{-570.1, 244.8, 167.4},
};



	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BLOBBOSS_ObjType_BarPlatform_Blue + type;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= y = GetTerrainY_Undeformed(x,z) + 550.0f + ((float)itemPtr->parm[2] * 10.0f);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveSpinningPlatform;
	gNewObjectDefinition.rot 		= gSpinningPlatformRot;
	gNewObjectDefinition.scale 	= s = 2.0f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


				/* CALC COLLISION INFO */

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_TRIGGER2|CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_BLOCKSHADOW;
	newObj->CBits			= CBITS_TOP;
	newObj->TriggerSides 	= SIDE_BITS_TOP;						// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_CIRCULARPLATFORM;

	CreateCollisionBoxFromBoundingBox_Maximized(newObj);			// calc collision box
	newObj->TopOff 	= 0;											// reset the top to eliminate those light poles
	CalcObjectBoxFromNode(newObj);
	KeepOldCollisionBoxes(newObj);


				/* SET SPIN */

	if (itemPtr->parm[3] & 1)							// see if CW or CCW
	{
		newObj->SpinDirectionCCW = true;
		newObj->DeltaRot.y = SPINNING_PLATFORM_SPINSPEED;
	}
	else
	{
		newObj->SpinDirectionCCW = false;
		newObj->DeltaRot.y = -SPINNING_PLATFORM_SPINSPEED;
	}

	newObj->SpinOffset = (float)itemPtr->parm[1] * (PI2 / 8.0f);


			/*********************/
			/* SET PLATFORM INFO */
			/*********************/

	switch(type)
	{
				/* BARS */

		case	0:
		case	1:
		case	2:
		case	3:
				newObj->SpinningPlatformType = SPINNING_PLATFORM_TYPE_BAR;
				for (c = 0; c < 4; c++)
				{
					i = newObj->Sparkles[c] = GetFreeSparkle(newObj);				// get free sparkle slot
					if (i != -1)
					{
						gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_TRANSFORMWITHOWNER;
						gSparkles[i].where = barLights[c];
						gSparkles[i].color.r = 1;
						gSparkles[i].color.g = 1;
						gSparkles[i].color.b = 1;
						gSparkles[i].color.a = 1;
						gSparkles[i].scale = 30.0f;
						gSparkles[i].separation = 20.0f;
						gSparkles[i].textureNum = PARTICLE_SObjType_RedSpark;
					}
				}
				break;

				/* CIRCULAR */

		case	4:
		case	5:
		case	6:
		case	7:
				newObj->SpinningPlatformType = SPINNING_PLATFORM_TYPE_CIRCULAR;
				for (r = 0, c = 0; c < 10; c++, r += PI2/10.0f)
				{
					i = newObj->Sparkles[c] = GetFreeSparkle(newObj);				// get free sparkle slot
					if (i != -1)
					{
						gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_TRANSFORMWITHOWNER|SPARKLE_FLAG_FLICKER;
						gSparkles[i].where.x = sin(r) * newObj->BBox.max.x * .9f;
						gSparkles[i].where.y = 5;
						gSparkles[i].where.z = cos(r) * newObj->BBox.max.x * .9f;
						gSparkles[i].color.r = 1;
						gSparkles[i].color.g = 1;
						gSparkles[i].color.b = 1;
						gSparkles[i].color.a = 1;
						gSparkles[i].scale = 20.0f;
						gSparkles[i].separation = 20.0f;
						if (c&1)
							gSparkles[i].textureNum = PARTICLE_SObjType_RedSpark;
						else
							gSparkles[i].textureNum = PARTICLE_SObjType_BlueSpark;
					}
				}
				break;

				/* COG POLE DOWN */

		case	8:
		case	9:
		case	10:
		case	11:
				newObj->SpinningPlatformType = SPINNING_PLATFORM_TYPE_COG;
				break;


				/* COG POLE UP */

		case	12:
		case	13:
		case	14:
		case	15:
				newObj->SpinningPlatformType = SPINNING_PLATFORM_TYPE_COG;

				newObj->NumCollisionBoxes = 2;									// add the pole collision box
				boxPtr = &newObj->CollisionBoxes[1];							// get ptr to 1st box (presumed only box)
				boxPtr->left 	= x - 60.0f;
				boxPtr->right 	= x + 60.0f;
				boxPtr->top 	= y + 1000.0f;
				boxPtr->bottom 	= y - 10.0f;
				boxPtr->back 	= z - 60.0f;
				boxPtr->front 	= z + 60.0f;
				KeepOldCollisionBoxes(newObj);
				newObj->CBits = CBITS_ALLSOLID;
				break;

				/* X */

		case	16:
		case	17:
		case	18:
		case	19:
				newObj->SpinningPlatformType = SPINNING_PLATFORM_TYPE_X;
				for (c = 0; c < 8; c++)
				{
					i = newObj->Sparkles[c] = GetFreeSparkle(newObj);				// get free sparkle slot
					if (i != -1)
					{
						gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_TRANSFORMWITHOWNER;
						gSparkles[i].where = xLights[c];
						gSparkles[i].color.r = 1;
						gSparkles[i].color.g = 1;
						gSparkles[i].color.b = 1;
						gSparkles[i].color.a = 1;
						gSparkles[i].scale = 30.0f;
						gSparkles[i].separation = 20.0f;
						gSparkles[i].textureNum = PARTICLE_SObjType_RedSpark;
					}
				}
				break;

		default:
				DoFatalAlert("AddSpinningPlatform: unknown type");
	}


	return(true);													// item was added
}


/************************* MOVE SPINNING PLATFORM *******************************/

static void MoveSpinningPlatform(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))									// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	if (theNode->SpinDirectionCCW)
		theNode->Rot.y = gSpinningPlatformRot + theNode->SpinOffset;	// set to global rotation (use global to keep all platforms in sync)
	else
		theNode->Rot.y = -gSpinningPlatformRot - theNode->SpinOffset;

	UpdateObjectTransforms(theNode);
}


/************** DO TRIGGER - SPINNING PLATFORM ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static Boolean DoTrig_SpinningPlatform(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
double			dist,deltaRot;
double			platX, platZ, fps = gFramesPerSecondFrac;
float			s;
OGLMatrix3x3	m;
OGLPoint2D		origin,pt,p[12];


#pragma unused (sideBits, whoNode)

	platX = theNode->Coord.x;
	platZ = theNode->Coord.z;

	s = theNode->Scale.x;												// get scale

	switch(theNode->SpinningPlatformType)
	{
						/*************************************/
						/* SEE IF OFF THE EDGE OF THE CIRCLE */
						/*************************************/

		case	SPINNING_PLATFORM_TYPE_CIRCULAR:
				dist = CalcDistance(gCoord.x, gCoord.z, platX, platZ);
				if (dist > ((theNode->BBox.max.x * s) + 30.0f))
					return(false);												// fall off
				break;


						/**********************************/
						/* SEE IF OFF THE EDGE OF THE COG */
						/**********************************/

		case	SPINNING_PLATFORM_TYPE_COG:

						/* QUCK CHECK TO SEE IF OUT OF OUTER RADIUS */

				dist = CalcDistance(gCoord.x, gCoord.z, platX, platZ);
				if (dist > ((theNode->BBox.max.x * s) + 30.0f))
					return(false);												// fall off

						/* QUCK CHECK TO SEE IF IN INNER RADIUS */

				if (dist > ((180.0f * s) + 30.0f))								// nope, on the cogs
				{
								/* DO POLY COLLISION FOR COGS */

								/* PLANK A */

					p[0].x = -144.8f * s;
					p[0].y = -244.8f * s;

					p[1].x = 0;
					p[1].y = -271.6f * s;

					p[2].x = 144.8f * s;
					p[2].y = 244.8f * s;

					p[3].x = p[1].x;
					p[3].y = 267.6f * s;


							/* PLANK B */

					p[4].x = -271.6f * s;
					p[4].y = 0;

					p[5].x = -244.8f * s;
					p[5].y = -144.8f * s;

					p[6].x = 271.0f * s;
					p[6].y = 0;

					p[7].x = 244.8f * s;
					p[7].y = 144.8f * s;

							/* PLANK C */

					p[8].x = -144.6f * s;
					p[8].y = 244.8f * s;

					p[9].x = -239.8f * s;
					p[9].y = 144.8f * s;

					p[10].x = 144.8f * s;
					p[10].y = -244.8f * s;

					p[11].x = 244.8f * s;
					p[11].y = -144.8f * s;

					OGLMatrix3x3_SetRotate(&m, -theNode->Rot.y);
					OGLPoint2D_TransformArray(p, &m, p, 12);										// transform points

					if (!IsPointInPoly2D(gCoord.x - platX, gCoord.z - platZ, 4, &p[0]))			// see if player is in plank A
						if (!IsPointInPoly2D(gCoord.x - platX, gCoord.z - platZ, 4, &p[4]))		// see if player is in plank B
							if (!IsPointInPoly2D(gCoord.x - platX, gCoord.z - platZ, 4, &p[8]))	// see if player is in plank C
								return(false);
				}
				break;


						/**********************************/
						/* SEE IF OFF THE EDGE OF THE BAR */
						/**********************************/

		case	SPINNING_PLATFORM_TYPE_BAR:

							/* CALC 4 CORNERS OF BAR */

				p[0].x = theNode->BBox.min.x * s - 30.0f;
				p[0].y = theNode->BBox.min.z * s - 30.0f;

				p[1].x = theNode->BBox.max.x * s + 30.0f;
				p[1].y = p[0].y;

				p[2].x = p[1].x;
				p[2].y = theNode->BBox.max.z * s + 30.0f;

				p[3].x = p[0].x;
				p[3].y = p[2].y;
				OGLMatrix3x3_SetRotate(&m, -theNode->Rot.y);
				OGLPoint2D_TransformArray(p, &m, p, 4);				// transform points

				if (!IsPointInPoly2D(gCoord.x - platX, gCoord.z - platZ, 4, p))		// see if player is in there
					return(false);

				break;


						/************************************/
						/* SEE IF OFF THE EDGE OF THE X-BAR */
						/************************************/

		case	SPINNING_PLATFORM_TYPE_X:
							/* PLANK A */

				p[0].x = -230.0f * s;
				p[0].y = -670.0f * s;

				p[1].x = 230.0f * s;
				p[1].y = p[0].y;

				p[2].x = p[1].x;
				p[2].y = 670.0f * s;

				p[3].x = p[0].x;
				p[3].y = p[2].y;


						/* PLANK B */

				p[4].x = -670.0f * s;
				p[4].y = -230.0f * s;

				p[5].x = 670.0f * s;
				p[5].y = p[4].y;

				p[6].x = p[5].x;
				p[6].y = 230.0f * s;

				p[7].x = p[4].x;
				p[7].y = p[6].y;

				OGLMatrix3x3_SetRotate(&m, -theNode->Rot.y);
				OGLPoint2D_TransformArray(p, &m, p, 8);										// transform points

				if (!IsPointInPoly2D(gCoord.x - platX, gCoord.z - platZ, 4, &p[0]))			// see if player is in plank A
					if (!IsPointInPoly2D(gCoord.x - platX, gCoord.z - platZ, 4, &p[4]))		// see if player is in plank B
						return(false);
				break;

		default:
				DoFatalAlert("DoTrig_SpinningPlatform: unknown type");
	}

				/*******************/
				/* MOVE THE PLAYER */
				/*******************/

			/* SPIN PLAYER AROUND THE PLATFORM'S CENTER */

 	deltaRot = fps * theNode->DeltaRot.y;							// get rotation delta

   	origin.x = platX;
	origin.y = platZ;
	OGLMatrix3x3_SetRotateAboutPoint(&m, &origin, -deltaRot);		// do (-) cuz 2D y rot is reversed from 3D y rot

	pt.x = gCoord.x;
	pt.y = gCoord.z;
	OGLPoint2D_Transform(&pt, &m, &pt);								// transform player's coord
	gCoord.x = pt.x;
	gCoord.z = pt.y;

	whoNode->Rot.y += deltaRot;										// spin player

	return(true);
}





