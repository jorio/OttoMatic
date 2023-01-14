/****************************/
/*   	VAPORTRAILS.C  		*/
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/
// Revised color streak rendering: (C)2021 Iliyas Jorio - https://github.com/jorio/ottomatic


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawVaporTrail_ColorStreak(int	i);
static void DrawVaporTrail_SmokeColumn(int	i);
static void CreateSmokeColumnRing(int trailNum, int segmentNum);
static void DeleteVaporTrailSegment(int t, int	seg, int num);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_VAPORTRAILS		100
#define	MAX_TRAIL_SEGMENTS	40								// max segments in skid

#define NUM_RING_POINTS		7


typedef struct
{
	Boolean				isActive;								// true if this one is active
	ObjNode				*owner;									// who owns this skid
	Byte				ownerTrailNum;							// index so owner can have multiple trails

	Byte				type;								// type of trail to create
	uint16_t				numSegments;						// # segments in trail
	OGLPoint3D			points[MAX_TRAIL_SEGMENTS];			// segment points
	OGLColorRGBA		color[MAX_TRAIL_SEGMENTS];			// each segment has a color+alpha value to fade
	Boolean				isLastPointPinned;					// if false, last point is currently tracking joint pos each frame

	float				scale[MAX_TRAIL_SEGMENTS];
	float				scaleDelta[MAX_TRAIL_SEGMENTS];

	float				size;
	float				decayRate;
	float				segmentDist;

		/* SMOKE COULD INFO */

	OGLPoint3D			ringShapes[MAX_TRAIL_SEGMENTS][NUM_RING_POINTS];
	float				uOff;

}VaporTrailType;


/*********************/
/*    VARIABLES      */
/*********************/

static VaporTrailType	gVaporTrails[MAX_VAPORTRAILS];

OGLPoint3D			gSmokeColumnPoints[MAX_TRAIL_SEGMENTS * NUM_RING_POINTS];
OGLColorRGBA		gSmokeColumnColors[MAX_TRAIL_SEGMENTS * NUM_RING_POINTS];
OGLTextureCoord		gSmokeColumnUVs[MAX_TRAIL_SEGMENTS * NUM_RING_POINTS];
MOTriangleIndecies	gSmokeColumnTriangles[MAX_TRAIL_SEGMENTS * (NUM_RING_POINTS-1) * 2];


MOVertexArrayData	gSmokeColumnMesh;

MetaObjectPtr		gColorStreakMaterial;

/*************************** INIT VAPOR TRAILS **********************************/

void InitVaporTrails(void)
{
int		i;

	for (i = 0; i < MAX_VAPORTRAILS; i++)
	{
		gVaporTrails[i].isActive = false;
	}

	gSmokeColumnMesh.numMaterials 	= -1;
	gSmokeColumnMesh.points 		= gSmokeColumnPoints;
	gSmokeColumnMesh.triangles 		= gSmokeColumnTriangles;
	gSmokeColumnMesh.normals		= nil;
	gSmokeColumnMesh.uvs[0]			= gSmokeColumnUVs;
	gSmokeColumnMesh.colorsByte		= nil;
	gSmokeColumnMesh.colorsFloat	= gSmokeColumnColors;



	MOMaterialData matData;
	memset(&matData, 0, sizeof(matData));
	matData.setupInfo		= gGameViewInfoPtr;
	matData.flags			= BG3D_MATERIALFLAG_ALWAYSBLEND;
	matData.diffuseColor	= (OGLColorRGBA) {1, 1, 1, 1};
	gColorStreakMaterial = MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);
}

/*************************** DISPOSE STREAK MATERIAL ******************************/


void DisposeVaporTrails(void)
{
	MO_DisposeObjectReference(gColorStreakMaterial);
	gColorStreakMaterial = nil;
}


/*************************** CREATE NEW VAPOR TRAIL ******************************/
//
// INPUT:	x/z = coord of skid
//			dx/dz = direction of motion (not necessarily normalized)
//			owner = objNode to assign to
//
// OUTPUT:  trail #, or -1 if none
//

