//
// Terrain.h
//

#pragma once

#include "main.h"




enum
{
	MAP_ITEM_MYSTARTCOORD		= 0,			// map item # for my start coords
	MAP_ITEM_HUMAN				= 4,
	MAP_ITEM_ATOM				= 5,
	MAP_ITEM_POWERUPPOD			= 6,
	MAP_ITEM_EXITROCKET			= 26,
	MAP_ITEM_CHECKPOINT			= 27,
	MAP_ITEM_BUBBLEPUMP			= 37,
	MAP_ITEM_BLOBBOSS			= 41,
	MAP_ITEM_TELEPORTER			= 57,
	MAP_ITEM_ZIPLINE			= 58,
	MAP_ITEM_HUMAN_SCIENTIST	= 61,
	MAP_ITEM_BUMPERCARPOWER		= 80,
	MAP_ITEM_BUMPERCARGATE		= 82,
	MAP_ITEM_ROCKETSLED			= 83,
	MAP_ITEM_PEOPLE_HUT			= 101,
	MAP_ITEM_BRAINPORTAL		= 108,
};



		/* SUPER TILE MODES */

enum
{
	SUPERTILE_MODE_FREE,
	SUPERTILE_MODE_USED
};

#define	SUPERTILE_TEXMAP_SIZE		128												// the width & height of a supertile's texture

#define	OREOMAP_TILE_SIZE			16 												// pixel w/h of texture tile

#define	TERRAIN_POLYGON_SIZE		225.0f 											// size in world units of terrain polygon

#define	TERRAIN_POLYGON_SIZE_Frac	((float)1.0f/(float)TERRAIN_POLYGON_SIZE)

#define	SUPERTILE_SIZE				8  												// size of a super-tile / terrain object zone

#define	NUM_TRIS_IN_SUPERTILE		(SQUARED(SUPERTILE_SIZE) * 2)					// 2 triangles per tile
#define	NUM_VERTICES_IN_SUPERTILE	(SQUARED(SUPERTILE_SIZE + 1))					// # vertices in a supertile

#define	MAP2UNIT_VALUE				((float)TERRAIN_POLYGON_SIZE/OREOMAP_TILE_SIZE)	//value to xlate Oreo map pixel coords to 3-space unit coords

#define	TERRAIN_SUPERTILE_UNIT_SIZE	(SUPERTILE_SIZE*TERRAIN_POLYGON_SIZE)			// world unit size of a supertile
#define	TERRAIN_SUPERTILE_UNIT_SIZE_Frac (1.0f / TERRAIN_SUPERTILE_UNIT_SIZE)

#define	SUPERTILE_ACTIVE_RANGE		9
#define	SUPERTILE_ITEMRING_MARGIN	0												// # supertile margin for adding new items

#define	SUPERTILE_DIST_WIDE			(SUPERTILE_ACTIVE_RANGE*2)
#define	SUPERTILE_DIST_DEEP			(SUPERTILE_ACTIVE_RANGE*2)

								// # visible supertiles * 2 players * 2 buffers
								// We need the x2 buffer because we dont free unused supertiles
								// until after we've allocated new supertiles, so we'll always
								// need more supertiles than are actually ever used.
								// (Source port note: "2 players" here is probably a remnant from CMR)

#define	MAX_SUPERTILES			(SQUARED(SUPERTILE_ACTIVE_RANGE*2) * 2)	// the final *2 is because the old supertiles are not deleted until
																		// after new ones are created, thus we need some extas - worst case
																		// scenario is twice as many.



#define	MAX_TERRAIN_WIDTH		400
#define	MAX_TERRAIN_DEPTH		400

#define	MAX_SUPERTILES_WIDE		(MAX_TERRAIN_WIDTH/SUPERTILE_SIZE)
#define	MAX_SUPERTILES_DEEP		(MAX_TERRAIN_DEPTH/SUPERTILE_SIZE)


#define	MAX_SUPERTILE_TEXTURES	(MAX_SUPERTILES_WIDE*MAX_SUPERTILES_DEEP)


//=====================================================================


