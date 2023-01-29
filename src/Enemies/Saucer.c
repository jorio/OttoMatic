/****************************/
/*   	SAUCER.C		    */
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

static void CreateAlienSaucerObject(ObjNode *who);

static void MoveAlienSaucer(ObjNode *theNode);
static void ActivateSaucerBeam(ObjNode *saucer, OGLColorRGBA *color);
static void UpdateSaucerBeam(ObjNode *theNode);
static void UpdateSaucer(ObjNode *topObj);
static void MoveAlienSaucer_TowardTarget(ObjNode *topObj);
static void SaucerReachedTarget(ObjNode *saucer);
static void MoveAlienSaucer_Leaving(ObjNode *topObj);
static void MoveAlienSaucer_Abducting(ObjNode *topObj);
static void DeleteSaucer(ObjNode *saucer);
static void MoveAlienSaucer_Radiating(ObjNode *topObj);


/****************************/
/*    CONSTANTS             */
/****************************/


#define	SAUCER_HOVER_HEIGHT	1000.0f
#define	SAUCER_MAX_SPEED	3000.0f

#define	BEAM_ON_DIST		1500.0f

#define	SAUCER_SCALE		2.5f

#define SAUCER_FADEIN_TIME	1.0f

#define	SAUCER_VANISH_DIST	9000.0f
#define	SAUCER_FADEOUT_DIST	2500.0f
#define	SAUCER_STARTFADEOUT_DIST	(SAUCER_VANISH_DIST - SAUCER_FADEOUT_DIST)


