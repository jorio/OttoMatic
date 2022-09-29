//
// 3DMath.h
//

#pragma once

#define OGLMath_RadiansToDegrees(x)	((float)((x) * 180.0f / PI))
#define OGLMath_DegreesToRadians(x)	((float)((x) * PI / 180.0f))

float OGLPoint3D_DistanceToPlane(const OGLPoint3D *point,const OGLPlaneEquation	*plane);
float OGLPoint2D_LineDistance(OGLPoint2D *point, float p1x, float p1y, float p2x, float p2y, float *t);


float CalcXAngleFromPointToPoint(float fromY, float fromZ, float toY, float toZ);
float	CalcYAngleFromPointToPoint(float oldRot, float fromX, float fromZ, float toX, float toZ);
float TurnObjectTowardTarget(ObjNode *theNode, const OGLPoint3D *from, float toX, float toZ, float turnSpeed, Boolean useOffsets);
float TurnObjectTowardTarget2D(ObjNode *theNode, float fromX, float fromY, float toX, float toY, float turnSpeed);
float TurnObjectTowardTargetOnX(ObjNode *theNode, const OGLPoint3D *from, const OGLPoint3D *to, float turnSpeed);
float TurnPointTowardPoint(float *rotY, OGLPoint3D *from, float toX, float toZ, float turnSpeed);
float TurnObjectTowardPlayer(ObjNode *theNode, const OGLPoint3D *from, float turnSpeed, float thresholdAngle, float *turnDirection);
void CalcPointOnObject(const ObjNode *theNode, const OGLPoint3D *inPt, OGLPoint3D *outPt);
Boolean IntersectionOfLineSegAndPlane(const OGLPlaneEquation *plane, float v1x, float v1y, float v1z,
								 float v2x, float v2y, float v2z, OGLPoint3D *outPoint);
void ApplyFrictionToDeltas(float f,OGLVector3D *d);

Boolean VectorsAreCloseEnough(const OGLVector3D *v1, const OGLVector3D *v2);
Boolean PointsAreCloseEnough(const OGLPoint3D *v1, const OGLPoint3D *v2);
float IntersectionOfYAndPlane_Func(float x, float z, const OGLPlaneEquation *p);
void OGLVector3D_Cross(const OGLVector3D *v1, const OGLVector3D	*v2, OGLVector3D *result);
void OGLMatrix4x4_Invert(const OGLMatrix4x4 *inMatrix, OGLMatrix4x4 *result);

extern	Boolean IsPointInTriangle3D(const OGLPoint3D *point3D,	const OGLPoint3D *trianglePoints, OGLVector3D *normal);
void OGLCreateFromToRotationMatrix(OGLMatrix4x4 *matrix4x4,	const OGLVector3D *v1, const OGLVector3D *v2);
void SetLookAtMatrix(OGLMatrix4x4 *m, const OGLVector3D *upVector, const OGLPoint3D *from, const OGLPoint3D *to);
void SetLookAtMatrixAndTranslate(OGLMatrix4x4 *m, const OGLVector3D *upVector, const OGLPoint3D *from, const OGLPoint3D *to);
void SetAlignmentMatrix(OGLMatrix4x4 *m, const OGLVector3D *aim);
void SetAlignmentMatrixWithZRot(OGLMatrix4x4 *m, const OGLVector3D *aim, float rotZ);

Boolean IntersectLineSegments(float x1, float y1, float x2, float y2,
		                     float x3, float y3, float x4, float y4,
                             float *x, float *y);

void CalcLineNormal2D(float p0x, float p0y, float p1x, float p1y,
					 float	px, float py, OGLVector2D *normal);
Boolean CalcRayNormal2D(const OGLVector2D *vec, float p0x, float p0y,
					 float	px, float py, OGLVector2D *normal);

