/****************************/
/*   	HUMANS.C		    */
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

static Boolean HumanIceHitByWeapon(ObjNode *weapon, ObjNode *ice, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean BlowUpPeopleHut(ObjNode *hut, float unused);
static void MoveHuman(ObjNode* theNode);
static void MoveHumanOnSpline(ObjNode* theNode);
static void MoveHuman_Standing(ObjNode* theNode);
static void MoveHuman_Walking(ObjNode* theNode);
static void MoveHuman_Teleport(ObjNode* theNode);
static void MoveHuman_ToRocketRamp(ObjNode *human);
static void MoveHuman_UpRocketRamp(ObjNode *human);
static void UpdateHumanFromSaucer(ObjNode *theNode);
static void AbductHuman(ObjNode* theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	HUMAN_SCALE			(2.0f * gHumanScaleRatio)

enum
{
	HUMAN_ANIM_STAND,
	HUMAN_ANIM_WALK,
	HUMAN_ANIM_ABDUCTED,
	HUMAN_ANIM_TELEPORT
};

/*********************/
/*    VARIABLES      */
/*********************/


#define	RingOffset	SpecialF[0]

int		gNumHumansInLevel = 0;
int		gNumHumansRescuedTotal;
int		gNumHumansRescuedOfType[NUM_HUMAN_TYPES];

float	gHumanScaleRatio;

#define	HumanTypeInHut		Special[0]
#define	NumPeopleInHut		Special[1]


/********************* GET SKELETON FROM HUMAN TYPE **************************/

static Byte GetSkeletonFromHumanType(Byte humanType)
{
	switch (humanType)
	{
		case HUMAN_TYPE_FARMER:		return SKELETON_TYPE_FARMER;
		case HUMAN_TYPE_BEEWOMAN:	return SKELETON_TYPE_BEEWOMAN;
		case HUMAN_TYPE_SCIENTIST:	return SKELETON_TYPE_SCIENTIST;
		case HUMAN_TYPE_SKIRTLADY:	return SKELETON_TYPE_SKIRTLADY;
		default:
			DoFatalAlert("GetSkeletonFromHumanType: who?");
			return SKELETON_TYPE_FARMER;
	}
}


/********************* INIT HUMANS **************************/

void InitHumans(void)
{
	gNumHumansRescuedTotal	= 0;

	for (int i = 0; i < NUM_HUMAN_TYPES; i++)
		gNumHumansRescuedOfType[i] = 0;


	if (gLevelNum == LEVEL_NUM_SAUCER)
		gHumanScaleRatio = .5f;
	else
		gHumanScaleRatio = 1.0f;


		/* COUNT STATIC HUMANS */

	gNumHumansInLevel = 0;
	for (int i = 0; i < gNumTerrainItems; i++)
	{
		TerrainItemEntryType* item = &(*gMasterItemList)[i];

		switch (item->type)
		{
			case MAP_ITEM_HUMAN_SCIENTIST:
			case MAP_ITEM_HUMAN:
				gNumHumansInLevel++;
				break;

			case MAP_ITEM_PEOPLE_HUT:
				if (item->parm[0] == 0)												// see if random #
				{
					item->parm[0] = 1 + (MyRandomLong() & 7);
				}
				gNumHumansInLevel += item->parm[0];
				break;
		}
	}

		/* COUNT SPLINE-BOUND HUMANS */

	for (int i = 0; i < gNumSplines; i++)
	{
		const SplineDefType* spline = &(*gSplineList)[i];

		for (int j = 0; j < spline->numItems; j++)
		{
			switch ((*spline->itemList)[j].type)
			{
				case MAP_ITEM_HUMAN:
					gNumHumansInLevel++;
					break;

				case MAP_ITEM_HUMAN_SCIENTIST:
					gNumHumansInLevel++;
					break;

				case MAP_ITEM_PEOPLE_HUT:
					GAME_ASSERT_MESSAGE(false, "people hut moving along spline!?");
					break;
			}
		}
	}
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



/************************ MAKE HUMAN *************************/

ObjNode *MakeHuman(int humanType, float x, float z)
{
ObjNode	*newObj;

		/* MAKE OBJECT */

	gNewObjectDefinition.type 		= GetSkeletonFromHumanType(humanType);
	gNewObjectDefinition.animNum	= HUMAN_ANIM_STAND;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= FindHighestCollisionAtXZ(x,z,CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TERRAIN);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_SAUCERTARGET|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= HUMAN_SLOT;
	gNewObjectDefinition.moveCall	= MoveHuman;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= HUMAN_SCALE;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);


				/* SET BETTER INFO */

	newObj->HumanType = humanType;

	newObj->Coord.y 	-= newObj->BBox.min.y - 20.0f;
	UpdateObjectTransforms(newObj);

	newObj->Health 		= 1.0;

	newObj->SaucerTargetType = SAUCER_TARGET_TYPE_ABDUCT;
	newObj->SaucerAbductHandler = AbductHuman;


				/* SET COLLISION INFO */

	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_HUMAN;
	newObj->CType			= CTYPE_HUMAN|CTYPE_TRIGGER;
	newObj->CBits			= CBITS_ALLSOLID|CBITS_ALWAYSTRIGGER;
	CreateCollisionBoxFromBoundingBox(newObj, 1,1);



				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8.0f * gHumanScaleRatio, 8.0f * gHumanScaleRatio, true);

	return(newObj);
}


/************************ ADD HUMAN *************************/

Boolean AddHuman(TerrainItemEntryType* itemPtr, long x, long z)
{
	ObjNode* newObj;

	Byte humanType = itemPtr->parm[0];

	/* MAKE OBJECT */

	newObj = MakeHuman(humanType, x, z);
	newObj->TerrainItemPtr = itemPtr;


	/* SEE IF ENCASED IN ICE */

	if (itemPtr->parm[3] & 1)
		EncaseHumanInIce(newObj);

	return true;
}


/******************* PRIME HUMAN *************************/

Boolean PrimeHuman(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	Byte humanType = itemPtr->parm[0];

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);



		/* MAKE OBJECT */

	gNewObjectDefinition.type 		= GetSkeletonFromHumanType(humanType);
	gNewObjectDefinition.animNum	= HUMAN_ANIM_WALK;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= STATUS_BIT_SAUCERTARGET|STATUS_BIT_ONSPLINE|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= HUMAN_SLOT;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= HUMAN_SCALE;
	gNewObjectDefinition.moveCall 	= MoveHuman;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);


	newObj->Skeleton->AnimSpeed = 1.5;


				/* SET MORE INFO */

	newObj->HumanType = humanType;

	newObj->SaucerTargetType = SAUCER_TARGET_TYPE_ABDUCT;
	newObj->SaucerAbductHandler = AbductHuman;

	newObj->SplineItemPtr 	= itemPtr;
	newObj->SplineNum 		= splineNum;
	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveHumanOnSpline;						// set move call
	newObj->CBits			= CBITS_ALLSOLID|CBITS_ALWAYSTRIGGER;

	newObj->TriggerSides 	= ALL_SOLID_SIDES;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_HUMAN;
	newObj->CType			= CTYPE_MISC|CTYPE_HUMAN|CTYPE_TRIGGER;

	CreateCollisionBoxFromBoundingBox(newObj,1,1);

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8.0f * gHumanScaleRatio, 8.0f * gHumanScaleRatio, false);


			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);									// detach this object from the linked list
	AddToSplineObjectList(newObj, true);


	return(true);
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


