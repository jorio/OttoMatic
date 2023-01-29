/*******************************/
/*   	PLAYER_ROBOT.C		   */
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

static void MovePlayerRobot_JumpJet(ObjNode *theNode);
static void MovePlayerRobot_Jump(ObjNode *theNode);
static void MovePlayerRobot_Punch(ObjNode *theNode);
static void MovePlayerRobot_PickupAndDeposit(ObjNode *theNode);
static void MovePlayerRobot_PickupAndHoldGun(ObjNode *theNode);
static void MovePlayerRobot_Drink(ObjNode *theNode);
static void MovePlayerRobot_RideZip(ObjNode *theNode);
static void MovePlayerRobot_ClimbInto(ObjNode *theNode);
static void MovePlayerRobot_ShotFromCannon(ObjNode *theNode);
static void MovePlayerRobot_BumperCar(ObjNode *theNode);
static void MovePlayerRobot_GrabbedByStrongMan(ObjNode *theNode);
static void MovePlayerRobot_RocketSled(ObjNode *theNode);
static void MovePlayerRobot_Fall_BottomlessPit(ObjNode *theNode);
static void MovePlayerRobot_Drilled(ObjNode *theNode);

static Boolean DoRobotCollisionDetect(ObjNode *theNode, Boolean useBBoxForTerrain);
static void CheckPunchCollision(ObjNode *player);
static void CheckPlayerActionControls(ObjNode *theNode);
static void DoRobotFrictionAndGravity(ObjNode *theNode, float friction);
static Boolean DoPlayerMovementAndCollision(ObjNode *theNode, Byte aimMode, Boolean useBBoxForTerrain);
static Boolean DoPlayerMovementAndCollision_JumpJet(ObjNode *theNode);
static void DoPlayerMovementAndCollision_Bubble(ObjNode *theNode);
static void MovePlayerRobot_Stand(ObjNode *theNode);
static void MovePlayerRobot_Walk(ObjNode *theNode);
static void MovePlayerRobot_Fall(ObjNode *theNode);
static void MovePlayerRobot_Grabbed(ObjNode *theNode);
static void MovePlayerRobot_Flattened(ObjNode *theNode);
static void MovePlayerRobot_Charging(ObjNode *theNode);
static void MovePlayerRobot_Zapped(ObjNode *theNode);
static void MovePlayerRobot_Drowned(ObjNode *theNode);
static void MovePlayerRobot_Thrown(ObjNode *theNode);
static void MovePlayerRobot_GotHit(ObjNode *theNode);
static void MovePlayerRobot_Accordian(ObjNode *theNode);
static void MovePlayerRobot_ChangeWeapon(ObjNode *theNode);
static void MovePlayerRobot_Bubble(ObjNode *theNode);
static void MovePlayerRobot_PickupAndHoldMagnet(ObjNode *theNode);
static void MovePlayerRobot_SuckedIntoWell(ObjNode *theNode);
static void MovePlayerRobot_RideZip(ObjNode *theNode);
static void MovePlayerRobot_Throw(ObjNode *theNode);
static void MovePlayerRobot_BellySlide(ObjNode *theNode);

static float CalcWalkAnimSpeed(ObjNode *theNode);
static void StartJumpJet(ObjNode *theNode);
static void EndJumpJet(ObjNode *theNode);
static void UpdateJumpJetParticles(ObjNode *theNode);
static void UpdatePlayerAutoAim(ObjNode *player);

static Boolean IsPlayerDoingWalkAnim(ObjNode *theNode);
static Boolean IsPlayerDoingStandAnim(ObjNode *theNode);

static void TurnPlayerTowardPunchable(ObjNode *player);

static void DoPlayerMagnetSkiing(ObjNode *player);
static void EndMagnetSkiing(ObjNode *player, Boolean crash);
static void VerifyTargetPickup(void);

static Boolean ShouldApplySlopesToPlayer(float newDistToFloor);

/****************************/
/*    CONSTANTS             */
/****************************/

#define	PUNCH_DAMAGE			.25f

#define	MAX_BUBBLE_SPEED		400.0f

#define	PLAYER_NORMAL_MAX_SPEED	900.0f

#define	PLAYER_SLOPE_ACCEL		3000.0f

#define	PLAYER_AIR_FRICTION		400.0f
#define	PLAYER_DEFAULT_FRICTION	1200.0f
#define	PLAYER_HEAVY_FRICTION	2700.0f

#define DEBUG_PLAYER_VAPOR		0
#if DEBUG_PLAYER_VAPOR
	#define PLAYER_VAPOR_THRESHOLD	10.0f
	#define PLAYER_VAPOR_ALPHA		1.0f
#else
	#define	PLAYER_VAPOR_THRESHOLD	700.0f
	#define PLAYER_VAPOR_ALPHA		0.2f
#endif

#define	JUMP_DELTA					1800.0f
#define	JUMP_JET_ACCELERATION		2000.0f
#define	JUMP_JET_ACCELERATION_GIANT	(JUMP_JET_ACCELERATION * .5f)
#define	JUMP_JET_DECELERATION		2000.0f

#define	DELTA_SUBDIV			15.0f				// smaller == more subdivisions per frame

#define	CONTROL_SENSITIVITY		2200.0f
#define	CONTROL_SENSITIVITY_AIR	5000.0f

#define	CONTROL_SENSITIVITY_PR_TURN	4.0f
#define	CONTROL_SENSITIVITY_PR		4200.0f
#define	CONTROL_SENSITIVITY_AIR_PR	6000.0f


enum
{
	AIM_MODE_NONE,
	AIM_MODE_NORMAL,
	AIM_MODE_REVERSE
};

#define	TARGET_SKIING_DIST		150.0f				// desired dist behind magnet monster to ski

#define	WALK_STAND_THRESHOLD	30.0f

#define	THRUST_TIMER			.7f				// time after player stops controlling to keep into stand anim regardless of movement

/*********************/
/*    VARIABLES      */
/*********************/

uint16_t			gPlayerTileAttribs;

OGLVector3D		gPreCollisionDelta;

ObjNode 		*gTargetPickup = nil;
ObjNode			*gCannon = nil;

float			gPlayerBottomOff = 0;

float			gTimeSinceLastThrust = 0;			// time since player last made player move
float			gTimeSinceLastShoot = 0;

float			gPlayerSlipperyFactor = 0;

Boolean			gResetJumpJet = true;				// set to true when player is on ground to reset jump jet
Boolean			gDoJumpJetAtApex = false;			// true if want to do jump jet when player reaches apex of jump

#define	JumpJetEnginesOff		Flag[0]				// set by anim when jump jet acceleration turns off

//
// In order to let the player move faster than the max speed, we use a current and target value.
// the target is what we want the max to normally be, and the current is what it currently is.
// So, when the player needs to go faster, we modify Current and then slowly decay it back to Target.
//

float	gTargetMaxSpeed = PLAYER_NORMAL_MAX_SPEED;
float	gCurrentMaxSpeed = PLAYER_NORMAL_MAX_SPEED;


/*************************** INIT PLAYER: ROBOT ****************************/
//
// Creates an ObjNode for the player
//
// INPUT:
//			where = floor coord where to init the player.
//			rotY = rotation to assign to player if oldObj is nil.
//

void InitPlayer_Robot(OGLPoint3D *where, float rotY)
{
ObjNode	*newObj;
float	y;
int		i;

		/* FIND THE Y COORD TO START */

	y = FindHighestCollisionAtXZ(where->x,where->z, CTYPE_MISC|CTYPE_TERRAIN);

	if (gLevelNum == LEVEL_NUM_BLOBBOSS)
		y += 5000.0f;				// blob boss, player falls from above
	else
		y += -gObjectGroupBBoxList[MODEL_GROUP_SKELETONBASE+SKELETON_TYPE_OTTO][0].min.y * PLAYER_DEFAULT_SCALE + 5.0f;	// offset y so foot is on ground


		/*****************************/
		/* MAKE OTTO'S SKELETON BODY */
		/*****************************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_OTTO;
	gNewObjectDefinition.animNum	= 0;
	gNewObjectDefinition.coord.x 	= where->x;
	gNewObjectDefinition.coord.z 	= where->z;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG|STATUS_BIT_DONTCULL;
	gNewObjectDefinition.slot 		= PLAYER_SLOT;
	gNewObjectDefinition.moveCall	= MovePlayer_Robot;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= PLAYER_DEFAULT_SCALE;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

	gPlayerInfo.objNode 		= newObj;
	gPlayerInfo.isTeleporting 	= false;

	newObj->Rot.y = rotY;


				/* SET COLLISION INFO */

	newObj->BoundingSphereRadius *= 2.0f;

	newObj->CType = CTYPE_PLAYER;
	newObj->CBits = CBITS_ALLSOLID;

	SetObjectCollisionBounds(newObj, newObj->BBox.max.y, gPlayerBottomOff = newObj->BBox.min.y, -40, 40, 40, -40);


		/***********************/
		/* CREATE HAND OBJECTS */
		/***********************/

	for (i = 0; i < (GLOBAL_ObjType_FlameGunHand-GLOBAL_ObjType_OttoLeftHand+1); i++)
	{
		BG3D_SphereMapGeomteryMaterial(MODEL_GROUP_GLOBAL, GLOBAL_ObjType_OttoLeftHand+i, 0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);	// set this model to be sphere mapped
	}


			/* LEFT HAND */

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_OttoLeftHand;
	gNewObjectDefinition.slot		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	gPlayerInfo.leftHandObj 		= MakeNewDisplayGroupObject(&gNewObjectDefinition);



			/* RIGHT HAND */

	gNewObjectDefinition.type 	= GLOBAL_ObjType_OttoRightHand;
	gPlayerInfo.rightHandObj 	= MakeNewDisplayGroupObject(&gNewObjectDefinition);

	gPlayerInfo.rightHandObj->Kind = WEAPON_TYPE_FIST;			// set weapon type since this gets passed to weapon handlers

	gPlayerInfo.rightHandObj->Damage = PUNCH_DAMAGE;


		/*******************/
		/* SET OTHER STUFF */
		/*******************/

	gTargetMaxSpeed = PLAYER_NORMAL_MAX_SPEED;
	gCurrentMaxSpeed = PLAYER_NORMAL_MAX_SPEED;



	AttachShadowToObject(newObj, 0, DEFAULT_PLAYER_SHADOW_SCALE,DEFAULT_PLAYER_SHADOW_SCALE * .8f, true);

	CreatePlayerSparkles(newObj);


	gTimeSinceLastThrust = -8;			// start negative so camera won't swing around right off the bat
	gTimeSinceLastShoot = 10;
	gResetJumpJet = true;
	gExplodePlayerAfterElectrocute = false;
}


/******************** MOVE PLAYER: ROBOT ***********************/

void MovePlayer_Robot(ObjNode *theNode)
{
	static void(*myMoveTable[])(ObjNode *) =
	{
		MovePlayerRobot_Stand,							// standing
		MovePlayerRobot_Walk,							// walking
		MovePlayerRobot_Walk,							// walk with gun
		MovePlayerRobot_Stand,							// stand with gun
		MovePlayerRobot_Punch,							// punch
		MovePlayerRobot_PickupAndDeposit,							// pickup
		MovePlayerRobot_PickupAndHoldGun,						// pickup2
		MovePlayerRobot_Grabbed,						// grabbed
		MovePlayerRobot_Flattened,						// flattened
		MovePlayerRobot_Charging,						// charging
		MovePlayerRobot_JumpJet,						// jump jet
		MovePlayerRobot_Jump,							// jump
		MovePlayerRobot_Fall,							// fall
		MovePlayerRobot_Zapped,							// zapped
		MovePlayerRobot_Drowned,						// drowned
		MovePlayerRobot_GotHit,							// got hit
		MovePlayerRobot_Accordian,						// accordian
		MovePlayerRobot_ChangeWeapon,					// changeweapon
		MovePlayerRobot_Bubble,							// bubble
		MovePlayerRobot_PickupAndHoldMagnet,			// magnet
		MovePlayerRobot_SuckedIntoWell,					// sucked into well
		MovePlayerRobot_JumpJet,						// GIANT jump jet
		MovePlayerRobot_Thrown,							// thrown
		MovePlayerRobot_Grabbed,						// grabbed2
		MovePlayerRobot_Drink,							// drink
		MovePlayerRobot_RideZip,						// ride zip
		MovePlayerRobot_ClimbInto,						// climb into
		MovePlayerRobot_ShotFromCannon,					// shot from cannon
		MovePlayerRobot_BumperCar,						// drive bumper car
		MovePlayerRobot_GrabbedByStrongMan,				// grabbed by strongman
		MovePlayerRobot_Throw,							// throw dart
		MovePlayerRobot_RocketSled,						// tobogan
		nil,											// sit on ledge for main menu
		MovePlayerRobot_Drilled,						// drilled
		MovePlayerRobot_BellySlide,						// slide
	};

	GetObjectInfo(theNode);

	if (gPlayerHasLanded)								// inc thrust timer since assume no thrust this frame
	{
		gTimeSinceLastThrust += gFramesPerSecondFrac;
		gTimeSinceLastShoot += gFramesPerSecondFrac;
	}

			/* FIRST SEE IF NEED TO UPDATE GROWTH */

	UpdatePlayerGrowth(theNode);


			/* JUMP TO HANDLER */

	myMoveTable[theNode->Skeleton->AnimNum](theNode);


		/* SEE IF SHOULD FREEZE CAMERA */

	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_JUMPJET)		// dont move camera from during jump-jet
		gFreezeCameraFromXZ = true;
	else
		gFreezeCameraFromXZ = false;
}



