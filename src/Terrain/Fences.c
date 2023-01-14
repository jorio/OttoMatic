/**********************/
/*   	FENCES.C      */
/**********************/


#include "game.h"


/***************/
/* EXTERNALS   */
/***************/

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawFences(ObjNode *theNode);
static void SubmitFence(int f, float camX, float camZ);
static void MakeFenceGeometry(void);
static void DrawFenceNormals(short f);


/****************************/
/*    CONSTANTS             */
/****************************/

#define MAX_FENCES			90
#define	MAX_NUBS_IN_FENCE	80


#define	FENCE_SINK_FACTOR	70.0f


/**********************/
/*     VARIABLES      */
/**********************/

int				gNumFences = 0;
int				gNumFencesDrawn;
FenceDefType	*gFenceList = nil;


float			gFenceHeight[] =
{
	430,					// wood fence
	1100,					// corn stalk
	440,					// chickenwire
	440,					// metal fence
	1100,					// PINK crystal
	550,					// mech
	1100,					// slime tree
	1100,					// BLUE crystal
	550,					// mech 2
	1500,					// jungle wooden
	1300,					// jungle fern
	900,					// lamp
	1000,					// rubble
	900,					// CRUNCH

	500,					// FENCE_TYPE_FUN
	1100,					// FENCE_TYPE_HEDGE
	800,					// FENCE_TYPE_LINE
	600,					// FENCE_TYPE_TENT

	800,					// FENCE_TYPE_LAVA
	800,					// FENCE_TYPE_ICE

	150,					// FENCE_TYPE_SAUCER

	600,					// FENCE_TYPE_NEURON
};


static MOVertexArrayData		gFenceTriMeshData[MAX_FENCES];
static MOTriangleIndecies		gFenceTriangles[MAX_FENCES][MAX_NUBS_IN_FENCE*2];
static OGLPoint3D				gFencePoints[MAX_FENCES][MAX_NUBS_IN_FENCE*2];
static OGLTextureCoord			gFenceUVs[MAX_FENCES][MAX_NUBS_IN_FENCE*2];
static OGLColorRGBA_Byte			gFenceColors[MAX_FENCES][MAX_NUBS_IN_FENCE*2];

static short	gSeaweedFrame;
static float	gSeaweedFrameTimer;


/********************** DISPOSE FENCES *********************/

void DisposeFences(void)
{
int		i;

	if (!gFenceList)
		return;

	for (i = 0; i < gNumFences; i++)
	{
		if (gFenceList[i].sectionVectors)
			SafeDisposePtr((Ptr)gFenceList[i].sectionVectors);			// nuke section vectors
		gFenceList[i].sectionVectors = nil;

		if (gFenceList[i].sectionNormals)
			SafeDisposePtr((Ptr)gFenceList[i].sectionNormals);			// nuke normal vectors
		gFenceList[i].sectionNormals = nil;

		if (gFenceList[i].nubList)
			SafeDisposePtr((Ptr)gFenceList[i].nubList);
		gFenceList[i].nubList = nil;
	}

	SafeDisposePtr((Ptr)gFenceList);
	gFenceList = nil;
	gNumFences = 0;
}



/********************* PRIME FENCES ***********************/
//
// Called during terrain prime function to initialize
//

