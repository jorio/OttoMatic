/****************************/
/*   	FARMER.C		    */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


#include "game.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	OGLPoint3D			gCoord;
extern	PlayerInfoType		gPlayerInfo;
extern	OGLVector3D			gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLVector3D			gRecentTerrainNormal;
extern	SplineDefType		**gSplineList;
extern	u_long				gAutoFadeStatusBits;
extern	short				gNumCollisions,gDisplayedHelpMessage;
extern	SuperTileStatus	**gSuperTileStatusGrid;
extern	NewParticleGroupDefType	gNewParticleGroupDef;
extern	PrefsType			gGamePrefs;
extern	int					gLevelNum,gNumHumansInTransit;
extern	ObjNode				*gFirstNodePtr,*gSaucerTarget,*gExitRocket,*gPlayerSaucer;
extern	SparkleType			gSparkles[];
extern	Boolean				gHelpMessageDisabled[],gOttoLeftSaucerIntoRocket,gPlayerHasLanded;
extern	short				gNumHumansInSaucer;
extern	Byte				gHumansInSaucerList[];

/****************************/
/*    PROTOTYPES            */
/****************************/



static Boolean HumanIceHitByWeapon(ObjNode *weapon, ObjNode *ice, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean BlowUpPeopleHut(ObjNode *hut, float unused);
static void MoveHuman_ToRocketRamp(ObjNode *human);
static void MoveHuman_UpRocketRamp(ObjNode *human);
static void UpdateHumanFromSaucer(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/


#define	RingOffset	SpecialF[0]

int		gNumHumansRescuedTotal;
short	gNumHumansRescuedOfType[NUM_HUMAN_TYPES];

float	gHumanScaleRatio;

#define	HumanTypeInHut		Special[0]
#define	NumPeopleInHut		Special[1]


/********************* INIT HUMANS **************************/

void InitHumans(void)
{
int	i;

	gNumHumansRescuedTotal	= 0;

	for (i = 0; i < NUM_HUMAN_TYPES; i++)
		gNumHumansRescuedOfType[i] = 0;


	if (gLevelNum == LEVEL_NUM_SAUCER)
		gHumanScaleRatio = .5f;
	else
		gHumanScaleRatio = 1.0f;


}


/********************** DO HUMAN COLLISION DETECT **************************/

void DoHumanCollisionDetect(ObjNode *theNode)
{

			/* HANDLE BASICS */

	HandleCollisions(theNode, CTYPE_MISC|CTYPE_FENCE|CTYPE_TERRAIN|CTYPE_MPLATFORM|CTYPE_TRIGGER2, 0);
	if (gNumCollisions > 0)
	{
		if (theNode->Skeleton->AnimNum == 1)					// see if human was walking
		{
			MorphToSkeletonAnim(theNode->Skeleton, 0, 3);		// set to universal stand anim
		}
	}
}



/************************ ADD HUMAN *************************/

Boolean AddHuman(TerrainItemEntryType *itemPtr, long x, long z)
{

	switch(itemPtr->parm[0])
	{
				/* FARMER */
		case	0:
				return (AddFarmer(itemPtr, x, z));

				/* BEE WOMAN */
		case	1:
				return (AddBeeWoman(itemPtr,x,z));

				/* SKIRT LADY */
		case	2:
				return (AddSkirtLady(itemPtr,x,z));

				/* SCIENTIST */
		case	3:
				return (AddScientist(itemPtr,x,z));


	}

	return(false);
}


/******************* PRIME HUMAN *************************/

Boolean PrimeHuman(long splineNum, SplineItemType *itemPtr)
{
	switch(itemPtr->parm[0])
	{
				/* FARMER */
		case	0:
				return (PrimeFarmer(splineNum, itemPtr));


				/* BEE WOMAN */
		case	1:
				return (PrimeBeeWoman(splineNum, itemPtr));

				/* SKIRT LADY */
		case	2:
				return (PrimeSkirtLady(splineNum, itemPtr));

				/* SCIENTIST */
		case	3:
				return (PrimeScientist(splineNum, itemPtr));

	}

	return(false);
}



/********************* START HUMAN TELEPORT ***********************/

void StartHumanTeleport(ObjNode *human)
{
ObjNode	*ring1,*ring2;
int		i;

	DisableHelpType(HELP_MESSAGE_SAVEHUMAN);

	human->TerrainItemPtr = nil;		// dont come back

		/***********************/
		/* MAKE TELEPORT RINGS */
		/***********************/

			/* RING 1 */

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_TeleportRing;
	gNewObjectDefinition.coord.x 	= human->Coord.x;
	gNewObjectDefinition.coord.z 	= human->Coord.z;
	gNewObjectDefinition.coord.y 	= human->Coord.y;
	gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES|STATUS_BIT_GLOW|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOZWRITES;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+20;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .3;
	ring1 = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	ring1->RingOffset = 0;
	ring1->ColorFilter.a = .99;
	ring1->Rot.x = RandomFloat2() * .3f;
	ring1->Rot.z = RandomFloat2() * .3f;
	human->ChainNode = ring1;


			/* RING 2 */

	gNewObjectDefinition.scale 		= .7;
	ring2 = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	ring2->RingOffset = 0;
	ring2->ColorFilter.a = .99;
	ring2->Rot.x = RandomFloat2() * .3f;
	ring2->Rot.z = RandomFloat2() * .3f;
	ring1->ChainNode = ring2;


			/* MAKE SPARKLE */


	i = human->Sparkles[0] = GetFreeSparkle(human);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
		gSparkles[i].where = human->Coord;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 240.0f;
		gSparkles[i].separation = 100.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_BlueSpark;
	}

	PlayEffect3D(EFFECT_TELEPORTHUMAN, &human->Coord);
}


/******************** UPDATE HUMAN TELEPORT *********************/
//
// return true if deleted
//

Boolean UpdateHumanTeleport(ObjNode *human, Boolean fadeOut)
{
float	fps = gFramesPerSecondFrac;
ObjNode *ring1,*ring2;
int		i;

			/* GET RINGS */

	ring1 = human->ChainNode;
	ring2 = ring1->ChainNode;


			/* MOVE RINGS */

	ring1->RingOffset += fps * 100.0f;
	ring1->Coord.x = human->Coord.x;
	ring1->Coord.y = human->Coord.y + ring1->RingOffset;
	ring1->Coord.z = human->Coord.z;
	ring1->Scale.x = ring1->Scale.y = ring1->Scale.z += fps * .5f;
	ring1->Rot.y += fps * PI2;

	ring2->RingOffset += fps * 100.0f;
	ring2->Coord.x = human->Coord.x;
	ring2->Coord.y = human->Coord.y - ring2->RingOffset;
	ring2->Coord.z = human->Coord.z;
	ring2->Scale.x = ring2->Scale.y = ring2->Scale.z -= fps * .5f;
	ring2->Rot.y -= fps * PI2;

	UpdateObjectTransforms(ring1);
	UpdateObjectTransforms(ring2);

	if (fadeOut)
	{
				/* FADE OUT */

		human->ColorFilter.a -= fps * .7f;
		if (human->ColorFilter.a <= 0.0f)
		{
			DeleteObject(human);
			return(true);
		}

		ring1->ColorFilter.a = human->ColorFilter.a;
		ring2->ColorFilter.a = human->ColorFilter.a;
	}
			/* FADE IN */
	else
	{
		human->ColorFilter.a += fps * .7f;
		if (human->ColorFilter.a >= 1.0f)
		{
			human->ColorFilter.a = 1.0f;
		}

		ring1->ColorFilter.a = 1.0f - human->ColorFilter.a;
		ring2->ColorFilter.a = 1.0f - human->ColorFilter.a;
	}


	if (human->ShadowNode)
		human->ShadowNode->ColorFilter.a = human->ColorFilter.a;


		/* UPDATE SPARKLE */

	i = human->Sparkles[0];
	if (i != -1)
	{
		gSparkles[i].color.a -= fps * 1.0f;
		if (gSparkles[i].color.a <= 0.0f)
		{
			DeleteSparkle(i);
			human->Sparkles[0] = -1;
		}
	}


	return(false);
}


/****************** DO TRIG:  HUMAN ************************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_Human(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
#pragma unused (whoNode, sideBits)

			/* SEE IF REMOVE FROM SPLINE */

	if (theNode->StatusBits & STATUS_BIT_ONSPLINE)
		DetachObjectFromSpline(theNode, theNode->MoveCall);

	theNode->StatusBits &= ~STATUS_BIT_SAUCERTARGET;			// no longer a saucer target

	if (gSaucerTarget == theNode)
		gSaucerTarget = nil;

	theNode->Delta.x =
	theNode->Delta.y =
	theNode->Delta.z = 0;

	theNode->CType = CTYPE_MISC;

	StartHumanTeleport(theNode);

	MorphToSkeletonAnim(theNode->Skeleton, 3, 6);

	gNumHumansRescuedTotal++;							// inc rescue counter
	gNumHumansRescuedOfType[theNode->HumanType]++;

	return(true);
}


