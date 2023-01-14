//
// fences.h
//

#pragma once

typedef struct
{
	float	top,bottom,left,right;
}RectF;

typedef struct
{
	uint16_t		type;				// type of fence
	int				numNubs;			// # nubs in fence
	OGLPoint3D		*nubList;			// pointer to nub list	
	OGLBoundingBox	bBox;				// bounding box of fence area	
	OGLVector2D		*sectionVectors;	// for each section/span, this is the vector from nub(n) to nub(n+1)
	OGLVector2D		*sectionNormals;	// for each section/span, this is the perpendicular normal vector
}FenceDefType;


//============================================

void PrimeFences(void);
Boolean DoFenceCollision(ObjNode *theNode);
void DisposeFences(void);
Boolean SeeIfLineSegmentHitsFence(const OGLPoint3D *endPoint1, const OGLPoint3D *endPoint2, OGLPoint3D *intersect, Boolean *overTop, float *fenceTopY);
