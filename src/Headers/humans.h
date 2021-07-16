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

ObjNode* MakeHuman(int humanType, float x, float z);
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

Boolean AddPeopleHut(TerrainItemEntryType *itemPtr, long x, long z);

Boolean AddHumanScientist(TerrainItemEntryType *itemPtr, long x, long z);
Boolean PrimeHumanScientist(long splineNum, SplineItemType *itemPtr);

