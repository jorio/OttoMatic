/****************************/
/*   BLOB BOSS.C			*/
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

static void MakeBlobBoss_Beams(float x, float z);
static void DrawBeamsDischarge(ObjNode *theNode);
static void MakeBlobBoss_Platforms(float centerX, float centerZ);
static void MoveBlobBossPlatform(ObjNode *theNode);
static void MoveBlobBossBeams(ObjNode *theNode);
static void MakeBlobBoss_CentralUnit(float x, float z);

static void MoveBeamEnd(ObjNode *theNode);
static Boolean BeamEndGotHit(ObjNode *weapon, ObjNode *theNode, OGLPoint3D *where, OGLVector3D *weaponDelta);
static void MoveCasing(ObjNode *casing);
static void MoveHorn(ObjNode *horn);
static void ActivateHorns(void);
static Boolean HornGotHit(ObjNode *weapon, ObjNode *horn, OGLPoint3D *where, OGLVector3D *weaponDelta);
static Boolean TubeGotHit(ObjNode *weapon, ObjNode *tube, OGLPoint3D *where, OGLVector3D *weaponDelta);
static void MoveTubeSegment(ObjNode *tube);
static void MoveHornBullet(ObjNode *theNode);
static void HornShoot(ObjNode *horn);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	BEAM_RADIUS		3500.0f
#define	BEAM_THICKNESS	30.0f
#define	BEAM_YOFF		400.0f
#define	DISRUPTION_TIME	2.0f

#define	BEAM_SEGMENTS		70
#define	NUM_BEAM_POINTS		((BEAM_SEGMENTS + 2) * 2)
#define	NUM_BEAM_TRIANGLES	(BEAM_SEGMENTS * 2)

#define	NUM_PLATFORMS		10
#define	PLATFORM_DIST		3000.0f

enum
{
	CASING_MODE_SHIELD,
	CASING_MODE_UP,
	CASING_MODE_STAYOPEN,
	CASING_MODE_DOWN,
	CASING_MODE_STAYDOWN,
	CASING_MODE_BOOM
};


/*********************/
/*    VARIABLES      */
/*********************/

static MOTriangleIndecies gBeamTriangles[NUM_BEAM_TRIANGLES];

static const OGLPoint3D gBeamEndPointOffsets[4] =
{
	{0,0,-BEAM_RADIUS},
	{BEAM_RADIUS,0,0},
	{0,0,BEAM_RADIUS},
	{-BEAM_RADIUS,0,0},
};

static 	float	gBeamDisruptionTimer[4];
static	short	gNumBeamsDestroyed;
static	Boolean	gBeamDestroyed[4],gNumHornsDestroyed;
static	ObjNode	*gBeamEnds[4],*gHorns[4],*gTube,*gCasing;

static float	gBlobBossX,gBlobBossZ;

static	float	gCasingTimer;
static	int		gCasingMode;

#define	BeamNum		Special[0]

#define	BaseRot		SpecialF[0]
#define	HornWobble	SpecialF[1]
#define	HornShootTimer	SpecialF[2]

/************************ MAKE BLOB BOSS MACHINE *************************/

void MakeBlobBossMachine(void)
{
long					i;
TerrainItemEntryType	*itemPtr;
float					x = 0;
float					z = 0;

			/*********************************/
			/* FIND BLOB BOSS ON THE TERRAIN */
			/*********************************/

	itemPtr = *gMasterItemList; 										// get pointer to data inside the LOCKED handle

	for (i= 0; i < gNumTerrainItems; i++)
	{
		if (itemPtr[i].type == MAP_ITEM_BLOBBOSS)						// see if it's what we're looking for
		{
			x = itemPtr[i].x;
			z = itemPtr[i].y;
			break;
		}
	}

	GAME_ASSERT_MESSAGE(i != gNumTerrainItems, "MAP_ITEM_BLOBBOSS not found on terrain!");

	gBlobBossX = x;
	gBlobBossZ = z;


			/* MAKE THE BEAM PROJECTOR OBJECT */

	MakeBlobBoss_Beams(x, z);


			/* MAKE THE MOVING PLATFORMS */

	MakeBlobBoss_Platforms(x, z);

			/* MAKE CENTRAL UNIT */

	MakeBlobBoss_CentralUnit(x,z);

}