/******************** MOVE PLAYER ROBOT: STAND ***********************/

static void MovePlayerRobot_Stand(ObjNode *theNode)
{
	theNode->Rot.x = 0;								// make sure this is correct
	theNode->Rot.z = 0;								// make sure this is correct

			/* MOVE PLAYER */

	UpdatePlayerAutoAim(theNode);
	DoRobotFrictionAndGravity(theNode, PLAYER_HEAVY_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, AIM_MODE_NORMAL, false))
		goto update;


			/* SEE IF SHOULD WALK */

	if (IsPlayerDoingStandAnim(theNode))
	{
		float	animSpeed =  CalcWalkAnimSpeed(theNode);

		if ((animSpeed > WALK_STAND_THRESHOLD) && (gTimeSinceLastThrust < THRUST_TIMER))
			SetPlayerWalkAnim(theNode);
	}

			/* DO CONTROL */
			//
			// do this last since we want any jump command to work smoothly
			//

	CheckPlayerActionControls(theNode);




			/* UPDATE IT */

update:
	UpdatePlayer_Robot(theNode);
}


/******************** MOVE PLAYER ROBOT: WALK ***********************/

static void MovePlayerRobot_Walk(ObjNode *theNode)
{
	theNode->Rot.x = 0;								// make sure this is correct
	theNode->Rot.z = 0;								// make sure this is correct

			/* DO CONTROL */

	CheckPlayerActionControls(theNode);


			/* MOVE PLAYER */

	UpdatePlayerAutoAim(theNode);

	if ((gPlayerInfo.analogControlX == 0.0f) &&			// if no user control, do heavy friction
		 (gPlayerInfo.analogControlZ == 0.0f))
	{
		DoRobotFrictionAndGravity(theNode, PLAYER_HEAVY_FRICTION);
	}
	else
	{
		DoRobotFrictionAndGravity(theNode, PLAYER_DEFAULT_FRICTION);
	}

	if (DoPlayerMovementAndCollision(theNode, AIM_MODE_NORMAL, false))
		goto update;


			/* SEE IF SHOULD STAND */

	if (IsPlayerDoingWalkAnim(theNode))
	{
		float	animSpeed =  CalcWalkAnimSpeed(theNode);

		if ((animSpeed < WALK_STAND_THRESHOLD) || (gTimeSinceLastThrust > THRUST_TIMER))
			SetPlayerStandAnim(theNode, 8);
		else
			theNode->Skeleton->AnimSpeed = animSpeed * .004f;
	}



			/* UPDATE IT */

update:
	UpdatePlayer_Robot(theNode);
}

/******************* CALC WALK ANIM SPEED **********************/

static float CalcWalkAnimSpeed(ObjNode *theNode)
{
float	animSpeed,accSpeed;

	accSpeed = sqrt(theNode->AccelVector.x * theNode->AccelVector.x + theNode->AccelVector.y * theNode->AccelVector.y);		// calc dist of accel vector
	accSpeed *= 1000.0f;

	animSpeed = theNode->Speed2D * (1.0f - gPlayerSlipperyFactor) + (accSpeed * gPlayerSlipperyFactor);							// calc weighted anim speed

				/* NOW ACCOUNT FOR GIANT SCALE */

	if (theNode->Scale.x != PLAYER_DEFAULT_SCALE)
	{
		float	f;

		f = PLAYER_DEFAULT_SCALE / theNode->Scale.x;
		animSpeed *= f;
	}

	return(animSpeed);
}


/******************** MOVE PLAYER ROBOT: JUMP ***********************/

static void MovePlayerRobot_Jump(ObjNode *theNode)
{
Byte	aimMode;

			/* DO CONTROL */

	CheckPlayerActionControls(theNode);


			/* MOVE PLAYER */

	if (gTimeSinceLastThrust > .5f)			// only aim player if he's under user control
		aimMode = AIM_MODE_NONE;
	else
		aimMode = AIM_MODE_NORMAL;

	DoRobotFrictionAndGravity(theNode, PLAYER_AIR_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, aimMode, false))
		goto update;


	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_JUMP)		// only bother if still in Fall anim
	{
				/* SEE IF LANDED */

		if (theNode->StatusBits & STATUS_BIT_ONGROUND)
		{
			SetPlayerStandAnim(theNode, 8);
		}

				/* SEE IF FALLING */
		else
		if (gDelta.y < 0.0f)
		{
			if (gDoJumpJetAtApex)							// see if we wanted to jet @ the apex
				StartJumpJet(theNode);
			else
				MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_FALL, 4.0);
		}
	}


			/* UPDATE IT */

update:
	UpdatePlayer_Robot(theNode);
}

/******************** MOVE PLAYER ROBOT: PICKUP AND DEPOSIT ***********************/

static void MovePlayerRobot_PickupAndDeposit(ObjNode *theNode)
{
OGLMatrix4x4	m,m2;

	gTimeSinceLastThrust = 0;							// reset this so camera won't auto-adjust during this anim (a bit of a hack really)

	VerifyTargetPickup();

			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no user control during this anim
	DoRobotFrictionAndGravity(theNode, PLAYER_HEAVY_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, AIM_MODE_NONE, true))
		goto update;


			/* KEEP AIMED AT TARGET */

	if (gTargetPickup && (!theNode->IsHoldingPickup))
	{
		TurnObjectTowardTarget(theNode, &gCoord, gTargetPickup->Coord.x,
								gTargetPickup->Coord.z, 9.0, false);
	}

			/* SEE IF DONE */

	if (theNode->Skeleton->AnimHasStopped)										// go to stand when done with anim
		SetPlayerStandAnim(theNode, 2);

	if (theNode->Skeleton->AnimNum != PLAYER_ANIM_PICKUPDEPOSIT)				// if anim has changed then finish things up with the pickup
	{
		if (theNode->IsHoldingPickup)											// if we had it then add it
		{
			theNode->IsHoldingPickup = false;

			if (gTargetPickup != nil)											// verify that there's a target
			{
				AddPowerupToInventory(gTargetPickup);
				PlayEffect3D(EFFECT_WEAPONDEPOSIT, &gCoord);


				if (gTargetPickup->CType & CTYPE_BLOBPOW) 				 			// special check for blobule powerups
					DisableHelpType(HELP_MESSAGE_SLIMEBALLS);						// picked up a blob so dont need to help on this anymore

				if (gTargetPickup->POWRegenerate)							// if regeneratable, then just hide until needed
				{
					gTargetPickup->MoveCall = MovePowerup;					// restore the Move Call
					gTargetPickup->StatusBits |= STATUS_BIT_HIDDEN;
					if (gTargetPickup->ShadowNode)							// and hide shadow
						gTargetPickup->ShadowNode->StatusBits |= STATUS_BIT_HIDDEN;

					gTargetPickup->CType = 0;								// no collision when hidden
				}
				else
				{
					DeleteObject(gTargetPickup);
				}
				gTargetPickup = nil;
			}
		}
	}


			/* UPDATE IT */

update:
	UpdatePlayer_Robot(theNode);



		/*********************/
		/* UPDATE THE PICKUP */
		/*********************/

	if (gTargetPickup && theNode->IsHoldingPickup)
	{
		gTargetPickup->MoveCall = nil;										// make sure the pickup no longer has control if itself

		OGLMatrix4x4_SetTranslate(&m,0,-20,0);								// set offset to palm of hand
		m.value[M00] = gTargetPickup->Scale.x;								// insert scale values
		m.value[M11] = gTargetPickup->Scale.y;
		m.value[M22] = gTargetPickup->Scale.z;

		FindJointFullMatrix(theNode, PLAYER_JOINT_RIGHTHAND, &m2);
		OGLMatrix4x4_Multiply(&m, &m2, &m2);
		OGLMatrix4x4_SetScale(&m, 1.0f/theNode->Scale.x,1.0f/theNode->Scale.x,1.0f/theNode->Scale.x);	// cancel out the player's scale thats in that matrix
		OGLMatrix4x4_Multiply(&m, &m2, &gTargetPickup->BaseTransformMatrix);

		SetObjectTransformMatrix(gTargetPickup);
		UpdateShadow(gTargetPickup);
	}
}


/******************** MOVE PLAYER ROBOT: PICKUP AND HOLD GUN ***********************/
//
// For picking up holdable objects when left hand is currently empty - item goes into hand instead of being deposited.
//

static void MovePlayerRobot_PickupAndHoldGun(ObjNode *theNode)
{
	gTimeSinceLastThrust = 0;							// reset this so camera won't auto-adjust during this anim (a bit of a hack really)

	VerifyTargetPickup();

	if (gTargetPickup)														// if not holding it yet, then keep aimed at it
	{
		TurnObjectTowardTarget(theNode, &gCoord, gTargetPickup->Coord.x,
								gTargetPickup->Coord.z, 9.0, false);
	}

			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no user control during this anim
	DoRobotFrictionAndGravity(theNode, PLAYER_HEAVY_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, AIM_MODE_NONE, true))
		goto update;


			/* SEE IF DONE */

	if (theNode->Skeleton->AnimHasStopped)
		SetPlayerStandAnim(theNode, 4);


			/* SEE IF GRAB IT NOW */

	if (gTargetPickup)
	{
		if ((theNode->Skeleton->AnimNum != PLAYER_ANIM_PICKUPANDHOLDGUN) || (theNode->IsHoldingPickup))
		{
			PlayEffect3D(EFFECT_WEAPONCLICK, &gCoord);

			AddPowerupToInventory(gTargetPickup);
			theNode->IsHoldingPickup = false;

			if (gTargetPickup->POWRegenerate)							// if regeneratable, then just hide until needed
			{
				gTargetPickup->StatusBits |= STATUS_BIT_HIDDEN;
				if (gTargetPickup->ShadowNode)							// and hide shadow
					gTargetPickup->ShadowNode->StatusBits |= STATUS_BIT_HIDDEN;
				gTargetPickup->CType = 0;
			}
			else
				DeleteObject(gTargetPickup);
			gTargetPickup = nil;
		}
	}

			/* UPDATE IT */

update:
	UpdatePlayer_Robot(theNode);
}

/******************** MOVE PLAYER ROBOT: PICKUP AND HOLD MAGNET ***********************/
//
// Does both the pickup and holding of the magnet
//

static void MovePlayerRobot_PickupAndHoldMagnet(ObjNode *theNode)
{
	VerifyTargetPickup();


			/****************************/
			/* MOVE DURING HOLDING PART */
			/****************************/

	if (theNode->IsHoldingPickup)
	{
		DoPlayerMagnetSkiing(theNode);
	}

			/***************************/
			/* MOVE DURING PICKUP PART */
			/***************************/
	else
	{

				/* MOVE PLAYER */

		gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no user control during this anim
		DoRobotFrictionAndGravity(theNode, PLAYER_HEAVY_FRICTION);
		if (DoPlayerMovementAndCollision(theNode, AIM_MODE_NONE, true))
			goto update;


				/* KEEP AIMED AT TARGET */

		if (gTargetPickup && (!theNode->IsHoldingPickup))
		{
			TurnObjectTowardTarget(theNode, &gCoord, gTargetPickup->Coord.x,
									gTargetPickup->Coord.z, 9.0, false);
		}
	}

			/* SEE IF DONE */

	if (theNode->Skeleton->AnimNum != PLAYER_ANIM_PICKUPANDHOLDMAGNET)			// if anim has changed then finish things up with the pickup
	{
		EndMagnetSkiing(theNode, false);
	}


			/* UPDATE IT */

update:
	UpdatePlayer_Robot(theNode);



		/*********************/
		/* UPDATE THE MAGNET */
		/*********************/

	if (gTargetPickup && theNode->IsHoldingPickup)
	{
		static const OGLPoint3D	off = {0,0,10};
		OGLPoint3D	p1,p2;

		gTargetPickup->MoveCall = nil;										// make sure the pickup no longer has control if itself
		gTargetPickup->CType = 0;											// no collision during this

					/* CALC AVERGE BETWEEN 2 HANDS */

		FindCoordOnJoint(theNode, PLAYER_JOINT_RIGHTHAND, &off, &p1);
		FindCoordOnJoint(theNode, PLAYER_JOINT_LEFTHAND, &off, &p2);
		gTargetPickup->Coord.x = (p1.x + p2.x) * .5f;
		gTargetPickup->Coord.y = (p1.y + p2.y) * .5f;
		gTargetPickup->Coord.z = (p1.z + p2.z) * .5f;

		gTargetPickup->Rot.y = theNode->Rot.y;								// match y rot to player
		gTargetPickup->Rot.x = -PI/2;										// flip to aim forward

					/* UPDATE NODE */

		UpdateObjectTransforms(gTargetPickup);
		UpdateShadow(gTargetPickup);

		DisplayHelpMessage(HELP_MESSAGE_LETGOMAGNET,1.0, true);
	}
}


/************************ VERIFY TARGET PICKUP **************************/

static void VerifyTargetPickup(void)
{
	if (gTargetPickup)
	{
		if (gTargetPickup->CType == INVALID_NODE_FLAG)
		{
			gTargetPickup = nil;
		}
	}
}



