/****************************/
/*   	PLAYER_WEAPONS.C    */
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

static void StartPunch(ObjNode *player);
static void ShootWeapon(ObjNode *theNode);
static void MoveStunPulse(ObjNode *theNode);
static void ShootStunPulse(ObjNode *theNode, OGLPoint3D *where, OGLVector3D *aim);
static void MoveStunPulseRipple(ObjNode *theNode);
static Boolean DoWeaponCollisionDetect(ObjNode *theNode);
static void	WeaponAutoTarget(OGLPoint3D *where, OGLVector3D *aim);
static Boolean SeeIfDoPickup(ObjNode *player);
static void MoveDisposedWeapon(ObjNode *theNode);
static void ChangeWeapons(int startIndex, int delta, bool tryStartSlotFirst);
static void StartSuperNovaCharge(ObjNode *player);
static void DrawSuperNovaCharge(ObjNode *theNode);
static void ShootFreezeGun(ObjNode *theNode, OGLPoint3D *where, OGLVector3D *aim);
static void MoveFreezeBullet(ObjNode *theNode);
static void ExplodeFreeze(ObjNode *bullet);
static void CalcAntennaDischargePoint(OGLPoint3D *p);

static void ShootFlameGun(ObjNode *player, OGLPoint3D *where, OGLVector3D *aim);
static void MoveFlameBullet(ObjNode *theNode);
static void ExplodeFlameBullet(ObjNode *bullet);

static void TossGrowthVial(void);
static void MoveTossedGrowthVial(ObjNode *vial);

static void ShootFlareGun(ObjNode *player, OGLPoint3D *where, OGLVector3D *aim);
static void MoveFlareBullet(ObjNode *theNode);
static void ExplodeFlareBullet(ObjNode *bullet);
static void MoveFlareBullet_Boom(ObjNode *theNode);

static void StartThrowDart(ObjNode *player);
static void MoveDart(ObjNode *theNode);
static void ExplodeDart(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_AUTO_AIM_DIST	5000.0f

#define	STUN_PULSE_SPEED	3500.0f

#define	FREEZE_BULLET_SPEED	1500.0f
#define FLAME_BULLET_SPEED	1700.0f
#define	FLARE_BULLET_SPEED	1000.0f
#define	DART_SPEED			900.0f

typedef struct
{
	Boolean		active;
	float		dist;
	OGLPoint3D	where;
	ObjNode		*who;
}SuperNovaTargetType;


/*********************/
/*    VARIABLES      */
/*********************/

const OGLPoint3D 	gPlayerMuzzleTipOff = {-1,-70,-22};		//  offset to muzzle tip
const OGLVector3D	gPlayerMuzzleTipAim = {0,-1,-.149};

static MOVertexArrayData	gNovaChargeMesh;

const OGLPoint3D antennaLOff = {-40,45,12};
const OGLPoint3D antennaROff = {40,45,12};

static SuperNovaTargetType	gSuperNovaTargets[MAX_SUPERNOVA_DISCHARGES];
float		gDischargeTimer;


#define	DelayToSeek		SpecialF[0]									// timer for seeking flares

static const Boolean gWeaponIsGun[NUM_WEAPON_TYPES] =
{
	true,			// WEAPON_TYPE_STUNPULSE
	true,			// WEAPON_TYPE_FREEZE
	true,			// WEAPON_TYPE_FLAME
	true,			// WEAPON_TYPE_GROWTH
	true,			// WEAPON_TYPE_FLARE

	false,			// WEAPON_TYPE_FIST
	false,			// WEAPON_TYPE_SUPERNOVA
	false,			// WEAPON_TYPE_DART
};

/*********************** INIT WEAPON INVENTORY *************************/

void InitWeaponInventory(void)
{
int	i;

	for (i = 0; i < MAX_INVENTORY_SLOTS; i++)							// init weapon inventory
	{
		gPlayerInfo.weaponInventory[i].type = NO_INVENTORY_HERE;
		gPlayerInfo.weaponInventory[i].quantity = 0;
	}

			/* ALWAYS HAS FIST AS WEAPON */

	gPlayerInfo.weaponInventory[0].type = WEAPON_TYPE_FIST;
	gPlayerInfo.weaponInventory[0].quantity = 99;
	gPlayerInfo.currentWeaponType = WEAPON_TYPE_FIST;
	gPlayerInfo.holdingGun = false;
}


#pragma mark -


/******************* CHECK WEAPON CHANGE CONTROLS ************************/

void CheckWeaponChangeControls(ObjNode* theNode)
{
	if (GetNewNeedState(kNeed_NextWeapon))
	{
		int i = FindWeaponInventoryIndex(gPlayerInfo.currentWeaponType);	// get current into inventory list
		ChangeWeapons(i, 1, false);
	}
	else if (GetNewNeedState(kNeed_PrevWeapon))
	{
		int i = FindWeaponInventoryIndex(gPlayerInfo.currentWeaponType);	// get current into inventory list
		ChangeWeapons(i, -1, false);
	}
}


/******************* CHECK POWERUP CONTROLS ************************/

void CheckPOWControls(ObjNode *theNode)
{
		/* SEE IF SHOOT GUN */

	if (GetNewNeedState(kNeed_Shoot))					// see if user pressed the key
	{
		ShootWeapon(theNode);
	}

		/* SEE IF PUNCH / PICKUP */

	else
	if (GetNewNeedState(kNeed_PunchPickup))
	{
		if (!SeeIfDoPickup(theNode))						// if not picking up then punching
		{
			StartPunch(theNode);
		}
	}

		/* SEE IF CHANGE WEAPON */

	else
	{
		CheckWeaponChangeControls(theNode);
	}
}


/******************* START PUNCH *******************/

static void StartPunch(ObjNode *player)
{
	player->PunchCanHurt = false;
	MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_PUNCH, 8);

	PlayEffect3D(EFFECT_SERVO, &gCoord);
}

/******************* START THROW DART *******************/

static void StartThrowDart(ObjNode *player)
{
	player->ThrowDartNow = false;
	MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_THROW, 8);

	PlayEffect3D(EFFECT_SERVO, &gCoord);
}



/******************** SHOOT WEAPON *************************/

static void ShootWeapon(ObjNode *theNode)
{
OGLPoint3D		muzzleCoord;
OGLVector3D		muzzleVector;
OGLMatrix4x4	m;

	gTimeSinceLastShoot = 0;
	gCameraUserRotY = 0;											// reset user rot see we can see where we're shooting
	gForceCameraAlignment = true;

		/* CALC COORD & VECTOR OF MUZZLE */

	if (gPlayerInfo.holdingGun)
	{
		FindJointFullMatrix(theNode,PLAYER_JOINT_LEFTHAND,&m);
		OGLPoint3D_Transform(&gPlayerMuzzleTipOff, &m, &muzzleCoord);
		OGLVector3D_Transform(&gPlayerMuzzleTipAim, &m, &muzzleVector);
	}

			/* SHOOT APPROPRIATE WEAPON */

	switch(gPlayerInfo.currentWeaponType)
	{
		case	WEAPON_TYPE_STUNPULSE:
				ShootStunPulse(theNode, &muzzleCoord, &muzzleVector);
				DecWeaponQuantity(WEAPON_TYPE_STUNPULSE);
				break;

		case	WEAPON_TYPE_SUPERNOVA:
				if (theNode->StatusBits & STATUS_BIT_ONGROUND)						// only allow charge if on ground
					StartSuperNovaCharge(theNode);
				break;

		case	WEAPON_TYPE_FREEZE:
				ShootFreezeGun(theNode, &muzzleCoord, &muzzleVector);
				DecWeaponQuantity(WEAPON_TYPE_FREEZE);
				break;

		case	WEAPON_TYPE_FLAME:
				ShootFlameGun(theNode, &muzzleCoord, &muzzleVector);
				DecWeaponQuantity(WEAPON_TYPE_FLAME);
				break;

		case	WEAPON_TYPE_FIST:
				StartPunch(theNode);
				break;

		case	WEAPON_TYPE_GROWTH:
				MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_DRINK, 4);
				break;

		case	WEAPON_TYPE_FLARE:
				ShootFlareGun(theNode, &muzzleCoord, &muzzleVector);
				DecWeaponQuantity(WEAPON_TYPE_FLARE);
				break;

		case	WEAPON_TYPE_DART:
				StartThrowDart(theNode);
				break;

	}
}




#pragma mark -


/******************* FIND WEAPON INVENTORY INDEX **************************/