void ReflectVector2D(OGLVector2D *theVector, const OGLVector2D *N, OGLVector2D *outVec);
void ReflectVector3D(const OGLVector3D *vec, OGLVector3D *N, OGLVector3D *out);

float CalcVectorLength(OGLVector3D *v);
float CalcVectorLength2D(const OGLVector2D *v);
void OGLPoint3D_Transform(const OGLPoint3D *point3D, const OGLMatrix4x4 *matrix4x4,
								OGLPoint3D *result);
void OGLMatrix4x4_SetScale(OGLMatrix4x4 *m, float x, float y, float z);
void OGLMatrix4x4_SetRotate_X(OGLMatrix4x4	*m, float angle);
void OGLMatrix4x4_SetRotate_Y(OGLMatrix4x4	*m, float angle);
void OGLMatrix4x4_SetRotate_Z(OGLMatrix4x4	*m, float angle);
void OGLMatrix4x4_SetRotateAboutPoint(OGLMatrix4x4 *matrix4x4, const OGLPoint3D	*origin,
									float xAngle, float yAngle, float zAngle);
void OGLMatrix4x4_GetFrustumToWindow(OGLMatrix4x4 *m);

void OGLMatrix4x4_Multiply(const OGLMatrix4x4	*mA, const OGLMatrix4x4 *mB, OGLMatrix4x4	*result);
void OGLMatrix4x4_SetRotate_XYZ(OGLMatrix4x4 *m, float rx, float ry, float rz);
void OGLMatrix3x3_SetRotate(OGLMatrix3x3 *m, double angle);
void OGLMatrix3x3_SetIdentity(OGLMatrix3x3 *m);
void OGLPoint2D_Transform(const OGLPoint2D *p, const OGLMatrix3x3 *m, OGLPoint2D *result);
float OGLVector3D_Dot(const OGLVector3D	*v1, const OGLVector3D	*v2);
float OGLVector2D_Dot(const OGLVector2D	*v1,  const OGLVector2D	*v2);
float OGLVector2D_Cross(const OGLVector2D *v1, const OGLVector2D *v2);
void OGLMatrix4x4_Transpose(const OGLMatrix4x4 *matrix4x4, OGLMatrix4x4 *result);
void OGLVector3D_Normalize(const OGLVector3D *vector3D, OGLVector3D	*result);
void OGLVector3D_Transform(const OGLVector3D *vector3D,	const OGLMatrix4x4	*matrix4x4, OGLVector3D *result);
void OGLMatrix4x4_SetRotateAboutAxis(OGLMatrix4x4	*m, const OGLVector3D	*axis, float angle);
void OGLPoint3D_CalcBoundingBox(const OGLPoint3D *points, int numPoints, OGLBoundingBox *bBox);

void OGLMatrix3x3_Multiply(const OGLMatrix3x3	*matrixA,
							const OGLMatrix3x3	*matrixB,
							OGLMatrix3x3		*result);

void OGLMatrix3x3_SetRotateAboutPoint(OGLMatrix3x3 *m, OGLPoint2D *origin, double angle);
void OGLVector2D_Transform(const OGLVector2D *vector2D, 
							const OGLMatrix3x3	*matrix3x3,
							OGLVector2D			*result);
void OGLMatrix4x4_SetTranslate(OGLMatrix4x4 *m, float x, float y, float z);
void OGLMatrix4x4_SetIdentity(OGLMatrix4x4 *m);
void OGLMatrix3x3_SetTranslate(OGLMatrix3x3 *m, float x, float y);

void OGLPoint3D_To4DTransformArray(const OGLPoint3D *inVertex, const OGLMatrix4x4  *matrix,
									OGLPoint4D *outVertex,  long numVertices);
void OGLPoint3D_TransformArray(const OGLPoint3D *inVertex, const OGLMatrix4x4  *matrix,
									OGLPoint3D *outVertex,  long numVertices);
void OGLPoint2D_TransformArray(const OGLPoint2D *inVertex, const OGLMatrix3x3  *matrix,
									OGLPoint2D *outVertex,  long numVertices);
