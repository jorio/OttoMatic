/****************************/
/*   	EFFECTS.C		    */
/* By Brian Greenstone      */
/* (c)2000 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DeleteParticleGroup(int groupNum);
static void MoveParticleGroups(ObjNode *theNode);

static void DrawParticleGroup(ObjNode *theNode);


static void MoveBlobDroplet(ObjNode *theNode);

static void MoveSmoker(ObjNode *theNode);

/****************************/
/*    CONSTANTS             */
/****************************/

#define	FIRE_TIMER	.05f
#define	SMOKE_TIMER	.07f

enum
{
	DEATH_EXIT_MODE_CLOSE,
	DEATH_EXIT_MODE_OPEN,
	DEATH_EXIT_MODE_STAY
};

/*********************/
/*    VARIABLES      */
/*********************/

static ParticleGroupType	gParticleGroups[MAX_PARTICLE_GROUPS];
Pool *gParticleGroupPool = NULL;

static float	gGravitoidDistBuffer[MAX_PARTICLES][MAX_PARTICLES];

NewParticleGroupDefType	gNewParticleGroupDef;

Boolean	gDoDeathExit = false;
static float	gDeathExitX = 0;
static Byte		gDeathExitMode = DEATH_EXIT_MODE_CLOSE;
static	float	gDeathExitDelay;


/************************* INIT EFFECTS ***************************/

void InitEffects(void)
{
	InitParticleSystem();
	InitShardSystem();
	InitSparkles();


			/* SET SPRITE BLENDING FLAGS */

	BlendASprite(SPRITE_GROUP_PARTICLES, PARTICLE_SObjType_Splash);
}



/********************** DISPOSE EFFECT SYSTEMS *********************/

void DisposeEffects(void)
{
	DisposeParticleSystem();
	DisposeShardSystem();
	DisposeSparkles();
}



#pragma mark -

/************************ INIT PARTICLE SYSTEM **************************/

void InitParticleSystem(void)
{
	GAME_ASSERT(!gParticleGroupPool);
	gParticleGroupPool = Pool_New(MAX_PARTICLE_GROUPS);


			/* INIT GROUP ARRAY */

	memset(gParticleGroups, 0, sizeof(gParticleGroups));

	for (int i = 0; i < MAX_PARTICLE_GROUPS; i++)
	{
		gParticleGroups[i].pool = Pool_New(MAX_PARTICLES);

				/* VERTEX ARRAY DATA BASE */

		MOVertexArrayData vertexArrayData =
		{
			.numMaterials	= 0,
			.numPoints		= 0,
			.numTriangles	= 0,
			.points			= (OGLPoint3D*) AllocPtr(sizeof(OGLPoint3D) * MAX_PARTICLES * 4),
			.normals		= NULL,
			.uvs[0]			= (OGLTextureCoord*) AllocPtr(sizeof(OGLTextureCoord) * MAX_PARTICLES * 4),
			.colorsByte		= (OGLColorRGBA_Byte*) AllocPtr(sizeof(OGLColorRGBA_Byte) * MAX_PARTICLES * 4),
			.colorsFloat	= NULL,
			.triangles		= (MOTriangleIndecies*) AllocPtr(sizeof(MOTriangleIndecies) * MAX_PARTICLES * 2),
		};
		
				/* INIT UV ARRAYS */

		OGLTextureCoord* uv = vertexArrayData.uvs[0];
		for (int j = 0; j < MAX_PARTICLES * 4; j += 4)
		{
			uv[j  ] = (OGLTextureCoord) {0, 0};				// upper left
			uv[j+1] = (OGLTextureCoord) {0, 1};				// lower left
			uv[j+2] = (OGLTextureCoord) {1, 1};				// lower right
			uv[j+3] = (OGLTextureCoord) {1, 0};				// upper right
		}

				/* INIT TRIANGLE ARRAYS */

		for (int j = 0, k = 0; j < MAX_PARTICLES * 2; j += 2, k += 4)
		{
			MOTriangleIndecies* t1 = &vertexArrayData.triangles[j];
			MOTriangleIndecies* t2 = &vertexArrayData.triangles[j+1];

			t1->vertexIndices[0] = k;
			t1->vertexIndices[1] = k + 1;
			t1->vertexIndices[2] = k + 2;

			t2->vertexIndices[0] = k;
			t2->vertexIndices[1] = k + 2;
			t2->vertexIndices[2] = k + 3;
		}


				/* CREATE NEW GEOMETRY OBJECT */

		GAME_ASSERT(gParticleGroups[i].geometryObj == NULL);

		gParticleGroups[i].geometryObj = MO_CreateNewObjectOfType(MO_TYPE_GEOMETRY, MO_GEOMETRY_SUBTYPE_VERTEXARRAY, &vertexArrayData);
	}



			/* LOAD SPRITES */

	LoadSpriteGroup(SPRITE_GROUP_PARTICLES, PARTICLE_SObjType_COUNT, "particle");
	BlendAllSpritesInGroup(SPRITE_GROUP_PARTICLES);



		/*************************************************************************/
		/* CREATE DUMMY CUSTOM OBJECT TO CAUSE PARTICLE DRAWING AT THE DESIRED TIME */
		/*************************************************************************/
		//
		// The particles need to be drawn after the fences object, but before any sprite or font objects.
		//

	NewObjectDefinitionType driverDef =
	{
		.genre = CUSTOM_GENRE,
		.slot = PARTICLE_SLOT,
		.moveCall = MoveParticleGroups,
		.scale = 1,
		.flags = STATUS_BIT_KEEPBACKFACES | STATUS_BIT_NOLIGHTING | STATUS_BIT_NOZWRITES,
	};

	ObjNode* obj;
	obj = MakeNewObject(&driverDef);
	obj->CustomDrawFunction = DrawParticleGroup;
}


/******************** DISPOSE PARTICLE SYSTEM **********************/

void DisposeParticleSystem(void)
{
	DeleteAllParticleGroups();

			/* NUKE MEMORY */

	for (int i = 0; i < MAX_PARTICLE_GROUPS; i++)
	{
		GAME_ASSERT(gParticleGroups[i].geometryObj);
		MO_DisposeObjectReference(gParticleGroups[i].geometryObj);
		gParticleGroups[i].geometryObj = NULL;

		GAME_ASSERT(gParticleGroups[i].pool);
		Pool_Free(gParticleGroups[i].pool);
		gParticleGroups[i].pool = NULL;
	}


	DisposeSpriteGroup(SPRITE_GROUP_PARTICLES);

	Pool_Free(gParticleGroupPool);
	gParticleGroupPool = NULL;
}


