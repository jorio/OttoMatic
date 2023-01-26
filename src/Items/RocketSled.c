/****************************/
/*   	ROCKETSLED.C		    */
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

static void MoveRocketSled(ObjNode *theNode);
static void MakeRocketSledSmoke(ObjNode *theNode);
static void DisableRocketSled(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	ROCKETSLED_SCALE	1.3f

#define	CONTROL_SENSITIVITY_ROCKETSLED_TURN	2.2f

#define	MAX_ROCKETSLED_SPEED			3400.0f




/*********************/
/*    VARIABLES      */
/*********************/


#define	RocketSledBeingRidden	Flag[0]

ObjNode	*gPlayerRocketSled;

static OGLVector2D	gRocketSledStartAim;

static	float		gAirTime;
static	Boolean		gStopOnLanding;

/************************* ADD ROCKETSLED *********************************/

Boolean AddRocketSled(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	if (gLevelNum == LEVEL_NUM_CLOUD)
		gNewObjectDefinition.type 	= CLOUD_ObjType_RocketSled;
	else
		gNewObjectDefinition.type	= FIREICE_ObjType_RocketSled;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= PLAYER_SLOT-3;								// move before player
	gNewObjectDefinition.moveCall 	= MoveRocketSled;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/8);
	gNewObjectDefinition.scale 		= ROCKETSLED_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;							// keep ptr to item list

	newObj->CType 			= CTYPE_MISC|CTYPE_TRIGGER;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= SIDE_BITS_TOP;				// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_ROCKETSLED;
	CreateCollisionBoxFromBoundingBox(newObj,.9,.8);

	return(true);													// item was added
}

/************** DO TRIGGER - ROCKETSLED ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_RocketSled(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
int	i;

#pragma unused (sideBits)

	if (gPlayerRocketSled)
		DoFatalAlert("DoTrig_RocketSled: already in a rocket sled, why double trigger?");

	theNode->CType = CTYPE_MISC;									// no longer a trigger

	gAirTime = 0;
	gStopOnLanding = false;

	gPlayerRocketSled = theNode;

	theNode->RocketSledBeingRidden = true;

	SetSkeletonAnim(whoNode->Skeleton, PLAYER_ANIM_ROCKETSLED);
	AlignPlayerInRocketSled(whoNode);


			/* REMEMBER INITIAL DIRECTION OF AIM */

	gRocketSledStartAim.x = -sin(theNode->Rot.y);
	gRocketSledStartAim.y = -cos(theNode->Rot.y);


			/*****************/
			/* MAKE SPARKLES */
			/*****************/

	i = theNode->Sparkles[0] = GetFreeSparkle(theNode);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_FLICKER|SPARKLE_FLAG_TRANSFORMWITHOWNER;
		gSparkles[i].where.x = -63;
		gSparkles[i].where.y = 24;
		gSparkles[i].where.z = 153;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 300.0f;
		gSparkles[i].separation = 50.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark4;
	}

	i = theNode->Sparkles[1] = GetFreeSparkle(theNode);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_FLICKER|SPARKLE_FLAG_TRANSFORMWITHOWNER;
		gSparkles[i].where.x = 63;
		gSparkles[i].where.y = 24;
		gSparkles[i].where.z = 153;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 300.0f;
		gSparkles[i].separation = 50.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark4;
	}



	return(true);
}






/******************* PLAYER MOVE ROCKETSLED *****************************/

