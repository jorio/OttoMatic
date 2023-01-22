/*********************************/
/*    OBJECT MANAGER 		     */
/* (c)2001 Pangea Software  	*/
/* By Brian Greenstone      	 */
/*********************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawShadow(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	SHADOW_Y_OFF	6.0f

/**********************/
/*     VARIABLES      */
/**********************/

#define	CheckForBlockers	Flag[0]


//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT COLLISION ------

/******************* ADD COLLISION BOX TO OBJECT ************************/

void AddCollisionBoxToObject(ObjNode *theNode, float top, float bottom, float left,
							 float right, float front, float back)
{
int	i;
CollisionBoxType *boxPtr = &theNode->CollisionBoxes[0];

	i = theNode->NumCollisionBoxes++;					// inc # collision boxes

	boxPtr[i].left 		= theNode->Coord.x + left;
	boxPtr[i].right 	= theNode->Coord.x + right;
	boxPtr[i].top 		= theNode->Coord.y + top;
	boxPtr[i].bottom 	= theNode->Coord.y + bottom;
	boxPtr[i].back 		= theNode->Coord.z + back;
	boxPtr[i].front 	= theNode->Coord.z + front;

	KeepOldCollisionBoxes(theNode);
}



/***************** CREATE COLLISION BOX FROM BOUNDING BOX ********************/

void CreateCollisionBoxFromBoundingBox(ObjNode *theNode, float tweakXZ, float tweakY)
{
OGLBoundingBox	*bBox;
float			sx,sy,sz;

	theNode->NumCollisionBoxes = 1;


	if (theNode->Genre == SKELETON_GENRE)
	{
		sx = sz = tweakXZ;
		sy = 1.0;
	}
	else
	{
		sx = theNode->Scale.x * tweakXZ;
		sy = theNode->Scale.y;
		sz = theNode->Scale.z * tweakXZ;
	}

			/* CONVERT TO COLLISON BOX */

	bBox =  &theNode->BBox;

	theNode->LeftOff 	= bBox->min.x * sx;
	theNode->RightOff 	= bBox->max.x * sx;
	theNode->FrontOff 	= bBox->max.z * sz;
	theNode->BackOff 	= bBox->min.z * sz;
	theNode->BottomOff 	= bBox->min.y * sy;
	theNode->TopOff 	= theNode->BottomOff + (bBox->max.y - bBox->min.y) * sy * tweakY;

	CalcObjectBoxFromNode(theNode);
	KeepOldCollisionBoxes(theNode);
}


/***************** CREATE COLLISION BOX FROM BOUNDING BOX: UPDATE ********************/
//
// Same as above, but does not touch the old boxes
//

void CreateCollisionBoxFromBoundingBox_Update(ObjNode *theNode, float tweakXZ, float tweakY)
{
OGLBoundingBox	*bBox;
float			sx,sy,sz;

	theNode->NumCollisionBoxes = 1;


	if (theNode->Genre == SKELETON_GENRE)
	{
		sx = sz = tweakXZ;
		sy = 1.0;
	}
	else
	{
		sx = theNode->Scale.x * tweakXZ;
		sy = theNode->Scale.y;
		sz = theNode->Scale.z * tweakXZ;
	}

			/* CONVERT TO COLLISON BOX */

	bBox =  &theNode->BBox;

	theNode->LeftOff 	= bBox->min.x * sx;
	theNode->RightOff 	= bBox->max.x * sx;
	theNode->FrontOff 	= bBox->max.z * sz;
	theNode->BackOff 	= bBox->min.z * sz;
	theNode->BottomOff 	= bBox->min.y * sy;
	theNode->TopOff 	= theNode->BottomOff + (bBox->max.y - bBox->min.y) * sy * tweakY;

	CalcObjectBoxFromNode(theNode);
}


/***************** CREATE COLLISION BOX FROM BOUNDING BOX MAXIMIZED ********************/
//
// Same as above except it expands the x/z box to the max of x or z so object can rotate without problems.
//

