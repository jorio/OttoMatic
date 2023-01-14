/****************************/
/*   	TERRAIN2.C 	        */
/****************************/


#include "game.h"

/***************/
/* EXTERNALS   */
/***************/

/****************************/
/*    PROTOTYPES            */
/****************************/

static Boolean NilAdd(TerrainItemEntryType *itemPtr,long x, long z);


/****************************/
/*    CONSTANTS             */
/****************************/



/**********************/
/*     VARIABLES      */
/**********************/

int						gNumTerrainItems;
TerrainItemEntryType 	**gMasterItemList = nil;
int						*gTerrainItemFileIDs = nil;		// maps sorted terrain item IDs to the order in which they appear in the .ter.rsrc file (for debugging)

float					**gMapYCoords = nil;			// 2D array of map vertex y coords
float					**gMapYCoordsOriginal = nil;	// copy of gMapYCoords data as it was when file was loaded
Byte					**gMapSplitMode = nil;

SuperTileItemIndexType	**gSuperTileItemIndexGrid = nil;



/**********************/
/*     TABLES         */
/**********************/

#define	MAX_ITEM_NUM	108					// for error checking!

static Boolean (*gTerrainItemAddRoutines[MAX_ITEM_NUM+1])(TerrainItemEntryType *itemPtr, long x, long z) =
{
		NilAdd,								// My Start Coords
		AddBasicPlant,						// 1:  basic plant/tree
		AddSpacePodGenerator,				// 2:  space pod
		AddEnemy_Squooshy,					// 3: squooshy enemy
		AddHuman,							// 4: Human
		AddAtom,
		AddPowerupPod,
		AddEnemy_BrainAlien,				// 7:  brain alien
		AddEnemy_Onion,						// 8:  Onion
		AddEnemy_Corn,						// 9:  Corn
		AddEnemy_Tomato,					// 10:  tomato
		AddBarn,							// 11:  barn
		AddSilo,							// 12:  solo
		AddWoodenGate,						// 13:  wooden gate
		AddPhonePole,						// 14:  phone pole
		AddTractor,							// 15:  farm tractor
		AddSprout,							// 16:  add sprout
		AddCornStalk,						// 17:  corn stalk
		AddBigLeafPlant,					// 18:  big leaf plant
		AddMetalGate,						// 19:  metal gate
		AddFencePost,						// 20:  fence post
		AddWindmill,						// 21:  windmill
		AddMetalTub,						// 22:  metal tub
		AddOutHouse,						// 23:  outhouse
		AddRock,							// 24:  rock
		AddHay,								// 25:  hay bale
		AddExitRocket,						// 26:  exit rocket
		AddCheckpoint,						// 27:  checkpoint
		AddSlimePipe,						// 28:  slime pipe
		AddFallingCrystal,					// 29:	falling crystal
		AddEnemy_Blob,						// 30:  blob enemy
		AddBumperBubble,					// 31:  bumper bubble
		AddBasicCrystal,					// 32:  basic crystal
		AddInertBubble,						// 33:  soap bubble
		AddSlimeTree,						// 34:  slime tree
		NilAdd,								// 35:  magnet monster (spline only)
		AddFallingSlimePlatform,			// 36:  falling slime platform
		AddBubblePump,						// 37:  bubble pump
		AddSlimeMech,						// 38:  slime mech
		AddSpinningPlatform,				// 39:  spinning platform
		NilAdd,								// 40:  moving platform (spline only)
		NilAdd,								// 41:  blob boss machine
		AddBlobBossTube,					// 42: 	blob boss tube
		AddScaffoldingPost,					// 43:  scaffolding post
		AddJungleGate,						// 44:  jungle gate
		AddCrunchDoor,						// 45:  crunch door
		AddManhole,							// 46:  manhole
		AddCloudPlatform,					// 47:  cloud platform
		AddCloudTunnel,						// 48:  cloud tunnel
		AddEnemy_Flamester,					// 49:  flamester
		AddEnemy_GiantLizard,				// 50:  giant lizard
		AddEnemy_FlyTrap,					// 51:  venus flytrap
		AddEnemy_Mantis,					// 52:  mantis
		AddTurtlePlatform,					// 53:  turtle platform
		AddSmashable,						// 54:  jungle smashable
		AddLeafPlatform,					// 55:  leaf platform
		AddHelpBeacon,						// 56:  help beacon
		AddTeleporter,						// 57:  teleporter
		AddZipLinePost,						// 58:  zip line post
		AddEnemy_Mutant,					// 59:  mutant enemy
		AddEnemy_MutantRobot,				// 60:  mutant robot enemy
		AddHumanScientist,					// 61:  scientist human
		AddProximityMine,					// 62:  proximity mine
		AddLampPost,						// 63:  lamp posts
		AddDebrisGate,						// 64:  debris gate
		AddGraveStone,						// 65:  grave stone
		AddCrashedShip,						// 66:  crashed ship
		AddChainReactingMine,				// 67:  chain reacting mine
		AddRubble,							// 68:  rubble
		AddTeleporterMap,					// 69:  teleporter map (UNUSED)
		AddGreenSteam,						// 70:  green steam
		AddTentacleGenerator,				// 71:  tentacle generator
		AddPitcherPlantBoss,				// 72:  pitcher plant boss
		AddPitcherPod,						// 73:  pitcher pod
		AddTractorBeamPost,					// 74:  tractor beam post
		AddCannon,							// 75:  cannon
		AddBumperCar,						// 76:  bumper car
		AddTireBumperStrip,					// 77:	tire bumper
		AddEnemy_Clown,						// 78:	clown enemy
		NilAdd,								// 79: clown fish
		AddBumperCarPowerPost,				// 80: bumper car power post
		AddEnemy_StrongMan,					// 81:	strongman enemy
		AddBumperCarGate,					// 82:  bumper car gate
		AddRocketSled,						// 83: tobogan
		AddTrapDoor,						// 84:  trap door
		AddZigZagSlats,						// 85:  zig-zag slats
		NilAdd,								// 86: ?????
		AddLavaPillar,						// 87:  lava pillar
		AddVolcanoGeneratorZone,			// 88:  volcano generator
		AddJawsBot,							// 89:  jaws bot enemy
		AddIceSaucer,						// 90:  ice saucer
		AddRunwayLights,					// 91:  runway lights
		AddEnemy_IceCube,					// 92:  ice cube enemy
		AddHammerBot,						// 93:  HAMMER BOT
		AddDrillBot,						// 94:  drill bot
		AddSwingerBot,						// 95:  swinger bot
		AddLavaStone,						// 96:  lava stone
		AddSnowball,						// 97: 	snowball
		AddLavaPlatform,					// 98:  lava platform
		AddSmoker,							// 99:  smoker
		AddRadarDish,						// 100:  tower / radar dish
		AddPeopleHut,						// 101: people hut
		AddBeemer,							// 102:  bemmer
		NilAdd,								// 103:  rail gun
		AddTurret,							// 104:  turret
		AddEnemy_BrainBoss,					// 105:  brain boss
		AddBlobArrow,						// 106:  blob arrow
		AddNeuronStrand,					// 107:  neuron strand
		AddBrainPort,						// 108:  brain port
};