/*************** UPDATE HUMAN *********************/

void UpdateHuman(ObjNode *theNode)
{
	if (!(theNode->StatusBits & STATUS_BIT_ONSPLINE))						// don't call update if on spline
		UpdateObject(theNode);

	if (!theNode->InIce)
	{
		CallAlienSaucer(theNode);
		CheckHumanHelp(theNode);
	}

			/* CHECK SAUCER SHADOW FADING */

	if (gLevelNum == LEVEL_NUM_SAUCER)
	{
		SeeIfUnderPlayerSaucerShadow(theNode);
	}
}


/************* CHECK HUMAN HELP ***************/

void CheckHumanHelp(ObjNode *theNode)
{
			/* SPECIAL CASE FOR SAUCER LEVEL */

	if (gLevelNum == LEVEL_NUM_SAUCER)
	{
		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < 600.0f)
		{
			DisplayHelpMessage(HELP_MESSAGE_BEAMHUMANS, 3.0f, true);
		}
		return;
	}


			/* ALL OTHER LEVELS */

	if (gHelpMessageDisabled[HELP_MESSAGE_SAVEHUMAN])		// quick check to see if this message is disabled
		return;

	if ((gDisplayedHelpMessage != HELP_MESSAGE_NONE) &&		// dont override any other message for this
		(gDisplayedHelpMessage != HELP_MESSAGE_SAVEHUMAN))
		return;

	if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < 600.0f)
	{
		DisplayHelpMessage(HELP_MESSAGE_SAVEHUMAN, 2.0f, false);
	}

}


