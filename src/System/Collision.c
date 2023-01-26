/****************************/
/*   	COLLISION.c		    */
/* By Brian Greenstone      */
/* (c)2001 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/




/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_COLLISIONS				60
#define	MAX_TEMP_COLL_TRIANGLES		300

enum
{
	WH_HEAD	=	1,
	WH_FOOT =	1<<1
};

/****************************/
/*    VARIABLES             */
/****************************/


CollisionRec	gCollisionList[MAX_COLLISIONS];
int				gNumCollisions = 0;
Byte			gTotalSides;


/******************* BOX/BOX INTERSECTION *********************/

static inline Boolean IntersectBoxes(const CollisionBoxType* a, const CollisionBoxType* b)
{
	Boolean outside
		=  (a->right < b->left)
		|| (a->left > b->right)
		|| (a->front < b->back)
		|| (a->back > b->front)
		|| (a->bottom > b->top)
		|| (a->top < b->bottom);

	return !outside;
}

/******************* DOES BOX CONTAIN POINT *********************/

static inline Boolean BoxContainsPoint(const CollisionBoxType* box, const OGLPoint3D* point)
{
	Boolean outside
		=  (point->x < box->left)
		|| (point->x > box->right)
		|| (point->z < box->back)
		|| (point->z > box->front)
		|| (point->y > box->top)
		|| (point->y < box->bottom);

	return !outside;
}

/******************* DOES BOX CONTAIN POINT (XZ ONLY) *********************/

static inline Boolean BoxContainsPoint_XZ(const CollisionBoxType* box, const OGLPoint3D* point)
{
	Boolean outside
		= (point->x < box->left)
		|| (point->x > box->right)
		|| (point->z < box->back)
		|| (point->z > box->front);

	return !outside;
}

/******************* COLLISION DETECT *********************/
//
// INPUT: startNumCollisions = value to start gNumCollisions at should we need to keep existing data in collision list
//

