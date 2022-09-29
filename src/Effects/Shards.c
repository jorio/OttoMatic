/****************************/
/*   	SHARDS.C		    */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static int FindFreeShard(void);
static void ExplodeGeometry_Recurse(MetaObjectPtr obj);
static void ExplodeVertexArray(MOVertexArrayData *data, MOMaterialObject *overrideTexture);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_SHARDS		1000

typedef struct
{
	Boolean					isUsed;
	OGLVector3D				rot,rotDelta;
	OGLPoint3D				coord,coordDelta;
	float					decaySpeed,scale;
	Byte					mode;
	OGLMatrix4x4			matrix;

	OGLPoint3D				points[3];
	OGLTextureCoord			uvs[3];
	MOMaterialObject		*material;
	OGLColorRGBA			colorFilter;
	u_long					glow;
}ShardType;


/*********************/
/*    VARIABLES      */
/*********************/

int			gNumShards = 0;
ShardType	gShards[MAX_SHARDS];

static	float		gBoomForce,gShardDecaySpeed;
static	Byte		gShardMode;
static 	long		gShardDensity;
static 	OGLMatrix4x4	gWorkMatrix;
static 	ObjNode		*gShardSrcObj;

/************************* INIT SHARD SYSTEM ***************************/

void InitShardSystem(void)
{
int	i;

	gNumShards = 0;

	for (i = 0; i < MAX_SHARDS; i++)
		gShards[i].isUsed = false;

}




/********************* FIND FREE PARTICLE ***********************/
//
// OUTPUT: -1 == none free found
//

static int FindFreeShard(void)
{
int	i;

	if (gNumShards >= MAX_SHARDS)
		return(-1);

	for (i = 0; i < MAX_SHARDS; i++)
		if (gShards[i].isUsed == false)
			return(i);

	return(-1);
}


/****************** EXPLODE GEOMETRY ***********************/

void ExplodeGeometry(ObjNode *theNode, float boomForce, Byte particleMode, long particleDensity, float particleDecaySpeed)
{
MetaObjectPtr	theObject;

	gShardSrcObj	= theNode;
	gBoomForce 		= boomForce;
	gShardMode 		= particleMode;
	gShardDensity 	= particleDensity;
	gShardDecaySpeed = particleDecaySpeed;
	OGLMatrix4x4_SetIdentity(&gWorkMatrix);				// init to identity matrix


		/* SEE IF EXPLODING SKELETON OR PLAIN GEOMETRY */

	if (theNode->Genre == SKELETON_GENRE)
	{
		short	numMeshes,i,skelType;

		skelType = theNode->Type;
		numMeshes = theNode->Skeleton->skeletonDefinition->numDecomposedTriMeshes;

		UpdateSkinnedGeometry(theNode);				// be sure this skeleton's geometry is loaded into gLocalTriMeshesOfSkelType

		for (i = 0; i < numMeshes; i++)
		{
			ExplodeVertexArray(&gLocalTriMeshesOfSkelType[skelType][i], theNode->Skeleton->overrideTexture);		// explode each trimesh individually
		}
	}
	else
	{
		theObject = theNode->BaseGroup;
		ExplodeGeometry_Recurse(theObject);
	}


				/* SUBRECURSE CHAINS */

	if (theNode->ChainNode)
		ExplodeGeometry(theNode->ChainNode, boomForce, particleMode, particleDensity, particleDecaySpeed);

}


/****************** EXPLODE GEOMETRY - RECURSE ***********************/