short FindWeaponInventoryIndex(short weaponType)
{
short	i;

	for (i = 0; i < MAX_INVENTORY_SLOTS; i++)
	{
		if (gPlayerInfo.weaponInventory[i].type == weaponType)
			if (gPlayerInfo.weaponInventory[i].quantity > 0)
				return(i);
	}

	return(NO_INVENTORY_HERE);
}



/******************* DECREMENT WEAPON QUANTITY ***********************/

void DecWeaponQuantity(short weaponType)
{
ObjNode	*newObj;
short	i,type;
static const short weaponToModel[] =
{
	GLOBAL_ObjType_StunPulseGun,			// WEAPON_TYPE_STUNPULSE
	GLOBAL_ObjType_FreezeGun,				// WEAPON_TYPE_FREEZE
	GLOBAL_ObjType_FlameGun,				// WEAPON_TYPE_FLAME
	-1,										// WEAPON_TYPE_GROWTH
	GLOBAL_ObjType_FlareGun,				// WEAPON_TYPE_FLARE
};

	i = FindWeaponInventoryIndex(weaponType);					// get index into inventory list
	if (i == NO_INVENTORY_HERE)
		DoFatalAlert("DecWeaponQuantity: weapon not in inventory");

	type = gPlayerInfo.currentWeaponType;						// get current weapon type

	gPlayerInfo.weaponInventory[i].quantity--;					// decrement the inventory


			/****************/
			/* TOSS THE GUN */
			/****************/

	if (gPlayerInfo.weaponInventory[i].quantity <= 0)			// see if need to dispose of it
	{
		gPlayerInfo.weaponInventory[i].quantity = 0;
		gPlayerInfo.weaponInventory[i].type = NO_INVENTORY_HERE;

		if (gPlayerInfo.holdingGun)
		{
			gPlayerInfo.wasHoldingGun = true;

					/* MAKE IT FLY AWAY */

			if (type == WEAPON_TYPE_GROWTH)			// special case the growth powerup
				TossGrowthVial();
			else
			{
				gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
				gNewObjectDefinition.type 		= weaponToModel[type];
				gNewObjectDefinition.coord		= gPlayerInfo.leftHandObj->Coord;
				gNewObjectDefinition.flags 		= 0;
				gNewObjectDefinition.slot 		= SLOT_OF_DUMB+1;
				gNewObjectDefinition.moveCall 	= MoveDisposedWeapon;
				gNewObjectDefinition.rot 		= gPlayerInfo.objNode->Rot.y;
				gNewObjectDefinition.scale 		= gPlayerInfo.leftHandObj->Scale.x;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->Rot.x = -PI/2;
				newObj->Delta.y = 600.0f;
				newObj->Delta.x = RandomFloat2() * 200.0f;
				newObj->Delta.z = RandomFloat2() * 200.0f;
			}

			gPlayerInfo.holdingGun = false;
		}
		else
			gPlayerInfo.wasHoldingGun = false;

			/* CHANGE TO NEXT WEAPON IF ANY */

		gPlayerInfo.currentWeaponType = NO_INVENTORY_HERE;			// clear this so change will not try to do any funky anims
		ChangeWeapons(i, 1, true);

		if ((gPlayerInfo.currentWeaponType == WEAPON_TYPE_SUPERNOVA) &&		// dont allow automatic switch to supernova since that screws up the player
			(gPlayerInfo.wasHoldingGun))
		{
			gPlayerInfo.currentWeaponType = WEAPON_TYPE_FIST;				// just go to punching in these cases
		}
	}
}

/*********************** TOSS GROWTH VIAL ************************/

static void TossGrowthVial(void)
{
float	r;
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= JUNGLE_ObjType_GrowthPOW;
	gNewObjectDefinition.coord		= gPlayerInfo.leftHandObj->Coord;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+1;
	gNewObjectDefinition.moveCall 	= MoveTossedGrowthVial;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .25;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	r = gPlayerInfo.objNode->Rot.y;
	newObj->Delta.y = 600.0f;
	newObj->Delta.x = -sin(r) * 600.0f;
	newObj->Delta.z = -cos(r) * 600.0f;

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
}


/***************** MOVE TOSSED GROWTH VIAL *********************/

static void MoveTossedGrowthVial(ObjNode *vial)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(vial);

	gDelta.y -= gGravity * .5f * fps;
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	if (HandleCollisions(vial, CTYPE_MISC | CTYPE_TERRAIN | CTYPE_FENCE, 0))
	{
		ExplodeGeometry(vial, 200, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .9);
		PlayEffect3D(EFFECT_SHATTER, &vial->Coord);
		DeleteObject(vial);
		return;
	}

	vial->Rot.x += PI2 * fps;
	vial->Rot.z += 9.0f * fps;

	UpdateObject(vial);
}


/******************* INCREMENT WEAPON QUANTITY ***********************/

void IncWeaponQuantity(short weaponType, short amount)
{
short	i;


			/* FIND EXISTING WEAPON IN INVENTORY */

	i = FindWeaponInventoryIndex(weaponType);					// get index into inventory list


			/* ADD NEW WEAPON TO INVENTORY */

	if (i == NO_INVENTORY_HERE)
	{
		for (i = 0; i < MAX_INVENTORY_SLOTS; i++)
		{
			if (gPlayerInfo.weaponInventory[i].type == NO_INVENTORY_HERE)		// see if found a blank slot
			{
				gPlayerInfo.weaponInventory[i].type = weaponType;
				gPlayerInfo.weaponInventory[i].quantity = amount;
				return;
			}
		}
	}

	gPlayerInfo.weaponInventory[i].quantity += amount;					// inc current inventory
	if (gPlayerInfo.weaponInventory[i].quantity > 99)					// max @ 99
		gPlayerInfo.weaponInventory[i].quantity = 99;
}



/****************** MOVED DISPOSED WEAPON *******************/
//
// When player runs out of ammo and the gun gets thrown.
//


static void MoveDisposedWeapon(ObjNode *theNode)
{
float fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	theNode->Rot.x += fps * 3.0f;

	gDelta.y -= 2000.0f * fps;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	if (gCoord.y <= GetTerrainY(gCoord.x, gCoord.z))
	{
		DeleteObject(theNode);
		return;
	}

	UpdateObject(theNode);
}


/************************* CHANGE WEAPONS *************************/
//
// INPUT:	i = starting index in inventory to scan
//

static void ChangeWeapons(int startIndex, int delta, bool tryStartSlotFirst)
{
int		i,newWeapon;
Boolean	wasHoldingGun = gPlayerInfo.holdingGun;

	if (tryStartSlotFirst)
	{
		i = startIndex;
	}
	else
	{
		i = PositiveModulo(startIndex + delta, MAX_INVENTORY_SLOTS);
	}

			/* SCAN FOR NEXT WEAPON */

	do
	{
		if (gPlayerInfo.weaponInventory[i].quantity > 0)			// do we have any of this stuff?
		{
				/* THIS IS THE ONE */

			if (gPlayerInfo.weaponInventory[i].type	== gPlayerInfo.currentWeaponType)		// see if must have looped back & selected the same weapon
				return;

			newWeapon = gPlayerInfo.weaponInventory[i].type;								// get new seleted weapon type

					/* DO WEAPON CHANGE ANIM */

			if (gPlayerInfo.objNode->Skeleton->AnimNum == PLAYER_ANIM_CHANGEWEAPON)
			{
				GAME_ASSERT_MESSAGE(!gPlayerInfo.objNode->ChangeWeapon,
									"shouldn't allow fast weapon switching once anim raises ChangeWeapon flag");
			}
			else
			if (wasHoldingGun || gWeaponIsGun[newWeapon])									// do change anim if either weapon is a gun
			{
				gPlayerInfo.wasHoldingGun = wasHoldingGun;										// remember if was holding gun during the anim
				gPlayerInfo.oldWeaponType = gPlayerInfo.currentWeaponType;						// remember old during the anim
				MorphToSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_CHANGEWEAPON, 8);
				gPlayerInfo.objNode->ChangeWeapon = false;
				PlayEffect_Parms3D(EFFECT_WEAPONWHIR, &gCoord, NORMAL_CHANNEL_RATE/3, 1.0);
			}

			gPlayerInfo.currentWeaponType = newWeapon;											// set new weapon type
			gPlayerInfo.holdingGun = gWeaponIsGun[newWeapon];									// set gun flag

			PlayEffect_Parms(EFFECT_CHANGEWEAPON, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME/5, NORMAL_CHANNEL_RATE);
			return;
		}

		i = PositiveModulo(i + delta, MAX_INVENTORY_SLOTS);

	}while(i != startIndex);
}


#pragma mark -

/***************** SHOOT STUN PULSE ************************/

