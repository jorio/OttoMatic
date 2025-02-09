/****************************/
/*   	PLAYER.C   			*/
/* By Brian Greenstone      */
/* (c)2001 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void InitRocketShip(OGLPoint3D *where, float rot);
static void MoveRocketShip(ObjNode *rocket);
static void MoveRocketShip_Landing(ObjNode *rocket);
static void MoveRocketShip_Leave(ObjNode *rocket);
static void MoveRocketShip_OpenDoor(ObjNode *rocket);
static void MoveRocketShip_Deplane(ObjNode *rocket);
static void MoveRocketShip_PreEnter(ObjNode *rocket);
static void MoveRocketShip_Enter(ObjNode *rocket);
static void MoveRocketShip_CloseDoor(ObjNode *rocket);
static void MoveRocketShip_Waiting(ObjNode *rocket);
static void MoveRocketShip_Waiting2(ObjNode *rocket);
static void ExplodePlayer(void);
static void CheckIfPlayerSuckedIntoBlobWell(ObjNode *player, ObjNode *rocket);
static void AlignRocketDoor(ObjNode *rocket);


/****************************/
/*    CONSTANTS             */
/****************************/


#define	ROCKET_SCALE	.8f


#define	DOOR_OFF_Y	444.688f
#define	DOOR_OFF_Z	77.563f

#define	EXHAUST_YOFF	(270.0f * ROCKET_SCALE)

#define	PLAYER_OFF_Y	(445.8f * ROCKET_SCALE)
#define PLAYER_OFF_Z	(9.3f * ROCKET_SCALE)

static const OGLPoint2D kRocketShipHotZoneTemplate[4] =
{
	{-40, ROCKET_SCALE * 370.0f},
	{40, ROCKET_SCALE * 370.0f},
	{50, ROCKET_SCALE * 490.0f},
	{-50, ROCKET_SCALE * 490.0f},
};

static const OGLPoint2D kRocketShipRampEntryPoint = {0, ROCKET_SCALE*400.0f};


/*********************/
/*    VARIABLES      */
/*********************/


PlayerInfoType	gPlayerInfo;

static	float	gDeplaneTimer;

float	gDeathTimer = 0;

Boolean	gPlayerHasLanded = false;
Boolean	gPlayerIsDead = false;
Boolean	gExplodePlayerAfterElectrocute = false;

ObjNode	*gExitRocket;

Boolean	gPlayerFellIntoBottomlessPit;

float	gRocketScaleAdjust;
OGLPoint2D	gRocketShipHotZone[4];


/******************** INIT PLAYER INFO ***************************/
//
// Called once at beginning of game
//

void InitPlayerInfo_Game(void)
{

			/* INIT SOME THINGS IF NOT LOADING SAVED GAME */

	if (!gPlayingFromSavedGame)
	{
		gScore = 0;

		gPlayerInfo.lives			= 4;
		gPlayerInfo.health 			= 1.0;
		gPlayerInfo.jumpJet 		= 0;
	}


	gDeathTimer = 0;
	gDoDeathExit = false;

	gPlayerInfo.startX 			= 0;
	gPlayerInfo.startZ 			= 0;
	gPlayerInfo.coord.x 		= 0;
	gPlayerInfo.coord.y 		= 0;
	gPlayerInfo.coord.z 		= 0;

	gPlayerInfo.didCheat		= false;
}


/******************* INIT PLAYER AT START OF LEVEL ********************/
//
// Initializes player stuff at the beginning of each level.
//

void InitPlayersAtStartOfLevel(void)
{
	InitWeaponInventory();								// always clear weapon inventory on each level

	gExitRocket = nil;

	if (gLevelNum == LEVEL_NUM_SAUCER)							// scale down rocket for saucer level
		gRocketScaleAdjust = .4f;
	else
		gRocketScaleAdjust = 1.0f;


	gPlayerFellIntoBottomlessPit = false;

	gPlayerInfo.growMode 	= GROWTH_MODE_NONE;
	gPlayerInfo.scale 		= PLAYER_DEFAULT_SCALE;
	gPlayerInfo.scaleRatio 	= 1.0;
	gPlayerInfo.giantTimer 	= 0;

	gPlayerInfo.leftHandObj = gPlayerInfo.rightHandObj = nil;
	gPlayerInfo.jumpJetSpeed = 0;

	gPlayerInfo.superNovaCharge = 0;
	gPlayerInfo.superNovaStatic = nil;

	gPlayerInfo.invincibilityTimer = 0;

	if ((gLevelNum == LEVEL_NUM_BLOBBOSS) ||				// player still has full fuel on Blob Boss from prev level
		(gLevelNum == LEVEL_NUM_JUNGLEBOSS))
		gPlayerInfo.fuel = 1;
	else
		gPlayerInfo.fuel = 0;

	gPlayerInfo.autoAimTimer = 0;
	gPlayerInfo.burnTimer = 0;

	gPlayerIsDead = false;

		/* FIRST PRIME THE TERRAIN TO CAUSE ALL OBJECTS TO BE GENERATED BEFORE WE PUT THE PLAYER DOWN */

	InitCurrentScrollSettings();
	DoPlayerTerrainUpdate(gPlayerInfo.coord.x, gPlayerInfo.coord.z);


			/**************************/
			/* THEN CREATE THE PLAYER */
			/**************************/

	gFreezeCameraFromY 		= false;					// assume no camera freeze
	gFreezeCameraFromXZ		= false;

	gPlayerHasLanded = true;							// assume true, will get overridden by rocket init code if not true
	switch(gLevelNum)
	{
		case	LEVEL_NUM_BLOBBOSS:
				InitPlayer_Robot(&gPlayerInfo.coord, gPlayerInfo.startRotY);
				break;

		case	LEVEL_NUM_SAUCER:
				InitPlayer_Saucer(&gPlayerInfo.coord);
				break;

		default:
				InitRocketShip(&gPlayerInfo.coord, gPlayerInfo.startRotY);
				InitPlayer_Robot(&gPlayerInfo.coord, gPlayerInfo.startRotY);

				gPlayerInfo.objNode->StatusBits |= STATUS_BIT_NOMOVE;		// disable player's move function until ship has landed
	}

	gBestCheckpointCoord.x = gPlayerInfo.coord.x;					// set first checkpoint @ starting location
	gBestCheckpointCoord.y = gPlayerInfo.coord.z;


}


#pragma mark -



/********************* PLAYER GOT HIT ****************************/
//
// Normally damage comes from byWhat, but if byWhat = nil, then use altDamage
//
// Returns true if player did take damage; false if player was invincible.
//