/******************** MOVE PLAYER ROBOT: CHANGE WEAPON ***********************/

static void MovePlayerRobot_ChangeWeapon(ObjNode *theNode)
{
	gTimeSinceLastThrust = 0;							// reset this so camera won't auto-adjust during this anim (a bit of a hack really)

			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no user control during this anim
	DoRobotFrictionAndGravity(theNode, PLAYER_HEAVY_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, AIM_MODE_NONE, false))
		goto update;


			/* SEE IF DONE */

	if (theNode->Skeleton->AnimHasStopped)
	{
		SetPlayerStandAnim(theNode, 3);
	}


			/* FAST WEAPON SWITCHING */
			//
			// Player can keep cycling weapons until Otto has started
			// pulling out a new gun (ChangeWeapon anim flag)
			//

	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_CHANGEWEAPON
		&& !theNode->ChangeWeapon)
	{
		CheckWeaponChangeControls(theNode);
	}

			/* UPDATE IT */

update:
	UpdatePlayer_Robot(theNode);
}

/******************** MOVE PLAYER ROBOT: SUCKED INTO WELL ***********************/

static void MovePlayerRobot_SuckedIntoWell(ObjNode *theNode)
{

	gTimeSinceLastThrust = 0;							// reset this so camera won't auto-adjust during this anim (a bit of a hack really)

			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;				// no user control during this anim
	gCoord.y = GetTerrainY(gCoord.x, gCoord.z) - theNode->BBox.min.y - 40.0f;	// just keep stuck to ground

			/* UPDATE IT */

	UpdatePlayer_Robot(theNode);


	gMinHeightOffGround += 150.0f * gFramesPerSecondFrac;						// raise camera as player gets sucked down
	gCameraDistFromMe += 100.0f * gFramesPerSecondFrac;
	gCameraLookAtYOff += 300.0f * gFramesPerSecondFrac;
}




/******************** MOVE PLAYER ROBOT: PUNCH ***********************/

static void MovePlayerRobot_Punch(ObjNode *theNode)
{

			/* MOVE PLAYER */

	TurnPlayerTowardPunchable(theNode);										// aim at punchable

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no user control during this anim
	DoRobotFrictionAndGravity(theNode, PLAYER_HEAVY_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, AIM_MODE_NONE, false))
		goto update;

			/* SEE IF CHECK FOR PUNCH COLLISION */

	if (theNode->PunchCanHurt)
	{
		CheckPunchCollision(theNode);

	}


			/* SEE IF SHOULD STAND */

	if (theNode->Skeleton->AnimHasStopped)
	{
		SetPlayerStandAnim(theNode, 8);
	}


			/* UPDATE IT */

update:
	UpdatePlayer_Robot(theNode);
}


/******************** MOVE PLAYER ROBOT: THROW ***********************/

static void MovePlayerRobot_Throw(ObjNode *theNode)
{

			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no user control during this anim
	DoRobotFrictionAndGravity(theNode, PLAYER_HEAVY_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, AIM_MODE_NONE, false))
		goto update;

			/* SEE IF CHECK FOR RELEASE DART */

	if (theNode->ThrowDartNow)
	{
		ThrowDart(theNode);
		theNode->ThrowDartNow = false;
	}


			/* SEE IF SHOULD STAND */

	if (theNode->Skeleton->AnimHasStopped)
	{
		SetPlayerStandAnim(theNode, 4);
	}


			/* UPDATE IT */

update:
	UpdatePlayer_Robot(theNode);
}



/*********************** TURN PLAYER TOWARD PUNCHABLE ****************************/

static void TurnPlayerTowardPunchable(ObjNode *player)
{
ObjNode *thisNode,*nearest;
float	bestDist,maxDist;

	bestDist = 10000000;
	nearest = nil;

	maxDist = 220.0f * player->Scale.x;							// calc max dist to check for punchables

			/* SCAN FOR NEAREST PUNCHABLE IN FRONT */

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode->HitByWeaponHandler[WEAPON_TYPE_FIST] == nil)					// only look for punchables
			goto next;

		float ex = thisNode->Coord.x;							// get obj coords
		float ez = thisNode->Coord.z;

		float dist = CalcDistance(gCoord.x, gCoord.z, ex, ez);
		if ((dist < bestDist) && (dist < maxDist))				// see if best dist & close enough
		{
			bestDist = dist;
			nearest = thisNode;
		}

next:
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);


			/* THERE IS SOMETHING THERE */

	if (nearest)
	{
		TurnObjectTowardTarget(player, &gCoord, nearest->Coord.x,
								nearest->Coord.z, 6.0, false);
	}


}


/******************** MOVE PLAYER ROBOT: JUMPJET ***********************/

static void MovePlayerRobot_JumpJet(ObjNode *theNode)
{


			/* MOVE PLAYER */

	if (DoPlayerMovementAndCollision_JumpJet(theNode))
		goto update;


			/* SEE IF LANDED */

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
		SetPlayerStandAnim(theNode, 7);
	}
			/* SEE IF DONE WITH JUMP-JET ANIM */
	else
	if (theNode->Skeleton->AnimHasStopped || GetNewNeedState(kNeed_Jump))
	{
		MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_FALL, 4.0);		// make fall anim
	}



			/* UPDATE IT */

update:

	if ((theNode->Skeleton->AnimNum != PLAYER_ANIM_JUMPJET) &&
		(theNode->Skeleton->AnimNum != PLAYER_ANIM_GIANTJUMPJET))		// if anything changed, then make sure to cleanup
	{
		EndJumpJet(theNode);
	}

	UpdatePlayer_Robot(theNode);
	UpdateJumpJetParticles(theNode);



}

/******************** MOVE PLAYER ROBOT: FALL ***********************/

static void MovePlayerRobot_Fall(ObjNode *theNode)
{
float	oldDY = gDelta.y;
Byte	aimMode;

	gTimeSinceLastThrust = 0;							// reset this so camera won't auto-adjust during this anim (a bit of a hack really)

			/********************************************/
			/* SEE IF FALLING IN A BOTTOMLESS PIT DEATH */
			/********************************************/

	if (gPlayerFellIntoBottomlessPit)
	{
		MovePlayerRobot_Fall_BottomlessPit(theNode);
		return;
	}

				/* DO CONTROL */

	CheckPlayerActionControls(theNode);


			/* MOVE PLAYER */

	if (gTimeSinceLastThrust > .5f)			// only aim player if he's under user control
		aimMode = AIM_MODE_NONE;
	else
		aimMode = AIM_MODE_NORMAL;

	DoRobotFrictionAndGravity(theNode, PLAYER_AIR_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, aimMode, false))
		goto update;


			/*****************/
			/* SEE IF LANDED */
			/*****************/

	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_FALL)						// only bother if still in Fall anim
	{
		if ((theNode->StatusBits & STATUS_BIT_ONGROUND) || (gPlayerInfo.distToFloor < 10.0f))
		{

					/* DO ANIM AFTER LANDED */

			switch(theNode->Skeleton->AnimNum)
			{
				case	PLAYER_ANIM_BUMPERCAR:								// check if trigger put us in bumper car
						break;

				default:
						SetPlayerStandAnim(theNode, 8);						// default to standing when landing
			}

			if (oldDY < -300.0f)											// if landing fast enough
			{
				PlayEffect3D(EFFECT_METALLAND, &gCoord);					// play hard land sound

						/* MAKE DEFORMATION WAVE */

				if (gLevelNum == LEVEL_NUM_BLOB)
				{
					if (gPlayerInfo.distToFloor < 10.0f)						// if on terrain, then deform
					{
						DeformationType		defData;

						defData.type 				= DEFORMATION_TYPE_RADIALWAVE;
						defData.amplitude 			= oldDY * -.08f * gPlayerInfo.scaleRatio;
						if (defData.amplitude > 1000.0f)
							defData.amplitude = 1000.0f;

						defData.radius 				= 0;
						defData.speed 				= 4000;
						defData.origin.x			= gCoord.x;
						defData.origin.y			= gCoord.z;
						defData.oneOverWaveLength 	= 1.0f / (400.0f * gPlayerInfo.scaleRatio);
						defData.radialWidth			= 600.0f;
						defData.decayRate			= 200.0f * gPlayerInfo.scaleRatio;
						NewSuperTileDeformation(&defData);
					}
				}
			}
		}
	}


			/* UPDATE IT */

update:
	UpdatePlayer_Robot(theNode);
}


/******************** MOVE PLAYER ROBOT: FALL BOTTOMLESS PIT ***********************/

static void MovePlayerRobot_Fall_BottomlessPit(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;
	gDelta.x = gDelta.z = 0;

			/* SEE IF TIME FOR DEATH EXIT TRANSITION */

	if (gDeathTimer > 0.0f)
	{
		gDeathTimer -= fps;					// dec timer
		if (gDeathTimer <= 0.0f)								// see if time to reset player
			StartDeathExit(0);
	}

			/* DO MOVEMENT */

	DoRobotFrictionAndGravity(theNode, PLAYER_AIR_FRICTION);
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	UpdatePlayer_Robot(theNode);
}


/******************** MOVE PLAYER ROBOT: THROWN ***********************/
//
// Player does very basic stuff when being thrown since player has
// no control and we don't care about a lot of things in the collision.
//

static void MovePlayerRobot_Thrown(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	gTimeSinceLastThrust = 0;							// reset this so camera won't auto-adjust during this anim (a bit of a hack really)


			/* MOVE IT */

	gDelta.y -= gGravity * fps * .7f;
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.x += fps * 10.0f;


			/* COLLISION */

	HandleCollisions(theNode, CTYPE_MISC|CTYPE_TERRAIN|CTYPE_FENCE, -.6);
	if (theNode->StatusBits & STATUS_BIT_ONGROUND)								// once on ground then stop
	{
		theNode->Rot.x = 0;

		gPlayerInfo.invincibilityTimer = 2.0f;
		MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_FLATTENED, 8.0);
		gPlayerInfo.knockDownTimer = 1.5;
		PlayEffect_Parms3D(EFFECT_PLAYERCLANG, &gCoord, NORMAL_CHANNEL_RATE, 0.8f);
		PlayerLoseHealth(.2, PLAYER_DEATH_TYPE_EXPLODE);
	}



			/* UPDATE IT */

	UpdatePlayer_Robot(theNode);
}

/******************** MOVE PLAYER ROBOT: GRABBED ***********************/

static void MovePlayerRobot_Grabbed(ObjNode *theNode)
{
	gTimeSinceLastThrust = 0;							// reset this so camera won't auto-adjust during this anim (a bit of a hack really)


	UpdatePlayer_Robot(theNode);
}


/******************** MOVE PLAYER ROBOT: GRABBED BY STRONGMAN ***********************/

static void MovePlayerRobot_GrabbedByStrongMan(ObjNode *theNode)
{
	gTimeSinceLastThrust = 0;							// reset this so camera won't auto-adjust during this anim (a bit of a hack really)


	UpdatePlayer_Robot(theNode);
}


/******************** MOVE PLAYER ROBOT: DRILLED ***********************/

static void MovePlayerRobot_Drilled(ObjNode *theNode)
{
	gTimeSinceLastThrust = 0;							// reset this so camera won't auto-adjust during this anim (a bit of a hack really)

	UpdatePlayer_Robot(theNode);
}




/******************** MOVE PLAYER ROBOT: ZAPPED ***********************/
//
// when in H20
//

static void MovePlayerRobot_Zapped(ObjNode *theNode)
{
	if (gLevelNum == LEVEL_NUM_BLOBBOSS)				// if blob boss then must have fallen into water/ground to bob
		gCoord.y = GetTerrainY(gCoord.x, gCoord.z);

	theNode->ZappedTimer -= gFramesPerSecondFrac;					// dec timer
	if (theNode->ZappedTimer <= 0.0f)
	{
			/* KILL THE PLAYER */

		if (gExplodePlayerAfterElectrocute)
			KillPlayer(PLAYER_DEATH_TYPE_EXPLODE);
		else
			KillPlayer(PLAYER_DEATH_TYPE_DROWN);

	}

	UpdatePlayer_Robot(theNode);
}

/******************** MOVE PLAYER ROBOT: DROWNED ***********************/

static void MovePlayerRobot_Drowned(ObjNode *theNode)
{
	if (gPlayerInfo.waterPatch == -1)						// see if have patch, otherwise use terrain y
		gCoord.y = GetTerrainY(gCoord.x, gCoord.z);
	else
		gCoord.y = gWaterBBox[gPlayerInfo.waterPatch].max.y;


			/* SEE IF END */

	if (gDeathTimer > 0.0f)
	{
		gDeathTimer -= gFramesPerSecondFrac;					// dec timer
		if (gDeathTimer <= 0.0f)								// see if time to reset player
		{
			StartDeathExit(0);
		}
	}

	UpdatePlayer_Robot(theNode);
}


/******************** MOVE PLAYER ROBOT: FLATTENED ***********************/

