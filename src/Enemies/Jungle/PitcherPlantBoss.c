/****************************/
/*   PITCHER PLANET BOSS.C	*/
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

static void MoveTentacleGenerator(ObjNode *theNode);
static Boolean StartTentacle(ObjNode *generator);
static void UpdateTentacles(ObjNode *theNode);
static void DrawTentacles(ObjNode *theNode);
static Boolean TentacleGeneratorHitByFire(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void KillTentacleGenerator(ObjNode *theNode);

static void MovePitcherPlant(ObjNode *theNode);
static void  MovePitcherPlant_Wait(ObjNode *theNode);
static void MovePitcherPod(ObjNode *theNode);
static void MakePitcherPodSteam(ObjNode *theNode);
static void PitcherPodShoot(ObjNode *pod);
static void MovePollenSpore(ObjNode *spore);
static Boolean PitcherPodHitByFire(ObjNode *weapon, ObjNode *pod, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void  MovePitcherPlant_Attack(ObjNode *theNode);
static void MovePitcherPlant_Burning(ObjNode *theNode);
static void  MovePitcherPlant_GotHit(ObjNode *theNode);
static Boolean PitcherPlantHitByFire(ObjNode *weapon, ObjNode *plant, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);

static void InitTractorBeam(void);
static void MoveTractorBeam(ObjNode *);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	POD_MOUTH_OFF_Y		-93.0f
#define	POD_MOUTH_OFF_Z		71.0f


#define	TENTACLE_ATTACK_DIST	3500.0f

#define	MAX_TENTACLES		5

#define	MAX_SPLINE_POINTS	380

#define	SKIP_FACTOR			5

#define	MAX_TENTACLE_SEGMENTS	(MAX_SPLINE_POINTS/SKIP_FACTOR)
#define NUM_RING_POINTS			8

#define	TENTACLE_RADIUS		30.0f

#define	NUM_POD_PLANTS_TO_KILL	6

enum
{
	TENTACLE_GEN_MODE_ALIVE,
	TENTACLE_GEN_MODE_DEAD
};


enum
{
	TENTACLE_MODE_GROW,
	TENTACLE_MODE_AWAY
};

typedef struct
{
	Boolean		isUsed;
	Byte		mode;

	ObjNode		*generator;
	OGLPoint3D	base,tipCoord;

	float		rot;
	float		cosIndex;
	float		growTimer;
	float		delayToGoAway;

	int			numPoints;
	OGLPoint3D	splinePoints[MAX_SPLINE_POINTS];

}TentacleType;


#define	PITCHER_PLANT_SCALE	9.5f

enum
{
	PITCHER_ANIM_WAIT,
	PITCHER_ANIM_ATTACK,
	PITCHER_ANIM_GOTHIT
};


/*********************/
/*    VARIABLES      */
/*********************/

static	int	gNumTentacles;

static	TentacleType	gTentacles[MAX_TENTACLES];

#define	HasTentacle		Flag[0]
#define TentacleIndex	Special[0]

#define	Vulnerable	Flag[0]
#define	ShootNow	Flag[1]

#define	PodBlown	Flag[0]

static OGLPoint3D			gTentaclePoints[MAX_TENTACLE_SEGMENTS * NUM_RING_POINTS];
static OGLVector3D			gTentacleNormals[MAX_TENTACLE_SEGMENTS * NUM_RING_POINTS];
static OGLTextureCoord		gTentacleUVs[MAX_TENTACLE_SEGMENTS * NUM_RING_POINTS];
static MOTriangleIndecies	gTentacleTriangles[MAX_TENTACLE_SEGMENTS * (NUM_RING_POINTS-1) * 2];

static MOVertexArrayData	gTentacleMesh;

static	OGLPoint3D			gRingPoints[NUM_RING_POINTS];
static	OGLVector3D			gRingNormals[NUM_RING_POINTS];

static	short	gNumDestroyedPods;

ObjNode		*gTractorBeamObj;


/******************* INIT JUNGLE BOSS STUFF **************************/

void InitJungleBossStuff(void)
{
int	i;
float	r;
ObjNode	*newObj;

	InitTractorBeam();

	gNumDestroyedPods = 0;						// no pods have been destroyed


				/******************/
				/* INIT TENTACLES */
				/******************/

	gNumTentacles = 0;

	for (i = 0; i < MAX_TENTACLES; i++)
		gTentacles[i].isUsed = false;

			/* INIT MESH DATA */

	gTentacleMesh.numMaterials 	= 1;
	gTentacleMesh.materials[0] 	= gSpriteGroupList[SPRITE_GROUP_LEVELSPECIFIC][JUNGLE_SObjType_Vine].materialObject;	// set illegal ref to material
	gTentacleMesh.points 		= gTentaclePoints;
	gTentacleMesh.triangles 	= gTentacleTriangles;
	gTentacleMesh.normals		= gTentacleNormals;
	gTentacleMesh.uvs[0]		= gTentacleUVs;
	gTentacleMesh.colorsByte	= nil;
	gTentacleMesh.colorsFloat	= nil;

	SetSphereMapInfoOnVertexArrayData(&gTentacleMesh, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_GreenSheen);


			/* INIT RING DATA */

	r = 0;
	for (i = 0; i < NUM_RING_POINTS; i++)
	{
		gRingNormals[i].x = sin(r);									// set vector
		gRingNormals[i].z = cos(r);
		gRingNormals[i].y = 0;

		gRingPoints[i].x = gRingNormals[i].x * TENTACLE_RADIUS;		// set ring point
		gRingPoints[i].z = gRingNormals[i].z * TENTACLE_RADIUS;
		gRingPoints[i].y = 0;

		r += PI2 / (NUM_RING_POINTS-1);
	}

			/* MAKE DRAW EVENT */

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+7;
	gNewObjectDefinition.moveCall 	= UpdateTentacles;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->CustomDrawFunction = DrawTentacles;
}


/*********************** ADD TENTACLE GENERATOR ***************************/

Boolean AddTentacleGenerator(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
uint32_t	burned = itemPtr->flags & ITEM_FLAGS_USER1;


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= JUNGLE_ObjType_TentacleGenerator;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
	gNewObjectDefinition.slot 		= 140;
	gNewObjectDefinition.moveCall 	= MoveTentacleGenerator;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 5.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list



				/* SET DEFAULT COLLISION INFO */

	newObj->CType = CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_AUTOTARGETWEAPON;
	newObj->CBits = CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj, 1,1);

	newObj->WeaponAutoTargetOff.y = 300.0f;

	if (burned)														// see if burning
		KillTentacleGenerator(newObj);
	else
	{
		newObj->Mode = TENTACLE_GEN_MODE_ALIVE;
		newObj->HasTentacle = false;
		newObj->Timer = RandomFloat() * 3.0f;
		newObj->HitByWeaponHandler[WEAPON_TYPE_FLAME] = TentacleGeneratorHitByFire;
	}

	return(true);
}


/*********************** MOVE TENTACLE GENERATOR *************************/

static void MoveTentacleGenerator(ObjNode *theNode)
{
	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	if (!gPlayerHasLanded)									// no tentacle attack until player has landed
		return;

	switch(theNode->Mode)
	{
		case	TENTACLE_GEN_MODE_ALIVE:
				if (!theNode->HasTentacle)
				{
					theNode->Timer -= gFramesPerSecondFrac;				// check delay
					if (theNode->Timer < 0.0f)
					{

								/* IF PLAYER CLOSE ENOUGH THEN START NEW TENTACLE */

						if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < TENTACLE_ATTACK_DIST)
						{
							StartTentacle(theNode);
							theNode->Timer = RandomFloat() * 4.0f;
						}
					}
				}
				break;

		case	TENTACLE_GEN_MODE_DEAD:
				BurnFire(theNode, theNode->Coord.x, theNode->Coord.y, theNode->Coord.z, true, PARTICLE_SObjType_Fire, 4.0, 0);
				break;
	}
}


