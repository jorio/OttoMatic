/****************************/
/*   	TRAPS2.C		    */
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

static void SetLavaPillarCollisionBoxes(ObjNode *theNode);
static void MoveLavaPillar(ObjNode *theNode);
static void ShatterLavaPillar(ObjNode *theNode);
static void MoveVolcanoGenerator(ObjNode *theNode);
static void MoveLavaSpewer(ObjNode *theNode);
static void MoveLavaBoulder(ObjNode *theNode);
static void RecalcSuperTileBBoxY(SuperTileMemoryType *superTile, OGLPoint3D *points);
static void CreateHillGenerator(long  x, long z);
static void MoveHillGenerator(ObjNode *theNode);
static void MoveLavaPillarChunk(ObjNode *theNode);

static void MoveLavaPlatform(ObjNode *theNode);
static void MoveLavaPlatformOnSpline(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	HILL_MESA_RADIUS	2000.0f

#define	LAVA_PILLAR_SCALE	8.0f


/*********************/
/*    VARIABLES      */
/*********************/

#define	Falling		Flag[0]

#define	SpewDelay	SpecialF[2]

#define	FallDelay	SpecialF[0]


/************************* ADD LAVA PILLAR *********************************/

Boolean AddLavaPillar(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FIREICE_ObjType_LavaPillar_Full;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 643;
	gNewObjectDefinition.moveCall 	= MoveLavaPillar;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/8.0f);			// use given rot
	gNewObjectDefinition.scale 		= LAVA_PILLAR_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;							// keep ptr to item list


	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_ALLSOLID;

	SetLavaPillarCollisionBoxes(newObj);						// set collision boxes
	KeepOldCollisionBoxes(newObj);

	newObj->Falling = false;

	return(true);												// item was added
}


/***************** SET LAVA PILLAR COLLISION BOXES ********************/

static void SetLavaPillarCollisionBoxes(ObjNode *theNode)
{
static const OGLPoint3D	 baseOff = {0, 0, 40.4f};
static const OGLVector3D up = {0,1,0};
OGLVector3D				v;
OGLPoint3D				base;
float					w,w2,x,y,z,s;
int						numBoxes,i;
CollisionBoxType		*box;

			/* CALC VECTOR OF TILT & BASE POINT */

	OGLVector3D_Transform(&up, &theNode->BaseTransformMatrix, &v);
	OGLPoint3D_Transform(&baseOff, &theNode->BaseTransformMatrix, &base);


			/* CALC SIZE INFO */

	s = theNode->Scale.x;
	w = theNode->BBox.max.x * s * .7f;								// get the cubic size of each collision box
	w2 = w * 2.0f;

	numBoxes = (theNode->BBox.max.y * s / w2) + .5f;				// calc # collision boxes needed (round up)
	if (numBoxes > MAX_COLLISION_BOXES)
		numBoxes = MAX_COLLISION_BOXES;

	theNode->NumCollisionBoxes = numBoxes;						// set # boxes
	box = &theNode->CollisionBoxes[0];							// point to boxes



			/* SET ALL BOXES */

	x = base.x + v.x * w;						// move up to center 1st box
	y = base.y + v.y * w;
	z = base.z + v.z * w;


	for (i = 0; i < numBoxes; i++)
	{
		box[i].top 		= y + w;
		box[i].bottom 	= y - w;
		box[i].left 	= x - w;
		box[i].right 	= x + w;
		box[i].front 	= z + w;
		box[i].back 	= z - w;

		x += v.x * w2;
		y += v.y * w2;
		z += v.z * w2;

	}
}




/******************** MOVE LAVEA PILLAR **************************/