#pragma mark -

/**************** MAKE BLOB BOSS:  BEAMS *************************/
//
// Creates the stuff needed for the beams
//

static void MakeBlobBoss_Beams(float x, float z)
{
ObjNode	*newObj;
int		i,t,a,b,c;
float	y,r;

			/* GENERATE THE TRIANGLE LIST */

	t = 0;
	a = 0;
	b = 1;
	c = 2;

	for (i = 0; i < BEAM_SEGMENTS; i++)
	{
		gBeamTriangles[t].vertexIndices[0] = a;						// fist triangle of segment
		gBeamTriangles[t].vertexIndices[1] = b;
		gBeamTriangles[t].vertexIndices[2] = c;

		t++;														// next triangle
		a += 2;
		c += 1;

		gBeamTriangles[t].vertexIndices[0] = a;						// 2nd triangle of segment
		gBeamTriangles[t].vertexIndices[1] = b;
		gBeamTriangles[t].vertexIndices[2] = c;

		t++;														// next triangle
		b += 2;
		c += 1;
	}


			/***********************/
			/* CREATE BEAMS OBJECT */
			/***********************/

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= y = GetTerrainY_Undeformed(x,z) + BEAM_YOFF;
	gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES | STATUS_BIT_GLOW | STATUS_BIT_NOZWRITES | STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+10;
	gNewObjectDefinition.moveCall 	= MoveBlobBossBeams;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->CustomDrawFunction = DrawBeamsDischarge;

	newObj->Damage = .3;						// set damage that hitting a beam will do


			/***************/
			/* CREATE ENDS */
			/***************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BLOBBOSS_ObjType_BeamEnd;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 70;
	gNewObjectDefinition.moveCall 	= MoveBeamEnd;
	gNewObjectDefinition.scale 		= 2.0;

	for (i = 0; i < 4; i++)
	{
		gBeamDisruptionTimer[i] = 0;
		gBeamDestroyed[i] = false;


		gNewObjectDefinition.coord.x 	= x + gBeamEndPointOffsets[i].x;
		gNewObjectDefinition.coord.z 	= z + gBeamEndPointOffsets[i].z;
		gNewObjectDefinition.rot 		= r = (float)i * (-PI/2);
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		gBeamEnds[i] = newObj;

				/* SET COLLISION STUFF */

		newObj->WeaponAutoTargetOff.x = -sin(r) * 100.0f;			// set auto-target offset
		newObj->WeaponAutoTargetOff.z = -cos(r) * 100.0f;

		newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_AUTOTARGETWEAPON;
		newObj->CBits			= CBITS_ALLSOLID;
		CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,1);

		newObj->BeamNum = i;														// set beam #
		newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] = BeamEndGotHit;			// set callback

		newObj->Health = 1.0f;


				/* CREATE SPARKLE */

		t = newObj->Sparkles[0] = GetFreeSparkle(newObj);				// get free sparkle slot
		if (t != -1)
		{
			gSparkles[t].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
			gSparkles[t].where.x = newObj->Coord.x;
			gSparkles[t].where.y = newObj->Coord.y;
			gSparkles[t].where.z = newObj->Coord.z;

			gSparkles[t].color.r = 1;
			gSparkles[t].color.g = 1;
			gSparkles[t].color.b = .3;
			gSparkles[t].color.a = .7;

			gSparkles[t].scale = 400.0f;
			gSparkles[t].separation = 30.0f;

			gSparkles[t].textureNum = PARTICLE_SObjType_WhiteSpark4;

		}
	}


	gNumBeamsDestroyed = 0;
}

