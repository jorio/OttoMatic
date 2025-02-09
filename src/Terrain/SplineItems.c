/****************************/
/*   	SPLINE ITEMS.C      */
/* (C)2001 Pangea Software  */
/* (C)2023 Iliyas Jorio     */
/****************************/


#include "game.h"

/***************/
/* EXTERNALS   */
/***************/

/****************************/
/*    PROTOTYPES            */
/****************************/

static Boolean NilPrime(long splineNum, SplineItemType *itemPtr);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_SPLINE_OBJECTS		100
#define	MAX_PLACEMENT			(1.0f - EPS)



/**********************/
/*     VARIABLES      */
/**********************/

SplineDefType	**gSplineList = nil;
int				gNumSplines = 0;

static int		gNumSplineObjects = 0;
static ObjNode	*gSplineObjectList[MAX_SPLINE_OBJECTS];


/**********************/
/*     TABLES         */
/**********************/

#define	MAX_SPLINE_ITEM_NUM		103				// for error checking!

Boolean (*gSplineItemPrimeRoutines[MAX_SPLINE_ITEM_NUM+1])(long, SplineItemType *) =
{
		NilPrime,							// My Start Coords
		NilPrime,
		NilPrime,
		NilPrime,							// 3: squooshy enemy
		PrimeHuman,							// 4: human
		NilPrime,
		NilPrime,
		PrimeEnemy_BrainAlien,				// 7:  brain alien
		PrimeEnemy_Onion,					// 8:  onion
		PrimeEnemy_Corn,					// 9:  corn
		PrimeEnemy_Tomato,					// 10: tomato
		NilPrime,							// 11:
		NilPrime,							// 12:
		NilPrime,							// 13:
		NilPrime,							// 14:
		NilPrime,							// 15:
		NilPrime,							// 16:
		NilPrime,							// 17:
		NilPrime,							// 18:
		NilPrime,							// 19:
		NilPrime,							// 20:
		NilPrime,							// 21:
		NilPrime,							// 22:
		NilPrime,							// 23:
		NilPrime,							// 24:
		NilPrime,							// 25:
		NilPrime,							// 26:
		NilPrime,							// 27:
		NilPrime,							// 28:
		NilPrime,							// 29:
		NilPrime,							// 30:
		NilPrime,							// 31:
		NilPrime,							// 32:
		NilPrime,							// 33:
		NilPrime,							// 34:
		PrimeMagnetMonster,					// 35: magnet monster
		NilPrime,							// 36:
		NilPrime,							// 37:
		NilPrime,							// 38:
		NilPrime,							// 39:
		PrimeMovingPlatform,				// 40: moving platform
		NilPrime,							// 41:
		NilPrime,							// 42:
		NilPrime,							// 43:
		NilPrime,							// 44:
		NilPrime,							// 45:
		NilPrime,							// 46:
		NilPrime,							// 47:
		NilPrime,							// 48:
		PrimeEnemy_Flamester,				// 49: flamester
		PrimeEnemy_GiantLizard,				// 50: giant lizard
		NilPrime,							// 51:
		PrimeEnemy_Mantis,					// 52: mantis
		NilPrime,							// 53:
		NilPrime,							// 54:
		NilPrime,							// 55:
		NilPrime,							// 56:
		NilPrime,							// 57:
		NilPrime,							// 58:
		PrimeEnemy_Mutant,					// 59:
		PrimeEnemy_MutantRobot,				// 60:
		PrimeHumanScientist,				// 61:  scientist human
		NilPrime,							// 62:  proximity mine
		NilPrime,							// 63:  lamp posts
		NilPrime,							// 64:  debris gate
		NilPrime,							// 65:  grave stone
		NilPrime,							// 66:  crashed ship
		NilPrime,							// 67:  chain reacting mine
		NilPrime,							// 68:  rubble
		NilPrime,							// 69:  teleporter map (UNUSED)
		NilPrime,							// 70:  green steam
		NilPrime,							// 71:  tentacle generator
		NilPrime,							// 72:  pitcher plant boss
		NilPrime,							// 73:  pitcher pod
		NilPrime,							// 74:  tractor beam post
		NilPrime,							// 75:  cannon
		NilPrime,							// 76:  bumper car
		NilPrime,							// 77:	tire bumper
		PrimeEnemy_Clown,					// 78:	clown enemy
		PrimeEnemy_ClownFish,				// 79: clown fish
		NilPrime,							// 80:
		PrimeEnemy_StrongMan,				// 81: strongman
		NilPrime,							// 82:
		NilPrime,							// 83:
		NilPrime,							// 84:
		NilPrime,							// 85:
		NilPrime,							// 86:
		NilPrime,							// 87:
		NilPrime,							// 88:
		PrimeEnemy_JawsBot,					// 89: jawsbot enemy
		NilPrime,							// 90:
		NilPrime,							// 91:
		PrimeEnemy_IceCube,					// 92:	ice cube enemy
		PrimeEnemy_HammerBot,				// 93:	hammer bot enemy
		PrimeEnemy_DrillBot,				// 94:  drill bot
		PrimeEnemy_SwingerBot,				// 95:  swinger bot
		NilPrime,							// 96:
		NilPrime,							// 97:
		PrimeLavaPlatform,					// 98:	lava platform
		NilPrime,							// 99:
		NilPrime,							// 100:
		NilPrime,							// 101:
		NilPrime,							// 102:
		PrimeRailGun,						// 103: rail gun
};