void OGLVector3D_TransformArray(const OGLVector3D *inVectors, const OGLMatrix4x4 *m,
								 OGLVector3D *outVectors, int numVectors);


Boolean OGL_IsBBoxVisible(const OGLBoundingBox *bBox, OGLMatrix4x4	*localToWorld);
void OGLVector2D_Normalize(const OGLVector2D *vector2D, OGLVector2D	*result);
float OGLPoint3D_Distance(const OGLPoint3D *p1, const OGLPoint3D *p2);
float OGLPoint2D_Distance(const OGLPoint2D *p1, const OGLPoint2D *p2);
void ApplyFrictionToRotation(float f,OGLVector3D *d);
void ApplyFrictionToDeltasXZ(float f,OGLVector3D *d);

void OGLBoundingBox_Transform(OGLBoundingBox *inBox, OGLMatrix4x4 *m, OGLBoundingBox *outBox);


void OGL_SetGluPerspectiveMatrix(OGLMatrix4x4* m, float fov, float aspect, float hither, float yon);

void OGL_SetGluLookAtMatrix(OGLMatrix4x4* m, const OGLPoint3D* eye, const OGLPoint3D* target, const OGLVector3D* upDir);




static inline float OGLVector3D_RawDot(const OGLVector3D *v1, const OGLVector3D *v2)
{
	return (v1->x * v2->x) + (v1->y * v2->y) + (v1->z * v2->z);
}


static inline void OGLVector3D_RawCross(const OGLVector3D *v1, const OGLVector3D *v2, OGLVector3D *result)
{
	float rx = (v1->y * v2->z) - (v1->z * v2->y);
	float ry = (v1->z * v2->x) - (v1->x * v2->z);
	float rz = (v1->x * v2->y) - (v1->y * v2->x);

	result->x = rx;
	result->y = ry;
	result->z = rz;
}

static inline void OGLPoint3D_Add(const OGLPoint3D* v1, const OGLPoint3D* v2, OGLPoint3D* result)
{
	result->x = v1->x + v2->x;
	result->y = v1->y + v2->y;
	result->z = v1->z + v2->z;
}


/*********** INTERSECTION OF Y AND PLANE ********************/
//
// INPUT:	
//			x/z		:	xz coords of point
//			p		:	ptr to the plane
//
//
// *** IMPORTANT-->  This function does not check for divides by 0!! As such, there should be no
//					"vertical" polygons (polys with normal->y == 0).
//

#define IntersectionOfYAndPlane(_x, _z, _p)	(((_p)->constant - (((_p)->normal.x * _x) + ((_p)->normal.z * _z))) / (_p)->normal.y)


/***************** ANGLE TO VECTOR ******************/
//
// Returns a normalized 2D vector based on a radian angle
//
// NOTE: +rotation == counter clockwise!
//

static inline void AngleToVector(float angle, OGLVector2D *theVector)
{
	theVector->x = -sin(angle);
	theVector->y = -cos(angle);
}



/******************** FAST RECIPROCAL **************************/
//
//   y1 = y0 + y0*(1 - num*y0), where y0 = recip_est(num)
//

#define FastReciprocal(_num,_out)		\
{										\
	float	_y0;						\
										\
	_y0 = __fres(_num);					\
	_out = _y0 * (1 - _num*_y0) + _y0;	\
}


/***************** FAST VECTOR LENGTH 3D/2D *********************/

#define VectorLength3D(_r, _inX, _inY, _inZ)	_r = sqrtf((_inY*_inY) + (_inX*_inX) + (_inZ*_inZ));

#define VectorLength2D(_r, _inX, _inY)			_r = sqrtf((_inY)*(_inY) + (_inX)*(_inX));



/******************** FAST NORMALIZE VECTOR ***********************/
//
// My special version here will use the frsqrte opcode if available.
// It does Newton-Raphson refinement to get a good result.
//