void PrimeFences(void)
{
long					f,i,numNubs;
FenceDefType			*fence;
OGLPoint3D				*nubs;
ObjNode					*obj;

	gSeaweedFrame = 0;
	gSeaweedFrameTimer = 0;


	if (gNumFences > MAX_FENCES)
		DoFatalAlert("PrimeFences: gNumFences > MAX_FENCES");

		/* SET TEXTURE WARPPING MODE ON ALL FENCE SPRITE TEXTURES */

	for (i = 0; i < gNumSpritesInGroupList[SPRITE_GROUP_FENCES]; i++)
	{
		MOMaterialObject	*mat;

		mat = gSpriteGroupList[SPRITE_GROUP_FENCES][i].materialObject;
		mat->objectData.flags |= BG3D_MATERIALFLAG_CLAMP_V;				// don't wrap v, only u
	}


			/******************************/
			/* ADJUST TO GAME COORDINATES */
			/******************************/

	for (f = 0; f < gNumFences; f++)
	{
		fence 				= &gFenceList[f];					// point to this fence
		nubs 				= fence->nubList;					// point to nub list
		numNubs 			= fence->numNubs;					// get # nubs in fence

		if (numNubs == 1)
			DoFatalAlert("PrimeFences: numNubs == 1");

		if (numNubs > MAX_NUBS_IN_FENCE)
			DoFatalAlert("PrimeFences: numNubs > MAX_NUBS_IN_FENCE");

		for (i = 0; i < numNubs; i++)							// adjust nubs
		{
			nubs[i].x *= MAP2UNIT_VALUE;
			nubs[i].z *= MAP2UNIT_VALUE;
			nubs[i].y = GetTerrainY2(nubs[i].x,nubs[i].z);		// calc Y
		}

		/* CALCULATE VECTOR FOR EACH SECTION */

		fence->sectionVectors = (OGLVector2D *)AllocPtr(sizeof(OGLVector2D) * (numNubs-1));		// alloc array to hold vectors
		if (fence->sectionVectors == nil)
			DoFatalAlert("PrimeFences: AllocPtr failed!");

		for (i = 0; i < (numNubs-1); i++)
		{
			fence->sectionVectors[i].x = nubs[i+1].x - nubs[i].x;
			fence->sectionVectors[i].y = nubs[i+1].z - nubs[i].z;

			OGLVector2D_Normalize(&fence->sectionVectors[i], &fence->sectionVectors[i]);
		}


		/* CALCULATE NORMALS FOR EACH SECTION */

		fence->sectionNormals = (OGLVector2D *)AllocPtr(sizeof(OGLVector2D) * (numNubs-1));		// alloc array to hold vectors
		if (fence->sectionNormals == nil)
			DoFatalAlert("PrimeFences: AllocPtr failed!");

		for (i = 0; i < (numNubs-1); i++)
		{
			OGLVector3D	v;

			v.x = fence->sectionVectors[i].x;					// get section vector (as calculated above)
			v.z = fence->sectionVectors[i].y;

			fence->sectionNormals[i].x = -v.z;					//  reduced cross product to get perpendicular normal
			fence->sectionNormals[i].y = v.x;
			OGLVector2D_Normalize(&fence->sectionNormals[i], &fence->sectionNormals[i]);
		}

	}

			/***********************/
			/* MAKE FENCE GEOMETRY */
			/***********************/

	MakeFenceGeometry();

		/*************************************************************************/
		/* CREATE DUMMY CUSTOM OBJECT TO CAUSE FENCE DRAWING AT THE DESIRED TIME */
		/*************************************************************************/
		//
		// The fences need to be drawn after the Cyc object, but before any sprite or font objects.
		//

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.slot 		= FENCE_SLOT;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOLIGHTING;

	obj = MakeNewObject(&gNewObjectDefinition);
	obj->CustomDrawFunction = DrawFences;
}


/*************** MAKE FENCE GEOMETRY *********************/