/********************* PRIME SPLINES ***********************/
//
// Called during terrain prime function to initialize
// all items on the splines and recalc spline coords
//

void PrimeSplines(void)
{
long			s,i,type;
SplineItemType 	*itemPtr;
SplineDefType	*spline;
Boolean			flag;
SplinePointType	*points;


			/* ADJUST SPLINE TO GAME COORDINATES */

	for (s = 0; s < gNumSplines; s++)
	{
		spline = &(*gSplineList)[s];							// point to this spline
		points = (*spline->pointList);							// point to points list

		for (i = 0; i < spline->numPoints; i++)
		{
			points[i].x *= MAP2UNIT_VALUE;
			points[i].z *= MAP2UNIT_VALUE;
		}

	}


				/* CLEAR SPLINE OBJECT LIST */

	gNumSplineObjects = 0;										// no items in spline object node list yet

	for (s = 0; s < gNumSplines; s++)
	{
		spline = &(*gSplineList)[s];							// point to this spline

				/* SCAN ALL ITEMS ON THIS SPLINE */

		HLockHi((Handle)spline->itemList);						// make sure this is permanently locked down
		for (i = 0; i < spline->numItems; i++)
		{
			itemPtr = &(*spline->itemList)[i];					// point to this item
			type = itemPtr->type;								// get item type
			if (type > MAX_SPLINE_ITEM_NUM)
				DoFatalAlert("PrimeSplines: type > MAX_SPLINE_ITEM_NUM");

			flag = gSplineItemPrimeRoutines[type](s,itemPtr); 	// call item's Prime routine
			if (flag)
				itemPtr->flags |= ITEM_FLAGS_INUSE;				// set in-use flag
		}
	}
}


/******************** NIL PRIME ***********************/
//
// nothing prime
//

static Boolean NilPrime(long splineNum, SplineItemType *itemPtr)
{
#pragma unused (splineNum, itemPtr)

	return(false);
}


/*********************** GET COORD ON SPLINE **********************/

int GetCoordOnSpline(const SplineDefType* spline, float placement, float* x, float* z)
{
	int numPoints = spline->numPoints;

	GAME_ASSERT(spline->numPoints > 0);

	// Clamp placement before accessing array to avoid overrun
	placement = ClampFloat(placement, 0, MAX_PLACEMENT);

	float scaledPlacement = placement * numPoints;

	int index1 = (int)(scaledPlacement);
	int index2 = (index1 < numPoints - 1) ? (index1 + 1) : (0);

	GAME_ASSERT(index1 >= 0 && index1 <= numPoints);
	GAME_ASSERT(index2 >= 0 && index2 <= numPoints);

	const SplinePointType* point1 = &(*spline->pointList)[index1];
	const SplinePointType* point2 = &(*spline->pointList)[index2];

	// Fractional of progression from point1 to point2
	float interpointFrac = scaledPlacement - (int)scaledPlacement;

	// Lerp point1 -> point2
	*x = LerpFloat(point1->x, point2->x, interpointFrac);
	*z = LerpFloat(point1->z, point2->z, interpointFrac);

	return index1;
}