void CollisionDetect(ObjNode* baseNode, uint32_t CType, int startNumCollisions)
{
ObjNode 	*thisNode;
long		sideBits,cBits;
uint32_t	cType;
short		numBaseBoxes,targetNumBoxes;

	gNumCollisions = startNumCollisions;								// clear list

			/* GET BASE BOX INFO */

	numBaseBoxes = baseNode->NumCollisionBoxes;
	if (numBaseBoxes == 0)
		return;

	const CollisionBoxType* baseBoxList = baseNode->CollisionBoxes;
	const CollisionBoxType* baseBoxList_old = baseNode->OldCollisionBoxes;


			/****************************/
			/* SCAN AGAINST ALL OBJECTS */
			/****************************/

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		cType = thisNode->CType;
		if (cType == INVALID_NODE_FLAG)							// see if something went wrong
			break;

		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (!(cType & CType))									// see if we want to check this Type
			goto next;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)		// don't collide against these
			goto next;

		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;

		if (thisNode == baseNode)								// dont collide against itself
			goto next;

		if (baseNode->ChainNode == thisNode)					// don't collide against its own chained object
			goto next;

				/******************************/
				/* NOW DO COLLISION BOX CHECK */
				/******************************/

		targetNumBoxes = thisNode->NumCollisionBoxes;			// see if target has any boxes
		if (targetNumBoxes == 0)
			goto next;

		const CollisionBoxType* targetBoxList = thisNode->CollisionBoxes;
		const CollisionBoxType* targetBoxList_old = thisNode->OldCollisionBoxes;


				/******************************************/
				/* CHECK BASE BOX AGAINST EACH TARGET BOX */
				/*******************************************/

		for (int target = 0; target < targetNumBoxes; target++)
		{
					/* DO RECTANGLE INTERSECTION */

			if (!IntersectBoxes(&baseBoxList[0], &targetBoxList[target]))
				continue;


					/* THERE HAS BEEN A COLLISION SO CHECK WHICH SIDE PASSED THRU */

			sideBits = 0;
			cBits = thisNode->CBits;					// get collision info bits

			if (cBits & CBITS_TOUCHABLE)				// if it's generically touchable, then add it without side info
				goto got_sides;


							/* CHECK FRONT COLLISION */

			if (cBits & SIDE_BITS_BACK											// see if target has solid back
				&& baseBoxList_old->front < targetBoxList_old[target].back		// see if wasn't in target already
				&& baseBoxList->front >= targetBoxList[target].back				// see if currently in target
				&& baseBoxList->front <= targetBoxList[target].front)
			{
				sideBits = SIDE_BITS_FRONT;
			}

							/* CHECK BACK COLLISION */

			if (cBits & SIDE_BITS_FRONT											// see if target has solid front
				&& baseBoxList_old->back > targetBoxList_old[target].front		// see if wasn't in target already
				&& baseBoxList->back <= targetBoxList[target].front				// see if currently in target
				&& baseBoxList->back >= targetBoxList[target].back)
			{
				sideBits = SIDE_BITS_BACK;
			}

							/* CHECK RIGHT COLLISION */

			if (cBits & SIDE_BITS_LEFT											// see if target has solid left
				&& baseBoxList_old->right < targetBoxList_old[target].left		// see if wasn't in target already
				&& baseBoxList->right >= targetBoxList[target].left				// see if currently in target
				&& baseBoxList->right <= targetBoxList[target].right)
			{
				sideBits |= SIDE_BITS_RIGHT;
			}

							/* CHECK LEFT COLLISION */

			if (cBits & SIDE_BITS_RIGHT											// see if target has solid right
				&& baseBoxList_old->left > targetBoxList_old[target].right		// see if wasn't in target already
				&& baseBoxList->left <= targetBoxList[target].right				// see if currently in target
				&& baseBoxList->left >= targetBoxList[target].left)
			{
				sideBits |= SIDE_BITS_LEFT;
			}

						/* CHECK TOP COLLISION */

			if (cBits & SIDE_BITS_BOTTOM										// see if target has solid bottom
				&& baseBoxList_old->top < targetBoxList_old[target].bottom		// see if wasn't in target already
				&& baseBoxList->top >= targetBoxList[target].bottom				// see if currently in target
				&& baseBoxList->top <= targetBoxList[target].top)
			{
				sideBits |= SIDE_BITS_TOP;
			}

						/* CHECK BOTTOM COLLISION */

			if (cBits & SIDE_BITS_TOP											// see if target has solid top
				&& baseBoxList_old->bottom > targetBoxList_old[target].top		// see if wasn't in target already
				&& baseBoxList->bottom <= targetBoxList[target].top				// see if currently in target
				&& baseBoxList->bottom >= targetBoxList[target].bottom)
			{
				sideBits |= SIDE_BITS_BOTTOM;
			}

				/*********************************************/
				/* SEE IF ANYTHING TO ADD OR IF IMPENETRABLE */
				/*********************************************/

			if (!sideBits)														// if 0 then no new sides passed thru this time
			{
				if (cType & CTYPE_IMPENETRABLE)									// if its impenetrable, add to list regardless of sides
				{
					if (gCoord.x < thisNode->Coord.x)							// try to assume some side info based on which side we're on relative to the target
						sideBits |= SIDE_BITS_RIGHT;
					else
						sideBits |= SIDE_BITS_LEFT;

					if (gCoord.z < thisNode->Coord.z)
						sideBits |= SIDE_BITS_FRONT;
					else
						sideBits |= SIDE_BITS_BACK;

					goto got_sides;
				}

				if (cBits & CBITS_ALWAYSTRIGGER)								// also always add if always trigger
					goto got_sides;

				continue;														// next target box
			}

					/* ADD TO COLLISION LIST */
got_sides:
			gCollisionList[gNumCollisions].baseBox 		= 0;
			gCollisionList[gNumCollisions].targetBox 	= target;
			gCollisionList[gNumCollisions].sides 		= sideBits;
			gCollisionList[gNumCollisions].type 		= COLLISION_TYPE_OBJ;
			gCollisionList[gNumCollisions].objectPtr 	= thisNode;
			gNumCollisions++;
			gTotalSides |= sideBits;											// remember total of this
		}

next:
		thisNode = thisNode->NextNode;												// next target node
	}while(thisNode != nil);

	if (gNumCollisions > MAX_COLLISIONS)											// see if overflowed (memory corruption ensued)
		DoFatalAlert("CollisionDetect: gNumCollisions > MAX_COLLISIONS");
}


/***************** HANDLE COLLISIONS ********************/
//
// This is a generic collision handler.  Takes care of
// all processing.
//
// INPUT:  cType = CType bit mask for collision matching
//
// OUTPUT: totalSides
//