static void MakeFenceGeometry(void)
{
uint16_t				type;
float					u,height,aspectRatio,textureUOff;
int						numNubs;
FenceDefType			*fence;
OGLPoint3D				*nubs;
float					minX,minY,minZ,maxX,maxY,maxZ;

	for (int f = 0; f < gNumFences; f++)
	{
				/******************/
				/* GET FENCE INFO */
				/******************/

		fence = &gFenceList[f];								// point to this fence
		nubs = fence->nubList;								// point to nub list
		numNubs = fence->numNubs;							// get # nubs in fence
		type = fence->type;									// get fence type
		if (type > gNumSpritesInGroupList[SPRITE_GROUP_FENCES])
			DoFatalAlert("MakeFenceGeometry: illegal fence type");
		height = gFenceHeight[type];						// get fence height

		aspectRatio = gSpriteGroupList[SPRITE_GROUP_FENCES][type].aspectRatio;	// get aspect ratio

		textureUOff = 1.0f / height * aspectRatio;			// calc UV offset


					/***************************/
					/* SET VERTEX ARRAY HEADER */
					/***************************/

		gFenceTriMeshData[f].numMaterials		 		= 1;
		gFenceTriMeshData[f].materials[0]				= nil;
		gFenceTriMeshData[f].points 					= &gFencePoints[f][0];
		gFenceTriMeshData[f].triangles					= &gFenceTriangles[f][0];
		gFenceTriMeshData[f].uvs[0]						= &gFenceUVs[f][0];
		gFenceTriMeshData[f].normals					= nil;
		gFenceTriMeshData[f].colorsByte					= &gFenceColors[f][0];
		gFenceTriMeshData[f].colorsFloat				= nil;
		gFenceTriMeshData[f].numPoints					= numNubs * 2;					// 2 vertices per nub
		gFenceTriMeshData[f].numTriangles				= (numNubs-1) * 2;			// 2 faces per nub (minus 1st)


				/* BUILD TRIANGLE INFO */

		for (int i = 0, j = 0; i < MAX_NUBS_IN_FENCE; i++, j+=2)
		{
			gFenceTriangles[f][j].vertexIndices[0] = 1 + j;
			gFenceTriangles[f][j].vertexIndices[1] = 0 + j;
			gFenceTriangles[f][j].vertexIndices[2] = 3 + j;

			gFenceTriangles[f][j+1].vertexIndices[0] = 3 + j;
			gFenceTriangles[f][j+1].vertexIndices[1] = 0 + j;
			gFenceTriangles[f][j+1].vertexIndices[2] = 2 + j;
		}

				/* INIT VERTEX COLORS */

		for (int i = 0; i < (MAX_NUBS_IN_FENCE*2); i++)
			gFenceColors[f][i].r = gFenceColors[f][i].g = gFenceColors[f][i].b = 0xff;


				/* SET TEXTURE */

		gFenceTriMeshData[f].materials[0] = gSpriteGroupList[SPRITE_GROUP_FENCES][type].materialObject;	// set illegal temporary ref to material


				/**********************/
				/* BUILD POINTS, UV's */
				/**********************/

		maxX = maxY = maxZ = -1000000;									// build new bboxes while we do this
		minX = minY = minZ = -maxX;

		u = 0;
		for (int i = 0, j = 0; i < numNubs; i++, j+=2)
		{
			float		x,y,z,y2;

					/* GET COORDS */

			x = nubs[i].x;
			z = nubs[i].z;

			y = nubs[i].y;
			if ((gLevelNum != LEVEL_NUM_CLOUD)	&& (gLevelNum != LEVEL_NUM_SAUCER))		// dont sink in cloud or saucer - make flussh
				y -= FENCE_SINK_FACTOR;									// sink into ground a little bit
			y2 = y + height;

					/* CHECK BBOX */

			if (x < minX)	minX = x;									// find min/max bounds for bbox
			if (x > maxX)	maxX = x;
			if (z < minZ)	minZ = z;
			if (z > maxZ)	maxZ = z;
			if (y < minY)	minY = y;
			if (y2 > maxY)	maxY = y2;


				/* SET COORDS */

			gFencePoints[f][j].x = x;
			gFencePoints[f][j].y = y;
			gFencePoints[f][j].z = z;

			gFencePoints[f][j+1].x = x;
			gFencePoints[f][j+1].y = y2;
			gFencePoints[f][j+1].z = z;


				/* CALC UV COORDS */

			if (i > 0)
			{
				u += CalcDistance3D(gFencePoints[f][j].x, gFencePoints[f][j].y, gFencePoints[f][j].z,
									gFencePoints[f][j-2].x, gFencePoints[f][j-2].y, gFencePoints[f][j-2].z) * textureUOff;
			}

			gFenceUVs[f][j].v 		= 1;									// bottom
			gFenceUVs[f][j+1].v 	= 0;									// top
			gFenceUVs[f][j].u 		= gFenceUVs[f][j+1].u = u;
		}

				/* SET CALCULATED BBOX */

		fence->bBox.min.x = minX;
		fence->bBox.max.x = maxX;
		fence->bBox.min.y = minY;
		fence->bBox.max.y = maxY;
		fence->bBox.min.z = minZ;
		fence->bBox.max.z = maxZ;
		fence->bBox.isEmpty = false;
	}
}


#pragma mark -

/********************* DRAW FENCES ***********************/

static void DrawFences(ObjNode *theNode)
{
float			cameraX, cameraZ;

#pragma unused (theNode)

			/* UPDATE SEAWEED ANIMATION */

	gSeaweedFrameTimer += gFramesPerSecondFrac;
	if (gSeaweedFrameTimer > .09f)
	{
		gSeaweedFrameTimer -= .09f;

		if (++gSeaweedFrame > 5)
			gSeaweedFrame = 0;
	}


			/* GET CAMERA COORDS */

	cameraX = gGameViewInfoPtr->cameraPlacement.cameraLocation.x;
	cameraZ = gGameViewInfoPtr->cameraPlacement.cameraLocation.z;


			/* SET GLOBAL MATERIAL FLAGS */

	gGlobalMaterialFlags = BG3D_MATERIALFLAG_CLAMP_V|BG3D_MATERIALFLAG_ALWAYSBLEND;


			/*******************/
			/* DRAW EACH FENCE */
			/*******************/

	gNumFencesDrawn = 0;

	for (int f = 0; f < gNumFences; f++)
	{
					/* DO BBOX CULLING */

		if (OGL_IsBBoxVisible(&gFenceList[f].bBox, nil))
		{
				/* SUBMIT GEOMETRY */

			SubmitFence(f, cameraX, cameraZ);
			gNumFencesDrawn++;

//			if (gDebugMode == 2)
//			{
//				DrawFenceNormals(f);
//			}
		}
	}

	gGlobalMaterialFlags = 0;
}