/*********************** GET NEXT COORD ON SPLINE **********************/
//
// Same as above except returns coord of the next point on the spline instead of the exact
// current one.
//

void GetNextCoordOnSpline(SplineDefType *splinePtr, float placement, float *x, float *z)
{
float			numPointsInSpline;
SplinePointType	*points;
int				i;

	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline
	points = *splinePtr->pointList;								// point to point list

	i = numPointsInSpline * placement;							// get index
	i++;														// bump it up +1

	if (i >= numPointsInSpline)									// see if wrap around
		i = 0;

	*x = points[i].x;		// get coord
	*z = points[i].z;
}


/********************* IS SPLINE ITEM VISIBLE ********************/
//
// Returns true if the input objnode is in visible range.
// Also, this function handles the attaching and detaching of the objnode
// as needed.
//

Boolean IsSplineItemVisible(ObjNode *theNode)
{
Boolean	visible = true;
long	row,col;


			/* IF IS ON AN ACTIVE SUPERTILE, THEN ASSUME VISIBLE */

	row = theNode->Coord.z * TERRAIN_SUPERTILE_UNIT_SIZE_Frac;	// calc supertile row,col
	col = theNode->Coord.x * TERRAIN_SUPERTILE_UNIT_SIZE_Frac;

	if ((row < 0) || (row >= gNumSuperTilesDeep) || (col < 0) || (col >= gNumSuperTilesWide))		// make sure in bounds
		visible = false;
	else
	{
		if (gSuperTileStatusGrid[row][col].playerHereFlag)
			visible = true;
		else
			visible = false;
	}
			/* HANDLE OBJNODE UPDATES */

	if (visible)
	{
		if (theNode->StatusBits & STATUS_BIT_DETACHED)			// see if need to insert into linked list
		{
			AttachObject(theNode, true);
		}
	}
	else
	{
		if (!(theNode->StatusBits & STATUS_BIT_DETACHED))		// see if need to remove from linked list
		{
			DetachObject(theNode, true);
		}
	}

	return(visible);
}


#pragma mark ======= SPLINE OBJECTS ================

/******************* ADD TO SPLINE OBJECT LIST ***************************/
//
// Called by object's primer function to add the detached node to the spline item master
// list so that it can be maintained.
//

void AddToSplineObjectList(ObjNode *theNode, Boolean setAim)
{

	if (gNumSplineObjects >= MAX_SPLINE_OBJECTS)
		DoFatalAlert("AddToSplineObjectList: too many spline objects");

	theNode->SplineObjectIndex = gNumSplineObjects;					// remember where in list this is

	gSplineObjectList[gNumSplineObjects++] = theNode;


			/* SET INITIAL AIM */

	if (setAim)
		SetSplineAim(theNode);
}

/************************ SET SPLINE AIM ***************************/

void SetSplineAim(ObjNode *theNode)
{
float	x,z;

	GetNextCoordOnSpline(&(*gSplineList)[theNode->SplineNum], theNode->SplinePlacement, &x, &z);			// get coord of next point on spline
	theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->Coord.x, theNode->Coord.z,x,z);	// calc y rot aim


}

/****************** REMOVE FROM SPLINE OBJECT LIST **********************/
//
// OUTPUT:  true = the obj was on a spline and it was removed from it
//			false = the obj was not on a spline.
//

Boolean RemoveFromSplineObjectList(ObjNode *theNode)
{
	theNode->StatusBits &= ~STATUS_BIT_ONSPLINE;		// make sure this flag is off

	if (theNode->SplineObjectIndex != -1)
	{
		gSplineObjectList[theNode->SplineObjectIndex] = nil;			// nil out the entry into the list
		theNode->SplineObjectIndex = -1;
		theNode->SplineItemPtr = nil;
		theNode->SplineMoveCall = nil;
		return(true);
	}
	else
	{
		return(false);
	}
}


/**************** EMPTY SPLINE OBJECT LIST ***********************/
//
// Called by level cleanup to dispose of the detached ObjNode's in this list.
//

void EmptySplineObjectList(void)
{
int	i;
ObjNode	*o;

	for (i = 0; i < gNumSplineObjects; i++)
	{
		o = gSplineObjectList[i];
		if (o)
			DeleteObject(o);			// This will dispose of all memory used by the node.
										// RemoveFromSplineObjectList will be called by it.
	}

}


