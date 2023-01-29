//
// enemy.h
//

#include "terrain.h"
#include "splineitems.h"

#define	DEFAULT_ENEMY_COLLISION_CTYPES	(CTYPE_MISC|CTYPE_HURTENEMY|CTYPE_ENEMY|CTYPE_TRIGGER2|CTYPE_FENCE|CTYPE_PLAYER)
#define	DEATH_ENEMY_COLLISION_CTYPES	(CTYPE_MISC|CTYPE_ENEMY|CTYPE_FENCE)

#define ENEMY_GRAVITY		3500.0f
#define	ENEMY_SLOPE_ACCEL		3000.0f

#define	EnemyRegenerate	Flag[3]


		/* ENEMY KIND */
		
enum
{
	ENEMY_KIND_BRAINALIEN = 0,
	ENEMY_KIND_ONION,
	ENEMY_KIND_CORN,
	ENEMY_KIND_TOMATO,
	ENEMY_KIND_BLOB,
	ENEMY_KIND_SQUOOSHY,
	ENEMY_KIND_FLAMESTER,
	ENEMY_KIND_GIANTLIZARD,
	ENEMY_KIND_FLYTRAP,
	ENEMY_KIND_MANTIS,
	ENEMY_KIND_MUTANT,
	ENEMY_KIND_MUTANTROBOT,
	ENEMY_KIND_CLOWN,
	ENEMY_KIND_CLOWNFISH,
	ENEMY_KIND_STRONGMAN,
	ENEMY_KIND_ROBOBLADE,
	ENEMY_KIND_ICECUBE,
	ENEMY_KIND_JAWSBOT,
	ENEMY_KIND_HAMMERBOT,
	ENEMY_KIND_DRILLBOT,
	ENEMY_KIND_SWINGERBOT,
	ENEMY_KIND_ELITEBRAINALIEN,
	
	NUM_ENEMY_KINDS
};




		/* SAUCER INFO */
		
#define	SaucerTargetType	Special[0]
			
enum
{
	SAUCER_TARGET_TYPE_ABDUCT,
	SAUCER_TARGET_TYPE_RADIATE
};


//=====================================================================
//=====================================================================
//=====================================================================


			/* ENEMY */
			
ObjNode *MakeEnemySkeleton(Byte skeletonType, float x, float z, float scale, float rot, void (*moveCall)(ObjNode*));
extern	void DeleteEnemy(ObjNode *theEnemy);
Boolean DoEnemyCollisionDetect(ObjNode *theEnemy, uint32_t ctype, Boolean useBBoxBottom);
extern	void UpdateEnemy(ObjNode *theNode);
extern	void InitEnemyManager(void);
void MoveEnemy(ObjNode *theNode);
void DetachEnemyFromSpline(ObjNode *theNode, void (*moveCall)(ObjNode*));
ObjNode *FindClosestEnemy(OGLPoint3D *pt, float *dist);
Boolean	IsBottomlessPitInFrontOfEnemy(float r);
void MoveEnemyRobotChunk(ObjNode *chunk);

			/* SAUCER */
			
Boolean CallAlienSaucer(ObjNode *who);


		/* BRAIN ALIEN */
		