/******************** DELETE ALL PARTICLE GROUPS *********************/

void DeleteAllParticleGroups(void)
{
	for (int i = 0; i < MAX_PARTICLE_GROUPS; i++)
	{
		DeleteParticleGroup(i);
	}

	GAME_ASSERT(0 == Pool_Size(gParticleGroupPool));
}


/******************* DELETE PARTICLE GROUP ***********************/

static void DeleteParticleGroup(int groupNum)
{
	if (!Pool_IsUsed(gParticleGroupPool, groupNum))
		return;

	gParticleGroups[groupNum].geometryObj->objectData.numMaterials = 0;
	gParticleGroups[groupNum].geometryObj->objectData.numPoints = 0;
	gParticleGroups[groupNum].geometryObj->objectData.numTriangles = 0;

	Pool_ReleaseIndex(gParticleGroupPool, groupNum);
}


#pragma mark -


/********************** NEW PARTICLE GROUP *************************/
//
// INPUT:	type 	=	group type to create
//
// OUTPUT:	group ID#
//

int NewParticleGroup(const NewParticleGroupDefType *def)
{
			/*************************/
			/* SCAN FOR A FREE GROUP */
			/*************************/

	int i = Pool_AllocateIndex(gParticleGroupPool);
	if (i < 0)		// nothing free
		return i;

	ParticleGroupType* pg = &gParticleGroups[i];


		/* INITIALIZE THE GROUP */

	GAME_ASSERT(pg->pool);
	Pool_Reset(pg->pool);						// mark all particles unused

	pg->type					= def->type;
	pg->flags					= def->flags;
	pg->gravity					= def->gravity;
	pg->magnetism				= def->magnetism;
	pg->baseScale				= def->baseScale;
	pg->decayRate				= def->decayRate;
	pg->fadeRate				= def->fadeRate;
	pg->magicNum				= def->magicNum;
	pg->particleTextureNum		= def->particleTextureNum;

	pg->srcBlend				= def->srcBlend;
	pg->dstBlend				= def->dstBlend;

		/**************************************/
		/* INIT THE GROUP'S VERTEX ARRAY DATA */
		/**************************************/
		// Note: most everything was initialized in InitParticleSystem

	MOVertexArrayData* vertexArrayData = &pg->geometryObj->objectData;

	vertexArrayData->numPoints = 0;		// no quads until we call AddParticleToGroup
	vertexArrayData->numTriangles = 0;

	vertexArrayData->numMaterials = 1;
	vertexArrayData->materials[0] = gSpriteGroupList[SPRITE_GROUP_PARTICLES][def->particleTextureNum].materialObject;	// set illegal ref because it is made legit below

	return i;
}


/******************** ADD PARTICLE TO GROUP **********************/
//
// Returns true if particle group was invalid or is full.
//

Boolean AddParticleToGroup(const NewParticleDefType *def)
{
	short group = def->groupNum;

	GAME_ASSERT(group >= 0);
	GAME_ASSERT(group < MAX_PARTICLE_GROUPS);

	ParticleGroupType* pg = &gParticleGroups[group];

	if (!Pool_IsUsed(gParticleGroupPool, group))
	{
		return(true);
	}


			/* SCAN FOR FREE SLOT */

	int p = Pool_AllocateIndex(pg->pool);

			/* NO FREE SLOTS */

	if (p < 0)
		return true;


			/* INIT PARAMETERS */

	pg->alpha[p] = def->alpha;
	pg->scale[p] = def->scale;
	pg->coord[p] = *def->where;
	pg->delta[p] = *def->delta;
	pg->rotZ[p] = def->rotZ;
	pg->rotDZ[p] = def->rotDZ;


			/* SEE IF ATTACH VAPOR TRAIL */

	if (pg->flags & PARTICLE_FLAGS_VAPORTRAIL)
	{
		static OGLColorRGBA color = {1,.5,.5,1};

		color.a = def->alpha;

		pg->vaporTrail[p] = CreateNewVaporTrail(nil, group, VAPORTRAIL_TYPE_COLORSTREAK,
																	def->where, &color, .3, 2.0, 50);
	}


	return(false);
}


/****************** MOVE PARTICLE GROUPS *********************/