static void MovePlayerRobot_Flattened(ObjNode *theNode)
{
Boolean	wasOnGround = !!(theNode->StatusBits & STATUS_BIT_ONGROUND);

	gTimeSinceLastThrust = 0;							// reset this so camera won't auto-adjust during this anim (a bit of a hack really)

			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no user control during this anim
	DoRobotFrictionAndGravity(theNode, PLAYER_DEFAULT_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, AIM_MODE_NONE, true))
		goto update;


			/* PLAY BOUNCE SFX */

	if ((theNode->StatusBits & STATUS_BIT_ONGROUND) && (!wasOnGround))
	{
//		if (gDelta.y > 20.0f)
			PlayEffect_Parms3D(EFFECT_PLAYERCLANG, &gCoord, NORMAL_CHANNEL_RATE, 0.8f);
	}


	gPlayerInfo.knockDownTimer -= gFramesPerSecondFrac;
	if (gPlayerInfo.knockDownTimer <= 0.0f)
	{
		SetPlayerStandAnim(theNode, 4);
		gPlayerInfo.invincibilityTimer = 1.0f;
	}

update:
	UpdatePlayer_Robot(theNode);
}



/******************** MOVE PLAYER ROBOT: BELLY SLIDE ***********************/

static void MovePlayerRobot_BellySlide(ObjNode *theNode)
{
	theNode->Rot.x = theNode->Rot.z = 0;

			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no user control during this anim
	DoRobotFrictionAndGravity(theNode, PLAYER_DEFAULT_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, AIM_MODE_NONE, true))
		goto update;


	gPlayerInfo.knockDownTimer -= gFramesPerSecondFrac;
	if ((gPlayerInfo.knockDownTimer <= 0.0f) || (theNode->Speed2D <= 0.0f))
	{
		SetPlayerStandAnim(theNode, 3);
		gPlayerInfo.invincibilityTimer = 1.0f;
	}

update:
	UpdatePlayer_Robot(theNode);
}



/******************** MOVE PLAYER ROBOT: CHARGING ***********************/

static void MovePlayerRobot_Charging(ObjNode *theNode)
{
	gTimeSinceLastThrust = 0;							// reset this so camera won't auto-adjust during this anim (a bit of a hack really)

			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no user control during this anim
	DoRobotFrictionAndGravity(theNode, PLAYER_HEAVY_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, AIM_MODE_NONE, false))
		goto update;


			/* SEE IF DISCHARGE SUPERNOVA */

	if (gPlayerInfo.superNovaStatic)						// if this obj exists then we're still charging
	{
		if (!GetNeedState(kNeed_Shoot))
		{
			DischargeSuperNova();							// attempt to discharge it
		}
	}

			/* UPDATE IT */

update:
	UpdatePlayer_Robot(theNode);
}

/******************** MOVE PLAYER ROBOT: GOT HIT ***********************/

static void MovePlayerRobot_GotHit(ObjNode *theNode)
{
float	f;

	gTimeSinceLastThrust = 0;							// reset this so camera won't auto-adjust during this anim (a bit of a hack really)

			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no user control during this anim

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
		f = PLAYER_HEAVY_FRICTION;
	else
		f = PLAYER_AIR_FRICTION;
	DoRobotFrictionAndGravity(theNode, f);
	if (DoPlayerMovementAndCollision(theNode, AIM_MODE_REVERSE, true))
		goto update;


			/* SEE IF DONE */

	if (theNode->Skeleton->AnimHasStopped)
	{
		gPlayerInfo.invincibilityTimer = 2.0f;
		SetPlayerStandAnim(theNode, 5);
	}


			/* UPDATE IT */

update:
	UpdatePlayer_Robot(theNode);
}


/******************** MOVE PLAYER ROBOT: ACCORDIAN ***********************/

static void MovePlayerRobot_Accordian(ObjNode *theNode)
{
	gTimeSinceLastThrust = 0;							// reset this so camera won't auto-adjust during this anim (a bit of a hack really)

			/* MATCH FEET TO GROUND */

	gCoord.y = GetTerrainY(gCoord.x, gCoord.z) + theNode->BBox.min.y;


			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no user control during this anim
	DoRobotFrictionAndGravity(theNode, PLAYER_HEAVY_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, AIM_MODE_NONE, true))
		goto update;


			/* SEE IF DONE */

	theNode->AccordianTimer -= gFramesPerSecondFrac;
	if (theNode->AccordianTimer <= 0.0f)
	{
		gPlayerInfo.invincibilityTimer = 1.3f;
		SetPlayerStandAnim(theNode, 3);
	}


			/* UPDATE IT */

update:
	UpdatePlayer_Robot(theNode);
}


/******************** MOVE PLAYER ROBOT: BUBBLE ***********************/

static void MovePlayerRobot_Bubble(ObjNode *theNode)
{
float	y;

			/* KEEP FLOATING ABOVE GROUND */

	y = GetTerrainY(gCoord.x, gCoord.z) + 300.0f;
	gDelta.y = y - gCoord.y;
	if (gDelta.y > 0.0f)
		gDelta.y *= 2.0f;					// float up faster than float down


			/* MOVE IT */

	DoPlayerMovementAndCollision_Bubble(theNode);


			/* UPDATE ME */

	UpdatePlayer_Robot(theNode);


		/* UPDATE BUBBLE */

	if (gSoapBubble)
	{
		gSoapBubble->Coord = gCoord;
		UpdateObjectTransforms(gSoapBubble);
		UpdateShadow(gSoapBubble);
	}
}


/******************** MOVE PLAYER ROBOT: DRINK ***********************/

static void MovePlayerRobot_Drink(ObjNode *theNode)
{
	gTimeSinceLastThrust = 0;							// reset this so camera won't auto-adjust during this anim (a bit of a hack really)

			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no user control during this anim
	DoRobotFrictionAndGravity(theNode, PLAYER_DEFAULT_FRICTION);
	if (DoPlayerMovementAndCollision(theNode, AIM_MODE_NONE, true))
		goto update;


			/* SEE IF DONE */

	if (theNode->Skeleton->AnimHasStopped)
	{
		gPlayerInfo.growMode = GROWTH_MODE_GROW;
		gPlayerInfo.giantTimer = 12.0f;

		SetPlayerStandAnim(theNode, 6.0);
		DecWeaponQuantity(WEAPON_TYPE_GROWTH);
	}
update:
	UpdatePlayer_Robot(theNode);
}


/******************** MOVE PLAYER ROBOT: RIDE ZIP ***********************/

static void MovePlayerRobot_RideZip(ObjNode *theNode)
{
float	r = theNode->Rot.y;


	gCoord = gCurrentZip->Coord;		// get pully coord

	gCoord.y -= 170.0f;					// offset y

	gCoord.x += sin(r) * 25.0f;			// move back a little to align it
	gCoord.z += cos(r) * 25.0f;

	UpdatePlayer_Robot(theNode);
}


/******************** MOVE PLAYER ROBOT: CLIMB INTO ***********************/

static void MovePlayerRobot_ClimbInto(ObjNode *theNode)
{
	theNode->Skeleton->AnimSpeed = 1.6;

			/* MOVE PLAYER */

	gPlayerInfo.analogControlX = gPlayerInfo.analogControlZ = 0;			// no user control during this anim
	gDelta.x = gDelta.y = gDelta.z = 0.0f;
	if (DoPlayerMovementAndCollision(theNode, AIM_MODE_NONE, true))
		goto update;


update:
	theNode->Rot.y = gCannon->Rot.y + PI;			// keep aligned w/ cannon

	UpdatePlayer_Robot(theNode);
}

/******************** MOVE PLAYER ROBOT: SHOT FROM CANNON ***********************/

static void MovePlayerRobot_ShotFromCannon(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* MOVE PLAYER */

	gDelta.y -= 500.0f * fps;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.x -= PI/7 * fps;

			/* SEE IF HIT GROUND */

	if ((gCoord.y + theNode->BBox.min.y) <= GetTerrainY(gCoord.x, gCoord.z))
	{
		SetSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_BELLYSLIDE);
		gPlayerInfo.knockDownTimer = 1.0f;
		PlayEffect_Parms3D(EFFECT_PLAYERCLANG, &gCoord, NORMAL_CHANNEL_RATE, 0.8f);
	}



			/* UPDATE IT */

	UpdatePlayer_Robot(theNode);
}



/******************** MOVE PLAYER ROBOT: BUMPER CAR ***********************/

static void MovePlayerRobot_BumperCar(ObjNode *theNode)
{


			/* UPDATE IT */

	AlignPlayerInBumperCar(theNode);
	UpdatePlayer_Robot(theNode);
}

/******************** MOVE PLAYER ROBOT: ROCKETSLED ***********************/

static void MovePlayerRobot_RocketSled(ObjNode *theNode)
{


			/* UPDATE IT */

	AlignPlayerInRocketSled(theNode);
	UpdatePlayer_Robot(theNode);
}



#pragma mark -

/************************ UPDATE PLAYER: ROBOT ***************************/

void UpdatePlayer_Robot(ObjNode *theNode)
{
float fps = gFramesPerSecondFrac;

			/* VERIFY BOTTOMLESS PIT */

	if (gLevelNum == LEVEL_NUM_CLOUD)
	{
		if ((gCoord.y + theNode->BBox.min.y) < (GetTerrainY2(gCoord.x, gCoord.z) - 30.0f))		// if player is below terrain's real height
		{
			if (GetTerrainY(gCoord.x, gCoord.z) == BOTTOMLESS_PIT_Y)							//  and if this is a bottomless pit zone
				KillPlayer(PLAYER_DEATH_TYPE_FALL);
		}
	}


		/* VERIFY ANIM INFO */

	switch(theNode->Skeleton->AnimNum)
	{
		case	PLAYER_ANIM_STANDWITHGUN:
				if (!gPlayerInfo.holdingGun)											// verify using correct stand anim
					MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_STAND, 6.0);
				break;

		case	PLAYER_ANIM_STAND:
				if (gPlayerInfo.holdingGun)
					MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_STANDWITHGUN, 6.0);
				break;

		case	PLAYER_ANIM_WALKWITHGUN:
				if (!gPlayerInfo.holdingGun)											// verify using correct walk anim
					MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_WALK, 9.0);
				break;

		case	PLAYER_ANIM_WALK:
				if (gPlayerInfo.holdingGun)
					MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_WALKWITHGUN, 9.0);
				break;

	}


		/* UPDATE SPECIAL COLLISION INFO */

	switch(theNode->Skeleton->AnimNum)
	{
		case	PLAYER_ANIM_ACCORDIAN:
		case	PLAYER_ANIM_FLATTENED:
		case	PLAYER_ANIM_DROWNED:
		case	PLAYER_ANIM_GOTHIT:
				theNode->TopOff = theNode->BBox.max.y * .7f;
				break;

		default:
				theNode->TopOff = gObjectGroupBBoxList[MODEL_GROUP_SKELETONBASE+SKELETON_TYPE_OTTO][0].max.y * theNode->Scale.y * .7f;
				break;
	}


		/* UPDATE OBJECT AS LONG AS NOT BEING MATRIX CONTROLLED */

	switch(theNode->Skeleton->AnimNum)
	{
		case	PLAYER_ANIM_GRABBED:
		case	PLAYER_ANIM_GRABBED2:
		case	PLAYER_ANIM_GRABBEDBYSTRONGMAN:
		case	PLAYER_ANIM_ROCKETSLED:
				break;

		default:
				UpdateObject(theNode);
	}

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

			/* UPDATE SPARKLES & FLAME */

	UpdatePlayerSparkles(theNode);

	if (gPlayerInfo.burnTimer > 0.0f)
	{
		gPlayerInfo.burnTimer -= fps;
		BurnSkeleton(theNode,40.0f * gPlayerInfo.scaleRatio);
	}




			/****************/
			/* UPDATE HANDS */
			/****************/

	UpdateRobotHands(theNode);


			/* CHECK FOR ACCIDENTAL SUPERNOVA DISCHARGE */

	if ((theNode->Skeleton->AnimNum != PLAYER_ANIM_CHARGING) &&		// if we are not charging but we have a static object
		(gPlayerInfo.superNovaStatic != nil))
	{
		DischargeSuperNova();
	}


		/* CHECK INV TIMER */

	gPlayerInfo.invincibilityTimer -= fps;


	if (theNode->StatusBits & STATUS_BIT_ONGROUND)		// if on ground then reset jump jet
		gResetJumpJet = true;
}


/********************* UPDATE ROBOT HANDS *************************/

