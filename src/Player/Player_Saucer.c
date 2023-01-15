/*******************************/
/*   	PLAYER_SAUCER.C		   */
/* (c)2001 Pangea Software     */
/* By Brian Greenstone         */
/*******************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MovePlayer_Saucer(ObjNode *theNode);
static void CheckSaucerActionControls(ObjNode *theNode);
static void UpdatePlayer_Saucer(ObjNode *theNode);
static void ActivatePlayerSaucerBeam(ObjNode *saucer);
static void UpdatePlayerSaucerBeam(ObjNode *saucer);
static void ChargeBeam(ObjNode *saucer);
static void DischargeBeam(ObjNode *saucer);
static void MovePlayer_Saucer_ToDropZone(ObjNode *theNode);
static void MovePlayer_Saucer_BeamDownHumans(ObjNode *theNode);
static Boolean CalcRocketDropZone(OGLPoint2D *where);
static void DoPlayerSaucerCollision(ObjNode *theNode);
static void MovePlayer_Saucer_BeamPlayer(ObjNode *theNode);
static void MakeOttoFromSaucer(ObjNode *saucer);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	PLAYER_SAUCER_HOVER_HEIGHT	450.0f

#define	PLAYER_SAUCER_SCALE			.6f

#define	PLAYER_SAUCER_MAX_SPEED		900.0f

#define	CONTROL_SENSITIVITY_SAUCER	900.0f

#define	SHADOW_RADIUS				200.0f

#define	MAX_HUMANS_IN_SAUCER		10

#define	OTTO_SAUCER_SCALE			(PLAYER_DEFAULT_SCALE * .5f)


/*********************/
/*    VARIABLES      */
/*********************/

static	Boolean	gBeamIsCharging,gBeamIsDischarging;
float	gBeamCharge;
static	short	gBeamChargeEffectChannel;
short	gBeamModeSelected,gBeamMode;

int		gNumHumansInTransit;

int		gNumHumansInSaucer;
Byte	gHumansInSaucerList[MAX_HUMANS_IN_SAUCER];

static	float	gBeamDownDelay;

ObjNode	*gPlayerSaucer;




/*************************** INIT PLAYER: SAUCER ****************************/
//
// INPUT:
//			where = floor coord where to init the player.
//			rotY = rotation to assign to player if oldObj is nil.
//

void InitPlayer_Saucer(OGLPoint3D *where)
{
ObjNode	*newObj;
float	y;

	gNumHumansInSaucer = 0;									// no humans in the saucer yet


	gBeamIsCharging 		= gBeamIsDischarging = false;
	gBeamCharge 			= 0;
	gBeamChargeEffectChannel = -1;
	gNumHumansInTransit 	= 0;
	gBeamMode = gBeamModeSelected = BEAM_MODE_DESTRUCTO;

		/* FIND THE Y COORD TO START */

	y = GetTerrainY(where->x,where->z) + PLAYER_SAUCER_HOVER_HEIGHT;


			/*****************/
			/* CREATE SAUCER */
			/*****************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SAUCER_ObjType_Saucer;
	gNewObjectDefinition.coord.x 	= where->x;
	gNewObjectDefinition.coord.z 	= where->z;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= PLAYER_SLOT;
	gNewObjectDefinition.moveCall 	= MovePlayer_Saucer;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= PLAYER_SAUCER_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


	gPlayerSaucer = gPlayerInfo.objNode 	= newObj;


				/* SET COLLISION INFO */

	newObj->CType = CTYPE_PLAYER;
	newObj->CBits = CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj, 1,1);


		/*******************/
		/* SET OTHER STUFF */
		/*******************/

	gTargetMaxSpeed = PLAYER_SAUCER_MAX_SPEED;
	gCurrentMaxSpeed = PLAYER_SAUCER_MAX_SPEED;

	AttachShadowToObject(newObj, 0, 15, 15, true);


	gTimeSinceLastThrust = -8;			// start negative so camera won't swing around right off the bat
	gTimeSinceLastShoot = 10;

	gPlayerHasLanded = true;


}