static void ShootStunPulse(ObjNode *theNode, OGLPoint3D *where, OGLVector3D *aim)
{
ObjNode	*newObj;
int				i;

	PlayEffect3D(EFFECT_STUNGUN, where);
	Rumble(0.2f, 150);

		/*********************/
		/* MAKE MUZZLE FLASH */
		/*********************/

	if (theNode->Sparkles[PLAYER_SPARKLE_MUZZLEFLASH] != -1)							// see if delete existing sparkle
	{
		DeleteSparkle(theNode->Sparkles[PLAYER_SPARKLE_MUZZLEFLASH]);
		theNode->Sparkles[PLAYER_SPARKLE_MUZZLEFLASH] = -1;
	}

	i = theNode->Sparkles[PLAYER_SPARKLE_MUZZLEFLASH] = GetFreeSparkle(theNode);		// make new sparkle
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
		gSparkles[i].where = *where;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 100.0f;
		gSparkles[i].separation = 10.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_BlueSpark;
	}



			/* ADJUST WEAPON AIM TO AUTO-TARGET NEAREST ENEMY */

	WeaponAutoTarget(where, aim);


				/************************/
				/* CREATE WEAPON OBJECT */
				/************************/

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_StunPulseBullet;
	gNewObjectDefinition.coord		= *where;
	gNewObjectDefinition.flags 		= STATUS_BIT_USEALIGNMENTMATRIX|STATUS_BIT_GLOW|STATUS_BIT_NOZWRITES|
									STATUS_BIT_NOFOG|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-9;
	gNewObjectDefinition.moveCall 	= MoveStunPulse;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


	newObj->Kind = WEAPON_TYPE_STUNPULSE;

	newObj->ColorFilter.a = .99;			// do this just to turn on transparency so it'll glow

	newObj->Delta.x = aim->x * STUN_PULSE_SPEED;
	newObj->Delta.y = aim->y * STUN_PULSE_SPEED;
	newObj->Delta.z = aim->z * STUN_PULSE_SPEED;

	newObj->Health = 2.0f;
	newObj->Damage = .5f;


				/* SET COLLISION */

	newObj->CType 			= CTYPE_WEAPON;
	newObj->CBits			= CBITS_TOUCHABLE;
	CreateCollisionBoxFromBoundingBox_Maximized(newObj);


			/* SET THE ALIGNMENT MATRIX */

	SetAlignmentMatrix(&newObj->AlignmentMatrix, aim);
}


/******************* MOVE STUN PULSE ***********************/

static void MoveStunPulse(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* SEE IF GONE */

	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}


	GetObjectInfo(theNode);

			/* MOVE IT */

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


		/***********************/
		/* SEE IF HIT ANYTHING */
		/***********************/

	if (DoWeaponCollisionDetect(theNode))
		return;

	UpdateObject(theNode);
}



/********************* EXPLODE STUN PULSE ***********************/

void ExplodeStunPulse(ObjNode *theNode)
{
ObjNode	*newObj;
long					pg,i;
OGLVector3D				delta;
NewParticleDefType		newParticleDef;
float					s = theNode->Scale.x;


			/***************************/
			/* CREATE DISTORTION RINGS */
			/***************************/

			/* CREATE AN EVENT OBJECT */

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord		= gCoord;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveStunPulseRipple;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->Scale.x = s;

			/* CREATE BLUE GLINT */

	i = newObj->Sparkles[0] = GetFreeSparkle(newObj);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
		gSparkles[i].where = gCoord;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1.0;

		gSparkles[i].scale = 200.0f * s;
		gSparkles[i].separation = 200.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_BlueGlint;
	}


			/* CREATE PURPLE RING */

	i = newObj->Sparkles[1] = GetFreeSparkle(newObj);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
		gSparkles[i].where = gCoord;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 2.0;

		gSparkles[i].scale = 5.0f * s;
		gSparkles[i].separation = 200.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_PurpleRing;
	}


			/**************/
			/*  PARTICLES */
			/**************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
	gNewParticleGroupDef.gravity				= 0;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 10.0f * s;
	gNewParticleGroupDef.decayRate				= 3.0;
	gNewParticleGroupDef.fadeRate				= 0;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_WhiteSpark4;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < 30; i++)
		{
			delta.x = RandomFloat2() * (200.0f * s);
			delta.y = RandomFloat2() * (200.0f * s);
			delta.z = RandomFloat2() * (200.0f * s);


			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &gCoord;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= RandomFloat() + 1.0f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA;
			if (AddParticleToGroup(&newParticleDef))
				break;
		}
	}

	PlayEffect3D(EFFECT_LASERHIT, &gCoord);

	DeleteObject(theNode);									// delete the pulse

}


/****************** MOVE STUN PULSE RIPPLE ************************/

static void MoveStunPulseRipple(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		i,n = 0;
float	s = theNode->Scale.x;

			/* UPDATE BLUE GLINT */

	i = theNode->Sparkles[0];
	if (i != -1)
	{
		n++;
		gSparkles[i].scale -= fps * 500.0f * s;
		if (gSparkles[i].scale <= 0.0f)
		{
			DeleteSparkle(i);
			theNode->Sparkles[0] = -1;
		}
	}

			/* UPDATE PURPLE RING */

	i = theNode->Sparkles[1];
	if (i != -1)
	{
		n++;
		gSparkles[i].scale += fps * 650.0f * s;
		gSparkles[i].color.a -= fps * 12.0f;
		if (gSparkles[i].color.a <= 0.0f)
		{
			DeleteSparkle(i);
			theNode->Sparkles[1] = -1;
		}
	}


	if (n == 0)								// if no sparkles remaining then delete the object
		DeleteObject(theNode);
}


#pragma mark -


/*********************** WEAPON AUTO-TARGET ENEMY *******************************/
//
// Slightly adjusts the aim vector of the weapon to aim at the closest auto-target that is in range.
//

static void	WeaponAutoTarget(OGLPoint3D *where, OGLVector3D *aim)
{
ObjNode*	thisNode = nil;
ObjNode*	nearestEnemy = nil;
OGLVector3D	vecToEnemy;
OGLVector3D	bestAngle = {0,0,0};
float		dist,dot,bestDist,angle,angleRange;
float		x,y,z, ex,ey,ez;

	x = where->x;														// get muzzle coords
	y = where->y;
	z = where->z;

	nearestEnemy = nil;
	bestDist = 100000000;

	thisNode = gFirstNodePtr;											// start on 1st node

	do
	{
		if (thisNode->CType & CTYPE_AUTOTARGETWEAPON)					// only look for auto targetable items
		{
			ex = thisNode->Coord.x + thisNode->WeaponAutoTargetOff.x;	// get target coords w/ offset
			ey = thisNode->Coord.y + thisNode->WeaponAutoTargetOff.y;
			ez = thisNode->Coord.z + thisNode->WeaponAutoTargetOff.z;

			vecToEnemy.x = ex - x;										// calc vector to target
			vecToEnemy.y = ey - y;
			vecToEnemy.z = ez - z;
			FastNormalizeVector(vecToEnemy.x, vecToEnemy.y, vecToEnemy.z, &vecToEnemy);

			dist = CalcDistance(x, z, ex, ez);							// calc distance
			if (dist < MAX_AUTO_AIM_DIST)								// only if in range
			{
				dot = OGLVector3D_Dot(&vecToEnemy, aim);				// calc angle
				angle = acos(dot);

				angleRange = (PI/2.0f) - ((PI/2.5f) * (dist * (1.0f/MAX_AUTO_AIM_DIST))); 	// calc the acceptable angle based on dist:  closer == more tolerant

				if (angle < angleRange)									// see if it's within angular range
				{
					if (dist < bestDist)
					{
						bestDist = dist;
						nearestEnemy = thisNode;
						bestAngle = vecToEnemy;
					}
				}
			}
		}

		thisNode = thisNode->NextNode;									// next target node
	}while(thisNode != nil);


			/* IF FOUND SOMETHING, THEN ADJUST AIM TO IT */

	if (nearestEnemy)
	{
		aim->x += bestAngle.x * 4.0f;									// average the two, but weighted on the bestAngle aim
		aim->y += bestAngle.y * 4.0f;
		aim->z += bestAngle.z * 4.0f;
		FastNormalizeVector(aim->x, aim->y, aim->z, aim);

			/* ALSO TURN PLAYER TOWARD IT */

		gPlayerInfo.autoAimTimer = .8;									// start auto-aiming
		gPlayerInfo.autoAimTargetX = nearestEnemy->Coord.x;
		gPlayerInfo.autoAimTargetZ = nearestEnemy->Coord.z;
	}
}



/**************** DO WEAPON COLLISION DETECT *************************/
//
// returns true if weapon is destroyed
//