Byte HandleCollisions(ObjNode* theNode, uint32_t cType, float deltaBounce)
{
Byte		totalSides;
float		originalX,originalY,originalZ;
float		offset,maxOffsetX,maxOffsetZ,maxOffsetY;
float		offXSign,offZSign,offYSign;
Byte		base,target;
ObjNode		*targetObj = nil;
const CollisionBoxType* baseBoxPtr = nil;
const CollisionBoxType* targetBoxPtr = nil;
const CollisionBoxType* boxList = nil;
short		numSolidHits, numPasses = 0;
Boolean		hitImpenetrable = false;
short		oldNumCollisions;
Boolean		previouslyOnGround, hitMPlatform = false;
Boolean		hasTriggered = false;
Boolean		didBounceOffFence = false;
float		fps = gFramesPerSecondFrac;

	if (deltaBounce > 0.0f)									// make sure Brian entered a (-) bounce value!
		deltaBounce = -deltaBounce;

	theNode->MPlatform = nil;									// assume not on MPlatform

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// remember if was on ground or not
		previouslyOnGround = true;
	else
		previouslyOnGround = false;

	theNode->StatusBits &= ~STATUS_BIT_ONGROUND;			// assume not on anything now

	gNumCollisions = oldNumCollisions = 0;
	gTotalSides = 0;

again:
	originalX = gCoord.x;									// remember starting coords
	originalY = gCoord.y;
	originalZ = gCoord.z;

	numSolidHits = 0;

	CalcObjectBoxFromGlobal(theNode);						// calc current collision box

			/**************************/
			/* GET THE COLLISION LIST */
			/**************************/

	CollisionDetect(theNode,cType, gNumCollisions);			// get collision info

	totalSides = 0;
	maxOffsetX = maxOffsetZ = maxOffsetY = -10000;
	offXSign = offYSign = offZSign = 0;

			/* GET BASE BOX INFO */

	if (theNode->NumCollisionBoxes == 0)					// it's gotta have a collision box
		return(0);
	boxList 	= theNode->CollisionBoxes;
//	bottomSide	= boxList->bottom;

			/*************************************/
			/* SCAN THRU ALL RETURNED COLLISIONS */
			/*************************************/

	for (int i = oldNumCollisions; i < gNumCollisions; i++)	// handle all collisions
	{

		totalSides |= gCollisionList[i].sides;				// keep sides info
		base 		= gCollisionList[i].baseBox;			// get collision box index for base & target
		target 		= gCollisionList[i].targetBox;
		targetObj 	= gCollisionList[i].objectPtr;			// get ptr to target objnode

		baseBoxPtr 	= boxList + base;						// calc ptrs to base & target collision boxes
		if (targetObj)
		{
			targetBoxPtr = targetObj->CollisionBoxes;
			targetBoxPtr += target;
		}

					/********************************/
					/* HANDLE OBJECT COLLISIONS 	*/
					/********************************/

		if (gCollisionList[i].type == COLLISION_TYPE_OBJ)
		{
				/* SEE IF THIS OBJECT HAS SINCE BECOME INVALID */

			uint32_t	targetCType = targetObj->CType;						// get ctype of hit obj
			if (targetCType == INVALID_NODE_FLAG)
				continue;

						/* HANDLE TRIGGERS */

			if (((targetCType & CTYPE_TRIGGER) && (cType & CTYPE_TRIGGER)) ||	// target must be trigger and we must have been looking for them as well
				((targetCType & CTYPE_TRIGGER2) && (cType & CTYPE_TRIGGER2)))
	  		{
 				if (!HandleTrigger(targetObj,theNode,gCollisionList[i].sides))	// returns false if handle as non-solid trigger
					gCollisionList[i].sides = 0;

				numSolidHits++;

				maxOffsetX = gCoord.x - originalX;								// see if trigger caused a move
				if (maxOffsetX < 0.0f)
				{
					maxOffsetX = -maxOffsetX;
					offXSign = -1;
				}
				else
				if (maxOffsetX > 0.0f)
					offXSign = 1;

				maxOffsetZ = gCoord.z - originalZ;
				if (maxOffsetZ < 0.0f)
				{
					maxOffsetZ = -maxOffsetZ;
					offZSign = -1;
				}
				else
				if (maxOffsetZ > 0.0f)
					offZSign = 1;

				hasTriggered = true;									// dont allow multi-pass collision once there is a trigger (to avoid multiple hits on the same trigger)
			}


				/* CHECK FOR MPLATFORM */

			if (targetCType & CTYPE_MPLATFORM)
			{
				if (gCollisionList[i].sides & SIDE_BITS_BOTTOM)				// only if bottom hit it
				{
					theNode->MPlatform = gCollisionList[i].objectPtr;
					hitMPlatform = true;
				}
			}


					/*******************/
					/* DO SOLID FIXING */
					/*******************/

			if (gCollisionList[i].sides & ALL_SOLID_SIDES)						// see if object with any solidness
			{
				numSolidHits++;

				if (targetObj->CType & CTYPE_IMPENETRABLE)							// if this object is impenetrable, then throw out any other collision offsets
				{
					hitImpenetrable = true;
					maxOffsetX = maxOffsetZ = maxOffsetY = -10000;
					offXSign = offYSign = offZSign = 0;
				}

				if (gCollisionList[i].sides & SIDE_BITS_BACK)						// SEE IF BACK HIT
				{
					offset = (targetBoxPtr->front - baseBoxPtr->back)+1;			// see how far over it went
					if (offset > maxOffsetZ)
					{
						maxOffsetZ = offset;
						offZSign = 1;
					}
					gDelta.z *= deltaBounce;
				}
				else
				if (gCollisionList[i].sides & SIDE_BITS_FRONT)						// SEE IF FRONT HIT
				{
					offset = (baseBoxPtr->front - targetBoxPtr->back)+1;			// see how far over it went
					if (offset > maxOffsetZ)
					{
						maxOffsetZ = offset;
						offZSign = -1;
					}
					gDelta.z *= deltaBounce;
				}

				if (gCollisionList[i].sides & SIDE_BITS_LEFT)						// SEE IF HIT LEFT
				{
					offset = (targetBoxPtr->right - baseBoxPtr->left)+1;			// see how far over it went
					if (offset > maxOffsetX)
					{
						maxOffsetX = offset;
						offXSign = 1;
					}
					gDelta.x *= deltaBounce;
				}
				else
				if (gCollisionList[i].sides & SIDE_BITS_RIGHT)						// SEE IF HIT RIGHT
				{
					offset = (baseBoxPtr->right - targetBoxPtr->left)+1;			// see how far over it went
					if (offset > maxOffsetX)
					{
						maxOffsetX = offset;
						offXSign = -1;
					}
					gDelta.x *= deltaBounce;
				}

				if (gCollisionList[i].sides & SIDE_BITS_BOTTOM)						// SEE IF HIT BOTTOM
				{
					offset = (targetBoxPtr->top - baseBoxPtr->bottom);				// see how far over it went

					// In order to prevent falling through the object, push our collision box upward
					// by some epsilon distance so the boxes aren't perfectly flush.
					// The epsilon distance must not be too large, because that would cause too much
					// vertical jitter that our downward momentum won't be able to erase.
					offset += fps;

					if (offset > maxOffsetY)
					{
						maxOffsetY = offset;
						offYSign = 1;
					}

					gDelta.y = -100;												// keep some downward momentum!!
				}
				else
				if (gCollisionList[i].sides & SIDE_BITS_TOP)						// SEE IF HIT TOP
				{
					offset = (baseBoxPtr->top - targetBoxPtr->bottom)+1;			// see how far over it went
					if (offset > maxOffsetY)
					{
						maxOffsetY = offset;
						offYSign = -1;
					}
					gDelta.y =0;
				}
			}
		}


		if (hitImpenetrable)						// if that was impenetrable, then we dont need to check other collisions
		{
			break;
		}
	}

		/* IF THERE WAS A SOLID HIT, THEN WE NEED TO UPDATE AND TRY AGAIN */

	if (numSolidHits > 0)
	{
				/* ADJUST MAX AMOUNTS */

		gCoord.x = originalX + (maxOffsetX * offXSign);
		gCoord.z = originalZ + (maxOffsetZ * offZSign);
		gCoord.y = originalY + (maxOffsetY * offYSign);			// y is special - we do some additional rouding to avoid the jitter problem


				/* SEE IF NEED TO SET GROUND FLAG */

		if (totalSides & SIDE_BITS_BOTTOM)
		{
			if (!previouslyOnGround)							// if not already on ground, then add some friction upon landing
			{
				if (hitMPlatform)								// special case landing on moving platforms - stop deltas
				{
					gDelta.x =
					gDelta.z = 0.0f;
				}
				else
				{
					gDelta.x *= .3f;
					gDelta.z *= .3f;
				}
			}
			theNode->StatusBits |= STATUS_BIT_ONGROUND;
		}


				/* SEE IF DO ANOTHER PASS */

		numPasses++;
		if ((numPasses < 3) && (!hitImpenetrable) && (!hasTriggered))	// see if can do another pass and havnt hit anything impenetrable
			goto again;
	}


				/*************************/
				/* CHECK FENCE COLLISION */
				/*************************/

	if (cType & CTYPE_FENCE)
	{
		if (DoFenceCollision(theNode))
		{
			didBounceOffFence = true;

			numPasses++;
			if (numPasses < 3)
				goto again;
		}
	}

			/******************************************/
			/* SEE IF DO AUTOMATIC TERRAIN GROUND HIT */
			/******************************************/

	if (cType & CTYPE_TERRAIN)
	{
		float	y = GetTerrainY(gCoord.x, gCoord.z);

		if (boxList[0].bottom < y)								// see if bottom is under ground
		{
			gCoord.y += y - boxList[0].bottom;

			if (gDelta.y < 0.0f)								// if was going down then bounce y
				gDelta.y *= deltaBounce;

			theNode->StatusBits |= STATUS_BIT_ONGROUND;

			totalSides |= SIDE_BITS_BOTTOM;
		}
	}

			/* SEE IF DO WATER COLLISION TEST */

	if (cType & CTYPE_WATER)
	{
		int	patchNum;

		DoWaterCollisionDetect(theNode, gCoord.x, gCoord.y, gCoord.z, &patchNum);
	}


	if (didBounceOffFence)
		totalSides |= SIDE_BITS_FENCE;

	return(totalSides);
}



