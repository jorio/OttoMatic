/****************************/
/*   	BONES.C	    	    */
/* (c)1997-9 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"

extern	MOVertexArrayData	**gLocalTriMeshesOfSkelType;
extern	BG3DFileContainer		*gBG3DContainerList[];

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DecomposeVertexArrayGeometry(MOVertexArrayObject *theTriMesh);
static void DecompRefMo_Recurse(MetaObjectPtr inObj);
static void DecomposeReferenceModel(MetaObjectPtr theModel);
static void UpdateSkinnedGeometry_Recurse(short joint, short skelType);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/


SkeletonDefType		*gCurrentSkeleton;
SkeletonObjDataType	*gCurrentSkelObjData;

static	OGLMatrix4x4		gMatrix;

static	OGLBoundingBox		*gBBox;

static	OGLVector3D			gTransformedNormals[MAX_DECOMPOSED_NORMALS];	// temporary buffer for holding transformed normals before they're applied to their trimeshes


/******************** LOAD BONES REFERENCE MODEL *********************/
//
// INPUT: inSpec = spec of 3dmf file to load or nil to StdDialog it.
//

void LoadBonesReferenceModel(FSSpec	*inSpec, SkeletonDefType *skeleton, int skeletonType, OGLSetupOutputType *setupInfo)
{
int				g;
MetaObjectPtr	model;

	gCurrentSkeleton = skeleton;

	g = MODEL_GROUP_SKELETONBASE+skeletonType;						// calc group # to store model into
	ImportBG3D(inSpec, g, setupInfo);								// load skeleton model into group

	model = gBG3DContainerList[g]->root;							// point to base group

	DecomposeReferenceModel(model);
}


/****************** DECOMPOSE REFERENCE MODEL ************************/

static void DecomposeReferenceModel(MetaObjectPtr theModel)
{
	gCurrentSkeleton->numDecomposedTriMeshes = 0;
	gCurrentSkeleton->numDecomposedPoints = 0;
	gCurrentSkeleton->numDecomposedNormals = 0;


		/* DO SUBRECURSIVE SCAN */

	DecompRefMo_Recurse(theModel);

}


/***************** DECOM REF MO: RECURSE ***************************/

static void DecompRefMo_Recurse(MetaObjectPtr inObj)
{
MetaObjectHeader	*head;

	head = inObj;												// convert to header ptr

				/* SEE IF FOUND GEOMETRY */

	if (head->type == MO_TYPE_GEOMETRY)
	{
		if (head->subType == MO_GEOMETRY_SUBTYPE_VERTEXARRAY)
			DecomposeVertexArrayGeometry(inObj);
		else
			DoFatalAlert("\pDecompRefMo_Recurse: unknown geometry subtype");
	}
	else

			/* SEE IF RECURSE SUB-GROUP */

	if (head->type == MO_TYPE_GROUP)
 	{
 		MOGroupObject	*groupObj;
 		MOGroupData		*groupData;
		int				i;

 		groupObj = inObj;
 		groupData = &groupObj->objectData;							// point to group data

  		for (i = 0; i < groupData->numObjectsInGroup; i++)			// scan all objects in group
 		{
 			MetaObjectPtr	subObj;

 			subObj = groupData->groupContents[i];					// get illegal ref to object in group
    		DecompRefMo_Recurse(subObj);							// sub-recurse this object
  		}
	}
}


/******************* DECOMPOSE VERTEX ARRAY GEOMETRY ***********************/