static void MoveParticleGroups(ObjNode *theNode)
{
uint32_t	flags;
float		fps = gFramesPerSecondFrac;
float		y,baseScale,oneOverBaseScaleSquared,gravity;
float		decayRate,magnetism,fadeRate;
OGLPoint3D	*coord;
OGLVector3D	*delta;

#pragma unused(theNode)

	int g = Pool_First(gParticleGroupPool);
	while (g >= 0)
	{
		int nextGroupIndex = Pool_Next(gParticleGroupPool, g);

		ParticleGroupType* particleGroup = &gParticleGroups[g];

		baseScale 	= particleGroup->baseScale;					// get base scale
		oneOverBaseScaleSquared = 1.0f/(baseScale*baseScale);
		gravity 	= particleGroup->gravity;					// get gravity
		decayRate 	= particleGroup->decayRate;					// get decay rate
		fadeRate 	= particleGroup->fadeRate;					// get fade rate
		magnetism 	= particleGroup->magnetism;					// get magnetism
		flags 		= particleGroup->flags;

		int n = 0;												// init counter
		int p = Pool_First(particleGroup->pool);
		while (p >= 0)
		{
			int nextParticleIndex = Pool_Next(particleGroup->pool, p);
			Boolean deleteParticle = false;

			n++;													// inc counter
			delta = &particleGroup->delta[p];						// get ptr to deltas
			coord = &particleGroup->coord[p];						// get ptr to coords

						/* ADD GRAVITY */

			delta->y -= gravity * fps;									// add gravity


					/* DO ROTATION */

			particleGroup->rotZ[p] += particleGroup->rotDZ[p] * fps;




			switch (particleGroup->type)
			{
						/* FALLING SPARKS */

				case	PARTICLE_TYPE_FALLINGSPARKS:
						coord->x += delta->x * fps;						// move it
						coord->y += delta->y * fps;
						coord->z += delta->z * fps;
						break;


						/* GRAVITOIDS */
						//
						// Every particle has gravity pull on other particle
						//

				case	PARTICLE_TYPE_GRAVITOIDS:
						for (int q = Pool_Last(particleGroup->pool); q >= 0; q = Pool_Prev(particleGroup->pool, q))
						{
							float		dist,x,z;
							OGLVector3D	v;

							if (p == q)									// don't check against self
								continue;

							x = particleGroup->coord[q].x;
							y = particleGroup->coord[q].y;
							z = particleGroup->coord[q].z;

									/* calc 1/(dist2) */

							if (p < q)									// see if calc or get from buffer
							{
								float dx = coord->x - x;
								float dy = coord->y - y;
								float dz = coord->z - z;
								dist = sqrtf(dx*dx + dy*dy + dz*dz);
								if (dist != 0.0f)
									dist = 1.0f / (dist*dist);

								if (dist > oneOverBaseScaleSquared)		// adjust if closer than radius
									dist = oneOverBaseScaleSquared;

								gGravitoidDistBuffer[p][q] = dist;		// remember it
							}
							else
							{
								dist = gGravitoidDistBuffer[q][p];		// use from buffer
							}

										/* calc vector to particle */

							if (dist != 0.0f)
							{
								x = x - coord->x;
								y = y - coord->y;
								z = z - coord->z;
								FastNormalizeVector(x, y, z, &v);
							}
							else
							{
								v.x = v.y = v.z = 0;
							}

							delta->x += v.x * (dist * magnetism * fps);		// apply gravity to particle
							delta->y += v.y * (dist * magnetism * fps);
							delta->z += v.z * (dist * magnetism * fps);
						}

						coord->x += delta->x * fps;						// move it
						coord->y += delta->y * fps;
						coord->z += delta->z * fps;
						break;
			}

			/*****************/
			/* SEE IF BOUNCE */
			/*****************/

			if (!(flags & PARTICLE_FLAGS_DONTCHECKGROUND))
			{
				y = GetTerrainY(coord->x, coord->z)+10.0f;					// get terrain coord at particle x/z

				if (flags & PARTICLE_FLAGS_BOUNCE)
				{
					if (delta->y < 0.0f)									// if moving down, see if hit floor
					{
						if (coord->y < y)
						{
							coord->y = y;
							delta->y *= -.4f;

							delta->x += gRecentTerrainNormal.x * 300.0f;	// reflect off of surface
							delta->z += gRecentTerrainNormal.z * 300.0f;

							if (flags & PARTICLE_FLAGS_DISPERSEIFBOUNCE)	// see if disperse on impact
							{
								delta->y *= .4f;
								delta->x *= 5.0f;
								delta->z *= 5.0f;
							}
						}
					}
				}

				/***************/
				/* SEE IF GONE */
				/***************/

				else
				{
					if (coord->y < y)									// if hit floor then nuke particle
					{
						deleteParticle = true;
					}
				}
			}



				/* DO SCALE */

			particleGroup->scale[p] -= decayRate * fps;				// shrink it
			if (particleGroup->scale[p] <= 0.0f)					// see if deleteParticle
				deleteParticle = true;

				/* DO FADE */

			particleGroup->alpha[p] -= fadeRate * fps;				// fade it
			if (particleGroup->alpha[p] <= 0.0f)					// see if deleteParticle
				deleteParticle = true;


					/*****************************/
					/* SEE IF UPDATE VAPOR TRAIL */
					/*****************************/

			if (particleGroup->flags & PARTICLE_FLAGS_VAPORTRAIL)
			{
				static OGLColorRGBA color = {.9,1,.8,1};
				short vaporTrailIndex = particleGroup->vaporTrail[p];

				if (vaporTrailIndex != -1)
				{
					if (VerifyVaporTrail(vaporTrailIndex, nil, g))	// if valid then update it
					{
						color.a = particleGroup->alpha[p];
						AddToVaporTrail(&vaporTrailIndex, coord, &color);
					}
					else
						particleGroup->vaporTrail[p] = -1;			// no longer valid
				}
			}

					/* IF GONE THEN RELEASE INDEX */

			if (deleteParticle)
				Pool_ReleaseIndex(particleGroup->pool, p);

					/* NEXT PARTICLE */

			p = nextParticleIndex;
		}

			/* SEE IF GROUP WAS EMPTY, THEN DELETE */

		if (n == 0)
		{
			DeleteParticleGroup(g);
		}

		g = nextGroupIndex;
	}
}


/**************** DRAW PARTICLE GROUPS *********************/