static Boolean DoWeaponCollisionDetect(ObjNode *theNode)
{
short	weaponType = theNode->Kind;
int		i;
ObjNode	*hitObj;
CollisionBoxType	*baseBoxList = theNode->CollisionBoxes;

			/* SEE IF HIT GROUND */

	if (gCoord.y <= GetTerrainY(gCoord.x, gCoord.z))					// see if hit ground
		goto explode_weapon;


			/*****************************/
			/* SEE IF HIT ANYTHING SOLID */
			/*****************************/

	DoSimpleBoxCollision(baseBoxList->top, baseBoxList->bottom, baseBoxList->left, baseBoxList->right,
						baseBoxList->front,baseBoxList->back,CTYPE_MISC | CTYPE_ENEMY | CTYPE_AUTOTARGETWEAPON);
	if (gNumCollisions > 0)
	{
				/* CALL OBJECT'S HANDLER FOR THE HIT */

		for (i = 0; i < gNumCollisions; i++)								// affect all hit objects
		{
			hitObj = gCollisionList[i].objectPtr;							// get hit ObjNode
			if (hitObj)
			{
				if (hitObj->HitByWeaponHandler[weaponType])					// see if there is a handler for this weapon
				{
					if ((hitObj->HitByWeaponHandler[weaponType])(theNode, hitObj, &gCoord, &gDelta))	// call the handler
						goto explode_weapon;
					else
						return(false);										// dont blow up bullet after collision handler
				}
			}
		}


				/* DEAL WITH THE WEAPON */
explode_weapon:
		switch(theNode->Kind)
		{
			case	WEAPON_TYPE_STUNPULSE:
					ExplodeStunPulse(theNode);
					break;

			case	WEAPON_TYPE_FREEZE:
					ExplodeFreeze(theNode);
					break;

			case	WEAPON_TYPE_FLAME:
					ExplodeFlameBullet(theNode);
					break;

			case	WEAPON_TYPE_FLARE:
					ExplodeFlareBullet(theNode);
					break;

			case	WEAPON_TYPE_DART:
					ExplodeDart(theNode);
					break;

			default:
					DeleteObject(theNode);
		}
		return(true);
	}

	return(false);
}


/********************** SEE IF DO PICKUP *****************************/
//
// Checks if there is a pickup item in front of the player.
//

static Boolean SeeIfDoPickup(ObjNode *player)
{
ObjNode *thisNode,*nearest;
float	ex,ey,ez,dist,bestDist;
OGLVector2D	aim;
short		anim;

	aim.x = -sin(player->Rot.y);								// calc player aim vector
	aim.y = -cos(player->Rot.y);

	bestDist = 10000000;
	nearest = nil;


			/* SCAN FOR NEAREST PICKUP IN FRONT */

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (!(thisNode->CType & CTYPE_POWERUP))					// only look for powerups...
			goto next;
		if (thisNode->StatusBits & STATUS_BIT_HIDDEN)			// ... that are visible
			goto next;

		ex = thisNode->Coord.x;									// get coords
		ey = thisNode->Coord.y;
		ez = thisNode->Coord.z;

		dist = CalcDistance(gCoord.x, gCoord.z, ex, ez);
		if ((dist < bestDist) && (dist < (170.0f * gPlayerInfo.scaleRatio)))			// see if best dist & close enough
		{
			bestDist = dist;
			nearest = thisNode;
		}

next:
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);


			/*****************************************/
			/* THERE IS SOMETHING THERE SO DO PICKUP */
			/*****************************************/

	if (nearest)
	{
		gTargetPickup = nearest;

		DisableHelpType(HELP_MESSAGE_PICKUPPOW);							// disable this help message since the player can now do it

		switch(gTargetPickup->POWType)
		{
				/* DEPOSIT TYPES */

			case	POW_TYPE_HEALTH:
			case	POW_TYPE_JUMPJET:
			case	POW_TYPE_FUEL:
			case	POW_TYPE_SUPERNOVA:
			case	POW_TYPE_DART:
			case	POW_TYPE_FREELIFE:
					anim = PLAYER_ANIM_PICKUPDEPOSIT;
					break;


				/* SPECIAL CASE STUFF */

			case	POW_TYPE_MAGNET:
					anim = PLAYER_ANIM_PICKUPANDHOLDMAGNET;
					break;


				/* HOLD TYPES */

			default:
					if (gPlayerInfo.holdingGun)									// if already holding then deposit it
						anim = PLAYER_ANIM_PICKUPDEPOSIT;
					else
						anim = PLAYER_ANIM_PICKUPANDHOLDGUN;
		}

				/* SET THE NEEDED ANIM */

		MorphToSkeletonAnim(player->Skeleton, anim, 8);
		PlayEffect_Parms3D(EFFECT_WEAPONWHIR, &gCoord, NORMAL_CHANNEL_RATE / 3, 1.0);
		player->IsHoldingPickup = false;										// not holding anything yet

		return(true);
	}

	return(false);
}


#pragma mark -

/****************** START SUPERNOVA CHARGE **************************/

static void StartSuperNovaCharge(ObjNode *player)
{
ObjNode	*newObj;
int		i,j;

	MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_CHARGING, 4);

	gPlayerInfo.superNovaCharge = .1f;


		/* CREATE CHARGE OBJECT */

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.coord 		= gCoord;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOZWRITES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG|STATUS_BIT_NOTEXTUREWRAP|
										STATUS_BIT_GLOW|STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.slot 		= SPRITE_SLOT-1;
	gNewObjectDefinition.moveCall 	= nil;

	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->CustomDrawFunction = DrawSuperNovaCharge;

	gPlayerInfo.superNovaStatic = newObj;


			/* CREATE ANTENNA GLOW */

	for (j = 0; j < 2; j++)
	{
		i = newObj->Sparkles[j] = GetFreeSparkle(newObj);				// get free sparkle slot
		if (i != -1)
		{
			gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
			gSparkles[i].where = newObj->Coord;

			gSparkles[i].color.r = 1;
			gSparkles[i].color.g = 1;
			gSparkles[i].color.b = 1;
			gSparkles[i].color.a = .99;

			gSparkles[i].scale = 80.0f;
			gSparkles[i].separation = 30.0f;

			gSparkles[i].textureNum = PARTICLE_SObjType_BlueGlint;
		}
	}
}


/******************** DISCHARGE SUPERNOVA *********************/
//
// returns true if aborted
//

Boolean DischargeSuperNova(void)
{
ObjNode	*novaObj;
float	maxDist;
int		i,maxDischarges,j;
OGLPoint3D	base;


		/* GET RID OF STATIC */

	novaObj = gPlayerInfo.superNovaStatic;
	if (novaObj)
	{
		gPlayerInfo.superNovaStatic = nil;

		for (j = 0; j < 2; j++)							// delete the sparkle objects
		{
			i = novaObj->Sparkles[j];
			if (i != -1)
			{
				DeleteSparkle(i);
				novaObj->Sparkles[j] = -1;
			}
		}

		/* CHANGE STATIC EVENT OBJ INTO DISCHARGE OBJ */

		novaObj->CustomDrawFunction = DrawSuperNovaDischarge;

		StopAChannel(&novaObj->EffectChannel);				// stop chargup effect
	}

			/*******************************/
			/* SEE WHAT WE'RE GOING TO ZAP */
			/*******************************/

	if (gPlayerInfo.superNovaCharge < .4f)			// see if not enough charge to bother
	{
		if (novaObj)								// get rid of the discharge obj
			DeleteObject(novaObj);

		SetPlayerStandAnim(gPlayerInfo.objNode, 5);
		return(true);
	}

		/* DETERMINE HOW MANY ZAPS WE CAN DO AND THE RANGE */

	maxDischarges = gPlayerInfo.superNovaCharge * MAX_SUPERNOVA_DISCHARGES;
	maxDist = gPlayerInfo.superNovaCharge * MAX_SUPERNOVA_DIST;

	CalcAntennaDischargePoint(&base);
	if (CreateSuperNovaStatic(novaObj, base.x, base.y, base.z, maxDischarges, maxDist) == 0)
		return(true);

	DecWeaponQuantity(WEAPON_TYPE_SUPERNOVA);	// dec quantity

	PlayEffect3D(EFFECT_ZAP, &gCoord);


	return(false);
}


/****************** CREATE SUPERNOVA STATIC ***********************/
//
// This call simply creates the static without actually affecting the
// supernova inventory.
//