static void DecomposeVertexArrayGeometry(MOVertexArrayObject *theTriMesh)
{
unsigned long		numVertecies,vertNum;
OGLPoint3D			*vertexList;
long				i,n,refNum,pointNum;
OGLVector3D			*normalPtr;
DecomposedPointType	*decomposedPoint;
MOVertexArrayData	*data;

	n = gCurrentSkeleton->numDecomposedTriMeshes;										// get index into list of trimeshes
	if (n >= MAX_DECOMPOSED_TRIMESHES)
		DoFatalAlert("\pDecomposeATriMesh: gNumDecomposedTriMeshes > MAX_DECOMPOSED_TRIMESHES");


			/* GET TRIMESH DATA */

	gCurrentSkeleton->decomposedTriMeshes[n] = theTriMesh->objectData;					// copy vertex array geometry data
																						// NOTE that this also creates
																						// new ILLEGAL refs to any material objects
	data = &gCurrentSkeleton->decomposedTriMeshes[n];

	numVertecies = data->numPoints;														// get # verts in trimesh
	vertexList = data->points;															// point to vert list
	normalPtr  = data->normals; 															// point to normals


				/*******************************/
				/* EXTRACT VERTECIES & NORMALS */
				/*******************************/

	for (vertNum = 0; vertNum < numVertecies; vertNum++)
	{
			/* SEE IF THIS POINT IS ALREADY IN DECOMPOSED LIST */

		for (pointNum=0; pointNum < gCurrentSkeleton->numDecomposedPoints; pointNum++)
		{
			decomposedPoint = &gCurrentSkeleton->decomposedPointList[pointNum];					// point to this decomposed point

			if (PointsAreCloseEnough(&vertexList[vertNum],&decomposedPoint->realPoint))			// see if close enough to match
			{
					/* ADD ANOTHER REFERENCE */

				refNum = decomposedPoint->numRefs;												// get # refs for this point
				if (refNum >= MAX_POINT_REFS)
					DoFatalAlert("\pDecomposeATriMesh: MAX_POINT_REFS exceeded!");

				decomposedPoint->whichTriMesh[refNum] = n;										// set triMesh #
				decomposedPoint->whichPoint[refNum] = vertNum;									// set point #
				decomposedPoint->numRefs++;														// inc counter
				goto added_vert;
			}
		}
				/* IT'S A NEW POINT SO ADD TO LIST */

		pointNum = gCurrentSkeleton->numDecomposedPoints;
		if (pointNum >= MAX_DECOMPOSED_POINTS)
			DoFatalAlert("\pDecomposeATriMesh: MAX_DECOMPOSED_POINTS exceeded!");

		refNum = 0;																			// it's the 1st entry (need refNum for below).

		decomposedPoint = &gCurrentSkeleton->decomposedPointList[pointNum];					// point to this decomposed point
		decomposedPoint->realPoint 				= vertexList[vertNum];						// add new point to list
		decomposedPoint->whichTriMesh[refNum] 	= n;										// set triMesh #
		decomposedPoint->whichPoint[refNum] 	= vertNum;									// set point #
		decomposedPoint->numRefs 				= 1;										// set # refs to 1

		gCurrentSkeleton->numDecomposedPoints++;											// inc # decomposed points

added_vert:


				/***********************************************/
				/* ADD THIS POINT'S NORMAL TO THE NORMALS LIST */
				/***********************************************/

					/* SEE IF NORMAL ALREADY IN LIST */

		FastNormalizeVector(normalPtr[vertNum].x,
							normalPtr[vertNum].y,
							normalPtr[vertNum].z,
							&normalPtr[vertNum]);						// normalize to be safe

		for (i=0; i < gCurrentSkeleton->numDecomposedNormals; i++)
			if (VectorsAreCloseEnough(&normalPtr[vertNum],&gCurrentSkeleton->decomposedNormalsList[i]))	// if already in list, then dont add it again
				goto added_norm;


				/* ADD NEW NORMAL TO LIST */

		i = gCurrentSkeleton->numDecomposedNormals;										// get # decomposed normals already in list
		if (i >= MAX_DECOMPOSED_NORMALS)
			DoFatalAlert("\pDecomposeATriMesh: MAX_DECOMPOSED_NORMALS exceeded!");

		gCurrentSkeleton->decomposedNormalsList[i] = normalPtr[vertNum];				// add new normal to list
		gCurrentSkeleton->numDecomposedNormals++;										// inc # decomposed normals

added_norm:
					/* KEEP REF TO NORMAL IN POINT LIST */

		decomposedPoint->whichNormal[refNum] = i;										// save index to normal
	}

	gCurrentSkeleton->numDecomposedTriMeshes++;											// inc # of trimeshes in decomp list
}