enum
{
	SAUCER_MODE_TOTARGET,
	SAUCER_MODE_ABDUCTING,
	SAUCER_MODE_RADIATING,
	SAUCER_MODE_LEAVING
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	Deformation		Special[1]
#define	SpecialSaucerFade	SpecialF[0]
#define	SpecialSaucerAge	SpecialF[1]

ObjNode	*gAlienSaucer = nil;
ObjNode *gSaucerTarget = nil;
static float gSaucerRadiatingTimer = 0;

static float gSaucerStuckTimer = 0;


/**************************** CALL ALIEN SAUCER **************************/
//
// Objects which can attract the saucer call this function to try and get the
// saucer to come after it.
//

Boolean CallAlienSaucer(ObjNode *who)
{
	float maxCallDistance = 4000.0f;

	switch(gLevelNum)									// no saucers on some levels
	{
		case	LEVEL_NUM_BLOBBOSS:
		case	LEVEL_NUM_SAUCER:
		case	LEVEL_NUM_BRAINBOSS:
				return(false);
	}

	if (!gPlayerHasLanded)								// no saucers until player is ready
		return(false);

	if (gAlienSaucer)									// cannot call if the saucer is already active
		return(false);

	if (who->StatusBits & STATUS_BIT_ISCULLED)			// only call if this object currently visible by player
		return(false);

	if (!(who->StatusBits & STATUS_BIT_SAUCERTARGET))	// must still be a saucer target
		return(false);

	if (MyRandomLong() & 0xf)							// randomize so that various types of callers have a chance each time
		return(false);

	if (gPlayerInfo.isTeleporting)						// not while Otto is teleporting (no controls)
		return(false);

	// Reduce call distance if Otto is doing some level-specific acrobatics with non-standard controls
	if (gPlayerInfo.objNode && gPlayerInfo.objNode->Skeleton)
	{
		switch (gPlayerInfo.objNode->Skeleton->AnimNum)
		{
			case PLAYER_ANIM_ROCKETSLED:
			case PLAYER_ANIM_PICKUPANDHOLDMAGNET:
			case PLAYER_ANIM_SHOOTFROMCANNON:
			case PLAYER_ANIM_RIDEZIP:					// Level 4: scientist near zipline 2 nearly impossible to rescue otherwise
			case PLAYER_ANIM_BUBBLE:					// Level 2: some women cannot be rescued if Otto's timing is out of phase with their splinePlacement
				maxCallDistance = 2000.0f;
				break;
		}
	}

	// Don't call if too far away
	if (CalcQuickDistance(who->Coord.x, who->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) > maxCallDistance)
		return(false);

	// Don't call if there's a fence or a gate between us
	if (SeeIfLineSegmentHitsAnything(&who->Coord, &gPlayerInfo.coord, nil, CTYPE_FENCE | CTYPE_BLOCKRAYS))
		return(false);




			/* CREATE THE SAUCER */

	CreateAlienSaucerObject(who);

	return(true);
}


#pragma mark -

/************************* CREATE ALIEN SAUCER OBJECT *********************************/

static void CreateAlienSaucerObject(ObjNode *who)
{
ObjNode	*bottom;
int		i;
float	x,z,y;
OGLVector2D	v;
DeformationType		defData;

	gSaucerStuckTimer = 0;

	gSaucerTarget = who;							// remember who we're going after
	who->StatusBits |= STATUS_BIT_SAUCERTARGET;

				/* FIGURE OUT WHERE WE WANT TO START IT */

	v.x = gGameViewInfoPtr->cameraPlacement.pointOfInterest.x - gGameViewInfoPtr->cameraPlacement.cameraLocation.x;
	v.y = gGameViewInfoPtr->cameraPlacement.pointOfInterest.z - gGameViewInfoPtr->cameraPlacement.cameraLocation.z;
	FastNormalizeVector2D(v.x, v.y, &v, true);

	x = gGameViewInfoPtr->cameraPlacement.cameraLocation.x + v.x * 8000.0f;
	z = gGameViewInfoPtr->cameraPlacement.cameraLocation.z + v.y * 8000.0f;
	y = GetTerrainY2(x,z) + 2500.0f;


				/* CREATE TOP */

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_EnemySaucer_Top;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveAlienSaucer;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= SAUCER_SCALE;
	gAlienSaucer = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	gAlienSaucer->Mode = SAUCER_MODE_TOTARGET;
	gAlienSaucer->SpecialSaucerFade = 0.0f;
	gAlienSaucer->SpecialSaucerAge = 0.0f;

	CreateCollisionBoxFromBoundingBox(gAlienSaucer, 1.1,1.1);


			/* CREATE BOTTOM */

	gNewObjectDefinition.type 		= GLOBAL_ObjType_EnemySaucer_Bottom;
	gNewObjectDefinition.moveCall 	= nil;
	bottom = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	gAlienSaucer->ChainNode = bottom;


			/* INIT FADE COLOR */

	gAlienSaucer->ColorFilter.a = 0;
	bottom->ColorFilter.a = 0;



			/*******************/
			/* CREATE SPARKLES */
			/*******************/

	i = gAlienSaucer->Sparkles[1] = GetFreeSparkle(gAlienSaucer);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
		gSparkles[i].where = gAlienSaucer->Coord;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = .5;
		gSparkles[i].color.b = 0;
		gSparkles[i].color.a = 1.0;

		gSparkles[i].scale = 100.0f;
		gSparkles[i].separation = 20.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark;
	}

	i = gAlienSaucer->Sparkles[2] = GetFreeSparkle(gAlienSaucer);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
		gSparkles[i].where = gAlienSaucer->Coord;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = .5;
		gSparkles[i].color.b = 0;
		gSparkles[i].color.a = 1.0;

		gSparkles[i].scale = 100.0f;
		gSparkles[i].separation = 20.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark;
	}


		/* CREATE DEFORM WAVE */

	if (gG4)										// only do this on a G4 since we need the horsepower
	{
		defData.type 				= DEFORMATION_TYPE_CONTINUOUSWAVE;
		defData.amplitude 			= 15;
		defData.radius 				= 0;
		defData.speed 				= 2000;
		defData.origin.x			= x;
		defData.origin.y			= z;
		defData.oneOverWaveLength 	= 1.0f / 100.0f;
		defData.radialWidth			= 600.0f;
		defData.decayRate			= 20.0f;
		gAlienSaucer->Deformation = NewSuperTileDeformation(&defData);
	}
	else
		gAlienSaucer->Deformation = -1;
}


/******************** DELETE SAUCER *************************/

static void DeleteSaucer(ObjNode *saucer)
{
		/* STOP THE DEFORMATION FUNCTION */

	if (gAlienSaucer->Deformation != -1)
	{
		DeleteTerrainDeformation(gAlienSaucer->Deformation);
		gAlienSaucer->Deformation = -1;
	}

	DeleteObject(saucer);
	gAlienSaucer = nil;
}