Boolean PlayerGotHit(ObjNode *byWhat, float altDamage)
{
ObjNode	*player = gPlayerInfo.objNode;

	if (gPlayerInfo.invincibilityTimer > 0.0f)							// cant get hit if invincible
		return false;

	if (player->Skeleton == nil)									// make sure there's a skeleton
		DoFatalAlert("PlayerGotHit: no skeleton on player");


			/* SPECIAL CASE IF WAS RIDING A BUBBLE */

	if ((player->Skeleton->AnimNum == PLAYER_ANIM_BUBBLE) && gSoapBubble)
	{
		PopSoapBubble(gSoapBubble);
	}

			/*****************/
			/* DEFAULT STUFF */
			/*****************/

	else
	{
				/* DO HIT ANIM UNLESS SPECIAL */

		switch(player->Skeleton->AnimNum)
		{
			case	PLAYER_ANIM_GOTHIT:
			case	PLAYER_ANIM_GRABBED:
			case	PLAYER_ANIM_ZAPPED:
			case	PLAYER_ANIM_DROWNED:
			case	PLAYER_ANIM_PICKUPANDHOLDMAGNET:
			case	PLAYER_ANIM_SUCKEDINTOWELL:
			case	PLAYER_ANIM_GRABBED2:
			case	PLAYER_ANIM_BUMPERCAR:
			case	PLAYER_ANIM_GRABBEDBYSTRONGMAN:
					break;

			default:
					MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_GOTHIT, 7);
		}

		float r;
		if (byWhat)
		{
			r = byWhat->Rot.y;
			PlayerLoseHealth(byWhat->Damage, PLAYER_DEATH_TYPE_EXPLODE);
		}
		else
		{
			r = player->Rot.y;
			PlayerLoseHealth(altDamage, PLAYER_DEATH_TYPE_EXPLODE);
		}

		player->Delta.x = -sin(r) * 1000.0f;
		player->Delta.z = -cos(r) * 1000.0f;
		player->Delta.y = 400.0f;

			/* SEE IF RESET POWERUP THAT WASN'T PICKED UP */

		if (gTargetPickup)
		{
			gTargetPickup->MoveCall = MovePowerup;	// restore the Move Call
			gTargetPickup = nil;
		}
	}

	gPlayerInfo.invincibilityTimer = 2.5;

	return true;
}



/***************** PLAYER LOSE HEALTH ************************/
//
// return true if player killed
//

Boolean PlayerLoseHealth(float damage, Byte deathType)
{
Boolean	killed = false;

	if (gPlayerInfo.health < 0.0f)				// see if already dead
		return(true);

	gPlayerInfo.health -= damage;

		/* SEE IF DEAD */

	if (gPlayerInfo.health <= 0.0f)
	{
		gPlayerInfo.health = 0;

		KillPlayer(deathType);
		killed = true;
	}
	else
	{
		Rumble(0.8f, 300);
	}


	return(killed);
}


/****************** KILL PLAYER *************************/

void KillPlayer(Byte deathType)
{
ObjNode	*player = gPlayerInfo.objNode;

		/* VERIFY ANIM IF ALREADY DEAD */

	if (gPlayerIsDead)						// see if already dead
	{
		switch(deathType)
		{
			case	PLAYER_DEATH_TYPE_DROWN:
					if (player->Skeleton->AnimNum != PLAYER_ANIM_DROWNED)
						SetSkeletonAnim(player->Skeleton,PLAYER_ANIM_DROWNED);
					break;

			case	PLAYER_DEATH_TYPE_FALL:
					if (player->Skeleton->AnimNum != PLAYER_ANIM_FALL)
						SetSkeletonAnim(player->Skeleton,PLAYER_ANIM_FALL);
					gPlayerFellIntoBottomlessPit = true;
					break;

		}
		return;
	}


			/* KILL US NOW */

	gPlayerIsDead = true;
	gPlayerInfo.health = 0;					// make sure this is set correctly

	switch(deathType)
	{
		case	PLAYER_DEATH_TYPE_DROWN:
				MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_DROWNED, 5);
				gDeathTimer = gPlayerInfo.invincibilityTimer = 3.0f;
				break;


		case	PLAYER_DEATH_TYPE_EXPLODE:
				ExplodePlayer();
				gDeathTimer = gPlayerInfo.invincibilityTimer = 3.0f;
				break;


		case	PLAYER_DEATH_TYPE_FALL:
		{
				gPlayerInfo.fellThroughTrapDoor = NULL;
				if (player->Skeleton->AnimNum != PLAYER_ANIM_FALL)
					MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_FALL, 5);
				gPlayerInfo.invincibilityTimer = 2.0f;
				gDeathTimer = 2.0;
				gPlayerFellIntoBottomlessPit = true;
				PlayEffect_Parms3D(EFFECT_FALLYAA, &player->Coord, NORMAL_CHANNEL_RATE, 1.5);

				float bestTrapDoorDistance = 1.5f * TERRAIN_POLYGON_SIZE;
				for (ObjNode* trapDoorCandidate = gFirstNodePtr; trapDoorCandidate; trapDoorCandidate = trapDoorCandidate->NextNode)
				{
					if (trapDoorCandidate->TerrainItemPtr && trapDoorCandidate->TerrainItemPtr->type == 84)
					{
						float candidateDistance = CalcQuickDistance(player->Coord.x, player->Coord.z, trapDoorCandidate->Coord.x, trapDoorCandidate->Coord.z);
						if (candidateDistance < bestTrapDoorDistance)
						{
							gPlayerInfo.fellThroughTrapDoor = trapDoorCandidate;
							bestTrapDoorDistance = candidateDistance;
						}
					}
				}
				break;
		}

		case	PLAYER_DEATH_TYPE_SAUCERBOOM:
				BlowUpSaucer(player);
				break;
	}


			/* STOP ANY SUPERNOVA THAT WAS CHARGING */

	if (gPlayerInfo.superNovaStatic)
	{
		gPlayerInfo.superNovaCharge = 0;			// reset charget to 0
		DischargeSuperNova();						// kill it
	}

}


/*********************** EXPLODE PLAYER ***************************/

static void ExplodePlayer(void)
{
ObjNode	*player = gPlayerInfo.objNode;
ObjNode	*leftHand = gPlayerInfo.leftHandObj;
ObjNode	*rightHand = gPlayerInfo.rightHandObj;


		/* HIDE & DISABLE THE REAL PLAYER */

	HidePlayer(player);


			/***************/
			/* MAKE SPARKS */
			/***************/

	MakeSparkExplosion(player->Coord.x, player->Coord.y, player->Coord.z, 400, 1.0, PARTICLE_SObjType_BlueSpark, 0);

	ExplodeGeometry(player, 150.0, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 2, .4);
	ExplodeGeometry(leftHand, 150.0, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 2, .4);
	ExplodeGeometry(rightHand, 150.0, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 2, .4);

	PlayEffect3D(EFFECT_PLAYERCRASH, &player->Coord);

	StartDeathExit(2.5);

}


/********************* HIDE PLAYER ************************/

void HidePlayer(ObjNode *player)
{
ObjNode	*leftHand = gPlayerInfo.leftHandObj;
ObjNode	*rightHand = gPlayerInfo.rightHandObj;

	player->StatusBits |= STATUS_BIT_HIDDEN | STATUS_BIT_NOMOVE;
	player->CType = 0;
	leftHand->StatusBits |= STATUS_BIT_HIDDEN | STATUS_BIT_NOMOVE;
	rightHand->StatusBits |= STATUS_BIT_HIDDEN | STATUS_BIT_NOMOVE;
}