/*************** IMPACT PLAYER SAUCER ***********************/

void ImpactPlayerSaucer(float x, float z, float damage, ObjNode *saucer, short particleLimit)
{
float	e;

	if (saucer->StatusBits & STATUS_BIT_HIDDEN)		// make sure it's still there
		return;

	e = 300.0f * damage;
	if (e < 50.0f)
		e = 50.0f;

	MakeSparkExplosion(x, saucer->Coord.y, z, e, 1.0, PARTICLE_SObjType_WhiteSpark, particleLimit);
	PlayerLoseHealth(damage, PLAYER_DEATH_TYPE_SAUCERBOOM);


}


/******************** MOVE PLAYER: SAUCER ***********************/

static void MovePlayer_Saucer(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
OGLPoint2D	dropZonePt = {1e9f, 1e9f};

	GetObjectInfo(theNode);


			/* DO CONTROL */

	CheckSaucerActionControls(theNode);


			/***********/
			/* MOVE IT */
			/***********/

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;

		/* CHECK IF OUT OF BOUNDS */

	if (gCoord.x < (TERRAIN_SUPERTILE_UNIT_SIZE * 4))
		gCoord.x = TERRAIN_SUPERTILE_UNIT_SIZE * 4;
	else
	if (gCoord.x > (gTerrainUnitWidth - TERRAIN_SUPERTILE_UNIT_SIZE * 4))
		gCoord.x = gTerrainUnitWidth - TERRAIN_SUPERTILE_UNIT_SIZE * 4;

	if (gCoord.z < (TERRAIN_SUPERTILE_UNIT_SIZE * 4))
		gCoord.z = TERRAIN_SUPERTILE_UNIT_SIZE * 4;
	else
	if (gCoord.z > (gTerrainUnitDepth - TERRAIN_SUPERTILE_UNIT_SIZE * 4))
		gCoord.z = gTerrainUnitDepth - TERRAIN_SUPERTILE_UNIT_SIZE * 4;



				/* CALC SPEED */

	VectorLength2D(theNode->Speed2D, gDelta.x, gDelta.z);					// calc 2D speed value
	if ((theNode->Speed2D >= 0.0f) && (theNode->Speed2D < 10000000.0f))		// check for weird NaN bug
	{
	}
	else
	{
		theNode->Speed2D = 0;
		gDelta.x = gDelta.z = 0;
	}

	if (theNode->Speed2D > gCurrentMaxSpeed)						// see if limit top speed
	{
		float	tweak = theNode->Speed2D / gCurrentMaxSpeed;
		gDelta.x /= tweak;
		gDelta.z /= tweak;
		theNode->Speed2D = gCurrentMaxSpeed;
	}

		/*************/
		/* COLLISION */
		/*************/

	DoPlayerSaucerCollision(theNode);


		/* SEE IF @ DROP-OFF ZONE */

	if (CalcRocketDropZone(&dropZonePt)													// calc coord of zone
		&& CalcQuickDistance(dropZonePt.x, dropZonePt.y, gCoord.x, gCoord.z) < 200.0f)	// see if in the zone
	{
		if (gNumHumansInSaucer > 0)														// see if there is anyone to put down there
		{
			theNode->MoveCall = MovePlayer_Saucer_ToDropZone;
			DisableHelpType(HELP_MESSAGE_SAUCEREMPTY);									// won't need to help player on this again
		}
		else
			DisplayHelpMessage(HELP_MESSAGE_SAUCEREMPTY, 1, false);
	}





			/* UPDATE IT */

	UpdatePlayer_Saucer(theNode);
}


/******************** MOVE PLAYER: SAUCER TO DROPZONE ***********************/

