/****************************/
/*   	ZIPLINE.C		    */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void GenerateZipSpline(short zipNum, OGLPoint3D *nubPoints);
static void DrawZipLines(ObjNode *theNode);
static void BuildZipGeometry(short zipNum);
static void AttachPullyToZip(short zipNum);
static void MoveZipPully_Waiting(ObjNode *theNode);
static void MoveZipPully_Moving(ObjNode *theNode);
static void MoveZipPully_Completed(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_ZIP_LINES	15

#define	MAX_POINTS_PER_SPAN		1600
#define	MAX_ZIP_SPLINE_POINTS	(MAX_POINTS_PER_SPAN * 3)

#define	ZIP_GEOMETRY_DENSITY	80					// bigger == less resolution in geometry

#define	MAX_ZIP_GEOMETRY_POINTS	(MAX_ZIP_SPLINE_POINTS/ZIP_GEOMETRY_DENSITY*4+4)

#define	ZIP_THICKNESS		5.0f

typedef struct
{
	Boolean		isUsed;
	OGLPoint3D	start,end;
	short		numPoints;
	OGLPoint3D	splinePoints[MAX_ZIP_SPLINE_POINTS];
	OGLBoundingBox	bbox;

	MOVertexArrayData	mesh;
	OGLPoint3D			meshPoints[MAX_ZIP_GEOMETRY_POINTS];
	OGLTextureCoord		meshUVs[MAX_ZIP_GEOMETRY_POINTS];
	MOTriangleIndecies	meshTriangles[MAX_ZIP_GEOMETRY_POINTS];

}ZipLineType;


#define	ZIP_LINE_HEIGHT	500.0f

#define	ZIP_DIP			(ZIP_LINE_HEIGHT * .9f)

#define	ZIP_PULLY_STARTOFF	20.0f

/*********************/
/*    VARIABLES      */
/*********************/

#define	ZipID			Special[0]

static ZipLineType	gZipLines[MAX_ZIP_LINES];


ObjNode	*gCurrentZip;


/************************ INIT ZIP LINES *************************/

void InitZipLines(void)
{
long					i;
TerrainItemEntryType	*itemPtr;
float					x,z;
short					id;
OGLPoint3D				nubs[4];
ObjNode					*newObj;

	gCurrentZip = nil;

	for (i = 0; i < MAX_ZIP_LINES; i++)								// init the list
		gZipLines[i].isUsed = false;


			/*****************************/
			/* FIND ALL OF THE ZIP LINES */
			/*****************************/

	itemPtr = *gMasterItemList; 									// get pointer to data inside the LOCKED handle

	for (i= 0; i < gNumTerrainItems; i++)
	{
		if (itemPtr[i].type == MAP_ITEM_ZIPLINE)					// see if it's what we're looking for
		{
			id = itemPtr[i].parm[0];								// get zip ID
			if (id >= MAX_ZIP_LINES)
				DoFatalAlert("InitZipLines: zip line ID# >= MAX_ZIP_LINES");

			gZipLines[id].isUsed = true;							// this ID is used

			x = itemPtr[i].x;										// get coords
			z = itemPtr[i].y;

			if (itemPtr[i].parm[1] == 0)							// see if start or end pt
			{
				gZipLines[id].start.x = x;
				gZipLines[id].start.z = z;
				gZipLines[id].start.y = GetTerrainY_Undeformed(x,z);
			}
			else													// end pt
			{
				gZipLines[id].end.x = x;
				gZipLines[id].end.z = z;
				gZipLines[id].end.y = GetTerrainY_Undeformed(x,z);
			}
		}
	}


		/**************************************/
		/* BUILD THE SPLINES FOR EACH ZIPLINE */
		/**************************************/

	for (i = 0; i < MAX_ZIP_LINES; i++)
	{
		if (!gZipLines[i].isUsed)									// see if this one was used
			continue;


			/* CREATE NUB ARRAY */

		nubs[0] = gZipLines[i].start;								// set start & end nubs
		nubs[0].y += ZIP_LINE_HEIGHT;
		nubs[3] = gZipLines[i].end;
		nubs[3].y += ZIP_LINE_HEIGHT;

		nubs[1].x = nubs[0].x + ((nubs[3].x - nubs[0].x) * (1.0f/3.0f));					// calc 2nd segment @ 1/3rd dist
		nubs[1].y = nubs[0].y + ((nubs[3].y - nubs[0].y) * (1.0f/3.0f) - ZIP_DIP);
		nubs[1].z = nubs[0].z + ((nubs[3].z - nubs[0].z) * (1.0f/3.0f));

		nubs[2].x = nubs[0].x + ((nubs[3].x - nubs[0].x) * (2.0f/3.0f));					// calc 3rd segment @ 2/3rd dist
		nubs[2].y = nubs[0].y + ((nubs[3].y - nubs[0].y) * (2.0f/3.0f) - ZIP_DIP);
		nubs[2].z = nubs[0].z + ((nubs[3].z - nubs[0].z) * (2.0f/3.0f));


			/* CALC SPLINE */

		GenerateZipSpline(i, nubs);


			/* BUILD THE GEOMETRY */

		BuildZipGeometry(i);


			/* BUILD THE PULLY */

		AttachPullyToZip(i);
	}


		/****************************/
		/* CREATE DUMMY DRAW OBJECT */
		/****************************/

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.slot 		= 105;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.scale 		= 1;
	gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOLIGHTING;

	newObj = MakeNewObject(&gNewObjectDefinition);
	newObj->CustomDrawFunction = DrawZipLines;

}



#pragma mark -

/************************* ADD ZIP LINE POST *********************************/

Boolean AddZipLinePost(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	if (gLevelNum == LEVEL_NUM_APOCALYPSE)
		gNewObjectDefinition.type 	= APOCALYPSE_ObjType_ZipLinePost;
	else
		gNewObjectDefinition.type 	= FIREICE_ObjType_ZipLinePost;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 70;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->ZipID = itemPtr->parm[0];						// get id#


			/* SET COLLISION STUFF */

//	newObj->CType 			= CTYPE_MISC|CTYPE_IMPENETRABLE|CTYPE_BLOCKCAMERA;
//	newObj->CBits			= CBITS_ALLSOLID;
//	CreateCollisionBoxFromBoundingBox_Rotated(newObj,.8,1);

	return(true);													// item was added
}




/******************** GENERATE SPLINE POINTS ***************************/

static void GenerateZipSpline(short zipNum, OGLPoint3D *nubPoints)
{
OGLPoint3D	**space,*splinePoints;
OGLPoint3D 	*a, *b, *c, *d;
OGLPoint3D 	*h0, *h1, *h2, *h3, *hi_a;
short 		imax;
float 		t, dt;
long		i,i1,numPoints;
long		nub;
long		maxX,minX,maxZ,minZ,maxY,minY;
int			pointsPerSpan;

const short numNubs = 4;

	maxX = maxY = maxZ = -10000000;
	minX = minY = minZ = -maxX;

				/* CALC POINT DENSITY */

	pointsPerSpan = OGLPoint3D_Distance(&nubPoints[0], &nubPoints[1]) * .2f;		// # points per span is a function of the length of the spline, so measure from start to end
	if (pointsPerSpan > MAX_POINTS_PER_SPAN)
		DoFatalAlert("GenerateZipSpline: pointsPerSpan > MAX_POINTS_PER_SPAN");


		/* GET SPLINE INFO */

	splinePoints = gZipLines[zipNum].splinePoints;						// point to point list



				/* ALLOCATE 2D ARRAY FOR CALCULATIONS */

	Alloc_2d_array(OGLPoint3D, space, 8, numNubs);


		/*******************************************************/
		/* DO MAGICAL CUBIC SPLINE CALCULATIONS ON CONTROL PTS */
		/*******************************************************/

	h0 = space[0];
	h1 = space[1];
	h2 = space[2];
	h3 = space[3];

	a = space[4];
	b = space[5];
	c = space[6];
	d = space[7];


				/* COPY CONTROL POINTS INTO ARRAY */

	for (i = 0; i < numNubs; i++)
		d[i] = nubPoints[i];


	for (i = 0, imax = numNubs - 2; i < imax; i++)
	{
		h2[i].x = h2[i].y = h2[i].z = 1;
		h3[i].x = 3 *(d[i+ 2].x - 2 * d[i+ 1].x + d[i].x);
		h3[i].y = 3 *(d[i+ 2].y - 2 * d[i+ 1].y + d[i].y);
		h3[i].z = 3 *(d[i+ 2].z - 2 * d[i+ 1].z + d[i].z);
	}
  	h2[numNubs - 3].x = h2[numNubs - 3].z = 0;

	a[0].x = a[0].y = a[0].z = 4;
	h1[0].x = h3[0].x / a[0].x;
	h1[0].y = h3[0].y / a[0].y;
	h1[0].z = h3[0].z / a[0].z;

	for (i = 1, i1 = 0, imax = numNubs - 2; i < imax; i++, i1++)
	{
		h0[i1].x = h2[i1].x / a[i1].x;
		a[i].x = 4.0f - h0[i1].x;
		h1[i].x = (h3[i].x - h1[i1].x) / a[i].x;

		h0[i1].y = h2[i1].y / a[i1].y;
		a[i].y = 4.0f - h0[i1].y;
		h1[i].y = (h3[i].y - h1[i1].y) / a[i].y;

		h0[i1].z = h2[i1].z / a[i1].z;
		a[i].z = 4.0f - h0[i1].z;
		h1[i].z = (h3[i].z - h1[i1].z) / a[i].z;
	}

	b[numNubs - 3] = h1[numNubs - 3];

	for (i = numNubs - 4; i >= 0; i--)
	{
 		b[i].x = h1[i].x - h0[i].x * b[i+ 1].x;
 		b[i].y = h1[i].y - h0[i].y * b[i+ 1].y;
 		b[i].z = h1[i].z - h0[i].z * b[i+ 1].z;
 	}

	for (i = numNubs - 2; i >= 1; i--)
		b[i] = b[i - 1];

	b[0].x = b[numNubs - 1].x =
	b[0].y = b[numNubs - 1].y =
	b[0].z = b[numNubs - 1].z = 0;
	hi_a = a + numNubs - 1;

	for (; a < hi_a; a++, b++, c++, d++)
	{
		c->x = ((d+1)->x - d->x) -(2.0f * b->x + (b+1)->x) * (1.0f/3.0f);
		a->x = ((b+1)->x - b->x) * (1.0f/3.0f);

		c->y = ((d+1)->y - d->y) -(2.0f * b->y + (b+1)->y) * (1.0f/3.0f);
		a->y = ((b+1)->y - b->y) * (1.0f/3.0f);

		c->z = ((d+1)->z - d->z) -(2.0f * b->z + (b+1)->z) * (1.0f/3.0f);
		a->z = ((b+1)->z - b->z) * (1.0f/3.0f);
	}

		/***********************************/
		/* NOW CALCULATE THE SPLINE POINTS */
		/***********************************/

	a = space[4];
	b = space[5];
	c = space[6];
	d = space[7];

  	numPoints = 0;
	for (nub = 0; a < hi_a; a++, b++, c++, d++, nub++)
	{

				/* CALC THIS SPAN */

		dt = 1.0f / pointsPerSpan;
		for (t = 0; t < (1.0f - EPS); t += dt)
		{
			if (numPoints >= MAX_ZIP_SPLINE_POINTS)				// see if overflow
				DoFatalAlert("GenerateZipSpline: numPoints >= MAX_ZIP_SPLINE_POINTS");

 			splinePoints[numPoints].x = ((a->x * t + b->x) * t + c->x) * t + d->x;		// save point
 			splinePoints[numPoints].y = ((a->y * t + b->y) * t + c->y) * t + d->y;
 			splinePoints[numPoints].z = ((a->z * t + b->z) * t + c->z) * t + d->z;

 					/* UPDATE BBOX */

 			if (splinePoints[numPoints].x > maxX)
 				maxX = splinePoints[numPoints].x;
 			if (splinePoints[numPoints].x < minX)
 				minX = splinePoints[numPoints].x;

 			if (splinePoints[numPoints].y > maxY)
 				maxY = splinePoints[numPoints].y;
 			if (splinePoints[numPoints].y < minY)
 				minY = splinePoints[numPoints].y;

 			if (splinePoints[numPoints].z > maxZ)
 				maxZ = splinePoints[numPoints].z;
 			if (splinePoints[numPoints].z < minZ)
 				minZ = splinePoints[numPoints].z;

 			numPoints++;
 		}
	}


		/* END */

	gZipLines[zipNum].bbox.min.x = minX;
	gZipLines[zipNum].bbox.max.x = maxX;

	gZipLines[zipNum].bbox.min.y = minY;
	gZipLines[zipNum].bbox.max.y = maxY;

	gZipLines[zipNum].bbox.min.z = minZ;
	gZipLines[zipNum].bbox.max.z = maxZ;

	gZipLines[zipNum].numPoints = numPoints;
	Free_2d_array(space);
}


/*************************** BUILD ZIP GEOMETRY *********************************/

static void BuildZipGeometry(short zipNum)
{
MOVertexArrayData	*mesh		= &gZipLines[zipNum].mesh;
OGLPoint3D			*meshPoints = &gZipLines[zipNum].meshPoints[0];
OGLTextureCoord		*uvs 		= &gZipLines[zipNum].meshUVs[0];
MOTriangleIndecies	*triangles 	= &gZipLines[zipNum].meshTriangles[0];

OGLPoint3D			*splinePoints = &gZipLines[zipNum].splinePoints[0];
int					i,p,t;
float				u;

OGLVector3D			v1;
static const OGLVector3D	up = {0,1,0};

			/* INIT MESH BASICS */

	mesh->numMaterials = 1;										// 1 material
	if (gLevelNum == LEVEL_NUM_APOCALYPSE)
		mesh->materials[0] = gSpriteGroupList[SPRITE_GROUP_LEVELSPECIFIC][APOCALYPSE_SObjType_Rope].materialObject;	// set ILLEGAL ref to this texture
	else
		mesh->materials[0] = gSpriteGroupList[SPRITE_GROUP_LEVELSPECIFIC][FIREICE_SObjType_Rope].materialObject;
	mesh->points 		= meshPoints;
	mesh->normals 		= nil;
	mesh->uvs[0]		= uvs;
	mesh->colorsByte 	= nil;
	mesh->colorsFloat 	= nil;
	mesh->triangles 	= triangles;



			/* BUILD THE VERTEX LIST */

	v1.x = gZipLines[zipNum].end.x - gZipLines[zipNum].start.x;				// calc direction vector
	v1.y = gZipLines[zipNum].end.y - gZipLines[zipNum].start.y;
	v1.z = gZipLines[zipNum].end.z - gZipLines[zipNum].start.z;
	OGLVector3D_Normalize(&v1, &v1);
	OGLVector3D_Cross(&v1, &up, &v1);										// calc side vector
	v1.x *= ZIP_THICKNESS;													// scale vector
	v1.y *= ZIP_THICKNESS;
	v1.z *= ZIP_THICKNESS;

	p = 0;
	u = 0.0f;
	for (i = 0; i < gZipLines[zipNum].numPoints; i += ZIP_GEOMETRY_DENSITY)
	{
		if (p >= MAX_ZIP_GEOMETRY_POINTS)
			DoFatalAlert("BuildZipGeometry: p >= MAX_ZIP_GEOMETRY_POINTS");

		meshPoints[p] 	= 									// start 4 verts
		meshPoints[p+1] =
		meshPoints[p+2] =
		meshPoints[p+3] = splinePoints[i];

		meshPoints[p].y 	+= ZIP_THICKNESS;				// top vertex
		meshPoints[p+1].y 	-= ZIP_THICKNESS;				// bottom vertex

		meshPoints[p+2].x 	-= v1.x;						// left
		meshPoints[p+2].z 	-= v1.z;

		meshPoints[p+3].x 	+= v1.x;						// right
		meshPoints[p+3].z 	+= v1.z;

		uvs[p].u 	= u;									// top uv
		uvs[p].v 	= 1.0;

		uvs[p+1].u 	= u;									// bottom uv
		uvs[p+1].v 	= 0;

		uvs[p+2].u 	= u;									// left uv
		uvs[p+2].v 	= 1.0;

		uvs[p+3].u 	= u;									// right uv
		uvs[p+3].v 	= 0;

		p += 4;
		u += 12.0f;
	}

	meshPoints[p] =
	meshPoints[p+1] =
	meshPoints[p+2] =
	meshPoints[p+3] = gZipLines[zipNum].splinePoints[gZipLines[zipNum].numPoints-1];			// set final end points

	meshPoints[p].y 	+= ZIP_THICKNESS;				// top vertex
	meshPoints[p+1].y 	-= ZIP_THICKNESS;				// bottom vertex

	meshPoints[p+2].x 	-= v1.x;						// left
	meshPoints[p+2].z 	-= v1.z;

	meshPoints[p+3].x 	+= v1.x;						// right
	meshPoints[p+3].z 	+= v1.z;

	uvs[p].u 	= u;									// top uv
	uvs[p].v 	= 1.0;

	uvs[p+1].u 	= u;									// bottom uv
	uvs[p+1].v 	= 0;

	uvs[p+2].u 	= u;									// left uv
	uvs[p+2].v 	= 1.0;

	uvs[p+3].u 	= u;									// right uv
	uvs[p+3].v 	= 0;

	p += 4;
	if (p > MAX_ZIP_GEOMETRY_POINTS)
		DoFatalAlert("BuildZipGeometry: p > MAX_ZIP_GEOMETRY_POINTS");

	mesh->numPoints 	= p;
	mesh->numTriangles 	= p-4;


		/* BUILD THE TRIANGLE LIST */

	t = 0;
	p = 0;
	for (i = 0; i < mesh->numTriangles; i+=2)
	{
		if (t >= MAX_ZIP_GEOMETRY_POINTS)
			DoFatalAlert("BuildZipGeometry: t >= MAX_ZIP_GEOMETRY_POINTS");

		triangles[t].vertexIndices[0] = p;
		triangles[t].vertexIndices[1] = p+1;
		triangles[t].vertexIndices[2] = p+4;
		t++;

		triangles[t].vertexIndices[0] = p+1;
		triangles[t].vertexIndices[1] = p+5;
		triangles[t].vertexIndices[2] = p+4;
		t++;

		p += 2;
	}
}

/******************** DRAW ZIP LINES *************************/

static void DrawZipLines(ObjNode *theNode)
{
short	i;

#pragma unused(theNode)

	for (i = 0; i < MAX_ZIP_LINES; i++)
	{
		if (!gZipLines[i].isUsed)									// see if this one was used
			continue;

		if (OGL_IsBBoxVisible(&gZipLines[i].bbox, nil))				// draw if not culled
			MO_DrawGeometry_VertexArray(&gZipLines[i].mesh);
	}
}

#pragma mark -

/************************* ATTACH PULLY TO ZIP ************************/

static void AttachPullyToZip(short zipNum)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	if (gLevelNum == LEVEL_NUM_APOCALYPSE)
		gNewObjectDefinition.type 		= APOCALYPSE_ObjType_ZipLinePully;
	else
		gNewObjectDefinition.type 		= FIREICE_ObjType_ZipLinePully;

	gNewObjectDefinition.coord 		= gZipLines[zipNum].splinePoints[(int)ZIP_PULLY_STARTOFF];				// get coord to start it
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= PLAYER_SLOT - 5;									// must move before player so that player will udpate correctly
	gNewObjectDefinition.moveCall 	= MoveZipPully_Waiting;
	gNewObjectDefinition.rot 		= CalcYAngleFromPointToPoint(0, gZipLines[zipNum].start.x, gZipLines[zipNum].start.z, gZipLines[zipNum].end.x, gZipLines[zipNum].end.z);
	gNewObjectDefinition.scale 		= 2.5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->ZipID = zipNum;
	newObj->SplinePlacement = ZIP_PULLY_STARTOFF;
	newObj->Speed3D = 0;

}