/****************** DRAW FENCE NORMALS ***************************/

static void DrawFenceNormals(short f)
{
int				i,numNubs;
OGLPoint3D		*nubs;
OGLVector2D		*normals;
float			x,y,z,nx,nz;

	OGL_PushState();
	glDisable(GL_TEXTURE_2D);
	SetColor4f(1,0,0,1);
	glLineWidth(3);

	numNubs  	= gFenceList[f].numNubs - 1;					// get # nubs in fence minus 1
	nubs  		= gFenceList[f].nubList;						// get ptr to nub list
	normals 	= gFenceList[f].sectionNormals;					// get ptr to normals

	for (i = 0; i < numNubs; i++)
	{
		glBegin(GL_LINES);

		x = nubs[i].x;
		y = nubs[i].y + 200.0f;			// show normal up a ways
		z = nubs[i].z;

		nx = normals[i].x * 150.0f;
		nz = normals[i].y * 150.0f;

		glVertex3f(x-nx,y,z-nz);
		glVertex3f(x + nx,y, z + nz);

		glEnd();
	}
	OGL_PopState();
	glLineWidth(1);

}


/******************** SUBMIT FENCE **************************/
//
// Visibility checks have already been done, so there's a good chance the fence is visible
//

static void SubmitFence(int f, float camX, float camZ)
{
float					dist,alpha;
long					i,numNubs,j;
FenceDefType			*fence;
OGLPoint3D				*nubs;

			/* GET FENCE INFO */

	fence = &gFenceList[f];								// point to this fence
	nubs = fence->nubList;								// point to nub list
	numNubs = fence->numNubs;							// get # nubs in fence


				/* CALC & SET TRANSPARENCY */

	for (i = j = 0; i < numNubs; i++, j+=2)
	{
		float		x,z;

		x = nubs[i].x;
		z = nubs[i].z;

				/* CALC & SET TRANSPARENCY */

		if (gAutoFadeStatusBits)									// see if this level has xparency
		{
			dist = CalcQuickDistance(camX, camZ, x, z);				// see if in fade zone
			if (dist < gAutoFadeStartDist)
				alpha = 1.0;
			else
			if (dist >= gAutoFadeEndDist)
				alpha = 0.0;
			else
			{
				dist -= gAutoFadeStartDist;							// calc xparency %
				dist *= gAutoFadeRange_Frac;
				if (dist < 0.0f)
					alpha = 0;
				else
					alpha = 1.0f - dist;
			}
		}
		else
			alpha = 1.0f;

		gFenceColors[f][j].a =
		gFenceColors[f][j+1].a = 255.0f * alpha;
	}



		/*******************/
		/* SUBMIT GEOMETRY */
		/*******************/

	MO_DrawGeometry_VertexArray(&gFenceTriMeshData[f]);
}




#pragma mark -

/******************** DO FENCE COLLISION **************************/
//
// returns True if hit a fence
//