#pragma mark ------------------ POINT COLLISION ----------------------

/****************** IS POINT IN POLY ****************************/
/*
 * Quadrants:
 *    1 | 0
 *    -----
 *    2 | 3
 */
//
//	INPUT:	pt_x,pt_y	:	point x,y coords
//			cnt			:	# points in poly
//			polypts		:	ptr to array of 2D points
//

Boolean IsPointInPoly2D(float pt_x, float pt_y, Byte numVerts, const OGLPoint2D* polypts)
{
Byte 		oldquad,newquad;
float 		thispt_x,thispt_y,lastpt_x,lastpt_y;
signed char	wind;										// current winding number
Byte		i;

			/************************/
			/* INIT STARTING VALUES */
			/************************/

	wind = 0;
    lastpt_x = polypts[numVerts-1].x;  					// get last point's coords
    lastpt_y = polypts[numVerts-1].y;

	if (lastpt_x < pt_x)								// calc quadrant of the last point
	{
    	if (lastpt_y < pt_y)
    		oldquad = 2;
 		else
 			oldquad = 1;
 	}
 	else
    {
    	if (lastpt_y < pt_y)
    		oldquad = 3;
 		else
 			oldquad = 0;
	}


			/***************************/
			/* WIND THROUGH ALL POINTS */
			/***************************/

    for (i=0; i<numVerts; i++)
    {
   			/* GET THIS POINT INFO */

		thispt_x = polypts[i].x;						// get this point's coords
		thispt_y = polypts[i].y;

		if (thispt_x < pt_x)							// calc quadrant of this point
		{
	    	if (thispt_y < pt_y)
	    		newquad = 2;
	 		else
	 			newquad = 1;
	 	}
	 	else
	    {
	    	if (thispt_y < pt_y)
	    		newquad = 3;
	 		else
	 			newquad = 0;
		}

				/* SEE IF QUADRANT CHANGED */

        if (oldquad != newquad)
        {
			if (((oldquad+1)&3) == newquad)				// see if advanced
            	wind++;
			else
        	if (((newquad+1)&3) == oldquad)				// see if backed up
				wind--;
    		else
			{
				float	a,b;

             		/* upper left to lower right, or upper right to lower left.
             		   Determine direction of winding  by intersection with x==0. */

    			a = (lastpt_y - thispt_y) * (pt_x - lastpt_x);
                b = lastpt_x - thispt_x;
                a += lastpt_y * b;
                b *= pt_y;

				if (a > b)
                	wind += 2;
 				else
                	wind -= 2;
    		}
  		}

  				/* MOVE TO NEXT POINT */

   		lastpt_x = thispt_x;
   		lastpt_y = thispt_y;
   		oldquad = newquad;
	}


	return(wind); 										// non zero means point in poly
}