#pragma mark -

/*********************** MOVE ALIEN SAUCER ****************************/

static void MoveAlienSaucer(ObjNode *topObj)
{
	GetObjectInfo(topObj);

	topObj->SpecialSaucerAge += gFramesPerSecondFrac;
	topObj->SpecialSaucerFade = 1.0f;		// reset fade

			/* STATE UPDATE */

	switch(topObj->Mode)
	{
		case	SAUCER_MODE_TOTARGET:
				MoveAlienSaucer_TowardTarget(topObj);
				break;

		case	SAUCER_MODE_ABDUCTING:
				MoveAlienSaucer_Abducting(topObj);
				break;

		case	SAUCER_MODE_RADIATING:
				MoveAlienSaucer_Radiating(topObj);
				break;

		case	SAUCER_MODE_LEAVING:
				MoveAlienSaucer_Leaving(topObj);
				break;
	}

	if (gAlienSaucer)
	{
		UpdateSaucer(topObj);
	}

}



/********************** MOVE ALIEN SAUCER:  TOWARD TARGET ***************************/

static void MoveAlienSaucer_TowardTarget(ObjNode *topObj)
{
float	fps = gFramesPerSecondFrac;
float	dist,speed,tx,ty,tz,gy;
OGLVector3D	aim;


			/* FADE IN IF APPEARED RECENTLY */

	if (topObj->SpecialSaucerAge < SAUCER_FADEIN_TIME)
	{
		topObj->SpecialSaucerFade = topObj->SpecialSaucerAge/SAUCER_FADEIN_TIME;
	}


		/**********************/
		/* MOVE TOWARD TARGET */
		/**********************/

	if (gSaucerTarget)
	{
				/* VERIFY TARGET */

		if (gSaucerTarget->CType == INVALID_NODE_FLAG)							// see if node is invalid
			goto no_target;

		if (!(gSaucerTarget->StatusBits & STATUS_BIT_SAUCERTARGET))				// if obj is not a saucer target anymore
			goto no_target;

		if (gSaucerTarget->StatusBits & STATUS_BIT_DETACHED)					// check if target is detached (usually for spline items that are out of range)
			goto no_target;


		tx = gSaucerTarget->Coord.x;											// get target coords
		ty = gSaucerTarget->Coord.y;
		tz = gSaucerTarget->Coord.z;

				/* SEE IF CLOSE ENOUGH TO LATCH */

		dist = CalcDistance(tx, tz, gCoord.x, gCoord.z);
		if (dist < 60.0f)
		{
			SaucerReachedTarget(topObj);
		}

			/* KEEP TARGETING */

		gy = GetTerrainY_Undeformed(tx, tz);									// get ground y

		aim.x = tx - gCoord.x;													// calc vector to target
		aim.y = gy + SAUCER_HOVER_HEIGHT - gCoord.y;
		aim.z = tz - gCoord.z;
		FastNormalizeVector(aim.x,aim.y,aim.z, &aim);

		dist = CalcDistance3D(tx,ty,tz,gCoord.x,gCoord.y,gCoord.z);				// calc dist to target
		speed = dist * .3f;													// calc speed
		if (speed < 600.0f)
			speed = 600.0f;
		else
		if (speed > SAUCER_MAX_SPEED)
			speed = SAUCER_MAX_SPEED;

		gDelta.x = aim.x * speed;						// calc deltas
		gDelta.y = aim.y * speed;
		gDelta.z = aim.z * speed;
	}

			/* NO TARGET */
	else
	{
no_target:
		gSaucerTarget = nil;
		topObj->Mode = SAUCER_MODE_LEAVING;
	}

			/* MOVE IT */

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	gy = GetTerrainY_Undeformed(gCoord.x, gCoord.z);	// get ground y

	if (gCoord.y < (gy + 500.0f))						// dont let it get too close to the ground; if it does, then bail out
		goto no_target;

	if (HandleCollisions(topObj, CTYPE_MISC, -.6))		// if hit something then leave & try again
	{
		gSaucerStuckTimer += fps;						// see if stuck too long
		if (gSaucerStuckTimer > 3.0f)
			goto no_target;
	}
	else
		gSaucerStuckTimer = 0;
}