/******************* MOVE SPLINE OBJECTS **********************/

void MoveSplineObjects(void)
{
long	i;
ObjNode	*theNode;

	for (i = 0; i < gNumSplineObjects; i++)
	{
		theNode = gSplineObjectList[i];
		if (theNode)
		{
			if (theNode->SplineMoveCall)
			{
				KeepOldCollisionBoxes(theNode);					// keep old boxes & other stuff
				theNode->SplineMoveCall(theNode);				// call object's spline move routine
			}
		}
	}
}


/*********************** GET OBJECT COORD ON SPLINE **********************/

void GetObjectCoordOnSpline(ObjNode *theNode)
{
	GetCoordOnSpline(
			&(*gSplineList)[theNode->SplineNum],
			theNode->SplinePlacement,
			&theNode->Coord.x,
			&theNode->Coord.z);

	theNode->Delta.x = (theNode->Coord.x - theNode->OldCoord.x) * gFramesPerSecond;	// calc delta
	theNode->Delta.z = (theNode->Coord.z - theNode->OldCoord.z) * gFramesPerSecond;
}


/******************* INCREASE SPLINE INDEX *********************/
//
// Moves objects on spline at given speed
//

float IncreaseSplineIndex(ObjNode *theNode, float speed)
{
	SplineDefType* spline = &(*gSplineList)[theNode->SplineNum];

	float placement = theNode->SplinePlacement;

	placement += speed * gFramesPerSecondFrac / spline->numPoints;

	if (placement > MAX_PLACEMENT)
	{
		// Loop to start
		placement -= 1.0f;
		placement = ClampFloat(placement, 0, MAX_PLACEMENT);
	}
	else if (placement < 0)
	{
		// Loop to end
		placement += 1.0f;
		placement = ClampFloat(placement, 0, MAX_PLACEMENT);
	}

	theNode->SplinePlacement = placement;

	return placement;
}


/******************* INCREASE SPLINE INDEX ZIGZAG *********************/
//
// Moves objects on spline at given speed, but zigzags
//

void IncreaseSplineIndexZigZag(ObjNode *theNode, float speed)
{
SplineDefType	*splinePtr;
float			numPointsInSpline;

	speed *= gFramesPerSecondFrac;

	splinePtr = &(*gSplineList)[theNode->SplineNum];			// point to the spline
	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline

			/* GOING BACKWARD */

	if (theNode->StatusBits & STATUS_BIT_REVERSESPLINE)			// see if going backward
	{
		theNode->SplinePlacement -= speed / numPointsInSpline;
		if (theNode->SplinePlacement <= 0.0f)
		{
			theNode->SplinePlacement = 0;
			theNode->StatusBits ^= STATUS_BIT_REVERSESPLINE;	// toggle direction
		}
	}

		/* GOING FORWARD */

	else
	{
		theNode->SplinePlacement += speed / numPointsInSpline;
		if (theNode->SplinePlacement >= MAX_PLACEMENT)
		{
			theNode->SplinePlacement = MAX_PLACEMENT;
			theNode->StatusBits ^= STATUS_BIT_REVERSESPLINE;	// toggle direction
		}
	}
}


/******************** DETACH OBJECT FROM SPLINE ***********************/

void DetachObjectFromSpline(ObjNode *theNode, void (*moveCall)(ObjNode*))
{

	if (!(theNode->StatusBits & STATUS_BIT_ONSPLINE))
		return;

		/***********************************************/
		/* MAKE SURE ALL COMPONENTS ARE IN LINKED LIST */
		/***********************************************/

	AttachObject(theNode, true);


			/* REMOVE FROM SPLINE */

	RemoveFromSplineObjectList(theNode);

	theNode->InitCoord  = theNode->Coord;			// remember where started

	theNode->MoveCall = moveCall;

}




/******************* DRAW SPLINES FOR DEBUGGING *********************/

