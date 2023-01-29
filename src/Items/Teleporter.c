/****************************/
/*   	TELEPORTER.C	    */
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

static void MoveTeleporter(ObjNode *teleporter);
static void ActivateTeleporter(ObjNode *teleporter);
static void InitiatePlayerTeleportation(ObjNode *teleporter);
static void MovePlayer_Teleporter(ObjNode *player);
static Boolean TeleporterHitBySuperNova(ObjNode *weapon, ObjNode *teleporter, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_TELEPORTERS		20

enum
{
	TELEPORT_MODE_DEMATERIALIZE,
	TELEPORT_MODE_MATERIALIZE
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	TeleporterID	Special[0]
#define	DestinationID	Special[1]

#define	Activated		Flag[0]

static OGLPoint3D	gTeleporterTargetCoords;
static OGLPoint3D	gTeleporterCoords[MAX_TELEPORTERS];
static	float		gTeleporterRot[MAX_TELEPORTERS];
static Boolean		gTeleporterActive[MAX_TELEPORTERS];

static Byte			gTeleportMode;
static	float		gTeleporterTargetRot;

/******************* INIT TELEPORTERS ************************/

void InitTeleporters(void)
{
long					i;
TerrainItemEntryType	*itemPtr;
short					id;
float					rot;


			/**********************************/
			/* FIND COORDS OF ALL TELEPORTERS */
			/**********************************/

	itemPtr = *gMasterItemList; 									// get pointer to data inside the LOCKED handle

	for (i= 0; i < gNumTerrainItems; i++)
	{
		if (itemPtr[i].type == MAP_ITEM_TELEPORTER)					// see if it's what we're looking for
		{
			id = itemPtr[i].parm[0];								// get teleporter ID
			if (id >= MAX_TELEPORTERS)
				DoFatalAlert("InitTeleporters: ID# >= MAX_TELEPORTERS");

			rot = gTeleporterRot[id] = itemPtr[i].parm[2] * (PI2/8.0f) + PI;			// get rot

			gTeleporterCoords[id].x = itemPtr[i].x - sin(rot) * 130.0f;					// get coords
			gTeleporterCoords[id].z = itemPtr[i].y - cos(rot) * 130.0f;
			gTeleporterCoords[id].y = GetTerrainY_Undeformed(gTeleporterCoords[id].x,gTeleporterCoords[id].z);

			gTeleporterActive[id] = false;
		}
	}
}

/************************* ADD TELEPORTER *********************************/

Boolean AddTeleporter(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj,*console;

	int id = itemPtr->parm[0];
	int destinationID = itemPtr->parm[1];

	float rotation = itemPtr->parm[2] * (PI2/8.0f);

	// There's no console model for the 12<-->13 pair
	Boolean consoleModelExists = destinationID >= 0 && destinationID <= 11;

			/************************/
			/* MAKE TELEPORTER ARCH */
			/************************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= APOCALYPSE_ObjType_Teleporter;
	gNewObjectDefinition.scale 		= 1.5;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x, z, gNewObjectDefinition.group, gNewObjectDefinition.type, gNewObjectDefinition.scale);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 70;
	gNewObjectDefinition.moveCall 	= MoveTeleporter;
	gNewObjectDefinition.rot 		= rotation;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->TeleporterID = id;										// get id#
	newObj->DestinationID = destinationID;							// get dest id#


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_IMPENETRABLE|CTYPE_BLOCKCAMERA|CTYPE_LIGHTNINGROD;

	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= TeleporterHitBySuperNova;

			/****************/
			/* MAKE CONSOLE */
			/****************/

	gNewObjectDefinition.type		= APOCALYPSE_ObjType_Console0 + (consoleModelExists? newObj->DestinationID: 0);
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	console = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	if (!consoleModelExists)
		console->StatusBits |= STATUS_BIT_HIDDEN;

	newObj->ChainNode = console;

	if (gTeleporterActive[id])									// see if this guy is active
		ActivateTeleporter(newObj);



	return(true);													// item was added
}


/************** TELEPORTER HIT BY SUPERNOVA *****************/

static Boolean TeleporterHitBySuperNova(ObjNode *weapon, ObjNode *teleporter, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused(weapon, weaponCoord, weaponDelta)

	gTeleporterActive[teleporter->TeleporterID] =
	gTeleporterActive[teleporter->DestinationID] = true;
	return(false);
}

/************************* MOVE TELEPORTER ****************************/

static void MoveTeleporter(ObjNode *teleporter)
{
float	fps = gFramesPerSecondFrac;

	if (TrackTerrainItem(teleporter))							// just check to see if it's gone
	{
		DeleteObject(teleporter);
		return;
	}

	if (!teleporter->Activated)											// see if not activated
	{
		if (gTeleporterActive[teleporter->TeleporterID])				// see if should be activated
			ActivateTeleporter(teleporter);
		else
			return;														// not activated, so bail
	}
	else
	{
		if (teleporter->EffectChannel == -1)								// update sound
			teleporter->EffectChannel = PlayEffect3D(EFFECT_TELEPORTERDRONE, &teleporter->Coord);
		else
			Update3DSoundChannel(EFFECT_TELEPORTERDRONE, &teleporter->EffectChannel, &teleporter->Coord);

	}

		/***************/
		/* UPDATE ZAPS */
		/***************/

	ObjNode* zap1 = teleporter->ChainNode->ChainNode;			// 1st zap is chained off of console
	if (zap1)
	{
		ObjNode* zap2 = zap1->ChainNode;
		if (zap2)
		{
			zap1->Rot.z += RandomFloat()*PI2;
			zap2->Rot.z += RandomFloat()*PI2;
			UpdateObjectTransforms(zap1);
			UpdateObjectTransforms(zap2);
		}
	}

	float dist = OGLPoint3D_Distance(&teleporter->Coord, &gPlayerInfo.coord);		// calc dist to player

			/* SEE IF INITIATE TELEPORT */

	if (dist < (281 * teleporter->Scale.x))							// see if within teleportation radius
	{
			/* DETERMINE WHICH SIDE PLAYER IS/WAS ON */

		OGLMatrix3x3	m;

		OGLPoint3D tele = teleporter->Coord;
		OGLPoint3D oldP = gPlayerInfo.objNode->OldCoord;
		OGLPoint3D newP = gPlayerInfo.objNode->Coord;

		OGLVector2D oldT2P = {oldP.x - tele.x, oldP.z - tele.z};	// old vector teleporter --> player
		OGLVector2D newT2P = {newP.x - tele.x, newP.z - tele.z};	// new vector teleporter --> player
		FastNormalizeVector2D(oldT2P.x, oldT2P.y, &oldT2P, true);
		FastNormalizeVector2D(newT2P.x, newT2P.y, &newT2P, true);

		OGLVector2D gateVector = {1, 0};
		OGLMatrix3x3_SetRotate(&m, -teleporter->Rot.y);				// not sure why angle must be negated for correct behavior on non-axis-aligned teleporters?
		OGLVector2D_Transform(&gateVector, &m, &gateVector);

		float oldSide = OGLVector2D_Cross(&gateVector, &oldT2P);
		float newSide = OGLVector2D_Cross(&gateVector, &newT2P);

			/* SEE IF CROSSED FROM FRONT TO BACK - THEN START TELEPORT */

		if ((oldSide * newSide) < 0.0f)								// if (-) then it changed signs, thus changed sides
		{
			InitiatePlayerTeleportation(teleporter);
		}
	}

			/************************/
			/* ABSORPTION PARTICLES */
			/************************/

	if (dist < 900.0f)
	{
		teleporter->ParticleTimer += fps;
		while (teleporter->ParticleTimer > 0.01f)
		{
			teleporter->ParticleTimer -= 0.01f;

			short particleGroup	= teleporter->ParticleGroup;
			uint32_t magicNum	= teleporter->ParticleMagicNum;

			if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
			{
				teleporter->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

				NewParticleGroupDefType groupDef =
				{
					.magicNum				= magicNum,
					.type					= PARTICLE_TYPE_GRAVITOIDS,
					.flags					= PARTICLE_FLAGS_DONTCHECKGROUND,
					.gravity				= 0,
					.magnetism				= 0,
					.baseScale				= 30,
					.decayRate				= 0,
					.fadeRate				= 1,
					.particleTextureNum		= PARTICLE_SObjType_WhiteSpark3,
					.srcBlend				= GL_SRC_ALPHA,
					.dstBlend				= GL_ONE,
				};
				teleporter->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
			}

			if (particleGroup != -1)
			{
				OGLPoint3D p = gPlayerInfo.coord;
				p.y += RandomFloat2() * 90;

				OGLVector3D d =
				{
					teleporter->Coord.x - p.x,
					teleporter->Coord.y - p.y + 200,
					teleporter->Coord.z - p.z,
				};

				NewParticleDefType	newParticleDef =
				{
					.groupNum	= particleGroup,
					.where		= &p,
					.delta		= &d,
					.scale		= RandomFloat() + 1.0f,
					.rotZ		= RandomFloat()*PI2,
					.rotDZ		= 0,
					.alpha		= .9f,
				};

				if (AddParticleToGroup(&newParticleDef))
					teleporter->ParticleGroup = -1;
			}
		}
	}


		/******************/
		/* UPDATE SPARKLE */
		/******************/

	int i = teleporter->Sparkles[0];
	if (i != -1)
	{
		gSparkles[i].color.a -= fps * 1.0f;
		if (gSparkles[i].color.a <= 0.0f)
		{
			DeleteSparkle(i);
			teleporter->Sparkles[0] = -1;
		}
	}

}



/*********************** ACTIVATE TELEPORTER **************************/

static void ActivateTeleporter(ObjNode *teleporter)
{
ObjNode	*zap1, *zap2;

	if (teleporter->Activated)
		return;

	teleporter->Activated = true;

			/************************/
			/* MAKE AND ATTACH ZAPS */
			/************************/

				/* ZAP 1 */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= APOCALYPSE_ObjType_TeleporterZap;
	gNewObjectDefinition.coord		= teleporter->Coord;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_KEEPBACKFACES | STATUS_BIT_GLOW |
									 STATUS_BIT_NOLIGHTING | STATUS_BIT_NOZWRITES | STATUS_BIT_ROTZXY | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB + 4;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= teleporter->Rot.y + (PI/14.0f);
	gNewObjectDefinition.scale 		= teleporter->Scale.x;
	zap1 = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	zap1->ColorFilter.a = .8;
	teleporter->ChainNode->ChainNode = zap1;

				/* ZAP 2 */

	gNewObjectDefinition.slot++;
	gNewObjectDefinition.rot 		= teleporter->Rot.y - (PI/14.0f);
	zap2 = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	zap2->ColorFilter.a = .5;
	zap1->ChainNode = zap2;


}


#pragma mark -

/*************** INITIATE PLAYER TELEPORTATION **************************/

static void InitiatePlayerTeleportation(ObjNode *teleporter)
{
ObjNode *player = gPlayerInfo.objNode;
short	i;

	player->MoveCall = MovePlayer_Teleporter;

	gTeleporterTargetCoords = gTeleporterCoords[teleporter->DestinationID];
	gTeleporterTargetRot = gTeleporterRot[teleporter->DestinationID];

	gTeleportMode = TELEPORT_MODE_DEMATERIALIZE;

	player->Rot.y = teleporter->Rot.y + PI;
	player->Delta.x = player->Delta.y = player->Delta.z = 0;


			/* MAKE SPARKLE */


	i = teleporter->Sparkles[0] = GetFreeSparkle(teleporter);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
		gSparkles[i].where = player->Coord;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 240.0f;
		gSparkles[i].separation = 100.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_BlueSpark;
	}


	PlayEffect_Parms3D(EFFECT_PLAYERTELEPORT, &teleporter->Coord, NORMAL_CHANNEL_RATE, 1.5);
}


/****************** MOVE PLAYER: TELEPORTER **************************/

static void MovePlayer_Teleporter(ObjNode *player)
{
ObjNode	*leftHand = gPlayerInfo.leftHandObj;
ObjNode	*rightHand = gPlayerInfo.rightHandObj;
float	fps = gFramesPerSecondFrac;

	gPlayerInfo.isTeleporting = true;

	switch(gTeleportMode)
	{
						/* FADE OUT */

		case	TELEPORT_MODE_DEMATERIALIZE:
				player->Scale.x = player->Scale.z -= fps * 2.5f;
				if (player->Scale.x <= 0.0f)							// once completely faded, move to target location
				{
					float r= player->Rot.y;
					player->Scale.x = player->Scale.z = 0;
					gPlayerInfo.coord.x = player->Coord.x = player->OldCoord.x = gTeleporterTargetCoords.x - sin(r) * 30.0f;
					gPlayerInfo.coord.z = player->Coord.z = player->OldCoord.z = gTeleporterTargetCoords.z - cos(r) * 30.0f;
					gPlayerInfo.coord.y = player->Coord.y = player->OldCoord.y = GetTerrainY(gPlayerInfo.coord.x, gPlayerInfo.coord.z) - player->BottomOff;
					gTeleportMode = TELEPORT_MODE_MATERIALIZE;
					InitCamera();
					gCameraUserRotY = PI;							// make camera look at Otto in the face when re-materializes
					gForceCameraAlignment = true;
					player->Rot.y = gTeleporterTargetRot;
				}

				break;


						/* FADE IN */

		case	TELEPORT_MODE_MATERIALIZE:
				player->Scale.x = player->Scale.z += fps * 2.5f;
				if (player->Scale.x >= gPlayerInfo.scale)			// once completely faded, move to target location
				{
					float r= player->Rot.y;

					player->Scale.x = player->Scale.z = gPlayerInfo.scale;
					player->MoveCall = MovePlayer_Robot;
					SetPlayerWalkAnim(player);

					player->Delta.x = -sin(r) * 1300.0f;				// give some forward momentum
					player->Delta.z = -cos(r) * 1300.0f;

					gPlayerInfo.isTeleporting = false;
				}
				break;
	}

	leftHand->Scale.x =	rightHand->Scale.x = player->Scale.x;		// also scale hands
	leftHand->Scale.z =	rightHand->Scale.z = player->Scale.z;

	UpdateObjectTransforms(player);
	UpdateRobotHands(player);
}

