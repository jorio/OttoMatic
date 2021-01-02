/****************************/
/*   	TELEPORTER.C	    */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


#include "3dmath.h"
#include <AGL/aglmacro.h>

extern	float				gFramesPerSecondFrac,gFramesPerSecond,gCurrentAspectRatio,gSpinningPlatformRot;
extern	OGLPoint3D			gCoord;
extern	OGLVector3D			gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLBoundingBox 		gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	OGLSetupOutputType	*gGameViewInfoPtr;
extern	u_long				gAutoFadeStatusBits,gGlobalMaterialFlags;
extern	Boolean				gForceCameraAlignment;
extern	PlayerInfoType		gPlayerInfo;
extern	int					gLevelNum;
extern	SplineDefType	**gSplineList;
extern	SparkleType	gSparkles[MAX_SPARKLES];
extern	short				gNumEnemies,gNumPods,gNumTerrainItems;
extern	SpriteType	*gSpriteGroupList[];
extern	AGLContext		gAGLContext;
extern	TerrainItemEntryType 	**gMasterItemList;
extern	float				gCameraUserRotY;

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
				DoFatalAlert("\pInitTeleporters: ID# >= MAX_TELEPORTERS");
			
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
short	id;
				
	id = itemPtr->parm[0];
	
	if (id >= 12)						// error check since there is a 12 <--> 13 pair of teleporters which we have no 3D model for (for 1.0.2 update)
		return(true);
				
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
	gNewObjectDefinition.rot 		= itemPtr->parm[2] * (PI2/8.0f);
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->TeleporterID = id;										// get id#
	newObj->DestinationID = itemPtr->parm[1];						// get dest id#


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_IMPENETRABLE|CTYPE_BLOCKCAMERA|CTYPE_LIGHTNINGROD;
//	newObj->CBits			= CBITS_ALLSOLID;
//	CreateCollisionBoxFromBoundingBox_Rotated(newObj,.8,1);

	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= TeleporterHitBySuperNova;

			/****************/
			/* MAKE CONSOLE */
			/****************/

	gNewObjectDefinition.type 		= APOCALYPSE_ObjType_Console0 + newObj->DestinationID;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	console = MakeNewDisplayGroupObject(&gNewObjectDefinition);


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
ObjNode *zap1,*zap2,*player = gPlayerInfo.objNode;
float	dist,oldSide,newSide;
float	fps = gFramesPerSecondFrac;
OGLMatrix3x3	m;
OGLVector2D		oldTtoV,v,newTtoV;
int				i;

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

	zap1 = teleporter->ChainNode->ChainNode;			// 1st zap is chained off of console
	if (zap1)
	{
		zap2 = zap1->ChainNode;
		if (zap2)
		{
			zap1->Rot.z += RandomFloat()*PI2;
			zap2->Rot.z += RandomFloat()*PI2;
			UpdateObjectTransforms(zap1);
			UpdateObjectTransforms(zap2);
		}
	}
	
			/*****************************************/
			/* DETERMINE WHICH SIDE PLAYER IS/WAS ON */
			/*****************************************/
	
	dist = OGLPoint3D_Distance(&teleporter->Coord, &gPlayerInfo.coord);					// calc dist to player
	if (dist < 900.0f)
	{	
					/* CALC PERPENDICULAR AIM VECTOR OF TELEPORTER */
		v.x = 1;
		v.y = 0;
		OGLMatrix3x3_SetRotate(&m, teleporter->Rot.y);
		OGLVector2D_Transform(&v,&m,&v);
		
						/* CALC TELE->PLAYER VECTORS */
	
		oldTtoV.x = player->OldCoord.x - teleporter->Coord.x;					// calc vector to old player coord
		oldTtoV.y = player->OldCoord.z - teleporter->Coord.z;
		FastNormalizeVector2D(oldTtoV.x, oldTtoV.y,&oldTtoV,true);

		newTtoV.x = player->Coord.x - teleporter->Coord.x;						// calc vector to new player coord
		newTtoV.y = player->Coord.z - teleporter->Coord.z;
		FastNormalizeVector2D(newTtoV.x, newTtoV.y,&newTtoV,true);
				
						/* DETERMINE SIDEDNESS */
						
		oldSide = OGLVector2D_Cross(&v, &oldTtoV);
		newSide = OGLVector2D_Cross(&v, &newTtoV);

			/* SEE IF CROSSED FROM FRONT TO BACK - THEN START TELEPORT */
					
		if (dist < (281.0f * teleporter->Scale.x))								// see if within zap radius
		{
			if ((oldSide * newSide) < 0.0f)										// if (-) then it changed signs, thus changed sides
			{
				InitiatePlayerTeleportation(teleporter);			
			}
		}
			
			/**********************************/
			/* SEE IF DO ABSORPTOIN PARTICLES */
			/**********************************/
		
		{
			int					particleGroup,magicNum;
			NewParticleGroupDefType	groupDef;
			NewParticleDefType	newParticleDef;
			OGLVector3D			d;
			OGLPoint3D			p;
		
			teleporter->ParticleTimer += fps;
			if (teleporter->ParticleTimer > 0.01f)
			{
				teleporter->ParticleTimer += 0.01f;

				particleGroup 	= teleporter->ParticleGroup;
				magicNum 		= teleporter->ParticleMagicNum;

				if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
				{
					teleporter->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num
					
					groupDef.magicNum				= magicNum;
					groupDef.type					= PARTICLE_TYPE_GRAVITOIDS;
					groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
					groupDef.gravity				= 0;
					groupDef.magnetism				= 0;
					groupDef.baseScale				= 30;
					groupDef.decayRate				= 0;
					groupDef.fadeRate				= 1.0;
					groupDef.particleTextureNum		= PARTICLE_SObjType_WhiteSpark3;
					groupDef.srcBlend				= GL_SRC_ALPHA;
					groupDef.dstBlend				= GL_ONE;
					teleporter->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
				}

				if (particleGroup != -1)
				{
					p.x = gPlayerInfo.coord.x;
					p.y = gPlayerInfo.coord.y + RandomFloat2() * 100.0f;
					p.z = gPlayerInfo.coord.z;

					d.x = teleporter->Coord.x - p.x;
					d.y = teleporter->Coord.y + 200.0f - p.y;
					d.z = teleporter->Coord.z - p.z;
				
					newParticleDef.groupNum		= particleGroup;
					newParticleDef.where		= &p;
					newParticleDef.delta		= &d;
					newParticleDef.scale		= RandomFloat() + 1.0f;
					newParticleDef.rotZ			= RandomFloat()*PI2;
					newParticleDef.rotDZ		= 0;
					newParticleDef.alpha		= .9;		
					if (AddParticleToGroup(&newParticleDef))
						teleporter->ParticleGroup = -1;
				}
			}
		}
	}
	
	
		/******************/
		/* UPDATE SPARKLE */
		/******************/
	
	i = teleporter->Sparkles[0];	
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
				}
				break;
	}

	leftHand->Scale.x =	rightHand->Scale.x = player->Scale.x;		// also scale hands
	leftHand->Scale.z =	rightHand->Scale.z = player->Scale.z;
	
	UpdateObjectTransforms(player);
	UpdateRobotHands(player);
}





