int CreateNewVaporTrail(ObjNode *owner, Byte ownerTrailNum, Byte trailType,
						const OGLPoint3D *startPoint, const OGLColorRGBA *color,
						float size, float decayRate, float segmentDist)
{
int		i;

			/* FIND A FREE SLOT */

	for (i = 0; i < MAX_VAPORTRAILS; i++)
	{
		if (!gVaporTrails[i].isActive)
			goto got_it;

	}
	return(-1);

got_it:


		/**********************/
		/* CREATE 1ST SEGMENT */
		/**********************/

	gVaporTrails[i].isActive  		= true;
	gVaporTrails[i].owner			= owner;
	gVaporTrails[i].ownerTrailNum	= ownerTrailNum;
	gVaporTrails[i].type			= trailType;
	gVaporTrails[i].numSegments 	= 1;
	gVaporTrails[i].points[0] 		= *startPoint;
	gVaporTrails[i].color[0]		= *color;
	gVaporTrails[i].isLastPointPinned = true;

	gVaporTrails[i].size			= size;
	gVaporTrails[i].decayRate		= decayRate;
	gVaporTrails[i].segmentDist		= segmentDist;

	switch(trailType)
	{
		case	VAPORTRAIL_TYPE_COLORSTREAK:
				break;

		case	VAPORTRAIL_TYPE_SMOKECOLUMN:
				CreateSmokeColumnRing(i, 0);
				gVaporTrails[i].uOff = 0;
				break;

		default:
				DoFatalAlert("CreateNewVaporTrail: unsupported type");
	}

	return(i);
}


/********************** ADD TO VAPOR TRAIL **************************/
//
// Called every frame.  Adds a new segment to an existing vapor trail if distance is enough.
//

void AddToVaporTrail(short *trailNum, const OGLPoint3D *where, const OGLColorRGBA *color)
{
	if (*trailNum == -1)
		return;

	VaporTrailType* trail = &gVaporTrails[*trailNum];

			/* SEE IF THIS TRAIL HAS GONE AWAY */

	if (!trail->isActive)
	{
		*trailNum = -1;
		return;
	}

			/* SEE IF THIS TRAIL IS FULL */

	if (trail->numSegments >= MAX_TRAIL_SEGMENTS)
		return;

			/* GET DISTANCE TO LAST FIXED POINT */

	const OGLPoint3D* lastFixedPoint;
	if (trail->isLastPointPinned)
		lastFixedPoint = &trail->points[trail->numSegments - 1];
	else
		lastFixedPoint = &trail->points[trail->numSegments - 2];

	float dist = OGLPoint3D_Distance(where, lastFixedPoint);

	if (dist > 1000.0f		// see if too far (teleported, or something like that)
		|| dist < 0.1f)		// and avoid creating a new point that overlaps the previous one
	{
		return;
	}

			/* IF LAST POINT WAS FIXED, ADD NEW POINT TO LIST */

	if (trail->isLastPointPinned)
	{
		trail->numSegments++;

		if (trail->type == VAPORTRAIL_TYPE_SMOKECOLUMN)
			CreateSmokeColumnRing(*trailNum, trail->numSegments - 1);
	}

			/* MAKE LAST POINT TRACK CURRENT POS */

	trail->points[trail->numSegments - 1] = *where;
	trail->color[trail->numSegments - 1] = *color;

			/* PIN LAST POINT IF FAR ENOUGH FROM PREVIOUS */

	trail->isLastPointPinned = (dist >= trail->segmentDist);
}


/************************* VERIFY VAPOR TRAIL *****************************/
//
// Returns true if the specified vapor trail matches the objnode info.
//

Boolean VerifyVaporTrail(short	trailNum, ObjNode *owner, short ownerIndex)
{
	if ((trailNum < 0) || (trailNum >= MAX_VAPORTRAILS))
		return(false);

	if (!gVaporTrails[trailNum].isActive)
		return(false);

	if (gVaporTrails[trailNum].owner != owner)
		return(false);

	if (gVaporTrails[trailNum].ownerTrailNum != ownerIndex)
		return(false);

	return(true);
}