/************ TENTACLE GENERATOR HIT BY FIRE *****************/

static Boolean TentacleGeneratorHitByFire(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	if (enemy->Mode != TENTACLE_GEN_MODE_DEAD)				// dont do anything if already dead
	{
		enemy->ColorFilter.r -= .3f;
		enemy->ColorFilter.g -= .35f;
		enemy->ColorFilter.b -= .35f;

		if (enemy->ColorFilter.b <= 0.1f)
		{
			KillTentacleGenerator(enemy);
		}
	}

	return(true);			// stop weapon
}


/********************* KILL TENTACLE GENERATOR ***********************/

static void KillTentacleGenerator(ObjNode *theNode)
{
	theNode->ColorFilter.r = .15f;
	theNode->ColorFilter.g = .1f;
	theNode->ColorFilter.b = .1f;

	theNode->Mode = TENTACLE_GEN_MODE_DEAD;
	theNode->CType &= ~CTYPE_AUTOTARGETWEAPON;
	if (theNode->HasTentacle)							// make tentacle retreat
		gTentacles[theNode->TentacleIndex].mode = TENTACLE_MODE_AWAY;

	theNode->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;	// when it comes back, it's burned
}


/*********************** START TENTACLE ***************************/

static Boolean StartTentacle(ObjNode *generator)
{
int		i;

			/* FIND FREE TENTACLE SLOT */

	for (i = 0; i < MAX_TENTACLES; i++)
	{
		if (!gTentacles[i].isUsed)
			goto got_it;

	}
	return(false);

got_it:

	gTentacles[i].isUsed 	= true;								// mark this slot as used
	gTentacles[i].mode		= TENTACLE_MODE_GROW;

	gTentacles[i].generator	= generator;
	gTentacles[i].base 		= generator->Coord;					// set base coord
	gTentacles[i].tipCoord 	= generator->Coord;
	gTentacles[i].rot		= RandomFloat() * PI2;				// set random aim
	gTentacles[i].cosIndex	= -PI;

	gTentacles[i].growTimer = 0;
	gTentacles[i].delayToGoAway = 1.0;							// # seconds to stay @ full size before going away

	gTentacles[i].numPoints = 1;								// set 1st point
	gTentacles[i].splinePoints[0] = generator->Coord;

	generator->HasTentacle = true;
	generator->TentacleIndex = i;

	return(true);
}