/****************** RESET PLAYER @ BEST CHECKPOINT *********************/

void ResetPlayerAtBestCheckpoint(void)
{
ObjNode	*player = gPlayerInfo.objNode;


	gPlayerInfo.coord.x = player->Coord.x = gBestCheckpointCoord.x;
	gPlayerInfo.coord.z = player->Coord.z = gBestCheckpointCoord.y;
	DoPlayerTerrainUpdate(gPlayerInfo.coord.x, gPlayerInfo.coord.z);		// do this to prime any objecs/platforms there before we calc our new y Coord

	player->OldCoord = player->Coord;
	player->Delta.x = player->Delta.y = player->Delta.z = 0;

	player->CType = CTYPE_PLAYER;										// make sure collision is set
	player->StatusBits &= ~(STATUS_BIT_HIDDEN | STATUS_BIT_NOMOVE);		// make sure not hidden and movable

	gPlayerInfo.health = player->Health = 1.0;
	gPlayerInfo.burnTimer = 0;

	gPlayerIsDead 					= false;
	gPlayerFellIntoBottomlessPit 	= false;
	gExplodePlayerAfterElectrocute 	= false;


	if (gLevelNum != LEVEL_NUM_SAUCER)									// dont do some of this on saucer level
	{
		ObjNode	*leftHand = gPlayerInfo.leftHandObj;
		ObjNode	*rightHand = gPlayerInfo.rightHandObj;


		gPlayerInfo.coord.y = gPlayerInfo.objNode->Coord.y = FindHighestCollisionAtXZ(gPlayerInfo.coord.x, gPlayerInfo.coord.z, CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TERRAIN) - player->BottomOff + 20.0f;

		player->Rot.y = gBestCheckpointAim;										// set the aim

		gPlayerInfo.scale 		= player->Scale.x = player->Scale.y = player->Scale.z = PLAYER_DEFAULT_SCALE;
		gPlayerInfo.scaleRatio 	= 1.0;
		gPlayerInfo.growMode 	= GROWTH_MODE_SHRINK;							// set to shrink to cause things to reset fully on 1st frame
		gPlayerInfo.giantTimer 	= 0;

		SetPlayerStandAnim(player, 100);

		leftHand->StatusBits &= ~(STATUS_BIT_HIDDEN | STATUS_BIT_NOMOVE);
		rightHand->StatusBits &= ~(STATUS_BIT_HIDDEN | STATUS_BIT_NOMOVE);
	}


			/* PLACE CAMERA */

	InitCamera();


	PlayEffect_Parms(EFFECT_NEWLIFE, FULL_CHANNEL_VOLUME * 3, FULL_CHANNEL_VOLUME/3, NORMAL_CHANNEL_RATE / 1.5);
}



#pragma mark -

/******************** ADD EXIT ROCKET *******************************/

Boolean AddExitRocket(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*door, *rocket;
				/*************/
				/* MAKE SHIP */
				/*************/

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_Rocket;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= FindHighestCollisionAtXZ(x,z,CTYPE_MPLATFORM|CTYPE_TERRAIN);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 99;
	gNewObjectDefinition.moveCall 	= MoveRocketShip;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/8.0f);
	gNewObjectDefinition.scale 		= ROCKET_SCALE * gRocketScaleAdjust;
	rocket = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	rocket->TerrainItemPtr = itemPtr;								// keep ptr to item list

	BG3D_SphereMapGeomteryMaterial(gNewObjectDefinition.group,gNewObjectDefinition.type,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);	// set this model to be sphere mapped

	rocket->Kind = ROCKET_KIND_EXIT;


			/* SET COLLISION STUFF */

	rocket->CType 			= CTYPE_MISC;
	rocket->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(rocket,.8,1);



			/*************/
			/* MAKE DOOR */
			/*************/

	gNewObjectDefinition.type 		= GLOBAL_ObjType_RocketDoor;
	gNewObjectDefinition.moveCall 	= nil;
	door = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	rocket->ChainNode = door;
	door->ChainHead = rocket;

	BG3D_SphereMapGeomteryMaterial(gNewObjectDefinition.group,gNewObjectDefinition.type,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);	// set this model to be sphere mapped


				/* OTHER STUFF */

	AttachShadowToObject(rocket, GLOBAL_SObjType_Shadow_Circular, 30.0f * gRocketScaleAdjust,30.0f * gRocketScaleAdjust, false);


	rocket->Mode = ROCKET_MODE_WAITING;

	gExitRocket = rocket;

	return(true);													// item was added
}



/******************** INIT ROCKET SHIP *************************/

static void InitRocketShip(OGLPoint3D *where, float rot)
{
ObjNode	*door, *rocket;

	gAutoRotateCamera 		= true;
	gAutoRotateCameraSpeed 	= -.06f;
	gPlayerHasLanded 		= false;

	gFreezeCameraFromY 		= true;
	gFreezeCameraFromXZ		= true;

				/*************/
				/* MAKE SHIP */
				/*************/

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_Rocket;
	gNewObjectDefinition.coord.x 	= where->x;
	gNewObjectDefinition.coord.z 	= where->z;
	gNewObjectDefinition.coord.y 	= FindHighestCollisionAtXZ(where->x, where->z, CTYPE_MPLATFORM|CTYPE_TERRAIN) + 6000.0f;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 99;
	gNewObjectDefinition.moveCall 	= MoveRocketShip;
	gNewObjectDefinition.rot 		= rot + PI;
	gNewObjectDefinition.scale 		= ROCKET_SCALE;
	rocket = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	BG3D_SphereMapGeomteryMaterial(gNewObjectDefinition.group,gNewObjectDefinition.type,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);	// set this model to be sphere mapped

	rocket->Kind = ROCKET_KIND_ENTER;
	rocket->Mode = ROCKET_MODE_LANDING;

			/* SET COLLISION STUFF */

	rocket->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	rocket->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(rocket,.8,1);


			/*************/
			/* MAKE DOOR */
			/*************/

	gNewObjectDefinition.type 		= GLOBAL_ObjType_RocketDoor;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.slot++;
	door = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	rocket->ChainNode = door;
	door->ChainHead = rocket;

	BG3D_SphereMapGeomteryMaterial(gNewObjectDefinition.group,gNewObjectDefinition.type,
									-1, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_Blue);	// set this model to be sphere mapped


				/* OTHER STUFF */

	MakeRocketFlame(rocket);
	AttachShadowToObject(rocket, GLOBAL_SObjType_Shadow_Circular, 30,30, false);
}


/******************** MAKE ROCKET FLAME *************************/