#pragma mark -



/****************************** MOVE VAPOR TRAILS *****************************/

void MoveVaporTrails(void)
{
int		i,n,s;
float	fadeRate, fps = gFramesPerSecondFrac;
Byte	type;

	for (i = 0; i < MAX_VAPORTRAILS; i++)
	{
		if (!gVaporTrails[i].isActive)
			continue;

		type = gVaporTrails[i].type;

		fadeRate = gVaporTrails[i].decayRate * fps;


				/* FADE EACH SEGMENT */

		n = gVaporTrails[i].numSegments-1;							// get # -1

		for (s = 0; s <= n; s++)
		{
			gVaporTrails[i].color[s].a -= fadeRate;					// fade the alpha
			if (gVaporTrails[i].color[s].a <= 0.0f)					// see if invisible
			{
				if (s == n)											// if this was the last segment in the trail, then it's all invisible so delete
				{
					gVaporTrails[i].isActive = false;
					break;
				}
				else												// if next segment is also invisible, then delete this segment
				if (gVaporTrails[i].color[s+1].a <= 0.0f)
				{
					DeleteVaporTrailSegment(i,s,n);
					n--;
				}
				else											// keep this invisible segment because next segment is still visible and we need the trailing fade
				{
					gVaporTrails[i].color[s].a = 0.0f;
				}
			}

					/*********************/
					/* DO TYPE SPECIFICS */
					/*********************/

			switch(type)
			{
				case	VAPORTRAIL_TYPE_SMOKECOLUMN:
						gVaporTrails[i].scale[s] += gVaporTrails[i].scaleDelta[s] * fps;
						break;
			}
		}
	}
}


/***************** DELETE VAPOR TRAIL SEGMENT *********************/

static void DeleteVaporTrailSegment(int t, int	seg, int num)
{
int		j,i;
Byte	trailType = gVaporTrails[t].type;

	for (j = seg+1; j <= num; j++)										// shift other data down
	{
		gVaporTrails[t].color[j-1] 	= gVaporTrails[t].color[j];
		gVaporTrails[t].points[j-1] = gVaporTrails[t].points[j];

		switch(trailType)
		{
			case	VAPORTRAIL_TYPE_SMOKECOLUMN:
					for (i = 0; i < NUM_RING_POINTS; i++)
						gVaporTrails[t].ringShapes[j-1][i] = gVaporTrails[t].ringShapes[j][i];


					gVaporTrails[t].scale[j-1]		= gVaporTrails[t].scale[j];
					gVaporTrails[t].scaleDelta[j-1] = gVaporTrails[t].scaleDelta[j];
					break;
		}
	}

	gVaporTrails[t].numSegments--;
}


/*************************** DRAW VAPOR TRAILS ******************************/

void DrawVaporTrails(void)
{
	OGL_PushState();

	OGL_DisableLighting();									// deactivate lighting
	glDisable(GL_NORMALIZE);								// disable vector normalizing since scale == 1
	glDisable(GL_CULL_FACE);								// deactivate culling
	glDepthMask(GL_FALSE);									// no z-writes
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);



			/*******************/
			/* DRAW EACH TRAIL */
			/*******************/

	for (int i = 0; i < MAX_VAPORTRAILS; i++)
	{
		if (!gVaporTrails[i].isActive)							// must be active
			continue;
		if (gVaporTrails[i].numSegments < 2)					// takes at least 2 segments to draw something
			continue;

		switch(gVaporTrails[i].type)
		{
			case	VAPORTRAIL_TYPE_COLORSTREAK:
					DrawVaporTrail_ColorStreak(i);
					break;

			case	VAPORTRAIL_TYPE_SMOKECOLUMN:
					DrawVaporTrail_SmokeColumn(i);
					break;

			default:
					DoFatalAlert("DrawVaporTrails: unsupported type");
		}


	}


			/* RESTORE STATE */

	OGL_PopState();
	glLineWidth(1);
}