/********************** UPDATE TENTACLES ***************************/

static void UpdateTentacles(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	fps2;
ObjNode	*player = gPlayerInfo.objNode;

#pragma unused (theNode)

	for (int i = 0; i < MAX_TENTACLES; i++)
	{
		if (!gTentacles[i].isUsed)													// see if skip this slot
			continue;

		switch(gTentacles[i].mode)
		{
					/**********************/
					/* MAKE TENTACLE GROW */
					/**********************/

			case	TENTACLE_MODE_GROW:

					fps2 = fps * .2f;
					for (int q = 0; q < 5; q++)									// do this several times to increase the resolution nicely
					{
							/* UPDATE GROWTH */

						int n = gTentacles[i].numPoints;						// get # points
						if (n >= MAX_SPLINE_POINTS)								// see if list is full
						{
							gTentacles[i].mode = TENTACLE_MODE_AWAY;			// make go away
							goto next_i;
						}

						gTentacles[i].cosIndex += fps2 * 4.0f;						// update the cosing y-index

						TurnPointTowardPoint(&gTentacles[i].rot, &gTentacles[i].splinePoints[n-1], gPlayerInfo.coord.x, gPlayerInfo.coord.z, 1.9f * fps2);	// rotate toward player

							/* CALC A NEW POINT TOWARD ME */

						float r = gTentacles[i].rot;
						gTentacles[i].tipCoord.x -= sin(r) * (400.0f * fps2);
						gTentacles[i].tipCoord.z -= cos(r) * (400.0f * fps2);
						gTentacles[i].tipCoord.y = 60.0f + GetTerrainY(gTentacles[i].tipCoord.x,gTentacles[i].tipCoord.z) + cos(gTentacles[i].cosIndex) * 40.0f;


							/* CHECK GROW TIMER TO SEE IF SHOULD ADD THE NEW POINT */

						gTentacles[i].growTimer -= fps2;
						if (gTentacles[i].growTimer <= 0.0f)
						{
							gTentacles[i].growTimer += .025f;

							gTentacles[i].splinePoints[n] = gTentacles[i].tipCoord;					// add the point to the list
							gTentacles[i].numPoints++;
						}
					}
					break;

					/**************************/
					/* MAKE TENTACLE GO AWAY */
					/*************************/

			case	TENTACLE_MODE_AWAY:
					gTentacles[i].delayToGoAway -= fps;			// dec wait timer
					if (gTentacles[i].delayToGoAway <= 0.0f)
					{
						int n = gTentacles[i].numPoints;		// get # points in list
						int j = fps * 200.0f;					// calc # points to remove this time
						if (j < 1)
							j = 1;

						int q = n-j;							// calc # points after we remove some
						if (q <= 0)
						{
							gTentacles[i].isUsed = false;		// free this one
							if (gTentacles[i].generator)
								gTentacles[i].generator->HasTentacle = false;
						}
						else
						{
							gTentacles[i].numPoints = q;
						}
					}
					break;


		}

				/***********************************/
				/* DO TENTACLE COLLISION DETECTION */
				/***********************************/

//		px = gPlayerInfo.coord.x;											// get player coords
//		py = gPlayerInfo.coord.y + gPlayerInfo.objNode->BBox.min.y;			// y = bottom of player
//		pz = gPlayerInfo.coord.z;

		int n = gTentacles[i].numPoints;
		for (int j = 0; j < n; j += SKIP_FACTOR)							// calc at intervals along the spline
		{
			float	tx = gTentacles[i].splinePoints[j].x;
			float	ty = gTentacles[i].splinePoints[j].y;
			float	tz = gTentacles[i].splinePoints[j].z;

						/* GET HITS */

			int numHits = DoSimpleBoxCollision(ty + TENTACLE_RADIUS, ty - TENTACLE_RADIUS,
											tx - TENTACLE_RADIUS, tx + TENTACLE_RADIUS,
											tz + TENTACLE_RADIUS, tz - TENTACLE_RADIUS,
											CTYPE_PLAYER|CTYPE_WEAPON);

			for (int c = 0; c < numHits; c++)
			{
				ObjNode	*hitObj = gCollisionList[c].objectPtr;

					/* SEE IF TOUCHED PLAYER */

				if (hitObj->CType & CTYPE_PLAYER)
				{
					if (gTentacles[i].mode != TENTACLE_MODE_AWAY)			// make go away
					{
						gTentacles[i].mode = TENTACLE_MODE_AWAY;
						gTentacles[i].delayToGoAway	= 1.0;
						MakeSparkExplosion(tx, ty, tz, 200, .3, PARTICLE_SObjType_GreenSpark,0);
					}

					player->Delta.x =
					player->Delta.z = 0;									// player stuck

					PlayerGotHit(nil, .2);							// hurt player
				}

						/* TOUCHED WEAPON */
				else
				if (hitObj->Kind == WEAPON_TYPE_FLAME)				// only hurt by flame
				{
					if (gTentacles[i].mode != TENTACLE_MODE_AWAY)			// make go away
					{
						gTentacles[i].mode = TENTACLE_MODE_AWAY;
						gTentacles[i].delayToGoAway	= 1.0;
					}
				}

			}
		}
next_i:;
	}
}