/********************* MOVE BEAM END *****************************/

static void MoveBeamEnd(ObjNode *theNode)
{
			/* JUST UPDATE THE SOUND */

	if (theNode->EffectChannel == -1)
		theNode->EffectChannel = PlayEffect3D(EFFECT_BLOBBEAMHUM, &theNode->Coord);
	else
		Update3DSoundChannel(EFFECT_BLOBBEAMHUM, &theNode->EffectChannel, &theNode->Coord);

}


/************************* BEAM END GOT HIT ******************************/

static Boolean BeamEndGotHit(ObjNode *weapon, ObjNode *theNode, OGLPoint3D *where, OGLVector3D *weaponDelta)
{
short	beamNum;

#pragma unused (weapon,	where, weaponDelta)

	beamNum = theNode->BeamNum;								// get beam # to be affected

	theNode->Health -= .3f;									// dec health
	if (theNode->Health < 0.0f)
	{
		gNumBeamsDestroyed++;
		gBeamDestroyed[beamNum] = true;

		ExplodeGeometry(theNode, 600, SHARD_MODE_BOUNCE, 1, .5);
		PlayEffect_Parms3D(EFFECT_SLIMEBOOM, &theNode->Coord, NORMAL_CHANNEL_RATE, 3.0);
		DeleteObject(theNode);

				/* IF ALL BEAMS DESTORYED THEN START ATTACK */

		if (gNumBeamsDestroyed == 4)
		{
			ActivateHorns();
		}
	}

	gBeamDisruptionTimer[beamNum] = DISRUPTION_TIME;

	return(true);
}



/******************* DRAW BEAMS DISCHARGE ******************/

static void DrawBeamsDischarge(ObjNode *theNode)
{
float		fps = gFramesPerSecondFrac;
OGLPoint3D	targetCoord,points[NUM_BEAM_POINTS];
float		dx,dy,dz,x,y,z,u;
int			i,j,n;
OGLTextureCoord	uvs[NUM_BEAM_POINTS];
MOVertexArrayData	mesh;
float		disruptionSize,thickness;

	if (gNumBeamsDestroyed == 4)								// dont draw if all dead
		return;

			/* INIT MESH BASICS */

	mesh.numMaterials 	= -1;
	mesh.numPoints 		= NUM_BEAM_POINTS;
	mesh.numTriangles 	= NUM_BEAM_TRIANGLES;
	mesh.points			= (OGLPoint3D *)points;
	mesh.normals		= nil;
	mesh.uvs[0]			= (OGLTextureCoord *)uvs;
	mesh.colorsByte		= nil;
	mesh.colorsFloat	= nil;
	mesh.triangles		= gBeamTriangles;


			/***************************/
			/* DRAW ALL 4 OF THE BEAMS */
			/***************************/

	for (n = 0; n < 4; n++)
	{
		if (gBeamDestroyed[n])									// dont draw if destroyed
			continue;

		x = theNode->Coord.x;									// get coord of base
		y = theNode->Coord.y;
		z = theNode->Coord.z;

		targetCoord.x = x + gBeamEndPointOffsets[n].x;				// get endpoint coord
		targetCoord.y = y + gBeamEndPointOffsets[n].y;
		targetCoord.z = z + gBeamEndPointOffsets[n].z;



			/************************/
			/* BUILD SECTION POINTS */
			/************************/

		/* SUB-DIVIDE LINE BETWEEN ENDPOINTS */

		dx = (targetCoord.x - x) * (1.0f / (float)BEAM_SEGMENTS);			// calc vector from L to R
		dy = (targetCoord.y - y) * (1.0f / (float)BEAM_SEGMENTS);
		dz = (targetCoord.z - z) * (1.0f / (float)BEAM_SEGMENTS);

		if (gBeamDisruptionTimer[n] > 0.0f)
		{
			disruptionSize = (DISRUPTION_TIME - gBeamDisruptionTimer[n]) * 20.0f;
//			thickness = (DISRUPTION_TIME - gBeamDisruptionTimer[n]) * BEAM_THICKNESS;
			gBeamDisruptionTimer[n] -= fps;
		}
		else
		{
			disruptionSize = 20.0f;
//			thickness = BEAM_THICKNESS;
		}

		thickness = gBeamEnds[n]->Health * BEAM_THICKNESS + 1.0f;						// thickness is based on health of this beam

		u = 0;
		j = 0;
		for (i = 0; i < (BEAM_SEGMENTS+1); i++)										// build the point list
		{
			float yoff;

			if ((i > 0) && (i < BEAM_SEGMENTS))
				yoff = RandomFloat2() * disruptionSize;
			else
				yoff = 0;

			points[j].x = x;
			points[j].y = y + yoff + thickness;
			points[j].z = z;
			uvs[j].u = u;
			uvs[j].v = 1;
			j++;
			points[j].x = x;
			points[j].y = y + yoff - thickness;
			points[j].z = z;
			uvs[j].u = u;
			uvs[j].v = 0;
			j++;

			x += dx;
			y += dy;
			z += dz;
			u += 1.0f / (float)BEAM_SEGMENTS;

		}

				/***********/
				/* DRAW IT */
				/***********/

		gGlobalTransparency = .7f;
		MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_NovaCharge].materialObject);
		MO_DrawGeometry_VertexArray(&mesh);
		gGlobalTransparency = 1.0f;
	}
}