/****************** IS POINT IN TRIANGLE ****************************/
/*
 * Quadrants:
 *    1 | 0
 *    -----
 *    2 | 3
 */
//
//	INPUT:	pt_x,pt_y	:	point x,y coords
//			cnt			:	# points in poly
//			polypts		:	ptr to array of 2D points
//

Boolean IsPointInTriangle(float pt_x, float pt_y, float x0, float y0, float x1, float y1, float x2, float y2)
{
Byte 		oldquad,newquad;
float		m;
signed char	wind;										// current winding number

			/*********************/
			/* DO TRIVIAL REJECT */
			/*********************/

	m = x0;												// see if to left of triangle
	if (x1 < m)
		m = x1;
	if (x2 < m)
		m = x2;
	if (pt_x < m)
		return(false);

	m = x0;												// see if to right of triangle
	if (x1 > m)
		m = x1;
	if (x2 > m)
		m = x2;
	if (pt_x > m)
		return(false);

	m = y0;												// see if to back of triangle
	if (y1 < m)
		m = y1;
	if (y2 < m)
		m = y2;
	if (pt_y < m)
		return(false);

	m = y0;												// see if to front of triangle
	if (y1 > m)
		m = y1;
	if (y2 > m)
		m = y2;
	if (pt_y > m)
		return(false);


			/*******************/
			/* DO WINDING TEST */
			/*******************/

		/* INIT STARTING VALUES */


	if (x2 < pt_x)								// calc quadrant of the last point
	{
    	if (y2 < pt_y)
    		oldquad = 2;
 		else
 			oldquad = 1;
 	}
 	else
    {
    	if (y2 < pt_y)
    		oldquad = 3;
 		else
 			oldquad = 0;
	}


			/***************************/
			/* WIND THROUGH ALL POINTS */
			/***************************/

	wind = 0;


//=============================================

	if (x0 < pt_x)									// calc quadrant of this point
	{
    	if (y0 < pt_y)
    		newquad = 2;
 		else
 			newquad = 1;
 	}
 	else
    {
    	if (y0 < pt_y)
    		newquad = 3;
 		else
 			newquad = 0;
	}

			/* SEE IF QUADRANT CHANGED */

    if (oldquad != newquad)
    {
		if (((oldquad+1)&3) == newquad)				// see if advanced
        	wind++;
		else
    	if (((newquad+1)&3) == oldquad)				// see if backed up
			wind--;
		else
		{
			float	a,b;

         		/* upper left to lower right, or upper right to lower left.
         		   Determine direction of winding  by intersection with x==0. */

			a = (y2 - y0) * (pt_x - x2);
            b = x2 - x0;
            a += y2 * b;
            b *= pt_y;

			if (a > b)
            	wind += 2;
			else
            	wind -= 2;
		}
	}

	oldquad = newquad;

//=============================================

	if (x1 < pt_x)							// calc quadrant of this point
	{
    	if (y1 < pt_y)
    		newquad = 2;
 		else
 			newquad = 1;
 	}
 	else
    {
    	if (y1 < pt_y)
    		newquad = 3;
 		else
 			newquad = 0;
	}

			/* SEE IF QUADRANT CHANGED */

    if (oldquad != newquad)
    {
		if (((oldquad+1)&3) == newquad)				// see if advanced
        	wind++;
		else
    	if (((newquad+1)&3) == oldquad)				// see if backed up
			wind--;
		else
		{
			float	a,b;

         		/* upper left to lower right, or upper right to lower left.
         		   Determine direction of winding  by intersection with x==0. */

			a = (y0 - y1) * (pt_x - x0);
            b = x0 - x1;
            a += y0 * b;
            b *= pt_y;

			if (a > b)
            	wind += 2;
			else
            	wind -= 2;
		}
	}

	oldquad = newquad;

//=============================================

	if (x2 < pt_x)							// calc quadrant of this point
	{
    	if (y2 < pt_y)
    		newquad = 2;
 		else
 			newquad = 1;
 	}
 	else
    {
    	if (y2 < pt_y)
    		newquad = 3;
 		else
 			newquad = 0;
	}

			/* SEE IF QUADRANT CHANGED */

    if (oldquad != newquad)
    {
		if (((oldquad+1)&3) == newquad)				// see if advanced
        	wind++;
		else
    	if (((newquad+1)&3) == oldquad)				// see if backed up
			wind--;
		else
		{
			float	a,b;

         		/* upper left to lower right, or upper right to lower left.
         		   Determine direction of winding  by intersection with x==0. */

			a = (y1 - y2) * (pt_x - x1);
            b = x1 - x2;
            a += y1 * b;
            b *= pt_y;

			if (a > b)
            	wind += 2;
			else
            	wind -= 2;
		}
	}

	return(wind); 										// non zero means point in poly
}