void CreateCollisionBoxFromBoundingBox_Maximized(ObjNode *theNode)
{
OGLBoundingBox	*bBox;
float			s;
float			maxSide,off;

	theNode->NumCollisionBoxes = 1;

			/* POINT TO BOUNDING BOX */

	bBox =  &theNode->BBox;


			/* DETERMINE LARGEST SIDE */

	if (theNode->Genre == SKELETON_GENRE)
		s = 1.0f;										// skeleton bboxes are already scaled correctly
	else
		s = theNode->Scale.x;

	maxSide = fabs(bBox->min.x * s);
	off = fabs(bBox->max.x * s);
	if (off > maxSide)
		maxSide = off;
	off = fabs(bBox->max.z * s);
	if (off > maxSide)
		maxSide = off;
	off = fabs(bBox->min.z) * s;
	if (off > maxSide)
		maxSide = off;


			/* CONVERT TO COLLISON BOX */

	theNode->LeftOff 	= -maxSide;
	theNode->RightOff 	= maxSide;
	theNode->FrontOff 	= maxSide;
	theNode->BackOff 	= -maxSide;
	theNode->TopOff 	= bBox->max.y * s;
	theNode->BottomOff 	= bBox->min.y * s;
	theNode->TopOff 	= theNode->BottomOff + (bBox->max.y - bBox->min.y) * s;

	CalcObjectBoxFromNode(theNode);
	KeepOldCollisionBoxes(theNode);
}



/***************** CREATE COLLISION BOX FROM BOUNDING BOX ROTATED ********************/

void CreateCollisionBoxFromBoundingBox_Rotated(ObjNode *theNode, float tweakXZ, float tweakY)
{
OGLBoundingBox	*bBox;
float			s;
OGLMatrix3x3	m;
OGLPoint2D		p,p1,p2,p3,p4;
float			minX,minZ,maxX,maxZ;

	theNode->NumCollisionBoxes = 1;

			/* POINT TO BOUNDING BOX */

	if (theNode->Genre == SKELETON_GENRE)
		bBox =  &gObjectGroupBBoxList[MODEL_GROUP_SKELETONBASE+theNode->Type][0];	// (skeleton type is actually the Group #, and only 1 model in there, so 0)
	else
		bBox =  &gObjectGroupBBoxList[theNode->Group][theNode->Type];				// get ptr to this model's local bounding box


			/* ROTATE BOUNDING BOX */

	OGLMatrix3x3_SetRotate(&m, -theNode->Rot.y);	// NOTE: we (-) the rotation cuz 2D y rot will be inverse of a 3D y rot (dont ask, just trust me)


	p.x = bBox->min.x;								// far left corner
	p.y = bBox->min.z;
	OGLPoint2D_Transform(&p, &m, &p1);
	p.x = bBox->max.x;								// far right corner
	p.y = bBox->min.z;
	OGLPoint2D_Transform(&p, &m, &p2);
	p.x = bBox->max.x;								// near right corner
	p.y = bBox->max.z;
	OGLPoint2D_Transform(&p, &m, &p3);
	p.x = bBox->min.x;								// near left corner
	p.y = bBox->max.z;
	OGLPoint2D_Transform(&p, &m, &p4);


			/* FIND MIN/MAX */

	minX = p1.x;
	if (p2.x < minX)
		minX = p2.x;
	if (p3.x < minX)
		minX = p3.x;
	if (p4.x < minX)
		minX = p4.x;

	maxX = p1.x;
	if (p2.x > maxX)
		maxX = p2.x;
	if (p3.x > maxX)
		maxX = p3.x;
	if (p4.x > maxX)
		maxX = p4.x;

	minZ = p1.y;
	if (p2.y < minZ)
		minZ = p2.y;
	if (p3.y < minZ)
		minZ = p3.y;
	if (p4.y < minZ)
		minZ = p4.y;

	maxZ = p1.y;
	if (p2.y > maxZ)
		maxZ = p2.y;
	if (p3.y > maxZ)
		maxZ = p3.y;
	if (p4.y > maxZ)
		maxZ = p4.y;


			/* CONVERT TO COLLISON BOX */

	s = theNode->Scale.x;

	theNode->LeftOff 	= minX * (s * tweakXZ);
	theNode->RightOff 	= maxX * (s * tweakXZ);
	theNode->FrontOff 	= maxZ * (s * tweakXZ);
	theNode->BackOff 	= minZ * (s * tweakXZ);

	theNode->BottomOff 	= bBox->min.y * s;
	theNode->TopOff 	= theNode->BottomOff + (bBox->max.y - bBox->min.y) * s * tweakY;

	CalcObjectBoxFromNode(theNode);
	KeepOldCollisionBoxes(theNode);
}




