//
// items.h
//

typedef struct
{
	Boolean	isWeapon;

}POWInfoType;


extern	void InitItemsManager(void);
void CreateCyclorama(void);

Boolean AddNeuronStrand(TerrainItemEntryType *itemPtr, long  x, long z);

	/* farm */
	
Boolean AddBasicPlant(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddPlatform(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddBarn(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddSilo(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddSprout(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddCornStalk(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddBigLeafPlant(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddFencePost(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddPhonePole(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddWindmill(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddMetalTub(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddOutHouse(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddRock(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddHay(TerrainItemEntryType *itemPtr, long  x, long z);


/* slime level */

Boolean AddSlimePipe(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddBasicCrystal(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddSlimeTree(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddSlimeMech(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean PrimeMovingPlatform(long splineNum, SplineItemType *itemPtr);

Boolean AddBlobBossTube(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddBlobArrow(TerrainItemEntryType *itemPtr, long  x, long z);


	/* CLOUD */
	
Boolean AddScaffoldingPost(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddCloudTunnel(TerrainItemEntryType *itemPtr, long  x, long z);



	/* TELEPORTER */
	
Boolean AddTeleporter(TerrainItemEntryType *itemPtr, long  x, long z);
void InitTeleporters(void);
void UpdateTeleporterTimers(void);
		
		
	/* ZIP LINE */

void InitZipLines(void);
Boolean AddZipLinePost(TerrainItemEntryType *itemPtr, long  x, long z);

Boolean AddLampPost(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddGraveStone(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddCrashedShip(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddRubble(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddTeleporterMap(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddGreenSteam(TerrainItemEntryType *itemPtr, long  x, long z);


	/* HUMAN CANNONBALL */
	

Boolean AddCannon(TerrainItemEntryType *itemPtr, long  x, long z);
ObjNode *IsPlayerInPositionToEnterCannon(float *outX, float *outZ);
void StartCannonFuse(ObjNode *theNode);

		/* BUMPER CAR */
		
void InitBumperCars(void);
Boolean AddBumperCar(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_BumperCar(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
void AlignPlayerInBumperCar(ObjNode *player);
void PlayerTouchedElectricFloor(ObjNode *theNode);
Boolean AddTireBumperStrip(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddBumperCarPowerPost(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_BumperCarPowerPost(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
Boolean AddBumperCarGate(TerrainItemEntryType *itemPtr, long  x, long z);


		/* ROCKETSLED */
		
Boolean AddRocketSled(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean DoTrig_RocketSled(ObjNode *theNode, ObjNode *whoNode, Byte sideBits);
void AlignPlayerInRocketSled(ObjNode *player);


Boolean AddZigZagSlats(TerrainItemEntryType *itemPtr, long  x, long z);
void InitZigZagSlats(void);
void UpdateZigZagSlats(void);


Boolean AddCloudPlatform(TerrainItemEntryType *itemPtr, long  x, long z);



	/* ICE SAUCER */
	
Boolean AddIceSaucer(TerrainItemEntryType *itemPtr, long  x, long z);
void SeeIfCrackSaucerIce(OGLPoint3D *impactPt);


Boolean AddLavaStone(TerrainItemEntryType *itemPtr, long  x, long z);
Boolean AddRadarDish(TerrainItemEntryType *itemPtr, long  x, long z);