static void MovePlayer_Saucer_ToDropZone(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
OGLPoint2D	dropZonePt;
OGLVector2D	v;

	GetObjectInfo(theNode);

	if (!CalcRocketDropZone(&dropZonePt))						// calc coord of zone
		DoFatalAlert("MovePlayer_Saucer_ToDropZone: no drop zone");

	if ((CalcDistance(dropZonePt.x, dropZonePt.y, gCoord.x, gCoord.z)) < 10.0f)		// see if close enough
	{

				/* STOP ANY CURRENT BEAM */

		ObjNode	*beam = theNode->ChainNode;
		if (beam)
		{
			DeleteObject(beam);									// this will subrecurse and delete the spiral as well!
			theNode->ChainNode = nil;
			gBeamCharge = 0;
			gBeamIsCharging = false;
		}

			/* CREATE NEW BEAM */

		theNode->MoveCall = MovePlayer_Saucer_BeamDownHumans;
		gBeamMode = BEAM_MODE_DROPZONE;
		ActivatePlayerSaucerBeam(theNode);
		gBeamDownDelay = 1.0;
		gNumHumansInTransit = gNumHumansInSaucer;				// set counter
	}
	else
	{
		v.x = dropZonePt.x - gCoord.x;							// calc vector to zone
		v.y = dropZonePt.y - gCoord.z;
		OGLVector2D_Normalize(&v, &v);

		gDelta.x = v.x * 100.0f;								// set deltas
		gDelta.z = v.y * 100.0f;

		gCoord.x += gDelta.x * fps;								// move it
		gCoord.z += gDelta.z * fps;
	}


			/* UPDATE IT */

	UpdatePlayer_Saucer(theNode);
}


/******************** MOVE PLAYER SAUCER: BEAM DOWN HUMANS ***********************/

static void MovePlayer_Saucer_BeamDownHumans(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
short	humanType;
ObjNode	*human;

	GetObjectInfo(theNode);


			/****************************/
			/* SEE IF BEAM DOWN A HUMAN */
			/****************************/

	if (gNumHumansInSaucer > 0)								// see if any humans remaining or if just waiting for others to finish beaming
	{
		gBeamDownDelay -= fps;								// see if time to beam someone down
		if (gBeamDownDelay <= 0.0f)
		{
			gBeamDownDelay = 1.5f;

			gNumHumansInSaucer--;									// dec # in saucer
			humanType = gHumansInSaucerList[gNumHumansInSaucer];	// start @ end of list and work down

					/* MAKE THE HUMAN */

			human = MakeHuman(humanType, gCoord.x, gCoord.z);

				/* SET HUMAN INFO */


			human->Coord.y = gCoord.y - 40.0f;
			human->Rot.y = RandomFloat()*PI2;
			SetSkeletonAnim(human->Skeleton, 2);					// set adbuction anim (#2 for all humans)
			human->MoveCall = MoveHuman_BeamDown;
		}
	}

			/* UPDATE IT */

	UpdatePlayer_Saucer(theNode);

	if (gNumHumansInTransit == 0)								// see if all done and clear to leave
	{
		StopPlayerSaucerBeam(theNode);
		theNode->MoveCall = MovePlayer_Saucer;
	}

}


/******************** MOVE PLAYER SAUCER: BEAM PLAYER ***********************/

static void MovePlayer_Saucer_BeamPlayer(ObjNode *theNode)
{

	GetObjectInfo(theNode);

			/* UPDATE IT */

	UpdatePlayer_Saucer(theNode);

}




/**************** CALC SAUCER DROPZONE ***********************/

static Boolean CalcRocketDropZone(OGLPoint2D *where)
{
	if (gExitRocket)
	{
		float	r = gExitRocket->Rot.y;					// calc coord of zone
		where->x = gExitRocket->Coord.x + sin(r) * 400.0f;
		where->y = gExitRocket->Coord.z + cos(r) * 400.0f;
		return(true);
	}

	return(false);
}


/************************ UPDATE PLAYER: SAUCER ***************************/