static void MoveLavaPillar(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		i;

				/* TRACK IT */

	if (TrackTerrainItem(theNode))
	{
		DeleteObject(theNode);
		return;
	}

			/*************************************/
			/* NOT FALLING YET, SO SEE IF SHOULD */
			/*************************************/

	if (!theNode->Falling)									// see if falling
	{
		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < 1000.0f)
			theNode->Falling = true;
		else
			return;
	}


			/****************/
			/* ITS FALLING  */
			/****************/

	GetObjectInfo(theNode);


			/* KEEL OVER */

	theNode->DeltaRot.x -= fps * 1.6f;
	theNode->Rot.x += theNode->DeltaRot.x * fps;

	if (theNode->Rot.x <= -PI/2)							// see if down
	{
		ShatterLavaPillar(theNode);
		return;
	}

	UpdateObject(theNode);
	SetLavaPillarCollisionBoxes(theNode);


		/***********************/
		/* SEE IF CRUSH PLAYER */
		/***********************/

	if (gPlayerInfo.invincibilityTimer <= 0.0f)								// dont check if player is invincible
	{
		for (i = 0; i < theNode->NumCollisionBoxes; i++)
		{
			if (DoSimpleBoxCollisionAgainstPlayer(theNode->CollisionBoxes[i].top, theNode->CollisionBoxes[i].bottom,
												 theNode->CollisionBoxes[i].left, theNode->CollisionBoxes[i].right,
												theNode->CollisionBoxes[i].front, theNode->CollisionBoxes[i].back))
			{
				if (gPlayerInfo.objNode->Skeleton->AnimNum != PLAYER_ANIM_ACCORDIAN)
				{
					CrushPlayer();

//					MorphToSkeletonAnim(gPlayerInfo.objNode->Skeleton, PLAYER_ANIM_ACCORDIAN, 5);
//					PlayerLoseHealth(.3, PLAYER_DEATH_TYPE_EXPLODE);
//					gPlayerInfo.objNode->AccordianTimer = 2.0;
//					PlayEffect3D(EFFECT_PLAYERCRUSH, &gPlayerInfo.coord);
					break;
				}
			}
		}
	}
}


/*************************** SHATTER LAVA PILLAR **************************************/

static void ShatterLavaPillar(ObjNode *theNode)
{
ObjNode	*newObj;
float	x,z,dx,dz,r;
int		i;
			/******************************/
			/* CREATE ALL OF THE SEGMENTS */
			/******************************/

	x = theNode->Coord.x;
	z = theNode->Coord.z;

	r = theNode->Rot.y;
	dx = -sin(r) * (30.0f * LAVA_PILLAR_SCALE);
	dz = -cos(r) * (30.0f * LAVA_PILLAR_SCALE);

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FIREICE_ObjType_LavaPillar_Segment;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= theNode->Slot;
	gNewObjectDefinition.moveCall 	= MoveLavaPillarChunk;
	gNewObjectDefinition.rot 		= r;										// use given rot
	gNewObjectDefinition.scale 		= LAVA_PILLAR_SCALE;

	for (i = 0; i < 5; i++)
	{
				/* MAKE NEW OBJ */

		gNewObjectDefinition.coord.y = GetTerrainY(x,z) + 50.0f;
		gNewObjectDefinition.coord.x 	= x;
		gNewObjectDefinition.coord.z 	= z;
		gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		newObj->Rot.x = PI/2;

					/* SET COLLISION */

		newObj->CType = CTYPE_MISC | CTYPE_HURTME;
		newObj->CBits = CBITS_ALLSOLID;
		CreateCollisionBoxFromBoundingBox_Maximized(newObj);

		newObj->Delta.y = 1000.0f + RandomFloat() * 500.0f;
		newObj->Delta.x = RandomFloat2() * 600.0f;
		newObj->Delta.z = RandomFloat2() * 600.0f;

		newObj->DeltaRot.x = RandomFloat2() * PI;
		newObj->DeltaRot.z = RandomFloat2() * PI;

		x += dx;
		z += dz;
	}

	PlayEffect_Parms3D(EFFECT_PILLARCRUNCH, &theNode->Coord, NORMAL_CHANNEL_RATE / 2, 1.5);

	DeleteObject(theNode);
}


/****************** MOVE LAVA PILLAR CHUNK **************************/