/********************* BUILD TERRAIN ITEM LIST ***********************/
//
// This takes the input item list and resorts it according to supertile grid number
// such that the items on any supertile are all sequential in the list instead of scattered.
//

void BuildTerrainItemList(void)
{
TerrainItemEntryType	**tempItemList;
TerrainItemEntryType	*srcList,*newList;
int						row,col,i;
int						itemX, itemZ;
int						total;

			/* ALLOC MEMORY FOR SUPERTILE ITEM INDEX GRID */


	Alloc_2d_array(SuperTileItemIndexType, gSuperTileItemIndexGrid, gNumSuperTilesDeep, gNumSuperTilesWide);

	if (gNumTerrainItems == 0)
		DoFatalAlert("BuildTerrainItemList: there must be at least 1 terrain item!");


			/* ALLOC MEMORY FOR NEW LIST */

	tempItemList = (TerrainItemEntryType **)AllocHandle(sizeof(TerrainItemEntryType) * gNumTerrainItems);
	if (tempItemList == nil)
		DoFatalAlert("BuildTerrainItemList: AllocPtr failed!");

	HLock((Handle)gMasterItemList);
	HLock((Handle)tempItemList);

	srcList = *gMasterItemList;
	newList = *tempItemList;

	GAME_ASSERT(!gTerrainItemFileIDs);
	gTerrainItemFileIDs = (int*) AllocPtrClear(sizeof(int) * gNumTerrainItems);


			/************************/
			/* SCAN ALL SUPERTILES  */
			/************************/

	total = 0;

	for (row = 0; row < gNumSuperTilesDeep; row++)
	{
		for (col = 0; col < gNumSuperTilesWide; col++)
		{
			gSuperTileItemIndexGrid[row][col].numItems = 0;			// no items on this supertile yet


			/* FIND ALL ITEMS ON THIS SUPERTILE */

			for (i = 0; i < gNumTerrainItems; i++)
			{
				itemX = srcList[i].x;								// get pixel coords of item
				itemZ = srcList[i].y;

				itemX /= TERRAIN_SUPERTILE_UNIT_SIZE;				// convert to supertile row
				itemZ /= TERRAIN_SUPERTILE_UNIT_SIZE;				// convert to supertile column

				if ((itemX == col) && (itemZ == row))				// see if its on this supertile
				{
					if (gSuperTileItemIndexGrid[row][col].numItems == 0)		// see if this is the 1st item
						gSuperTileItemIndexGrid[row][col].itemIndex = total;	// set starting index

					newList[total] = srcList[i];					// copy into new list
					gTerrainItemFileIDs[total] = i;
					total++;										// inc counter
					gSuperTileItemIndexGrid[row][col].numItems++;	// inc # items on this supertile

				}
				else
				if (itemX > col)									// since original list is sorted, we can know when we are past the usable edge
					break;
			}
		}
	}


		/* NUKE THE ORIGINAL ITEM LIST AND REASSIGN TO THE NEW SORTED LIST */

	DisposeHandle((Handle)gMasterItemList);						// nuke old list
	gMasterItemList = tempItemList;								// reassign



			/* FIGURE OUT WHERE THE STARTING POINT IS */

	FindPlayerStartCoordItems();										// look thru items for my start coords
}