void UpdateRobotHands(ObjNode *theNode)
{
ObjNode	*lhand,*rhand;
Boolean	holdingGun;
int		weaponType;

		/****************************/
		/* DETERMINE IF HOLDING GUN */
		/****************************/

	if ((theNode->Skeleton->AnimNum == PLAYER_ANIM_CHANGEWEAPON) && (!theNode->ChangeWeapon))	// see if still holding old gun during weapon change
	{
		weaponType = gPlayerInfo.oldWeaponType;				// use old weapon Type
		holdingGun = gPlayerInfo.wasHoldingGun;				// use old gun flag
	}
	else
	{
		holdingGun = gPlayerInfo.holdingGun;
		weaponType = gPlayerInfo.currentWeaponType;
		if (weaponType >= WEAPON_TYPE_FIST)							// verify it while we're here
			holdingGun = gPlayerInfo.holdingGun = false;
	}

	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_BUMPERCAR)		// don't show gun while driving bumper car
		holdingGun = false;
	else
	if (theNode->Skeleton->AnimNum == PLAYER_ANIM_BUBBLE)			// don't show gun while riding bubble
		holdingGun = false;

			/*****************************/
			/* UPDATE GEOMETRY FOR HANDS */
			/*****************************/

	lhand = gPlayerInfo.leftHandObj;						// get hand objects
	rhand = gPlayerInfo.rightHandObj;

	lhand->ColorFilter.a = rhand->ColorFilter.a = theNode->ColorFilter.a;	// match alpha fades

	if (rhand && lhand)										// only update if both hands are legit
	{

		switch(theNode->Skeleton->AnimNum)
		{
					/* OPEN HANDS */

			case	PLAYER_ANIM_STAND:
			case	PLAYER_ANIM_STANDWITHGUN:
			case	PLAYER_ANIM_JUMP:
			case	PLAYER_ANIM_FALL:
			case	PLAYER_ANIM_BUBBLE:
			case	PLAYER_ANIM_SITONLEDGE:
			case	PLAYER_ANIM_DRILLED:
					if (rhand->Type != GLOBAL_ObjType_OttoRightHand)
					{
						rhand->Type = GLOBAL_ObjType_OttoRightHand;
						ResetDisplayGroupObject(rhand);
					}

					if (!holdingGun)
					{
						if (lhand->Type != GLOBAL_ObjType_OttoLeftHand)
						{
							lhand->Type = GLOBAL_ObjType_OttoLeftHand;
							ResetDisplayGroupObject(lhand);
						}
					}
					break;


					/* FISTS */

			default:
					if (rhand->Type != GLOBAL_ObjType_OttoRightFist)
					{
						rhand->Type = GLOBAL_ObjType_OttoRightFist;
						ResetDisplayGroupObject(rhand);
					}

					if (!holdingGun)
					{
						if (lhand->Type != GLOBAL_ObjType_OttoLeftFist)
						{
							lhand->Type = GLOBAL_ObjType_OttoLeftFist;
							ResetDisplayGroupObject(lhand);
						}
					}
					break;
		}

				/* GUN IN LEFT HAND */

		if (holdingGun)
		{
			if (lhand->Type != (GLOBAL_ObjType_PulseGunHand + weaponType))
			{
				lhand->Type = GLOBAL_ObjType_PulseGunHand + weaponType;
				ResetDisplayGroupObject(lhand);
			}
		}


			/* UPDATE HAND MATRICES */

		FindJointFullMatrix(theNode, PLAYER_JOINT_LEFTHAND, &lhand->BaseTransformMatrix);
		SetObjectTransformMatrix(lhand);
		FindJointFullMatrix(theNode, PLAYER_JOINT_RIGHTHAND, &rhand->BaseTransformMatrix);
		SetObjectTransformMatrix(rhand);

		lhand->Coord.x = lhand->BaseTransformMatrix.value[M03];			// get coords from matrix
		lhand->Coord.y = lhand->BaseTransformMatrix.value[M13];
		lhand->Coord.z = lhand->BaseTransformMatrix.value[M23];

		rhand->Coord.x = rhand->BaseTransformMatrix.value[M03];
		rhand->Coord.y = rhand->BaseTransformMatrix.value[M13];
		rhand->Coord.z = rhand->BaseTransformMatrix.value[M23];
	}
}


/******************** UPDATE PLAYER MOTION BLUR **********************/
//
// Called automatically as a callback from UpdateSkinnedGeometry() since
// we want to update the motion blur with the most recent skeleton joint coordinates.
//


void UpdatePlayerMotionBlur(ObjNode *theNode)
{
int		v,i;
static OGLColorRGBA color	= { 0.6f, 0.6f, 1.0f, PLAYER_VAPOR_ALPHA };
static OGLColorRGBA color2	= { 0.4f, 1.0f, 0.4f, PLAYER_VAPOR_ALPHA };
OGLPoint3D	p;
OGLVector3D	playerVec,viewVec;
float		dot;


			/*************************/
			/* SEE IF SHOULD TO BLUR */
			/*************************/

	if (theNode->Skeleton->AnimNum != PLAYER_ANIM_JUMPJET)													// always blur when jump-jetting
	{
		if (theNode->Speed3D < PLAYER_VAPOR_THRESHOLD)				// only add trail if going fast
			return;

			/* DONT DO BLUR IF PLAYER IS MOVING AWAY FROM CAMERA */

		FastNormalizeVector(theNode->Delta.x, theNode->Delta.y, theNode->Delta.z, &playerVec);					// calc player motion vector
		FastNormalizeVector(theNode->Coord.x - gGameViewInfoPtr->cameraPlacement.cameraLocation.x,				// calc vector from camera to player
							theNode->Coord.y - gGameViewInfoPtr->cameraPlacement.cameraLocation.y,
							theNode->Coord.z - gGameViewInfoPtr->cameraPlacement.cameraLocation.z,
							&viewVec);

		dot = OGLVector3D_Dot(&playerVec, &viewVec);															// calc dot product to determine angle
		if ((dot > .9f) || (dot < -.9f))
			return;
	}


			/*******************/
			/* UPDATE THE BLUR */
			/*******************/

	for (v = 0; v< theNode->Skeleton->skeletonDefinition->NumBones; v++)
	{
		FindCoordOfJoint(theNode, v, &p);

		i = theNode->VaporTrails[v];							// get vaport trail #

		if (VerifyVaporTrail(i, theNode, v))
		{
			if (v == 1)											// head has white trail
				AddToVaporTrail(&theNode->VaporTrails[v], &p, &color2);
			else
				AddToVaporTrail(&theNode->VaporTrails[v], &p, &color);
		}
		else
		{
			if (theNode->Speed3D > (PLAYER_VAPOR_THRESHOLD * 1.2f))				// only start new vapor if above the threshold by a good margin to avoid "popping"
				theNode->VaporTrails[v] = CreateNewVaporTrail(theNode, v, VAPORTRAIL_TYPE_COLORSTREAK, &p, &color, 1.0, 4.0, 20.0);
		}
	}

}


/******************* UPDATE PLAYER AUTO AIM ****************************/

static void UpdatePlayerAutoAim(ObjNode *player)
{
float	tx,tz;

	if (gPlayerInfo.autoAimTimer <= 0.0f)
		return;

	gPlayerInfo.autoAimTimer -= gFramesPerSecondFrac;			// dec timer

	tx = gPlayerInfo.autoAimTargetX;									// get targt aim coords
	tz = gPlayerInfo.autoAimTargetZ;

	TurnObjectTowardTarget(player, &gCoord, tx, tz, PI2, false);

}

#pragma mark -

/******************* DO PLAYER MOVEMENT AND COLLISION DETECT *********************/
//
// OUTPUT: true if disabled or killed
//

static Boolean DoPlayerMovementAndCollision(ObjNode *theNode, Byte aimMode, Boolean useBBoxForTerrain)
{
float				fps = gFramesPerSecondFrac,oldFPS,oldFPSFrac,terrainY;
OGLVector2D			aimVec,deltaVec, accVec;
OGLMatrix3x3		m;
static OGLPoint2D origin = {0,0};
int					numPasses,pass;
Boolean				killed = false;

	if (gPlayerInfo.analogControlX || gPlayerInfo.analogControlZ)	// if player is attempting some control then reset this timer
	{
		gTimeSinceLastThrust = 0;
		gForceCameraAlignment = false;								// now that player is moving us, dont force auto-align
		gCameraUserRotY = 0;										// ... and reset user rot
	}


				/*******************************/
				/* DO PLAYER-RELATIVE CONTROLS */
				/*******************************/

	if (gGamePrefs.playerRelControls)
	{
		float	r,sens;

		sens = gPlayerInfo.analogControlX * fps * CONTROL_SENSITIVITY_PR_TURN;
		if (theNode->Speed2D > 400.0f)												// turning less sensitive if walking
			sens *= .5f;

		r = theNode->Rot.y -= sens;													// use x to rotate

		theNode->AccelVector.x = sin(r);
		theNode->AccelVector.y = cos(r);

		if (theNode->StatusBits & STATUS_BIT_ONGROUND)
		{
			gDelta.x += theNode->AccelVector.x * ((CONTROL_SENSITIVITY_PR * gPlayerInfo.analogControlZ) * (1.1f - gPlayerSlipperyFactor) * fps);
			gDelta.z += theNode->AccelVector.y * ((CONTROL_SENSITIVITY_PR * gPlayerInfo.analogControlZ) * (1.1f - gPlayerSlipperyFactor) * fps);
		}
		else
		{
			gDelta.x += theNode->AccelVector.x * (CONTROL_SENSITIVITY_AIR_PR * gPlayerInfo.analogControlZ * fps);
			gDelta.z += theNode->AccelVector.y * (CONTROL_SENSITIVITY_AIR_PR * gPlayerInfo.analogControlZ * fps);
		}

	}

				/*******************************/
				/* DO CAMERA-RELATIVE CONTROLS */
				/*******************************/
	else
	{
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
//			OGLVector2D_Normalize(&theNode->AccelVector, &theNode->AccelVector);
			OGLVector2D_Transform(&theNode->AccelVector, &m, &theNode->AccelVector);		// rotate the acceleration vector


						/* APPLY ACCELERATION TO DELTAS */

			if (theNode->StatusBits & STATUS_BIT_ONGROUND)
			{
				gDelta.x += theNode->AccelVector.x * (CONTROL_SENSITIVITY * (1.1f - gPlayerSlipperyFactor) * fps);
				gDelta.z += theNode->AccelVector.y * (CONTROL_SENSITIVITY * (1.1f - gPlayerSlipperyFactor) * fps);
			}
			else
			{
				gDelta.x += theNode->AccelVector.x * (CONTROL_SENSITIVITY_AIR * fps);
				gDelta.z += theNode->AccelVector.y * (CONTROL_SENSITIVITY_AIR * fps);
			}
		}



				/**********************************************************/
				/* TURN PLAYER TO AIM DIRECTION OF ACCELERATION OR MOTION */
				/**********************************************************/
				//
				// Depending on how slippery the terrain is, we aim toward the direction
				// of motion or the direction of acceleration.  We'll use the gPlayerSlipperyFactor value
				// to average an aim vector between the two.
				//

		if ((aimMode != AIM_MODE_NONE) && (theNode->Speed2D > 0.0f))
		{
			FastNormalizeVector2D(gDelta.x, gDelta.z, &deltaVec, true);
			FastNormalizeVector2D(theNode->AccelVector.x, theNode->AccelVector.y, &accVec, true);

			aimVec.x = deltaVec.x * (1.0f - gPlayerSlipperyFactor) + (accVec.x * gPlayerSlipperyFactor);
			aimVec.y = deltaVec.y * (1.0f - gPlayerSlipperyFactor) + (accVec.y * gPlayerSlipperyFactor);

			if (aimMode == AIM_MODE_REVERSE)
				TurnObjectTowardTarget(theNode, &gCoord, gCoord.x - aimVec.x, gCoord.z - aimVec.y, 8.0f, false);
			else
			if (aimMode == AIM_MODE_NORMAL)
			{
				float	turnSpeed;

				if (theNode->Speed2D > 400.0f)					// if walking fast then decrease the turn sensitivity
					turnSpeed = 8;
				else
					turnSpeed = 15;

				TurnObjectTowardTarget(theNode, &gCoord, gCoord.x + aimVec.x, gCoord.z + aimVec.y, turnSpeed, false);
			}
		}
	}

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
		float	tweak;

		tweak = theNode->Speed2D / gCurrentMaxSpeed;

		gDelta.x /= tweak;
		gDelta.z /= tweak;

		theNode->Speed2D = gCurrentMaxSpeed;
	}


		/*****************************************/
		/* PART 1: MOVE AND COLLIDE IN MULTIPASS */
		/*****************************************/

		/* SUB-DIVIDE DELTA INTO MANAGABLE LENGTHS */

	oldFPS = gFramesPerSecond;											// remember what fps really is
	oldFPSFrac = gFramesPerSecondFrac;

	numPasses = (theNode->Speed2D*oldFPSFrac) * (1.0f / DELTA_SUBDIV);	// calc how many subdivisions to create
	numPasses++;

	gFramesPerSecondFrac *= 1.0f / (float)numPasses;					// adjust frame rate during motion and collision
	gFramesPerSecond *= 1.0f / (float)numPasses;

	fps = gFramesPerSecondFrac;

	for (pass = 0; pass < numPasses; pass++)
	{
		float	dx,dy,dz;

//		OGLPoint3D oldCoord = gCoord;									// remember starting coord


				/* GET DELTA */

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

				/* MOVE IT */

		gCoord.x += dx*fps;
		gCoord.y += dy*fps;
		gCoord.z += dz*fps;


				/******************************/
				/* DO OBJECT COLLISION DETECT */
				/******************************/

		if (DoRobotCollisionDetect(theNode, useBBoxForTerrain))
			killed = true;



	}

	gFramesPerSecond = oldFPS;										// restore real FPS values
	gFramesPerSecondFrac = oldFPSFrac;


				/*************************/
				/* CHECK FENCE COLLISION */
				/*************************/

	DoFenceCollision(theNode);


				/* CHECK FLOOR */

	terrainY = GetTerrainY(gCoord.x, gCoord.z);
	gPlayerInfo.distToFloor = gCoord.y + theNode->BBox.min.y - terrainY;				// calc dist to floor