/********************** MOVE ALIEN SAUCER:  ABDUCTING ***************************/

static void MoveAlienSaucer_Abducting(ObjNode *topObj)
{
float	fps = gFramesPerSecondFrac;


		/***********************/
		/* MATCH TARGET MOTION */
		/***********************/

	if (gSaucerTarget)
	{
				/* VERIFY TARGET */

		if (gSaucerTarget->CType == INVALID_NODE_FLAG)							// see if node is invalid
			goto no_target;

//		if (!(gSaucerTarget->StatusBits & STATUS_BIT_SAUCERTARGET))				// if obj is not a saucer target anymore
//			goto no_target;

		if (gSaucerTarget->StatusBits & STATUS_BIT_DETACHED)					// check if target is detached (usually for spline items that are out of range)
			goto no_target;


		float tx = gSaucerTarget->Coord.x;										// get target coords
		float tz = gSaucerTarget->Coord.z;
		float y = GetTerrainY_Undeformed(tx, tz) + SAUCER_HOVER_HEIGHT;			// move to y

		gDelta.x = gSaucerTarget->Delta.x;										// match dx/dz of target
		gDelta.z = gSaucerTarget->Delta.z;
		gDelta.y = y - gCoord.y;

		gCoord.x += gDelta.x * fps;
		gCoord.y += gDelta.y * fps;
		gCoord.z += gDelta.z * fps;

		HandleCollisions(topObj, CTYPE_MISC, -.6);


		/***********************/
		/* BRING UP THE TARGET */
		/***********************/

			/* FADE TO X-RED */

		gSaucerTarget->ColorFilter.a -= fps * .1f;
		if (gSaucerTarget->ColorFilter.a < 0.0f)
			goto done;
		gSaucerTarget->ColorFilter.g -= fps * .5f;
		if (gSaucerTarget->ColorFilter.g < 0.0f) gSaucerTarget->ColorFilter.g = 0.0f;
		gSaucerTarget->ColorFilter.b -= fps * .6f;
		if (gSaucerTarget->ColorFilter.b < 0.0f) gSaucerTarget->ColorFilter.b = 0.0f;


		gSaucerTarget->Rot.y += fps * 7.0f;									// make target spin on its way up
		gSaucerTarget->Coord.y += 200.0f * fps;									// move target up to saucer


		if ((gSaucerTarget->Coord.y + gSaucerTarget->BBox.min.y * gSaucerTarget->Scale.y) > (gCoord.y + 200.0f))	// see if done abducting
		{
done:
			topObj->Mode = SAUCER_MODE_LEAVING;
			DeleteObject(gSaucerTarget);
			gSaucerTarget = nil;
		}
		else
		{
			UpdateObjectTransforms(gSaucerTarget);
			UpdateShadow(gSaucerTarget);
		}
	}

				/* NO TARGET */
	else
	{
no_target:
		gSaucerTarget = nil;
		topObj->Mode = SAUCER_MODE_LEAVING;
	}
}


/********************** MOVE ALIEN SAUCER:  RADIATING ***************************/

static void MoveAlienSaucer_Radiating(ObjNode *topObj)
{
float	fps = gFramesPerSecondFrac;


			/* SEE IF DONE RADIATING */

	gSaucerRadiatingTimer -= fps;
	if (gSaucerRadiatingTimer <= 0.0f)
	{
		gSaucerTarget = nil;
		topObj->Mode = SAUCER_MODE_LEAVING;
	}
}


/********************** MOVE ALIEN SAUCER:  LEAVING ***************************/

static void MoveAlienSaucer_Leaving(ObjNode *topObj)
{
float	fps = gFramesPerSecondFrac;


	gDelta.x += 200.0f * fps;
	gDelta.y += 100.0f * fps;
	gDelta.z += 300.0f * fps;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	HandleCollisions(topObj, CTYPE_MISC, -1.0);

		/* SEE HOW FAR */

	float distance = CalcQuickDistance(gCoord.x, gCoord.z, gGameViewInfoPtr->cameraPlacement.cameraLocation.x, gGameViewInfoPtr->cameraPlacement.cameraLocation.z);

		/* FADE OUT IF FAR ENOUGH */

	if (distance > SAUCER_STARTFADEOUT_DIST)
	{
		topObj->SpecialSaucerFade = 1.0f - (distance - SAUCER_STARTFADEOUT_DIST) / (SAUCER_FADEOUT_DIST);
	}

		/* SEE IF GONE */

	if (distance > SAUCER_VANISH_DIST)
	{
		DeleteSaucer(topObj);
		return;
	}

}



