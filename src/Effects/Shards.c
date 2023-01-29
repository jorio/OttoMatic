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

static void ExplodeGeometry_Recurse(MetaObjectPtr obj);
static void ExplodeVertexArray(MOVertexArrayData *data, MOMaterialObject *overrideTexture);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_SHARDS		1000

typedef struct
{
	OGLVector3D				rot,rotDelta;
	OGLPoint3D				coord,coordDelta;
	float					decaySpeed,scale;
	Byte					mode;
	OGLMatrix4x4			matrix;

	OGLPoint3D				points[3];
	OGLTextureCoord			uvs[3];
	MOMaterialObject		*material;
	OGLColorRGBA			colorFilter;
	Boolean					glow;
}ShardType;


/*********************/
/*    VARIABLES      */
/*********************/

static ShardType	gShards[MAX_SHARDS];
Pool*		gShardPool = NULL;

static	float		gBoomForce,gShardDecaySpeed;
static	Byte		gShardMode;
static 	long		gShardDensity;
static 	OGLMatrix4x4	gWorkMatrix;
static 	ObjNode		*gShardSrcObj;

/************************* INIT SHARD SYSTEM ***************************/

void InitShardSystem(void)
{
	GAME_ASSERT(!gShardPool);
	gShardPool = Pool_New(MAX_SHARDS);
}


/********************** DISPOSE SHARD SYSTEM ***************************/