//	if ((terrainY == BOTTOMLESS_PIT_Y) && ((gCoord.y + theNode->BBox.min.y) < 0.0f))	// special check if fallen into bottomless pit
//	{
//		KillPlayer(PLAYER_DEATH_TYPE_FALL);
//	}

	return(killed);
}


/******************* DO PLAYER MOVEMENT AND COLLISION DETECT: JUMP JET *********************/
//
// OUTPUT: true if disabled or killed
//

static Boolean DoPlayerMovementAndCollision_JumpJet(ObjNode *theNode)
{
float				fps = gFramesPerSecondFrac,oldFPS,oldFPSFrac;
OGLVector3D			accVec;
OGLMatrix4x4		m;
static const OGLVector3D up = {0,1,0};
Boolean				killed = false;
int					numPasses,pass;



				/* SET INITIAL INFO */

	theNode->AccelVector.x = theNode->AccelVector.y = 0;							// player has no acceleration control of jump-jet

				/* CALC TURNING */

	theNode->Rot.y -= gPlayerInfo.analogControlX * 1.5f * fps;


		/* CALC MOTION VECTOR BASED ON AIM OF JUMP-JET ANIM */

	FindJointFullMatrix(theNode,  PLAYER_JOINT_BASE, &m);							// get matrix from skeleton joint
	OGLVector3D_Transform(&up, &m, &accVec);										// calculate a motion vector

				/* CALC SPEED */

	VectorLength2D(theNode->Speed2D, gDelta.x, gDelta.z);					// calc 2D speed value
	if ((theNode->Speed2D >= 0.0f) && (theNode->Speed2D < 10000000.0f))	// check for weird NaN bug
	{
	}
	else
	{
		theNode->Speed2D = 0;
		gDelta.x = gDelta.z = 0;
	}


		/*****************************************/
		/* PART 1: MOVE AND COLLIDE IN MULTIPASS */
		/*****************************************/

		/* SUB-DIVIDE DELTA INTO MANAGABLE LENGTHS */

	oldFPS = gFramesPerSecond;											// remember what fps really is
	oldFPSFrac = gFramesPerSecondFrac;

	numPasses = (theNode->Speed2D*oldFPSFrac) * (1.0f / DELTA_SUBDIV);	// calc how many subdivisions to create
	numPasses++;

	gFramesPerSecondFrac *= 1.0f / (float)numPasses;					// adjust frame rate during motion and collision
	gFramesPerSecond *= 1.0f / (float)numPasses;

	fps = gFramesPerSecondFrac;

	for (pass = 0; pass < numPasses; pass++)
	{
		float	dx,dy,dz;

//		OGLPoint3D oldCoord = gCoord;									// remember starting coord


				/* GET DELTA */

		dx = gDelta.x;
		dy = gDelta.y;
		dz = gDelta.z;


					/* MOVE IT */

		if (theNode->JumpJetEnginesOff)
		{
			gPlayerInfo.jumpJetSpeed -= JUMP_JET_DECELERATION * fps;					// decelerate jump jet speed
			if (gPlayerInfo.jumpJetSpeed < 0.0f)
				gPlayerInfo.jumpJetSpeed = 0;
		}
		else
		{
			if (theNode->Skeleton->AnimNum == PLAYER_ANIM_GIANTJUMPJET)
				gPlayerInfo.jumpJetSpeed += JUMP_JET_ACCELERATION_GIANT * fps;					// accelerate jump jet speed as giant
			else
				gPlayerInfo.jumpJetSpeed += JUMP_JET_ACCELERATION * fps;					// accelerate jump jet speed
		}

		gDelta.x = accVec.x * gPlayerInfo.jumpJetSpeed;		// calc delta
		gDelta.y = accVec.y * gPlayerInfo.jumpJetSpeed;
		gDelta.z = accVec.z * gPlayerInfo.jumpJetSpeed;

		gCoord.x += dx*fps;
		gCoord.y += dy*fps;
		gCoord.z += dz*fps;


					/******************************/
					/* DO OBJECT COLLISION DETECT */
					/******************************/

		killed = DoRobotCollisionDetect(theNode, true);


	}

	gFramesPerSecond = oldFPS;										// restore real FPS values
	gFramesPerSecondFrac = oldFPSFrac;

				/*************************/
				/* CHECK FENCE COLLISION */
				/*************************/

	DoFenceCollision(theNode);

	gPlayerInfo.distToFloor = gCoord.y + theNode->BBox.min.y - GetTerrainY(gCoord.x, gCoord.z);



				/******************************/
				/* DO JUMP-JET FIST COLLISION */
				/******************************/

	if (!killed)
	{
		static const OGLPoint3D fistOff = {0,-50,0};
		OGLPoint3D	fistCoord1,fistCoord2;
		int			i;
		ObjNode		*hitObj;

					/* CALC COORD TO TEST */

		FindCoordOnJoint(theNode, PLAYER_JOINT_RIGHTHAND, &fistOff, &fistCoord1);			// calc coord of fists
		FindCoordOnJoint(theNode, PLAYER_JOINT_LEFTHAND, &fistOff, &fistCoord2);

		fistCoord1.x = (fistCoord1.x + fistCoord2.x) * .5f;								// average fist coords
		fistCoord1.y = (fistCoord1.y + fistCoord2.y) * .5f;
		fistCoord1.z = (fistCoord1.z + fistCoord2.z) * .5f;

						/* SEE IF HIT ANYTHING */

		if (DoSimpleBoxCollision(fistCoord1.y + 30.0f, fistCoord1.y - 30.0f, fistCoord1.x - 30.0f, fistCoord1.x + 30.0f,
								fistCoord1.z + 30.0f, fistCoord1.z - 30.0f, CTYPE_MISC|CTYPE_ENEMY))
		{
			for (i = 0; i < gNumCollisions; i++)										// scan hits for a handler
			{
				hitObj = gCollisionList[i].objectPtr;									// get hit ObjNode
				if (hitObj)
				{
					if (hitObj->HitByJumpJetHandler)									// see if there is a handler
						(hitObj->HitByJumpJetHandler)(hitObj);							// call the handler
				}
			}
		}
	}

	return(killed);
}


/******************* DO PLAYER MOVEMENT AND COLLISION DETECT: BUBBLE *********************/
//
// OUTPUT: true if disabled or killed
//

static void DoPlayerMovementAndCollision_Bubble(ObjNode *theNode)
{
float				fps = gFramesPerSecondFrac;
OGLVector2D			aimVec,deltaVec, accVec;
OGLMatrix3x3		m;
static OGLPoint2D origin = {0,0};
Boolean				hit = false;
float				oldRadius;

				/*******************************/
				/* DO PLAYER-RELATIVE CONTROLS */
				/*******************************/

	if (gGamePrefs.playerRelControls)
	{
		float	r;

		r = theNode->Rot.y -= gPlayerInfo.analogControlX * fps * CONTROL_SENSITIVITY_PR_TURN;			// use x to rotate

		theNode->AccelVector.x = sin(r);
		theNode->AccelVector.y = cos(r);

		if (theNode->StatusBits & STATUS_BIT_ONGROUND)
		{
			gDelta.x += theNode->AccelVector.x * (CONTROL_SENSITIVITY_PR * gPlayerInfo.analogControlZ) * (1.1f - gPlayerSlipperyFactor);
			gDelta.z += theNode->AccelVector.y * (CONTROL_SENSITIVITY_PR * gPlayerInfo.analogControlZ) * (1.1f - gPlayerSlipperyFactor);
		}
		else
		{
			gDelta.x += theNode->AccelVector.x * (CONTROL_SENSITIVITY_AIR_PR * gPlayerInfo.analogControlZ);
			gDelta.z += theNode->AccelVector.y * (CONTROL_SENSITIVITY_AIR_PR * gPlayerInfo.analogControlZ);
		}

	}

				/*******************************/
				/* DO CAMERA-RELATIVE CONTROLS */
				/*******************************/

	else
	{


				/* ROTATE ANALOG ACCELERATION VECTOR BASED ON CAMERA POS & APPLY TO DELTA */

		OGLMatrix3x3_SetRotateAboutPoint(&m, &origin, gPlayerToCameraAngle);			// make a 2D rotation matrix camera-rel
		theNode->AccelVector.x = gPlayerInfo.analogControlX;
		theNode->AccelVector.y = gPlayerInfo.analogControlZ;
		OGLVector2D_Transform(&theNode->AccelVector, &m, &theNode->AccelVector);		// rotate the acceleration vector


					/* APPLY ACCELERATION TO DELTAS */

		gDelta.x += theNode->AccelVector.x * CONTROL_SENSITIVITY * (1.1f - gPlayerSlipperyFactor);
		gDelta.z += theNode->AccelVector.y * CONTROL_SENSITIVITY * (1.1f - gPlayerSlipperyFactor);

				/**********************************************************/
				/* TURN PLAYER TO AIM DIRECTION OF ACCELERATION OR MOTION */
				/**********************************************************/
				//
				// Depending on how slippery the terrain is, we aim toward the direction
				// of motion or the direction of acceleration.  We'll use the gPlayerSlipperyFactor value
				// to average an aim vector between the two.
				//

		FastNormalizeVector2D(gDelta.x, gDelta.z, &deltaVec, true);
		FastNormalizeVector2D(theNode->AccelVector.x, theNode->AccelVector.y, &accVec, true);

		aimVec.x = deltaVec.x * (1.0f - gPlayerSlipperyFactor) + (accVec.x * gPlayerSlipperyFactor);
		aimVec.y = deltaVec.y * (1.0f - gPlayerSlipperyFactor) + (accVec.y * gPlayerSlipperyFactor);

		TurnObjectTowardTarget(theNode, &gCoord, gCoord.x + aimVec.x, gCoord.z + aimVec.y, 8.0f, false);
	}

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

	if (theNode->Speed2D > MAX_BUBBLE_SPEED)						// see if limit top speed
	{
		float	tweak;

		tweak = theNode->Speed2D / MAX_BUBBLE_SPEED;

		gDelta.x /= tweak;
		gDelta.z /= tweak;

		theNode->Speed2D = MAX_BUBBLE_SPEED;
	}


			/* MOVE IT */

	gCoord.x += gDelta.x*fps;
	gCoord.y += gDelta.y*fps;
	gCoord.z += gDelta.z*fps;


			/******************************/
			/* DO OBJECT COLLISION DETECT */
			/******************************/
			//
			// for the bubble, we do only rudimentary collision to see
			// if we need to pop the bubble
			//

	if (DoSimpleBoxCollision(gCoord.y + gSoapBubble->TopOff, gCoord.y + gSoapBubble->BottomOff,
							 gCoord.x + gSoapBubble->LeftOff, gCoord.x + gSoapBubble->RightOff,
							 gCoord.z + gSoapBubble->FrontOff, gCoord.z + gSoapBubble->BackOff,
							CTYPE_MISC|CTYPE_ENEMY|CTYPE_TRIGGER))
	{
		hit = true;
	}

				/*************************/
				/* CHECK FENCE COLLISION */
				/*************************/

	oldRadius = theNode->BoundingSphereRadius;								// tweak the radius for this
	theNode->BoundingSphereRadius =  gSoapBubble->BBox.max.x;
	if (DoFenceCollision(theNode) || hit)
	{
		PopSoapBubble(gSoapBubble);
	}
	theNode->BoundingSphereRadius = oldRadius;

	gPlayerInfo.distToFloor = gCoord.y + theNode->BottomOff - GetTerrainY(gCoord.x, gCoord.z);
}


/************************ DO FRICTION & GRAVITY ****************************/
//
// Applies friction to the gDeltas
//

static void DoRobotFrictionAndGravity(ObjNode *theNode, float friction)
{
OGLVector2D	v;
float	x,z,fps;

	fps = gFramesPerSecondFrac;

			/**************/
			/* DO GRAVITY */
			/**************/

	gDelta.y -= gGravity*fps;					// add gravity

	if (gDelta.y < 0.0f)							// if falling, keep dy at least -1.0 to avoid collision jitter on platforms
		if (gDelta.y > (-20.0f * fps))
			gDelta.y = (-20.0f * fps);


			/***************/
			/* DO FRICTION */
			/***************/
			//
			// Dont do friction if player is pressing controls
			//

	if (!gGamePrefs.playerRelControls)
	{
		if (gPlayerInfo.analogControlX || gPlayerInfo.analogControlZ)	// if there is any player control then no friction
			return;
	}
	else										// with player relative controls, do heavier friction in some cases
	{
		switch(theNode->Skeleton->AnimNum)
		{
			case	PLAYER_ANIM_STAND:
			case	PLAYER_ANIM_WALK:
			case	PLAYER_ANIM_WALKWITHGUN:
			case	PLAYER_ANIM_STANDWITHGUN:
			case	PLAYER_ANIM_PUNCH:
			case	PLAYER_ANIM_PICKUPDEPOSIT:
			case	PLAYER_ANIM_PICKUPANDHOLDGUN:
					friction *= 2.0f;
					break;
		}
	}


	friction *= fps;							// adjust friction
	friction *= (1.0f - gPlayerSlipperyFactor);

	v.x = gDelta.x;
	v.y = gDelta.z;

	OGLVector2D_Normalize(&v, &v);				// get normalized motion vector
	x = -v.x * friction;						// make counter-motion vector
	z = -v.y * friction;

	if (gDelta.x < 0.0f)						// decelerate against vector
	{
		gDelta.x += x;
		if (gDelta.x > 0.0f)					// see if sign changed
			gDelta.x = 0;
	}
	else
	if (gDelta.x > 0.0f)
	{
		gDelta.x += x;
		if (gDelta.x < 0.0f)
			gDelta.x = 0;
	}

	if (gDelta.z < 0.0f)
	{
		gDelta.z += z;
		if (gDelta.z > 0.0f)
			gDelta.z = 0;
	}
	else
	if (gDelta.z > 0.0f)
	{
		gDelta.z += z;
		if (gDelta.z < 0.0f)
			gDelta.z = 0;
	}

	if ((gDelta.x == 0.0f) && (gDelta.z == 0.0f))
	{
		theNode->Speed2D = 0;
	}


}