#pragma mark -


/******************** ENCASE HUMAN IN ICE *************************/

void EncaseHumanInIce(ObjNode *human)
{
ObjNode	*ice;
float	scaleX,scaleZ,scaleY;

	scaleX = (human->BBox.max.x - human->BBox.min.x) * 1.7f;
	scaleY = (human->BBox.max.y - human->BBox.min.y) * 1.1f;
	scaleZ = (human->BBox.max.z - human->BBox.min.z) * 1.7f;

				/************/
				/* MAKE ICE */
				/************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FIREICE_ObjType_Peoplecicle;
	gNewObjectDefinition.scale 		= scaleX;
	gNewObjectDefinition.coord.x	= human->Coord.x;
	gNewObjectDefinition.coord.z 	= human->Coord.z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(human->Coord.x, human->Coord.z, gNewObjectDefinition.group, gNewObjectDefinition.type, gNewObjectDefinition.scale);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_KEEPBACKFACES |
									 STATUS_BIT_NOFOG | STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-3;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= human->Rot.y;
	ice = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	ice->ColorFilter.a = .99;

	ice->CType = CTYPE_MISC|CTYPE_AUTOTARGETWEAPON;
	ice->CBits = CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(ice, 1,1);

				/* SET WEAPON HANDLERS */

	ice->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= HumanIceHitByWeapon;
	ice->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= HumanIceHitByWeapon;
	ice->HitByWeaponHandler[WEAPON_TYPE_FLARE] 		= HumanIceHitByWeapon;
	ice->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= HumanIceHitByWeapon;

	ice->Health = .5f;


	ice->Scale.y = scaleY;
	ice->Scale.z = scaleZ;
	UpdateObjectTransforms(ice);

			/**********************/
			/* MODIFY HUMAN PARMS */
			/**********************/

	human->InIce = true;
	human->Skeleton->AnimSpeed = 0;
	human->CType &= ~CTYPE_TRIGGER;

	human->ChainNode = ice;
	ice->ChainHead = human;
}


/************ HUMAN ICE HIT BY WEAPON *****************/

static Boolean HumanIceHitByWeapon(ObjNode *weapon, ObjNode *ice, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weaponCoord,weaponDelta, weapon)

ObjNode	*human = ice->ChainHead;

	ExplodeGeometry(ice, 300, SHARD_MODE_FROMORIGIN|SHARD_MODE_BOUNCE, 1, .7);
	PlayEffect3D(EFFECT_SHATTER, &ice->Coord);

	human->ChainNode = nil;								// detach ice from chain
	DeleteObject(ice);									// delete ice

		/* SET HUMAN BACK TO NORMAL */

	human->InIce = false;
	human->Skeleton->AnimSpeed = 1.0f;
	human->CType |= CTYPE_TRIGGER;

	return(true);			// stop weapon
}


#pragma mark -


/******************** MOVE HUMAN:  TO PLAYER SAUCER *********************/