static void ExplodeGeometry_Recurse(MetaObjectPtr obj)
{
MOGroupObject		*groupObj;
MOMatrixObject 		*matObj;
OGLMatrix4x4		*transform;
OGLMatrix4x4  		stashMatrix;
int					i,numChildren;

MetaObjectHeader	*objHead = obj;
MOVertexArrayData	*vaData;

			/* VERIFY COOKIE */

	if (objHead->cookie != MO_COOKIE)
		DoFatalAlert("ExplodeGeometry_Recurse: cookie is invalid!");


			/* HANDLE TYPE */

	switch(objHead->type)
	{
		case	MO_TYPE_GEOMETRY:
				switch(objHead->subType)
				{
					case	MO_GEOMETRY_SUBTYPE_VERTEXARRAY:
							vaData = &((MOVertexArrayObject *)obj)->objectData;
							ExplodeVertexArray(vaData, nil);
							break;

					default:
							DoFatalAlert("ExplodeGeometry_Recurse: unknown sub-type!");
				}
				break;

		case	MO_TYPE_MATERIAL:
				break;

		case	MO_TYPE_GROUP:
		 		groupObj = obj;
		  		stashMatrix = gWorkMatrix;											// push matrix

				numChildren = groupObj->objectData.numObjectsInGroup;				// get # objects in group

		  		for (i = 0; i < numChildren; i++)									// scan all objects in group
	    			ExplodeGeometry_Recurse(groupObj->objectData.groupContents[i]);	// sub-recurse this object

		  		gWorkMatrix = stashMatrix;											// pop matrix
				break;

		case	MO_TYPE_MATRIX:
				matObj = obj;
				transform = &matObj->matrix;										// point to matrix
				OGLMatrix4x4_Multiply(transform, &gWorkMatrix, &gWorkMatrix);		// multiply it in
				break;
	}
}


/********************** EXPLODE VERTEX ARRAY *******************************/