/******************** DO ROBOT COLLISION DETECT **************************/
//
// Standard collision handler for player
//
// OUTPUT: true = disabled/killed
//

static Boolean DoRobotCollisionDetect(ObjNode *theNode, Boolean useBBoxForTerrain)
{
ObjNode		*hitObj;
uint32_t	ctype;
float		distToFloor, terrainY, fps = gFramesPerSecondFrac;
float		bottomOff;
Boolean		killed = false;

			/* DETERMINE CTYPE BITS TO CHECK FOR */

	ctype = PLAYER_COLLISION_CTYPE;

	gPreCollisionDelta = gDelta;									// remember delta before collision handler messes with it

	gPlayerSlipperyFactor = 0;									// assume normal friction

			/***************************************/
			/* AUTOMATICALLY HANDLE THE GOOD STUFF */
			/***************************************/
			//
			// this also sets the ONGROUND status bit if on a solid object.
			//

	if (useBBoxForTerrain)
		theNode->BottomOff = theNode->BBox.min.y;
	else
		theNode->BottomOff = gPlayerBottomOff;

	HandleCollisions(theNode, ctype, -.3);

			/* SCAN FOR INTERESTING STUFF */


	for (int i = 0; i < gNumCollisions; i++)
	{
		if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
		{
			hitObj = gCollisionList[i].objectPtr;				// get ObjNode of this collision

			if (hitObj->CType == INVALID_NODE_FLAG)				// see if has since become invalid
				continue;

			if (gCollisionList[i].sides & SIDE_BITS_BOTTOM)		// if bottom hit then set gPlayerSlipperyFactor from the object's friction value
				gPlayerSlipperyFactor = hitObj->Friction;


			ctype = hitObj->CType;								// get collision ctype from hit obj


			/* CHECK FOR TOTALLY IMPENETRABLE */

			if (ctype & CTYPE_IMPENETRABLE2)
			{
				if (!(gCollisionList[i].sides & SIDE_BITS_BOTTOM))	// dont do this if we landed on top of it
				{
					gCoord.x = theNode->OldCoord.x;					// dont take any chances, just move back to original safe place
					gCoord.z = theNode->OldCoord.z;
				}
			}

			/* CHECK FOR HURT ME */

			if (ctype & CTYPE_HURTME)
			{
				PlayerGotHit(hitObj, 0);

				if (ctype & CTYPE_FLAMING)							// see if hurter was flaming
					gPlayerInfo.burnTimer = 2.5;
			}
		}
	}

		/*************************************/
		/* CHECK & HANDLE TERRAIN  COLLISION */
		/*************************************/

	if (useBBoxForTerrain)
		bottomOff = theNode->BBox.min.y;							// use bbox for bottom
	else
		bottomOff = theNode->BottomOff;								// use collision box for bottom

	terrainY =  GetTerrainY(gCoord.x, gCoord.z);					// get terrain Y

	distToFloor = (gCoord.y + bottomOff) - terrainY;				// calc amount I'm above or under

	if (distToFloor <= 0.0f)										// see if on or under floor
	{
		gCoord.y = terrainY - bottomOff;
		gDelta.y = -200.0f;											// keep some downward momentum
		theNode->StatusBits |= STATUS_BIT_ONGROUND;
		gPlayerSlipperyFactor = gTileSlipperyFactor;				// use this slippery factor

		switch(gLevelNum)
		{
			case	LEVEL_NUM_BLOBBOSS:								// if touch blob boss ground, then hurt player and bounce back up
					PlayerLoseHealth(.25, PLAYER_DEATH_TYPE_EXPLODE);
					gDelta.y = 2500.0f;
					PlayEffect3D(EFFECT_SLIMEBOUNCE, &gCoord);
					break;

			case	LEVEL_NUM_CLOUD:								// see if touch electric floor
					if (gTileAttribFlags & (TILE_ATTRIB_ELECTROCUTE_AREA0|TILE_ATTRIB_ELECTROCUTE_AREA1))
					{
						if (gTileAttribFlags & TILE_ATTRIB_ELECTROCUTE_AREA0)	// check area #0
						{
							if (!gBumperCarGateBlown[0])
								PlayerTouchedElectricFloor(theNode);
						}
						else													// check area #1
						{
							if (!gBumperCarGateBlown[1])
								PlayerTouchedElectricFloor(theNode);
						}
					}
					break;
		}
	}


			/* DEAL WITH SLOPES */
			//
			// Using the floor normal here, apply some deltas to it.
			// Only apply slopes when on the ground (or really close to it)
			//

	if (ShouldApplySlopesToPlayer(distToFloor))
	{
		gDelta.x += gRecentTerrainNormal.x * (fps * PLAYER_SLOPE_ACCEL);
		gDelta.z += gRecentTerrainNormal.z * (fps * PLAYER_SLOPE_ACCEL);
	}

			/**************************/
			/* SEE IF IN WATER VOLUME */
			/**************************/

	if (!killed)
	{
		int		patchNum;
		Boolean	wasInWater;

					/* REMEMBER IF ALREADY IN WATER */

		if (theNode->StatusBits & STATUS_BIT_UNDERWATER)
			wasInWater = true;
		else
			wasInWater = false;

					/* CHECK IF IN WATER NOW */

		if (DoWaterCollisionDetect(theNode, gCoord.x, gCoord.y+theNode->BottomOff + 50.0f, gCoord.z, &patchNum))
		{
			gPlayerInfo.waterPatch = patchNum;

			gCoord.y = gWaterBBox[patchNum].max.y;

			switch(gWaterList[patchNum].type)
			{
					/* JUNGLE MUD */

				case	WATER_TYPE_MUD:
						ApplyFrictionToDeltas(300.0f, &gDelta);									// mud is viscous so slow player
						killed = PlayerLoseHealth(.3f * fps, PLAYER_DEATH_TYPE_DROWN);			// lose health
						MakeSteam(theNode, gCoord.x, gCoord.y, gCoord.z);
						break;

				case	WATER_TYPE_LAVA:
						ApplyFrictionToDeltas(2000.0f, &gDelta);								// lava is viscous so slow player
						killed = PlayerLoseHealth(.5f * fps, PLAYER_DEATH_TYPE_EXPLODE);		// lose health
						BurnFire(theNode, gCoord.x, gCoord.y, gCoord.z, true, PARTICLE_SObjType_Fire, 4.0, PARTICLE_FLAGS_ALLAIM);
						break;

						/* GOOD OLD ZAPPING H2O */
				default:
						if (!wasInWater)
							PlayerEntersWater(theNode, patchNum);
			}
		}
	}

	return(killed);
}


/***************** CHECK PUNCH COLLISION ********************/
//
//
// Handles fist collision with punchable objects
//

static void CheckPunchCollision(ObjNode *player)
{
static OGLPoint3D fistOff = {0,-12 ,0};
OGLPoint3D	fistCoord;
int			i;
ObjNode		*hitObj;
Boolean		endPunch;
float		fistSize = 30.0f * gPlayerInfo.scale;

	FindCoordOnJoint(player, PLAYER_JOINT_RIGHTHAND, &fistOff, &fistCoord);			// calc coord of fist

	if (DoSimpleBoxCollision(fistCoord.y + fistSize, fistCoord.y - fistSize,
							 fistCoord.x - fistSize, fistCoord.x + fistSize,
							 fistCoord.z + fistSize, fistCoord.z - fistSize,
							 CTYPE_MISC|CTYPE_ENEMY|CTYPE_TRIGGER|CTYPE_POWERUP))
	{
		for (i = 0; i < gNumCollisions; i++)										// affect all hit objects
		{
			hitObj = gCollisionList[i].objectPtr;									// get hit ObjNode
			if (hitObj)
			{
				if (hitObj->HitByWeaponHandler[WEAPON_TYPE_FIST])					// see if there is a handler for the punch
				{
					endPunch = (hitObj->HitByWeaponHandler[WEAPON_TYPE_FIST])(gPlayerInfo.rightHandObj, hitObj, &fistCoord, nil);	// call the handler
					if (endPunch)
					{
						player->PunchCanHurt = false;
						PlayEffect3D(EFFECT_PUNCHHIT, &fistCoord);
					}
				}
			}
		}
	}
}


#pragma mark -


/**************** CHECK PLAYER ACTION CONTROLS ***************/
//
// Checks for special action controls
//
// INPUT:	theNode = the node of the player
//

static void CheckPlayerActionControls(ObjNode *theNode)
{
		/* SEE IF DO CHEAT */

	if (GetCheatKeyCombo())
	{
		if (gPlayerInfo.lives < 3)
			gPlayerInfo.lives 	= 3;
		gPlayerInfo.health 	= 1.0;
		gPlayerInfo.fuel 	= 1.0;
		gPlayerInfo.jumpJet = 1.0;

		gPlayerInfo.weaponInventory[1].type = WEAPON_TYPE_STUNPULSE;
		gPlayerInfo.weaponInventory[2].type = WEAPON_TYPE_FREEZE;
		gPlayerInfo.weaponInventory[3].type = WEAPON_TYPE_FLAME;
		gPlayerInfo.weaponInventory[4].type = WEAPON_TYPE_GROWTH;
		gPlayerInfo.weaponInventory[5].type = WEAPON_TYPE_FLARE;
		gPlayerInfo.weaponInventory[6].type = WEAPON_TYPE_SUPERNOVA;
		gPlayerInfo.weaponInventory[7].type = WEAPON_TYPE_DART;

		for (int i = 1; i <= 7; i++)
			gPlayerInfo.weaponInventory[i].quantity = 99;

		gPlayerInfo.weaponInventory[4].quantity = 1;		// just one growth vial so we can test tossing it

		gPlayerInfo.didCheat = true;
	}

			/***************/
			/* SEE IF JUMP */
			/***************/

	if (GetNewNeedState(kNeed_Jump))										// see if user pressed the key
	{
		/* SEE IF ENTER CANNON ON CLOUD LEVEL */

		if (gLevelNum == LEVEL_NUM_CLOUD)
		{
			float	cannonTipX, cannonTipZ;
			gCannon = IsPlayerInPositionToEnterCannon(&cannonTipX, &cannonTipZ);		// see if in position for cannon

			if (gCannon != nil)
			{
				gCoord.x = cannonTipX;
				gCoord.z = cannonTipZ;
				theNode->Rot.y = gCannon->Rot.y + PI;
				MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_CLIMBINTO, 7.0);
				StartCannonFuse(gCannon);
				gCameraUserRotY = -PI/2;									// swing camera to side
				DisableHelpType(HELP_MESSAGE_INTOCANNON);
				goto no_jump;
			}
		}

			/* CHECK JUMP-JET */

		if ((theNode->Skeleton->AnimNum == PLAYER_ANIM_JUMP) ||
			(theNode->Skeleton->AnimNum == PLAYER_ANIM_FALL))			// if already jumping or falling then try Jump Jet
		{
			if (fabs(gDelta.y) < 800.0f)								// do it now if near apex of jump
				StartJumpJet(theNode);
			else
			if (theNode->Skeleton->AnimNum == PLAYER_ANIM_JUMP)			// not @ apex, but if already jumping then tag to do JJ @ apex
				gDoJumpJetAtApex = true;
			else
			if (gPlayerInfo.distToFloor > 200.0f)						// not @ apex, but if falling still allow JJ if high enough off ground
				StartJumpJet(theNode);
		}

			/* CHECK REGULAR JUMP */

		else
		if ((theNode->StatusBits & STATUS_BIT_ONGROUND)	|| (gPlayerInfo.distToFloor < 15.0f))		// must be on something solid
		{
			MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_JUMP, 5.0f);
			PlayEffect3D(EFFECT_JUMP, &gCoord);

			gDoJumpJetAtApex = false;

			gDelta.y += JUMP_DELTA;

			if (theNode->MPlatform != nil)			
			{
				if (theNode->MPlatform->CType & CTYPE_MPLATFORM_FREEJUMP)
				{
					theNode->MPlatform = nil;
				}
				else
				{
					// if jumping off of mplatform then also use platform's deltas
					gDelta.x += theNode->MPlatform->Delta.x;
					gDelta.y += theNode->MPlatform->Delta.y;
					gDelta.z += theNode->MPlatform->Delta.z;
				}
			}
		}
	}
no_jump:

		/******************/
		/* HANDLE WEAPONS */
		/******************/

	CheckPOWControls(theNode);
}


/******************* START JUMPJET *********************/