short CreateSuperNovaStatic(ObjNode *novaObj, float x, float y, float z, int maxDischarges, float maxDist)
{
int	i,j,n;
ObjNode *thisNode;
float	dist,worstDist;
int		count;
u_long	ctype;
				/* INIT THE LIST */

	for (i = 0; i < MAX_SUPERNOVA_DISCHARGES; i++)
	{
		gSuperNovaTargets[i].active = false;
		gSuperNovaTargets[i].dist = 100000000;
	}

				/****************************/
				/* FIND THE NEAREST TARGETS */
				/****************************/


	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		ctype = thisNode->CType;

		if (ctype & CTYPE_PLAYER)								// skip the player
			goto next;

		if (!(ctype& (CTYPE_MISC|CTYPE_ENEMY|CTYPE_POWERUP|CTYPE_TRIGGER|CTYPE_LIGHTNINGROD)))		// its gotta be one of these at least
			goto next;

//		if (thisNode->Slot >= SLOT_OF_DUMB)						// dont do dumb objects
//			break;

		dist = CalcQuickDistance(thisNode->Coord.x, thisNode->Coord.z, x, z);

		if (ctype & CTYPE_LIGHTNINGROD)						// give these high priority by decrementing their distance
			dist *= .25f;

		if (dist > maxDist)
			goto next;

		if (SeeIfLineSegmentHitsAnything(&thisNode->Coord, &gPlayerInfo.coord, nil, CTYPE_FENCE|CTYPE_BLOCKRAYS))	// don't zap thru fences or other solid things
			goto next;



			/* FIRST SEE IF THERE'S A FREE SLOT TO PUT IT IN */

		for (i = 0; i < maxDischarges; i++)
		{
			if (gSuperNovaTargets[i].active == false)
			{
				float	toY;
assign_me:
				toY = thisNode->Coord.y + thisNode->BBox.max.y;		// calc coord of top of target
				if ((toY - y) > 600.0f)								// if too high up, then shoot to middle
					toY = thisNode->Coord.y + (thisNode->BBox.max.y * thisNode->Scale.y + thisNode->BBox.min.y * thisNode->Scale.y) * .5f;

				gSuperNovaTargets[i].active = true;
				gSuperNovaTargets[i].dist = dist;
				gSuperNovaTargets[i].where = thisNode->Coord;
				gSuperNovaTargets[i].where.y = toY;
				gSuperNovaTargets[i].who = thisNode;
				goto next;
			}
		}

				/* NO FREE SLOTS SO ELIMINATE THE FARTHEST */

		worstDist = 10000000;
		j = 0;
		for (i = 0; i < maxDischarges; i++)						// scan for the farthest in the list
		{
			if (gSuperNovaTargets[i].dist < worstDist)
			{
				worstDist = gSuperNovaTargets[i].dist;
				j = i;
			}
		}

		if (dist < worstDist)									// see if we're closer than the worst thing in there
		{
			i = j;
			goto assign_me;										// replace that guy with this one
		}

next:
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);




			/***************************************************/
			/* CALL THE SUPERNOVA HANDLER FOR TARGETED OBJECTS */
			/***************************************************/

	count = 0;
	for (i = 0; i < maxDischarges; i++)						// scan for the farthest in the list
	{
		if (gSuperNovaTargets[i].active)
		{
			count++;														// also count the # discharges
			if (gSuperNovaTargets[i].who->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] != nil)
			{
				gSuperNovaTargets[i].who->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA](nil, gSuperNovaTargets[i].who, nil, nil);
			}
		}
	}

			/*******************/
			/* CREATE SPARKLES */
			/*******************/

	if (count)
	{
				/* CREATE BASE SPARKLE */

		i = novaObj->Sparkles[0] = GetFreeSparkle(novaObj);				// get free sparkle slot
		if (i != -1)
		{
			gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
			gSparkles[i].where.x = x;
			gSparkles[i].where.y = y;
			gSparkles[i].where.z = z;

			gSparkles[i].color.r = 1;
			gSparkles[i].color.g = 1;
			gSparkles[i].color.b = 1;
			gSparkles[i].color.a = .99;

			gSparkles[i].scale = 130.0f;
			gSparkles[i].separation = 100.0f;

			gSparkles[i].textureNum = PARTICLE_SObjType_BlueGlint;
		}

				/* CREATE SPARKELS FOR EACH ENDPOINT */

		n = 1;
		for (j = 0; j < maxDischarges; j++)
		{
			if (gSuperNovaTargets[j].active)
			{
				i = novaObj->Sparkles[n] = GetFreeSparkle(novaObj);				// get free sparkle slot
				if (i != -1)
				{
					n++;
					gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
					gSparkles[i].where = gSuperNovaTargets[j].where;

					gSparkles[i].color.r = 1;
					gSparkles[i].color.g = 1;
					gSparkles[i].color.b = 1;
					gSparkles[i].color.a = .99;

					gSparkles[i].scale = 300.0f;
					gSparkles[i].separation = 200.0f;

					gSparkles[i].textureNum = PARTICLE_SObjType_BlueGlint;
				}
			}
		}
	}




	gDischargeTimer = 2.0f;

	return(count);
}



/********************* CALC ANTENNA DISCHARGE POINT **********************/

static void CalcAntennaDischargePoint(OGLPoint3D *p)
{
OGLPoint3D	antennaL,antennaR;

			/* CALC COORD OF LEFT & RIGHT ANTENNAE */

	FindCoordOnJoint(gPlayerInfo.objNode, PLAYER_JOINT_HEAD, &antennaLOff, &antennaL);
	FindCoordOnJoint(gPlayerInfo.objNode, PLAYER_JOINT_HEAD, &antennaROff, &antennaR);

	p->x = (antennaL.x + antennaR.x) *.5f;				// calc base point for the discharge
	p->y = (antennaL.y + antennaR.y) *.5f;
	p->z = (antennaL.z + antennaR.z) *.5f;
}


/******************* DRAW SUPERNOVA CHARGE ******************/

static void DrawSuperNovaCharge(ObjNode *theNode)
{
OGLPoint3D	points[7*2],antennaL,antennaR,base;
float		dx,dy,dz,x,y,z,u;
int			i,j;
OGLTextureCoord	uvs[7*2];
static MOTriangleIndecies triangles[6*2] =
{
	{{ 0, 1, 2}},	{{ 2, 1, 3}},
	{{ 2, 3, 4}},	{{ 4, 3, 5}},
	{{ 4, 5, 6}},	{{ 6, 5, 7}},
	{{ 6, 7, 8}},	{{ 8, 7, 9}},
	{{ 8, 9,10}},	{{10, 9,11}},
	{{10,11,12}},	{{12,11,13}},
};

			/*******************************/
			/* BUILD STATIC SECTION POINTS */
			/*******************************/

			/* CALC COORD OF LEFT & RIGHT ANTENNAE */

	FindCoordOnJoint(gPlayerInfo.objNode, PLAYER_JOINT_HEAD, &antennaLOff, &antennaL);
	FindCoordOnJoint(gPlayerInfo.objNode, PLAYER_JOINT_HEAD, &antennaROff, &antennaR);

	base.x = (antennaL.x + antennaR.x) *.5f;							// calc base point for the discharge
	base.y = (antennaL.y + antennaR.y) *.5f;
	base.z = (antennaL.z + antennaR.z) *.5f;


			/* SUB-DIVIDE LINE BETWEEN ANTENNAE */

	dx = (antennaR.x - antennaL.x) / 6.0f;								// calc vector from L to R
	dy = (antennaR.y - antennaL.y) / 6.0f;
	dz = (antennaR.z - antennaL.z) / 6.0f;

	x = antennaL.x;
	y = antennaL.y;
	z = antennaL.z;

	u = 0;

	j = 0;
	for (i = 0; i < 7; i++)												// build the point list
	{
		float yoff;

		if ((i > 0) && (i != 6))
			yoff = RandomFloat2() * 8.0f;
		else
			yoff = 0;

		points[j].x = x;
		points[j].y = y + yoff + 4.0f;
		points[j].z = z;
		uvs[j].u = u;
		uvs[j].v = 1;
		j++;
		points[j].x = x;
		points[j].y = y + yoff -4.0f;
		points[j].z = z;
		uvs[j].u = u;
		uvs[j].v = 0;
		j++;

		x += dx;
		y += dy;
		z += dz;
		u += 1.0f / 6.0f;

	}


			/******************/
			/* BUILD THE MESH */
			/******************/

	gNovaChargeMesh.numMaterials 	= -1;
	gNovaChargeMesh.numPoints 		= 7*2;
	gNovaChargeMesh.numTriangles 	= 6*2;
	gNovaChargeMesh.points			= (OGLPoint3D *)points;
	gNovaChargeMesh.normals			= nil;
	gNovaChargeMesh.uvs[0]				= (OGLTextureCoord *)uvs;
	gNovaChargeMesh.colorsByte		= nil;
	gNovaChargeMesh.colorsFloat		= nil;
	gNovaChargeMesh.triangles		= triangles;


			/***********/
			/* DRAW IT */
			/***********/

	gGlobalTransparency = gPlayerInfo.superNovaCharge;
	MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_NovaCharge].materialObject);
	MO_DrawGeometry_VertexArray(&gNovaChargeMesh);
	gGlobalTransparency = 1.0f;


			/* ALSO REALIGN SPARKLES */

	if ((i = theNode->Sparkles[0]) != -1)
	{
		gSparkles[i].where = antennaL;
		gSparkles[i].scale = 30.0f + (gPlayerInfo.superNovaCharge*80.0f) + RandomFloat() * 20.0f;
	}
	if ((i = theNode->Sparkles[1]) != -1)
	{
		gSparkles[i].where = antennaR;
		gSparkles[i].scale = 30.0f + (gPlayerInfo.superNovaCharge*80.0f) + RandomFloat() * 20.0f;
	}

		/* AND INCREASE CHARGE */

	gPlayerInfo.superNovaCharge += gFramesPerSecondFrac * .33f;
	if (gPlayerInfo.superNovaCharge > 1.0f)
		gPlayerInfo.superNovaCharge = 1.0f;


			/* UPDATE AUDIO */

	if (theNode->EffectChannel == -1)
		theNode->EffectChannel = PlayEffect(EFFECT_NOVACHARGE);
	else
		ChangeChannelRate(theNode->EffectChannel, NORMAL_CHANNEL_RATE * (.8f + gPlayerInfo.superNovaCharge));

}