static void ExplodeVertexArray(MOVertexArrayData *data, MOMaterialObject *overrideTexture)
{
OGLPoint3D			centerPt = {0,0,0};
u_long				ind[3];
int					t;
OGLTextureCoord		*uvPtr;
long				i;
float				boomForce = gBoomForce;
OGLPoint3D			origin = {0,0,0};

	if (gShardMode & SHARD_MODE_FROMORIGIN)
	{
		origin.x = gShardSrcObj->Coord.x + (gShardSrcObj->BBox.max.x + gShardSrcObj->BBox.min.x) * .5f;		// set origin to center of object's bbox
		origin.y = gShardSrcObj->Coord.y + (gShardSrcObj->BBox.max.y + gShardSrcObj->BBox.min.y) * .5f;
		origin.z = gShardSrcObj->Coord.z + (gShardSrcObj->BBox.max.z + gShardSrcObj->BBox.min.z) * .5f;
	}

			/***************************/
			/* SCAN THRU ALL TRIANGLES */
			/***************************/

	for (t = 0; t < data->numTriangles; t += gShardDensity)				// scan thru all triangles
	{
				/* GET FREE PARTICLE INDEX */

		i = FindFreeShard();
		if (i == -1)														// see if all out
			break;


				/* DO POINTS */

		ind[0] = data->triangles[t].vertexIndices[0];						// get indecies of 3 points
		ind[1] = data->triangles[t].vertexIndices[1];
		ind[2] = data->triangles[t].vertexIndices[2];

		gShards[i].points[0] = data->points[ind[0]];						// get coords of 3 points
		gShards[i].points[1] = data->points[ind[1]];
		gShards[i].points[2] = data->points[ind[2]];

		OGLPoint3D_Transform(&gShards[i].points[0],&gWorkMatrix,&gShards[i].points[0]);							// transform points
		OGLPoint3D_Transform(&gShards[i].points[1],&gWorkMatrix,&gShards[i].points[1]);
		OGLPoint3D_Transform(&gShards[i].points[2],&gWorkMatrix,&gShards[i].points[2]);

		centerPt.x = (gShards[i].points[0].x + gShards[i].points[1].x + gShards[i].points[2].x) * 0.3333f;		// calc center of polygon
		centerPt.y = (gShards[i].points[0].y + gShards[i].points[1].y + gShards[i].points[2].y) * 0.3333f;
		centerPt.z = (gShards[i].points[0].z + gShards[i].points[1].z + gShards[i].points[2].z) * 0.3333f;

		gShards[i].points[0].x -= centerPt.x;								// offset coords to be around center
		gShards[i].points[0].y -= centerPt.y;
		gShards[i].points[0].z -= centerPt.z;
		gShards[i].points[1].x -= centerPt.x;
		gShards[i].points[1].y -= centerPt.y;
		gShards[i].points[1].z -= centerPt.z;
		gShards[i].points[2].x -= centerPt.x;
		gShards[i].points[2].y -= centerPt.y;
		gShards[i].points[2].z -= centerPt.z;


				/* DO VERTEX UV'S */

		uvPtr = data->uvs[0];
		if (uvPtr)																// see if also has UV (texture layer 0 only!)
		{
			gShards[i].uvs[0] = uvPtr[ind[0]];									// get vertex u/v's
			gShards[i].uvs[1] = uvPtr[ind[1]];
			gShards[i].uvs[2] = uvPtr[ind[2]];
		}

				/* DO MATERIAL INFO */

		if (overrideTexture)
			gShards[i].material = overrideTexture;
		else
		{
			if (data->numMaterials > 0)
				gShards[i].material = data->materials[0];							// keep material ptr
			else
				gShards[i].material = nil;
		}

		gShards[i].colorFilter = gShardSrcObj->ColorFilter;						// keep color
		gShards[i].glow = gShardSrcObj->StatusBits & STATUS_BIT_GLOW;

			/*********************/
			/* SET PHYSICS STUFF */
			/*********************/

		gShards[i].coord 	= centerPt;
		gShards[i].rot.x 	= gShards[i].rot.y = gShards[i].rot.z = 0;
		gShards[i].scale 	= 1.0;

		if (gShardMode & SHARD_MODE_FROMORIGIN)								// see if random deltas or from origin
		{
			OGLVector3D	v;

			v.x = centerPt.x - origin.x;									// calc vector from object's origin
			v.y = centerPt.y - origin.y;
			v.z = centerPt.z - origin.z;
			FastNormalizeVector(v.x, v.y, v.z, &v);

			gShards[i].coordDelta.x = v.x * boomForce;
			gShards[i].coordDelta.y = v.y * boomForce;
			gShards[i].coordDelta.z = v.z * boomForce;
		}
		else
		{
			gShards[i].coordDelta.x = RandomFloat2() * boomForce;
			gShards[i].coordDelta.y = RandomFloat2() * boomForce;
			gShards[i].coordDelta.z = RandomFloat2() * boomForce;
		}

		if (gShardMode & SHARD_MODE_UPTHRUST)
			gShards[i].coordDelta.y += 1.5f * gBoomForce;

		gShards[i].rotDelta.x 	= RandomFloat2() * (boomForce * .008f);			// random rotation deltas
		gShards[i].rotDelta.y 	= RandomFloat2() * (boomForce * .008f);
		gShards[i].rotDelta.z	= RandomFloat2() * (boomForce * .008f);

		gShards[i].decaySpeed 	= gShardDecaySpeed;
		gShards[i].mode 		= gShardMode;

				/* SET VALID & INC COUNTER */

		gShards[i].isUsed = true;
		gNumShards++;
	}
}


/************************** MOVE SHARDS ****************************/