static void StartJumpJet(ObjNode *theNode)
{
	if (!gResetJumpJet)								// cannot do this until jump jet has been reset
		return;

	if (gPlayerInfo.jumpJet <= 0.0f)				// can't do it if out of jump jet fuel
	{
		gDoJumpJetAtApex = false;
		PlayEffect(EFFECT_NOJUMPJET);
		gJumpJetWarningCooldown = .5f;
		return;
	}

	gPlayerInfo.jumpJet -= .1f;						// dec fuel
	if (gPlayerInfo.jumpJet < EPS)					// avoid free shot when real close to 0 due to FP imprecision
		gPlayerInfo.jumpJet = 0.0f;

	gResetJumpJet = false;

			/* SET ANIM */

	if (gPlayerInfo.scaleRatio > 1.0f)				// do special version of player is giant
	{
		MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_GIANTJUMPJET, 6.0f);
		theNode->Skeleton->AnimSpeed = .6;
	}
	else
		MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_JUMPJET, 6.0f);


			/* SET SOME VARIABLES */

	gDelta.y = 0;
	gPlayerInfo.jumpJetSpeed = 0;
	theNode->JumpJetEnginesOff = false;


			/* START PARTICLES */

	gPlayerInfo.jjParticleGroup[0] = -1;			// white sparks
	gPlayerInfo.jjParticleMagic[0]	= 0;
	gPlayerInfo.jjParticleTimer[0]	= 0;

	gPlayerInfo.jjParticleGroup[1] = -1;			// smoke
	gPlayerInfo.jjParticleMagic[1]	= 0;
	gPlayerInfo.jjParticleTimer[1]	= 0;



		/* CREATE SPARKLE */

	CreatePlayerJumpJetSparkles(theNode);


	PlayEffect3D(EFFECT_JUMPJET, &gCoord);
}

/**************** END JUMPJET *****************/

static void EndJumpJet(ObjNode *theNode)
{
	DeleteSparkle(theNode->Sparkles[PLAYER_SPARKLE_LEFTFOOT]);							// delete sparkles on feet
	theNode->Sparkles[PLAYER_SPARKLE_LEFTFOOT] = -1;
	DeleteSparkle(theNode->Sparkles[PLAYER_SPARKLE_RIGHTFOOT]);
	theNode->Sparkles[PLAYER_SPARKLE_RIGHTFOOT] = -1;
}


/****************** UPDATE JUMP JET PARTICLES ********************/

static void UpdateJumpJetParticles(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;
int					i;
float				x,y,z;
static const OGLPoint3D		particleBaseOff = {0,-100,0};
OGLPoint3D			particleBase;

				/* CALC FOOT POINT */

	FindCoordOnJoint(theNode, PLAYER_JOINT_BASE, &particleBaseOff, &particleBase);
	x = particleBase.x;
	y = particleBase.y;
	z = particleBase.z;


			/*********************/
			/* MAKE WHITE SPARKS */
			/*********************/

	gPlayerInfo.jjParticleTimer[0] -= fps;											// see if add particles
	if (gPlayerInfo.jjParticleTimer[0] <= 0.0f)
	{
		gPlayerInfo.jjParticleTimer[0] += .03f;									// reset timer

		particleGroup 	= gPlayerInfo.jjParticleGroup[0];
		magicNum 		= gPlayerInfo.jjParticleMagic[0];

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			gPlayerInfo.jjParticleMagic[0] = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= 800;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 20.0f * gPlayerInfo.scaleRatio;
			groupDef.decayRate				= .8;
			groupDef.fadeRate				= 1.4;
			groupDef.particleTextureNum		= PARTICLE_SObjType_WhiteSpark2;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			gPlayerInfo.jjParticleGroup[0] = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			for (i = 0; i < 7; i++)
			{
				p.x = x + RandomFloat2() * 30.0f;
				p.y = y + RandomFloat2() * 30.0f;
				p.z = z + RandomFloat2() * 30.0f;

				d.x = RandomFloat2() * 140.0f;
				d.y = RandomFloat2() * 140.0f;
				d.z = RandomFloat2() * 140.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= 1.0f;
				newParticleDef.rotZ			= 0;
				newParticleDef.rotDZ		= 0;
				newParticleDef.alpha		= 1.0;
				if (AddParticleToGroup(&newParticleDef))
				{
					gPlayerInfo.jjParticleGroup[0] = -1;
					break;
				}
			}
		}
	}


			/**************/
			/* MAKE SMOKE */
			/**************/

	gPlayerInfo.jjParticleTimer[1] -= fps;											// see if add particles
	if (gPlayerInfo.jjParticleTimer[1] <= 0.0f)
	{
		gPlayerInfo.jjParticleTimer[1] += .05f;									// reset timer

		particleGroup 	= gPlayerInfo.jjParticleGroup[1];
		magicNum 		= gPlayerInfo.jjParticleMagic[1];

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			gPlayerInfo.jjParticleMagic[1] = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= 0;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 20.0f * gPlayerInfo.scaleRatio;
			groupDef.decayRate				= -.2;
			groupDef.fadeRate				= .7;
			groupDef.particleTextureNum		= PARTICLE_SObjType_GreySmoke;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE_MINUS_SRC_ALPHA;
			gPlayerInfo.jjParticleGroup[1] = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			for (i = 0; i < 8; i++)
			{
				p.x = x + RandomFloat2() * 40.0f;
				p.y = y + RandomFloat2() * 40.0f;
				p.z = z + RandomFloat2() * 40.0f;

				d.x = RandomFloat2() * 40.0f;
				d.y = RandomFloat2() * 40.0f;
				d.z = RandomFloat2() * 40.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat() * PI2;
				newParticleDef.rotDZ		= RandomFloat2();
				newParticleDef.alpha		= .4f + RandomFloat() * .3f;
				if (AddParticleToGroup(&newParticleDef))
				{
					gPlayerInfo.jjParticleGroup[1] = -1;
					break;
				}
			}
		}
	}

}


#pragma mark -

/*********************** SET PLAYER WALK ANIM *******************************/

void SetPlayerWalkAnim(ObjNode *theNode)
{
	if (gPlayerInfo.holdingGun)
		MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_WALKWITHGUN, 9);
	else
		MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_WALK, 9);
}


/********************* IS PLAYER DOING WALK ANIM *********************/

static Boolean IsPlayerDoingWalkAnim(ObjNode *theNode)
{
	if ((theNode->Skeleton->AnimNum == PLAYER_ANIM_WALK) ||
		(theNode->Skeleton->AnimNum == PLAYER_ANIM_WALKWITHGUN))
	{
		return(true);
	}


	return(false);
}

/*********************** SET PLAYER STAND ANIM *******************************/

void SetPlayerStandAnim(ObjNode *theNode, float speed)
{
	if (gPlayerInfo.holdingGun)
		MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_STANDWITHGUN, speed);
	else
		MorphToSkeletonAnim(theNode->Skeleton, PLAYER_ANIM_STAND, speed);
}


/********************* IS PLAYER DOING STAND ANIM *********************/

static Boolean IsPlayerDoingStandAnim(ObjNode *theNode)
{
	if ((theNode->Skeleton->AnimNum == PLAYER_ANIM_STAND) ||
		(theNode->Skeleton->AnimNum == PLAYER_ANIM_STANDWITHGUN))
	{
		return(true);
	}


	return(false);
}


/********************** START WEAPON CHANGE ANIM ***********************/

void StartWeaponChangeAnim(void)
{




}


#pragma mark -


/**************** DO PLAYER MAGNET SKIING ****************************/

static void DoPlayerMagnetSkiing(ObjNode *player)
{
short		magnetMonsterID;
ObjNode		*magMonster;
float		distToTarget,mx,mz,tx,tz,y;
OGLVector2D	v;
float		fps = gFramesPerSecondFrac;
OGLMatrix3x3	m;


			/* GET INFO ABOUT THE MAGNET MONSTER */

	magnetMonsterID = gTargetPickup->MagnetMonsterID;		// get ID# of the magnet so we know which monster to go to
	magMonster = gMagnetMonsterList[magnetMonsterID];		// get objNode of magnet monster

	magMonster->MagnetMonsterMoving = true;					// make sure the monster is moving

	mx = magMonster->Coord.x;
	mz = magMonster->Coord.z;

			/* KEEP PLAYER AIMED AT IT */

	TurnObjectTowardTarget(player, &gCoord, mx, mz, 11.0, false);




			/*******************/
			/* CALC AIM VECTOR */
			/*******************/

		/* CALC VEC FROM MONSTER TO PLAYER */

	v.x = gCoord.x - mx;									// calc vector from monster to player
	v.y = gCoord.z - mz;
	FastNormalizeVector2D(v.x, v.y, &v, true);


			/* ROTATE BASED ON PLAYER INPUT */

	OGLMatrix3x3_SetRotate(&m, gPlayerInfo.analogControlX * .5f);
	OGLVector2D_Transform(&v, &m, &v);



			/* CALC ACCELERATION */

	tx = mx + v.x * TARGET_SKIING_DIST;						// calc target point where we'd like to be
	tz = mz + v.y * TARGET_SKIING_DIST;

	distToTarget = CalcDistance(gCoord.x, gCoord.z,  tx, tz);							// calc dist to target point
	if (distToTarget > 2000.0f)								// limit it
		distToTarget = 2000.0f;


			/***********/
			/* MOVE IT */
			/***********/

	gDelta.x = distToTarget * -v.x * 1.5f;
	gDelta.z = distToTarget * -v.y * 1.5f;
	gDelta.y -= gGravity * fps;						// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/***********************/
			/* SEE IF HIT ANYTHING */
			/***********************/

			/* SEE IF HIT WATER */

	if (GetWaterY(gCoord.x, gCoord.z, &y))					// get water y
	{
		float	playerBottomY = gCoord.y + player->BBox.min.y + 10.0f;
		float	diff;

		diff = y - playerBottomY;

		if (diff >= 0.0f)								// see if hit water
		{
			gDelta.y += 7000.0f * fps;					// thrust up

			if (diff > 30.0f)							// see if too far under
				gCoord.y += diff - 30.0f;				// adjust back up

			gCoord.y += gDelta.y * fps;

			SprayWater(player, gCoord.x, y, gCoord.z);
		}
	}

			/* SEE IF HIT GROUND */

	y = GetTerrainY(gCoord.x, gCoord.z);					// get terrain y
	if ((gCoord.y + player->BBox.min.y) <= y)				// see if hit ground
	{
		gCoord.y = y - player->BBox.min.y;

		if (gDelta.y < 0.0f)
			gDelta.y *= -.5f;
	}


			/* SEE IF HIT SOLID OBJECT HARD */

	if (HandleCollisions(player, CTYPE_MISC, -.6) || DoFenceCollision(player))
	{
		if (player->Speed2D > 500.0f)											// see if hit hard
		{
			EndMagnetSkiing(player, true);										// end skiing
		}
	}

			/***********************/
			/* SEE IF ABORT SKIING */
			/***********************/

	if (gTargetPickup != nil)															// see if already aborted via collision
	{
		if (GetNewNeedState(kNeed_Jump)
			|| GetNewNeedState(kNeed_Shoot)
			|| GetNewNeedState(kNeed_PunchPickup))
		{
			DisableHelpType(HELP_MESSAGE_LETGOMAGNET);							// player has figured it out, so don't show this anymore

			EndMagnetSkiing(player, false);										// end skiing
			gDelta.y = 2000.0f;													// jump up a little
		}
	}
}


/******************** END MAGNET SKIING **************************/

static void EndMagnetSkiing(ObjNode *player, Boolean crash)
{
short		magnetMonsterID;
ObjNode		*magMonster;

	if (gTargetPickup == nil)					// this can get called twice, so check if pickup==nil
		return;

			/* GET INFO ABOUT THE MAGNET MONSTER */

	magnetMonsterID = gTargetPickup->MagnetMonsterID;		// get ID# of the magnet so we know which monster to go to
	magMonster = gMagnetMonsterList[magnetMonsterID];		// get objNode of magnet monster

	magMonster->MagnetMonsterResetToStart = true;			// send monster back to the start

	gTargetPickup->MoveCall = MoveMagnetAfterDrop;
	gTargetPickup->Delta.x = gDelta.x * .8f;
	gTargetPickup->Delta.y = 300.0f;
	gTargetPickup->Delta.z = gDelta.z * .8f;

	if (crash)
	{
		KillPlayer(PLAYER_DEATH_TYPE_EXPLODE);
	}
	else
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_FALL, 6);

	gTargetPickup = nil;										// no longer a target
}



/******************** SHOULD APPLY SLOPES **************************/

static Boolean ShouldApplySlopesToPlayer(float newDistToFloor)
{
	switch (gPlayerInfo.objNode->Skeleton->AnimNum)
	{
		case PLAYER_ANIM_WALK:
		case PLAYER_ANIM_WALKWITHGUN:
		case PLAYER_ANIM_BELLYSLIDE:
		case PLAYER_ANIM_THROWN:
			// allow slopes for these
			break;

		default:
			// don't apply slopes this while standing
			// otherwise some weird rotation may occur on steep slopes at high framerates
			return false;
	}

	// only do slopes if player isn't controlling
	if ((fabs(gPlayerInfo.analogControlX) >= 0.5f)
		|| (fabs(gPlayerInfo.analogControlZ) >= 0.5f))
	{
		return false;
	}

	if ((newDistToFloor > 0.0f) && (gPlayerInfo.distToFloor >= 15.0f))
	{
		return false;
	}

	return true;
}

