/****************************/
/*   	PODWORMS.C		    */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


#include "game.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond,gGlobalTransparency,gGravity;
extern	OGLPoint3D			gCoord;
extern	PlayerInfoType	gPlayerInfo;
extern	OGLVector3D			gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLVector3D			gRecentTerrainNormal;
extern	SplineDefType		**gSplineList;
extern	u_long		gAutoFadeStatusBits;
extern	short				gNumCollisions;
extern	SuperTileStatus	**gSuperTileStatusGrid;
extern	NewParticleGroupDefType	gNewParticleGroupDef;
extern	PrefsType			gGamePrefs;
extern	const float gSkyY[];
extern	int					gLevelNum;
extern	ObjNode				*gFirstNodePtr, *gTargetPickup;
extern	SpriteType	*gSpriteGroupList[MAX_SPRITE_GROUPS];
extern	SparkleType	gSparkles[MAX_SPARKLES];
extern	OGLBoundingBox			gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	Boolean				gG4;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveSpacePod_Generator(ObjNode *theNode);
static void MoveSpacePod_Falling(ObjNode *theNode);
static void MoveSpacePod_Landed(ObjNode *theNode);
static void MovePodRipple(ObjNode *theNode);

static void SpawnPodWorms(OGLPoint3D *where);
static void GenerateWormSplinePoints(short wormNum, OGLPoint3D *nubPoints);
static void MovePodWorm(ObjNode *worm);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_PODS				10

#define	MAX_POD_WORMS			8

#define	NUM_SPLINE_SPANS		6
#define	NUM_SPLINE_NUBS			(NUM_SPLINE_SPANS+1)
#define	MAX_POINTS_PER_SPAN		300
#define	MAX_SPLINE_POINTS		(MAX_POINTS_PER_SPAN * NUM_SPLINE_SPANS)


typedef struct
{
	Boolean		isUsed;
	short		numPoints;
	OGLPoint3D	splinePoints[MAX_SPLINE_POINTS];
}PodWormType;


/*********************/
/*    VARIABLES      */
/*********************/

static short	gNumPods;

static	PodWormType	gPodWorms[MAX_POD_WORMS];


/************************ INIT SPACE PODS *********************************/

void InitSpacePods(void)
{
int	i;

	gNumPods = 0;

	for (i = 0; i < MAX_POD_WORMS; i++)
		gPodWorms[i].isUsed = false;


}


/************************* ADD SPACE POD GENERATOR *********************************/

Boolean AddSpacePodGenerator(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	if (!gG4)								// dont do these if low horsepower
		return(true);

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= GetTerrainY(x,z) + 5000.0f;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= MoveSpacePod_Generator;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Timer = RandomFloat() * 3.0f;

	return(true);													// item was added
}



/********************* MOVE SPACE POD GENERATOR **********************/

static void MoveSpacePod_Generator(ObjNode *theNode)
{
OGLVector3D	aim;

	if (TrackTerrainItem(theNode))									// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

			/*********************************/
			/* SEE IF TIME TO DROP A NEW POD */
			/*********************************/

	if (gNumPods < MAX_PODS)
	{
		theNode->Timer -= gFramesPerSecondFrac;
		if (theNode->Timer <= 0.0f)
		{
			ObjNode	*newObj;

			theNode->Timer = RandomFloat() * 6.0f + 4.0f;								// reset timer

					/* CREATE NEW POD OBJECT */

			gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
			gNewObjectDefinition.type 		= APOCALYPSE_ObjType_SpacePod;
			gNewObjectDefinition.coord.x 	= theNode->Coord.x + RandomFloat2() * 900.0f ;
			gNewObjectDefinition.coord.z 	= theNode->Coord.z + RandomFloat2() * 900.0f ;
			gNewObjectDefinition.coord.y 	= GetTerrainY(gNewObjectDefinition.coord.x,gNewObjectDefinition.coord.z) + 6000.0f;
			gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_USEALIGNMENTMATRIX;
			gNewObjectDefinition.slot 		= 257;
			gNewObjectDefinition.moveCall 	= MoveSpacePod_Falling;
			gNewObjectDefinition.rot 		= 0;
			gNewObjectDefinition.scale 		= 2.0;
			newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			newObj->Delta.y = -7000.0f;
			newObj->Delta.x = RandomFloat2() * 5000.0f;
			newObj->Delta.z = RandomFloat2() * 5000.0f;

					/* SET THE ALIGNMENT MATRIX */

			OGLVector3D_Normalize(&newObj->Delta, &aim);
			SetAlignmentMatrix(&newObj->AlignmentMatrix, &aim);

		}
	}
}