/******************* DRAW SUPERNOVA DISCHARGE ******************/

void DrawSuperNovaDischarge(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
OGLPoint3D	targetCoord,points[10*2],base;
float		dx,dy,dz,u;
int			i,j,n;
OGLTextureCoord	uvs[10*2];
static MOTriangleIndecies triangles[9*2] =
{
	{{ 0, 1, 2}},	{{ 2, 1, 3}},
	{{ 2, 3, 4}},	{{ 4, 3, 5}},
	{{ 4, 5, 6}},	{{ 6, 5, 7}},
	{{ 6, 7, 8}},	{{ 8, 7, 9}},
	{{ 8, 9,10}},	{{10, 9,11}},
	{{10,11,12}},	{{12,11,13}},
	{{12,13,14}},	{{14,13,15}},
	{{14,15,16}},	{{16,15,17}},
	{{16,17,18}},	{{18,17,19}},
};


	for (n = 0; n < MAX_SUPERNOVA_DISCHARGES; n++)
	{
		if (!gSuperNovaTargets[n].active)								// skip inactive ones
			continue;

		targetCoord = gSuperNovaTargets[n].where;						// get coord of target


				/* CALC BASE POINT */

		CalcAntennaDischargePoint(&base);


				/* UPDATE BASE SPARKLE */

		i = theNode->Sparkles[0];
		if (i != -1)
			gSparkles[i].where = base;


			/*******************************/
			/* BUILD STATIC SECTION POINTS */
			/*******************************/


			/* SUB-DIVIDE LINE BETWEEN ENDPOINTS */

		dx = (targetCoord.x - base.x) / 9.0f;								// calc vector from L to R
		dy = (targetCoord.y - base.y) / 9.0f;
		dz = (targetCoord.z - base.z) / 9.0f;


		u = 0;

		j = 0;
		for (i = 0; i < 10; i++)												// build the point list
		{
			float yoff;

			if (i > 0)
				yoff = RandomFloat2() * 20.0f;
			else
				yoff = 0;

			points[j].x = base.x;
			points[j].y = base.y + yoff + 6.0f;
			points[j].z = base.z;
			uvs[j].u = u;
			uvs[j].v = 1;
			j++;
			points[j].x = base.x;
			points[j].y = base.y + yoff -6.0f;
			points[j].z = base.z;
			uvs[j].u = u;
			uvs[j].v = 0;
			j++;

			base.x += dx;
			base.y += dy;
			base.z += dz;
			u += 1.0f / 9.0f;

		}


				/******************/
				/* BUILD THE MESH */
				/******************/

		gNovaChargeMesh.numMaterials 	= -1;
		gNovaChargeMesh.numPoints 		= 10*2;
		gNovaChargeMesh.numTriangles 	= 9*2;
		gNovaChargeMesh.points			= (OGLPoint3D *)points;
		gNovaChargeMesh.normals			= nil;
		gNovaChargeMesh.uvs[0]			= (OGLTextureCoord *)uvs;
		gNovaChargeMesh.colorsByte		= nil;
		gNovaChargeMesh.colorsFloat		= nil;
		gNovaChargeMesh.triangles		= triangles;


				/***********/
				/* DRAW IT */
				/***********/

		gGlobalTransparency = gPlayerInfo.superNovaCharge;
		MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_NovaCharge].materialObject);
		MO_DrawGeometry_VertexArray(&gNovaChargeMesh);
		gGlobalTransparency = 1.0f;
	}

			/********************/
			/* FLICKER SPARKLES */
			/********************/

	for (j = 0; j < MAX_NODE_SPARKLES; j++)
	{
		if ((i = theNode->Sparkles[j]) != -1)
		{
			if (j == 0)
				gSparkles[i].scale = 100.0f + RandomFloat() * 50.0f;
			else
				gSparkles[i].scale = 250.0f + RandomFloat() * 50.0f;
		}
	}


			/* SEE IF DONE */

	if (!gGamePaused)
	{
		gPlayerInfo.superNovaCharge -= fps * .2f;						// fade out	just a little
		gDischargeTimer -= fps;
	}

	if ((gDischargeTimer <= 0.0f) || (gPlayerInfo.superNovaCharge <= 0.0f))
	{
		DeleteObject(theNode);

		if (gPlayerInfo.objNode->Skeleton->AnimNum == PLAYER_ANIM_CHARGING)		// change player anim
		{
			SetPlayerStandAnim(gPlayerInfo.objNode, 4.0);
		}
	}
}


#pragma mark -

/***************** SHOOT FREEZE GUN ************************/

static void ShootFreezeGun(ObjNode *theNode, OGLPoint3D *where, OGLVector3D *aim)
{
ObjNode *newObj;

#pragma unused (theNode)

			/* ADJUST WEAPON AIM TO AUTO-TARGET NEAREST ENEMY */

	WeaponAutoTarget(where, aim);


				/* CREATE BULLET OBJECT */

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_FreezeBullet;
	gNewObjectDefinition.coord		= *where;
	gNewObjectDefinition.flags 		= STATUS_BIT_AIMATCAMERA|STATUS_BIT_GLOW|STATUS_BIT_NOZWRITES|
									STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-6;
	gNewObjectDefinition.moveCall 	= MoveFreezeBullet;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .3;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


	newObj->Kind = WEAPON_TYPE_FREEZE;

	newObj->ColorFilter.a = .99;			// do this just to turn on transparency so it'll glow

	newObj->Delta.x = aim->x * FREEZE_BULLET_SPEED;
	newObj->Delta.y = aim->y * FREEZE_BULLET_SPEED;
	newObj->Delta.z = aim->z * FREEZE_BULLET_SPEED;

	newObj->Health = 2.0f;
	newObj->Damage = 0;

	newObj->ParticleTimer = 0;

	newObj->CType 			= CTYPE_WEAPON;
	newObj->CBits			= CBITS_TOUCHABLE;
	CreateCollisionBoxFromBoundingBox_Maximized(newObj);

	PlayEffect_Parms3D(EFFECT_FREEZEGUN, where, NORMAL_CHANNEL_RATE / 2, 1.0);

}


/******************* MOVE FREEZE BULLET ***********************/