/************************** UPDATE SKINNED GEOMETRY *******************************/
//
// Updates all of the points in the local trimesh data to coordinate with the
// current joint transforms.
//

void UpdateSkinnedGeometry(ObjNode *theNode)
{
long	skelType;
SkeletonObjDataType	*currentSkelObjData;

			/* MAKE SURE OBJNODE IS STILL VALID */
			//
			// It's possible that Deleting a Skeleton and then creating a new
			// ObjNode like an explosion can make it seem like the current
			// objNode pointer is still valid when in fact it isn't.
			// Detecting this is too complex to be worth-while, so it's
			// easier to just verify the objNode here instead.
			//

	if (theNode->CType == INVALID_NODE_FLAG)
		return;
	gCurrentSkelObjData = currentSkelObjData = theNode->Skeleton;
	if (gCurrentSkelObjData == nil)
		return;

	gCurrentSkeleton = currentSkelObjData->skeletonDefinition;
	if (gCurrentSkeleton == nil)
		DoFatalAlert("\pUpdateSkinnedGeometry: gCurrentSkeleton is invalid!");

	if (currentSkelObjData->JointsAreGlobal)
		OGLMatrix4x4_SetIdentity(&gMatrix);
	else
		gMatrix = theNode->BaseTransformMatrix;

	gBBox = &theNode->BBox;													// point objnode's bbox

	gBBox->min.x = gBBox->min.y = gBBox->min.z = 10000000;
	gBBox->max.x = gBBox->max.y = gBBox->max.z = -gBBox->min.x;								// init bounding box calc

	if (gCurrentSkeleton->Bones[0].parentBone != NO_PREVIOUS_JOINT)
		DoFatalAlert("\pUpdateSkinnedGeometry: joint 0 isnt base - fix code Brian!");

	skelType = theNode->Type;

				/* DO RECURSION TO BUILD IT */

	UpdateSkinnedGeometry_Recurse(0, skelType);											// start @ base


				/* BUILD A LOCAL BBOX */
				//
				// Note:  we want to store a local-coord bbox, not the world-coord one that we now have
				//

	gBBox->min.x -= theNode->Coord.x;
	gBBox->max.x -= theNode->Coord.x;
	gBBox->min.y -= theNode->Coord.y;
	gBBox->max.y -= theNode->Coord.y;
	gBBox->min.z -= theNode->Coord.z;
	gBBox->max.z -= theNode->Coord.z;

	gBBox->isEmpty = false;



}


/******************** UPDATE SKINNED GEOMETRY: RECURSE ************************/

