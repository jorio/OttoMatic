/****************************/
/*   	SKELETON2.C    	    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;


/****************************/
/*    PROTOTYPES            */
/****************************/



/****************************/
/*    CONSTANTS             */
/****************************/


#define	SIZE_OF_JOINT_NUB	1.0f


/*********************/
/*    VARIABLES      */
/*********************/




/******************** UPDATE JOINT TRANSFORMS ****************************/
//
// Updates ALL of the transforms in a joint's transform group based on the theNode->Skeleton->JointCurrentPosition
//
// INPUT:	jointNum = joint # to rotate
//

void UpdateJointTransforms(SkeletonObjDataType *skeleton,long jointNum)
{
OGLMatrix4x4			matrix1;
static OGLMatrix4x4		matrix2 = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1};
OGLMatrix4x4			*destMatPtr;
const JointKeyframeType	*kfPtr;

	destMatPtr = &skeleton->jointTransformMatrix[jointNum];					// get ptr to joint's xform matrix

	kfPtr = &skeleton->JointCurrentPosition[jointNum];													// get ptr to keyframe

	if ((kfPtr->scale.x != 1.0f) || (kfPtr->scale.y != 1.0f) || (kfPtr->scale.z != 1.0f))				// SEE IF CAN IGNORE SCALE
	{
						/* ROTATE IT */

		OGLMatrix4x4_SetRotate_XYZ(&matrix1, kfPtr->rotation.x, kfPtr->rotation.y, kfPtr->rotation.z);	// set matrix for x/y/z rot


					/* SCALE & TRANSLATE */

		matrix2.value[M00] = kfPtr->scale.x;
												matrix2.value[M11] = kfPtr->scale.y;
																						matrix2.value[M22] = kfPtr->scale.z;
		matrix2.value[M03] = kfPtr->coord.x;	matrix2.value[M13] = kfPtr->coord.y;	matrix2.value[M23] = kfPtr->coord.z;

		OGLMatrix4x4_Multiply(&matrix1,&matrix2,destMatPtr);
	}
	else
	{
						/* ROTATE IT */

		OGLMatrix4x4_SetRotate_XYZ(destMatPtr, kfPtr->rotation.x, kfPtr->rotation.y, kfPtr->rotation.z);	// set matrix for x/y/z rot

						/* NOW TRANSLATE IT */

		destMatPtr->value[M03] =  kfPtr->coord.x;
		destMatPtr->value[M13] =  kfPtr->coord.y;
		destMatPtr->value[M23] =  kfPtr->coord.z;
	}

}


/*************** FIND COORD OF JOINT *****************/
//
// Returns the 3-space coord of the given joint.
//

void FindCoordOfJoint(ObjNode *theNode, long jointNum, OGLPoint3D *outPoint)
{
OGLMatrix4x4	matrix;
static OGLPoint3D		point3D = {0,0,0};				// use joint's origin @ 0,0,0

	FindJointFullMatrix(theNode,jointNum,&matrix);		// calc matrix
	OGLPoint3D_Transform(&point3D, &matrix, outPoint);	// apply matrix to origin @ 0,0,0 to get new 3-space coords
}


/*************** FIND COORD ON JOINT *****************/
//
// Returns the 3-space coord of a point on the given joint.
//
// Essentially the same as FindCoordOfJoint except that rather than
// using 0,0,0 as the applied point, it takes the input point which
// is an offset from the origin of the joint.  Thus, the returned point
// is "on" the joint rather than the exact coord of the hinge.
//

void FindCoordOnJoint(ObjNode *theNode, long jointNum, const OGLPoint3D *inPoint, OGLPoint3D *outPoint)
{
OGLMatrix4x4	matrix;

	FindJointFullMatrix(theNode,jointNum,&matrix);		// calc matrix
	OGLPoint3D_Transform(inPoint, &matrix, outPoint);	// apply matrix to origin @ 0,0,0 to get new 3-space coords
}


/*************** FIND COORD ON JOINT AT FLAG EVENT *****************/
//
// Same as above except that it finds the coordinate on the skeleton structure
// at the EXACT moment that the Flag event occurred.
//