void MakeRocketFlame(ObjNode *rocket)
{
ObjNode	*newObj, *door = rocket->ChainNode;
int		i;

	if (door == nil)									// if no door, then probably using rocket on Bonus screen
		door = rocket;

				/* MAKE FLAME OBJECT */

	NewObjectDefinitionType def =
	{
		.genre		= CUSTOM_GENRE,
		.coord 		= rocket->Coord,
		.flags		= STATUS_BIT_NOZWRITES | STATUS_BIT_NOTEXTUREWRAP | STATUS_BIT_NOLIGHTING | STATUS_BIT_NOFOG | STATUS_BIT_GLOW,
		.slot 		= PARTICLE_SLOT+1,
		.moveCall 	= nil,
		.rot 		= 0,
		.scale 		= 1,
	};

	newObj = MakeNewObject(&def);

	newObj->CustomDrawFunction = DrawRocketFlame;

	door->ChainNode = newObj;

	newObj->ChainHead = door;				// point back to door


			/**********************/
			/* CREATE ENGINE GLOW */
			/**********************/

	if (rocket->Sparkles[0] == -1)
	{
		i = rocket->Sparkles[0] = GetFreeSparkle(rocket);				// get free sparkle slot
		if (i != -1)
		{
			gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL | SPARKLE_FLAG_TRANSFORMWITHOWNER;
			gSparkles[i].where.x = 0;
			gSparkles[i].where.y = EXHAUST_YOFF;
			gSparkles[i].where.z = 0;

			gSparkles[i].color.r = 1;
			gSparkles[i].color.g = 1;
			gSparkles[i].color.b = .9;
			gSparkles[i].color.a = .5;

			gSparkles[i].scale = 300.0f;
			gSparkles[i].separation = 400.0f * gRocketScaleAdjust;

			gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark;
		}
	}
}


/******************* GENERATE ROCKET FLAME MESH ************************/

#define ROCKETFLAME_SEGMENTS	5		// amount of trapezoids making up the flame
#define ROCKETFLAME_HDIV		8		// how much to tesselate each trapezoid horizontally (more => mitigate texture distortion)

static MOVertexArrayData* GenerateRocketFlameMesh(void)
{
	static const float flameWidth = 100;
	static const float flameHeight = 800;
	static const float segmentThickness[ROCKETFLAME_SEGMENTS]	= { .51, .70, .85,   1, 1 };
	static const float segmentBottomYFrac[ROCKETFLAME_SEGMENTS]	= { .03, .07, .15, .30, 1 };

	static OGLPoint3D			pts[ROCKETFLAME_SEGMENTS * ROCKETFLAME_HDIV * 4];
	static OGLTextureCoord		uvs[ROCKETFLAME_SEGMENTS * ROCKETFLAME_HDIV * 4];
	static MOTriangleIndecies	tris[ROCKETFLAME_SEGMENTS * ROCKETFLAME_HDIV * 2];
	static MOVertexArrayData	mesh;

	int v = 0;		// vertex index
	int t = 0;		// triangle index
	for (int i = 0; i < ROCKETFLAME_SEGMENTS; i++)
	{
		float x1 = flameWidth * segmentThickness[i];											// top thickness
		float x2 = flameWidth * segmentThickness[MinInt(i + 1, ROCKETFLAME_SEGMENTS - 1)];		// bottom thickness

		float yfrac1 = i<=0 ? 0 : segmentBottomYFrac[i - 1];
		float yfrac2 = segmentBottomYFrac[i];

		float y1 = -flameHeight * yfrac1;
		float y2 = -flameHeight * yfrac2;

		for (int j = 0; j < ROCKETFLAME_HDIV; j++) 		// tesselate the trapezoid horizontally to mitigate texture distortion
		{
			float xfrac1 = (float)(j + 0) / ROCKETFLAME_HDIV;
			float xfrac2 = (float)(j + 1) / ROCKETFLAME_HDIV;
			
			pts[v+0] = (OGLPoint3D){ LerpFloat(-x1, x1, xfrac1), y1, 0 };
			pts[v+1] = (OGLPoint3D){ LerpFloat(-x1, x1, xfrac2), y1, 0 };
			pts[v+2] = (OGLPoint3D){ LerpFloat(-x2, x2, xfrac2), y2, 0 };
			pts[v+3] = (OGLPoint3D){ LerpFloat(-x2, x2, xfrac1), y2, 0 };

			uvs[v+0] = (OGLTextureCoord){ xfrac1, yfrac1 };
			uvs[v+1] = (OGLTextureCoord){ xfrac2, yfrac1 };
			uvs[v+2] = (OGLTextureCoord){ xfrac2, yfrac2 };
			uvs[v+3] = (OGLTextureCoord){ xfrac1, yfrac2 };

			tris[t+0] = (MOTriangleIndecies){ {v+0, v+1, v+3} };
			tris[t+1] = (MOTriangleIndecies){ {v+1, v+2, v+3} };

			v += 4;
			t += 2;
		}
	}

	mesh = (MOVertexArrayData)
	{
		.triangles		= tris,
		.uvs[0]			= uvs,
		.points			= pts,
		.numPoints		= ROCKETFLAME_SEGMENTS * ROCKETFLAME_HDIV * 4,
		.numTriangles	= ROCKETFLAME_SEGMENTS * ROCKETFLAME_HDIV * 2,
		.numMaterials	= 0,		// set in draw call
	};

	return &mesh;
}


/******************* DRAW ROCKET FLAME ************************/

void DrawRocketFlame(ObjNode *theNode)
{
float		x,y,z,r,s;
OGLMatrix4x4	m1, m2;
ObjNode			*rocket;

	rocket = theNode->ChainHead->ChainHead;					// assume landing mode
	if (rocket == nil)										// nope, must be bonus screen
		rocket = theNode->ChainHead;


		/* CALC COORDS */

	x = rocket->Coord.x;
	y = rocket->Coord.y + EXHAUST_YOFF * gRocketScaleAdjust;
	z = rocket->Coord.z;

	r = theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, x, z,
												gGameViewInfoPtr->cameraPlacement.cameraLocation.x,
												gGameViewInfoPtr->cameraPlacement.cameraLocation.z);

	s = gRocketScaleAdjust;

	OGLMatrix4x4_SetRotate_Y(&m1, r);
	OGLMatrix4x4_SetScale(&m2, s, s, s);
	OGLMatrix4x4_Multiply(&m1, &m2, &m1);
	OGLMatrix4x4_SetTranslate(&m2, x, y, z);
	OGLMatrix4x4_Multiply(&m1, &m2, &m1);

	MOVertexArrayData* mesh = GenerateRocketFlameMesh();
	OGLPoint3D_TransformArray(mesh->points, &m1, mesh->points, mesh->numPoints);

		/* SET TEXTURE FOR CURRENT FRAME */

	mesh->numMaterials = 1;
	int frame = (int)(theNode->Special[0]);
	mesh->materials[0] = gSpriteGroupList[SPRITE_GROUP_PARTICLES][PARTICLE_SObjType_RocketFlame0 + frame].materialObject;

		/* DRAW IT */

	MO_DrawGeometry_VertexArray(mesh);

		/* ANIMATE */

	theNode->SpecialF[0] -= gFramesPerSecondFrac;			// animate
	if (theNode->SpecialF[0] <= 0.0f)
	{
		theNode->SpecialF[0] = .025f;
		theNode->Special[0]++;
		if (theNode->Special[0] > 7)
			theNode->Special[0] = 0;
	}
}


