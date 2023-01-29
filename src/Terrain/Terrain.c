/****************************/
/*     TERRAIN.C           	*/
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/

/***************/
/* EXTERNALS   */
/***************/

#include "game.h"

/****************************/
/*  PROTOTYPES             */
/****************************/

static short GetFreeSuperTileMemory(void);
static inline void ReleaseSuperTileObject(short superTileNum);
static void CalcNewItemDeleteWindow(void);
static uint16_t	BuildTerrainSuperTile(long	startCol, long startRow);
static void ReleaseAllSuperTiles(void);
static void DoSuperTileDeformation(SuperTileMemoryType *superTile);
static void UpdateTerrainDeformationFunctions(void);
static void DrawTerrain(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/


#define	ITEM_WINDOW		SUPERTILE_ITEMRING_MARGIN	// # supertiles for item add window (must be integer)
#define	OUTER_SIZE		0 //0.6f						// size of border out of add window for delete window (can be float)





/**********************/
/*     VARIABLES      */
/**********************/


int				gNumSuperTilesDrawn;
static	Byte	gHiccupTimer;

Boolean			gDisableHiccupTimer = false;


SuperTileStatus	**gSuperTileStatusGrid = nil;				// supertile status grid

int					gTerrainTileWidth,gTerrainTileDepth;			// width & depth of terrain in tiles
int					gTerrainUnitWidth,gTerrainUnitDepth;			// width & depth of terrain in world units (see TERRAIN_POLYGON_SIZE)

int					gNumUniqueSuperTiles;
SuperTileGridType 	**gSuperTileTextureGrid = nil;			// 2d array

MOMaterialObject	*gSuperTileTextureObjects[MAX_SUPERTILE_TEXTURES];

uint16_t			**gTileGrid = nil;

int					gNumSuperTilesDeep,gNumSuperTilesWide;	  		// dimensions of terrain in terms of supertiles
static int			gCurrentSuperTileRow,gCurrentSuperTileCol;
static int			gPreviousSuperTileCol,gPreviousSuperTileRow;

int					gNumFreeSupertiles = 0;
SuperTileMemoryType	gSuperTileMemoryList[MAX_SUPERTILES];



TileAttribType	**gTileAttribList = nil;
uint16_t		gTileAttribFlags;
float			gTileSlipperyFactor = 1.0f;			// 1 = totally slippery, 0 = no slippery

			/* TILE SPLITTING TABLES */

					/* /  */
static	Byte	gTileTriangles1_A[SUPERTILE_SIZE][SUPERTILE_SIZE][3];
static	Byte	gTileTriangles2_A[SUPERTILE_SIZE][SUPERTILE_SIZE][3];

					/* \  */

static	Byte	gTileTriangles1_B[SUPERTILE_SIZE][SUPERTILE_SIZE][3];
static	Byte	gTileTriangles2_B[SUPERTILE_SIZE][SUPERTILE_SIZE][3];

OGLVertex		gWorkGrid[SUPERTILE_SIZE+1][SUPERTILE_SIZE+1];

OGLVector3D		gRecentTerrainNormal;							// from _Planar


		/* MASTER ARRAYS FOR ALL SUPERTILE DATA FOR CURRENT LEVEL */

static MOVertexArrayData		*gSuperTileMeshData = nil;
static OGLPoint3D				*gSuperTileCoords = nil;
static MOTriangleIndecies		*gSuperTileTriangles = nil;
static OGLTextureCoord			*gSuperTileUVs = nil;
static OGLVector3D				*gSuperTileNormals = nil;


		/* TERRAIN DEFORMATIONS */

int	gNumTerrainDeformations = 0;
static Boolean	gCleanupDeformation = false;

static DeformationType	gDeformationList[MAX_DEFORMATIONS];


/****************** INIT TERRAIN MANAGER ************************/
//
// Only called at boot!
//

void InitTerrainManager(void)
{
int	x,y;

		/* BUILT TRIANGLE SPLITTING TABLES */

	for (y = 0; y < SUPERTILE_SIZE; y++)
	{
		for (x = 0; x < SUPERTILE_SIZE; x++)
		{
			gTileTriangles1_A[y][x][0] = (SUPERTILE_SIZE+1) * y + x + 1;
			gTileTriangles1_A[y][x][1] = (SUPERTILE_SIZE+1) * y + x;
			gTileTriangles1_A[y][x][2] = (SUPERTILE_SIZE+1) * (y+1) + x;

			gTileTriangles2_A[y][x][0] = (SUPERTILE_SIZE+1) * (y+1) + x + 1;
			gTileTriangles2_A[y][x][1] = (SUPERTILE_SIZE+1) * y + x + 1;
			gTileTriangles2_A[y][x][2] = (SUPERTILE_SIZE+1) * (y+1) + x;

			gTileTriangles1_B[y][x][0] = (SUPERTILE_SIZE+1) * (y+1) + x;
			gTileTriangles1_B[y][x][1] = (SUPERTILE_SIZE+1) * (y+1) + x + 1;
			gTileTriangles1_B[y][x][2] = (SUPERTILE_SIZE+1) * y + x;

			gTileTriangles2_B[y][x][0] = (SUPERTILE_SIZE+1) * (y+1) + x + 1;
			gTileTriangles2_B[y][x][1] = (SUPERTILE_SIZE+1) * y + x + 1;
			gTileTriangles2_B[y][x][2] = (SUPERTILE_SIZE+1) * y + x;
		}
	}
}


/****************** INIT CURRENT SCROLL SETTINGS *****************/

void InitCurrentScrollSettings(void)
{
long	x,y;
int		dummy;
ObjNode	*obj;

	x = gPlayerInfo.coord.x-(SUPERTILE_ACTIVE_RANGE*SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE);
	y = gPlayerInfo.coord.z-(SUPERTILE_ACTIVE_RANGE*SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE);
	GetSuperTileInfo(x, y, &gCurrentSuperTileCol, &gCurrentSuperTileRow, &dummy, &dummy);

	gPreviousSuperTileCol = -100000;
	gPreviousSuperTileRow = -100000;


		/*************************************************************************/
		/* CREATE DUMMY CUSTOM OBJECT TO CAUSE TERRAIN DRAWING AT THE DESIRED TIME */
		/*************************************************************************/

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.slot 		= TERRAIN_SLOT;
	gNewObjectDefinition.moveCall 	= nil;

	if (gLevelNum == LEVEL_NUM_CLOUD)
		gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES;		// show both sides of scaffolding
	else
		gNewObjectDefinition.flags 		= 0;

	obj = MakeNewObject(&gNewObjectDefinition);
	obj->CustomDrawFunction = DrawTerrain;

}


/******************** INIT SUPERTILE GRID *************************/

void InitSuperTileGrid(void)
{
int		r,c,i;

	Alloc_2d_array(SuperTileStatus, gSuperTileStatusGrid, gNumSuperTilesDeep, gNumSuperTilesWide);	// alloc 2D grid array


			/* INIT ALL GRID SLOTS TO EMPTY AND UNUSED */

	for (r = 0; r < gNumSuperTilesDeep; r++)
	{
		for (c = 0; c < gNumSuperTilesWide; c++)
		{
			gSuperTileStatusGrid[r][c].supertileIndex 	= 0;
			gSuperTileStatusGrid[r][c].statusFlags 		= 0;
			gSuperTileStatusGrid[r][c].playerHereFlag 	= false;
		}
	}


			/* ALSO INIT T-DEFS WHILE WE'RE HERE */

	gNumTerrainDeformations = 0;

	for (i = 0; i < MAX_DEFORMATIONS; i++)
	{
		gDeformationList[i].isUsed = false;				// set all free
	}
}


/***************** DISPOSE TERRAIN **********************/
//
// Deletes any existing terrain data
//

void DisposeTerrain(void)
{
			/* FREE ALL TEXTURE OBJECTS */

	GAME_ASSERT_MESSAGE(gSuperTileTextureObjects[0] == nil, "supertile 0's texture is supposed to be blank!");

	for (int i = 1; i < gNumUniqueSuperTiles; i++)
	{
		MO_DisposeObjectReference(gSuperTileTextureObjects[i]);
		gSuperTileTextureObjects[i] = nil;
	}
	gNumUniqueSuperTiles = 0;

	if (gTileGrid)														// free old array
	{
		Free_2d_array(gTileGrid);
		gTileGrid = nil;
	}

	if (gSuperTileItemIndexGrid)
	{
		Free_2d_array(gSuperTileItemIndexGrid);
		gSuperTileItemIndexGrid = nil;
	}

	if (gSuperTileTextureGrid)
	{
		Free_2d_array(gSuperTileTextureGrid);
		gSuperTileTextureGrid = nil;
	}

	if (gSuperTileStatusGrid)
	{
		Free_2d_array(gSuperTileStatusGrid);
		gSuperTileStatusGrid = nil;
	}

	if (gMasterItemList)
	{
		DisposeHandle((Handle)gMasterItemList);
		gMasterItemList = nil;
	}

	if (gTerrainItemFileIDs)
	{
		DisposePtr((Ptr) gTerrainItemFileIDs);
		gTerrainItemFileIDs = nil;
	}

	if (gMapYCoords)
	{
		Free_2d_array(gMapYCoords);
		gMapYCoords = nil;
	}

	if (gMapYCoordsOriginal)
	{
		Free_2d_array(gMapYCoordsOriginal);
		gMapYCoordsOriginal = nil;
	}

	if (gMapSplitMode)
	{
		Free_2d_array(gMapSplitMode);
		gMapSplitMode = nil;
	}

	if (gTileAttribList)
	{
		DisposeHandle((Handle)gTileAttribList);
		gTileAttribList = nil;
	}

			/* NUKE SPLINE DATA */

	if (gSplineList)
	{
		for (int i = 0; i < gNumSplines; i++)
		{
			DisposeHandle((Handle)(*gSplineList)[i].nubList);		// nuke nub list
			DisposeHandle((Handle)(*gSplineList)[i].pointList);		// nuke point list
			DisposeHandle((Handle)(*gSplineList)[i].itemList);		// nuke item list
		}
		DisposeHandle((Handle)gSplineList);
		gSplineList = nil;
	}
	gNumSplines = 0;

				/* NUKE WATER PATCH */

	if (gWaterListHandle != nil)
	{
		DisposeHandle((Handle)gWaterListHandle);
	}
	gWaterListHandle = nil;
	gWaterList = nil;
	gNumWaterPatches = 0;

	ReleaseAllSuperTiles();
}


#pragma mark -

/************** CREATE SUPERTILE MEMORY LIST ********************/
//
// Preallocates memory and initializes the data for the maximum number of supertiles that
// we will ever need on this level.
//

void CreateSuperTileMemoryList(void)
{
int	u,v,i,j;


			/*************************************************/
			/* ALLOCATE ARRAYS FOR ALL THE DATA WE WILL NEED */
			/*************************************************/

	gNumFreeSupertiles = MAX_SUPERTILES;

			/* ALLOC BASE TRIMESH DATA FOR ALL SUPERTILES */

	gSuperTileMeshData = AllocPtr(sizeof(MOVertexArrayData) * MAX_SUPERTILES);
	if (gSuperTileMeshData == nil)
		DoFatalAlert("CreateSuperTileMemoryList: AllocPtr failed - gSuperTileMeshData");


			/* ALLOC POINTS FOR ALL SUPERTILES */

	gSuperTileCoords = AllocPtr(sizeof(OGLPoint3D) * (NUM_VERTICES_IN_SUPERTILE * MAX_SUPERTILES));
	if (gSuperTileCoords == nil)
		DoFatalAlert("CreateSuperTileMemoryList: AllocPtr failed - gSuperTileCoords");


			/* ALLOC VERTEX NORMALS FOR ALL SUPERTILES */

	gSuperTileNormals = AllocPtr(sizeof(OGLVector3D) * (NUM_VERTICES_IN_SUPERTILE * MAX_SUPERTILES));
	if (gSuperTileNormals == nil)
		DoFatalAlert("CreateSuperTileMemoryList: AllocPtr failed - gSuperTileNormals");


			/* ALLOC UVS FOR ALL SUPERTILES */

	gSuperTileUVs = AllocPtr(sizeof(OGLTextureCoord) * NUM_VERTICES_IN_SUPERTILE * MAX_SUPERTILES);
	if (gSuperTileUVs == nil)
		DoFatalAlert("CreateSuperTileMemoryList: AllocPtr failed - gSuperTileUVs");


			/* ALLOC TRIANGLE ARRAYS ALL SUPERTILES */

	gSuperTileTriangles = AllocPtr(sizeof(MOTriangleIndecies) * NUM_TRIS_IN_SUPERTILE * MAX_SUPERTILES);
	if (gSuperTileTriangles == nil)
		DoFatalAlert("CreateSuperTileMemoryList: AllocPtr failed - gSuperTileTriangles");


			/****************************************/
			/* FOR EACH POSSIBLE SUPERTILE SET INFO */
			/****************************************/


	for (i = 0; i < MAX_SUPERTILES; i++)
	{
		MOVertexArrayData		*meshPtr;
		OGLPoint3D				*coordPtr;
		MOTriangleIndecies		*triPtr;
		OGLTextureCoord			*uvPtr;
		OGLVector3D				*normalPtr;

		gSuperTileMemoryList[i].mode = SUPERTILE_MODE_FREE;					// it's free for use


				/* POINT TO ARRAYS */

		j = i* NUM_VERTICES_IN_SUPERTILE;									// index * supertile needs

		meshPtr 		= &gSuperTileMeshData[i];
		coordPtr 		= &gSuperTileCoords[j];
		normalPtr 		= &gSuperTileNormals[j];
		uvPtr 			= &gSuperTileUVs[j];
		triPtr 			= &gSuperTileTriangles[i * NUM_TRIS_IN_SUPERTILE];

		gSuperTileMemoryList[i].meshData = meshPtr;



				/* SET MESH STRUCTURE */

		meshPtr->numMaterials	= -1;							// textures will be manually submitted in drawing function!
		meshPtr->numPoints		= NUM_VERTICES_IN_SUPERTILE;
		meshPtr->numTriangles 	= NUM_TRIS_IN_SUPERTILE;

		meshPtr->points			= coordPtr;
		meshPtr->normals 		= normalPtr;
		meshPtr->uvs[0]			= uvPtr;
		meshPtr->colorsByte		= nil;
		meshPtr->colorsFloat	= nil;
		meshPtr->triangles 		= triPtr;


				/* SET UV & COLOR VALUES */

		const float seamlessTexmapSize	= (2.0f + SUPERTILE_TEXMAP_SIZE);
		const float seamlessUVScale		= SUPERTILE_TEXMAP_SIZE / seamlessTexmapSize;
		const float seamlessUVTranslate	= 1.0f / seamlessTexmapSize;

		j = 0;
		for (v = 0; v <= SUPERTILE_SIZE; v++)
		{
			for (u = 0; u <= SUPERTILE_SIZE; u++)
			{
				uvPtr[j].u = (float)u / (float)SUPERTILE_SIZE;		// sets uv's 0.0 -> 1.0 for single texture map
				uvPtr[j].v = (float)v / (float)SUPERTILE_SIZE;

				if (gG4)											// do seamless texturing if we're in high-detail mode
				{
					uvPtr[j].u = (uvPtr[j].u * seamlessUVScale) + seamlessUVTranslate;
					uvPtr[j].v = (uvPtr[j].v * seamlessUVScale) + seamlessUVTranslate;
				}

				j++;
			}
		}
	}
}


/********************* DISPOSE SUPERTILE MEMORY LIST **********************/

void DisposeSuperTileMemoryList(void)
{

			/* NUKE ALL MASTER ARRAYS WHICH WILL FREE UP ALL SUPERTILE MEMORY */

	if (gSuperTileMeshData)
		SafeDisposePtr((Ptr)gSuperTileMeshData);
	gSuperTileMeshData = nil;

	if (gSuperTileCoords)
		SafeDisposePtr((Ptr)gSuperTileCoords);
	gSuperTileCoords = nil;

	if (gSuperTileNormals)
		SafeDisposePtr((Ptr)gSuperTileNormals);
	gSuperTileNormals = nil;

	if (gSuperTileTriangles)
		SafeDisposePtr((Ptr)gSuperTileTriangles);
	gSuperTileTriangles = nil;

	if (gSuperTileUVs)
		SafeDisposePtr((Ptr)gSuperTileUVs);
	gSuperTileUVs = nil;
}


/***************** GET FREE SUPERTILE MEMORY *******************/
//
// Finds one of the preallocated supertile memory blocks and returns its index
// IT ALSO MARKS THE BLOCK AS USED
//
// OUTPUT: index into gSuperTileMemoryList
//

static short GetFreeSuperTileMemory(void)
{
int	i;

				/* SCAN FOR A FREE BLOCK */

	for (i = 0; i < MAX_SUPERTILES; i++)
	{
		if (gSuperTileMemoryList[i].mode == SUPERTILE_MODE_FREE)
		{
			gSuperTileMemoryList[i].mode = SUPERTILE_MODE_USED;
			gNumFreeSupertiles--;
			return(i);
		}
	}

	DoFatalAlert("No Free Supertiles!");
	return(-1);											// ERROR, NO FREE BLOCKS!!!! SHOULD NEVER GET HERE!
}

#pragma mark -


/******************* BUILD TERRAIN SUPERTILE *******************/
//
// Builds a new supertile which has scrolled on
//
// INPUT: startCol = starting tile column in map
//		  startRow = starting tile row in map
//
// OUTPUT: index to supertile
//

static uint16_t	BuildTerrainSuperTile(long	startCol, long startRow)
{
long	 			row,col,row2,col2,numPoints,i;
uint16_t				superTileNum;
float				height,miny,maxy;
MOVertexArrayData	*meshData = nil;
SuperTileMemoryType	*superTilePtr = nil;
OGLColorRGBA		*vertexColorList = nil;
float				ambientR = 1.0f, ambientG = 1.0f, ambientB = 1.0f;
float				fillR0 = 1.0f, fillG0 = 0.0f, fillB0 = 1.0f;
float				fillR1 = 1.0f, fillG1 = 0.0f, fillB1 = 1.0f;
OGLVector3D			*fillDir0 = nil;
OGLVector3D			*fillDir1 = nil;
Byte				numFillLights;
MOTriangleIndecies	*triangleList = nil;
OGLPoint3D			*vertexPointList = nil;
//OGLTextureCoord		*uvs = nil;
OGLVector3D			*vertexNormals = nil;

	superTileNum = GetFreeSuperTileMemory();						// get memory block for the data
	superTilePtr = &gSuperTileMemoryList[superTileNum];				// get ptr to it

	superTilePtr->x = (startCol * TERRAIN_POLYGON_SIZE) + (TERRAIN_SUPERTILE_UNIT_SIZE/2);		// also remember world coords
	superTilePtr->z = (startRow * TERRAIN_POLYGON_SIZE) + (TERRAIN_SUPERTILE_UNIT_SIZE/2);

	superTilePtr->left = (startCol * TERRAIN_POLYGON_SIZE);			// also save left/back coord
	superTilePtr->back = (startRow * TERRAIN_POLYGON_SIZE);

	superTilePtr->tileRow = startRow;								// save tile row/col
	superTilePtr->tileCol = startCol;


		/* GET LIGHT DATA */

	ambientR = gGameViewInfoPtr->lightList.ambientColor.r;			// get ambient color
	ambientG = gGameViewInfoPtr->lightList.ambientColor.g;
	ambientB = gGameViewInfoPtr->lightList.ambientColor.b;

	fillR0 = gGameViewInfoPtr->lightList.fillColor[0].r;			// get fill color
	fillG0 = gGameViewInfoPtr->lightList.fillColor[0].g;
	fillB0 = gGameViewInfoPtr->lightList.fillColor[0].b;
	fillDir0 = &gGameViewInfoPtr->lightList.fillDirection[0];		// get fill direction

	numFillLights = gGameViewInfoPtr->lightList.numFillLights;
	if (numFillLights > 1)
	{
		fillR1 = gGameViewInfoPtr->lightList.fillColor[1].r;
		fillG1 = gGameViewInfoPtr->lightList.fillColor[1].g;
		fillB1 = gGameViewInfoPtr->lightList.fillColor[1].b;
		fillDir1 = &gGameViewInfoPtr->lightList.fillDirection[1];
	}



				/*******************/
				/* GET THE TRIMESH */
				/*******************/

	meshData 				= gSuperTileMemoryList[superTileNum].meshData;		// get ptr to mesh data
	triangleList 			= meshData->triangles;								// get ptr to triangle index list
	vertexPointList 		= meshData->points;									// get ptr to points list
	vertexColorList 		= nil;	//meshData->colorsByte;								// get ptr to vertex color
	vertexNormals			= meshData->normals;								// get ptr to vertex normals
//	uvs						= meshData->uvs[0];									// get ptr to uvs

	miny = 10000000;													// init bbox counters
	maxy = -miny;


			/**********************/
			/* CREATE VERTEX GRID */
			/**********************/

	for (row2 = 0; row2 <= SUPERTILE_SIZE; row2++)
	{
		row = row2 + startRow;

		for (col2 = 0; col2 <= SUPERTILE_SIZE; col2++)
		{
			col = col2 + startCol;

			if ((row >= gTerrainTileDepth) || (col >= gTerrainTileWidth)) 	// check for edge vertices (off map array)
				height = 0;
			else
				height = gMapYCoords[row][col]; 							// get pixel height here


					/* SET COORD */

			gWorkGrid[row2][col2].point.x = (col*TERRAIN_POLYGON_SIZE);
			gWorkGrid[row2][col2].point.z = (row*TERRAIN_POLYGON_SIZE);
			gWorkGrid[row2][col2].point.y = height;							// save height @ this tile's upper left corner

					/* SET UV */

			gWorkGrid[row2][col2].uv.u = (float)col2 * (1.0f / (float)SUPERTILE_SIZE);		// sets uv's 0.0 -> 1.0 for single texture map
			gWorkGrid[row2][col2].uv.v = 1.0f - ((float)row2 * (1.0f / (float)SUPERTILE_SIZE));


					/* SET COLOR */

			gWorkGrid[row2][col2].color.r =
			gWorkGrid[row2][col2].color.g =
			gWorkGrid[row2][col2].color.b =
			gWorkGrid[row2][col2].color.a = 1.0;

			if (height > maxy)											// keep track of min/max
				maxy = height;
			if (height < miny)
				miny = height;
		}
	}


			/*********************************/
			/* CREATE TERRAIN MESH POLYGONS  */
			/*********************************/

					/* SET VERTEX COORDS */

	numPoints = 0;
	for (row = 0; row < (SUPERTILE_SIZE+1); row++)
	{
		for (col = 0; col < (SUPERTILE_SIZE+1); col++)
		{
			vertexPointList[numPoints] = gWorkGrid[row][col].point;						// copy from work grid
			numPoints++;
		}
	}

				/* UPDATE TRIMESH DATA WITH NEW INFO */

	i = 0;
	for (row2 = 0; row2 < SUPERTILE_SIZE; row2++)
	{
		row = row2 + startRow;

		for (col2 = 0; col2 < SUPERTILE_SIZE; col2++)
		{

			col = col2 + startCol;


					/* SET SPLITTING INFO */

			if (gMapSplitMode[row][col] == SPLIT_BACKWARD)	// set coords & uv's based on splitting
			{
					/* \ */
				triangleList[i].vertexIndices[0] 	= gTileTriangles1_B[row2][col2][0];
				triangleList[i].vertexIndices[1] 	= gTileTriangles1_B[row2][col2][1];
				triangleList[i++].vertexIndices[2] 	= gTileTriangles1_B[row2][col2][2];
				triangleList[i].vertexIndices[0] 	= gTileTriangles2_B[row2][col2][0];
				triangleList[i].vertexIndices[1] 	= gTileTriangles2_B[row2][col2][1];
				triangleList[i++].vertexIndices[2] 	= gTileTriangles2_B[row2][col2][2];
			}
			else
			{
					/* / */

				triangleList[i].vertexIndices[0] 	= gTileTriangles1_A[row2][col2][0];
				triangleList[i].vertexIndices[1] 	= gTileTriangles1_A[row2][col2][1];
				triangleList[i++].vertexIndices[2] 	= gTileTriangles1_A[row2][col2][2];
				triangleList[i].vertexIndices[0] 	= gTileTriangles2_A[row2][col2][0];
				triangleList[i].vertexIndices[1] 	= gTileTriangles2_A[row2][col2][1];
				triangleList[i++].vertexIndices[2] 	= gTileTriangles2_A[row2][col2][2];
			}
		}
	}


			/******************************/
			/* CALCULATE VERTEX NORMALS   */
			/******************************/

	CalculateSupertileVertexNormals(meshData, startRow, startCol);


	if (vertexColorList)
	{
			/*****************************/
			/* CALCULATE VERTEX COLORS   */
			/*****************************/

		i = 0;
		for (row = 0; row <= SUPERTILE_SIZE; row++)
		{
			for (col = 0; col <= SUPERTILE_SIZE; col++)
			{
				float	r,g,b,dot;


						/* APPLY LIGHTING TO THE VERTEX */

				r = ambientR;												// factor in the ambient
				g = ambientG;
				b = ambientB;

				dot = vertexNormals[i].x * fillDir0->x;					// calc dot product of fill #0
				dot += vertexNormals[i].y * fillDir0->y;
				dot += vertexNormals[i].z * fillDir0->z;
				dot = -dot;

				if (dot > 0.0f)
				{
					r += fillR0 * dot;
					g += fillG0 * dot;
					b += fillB0 * dot;
				}

				if (numFillLights > 1)
				{
					dot = vertexNormals[i].x * fillDir1->x;				// calc dot product of fill #1
					dot += vertexNormals[i].y * fillDir1->y;
					dot += vertexNormals[i].z * fillDir1->z;
					dot = -dot;

					if (dot > 0.0f)
					{
						r += fillR1 * dot;
						g += fillG1 * dot;
						b += fillB1 * dot;
					}
				}

				if (r > 1.0f)
					r = 1.0f;
				if (g > 1.0f)
					g = 1.0f;
				if (b > 1.0f)
					b = 1.0f;


						/* SAVE COLOR INTO LIST */

				vertexColorList[i].r = r * 255.0f;		// convert to Byte values
				vertexColorList[i].g = g * 255.0f;
				vertexColorList[i].b = b * 255.0f;
				i++;
			}
		}
	}

			/*********************/
			/* CALC COORD & BBOX */
			/*********************/
			//
			// This y coord is not used to translate since the terrain has no translation matrix
			// instead, this is used by the culling routine for culling tests
			//

	superTilePtr->y = (miny+maxy)*.5f;					// calc center y coord as average of top & bottom

	superTilePtr->bBox.min.x = gWorkGrid[0][0].point.x;
	superTilePtr->bBox.max.x = gWorkGrid[0][0].point.x + TERRAIN_SUPERTILE_UNIT_SIZE;
	superTilePtr->bBox.min.y = miny;
	superTilePtr->bBox.max.y = maxy;
	superTilePtr->bBox.min.z = gWorkGrid[0][0].point.z;
	superTilePtr->bBox.max.z = gWorkGrid[0][0].point.z + TERRAIN_SUPERTILE_UNIT_SIZE;

	if (gDisableHiccupTimer)
	{
		superTilePtr->hiccupTimer = 0;
	}
	else
	{
		superTilePtr->hiccupTimer = gHiccupTimer++;
		gHiccupTimer &= 0x3;							// spread over 4 frames
	}

	return(superTileNum);
}


/******************** CALCULATE SUPERTILE VERTEX NORMALS **********************/

void CalculateSupertileVertexNormals(MOVertexArrayData	*meshData, long	startRow, long startCol)
{
OGLPoint3D			*vertexPointList;
OGLVector3D			*vertexNormals;
int					i,row,col;
MOTriangleIndecies	*triangleList;
OGLVector3D			faceNormal[NUM_TRIS_IN_SUPERTILE];
OGLVector3D			*n1,*n2;
float				avX,avY,avZ;
OGLVector3D			nA,nB;
long				ro,co;

	vertexPointList 		= meshData->points;									// get ptr to points list
	vertexNormals			= meshData->normals;								// get ptr to vertex normals
	triangleList 			= meshData->triangles;								// get ptr to triangle index list


						/* CALC FACE NORMALS */

	for (i = 0; i < NUM_TRIS_IN_SUPERTILE; i++)
	{
		CalcFaceNormal_NotNormalized(&vertexPointList[triangleList[i].vertexIndices[0]],
									&vertexPointList[triangleList[i].vertexIndices[1]],
									&vertexPointList[triangleList[i].vertexIndices[2]],
									&faceNormal[i]);
	}


			/******************************/
			/* CALCULATE VERTEX NORMALS   */
			/******************************/

	i = 0;
	for (row = 0; row <= SUPERTILE_SIZE; row++)
	{
		for (col = 0; col <= SUPERTILE_SIZE; col++)
		{

			/* SCAN 4 TILES AROUND THIS TILE TO CALC AVERAGE NORMAL FOR THIS VERTEX */
			//
			// We use the face normal already calculated for triangles inside the supertile,
			// but for tiles/tris outside the supertile (on the borders), we need to calculate
			// the face normals there.
			//

			avX = avY = avZ = 0;									// init the normal

			for (ro = -1; ro <= 0; ro++)
			{
				for (co = -1; co <= 0; co++)
				{
					long	cc = col + co;
					long	rr = row + ro;

					if ((cc >= 0) && (cc < SUPERTILE_SIZE) && (rr >= 0) && (rr < SUPERTILE_SIZE)) // see if this vertex is in supertile bounds
					{
						n1 = &faceNormal[rr * (SUPERTILE_SIZE*2) + (cc*2)];				// average 2 triangles...
						n2 = n1+1;
						avX += n1->x + n2->x;											// ...and average with current average
						avY += n1->y + n2->y;
						avZ += n1->z + n2->z;
					}
					else																// tile is out of supertile, so calc face normal & average
					{
						CalcTileNormals_NotNormalized(startRow + rr, startCol + cc, &nA,&nB);		// calculate the 2 face normals for this tile
						avX += nA.x + nB.x;												// average with current average
						avY += nA.y + nB.y;
						avZ += nA.z + nB.z;
					}
				}
			}

			FastNormalizeVector(avX, avY, avZ, &vertexNormals[i]);						// normalize the vertex normal
			i++;
		}
	}


}


/******************* RELEASE SUPERTILE OBJECT *******************/
//
// Deactivates the terrain object
//

static inline void ReleaseSuperTileObject(short superTileNum)
{
	gSuperTileMemoryList[superTileNum].mode = SUPERTILE_MODE_FREE;		// it's free!
	gNumFreeSupertiles++;
}

/******************** RELEASE ALL SUPERTILES ************************/

static void ReleaseAllSuperTiles(void)
{
long	i;

	for (i = 0; i < MAX_SUPERTILES; i++)
		ReleaseSuperTileObject(i);

	gNumFreeSupertiles = MAX_SUPERTILES;
}

#pragma mark -

/********************* DRAW TERRAIN **************************/
//
// This is the main call to update the screen.  It draws all ObjNode's and the terrain itself
//

static void DrawTerrain(ObjNode *theNode)
{
int				r,c;
uint16_t			i,unique;
Boolean			superTileVisible;

#pragma unused(theNode)

				/**************/
				/* DRAW STUFF */
				/**************/

		/* SET A NICE STATE FOR TERRAIN DRAWING */

	OGL_PushState();

  	glDisable(GL_NORMALIZE);											// turn off vector normalization since scale == 1
    glDisable(GL_BLEND);												// no blending for terrain - its always opaque


	gNumSuperTilesDrawn	= 0;

	/******************************************************************/
	/* SCAN THE SUPERTILE GRID AND LOOK FOR USED & VISIBLE SUPERTILES */
	/******************************************************************/

	for (r = 0; r < gNumSuperTilesDeep; r++)
	{
		for (c = 0; c < gNumSuperTilesWide; c++)
		{
			if (gSuperTileStatusGrid[r][c].statusFlags & SUPERTILE_IS_USED_THIS_FRAME)			// see if used
			{

				i = gSuperTileStatusGrid[r][c].supertileIndex;			// extract supertile #

					/* SEE WHICH UNIQUE SUPERTILE TEXTURE TO USE */

				unique = gSuperTileTextureGrid[r][c].superTileID;
				if (unique == 0)										// if 0 then its a blank
					continue;

						/* SEE IF DELAY HICCUP TIMER */

				if (gSuperTileMemoryList[i].hiccupTimer)
				{
					gSuperTileMemoryList[i].hiccupTimer--;
					continue;
				}


					/* SEE IF IS CULLED & DO SUPERTILE DEFORMATION */

				superTileVisible = OGL_IsBBoxVisible(&gSuperTileMemoryList[i].bBox, nil);

				if (superTileVisible || gCleanupDeformation)					// update deformation of this ST under these conditions
					DoSuperTileDeformation(&gSuperTileMemoryList[i]);

				if (!superTileVisible)
					continue;



						/***********************************/
						/* DRAW THE MESH IN THIS SUPERTILE */
						/***********************************/

					/* SUBMIT THE TEXTURE */

				MO_DrawMaterial(gSuperTileTextureObjects[unique]);


					/* SUBMIT THE GEOMETRY */

				MO_DrawGeometry_VertexArray(gSuperTileMemoryList[i].meshData);
				gNumSuperTilesDrawn++;
			}
		}
	}
	gCleanupDeformation = false;							// reset this now


	if (gDebugMode == 2)
	{
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glColor4f(1,1,1,1);
		DrawSplines();
	}

	OGL_PopState();



		/*********************************************/
		/* PREPARE SUPERTILE GRID FOR THE NEXT FRAME */
		/*********************************************/

	for (r = 0; r < gNumSuperTilesDeep; r++)
	{
		for (c = 0; c < gNumSuperTilesWide; c++)
		{
			/* IF THIS SUPERTILE WAS NOT USED BUT IS DEFINED, THEN FREE IT */

			if (gSuperTileStatusGrid[r][c].statusFlags & SUPERTILE_IS_DEFINED)					// is it defined?
			{
				if (!(gSuperTileStatusGrid[r][c].statusFlags & SUPERTILE_IS_USED_THIS_FRAME))	// was it used?  If not, then release the supertile definition
				{
					ReleaseSuperTileObject(gSuperTileStatusGrid[r][c].supertileIndex);
					gSuperTileStatusGrid[r][c].statusFlags = 0;									// no longer defined
				}
			}

				/* ASSUME SUPERTILES WILL BE UNUSED ON NEXT FRAME */

			if ((gGamePrefs.anaglyphMode == ANAGLYPH_OFF) || (gAnaglyphPass > 0))
				gSuperTileStatusGrid[r][c].statusFlags &= ~SUPERTILE_IS_USED_THIS_FRAME;			// clear the isUsed bit

		}
	}
}

#pragma mark -


/******************* NEW SUPERTILE DEFORMATION ********************/

short NewSuperTileDeformation(DeformationType *data)
{
int	i;

				/* SCAN FOR A FREE SLOT */

	for (i = 0; i < MAX_DEFORMATIONS; i++)
	{
		if (gDeformationList[i].isUsed == false)
		{
				/* INSERT DATA IN THIS SLOT */

			gDeformationList[i] = *data;
			gDeformationList[i].isUsed = true;
			gNumTerrainDeformations++;
			return(i);
		}
	}

	return(-1);
}


/********************* DO SUPERTILE DEFORMATION *****************************/

static void DoSuperTileDeformation(SuperTileMemoryType *superTile)
{
int		v;
float	x,y,z, dist, decay, off, d2, originalY;
float	oneOverWaveLength,r,rw,dampenRatio;

	if ((gNumTerrainDeformations == 0) && (!gCleanupDeformation))
		return;

	long startRow = superTile->tileRow;													// get tile row/col of this supertile
	long startCol = superTile->tileCol;


			/***************************************/
			/* PROCESS EACH VERTEX FOR DEFORMATION */
			/***************************************/

	v = 0;
	for (int row = 0; row < (SUPERTILE_SIZE+1); row++)
	{
		for (int col = 0; col < (SUPERTILE_SIZE+1); col++)
		{
					/* GET ORIGINAL COORDS */

			x = superTile->meshData->points[v].x;
			z = superTile->meshData->points[v].z;
			originalY = y = gMapYCoordsOriginal[startRow+row][startCol+col];					// get original y value


				/*****************************************/
				/* APPLY ALL DEFORMATIONS TO THIS VERTEX */
				/*****************************************/

			dampenRatio = 1.0;								// assume no dampening

			for (int i = 0; i < MAX_DEFORMATIONS; i++)
			{
				if (!gDeformationList[i].isUsed)			// skip blank slots
					continue;

				oneOverWaveLength = gDeformationList[i].oneOverWaveLength;

				switch(gDeformationList[i].type)
				{
							/* JELLO SEA */

					case	DEFORMATION_TYPE_JELLO:
							dist = CalcQuickDistance(x,z, gDeformationList[i].origin.x, gDeformationList[i].origin.y);		// calc distance to origin
							dist += gDeformationList[i].radius;																// add in radius animation
							dist *= oneOverWaveLength;																		// adjust by wavelength
							y += sin(dist) * gDeformationList[i].amplitude;												// calc the new y value
							break;


							/* RADIAL WAVE */

					case	DEFORMATION_TYPE_RADIALWAVE:
							dist = CalcQuickDistance(x,z, gDeformationList[i].origin.x, gDeformationList[i].origin.y);			// calc distance to origin
							dist -= gDeformationList[i].radius;																// calc dist to the radial ring

							rw = gDeformationList[i].radialWidth;

							decay = (rw - fabs(dist)) / rw;
							if (decay < 0.0f)
								decay = 0.0f;

							dist *= oneOverWaveLength;																		// adjust by wavelength

							y += sin(dist) * (gDeformationList[i].amplitude * decay);									// calc the new y value

							break;


							/* CONTINUOUS WAVE */

					case	DEFORMATION_TYPE_CONTINUOUSWAVE:
							d2 = CalcQuickDistance(x,z, gDeformationList[i].origin.x, gDeformationList[i].origin.y);		// calc distance to origin
							dist = d2 + gDeformationList[i].radius;																// add in radius animation
							dist *= oneOverWaveLength;																		// adjust by wavelength
							off = sin(dist) * gDeformationList[i].amplitude;												// calc the new undecayed yoffset

							dist = d2 - gDeformationList[i].radialWidth;
							if (dist <= 0.0f)										// if within radial with then do full wave - no decay
							{
								y += off;
							}
							else													// decay beyond radial width
							{
								decay = gDeformationList[i].decayRate / dist;
								if (decay > 1.0f)
									decay = 1.0f;
								y += off * decay;																				// calc the new y value
							}
							break;

							/* DAMPEN */

					case	DEFORMATION_TYPE_DAMPEN:
							dist = CalcQuickDistance(x,z, gDeformationList[i].origin.x, gDeformationList[i].origin.y);			// calc distance to origin
							r = gDeformationList[i].radius;
							if (dist < r)
							{
								rw = gDeformationList[i].radialWidth;
								if (dist < rw)									// see if in dead zone
									dampenRatio = 0.0f;
								else
								{
									d2 = r - rw;								// size of decay margin
									dist -= rw;
									dampenRatio *= (dist / d2);
								}
							}
							break;


							/* WELL */

					case	DEFORMATION_TYPE_WELL:
							dist = CalcDistance(x,z, gDeformationList[i].origin.x, gDeformationList[i].origin.y);			// calc accurate distance to origin
							r = gDeformationList[i].radius;
							dist -= r;
							if (dist < 1.0f)
								dist = 1.0f;

							d2 = 30.0f / dist;
							if (d2 > dist)
								d2 = dist;

							y -= gDeformationList[i].amplitude * d2;
							break;

				}
			}

						/* DO DAMPENING RATIO */

			if (dampenRatio != 1.0f)
			{
				y = (y*dampenRatio) + ((1.0f - dampenRatio) * originalY);
			}

						/* SET Y COORD */

			superTile->meshData->points[v++].y = y;							// save final y coord
			gMapYCoords[startRow+row][startCol+col] = y;					// also save into master grid
		}
	}


			/*************************/
			/* UPDATE VERTEX NORMALS */
			/*************************/

	CalculateSupertileVertexNormals(superTile->meshData, startRow, startCol);


}


/*************** UPDATE TERRAIN DEFORMATION FUNCTION ***************/
//
// This does NOT actually do the terrain deformations.  This only updates
// the deformation functions.  The actual deformation is done during the Terrain
// draw loop above.
//

static void UpdateTerrainDeformationFunctions(void)
{
float	fps = gFramesPerSecondFrac;

	for (int i = 0; i < gNumTerrainDeformations; i++)
	{

		switch(gDeformationList[i].type)
		{
					/* JELLO SEA */

			case	DEFORMATION_TYPE_JELLO:
					gDeformationList[i].radius += fps * gDeformationList[i].speed;
					break;


					/* RADIAL WAVE */

			case	DEFORMATION_TYPE_RADIALWAVE:
					gDeformationList[i].amplitude -= fps * gDeformationList[i].decayRate;
					if (gDeformationList[i].amplitude <= 0.0f)						// see if deformation is done
					{
						DeleteTerrainDeformation(i);
						break;
					}
					gDeformationList[i].radius += fps * gDeformationList[i].speed;
					break;


					/* CONTINUOUS WAVE */

			case	DEFORMATION_TYPE_CONTINUOUSWAVE:
					gDeformationList[i].radius += fps * gDeformationList[i].speed;
					break;

					/* WELL */

			case	DEFORMATION_TYPE_WELL:
//					gDeformationList[i].speed += fps * 300.0f;
					gDeformationList[i].amplitude += fps * gDeformationList[i].speed;
					break;

		}
	}

}


/******************** DELETE TERRAIN DEFORMATION **********************/

void DeleteTerrainDeformation(short	i)
{
	if ((i < 0) || (i >= MAX_DEFORMATIONS))
		return;

	gDeformationList[i].isUsed = false;
	gNumTerrainDeformations--;

	if (gNumTerrainDeformations <= 0)			// if all done, then be sure to deform once more to reset everything
		gCleanupDeformation = true;
}



/************************ UPDATE DEFORMATION COORDS *************************/

void UpdateDeformationCoords(short defNum, float x, float z)
{
	if (defNum == -1)
		return;

	gDeformationList[defNum].origin.x	= x;
	gDeformationList[defNum].origin.y	= z;

}



#pragma mark -

/***************** GET TERRAIN HEIGHT AT COORD ******************/
//
// Given a world x/z coord, return the y coord based on height map
//
// INPUT: x/z = world coords
//
// OUTPUT: y = world y coord
//

float	GetTerrainY(float x, float z)
{
OGLPlaneEquation	planeEq;
int					row,col;
OGLPoint3D			p[4];
float				xi,zi;

	if (!gMapYCoords)													// make sure there's a terrain
		return(-10000);

				/* CALC TILE ROW/COL INFO */

	col = x * TERRAIN_POLYGON_SIZE_Frac;								// see which tile row/col we're on
	row = z * TERRAIN_POLYGON_SIZE_Frac;

	if ((col < 0) || (col >= gTerrainTileWidth))						// check bounds
		return(0);
	if ((row < 0) || (row >= gTerrainTileDepth))
		return(0);

	if (gLevelNum == LEVEL_NUM_CLOUD)									// special check on cloud level
	{
		uint16_t	 flags = GetTileAttribsAtRowCol(row, col);
		if (flags & TILE_ATTRIB_BLANK)									// see if on blank tile
		{
			gRecentTerrainNormal.x = gRecentTerrainNormal.z = 0;		// for blanks, return some really low y
			gRecentTerrainNormal.y = 1;
			return(BOTTOMLESS_PIT_Y);
		}
	}


	xi = x - (col * TERRAIN_POLYGON_SIZE);								// calc x/z offset into the tile
	zi = z - (row * TERRAIN_POLYGON_SIZE);


			/* BUILD VERTICES FOR THE 4 CORNERS OF THE TILE */

	p[0].x = col * TERRAIN_POLYGON_SIZE;								// far left
	p[0].y = gMapYCoords[row][col];
	p[0].z = row * TERRAIN_POLYGON_SIZE;

	p[1].x = p[0].x + TERRAIN_POLYGON_SIZE;								// far right
	p[1].y = gMapYCoords[row][col+1];
	p[1].z = p[0].z;

	p[2].x = p[1].x;													// near right
	p[2].y = gMapYCoords[row+1][col+1];
	p[2].z = p[1].z + TERRAIN_POLYGON_SIZE;

	p[3].x = col * TERRAIN_POLYGON_SIZE;								// near left
	p[3].y = gMapYCoords[row+1][col];
	p[3].z = p[2].z;


		/************************************/
		/* CALC PLANE EQUATION FOR TRIANGLE */
		/************************************/

	if (gMapSplitMode[row][col] == SPLIT_BACKWARD)			// if \ split
	{
		if (xi < zi)														// which triangle are we on?
			CalcPlaneEquationOfTriangle(&planeEq, &p[0], &p[2],&p[3]);		// calc plane equation for left triangle
		else
			CalcPlaneEquationOfTriangle(&planeEq, &p[0], &p[1], &p[2]);		// calc plane equation for right triangle
	}
	else																	// otherwise, / split
	{
		xi = TERRAIN_POLYGON_SIZE-xi;										// flip x
		if (xi > zi)
			CalcPlaneEquationOfTriangle(&planeEq, &p[0], &p[1], &p[3]);		// calc plane equation for left triangle
		else
			CalcPlaneEquationOfTriangle(&planeEq, &p[1], &p[2], &p[3]);		// calc plane equation for right triangle
	}

	gRecentTerrainNormal = planeEq.normal;									// remember the normal here

	return (IntersectionOfYAndPlane(x,z,&planeEq));							// calc intersection
}


/***************** GET TERRAIN HEIGHT AT COORD 2******************/
//
// Same as above except that it doesn't check for bottomless pits
//

float	GetTerrainY2(float x, float z)
{
OGLPlaneEquation	planeEq;
int					row,col;
OGLPoint3D			p[4];
float				xi,zi;

	if (!gMapYCoords)													// make sure there's a terrain
		return(-10000);

				/* CALC TILE ROW/COL INFO */

	col = x * TERRAIN_POLYGON_SIZE_Frac;								// see which tile row/col we're on
	row = z * TERRAIN_POLYGON_SIZE_Frac;

	if ((col < 0) || (col >= gTerrainTileWidth))						// check bounds
		return(0);
	if ((row < 0) || (row >= gTerrainTileDepth))
		return(0);


	xi = x - (col * TERRAIN_POLYGON_SIZE);								// calc x/z offset into the tile
	zi = z - (row * TERRAIN_POLYGON_SIZE);


			/* BUILD VERTICES FOR THE 4 CORNERS OF THE TILE */

	p[0].x = col * TERRAIN_POLYGON_SIZE;								// far left
	p[0].y = gMapYCoords[row][col];
	p[0].z = row * TERRAIN_POLYGON_SIZE;

	p[1].x = p[0].x + TERRAIN_POLYGON_SIZE;								// far right
	p[1].y = gMapYCoords[row][col+1];
	p[1].z = p[0].z;

	p[2].x = p[1].x;													// near right
	p[2].y = gMapYCoords[row+1][col+1];
	p[2].z = p[1].z + TERRAIN_POLYGON_SIZE;

	p[3].x = col * TERRAIN_POLYGON_SIZE;								// near left
	p[3].y = gMapYCoords[row+1][col];
	p[3].z = p[2].z;


		/************************************/
		/* CALC PLANE EQUATION FOR TRIANGLE */
		/************************************/

	if (gMapSplitMode[row][col] == SPLIT_BACKWARD)			// if \ split
	{
		if (xi < zi)														// which triangle are we on?
			CalcPlaneEquationOfTriangle(&planeEq, &p[0], &p[2],&p[3]);		// calc plane equation for left triangle
		else
			CalcPlaneEquationOfTriangle(&planeEq, &p[0], &p[1], &p[2]);		// calc plane equation for right triangle
	}
	else																	// otherwise, / split
	{
		xi = TERRAIN_POLYGON_SIZE-xi;										// flip x
		if (xi > zi)
			CalcPlaneEquationOfTriangle(&planeEq, &p[0], &p[1], &p[3]);		// calc plane equation for left triangle
		else
			CalcPlaneEquationOfTriangle(&planeEq, &p[1], &p[2], &p[3]);		// calc plane equation for right triangle
	}

	gRecentTerrainNormal = planeEq.normal;									// remember the normal here

	return (IntersectionOfYAndPlane(x,z,&planeEq));							// calc intersection
}



/***************** GET TERRAIN HEIGHT AT COORD: UNDEFORMED ******************/
//
// Same as above, but gets the undeformed y coord
//

float	GetTerrainY_Undeformed(float x, float z)
{
OGLPlaneEquation	planeEq;
int					row,col;
OGLPoint3D			p[4];
float				xi,zi;

	if (!gMapYCoordsOriginal)											// make sure there's a terrain
		return(-10000);

				/* CALC TILE ROW/COL INFO */

	col = x * TERRAIN_POLYGON_SIZE_Frac;								// see which tile row/col we're on
	row = z * TERRAIN_POLYGON_SIZE_Frac;

	if ((col < 0) || (col >= gTerrainTileWidth))						// check bounds
		return(0);
	if ((row < 0) || (row >= gTerrainTileDepth))
		return(0);

	xi = x - (col * TERRAIN_POLYGON_SIZE);								// calc x/z offset into the tile
	zi = z - (row * TERRAIN_POLYGON_SIZE);


			/* BUILD VERTICES FOR THE 4 CORNERS OF THE TILE */

	p[0].x = col * TERRAIN_POLYGON_SIZE;								// far left
	p[0].y = gMapYCoordsOriginal[row][col];
	p[0].z = row * TERRAIN_POLYGON_SIZE;

	p[1].x = p[0].x + TERRAIN_POLYGON_SIZE;								// far right
	p[1].y = gMapYCoordsOriginal[row][col+1];
	p[1].z = p[0].z;

	p[2].x = p[1].x;													// near right
	p[2].y = gMapYCoordsOriginal[row+1][col+1];
	p[2].z = p[1].z + TERRAIN_POLYGON_SIZE;

	p[3].x = col * TERRAIN_POLYGON_SIZE;								// near left
	p[3].y = gMapYCoordsOriginal[row+1][col];
	p[3].z = p[2].z;


		/************************************/
		/* CALC PLANE EQUATION FOR TRIANGLE */
		/************************************/

	if (gMapSplitMode[row][col] == SPLIT_BACKWARD)			// if \ split
	{
		if (xi < zi)														// which triangle are we on?
			CalcPlaneEquationOfTriangle(&planeEq, &p[0], &p[2],&p[3]);		// calc plane equation for left triangle
		else
			CalcPlaneEquationOfTriangle(&planeEq, &p[0], &p[1], &p[2]);		// calc plane equation for right triangle
	}
	else																	// otherwise, / split
	{
		xi = TERRAIN_POLYGON_SIZE-xi;										// flip x
		if (xi > zi)
			CalcPlaneEquationOfTriangle(&planeEq, &p[0], &p[1], &p[3]);		// calc plane equation for left triangle
		else
			CalcPlaneEquationOfTriangle(&planeEq, &p[1], &p[2], &p[3]);		// calc plane equation for right triangle
	}

	gRecentTerrainNormal = planeEq.normal;									// remember the normal here

	return (IntersectionOfYAndPlane(x,z,&planeEq));							// calc intersection
}

/***************** GET MIN TERRAIN Y ***********************/
//
// Uses the models's bounding box to find the lowest y for all sides
//

float	GetMinTerrainY(float x, float z, short group, short type, float scale)
{
float			y,minY;
OGLBoundingBox	*bBox;
float			minX,maxX,minZ,maxZ;

			/* POINT TO BOUNDING BOX */

	bBox =  &gObjectGroupBBoxList[group][type];				// get ptr to this model's bounding box

	minX = x + bBox->min.x * scale;
	maxX = x + bBox->max.x * scale;
	minZ = z + bBox->min.z * scale;
	maxZ = z + bBox->max.z * scale;


		/* GET CENTER */

	minY = GetTerrainY(x, z);

		/* CHECK FAR LEFT */

	y = GetTerrainY(minX, minZ);
	if (y < minY)
		minY = y;


		/* CHECK FAR RIGHT */

	y = GetTerrainY(maxX, minZ);
	if (y < minY)
		minY = y;

		/* CHECK FRONT LEFT */

	y = GetTerrainY(minX, maxZ);
	if (y < minY)
		minY = y;

		/* CHECK FRONT RIGHT */

	y = GetTerrainY(maxX, maxZ);
	if (y < minY)
		minY = y;

	return(minY);
}



/***************** GET SUPERTILE INFO ******************/
//
// Given a world x/z coord, return some supertile info
//
// INPUT: x/y = world x/y coords
// OUTPUT: row/col in tile coords and supertile coords
//

void GetSuperTileInfo(long x, long z, int* superCol, int* superRow, int* tileCol, int* tileRow)
{
	if ((x < 0) || (z < 0))									// see if out of bounds
		return;
	if ((x >= gTerrainUnitWidth) || (z >= gTerrainUnitDepth))
		return;

	int col = (int) (x * (1.0f/TERRAIN_SUPERTILE_UNIT_SIZE));	// calc supertile relative row/col that the coord lies on
	int row = (int) (z * (1.0f/TERRAIN_SUPERTILE_UNIT_SIZE));

	*superRow = row;										// return which supertile relative row/col it is
	*superCol = col;
	*tileRow = row*SUPERTILE_SIZE;							// return which tile row/col the super tile starts on
	*tileCol = col*SUPERTILE_SIZE;
}


/******************** GET TILE ATTRIBS *************************/

uint16_t	GetTileAttribsAtRowCol(int row, int col)
{
			/* CHECK BOUNDS */

	if ((col < 0) || (col >= gTerrainTileWidth))
		return(0);

	if ((row < 0) || (row >= gTerrainTileDepth))
		return(0);


			/* READ DATA FROM MAP */
			//
			// This is the only place that the tile grid is used by the game.
			//

	uint16_t tile = gTileGrid[row][col];						// get tile data from map

	gTileAttribFlags = (*gTileAttribList)[tile].flags;			// get attribute flags

	gTileAttribFlags = SwizzleUShort(&gTileAttribFlags);		// swizzle attrib

	return(gTileAttribFlags);
}




#pragma mark -

/******************** DO MY TERRAIN UPDATE ********************/

void DoPlayerTerrainUpdate(float x, float y)
{
Boolean		fullItemScan,moved;
Byte		mask;

#if (SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN) == 9
static const Byte gridMask[(SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN)*2]
						  [(SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN)*2] =
{
											// 0 == empty, 1 == draw tile, 2 == item add ring
	{0,0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,0},
	{0,0,0,0,2,2,1,1,1,1,1,1,2,2,2,0,0,0},
	{0,0,2,2,1,1,1,1,1,1,1,1,1,1,2,2,0,0},
	{0,2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,0},
	{0,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,0},
	{2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2},
	{0,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,0},
	{0,2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,0},
	{0,0,2,2,1,1,1,1,1,1,1,1,1,1,2,2,0,0},
	{0,0,0,2,2,2,1,1,1,1,1,1,2,2,2,0,0,0},
	{0,0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,0},
};

#elif (SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN) == 8
static const Byte gridMask[(SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN)*2]
						  [(SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN)*2] =
{
											// 0 == empty, 1 == draw tile, 2 == item add ring
	{0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0},
	{0,0,2,2,2,1,1,1,1,1,1,2,2,2,0,0},
	{0,2,2,1,1,1,1,1,1,1,1,1,1,2,2,0},
	{0,2,1,1,1,1,1,1,1,1,1,1,1,1,2,0},
	{2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2},
	{0,2,1,1,1,1,1,1,1,1,1,1,1,1,2,0},
	{0,2,2,1,1,1,1,1,1,1,1,1,1,2,2,0},
	{0,0,2,2,2,1,1,1,1,1,1,2,2,2,0,0},
	{0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0},
};

#elif (SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN) == 7

static const Byte gridMask[(SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN)*2]
						  [(SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN)*2] =
{
											// 0 == empty, 1 == draw tile, 2 == item add ring
	{0,0,2,2,2,2,2,2,2,2,2,2,0,0},
	{0,2,2,1,1,1,1,1,1,1,1,2,2,0},
	{2,2,1,1,1,1,1,1,1,1,1,1,2,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,1,1,2},
	{2,2,1,1,1,1,1,1,1,1,1,1,2,2},
	{0,2,2,1,1,1,1,1,1,1,1,1,2,0},
	{0,0,2,2,2,2,2,2,2,2,2,2,2,0},
};

#elif (SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN) == 6

static const Byte gridMask[(SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN)*2]
						  [(SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN)*2] =
{
											// 0 == empty, 1 == draw tile, 2 == item add ring
	{0,0,2,2,2,2,2,2,2,2,0,0},
	{0,2,2,1,1,1,1,1,1,2,2,0},
	{2,2,1,1,1,1,1,1,1,1,2,2},
	{2,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,1,1,2},
	{2,2,1,1,1,1,1,1,1,1,2,2},
	{0,2,2,1,1,1,1,1,1,2,2,0},
	{0,0,2,2,2,2,2,2,2,2,0,0},
};


#elif (SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN) == 5

static const Byte gridMask[(SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN)*2]
						  [(SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN)*2] =
{
											// 0 == empty, 1 == draw tile, 2 == item add ring
	{0,2,2,2,2,2,2,2,2,0},
	{2,2,1,1,1,1,1,1,2,2},
	{2,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,1,1,2},
	{2,2,1,1,1,1,1,1,2,2},
	{0,2,2,2,2,2,2,2,2,0},
};


#elif (SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN) == 4

static const Byte gridMask[(SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN)*2]
						  [(SUPERTILE_ACTIVE_RANGE+SUPERTILE_ITEMRING_MARGIN)*2] =
{
											// 0 == empty, 1 == draw tile, 2 == item add ring
	{2,2,2,2,2,2,2,2},
	{2,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,2},
	{2,1,1,1,1,1,1,2},
	{2,2,2,2,2,2,2,2},
};


#endif




		/* FIRST CLEAR OUT THE PLAYER FLAGS - ASSUME NO PLAYERS ON ANY SUPERTILES */

	for (long row = 0; row < gNumSuperTilesDeep; row++)
		for (long col = 0; col < gNumSuperTilesWide; col++)
			gSuperTileStatusGrid[row][col].playerHereFlag 	= false;

	gHiccupTimer = 0;



			/* CALC PIXEL COORDS OF FAR LEFT SUPER TILE */

	x -= (SUPERTILE_ACTIVE_RANGE*SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE);
	y -= (SUPERTILE_ACTIVE_RANGE*SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE);

	gCurrentSuperTileCol = x * TERRAIN_SUPERTILE_UNIT_SIZE_Frac;		// top/left row,col
	gCurrentSuperTileRow = y * TERRAIN_SUPERTILE_UNIT_SIZE_Frac;


				/* SEE IF ROW/COLUMN HAVE CHANGED */

	long deltaRow = labs(gCurrentSuperTileRow - gPreviousSuperTileRow);
	long deltaCol = labs(gCurrentSuperTileCol - gPreviousSuperTileCol);

	if (deltaRow || deltaCol)
	{
		moved = true;
		if ((deltaRow > 1) || (deltaCol > 1))								// if moved > 1 tile then need to do full item scan
			fullItemScan = true;
		else
			fullItemScan = false;
	}
	else
	{
		moved = false;
		fullItemScan = false;
	}


		/*****************************************************************/
		/* SCAN THE GRID AND SEE WHICH SUPERTILES NEED TO BE INITIALIZED */
		/*****************************************************************/

	long maxRow = gCurrentSuperTileRow + (SUPERTILE_ACTIVE_RANGE*2) + SUPERTILE_ITEMRING_MARGIN;
	long maxCol = gCurrentSuperTileCol + (SUPERTILE_ACTIVE_RANGE*2) + SUPERTILE_ITEMRING_MARGIN;

	for (long row = gCurrentSuperTileRow - SUPERTILE_ITEMRING_MARGIN, maskRow = 0; row < maxRow; row++, maskRow++)
	{
		if (row < 0)													// see if row is out of range
			continue;
		if (row >= gNumSuperTilesDeep)
			break;

		for (long col = gCurrentSuperTileCol-SUPERTILE_ITEMRING_MARGIN, maskCol = 0; col < maxCol; col++, maskCol++)
		{
			if (col < 0)												// see if col is out of range
				continue;
			if (col >= gNumSuperTilesWide)
				break;

						/* CHECK MASK AND SEE IF WE NEED THIS */

			mask = gridMask[maskRow][maskCol];

			if (mask == 0)
				continue;
			else
			{
				gSuperTileStatusGrid[row][col].playerHereFlag = true;						// player is using this supertile

							/*************************/
			                /* ONLY CREATE GEOMETRY  */
							/*************************/

						/* IS THIS SUPERTILE NOT ALREADY DEFINED? */

				if (!(gSuperTileStatusGrid[row][col].statusFlags & SUPERTILE_IS_DEFINED))
				{
					if (gSuperTileTextureGrid[row][col].superTileID != 0)					// supertiles with texture ID #0 are blank, so dont build them
					{
							gSuperTileStatusGrid[row][col].supertileIndex = BuildTerrainSuperTile(col * SUPERTILE_SIZE, row * SUPERTILE_SIZE);		// build the supertile
							gSuperTileStatusGrid[row][col].statusFlags = SUPERTILE_IS_DEFINED|SUPERTILE_IS_USED_THIS_FRAME;		// mark as defined & used
						}
				}
				else
    					gSuperTileStatusGrid[row][col].statusFlags |= SUPERTILE_IS_USED_THIS_FRAME;		// mark this as used
			}



					/* SEE IF ADD ITEMS ON THIS SUPERTILE */

			if (moved)																			// dont check for items if we didnt move
			{
				if (fullItemScan || (mask == 2))												// if full scan or mask is 2 then check for items
				{
					AddTerrainItemsOnSuperTile(row,col);
				}
			}
		}
	}

			/* UPDATE STUFF */

	gPreviousSuperTileRow = gCurrentSuperTileRow;
	gPreviousSuperTileCol = gCurrentSuperTileCol;

	CalcNewItemDeleteWindow();											// recalc item delete window


			/* UPDATE TERRAIN DEFORMATION FUNCTIONS */

	UpdateTerrainDeformationFunctions();
}


/****************** CALC NEW ITEM DELETE WINDOW *****************/

static void CalcNewItemDeleteWindow(void)
{
float	temp,temp2;

				/* CALC LEFT SIDE OF WINDOW */

	temp = gCurrentSuperTileCol*TERRAIN_SUPERTILE_UNIT_SIZE;			// convert to unit coords
	temp2 = temp - (ITEM_WINDOW+OUTER_SIZE)*TERRAIN_SUPERTILE_UNIT_SIZE;// factor window left
	gPlayerInfo.itemDeleteWindow.left = temp2;


				/* CALC RIGHT SIDE OF WINDOW */

	temp += SUPERTILE_DIST_WIDE*TERRAIN_SUPERTILE_UNIT_SIZE;			// calc offset to right side
	temp += (ITEM_WINDOW+OUTER_SIZE)*TERRAIN_SUPERTILE_UNIT_SIZE;		// factor window right
	gPlayerInfo.itemDeleteWindow.right = temp;


				/* CALC FAR SIDE OF WINDOW */

	temp = gCurrentSuperTileRow*TERRAIN_SUPERTILE_UNIT_SIZE;			// convert to unit coords
	temp2 = temp - (ITEM_WINDOW+OUTER_SIZE)*TERRAIN_SUPERTILE_UNIT_SIZE;// factor window top/back
	gPlayerInfo.itemDeleteWindow.top = temp2;


				/* CALC NEAR SIDE OF WINDOW */

	temp += SUPERTILE_DIST_DEEP*TERRAIN_SUPERTILE_UNIT_SIZE;			// calc offset to bottom side
	temp += (ITEM_WINDOW+OUTER_SIZE)*TERRAIN_SUPERTILE_UNIT_SIZE;		// factor window bottom/front
	gPlayerInfo.itemDeleteWindow.bottom = temp;
}





/*************** CALCULATE SPLIT MODE MATRIX ***********************/

void CalculateSplitModeMatrix(void)
{
int		row,col;
float	y0,y1,y2,y3;

	Alloc_2d_array(Byte, gMapSplitMode, gTerrainTileDepth, gTerrainTileWidth);	// alloc 2D array

	for (row = 0; row < gTerrainTileDepth; row++)
	{
		for (col = 0; col < gTerrainTileWidth; col++)
		{
				/* GET Y COORDS OF 4 VERTICES */

			y0 = gMapYCoords[row][col];
			y1 = gMapYCoords[row][col+1];
			y2 = gMapYCoords[row+1][col+1];
			y3 = gMapYCoords[row+1][col];

					/* QUICK CHECK FOR FLAT POLYS */

			if ((y0 == y1) && (y0 == y2) && (y0 == y3))							// see if all same level
				gMapSplitMode[row][col] = SPLIT_BACKWARD;

				/* CALC FOLD-SPLIT */
			else
			{
				if (fabs(y0-y2) < fabs(y1-y3))
					gMapSplitMode[row][col] = SPLIT_BACKWARD; 	// use \ splits
				else
					gMapSplitMode[row][col] = SPLIT_FORWARD;		// use / splits
			}



		}
	}
}