/*******************************  KEEP OLD COLLISION BOXES **************************/
//
// Also keeps old coordinate and stuff
//

void KeepOldCollisionBoxes(ObjNode *theNode)
{
	for (int i = 0; i < theNode->NumCollisionBoxes; i++)
	{
		theNode->OldCollisionBoxes[i] = theNode->CollisionBoxes[i];
	}


	theNode->OldCoord = theNode->Coord;			// remember coord also
}


/**************** CALC OBJECT BOX FROM NODE ******************/
//
// This does a simple 1 box calculation for basic objects.
//
// Box is calculated based on theNode's coords.
//

void CalcObjectBoxFromNode(ObjNode *theNode)
{
CollisionBoxType *boxPtr;

	if (theNode->NumCollisionBoxes > 0)
	{
		boxPtr = &theNode->CollisionBoxes[0];					// get ptr to 1st box (presumed only box)

		boxPtr->left 	= theNode->Coord.x + (float)theNode->LeftOff;
		boxPtr->right 	= theNode->Coord.x + (float)theNode->RightOff;
		boxPtr->top 	= theNode->Coord.y + (float)theNode->TopOff;
		boxPtr->bottom 	= theNode->Coord.y + (float)theNode->BottomOff;
		boxPtr->back 	= theNode->Coord.z + (float)theNode->BackOff;
		boxPtr->front 	= theNode->Coord.z + (float)theNode->FrontOff;
	}
}


/**************** CALC OBJECT BOX FROM GLOBAL ******************/
//
// This does a simple 1 box calculation for basic objects.
//
// Box is calculated based on gCoord
//

void CalcObjectBoxFromGlobal(ObjNode *theNode)
{
CollisionBoxType *boxPtr;

	if (theNode == nil)
		return;

	boxPtr = theNode->CollisionBoxes;					// get ptr to 1st box (presumed only box)

	boxPtr->left 	= gCoord.x  + (float)theNode->LeftOff;
	boxPtr->right 	= gCoord.x  + (float)theNode->RightOff;
	boxPtr->back 	= gCoord.z  + (float)theNode->BackOff;
	boxPtr->front 	= gCoord.z  + (float)theNode->FrontOff;
	boxPtr->top 	= gCoord.y  + (float)theNode->TopOff;
	boxPtr->bottom 	= gCoord.y  + (float)theNode->BottomOff;
}


/******************* SET OBJECT COLLISION BOUNDS **********************/
//
// Sets an object's collision offset/bounds.  Adjust accordingly for input rotation 0..3 (clockwise)
//

void SetObjectCollisionBounds(ObjNode *theNode, short top, short bottom, short left,
							 short right, short front, short back)
{

	theNode->NumCollisionBoxes = 1;					// 1 collision box
	theNode->TopOff 		= top;
	theNode->BottomOff 	= bottom;
	theNode->LeftOff 	= left;
	theNode->RightOff 	= right;
	theNode->FrontOff 	= front;
	theNode->BackOff 	= back;

	CalcObjectBoxFromNode(theNode);
	KeepOldCollisionBoxes(theNode);
}