/******************** MOVE ROCKET SHIP **********************/

static void MoveRocketShip(ObjNode *rocket)
{

	switch(rocket->Mode)
	{
		case	ROCKET_MODE_LANDING:
				MoveRocketShip_Landing(rocket);
				MakeRocketExhaust(rocket);
				break;

		case	ROCKET_MODE_OPENDOOR:
				MoveRocketShip_OpenDoor(rocket);
				break;

		case	ROCKET_MODE_DEPLANE:
				MoveRocketShip_Deplane(rocket);
				break;

		case	ROCKET_MODE_CLOSEDOOR:
				MoveRocketShip_CloseDoor(rocket);
				break;

		case	ROCKET_MODE_LEAVE:
				MoveRocketShip_Leave(rocket);
				MakeRocketExhaust(rocket);
				break;

		case	ROCKET_MODE_WAITING:
				MoveRocketShip_Waiting(rocket);
				break;

		case	ROCKET_MODE_WAITING2:
				MoveRocketShip_Waiting2(rocket);
				break;

		case	ROCKET_MODE_PREENTER:
				MoveRocketShip_PreEnter(rocket);
				break;

		case	ROCKET_MODE_ENTER:
				MoveRocketShip_Enter(rocket);
				break;
	}


	/* SEE IF DISPLAY HELP */


	switch(gLevelNum)
	{
		case	LEVEL_NUM_BRAINBOSS:				// no help on these levels
		case	LEVEL_NUM_JUNGLEBOSS:
		case	LEVEL_NUM_SAUCER:
				break;

		default:
				if (rocket->Kind == ROCKET_KIND_EXIT)
				{
					if (!gHelpMessageDisabled[HELP_MESSAGE_ENTERSHIP])
					{
						if (CalcQuickDistance(rocket->Coord.x, rocket->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < 900.0f)
						{
							DisplayHelpMessage(HELP_MESSAGE_ENTERSHIP, .5f, false);
						}
					}
				}
	}
}


/*********************** MAKE ROCKET EXHAUST *********************/

void MakeRocketExhaust(ObjNode *rocket)
{
int	i;
float	x,y,z;

	x = rocket->Coord.x;
	y = rocket->Coord.y + EXHAUST_YOFF * gRocketScaleAdjust;
	z = rocket->Coord.z;


			/******************/
			/* CREATE EXHAUST */
			/******************/

				/* SMOKE */

	rocket->ParticleTimer -= gFramesPerSecondFrac;									// see if add smoke
	if (rocket->ParticleTimer <= 0.0f)
	{
		int		particleGroup,magicNum;
		NewParticleGroupDefType	groupDef;
		NewParticleDefType	newParticleDef;
		OGLVector3D			d;
		OGLPoint3D			p;

		rocket->ParticleTimer += .05f;										// reset timer

		particleGroup 	= rocket->ParticleGroup;
		magicNum 		= rocket->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			rocket->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_BOUNCE|PARTICLE_FLAGS_DISPERSEIFBOUNCE;
			groupDef.gravity				= 0;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 60 * gRocketScaleAdjust;
			groupDef.decayRate				= -2.0f;
			groupDef.fadeRate				= .7;
			groupDef.particleTextureNum		= PARTICLE_SObjType_GreySmoke;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			rocket->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			for (i = 0; i < 4; i++)
			{
				p.x = x + RandomFloat2() * (80.0f * gRocketScaleAdjust);
				p.y = y;
				p.z = z + RandomFloat2() * (80.0f * gRocketScaleAdjust);

				d.x = RandomFloat2() * (150.0f * gRocketScaleAdjust);
				d.y = rocket->Delta.y - (800.0f + RandomFloat() * 100.0f * gRocketScaleAdjust);
				d.z =  RandomFloat2() * (150.0f * gRocketScaleAdjust);

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat() * PI2;
				newParticleDef.rotDZ		= RandomFloat2();

				newParticleDef.alpha		= .8;

				if (AddParticleToGroup(&newParticleDef))
				{
					rocket->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}


/****************** MOVE ROCKET SHIP: LANDING *******************/

static void MoveRocketShip_Landing(ObjNode *rocket)
{
float	y,r;
int		i;
ObjNode	*door = rocket->ChainNode;


	GetObjectInfo(rocket);

	y = FindHighestCollisionAtXZ(gCoord.x, gCoord.z, CTYPE_MPLATFORM|CTYPE_TERRAIN);

	gDelta.y = -((gCoord.y - y) * .9f + 100.0f);
	if (gDelta.y < -3000.0f)
		gDelta.y = -3000.0f;

	gCoord.y += gDelta.y * gFramesPerSecondFrac;

	if (gSkipFluff)
		gCoord.y -= gFramesPerSecondFrac * 50000;

			/* MAKE SURE SOUND IS GOING */

	if (rocket->EffectChannel == -1)
		rocket->EffectChannel = PlayEffect3D(EFFECT_ROCKET, &gCoord);
	else
		Update3DSoundChannel(EFFECT_ROCKET, &rocket->EffectChannel, &gCoord);


			/* SEE IF TOUCHDOWN */

	if (gCoord.y <= y)
	{
		gCoord.y = y;
		rocket->Mode = ROCKET_MODE_OPENDOOR;

		i = rocket->Sparkles[0];			// turn off sparkle
		if (i != -1)
		{
			DeleteSparkle(i);
			rocket->Sparkles[0] = -1;
		}

		if (door->ChainNode)				// stop fire
		{
			DeleteObject(door->ChainNode);
			door->ChainNode = nil;
		}

		StopAChannelIfEffectNum(&rocket->EffectChannel, EFFECT_ROCKET);	// stop sound
		PlayEffect3D(EFFECT_ROCKETLANDED, &gCoord);							// play landed
	}

	UpdateObject(rocket);


			/* UPDATE DOOR */

	AlignRocketDoor(rocket);


			/* UPDATE PLAYER INFO */

	r = rocket->Rot.y;
	gPlayerInfo.objNode->Coord.x = gPlayerInfo.coord.x = gCoord.x + sin(r) * PLAYER_OFF_Z;
	gPlayerInfo.objNode->Coord.y = gPlayerInfo.coord.y = gCoord.y + PLAYER_OFF_Y - gPlayerInfo.objNode->BottomOff;
	gPlayerInfo.objNode->Coord.z = gPlayerInfo.coord.z = gCoord.z + cos(r) * PLAYER_OFF_Z;
	UpdateObjectTransforms(gPlayerInfo.objNode);
	UpdateRobotHands(gPlayerInfo.objNode);


}


/****************** MOVE ROCKET SHIP: LEAVE *******************/

static void MoveRocketShip_Leave(ObjNode *rocket)
{
float	y;


	GetObjectInfo(rocket);

	y = FindHighestCollisionAtXZ(gCoord.x, gCoord.z, CTYPE_MPLATFORM|CTYPE_TERRAIN);

	if (gCoord.y < y)
		gCoord.y = y;

	gDelta.y = (gCoord.y - y) * .9f + 100.0f;
	if (gDelta.y < -2000.0f)
		gDelta.y = -2000.0f;

	if (gSkipFluff)
		gDelta.y = 7500;


	gCoord.y += gDelta.y * gFramesPerSecondFrac;

			/* SEE IF GONE */

	if (gCoord.y > (GetTerrainY(gCoord.x, gCoord.z) + 9000.0f))
	{
		if (rocket->Kind == ROCKET_KIND_EXIT)			// see if that's the end of the level
		{
			gLevelCompleted = true;
			gLevelCompletedCoolDownTimer = 0;
		}
		else
		{
			DeleteObject(rocket);
		}

		return;
	}

	UpdateObject(rocket);

			/* MAKE SURE SOUND IS GOING */

	if (rocket->EffectChannel == -1)
		rocket->EffectChannel = PlayEffect3D(EFFECT_ROCKET, &rocket->Coord);
	else
		Update3DSoundChannel(EFFECT_ROCKET, &rocket->EffectChannel, &rocket->Coord);


			/* UPDATE DOOR */

	AlignRocketDoor(rocket);


	if (!gPlayerHasLanded)
	{
		float	r = rocket->Rot.y;

					/* UPDATE PLAYER INFO */

		gPlayerInfo.objNode->Coord.x = gPlayerInfo.coord.x = gCoord.x + sin(r) * PLAYER_OFF_Z;
		gPlayerInfo.objNode->Coord.y = gPlayerInfo.coord.y = gCoord.y + PLAYER_OFF_Y - gPlayerInfo.objNode->BottomOff;
		gPlayerInfo.objNode->Coord.z = gPlayerInfo.coord.z = gCoord.z + cos(r) * PLAYER_OFF_Z;
		UpdateObjectTransforms(gPlayerInfo.objNode);
		UpdateRobotHands(gPlayerInfo.objNode);
	}
}


/****************** MOVE ROCKET SHIP: OPEN DOOR *******************/

static void MoveRocketShip_OpenDoor(ObjNode *rocket)
{
ObjNode *door = rocket->ChainNode;


			/* ROTATE DOOR */

	door->Rot.x += gFramesPerSecondFrac * 1.9f;

	if (gSkipFluff)
		door->Rot.x += gFramesPerSecondFrac * 100;

		/* SEE IF DONE OPENING */

	if (door->Rot.x > 2.5f)
	{
		door->Rot.x = 2.5f;

		if (rocket->Kind == ROCKET_KIND_EXIT)
		{
			rocket->Mode = ROCKET_MODE_WAITING2;
		}
		else
		{
			rocket->Mode = ROCKET_MODE_DEPLANE;
			gDeplaneTimer = 2.5;

			if (gSkipFluff)
				gDeplaneTimer *= 0.1f;

			MorphToSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_WALK, 8);	// start robot walking
			gPlayerInfo.objNode->Skeleton->AnimSpeed = 2.0f;
		}
	}

	UpdateObjectTransforms(door);

}

/****************** MOVE ROCKET SHIP: CLOSE DOOR *******************/

static void MoveRocketShip_CloseDoor(ObjNode *rocket)
{
ObjNode	*door = rocket->ChainNode;

			/* ROTATE DOOR */

	door->Rot.x -= gFramesPerSecondFrac * 1.9f;
	if (door->Rot.x <= 0.0f)
	{
		door->Rot.x = 0.0f;

		if ((rocket->Kind == ROCKET_KIND_EXIT) && gPlayerHasLanded)
		{
			rocket->Mode = ROCKET_MODE_WAITING;
		}
		else
		{
			if (gLevelNum == LEVEL_NUM_JUNGLEBOSS)	// the rocket becomes the exit rocket on the Jungle Boss level
			{
				if (gTractorBeamObj)				// if tractor beam still active, then just wait
				{
					rocket->Kind = ROCKET_KIND_EXIT;
					rocket->Mode = ROCKET_MODE_WAITING;
				}
				else
					goto leave;
			}
			else
			if ((gLevelNum == LEVEL_NUM_BRAINBOSS) && gPlayerHasLanded)	//  the rocket becomes the exit rocket on the Brain Boss level
			{
				rocket->Kind = ROCKET_KIND_EXIT;
				rocket->Mode = ROCKET_MODE_WAITING;
			}
			else
			{
leave:
				if (rocket->Kind == ROCKET_KIND_EXIT)
					HidePlayer(gPlayerInfo.objNode);												// hide otto so gun doesnt poke thru hull
				rocket->Mode = ROCKET_MODE_LEAVE;
				MakeRocketFlame(rocket);
			}
		}
	}

	UpdateObjectTransforms(door);

}


/****************** MOVE ROCKET SHIP: DEPLANE *******************/

static void MoveRocketShip_Deplane(ObjNode *rocket)
{
float	fps = gFramesPerSecondFrac;
float	 r;
ObjNode	*player = gPlayerInfo.objNode;

	r = rocket->Rot.y;


	GetObjectInfo(player);

	gDelta.x = sin(r) * 250.0f;
	gDelta.z = cos(r) * 250.0f;

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;


			/* SEE IF GO DOWN RAMP */

	if (CalcDistance(gCoord.x, gCoord.z, rocket->Coord.x, rocket->Coord.z) > 170.0f)
	{
		gCoord.y -= 380.0f * fps;

					/* SEE IF ON GROUND */

		if ((gCoord.y + player->BottomOff) <= FindHighestCollisionAtXZ(gCoord.x, gCoord.z, CTYPE_MPLATFORM|CTYPE_TERRAIN))
		{
			gCoord.y = FindHighestCollisionAtXZ(gCoord.x, gCoord.z, CTYPE_MPLATFORM|CTYPE_TERRAIN) - player->BottomOff;
		}
	}

	gDeplaneTimer -= fps;
	if (gDeplaneTimer <= 0.0f)
	{
		gPlayerInfo.objNode->StatusBits &= ~STATUS_BIT_NOMOVE;
		gFreezeCameraFromXZ = false;
		gFreezeCameraFromY = false;
		gAutoRotateCamera = false;
		rocket->Mode = ROCKET_MODE_CLOSEDOOR;
		gPlayerHasLanded = true;
		PlayEffect3D(EFFECT_HATCH, &rocket->Coord);


		gBestCheckpointCoord.x = gCoord.x;					// set the start checkpoint to here
		gBestCheckpointCoord.y = gCoord.z;
	}

	UpdateObject(player);
	gPlayerInfo.coord = gCoord;				// update player coord
	UpdatePlayerSparkles(player);
	UpdateRobotHands(player);
}


/****************** MOVE ROCKET SHIP: PRE-ENTER (MOVE OTTO TO RAMP ENTRY POINT) *******************/

static void MoveRocketShip_PreEnter(ObjNode *rocket)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*player = gPlayerInfo.objNode;

	DisableHelpType(HELP_MESSAGE_ENTERSHIP);				// player is going up, so we don't need this help anymore

	gPlayerInfo.holdingGun = false;							// hide gun so it doesn't clip thru rocket once we're in
	gPlayerInfo.invincibilityTimer = 1;						// don't want to get hit by crap while walking

	GetObjectInfo(player);

	OGLPoint2D rampEntryPoint = kRocketShipRampEntryPoint;
	OGLMatrix3x3 m;
	OGLMatrix3x3_SetRotate(&m, -rocket->Rot.y);
	OGLPoint2D_Transform(&rampEntryPoint, &m, &rampEntryPoint);
	rampEntryPoint.x += rocket->Coord.x;
	rampEntryPoint.y += rocket->Coord.z;

	float oldDist = CalcDistance(gCoord.x, gCoord.z, rampEntryPoint.x, rampEntryPoint.y);

	if (oldDist < PLAYER_OFF_Z)								// close enough already, just skip to next state
		goto forceEnter;

	OGLVector2D direction =	{ rampEntryPoint.x - player->Coord.x, rampEntryPoint.y - player->Coord.z };
	OGLVector2D_Normalize(&direction, &direction);

	gDelta.x = direction.x * 250;
	gDelta.z = direction.y * 250;
	gDelta.y = -100;

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y = GetTerrainY2(gCoord.x, gCoord.z) - gPlayerInfo.objNode->BottomOff;

	float dist = CalcDistance(gCoord.x, gCoord.z, rampEntryPoint.x, rampEntryPoint.y);

	if (dist > 2000)
	{
#if _DEBUG
		DoAlert("Enormous pre-enter distance! Fast-forwarding...");
#endif
		goto forceEnter;
	}

			/* SEE IF WE'RE IN FRONT OF THE RAMP */

	if (dist < PLAYER_OFF_Z || dist > oldDist)
	{
forceEnter:
			/* PIN TO RAMP ENTRY POINT */

		gCoord.x = rampEntryPoint.x;
		gCoord.z = rampEntryPoint.y;
		gCoord.y = GetTerrainY2(gCoord.x, gCoord.z) - gPlayerInfo.objNode->BottomOff;

		rocket->Mode = ROCKET_MODE_ENTER;
		gPlayerHasLanded = false;
	}

	UpdatePlayer_Robot(player);
}


/****************** MOVE ROCKET SHIP: ENTER (WALK OTTO UP RAMP) *******************/

static void MoveRocketShip_Enter(ObjNode *rocket)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*player = gPlayerInfo.objNode;

	DisableHelpType(HELP_MESSAGE_ENTERSHIP);				// player is going up, so we don't need this help anymore

	gPlayerInfo.holdingGun = false;							// hide gun so it doesn't clip thru rocket once we're in
	gPlayerInfo.invincibilityTimer = 1;						// don't want to get hit by crap while walking

	GetObjectInfo(player);

	float oldDist = CalcDistance(gCoord.x, gCoord.z, rocket->Coord.x, rocket->Coord.z);

	gDelta.x = sin(rocket->Rot.y) * -250.0f;
	gDelta.z = cos(rocket->Rot.y) * -250.0f;

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;

			/* GO UP RAMP */

	gCoord.y += 400.0f * fps;

	gCoord.y = MinFloat(gCoord.y,
						rocket->Coord.y + PLAYER_OFF_Y - player->BottomOff);	// don't go higher than where we want it

			/* SEE IF DONE */

	float dist = CalcDistance(gCoord.x, gCoord.z, rocket->Coord.x, rocket->Coord.z);

	if ((dist <= PLAYER_OFF_Z) || (dist > oldDist))
	{
		rocket->Mode = ROCKET_MODE_CLOSEDOOR;
		gPlayerHasLanded = false;
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_STAND, 6);
		PlayEffect3D(EFFECT_HATCH, &rocket->Coord);
	}

	UpdatePlayer_Robot(player);
}