static void UpdatePlayer_Saucer(ObjNode *theNode)
{
float fps = gFramesPerSecondFrac;


			/* SPIN IT */


	theNode->Rot.y += fps * 3.0f;

	gPlayerInfo.coord = gCoord;				// update player coord


			/* UPDATE FINAL SPEED VALUES */

	VectorLength2D(theNode->Speed2D, gDelta.x, gDelta.z);
	theNode->Speed3D = CalcVectorLength(&gDelta);

	if (theNode->Speed2D < gTargetMaxSpeed)					// if we're less than the target, then just reset current to target
		gCurrentMaxSpeed = gTargetMaxSpeed;
	else
	if (gCurrentMaxSpeed > gTargetMaxSpeed)					// see if in overdrive, so readjust currnet
	{
		if (theNode->Speed2D < gCurrentMaxSpeed)			// we're slower than Current, so adjust current down to us
		{
			gCurrentMaxSpeed = theNode->Speed2D;
		}
	}


		/* CHECK INV TIMER */

	gPlayerInfo.invincibilityTimer -= fps;

	gTimeSinceLastThrust += fps;
	gTimeSinceLastShoot += fps;


	UpdateObject(theNode);


			/* UDPATE BEAM */

	UpdatePlayerSaucerBeam(theNode);


		/* UPDATE SAUCER HUM */

	if (theNode->EffectChannel != -1)
		Update3DSoundChannel(EFFECT_SAUCER, &theNode->EffectChannel, &theNode->Coord);
	else
		theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_SAUCER, &theNode->Coord, NORMAL_CHANNEL_RATE, 0.5f);



			/*************************/
			/* SEE IF READY TO LEAVE */
			/*************************/

	if ((gPlayerInfo.fuel >= 1.0f) && (!gBeamIsCharging) && (!gBeamIsDischarging) && (gNumHumansInTransit == 0) && (gNumHumansInSaucer == 0) && (gPlayerInfo.objNode == gPlayerSaucer))	// see if fueled up & ready
	{
		OGLPoint2D	where = {1e9, 1e9};

		if (CalcRocketDropZone(&where)
			&& CalcQuickDistance(gCoord.x, gCoord.z, where.x, where.y) < 400.0f)			// see if in range of rocket
		{
			DisplayHelpMessage(HELP_MESSAGE_PRESSJUMPTOLEAVE, 1, true);

			if (GetNewNeedState(kNeed_Jump))												// see if player wants out
			{
						/* START THE BEAM */

				gBeamMode = BEAM_MODE_DROPZONE;
				ActivatePlayerSaucerBeam(theNode);

						/* MAKE OTTO COME DOWN */

				MakeOttoFromSaucer(theNode);
				theNode->MoveCall = MovePlayer_Saucer_BeamPlayer;
			}
		}
	}
}


/************************** MAKE OTTO FROM SAUCER *********************************/

static void MakeOttoFromSaucer(ObjNode *saucer)
{
ObjNode	*otto;
int		i;

#pragma unused (saucer)

		/*****************************/
		/* MAKE OTTO'S SKELETON BODY */
		/*****************************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_OTTO;
	gNewObjectDefinition.animNum	= PLAYER_ANIM_DRILLED;
	gNewObjectDefinition.coord.x 	= gCoord.x;
	gNewObjectDefinition.coord.z 	= gCoord.z;
	gNewObjectDefinition.coord.y 	= gCoord.y - 20.0f;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG|STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= PLAYER_SLOT;
	gNewObjectDefinition.moveCall	= MoveHuman_BeamDown;		// share human's code
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= OTTO_SAUCER_SCALE;

	otto = MakeNewSkeletonObject(&gNewObjectDefinition);

	gPlayerInfo.objNode 	= otto;


	otto->CType = CTYPE_PLAYER;


		/***********************/
		/* CREATE HAND OBJECTS */
		/***********************/

	for (i = 0; i < (GLOBAL_ObjType_FlameGunHand-GLOBAL_ObjType_OttoLeftHand+1); i++)
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_OttoLeftHand+i, 0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);	// set this model to be sphere mapped


			/* LEFT HAND */

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_OttoLeftHand;
	gNewObjectDefinition.slot		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	gPlayerInfo.leftHandObj 		= MakeNewDisplayGroupObject(&gNewObjectDefinition);



			/* RIGHT HAND */

	gNewObjectDefinition.type 	= GLOBAL_ObjType_OttoRightHand;
	gPlayerInfo.rightHandObj 	= MakeNewDisplayGroupObject(&gNewObjectDefinition);

	gPlayerInfo.rightHandObj->Kind = WEAPON_TYPE_FIST;

	AttachShadowToObject(otto, 0, 2, 2, false);

	CreatePlayerSparkles(otto);

}