/******************** DRAW TENTACLES ****************************/

static void DrawTentacles(ObjNode *theNode)
{
int				i,p,j,numSplinePoints,pointNum,numRings;
float			u,v,s;
OGLVector3D		dir;
OGLMatrix4x4	m,m2;
const OGLVector3D from = {0,1,0};
MOTriangleIndecies	*triPtr;

#pragma unused (theNode)

	OGL_PushState();

	OGL_EnableLighting();


	for (i = 0; i < MAX_TENTACLES; i++)
	{
		if (!gTentacles[i].isUsed)									// see if skip this slot
			continue;

		numSplinePoints = gTentacles[i].numPoints;					// get # spline points


		numRings = numSplinePoints / SKIP_FACTOR;					// calc # geometry rings to build
		if (numRings < 2)											// gotta be at least 2 to do anything
			continue;
		if (numRings > MAX_TENTACLE_SEGMENTS)
			DoFatalAlert("DrawTentacles: numRings > MAX_TENTACLE_SEGMENTS");

		gTentacleMesh.numPoints 	= numRings * NUM_RING_POINTS;								// set # points in mesh
		gTentacleMesh.numTriangles 	= (numRings-1) * (NUM_RING_POINTS-1) * 2;	// set # triangles in mesh


					/**************************************************/
					/* FIRST BUILD ALL OF THE POINTS FOR THE GEOMETRY */
					/**************************************************/
					//
					// Remember that the last and 1st ring points overlap so that we can
					// do proper uv wrapping all the way around.
					//

		s = .001f;																		// start scale small @ tip

		p = numSplinePoints-1;															// start @ end of spline and work toward base
		for (j = 0; j < numRings; j++)
		{
			if (j == (numRings-1))														// last ring is always base
				p = 0;

					/* CALC VECTOR TO NEXT RING */

			if (p >= SKIP_FACTOR)																// (use previous normal if this is the last segment)
			{
				dir.x = gTentacles[i].splinePoints[p-SKIP_FACTOR].x - gTentacles[i].splinePoints[p].x;
				dir.y = gTentacles[i].splinePoints[p-SKIP_FACTOR].y - gTentacles[i].splinePoints[p].y;
				dir.z = gTentacles[i].splinePoints[p-SKIP_FACTOR].z - gTentacles[i].splinePoints[p].z;
				OGLVector3D_Normalize(&dir, &dir);
			}

					/* ROTATE RING INTO POSITION */

			OGLMatrix4x4_SetScale(&m2, s, s, s);
			OGLCreateFromToRotationMatrix(&m, &from, &dir);										// generate a rotation matrix
			OGLMatrix4x4_Multiply(&m2, &m, &m);

			m.value[M03] = gTentacles[i].splinePoints[p].x;										// insert translate
			m.value[M13] = gTentacles[i].splinePoints[p].y;
			m.value[M23] = gTentacles[i].splinePoints[p].z;

			OGLPoint3D_TransformArray(&gRingPoints[0], &m, &gTentaclePoints[j * NUM_RING_POINTS],  NUM_RING_POINTS);	// transform the points into master array

			OGLVector3D_TransformArray(&gRingNormals[0], &m, &gTentacleNormals[j * NUM_RING_POINTS],  NUM_RING_POINTS);	// transform the normals into master array

			p -= SKIP_FACTOR;
			s += .15f;															// increase scale to full size
			if (s > 1.0f)
				s = 1.0f;
		}

				/*******************************************/
				/* NOW BUILD THE TRIANGLES FROM THE POINTS */
				/*******************************************/

		triPtr = &gTentacleTriangles[0];										// point to triangle list

		for (p = 0; p < numRings; p++)											// for each segment
		{
			pointNum = p * NUM_RING_POINTS;										// calc index into point list to start this ring

			for (j = 0; j < (NUM_RING_POINTS-1); j++)							// build 2 triangles for each point in the ring
			{
						/* TRIANGLE A */

				triPtr->vertexIndices[0] = pointNum + j;
				triPtr->vertexIndices[1] = pointNum + j + 1;				// next pt over
				triPtr->vertexIndices[2] = pointNum + j + NUM_RING_POINTS;		// pt in next segment
				triPtr++;														// point to next triangle

						/* TRIANGLE B */

				triPtr->vertexIndices[0] = pointNum + j + 1;					// next pt over
				triPtr->vertexIndices[1] = pointNum + j + 1 + NUM_RING_POINTS;		// pt in next segment

				triPtr->vertexIndices[2] = pointNum + j + NUM_RING_POINTS;
				triPtr++;														// point to next triangle
			}
		}

				/*************************/
				/* SET RING COLORS & UVS */
				/*************************/

		v = 0;
		for (p = 0; p < numRings; p++)
		{
			OGLTextureCoord	*uvs 	= &gTentacleUVs[p * NUM_RING_POINTS];


			u = 0;
			for (j = 0; j < NUM_RING_POINTS; j++)
			{
				uvs[j].u = u;
				uvs[j].v = v;

				u += 3.0f / (float)(NUM_RING_POINTS-1);
			}
			v += .5f;
		}

				/***********************/
				/* SUBMIT VERTEX ARRAY */
				/***********************/

		MO_DrawGeometry_VertexArray(&gTentacleMesh);


	}

	OGL_PopState();
}


