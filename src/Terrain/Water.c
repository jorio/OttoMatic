/**********************/
/*   	WATER.C      */
/**********************/


#include "game.h"

/***************/
/* EXTERNALS   */
/***************/

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawWater(ObjNode *theNode);
static void MakeWaterGeometry(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define MAX_WATER			60
#define	MAX_NUBS_IN_WATER	80



/**********************/
/*     VARIABLES      */
/**********************/

long			gNumWaterPatches = 0;
short			gNumWaterDrawn;
WaterDefType	**gWaterListHandle = nil;
WaterDefType	*gWaterList = nil;


static MOVertexArrayData		gWaterTriMeshData[MAX_WATER];
static MOTriangleIndecies		gWaterTriangles[MAX_WATER][MAX_NUBS_IN_WATER*2];
static OGLPoint3D				gWaterPoints[MAX_WATER][MAX_NUBS_IN_WATER*2];
static OGLTextureCoord			gWaterUVs[MAX_WATER][MAX_NUBS_IN_WATER*2];
static OGLTextureCoord			gWaterUVs2[MAX_WATER][MAX_NUBS_IN_WATER*2];
OGLBoundingBox			gWaterBBox[MAX_WATER];

static const float gWaterTransparency[NUM_WATER_TYPES] =
{
	.7,				// blue water
	.6,				// soap
	.7,				// green water
	.9,				// oil
	.7,				// jungle water
	.7,				// mud
	.4,				// RADIOACTIVE
	.9,				// lava
};

static const Boolean gWaterGlow[NUM_WATER_TYPES] =
{
	false,				// blue water
	false,				// soap
	false,				// green water
	false,				// oil
	false,				// jungle water
	false,				// mud
	true,				// RADIOACTIVE
	true,				// lava
};

static const OGLTextureCoord	gWaterScrollUVDeltas[NUM_WATER_TYPES][2] =
{
	{ {.05f, .07f},		{.03f, .06f} },			// blue water
	{ {.05f, .07f},		{.03f, .06f} },			// soap
	{ {.05f, .07f},		{.03f, .06f} },			// green water
	{ {.05f, .07f},		{.03f, .06f} },			// oil
	{ {.05f, .07f},		{.03f, .06f} },			// jungle water
	{ {.05f, .07f},		{.03f, .06f} },			// mud
	{ {.05f, .07f},		{.03f, .06f} },			// RADIOACTIVE
	{ {.02f, .03f},		{.01f, .03f} },			// lava
};


/********************** DISPOSE WATER *********************/

void DisposeWater(void)
{

	if (!gWaterListHandle)
		return;

	DisposeHandle((Handle)gWaterListHandle);
	gWaterListHandle = nil;
	gWaterList = nil;
	gNumWaterPatches = 0;
}



/********************* PRIME WATER ***********************/
//
// Called during terrain prime function to initialize
//

void PrimeWater(void)
{
long					f,i,numNubs;
OGLPoint2D				*nubs;
ObjNode					*obj;
float					y,centerX,centerZ;


	if (gNumWaterPatches > MAX_WATER)
		DoFatalAlert("PrimeWater: gNumWaterPatches > MAX_WATER");


			/******************************/
			/* ADJUST TO GAME COORDINATES */
			/******************************/

	for (f = 0; f < gNumWaterPatches; f++)
	{
		nubs 				= &gWaterList[f].nubList[0];				// point to nub list
		numNubs 			= gWaterList[f].numNubs;					// get # nubs in water

		if (numNubs == 1)
			DoFatalAlert("PrimeWater: numNubs == 1");

		if (numNubs > MAX_NUBS_IN_WATER)
			DoFatalAlert("PrimeWater: numNubs > MAX_NUBS_IN_WATER");


				/* IF FIRST AND LAST NUBS ARE SAME, THEN ELIMINATE LAST */

		if ((nubs[0].x == nubs[numNubs-1].x) &&
			(nubs[0].y == nubs[numNubs-1].y))
		{
			numNubs--;
			gWaterList[f].numNubs = numNubs;
		}


				/* CONVERT TO WORLD COORDS */

		for (i = 0; i < numNubs; i++)
		{
			nubs[i].x *= MAP2UNIT_VALUE;
			nubs[i].y *= MAP2UNIT_VALUE;
		}


				/***********************/
				/* CREATE VERTEX ARRAY */
				/***********************/

					/* FIND Y @ HOT SPOT */

		y =  GetTerrainY(gWaterList[f].hotSpotX * MAP2UNIT_VALUE, gWaterList[f].hotSpotZ * MAP2UNIT_VALUE) + 100.0f;

		for (i = 0; i < numNubs; i++)
		{
			gWaterPoints[f][i].x = nubs[i].x;
			gWaterPoints[f][i].y = y;
			gWaterPoints[f][i].z = nubs[i].y;
		}

			/* APPEND THE CENTER POINT TO THE POINT LIST */

		centerX = centerZ = 0;											// calc average of points
		for (i = 0; i < numNubs; i++)
		{
			centerX += gWaterPoints[f][i].x;
			centerZ += gWaterPoints[f][i].z;
		}
		centerX /= (float)numNubs;
		centerZ /= (float)numNubs;

		gWaterPoints[f][numNubs].x = centerX;
		gWaterPoints[f][numNubs].z = centerZ;
		gWaterPoints[f][numNubs].y = y;
	}

			/*****************************/
			/* MAKE WATER GEOMETRY */
			/*****************************/

	MakeWaterGeometry();

		/*************************************************************************/
		/* CREATE DUMMY CUSTOM OBJECT TO CAUSE WATER DRAWING AT THE DESIRED TIME */
		/*************************************************************************/

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.slot 		= WATER_SLOT;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOLIGHTING|STATUS_BIT_DONTCULL;

	obj = MakeNewObject(&gNewObjectDefinition);
	obj->CustomDrawFunction = DrawWater;
}


/*************** MAKE WATER GEOMETRY *********************/

static void MakeWaterGeometry(void)
{
int						f;
u_short					type, numNubs;
long					i;
WaterDefType			*water;
float					minX,minY,minZ,maxX,maxY,maxZ;
double					x,y,z;

	for (f = 0; f < gNumWaterPatches; f++)
	{
				/* GET WATER INFO */

		water = &gWaterList[f];								// point to this water
		numNubs = water->numNubs;							// get # nubs in water (note:  this is the # from the file, not including the extra center point we added earlier!)
		if (numNubs < 3)
			DoFatalAlert("MakeWaterGeometry: numNubs < 3");
		type = water->type;									// get water type


					/***************************/
					/* SET VERTEX ARRAY HEADER */
					/***************************/

		gWaterTriMeshData[f].points 					= &gWaterPoints[f][0];
		gWaterTriMeshData[f].triangles					= &gWaterTriangles[f][0];
		gWaterTriMeshData[f].uvs[0]						= &gWaterUVs[f][0];
		gWaterTriMeshData[f].uvs[1]						= &gWaterUVs2[f][0];
		gWaterTriMeshData[f].normals					= nil;
		gWaterTriMeshData[f].colorsByte					= nil;
		gWaterTriMeshData[f].colorsFloat				= nil;
		gWaterTriMeshData[f].numPoints 					= numNubs+1;					// +1 is to include the extra center point
		gWaterTriMeshData[f].numTriangles 				= numNubs;


				/* BUILD TRIANGLE INFO */

		for (i = 0; i < gWaterTriMeshData[f].numTriangles; i++)
		{
			gWaterTriangles[f][i].vertexIndices[0] = numNubs;							// vertex 0 is always the radial center that we appended to the end of the list
			gWaterTriangles[f][i].vertexIndices[1] = i + 0;
			gWaterTriangles[f][i].vertexIndices[2] = i + 1;

			if (gWaterTriangles[f][i].vertexIndices[2] == numNubs)						// check for wrap back
				 gWaterTriangles[f][i].vertexIndices[2] = 0;
		}


				/* SET TEXTURE */

		gWaterTriMeshData[f].numMaterials	= 2;
		gWaterTriMeshData[f].materials[0] 	= 											// set illegal ref to material
		gWaterTriMeshData[f].materials[1] 	= gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_Water+type].materialObject;


				/*************/
				/* CALC BBOX */
				/*************/

		maxX = maxY = maxZ = -1000000;									// build new bboxes while we do this
		minX = minY = minZ = -maxX;

		for (i = 0; i < numNubs; i++)
		{

					/* GET COORDS */

			x = gWaterPoints[f][i].x;
			y = gWaterPoints[f][i].y;
			z = gWaterPoints[f][i].z;

					/* CHECK BBOX */

			if (x < minX)	minX = x;									// find min/max bounds for bbox
			if (x > maxX)	maxX = x;
			if (z < minZ)	minZ = z;
			if (z > maxZ)	maxZ = z;
			if (y < minY)	minY = y;
			if (y > maxY)	maxY = y;
		}

				/* SET CALCULATED BBOX */

		gWaterBBox[f].min.x = minX;
		gWaterBBox[f].max.x = maxX;
		gWaterBBox[f].min.y = minY;
		gWaterBBox[f].max.y = maxY;
		gWaterBBox[f].min.z = minZ;
		gWaterBBox[f].max.z = maxZ;
		gWaterBBox[f].isEmpty = false;


				/**************/
				/* BUILD UV's */
				/**************/

		for (i = 0; i <= numNubs; i++)
		{
			x = gWaterPoints[f][i].x;
			y = gWaterPoints[f][i].y;
			z = gWaterPoints[f][i].z;

			gWaterUVs[f][i].u 	= x * .002;
			gWaterUVs[f][i].v 	= z * .002;
			gWaterUVs2[f][i].u 	= x * .0015;
			gWaterUVs2[f][i].v 	= z * .0015;
		}
	}
}