void MoveHuman_ToPlayerSaucer(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->ColorFilter.r =
	theNode->ColorFilter.g =
	theNode->ColorFilter.b = 1.0;					// make sure bright white

	GetObjectInfo(theNode);

	theNode->Rot.y -= fps * PI2;					// spin

	gCoord.x = gPlayerInfo.coord.x;					// keep aligned w/ saucer
	gCoord.z = gPlayerInfo.coord.z;

	gCoord.y += fps * 130.0f;						// move up to saucer

	UpdateHumanFromSaucer(theNode);


				/* SEE IF DONE */

	if (gCoord.y >= (gPlayerInfo.coord.y - 50.0f))		// see if @ saucer
	{
		gHumansInSaucerList[gNumHumansInSaucer] = theNode->HumanType;	// add to saucer list
		gNumHumansInSaucer++;											// inc count
		gNumHumansInTransit--;

		DeleteObject(theNode);

		return;
	}

}


/******************** MOVE HUMAN:  BEAMDOWN *********************/

void MoveHuman_BeamDown(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	y,diff;

	GetObjectInfo(theNode);

	theNode->Rot.y -= fps * PI2;					// spin

	gCoord.y -= fps * 130.0f;						// move down to ground


				/* SEE IF DONE */

	y = gCoord.y + theNode->BBox.min.y;					// get bottom of human
	diff = y - GetTerrainY(gCoord.x, gCoord.z);			// get dist to ground

	if (diff <= 0.0f)									// see if hit ground or under
	{
		gCoord.y -= diff;								// adjust back up so flush

		MorphToSkeletonAnim(theNode->Skeleton, 1, 4);	// make walk
		theNode->MoveCall = MoveHuman_ToRocketRamp;


		if (theNode->CType & CTYPE_PLAYER)			// if that was Otto then stop the beam
			StopPlayerSaucerBeam(gPlayerSaucer);
		else
			gNumHumansInTransit--;
	}


	UpdateHumanFromSaucer(theNode);

}


/******************** MOVE HUMAN:  TO ROCKET RAMP *********************/

static void MoveHuman_ToRocketRamp(ObjNode *human)
{
float	r, rampX, rampZ;
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(human);

	human->Skeleton->AnimSpeed = 2.0f;

			/* TURN TOWARD RAMP PT */

	r = gExitRocket->Rot.y;
	rampX = gExitRocket->Coord.x + sin(r) * 200.0f;
	rampZ = gExitRocket->Coord.z + cos(r) * 200.0f;
	TurnObjectTowardTarget(human, &gCoord, rampX, rampZ, 3.0f, false);


			/* MOVE TOWARD RAMP PT */

	r = human->Rot.y;
	gDelta.x = -sin(r) * 200.0f;
	gDelta.z = -cos(r) * 200.0f;

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;

	gCoord.y = GetTerrainY(gCoord.x, gCoord.z) - human->BBox.min.y;


			/* SEE IF @ RAMP PT */

	if (CalcDistance(gCoord.x, gCoord.z, rampX, rampZ) < 10.0f)
	{
		human->MoveCall = MoveHuman_UpRocketRamp;
	}


			/* UPDATE */

	UpdateHumanFromSaucer(human);
}


/******************** MOVE HUMAN:  UP ROCKET RAMP *********************/

static void MoveHuman_UpRocketRamp(ObjNode *human)
{
float	r,dist;
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(human);

	human->Skeleton->AnimSpeed = 1.8f;

			/* AIM TOWARD CENTER OF ROCKET */

	TurnObjectTowardTarget(human, &gCoord, gExitRocket->Coord.x, gExitRocket->Coord.z, PI, false);


			/* MOVE TOWARD CENTER */

	r = human->Rot.y;
	gDelta.x = -sin(r) * 120.0f;
	gDelta.z = -cos(r) * 120.0f;

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;

	if (gCoord.y < (GetTerrainY(gCoord.x, gCoord.z) + 200.0f))
		gCoord.y += 100.0f * fps;								// move up ramp


			/* FADE ONCE CLOSE ENOUGH */

	dist = CalcDistance(gCoord.x, gCoord.z, gExitRocket->Coord.x, gExitRocket->Coord.z);
	if (dist < 60.0f)
	{
		human->ColorFilter.a -= fps * 1.3f;

		if (human->ColorFilter.a <= 0.0f)
		{
					/* SEE IF THAT WAS OTTO ENTERING THE ROCKET */

			if (human->CType & CTYPE_PLAYER)
			{
				gPlayerHasLanded = false;
				if (gExitRocket->Mode == ROCKET_MODE_WAITING2)
					gExitRocket->Mode = ROCKET_MODE_CLOSEDOOR;

			}

			/* COUNT THIS HUMAN AS RESCUED */

			else
			{
				gNumHumansRescuedTotal++;								// add to rescued list
				gNumHumansRescuedOfType[human->HumanType]++;

				PlayEffect3D(EFFECT_TELEPORTHUMAN, &human->Coord);

				DeleteObject(human);

				gPlayerInfo.fuel += .05f;								// get fuel for that
				if (gPlayerInfo.fuel > 1.0f)							// see if we're now full of fuel
				{
					gPlayerInfo.fuel = 1.0f;
				}

				if (gNumHumansRescuedTotal > 1)							// dont show this message after the first human
					DisableHelpType(HELP_MESSAGE_FUELFORHUMANS);
				else
					DisplayHelpMessage(HELP_MESSAGE_FUELFORHUMANS, 4.0f, false);
			}
			return;
		}
	}

				/* UPDATE */

	UpdateHumanFromSaucer(human);
}