/********************* MOVE HUMAN **************************/

static void MoveHuman(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveHuman_Standing,
					MoveHuman_Walking,
					MoveHuman_Standing,					// abducted
					MoveHuman_Teleport,
				};

	if (gLevelNum != LEVEL_NUM_SAUCER)
	{
		if (TrackTerrainItem(theNode))						// just check to see if it's gone
		{
			if (gSaucerTarget == theNode)					// see if was a saucer target
				gSaucerTarget = nil;
			DeleteObject(theNode);
			return;
		}
	}
	GetObjectInfo(theNode);
	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}


/******************** MOVE HUMAN ON SPLINE ***************************/

static void MoveHumanOnSpline(ObjNode *theNode)
{
Boolean isVisible;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility


		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 30.0f * gHumanScaleRatio);

	GetObjectCoordOnSpline(theNode);


			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/

	if (isVisible)
	{
		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,	// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z) - theNode->BottomOff;	// get ground Y
		UpdateObjectTransforms(theNode);												// update transforms
		CalcObjectBoxFromNode(theNode);													// update collision box

		UpdateHuman(theNode);
		UpdateShadow(theNode);
	}

			/* NOT VISIBLE */
	else
	{
	}
}


/********************** MOVE HUMAN: STANDING ******************************/

static void  MoveHuman_Standing(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	dx,dy,dz;

			/* MOVE */

	gDelta.x = gDelta.z = 0;					// no intertia while standing

	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity

	dx = gDelta.x;
	dy = gDelta.y;
	dz = gDelta.z;

	if (theNode->MPlatform)						// see if factor in moving platform
	{
		ObjNode *plat = theNode->MPlatform;
		dx += plat->Delta.x;
		dy += plat->Delta.y;
		dz += plat->Delta.z;
	}

	gCoord.x += dx*fps;
	gCoord.y += dy*fps;
	gCoord.z += dz*fps;


			/* COLLISION */

	DoHumanCollisionDetect(theNode);

	UpdateHuman(theNode);
}