static void MoveLavaPillarChunk(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	gDelta.y -= 2000.0f * fps;							// gravity

	gCoord.x += gDelta.x * fps;							// move
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.x += theNode->DeltaRot.x * fps;		// spin
	theNode->Rot.z += theNode->DeltaRot.z * fps;

			/* COLLISION */

	if (HandleCollisions(theNode, CTYPE_TERRAIN | CTYPE_FENCE | CTYPE_MISC, .8))
	{
		if (theNode->StatusBits & STATUS_BIT_ONGROUND)
		{
			theNode->Special[0]++;							// inc bounce count

			switch (theNode->Special[0])
			{
			case 0:
				break;

			case 1:
			case 2:
				PlayEffect_Parms3D(EFFECT_PILLARCRUNCH, &theNode->Coord, NORMAL_CHANNEL_RATE + (MyRandomLong() & 0x3fff), 1.0);
				break;

			default:
				PlayEffect_Parms3D(EFFECT_PILLARCRUNCH, &theNode->Coord, NORMAL_CHANNEL_RATE + (MyRandomLong() & 0x3fff), 1.2);
				MakePuff(&gCoord, 80, PARTICLE_SObjType_RedFumes, GL_SRC_ALPHA, GL_ONE, 1.0);
				ExplodeGeometry(theNode, 500, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, 2.0);
				ExplodeGeometry(theNode, 500, SHARD_MODE_BOUNCE, 1, 1.1);
				DeleteObject(theNode);
				return;
			}
		}
	}



	UpdateObject(theNode);
}


#pragma mark -


/******************* ADD VOLCANO GENERATOR ZONE ************************/

Boolean AddVolcanoGeneratorZone(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-10;
	gNewObjectDefinition.moveCall 	= MoveVolcanoGenerator;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	return(true);
}


/****************** MOVE VOLCANO GENERATOR ******************************/

static void MoveVolcanoGenerator(ObjNode *theNode)
{
	if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < 3000.0f)
	{
		CreateHillGenerator(theNode->Coord.x, theNode->Coord.z);
		theNode->TerrainItemPtr = nil;									// dont come back, only do this once!
		DeleteObject(theNode);
	}
}


/************************* EXPLODE VOLCANO TOP ********************************/

void ExplodeVolcanoTop(float x, float z)
{
float	y = GetTerrainY(x,z) + 10.0f;
OGLPoint3D	p;
ObjNode	*newObj;
int	i;

	p.x = x;
	p.y = y + 40.0f;
	p.z = z;

	MakeSparkExplosion(x, y, z, 700.0f, 2.0, PARTICLE_SObjType_RedSpark,0);
	MakePuff(&p, 50, PARTICLE_SObjType_RedFumes, GL_SRC_ALPHA, GL_ONE, .2);

	PlayEffect3D(EFFECT_VOLCANOBLOW, &p);


				/* MAKE LAVA EVENT */

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord	 	= p;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+60;
	gNewObjectDefinition.moveCall 	= MoveLavaSpewer;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->SpewDelay = 10;

				/*****************/
				/* SPEW BOULDERS */
				/*****************/

	for (i = 0; i < 4; i++)
	{
		SpewALavaBoulder(&p, false);
	}

}


/******************** SPEW A BOULDER **************************/

void SpewALavaBoulder(OGLPoint3D *p, Boolean fromFlamester)
{
ObjNode	*newObj;
float		r;
int			i;

			/* MAKE OBJECT */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FIREICE_ObjType_Boulder;
	gNewObjectDefinition.coord	 	= *p;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 200;
	gNewObjectDefinition.moveCall 	= MoveLavaBoulder;
	gNewObjectDefinition.rot 		= 0;
	if (fromFlamester)
		gNewObjectDefinition.scale 		= 1.0;
	else
		gNewObjectDefinition.scale 		= 2.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_HURTME;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,1);

			/* SET INFO */

	if (fromFlamester)
	{
		r = RandomFloat()*PI2;
		newObj->Delta.y = 200.0f;
		newObj->Health = 2.0f;
	}
	else
	{
		newObj->Delta.y = 1100.0f + RandomFloat() * 500.0f;
		newObj->Health = 5.0f + RandomFloat() * 3.0f;
		r = PI + CalcYAngleFromPointToPoint(0, p->x, p->z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) + RandomFloat2() * (PI/3);
	}

	newObj->Delta.x = sin(r) * 600.0f;
	newObj->Delta.z = cos(r) * 600.0f;

	newObj->Damage = .2;

	AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 10,10, false);


		/* CREATE BACK-GLOW */

	i = newObj->Sparkles[0] = GetFreeSparkle(newObj);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL | SPARKLE_FLAG_FLICKER | SPARKLE_FLAG_RANDOMSPIN;
		gSparkles[i].where = newObj->Coord;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 100.0f * newObj->Scale.x;
		gSparkles[i].separation = -50.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_RedSpark;
	}


}