static void DrawParticleGroup(ObjNode *theNode)
{
float				scale,baseScale;
OGLColorRGBA_Byte	*vertexColors;
MOVertexArrayData	*geoData;
OGLPoint3D		v[4],*camCoords;
static const OGLVector3D up = {0,1,0};
OGLBoundingBox	bbox;

#pragma unused(theNode)

	v[0].z = 												// init z's to 0
	v[1].z =
	v[2].z =
	v[3].z = 0;

				/* SETUP ENVIRONTMENT */

	OGL_PushState();

	glEnable(GL_BLEND);
	SetColor4f(1,1,1,1);										// full white & alpha to start with

	camCoords = &gGameViewInfoPtr->cameraPlacement.cameraLocation;

	for (int g = Pool_First(gParticleGroupPool); g >= 0; g = Pool_Next(gParticleGroupPool, g))
	{
		GAME_ASSERT(Pool_IsUsed(gParticleGroupPool, g));

		float	minX,minY,minZ,maxX,maxY,maxZ;

		const ParticleGroupType* pg = &gParticleGroups[g];

		// If we have enough horsepower (gG4), apply ALLAIM to all particles
		Boolean allAim = gG4 || (pg->flags & PARTICLE_FLAGS_ALLAIM);

		geoData = &pg->geometryObj->objectData;			// get pointer to geometry object data
		vertexColors = geoData->colorsByte;				// get pointer to vertex color array
		baseScale = pg->baseScale;						// get base scale

				/********************************/
				/* ADD ALL PARTICLES TO TRIMESH */
				/********************************/

		minX = minY = minZ = 1e9f;									// init bbox
		maxX = maxY = maxZ = -minX;

		int n = 0;
		for (int p = Pool_First(pg->pool); p >= 0; p = Pool_Next(pg->pool, p))
		{
			GAME_ASSERT(Pool_IsUsed(pg->pool, p));

			float			rot;
			OGLMatrix4x4	m;

						/* CREATE VERTEX DATA */

			scale = pg->scale[p] * baseScale;

			v[0].x = -scale;
			v[0].y = scale;

			v[1].x = -scale;
			v[1].y = -scale;

			v[2].x = scale;
			v[2].y = -scale;

			v[3].x = scale;
			v[3].y = scale;


				/* TRANSFORM THIS PARTICLE'S VERTICES & ADD TO TRIMESH */

			const OGLPoint3D* coord = &pg->coord[p];
			if ((n == 0) || allAim)										// only set the look-at matrix for the 1st particle unless we want to force it for all (optimization technique)
				SetLookAtMatrixAndTranslate(&m, &up, coord, camCoords);	// aim at camera & translate
			else
			{
				m.value[M03] = coord->x;								// update just the translate
				m.value[M13] = coord->y;
				m.value[M23] = coord->z;
			}

			rot = pg->rotZ[p];											// get z rotation
			if (rot != 0.0f)											// see if need to apply rotation matrix
			{
				OGLMatrix4x4	rm;

				OGLMatrix4x4_SetRotate_Z(&rm, rot);
				OGLMatrix4x4_Multiply(&rm, &m, &rm);
				OGLPoint3D_TransformArray(&v[0], &rm, &geoData->points[n*4], 4);	// transform w/ rot
			}
			else
				OGLPoint3D_TransformArray(&v[0], &m, &geoData->points[n*4], 4);		// transform no-rot


						/* UPDATE BBOX */

			for (int i = 0; i < 4; i++)
			{
				int j = n*4+i;

				if (geoData->points[j].x < minX) minX = geoData->points[j].x;
				if (geoData->points[j].x > maxX) maxX = geoData->points[j].x;
				if (geoData->points[j].y < minY) minY = geoData->points[j].y;
				if (geoData->points[j].y > maxY) maxY = geoData->points[j].y;
				if (geoData->points[j].z < minZ) minZ = geoData->points[j].z;
				if (geoData->points[j].z > maxZ) maxZ = geoData->points[j].z;
			}

				/* UPDATE COLOR/TRANSPARENCY */

			int temp = n*4;
			for (int i = temp; i < (temp+4); i++)
			{
				vertexColors[i].r =
				vertexColors[i].g =
				vertexColors[i].b = 0xff;
				vertexColors[i].a = (GLubyte)(pg->alpha[p] * 255.0f);		// set transparency alpha
			}

			n++;											// inc particle count
		}

		if (n == 0)											// if no particles, then skip
			continue;

			/* UPDATE FINAL VALUES */

		geoData->numTriangles = n*2;
		geoData->numPoints = n*4;

		bbox.min.x = minX;									// build bbox for culling test
		bbox.min.y = minY;
		bbox.min.z = minZ;
		bbox.max.x = maxX;
		bbox.max.y = maxY;
		bbox.max.z = maxZ;

		if (OGL_IsBBoxVisible(&bbox, nil))						// do cull test on it
		{
			GLint	src,dst;

			src = pg->srcBlend;
			dst = pg->dstBlend;

				/* DRAW IT */

			glBlendFunc(src, dst);								// set blending mode

			if (gGameViewInfoPtr->useFog)
			{
				if (dst == GL_ONE)
					glDisable(GL_FOG);	// fog screws up in this mode??
				else
					glEnable(GL_FOG);
			}

			MO_DrawObject(pg->geometryObj);						// draw geometry
		}
	}

			/* RESTORE MODES */

	SetColor4f(1,1,1,1);										// reset this
	OGL_PopState();
}


/**************** VERIFY PARTICLE GROUP MAGIC NUM ******************/

Boolean VerifyParticleGroupMagicNum(int group, uint32_t magicNum)
{
	return Pool_IsUsed(gParticleGroupPool, group)
		&& gParticleGroups[group].magicNum == magicNum;
}


/************* PARTICLE HIT OBJECT *******************/
//
// INPUT:	inFlags = flags to check particle types against
//

Boolean ParticleHitObject(ObjNode *theNode, uint16_t inFlags)
{
	for (int g = Pool_First(gParticleGroupPool); g >= 0; g = Pool_Next(gParticleGroupPool, g))
	{
		const ParticleGroupType* pg = &gParticleGroups[g];

		if (inFlags && !(inFlags & pg->flags))			// see if check flags
		{
			continue;
		}

		for (int p = Pool_First(pg->pool); p >= 0; p = Pool_Next(pg->pool, p))
		{
			if (pg->alpha[p] < .4f)						// if particle is too decayed, then skip
				continue;

			const OGLPoint3D* coord = &pg->coord[p];
			if (DoSimpleBoxCollisionAgainstObject(coord->y+40.0f,coord->y-40.0f,
												coord->x-40.0f, coord->x+40.0f,
												coord->z+40.0f, coord->z-40.0f,
												theNode))
			{
				return(true);
			}
		}
	}

	return(false);
}

#pragma mark -

/********************* MAKE PUFF ***********************/

void MakePuff(OGLPoint3D *where, float scale, short texNum, GLint src, GLint dst, float decayRate)
{
long					pg;
OGLVector3D				delta;
OGLPoint3D				pt;
float					x,y,z;

			/* white sparks */

	NewParticleGroupDefType groupDef =
	{
		.magicNum				= 0,
		.type					= PARTICLE_TYPE_FALLINGSPARKS,
		.flags					= PARTICLE_FLAGS_BOUNCE|PARTICLE_FLAGS_ALLAIM,
		.gravity				= -80,
		.magnetism				= 0,
		.baseScale				= scale,
		.decayRate				=  -1.5,
		.fadeRate				= decayRate,
		.particleTextureNum		= texNum,
		.srcBlend				= src,
		.dstBlend				= dst,
	};

	pg = NewParticleGroup(&groupDef);

	if (pg != -1)
	{
		x = where->x;
		y = where->y;
		z = where->z;

		for (int i = 0; i < 10; i++)
		{
			pt.x = x + RandomFloat2() * (2.0f * scale);
			pt.y = y + RandomFloat() * 2.0f * scale;
			pt.z = z + RandomFloat2() * (2.0f * scale);

			delta.x = RandomFloat2() * (3.0f * scale);
			delta.y = RandomFloat() * (2.0f  * scale);
			delta.z = RandomFloat2() * (3.0f * scale);


			NewParticleDefType newParticleDef =
			{
				.groupNum	= pg,
				.where		= &pt,
				.delta		= &delta,
				.scale		= 1.0f + RandomFloat2() * .2f,
				.rotZ		= RandomFloat() * PI2,
				.rotDZ		= RandomFloat2() * 4.0f,
				.alpha		= FULL_ALPHA,
			};
			AddParticleToGroup(&newParticleDef);
		}
	}
}


