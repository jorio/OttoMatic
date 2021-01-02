//
// skeletonJoints.h
//

enum
{
	SKELETON_LIMBTYPE_PILLOW,
	SKELETON_LIMBTYPE_LEFTHAND,
	SKELETON_LIMBTYPE_HEAD
};



#define	NO_LIMB_MESH		(-1)			// indicates that this joint's limb mesh does not exist
#define	NO_PREVIOUS_JOINT	(-1)

//===========================================

extern	void UpdateJointTransforms(SkeletonObjDataType *skeleton,long jointNum);
void FindCoordOfJoint(ObjNode *theNode, long jointNum, OGLPoint3D *outPoint);
void FindCoordOnJoint(ObjNode *theNode, long jointNum, const OGLPoint3D *inPoint, OGLPoint3D *outPoint);
void FindJointFullMatrix(ObjNode *theNode, long jointNum, OGLMatrix4x4 *outMatrix);
void FindCoordOnJointAtFlagEvent(ObjNode *theNode, long jointNum, const OGLPoint3D *inPoint, OGLPoint3D *outPoint);
void FindJointMatrixAtFlagEvent(ObjNode *theNode, long jointNum, OGLMatrix4x4 *m);