/******************** MOVE LAVA SPEWER ***************************/

static void MoveLavaSpewer(ObjNode *theNode)
{
	if (SeeIfCoordsOutOfRange(theNode->Coord.x,theNode->Coord.z))				// don't do anything if out of range
		return;

	BurnFire(theNode, theNode->Coord.x, theNode->Coord.y, theNode->Coord.z, true, PARTICLE_SObjType_Fire, 4.0, 0);

			/* SEE IF SPEW A BOULDER */

	theNode->SpewDelay -= gFramesPerSecondFrac;
	if (theNode->SpewDelay <= 0.0f)
	{
		SpewALavaBoulder(&theNode->Coord, false);
		theNode->SpewDelay = 5.0f + RandomFloat() * 3.0f;
		MakeSparkExplosion(theNode->Coord.x, theNode->Coord.y, theNode->Coord.z, 700.0f, 2.0, PARTICLE_SObjType_RedSpark,0);
	}

}



/************************ MOVE LAVA BOULDER ****************************/

static void MoveLavaBoulder(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		i;

		/* SEE IF DECAYED */

	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		ExplodeGeometry(theNode, 350, SHARD_MODE_BOUNCE, 1, 1.0);
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);

			/* DO GRAVITY & ACCELERATION */

	gDelta.y -= 3000.0f * fps;

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
	{
		GetTerrainY(gCoord.x,gCoord.z);									// call this just to get the terrain normal

		gDelta.x += gRecentTerrainNormal.x * 5000.0f * fps;				// accel from terrain normal
		gDelta.z += gRecentTerrainNormal.z * 5000.0f * fps;
	}

			/* MOVE IT */

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;



			/* DO COLLISION */

	HandleCollisions(theNode, CTYPE_MISC|CTYPE_TERRAIN, .1);


		/* SPIN IT */

	theNode->Rot.x += 5.0f * fps;
	theNode->Rot.y += 7.0f * fps;


			/* UPDATE */

	UpdateObject(theNode);

	i = theNode->Sparkles[0];											// update sparkle
	if (i != -1)
		gSparkles[i].where = gCoord;

	BurnFire(theNode, theNode->Coord.x, theNode->Coord.y, theNode->Coord.z, false, PARTICLE_SObjType_Fire, 1.0, PARTICLE_FLAGS_ALLAIM);
}



#pragma mark -


/************************* CREATE HILL GENERATOR *********************************/

static void CreateHillGenerator(long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= 0;			// make sure it's the first thing we do
	gNewObjectDefinition.moveCall 	= MoveHillGenerator;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->Health = 3.0f;
}


/************************* MOVE HILL GENERATOR *************************/