/********************** MOVE BLOB BOSS BEAMS **********************************/

static void MoveBlobBossBeams(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	x,y,z,tx,tz;
int		particleGroup,magicNum,i,j;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
OGLVector3D			d;
OGLPoint3D			p;
ObjNode				*player = gPlayerInfo.objNode;

	if (gNumBeamsDestroyed == 4)										// see if all dead
		return;

			/*********************/
			/* SEE IF HIT PLAYER */
			/*********************/

	x = theNode->Coord.x;												// get coord of base
	y = theNode->Coord.y;
	z = theNode->Coord.z;

	for (i = 0; i < 4; i++)
	{
		OGLPoint2D	pp;
		float		distToLine,t;

		if (gBeamDestroyed[i])											// skip the blown beams
			continue;

		if ((player->Coord.y + player->BBox.min.y) > (y + 30.0f))		// if player's foot is above beam, then skip
			continue;

		tx = x + gBeamEndPointOffsets[i].x;								// get endpoint coords
		tz = z + gBeamEndPointOffsets[i].z;

		pp.x = player->Coord.x;
		pp.y = player->Coord.z;

		distToLine = OGLPoint2D_LineDistance(&pp, x, z, tx, tz, &t);	// calc distance to the line for collision

		if (distToLine < 40.0f)
		{
			PlayerGotHit(theNode, 0);
		}
	}


		/***********************/
		/* MAKE FALLING SPARKS */
		/***********************/

	theNode->ParticleTimer -= fps;
	if (theNode->ParticleTimer <= 0.0f)										// see if time to make particles
	{
		theNode->ParticleTimer += .02f;

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
new_group:
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= 400;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= 70;
			groupDef.decayRate				=  0;
			groupDef.fadeRate				= .8;
			groupDef.particleTextureNum		= PARTICLE_SObjType_WhiteSpark3;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

		if (particleGroup != -1)
		{
			p.y = y;
			for (i = 0; i < 9; i++)
			{
				j = MyRandomLong() & 3;												// random beam
				if (gBeamDestroyed[j])												// no sparkes from blown beams
					continue;
				switch(j)
				{
					case	0:
							p.x = x;
							p.z = z - RandomFloat() * BEAM_RADIUS;
							break;

					case	1:
							p.x = x + RandomFloat() * BEAM_RADIUS;
							p.z = z;
							break;

					case	2:
							p.x = x;
							p.z = z + RandomFloat() * BEAM_RADIUS;
							break;

					case	3:
							p.x = x - RandomFloat() * BEAM_RADIUS;
							p.z = z;
							break;
				}

				d.x = d.z = 0;
				d.y = 0;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= 1.0f + RandomFloat();
				newParticleDef.rotZ			= 0;
				newParticleDef.rotDZ		= 0;
				newParticleDef.alpha		= 1.0f;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					goto new_group;
				}
			}
		}
	}

}


