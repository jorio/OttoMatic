/****************************/
/*   	VAPORTRAILS.C  		*/
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


#include "3dmath.h"
//#include <AGL/aglmacro.h> // srcport rm

extern	float			gFramesPerSecondFrac,gFramesPerSecond,gPlayerToCameraAngle;
extern	OGLPoint3D		gCoord;
extern	OGLVector3D		gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	OGLVector3D		gRecentTerrainNormal;
extern	PlayerInfoType	gPlayerInfo;
extern	u_long			gAutoFadeStatusBits;
extern	PrefsType			gGamePrefs;
extern	SpriteType	*gSpriteGroupList[];
extern	Boolean				gOSX;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawVaporTrail_ColorStreak(int	i, OGLSetupOutputType *setupInfo);
static void DrawVaporTrail_SmokeColumn(int	i, OGLSetupOutputType *setupInfo);
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
	u_short				numSegments;						// # segments in trail
	OGLPoint3D			points[MAX_TRAIL_SEGMENTS];			// segment points
	OGLColorRGBA		color[MAX_TRAIL_SEGMENTS];			// each segment has a color+alpha value to fade

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
}


/*************************** CREATE NEW VAPOR TRAIL ******************************/
//
// INPUT:	x/z = coord of skid
//			dx/dz = direction of motion (not necessarily normalized)
//			owner = objNode to assign to
//
// OUTPUT:  trail #, or -1 if none
//

int CreatetNewVaporTrail(ObjNode *owner, Byte ownerTrailNum, Byte trailType,
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
				DoFatalAlert("\pCreatetNewVaporTrail: unsupported type");
	}

	return(i);
}


/********************** ADD TO VAPOR TRAIL **************************/
//
// Called every frame.  Adds a new segment to an existing vapor trail if distance is enough.
//

void AddToVaporTrail(short *trailNum, const OGLPoint3D *where, const OGLColorRGBA *color)
{
int		numSegments;
short 	t = *trailNum;
float	dist;

	if (t == -1)
		return;

	if (!gVaporTrails[t].isActive)				// see if this trail has gone away
	{
		*trailNum = -1;
		return;
	}


			/* SEE IF THIS TRAIL IS FULL */

	numSegments = gVaporTrails[t].numSegments;						// get # segments in trail
	if (numSegments >= MAX_TRAIL_SEGMENTS)
		return;


			/* SEE IF FAR ENOUGH FROM PREVIOUS TO ADD TO LIST */

	dist = OGLPoint3D_Distance(where, &gVaporTrails[t].points[numSegments-1]);

				/* ADD NEW POINT TO LIST */

	if (dist > gVaporTrails[t].segmentDist)
	{
		if (dist > 1000.0f)							// see if too far (teleported, or something like that)
			return;

		gVaporTrails[t].points[numSegments] = *where;
		gVaporTrails[t].color[numSegments] = *color;


				/*********************/
				/* DO TYPE SPECIFICS */
				/*********************/

		switch(gVaporTrails[t].type)
		{
			case	VAPORTRAIL_TYPE_COLORSTREAK:
					break;

			case	VAPORTRAIL_TYPE_SMOKECOLUMN:
					CreateSmokeColumnRing(t, numSegments);
					break;

			default:
					DoFatalAlert("\pAddToVaporTrail: unsupported type");
		}


		gVaporTrails[t].numSegments++;
	}




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

void DrawVaporTrails(OGLSetupOutputType *setupInfo)
{
int		i;
AGLContext agl_ctx = setupInfo->drawContext;


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

	for (i = 0; i < MAX_VAPORTRAILS; i++)
	{
		if (!gVaporTrails[i].isActive)							// must be active
			continue;
		if (gVaporTrails[i].numSegments < 2)					// takes at least 2 segments to draw something
			continue;

		switch(gVaporTrails[i].type)
		{
			case	VAPORTRAIL_TYPE_COLORSTREAK:
					DrawVaporTrail_ColorStreak(i, setupInfo);
					break;

			case	VAPORTRAIL_TYPE_SMOKECOLUMN:
					DrawVaporTrail_SmokeColumn(i, setupInfo);
					break;

			default:
					DoFatalAlert("\pDrawVaporTrails: unsupported type");
		}


	}


			/* RESTORE STATE */

	OGL_PopState();
	glLineWidth(1);
}


/************************** DRAW VAPOR TRAIL:  COLOR STREAK **************************/

static void DrawVaporTrail_ColorStreak(int	i, OGLSetupOutputType *setupInfo)
{
u_long	w,p,n;
float	size,dist;
AGLContext agl_ctx = setupInfo->drawContext;


	n = gVaporTrails[i].numSegments;						// get # segments
	if (n > MAX_TRAIL_SEGMENTS)
		DoFatalAlert("\pDrawVaporTrail_ColorStreak: n > MAX_TRAIL_SEGMENTS");

			/* SET LINE SIZE */

	size = gVaporTrails[i].size;							// get size of trail
	dist = CalcQuickDistance(gVaporTrails[i].points[n-1].x, gVaporTrails[i].points[n-1].z,
								setupInfo->cameraPlacement.cameraLocation.x,setupInfo->cameraPlacement.cameraLocation.z);
	size *= 700.0f / dist;

	w = (float)(gGamePrefs.screenHeight / 15) * size;

	if (gOSX)					// check max line width differently since OS 9 Rage 128 drivers seem to crash when lines get too wide
	{
		if (w > 60)
			w = 60;
	}
	else
	{
		if (w > 20)
			w = 20;
	}

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
}


/************************** DRAW VAPOR TRAIL:  SMOKE COLUMN **************************/


static void DrawVaporTrail_SmokeColumn(int	i, OGLSetupOutputType *setupInfo)
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

	MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_PARTICLES][PARTICLE_SObjType_GreySmoke].materialObject, setupInfo);		// activate material
	MO_DrawGeometry_VertexArray(&gSmokeColumnMesh, setupInfo);

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