void DrawSplines(void)
{
	for (int i = 0; i < gNumSplines; i++)
	{
		const SplineDefType* spline = &(*gSplineList)[i];

		if (spline->numPoints == 0)
		{
			continue;
		}

		const SplinePointType* points = *spline->pointList;
		const SplinePointType* nubs = *spline->nubList;
		const int halfway = spline->numPoints / 2;

		if (SeeIfCoordsOutOfRange(points[0].x, points[0].z)
			&& SeeIfCoordsOutOfRange(points[halfway].x, points[halfway].z))
		{
			continue;
		}

		Boolean flat = false;
		float flatY = 0;

		if (gLevelNum == 2)
		{
			flat = true;
			flatY = 500;
		}
		else
		{
			flat = GetWaterY(points[halfway].x, points[halfway].z, &flatY);
		}

		glBegin(GL_LINE_STRIP);
		for (int j = 0; j < spline->numPoints; j++)
		{
			float x = points[j].x;
			float z = points[j].z;
			float y = flat? flatY: GetTerrainY2(x, z) + 10;
			glVertex3f(x, y + (j&1) * 5, z);
			glVertex3f(x, y + (!(j&1)) * 5, z);
		}
		glEnd();

		glBegin(GL_LINES);
		for (int nub = 0; nub < spline->numNubs; nub++)
		{
			float flagSize = nub == 0 ? 150 : 30;
			float poleSize = flagSize;

			float x = nubs[nub].x * MAP2UNIT_VALUE;
			float z = nubs[nub].z * MAP2UNIT_VALUE;
			float y1 = flat ? flatY : GetTerrainY2(x, z) + 17;
			float y2 = y1 + poleSize;
			glVertex3f(x, y1, z);
			glVertex3f(x, y2, z);

			if (nub < spline->numNubs - 1)
			{
				OGLVector3D flagDir = { nubs[nub+1].x * MAP2UNIT_VALUE - x, 0, nubs[nub+1].z * MAP2UNIT_VALUE - z };
				FastNormalizeVector(flagDir.x, flagDir.y, flagDir.z, &flagDir);

				float y3 = y2 - flagSize * 0.25f;
				float y4 = y2 - flagSize * 0.5f;

				glVertex3f(x, y2, z);
				glVertex3f(x + flagDir.x * flagSize, y3, z + flagDir.z * flagSize);
				glVertex3f(x + flagDir.x * flagSize, y3, z + flagDir.z * flagSize);
				glVertex3f(x, y4, z);

				if (nub == 0)
				{
					for (int baton = 0; baton < i + 1; baton++)
					{
						float x2 = x + ((2 + baton) * 4) * flagDir.x;
						float z2 = z + ((2 + baton) * 4) * flagDir.z;
						glVertex3f(x2, y3-10, z2);
						glVertex3f(x2, y3+10, z2);
					}
				}
			}
		}
		glEnd();
	}
}




/********** GET AMOUNT OF POINTS TO GENERATE FOR EACH SPAN OF THE SPLINE **********/
//
// Do not "improve" the cheezeball distance computation! It matches how the splines
// were generated by OreoTerrain in the original version of the game.
//
// If the amount of points per span changes, this will throw off the timing of
// spline-bound objects such as slugs, caterpillars, feet, patrolling ants and
// boxerflies, honeycomb platforms, etc.
// The timing of their movement depends on hardcoded point intervals!
//

static int GetSplinePointsPerSpan(int numNubs, const SplinePointType* nubs, int* outPointsPerSpan)
{
	int total = 0;

	if (outPointsPerSpan)
		SDL_memset(outPointsPerSpan, 0, sizeof(outPointsPerSpan[0]) * numNubs);

	for (int i = 0; i < numNubs - 1; i++)
	{
		float cheezeballDistance = CalcQuickDistance(
			nubs[i].x, nubs[i].z,
			nubs[i + 1].x, nubs[i + 1].z);

		int spanPoints = (int)(3.0f * cheezeballDistance);

		if (outPointsPerSpan)
			outPointsPerSpan[i] = spanPoints;

		total += spanPoints;
	}

#if 0
	// Add final nub for compatibility with stock OreoTerrain splines (Bugdom needed that - not Otto, apparently)
	if (outPointsPerSpan)
		outPointsPerSpan[numNubs - 1] = 1;
	total++;
#endif

	return total;
}


/********************* GENERATE SPLINE POINTS FROM NUBS *******************/
//
// Modified from Nanosaur 2.
// The result closely matches the pre-baked spline points in Bugdom terrain files.
//