/******************** FIND PLAYER START COORD ITEM *******************/
//
// Scans thru item list for item type #14 which is a teleport reciever / start coord,
//

void FindPlayerStartCoordItems(void)
{
long					i;
TerrainItemEntryType	*itemPtr;


	itemPtr = *gMasterItemList; 												// get pointer to data inside the LOCKED handle

				/* SCAN FOR "START COORD" ITEM */

	for (i= 0; i < gNumTerrainItems; i++)
	{
		if (itemPtr[i].type == MAP_ITEM_MYSTARTCOORD)						// see if it's a MyStartCoord item
		{

					/* CHECK FOR BIT INFO */


			gPlayerInfo.coord.x = gPlayerInfo.startX = itemPtr[i].x;
			gPlayerInfo.coord.z = gPlayerInfo.startZ = itemPtr[i].y;
			gPlayerInfo.startRotY = (float)itemPtr[i].parm[0] * (PI2/8.0f);	// calc starting rotation aim

			break;
		}
	}
}


/****************** ADD TERRAIN ITEMS ON SUPERTILE *******************/
//
// Called by DoPlayerTerrainUpdate() per each supertile needed.
// This scans all of the items on this supertile and attempts to add them.
//

void AddTerrainItemsOnSuperTile(long row, long col)
{
TerrainItemEntryType *itemPtr;
long			type,numItems,startIndex,i;
Boolean			flag;


	numItems = gSuperTileItemIndexGrid[row][col].numItems;		// see how many items are on this supertile
	if (numItems == 0)
		return;

	startIndex = gSuperTileItemIndexGrid[row][col].itemIndex;	// get starting index into item list
	itemPtr = &(*gMasterItemList)[startIndex];					// get pointer to 1st item on this supertile


			/*****************************/
			/* SCAN ALL ITEMS UNDER HERE */
			/*****************************/

	for (i = 0; i < numItems; i++)
	{
		float	x,z;

		if (itemPtr[i].flags & ITEM_FLAGS_INUSE)				// see if item available
			continue;

		x = itemPtr[i].x;										// get item coords
		z = itemPtr[i].y;

		if (SeeIfCoordsOutOfRange(x,z))						// only add if this supertile is active by player
			continue;

		type = itemPtr[i].type;									// get item #
		if (type > MAX_ITEM_NUM)								// error check!
		{
			DoFatalAlert("Illegal Map Item Type!");
		}

		flag = gTerrainItemAddRoutines[type](&itemPtr[i],itemPtr[i].x, itemPtr[i].y); // call item's ADD routine
		if (flag)
			itemPtr[i].flags |= ITEM_FLAGS_INUSE;				// set in-use flag
	}
}