static void MoveRocketSled(ObjNode *theNode)
{
float	r,oldRot;
float	fps = gFramesPerSecondFrac;
OGLVector2D	aim;
uint16_t		effect;


			/* JUST TRACK IT IF NOT BEING DRIVEN YET */

	if (!theNode->RocketSledBeingRidden)
	{
		if (TrackTerrainItem(theNode))
		{
			DeleteObject(theNode);
			return;
		}

		RotateOnTerrain(theNode, 0, nil);
		SetObjectTransformMatrix(theNode);

		return;
	}


				/***********************/
				/* ROCKETSLED BEING RIDDEN*/
				/***********************/

	GetObjectInfo(theNode);

	gForceCameraAlignment = true;								// keep camera behind the car!
	gCameraUserRotY = 0;										// ... and reset user rot


			/* CHECK USER CONTROLS FOR ROTATION */

	if (gPlayerInfo.analogControlX != 0.0f)																// see if spin
	{
		theNode->DeltaRot.y -= gPlayerInfo.analogControlX * fps * CONTROL_SENSITIVITY_ROCKETSLED_TURN;			// use x to rotate
		if (theNode->DeltaRot.y > PI2)																			// limit spin speed
			theNode->DeltaRot.y = PI2;
		else
		if (theNode->DeltaRot.y < -PI2)
			theNode->DeltaRot.y = -PI2;
	}

				/* ROTATE IT */

	oldRot = theNode->Rot.y;
	r = theNode->Rot.y += theNode->DeltaRot.y * fps;

	aim.x = -sin(r);											// see if over rotated
	aim.y = -cos(r);

	if (acos(OGLVector2D_Dot(&aim, &gRocketSledStartAim)) > (PI/4))
	{
		r = theNode->Rot.y = oldRot;							// pin @ previous rot
		theNode->DeltaRot.y = 0;
	}

				/*******************/
				/* DO ACCELERATION */
				/*******************/

//	GetTerrainY(gCoord.x, gCoord.z);								// call this to get the terrain normal
//	ny = cos(gRecentTerrainNormal.y);

	theNode->Speed2D += 900.0f * fps;
	theNode->Speed2D = MinFloat(theNode->Speed2D, MAX_ROCKETSLED_SPEED);

	gDelta.x = -sin(r) * theNode->Speed2D;
	gDelta.z = -cos(r) * theNode->Speed2D;


			/* MOVE IT */

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* UPDATE SOUND */

	if (gLevelNum == LEVEL_NUM_CLOUD)
		effect = EFFECT_ROCKETSLED;
	else
		effect = EFFECT_ROCKETSLED2;

	if (theNode->EffectChannel == -1)
		theNode->EffectChannel = PlayEffect_Parms3D(effect, &gCoord, NORMAL_CHANNEL_RATE*2/3, 2.5);
	else
	{
		ChangeChannelRate(theNode->EffectChannel, NORMAL_CHANNEL_RATE*2/3 + theNode->Speed2D * 15.0f);
		Update3DSoundChannel(effect, &theNode->EffectChannel, &gCoord);
	}


		/* DO COLLISION DETECT */

	if (SIDE_BITS_FENCE & HandleCollisions(theNode, CTYPE_MISC | CTYPE_FENCE, .5))
	{
		// Bouncing off fences may alter our gDelta, so re-compute it
		// (otherwise Otto might inherit a strong backwards gDelta when initiating belly slide)
		gDelta.x = -sin(r) * theNode->Speed2D;
		gDelta.z = -cos(r) * theNode->Speed2D;
	}



		/*******************************/
		/* SEE IF ON TERRAIN OR IN AIR */
		/*******************************/


	if ((gCoord.y + 2.0f) > GetTerrainY(gCoord.x, gCoord.z))	// see if above terrain
	{
		if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// if was on ground before then zero out deltay
			gDelta.y = 0;

		theNode->StatusBits &= ~STATUS_BIT_ONGROUND;

		gDelta.y -= 1100.0f * fps;								// gravity

//		theNode->Rot.x -= .2f * fps;							// rot to level
//		if (theNode->Rot.x < 0.0f)
//			theNode->Rot.x = 0.0f;

		gAirTime += fps;										// inc airtime & see if doing big jump
		if (gAirTime > 1.0f)
		{
			gStopOnLanding = true;
		}

		UpdateObject(theNode);
	}

			/* ON TERRAIN, SO CONFORM */
	else
	{
		OGLPoint3D	tvo, tvn;
		OGLVector3D	v,v2;

		gAirTime = 0;											// reset since not in air

		if (gStopOnLanding)										// see if landed after big jump
		{
			DisableRocketSled(theNode);
			return;
		}

		tvo.x = theNode->OldCoord.x;								// get old terrain coord
		tvo.z = theNode->OldCoord.z;
		tvo.y = GetTerrainY(tvo.x, tvo.z);

		tvn.x = gCoord.x;											// get new terrain coord
		tvn.z = gCoord.z;
		tvn.y = GetTerrainY(tvn.x, tvn.z);

		v.x = tvn.x - tvo.x;										// calc vector from old to new
		v.y = tvn.y - tvo.y;
		v.z = tvn.z - tvo.z;

		if (v.y > 0.0f)											// if slope up, then match terrain deltaY
			gDelta.y = v.y * gFramesPerSecond * 1.1f;					// calc delta y from actual
		else
			gDelta.y = -2000;

		RotateOnTerrain(theNode, 0, nil);
		SetObjectTransformMatrix(theNode);

		theNode->Coord.x = gCoord.x;
		theNode->Coord.y = tvn.y;
		theNode->Coord.z = gCoord.z;
		theNode->Delta = gDelta;

		theNode->StatusBits |= STATUS_BIT_ONGROUND;


				/* CALC X-ROT FROM ALIGNMENT */

		v.x = -sin(r);						// ahead vector
		v.z = -cos(r);
		v.y = 0;

		v2.x = 0;							// aligned vector
		v2.y = 0;
		v2.z = -1;
		OGLVector3D_Transform(&v2, &theNode->BaseTransformMatrix, &v2);
		OGLVector3D_Normalize(&v2,&v2);

		theNode->Rot.x = acos(OGLVector3D_Dot(&v,&v2));
		if (v2.y < 0.0f)
			theNode->Rot.x = -theNode->Rot.x;

	}

	MakeRocketSledSmoke(theNode);



}


/********************** MAKE ROCKET SLED SMOKE ***************************/