#pragma mark -


/******************** MAKE BLOB BOSS:  PLATFORMS ****************************/

static void MakeBlobBoss_Platforms(float centerX, float centerZ)
{
ObjNode	*newObj;
float	s,x,z,r;
int		i;


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BLOBBOSS_ObjType_CircularPlatform_Grey;
	gNewObjectDefinition.coord.y 	= GetTerrainY_Undeformed(centerX,centerZ) + (BEAM_YOFF - 30.0f);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveBlobBossPlatform;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= s = 2.0f;

	r = 0;
	for (i = 0; i < NUM_PLATFORMS; i++, r += (PI2 / NUM_PLATFORMS))
	{
		x = centerX + sin(r) * PLATFORM_DIST;
		z = centerZ + cos(r) * PLATFORM_DIST;

		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.z 	= z;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->SpinningPlatformType = SPINNING_PLATFORM_TYPE_CIRCULAR;

		newObj->CType 			= CTYPE_TRIGGER2|CTYPE_TRIGGER|CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_BLOCKSHADOW|CTYPE_MPLATFORM;
		newObj->CBits			= CBITS_TOP;
		newObj->TriggerSides 	= SIDE_BITS_TOP;						// side(s) to activate it
		newObj->Kind		 	= TRIGTYPE_CIRCULARPLATFORM;

		CreateCollisionBoxFromBoundingBox_Maximized(newObj);

		newObj->DeltaRot.y = .2;
	}
}


/************************* MOVE BLOB BOSS PLATFORM *******************************/

static void MoveBlobBossPlatform(ObjNode *theNode)
{
float		fps = gFramesPerSecondFrac;
float		deltaRot;
OGLPoint2D	origin,pt;
OGLMatrix3x3	m;

	GetObjectInfo(theNode);

			/* SPIN PLATFORM AROUND THE BOSS'S CENTER */

 	deltaRot = fps * theNode->DeltaRot.y;

   	origin.x = gBlobBossX;
	origin.y = gBlobBossZ;
	OGLMatrix3x3_SetRotateAboutPoint(&m, &origin, -deltaRot);		// not do (-) cuz 2D y rot is reversed from 3D y rot

	pt.x = gCoord.x;
	pt.y = gCoord.z;
	OGLPoint2D_Transform(&pt, &m, &pt);								// transform coord

	gDelta.x = (pt.x - gCoord.x) * gFramesPerSecond;										// calc delta
	gDelta.z = (pt.y - gCoord.z) * gFramesPerSecond;

	gCoord.x = pt.x;												// update coord
	gCoord.z = pt.y;


			/* SPIN THE PLATFORM ITSELF */

	theNode->Rot.y += deltaRot;
	if (theNode->Rot.y > PI2)							// wrap back rather than getting bigger and bigger
		theNode->Rot.y -= PI2;

	UpdateObject(theNode);
}


#pragma mark -

/****************** MAKE BLOB BOSS:  CENTRAL UNIT *************************/