/******************** DO SIMPLE POINT COLLISION *********************************/
//
// INPUT:  except == objNode to skip
//
// OUTPUT: # collisions detected
//

int DoSimplePointCollision(const OGLPoint3D* thePoint, uint32_t cType, const ObjNode* except)
{
	gNumCollisions = 0;

	ObjNode* thisNode = gFirstNodePtr;							// start on 1st node

	do
	{
		if (thisNode == except)									// see if skip this one
			goto next;

		if (!(thisNode->CType & cType))							// see if we want to check this Type
			goto next;

		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)		// don't collide against these
			goto next;

		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;


				/* GET BOX INFO FOR THIS NODE */

		int targetNumBoxes = thisNode->NumCollisionBoxes;		// if target has no boxes, then skip
		if (targetNumBoxes == 0)
			goto next;
		
		const CollisionBoxType* targetBoxList = thisNode->CollisionBoxes;


			/***************************************/
			/* CHECK POINT AGAINST EACH TARGET BOX */
			/***************************************/

		for (int target = 0; target < targetNumBoxes; target++)
		{
					/* DO RECTANGLE INTERSECTION */

			if (BoxContainsPoint(&targetBoxList[target], thePoint))
			{
					/* THERE HAS BEEN A COLLISION */

				gCollisionList[gNumCollisions].targetBox = target;
				gCollisionList[gNumCollisions].type = COLLISION_TYPE_OBJ;
				gCollisionList[gNumCollisions].objectPtr = thisNode;
				gNumCollisions++;
			}
		}

next:
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);

	return(gNumCollisions);
}


/******************** DO SIMPLE BOX COLLISION *********************************/
//
// OUTPUT: # collisions detected
//

int DoSimpleBoxCollision(float top, float bottom, float left, float right,
						float front, float back, uint32_t cType)
{
	const CollisionBoxType box = { .top = top, .bottom = bottom, .left = left, .right = right, .front = front, .back = back };

	gNumCollisions = 0;

	ObjNode* thisNode = gFirstNodePtr;							// start on 1st node

	do
	{
		if (!(thisNode->CType & cType))							// see if we want to check this Type
			goto next;

		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)		// don't collide against these
			goto next;

		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;


				/* GET BOX INFO FOR THIS NODE */

		int targetNumBoxes = thisNode->NumCollisionBoxes;		// if target has no boxes, then skip
		if (targetNumBoxes == 0)
			goto next;

		const CollisionBoxType* targetBoxList = thisNode->CollisionBoxes;


			/*********************************/
			/* CHECK AGAINST EACH TARGET BOX */
			/*********************************/

		for (int target = 0; target < targetNumBoxes; target++)
		{
					/* DO RECTANGLE INTERSECTION */

			if (IntersectBoxes(&box, &targetBoxList[target]))
			{
					/* THERE HAS BEEN A COLLISION */

				gCollisionList[gNumCollisions].targetBox = target;
				gCollisionList[gNumCollisions].type = COLLISION_TYPE_OBJ;
				gCollisionList[gNumCollisions].objectPtr = thisNode;
				gNumCollisions++;
			}
		}

next:
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);

	return(gNumCollisions);
}