/******************** CALC NEW TARGET OFFSETS **********************/

void CalcNewTargetOffsets(ObjNode *theNode, float scale)
{
	theNode->TargetOff.x = RandomFloat2() * scale;
	theNode->TargetOff.y = RandomFloat2() * scale;
}


//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT SHADOWS ------




/******************* ATTACH SHADOW TO OBJECT ************************/

ObjNode	*AttachShadowToObject(ObjNode *theNode, int shadowType, float scaleX, float scaleZ, Boolean checkBlockers)
{
ObjNode	*shadowObj;

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.coord 		= theNode->Coord;
	gNewObjectDefinition.coord.y 	+= SHADOW_Y_OFF;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOZWRITES|STATUS_BIT_NOLIGHTING|gAutoFadeStatusBits;

	if (theNode->Slot >= SLOT_OF_DUMB+1)					// shadow *must* be after parent!
		gNewObjectDefinition.slot 	= theNode->Slot+1;
	else
		gNewObjectDefinition.slot 	= SLOT_OF_DUMB+1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= scaleX;

	shadowObj = MakeNewObject(&gNewObjectDefinition);
	if (shadowObj == nil)
		return(nil);

	theNode->ShadowNode = shadowObj;

	shadowObj->SpecialF[0] = scaleX;							// need to remeber scales for update
	shadowObj->SpecialF[1] = scaleZ;

	shadowObj->CheckForBlockers = checkBlockers;

	shadowObj->CustomDrawFunction = DrawShadow;

	shadowObj->Kind = shadowType;							// remember the shadow type

	return(shadowObj);
}


/************************ UPDATE SHADOW *************************/

void UpdateShadow(ObjNode *theNode)
{
ObjNode *shadowNode,*thisNodePtr;
long	x,y,z;
float	dist,scaleX,scaleZ;

	if (theNode == nil)
		return;

	shadowNode = theNode->ShadowNode;
	if (shadowNode == nil)
		return;


	x = theNode->Coord.x;
	y = theNode->Coord.y + theNode->BottomOff;
	z = theNode->Coord.z;

	shadowNode->Coord = theNode->Coord;
	shadowNode->Rot.y = theNode->Rot.y;

		/****************************************************/
		/* SEE IF SHADOW IS ON BLOCKER OBJECT OR ON TERRAIN */
		/****************************************************/

	if (shadowNode->CheckForBlockers)
	{
		thisNodePtr = gFirstNodePtr;
		do
		{
			if (thisNodePtr->Slot >= SLOT_OF_DUMB)
				break;

			if (thisNodePtr->CType & CTYPE_BLOCKSHADOW)						// look for things which can block the shadow
			{
				if (thisNodePtr->NumCollisionBoxes)
				{
					if (y < thisNodePtr->CollisionBoxes[0].top-1)		// if bottom of owner is below top of blocker, then skip
						goto next;
					if (x < thisNodePtr->CollisionBoxes[0].left)
						goto next;
					if (x > thisNodePtr->CollisionBoxes[0].right)
						goto next;
					if (z > thisNodePtr->CollisionBoxes[0].front)
						goto next;
					if (z < thisNodePtr->CollisionBoxes[0].back)
						goto next;

						/* SHADOW IS ON OBJECT  */

					shadowNode->Coord.y = thisNodePtr->CollisionBoxes[0].top + SHADOW_Y_OFF;

					shadowNode->Scale.x = shadowNode->SpecialF[0];				// use preset scale
					shadowNode->Scale.z = shadowNode->SpecialF[1];
					UpdateObjectTransforms(shadowNode);
					return;

				}
			}
	next:
			thisNodePtr = (ObjNode *)thisNodePtr->NextNode;		// next node
		}
		while (thisNodePtr != nil);
	}

			/************************/
			/* SHADOW IS ON TERRAIN */
			/************************/

	RotateOnTerrain(shadowNode, SHADOW_Y_OFF, nil);							// set transform matrix

			/* CALC SCALE OF SHADOW */

	dist = (y - shadowNode->Coord.y) * (1.0f/2000.0f);					// as we go higher, shadow gets smaller
	if (dist < 0.0f)
		dist = 0;

	dist = 1.0f - dist;

	scaleX = dist * shadowNode->SpecialF[0];
	scaleZ = dist * shadowNode->SpecialF[1];

	if (scaleX < 0.0f)
		scaleX = 0;
	if (scaleZ < 0.0f)
		scaleZ = 0;

	shadowNode->Scale.x = scaleX;				// this scale wont get updated until next frame (RotateOnTerrain).
	shadowNode->Scale.z = scaleZ;
}