static inline void FastNormalizeVector(float vx, float vy, float vz, OGLVector3D *outV)
{
float	temp;
	
	if ((fabs(vx) <= EPS) && (fabs(vy) <= EPS) && (fabs(vz) <= EPS))		// check for zero length vectors
	{
		outV->x = outV->y = outV->z = 0;
		return;
	}
	
	temp = vx * vx;
	temp += vy * vy;
	temp += vz * vz;
	
#if defined (__ppc__)	
{
	float	isqrt, temp1, temp2;		

	isqrt = __frsqrte (temp);					// isqrt = first approximation of 1/sqrt() 
	temp1 = temp * (float)(-.5);				// temp1 = -a / 2 		
	temp2 = isqrt * isqrt;						// temp2 = sqrt^2
	temp1 *= isqrt;								// temp1 = -a * sqrt / 2 
	isqrt *= (float)(3.0/2.0);					// isqrt = 3 * sqrt / 2 
	temp = isqrt + temp1 * temp2;				// isqrt = (3 * sqrt - a * sqrt^3) / 2 
}
#else
	temp = 1.0 / sqrt(temp);
#endif	

	outV->x = vx * temp;						// return results
	outV->y = vy * temp;
	outV->z = vz * temp;
}

/******************** FAST NORMALIZE VECTOR 2D ***********************/
//
// My special version here will use the frsqrte opcode if available.
// It does Newton-Raphson refinement to get a good result.
//

static inline void FastNormalizeVector2D(float vx, float vy, OGLVector2D *outV, Boolean errCheck)
{
	if (errCheck)
	{
		if ((fabs(vx) <= EPS) && (fabs(vy) <= EPS))			// check for zero length vectors
		{
			outV->x = outV->y = 0;
			return;
		}
	}


float	temp;
	
	
	temp = vx * vx;
	temp += vy * vy;
	
#if defined (__ppc__)
{
float	isqrt, temp1, temp2;		

	isqrt = __frsqrte (temp);					// isqrt = first approximation of 1/sqrt() 
	temp1 = temp * (float)(-.5);				// temp1 = -a / 2 		
	temp2 = isqrt * isqrt;						// temp2 = sqrt^2
	temp1 *= isqrt;								// temp1 = -a * sqrt / 2 
	isqrt *= (float)(3.0/2.0);					// isqrt = 3 * sqrt / 2 
	temp = isqrt + temp1 * temp2;				// isqrt = (3 * sqrt - a * sqrt^3) / 2 
}
#else
	temp = 1.0 / sqrt(temp);
#endif	
		
	outV->x = vx * temp;						// return results
	outV->y = vy * temp;
}



/***************** CALC FACE NORMAL *********************/
//
// Returns the normal vector off the face defined by 3 points.
//

static inline void CalcFaceNormal(const OGLPoint3D *p1, const OGLPoint3D *p2, const OGLPoint3D *p3, OGLVector3D *normal)
{
float		dx1,dx2,dy1,dy2,dz1,dz2;
float		x,y,z;

	dx1 = (p1->x - p3->x);
	dy1 = (p1->y - p3->y);
	dz1 = (p1->z - p3->z);
	dx2 = (p2->x - p3->x);
	dy2 = (p2->y - p3->y);
	dz2 = (p2->z - p3->z);


			/* DO CROSS PRODUCT */
			
	x =   dy1 * dz2 - dy2 * dz1;
	y =   dx2 * dz1 - dx1 * dz2;
	z =   dx1 * dy2 - dx2 * dy1;
			
			/* NORMALIZE IT */

	FastNormalizeVector(x,y, z, normal);
}

/***************** CALC FACE NORMAL: NOT NORMALIZED *********************/
//
// Returns the normal vector off the face defined by 3 points.
//

