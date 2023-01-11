//
// collision.h
//

enum
{
	COLLISION_TYPE_OBJ,						// box
};



					/* COLLISION STRUCTURES */
struct CollisionRec
{
	Byte			baseBox,targetBox;
	uint16_t		sides;
	Byte			type;
	ObjNode			*objectPtr;			// object that collides with (if object type)
	float			planeIntersectY;	// where intersected triangle
};
typedef struct CollisionRec CollisionRec;



//=================================

void CollisionDetect(ObjNode *baseNode, uint32_t CType, int startNumCollisions);

Byte HandleCollisions(ObjNode *theNode, uint32_t cType, float deltaBounce);
Boolean IsPointInPoly2D(float, float, Byte, const OGLPoint2D*);
Boolean IsPointInTriangle(float pt_x, float pt_y, float x0, float y0, float x1, float y1, float x2, float y2);
int DoSimplePointCollision(const OGLPoint3D *thePoint, uint32_t cType, const ObjNode* except);
int DoSimpleBoxCollision(float top, float bottom, float left, float right, float front, float back, uint32_t cType);

Boolean DoSimplePointCollisionAgainstPlayer(const OGLPoint3D* thePoint);
Boolean DoSimpleBoxCollisionAgainstPlayer(float top, float bottom, float left, float right, float front, float back);
Boolean DoSimpleBoxCollisionAgainstObject(float top, float bottom, float left, float right, float front, float back, const ObjNode* targetNode);
float FindHighestCollisionAtXZ(float x, float z, uint32_t cType);

Boolean SeeIfLineSegmentHitsAnything(const OGLPoint3D* endPoint1, const OGLPoint3D* endPoint2, const ObjNode* except, uint32_t ctype);
Boolean SeeIfLineSegmentHitsPlayer(const OGLPoint3D* endPoint1, const OGLPoint3D* endPoint2);