#pragma mark -


/************************ ADD PITCHER PLANT BOSS *************************/

Boolean AddPitcherPlantBoss(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj,*grass;

#pragma unused(itemPtr)

			/****************************/
			/* MAKE NEW SKELETON OBJECT */
			/****************************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_PITCHERPLANT;
	gNewObjectDefinition.animNum 	= PITCHER_ANIM_WAIT;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
	gNewObjectDefinition.slot 		= 616;
	gNewObjectDefinition.moveCall 	= MovePitcherPlant;
	gNewObjectDefinition.rot 		= PI;
	gNewObjectDefinition.scale 		= PITCHER_PLANT_SCALE;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);

	newObj->Coord.y -= newObj->BBox.min.y;						// offset so bottom touches ground
	UpdateObjectTransforms(newObj);


				/* SET DEFAULT COLLISION INFO */

	newObj->CType = CTYPE_HURTME|CTYPE_BLOCKCAMERA;
	newObj->CBits = CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj, .8, 1);

	newObj->WeaponAutoTargetOff.y = 40.0f * PITCHER_PLANT_SCALE;

				/* SET BETTER INFO */

	newObj->Health 		= 1.0;
	newObj->Damage 		= .1;							// also hurt if touched

				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= PitcherPlantHitByFire;



				/*************/
				/* ADD GRASS */
				/*************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= JUNGLE_ObjType_PitcherPlant_Grass;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.slot		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	grass = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->ChainNode = grass;


	return(true);
}


/********************* MOVE PITCHER PLANT **************************/

static void MovePitcherPlant(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MovePitcherPlant_Wait,
					MovePitcherPlant_Attack,
					MovePitcherPlant_GotHit
				};


	myMoveTable[theNode->Skeleton->AnimNum](theNode);

}



/********************** MOVE PITCHER PLANT: WAIT ******************************/

static void  MovePitcherPlant_Wait(ObjNode *theNode)
{
	if (gNumDestroyedPods >= NUM_POD_PLANTS_TO_KILL)				// once the pods are gone we can attack
	{
		theNode->Timer -= gFramesPerSecondFrac;
		if (theNode->Timer <= 0.0f)
		{
			theNode->Timer = 3.0f + RandomFloat()*1.0f;

			MorphToSkeletonAnim(theNode->Skeleton, PITCHER_ANIM_ATTACK, 3);

		}
	}
}

/********************** MOVE PITCHER PLANT: ATTACK ******************************/

static void  MovePitcherPlant_Attack(ObjNode *theNode)
{
		/***********************/
		/* SEE IF SHOOT SPORES */
		/***********************/

	if (theNode->ShootNow)
	{
		ObjNode	*spore;
		short	i;

		for (i = 0; i < 6; i++)
		{
			gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
			gNewObjectDefinition.type 		= JUNGLE_ObjType_PitcherPod_Pollen;

			gNewObjectDefinition.coord.x = theNode->Coord.x + RandomFloat2() * 40.0f;
			gNewObjectDefinition.coord.y = theNode->Coord.y + (23.0f * theNode->Scale.x) - RandomFloat() * 40.0f;
			gNewObjectDefinition.coord.z = theNode->Coord.z + RandomFloat2() * 40.0f;

			gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
			gNewObjectDefinition.slot 		= 775;
			gNewObjectDefinition.moveCall 	= MovePollenSpore;
			gNewObjectDefinition.rot 		= 0;
			gNewObjectDefinition.scale 		= .3f;
			spore = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			spore->Delta.x = RandomFloat2() * 100.0f;
			spore->Delta.z = RandomFloat2() * 100.0f;
			spore->Delta.y = 300.0f + RandomFloat() * 200.0f;

			AttachShadowToObject(spore, SHADOW_TYPE_CIRCULAR, 4, 4, true);
		}

		theNode->ShootNow = false;
	}

		/************************/
		/* SEE IF IS VULNERABLE */
		/************************/

	if (theNode->Vulnerable)
	{
		float	s = theNode->Scale.x;

		SetObjectCollisionBounds(theNode, 62.0f * s, 18.0f * s, -14.0f * s, 14.0f * s, 22.0f * s, -11.0f * s);

		theNode->CType |= CTYPE_AUTOTARGETWEAPON;
	}
	else
		theNode->CType &= ~CTYPE_AUTOTARGETWEAPON;


		/***************/
		/* SEE IF DONE */
		/***************/

	if (theNode->Skeleton->AnimHasStopped)
	{
		MorphToSkeletonAnim(theNode->Skeleton, PITCHER_ANIM_WAIT, 3);
		CreateCollisionBoxFromBoundingBox(theNode, .8, 1);							// reset collision to normal
	}

}