static void UpdateSkinnedGeometry_Recurse(short joint, short skelType)
{
long					numChildren,numPoints,p,i,numRefs,r,triMeshNum,p2,c,numNormals,n;
OGLMatrix4x4			oldM;
OGLVector3D				*normalAttribs;
BoneDefinitionType		*bonePtr;
float					minX,maxX,maxY,minY,maxZ,minZ;
long					numDecomposedPoints,numDecomposedNormals;
float					m00,m01,m02,m10,m11,m12,m20,m21,m22,m30,m31,m32;
float					newX,newY,newZ;
const SkeletonObjDataType		*currentSkelObjData = gCurrentSkelObjData;
const SkeletonDefType	*currentSkeleton = gCurrentSkeleton;
const OGLMatrix4x4		*jointMat;
OGLMatrix4x4			*matPtr;
float					x,y,z;
const DecomposedPointType	*decomposedPointList = currentSkeleton->decomposedPointList;
const MOVertexArrayData		*localTriMeshes = &gLocalTriMeshesOfSkelType[skelType][0];

	minX = gBBox->min.x;				// calc local bbox with registers for speed
	minY = gBBox->min.y;
	minZ = gBBox->min.z;
	maxX = gBBox->max.x;
	maxY = gBBox->max.y;
	maxZ = gBBox->max.z;

	numDecomposedPoints = currentSkeleton->numDecomposedPoints;
	numDecomposedNormals = currentSkeleton->numDecomposedNormals;

				/*********************************/
				/* FACTOR IN THIS JOINT'S MATRIX */
				/*********************************/

	jointMat = &currentSkelObjData->jointTransformMatrix[joint];
	matPtr = &gMatrix;

	if (!gCurrentSkelObjData->JointsAreGlobal)
	{
		OGLMatrix4x4_Multiply(jointMat, matPtr, matPtr);

		m00 = matPtr->value[M00];	m01 = matPtr->value[M10];	m02 = matPtr->value[M20];
		m10 = matPtr->value[M01];	m11 = matPtr->value[M11];	m12 = matPtr->value[M21];
		m20 = matPtr->value[M02];	m21 = matPtr->value[M12];	m22 = matPtr->value[M22];
		m30 = matPtr->value[M03];	m31 = matPtr->value[M13];	m32 = matPtr->value[M23];
	}
	else
	{
		m00 = jointMat->value[M00];	m01 = jointMat->value[M10];	m02 = jointMat->value[M20];
		m10 = jointMat->value[M01];	m11 = jointMat->value[M11];	m12 = jointMat->value[M21];
		m20 = jointMat->value[M02];	m21 = jointMat->value[M12];	m22 = jointMat->value[M22];
		m30 = jointMat->value[M03];	m31 = jointMat->value[M13];	m32 = jointMat->value[M23];
	}

			/*************************/
			/* TRANSFORM THE NORMALS */
			/*************************/

		/* APPLY MATRIX TO EACH NORMAL VECTOR */

	bonePtr = &currentSkeleton->Bones[joint];									// point to bone def
	numNormals = bonePtr->numNormalsAttachedToBone;								// get # normals attached to this bone


	for (p=0; p < numNormals; p++)
	{
		i = bonePtr->normalList[p];												// get index to normal in gDecomposedNormalsList

		x = currentSkeleton->decomposedNormalsList[i].x;						// get xyz of normal
		y = currentSkeleton->decomposedNormalsList[i].y;
		z = currentSkeleton->decomposedNormalsList[i].z;

		gTransformedNormals[i].x = (m00*x) + (m10*y) + (m20*z);					// transform the normal
		gTransformedNormals[i].y = (m01*x) + (m11*y) + (m21*z);
		gTransformedNormals[i].z = (m02*x) + (m12*y) + (m22*z);
	}



			/* APPLY TRANSFORMED VECTORS TO ALL REFERENCES */

	numPoints = bonePtr->numPointsAttachedToBone;									// get # points attached to this bone

	for (p = 0; p < numPoints; p++)
	{
		i = bonePtr->pointList[p];													// get index to point in gDecomposedPointList

		numRefs = decomposedPointList[i].numRefs;									// get # times this point is referenced
		if (numRefs == 1)															// SPECIAL CASE IF ONLY 1 REF (OPTIMIZATION)
		{
			triMeshNum = decomposedPointList[i].whichTriMesh[0];					// get triMesh # that uses this point
			p2 = decomposedPointList[i].whichPoint[0];								// get point # in the triMesh
			n = decomposedPointList[i].whichNormal[0];								// get index into gDecomposedNormalsList

			normalAttribs = localTriMeshes[triMeshNum].normals;						// point to normals list
			normalAttribs[p2] = gTransformedNormals[n];								// copy transformed normal into triMesh
		}
		else																		// handle multi-case
		{
			for (r = 0; r < numRefs; r++)
			{
				triMeshNum = decomposedPointList[i].whichTriMesh[r];
				p2 = decomposedPointList[i].whichPoint[r];
				n = decomposedPointList[i].whichNormal[r];

				normalAttribs = localTriMeshes[triMeshNum].normals;
				normalAttribs[p2] = gTransformedNormals[n];
			}
		}
	}

			/************************/
			/* TRANSFORM THE POINTS */
			/************************/

	if (!gCurrentSkelObjData->JointsAreGlobal)
	{
		m00 = matPtr->value[M00];	m01 = matPtr->value[M10];	m02 = matPtr->value[M20];
		m10 = matPtr->value[M01];	m11 = matPtr->value[M11];	m12 = matPtr->value[M21];
		m20 = matPtr->value[M02];	m21 = matPtr->value[M12];	m22 = matPtr->value[M22];
		m30 = matPtr->value[M03];	m31 = matPtr->value[M13];	m32 = matPtr->value[M23];
	}

	for (p = 0; p < numPoints; p++)
	{
		float	x,y,z;

		i = bonePtr->pointList[p];														// get index to point in gDecomposedPointList
		x = decomposedPointList[i].boneRelPoint.x;										// get xyz of point
		y = decomposedPointList[i].boneRelPoint.y;
		z = decomposedPointList[i].boneRelPoint.z;

				/* TRANSFORM & UPDATE BBOX */

		newX = (m00*x) + (m10*y) + (m20*z) + m30;										// transform x value
		if (newX < minX)																// update bbox
			minX = newX;
		if (newX > maxX)
			maxX = newX;

		newY = (m01*x) + (m11*y) + (m21*z) + m31;										// transform y
		if (newY < minY)																// update bbox
			minY = newY;
		if (newY > maxY)
			maxY = newY;

		newZ = (m02*x) + (m12*y) + (m22*z) + m32;										// transform z
		if (newZ > maxZ)																// update bbox
			maxZ = newZ;
		if (newZ < minZ)
			minZ = newZ;


				/* APPLY NEW POINT TO ALL REFERENCES */

		numRefs = decomposedPointList[i].numRefs;										// get # times this point is referenced
		if (numRefs == 1)																// SPECIAL CASE IF ONLY 1 REF (OPTIMIZATION)
		{
			triMeshNum = decomposedPointList[i].whichTriMesh[0];						// get triMesh # that uses this point
			p2 = decomposedPointList[i].whichPoint[0];									// get point # in the triMesh

			localTriMeshes[triMeshNum].points[p2].x = newX;								// set the point in local copy of trimesh
			localTriMeshes[triMeshNum].points[p2].y = newY;
			localTriMeshes[triMeshNum].points[p2].z = newZ;
		}
		else																			// multi-refs
		{
			for (r = 0; r < numRefs; r++)
			{
				triMeshNum = decomposedPointList[i].whichTriMesh[r];
				p2 = decomposedPointList[i].whichPoint[r];

				localTriMeshes[triMeshNum].points[p2].x = newX;
				localTriMeshes[triMeshNum].points[p2].y = newY;
				localTriMeshes[triMeshNum].points[p2].z = newZ;
			}
		}
	}

				/* UPDATE GLOBAL BBOX */

	gBBox->min.x = minX;
	gBBox->min.y = minY;
	gBBox->min.z = minZ;
	gBBox->max.x = maxX;
	gBBox->max.y = maxY;
	gBBox->max.z = maxZ;


			/* RECURSE THRU ALL CHILDREN */

	numChildren = currentSkeleton->numChildren[joint];									// get # children
	for (c = 0; c < numChildren; c++)
	{
		oldM = gMatrix;																	// push matrix
		UpdateSkinnedGeometry_Recurse(currentSkeleton->childIndecies[joint][c], skelType);
		gMatrix = oldM;																	// pop matrix
	}

}


/******************* PRIME BONE DATA *********************/
//
// After a skeleton file is loaded, this will calc some other needed things.
//

void PrimeBoneData(SkeletonDefType *skeleton)
{
long	i,b,j;

	if (skeleton->NumBones == 0)
		DoFatalAlert("\pPrimeBoneData: # = 0??");


			/* SET THE FORWARD LINKS */

	for (b = 0; b < skeleton->NumBones; b++)
	{
		skeleton->numChildren[b] = 0;								// init child counter

		for (i = 0; i < skeleton->NumBones; i++)					// look for a child
		{
			if (skeleton->Bones[i].parentBone == b)					// is this "i" a child of "b"?
			{
				j = skeleton->numChildren[b];						// get # children
				if (j >= MAX_CHILDREN)
					DoFatalAlert("\pCreateSkeletonFromBones: MAX_CHILDREN exceeded!");

				skeleton->childIndecies[b][j] = i;					// set index to child

				skeleton->numChildren[b]++;							// inc # children
			}
		}
	}
}