/********************** MOVE HUMAN: WALKING ******************************/

static void  MoveHuman_Walking(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	r;

	r = theNode->Rot.y;							// get aim
	gDelta.x = -sin(r) * 100.0f;				// set delta
	gDelta.z = -cos(r) * 100.0f;

			/* MOVE */

	gDelta.y -= ENEMY_GRAVITY*fps;				// add gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* COLLISION */

	DoHumanCollisionDetect(theNode);

	UpdateHuman(theNode);


			/* SEE IF DONE WALKING */

	theNode->HumanWalkTimer -= fps;
	if (theNode->HumanWalkTimer <= 0.0f)
		MorphToSkeletonAnim(theNode->Skeleton, 0, 3);				// go to stand anim
}

/********************** MOVE HUMAN: TELEPORT ******************************/

static void  MoveHuman_Teleport(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->ColorFilter.r = theNode->ColorFilter.b -= fps * .5f;

	UpdateObject(theNode);


			/* UPDATE TELEPORT EFFECT */

	if (UpdateHumanTeleport(theNode, true))				// returns true if human is deleted
		return;

}


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
				{
					MorphToSkeletonAnim(human->Skeleton, PLAYER_ANIM_STAND, 1);		// stop footsteps
					gExitRocket->Mode = ROCKET_MODE_CLOSEDOOR;
				}

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


/**************** ABDUCT HUMAN **************************/
//
// Callback called when alien saucer is in position and ready to abduct.
//

static void AbductHuman(ObjNode *theNode)
{
			/* SEE IF REMOVE FROM SPLINE */

	if (theNode->StatusBits & STATUS_BIT_ONSPLINE)
		DetachObjectFromSpline(theNode, nil);

	theNode->CType = 0;					// no longer interesting
	theNode->Delta.x =
	theNode->Delta.y =
	theNode->Delta.z = 0;				// stop him

	MorphToSkeletonAnim(theNode->Skeleton, HUMAN_ANIM_ABDUCTED, 6);

	theNode->TerrainItemPtr = nil;		// dont come back

	if (gLevelNum == LEVEL_NUM_SAUCER)	// if abducted by player saucer then change move call
		theNode->MoveCall = MoveHuman_ToPlayerSaucer;
	else
		theNode->MoveCall = nil;
}


#pragma mark -


/************************ ADD PEOPLE HUT *************************/

Boolean AddPeopleHut(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;
short	humanType = itemPtr->parm[1];
short	numHumans = itemPtr->parm[0];

	GAME_ASSERT_MESSAGE(numHumans != 0, "PeopleHut human count should have been set in InitHumans!");


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

		human = MakeHuman(h, x, z);


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


#pragma mark -

/************************ ADD/PRIME SCIENTIST HUMAN *************************/
//
// Terrain item type 61 is reserved for the scientist,
// so we have to define special callbacks for him.
//

Boolean AddHumanScientist(TerrainItemEntryType* itemPtr, long x, long z)
{
	itemPtr->parm[0] = HUMAN_TYPE_SCIENTIST;
	return AddHuman(itemPtr, x, z);
}

Boolean PrimeHumanScientist(long splineNum, SplineItemType* itemPtr)
{
	itemPtr->parm[0] = HUMAN_TYPE_SCIENTIST;
	return PrimeHuman(splineNum, itemPtr);
}