static void MoveHillGenerator(ObjNode *theNode)
 {
SuperTileMemoryType *superTile;
int					v;
float				y,hillX,hillZ,d;
float				fps = gFramesPerSecondFrac;
float				h;

			/* SEE IF DONE */

	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		ExplodeVolcanoTop(theNode->Coord.x, theNode->Coord.z);
		DeleteObject(theNode);
		return;
	}

	h = theNode->Health * 150.0f * fps;								// calc grow speed


			/* CALC VARIOUS COORDS */

	hillX = theNode->Coord.x;
	hillZ = theNode->Coord.z;

	int row = hillZ * TERRAIN_SUPERTILE_UNIT_SIZE_Frac;					// calc supertile row/col of generator
	int col = hillX * TERRAIN_SUPERTILE_UNIT_SIZE_Frac;

	int rowStart	= ClampInt(row-3, 0, gNumSuperTilesDeep-1);			// pick a range
	int rowEnd		= ClampInt(row+3, 0, gNumSuperTilesDeep-1);
	int colStart	= ClampInt(col-3, 0, gNumSuperTilesWide-1);
	int colEnd		= ClampInt(col+3, 0, gNumSuperTilesWide-1);

			/*************************************/
			/* SCAN EACH SUPERTILE AND DEFORM IT */
			/*************************************/

	for (int r = rowStart; r <= rowEnd; r++)
	{
		for (int c = colStart; c <= colEnd; c++)
		{
			OGLPoint3D	*points;
			int			i,tRow,tCol;

			tRow = r * SUPERTILE_SIZE; 										// get tile row/col of this supertile
			tCol = c * SUPERTILE_SIZE;

			if (!(gSuperTileStatusGrid[r][c].statusFlags & SUPERTILE_IS_DEFINED))	// make sure this supertile is define
				continue;

			i = gSuperTileStatusGrid[r][c].supertileIndex;					// extract supertile #
			superTile = &gSuperTileMemoryList[i];							// get ptr to supertile
			points = superTile->meshData->points;							// get ptr to mesh points for this supertile


					/***************************************/
					/* PROCESS EACH VERTEX FOR DEFORMATION */
					/***************************************/

			v = 0;
			for (row = 0; row <= SUPERTILE_SIZE; row++)
			{
				for (col = 0; col <= SUPERTILE_SIZE; col++, v++)
				{
					d = CalcQuickDistance(points[v].x,points[v].z,hillX,hillZ);		// calc dist from this vertex to the generator origin

					d = HILL_MESA_RADIUS - d;									// reverse the dist such that d = distance from edge
					if (d >= 0.0f)												// only do thing inside the mesa radius
					{
						int	tileRow, tileCol;

						d *= 1.0f/HILL_MESA_RADIUS;								// convert dist to a 0...1 fraction where 0 == far from center, 1 = @ center
						d = (1.0f - d) * PI;									// convert dist to be 0..PI where PI = far, 0 = far center
						d = cos(d) + 1.0f;										// make d in the range of 0.0 -> 2.0

						if (d > 1.8f)											// do this to give it a flat top
							d = 2.0f;

						tileRow = tRow+row;										// calc tile row/col
						tileCol = tCol+col;

						y = gMapYCoordsOriginal[tileRow][tileCol];				// get original y value
						y += d * h;												// raise it based on distance


									/* SET Y COORD */

						points[v].y = y;										// save final y coord into geometry

						if (((row != SUPERTILE_SIZE) && (r != rowEnd)) &&
							((col != SUPERTILE_SIZE) && (c != colEnd)))			// also update the grid (but not the end edges since those are the start edges on next supertile and we don't want do duplicate them)
						{
							gMapYCoordsOriginal[tileRow][tileCol] =
							gMapYCoords[tileRow][tileCol] = y;
						}
					}
				}
			}

						/* RECALC NORMALS */

			CalculateSupertileVertexNormals(superTile->meshData, tRow, tCol);


						/* RECALC BBOX FOR GEOMETRY */

			RecalcSuperTileBBoxY(superTile, points);
		}
	}
}




/*********************** RECALC SUPERTILE BBOX Y ******************************/

static void RecalcSuperTileBBoxY(SuperTileMemoryType *superTile, OGLPoint3D *points)
{
float				miny = 10000000;													// init bbox counters
float				maxy = -miny;
int					i;

				/* SCAN FOR MIN/MAX Y */

	for (i = 0; i < (SUPERTILE_SIZE*SUPERTILE_SIZE); i++)
	{
		if (points[i].y < miny)
			miny = points[i].y;
		if (points[i].y > maxy)
			maxy = points[i].y;
	}

	superTile->bBox.min.y = miny;
	superTile->bBox.max.y = maxy;
}


