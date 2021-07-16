//
// triggers.h
//

#pragma once

#undef AddAtom	// some windows header defines this


#define	TRIGGER_SLOT	4					// needs to be early in the collision list
#define	TriggerSides	Special[5]


		/* TRIGGER TYPES */
enum
{
	TRIGTYPE_WOODENGATE,
	TRIGTYPE_METALGATE,
	TRIGTYPE_HUMAN,
	TRIGTYPE_CORNKERNEL,
	TRIGTYPE_CHECKPOINT,
	TRIGTYPE_BUMPERBUBBLE,
	TRIGTYPE_FALLINGSLIMEPLATFORM,
	TRIGTYPE_BUBBLEPUMP,
	TRIGTYPE_CIRCULARPLATFORM,
	TRIGTYPE_JUNGLEGATE,
	TRIGTYPE_TURTLE,
	TRIGTYPE_SMASHABLE,
	TRIGTYPE_LEAFPLATFORM,
	TRIGTYPE_POWERUPPOD,
	TRIGTYPE_DEBRISGATE,
	TRIGTYPE_CHAINREACTINGMINE,
	TRIGTYPE_CRUNCHDOOR,
	TRIGTYPE_BUMPERCAR,
	TRIGTYPE_BUMPERCARPOWERPOST,
	TRIGTYPE_ROCKETSLED,
	TRIGTYPE_TRAPDOOR,
	TRIGTYPE_LAVAPLATFORM
};


#define	SPINNING_PLATFORM_SPINSPEED 	.5f

#define	SpinningPlatformType	Special[0]

enum
{
	SPINNING_PLATFORM_TYPE_CIRCULAR,
	SPINNING_PLATFORM_TYPE_BAR,
	SPINNING_PLATFORM_TYPE_X,
	SPINNING_PLATFORM_TYPE_COG

};


enum
{
	FALLING_PLATFORM_MODE_WAIT,
	FALLING_PLATFORM_MODE_FALL,
	FALLING_PLATFORM_MODE_RISE
};


//===============================================================================

Boolean HandleTrigger(ObjNode *triggerNode, ObjNode *whoNode, Byte side);
void SpewAtoms(OGLPoint3D *where, short numRed, short numGreen, short numBlue, Boolean regenerate);
Boolean AddAtom(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddWoodenGate(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddMetalGate(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddCheckpoint(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddBumperBubble(TerrainItemEntryType *itemPtr, long  x, long z);
void MoveMagnetAfterDrop(ObjNode *theNode);
Boolean AddFallingSlimePlatform(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddPowerupPod(TerrainItemEntryType *itemPtr, long  x, long z);
void AddPowerupToInventory(ObjNode *pow);
void MovePowerup(ObjNode *);

Boolean AddSpinningPlatform(TerrainItemEntryType *itemPtr, long  x, long z);
void UpdatePlayerGrowth(ObjNode *player);

Boolean DoTrig_CrunchDoor(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

void MoveWoodenGateChunk(ObjNode *theNode);


			/* triggers2.c */
			
Boolean AddJungleGate(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_JungleGate(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);


Boolean AddTurtlePlatform(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_TurtlePlatform(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

Boolean AddSmashable(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_Smashable(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

Boolean AddLeafPlatform(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_LeafPlatform(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

Boolean DoTrig_PowerupPod(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

Boolean AddDebrisGate(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_DebrisGate(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

Boolean AddChainReactingMine(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_ChainReactingMine(ObjNode *mine, ObjNode *whoNode, Byte sideBits);

Boolean AddTrapDoor(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_TrapDoor(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