/******************* DRAW SHADOW ******************/

static void DrawShadow(ObjNode *theNode)
{
int	shadowType = theNode->Kind;


	OGL_PushState();

			/* SUBMIT THE MATRIX */

	glMultMatrixf(theNode->BaseTransformMatrix.value);


			/* SUBMIT SHADOW TEXTURE */

	gGlobalTransparency = theNode->ColorFilter.a;

	MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_Shadow_Circular+shadowType].materialObject);


			/* DRAW THE SHADOW */

	glDisable(GL_CULL_FACE);
	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex3f(-20, 0, 20);
	glTexCoord2f(1,1);	glVertex3f(20, 0, 20);
	glTexCoord2f(1,0);	glVertex3f(20, 0, -20);
	glTexCoord2f(0,0);	glVertex3f(-20, 0, -20);
	glEnd();
	glEnable(GL_CULL_FACE);

	OGL_PopState();
	gGlobalTransparency = 1.0;

}


//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- OBJECT CULLING ------


/**************** CULL TEST ALL OBJECTS *******************/

void CullTestAllObjects(void)
{
int			i;
ObjNode		*theNode;
float		m00,m01,m02,m03;
float		m10,m11,m12,m13;
float		m20,m21,m22,m23;
float		m30,m31,m32,m33;
float		minX,minY,minZ,maxX,maxY,maxZ;
OGLMatrix4x4		m;
OGLBoundingBox		*bBox;
float		minusHW;				// -hW
uint32_t	clipFlags;				// Clip in/out tests for point
uint32_t	clipCodeAND;			// Clip test for entire object
//uint32_t	clipCodeOR;				// Clip test for entire object
float		lX, lY, lZ;				// Local space co-ordinates
float		hX, hY, hZ, hW;			// Homogeneous co-ordinates


	theNode = gFirstNodePtr;														// get & verify 1st node
	if (theNode == nil)
		return;

					/* PROCESS EACH OBJECT */

	do
	{
		if (theNode->StatusBits & STATUS_BIT_ALWAYSCULL)
			goto try_cull;

		if (theNode->StatusBits & STATUS_BIT_HIDDEN)			// if hidden then skip
			goto next;

		if (theNode->StatusBits & STATUS_BIT_DONTCULL)			// see if dont want to use our culling
			goto draw_on;

try_cull:

	bBox = &theNode->BBox;
	if (bBox->isEmpty)											// skip culling if no bbox
		goto draw_on;


			/* CALCULATE THE LOCAL->FRUSTUM MATRIX FOR THIS OBJECT */

	if (theNode->Genre == SKELETON_GENRE)			// skeletons are already oriented, just need translation
	{
		OGLMatrix4x4_SetTranslate(&m, theNode->Coord.x, theNode->Coord.y, theNode->Coord.z);
		OGLMatrix4x4_Multiply(&m, &gWorldToFrustumMatrix, &m);
	}
	else											// non-skeletons need full transform
		OGLMatrix4x4_Multiply(&theNode->BaseTransformMatrix, &gWorldToFrustumMatrix, &m);

	m00 = m.value[M00];
	m01 = m.value[M01];
	m02 = m.value[M02];
	m03 = m.value[M03];
	m10 = m.value[M10];
	m11 = m.value[M11];
	m12 = m.value[M12];
	m13 = m.value[M13];
	m20 = m.value[M20];
	m21 = m.value[M21];
	m22 = m.value[M22];
	m23 = m.value[M23];
	m30 = m.value[M30];
	m31 = m.value[M31];
	m32 = m.value[M32];
	m33 = m.value[M33];


				/* TRANSFORM THE BOUNDING BOX */

	minX = bBox->min.x;													// load bbox into registers
	minY = bBox->min.y;
	minZ = bBox->min.z;
	maxX = bBox->max.x;
	maxY = bBox->max.y;
	maxZ = bBox->max.z;

	clipCodeAND = ~0u;
//	clipCodeOR 	= 0;

	for (i = 0; i < 8; i++)
	{
		switch (i)														// load current bbox corner in IX,IY,IZ
		{
			case	0:	lX = minX;	lY = minY;	lZ = minZ;	break;
			case	1:	lX = minX;	lY = minY;	lZ = maxZ;	break;
			case	2:	lX = minX;	lY = maxY;	lZ = minZ;	break;
			case	3:	lX = minX;	lY = maxY;	lZ = maxZ;	break;
			case	4:	lX = maxX;	lY = minY;	lZ = minZ;	break;
			case	5:	lX = maxX;	lY = minY;	lZ = maxZ;	break;
			case	6:	lX = maxX;	lY = maxY;	lZ = minZ;	break;
			default:	lX = maxX;	lY = maxY;	lZ = maxZ;
		}

		hW = lX * m30 + lY * m31 + lZ * m32 + m33;
		hY = lX * m10 + lY * m11 + lZ * m12 + m13;
		hZ = lX * m20 + lY * m21 + lZ * m22 + m23;
		hX = lX * m00 + lY * m01 + lZ * m02 + m03;

		minusHW = -hW;

				/* CHECK Y */

		if (hY < minusHW)
			clipFlags = 0x8;
		else
		if (hY > hW)
			clipFlags = 0x4;
		else
			clipFlags = 0;


				/* CHECK Z */

		if (hZ > hW)
			clipFlags |= 0x20;
		else
		if (hZ < 0.0f)
			clipFlags |= 0x10;


				/* CHECK X */

		if (hX < minusHW)
			clipFlags |= 0x2;
		else
		if (hX > hW)
			clipFlags |= 0x1;

		clipCodeAND &= clipFlags;
//		clipCodeOR |= clipFlags;
	}

	/****************************/
	/* SEE IF WAS CULLED OR NOT */
	/****************************/
	//
	//  We have one of 3 cases:
	//
	//	1. completely in bounds - no clipping
	//	2. completely out of bounds - no need to render
	//	3. some clipping is needed
	//

//	if (clipCodeOR == 0)														// check for case #1
//	{
//		theNode->StatusBits |= STATUS_BIT_NOCLIPTEST;
//	}


	if (clipCodeAND)															// check for case #2
	{
		theNode->StatusBits |= STATUS_BIT_ISCULLED;								// set cull bit
	}
	else
	{
draw_on:
		theNode->StatusBits &= ~STATUS_BIT_ISCULLED;							// clear cull bit
	}



				/* NEXT NODE */
next:
		theNode = theNode->NextNode;		// next node
	}
	while (theNode != nil);
}


//============================================================================================================
//============================================================================================================
//============================================================================================================

#pragma mark ----- MISC OBJECT FUNCTIONS ------


/************************ STOP OBJECT STREAM EFFECT *****************************/

void StopObjectStreamEffect(ObjNode *theNode)
{
	if (theNode->EffectChannel != -1)
	{
		StopAChannel(&theNode->EffectChannel);
	}
}