/******************* MOVE SPACE POD : FALLING ***********************/

static void MoveSpacePod_Falling(ObjNode *theNode)
{
DeformationType		defData;
int					i;
static OGLColorRGBA		vaporColor = {1,1,1, 1.1};

	GetObjectInfo(theNode);

	gCoord.x += gDelta.x * gFramesPerSecondFrac;
	gCoord.y += gDelta.y * gFramesPerSecondFrac;
	gCoord.z += gDelta.z * gFramesPerSecondFrac;


			/* SEE IF IMPACTED */

	if (gCoord.y <= GetTerrainY(gCoord.x, gCoord.z))
	{
		MakeBombExplosion(gCoord.x, gCoord.z, &gDelta);
		PlayEffect3D(EFFECT_PODCRASH, &gCoord);

			/* MAKE DEFORMATION WAVE */

		defData.type 				= DEFORMATION_TYPE_RADIALWAVE;
		defData.amplitude 			= 300;
		defData.radius 				= 20;
		defData.speed 				= 3000;
		defData.origin.x			= gCoord.x;
		defData.origin.y			= gCoord.z;
		defData.oneOverWaveLength 	= 1.0f / 500.0f;
		defData.radialWidth			= 800.0f;
		defData.decayRate			= 190.0f;
		NewSuperTileDeformation(&defData);

		theNode->MoveCall = MoveSpacePod_Landed;
		theNode->Health = 2.5;									// set delay before can hatch
		gNumPods++;
	}
	else
	{

				/**************************/
				/* UPDATE THE VAPOR TRAIL */
				/**************************/

		i = theNode->VaporTrails[0];							// get vaport trail #

		if (VerifyVaporTrail(i, theNode, 0))
			AddToVaporTrail(&theNode->VaporTrails[0], &gCoord, &vaporColor);
		else
		{
			theNode->VaporTrails[0] = CreatetNewVaporTrail(theNode, 0, VAPORTRAIL_TYPE_SMOKECOLUMN, &gCoord, &vaporColor, 40.0, 1.0, 200);
		}

				/* SEE IF BREAK ATMOSPHERE */

		if ((theNode->OldCoord.y > gSkyY[gLevelNum]) && (gCoord.y <= gSkyY[gLevelNum]))
		{
			ObjNode	*newObj;

			gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
			gNewObjectDefinition.type 		= GLOBAL_ObjType_Ripple;
			gNewObjectDefinition.coord.x 	= gCoord.x;
			gNewObjectDefinition.coord.z 	= gCoord.z;
			gNewObjectDefinition.coord.y 	= gSkyY[gLevelNum];
			gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_KEEPBACKFACES|
											STATUS_BIT_NOZWRITES|STATUS_BIT_NOLIGHTING;
			gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
			gNewObjectDefinition.moveCall 	= MovePodRipple;
			gNewObjectDefinition.rot 		= 0;
			gNewObjectDefinition.scale 		= 4.0;
			newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			newObj->Rot.x = PI/2;

			newObj->ColorFilter.a = .7f;
		}

					/****************/
					/* UPDATE SOUND */
					/****************/

		if (theNode->EffectChannel == -1)
			theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_PODBUZZ, &gCoord, NORMAL_CHANNEL_RATE, 2.0);
		else
			Update3DSoundChannel(EFFECT_PODBUZZ, &theNode->EffectChannel, &gCoord);


	}

	UpdateObject(theNode);
}

/******************** MOVE SPACE POD : LANDED ***************************/

static void MoveSpacePod_Landed(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))									// just check to see if it's gone
	{
delete:
		DeleteObject(theNode);
		gNumPods--;
		return;
	}

	GetObjectInfo(theNode);

	gCoord.y = GetTerrainY(gCoord.x, gCoord.z);						// keep height on terrain

	UpdateObject(theNode);


	if ((theNode->Health -= gFramesPerSecondFrac) <= 0.0f)
	{
				/* IF PLAYER CLOSE, THEN CRACK OPEN */

		if (OGLPoint3D_Distance(&gCoord, &gPlayerInfo.coord) < 2700.0f)
		{
			SpawnPodWorms(&gCoord);
			ExplodeGeometry(theNode, 700, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .9);
			goto delete;
		}
	}
}



/*************** MOVE POD RIPPLE *****************/

static void MovePodRipple(ObjNode *theNode)
{
	theNode->Scale.x =
	theNode->Scale.y =
	theNode->Scale.z += gFramesPerSecondFrac * 3.0f;

	theNode->ColorFilter.a -= gFramesPerSecondFrac * .6f;
	if (theNode->ColorFilter.a <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}

	UpdateObjectTransforms(theNode);
}

#pragma mark -