/******************** NIL ADD ***********************/
//
// nothing add
//

static Boolean NilAdd(TerrainItemEntryType *itemPtr,long x, long z)
{
#pragma unused (itemPtr, x, z)
	return(false);
}


/***************** TRACK TERRAIN ITEM ******************/
//
// Returns true if theNode is out of range
//

Boolean TrackTerrainItem(ObjNode *theNode)
{
	return(SeeIfCoordsOutOfRange(theNode->Coord.x,theNode->Coord.z));		// see if out of range of all players
}


/***************** TRACK TERRAIN ITEM: FROM INIT ******************/
//
// Returns true if theNode is out of range
//

Boolean TrackTerrainItem_FromInit(ObjNode *theNode)
{
	return(SeeIfCoordsOutOfRange(theNode->InitCoord.x,theNode->InitCoord.z));
}


/********************* SEE IF COORDS OUT OF RANGE RANGE *********************/
//
// Returns true if the given x/z coords are outside the item delete
// window of any of the players.
//

Boolean SeeIfCoordsOutOfRange(float x, float z)
{
int			row,col;

			/* SEE IF OUT OF RANGE */

	if ((x < 0) || (z < 0))
		return(true);
	if ((x >= gTerrainUnitWidth) || (z >= gTerrainUnitDepth))
		return(true);


		/* SEE IF A PLAYER USES THIS SUPERTILE */

	col = x * TERRAIN_SUPERTILE_UNIT_SIZE_Frac;						// calc supertile relative row/col that the coord lies on
	row = z * TERRAIN_SUPERTILE_UNIT_SIZE_Frac;

	if (gSuperTileStatusGrid[row][col].playerHereFlag)				// if a player is using this supertile, then coords are in range
		return(false);
	else
		return(true);												// otherwise, out of range since no players can see this supertile
}


/*************************** ROTATE ON TERRAIN ***************************/
//
// Rotates an object's x & z such that it's lying on the terrain.
//
// INPUT: surfaceNormal == optional input normal to use.
//

void RotateOnTerrain(ObjNode *theNode, float yOffset, OGLVector3D *surfaceNormal)
{
OGLVector3D		up;
float			r,x,z,y;
OGLPoint3D		to;
OGLMatrix4x4	*m,m2;

			/* GET CENTER Y COORD & TERRAIN NORMAL */

	x = theNode->Coord.x;
	z = theNode->Coord.z;
	y = theNode->Coord.y = GetTerrainY(x, z) + yOffset;

	if (surfaceNormal)
		up = *surfaceNormal;
	else
		up = gRecentTerrainNormal;


			/* CALC "TO" COORD */

	r = theNode->Rot.y;
	to.x = x + sin(r) * -30.0f;
	to.z = z + cos(r) * -30.0f;
	to.y = GetTerrainY(to.x, to.z) + yOffset;


			/* CREATE THE MATRIX */

	m = &theNode->BaseTransformMatrix;
	SetLookAtMatrix(m, &up, &theNode->Coord, &to);


		/* POP IN THE TRANSLATE INTO THE MATRIX */

	m->value[M03] = x;
	m->value[M13] = y;
	m->value[M23] = z;


			/* SET SCALE */

	OGLMatrix4x4_SetScale(&m2, theNode->Scale.x,				// make scale matrix
							 	theNode->Scale.y,
							 	theNode->Scale.z);
	OGLMatrix4x4_Multiply(&m2, m, m);
}