Boolean PrimeEnemy_BrainAlien(long splineNum, SplineItemType *itemPtr);
Boolean AddEnemy_BrainAlien(TerrainItemEntryType *itemPtr, long x, long z);
void CreateBrainAlienGlow(ObjNode *alien);
void UpdateBrainAlienGlow(ObjNode *alien);
ObjNode *MakeBrainAlien(float x, float z);
void MoveBrainAlien(ObjNode *theNode);
Boolean BrainAlienHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
Boolean BrainAlienHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
Boolean BrainAlienGotPunched(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta);
Boolean BrainAlienHitByFreeze(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
Boolean HurtBrainAlien(ObjNode *enemy, float damage);
void BrainAlienHitByJumpJet(ObjNode *enemy);
void MoveEliteBrainAlien_FromPortal(ObjNode *theNode);


		/* ONION */

void GrowOnion(float x, float z);
Boolean AddEnemy_Onion(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_Onion(long splineNum, SplineItemType *itemPtr);

		/* CORN */

Boolean AddEnemy_Corn(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_Corn(long splineNum, SplineItemType *itemPtr);
Boolean DoTrig_CornKernel(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
void GrowCorn(float x, float z);
void MovePopcorn(ObjNode *theNode);


		/* TOMATO */

void GrowTomato(float x, float z);
Boolean AddEnemy_Tomato(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_Tomato(long splineNum, SplineItemType *itemPtr);

		/* TRACTOR */
		
Boolean AddTractor(TerrainItemEntryType *itemPtr, long  x, long z);
void StopTractor(ObjNode* theNode);


		/* BLOB */
		
Boolean AddEnemy_Blob(TerrainItemEntryType *itemPtr, long x, long z);
void MoveBlobChunk_Thawed(ObjNode *theNode);
void MoveBlobChunk(ObjNode *theNode);


		/* BLOB BOSS */
		
void MakeBlobBossMachine(void);

	/* SQUOOSHY */
	
Boolean AddEnemy_Squooshy(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_Squooshy(long splineNum, SplineItemType *itemPtr);


	/* FLAMESTER */
	
Boolean AddEnemy_Flamester(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_Flamester(long splineNum, SplineItemType *itemPtr);


	/* GIANT LIZARD */
	
Boolean AddEnemy_GiantLizard(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_GiantLizard(long splineNum, SplineItemType *itemPtr);


		/* VENUS FLYTRAP */
		
Boolean AddEnemy_FlyTrap(TerrainItemEntryType *itemPtr, long x, long z);


		/* MANTIS */
		
Boolean AddEnemy_Mantis(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_Mantis(long splineNum, SplineItemType *itemPtr);


		/* MUTANTS */
		
void GrowMutant(float x, float z);
Boolean AddEnemy_Mutant(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_Mutant(long splineNum, SplineItemType *itemPtr);

Boolean AddEnemy_MutantRobot(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_MutantRobot(long splineNum, SplineItemType *itemPtr);


		/* PITCHER PLANT BOSS */
		
void InitJungleBossStuff(void);
Boolean AddTentacleGenerator(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddPitcherPlantBoss(TerrainItemEntryType *itemPtr, long x, long z);
Boolean AddPitcherPod(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddTractorBeamPost(TerrainItemEntryType *itemPtr, long  x, long z);


		/* CLOWN */
		
Boolean AddEnemy_Clown(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_Clown(long splineNum, SplineItemType *itemPtr);


Boolean PrimeEnemy_ClownFish(long splineNum, SplineItemType *itemPtr);


		/* STRONGMAN */
		
Boolean AddEnemy_StrongMan(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_StrongMan(long splineNum, SplineItemType *itemPtr);


		/* ROBO BLADE */

Boolean AddRoboBlade(TerrainItemEntryType *itemPtr, long  x, long z);


		/* ICE CUBE */
		
Boolean AddEnemy_IceCube(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeEnemy_IceCube(long splineNum, SplineItemType *itemPtr);


		/* BOTS */
		
Boolean AddJawsBot(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean PrimeEnemy_JawsBot(long splineNum, SplineItemType *itemPtr);

Boolean AddHammerBot(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean PrimeEnemy_HammerBot(long splineNum, SplineItemType *itemPtr);

Boolean AddDrillBot(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean PrimeEnemy_DrillBot(long splineNum, SplineItemType *itemPtr);

Boolean AddSwingerBot(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean PrimeEnemy_SwingerBot(long splineNum, SplineItemType *itemPtr);


	/* BRAIN BOSS */
	
void InitBrainBoss(void);
Boolean AddEnemy_BrainBoss(TerrainItemEntryType *itemPtr, long x, long z);
Boolean AddBrainPort(TerrainItemEntryType *itemPtr, long  x, long z);



