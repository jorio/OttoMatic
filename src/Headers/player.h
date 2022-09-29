//
// player.h
//

#ifndef __PLAYER_H__
#define __PLAYER_H__




#define	PLAYER_DEFAULT_SCALE	1.2f
#define	PLAYER_GIANT_SCALE		(PLAYER_DEFAULT_SCALE * 3.0f)
#define	DEFAULT_PLAYER_SHADOW_SCALE	3.7f


#define	InvincibleTimer	SpecialF[4]				// timer for invicibility after being hurt

#define	PunchCanHurt	Flag[0]					// set by anim when punch can actually hurt

#define	IsHoldingPickup	Flag[0]					// set by anim when pickup is ready to grab
#define	ChangeWeapon	Flag[0]					// set by anim when weapon should change
#define	ThrowDartNow	Flag[0]					// set by anim when should release dart

#define	ZappedTimer		SpecialF[0]				// time to do zap anim
#define	AccordianTimer	SpecialF[0]				// duration of accordian

#define	PLAYER_COLLISION_CTYPE	(CTYPE_MISC|CTYPE_TRIGGER|CTYPE_FENCE|CTYPE_ENEMY|CTYPE_HURTME)

enum
{
	ROCKET_KIND_ENTER,
	ROCKET_KIND_EXIT
};

enum
{
	ROCKET_MODE_LANDING,
	ROCKET_MODE_OPENDOOR,
	ROCKET_MODE_DEPLANE,
	ROCKET_MODE_CLOSEDOOR,
	ROCKET_MODE_LEAVE,
	ROCKET_MODE_WAITING,
	ROCKET_MODE_WAITING2,
	ROCKET_MODE_ENTER
};


enum
{
	GROWTH_MODE_NONE,
	GROWTH_MODE_GROW,
	GROWTH_MODE_SHRINK
};


enum
{
	PLAYER_DEATH_TYPE_DROWN,
	PLAYER_DEATH_TYPE_EXPLODE,
	PLAYER_DEATH_TYPE_FALL,
	PLAYER_DEATH_TYPE_SAUCERBOOM

};

enum
{
	BEAM_MODE_TELEPORT,
	BEAM_MODE_DESTRUCTO,
	BEAM_MODE_DROPZONE
};



enum
{
	PLAYER_ANIM_STAND,
	PLAYER_ANIM_WALK,
	PLAYER_ANIM_WALKWITHGUN,
	PLAYER_ANIM_STANDWITHGUN,
	PLAYER_ANIM_PUNCH,
	PLAYER_ANIM_PICKUPDEPOSIT,
	PLAYER_ANIM_PICKUPANDHOLDGUN,
	PLAYER_ANIM_GRABBED,
	PLAYER_ANIM_FLATTENED,
	PLAYER_ANIM_CHARGING,
	PLAYER_ANIM_JUMPJET,
	PLAYER_ANIM_JUMP,
	PLAYER_ANIM_FALL,
	PLAYER_ANIM_ZAPPED,
	PLAYER_ANIM_DROWNED,
	PLAYER_ANIM_GOTHIT,
	PLAYER_ANIM_ACCORDIAN,
	PLAYER_ANIM_CHANGEWEAPON,
	PLAYER_ANIM_BUBBLE,
	PLAYER_ANIM_PICKUPANDHOLDMAGNET,
	PLAYER_ANIM_SUCKEDINTOWELL,
	PLAYER_ANIM_GIANTJUMPJET,
	PLAYER_ANIM_THROWN,
	PLAYER_ANIM_GRABBED2,
	PLAYER_ANIM_DRINK,
	PLAYER_ANIM_RIDEZIP,
	PLAYER_ANIM_CLIMBINTO,
	PLAYER_ANIM_SHOOTFROMCANNON,
	PLAYER_ANIM_BUMPERCAR,
	PLAYER_ANIM_GRABBEDBYSTRONGMAN,
	PLAYER_ANIM_THROW,
	PLAYER_ANIM_ROCKETSLED,
	PLAYER_ANIM_SITONLEDGE,
	PLAYER_ANIM_DRILLED,
	PLAYER_ANIM_BELLYSLIDE
};


enum
{
	PLAYER_JOINT_BASE = 0,
	PLAYER_JOINT_TORSO = 1,
	PLAYER_JOINT_RIGHTFOOT = 6,
	PLAYER_JOINT_LEFTFOOT = 7,
	PLAYER_JOINT_RIGHTHAND = 12,
	PLAYER_JOINT_LEFTHAND = 15,
	PLAYER_JOINT_HEAD = 9
};

enum
{
	PLAYER_SPARKLE_CHEST = 0,
	PLAYER_SPARKLE_LEFTFOOT,
	PLAYER_SPARKLE_RIGHTFOOT,
	PLAYER_SPARKLE_MUZZLEFLASH

};

