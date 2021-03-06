/****************************/
/*   	SPLINE ITEMS.C      */
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



/**********************/
/*     VARIABLES      */
/**********************/

SplineDefType	**gSplineList = nil;
long			gNumSplines = 0;

static long		gNumSplineObjects = 0;
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
		NilPrime,							// 69:  teleporter map
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

void GetCoordOnSpline(SplineDefType *splinePtr, float placement, float *x, float *z)
{
float			numPointsInSpline;
SplinePointType	*points;
int				i;

	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline
	points = *splinePtr->pointList;								// point to point list

	i = numPointsInSpline * placement;							// get index

	*x = points[i].x;											// get coord
	*z = points[i].z;
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
//
// OUTPUT: 	x,y = coords
//

void GetObjectCoordOnSpline(ObjNode *theNode)
{
float			numPointsInSpline,placement;
SplinePointType	*points;
SplineDefType	*splinePtr;
long			i;

	placement = theNode->SplinePlacement;						// get placement
	if (placement < 0.0f)
		placement = 0;
	else
	if (placement >= 1.0f)
		placement = .999f;

	splinePtr = &(*gSplineList)[theNode->SplineNum];			// point to the spline

	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline
	points = *splinePtr->pointList;								// point to point list

	i = numPointsInSpline * placement;							// calc index
	theNode->Coord.x = points[i].x;								// get coord
	theNode->Coord.z = points[i].z;

	theNode->Delta.x = (theNode->Coord.x - theNode->OldCoord.x) * gFramesPerSecond;	// calc delta
	theNode->Delta.z = (theNode->Coord.z - theNode->OldCoord.z) * gFramesPerSecond;
}


/******************* INCREASE SPLINE INDEX *********************/
//
// Moves objects on spline at given speed
//

void IncreaseSplineIndex(ObjNode *theNode, float speed)
{
SplineDefType	*splinePtr;
float			numPointsInSpline;

	speed *= gFramesPerSecondFrac;

	splinePtr = &(*gSplineList)[theNode->SplineNum];			// point to the spline
	numPointsInSpline = splinePtr->numPoints;					// get # points in the spline

	theNode->SplinePlacement += speed / numPointsInSpline;
	if (theNode->SplinePlacement > .999f)
	{
		theNode->SplinePlacement -= .999f;
		if (theNode->SplinePlacement > .999f)			// see if it wrapped somehow
			theNode->SplinePlacement = 0;
	}
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
		if (theNode->SplinePlacement >= .999f)
		{
			theNode->SplinePlacement = .999f;
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




