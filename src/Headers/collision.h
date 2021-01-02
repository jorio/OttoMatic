//
// collision.h
//

enum
{
	COLLISION_TYPE_OBJ,						// box
	COLLISION_TYPE_TILE
};
							



					/* COLLISION STRUCTURES */
struct CollisionRec
{
	Byte			baseBox,targetBox;
	unsigned short	sides;
	Byte			type;
	ObjNode			*objectPtr;			// object that collides with (if object type)
	float			planeIntersectY;	// where intersected triangle
};
typedef struct CollisionRec CollisionRec;



//=================================

void CollisionDetect(ObjNode *baseNode, u_long CType, short startNumCollisions);

Byte HandleCollisions(ObjNode *theNode, u_long cType, float deltaBounce);
extern	Boolean IsPointInPoly2D( float,  float ,  Byte ,  OGLPoint2D *);
extern	Boolean IsPointInTriangle(float pt_x, float pt_y, float x0, float y0, float x1, float y1, float x2, float y2);
short DoSimplePointCollision(OGLPoint3D *thePoint, u_long cType, ObjNode *except);
short DoSimpleBoxCollision(float top, float bottom, float left, float right,
						float front, float back, u_long cType);



Boolean DoSimplePointCollisionAgainstPlayer(OGLPoint3D *thePoint);
Boolean DoSimpleBoxCollisionAgainstPlayer(float top, float bottom, float left, float right,
										float front, float back);
Boolean DoSimpleBoxCollisionAgainstObject(float top, float bottom, float left, float right,
										float front, float back, ObjNode *targetNode);
float FindHighestCollisionAtXZ(float x, float z, u_long cType);

Boolean SeeIfLineSegmentHitsAnything(const OGLPoint3D *endPoint1, const OGLPoint3D *endPoint2,
								 const ObjNode *except, u_long ctype);

Boolean SeeIfLineSegmentHitsPlayer(const OGLPoint3D *endPoint1, const OGLPoint3D *endPoint2);