static inline void CalcFaceNormal_NotNormalized(const OGLPoint3D *p1, const OGLPoint3D *p2, const OGLPoint3D *p3, OGLVector3D *normal)
{
float		dx1,dx2,dy1,dy2,dz1,dz2;

	dx1 = (p1->x - p3->x);
	dy1 = (p1->y - p3->y);
	dz1 = (p1->z - p3->z);
	dx2 = (p2->x - p3->x);
	dy2 = (p2->y - p3->y);
	dz2 = (p2->z - p3->z);


			/* DO CROSS PRODUCT */
			
	normal->x =   dy1 * dz2 - dy2 * dz1;
	normal->y =   dx2 * dz1 - dx1 * dz2; 
	normal->z =   dx1 * dy2 - dx2 * dy1;			
}


/******************* CALC PLANE EQUATION OF TRIANGLE ********************/
//
// input points should be clockwise!
//

static inline void CalcPlaneEquationOfTriangle(OGLPlaneEquation *plane, const OGLPoint3D *p3, const OGLPoint3D *p2, const OGLPoint3D *p1)
{
float	pq_x,pq_y,pq_z;
float	pr_x,pr_y,pr_z;
float	p1x,p1y,p1z;
float	x,y,z;

	p1x = p1->x;									// get point #1
	p1y = p1->y;
	p1z = p1->z;

	pq_x = p1x - p2->x;								// calc vector pq
	pq_y = p1y - p2->y;
	pq_z = p1z - p2->z;

	pr_x = p1->x - p3->x;							// calc vector pr
	pr_y = p1->y - p3->y;
	pr_z = p1->z - p3->z;


			/* CALC CROSS PRODUCT FOR THE FACE'S NORMAL */
			
	x = (pq_y * pr_z) - (pq_z * pr_y);				// cross product of edge vectors = face normal
	y = ((pq_z * pr_x) - (pq_x * pr_z));
	z = (pq_x * pr_y) - (pq_y * pr_x);

	FastNormalizeVector(x, y, z, &plane->normal);
	

		/* CALC DOT PRODUCT FOR PLANE CONSTANT */

	plane->constant = 	((plane->normal.x * p1x) +
						(plane->normal.y * p1y) +
						(plane->normal.z * p1z));
}


/************* CALC QUICK DISTANCE ****************/
//
// Does cheezeball quick distance calculation on 2 2D points.
//

static inline float CalcQuickDistance(float x1, float y1, float x2, float y2)
{
float	diffX,diffY;

	diffX = fabs(x1-x2);
	diffY = fabs(y1-y2);

	if (diffX > diffY)
	{
		return(diffX + (0.375f*diffY));			// same as (3*diffY)/8
	}
	else
	{
		return(diffY + (0.375f*diffX));
	}

}

/************* CALC DISTANCE ****************/

static inline float CalcDistance(float x1, float y1, float x2, float y2)
{
float	diffX,diffY;

	diffX = fabs(x1-x2);
	diffY = fabs(y1-y2);

	return(sqrt(diffX*diffX + diffY*diffY));
}

/************* CALC DISTANCE 3D ****************/

static inline float CalcDistance3D(float x1, float y1, float z1, float x2, float y2, float z2)
{
float	diffX,diffY,diffZ;

	diffX = fabs(x1-x2);
	diffY = fabs(y1-y2);
	diffZ = fabs(z1-z2);

	return(sqrt(diffX*diffX + diffY*diffY + diffZ*diffZ));
}


/******************** MASK ANGLE ****************************/
//
// Given an arbitrary angle, it limits it to between 0 and 2*PI
//

static inline float MaskAngle(float angle)
{
int		n;
Boolean	neg;

	neg = angle < 0.0f;
	n = angle * (1.0f/PI2);						// see how many times it wraps fully
	angle -= ((float)n * PI2);					// subtract # wrappings to get just the remainder
	
	if (neg)
		angle += PI2;

	return(angle);
}