/************ UPDATE SAUCER ************************/

static void UpdateSaucer(ObjNode *topObj)
{
ObjNode	*bottom;
float	fps = gFramesPerSecondFrac;
float	r;
int		i;

			/* MAKE IT SPIN */

	bottom = topObj->ChainNode;						// get bottom
	topObj->Rot.y += fps * 3.0f;
	r = bottom->Rot.y -= fps * 3.0f;


			/* UPDATE BEAM */

	UpdateSaucerBeam(topObj);

	UpdateObject(topObj);
	bottom->Coord = gCoord;
	bottom->Coord.y += 5.0f;
	UpdateObjectTransforms(bottom);


			/* SET ALPHA BASED ON FADE COMPUTED IN MOVE CALL */

	topObj->ColorFilter.a = topObj->SpecialSaucerFade;
	bottom->ColorFilter.a = topObj->SpecialSaucerFade;


			/*******************/
			/* UPDATE SPARKLES */
			/*******************/

	i = topObj->Sparkles[1];									// get sparkle index
	if (i != -1)
	{
		gSparkles[i].where.x = gCoord.x - sin(r) * (290.0f * SAUCER_SCALE);		// calc coord
		gSparkles[i].where.z = gCoord.z - cos(r) * (290.0f * SAUCER_SCALE);
		gSparkles[i].where.y = gCoord.y + (60.0f * SAUCER_SCALE);
	}

	i = topObj->Sparkles[2];
	if (i != -1)
	{
		gSparkles[i].where.x = gCoord.x + sin(r) * (290.0f * SAUCER_SCALE);		// calc coord
		gSparkles[i].where.z = gCoord.z + cos(r) * (290.0f * SAUCER_SCALE);
		gSparkles[i].where.y = gCoord.y + (60.0f * SAUCER_SCALE);
	}


		/* UPDATE DEFORMATION */

	if (gG4)										// only do this on a G4 since we need the horsepower
		UpdateDeformationCoords(topObj->Deformation, gCoord.x, gCoord.z);


			/* UPDATE AUDIO */

	if (gAlienSaucer->EffectChannel != -1)
		Update3DSoundChannel(EFFECT_SAUCER, &gAlienSaucer->EffectChannel, &gAlienSaucer->Coord);
	else
		gAlienSaucer->EffectChannel = PlayEffect_Parms3D(EFFECT_SAUCER, &gAlienSaucer->Coord, NORMAL_CHANNEL_RATE, 0.9f);

}


/**************** ACTIVATE SAUCER BEAM ***************************/

static void ActivateSaucerBeam(ObjNode *saucer, OGLColorRGBA *color)
{
ObjNode	*beam, *spiral, *bottom;
int		i;

	bottom = saucer->ChainNode;

						/* MAKE BEAM */

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_WhiteBeam;
	gNewObjectDefinition.coord		= gCoord;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_GLOW | STATUS_BIT_NOLIGHTING | STATUS_BIT_NOZWRITES | STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.slot 		= SPRITE_SLOT - 1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.1;
	beam = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	bottom->ChainNode = beam;

	beam->ColorFilter = *color;


						/* MAKE SPIRAL */

	gNewObjectDefinition.type 		= GLOBAL_ObjType_BlueSpiral;
	spiral = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	beam->ChainNode = spiral;

	spiral->ColorFilter.a = .1f;


					/* MAKE SPARKLE */

	i = saucer->Sparkles[0] = GetFreeSparkle(saucer);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
		gSparkles[i].where = gCoord;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 400.0f;
		gSparkles[i].separation = 1000.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_RedGlint;
	}
}

/****************** UPDATE SAUCER BEAM ***************************/