static void MakeBlobBoss_CentralUnit(float x, float z)
{
float	y = GetTerrainY_Undeformed(x,z) + BEAM_YOFF;
ObjNode	*newObj;
ObjNode	*prev = nil;
short	i;
float	r,s;

			/*************/
			/* MAKE BASE */
			/*************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BLOBBOSS_ObjType_Base;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 70;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= s = 6.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/***************/
			/* MAKE CASING */
			/***************/

	gNewObjectDefinition.type 		= BLOBBOSS_ObjType_Casing;
	gNewObjectDefinition.moveCall 	= MoveCasing;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,1);

	gCasingMode = CASING_MODE_SHIELD;
	gCasing = newObj;

			/**********************/
			/* MAKE TUBE SEGMENTS */
			/**********************/

	for (i = 0; i < 15; i++)
	{
		if (i == 0)
			gNewObjectDefinition.slot 		= 600;
		else
			gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
		gNewObjectDefinition.type 		= BLOBBOSS_ObjType_TubeSegment;
		gNewObjectDefinition.moveCall 	= MoveTubeSegment;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_UVTRANSFORM;

		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		gNewObjectDefinition.coord.y += 100.0f * gNewObjectDefinition.scale;

				/* FIRST ONE IS THE INTELLIGENT ONE */

		if (i == 0)
		{
			gTube = newObj;

			newObj->Health 			= 1.0;
			newObj->CType 			= CTYPE_MISC;
			newObj->CBits			= CBITS_ALLSOLID;

			SetObjectCollisionBounds(newObj, 3000, 0,
									 newObj->BBox.min.x * s, newObj->BBox.max.x * s,
									 newObj->BBox.max.z * s, newObj->BBox.min.z * s);

			newObj->WeaponAutoTargetOff.y += 300.0f;
		}
		else
		{
			prev->ChainNode = newObj;			// chain them together
		}

		prev = newObj;
	}

		/**************/
		/* MAKE HORNS */
		/**************/

	gNumHornsDestroyed = 0;

	for (r = PI/3, i = 0; i < 4; i++, r += PI/2)
	{
		gNewObjectDefinition.coord.x 	= x - sin(r) * 300.0f;
		gNewObjectDefinition.coord.z 	= z - cos(r) * 300.0f;
		gNewObjectDefinition.coord.y 	= y + (21.0 * gNewObjectDefinition.scale);
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_KEEPBACKFACES;

		gNewObjectDefinition.slot 		= 448;
		gNewObjectDefinition.type 		= BLOBBOSS_ObjType_Horn;
		gNewObjectDefinition.moveCall 	= MoveHorn;
		gNewObjectDefinition.rot 		= r;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Health = 1.0;
		newObj->BaseRot = r;
		newObj->HornWobble = RandomFloat2() * PI2;

		newObj->WeaponAutoTargetOff.y  += 150.0f;

		newObj->HornShootTimer = 0;

		gHorns[i] = newObj;
	}
}


/********************** MOVE CASING ******************************/

static void MoveCasing(ObjNode *casing)
{
float	fps = gFramesPerSecondFrac;

	if (gCasingMode == CASING_MODE_SHIELD)			// if in shield mode then don't do anything
		return;

	GetObjectInfo(casing);

	switch(gCasingMode)
	{
		case	CASING_MODE_UP:
				gCoord.y += 600.0f * fps;
				if (gCoord.y >= (casing->InitCoord.y + 1000.0f))			// see if @ top
				{
					gCasingMode = CASING_MODE_STAYOPEN;
					gCasingTimer = 3.0;
				}
				break;

		case	CASING_MODE_STAYOPEN:
				gCasingTimer -= fps;
				if (gCasingTimer <= 0.0f)
				{
					gCasingMode = CASING_MODE_DOWN;
					PlayEffect3D(EFFECT_SLIMEBOSSOPEN, &gCoord);
				}
				break;

		case	CASING_MODE_DOWN:
				gCoord.y -= 600.0f * fps;
				if (gCoord.y <= casing->InitCoord.y)			// see if @ bottom
				{
					gCoord.y = casing->InitCoord.y;
					gCasingMode = CASING_MODE_STAYDOWN;
					gCasingTimer = 2.0;
				}
				break;


		case	CASING_MODE_STAYDOWN:
				gCasingTimer -= fps;
				if (gCasingTimer <= 0.0f)
				{
					gCasingMode = CASING_MODE_UP;
					PlayEffect3D(EFFECT_SLIMEBOSSOPEN, &gCoord);
				}
				break;

	}

	UpdateObject(casing);
}