/********************* SPAWN POD WORMS **************************/

static void SpawnPodWorms(OGLPoint3D *where)
{
ObjNode	*worm,*player = gPlayerInfo.objNode;
int		i,j;
float	dx,dy,dz,x,y,z;
OGLPoint3D	nubs[NUM_SPLINE_NUBS],tc;
OGLVector2D perp;

				/***********************/
				/* FIND FREE WORM SLOT */
				/***********************/

	for (i = 0; i < MAX_POD_WORMS; i++)
	{
		if (!gPodWorms[i].isUsed)
		{
			gPodWorms[i].isUsed = true;
			goto got_slot;
		}
	}
	return;

got_slot:

			/************************/
			/* GENERATE WORM SPLINE */
			/************************/

	x = where->x;
	y = where->y;
	z = where->z;

	tc.x = player->Coord.x + player->Delta.x;					// calc player target coord
	tc.y = player->Coord.y + player->Delta.y;
	tc.z = player->Coord.z + player->Delta.z;

	dx = (tc.x - x) / NUM_SPLINE_SPANS;							// calc vector to player
	dy = (tc.y - y) / NUM_SPLINE_SPANS;
	dz = (tc.z - z) / NUM_SPLINE_SPANS;


			/* CALC PERPENDICULAR VECTOR FOR OFFSETS */

	perp.x = -dz;
	perp.y = dx;
	FastNormalizeVector2D(perp.x, perp.y, &perp, false);


				/* BUILD NUB LIST */

	for (j = 0; j < NUM_SPLINE_NUBS; j++)
	{
		nubs[j].x = x + (perp.x * RandomFloat2() * 300.0f);				// interpolated point offset by perpendicular amount
		nubs[j].z = z + (perp.y * RandomFloat2() * 300.0f);

		if (j == (NUM_SPLINE_NUBS-1))									// last nub is always deep underground
			nubs[j].y = GetTerrainY(nubs[j].x, nubs[j].z) - 250.0f;
		else
		if (j == 0)														// 1st nub is always @ pod
			nubs[j].y = y;
		else
			nubs[j].y = GetTerrainY(nubs[j].x, nubs[j].z) + 30.0f + RandomFloat() * 400.0f;

		x += dx;
		y += dy;
		z += dz;
	}

				/* GENERATE THE SPLINE */

	GenerateWormSplinePoints(i, nubs);



		/**************************/
		/* CREATE THE WORM OBJECT */
		/**************************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_PODWORM;
	gNewObjectDefinition.animNum 	= 0;
	gNewObjectDefinition.coord		= *where;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 551;
	gNewObjectDefinition.moveCall 	= MovePodWorm;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.5;
	worm = MakeNewSkeletonObject(&gNewObjectDefinition);

	worm->SplinePlacement = 0;
	worm->Kind = i;												// remember which worm slot we're in

	worm->Skeleton->JointsAreGlobal = true;

	worm->CType = CTYPE_HURTME;
	worm->CBits = CBITS_TOUCHABLE;
	SetObjectCollisionBounds(worm, 40,-40,-40,40,40,-40);

	worm->Damage = .1f;


			/*******************/
			/* CREATE SPARKLES */
			/*******************/

	for (j = 0; j < worm->Skeleton->skeletonDefinition->NumBones; j++)
	{
		i = worm->Sparkles[j] = GetFreeSparkle(worm);				// get free sparkle slot
		if (i != -1)
		{
			gSparkles[i].flags 	= SPARKLE_FLAG_OMNIDIRECTIONAL;
			gSparkles[i].where	= gNewObjectDefinition.coord;

			gSparkles[i].color.r = .3;
			gSparkles[i].color.g = 1;
			gSparkles[i].color.b = 1;
			gSparkles[i].color.a = .7;

			gSparkles[i].scale = 90.0f;
			gSparkles[i].separation = -60.0f;

			gSparkles[i].textureNum = PARTICLE_SObjType_WhiteGlow;
		}
	}
}


/***************************** MOVE POD WORM ***********************************/