/********************* MAKE SPARK EXPLOSION ***********************/

void MakeSparkExplosion(float x, float y, float z, float force, float scale, short sparkTexture, short quantityLimit)
{
long					pg,n;
OGLVector3D				delta,v;
OGLPoint3D				pt;

	n = force * .2f;

	if (quantityLimit)
		if (n > quantityLimit)
			n = quantityLimit;


			/* white sparks */

	NewParticleGroupDefType groupDef =
	{
		.magicNum				= 0,
		.type					= PARTICLE_TYPE_FALLINGSPARKS,
		.flags					= PARTICLE_FLAGS_BOUNCE|PARTICLE_FLAGS_ALLAIM,
		.gravity				= 200,
		.magnetism				= 0,
		.baseScale				= 15.0f * scale,
		.decayRate				=  0,
		.fadeRate				= .5,
		.particleTextureNum		= sparkTexture,
		.srcBlend				= GL_SRC_ALPHA,
		.dstBlend				= GL_ONE,
	};

	pg = NewParticleGroup(&groupDef);

	if (pg != -1)
	{
		for (int i = 0; i < n; i++)
		{
			pt.x = x + RandomFloat2() * (40.0f * scale);
			pt.y = y + RandomFloat2() * (40.0f * scale);
			pt.z = z + RandomFloat2() * (40.0f * scale);

			v.x = pt.x - x;
			v.y = pt.y - y;
			v.z = pt.z - z;
			FastNormalizeVector(v.x,v.y,v.z,&v);

			delta.x = v.x * (force * scale);
			delta.y = v.y * (force * scale);
			delta.z = v.z * (force * scale);


			NewParticleDefType newParticleDef =
			{
				.groupNum	= pg,
				.where		= &pt,
				.delta		= &delta,
				.scale		= 1.0f + RandomFloat()  * .5f,
				.rotZ		= 0,
				.rotDZ		= 0,
				.alpha		= FULL_ALPHA,
			};

			if (AddParticleToGroup(&newParticleDef))
				break;
		}
	}
}

/****************** MAKE STEAM ************************/