Boolean DoFenceCollision(ObjNode *theNode)
{
double			fromX,fromZ,toX,toZ;
long			f,numFenceSegments,i,numReScans;
double			segFromX,segFromZ,segToX,segToZ;
OGLPoint3D		*nubs;
Boolean			intersected;
float			intersectX,intersectZ;
OGLVector2D		lineNormal;
double			radius;
double			oldX,oldZ,newX,newZ;
Boolean			hit = false;

			/* CALC MY MOTION LINE SEGMENT */

	oldX = theNode->OldCoord.x;						// from old coord
	oldZ = theNode->OldCoord.z;
	newX = gCoord.x;								// to new coord
	newZ = gCoord.z;
	radius = theNode->BoundingSphereRadius;


			/****************************************/
			/* SCAN THRU ALL FENCES FOR A COLLISION */
			/****************************************/

	for (f = 0; f < gNumFences; f++)
	{
		float	temp;
		float	r2 = radius + 20.0f;								// tweak a little to be safe

		if ((oldX == newX) && (oldZ == newZ))						// if no movement, then don't check anything
			break;


		/* QUICK CHECK TO SEE IF OLD & NEW COORDS (PLUS RADIUS) ARE OUTSIDE OF FENCE'S BBOX */

		temp = gFenceList[f].bBox.min.x - r2;
		if ((oldX < temp) && (newX < temp))
			continue;
		temp = gFenceList[f].bBox.max.x + r2;
		if ((oldX > temp) && (newX > temp))
			continue;

		temp = gFenceList[f].bBox.min.z - r2;
		if ((oldZ < temp) && (newZ < temp))
			continue;
		temp = gFenceList[f].bBox.max.z + r2;
		if ((oldZ > temp) && (newZ > temp))
			continue;

		nubs = gFenceList[f].nubList;				// point to nub list
		numFenceSegments = gFenceList[f].numNubs-1;	// get # line segments in fence



				/**********************************/
				/* SCAN EACH SECTION OF THE FENCE */
				/**********************************/

		numReScans = 0;
		for (i = 0; i < numFenceSegments; i++)
		{
			float	cross;

					/* GET LINE SEG ENDPOINTS */

			segFromX = nubs[i].x;
			segFromZ = nubs[i].z;
			segToX = nubs[i+1].x;
			segToZ = nubs[i+1].z;



					/* CALC NORMAL TO THE LINE */
					//
					// We need to find the point on the bounding sphere which is closest to the line
					// in order to do good collision checks
					//

#if 1
			lineNormal.x = oldX - segFromX;						// calc normalized vector from ref pt. to section endpoint 0
			lineNormal.y = oldZ - segFromZ;
			OGLVector2D_Normalize(&lineNormal, &lineNormal);
			cross = OGLVector2D_Cross(&gFenceList[f].sectionVectors[i], &lineNormal);	// calc cross product to determine which side we're on

//			if (cross == 0.0f)		// see if exactly on the line
//				continue;

			if (cross < 0.0f)
			{
				lineNormal.x = -gFenceList[f].sectionNormals[i].x;		// on the other side, so flip vector
				lineNormal.y = -gFenceList[f].sectionNormals[i].y;
			}
			else
			{
				lineNormal = gFenceList[f].sectionNormals[i];			// use pre-calculated vector
			}

#else
			if (!CalcRayNormal2D(&gFenceList[f].sectionVectors[i], segFromX, segFromZ, oldX, oldZ, &lineNormal))
				continue;										// if oldX,oldZ lines on the line segment then we can skip this cuz it's
#endif																// from a fence seg elsewhere that coincidentally is in line with this  pt.

					/* CALC FROM-TO POINTS OF MOTION */

			fromX = oldX; // - (lineNormal.x * radius);
			fromZ = oldZ; // - (lineNormal.y * radius);
			toX = newX - (lineNormal.x * radius);
			toZ = newZ - (lineNormal.y * radius);

					/* SEE IF THE LINES INTERSECT */

			intersected = IntersectLineSegments(fromX,  fromZ, toX, toZ,
						                     segFromX, segFromZ, segToX, segToZ,
				                             &intersectX, &intersectZ);

			if (intersected)
			{
				hit = true;

						/***************************/
						/* HANDLE THE INTERSECTION */
						/***************************/
						//
						// Move so edge of sphere would be tangent, but also a bit
						// farther so it isnt tangent.
						//

				gCoord.x = intersectX + (lineNormal.x * radius) + (lineNormal.x * 8.0f);
				gCoord.z = intersectZ + (lineNormal.y * radius) + (lineNormal.y * 8.0f);


						/* BOUNCE OFF WALL */

#if 1
				{
					OGLVector2D deltaV;

					deltaV.x = gDelta.x;
					deltaV.y = gDelta.z;
					ReflectVector2D(&deltaV, &lineNormal, &deltaV);
					gDelta.x = deltaV.x * .6f;
					gDelta.z = deltaV.y * .6f;
				}
#endif

						/* UPDATE COORD & SCAN AGAIN */

				newX = gCoord.x;
				newZ = gCoord.z;
				if (++numReScans < 4)
					i = -1;							// reset segment index to scan all again (will ++ to 0 on next loop)
				else
				{
					gCoord.x = oldX;				// woah!  there were a lot of hits, so let's just reset the coords to be safe!
					gCoord.z = oldZ;
					break;
				}
			}

			/**********************************************/
			/* NO INTERSECT, DO SAFETY CHECK FOR /\ CASES */
			/**********************************************/
			//
			// The above check may fail when the sphere is going thru
			// the tip of a tee pee /\ intersection, so this is a hack
			// to get around it.
			//

			else
			{
					/* SEE IF EITHER ENDPOINT IS IN SPHERE */

				if ((CalcQuickDistance(segFromX, segFromZ, newX, newZ) <= radius) ||
					(CalcQuickDistance(segToX, segToZ, newX, newZ) <= radius))
				{
					OGLVector2D deltaV;

					hit = true;

					gCoord.x = oldX;
					gCoord.z = oldZ;

						/* BOUNCE OFF WALL */

					deltaV.x = gDelta.x;
					deltaV.y = gDelta.z;
					ReflectVector2D(&deltaV, &lineNormal, &deltaV);
					gDelta.x = deltaV.x * .5f;
					gDelta.z = deltaV.y * .5f;
					return(hit);
				}
				else
					continue;
			}
		} // for i
	}

	return(hit);
}