static void MovePodWorm(ObjNode *worm)
{
float	fps = gFramesPerSecondFrac;
short	wormNum;
int		i,j,q,q2,numSegments,p;
float	scale = worm->Scale.x;
SkeletonObjDataType	*skeleton = worm->Skeleton;

	wormNum = worm->Kind;													// get slot index

			/* MOVE ALONG SPLINE */

	p = worm->SplinePlacement += fps * (MAX_POINTS_PER_SPAN * 1.8f);


		/*******************/
		/* UPDATE SEGMENTS */
		/*******************/

	numSegments = skeleton->skeletonDefinition->NumBones;											// get # joints in skeleton

	for (j = 0; j < numSegments; j++)
	{
		OGLPoint3D		coord,toCoord;
		OGLMatrix4x4	*jointMatrix,m,m2;
		static const	OGLVector3D up = {0,1,0};

		q = p;
		if (q < 0)
			q = 0;
		if (q >= gPodWorms[wormNum].numPoints)
		{
			if (j == (numSegments-1))									// if last segment is beyond end, then we're done
			{
				gPodWorms[wormNum].isUsed = false;
				DeleteObject(worm);
				return;
			}
			q = gPodWorms[wormNum].numPoints-1;
		}

		q2 = q + 10;
		if (q2 < 0)
			q2 = 0;
		if (q2 >= gPodWorms[wormNum].numPoints)
			q2 = gPodWorms[wormNum].numPoints-1;


				/* GET COORDS OF THIS SEGMENT */

		coord.x = gPodWorms[wormNum].splinePoints[q].x;
		coord.y = gPodWorms[wormNum].splinePoints[q].y;
		coord.z = gPodWorms[wormNum].splinePoints[q].z;

		if (j == 0)
			worm->Coord = coord;

			/* GET "TO" SPLINE COORD FOR ROTATION CALCULATION */

		toCoord.x = gPodWorms[wormNum].splinePoints[q2].x;
		toCoord.y = gPodWorms[wormNum].splinePoints[q2].y;
		toCoord.z = gPodWorms[wormNum].splinePoints[q2].z;


				/* TRANSFORM JOINT'S MATRIX TO WORLD COORDS */

		jointMatrix = &skeleton->jointTransformMatrix[j];		// get ptr to joint's xform matrix which was set during regular animation functions

		OGLMatrix4x4_SetScale(&m, scale, scale, scale);

		SetLookAtMatrixAndTranslate(&m2, &up, &coord, &toCoord);
		OGLMatrix4x4_Multiply(&m, &m2, jointMatrix);


					/* UPDATE SPARKLES */

		i = worm->Sparkles[j];
		if (i != -1)
		{
			float	s;

			gSparkles[i].where = coord;

			s = q;
			gSparkles[i].color.g = (cos(s * .01f) + 1.0f) * .5f;
			gSparkles[i].color.a = .7f+ RandomFloat() * .3f;
		}


				/* NEXT SEGMENT */

		p -= MAX_POINTS_PER_SPAN/10;												// next seg is earlier in spline
	}



			/* UPDATE SOUND */

	if (worm->EffectChannel == -1)
		worm->EffectChannel = PlayEffect3D(EFFECT_PODWORM, &worm->Coord);
	else
		Update3DSoundChannel(EFFECT_PODWORM, &worm->EffectChannel, &worm->Coord);

	CalcObjectBoxFromNode(worm);

}


/******************** GENERATE WORM SPLINE POINTS ***************************/

static void GenerateWormSplinePoints(short wormNum, OGLPoint3D *nubPoints)
{
OGLPoint3D	**space,*splinePoints;
OGLPoint3D 	*a, *b, *c, *d;
OGLPoint3D 	*h0, *h1, *h2, *h3, *hi_a;
short 		imax;
float 		t, dt;
long		i,i1,numPoints;
long		nub;
int			pointsPerSpan;
const int	numNubs = NUM_SPLINE_NUBS;


				/* CALC POINT DENSITY */

	pointsPerSpan = MAX_POINTS_PER_SPAN; //OGLPoint3D_Distance(&nubPoints[0], &nubPoints[numNubs-1]) * .1f;		// # points per span is a function of the length of the spline, so measure from start to end
//	if (pointsPerSpan > MAX_POINTS_PER_SPAN)
//		DoFatalAlert("GenerateWormSplinePoints: pointsPerSpan > MAX_POINTS_PER_SPAN");


		/* GET SPLINE INFO */

	splinePoints = gPodWorms[wormNum].splinePoints;						// point to point list



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
  	h2[numNubs - 3].x = h2[numNubs - 3].y = 0;

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
			if (numPoints >= MAX_SPLINE_POINTS)				// see if overflow
				DoFatalAlert("GenerateWormSplinePoints: numPoints >= MAX_SPLINE_POINTS");

 			splinePoints[numPoints].x = ((a->x * t + b->x) * t + c->x) * t + d->x;		// save point
 			splinePoints[numPoints].y = ((a->y * t + b->y) * t + c->y) * t + d->y;
 			splinePoints[numPoints].z = ((a->z * t + b->z) * t + c->z) * t + d->z;

 			numPoints++;
 		}
	}


	gPodWorms[wormNum].numPoints = numPoints;
	Free_2d_array(space);
}