void MoveShards(void)
{
float	ty,y,fps,x,z;
long	i;
OGLMatrix4x4	matrix,matrix2;

	if (gNumShards == 0)												// quick check if any particles at all
		return;

	fps = gFramesPerSecondFrac;

	for (i=0; i < MAX_SHARDS; i++)
	{
				/* ROTATE IT */

		gShards[i].rot.x += gShards[i].rotDelta.x * fps;
		gShards[i].rot.y += gShards[i].rotDelta.y * fps;
		gShards[i].rot.z += gShards[i].rotDelta.z * fps;

					/* MOVE IT */

		if (gShards[i].mode & SHARD_MODE_HEAVYGRAVITY)
			gShards[i].coordDelta.y -= fps * 1000.0f;		// gravity
		else
			gShards[i].coordDelta.y -= fps * 300.0f;		// gravity

		x = (gShards[i].coord.x += gShards[i].coordDelta.x * fps);
		y = (gShards[i].coord.y += gShards[i].coordDelta.y * fps);
		z = (gShards[i].coord.z += gShards[i].coordDelta.z * fps);


					/* SEE IF BOUNCE */

		ty = GetTerrainY(x,z);								// get terrain height here
		if (y <= ty)
		{
			if (gShards[i].mode & SHARD_MODE_BOUNCE)
			{
				gShards[i].coord.y  = ty;
				gShards[i].coordDelta.y *= -0.5f;
				gShards[i].coordDelta.x *= 0.9f;
				gShards[i].coordDelta.z *= 0.9f;
			}
			else
				goto del;
		}

					/* SCALE IT */

		gShards[i].scale -= gShards[i].decaySpeed * fps;
		if (gShards[i].scale <= 0.0f)
		{
				/* DEACTIVATE THIS PARTICLE */
del:
			gShards[i].isUsed = false;
			gNumShards--;
			continue;
		}

			/***************************/
			/* UPDATE TRANSFORM MATRIX */
			/***************************/

				/* SET SCALE MATRIX */

		OGLMatrix4x4_SetScale(&gShards[i].matrix, gShards[i].scale,	gShards[i].scale, gShards[i].scale);

					/* NOW ROTATE IT */

		OGLMatrix4x4_SetRotate_XYZ(&matrix, gShards[i].rot.x, gShards[i].rot.y, gShards[i].rot.z);
		OGLMatrix4x4_Multiply(&gShards[i].matrix,&matrix, &matrix2);

					/* NOW TRANSLATE IT */

		OGLMatrix4x4_SetTranslate(&matrix, gShards[i].coord.x, gShards[i].coord.y, gShards[i].coord.z);
		OGLMatrix4x4_Multiply(&matrix2,&matrix, &gShards[i].matrix);
	}
}


/************************* DRAW SHARDS ****************************/

void DrawShards(void)
{
long	i;

	if (gNumShards == 0)												// quick check if any particles at all
		return;

			/* SET STATE */

	glDisable(GL_CULL_FACE);
	OGL_DisableLighting();

	for (i=0; i < MAX_SHARDS; i++)
	{
		if (gShards[i].isUsed)
		{
					/* SUBMIT MATERIAL */

			gGlobalColorFilter.r = gShards[i].colorFilter.r;
			gGlobalColorFilter.g = gShards[i].colorFilter.g;
			gGlobalColorFilter.b = gShards[i].colorFilter.b;
			gGlobalTransparency = gShards[i].colorFilter.a;

			if (gShards[i].glow)
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			else
			    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


			if (gShards[i].material)
				MO_DrawMaterial(gShards[i].material);

					/* SET MATRIX */

			glPushMatrix();
			glMultMatrixf((GLfloat *)&gShards[i].matrix);


					/* DRAW THE TRIANGLE */

			glBegin(GL_TRIANGLES);
			glTexCoord2f(gShards[i].uvs[0].u, gShards[i].uvs[0].v);	glVertex3f(gShards[i].points[0].x, gShards[i].points[0].y, gShards[i].points[0].z);
			glTexCoord2f(gShards[i].uvs[1].u, gShards[i].uvs[1].v);	glVertex3f(gShards[i].points[1].x, gShards[i].points[1].y, gShards[i].points[1].z);
			glTexCoord2f(gShards[i].uvs[2].u, gShards[i].uvs[2].v);	glVertex3f(gShards[i].points[2].x, gShards[i].points[2].y, gShards[i].points[2].z);
			glEnd();

			glPopMatrix();
		}
	}

		/* CLEANUP */

	gGlobalColorFilter.r =
	gGlobalColorFilter.g =
	gGlobalColorFilter.b =
	gGlobalTransparency = 1;
	glEnable(GL_CULL_FACE);
	OGL_EnableLighting();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}