void FindCoordOnJointAtFlagEvent(ObjNode *theNode, long jointNum, const OGLPoint3D *inPoint, OGLPoint3D *outPoint)
{
OGLMatrix4x4	matrix;
short			time,oldTime;
SkeletonObjDataType	*skeleton;
Byte			i,numEvents;

			/* GET THE TIME OF THE 1ST FLAG EVENT */

	skeleton = theNode->Skeleton;
	numEvents = skeleton->skeletonDefinition->NumAnimEvents[skeleton->AnimNum];

	for (i = 0; i < numEvents; i++)
	{
		if (skeleton->skeletonDefinition->AnimEventsList[skeleton->AnimNum][i].type == ANIMEVENT_TYPE_SETFLAG)
		{
			time = skeleton->skeletonDefinition->AnimEventsList[skeleton->AnimNum][i].time;
			break;
		}
	}

				/* TEMPORARILY SET THE TIME TO THE TIME OF THE FLAG EVENT */

	oldTime = skeleton->CurrentAnimTime;				// remember what time it is
	skeleton->CurrentAnimTime = time;					// set to time of flag event
	GetModelCurrentPosition(skeleton);					// reset all of the matrices


				/* CALC THE COORD */

	FindJointFullMatrix(theNode,jointNum,&matrix);		// calc matrix
	OGLPoint3D_Transform(inPoint, &matrix, outPoint);	// apply matrix to origin @ 0,0,0 to get new 3-space coords


				/* SET THINGS BACK TO NORMAL */

	skeleton->CurrentAnimTime = oldTime;
	GetModelCurrentPosition(skeleton);					// reset all of the matrices to original positions
}


/*************** FIND JOINT MATRIX AT FLAG EVENT *****************/

void FindJointMatrixAtFlagEvent(ObjNode *theNode, long jointNum, OGLMatrix4x4 *m)
{
short			time,oldTime;
SkeletonObjDataType	*skeleton;
Byte			i,numEvents;

			/* GET THE TIME OF THE 1ST FLAG EVENT */

	skeleton = theNode->Skeleton;
	numEvents = skeleton->skeletonDefinition->NumAnimEvents[skeleton->AnimNum];

	for (i = 0; i < numEvents; i++)
	{
		if (skeleton->skeletonDefinition->AnimEventsList[skeleton->AnimNum][i].type == ANIMEVENT_TYPE_SETFLAG)
		{
			time = skeleton->skeletonDefinition->AnimEventsList[skeleton->AnimNum][i].time;
			break;
		}
	}

				/* TEMPORARILY SET THE TIME TO THE TIME OF THE FLAG EVENT */

	oldTime = skeleton->CurrentAnimTime;				// remember what time it is
	skeleton->CurrentAnimTime = time;					// set to time of flag event
	GetModelCurrentPosition(skeleton);					// reset all of the matrices


				/* CALC THE COORD */

	FindJointFullMatrix(theNode,jointNum,m);			// calc matrix


				/* SET THINGS BACK TO NORMAL */

	skeleton->CurrentAnimTime = oldTime;
	GetModelCurrentPosition(skeleton);					// reset all of the matrices to original positions
}



/************* FIND JOINT FULL MATRIX ****************/
//
// Returns an accumulated matrix for a joint's coordinates.
//

void FindJointFullMatrix(ObjNode *theNode, long jointNum, OGLMatrix4x4 *outMatrix)
{
SkeletonDefType		*skeletonDefPtr;
SkeletonObjDataType	*skeletonPtr;
BoneDefinitionType	*bonePtr;

			/* ACCUMULATE A MATRIX DOWN THE CHAIN */

	*outMatrix = theNode->Skeleton->jointTransformMatrix[jointNum];		// init matrix

	skeletonPtr =  theNode->Skeleton;									// point to skeleton
	if (skeletonPtr == nil)												// if nothing, then return coord matrix
	{
		OGLMatrix4x4_SetTranslate(outMatrix, theNode->Coord.x, theNode->Coord.y, theNode->Coord.z);
		return;
	}

	skeletonDefPtr = skeletonPtr->skeletonDefinition;					// point to skeleton defintion

	if ((jointNum >= skeletonDefPtr->NumBones)	||						// check for illegal joints
		(jointNum < 0))
		DoFatalAlert("FindJointFullMatrix: illegal jointNum!");

	bonePtr = skeletonDefPtr->Bones;									// point to bones list

	while(bonePtr[jointNum].parentBone != NO_PREVIOUS_JOINT)
	{
		jointNum = bonePtr[jointNum].parentBone;

  		OGLMatrix4x4_Multiply(outMatrix,&skeletonPtr->jointTransformMatrix[jointNum],outMatrix);
	}

			/* ALSO FACTOR IN THE BASE MATRIX */
			//
			// Caller should make sure this is up to date!
			//

	OGLMatrix4x4_Multiply(outMatrix,&theNode->BaseTransformMatrix,outMatrix);
}