struct SuperTileMemoryType
{
	Byte				hiccupTimer;							// # frames to skip for use
	Byte				mode;									// free, used, etc.
	float				x,z,y;									// world coords
	long				left,back;								// integer coords of back/left corner
	long				tileRow,tileCol;						// tile row/col of the start of this supertile
	MOMaterialObject	*texture;								// refs to materials
	MOVertexArrayData	*meshData;								// mesh's data for the supertile
	OGLBoundingBox		bBox;									// bounding box
};
typedef struct SuperTileMemoryType SuperTileMemoryType;


typedef struct 
{
	uint16_t		flags;
	Byte			parm[2];
}TileAttribType;



typedef struct
{
	Boolean		    isEmpty;									// true if supertile is empty
	uint16_t		superTileID;								// ID # of supertile to use here
}SuperTileGridType;


typedef struct
{
	uint16_t	numItems;
	uint16_t	itemIndex;
}SuperTileItemIndexType;


		/* TILE ATTRIBUTE BITS */

enum
{
	TILE_ATTRIB_BLANK				=	1,
	TILE_ATTRIB_ELECTROCUTE_AREA0	=	(1<<1),
	TILE_ATTRIB_ELECTROCUTE_AREA1	=	(1<<2)
};

#define	BOTTOMLESS_PIT_Y	-100000.0f				// to identify a blank area on Cloud Level


		/* TERRAIN ITEM FLAGS */

enum
{
	ITEM_FLAGS_INUSE	=	(1),
	ITEM_FLAGS_USER1	=	(1<<1),
	ITEM_FLAGS_USER2	=	(1<<2)
};


enum
{
	SPLIT_BACKWARD = 0,
	SPLIT_FORWARD,
	SPLIT_ARBITRARY
};


typedef	struct
{
	uint16_t	supertileIndex;
	uint8_t		statusFlags;
	Boolean		playerHereFlag;
}SuperTileStatus;

enum									// statusFlags
{
	SUPERTILE_IS_DEFINED			=	1,
	SUPERTILE_IS_USED_THIS_FRAME	=	(1<<1)
};


typedef struct
{
	Boolean		isUsed;							
	Byte		type;
	float		amplitude;						// max height of wave
	float		radius;							// radius of deformation wave
	float		speed;							// speed of wave
	float		oneOverWaveLength;				// 1.0 / wavelength
	float		radialWidth;					// width of radial wave
	float		decayRate;						// decay rate of wave
	OGLVector2D	origin;							// origin of wave
}DeformationType;


#define	MAX_DEFORMATIONS	8

enum
{
	DEFORMATION_TYPE_JELLO,
	DEFORMATION_TYPE_RADIALWAVE,
	DEFORMATION_TYPE_CONTINUOUSWAVE,
	DEFORMATION_TYPE_DAMPEN,
	DEFORMATION_TYPE_WELL
};


//=====================================================================



void CreateSuperTileMemoryList(void);
void DisposeSuperTileMemoryList(void);
void DisposeTerrain(void);
void GetSuperTileInfo(long x, long z, int* superCol, int* superRow, int* tileCol, int* tileRow);
void InitTerrainManager(void);
float	GetTerrainY(float x, float z);
float	GetTerrainY2(float x, float z);
float	GetTerrainY_Undeformed(float x, float z);
float	GetMinTerrainY(float x, float z, short group, short type, float scale);
void InitCurrentScrollSettings(void);

void BuildTerrainItemList(void);
void AddTerrainItemsOnSuperTile(long row, long col);
Boolean TrackTerrainItem(ObjNode *theNode);
Boolean TrackTerrainItem_FromInit(ObjNode *theNode);
Boolean SeeIfCoordsOutOfRange(float x, float z);
void FindPlayerStartCoordItems(void);
void InitSuperTileGrid(void);
void RotateOnTerrain(ObjNode *theNode, float yOffset, OGLVector3D *surfaceNormal);
void DoPlayerTerrainUpdate(float x, float y);
void CalcTileNormals(long row, long col, OGLVector3D *n1, OGLVector3D *n2);
void CalcTileNormals_NotNormalized(long row, long col, OGLVector3D *n1, OGLVector3D *n2);
void CalculateSplitModeMatrix(void);
void CalculateSupertileVertexNormals(MOVertexArrayData	*meshData, long	startRow, long startCol);

uint16_t GetTileAttribsAtRowCol(int row, int col);
short NewSuperTileDeformation(DeformationType *data);
void DeleteTerrainDeformation(short	i);
void UpdateDeformationCoords(short defNum, float x, float z);