#pragma mark -


/********************** DO PLAYER SAUCER COLLISION ***************************/

static void DoPlayerSaucerCollision(ObjNode *theNode)
{
int		i;

			/* HANDLE IT */

	HandleCollisions(theNode, CTYPE_MISC|CTYPE_HURTME, .7);


			/* SCAN FOR INTERESTING STUFF */

	for (i=0; i < gNumCollisions; i++)
	{
		if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
		{
			ObjNode *hitObj = gCollisionList[i].objectPtr;				// get ObjNode of this collision

			if (hitObj->CType == INVALID_NODE_FLAG)				// see if has since become invalid
				continue;

			/* CHECK FOR HURT ME */

			if (hitObj->CType & CTYPE_HURTME)
			{
				PlayerLoseHealth(hitObj->Damage, PLAYER_DEATH_TYPE_SAUCERBOOM);
				DeleteObject(hitObj);								// for saucer ASSUME that this is a weapon which can be deleted
			}
		}
	}
}


/**************** CHECK PLAYER ACTION CONTROLS ***************/

static void CheckSaucerActionControls(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
OGLMatrix3x3		m;
static OGLPoint2D origin = {0,0};

				/* CHECK FOR CHEATS */

	if (GetCheatKeyCombo())
	{
		gPlayerInfo.fuel = 1;
	}

				/*****************************/
				/* CHECK USER MOTION CONTROL */
				/*****************************/

	if (gPlayerInfo.analogControlX || gPlayerInfo.analogControlZ)	// if player is attempting some control then reset this timer
	{
		gTimeSinceLastThrust = 0;
		gForceCameraAlignment = false;
	}

			/* ROTATE ANALOG ACCELERATION VECTOR BASED ON CAMERA POS & APPLY TO DELTA */

	if ((gPlayerInfo.analogControlX == 0.0f) && (gPlayerInfo.analogControlZ == 0.0f))	// see if not acceling
	{
		theNode->AccelVector.x = theNode->AccelVector.y = 0;
	}
	else
	{
		OGLMatrix3x3_SetRotateAboutPoint(&m, &origin, gPlayerToCameraAngle);			// make a 2D rotation matrix camera-rel
		theNode->AccelVector.x = gPlayerInfo.analogControlX;
		theNode->AccelVector.y = gPlayerInfo.analogControlZ;
		OGLVector2D_Normalize(&theNode->AccelVector, &theNode->AccelVector);
		OGLVector2D_Transform(&theNode->AccelVector, &m, &theNode->AccelVector);		// rotate the acceleration vector


					/* APPLY ACCELERATION TO DELTAS */

		gDelta.x += theNode->AccelVector.x * (CONTROL_SENSITIVITY_SAUCER * fps);
		gDelta.z += theNode->AccelVector.y * (CONTROL_SENSITIVITY_SAUCER * fps);
	}

				/***********************/
				/* CHECK BEAM CONTROLS */
				/***********************/

				/* SEE IF CHANGE BEAMS */

	if (GetNewNeedState(kNeed_NextWeapon) || GetNewNeedState(kNeed_PrevWeapon))
	{
		if (gBeamModeSelected == BEAM_MODE_DESTRUCTO)
			gBeamModeSelected = BEAM_MODE_TELEPORT;
		else
			gBeamModeSelected = BEAM_MODE_DESTRUCTO;

		PlayEffect(EFFECT_WEAPONCLICK);
	}


				/* SEE IF CHARGE BEAM */

	if (GetNewNeedState(kNeed_Shoot) || gBeamIsCharging)
		ChargeBeam(theNode);
	else
		DischargeBeam(theNode);
}