/******************** DO SIMPLE BOX COLLISION AGAINST PLAYER *********************************/

Boolean DoSimpleBoxCollisionAgainstPlayer(float top, float bottom, float left, float right,
										float front, float back)
{
	if (gPlayerIsDead)									// if dead then blown up and can't be hit
		return(false);


			/* GET BOX INFO FOR THIS NODE */

	int targetNumBoxes = gPlayerInfo.objNode->NumCollisionBoxes;			// if target has no boxes, then skip
	if (targetNumBoxes == 0)
		return(false);

	const CollisionBoxType* targetBoxList = gPlayerInfo.objNode->CollisionBoxes;

	const CollisionBoxType box = { .top = top, .bottom = bottom, .left = left, .right = right, .front = front, .back = back };


		/***************************************/
		/* CHECK POINT AGAINST EACH TARGET BOX */
		/***************************************/

	for (int target = 0; target < targetNumBoxes; target++)
	{
				/* DO RECTANGLE INTERSECTION */

		if (IntersectBoxes(&box, &targetBoxList[target]))
			return true;
	}

	return false;
}



/******************** DO SIMPLE POINT COLLISION AGAINST PLAYER *********************************/

Boolean DoSimplePointCollisionAgainstPlayer(const OGLPoint3D* thePoint)
{
	if (gPlayerIsDead)									// if dead then blown up and can't be hit
		return(false);

			/* GET BOX INFO FOR THIS NODE */

	int targetNumBoxes = gPlayerInfo.objNode->NumCollisionBoxes;		// if target has no boxes, then skip
	if (targetNumBoxes == 0)
		return(false);
	
	const CollisionBoxType* targetBoxList = gPlayerInfo.objNode->CollisionBoxes;


		/***************************************/
		/* CHECK POINT AGAINST EACH TARGET BOX */
		/***************************************/

	for (int target = 0; target < targetNumBoxes; target++)
	{
				/* DO RECTANGLE INTERSECTION */

		if (BoxContainsPoint(&targetBoxList[target], thePoint))
		{
			return true;
		}
	}

	return false;
}

/******************** DO SIMPLE BOX COLLISION AGAINST OBJECT *********************************/

Boolean DoSimpleBoxCollisionAgainstObject(float top, float bottom, float left, float right,
										float front, float back, const ObjNode* targetNode)
{
			/* GET BOX INFO FOR THIS NODE */

	int targetNumBoxes = targetNode->NumCollisionBoxes;			// if target has no boxes, then skip
	if (targetNumBoxes == 0)
		return(false);

	const CollisionBoxType* targetBoxList = targetNode->CollisionBoxes;

	const CollisionBoxType box = { .top = top, .bottom = bottom, .left = left, .right = right, .front = front, .back = back };

		/***************************************/
		/* CHECK POINT AGAINST EACH TARGET BOX */
		/***************************************/

	for (int target = 0; target < targetNumBoxes; target++)
	{
				/* DO RECTANGLE INTERSECTION */

		if (IntersectBoxes(&box, &targetBoxList[target]))
		{
			return true;
		}
	}

	return(false);
}



/************************ FIND HIGHEST COLLISION AT XZ *******************************/
//
// Given the XY input, this returns the highest Y coordinate of any collision
// box here.
//

float FindHighestCollisionAtXZ(float inX, float inZ, uint32_t cType)
{
const ObjNode* thisNode;
OGLPoint3D topPoint = { .x = inX, .y = -10000000, .z = inZ };

	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (!(thisNode->CType & cType))							// matching ctype
			goto next;

		if (!(thisNode->CBits & CBITS_TOP))						// only top solid objects
			goto next;


				/* GET BOX INFO FOR THIS NODE */

		int targetNumBoxes = thisNode->NumCollisionBoxes;		// if target has no boxes, then skip
		if (targetNumBoxes == 0)
			goto next;
		
		const CollisionBoxType* targetBoxList = thisNode->CollisionBoxes;


			/***************************************/
			/* CHECK POINT AGAINST EACH TARGET BOX */
			/***************************************/

		for (int target = 0; target < targetNumBoxes; target++)
		{
			if (targetBoxList[target].top < topPoint.y)			// check top
				continue;

					/* DO RECTANGLE INTERSECTION */

			if (BoxContainsPoint_XZ(&targetBoxList[target], &topPoint))
			{
				topPoint.y = targetBoxList[target].top;			// save as highest Y
			}
		}

next:
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);

			/*********************/
			/* NOW CHECK TERRAIN */
			/*********************/

	if (cType & CTYPE_TERRAIN)
	{
		float	ty = GetTerrainY(topPoint.x, topPoint.z);

		if (ty > topPoint.y)
			topPoint.y = ty;
	}

			/*******************/
			/* NOW CHECK WATER */
			/*******************/

	if (cType & CTYPE_WATER)
	{
		float	wy;

		if (GetWaterY(topPoint.x, topPoint.z, &wy))
		{
			if (wy > topPoint.y)
				topPoint.y = wy;
		}
	}


	return topPoint.y;
}