void DisposeShardSystem(void)
{
	Pool_Free(gShardPool);
	gShardPool = NULL;
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
uint32_t			ind[3];
OGLTextureCoord		*uvPtr;
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

	for (int t = 0; t < data->numTriangles; t += gShardDensity)				// scan thru all triangles
	{
				/* GET FREE PARTICLE INDEX */

		int i = Pool_AllocateIndex(gShardPool);
		if (i < 0)															// see if all out
			break;

		ShardType* shard = &gShards[i];

				/* DO POINTS */

		ind[0] = data->triangles[t].vertexIndices[0];						// get indecies of 3 points
		ind[1] = data->triangles[t].vertexIndices[1];
		ind[2] = data->triangles[t].vertexIndices[2];

		shard->points[0] = data->points[ind[0]];						// get coords of 3 points
		shard->points[1] = data->points[ind[1]];
		shard->points[2] = data->points[ind[2]];

		OGLPoint3D_Transform(&shard->points[0], &gWorkMatrix, &shard->points[0]);							// transform points
		OGLPoint3D_Transform(&shard->points[1], &gWorkMatrix, &shard->points[1]);
		OGLPoint3D_Transform(&shard->points[2], &gWorkMatrix, &shard->points[2]);

		centerPt.x = (shard->points[0].x + shard->points[1].x + shard->points[2].x) * 0.3333f;		// calc center of polygon
		centerPt.y = (shard->points[0].y + shard->points[1].y + shard->points[2].y) * 0.3333f;
		centerPt.z = (shard->points[0].z + shard->points[1].z + shard->points[2].z) * 0.3333f;

		shard->points[0].x -= centerPt.x;								// offset coords to be around center
		shard->points[0].y -= centerPt.y;
		shard->points[0].z -= centerPt.z;
		shard->points[1].x -= centerPt.x;
		shard->points[1].y -= centerPt.y;
		shard->points[1].z -= centerPt.z;
		shard->points[2].x -= centerPt.x;
		shard->points[2].y -= centerPt.y;
		shard->points[2].z -= centerPt.z;


				/* DO VERTEX UV'S */

		uvPtr = data->uvs[0];
		if (uvPtr)																// see if also has UV (texture layer 0 only!)
		{
			shard->uvs[0] = uvPtr[ind[0]];									// get vertex u/v's
			shard->uvs[1] = uvPtr[ind[1]];
			shard->uvs[2] = uvPtr[ind[2]];
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
		gShards[i].glow = !!(gShardSrcObj->StatusBits & STATUS_BIT_GLOW);

			/*********************/
			/* SET PHYSICS STUFF */
			/*********************/

		shard->coord	= centerPt;
		shard->rot		= (OGLVector3D) {0,0,0};
		shard->scale	= 1;

		if (gShardMode & SHARD_MODE_FROMORIGIN)								// see if random deltas or from origin
		{
			OGLVector3D	v;

			v.x = centerPt.x - origin.x;									// calc vector from object's origin
			v.y = centerPt.y - origin.y;
			v.z = centerPt.z - origin.z;
			FastNormalizeVector(v.x, v.y, v.z, &v);

			shard->coordDelta.x = v.x * boomForce;
			shard->coordDelta.y = v.y * boomForce;
			shard->coordDelta.z = v.z * boomForce;
		}
		else
		{
			shard->coordDelta.x = RandomFloat2() * boomForce;
			shard->coordDelta.y = RandomFloat2() * boomForce;
			shard->coordDelta.z = RandomFloat2() * boomForce;
		}

		if (gShardMode & SHARD_MODE_UPTHRUST)
			shard->coordDelta.y += 1.5f * gBoomForce;

		shard->rotDelta.x 	= RandomFloat2() * (boomForce * .008f);			// random rotation deltas
		shard->rotDelta.y 	= RandomFloat2() * (boomForce * .008f);
		shard->rotDelta.z	= RandomFloat2() * (boomForce * .008f);

		shard->decaySpeed 	= gShardDecaySpeed;
		shard->mode 		= gShardMode;
	}
}


/************************** MOVE SHARDS ****************************/

void MoveShards(void)
{
float	ty,y,fps,x,z;
OGLMatrix4x4	matrix,matrix2;

	if (!gShardPool || Pool_Empty(gShardPool))				// quick check if any shards at all
		return;

	fps = gFramesPerSecondFrac;

	int i = Pool_First(gShardPool);
	while (i >= 0)
	{
		int nextIndex = Pool_Next(gShardPool, i);

		ShardType* shard = &gShards[i];

				/* ROTATE IT */

		shard->rot.x += shard->rotDelta.x * fps;
		shard->rot.y += shard->rotDelta.y * fps;
		shard->rot.z += shard->rotDelta.z * fps;

					/* MOVE IT */

		if (shard->mode & SHARD_MODE_HEAVYGRAVITY)
			shard->coordDelta.y -= fps * 1000.0f;		// gravity
		else
			shard->coordDelta.y -= fps * 300.0f;		// gravity

		x = (shard->coord.x += shard->coordDelta.x * fps);
		y = (shard->coord.y += shard->coordDelta.y * fps);
		z = (shard->coord.z += shard->coordDelta.z * fps);


					/* SEE IF BOUNCE */

		ty = GetTerrainY(x,z);								// get terrain height here
		if (y <= ty)
		{
			if (shard->mode & SHARD_MODE_BOUNCE)
			{
				shard->coord.y  = ty;
				shard->coordDelta.y *= -0.5f;
				shard->coordDelta.x *= 0.9f;
				shard->coordDelta.z *= 0.9f;
			}
			else
				goto del;
		}

					/* SCALE IT */

		shard->scale -= shard->decaySpeed * fps;
		if (shard->scale <= 0.0f)
		{
				/* DEACTIVATE THIS PARTICLE */
del:
			Pool_ReleaseIndex(gShardPool, i);
			goto next;
		}

			/***************************/
			/* UPDATE TRANSFORM MATRIX */
			/***************************/

				/* SET SCALE MATRIX */

		OGLMatrix4x4_SetScale(&shard->matrix, shard->scale,	shard->scale, shard->scale);

					/* NOW ROTATE IT */

		OGLMatrix4x4_SetRotate_XYZ(&matrix, shard->rot.x, shard->rot.y, shard->rot.z);
		OGLMatrix4x4_Multiply(&shard->matrix,&matrix, &matrix2);

					/* NOW TRANSLATE IT */

		OGLMatrix4x4_SetTranslate(&matrix, shard->coord.x, shard->coord.y, shard->coord.z);
		OGLMatrix4x4_Multiply(&matrix2,&matrix, &shard->matrix);

next:
		i = nextIndex;
	}
}


/************************* DRAW SHARDS ****************************/

void DrawShards(void)
{
	if (!gShardPool || Pool_Empty(gShardPool))			// quick check if any shards at all
		return;

			/* SET STATE */

	glDisable(GL_CULL_FACE);
	OGL_DisableLighting();

	for (int i = Pool_First(gShardPool); i >= 0; i = Pool_Next(gShardPool, i))
	{
		const ShardType* shard = &gShards[i];

				/* SUBMIT MATERIAL */

		gGlobalColorFilter.r = shard->colorFilter.r;
		gGlobalColorFilter.g = shard->colorFilter.g;
		gGlobalColorFilter.b = shard->colorFilter.b;
		gGlobalTransparency = shard->colorFilter.a;

		glBlendFunc(GL_SRC_ALPHA, shard->glow ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA);

		if (shard->material)
			MO_DrawMaterial(shard->material);

				/* SET MATRIX */

		glPushMatrix();
		glMultMatrixf(shard->matrix.value);

				/* DRAW TRIANGLE */

		glBegin(GL_TRIANGLES);
		glTexCoord2fv(&shard->uvs[0].u);	glVertex3fv(&shard->points[0].x);
		glTexCoord2fv(&shard->uvs[1].u);	glVertex3fv(&shard->points[1].x);
		glTexCoord2fv(&shard->uvs[2].u);	glVertex3fv(&shard->points[2].x);
		glEnd();

		glPopMatrix();
	}

		/* CLEANUP */

	gGlobalColorFilter.r = 1;
	gGlobalColorFilter.g = 1;
	gGlobalColorFilter.b = 1;
	gGlobalTransparency = 1;
	glEnable(GL_CULL_FACE);
	OGL_EnableLighting();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