static SplinePointType** BakeSpline(int numNubs, const SplinePointType *nubs, const int* pointsPerSpan)
{
SplinePointType** pointsHandle;
SplinePointType**space,*points;
SplinePointType*a, *b, *c, *d;
SplinePointType*h0, *h1, *h2, *h3, *hi_a;
int			numPoints;


				/* ALLOCATE 2D ARRAY FOR CALCULATIONS */

	Alloc_2d_array(SplinePointType, space, 8, numNubs);


				/* ALLOC POINT ARRAY */

	int maxPoints = 0;
	for (int i = 0; i < numNubs; i++)
		maxPoints += pointsPerSpan[i];

	pointsHandle = (SplinePointType**) AllocHandle(sizeof(SplinePointType) * maxPoints);
	points = *pointsHandle;


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

	for (int i = 0; i < numNubs; i++)
		d[i] = nubs[i];


	for (int i = 0, imax = numNubs - 2; i < imax; i++)
	{
		h2[i].x = 1;
		h2[i].z = 1;
		h3[i].x = 3 * (d[i + 2].x - 2 * d[i + 1].x + d[i].x);
		h3[i].z = 3 * (d[i + 2].z - 2 * d[i + 1].z + d[i].z);
	}
	h2[numNubs - 3].x = 0;
	h2[numNubs - 3].z = 0;

	a[0].x = 4;
	a[0].z = 4;
	h1[0].x = h3[0].x / a[0].x;
	h1[0].z = h3[0].z / a[0].z;

	for (int i = 1, i1 = 0, imax = numNubs - 2; i < imax; i++, i1++)
	{
		h0[i1].x = h2[i1].x / a[i1].x;
		a[i].x = 4.0f - h0[i1].x;
		h1[i].x = (h3[i].x - h1[i1].x) / a[i].x;

		h0[i1].z = h2[i1].z / a[i1].z;
		a[i].z = 4.0f - h0[i1].z;
		h1[i].z = (h3[i].z - h1[i1].z) / a[i].z;
	}

	b[numNubs - 3] = h1[numNubs - 3];

	for (int i = numNubs - 4; i >= 0; i--)
	{
 		b[i].x = h1[i].x - h0[i].x * b[i+ 1].x;
 		b[i].z = h1[i].z - h0[i].z * b[i+ 1].z;
 	}

	for (int i = numNubs - 2; i >= 1; i--)
		b[i] = b[i - 1];

	b[0].x = 0;
	b[0].z = 0;
	b[numNubs - 1].x = 0;
	b[numNubs - 1].z = 0;
	hi_a = a + numNubs - 1;

	for (; a < hi_a; a++, b++, c++, d++)
	{
		c->x = ((d+1)->x - d->x) -(2.0f * b->x + (b+1)->x) * (1.0f/3.0f);
		a->x = ((b+1)->x - b->x) * (1.0f/3.0f);

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
	for (int nub = 0; a < hi_a; a++, b++, c++, d++, nub++)
	{
		GAME_ASSERT(nub < numNubs - 1);

		int subdivisions = pointsPerSpan[nub];

				/* CALC THIS SPAN */

		for (int spanPoint = 0; spanPoint < subdivisions; spanPoint++)
		{
			GAME_ASSERT(numPoints < maxPoints);										// see if overflow
			float t = spanPoint / (float) subdivisions;
			points[numPoints].x = ((a->x * t + b->x) * t + c->x) * t + d->x;		// save point
			points[numPoints].z = ((a->z * t + b->z) * t + c->z) * t + d->z;
			numPoints++;
		}
	}


		/* ADD FINAL NUB AS POINT IF REQUIRED */

	GAME_ASSERT(pointsPerSpan[numNubs - 1] == 0 || pointsPerSpan[numNubs - 1] == 1);

	if (pointsPerSpan[numNubs - 1] == 1)
	{
		GAME_ASSERT(numPoints < maxPoints);										// see if overflow
		points[numPoints].x = nubs[numNubs - 1].x;
		points[numPoints].z = nubs[numNubs - 1].z;
		numPoints++;
	}



		/* END */

	GAME_ASSERT(numPoints == maxPoints);

	Free_2d_array(space);

	return pointsHandle;
}


/********************* MAKE SPLINE LOOP SEAMLESSLY *******************/
//
// This attempts to keep the interval between nubs as close as possible to the original spline.
//

void PatchSplineLoop(SplineDefType* spline)
{
	int numNubs = spline->numNubs;
	SplinePointType* nubList = *spline->nubList;

	int numWrapNubs = 3;		// wrap around for this many nubs
	GAME_ASSERT(numNubs > numWrapNubs);

		/* FIND OUT HOW MANY POINTS TO GENERATE */

	int* pointsPerSpan_wrapping = (int*) AllocPtr(sizeof(int) * (numNubs + 2*numWrapNubs));
	int* pointsPerSpan = &pointsPerSpan_wrapping[numWrapNubs];

	int newNumPoints = GetSplinePointsPerSpan(numNubs, nubList, pointsPerSpan);

		/* MERGE ENDPOINT NUBS SO SPLINE LOOPS SEAMLESSLY */

	SplinePointType* firstNub = &nubList[0];
	SplinePointType* lastNub = &nubList[numNubs - 1];

	float endpointDistance = CalcDistance(firstNub->x, firstNub->z, lastNub->x, lastNub->z);
	GAME_ASSERT_MESSAGE(endpointDistance < 20, "spline endpoint nubs are too far apart for seamless looping");

	SplinePointType mergedEndpoint = { (firstNub->x + lastNub->x) * 0.5f, (firstNub->z + lastNub->z) * 0.5f };
	*firstNub = mergedEndpoint;
	*lastNub = mergedEndpoint;

		/* MAKE SPLINE WRAP AROUND A BIT TO AVOID ANGULAR PINCH AT SEAM */

	SplinePointType* nubList_wrapping = (SplinePointType*) AllocPtr(sizeof(SplinePointType) * (numNubs + 2 * numWrapNubs));
	SDL_memcpy(&nubList_wrapping[numWrapNubs], nubList, sizeof(SplinePointType) * numNubs);

	int wrapBeg = numWrapNubs - 1;
	int wrapEnd = numWrapNubs + numNubs;
	for (int i = 0; i < numWrapNubs; i++, wrapBeg--, wrapEnd++)
	{
		GAME_ASSERT(wrapBeg >= 0);
		GAME_ASSERT(wrapEnd <= numNubs + 2 * numWrapNubs);

		nubList_wrapping[wrapBeg] = nubList[PositiveModulo(-2-i, numNubs)];
		nubList_wrapping[wrapEnd] = nubList[PositiveModulo(1+numNubs+i, numNubs)];

		pointsPerSpan_wrapping[wrapBeg] = 0;
		pointsPerSpan_wrapping[wrapEnd] = 0;
	}

		/* BAKE SPLINE AGAIN */

	SplinePointType** newPointList = BakeSpline(numNubs + 2*numWrapNubs, nubList_wrapping, pointsPerSpan_wrapping);

#if _DEBUG
		/* MAKE SURE NEW SPLINE HASN'T STRAYED TOO FAR FROM FILE VALUES */

	int oldNumPoints = spline->numPoints;
	GAME_ASSERT(SDL_abs(newNumPoints - oldNumPoints) <= 10);		// tolerate some wiggle room
	// (note: level 3 will be especially bad here since I edited the nubs in the .ter.rsrc file)

	int currentSpan = 0;
	int currentSpanPoints = 0;
	for (int p = 0; p < MinInt(newNumPoints, oldNumPoints); p++)
	{
		float dx = (*spline->pointList)[p].x - (*newPointList)[p].x;
		float dz = (*spline->pointList)[p].z - (*newPointList)[p].z;
		float drift = sqrtf(dx*dx + dz*dz);

		if (currentSpan == 0 || currentSpan >= numNubs - 2)
			GAME_ASSERT(drift < 30);	// tolerate some drift for loop point nubs (we've doctored those)
		else
			GAME_ASSERT(drift < 10);	// tolerate less drift for nubs further from the loop point

		currentSpanPoints++;
		if (currentSpanPoints >= pointsPerSpan[currentSpan])
		{
			currentSpan++;
			currentSpanPoints = 0;
		}
	}
#endif

		/* COMMIT NEW BAKED SPLINE */

	DisposeHandle((Handle) spline->pointList);				// replace old point list

	spline->pointList = newPointList;
	spline->numPoints = newNumPoints;


		/* CLEAN UP */

	SafeDisposePtr((Ptr) pointsPerSpan_wrapping);
	SafeDisposePtr((Ptr) nubList_wrapping);
}