/****************** MOVE ROCKET SHIP: WAITING *******************/
//
// Wait to open door when player gets close enough
//

static void MoveRocketShip_Waiting(ObjNode *rocket)
{
ObjNode	*door = rocket->ChainNode;

	GetObjectInfo(rocket);

			/* SEE IF OUT OF RANGE */

	switch(gLevelNum)									// keep the rocket here on the Jungle Boss level, et.al.
	{
		case	LEVEL_NUM_JUNGLEBOSS:
		case	LEVEL_NUM_SAUCER:
		case	LEVEL_NUM_BRAINBOSS:
				break;

		default:
				if (TrackTerrainItem(rocket))
				{
					DeleteObject(rocket);
					gExitRocket = nil;
					return;
				}
	}

			/* UPDATE DOOR */

	AlignRocketDoor(rocket);


		/* SEE IF SHOULD OPEN DOOR */

	if (gLevelNum == LEVEL_NUM_JUNGLEBOSS)			// on this level, the door can't open until the tractor beam is gone
	{
		if (gTractorBeamObj)
			goto update;
	}

	if (CalcQuickDistance(rocket->Coord.x, rocket->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < 900.0f)	// see if close enough
	{
		if (gLevelNum == LEVEL_NUM_BRAINBOSS)						// don't do anything until boss is dead
		{
			if (!gBrainBossDead)
				goto update;
		}

		if (gPlayerInfo.fuel < 1.0f)								// see if have enough fuel to leave
		{
			if (gLevelNum == LEVEL_NUM_JUNGLEBOSS)					// don't need fuel on Jungle Boss
				goto open_door;

			if (gLevelNum == LEVEL_NUM_SAUCER)						// or Saucer
				goto open_door;

			DisplayHelpMessage(HELP_MESSAGE_NOTENOUGHFUELTOLEAVE, 1.0, true);
		}
		else														// we've got the fuel, so open the door
		{
open_door:
			rocket->Mode = ROCKET_MODE_OPENDOOR;
			PlayEffect3D(EFFECT_HATCH, &door->Coord);
		}

	}

		/* UPDATE */

update:
	UpdateObject(rocket);
}


/****************** MOVE ROCKET SHIP: WAITING 2 *******************/
//
// Door is open, so wait to either pickup player or to close door
//

static void MoveRocketShip_Waiting2(ObjNode *rocket)
{
ObjNode	*player = gPlayerInfo.objNode;
ObjNode	*door = rocket->ChainNode;

	GetObjectInfo(rocket);


			/* SEE IF OUT OF RANGE */

	switch(gLevelNum)									// keep the rocket here on the Jungle Boss level, et.al.
	{
		case	LEVEL_NUM_JUNGLEBOSS:
		case	LEVEL_NUM_SAUCER:
		case	LEVEL_NUM_BRAINBOSS:
				break;

		default:
				if (TrackTerrainItem(rocket))
				{
					DeleteObject(rocket);
					gExitRocket = nil;
					return;
				}
	}



			/* UPDATE DOOR */

	AlignRocketDoor(rocket);


		/* SEE IF SHOULD CLOSE DOOR */

	if (CalcQuickDistance(rocket->Coord.x, rocket->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) > 1200.0f)
	{
		if (gLevelNum != LEVEL_NUM_SAUCER)					// keep door open on this level
		{
			rocket->Mode = ROCKET_MODE_CLOSEDOOR;
			PlayEffect3D(EFFECT_HATCH, &door->Coord);
		}
	}


			/*******************************/
			/* SEE IF PLAYER CAN ENTER NOW */
			/*******************************/

	else
	if (player->StatusBits & STATUS_BIT_ONGROUND)
	{
		if (gLevelNum == LEVEL_NUM_BLOB)										// do special check for blob world
		{
			CheckIfPlayerSuckedIntoBlobWell(player, rocket);
		}
		else
		{
			float r = rocket->Rot.y;
			OGLMatrix3x3	m;

					/* CALC ZONE @ BASE OF RAMP */

			OGLMatrix3x3_SetRotate(&m, -r);											// rotate the hot zone
			m.value[N02] = rocket->Coord.x;											// translate the hot zone
			m.value[N12] = rocket->Coord.z;
			OGLPoint2D_TransformArray(kRocketShipHotZoneTemplate, &m, gRocketShipHotZone, 4);

			if (IsPointInPoly2D(gPlayerInfo.coord.x, gPlayerInfo.coord.z, 4, gRocketShipHotZone))	// see if play in hot zone
			{
				player->CType = 0;

				if (player->Skeleton->AnimNum != PLAYER_ANIM_WALK)
					MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_WALK, 9);		// start robot walking
				player->Skeleton->AnimSpeed = 2.0f;
				player->Rot.y = rocket->Rot.y;;										// aim directly at rocket
				player->StatusBits |= STATUS_BIT_NOMOVE;							// rocket now controls player

				rocket->Mode = ROCKET_MODE_PREENTER;
				gPlayerHasLanded = false;

				gFreezeCameraFromXZ = true;											// camera is frozen
				gFreezeCameraFromY = true;
			}
		}
	}

	UpdateObject(rocket);

}