/********************** MOVE ZIP PULLY : WAITING ***************************/

static void MoveZipPully_Waiting(ObjNode *theNode)
{
ObjNode	*player = gPlayerInfo.objNode;

	if ((player->Skeleton->AnimNum != PLAYER_ANIM_JUMP) && (player->Skeleton->AnimNum != PLAYER_ANIM_FALL))		// must be jumping or falling to grab pully
		return;

	if (gPlayerInfo.coord.y > theNode->Coord.y)																	// only if player below zip pully
		return;

	if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, player->Coord.x, player->Coord.z) > 200.0f)		// and close enough
		return;

	MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_RIDEZIP, 10.0f);
	player->Rot.y = theNode->Rot.y;

	gCurrentZip = theNode;

	theNode->MoveCall = MoveZipPully_Moving;
	theNode->Speed3D = 50;

	gCameraUserRotY = PI/8.0f;											// angle a little for a better view
}


/********************** MOVE ZIP PULLY : MOVING ***************************/

static void MoveZipPully_Moving(ObjNode *theNode)
{
ObjNode	*player = gPlayerInfo.objNode;
short	zipNum = theNode->ZipID;
float	s,dy;
int		i;
float	fps = gFramesPerSecondFrac;

	if (player->Skeleton->IsMorphing)																			// dont do anything until player is ready
		return;

				/*****************************/
				/* ACCELERATE BASED ON SLOPE */
				/*****************************/

	i = theNode->SplinePlacement;													// get current index
	dy = gZipLines[zipNum].splinePoints[i+1].y - gZipLines[zipNum].splinePoints[i].y;	// calc dy between points for slope

	theNode->Speed3D += dy * -40.0f * fps;											// accelerate
	if (theNode->Speed3D > 3000.0f)													// see if reached max speed
		theNode->Speed3D = 3000.0f;

	s = theNode->SplinePlacement += theNode->Speed3D * fps;							// inc the spline index
	i = (int)s;

	if ((i >= gZipLines[zipNum].numPoints) || (i < 0))								// see if hit either end of the zip spline
	{
		MorphToSkeletonAnim(player->Skeleton, PLAYER_ANIM_FALL, 4.0f);
		theNode->MoveCall = MoveZipPully_Completed;
		gCurrentZip = nil;
		PlayEffect3D(EFFECT_PLAYERCLANG, &theNode->Coord);
	}
	else
	{
		theNode->Coord = gZipLines[zipNum].splinePoints[i];															// get new coord

		UpdateObjectTransforms(theNode);
	}
}


/********************** MOVE ZIP PULLY : COMPLETED ***************************/
//
// Just wait until player cannot see it, and then reset it at start of zipline
//

static void MoveZipPully_Completed(ObjNode *theNode)
{
	if (theNode->StatusBits & STATUS_BIT_ISCULLED)
	{
		theNode->SplinePlacement = ZIP_PULLY_STARTOFF;
		theNode->Coord = theNode->InitCoord;
		UpdateObjectTransforms(theNode);

		theNode->MoveCall = MoveZipPully_Waiting;
	}
}