static void MakeRocketSledSmoke(ObjNode *theNode)
{
float						fps = gFramesPerSecondFrac;
static const OGLPoint3D		engineOffs[2] = {{-63,24,153}, {63,24,153}};
OGLPoint3D					nozzles[2];
float						x,y,z;
int							particleGroup,magicNum,i;
NewParticleGroupDefType		groupDef;
NewParticleDefType			newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;

	theNode->ParticleTimer -= fps;
	if (theNode->ParticleTimer <= 0.0f)
	{
		theNode->ParticleTimer += 0.02f;

				/* CALC COORD OF BOTH ENGINE NOZZLES */

		OGLPoint3D_TransformArray(engineOffs, &theNode->BaseTransformMatrix, nozzles, 2);


					/* SETUP PARTICLE GROUP */

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND|PARTICLE_FLAGS_ALLAIM;
			groupDef.gravity				= 0;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 25;
			groupDef.decayRate				= -1.0;
			groupDef.fadeRate				= .3;
			groupDef.particleTextureNum		= PARTICLE_SObjType_GreySmoke;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
					/* DO LEFT ENGINE */

			x = nozzles[0].x;
			y = nozzles[0].y;
			z = nozzles[0].z;

			for (i = 0; i < 3; i++)
			{
				p.x = x + RandomFloat2() * 20.0f;
				p.y = y + RandomFloat2() * 20.0f;
				p.z = z + RandomFloat2() * 20.0f;

				d.x = RandomFloat2() * 100.0f;
				d.y = RandomFloat2() * 100.0f;
				d.z = RandomFloat2() * 100.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= 1.0;
				newParticleDef.rotZ			= RandomFloat()*PI2;
				newParticleDef.rotDZ		= RandomFloat()*PI2;
				newParticleDef.alpha		= .6;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}

					/* DO RIGHT ENGINE */

			x = nozzles[1].x;
			y = nozzles[1].y;
			z = nozzles[1].z;

			for (i = 0; i < 3; i++)
			{
				p.x = x + RandomFloat2() * 20.0f;
				p.y = y + RandomFloat2() * 20.0f;
				p.z = z + RandomFloat2() * 20.0f;

				d.x = RandomFloat2() * 100.0f;
				d.y = RandomFloat2() * 100.0f;
				d.z = RandomFloat2() * 100.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= 1.0;
				newParticleDef.rotZ			= RandomFloat()*PI2;
				newParticleDef.rotDZ		= RandomFloat()*PI2;
				newParticleDef.alpha		= .6;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}


		}
	}
}


/******************** DISABLE ROCKET SLED *****************************/

static void DisableRocketSled(ObjNode *theNode)
{
ObjNode	*player = gPlayerInfo.objNode;

	gPlayerRocketSled = nil;

	SetSkeletonAnim(player->Skeleton, PLAYER_ANIM_BELLYSLIDE);
	gPlayerInfo.knockDownTimer = 1.0f;

	player->Delta = gDelta;
	player->Coord.y = GetTerrainY(player->Coord.x, player->Coord.z) - player->BBox.min.y;

	gCurrentMaxSpeed = theNode->Speed2D * .6f;
	gTimeSinceLastThrust = 0;


			/* BLOW UP THE SLED ON IMPACT */

	if (gLevelNum == LEVEL_NUM_CLOUD)
		PlayEffect3D(EFFECT_BUMPERPOLEBREAK, &gCoord);
	else
		PlayEffect3D(EFFECT_SLEDEXPLODE, &gCoord);

	ExplodeGeometry(theNode, 400, SHARD_MODE_UPTHRUST, 1, .5);
	DeleteObject(theNode);

}



#pragma mark -

/******************* ALIGN PLAYER IN ROCKETSLED **********************/
//
// NOTE: player's coords are loaded into gCoord
//

void AlignPlayerInRocketSled(ObjNode *player)
{
OGLMatrix4x4	m,m2,m3;
float		scale;
static const OGLPoint3D zero = {0,0,0};

	if (gPlayerRocketSled == nil)
		DoFatalAlert("AlignPlayerInRocketSled: nil");


			/* CALC SCALE MATRIX */

	scale = PLAYER_DEFAULT_SCALE/ROCKETSLED_SCALE;							// to adjust from tobogan scale to player's
	OGLMatrix4x4_SetScale(&m2, scale, scale, scale);


			/* CALC TRANSLATE MATRIX */

	OGLMatrix4x4_SetTranslate(&m3, 0 ,132, 70);
	OGLMatrix4x4_Multiply(&m2, &m3, &m);


			/* GET ALIGNMENT MATRIX */

	OGLMatrix4x4_Multiply(&m, &gPlayerRocketSled->BaseTransformMatrix, &gPlayerInfo.objNode->BaseTransformMatrix);
	SetObjectTransformMatrix(gPlayerInfo.objNode);

	OGLPoint3D_Transform(&zero, &gPlayerInfo.objNode->BaseTransformMatrix, &gCoord);		// calc player coord
	player->Coord = gCoord;

	player->Rot.y = gPlayerRocketSled->Rot.y;
}