/****************** ALIGN ROCKET DOOR *************************/

static void AlignRocketDoor(ObjNode *rocket)
{
ObjNode	*door = rocket->ChainNode;

float	r = rocket->Rot.y;


	door->Coord.x = gCoord.x + sin(r) * (DOOR_OFF_Z * ROCKET_SCALE * gRocketScaleAdjust);
	door->Coord.y = gCoord.y + (DOOR_OFF_Y * ROCKET_SCALE * gRocketScaleAdjust);
	door->Coord.z = gCoord.z + cos(r) * (DOOR_OFF_Z * ROCKET_SCALE * gRocketScaleAdjust);
	UpdateObjectTransforms(door);
}

/************************** CHECK IF PLAYER SUCKED INTO BLOB WELL ***************************/

static void CheckIfPlayerSuckedIntoBlobWell(ObjNode *player, ObjNode *rocket)
{
OGLMatrix3x3	m;
static const OGLPoint2D inArea[4] =
{
	{-200, ROCKET_SCALE * 360.0f},
	{200, ROCKET_SCALE * 360.0f},
	{350, ROCKET_SCALE * 750.0f},
	{-350, ROCKET_SCALE * 750.0f},
};
OGLPoint2D	area[4];
DeformationType		defData;


			/* CALC ZONE @ BASE OF RAMP */

	OGLMatrix3x3_SetRotate(&m, -rocket->Rot.y);								// rotate the hot zone
	OGLPoint2D_TransformArray(inArea, &m, area, 4);

			/* SEE IF PLAYER IN THAT ZONE */

	if (IsPointInPoly2D(gPlayerInfo.coord.x - rocket->Coord.x,
						gPlayerInfo.coord.z - rocket->Coord.z, 4, area))	// see if play in hot zone
	{
		player->CType = 0;

		if (player->Skeleton->AnimNum != PLAYER_ANIM_SUCKEDINTOWELL)
		{
			MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_SUCKEDINTOWELL, 9);		// start robot walking

			gPlayerHasLanded = false;

			player->Delta.x = player->Delta.z = 0;					// stop player

			gLevelCompleted = true;									// level is completed...
			gLevelCompletedCoolDownTimer = 4.0;						// .. but do a cooldown to give time to see player sink


				/* MAKE DEFORMATION WELL */

			defData.type 				= DEFORMATION_TYPE_WELL;
			defData.amplitude 			= 0;
			defData.radius 				= 90;
			defData.speed 				= 300;
			defData.origin.x			= gPlayerInfo.coord.x;
			defData.origin.y			= gPlayerInfo.coord.z;
			defData.oneOverWaveLength 	= 1.0f;
			defData.radialWidth			= 0.0f;
			defData.decayRate			= 0.0f;
			NewSuperTileDeformation(&defData);
		}
	}
}