void MakeSteam(ObjNode *blob, float x, float y, float z)
{
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum;
OGLVector3D			d;
OGLPoint3D			p;

	if (gFramesPerSecond < 20.0f)
		return;

	blob->ParticleTimer -= fps;
	if (blob->ParticleTimer <= 0.0f)
	{
		blob->ParticleTimer += 0.02f;


		particleGroup 	= blob->ParticleGroup;
		magicNum 		= blob->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			blob->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			NewParticleGroupDefType groupDef =
			{
				.magicNum				= magicNum,
				.type					= PARTICLE_TYPE_GRAVITOIDS,
				.flags					= PARTICLE_FLAGS_DONTCHECKGROUND,
				.gravity				= 0,
				.magnetism				= 90000,
				.baseScale				= 15,
				.decayRate				= -.5,
				.fadeRate				= .4,
				.particleTextureNum		= PARTICLE_SObjType_GreySmoke,
				.srcBlend				= GL_SRC_ALPHA,
				.dstBlend				= GL_ONE_MINUS_SRC_ALPHA,
			};
			blob->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			for (int i = 0; i < 2; i++)
			{
				p.x = x + RandomFloat2() * (20.0f);
				p.y = y + RandomFloat2() * (20.0f);
				p.z = z + RandomFloat2() * (20.0f);

				d.x = RandomFloat2() * 190.0f;
				d.y = 150.0f + RandomFloat() * 190.0f;
				d.z = RandomFloat2() * 190.0f;

				NewParticleDefType newParticleDef =
				{
					.groupNum	= particleGroup,
					.where		= &p,
					.delta		= &d,
					.scale		= RandomFloat() + 1.0f,
					.rotZ		= RandomFloat()*PI2,
					.rotDZ		= 0,
					.alpha		= .5,
				};
				if (AddParticleToGroup(&newParticleDef))
				{
					blob->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}



#pragma mark -


/************** MAKE BOMB EXPLOSION *********************/

void MakeBombExplosion(float x, float z, OGLVector3D *delta)
{
long					pg;
OGLVector3D				d;
OGLPoint3D				pt;
float					y;
float					dx,dz;
OGLPoint3D				where;

	where.x = x;
	where.z = z;
	where.y = GetTerrainY(x,z);

		/*********************/
		/* FIRST MAKE SPARKS */
		/*********************/

	NewParticleGroupDefType groupDef =
	{
		.magicNum				= 0,
		.type					= PARTICLE_TYPE_FALLINGSPARKS,
		.flags					= PARTICLE_FLAGS_VAPORTRAIL|PARTICLE_FLAGS_BOUNCE,
		.gravity				= 900,
		.magnetism				= 0,
		.baseScale				= 190,
		.decayRate				=  .4,
		.fadeRate				= .7,
		.particleTextureNum		= PARTICLE_SObjType_WhiteSpark3,
		.srcBlend				= GL_SRC_ALPHA,
		.dstBlend				= GL_ONE,
	};

	pg = NewParticleGroup(&groupDef);

	if (pg != -1)
	{
		x = where.x;
		y = where.y;
		z = where.z;
		dx = delta->x * .2f;
		dz = delta->z * .2f;

		for (int i = 0; i < 20; i++)
		{
			pt.x = x + (RandomFloat()-.5f) * 200.0f;
			pt.y = y + RandomFloat() * 60.0f;
			pt.z = z + (RandomFloat()-.5f) * 200.0f;

			d.y = RandomFloat() * 1500.0f;
			d.x = (RandomFloat()-.5f) * d.y * 3.0f + dx;
			d.z = (RandomFloat()-.5f) * d.y * 3.0f + dz;

			NewParticleDefType newParticleDef =
			{
				.groupNum	= pg,
				.where		= &pt,
				.delta		= &d,
				.scale		= RandomFloat() + 1.5f,
				.rotZ		= 0,
				.rotDZ		= 0,
				.alpha		= FULL_ALPHA + (RandomFloat() * .3f),
			};
			AddParticleToGroup(&newParticleDef);
		}
	}


}




#pragma mark -

/********************* MAKE SPLASH ***********************/

void MakeSplash(float x, float y, float z)
{
long	pg;
OGLVector3D	delta;
OGLPoint3D	pt;

	pt.y = y;

	NewParticleGroupDefType groupDef =
	{
		.magicNum				= 0,
		.type					= PARTICLE_TYPE_FALLINGSPARKS,
		.flags					= 0,
		.gravity				= 800,
		.magnetism				= 0,
		.baseScale				= 40,
		.decayRate				=  -.6,
		.fadeRate				= .8,
		.particleTextureNum		= PARTICLE_SObjType_Splash,
		.srcBlend				= GL_SRC_ALPHA,
		.dstBlend				= GL_ONE,
	};

	pg = NewParticleGroup(&groupDef);

	if (pg != -1)
	{
		for (int i = 0; i < 30; i++)
		{
			pt.x = x + RandomFloat2() * 130.0f;
			pt.z = z + RandomFloat2() * 130.0f;

			delta.x = RandomFloat2() * 600.0f;
			delta.y = 100.0f + RandomFloat() * 300.0f;
			delta.z = RandomFloat2() * 600.0f;

			NewParticleDefType newParticleDef =
			{
				.groupNum	= pg,
				.where		= &pt,
				.delta		= &delta,
				.scale		= RandomFloat() + 1.0f,
				.rotZ		= 0,
				.rotDZ		= 0,
				.alpha		= FULL_ALPHA,
			};
			AddParticleToGroup(&newParticleDef);
		}
	}

			/* PLAY SPLASH SOUND */

	pt.x = x;
	pt.z = z;
//	PlayEffect_Parms3D(EFFECT_SPLASH, &pt, NORMAL_CHANNEL_RATE, 3);
}


/***************** SPRAY WATER *************************/

void SprayWater(ObjNode *theNode, float x, float y, float z)
{
float	fps = gFramesPerSecondFrac;
int		particleGroup, magicNum;

	theNode->ParticleTimer += fps;				// see if time to spew water
	if (theNode->ParticleTimer > 0.02f)
	{
		theNode->ParticleTimer += .02f;

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->SmokeParticleMagic = magicNum = MyRandomLong();			// generate a random magic num

			NewParticleGroupDefType groupDef =
			{
				.magicNum				= magicNum,
				.type					= PARTICLE_TYPE_FALLINGSPARKS,
				.flags					= 0,
				.gravity				= 1400,
				.magnetism				= 0,
				.baseScale				= 35,
				.decayRate				=  -1.7f,
				.fadeRate				= 1.0,
				.particleTextureNum		= PARTICLE_SObjType_Splash,
				.srcBlend				= GL_SRC_ALPHA,
				.dstBlend				= GL_ONE,
			};
			theNode->SmokeParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}


					/* ADD PARTICLE TO GROUP */

		if (particleGroup != -1)
		{
			OGLPoint3D	pt;
			OGLVector3D delta;

			FastNormalizeVector(gDelta.x, 0, gDelta.z, &delta);			// calc spray delta shooting out of butt
			delta.x *= -300.0f;
			delta.z *= -300.0f;

			delta.x += (RandomFloat()-.5f) * 50.0f;						// spray delta
			delta.z += (RandomFloat()-.5f) * 50.0f;
			delta.y = 500.0f + RandomFloat() * 100.0f;

			pt.x = x + (RandomFloat()-.5f) * 80.0f;			// random noise to coord
			pt.y = y;
			pt.z = z + (RandomFloat()-.5f) * 80.0f;


			NewParticleDefType		newParticleDef =
			{
				.groupNum	= particleGroup,
				.where		= &pt,
				.delta		= &delta,
				.scale		= RandomFloat() + 1.0f,
				.rotZ		= 0,
				.rotDZ		= 0,
				.alpha		= FULL_ALPHA,
			};
			AddParticleToGroup(&newParticleDef);
		}
	}
}





#pragma mark -



/****************** BURN FIRE ************************/

void BurnFire(ObjNode *theNode, float x, float y, float z, Boolean doSmoke, short particleType, float scale, uint32_t moreFlags)
{
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum;
OGLVector3D			d;
OGLPoint3D			p;


		/**************/
		/* MAKE SMOKE */
		/**************/

	if (doSmoke && (gFramesPerSecond > 20.0f))										// only do smoke if running at good frame rate
	{
		theNode->SmokeTimer -= fps;													// see if add smoke
		if (theNode->SmokeTimer <= 0.0f)
		{
			theNode->SmokeTimer += SMOKE_TIMER;										// reset timer

			particleGroup 	= theNode->SmokeParticleGroup;
			magicNum 		= theNode->SmokeParticleMagic;

			if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
			{

				theNode->SmokeParticleMagic = magicNum = MyRandomLong();			// generate a random magic num

				NewParticleGroupDefType groupDef =
				{
					.magicNum				= magicNum,
					.type					= PARTICLE_TYPE_FALLINGSPARKS,
					.flags					= PARTICLE_FLAGS_DONTCHECKGROUND|moreFlags,
					.gravity				= 0,
					.magnetism				= 0,
					.baseScale				= 20 * scale,
					.decayRate				=  -.2f,
					.fadeRate				= .2,
					.particleTextureNum		= PARTICLE_SObjType_BlackSmoke,
					.srcBlend				= GL_SRC_ALPHA,
					.dstBlend				= GL_ONE_MINUS_SRC_ALPHA,
				};
				theNode->SmokeParticleGroup = particleGroup = NewParticleGroup(&groupDef);
			}

			if (particleGroup != -1)
			{
				for (int i = 0; i < 3; i++)
				{
					p.x = x + RandomFloat2() * (40.0f * scale);
					p.y = y + 200.0f + RandomFloat() * (50.0f * scale);
					p.z = z + RandomFloat2() * (40.0f * scale);

					d.x = RandomFloat2() * (20.0f * scale);
					d.y = 150.0f + RandomFloat() * (40.0f * scale);
					d.z = RandomFloat2() * (20.0f * scale);

					NewParticleDefType newParticleDef =
					{
						.groupNum	= particleGroup,
						.where		= &p,
						.delta		= &d,
						.scale		= RandomFloat() + 1.0f,
						.rotZ		= RandomFloat() * PI2,
						.rotDZ		= RandomFloat2(),
						.alpha		= .7,
					};
					if (AddParticleToGroup(&newParticleDef))
					{
						theNode->SmokeParticleGroup = -1;
						break;
					}
				}
			}
		}
	}

		/*************/
		/* MAKE FIRE */
		/*************/

	theNode->FireTimer -= fps;													// see if add fire
	if (theNode->FireTimer <= 0.0f)
	{
		theNode->FireTimer += FIRE_TIMER;										// reset timer

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			NewParticleGroupDefType groupDef =
			{
				.magicNum				= magicNum,
				.type					= PARTICLE_TYPE_FALLINGSPARKS,
				.flags					= PARTICLE_FLAGS_DONTCHECKGROUND|moreFlags,
				.gravity				= -200,
				.magnetism				= 0,
				.baseScale				= 30.0f * scale,
				.decayRate				=  0,
				.fadeRate				= .8,
				.particleTextureNum		= particleType,
				.srcBlend				= GL_SRC_ALPHA,
				.dstBlend				= GL_ONE,
			};
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			for (int i = 0; i < 3; i++)
			{
				p.x = x + RandomFloat2() * (30.0f * scale);
				p.y = y + RandomFloat() * (50.0f * scale);
				p.z = z + RandomFloat2() * (30.0f * scale);

				d.x = RandomFloat2() * (50.0f * scale);
				d.y = 50.0f + RandomFloat() * (60.0f * scale);
				d.z = RandomFloat2() * (50.0f * scale);

				NewParticleDefType newParticleDef =
				{
					.groupNum	= particleGroup,
					.where		= &p,
					.delta		= &d,
					.scale		= RandomFloat() + 1.0f,
					.rotZ		= RandomFloat() * PI2,
					.rotDZ		= RandomFloat2(),
					.alpha		= 1.0,
				};
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}






#pragma mark -

/************** MAKE FIRE EXPLOSION *********************/

void MakeFireExplosion(float x, float z, OGLVector3D *delta)
{
long					pg;
OGLVector3D				d;
OGLPoint3D				pt;
float					y;
float					dx,dz;
OGLPoint3D				where;

	where.x = x;
	where.z = z;
	where.y = GetTerrainY(x,z);

		/*********************/
		/* FIRST MAKE FLAMES */
		/*********************/

	NewParticleGroupDefType groupDef =
	{
		.magicNum				= 0,
		.type					= PARTICLE_TYPE_FALLINGSPARKS,
		.flags					= PARTICLE_FLAGS_DONTCHECKGROUND,
		.gravity				= -500,
		.magnetism				= 0,
		.baseScale				= 30,
		.decayRate				=  -.4,
		.fadeRate				= .6,
		.particleTextureNum		= PARTICLE_SObjType_Fire,
		.srcBlend				= GL_SRC_ALPHA,
		.dstBlend				= GL_ONE,
	};

	pg = NewParticleGroup(&groupDef);

	if (pg != -1)
	{
		x = where.x;
		y = where.y;
		z = where.z;
		dx = delta->x * .2f;
		dz = delta->z * .2f;


		for (int i = 0; i < 100; i++)
		{
			pt.x = x + (RandomFloat()-.5f) * 200.0f;
			pt.y = y + RandomFloat() * 60.0f;
			pt.z = z + (RandomFloat()-.5f) * 200.0f;

			d.y = RandomFloat() * 1000.0f;
			d.x = (RandomFloat()-.5f) * d.y * 3.0f + dx;
			d.z = (RandomFloat()-.5f) * d.y * 3.0f + dz;

			NewParticleDefType newParticleDef =
			{
				.groupNum	= pg,
				.where		= &pt,
				.delta		= &d,
				.scale		= RandomFloat() + 1.5f,
				.rotZ		= 0,
				.rotDZ		= RandomFloat2() * 10.0f,
				.alpha		= FULL_ALPHA + (RandomFloat() * .3f),
			};
			AddParticleToGroup(&newParticleDef);
		}
	}
}




#pragma mark -

/************************* START DEATH EXIT *****************************/
//
// Starts the animation to "fade" out the screen after player's death
//

void StartDeathExit(float delay)
{
	gDoDeathExit 		= true;
	gDeathExitX 		= -20;
	gDeathExitMode 		= DEATH_EXIT_MODE_CLOSE;
	gDeathExitDelay 	= delay;
}


/********************** DRAW DEATH EXIT **************************/

void DrawDeathExit(void)
{
float	s = 2.0;

	if (!gDoDeathExit)
		return;

	gDeathExitDelay -= gFramesPerSecondFrac;					// see if still in delay
	if (gDeathExitDelay > 0.0f)
		return;

		/***********/
		/* MOVE IT */
		/***********/

	switch(gDeathExitMode)
	{
		case	DEATH_EXIT_MODE_CLOSE:
				gDeathExitX += gFramesPerSecondFrac * 300.0f;
				if (gDeathExitX > 320.0f)
				{
					gDeathExitX = 320.0f;

					if (gPlayerInfo.lives == 0)				// see if any lives left, or if we end now
					{
						gDeathExitMode = DEATH_EXIT_MODE_STAY;
						gGameOver = true;
					}
					else
					{
						gDeathExitMode = DEATH_EXIT_MODE_OPEN;
						gPlayerInfo.lives--;				// dec # lives
						ResetPlayerAtBestCheckpoint();
					}
				}
				break;

		case	DEATH_EXIT_MODE_OPEN:
				gDeathExitX -= gFramesPerSecondFrac * 300.0f;
				if (gDeathExitX < -20.0f)
					gDoDeathExit = 0.0f;
				break;

	}


		/*************/
		/* SET STATE */
		/*************/

	OGL_PushState();

	if (gGameViewInfoPtr->useFog)
		glDisable(GL_FOG);
	OGL_DisableLighting();
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);								// no z-buffer

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 640, 480, 0, 0, 1);		// this one must NOT use g2DLogicalWidth
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


		/* DRAW LEFT */

	glTranslatef(gDeathExitX, 0, 0);
	glScalef(s,-s,s);
	MO_DrawObject(gBG3DGroupList[MODEL_GROUP_GLOBAL][GLOBAL_ObjType_DeathExitLeft]);


		/* DRAW RIGHT */

	glLoadIdentity();
	glTranslatef(640-gDeathExitX, 0, 0);
	glScalef(s,-s,s);
	MO_DrawObject(gBG3DGroupList[MODEL_GROUP_GLOBAL][GLOBAL_ObjType_DeathExitRight]);


			/***********/
			/* CLEANUP */
			/***********/

	OGL_PopState();
}



#pragma mark -


/********************** MAKE SPLATTER ************************/

void MakeSplatter(OGLPoint3D *where, short modelObjType)
{
OGLVector3D	aim;
int		i;
ObjNode	*newObj;

	for (i = 0; i < 14; i++)
	{
				/* RANDOM AIM TO START WITH */

		aim.x = RandomFloat2();
		aim.y = RandomFloat() * 3.0f;
		aim.z = RandomFloat2();
		FastNormalizeVector(aim.x, aim.y, aim.z, &aim);

		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= modelObjType;
		gNewObjectDefinition.coord.x	= where->x + aim.x * 50.0f;
		gNewObjectDefinition.coord.y	= where->y + aim.y * 50.0f;
		gNewObjectDefinition.coord.z	= where->z + aim.z * 50.0f;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_USEALIGNMENTMATRIX;
		gNewObjectDefinition.slot 		= 479;
		gNewObjectDefinition.moveCall 	= MoveBlobDroplet;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= .4f + RandomFloat();
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Special[0] = 0;						// init counter

			/* SET DELTAS */

		newObj->Delta.x = aim.x * 800.0f;
		newObj->Delta.y = aim.y * 800.0f;
		newObj->Delta.z = aim.z * 800.0f;

				/* SET THE ALIGNMENT MATRIX */

		SetAlignmentMatrix(&newObj->AlignmentMatrix, &aim);


			/* SET COLLISION */

		newObj->Damage 			= .1;
		newObj->CType 			= CTYPE_HURTME;
		newObj->CBits			= CBITS_TOUCHABLE;

		CreateCollisionBoxFromBoundingBox_Maximized(newObj);
	}

}


/******************* MOVE BLOB DROPLET ********************/

static void MoveBlobDroplet(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
OGLVector3D	aim;


	GetObjectInfo(theNode);

			/* MOVE IT */

	gDelta.y -= 1600.0f * fps;
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

			/* SHRINK */

	theNode->Scale.x =
	theNode->Scale.y =
	theNode->Scale.z -= fps * .7f;
	if (theNode->Scale.x <= 0.0f)
	{
gone:
		DeleteObject(theNode);
		return;
	}


		/***********************/
		/* SEE IF HIT ANYTHING */
		/***********************/

	if (HandleCollisions(theNode, CTYPE_MISC|CTYPE_TERRAIN|CTYPE_FENCE, -.5f))
	{
		theNode->Special[0]++;
		if (theNode->Special[0] > 3)
		{
			ExplodeGeometry(theNode, 200, SHARD_MODE_FROMORIGIN, 3, .5);
			goto gone;
		}
	}


		/* SET NEW ALIGNMENT & UPDATE */


	FastNormalizeVector(gDelta.x, gDelta.y, gDelta.z, &aim);
	SetAlignmentMatrix(&theNode->AlignmentMatrix, &aim);
	UpdateObject(theNode);
}


#pragma mark -

/******************** ADD SMOKER ****************************/

Boolean AddSmoker(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= FindHighestCollisionAtXZ(x,z, CTYPE_TERRAIN | CTYPE_WATER);
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+10;
	gNewObjectDefinition.moveCall 	= MoveSmoker;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Kind = itemPtr->parm[0];								// save smoke kind

	return(true);
}



/******************** MOVE SMOKER ************************/

static void MoveSmoker(ObjNode *theNode)
{
float				fps = gFramesPerSecondFrac;
int					particleGroup,magicNum;
OGLVector3D			d;
OGLPoint3D			p;
short				smokeType;

static const short	textures[] =
{
	PARTICLE_SObjType_GreySmoke,
	PARTICLE_SObjType_BlackSmoke,
	PARTICLE_SObjType_RedFumes,
	PARTICLE_SObjType_GreenFumes
};

static const Boolean	glow[] =
{
	false,
	false,
	true,
	true
};

		/* SEE IF OUT OF RANGE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}


	theNode->Coord.y = GetTerrainY(theNode->Coord.x, theNode->Coord.z);		// make sure on ground (for when volcanos grow over it)


		/**************/
		/* MAKE SMOKE */
		/**************/

	theNode->Timer -= fps;													// see if add smoke
	if (theNode->Timer <= 0.0f)
	{
		theNode->Timer += .1f;												// reset timer

		smokeType = theNode->Kind;
		GAME_ASSERT(smokeType >= 0);
		GAME_ASSERT(smokeType < 4);

		int t = textures[smokeType];										// get texture #

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			NewParticleGroupDefType groupDef =
			{
				.magicNum				= magicNum,
				.type					= PARTICLE_TYPE_FALLINGSPARKS,
				.flags					= PARTICLE_FLAGS_DONTCHECKGROUND,
				.gravity				= 170,
				.magnetism				= 0,
				.baseScale				= 20,
				.decayRate				= -.7,
				.fadeRate				= .2,
				.particleTextureNum		= t,
				.srcBlend				= GL_SRC_ALPHA,
				.dstBlend				= glow[smokeType] ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA,
			};

			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			float				x,y,z;

			x = theNode->Coord.x;
			y = theNode->Coord.y;
			z = theNode->Coord.z;

			for (int i = 0; i < 2; i++)
			{
				p.x = x + RandomFloat2() * 30.0f;
				p.y = y;
				p.z = z + RandomFloat2() * 30.0f;

				d.x = RandomFloat2() * 80.0f;
				d.y = 150.0f + RandomFloat() * 75.0f;
				d.z = RandomFloat2() * 80.0f;

				NewParticleDefType newParticleDef =
				{
					.groupNum	= particleGroup,
					.where		= &p,
					.delta		= &d,
					.scale		= RandomFloat() + 1.0f,
					.rotZ		= RandomFloat()*PI2,
					.rotDZ		= RandomFloat2() * .1f,
					.alpha		= .6f,
				};
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}