static void MoveFreezeBullet(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum,i;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;

			/* SEE IF GONE */

	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}


	GetObjectInfo(theNode);

			/* MOVE IT */

	gDelta.y -= 400.0f * fps;							// gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


		/***********************/
		/* SEE IF HIT ANYTHING */
		/***********************/

	if (DoWeaponCollisionDetect(theNode))
		return;

	UpdateObject(theNode);



		/*******************/
		/* LEAVE ICE TRAIL */
		/*******************/

	theNode->ParticleTimer -= fps;
	if (theNode->ParticleTimer <= 0.0f)										// see if time to make particles
	{
		theNode->ParticleTimer += .02f;

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= 200;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 15;
			groupDef.decayRate				=  -.8;
			groupDef.fadeRate				= .5;
			groupDef.particleTextureNum		= PARTICLE_SObjType_SnowFlakes;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			float	x = gCoord.x;
			float 	y = gCoord.y;
			float	z = gCoord.z;

			for (i = 0; i < 5; i++)
			{
				p.x = x - gDelta.x * RandomFloat() * .05f;
				p.y = y - gDelta.y * RandomFloat() * .05f;
				p.z = z - gDelta.z * RandomFloat() * .05f;

				d.x = RandomFloat2() * 20.0f;
				d.y = RandomFloat2() * 20.0f;
				d.z = RandomFloat2() * 20.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= 0;
				newParticleDef.rotDZ		= 0;
				newParticleDef.alpha		= .1f + RandomFloat() * .3f;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}


/********************* EXPLODE FREEZE ***********************/

static void ExplodeFreeze(ObjNode *bullet)
{
long					pg,i;
OGLVector3D				delta;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
float					x,y,z;

	x = gCoord.x;
	y = gCoord.y;
	z = gCoord.z;

			/***********************/
			/* MAKE SNOW PARTICLES */
			/***********************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND|PARTICLE_FLAGS_ALLAIM;
	gNewParticleGroupDef.gravity				= 200;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 30;
	gNewParticleGroupDef.decayRate				=  -1.5;
	gNewParticleGroupDef.fadeRate				= 1.1;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_SnowFlakes;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < 100; i++)
		{
			pt.x = x + RandomFloat2() * 30.0f;
			pt.y = y + RandomFloat2() * 30.0f;
			pt.z = z + RandomFloat2() * 30.0f;

			delta.x = RandomFloat2() * 250.0f;
			delta.y = RandomFloat2() * 250.0f;
			delta.z = RandomFloat2() * 250.0f;

			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= 1.0f + RandomFloat();
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= RandomFloat2() * 15.0f;
			newParticleDef.alpha		= FULL_ALPHA;
			if (AddParticleToGroup(&newParticleDef))
				break;
		}
	}


	PlayEffect3D(EFFECT_FREEZEPOOF, &bullet->Coord);

		/* DELETE THE BULLET */

	DeleteObject(bullet);
}


#pragma mark -

/***************** SHOOT FLAME GUN ************************/

static void ShootFlameGun(ObjNode *player, OGLPoint3D *where, OGLVector3D *aim)
{
ObjNode *newObj;

			/* ADJUST WEAPON AIM TO AUTO-TARGET NEAREST ENEMY */

	WeaponAutoTarget(where, aim);


				/* CREATE BULLET OBJECT */

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_FlameBullet;
	gNewObjectDefinition.coord		= *where;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+1;
	gNewObjectDefinition.moveCall 	= MoveFlameBullet;
	gNewObjectDefinition.rot 		= player->Rot.y;
	gNewObjectDefinition.scale 		= .5f * gPlayerInfo.scaleRatio;	// scale with player
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


	newObj->Kind = WEAPON_TYPE_FLAME;

	newObj->ColorFilter.a = .99;			// do this just to turn on transparency so it'll glow

	newObj->Delta.x = aim->x * FLAME_BULLET_SPEED;
	newObj->Delta.y = aim->y * FLAME_BULLET_SPEED;
	newObj->Delta.z = aim->z * FLAME_BULLET_SPEED;

	newObj->Health = 2.0f;
	newObj->Damage = .8;

	newObj->ParticleTimer = 0;

	newObj->CType 			= CTYPE_WEAPON;
	newObj->CBits			= CBITS_TOUCHABLE;
	CreateCollisionBoxFromBoundingBox_Maximized(newObj);

	PlayEffect_Parms3D(EFFECT_FREEZEGUN, where, NORMAL_CHANNEL_RATE / 2, 1.0);

}


/******************* MOVE FLAME BULLET ***********************/

static void MoveFlameBullet(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum,i;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;

			/* SEE IF GONE */

	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}


	GetObjectInfo(theNode);

			/* TUMBLE */

	theNode->Rot.x += fps * 6.0f;
	theNode->Rot.y -= fps * 5.0f;


			/* MOVE IT */

	gDelta.y -= 400.0f * fps;							// gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


		/***********************/
		/* SEE IF HIT ANYTHING */
		/***********************/

	if (DoWeaponCollisionDetect(theNode))
		return;

	UpdateObject(theNode);



		/*********************/
		/* LEAVE SMOKE TRAIL */
		/*********************/

	theNode->ParticleTimer -= fps;
	if (theNode->ParticleTimer <= 0.0f)										// see if time to make particles
	{
		theNode->ParticleTimer += .015f;

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= -100;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 10.0f * gPlayerInfo.scaleRatio;
			groupDef.decayRate				=  -1.1;
			groupDef.fadeRate				= .7;
			groupDef.particleTextureNum		= PARTICLE_SObjType_BlackSmoke;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			OGLPoint3D	fusePt;
			static const OGLPoint3D	fuseOff = {0,0,-50};

			OGLPoint3D_Transform(&fuseOff, &theNode->BaseTransformMatrix, &fusePt);

			for (i = 0; i < 4; i++)
			{
				p.x = fusePt.x;// + RandomFloat2() * 30.0f;
				p.y = fusePt.y;// + RandomFloat2() * 30.0f;
				p.z = fusePt.z;// + RandomFloat2() * 30.0f;

				d.x = RandomFloat2() * 20.0f;
				d.y = RandomFloat2() * 20.0f;
				d.z = RandomFloat2() * 20.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= 1.0f + RandomFloat() * .2f;
				newParticleDef.rotZ			= 0;
				newParticleDef.rotDZ		= RandomFloat2()*PI2;
				newParticleDef.alpha		= .6f + RandomFloat() * .3f;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}


/********************* EXPLODE FLAME BULLET ***********************/

static void ExplodeFlameBullet(ObjNode *bullet)
{
long					pg,i;
OGLVector3D				delta,v;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
float					x,y,z;

	x = gCoord.x;
	y = gCoord.y;
	z = gCoord.z;

			/***********************/
			/* MAKE FIRE PARTICLES */
			/***********************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
	gNewParticleGroupDef.gravity				= 200;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 50;
	gNewParticleGroupDef.decayRate				=  -1.5;
	gNewParticleGroupDef.fadeRate				= 1.5;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_Fire;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < 40; i++)
		{
			pt.x = x + RandomFloat2() * 50.0f;
			pt.y = y + RandomFloat2() * 50.0f;
			pt.z = z + RandomFloat2() * 50.0f;

			v.x = pt.x - x;
			v.y = pt.y - y;
			v.z = pt.z - z;
			FastNormalizeVector(v.x,v.y,v.z,&v);

			delta.x = v.x * 450.0f;
			delta.y = v.y * 450.0f;
			delta.z = v.z * 450.0f;

			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= 1.0f + RandomFloat();
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= RandomFloat2() * 7.0f;
			newParticleDef.alpha		= FULL_ALPHA;
			if (AddParticleToGroup(&newParticleDef))
				break;
		}
	}

	PlayEffect3D(EFFECT_FLAREUP, &bullet->Coord);


		/* DELETE THE BULLET */

	DeleteObject(bullet);
}



#pragma mark -

/***************** SHOOT FLARE GUN ************************/

static void ShootFlareGun(ObjNode *player, OGLPoint3D *where, OGLVector3D *aim)
{
ObjNode *newObj;
int		i;

#pragma unused(player)

				/* MAKE BULLET EVENT */

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord 		= *where;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+5;
	gNewObjectDefinition.moveCall 	= MoveFlareBullet;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->Kind = WEAPON_TYPE_FLARE;

	newObj->Delta.x = aim->x * FLARE_BULLET_SPEED;
	newObj->Delta.y = aim->y * FLARE_BULLET_SPEED;
	newObj->Delta.z = aim->z * FLARE_BULLET_SPEED;

	newObj->Health = 4.0f;
	newObj->Damage = .8f;

	newObj->DelayToSeek = .5;						// set a delay before they start heat seeking

	newObj->ParticleTimer = 0;

	newObj->CType 			= CTYPE_WEAPON;
	newObj->CBits			= CBITS_TOUCHABLE;
	SetObjectCollisionBounds(newObj, 40,-40,-40,40,40,-40);

			/* MAKE SPARKLE */

	i = newObj->Sparkles[0] = GetFreeSparkle(newObj);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_RANDOMSPIN|SPARKLE_FLAG_FLICKER;
		gSparkles[i].where = newObj->Coord;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 250.0f;

		gSparkles[i].separation = 0.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_BlueGlint;
	}

	PlayEffect_Parms3D(EFFECT_FLARESHOOT, where, NORMAL_CHANNEL_RATE, 1.6);

}


/******************* MOVE FLARE BULLET ***********************/

static void MoveFlareBullet(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum,i;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;


	GetObjectInfo(theNode);


			/* SEE IF GONE */

	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		ExplodeFlareBullet(theNode);				// note:  gCoord must be set!
		return;
	}



			/* SEE IF HEAT SEEK */

	theNode->DelayToSeek -= fps;
	if (theNode->DelayToSeek <= 0.0f)
	{
		float	dist;
		ObjNode *target  = FindClosestEnemy(&gCoord, &dist);

		if (target)
		{
			OGLVector3D	v;

			v.x = target->Coord.x - gCoord.x;
			v.y = target->Coord.y - gCoord.y;
			v.z = target->Coord.z - gCoord.z;
			FastNormalizeVector(v.x,v.y, v.z, &v);

			gDelta.x += v.x * (1900.0f * fps);
			gDelta.y += v.y * (1000.0f * fps);
			gDelta.z += v.z * (1900.0f * fps);

		}
	}



			/* MOVE IT */

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


		/***********************/
		/* SEE IF HIT ANYTHING */
		/***********************/

	if (DoWeaponCollisionDetect(theNode))
		return;

	UpdateObject(theNode);



		/*********************/
		/* LEAVE SPARK TRAIL */
		/*********************/

	theNode->ParticleTimer -= fps;
	if (theNode->ParticleTimer <= 0.0f)										// see if time to make particles
	{
		theNode->ParticleTimer += .015f;

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_BOUNCE;
			groupDef.gravity				= 400;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 70.0f;
			groupDef.decayRate				=  0;
			groupDef.fadeRate				= 1.1;
			groupDef.particleTextureNum		= PARTICLE_SObjType_WhiteSpark3;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			OGLPoint3D	fusePt;
			static const OGLPoint3D	fuseOff = {0,0,-50};

			OGLPoint3D_Transform(&fuseOff, &theNode->BaseTransformMatrix, &fusePt);

			d.x = RandomFloat2() * 20.0f;
			d.y = RandomFloat2() * 20.0f;
			d.z = RandomFloat2() * 20.0f;

			newParticleDef.groupNum		= particleGroup;
			newParticleDef.where		= &gCoord;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= 1.0f + RandomFloat() * .6f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= RandomFloat2()*PI2;
			newParticleDef.alpha		= .9;
			if (AddParticleToGroup(&newParticleDef))
			{
				theNode->ParticleGroup = -1;
			}
		}
	}

			/******************/
			/* UPDATE SPARKLE */
			/******************/

	i = theNode->Sparkles[0];
	if (i != -1)
	{
		gSparkles[i].where = gCoord;
	}
}


