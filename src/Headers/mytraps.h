//
// mytraps.h
//


#define	MAX_MAGNET_MONSTERS				5

#define	MagnetMonsterID					Special[0]
#define	MagnetMonsterMoving				Flag[0]
#define	MagnetMonsterResetToStart		Flag[1]
#define	MagnetMonsterWatchPlayerRespawn	Flag[2]
#define	MagnetMonsterSpeed				SpecialF[0]
#define	MagnetMonsterInitialPlacement	SpecialF[1]


Boolean AddFallingCrystal(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddInertBubble(TerrainItemEntryType *itemPtr, long  x, long z);
void PopSoapBubble(ObjNode *bubble);
Boolean AddBubblePump(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_BubblePump(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);

Boolean PrimeMagnetMonster(long splineNum, SplineItemType *itemPtr);

Boolean AddCrunchDoor(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddManhole(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddProximityMine(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddSnowball(TerrainItemEntryType *itemPtr, long  x, long z);


		/* SPACE PODS */
		
Boolean AddSpacePodGenerator(TerrainItemEntryType *itemPtr, long  x, long z);
void InitSpacePods(void);


		/* LAVA */
		
Boolean AddLavaPillar(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddVolcanoGeneratorZone(TerrainItemEntryType *itemPtr, long  x, long z);
void ExplodeVolcanoTop(float x, float z);
void SpewALavaBoulder(OGLPoint3D *p, Boolean fromFlamester);
Boolean AddLavaPlatform(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_LavaPlatform(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
Boolean PrimeLavaPlatform(long splineNum, SplineItemType *itemPtr);


	/* TRAPS 2 */
	
Boolean AddBeemer(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean PrimeRailGun(long splineNum, SplineItemType *itemPtr);
Boolean AddTurret(TerrainItemEntryType *itemPtr, long  x, long z);