/********************* UPDATE HUMAN FROM SAUCER **********************/

static void UpdateHumanFromSaucer(ObjNode *theNode)
{
	if (theNode->CType & CTYPE_PLAYER)			// see if actually Otto
	{
		UpdateRobotHands(theNode);
		UpdatePlayerSparkles(theNode);
	}

	UpdateObject(theNode);



}


#pragma mark -



/************************ ADD PEOPLE HUT *************************/

Boolean AddPeopleHut(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;
short	humanType = itemPtr->parm[1];
short	numHumans = itemPtr->parm[0];

	if (numHumans == 0)												// see if random #
		numHumans = 1 + (MyRandomLong()&7);


			/******************/
			/* CREATE THE HUT */
			/******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SAUCER_ObjType_PeopleHut;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 70;
	gNewObjectDefinition.moveCall 	= MoveObjectUnderPlayerSaucer;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Update(newObj, 1,1);

	newObj->BoundingSphereRadius =  newObj->BBox.max.x * newObj->Scale.x;	// set correct bounding sphere

	newObj->HurtCallback = BlowUpPeopleHut;							// set kaboom callback


	newObj->HumanTypeInHut = humanType;
	newObj->NumPeopleInHut = numHumans;

	return(true);
}



/************************ BLOW UP PEOPLE HUT ***************************/

static Boolean BlowUpPeopleHut(ObjNode *hut, float unused)
{
#pragma unused(unused)
short	humanType = hut->HumanTypeInHut;
short	n = hut->NumPeopleInHut;
int		i,h;
float	x,z;
ObjNode	*human;

	hut->CType = 0;														// clear this so humans don't get created on top of it

				/* CREATE THE HUMANS */

	for (i = 0; i < n; i++)
	{
		if (humanType == 0)												// see if pick a random human type
			h = MyRandomLong()&0x3;
		else
			h = humanType-1;

		x = hut->Coord.x + RandomFloat2() * (hut->BoundingSphereRadius * .8f);
		z = hut->Coord.z + RandomFloat2() * (hut->BoundingSphereRadius * .8f);

		switch(h)
		{
			case	HUMAN_TYPE_FARMER:
					human = MakeFarmer(x,z);
					break;

			case	HUMAN_TYPE_BEEWOMAN:
					human = MakeBeeWoman(x,z);
					break;

			case	HUMAN_TYPE_SCIENTIST:
					human = MakeScientist(x,z);
					break;


			case	HUMAN_TYPE_SKIRTLADY:
					human = MakeSkirtLady(x,z);
					break;
		}


				/* SET SOME HUMAN INFO */

		human->Rot.y = RandomFloat()*PI2;								// random aim
		SetSkeletonAnim(human->Skeleton, 1);							// anim #1 is "walk" for all humans
		human->HumanWalkTimer = 2.0f + RandomFloat() * 6.0f;			// walk for random time
	}


				/* BLOW UP THE HUT */

	PlayEffect_Parms3D(EFFECT_SAUCERKABOOM, &hut->Coord, NORMAL_CHANNEL_RATE, 2.0);
	MakeSparkExplosion(hut->Coord.x, hut->Coord.y, hut->Coord.z, 400.0f, 1.0, PARTICLE_SObjType_RedSpark,0);
	ExplodeGeometry(hut, 1000, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .5);
	hut->TerrainItemPtr = nil;
	DeleteObject(hut);


				/* SHOW HELP MESSAGE */

	DisplayHelpMessage(HELP_MESSAGE_BEAMHUMANS, 4.0f, true);

	return(false);										// return value doesn't mean anything
}