/********************** MOVE PITCHER PLANT: GOT HIT ******************************/

static void  MovePitcherPlant_GotHit(ObjNode *theNode)
{
	if (theNode->Skeleton->AnimHasStopped)
		MorphToSkeletonAnim(theNode->Skeleton, PITCHER_ANIM_WAIT, 6);
}

/********************** MOVE PITCHER PLANT: BURNING ******************************/

static void MovePitcherPlant_Burning(ObjNode *theNode)
{
	BurnFire(theNode, theNode->Coord.x, theNode->Coord.y, theNode->Coord.z, true, PARTICLE_SObjType_Fire, 4.0, 0);
}


/************ PITCHER PLANT HIT BY FIRE *****************/

static Boolean PitcherPlantHitByFire(ObjNode *weapon, ObjNode *plant, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	if (!plant->Vulnerable)								// see if vulnerable
		return(true);

	plant->ColorFilter.r -= .06f;
	plant->ColorFilter.g -= .11f;
	plant->ColorFilter.b -= .11f;

				/*********************/
				/* SEE IF GOT KILLED */
				/*********************/

	if (plant->ColorFilter.g <= .1f)
	{
		ExplodeGeometry(plant, 600, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, 1);
		MakeSparkExplosion(plant->Coord.x, plant->Coord.y, plant->Coord.z, 700, 30.0, PARTICLE_SObjType_GreenSpark,0);

		PlayEffect_Parms3D(EFFECT_PITCHERBOOM, &plant->Coord, NORMAL_CHANNEL_RATE, 3.0);

				/* MAKE CORPSE */

		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= JUNGLE_ObjType_PitcherPlant_Dead;
		gNewObjectDefinition.coord.x	= plant->Coord.x;
		gNewObjectDefinition.coord.z	= plant->Coord.z;
		gNewObjectDefinition.coord.y	= GetTerrainY(plant->Coord.x, plant->Coord.z);
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
		gNewObjectDefinition.slot 		= plant->Slot;
		gNewObjectDefinition.moveCall 	= MovePitcherPlant_Burning;
		gNewObjectDefinition.rot 		= plant->Rot.y;
		gNewObjectDefinition.scale 		= plant->Scale.x;
		MakeNewDisplayGroupObject(&gNewObjectDefinition);

				/* DELETE PITCHER PLANT */

		DeleteObject(plant);

				/* DELETE TRACTOR BEAM */

		DeleteObject(gTractorBeamObj);
		gTractorBeamObj = nil;
	}

			/*****************/
			/* JUST GOT HURT */
			/*****************/
	else
	{
		MorphToSkeletonAnim(plant->Skeleton, PITCHER_ANIM_GOTHIT, 4.0f);
		PlayEffect_Parms3D(EFFECT_PITCHERPAIN, &plant->Coord, NORMAL_CHANNEL_RATE, 1.5);
		plant->Vulnerable = false;
	}

	return(true);			// stop weapon
}



#pragma mark -


/**************** ADD PITCHER POD ********************/

Boolean AddPitcherPod(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*stem, *pod;
uint32_t	podDestroyed = itemPtr->flags & ITEM_FLAGS_USER1;

			/* CREATE STEM */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= JUNGLE_ObjType_PitcherPod_Stem;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) - 100.0f;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 387;
	gNewObjectDefinition.moveCall 	= MovePitcherPod;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 2.5f + RandomFloat() * .5f;
	stem = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	stem->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/* CREATE POD */

	if (podDestroyed)
		gNewObjectDefinition.type 		= JUNGLE_ObjType_PitcherPod_DeadPod;
	else
		gNewObjectDefinition.type 		= JUNGLE_ObjType_PitcherPod_Pod;

	gNewObjectDefinition.coord.z 	+= 164.6f * gNewObjectDefinition.scale;
	gNewObjectDefinition.coord.y 	+= 336.7f * gNewObjectDefinition.scale;

	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	pod = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	stem->ChainNode = pod;
	pod->ChainHead = stem;

	if (!podDestroyed)
	{
		pod->CType = CTYPE_BLOCKCAMERA|CTYPE_AUTOTARGETWEAPON;
		pod->CBits = CBITS_ALLSOLID;
		CreateCollisionBoxFromBoundingBox(pod, 1,1);

		pod->WeaponAutoTargetOff.y = POD_MOUTH_OFF_Y * pod->Scale.x;
		pod->WeaponAutoTargetOff.z = POD_MOUTH_OFF_Z * pod->Scale.x;

		pod->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= PitcherPodHitByFire;
	}
	else
		pod->PodBlown = true;

	return(true);
}