#pragma mark ======= TERRAIN PRE-CONSTRUCTION =========




/********************** CALC TILE NORMALS *****************************/
//
// Given a row, col coord, calculate the face normals for the 2 triangles.
//

void CalcTileNormals(long row, long col, OGLVector3D *n1, OGLVector3D *n2)
{
static OGLPoint3D	p1 = {0,0,0};
static OGLPoint3D	p2 = {0,0,TERRAIN_POLYGON_SIZE};
static OGLPoint3D	p3 = {TERRAIN_POLYGON_SIZE,0,TERRAIN_POLYGON_SIZE};
static OGLPoint3D	p4 = {TERRAIN_POLYGON_SIZE, 0, 0};


		/* MAKE SURE ROW/COL IS IN RANGE */

	if ((row >= gTerrainTileDepth) || (row < 0) ||
		(col >= gTerrainTileWidth) || (col < 0))
	{
		n1->x = n2->x = 0;						// pass back up vector by default since our of range
		n1->y = n2->y = 1;
		n1->z = n2->z = 0;
		return;
	}

	p1.y = gMapYCoords[row][col];		// far left
	p2.y = gMapYCoords[row+1][col];		// near left
	p3.y = gMapYCoords[row+1][col+1];	// near right
	p4.y = gMapYCoords[row][col+1];		// far right


		/* CALC NORMALS BASED ON SPLIT */

	if (gMapSplitMode[row][col] == SPLIT_BACKWARD)
	{
		CalcFaceNormal(&p2, &p3, &p1, n1);		// fl, nl, nr
		CalcFaceNormal(&p3, &p4, &p1, n2);		// fl, nr, fr
	}
	else
	{
		CalcFaceNormal(&p4, &p1, &p2, n1);		// fl, nl, fr
		CalcFaceNormal(&p3, &p4, &p2, n2);		// fr, nl, nr
	}
}


/********************** CALC TILE NORMALS: NOT NORMALIZED *****************************/
//
// Given a row, col coord, calculate the face normals for the 2 triangles.
//

void CalcTileNormals_NotNormalized(long row, long col, OGLVector3D *n1, OGLVector3D *n2)
{
static OGLPoint3D	p1 = {0,0,0};
static OGLPoint3D	p2 = {0,0,TERRAIN_POLYGON_SIZE};
static OGLPoint3D	p3 = {TERRAIN_POLYGON_SIZE,0,TERRAIN_POLYGON_SIZE};
static OGLPoint3D	p4 = {TERRAIN_POLYGON_SIZE, 0, 0};


		/* MAKE SURE ROW/COL IS IN RANGE */

	if ((row >= gTerrainTileDepth) || (row < 0) ||
		(col >= gTerrainTileWidth) || (col < 0))
	{
		n1->x = n2->x = 0;						// pass back up vector by default since our of range
		n1->y = n2->y = 1;
		n1->z = n2->z = 0;
		return;
	}

	p1.y = gMapYCoords[row][col];		// far left
	p2.y = gMapYCoords[row+1][col];		// near left
	p3.y = gMapYCoords[row+1][col+1];	// near right
	p4.y = gMapYCoords[row][col+1];		// far right


		/* CALC NORMALS BASED ON SPLIT */

	if (gMapSplitMode[row][col] == SPLIT_BACKWARD)
	{
		CalcFaceNormal_NotNormalized(&p2, &p3, &p1, n1);		// fl, nl, nr
		CalcFaceNormal_NotNormalized(&p3, &p4, &p1, n2);		// fl, nr, fr
	}
	else
	{
		CalcFaceNormal_NotNormalized(&p4, &p1, &p2, n1);		// fl, nl, fr
		CalcFaceNormal_NotNormalized(&p3, &p4, &p2, n2);		// fr, nl, nr
	}
}






