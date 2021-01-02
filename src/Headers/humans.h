//
// humans.h
//

enum
{
	HUMAN_TYPE_FARMER = 0,
	HUMAN_TYPE_BEEWOMAN,
	HUMAN_TYPE_SCIENTIST,
	HUMAN_TYPE_SKIRTLADY,
	
	NUM_HUMAN_TYPES
};

#define	HumanWalkTimer	SpecialF[0]
#define	HumanType		Special[2]
#define	InIce			Flag[3]

void InitHumans(void);

Boolean AddHuman(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeHuman(long splineNum, SplineItemType *itemPtr);
void DoHumanCollisionDetect(ObjNode *theNode);
void StartHumanTeleport(ObjNode *human);
Boolean UpdateHumanTeleport(ObjNode *human, Boolean fadeOut);
Boolean DoTrig_Human(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
void UpdateHuman(ObjNode *theNode);
void CheckHumanHelp(ObjNode *theNode);
void EncaseHumanInIce(ObjNode *human);
void MoveHuman_ToPlayerSaucer(ObjNode *theNode);
void MoveHuman_BeamDown(ObjNode *theNode);


Boolean AddFarmer(TerrainItemEntryType *itemPtr, long x, long z);
ObjNode *MakeFarmer(float x, float z);
Boolean PrimeFarmer(long splineNum, SplineItemType *itemPtr);


Boolean AddBeeWoman(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeBeeWoman(long splineNum, SplineItemType *itemPtr);
ObjNode *MakeBeeWoman(float x, float z);

Boolean AddScientist(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeScientist(long splineNum, SplineItemType *itemPtr);
ObjNode *MakeScientist(float x, float z);

Boolean AddSkirtLady(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeSkirtLady(long splineNum, SplineItemType *itemPtr);
ObjNode *MakeSkirtLady(float x, float z);

Boolean AddPeopleHut(TerrainItemEntryType *itemPtr, long x, long z);