/******************** MOVE PITCHER POD *****************/

static void MovePitcherPod(ObjNode *theNode)
{
ObjNode	*pod = theNode->ChainNode;
float	fps = gFramesPerSecondFrac;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	if (pod)
	{
		if (!pod->PodBlown)
		{
			/* MAKE IT STEAM */

			MakePitcherPodSteam(pod);


				/* SEE IF SHOOT */

			theNode->Timer -= fps;
			if (theNode->Timer <= 0.0f)
			{
				theNode->Timer = 2.0f + RandomFloat() * 3.0f;			// reset timer

				if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < 3500.0f)
				{
					PitcherPodShoot(pod);
				}
			}
		}
	}
}


/***************** MAKE PITCHER POD STEAM *****************/

static void MakePitcherPodSteam(ObjNode *theNode)
{
int		i;
float	fps = gFramesPerSecondFrac;
int		particleGroup,magicNum;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;

	theNode->ParticleTimer -= fps;
	if (theNode->ParticleTimer <= 0.0f)
	{
		theNode->ParticleTimer += 0.025f;


		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= 350;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 25;
			groupDef.decayRate				= -.7;
			groupDef.fadeRate				= .1;
			groupDef.particleTextureNum		= PARTICLE_SObjType_RedFumes;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			float	x = theNode->Coord.x;
			float	y = theNode->Coord.y + (POD_MOUTH_OFF_Y * theNode->Scale.x);
			float	z = theNode->Coord.z + (POD_MOUTH_OFF_Z * theNode->Scale.x);

			for (i = 0; i < 2; i++)
			{
				p.x = x + RandomFloat2() * 30.0f;
				p.y = y;
				p.z = z + RandomFloat2() * 30.0f;

				d.x = RandomFloat2() * 90.0f;
				d.y = 300.0f + RandomFloat() * 160.0f;
				d.z = RandomFloat2() * 90.0f;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= RandomFloat() + 1.0f;
				newParticleDef.rotZ			= RandomFloat()*PI2;
				newParticleDef.rotDZ		= RandomFloat2() * .1f;
				newParticleDef.alpha		= .2;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}


/********************* PITCHER POD SHOOT **************************/

static void PitcherPodShoot(ObjNode *pod)
{
ObjNode	*spore;
int		i;

			/****************/
			/* CREATE SPORE */
			/****************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= JUNGLE_ObjType_PitcherPod_Pollen;

	gNewObjectDefinition.coord.x = pod->Coord.x;
	gNewObjectDefinition.coord.y = pod->Coord.y + (POD_MOUTH_OFF_Y * pod->Scale.x);
	gNewObjectDefinition.coord.z = pod->Coord.z + (POD_MOUTH_OFF_Z * pod->Scale.x);

	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 775;
	gNewObjectDefinition.moveCall 	= MovePollenSpore;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .25f;
	spore = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	spore->Delta.x = spore->Delta.z = 0;
	spore->Delta.y = 400.0f;

	spore->Damage = .1;

	AttachShadowToObject(spore, SHADOW_TYPE_CIRCULAR, 4, 4, true);



				/* CREATE SPARKLE */

	i = spore->Sparkles[0] = GetFreeSparkle(spore);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_TRANSFORMWITHOWNER|SPARKLE_FLAG_FLICKER;
		gSparkles[i].where.x =
		gSparkles[i].where.y =
		gSparkles[i].where.z = 0;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = .95;
		gSparkles[i].color.b = 0;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 200.0f;
		gSparkles[i].separation = -40.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark4;
	}


	PlayEffect3D(EFFECT_PODSHOOT, &spore->Coord);
}


/*********************** MOVE POLLEN SPORE ************************/

static void MovePollenSpore(ObjNode *spore)
{
float		fps = gFramesPerSecondFrac;
OGLVector2D	v;
float		speed;

	GetObjectInfo(spore);

			/* MOVE IT */

	gDelta.y -= 250.0f * fps;								// gravity

	v.x = gPlayerInfo.coord.x - gCoord.x;					// calc vector to player
	v.y = gPlayerInfo.coord.z - gCoord.z;
	FastNormalizeVector2D(v.x, v.y, &v, false);

	gDelta.x += v.x * (350.0f * fps);						// accelerate toward player
	gDelta.z += v.y * (350.0f * fps);

	speed = CalcVectorLength(&gDelta);						// calc speed & pin max
	if (speed > 450.0f)
	{
		speed = 450.0f / speed;
		gDelta.x *= speed;
		gDelta.y *= speed;
		gDelta.z *= speed;
	}

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	spore->Rot.x += 4.0f * fps;							// spin
	spore->Rot.y += 2.0f * fps;


				/* SEE IF HIT GROUND */

	if (gCoord.y < GetTerrainY(gCoord.x, gCoord.z))
	{
poof:
		MakePuff(&gCoord, 20.0, PARTICLE_SObjType_RedFumes, GL_SRC_ALPHA, GL_ONE, 1.2);
		DeleteObject(spore);
		return;
	}

			/* SEE IF HIT PLAYER */

	if (DoSimpleBoxCollisionAgainstPlayer(gCoord.y + 20.0f, gCoord.y - 20.0f,
										gCoord.x - 20.0f, gCoord.x + 20.0f,
										gCoord.z + 20.0f, gCoord.z - 20.0f))
	{
		PlayerGotHit(spore, 0);				// hurt the player
		goto poof;
	}

			/* SEE IF HIT ANYTHING SOLID */
	else
	if (DoSimpleBoxCollision(gCoord.y + 20.0f, gCoord.y - 20.0f,
							gCoord.x - 20.0f, gCoord.x + 20.0f,
							gCoord.z + 20.0f, gCoord.z - 20.0f,
							CTYPE_MISC))
	{
		goto poof;
	}

	UpdateObject(spore);
}



/************ PITCHER POD HIT BY FIRE *****************/

static Boolean PitcherPodHitByFire(ObjNode *weapon, ObjNode *pod, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	pod->ColorFilter.r -= .15f;
	pod->ColorFilter.g -= .3f;
	pod->ColorFilter.b -= .3f;

	if (pod->ColorFilter.g <= .2f)
	{
		ObjNode	*stem = pod->ChainHead;

		MakePuff(&pod->Coord, 30.0, PARTICLE_SObjType_RedFumes, GL_SRC_ALPHA, GL_ONE,.5);
		ExplodeGeometry(pod, 600, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, 1.1);
		stem->TerrainItemPtr->flags |= ITEM_FLAGS_USER1;	// make sure the pod part doesn't come back
		gNumDestroyedPods++;

		pod->PodBlown = true;

		pod->Type = JUNGLE_ObjType_PitcherPod_DeadPod;		// change to blown up pod
		ResetDisplayGroupObject(pod);

		pod->CType = 0;
		pod->StatusBits = gAutoFadeStatusBits;
		pod->HitByWeaponHandler[WEAPON_TYPE_FLAME] 	= nil;
		pod->ColorFilter.r =
		pod->ColorFilter.g =
		pod->ColorFilter.b = 1;
		pod->NumCollisionBoxes = 0;

		PlayEffect_Parms3D(EFFECT_PODBOOM, &pod->Coord, NORMAL_CHANNEL_RATE, 3.0);
	}

	return(true);			// stop weapon
}


#pragma mark -


/**************** INIT TRACTOR BEAM ***************************/

static void InitTractorBeam(void)
{

						/* MAKE BEAM */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= JUNGLE_ObjType_TractorBeam;
	gNewObjectDefinition.coord.x	= gPlayerInfo.startX;
	gNewObjectDefinition.coord.z	= gPlayerInfo.startZ;
	gNewObjectDefinition.coord.y	= GetTerrainY(gPlayerInfo.startX, gPlayerInfo.startZ);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_GLOW | STATUS_BIT_NOLIGHTING | STATUS_BIT_NOZWRITES | STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.slot 		= SPRITE_SLOT - 1;
	gNewObjectDefinition.moveCall 	= MoveTractorBeam;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 13.0;
	gTractorBeamObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);



}