/************************** DRAW VAPOR TRAIL:  COLOR STREAK **************************/
//
// Source port note: The original game used GL_LINE_STRIP for this effect.
// However, modern OpenGL drivers may severely limit line thickness.
// For instance, the following snippet returns a max thickness of only 10 px
// on a GeForce GTX 970:
//		GLfloat range[2];
//		glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, range);
// 10 screen-space pixels is way too low for the desired effect.
// So my alternate version draws a ribbon mesh instead.
//

static void DrawVaporTrail_ColorStreak(int	i)
{
#if 0
uint32_t	w,p,n;
float	size,dist;


 	n = gVaporTrails[i].numSegments;						// get # segments
	if (n > MAX_TRAIL_SEGMENTS)
		DoFatalAlert("DrawVaporTrail_ColorStreak: n > MAX_TRAIL_SEGMENTS");

			/* SET LINE SIZE */

	size = gVaporTrails[i].size;							// get size of trail
	dist = CalcQuickDistance(gVaporTrails[i].points[n-1].x, gVaporTrails[i].points[n-1].z,
								gGameViewInfoPtr->cameraPlacement.cameraLocation.x,gGameViewInfoPtr->cameraPlacement.cameraLocation.z);
	size *= 700.0f / dist;

	w = (float)(gGameWindowHeight / 15) * size;

	if (w > 60)
		w = 60;

	if (w < 1)
		w = 1;
	glLineWidth(w);				// set line width (remember, it's screen-relative, so scale to rez)

	glBegin(GL_LINE_STRIP);

	for (p = 0; p < n; p++)
	{
		SetColor4fv((GLfloat *)&gVaporTrails[i].color[p]);
		glVertex3fv((GLfloat *)&gVaporTrails[i].points[p]);
	}

	glEnd();
#else
	VaporTrailType*			trail			= &gVaporTrails[i];
	const OGLVector3D		up				= {0,1,0};
	const OGLPoint3D*		camCoords		= &gGameViewInfoPtr->cameraPlacement.cameraLocation;
	MOTriangleIndecies*		triPtr			= &gSmokeColumnTriangles[0];
	const float				halfThickness	= trail->size * 30 / 2.0f;
	OGLPoint3D				viewSpaceTrailPoint[MAX_TRAIL_SEGMENTS];
	OGLMatrix4x4			billboardMatrix;
	OGLVector2D				normal2D		= { 0,1 };


	GAME_ASSERT(gVaporTrails[i].numSegments <= MAX_TRAIL_SEGMENTS);

	gSmokeColumnMesh.numPoints 		= 2 * (trail->numSegments-1);
	gSmokeColumnMesh.numTriangles 	= 2 * (trail->numSegments-1);


			/* TRANSFORM ALL TRAIL POINTS TO VIEW-SPACE */

	OGLPoint3D_TransformArray(trail->points, &gWorldToViewMatrix, viewSpaceTrailPoint, trail->numSegments);

			/* PREPARE BILLBOARD MATRIX */
			//
			// We're using the last point in the ribbon as the 'from' point
			// instead of computing the matrix for every point in the ribbon.
			//

	SetLookAtMatrix(&billboardMatrix, &up, &trail->points[trail->numSegments-1], camCoords);

			/* PROCESS EVERY POINT IN THE TRAIL */

	for (int p = 0; p < trail->numSegments; p++)
	{
		int p2 = p * 2;

			/*********************************/
			/* MAKE 2 VERTICES IN THE RIBBON */
			/*********************************/

			/* CALC NORMAL OF SEGMENT IN VIEW-SPACE */

		if (p < trail->numSegments-1)							// just use previous normal if this is the last segment
		{
			OGLPoint3D cur	= viewSpaceTrailPoint[p];			// view-space projection of cur & next points
			OGLPoint3D next	= viewSpaceTrailPoint[p+1];

			OGLVector2D dir2D = { next.x-cur.x, next.y-cur.y };
			OGLVector2D_Normalize(&dir2D, &dir2D);

			normal2D = (OGLVector2D) { dir2D.y, dir2D.x };		// here's the segment's normal

			normal2D.x *= halfThickness;						// scale it with desired line thickness
			normal2D.y *= halfThickness;
		}

			/* PREP 2 VIEW-SPACE X-Y POINTS PERPENDICULAR TO SEGMENT */

		OGLPoint3D thickened[2] = { { normal2D.x, normal2D.y, 0 }, { -normal2D.x, -normal2D.y, 0 } };

			/* MAKE THEM FACE THE CAMERA */

		OGLPoint3D_TransformArray(thickened, &billboardMatrix, thickened, 2);

			/* AND MOVE THEM BACK TO WORLD SPACE (TO TRAIL POINT'S WORLD POSITION) */

		OGLPoint3D_Add(&thickened[0], &trail->points[p], &gSmokeColumnPoints[p2]);
		OGLPoint3D_Add(&thickened[1], &trail->points[p], &gSmokeColumnPoints[p2+1]);

			/*******************************************/
			/* NOW BUILD THE TRIANGLES FROM THE POINTS */
			/*******************************************/

				/* TRIANGLE A */

		triPtr->vertexIndices[0] = p2 + 0;		// hi pt
		triPtr->vertexIndices[1] = p2 + 1;		// lo pt
		triPtr->vertexIndices[2] = p2 + 3;		// lo pt in next seg
		triPtr++;								// point to next tri

				/* TRIANGLE B */

		triPtr->vertexIndices[0] = p2 + 2;		// hi pt in next seg
		triPtr->vertexIndices[1] = p2 + 0;		// hi pt
		triPtr->vertexIndices[2] = p2 + 3;		// lo pt in next seg
		triPtr++;								// point to next triangle

			/**************/
			/* SET COLORS */
			/**************/

		gSmokeColumnColors[p2 + 0] = trail->color[p];
		gSmokeColumnColors[p2 + 1] = trail->color[p];
	}


			/* SUBMIT VERTEX ARRAY */


	OGL_PushState();

	glEnable(GL_CULL_FACE);
	MO_DrawMaterial(gColorStreakMaterial);		// activate material
	MO_DrawGeometry_VertexArray(&gSmokeColumnMesh);

	OGL_PopState();
#endif
}