#pragma mark -


/********************* DRAW WATER ***********************/

static void DrawWater(ObjNode *theNode)
{
long	f,i;
float	fps = gFramesPerSecondFrac;
float	ud1, uv1, ud2, uv2;

#pragma unused(theNode)


			/*******************/
			/* DRAW EACH WATER */
			/*******************/

	gNumWaterDrawn = 0;

	for (f = 0; f < gNumWaterPatches; f++)
	{
		short	waterType = gWaterList[f].type;

				/* DO BBOX CULLING */

		if (OGL_IsBBoxVisible(&gWaterBBox[f], nil))
		{
			if (gG4)
				gGlobalTransparency = gWaterTransparency[waterType];
			else
				gGlobalTransparency = 1.0f;

			if (gWaterGlow[waterType])								// set glow
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			else
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			MO_DrawGeometry_VertexArray(&gWaterTriMeshData[f]);
			gNumWaterDrawn++;
		}


		/********************************/
		/* ANIMATE UVS WHILE WE'RE HERE */
		/********************************/
		//
		// Unfortunately, we need to do UV animation on all water patches regardless if they are drawn or not
		// because the edges of adjacent patches must always match / be synchronized.
		//

		ud1 = gWaterScrollUVDeltas[waterType][0].u * fps;
		uv1 = gWaterScrollUVDeltas[waterType][0].v * fps;
		ud2 = gWaterScrollUVDeltas[waterType][1].u * fps;
		uv2 = gWaterScrollUVDeltas[waterType][1].v * fps;

		for (i = 0; i <= gWaterList[f].numNubs; i++)
		{
			gWaterUVs[f][i].u 	+= ud1;
			gWaterUVs[f][i].v 	+= uv1;

			gWaterUVs2[f][i].u 	-= ud2;
			gWaterUVs2[f][i].v 	-= uv2;
		}

	}

	gGlobalTransparency = 1.0;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

#pragma mark -

/**************** DO WATER COLLISION DETECT ********************/

Boolean DoWaterCollisionDetect(ObjNode *theNode, float x, float y, float z, int *patchNum)
{
int	i;

	for (i = 0; i < gNumWaterPatches; i++)
	{
				/* QUICK CHECK TO SEE IF IS IN BBOX */

		if ((x < gWaterBBox[i].min.x) || (x > gWaterBBox[i].max.x) ||
			(z < gWaterBBox[i].min.z) || (z > gWaterBBox[i].max.z) ||
			(y > gWaterBBox[i].max.y))
			continue;

					/* NOW CHECK IF INSIDE THE POLYGON */

		if (!IsPointInPoly2D(x, z, gWaterList[i].numNubs, gWaterList[i].nubList))
			continue;


					/* WE FOUND A HIT */

		theNode->StatusBits |= STATUS_BIT_UNDERWATER;
		if (patchNum)
			*patchNum = i;
		return(true);
	}

				/* NOT IN WATER */

	theNode->StatusBits &= ~STATUS_BIT_UNDERWATER;
	if (patchNum)
		*patchNum = 0;
	return(false);
}


/**************** GET WATER Y  ********************/

Boolean GetWaterY(float x, float z, float *y)
{
int	i;

	for (i = 0; i < gNumWaterPatches; i++)
	{
				/* QUICK CHECK TO SEE IF IS IN BBOX */

		if ((x < gWaterBBox[i].min.x) || (x > gWaterBBox[i].max.x) ||
			(z < gWaterBBox[i].min.z) || (z > gWaterBBox[i].max.z))
			continue;

					/* NOW CHECK IF INSIDE THE POLYGON */

		if (!IsPointInPoly2D(x, z, gWaterList[i].numNubs, gWaterList[i].nubList))
			continue;


					/* WE FOUND A HIT */

		*y = gWaterBBox[i].max.y;						// return y
		return(true);
	}

				/* NOT IN WATER */

	*y = 0;
	return(false);
}