#define	MAX_INVENTORY_SLOTS	NUM_WEAPON_TYPES
#define	NO_INVENTORY_HERE	-1					// when inventory slot is empty

typedef struct
{
	short	type;
	short	quantity;
}WeaponInventoryType;


		/***************/
		/* PLAYER INFO */
		/***************/
		
typedef struct
{
	int					startX,startZ;
	float				startRotY;
	
	OGLPoint3D			coord;
	ObjNode				*objNode;
	ObjNode				*leftHandObj, *rightHandObj;
		
	float				distToFloor;
	float				mostRecentFloorY;
		
	float				knockDownTimer;
	float				invincibilityTimer;
	
	OGLRect				itemDeleteWindow;
	
	OGLCameraPlacement	camera;
	
	short				snowParticleGroup;
	float				snowTimer;
	
	float				burnTimer;
		
			/* JUMP JET */
			
	float				jumpJetSpeed;
	float				jjParticleTimer[2];
	short				jjParticleGroup[2];
	short				jjParticleMagic[2];
	
	
		/* TILE ATTRIBUTE PHYSICS TWEAKS */			
	
	
	float				groundTraction;
	float				groundFriction;
	float				groundAcceleration;
			
	int					waterPatch;
	
	
			/* CONTROL INFO */
			
	float				analogControlX,analogControlZ;
	
	float				autoAimTimer;					// set when want player to auto-turn to aim at something
	float				autoAimTargetX;
	float				autoAimTargetZ;
	
			/* INVENTORY INFO */
			
	Byte				lives;
	float				health;
	float				fuel;
	float				jumpJet;
	
	WeaponInventoryType	weaponInventory[MAX_INVENTORY_SLOTS];	// quantity for each type of Weapon in inventory
	short				currentWeaponType,oldWeaponType;
	Boolean				holdingGun,wasHoldingGun;
	
	Byte				growMode;
	float				giantTimer;
	float				scale;								// player's scale can vary with the growth powerup
	float				scaleRatio;
				
	float				superNovaCharge;
	ObjNode				*superNovaStatic;
				
}PlayerInfoType;


#define	MAX_SUPERNOVA_DISCHARGES	(MAX_NODE_SPARKLES-1)
#define	MAX_SUPERNOVA_DIST			3000.0f


//=======================================================


extern	PlayerInfoType	gPlayerInfo;


void InitPlayerInfo_Game(void);
void InitPlayersAtStartOfLevel(void);
Boolean PlayerLoseHealth(float damage, Byte deathType);
void PlayerEntersWater(ObjNode *theNode, int patchNum);
void PlayerGotHit(ObjNode *byWhat, float altDamage);
Boolean AddExitRocket(TerrainItemEntryType *itemPtr, long  x, long z);
void DrawRocketFlame(ObjNode *theNode);
void MakeRocketExhaust(ObjNode *rocket);
void KillPlayer(Byte deathType);
void ResetPlayerAtBestCheckpoint(void);
void CrushPlayer(void);
void HidePlayer(ObjNode *player);
void MakeRocketFlame(ObjNode *rocket);


void MovePlayer_Robot(ObjNode *theNode);
void InitPlayer_Robot(OGLPoint3D *where, float rotY);
void UpdatePlayerMotionBlur(ObjNode *theNode);
void UpdateRobotHands(ObjNode *theNode);
void UpdatePlayer_Robot(ObjNode *theNode);
void SetPlayerStandAnim(ObjNode *theNode, float speed);
void SetPlayerWalkAnim(ObjNode *theNode);
void StartWeaponChangeAnim(void);


		/* WEAPON */

void InitWeaponInventory(void);		
void CheckWeaponChangeControls(ObjNode *theNode);
void CheckPOWControls(ObjNode *theNode);
Boolean DischargeSuperNova(void);
short CreateSuperNovaStatic(ObjNode *novaObj, float x, float y, float z, int maxDischarges, float maxDist);
void DrawSuperNovaDischarge(ObjNode *theNode);
void IncWeaponQuantity(short weaponType, short amount);
void DecWeaponQuantity(short weaponType);
short FindWeaponInventoryIndex(short weaponType);
void ExplodeStunPulse(ObjNode *theNode);
void ThrowDart(ObjNode *theNode);


		/* SAUCER */
		
void InitPlayer_Saucer(OGLPoint3D *where);
void MoveObjectUnderPlayerSaucer(ObjNode *theNode);
float SeeIfUnderPlayerSaucerShadow(ObjNode *theNode);
void BlowUpSaucer(ObjNode *saucer);
void StopPlayerSaucerBeam(ObjNode *saucer);
void ImpactPlayerSaucer(float x, float z, float damage, ObjNode *saucer, short particleLimit);


		


#endif