/******************* BLOW UP SAUCER **********************/

void BlowUpSaucer(ObjNode *saucer)
{
	PlayEffect_Parms3D(EFFECT_SAUCERKABOOM, &saucer->Coord, NORMAL_CHANNEL_RATE, 3.0);
	Rumble(1.0f, 1000);
	MakeSparkExplosion(saucer->Coord.x, saucer->Coord.y, saucer->Coord.z, 500.0f, 2.0, PARTICLE_SObjType_RedSpark,0);
	MakeSparkExplosion(saucer->Coord.x, saucer->Coord.y, saucer->Coord.z, 600.0f, 2.0, PARTICLE_SObjType_WhiteSpark3,0);
	ExplodeGeometry(saucer, 500, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .3);
	saucer->StatusBits |= STATUS_BIT_HIDDEN|STATUS_BIT_NOMOVE;

	StopAChannel(&saucer->EffectChannel);

	StopPlayerSaucerBeam(saucer);


	StartDeathExit(2.5);

}

#pragma mark -


/********************** CHARGE BEAM ***********************************/

static void ChargeBeam(ObjNode *saucer)
{
	if (gBeamIsDischarging)													// cant charge if still discharging
		return;

			/* SEE IF START NEW CHARGE */

	if (!gBeamIsCharging)
	{
		gBeamIsCharging = true;
		gBeamCharge = 0;
		gBeamMode = gBeamModeSelected;							// set this beam to the currently selected mode
	}


		/* CONTINUE CHARGING */

	else
	{
		gBeamCharge += gFramesPerSecondFrac * .5f;
		if (gBeamCharge > 1.0f)
		{
			gBeamCharge = 1.0f;
			gBeamIsCharging = false;
			ActivatePlayerSaucerBeam(saucer);
			return;
		}
	}


			/* UPDATE AUDIO */

	if (gBeamChargeEffectChannel == -1)
		gBeamChargeEffectChannel = PlayEffect(EFFECT_NOVACHARGE);
	else
		ChangeChannelRate(gBeamChargeEffectChannel, NORMAL_CHANNEL_RATE * (.8f + gBeamCharge));

}


/********************* DISCHARGE BEAM ******************************/

static void DischargeBeam(ObjNode *saucer)
{
ObjNode	*beam;

	if (!gBeamIsDischarging)
		return;

	gBeamCharge -= gFramesPerSecondFrac * .3f;
	if (gBeamCharge <= 0.0f)
	{
		gBeamCharge = 0;

		if (gNumHumansInTransit == 0)											// keep going until last human is up
		{
			gBeamIsDischarging = false;

			beam = saucer->ChainNode;											// get beam obj
			StopAChannel(&beam->EffectChannel);									// stop sfx
			saucer->ChainNode = nil;											// detach from chain
			DeleteObject(beam);													// stop beam & sparkle
		}
	}




}


/**************** ACTIVATE PLAYER SAUCER BEAM ***************************/

static void ActivatePlayerSaucerBeam(ObjNode *saucer)
{
ObjNode	*beam, *spiral;
int		i;
static const OGLColorRGBA beamColors[3] =
{
	{1,1,1,1},							// teleport
	{1,.1,.2,1},						// destructo
	{1,1,1,1},							// dropzone
};


	StopAChannel(&gBeamChargeEffectChannel);				// stop audio of any currently charging beam



						/* MAKE BEAM */

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_WhiteBeam;
	gNewObjectDefinition.coord		= gCoord;
	gNewObjectDefinition.flags 		= STATUS_BIT_GLOW | STATUS_BIT_NOLIGHTING | STATUS_BIT_NOZWRITES | STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.slot 		= SPRITE_SLOT - 1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .6;
	beam = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	saucer->ChainNode = beam;

	beam->ColorFilter = beamColors[gBeamMode];


						/* MAKE SPIRAL */

	gNewObjectDefinition.type 		= GLOBAL_ObjType_BlueSpiral;
	spiral = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	beam->ChainNode = spiral;

	spiral->ColorFilter.a = .1f;


					/* MAKE SPARKLE */

	i = beam->Sparkles[0] = GetFreeSparkle(beam);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
		gSparkles[i].where = gCoord;

		gSparkles[i].color = beamColors[gBeamMode];

		gSparkles[i].scale 		= 500.0f;
		gSparkles[i].separation = 400.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark4;
	}

	gBeamIsDischarging = true;
}