static void UpdateSaucerBeam(ObjNode *theNode)
{
int		i;
ObjNode	*beam, *spiral, *bottom;
Boolean	beamOn;
float	fps = gFramesPerSecondFrac;
OGLColorRGBA	beamColor;

			/***********************/
			/* SEE IF TURN ON BEAM */
			/***********************/

	if (gSaucerTarget)
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gSaucerTarget->Coord.x, gSaucerTarget->Coord.z) < BEAM_ON_DIST)
			beamOn = true;
		else
			beamOn = false;
	}
	else
	{
		if (theNode->Mode == SAUCER_MODE_RADIATING)
			beamOn = true;
		else
			beamOn = false;
	}

			/*************************/
			/* UPDATE BEAM ANIMATION */
			/*************************/

	bottom = theNode->ChainNode;						// saucer bottom
	beam = bottom->ChainNode;							// get beam obj
	if (beamOn)
	{
				/* CREATE NEW BEAM */

		if (beam == nil)
		{
						/* SET BEAM COLOR */

			if (gSaucerTarget)
			{
				switch(gSaucerTarget->SaucerTargetType)
				{
					case	SAUCER_TARGET_TYPE_ABDUCT:			// abduction beams are red
							beamColor.r = 1;
							beamColor.g = 0;
							beamColor.b = 0;
							beamColor.a = .99;
							break;

					case	SAUCER_TARGET_TYPE_RADIATE:
							beamColor.r = 0;
							beamColor.g = 1;
							beamColor.b = 0;
							beamColor.a = .99;
							break;
				}
				ActivateSaucerBeam(theNode, &beamColor);				// make new beam
			}
		}
		else
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

			if (gSaucerTarget)
			{
				switch(gSaucerTarget->SaucerTargetType)				// see how to animate spiral
				{
					case	SAUCER_TARGET_TYPE_ABDUCT:
							MO_Object_OffsetUVs(spiral->BaseGroup, fps * 4.0f, 0);
							break;

					case	SAUCER_TARGET_TYPE_RADIATE:
							MO_Object_OffsetUVs(spiral->BaseGroup, fps * -4.0f, 0);
							break;
				}
			}

					/* UPDATE SPARKLE */

			i = theNode->Sparkles[0];								// get sparkle index
			if (i != -1)
			{
				gSparkles[i].where.x = gCoord.x;
				gSparkles[i].where.y = gCoord.y - 50.0f;
				gSparkles[i].where.z = gCoord.z;

				gSparkles[i].color.a = .99f - RandomFloat()*.6f;
			}

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
				i = theNode->Sparkles[0];				// delete sparkle
				if (i != -1)
				{
					DeleteSparkle(i);
					theNode->Sparkles[0] = -1;
				}

				DeleteObject(beam);						// this will subrecurse and delete the spiral as well!
				beam = bottom->ChainNode = nil;
				spiral = nil;
			}
		}
	}

				/*******************/
				/* UPDATE BEAM HUM */
				/*******************/
	if (beam)
	{
				/* UPDATE AUDIO */

		if (beam->EffectChannel != -1)
		{
			Update3DSoundChannel(EFFECT_BEAMHUM, &beam->EffectChannel, &beam->Coord);
		}
		else
			beam->EffectChannel = PlayEffect_Parms3D(EFFECT_BEAMHUM, &beam->Coord, NORMAL_CHANNEL_RATE * 1.2f, 0.6f);
	}
}


#pragma mark -

/****************** SAUCER REACHED TARGET **********************/

static void SaucerReachedTarget(ObjNode *saucer)
{
			/* CALL TARGET'S HANDLER */

	if (gSaucerTarget->SaucerAbductHandler)
	{
		gSaucerTarget->SaucerAbductHandler(gSaucerTarget);
	}

			/* WHAT DO WE DO */

	switch(gSaucerTarget->SaucerTargetType)				// see how to animate spiral
	{
		case	SAUCER_TARGET_TYPE_ABDUCT:
				saucer->Mode = SAUCER_MODE_ABDUCTING;
				break;

		case	SAUCER_TARGET_TYPE_RADIATE:
				saucer->Mode = SAUCER_MODE_RADIATING;
				gSaucerRadiatingTimer = 2.5f;
				gSaucerTarget = nil;
				break;
	}

}