/************************ ACTIVATE HORNS **************************/

static void ActivateHorns(void)
{
int	i;

	gCasingMode = CASING_MODE_UP;					// start casing going up

	for (i = 0; i < 4; i++)
	{
		gHorns[i]->CType 			= CTYPE_MISC|CTYPE_AUTOTARGETWEAPON;
		gHorns[i]->CBits			= CBITS_ALLSOLID;
		CreateCollisionBoxFromBoundingBox_Rotated(gHorns[i],1,1);

		gHorns[i]->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] = HornGotHit;			// set callback

	}
}


/*************************** MOVE HORN *******************************/

static void MoveHorn(ObjNode *horn)
{
float	r,a;
OGLVector2D	v1,v2;
ObjNode	*player = gPlayerInfo.objNode;

	if (gCasingMode == CASING_MODE_SHIELD)					// dont do anything if cant see them
		return;

			/* DO WOBBLE */

	horn->HornWobble += gFramesPerSecondFrac * 2.5f;
	r = horn->Rot.y = horn->BaseRot + sin(horn->HornWobble) * .5f;

		/****************/
		/* SEE IF SHOOT */
		/****************/

	horn->HornShootTimer -= gFramesPerSecondFrac;	// dec shoot timer
	if ((horn->HornShootTimer <= 0.0f) && (gCasingMode == CASING_MODE_STAYOPEN))	// only check if time to do it
	{
		v1.x = -sin(r);									// calc aim of horn
		v1.y = -cos(r);

		v2.x = (player->Coord.x + player->Delta.x) - horn->Coord.x;		// calc vec from horn to where player will be player
		v2.y = (player->Coord.z + player->Delta.z) - horn->Coord.z;
		FastNormalizeVector2D(v2.x, v2.y, &v2, true);

		a = acos(OGLVector2D_Dot(&v1,&v2));				// calc angle between them


				/* IF AIMING CLOSE THEN SHOOT */

		if (a < (PI/4))
		{
			HornShoot(horn);
		}
	}

			/* UPDATE */

	UpdateObjectTransforms(horn);
}

/******************** HORN SHOOT **************************/

static void HornShoot(ObjNode *horn)
{
ObjNode	*newObj;
float	r = horn->Rot.y;
float	dx,dz,force;

	dx = -sin(r);
	dz = -cos(r);

	horn->HornShootTimer = .3f;							// reset shoot timer


		/* CREATE BLOBULE */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BLOBBOSS_ObjType_Blobule;
	gNewObjectDefinition.coord.x 	= horn->Coord.x + dx * 100.0f;
	gNewObjectDefinition.coord.y 	= horn->Coord.y + 220.0f;
	gNewObjectDefinition.coord.z 	= horn->Coord.z + dz * 100.0f;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 502;
	gNewObjectDefinition.moveCall 	= MoveHornBullet;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->Damage 			= .1;

	newObj->CType 			= CTYPE_HURTME|CTYPE_MISC;
	newObj->CBits			= CBITS_TOUCHABLE;
	CreateCollisionBoxFromBoundingBox(newObj, 1, 1);

	force = 1600.0f + RandomFloat() * 500.0f;

	newObj->Delta.x = dx * force;
	newObj->Delta.y = 500.0f;
	newObj->Delta.z = dz * force;

	newObj->DeltaRot.x = RandomFloat2() * PI2;
	newObj->DeltaRot.y = RandomFloat2() * PI2;
	newObj->DeltaRot.z = RandomFloat2() * PI2;

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 4, 4, true);

	PlayEffect3D(EFFECT_BLOBSHOOT, &gNewObjectDefinition.coord);
}