/****************** MOVE TRACTOR BEAM ***************************/

static void MoveTractorBeam(ObjNode *beam)
{

	GetObjectInfo(beam);

			/* UPDATE BEAM */

	beam->ColorFilter.a = .99f - RandomFloat()*.1f;					// flicker
	beam->Scale.x = beam->Scale.z = 13.0f - RandomFloat() * 1.0f;

	TurnObjectTowardTarget(beam, &gCoord, gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z, 100, false);	// aim at camera
	UpdateObjectTransforms(beam);


	MO_Object_OffsetUVs(beam->BaseGroup, 0, gFramesPerSecondFrac * 2.0f);

			/* UPDATE AUDIO */

	if (beam->EffectChannel != -1)
		Update3DSoundChannel(EFFECT_TRACTORBEAM, &beam->EffectChannel, &gCoord);
	else
		beam->EffectChannel = PlayEffect3D(EFFECT_TRACTORBEAM, &gCoord);

}


/************************* ADD TRACTOR BEAM POST *********************************/

Boolean AddTractorBeamPost(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int	i;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= JUNGLE_ObjType_TractorBeamPost;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 67;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= 1.2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj,1,1);


			/* SPARKLE */

	i = newObj->Sparkles[0] = GetFreeSparkle(newObj);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL | SPARKLE_FLAG_FLICKER;
		gSparkles[i].where.x = newObj->Coord.x;
		gSparkles[i].where.y = newObj->Coord.y + 582.0f * newObj->Scale.y;
		gSparkles[i].where.z = newObj->Coord.z;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 200.0f;

		gSparkles[i].separation = 30.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_BlueGlint;
	}


	return(true);													// item was added
}