#pragma mark -


/************************* ADD LAVA PLATFORM *********************************/

Boolean AddLavaPlatform(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FIREICE_ObjType_LavaPlatform;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	GetWaterY(x,z, &gNewObjectDefinition.coord.y);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= TRIGGER_SLOT;
	gNewObjectDefinition.moveCall 	= MoveLavaPlatform;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= 4.2f;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->CType 			= CTYPE_TRIGGER|CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_BLOCKSHADOW|CTYPE_MPLATFORM;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= SIDE_BITS_TOP;						// side(s) to activate it
	newObj->Kind		 	= TRIGTYPE_LAVAPLATFORM;

	newObj->Mode			= FALLING_PLATFORM_MODE_WAIT;

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);

	return(true);													// item was added
}


/******************* MOVE LAVA PLATFORM ************************/

static void MoveLavaPlatform(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

		/* SEE IF OUT OF RANGE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);

	switch(theNode->Mode)
	{
		case	FALLING_PLATFORM_MODE_FALL:

				theNode->FallDelay -= fps;								// see if still delayed
				if (theNode->FallDelay <= 0.0f)
				{
					gDelta.y -= 50.0f * fps;
					gCoord.y += gDelta.y * fps;

					if (gCoord.y < (theNode->InitCoord.y - 600.0f))		// see if reset
						theNode->Mode = FALLING_PLATFORM_MODE_RISE;

				}
				break;


		case	FALLING_PLATFORM_MODE_RISE:
				gDelta.y = 100.0f;
				gCoord.y += gDelta.y * fps;

				if (gCoord.y >= theNode->InitCoord.y)
				{
					gDelta.y = 0;
					theNode->Mode = FALLING_PLATFORM_MODE_WAIT;
					theNode->CType |= CTYPE_TRIGGER;		// its triggerable again
				}

				break;

	}

	UpdateObject(theNode);
}


/************** DO TRIGGER - LAVA PLATFORM ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_LavaPlatform(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
#pragma unused (sideBits, whoNode)

	theNode->FallDelay = 4.0;
	theNode->Mode = FALLING_PLATFORM_MODE_FALL;
	theNode->CType &= ~CTYPE_TRIGGER;							// no longer a trigger

	return(true);
}




/************************ PRIME LAVA PLATFORM *************************/

Boolean PrimeLavaPlatform(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/***************/
				/* MAKE OBJECT */
				/***************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FIREICE_ObjType_LavaPlatform;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	GetWaterY(x,z, &gNewObjectDefinition.coord.y);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_ONSPLINE;
	gNewObjectDefinition.slot 		= PLAYER_SLOT - 4;				// good idea to move this before the player
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 4.2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;


				/* SET BETTER INFO */

	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveLavaPlatformOnSpline;		// set move call

	newObj->CType 			= CTYPE_MISC | CTYPE_MPLATFORM | CTYPE_BLOCKCAMERA | CTYPE_BLOCKSHADOW;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 	1, 1);



			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);
	AddToSplineObjectList(newObj, false);



	return(true);
}


/******************** MOVE LAVA PLATFORM ON SPLINE ***************************/

static void MoveLavaPlatformOnSpline(ObjNode *theNode)
{
Boolean isVisible;
float	oldX, oldZ;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	oldX = theNode->Coord.x;
	oldZ = theNode->Coord.z;
	IncreaseSplineIndex(theNode, 90);
	GetObjectCoordOnSpline(theNode);

	theNode->Delta.x = (theNode->Coord.x - oldX) * gFramesPerSecond;	// set deltas that we just moved
	theNode->Delta.z = (theNode->Coord.z - oldZ) * gFramesPerSecond;

			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/

	if (isVisible)
	{
		UpdateObjectTransforms(theNode);					// update transforms
		CalcObjectBoxFromNode(theNode);						// calc current collision box
	}

}