/********************* STOP CURRENT BEAM **********************/

void StopPlayerSaucerBeam(ObjNode *saucer)
{
	StopAChannel(&gBeamChargeEffectChannel);				// stop audio of any currently charging beam

	gBeamIsCharging = gBeamIsDischarging = false;
	gBeamCharge = 0;

	if (saucer->ChainNode)							// if beam is attached, then just delete that too
	{
		DeleteObject(saucer->ChainNode);
		saucer->ChainNode = nil;
	}

}


/****************** UPDATE PLAYER SAUCER BEAM ***************************/

static void UpdatePlayerSaucerBeam(ObjNode *saucer)
{
int		i;
ObjNode	*beam, *spiral, *human;
float	fps = gFramesPerSecondFrac;



			/*************************/
			/* UPDATE BEAM ANIMATION */
			/*************************/

	beam = saucer->ChainNode;							// get beam obj
	if (gBeamIsDischarging)
	{
				/* UPDATE BEAM */

		beam->ColorFilter.a = .9f - RandomFloat()*.6f;

		beam->Coord = gCoord;						// set beam coords
		TurnObjectTowardTarget(beam, &gCoord, gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z, 100, false);	// aim at camera
		UpdateObjectTransforms(beam);


				/* UPDATE SPIRAL */

		spiral = beam->ChainNode;							// get spiral obj
		spiral->Coord = gCoord;								// set beam coords
		spiral->Rot.y -= fps * 5.0f;						// spin it
		UpdateObjectTransforms(spiral);

		switch(gBeamMode)									// see how to animate spiral
		{
			case	BEAM_MODE_TELEPORT:
					MO_Object_OffsetUVs(spiral->BaseGroup, fps * 4.0f, 0);
					break;

			case	BEAM_MODE_DESTRUCTO:
			case	BEAM_MODE_DROPZONE:
					MO_Object_OffsetUVs(spiral->BaseGroup, fps * -4.0f, 0);
					break;
		}

				/* UPDATE SPARKLE */

		i = beam->Sparkles[0];								// get sparkle index
		if (i != -1)
		{
			gSparkles[i].where.x = gCoord.x;
			gSparkles[i].where.z = gCoord.z;
			gSparkles[i].where.y = FindHighestCollisionAtXZ(gCoord.x, gCoord.z, CTYPE_MISC | CTYPE_TERRAIN) + 50.0f;

			gSparkles[i].color.a = .99f - RandomFloat()*.6f;
		}
	}

		/**********************/
		/* BEAM SHOULD BE OFF */
		/**********************/

	else
	{
		if (beam)
		{
					/* FADE BEAM PARTS OUT */

			spiral = beam->ChainNode;							// get spiral obj
			beam->ColorFilter.a -= 1.5f * fps;
			spiral->ColorFilter.a -= 1.0f * fps;
			if (spiral->ColorFilter.a < 0.0f)
				spiral->ColorFilter.a = 0;

					/* DELETE BEAM ONCE FADED COMPLETELY */

			if (beam->ColorFilter.a <= 0.0f)
			{
				i = saucer->Sparkles[0];				// delete sparkle
				if (i != -1)
				{
					DeleteSparkle(i);
					saucer->Sparkles[0] = -1;
				}

				DeleteObject(beam);						// this will subrecurse and delete the spiral as well!
				beam = saucer->ChainNode = nil;
				spiral = nil;
			}
		}
	}


	if (beam)
	{
				/*******************/
				/* UPDATE BEAM HUM */
				/*******************/

		if (beam->EffectChannel != -1)
			Update3DSoundChannel(EFFECT_BEAMHUM, &beam->EffectChannel, &beam->Coord);
		else
			beam->EffectChannel = PlayEffect_Parms3D(EFFECT_BEAMHUM, &beam->Coord, NORMAL_CHANNEL_RATE * 1.2f, 0.6f);



			/**************************/
			/* SEE IF TELEPORT PLAYER */
			/**************************/

		if ((gNumHumansInSaucer + gNumHumansInTransit) >= MAX_HUMANS_IN_SAUCER)						// see if saucer is full
		{
			if (saucer->MoveCall == MovePlayer_Saucer)												// show help message if still just cruising around
				DisplayHelpMessage(HELP_MESSAGE_SAUCERFULL, 4, true);
		}
		else
		if (gBeamMode == BEAM_MODE_TELEPORT)
		{
			if (gBeamCharge > 0.0f)																		// only do it if beam is still charged (not just on for last humans to get up)
			{
				human = FindClosestCType(&gCoord, CTYPE_HUMAN);											// find the closest human
				if (human)
				{
					if (CalcDistance(human->Coord.x, human->Coord.z, gCoord.x, gCoord.z) < 70.0f)		// see if close enough to teleport
					{
								/* CALL HUMAN'S HANDLER */

						if (human->SaucerAbductHandler)
							human->SaucerAbductHandler(human);
						gNumHumansInTransit++;
						DisableHelpType(HELP_MESSAGE_BEAMHUMANS);							// dont show this help message anymore
					}
				}
			}
		}
	}
}