/******************* MOVE HORN BULLET ************************/

static void MoveHornBullet(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;


		/* SEE IF OUT OF RANGE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);


			/* MOVE IT */

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// do friction if landed
	{
		ApplyFrictionToDeltasXZ(2000.0,&gDelta);
		ApplyFrictionToRotation(10.0,&theNode->DeltaRot);
	}

	gDelta.y -= 1000.0f * fps;								// gravity

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.x += theNode->DeltaRot.x * fps;
	theNode->Rot.y += theNode->DeltaRot.y * fps;
	theNode->Rot.z += theNode->DeltaRot.z * fps;


		/* HANDLE COLLISIONS */

	if (HandleCollisions(theNode, CTYPE_MISC|CTYPE_TERRAIN|CTYPE_FENCE, -.5))
	{
		MakeSplatter(&gCoord,BLOBBOSS_ObjType_BlobDroplet);
		DeleteObject(theNode);
		return;
	}

	UpdateObject(theNode);
}


/*************************** HORN GOT HIT *******************************/

static Boolean HornGotHit(ObjNode *weapon, ObjNode *horn, OGLPoint3D *where, OGLVector3D *weaponDelta)
{
#pragma unused (weapon,	where, weaponDelta)

	horn->Health -= .2f;
	if (horn->Health <= 0.0f)
	{
		ExplodeGeometry(horn, 400, SHARD_MODE_UPTHRUST|SHARD_MODE_HEAVYGRAVITY|SHARD_MODE_FROMORIGIN, 1, .7);
		PlayEffect_Parms3D(EFFECT_SLIMEBOOM, &horn->Coord, NORMAL_CHANNEL_RATE, 2.5);
		DeleteObject(horn);

				/* AUTO-TARGET THE TUBE ONCE ALL HORNS ARE DESTROYED */

		gNumHornsDestroyed++;
		if (gNumHornsDestroyed == 4)
		{
			gTube->CType = CTYPE_MISC|CTYPE_AUTOTARGETWEAPON;
			gTube->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] = TubeGotHit;
		}
	}

	return(true);
}


/********************* MOVE TUBE SEGMENT **************************/

static void MoveTubeSegment(ObjNode *tube)
{
	if (gCasingMode != CASING_MODE_BOOM)			// only check to see if should chain react explode
		return;

	tube->Health -= gFramesPerSecondFrac;
	if (tube->Health <= 0.0f)
	{
		ExplodeGeometry(tube, 500, SHARD_MODE_HEAVYGRAVITY|SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .3);
		PlayEffect3D(EFFECT_BLOBBOSSBOOM, &tube->Coord);
		tube->StatusBits |= STATUS_BIT_HIDDEN;				// hide instead of delete, cuz we still need to chain react the other tube segments
		tube->CType = 0;
		tube->MoveCall = nil;
	}

	tube->TextureTransformV += gFramesPerSecondFrac;		// animate

}


/*************************** TUBE GOT HIT *******************************/

static Boolean TubeGotHit(ObjNode *weapon, ObjNode *tube, OGLPoint3D *where, OGLVector3D *weaponDelta)
{
float	delay;

#pragma unused (weapon,	where, weaponDelta)

	tube->Health -= .2f;
	if (tube->Health <= 0.0f)
	{

				/* BLOW UP THE CASING */

		gCasingMode = CASING_MODE_BOOM;

		if (gCasing)
		{
			ExplodeGeometry(gCasing, 700, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .5);
			DeleteObject(gCasing);
			gCasing = nil;

			gLevelCompleted = true;							// we've won the level!
			gLevelCompletedCoolDownTimer = 4;				// set delay so we can see it blow up
		}

				/* SET EXPLOSION DELAY ON EACH SEGMENT */

		delay = 0;
		do
		{
			tube->Health = delay;
			delay += .25f;
			tube = tube->ChainNode;
		}while(tube != nil);
	}

	return(true);
}




