/******************** SEE IF LINE SEGMENT HITS FENCE **************************/
//
// returns True if hit a fence
//

Boolean SeeIfLineSegmentHitsFence(const OGLPoint3D *endPoint1, const OGLPoint3D *endPoint2, OGLPoint3D *intersect, Boolean *overTop, float *fenceTopY)
{
float			fromX,fromZ,toX,toZ;
long			f,numFenceSegments,i;
float			segFromX,segFromZ,segToX,segToZ;
OGLPoint3D		*nubs;
Boolean			intersected;


	fromX = endPoint1->x;
	fromZ = endPoint1->z;
	toX = endPoint2->x;
	toZ = endPoint2->z;

			/****************************************/
			/* SCAN THRU ALL FENCES FOR A COLLISION */
			/****************************************/

	for (f = 0; f < gNumFences; f++)
	{
		/* QUICK CHECK TO SEE IF OLD & NEW COORDS (PLUS RADIUS) ARE OUTSIDE OF FENCE'S BBOX */

		if ((fromX < gFenceList[f].bBox.min.x) && (toX < gFenceList[f].bBox.min.x))
			continue;
		if ((fromX > gFenceList[f].bBox.max.x) && (toX > gFenceList[f].bBox.max.x))
			continue;

		if ((fromZ < gFenceList[f].bBox.min.z) && (toZ < gFenceList[f].bBox.min.z))
			continue;
		if ((fromZ > gFenceList[f].bBox.max.z) && (toZ > gFenceList[f].bBox.max.z))
			continue;

		nubs = gFenceList[f].nubList;				// point to nub list
		numFenceSegments = gFenceList[f].numNubs-1;	// get # line segments in fence


				/**********************************/
				/* SCAN EACH SECTION OF THE FENCE */
				/**********************************/

		for (i = 0; i < numFenceSegments; i++)
		{
			float	ix,iz;

					/* GET LINE SEG ENDPOINTS */

			segFromX = nubs[i].x;
			segFromZ = nubs[i].z;
			segToX = nubs[i+1].x;
			segToZ = nubs[i+1].z;


					/* SEE IF THE LINES INTERSECT */

			intersected = IntersectLineSegments(fromX,  fromZ, toX, toZ,
						                     segFromX, segFromZ, segToX, segToZ,
				                             &ix, &iz);

			if (intersected)
			{
				float	fenceTop,dy,d1,d2,ratio,iy;


						/* SEE IF INTERSECT OCCURS OVER THE TOP OF THE FENCE */

				if (overTop || intersect || fenceTopY)
				{
					fenceTop = GetTerrainY_Undeformed(ix, iz) + gFenceHeight[gFenceList[f].type];		// calc y coord @ top of fence here

					dy = endPoint2->y - endPoint1->y;					// get dy of line segment

					d1 = CalcDistance(fromX, fromZ, toX, toZ);
					d2 = CalcDistance(fromX, fromZ, ix, iz);

					ratio = d2/d1;

					iy = endPoint1->y + (dy * ratio);					// calc intersect y coord

					if (overTop)
					{
						if (iy >= fenceTop)
							*overTop = true;
						else
							*overTop = false;
					}

					if (intersect)
					{
						intersect->x = ix;						// pass back intersect coords
						intersect->y = iy;
						intersect->z = iz;
					}

					if (fenceTopY)
						*fenceTopY = fenceTop;
				}

				return(true);
			}

		}
	}

	return(false);
}