#pragma mark -


/*************** PLAYER ENTERS WATER **************************/
//
// called from player's collision handler if just now entered a water patch
//

void PlayerEntersWater(ObjNode *theNode, int patchNum)
{
float	topY;
ObjNode	*novaObj;

	if (patchNum == -1)								// if no patch, then use terrain y
		topY = GetTerrainY(gCoord.x, gCoord.z);
	else
		topY = gWaterBBox[patchNum].max.y;

			/* SPLASH */

	if (gDelta.y < -700.0f)
		MakeSplash(gCoord.x, topY, gCoord.z);


			/* SPARKS */

	MakeSparkExplosion(gCoord.x, topY, gCoord.z, 200.0f, 1.0, PARTICLE_SObjType_WhiteSpark, 0);

	if (theNode->Skeleton->AnimNum != PLAYER_ANIM_ZAPPED)
	{
		SetSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_ZAPPED);
		theNode->ZappedTimer = 1.7f;
	}

	PlayEffect3D(EFFECT_ZAP, &gCoord);


			/* DISCHARGE A SUPERNOVA */

	if (gDischargeTimer <= 0.0f)
	{
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


/********************** CRUSH PLAYER *************************/

void CrushPlayer(void)
{
ObjNode	*player = gPlayerInfo.objNode;
float	y;

	if (player->Skeleton->AnimNum != PLAYER_ANIM_ACCORDIAN)
	{
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_ACCORDIAN, 5);
		PlayerLoseHealth(.3, PLAYER_DEATH_TYPE_EXPLODE);
		player->AccordianTimer = 2.0;
		PlayEffect3D(EFFECT_PLAYERCRUSH, &player->Coord);
	}

	gPlayerInfo.invincibilityTimer = 2.0;			// this gets reset @ end of flatten anim, but set here so others know what's going on


			/* POSITION SO FLAT ON GROUND */

	y = FindHighestCollisionAtXZ(player->Coord.x, player->Coord.z, CTYPE_TERRAIN | CTYPE_MISC | CTYPE_MPLATFORM);

	player->Coord.y = y + player->BBox.min.y + 2.0f;
	player->Delta.y = 50.0f;


			/* SEE IF WAS PICKING UP SOMETHING */

	if (gTargetPickup)
	{
		gTargetPickup->MoveCall = MovePowerup;	// restore the Move Call
		gTargetPickup = nil;
	}

}