#pragma mark -

/************* MOVE OBJECT UNDER PLAYER SAUCER ***********************/
//
// Move call for generic objects on Saucer level
//

void MoveObjectUnderPlayerSaucer(ObjNode *theNode)
{
float	dist;

			/* SEE IF GONE */

	if (TrackTerrainItem(theNode))
	{
		DeleteObject(theNode);
		return;
	}


		/* SEE IF IN SAUCER'S SHADOW */

	dist = SeeIfUnderPlayerSaucerShadow(theNode);


		/* SEE IF BLOWN UP BY BEAM */

	if (gBeamIsDischarging && (gBeamMode == BEAM_MODE_DESTRUCTO))
	{
		if (dist < theNode->BoundingSphereRadius)
		{
			if (theNode->HurtCallback)						// if has custom callback, then call it
			{
				theNode->HurtCallback(theNode,0);
			}
			else											// otherwise, do default kaboom
			{
				PlayEffect_Parms3D(EFFECT_SAUCERKABOOM, &theNode->Coord, NORMAL_CHANNEL_RATE, 2.0);
				MakeSparkExplosion(theNode->Coord.x, theNode->Coord.y, theNode->Coord.z, 400.0f, 1.0, PARTICLE_SObjType_RedSpark,0);
				ExplodeGeometry(theNode, 1000, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .5);
				theNode->TerrainItemPtr = nil;
				DeleteObject(theNode);
			}
			return;
		}
	}
}


/*********************** SEE IF UNDER SAUCER SHADOW ***************************/

float SeeIfUnderPlayerSaucerShadow(ObjNode *theNode)
{
float	d;
static const OGLColorRGBA white = {1,1,1,1};


	d = CalcDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
	if (d < SHADOW_RADIUS)
	{
		float	c = d * (1.0f / SHADOW_RADIUS);					// calc ratio 1.0..0.0

		c = .4f + (c * .6f);									// adjust to .4 to 1.0

		theNode->ColorFilter.r =
		theNode->ColorFilter.g =
		theNode->ColorFilter.b = c;

	}
	else
		theNode->ColorFilter = white;

	if (theNode->ChainNode)										// subrecurse
		if (!(theNode->ChainNode->StatusBits & STATUS_BIT_GLOW))
			SeeIfUnderPlayerSaucerShadow(theNode->ChainNode);

	return(d);													// return dist
}