/************************** DRAW VAPOR TRAIL:  SMOKE COLUMN **************************/


static void DrawVaporTrail_SmokeColumn(int	i)
{
int				p,j,n,pointNum;
float			u,v,uOff;
OGLVector3D		dir;
OGLMatrix4x4	m,m2;
const OGLVector3D from = {0,1,0};
MOTriangleIndecies	*triPtr;
float			fps = gFramesPerSecondFrac;

	uOff = gVaporTrails[i].uOff += fps * .3f;

	n = gVaporTrails[i].numSegments-1;

	gSmokeColumnMesh.numPoints 		= gVaporTrails[i].numSegments * NUM_RING_POINTS;
	gSmokeColumnMesh.numTriangles 	= n * (NUM_RING_POINTS-1) * 2;

				/**************************************************/
				/* FIRST BUILD ALL OF THE POINTS FOR THE GEOMETRY */
				/**************************************************/
				//
				// Remember that the last and 1st points overlap so that we can
				// do proper uv wrapping all the way around.
				//

	for (p = 0; p <= n; p++)
	{

				/* CALC VECTOR TO NEXT SEGMENT */

		if (p < n)																			// just use previous normal if this is the last segment
		{
			dir.x = gVaporTrails[i].points[p+1].x - gVaporTrails[i].points[p].x;
			dir.y = gVaporTrails[i].points[p+1].y - gVaporTrails[i].points[p].y;
			dir.z = gVaporTrails[i].points[p+1].z - gVaporTrails[i].points[p].z;
		}


				/* ROTATE RING INTO POSITION */

		OGLMatrix4x4_SetScale(&m2, gVaporTrails[i].scale[p], gVaporTrails[i].scale[p], gVaporTrails[i].scale[p]);
		OGLCreateFromToRotationMatrix(&m, &from, &dir);										// generate a rotation matrix
		OGLMatrix4x4_Multiply(&m2, &m, &m);

		m.value[M03] = gVaporTrails[i].points[p].x;											// insert translate
		m.value[M13] = gVaporTrails[i].points[p].y;
		m.value[M23] = gVaporTrails[i].points[p].z;
		OGLPoint3D_TransformArray(&gVaporTrails[i].ringShapes[p][0], &m,
								 &gSmokeColumnPoints[p * NUM_RING_POINTS],  NUM_RING_POINTS);	// transform the points into master array

	}


			/*******************************************/
			/* NOW BUILD THE TRIANGLES FROM THE POINTS */
			/*******************************************/

	triPtr = &gSmokeColumnTriangles[0];										// point to triangle list

	for (p = 0; p < n; p++)													// for each segment
	{
		pointNum = p * NUM_RING_POINTS;										// calc index into point list to start this ring

		for (j = 0; j < (NUM_RING_POINTS-1); j++)							// build 2 triangles for each point in the ring
		{
					/* TRIANGLE A */

			triPtr->vertexIndices[2] = pointNum + j;
			triPtr->vertexIndices[1] = pointNum + j + 1;				// next pt over
			triPtr->vertexIndices[0] = pointNum + j + NUM_RING_POINTS;		// pt in next segment
			triPtr++;														// point to next triangle

					/* TRIANGLE B */

			triPtr->vertexIndices[2] = pointNum + j + 1;					// next pt over
			triPtr->vertexIndices[1] = pointNum + j + 1 + NUM_RING_POINTS;		// pt in next segment

			triPtr->vertexIndices[0] = pointNum + j + NUM_RING_POINTS;
			triPtr++;														// point to next triangle
		}
	}

			/*************************/
			/* SET RING COLORS & UVS */
			/*************************/


	for (p = 0; p <= n; p++)
	{
		OGLColorRGBA	*color 	= &gSmokeColumnColors[p * NUM_RING_POINTS];
		OGLTextureCoord	*uvs 	= &gSmokeColumnUVs[p * NUM_RING_POINTS];

		v = gVaporTrails[i].points[p].y * .0003f;		// v mapping is based on y coord

		u = uOff;
		for (j = 0; j < NUM_RING_POINTS; j++)
		{
			color[j] = gVaporTrails[i].color[p];
			if (p == n)									// always fade the endpoint
			{
				color[j].a = 0.0f;
			}

			uvs[j].u = u;
			uvs[j].v = v;

			u += 1.0f / (float)(NUM_RING_POINTS-1);
		}
	}


			/* SUBMIT VERTEX ARRAY */


	OGL_PushState();

	MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_PARTICLES][PARTICLE_SObjType_GreySmoke].materialObject);		// activate material
	MO_DrawGeometry_VertexArray(&gSmokeColumnMesh);

	OGL_PopState();
}


/****************** CREATE SMOKE COLUMN RING *******************/

static void CreateSmokeColumnRing(int trailNum, int segmentNum)
{
float	r;
int		j;
float	size = gVaporTrails[trailNum].size;							// get size of trail
float	size2 = size * .2f;

	gVaporTrails[trailNum].scale[segmentNum]		= 1.0f + RandomFloat();
	gVaporTrails[trailNum].scaleDelta[segmentNum] 	= RandomFloat() * .9f;

	r = 0;
	for ( j = 0; j < NUM_RING_POINTS; j++)
	{
		gVaporTrails[trailNum].ringShapes[segmentNum][j].x = (sin(r) * size) + (RandomFloat2() * size2);
		gVaporTrails[trailNum].ringShapes[segmentNum][j].z = (cos(r) * size) + (RandomFloat2() * size2);
		gVaporTrails[trailNum].ringShapes[segmentNum][j].y = RandomFloat2() * size2;

		r += (PI2/(NUM_RING_POINTS-1));
	}
}