/******************* MOVE FLARE BULLET: BOOM ******************************/

static void MoveFlareBullet_Boom(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		i;

	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}

			/* UPDATE SPARKLE */

	i = theNode->Sparkles[0];
	if (i != -1)
	{
		gSparkles[i].scale = 300.0f + RandomFloat2() * 50.0f;
		gSparkles[i].color.a = .8f + RandomFloat() * .19f;
	}
}



/********************* EXPLODE FLARE BULLET ***********************/

static void ExplodeFlareBullet(ObjNode *bullet)
{
long					pg,i;
OGLVector3D				delta;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
float					x,y,z;
Boolean					upThrust;


	x = gCoord.x;
	y = gCoord.y;
	z = gCoord.z;

	if (y <= (GetTerrainY(x,z) + 20.0f))
	{
		upThrust = true;
	}
	else
		upThrust = false;


			/***********************/
			/* MAKE SNOW PARTICLES */
			/***********************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
	gNewParticleGroupDef.gravity				= 150;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 50;
	gNewParticleGroupDef.decayRate				= 0;
	gNewParticleGroupDef.fadeRate				= 1.1;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_WhiteSpark3;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < 20; i++)
		{
			pt.x = x + RandomFloat2() * 50.0f;
			pt.y = y;
			pt.z = z + RandomFloat2() * 50.0f;

			delta.x = RandomFloat2() * 500.0f;
			delta.z = RandomFloat2() * 500.0f;

			if (upThrust)
				delta.y = RandomFloat() * 600.0f;
			else
				delta.y = RandomFloat2() * 250.0f;

			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= 1.0f + RandomFloat();
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= RandomFloat2() * 7.0f;
			newParticleDef.alpha		= FULL_ALPHA;
			if (AddParticleToGroup(&newParticleDef))
				break;
		}
	}


	PlayEffect_Parms3D(EFFECT_FLAREEXPLODE, &gCoord, NORMAL_CHANNEL_RATE, 1.7);


		/* CHANGE MOVE CALL OF THE BULLET */

	bullet->MoveCall = MoveFlareBullet_Boom;
	bullet->CType = 0;
	bullet->Health = .75f;
}


#pragma mark -

/***************** THROW DART ************************/

void ThrowDart(ObjNode *theNode)
{
ObjNode *newObj;
float	r = theNode->Rot.y;
static const OGLColorRGBA color = {1,1,1,.5};
static const OGLPoint3D	off = {0,0,0};

	FindCoordOnJointAtFlagEvent(theNode, PLAYER_JOINT_LEFTHAND, &off, &gNewObjectDefinition.coord);


				/* CREATE BULLET OBJECT */

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_DartPOW;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveDart;
	gNewObjectDefinition.rot 		= r;
	gNewObjectDefinition.scale 		= .3;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


	newObj->Kind = WEAPON_TYPE_DART;

	newObj->Delta.x = -sin(r) * DART_SPEED;
	newObj->Delta.y = 300.0f;
	newObj->Delta.z = -cos(r) * DART_SPEED;

	newObj->Health = 3.0f;
	newObj->Damage = .4f;

	newObj->CType 			= CTYPE_WEAPON;
	newObj->CBits			= CBITS_TOUCHABLE;
	CreateCollisionBoxFromBoundingBox_Maximized(newObj);


				/* VAPOR TRAIL */

	newObj->VaporTrails[0] = CreateNewVaporTrail(newObj, 0, VAPORTRAIL_TYPE_COLORSTREAK,
													&gNewObjectDefinition.coord, &color, .1, .4, 50);


				/* MAKE SHADOW */

	AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 1,2, false);


	PlayEffect_Parms3D(EFFECT_DARTWOOSH, &newObj->Coord, NORMAL_CHANNEL_RATE, 1.1);

			/* dec inventory */

	DecWeaponQuantity(WEAPON_TYPE_DART);
}


/******************* MOVE DART ***********************/

static void MoveDart(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float				r;
ObjNode				*enemy;
static OGLColorRGBA color = {1,1,1,.3};
OGLVector3D		v,v2;

			/* SEE IF GONE */

	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		ExplodeDart(theNode);
		return;
	}


	GetObjectInfo(theNode);

			/* AIM TOWARD NEAREST ENEMY */

	enemy = FindClosestCType3D(&gCoord, CTYPE_ENEMY | CTYPE_HEATSEEKATTACT);
	if (enemy)
	{
		TurnObjectTowardTarget(theNode, &gCoord, enemy->Coord.x, enemy->Coord.z, PI, false);

		if (enemy->Coord.y > gCoord.y)				// go up/down
		{
			gDelta.y += 200.0f * fps;
			if (gDelta.y > 200.0f)
				gDelta.y = 200.0f;
		}
		else
		{
			gDelta.y -= 200.0f * fps;
			if (gDelta.y < -200.0f)
				gDelta.y = -200.0f;
		}
	}


			/* MOVE IT */

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * DART_SPEED;
	gDelta.z = -cos(r) * DART_SPEED;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* CALC X-ROT */

	v.x = gCoord.x - theNode->OldCoord.x;
	v.y = gCoord.y - theNode->OldCoord.y;
	v.z = gCoord.z - theNode->OldCoord.z;
	FastNormalizeVector(v.x, v.y, v.z, &v);				// calc aim vec
	FastNormalizeVector(v.x, 0, v.z, &v2);				// calc flat vec

	theNode->Rot.x = acos(OGLVector3D_Dot(&v,&v2));		// set angle
	if (v.y < 0.0f)
		theNode->Rot.x = -theNode->Rot.x;


		/***********************/
		/* SEE IF HIT ANYTHING */
		/***********************/

	if (DoWeaponCollisionDetect(theNode))
		return;

	UpdateObject(theNode);


	AddToVaporTrail(&theNode->VaporTrails[0], &gCoord, &color);
}



/******************* EXPLODE DART *****************************/

static void ExplodeDart(ObjNode *theNode)
{
	MakePuff(&theNode->Coord, 20, PARTICLE_SObjType_RedFumes, GL_SRC_ALPHA, GL_ONE, 1);


	DeleteObject(theNode);
}



