#pragma mark -




/******************** SEE IF LINE SEGMENT HITS ANYTHING **************************/

Boolean SeeIfLineSegmentHitsAnything(const OGLPoint3D* endPoint1, const OGLPoint3D* endPoint2, const ObjNode* except, uint32_t ctype)
{
const ObjNode	*thisNode;
OGLPoint2D	p1,p2,crossBeamP1,crossBeamP2;
short			targetNumBoxes;
float	ix,iz,iy;

			/* SEE IF HIT FENCE */

	if (ctype & CTYPE_FENCE)
	{
		if (SeeIfLineSegmentHitsFence(endPoint1, endPoint2, nil, nil, nil))
			return(true);
	}

			/***************************/
			/* SEE IF HIT ANY OBJNODES */
			/***************************/

	p1.x = endPoint1->x;	p1.y = endPoint1->z;				// get x/z of segment endpoints
	p2.x = endPoint2->x;	p2.y = endPoint2->z;


	thisNode = gFirstNodePtr;									// start on 1st node

	do
	{
		if (thisNode == except)									// see if skip this one
			goto next;

		if (!(thisNode->CType & ctype))							// see if we want to check this Type
			goto next;

		if (thisNode->Slot >= SLOT_OF_DUMB)						// see if reach end of usable list
			break;

		if (thisNode->StatusBits & STATUS_BIT_NOCOLLISION)		// don't collide against these
			goto next;

		if (!thisNode->CBits)									// see if this obj doesn't need collisioning
			goto next;


				/* GET BOX INFO FOR THIS NODE */

		targetNumBoxes = thisNode->NumCollisionBoxes;			// if target has no boxes, then skip
		if (targetNumBoxes == 0)
			goto next;

		const CollisionBoxType* targetBox = &thisNode->CollisionBoxes[0];


				/* CREATE SEGMENT FROM CROSSBEAM */

		crossBeamP1.x = targetBox->left;
		crossBeamP1.y = targetBox->back;

		crossBeamP2.x = targetBox->right;
		crossBeamP2.y = targetBox->front;


			/* SEE IF INPUT SEGMENT INTERSECTS THE CROSSBEAM SEGMENT */

		if (IntersectLineSegments(p1.x, p1.y, p2.x, p2.y,
			                     crossBeamP1.x, crossBeamP1.y, crossBeamP2.x, crossBeamP2.y,
	                             &ix, &iz))
	  	{
			float	dy = endPoint2->y - endPoint1->y;			// get dy of line segment

			float	d1 = CalcDistance(p1.x, p1.y, p2.x, p2.y);
			float	d2 = CalcDistance(p1.x, p1.y, ix, iz);

			float	ratio = d2/d1;

			iy = endPoint1->y + (dy * ratio);					// calc intersect y coord

			if ((iy <= targetBox->top) &&					// if below top & above bottom, then HIT
				(iy >= targetBox->bottom))
			{
				return(true);
			}
	  	}


next:
		thisNode = thisNode->NextNode;							// next target node
	}while(thisNode != nil);


	return(false);
}



/******************** SEE IF LINE SEGMENT HITS PLAYER **************************/

Boolean SeeIfLineSegmentHitsPlayer(const OGLPoint3D* endPoint1, const OGLPoint3D* endPoint2)
{
const ObjNode	*player = gPlayerInfo.objNode;
OGLPoint2D	p1,p2,crossBeamP1,crossBeamP2;
const CollisionBoxType *collisionBox;
float	ix,iz,iy;

	if (gPlayerIsDead)											// if player dead/gone then cant hit
		return(false);

	p1.x = endPoint1->x;	p1.y = endPoint1->z;				// get x/z of segment endpoints
	p2.x = endPoint2->x;	p2.y = endPoint2->z;


			/* CREATE SEGMENT FROM CROSSBEAM */

	collisionBox = &player->CollisionBoxes[0];

	crossBeamP1.x = collisionBox->left;
	crossBeamP1.y = collisionBox->back;

	crossBeamP2.x = collisionBox->right;
	crossBeamP2.y = collisionBox->front;


		/* SEE IF INPUT SEGMENT INTERSECTS THE CROSSBEAM SEGMENT */

	if (IntersectLineSegments(p1.x, p1.y, p2.x, p2.y,
		                     crossBeamP1.x, crossBeamP1.y, crossBeamP2.x, crossBeamP2.y,
                             &ix, &iz))
  	{
		float	dy = endPoint2->y - endPoint1->y;			// get dy of line segment

		float	d1 = CalcDistance(p1.x, p1.y, p2.x, p2.y);
		float	d2 = CalcDistance(p1.x, p1.y, ix, iz);

		float	ratio = d2/d1;

		iy = endPoint1->y + (dy * ratio);					// calc intersect y coord

		if ((iy <= collisionBox->top) &&					// if below top & above bottom, then HIT
			(iy >= collisionBox->bottom))
		{
			return(true);
		}
  	}


	return(false);
}

