/****************************/
/*   	TRAPS.C			    */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/


#include "game.h"

extern	float				gFramesPerSecondFrac,gFramesPerSecond,gGlobalTransparency,gGravity;
extern	OGLPoint3D			gCoord;
extern	PlayerInfoType	gPlayerInfo;
extern	OGLVector3D			gDelta;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLVector3D			gRecentTerrainNormal;
extern	SplineDefType		**gSplineList;
extern	u_long		gAutoFadeStatusBits;
extern	short				gNumCollisions;
extern	SuperTileStatus	**gSuperTileStatusGrid;
extern	NewParticleGroupDefType	gNewParticleGroupDef;
extern	PrefsType			gGamePrefs;
extern	int					gLevelNum;
extern	ObjNode				*gFirstNodePtr, *gTargetPickup;
extern	SpriteType	*gSpriteGroupList[MAX_SPRITE_GROUPS];
extern	SparkleType	gSparkles[MAX_SPARKLES];
extern	OGLBoundingBox			gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	CollisionRec	gCollisionList[];

/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveBeemer(ObjNode *theNode);
static Boolean HurtBeemer(ObjNode *beemer, float unused);
static void MoveRailGunOnSpline(ObjNode *theNode);
static void MoveTurret(ObjNode *base);
static Boolean HurtTurret(ObjNode *base, float unused);
static void MoveTurretBullet(ObjNode *theNode);
static void ShootTurret(ObjNode *turret);


/****************************/
/*    CONSTANTS             */
/****************************/

#define BEEMER_SCALE	1.3f

#define	RAIL_GUN_SCALE	1.0f

#define	TURRET_SCALE	1.0f


/*********************/
/*    VARIABLES      */
/*********************/

#define	ShootTimer	SpecialF[0]
#define	MuzzleSide	Special[0]

static const OGLPoint3D 	gMuzzleTipOff[2] =
{
	-57.0 * TURRET_SCALE, 0, -236.0f * TURRET_SCALE,
	57.0 * TURRET_SCALE, 0, -236.0f * TURRET_SCALE,
};
static const OGLVector3D	gMuzzleTipAim = {0,0,-1};


/************************* ADD BEEMER *********************************/

Boolean AddBeemer(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj,*beam;
short	i;
					/***************/
					/* MAKE BEEMER */
					/***************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SAUCER_ObjType_Beemer;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
	gNewObjectDefinition.slot 		= 272;
	gNewObjectDefinition.moveCall 	= MoveBeemer;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= BEEMER_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;							// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj, 1,1);

	newObj->HurtCallback = HurtBeemer;


					/* MAKE SPARKLE */

	i = newObj->Sparkles[0] = GetFreeSparkle(newObj);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_RANDOMSPIN|SPARKLE_FLAG_FLICKER;
		gSparkles[i].where.x = newObj->Coord.x;
		gSparkles[i].where.y = newObj->Coord.y + (200.0f * BEEMER_SCALE);
		gSparkles[i].where.z = newObj->Coord.z;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 500.0f;

		gSparkles[i].separation = 80.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_RedGlint;
	}


			/*****************/
			/* MAKE THE BEAM */
			/*****************/

	gNewObjectDefinition.type 		= SAUCER_ObjType_BeemerBeam;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_GLOW | STATUS_BIT_NOLIGHTING | STATUS_BIT_NOZWRITES | STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.slot 		= SPRITE_SLOT - 1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.scale 		= BEEMER_SCALE;
	beam = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->ChainNode = beam;

	return(true);
}


/*********************** MOVE BEEMER ***************************/

static void MoveBeemer(ObjNode *theNode)
{
ObjNode	*beam = theNode->ChainNode;

		/* SEE IF HURT PLAYER */

	if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < (gPlayerInfo.objNode->BBox.max.x * gPlayerInfo.objNode->Scale.x))
	{
		ImpactPlayerSaucer(theNode->Coord.x, theNode->Coord.z, gFramesPerSecondFrac * 2.0f, gPlayerInfo.objNode, 10);
	}


		/*******************/
		/* UPDATE THE BEAM */
		/*******************/

	beam->ColorFilter.a = .99f - RandomFloat()*.1f;					// flicker
	beam->Scale.x = beam->Scale.z = BEEMER_SCALE - RandomFloat() * .05f;

	TurnObjectTowardTarget(beam, &beam->Coord, gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z, 100, false);	// aim at camera
	UpdateObjectTransforms(beam);

	MO_Object_OffsetUVs(beam->BaseGroup, 0, RandomFloat2() * 20.0f);


			/* DO GENERIC SAUCER HANDLING */

	MoveObjectUnderPlayerSaucer(theNode);
}


/******************* HURT BEEMER *****************************/

static Boolean HurtBeemer(ObjNode *beemer, float unused)
{
#pragma unused (unused)

	PlayEffect_Parms3D(EFFECT_SAUCERKABOOM, &beemer->Coord, NORMAL_CHANNEL_RATE, 2.0);
	MakeSparkExplosion(beemer->Coord.x, beemer->Coord.y, beemer->Coord.z, 400.0f, 1.0, PARTICLE_SObjType_RedSpark,0);
	ExplodeGeometry(beemer, 700, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .5);
	beemer->TerrainItemPtr = nil;
	DeleteObject(beemer);

	return(false);										// return value doesn't mean anything
}


#pragma mark -

/************************ PRIME RAIL GUN *************************/

Boolean PrimeRailGun(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj,*beam;
float			x,z,placement;
int				i;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;
	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);


				/***************/
				/* MAKE OBJECT */
				/***************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SAUCER_ObjType_RailGun;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits|STATUS_BIT_ONSPLINE;
	gNewObjectDefinition.slot 		= 345;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= RAIL_GUN_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;


				/* SET BETTER INFO */

	newObj->SplinePlacement = placement;
	newObj->SplineMoveCall 	= MoveRailGunOnSpline;		// set move call

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Maximized(newObj);



					/****************/
					/* MAKE SPARKLE */
					/****************/

	i = newObj->Sparkles[0] = GetFreeSparkle(newObj);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_RANDOMSPIN|SPARKLE_FLAG_FLICKER;
		gSparkles[i].where.x = newObj->Coord.x;
		gSparkles[i].where.y = newObj->Coord.y + (110.0f * RAIL_GUN_SCALE);
		gSparkles[i].where.z = newObj->Coord.z;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 200.0f;

		gSparkles[i].separation = 40.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_BlueGlint;
	}


			/*****************/
			/* MAKE THE BEAM */
			/*****************/

	gNewObjectDefinition.type 		= SAUCER_ObjType_RailGunBeam;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_GLOW | STATUS_BIT_NOLIGHTING | STATUS_BIT_NOZWRITES | STATUS_BIT_KEEPBACKFACES;
	gNewObjectDefinition.slot 		= SPRITE_SLOT - 1;
	gNewObjectDefinition.moveCall 	= nil;
	beam = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->ChainNode = beam;



			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);
	AddToSplineObjectList(newObj, true);



	return(true);
}


/******************** MOVE RAIL GUN ON SPLINE ***************************/

static void MoveRailGunOnSpline(ObjNode *theNode)
{
Boolean isVisible;
float	oldX, oldZ;
int		i;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	oldX = theNode->Coord.x;
	oldZ = theNode->Coord.z;
	IncreaseSplineIndex(theNode, 100);
	GetObjectCoordOnSpline(theNode);

	theNode->Delta.x = (theNode->Coord.x - oldX) * gFramesPerSecond;	// set deltas that we just moved
	theNode->Delta.z = (theNode->Coord.z - oldZ) * gFramesPerSecond;

			/***************************/
			/* UPDATE STUFF IF VISIBLE */
			/***************************/

	if (isVisible)
	{
		ObjNode	*beam = theNode->ChainNode;

		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		UpdateObjectTransforms(theNode);					// update transforms
		CalcObjectBoxFromNode(theNode);						// calc current collision box


				/* UPDATE SPARKLE */

		i = theNode->Sparkles[0];
		if (i != -1)
		{
			gSparkles[i].where.x = theNode->Coord.x;
			gSparkles[i].where.z = theNode->Coord.z;
		}

			/* UPDATE THE BEAM */

		beam->ColorFilter.a = .99f - RandomFloat()*.2f;					// flicker
		beam->Scale.x = beam->Scale.z = RAIL_GUN_SCALE + RandomFloat() * .1f;
		beam->Coord = theNode->Coord;

		TurnObjectTowardTarget(beam, &beam->Coord, gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z, 100, false);	// aim at camera
		UpdateObjectTransforms(beam);

		MO_Object_OffsetUVs(beam->BaseGroup, 0, RandomFloat2() * 20.0f);


			/* SEE IF HURT PLAYER */

		if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < (gPlayerInfo.objNode->BBox.max.x * gPlayerInfo.objNode->Scale.x))
		{
			ImpactPlayerSaucer(theNode->Coord.x, theNode->Coord.z, gFramesPerSecondFrac * 2.0f, gPlayerInfo.objNode,10);
		}
	}
}

#pragma mark -




/************************* ADD TURRET *********************************/

Boolean AddTurret(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*base,*turret;
					/********************/
					/* MAKE TURRET BASE */
					/********************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SAUCER_ObjType_TurretBase;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB - 20;
	gNewObjectDefinition.moveCall 	= MoveTurret;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= TURRET_SCALE;
	base = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	base->TerrainItemPtr = itemPtr;							// keep ptr to item list

			/* SET COLLISION STUFF */

	base->CType 			= CTYPE_MISC;
	base->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(base, 1,1);

	base->HurtCallback = HurtTurret;


			/***********************/
			/* MAKE THE TURRET GUN */
			/***********************/

	gNewObjectDefinition.type 		= SAUCER_ObjType_Turret;
	gNewObjectDefinition.coord.y 	+= 114.0f * TURRET_SCALE;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	turret = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	base->ChainNode = turret;

	return(true);
}


/**************************** MOVE TURRET *******************************/

static void MoveTurret(ObjNode *base)
{
ObjNode	*turret = base->ChainNode;
int		i,n;
float	fps = gFramesPerSecondFrac;

			/* IF SAUCER IS CLOSE ENOUGH THEN AIM TURRET */

	turret->ShootTimer -= fps;

	if (CalcQuickDistance(base->Coord.x, base->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < 1600.0f)
	{
		float	angle =  TurnObjectTowardTarget(turret, nil, gPlayerInfo.coord.x, gPlayerInfo.coord.z, PI/5, false);

					/* IF AIMED CLOSE, THEN SHOOT */

		if (angle < (PI/2))
		{
			float angle2 = TurnObjectTowardTargetOnX(turret, &turret->Coord, &gPlayerInfo.coord, PI/5);

			if (angle2 < (PI/3))
			{
				if (turret->ShootTimer <= 0.0f)
					ShootTurret(turret);
			}
		}
	}

				/* UPDATE TURRET */

	UpdateObjectTransforms(turret);

			/* UPDATE MUZZLE SPARKLE(S) */

	for (n = 0; n < 2; n++)
	{
		i = turret->Sparkles[n];								// get sparkle index
		if (i != -1)
		{
			gSparkles[i].color.a -= fps * 4.0f;						// fade out
			if (gSparkles[i].color.a <= 0.0f)
			{
				DeleteSparkle(i);
				turret->Sparkles[n] = -1;
			}
		}
	}

			/* TRACK IT AND DO THE SPECIAL SAUCER STUFF */

	MoveObjectUnderPlayerSaucer(base);
}


/*********************** SHOOT TURRET **************************/

static void ShootTurret(ObjNode *turret)
{
int		n,i;
OGLPoint3D	muzzleCoord;
OGLVector3D	muzzleVector;
ObjNode	*newObj;

	turret->ShootTimer = .2f;											// reset delay

	turret->MuzzleSide ^= 1;											// alternate sides
	n = turret->MuzzleSide & 1;											// see which muzzle to shoot from


		/*********************/
		/* MAKE MUZZLE FLASH */
		/*********************/

	if (turret->Sparkles[n] != -1)							// see if delete existing sparkle
	{
		DeleteSparkle(turret->Sparkles[0]);
		turret->Sparkles[n] = -1;
	}

	i = turret->Sparkles[n] = GetFreeSparkle(turret);		// make new sparkle
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_TRANSFORMWITHOWNER|SPARKLE_FLAG_OMNIDIRECTIONAL;
		gSparkles[i].where = gMuzzleTipOff[n];

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale 	= 200.0f;
		gSparkles[i].separation = 100.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark4;
	}

			/************************/
			/* CREATE WEAPON OBJECT */
			/************************/

		/* CALC COORD & VECTOR OF MUZZLE */

	OGLPoint3D_Transform(&gMuzzleTipOff[n], &turret->BaseTransformMatrix, &muzzleCoord);
	OGLVector3D_Transform(&gMuzzleTipAim, &turret->BaseTransformMatrix, &muzzleVector);

				/* MAKE OBJECT */

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_StunPulseBullet;
	gNewObjectDefinition.coord		= muzzleCoord;
	gNewObjectDefinition.flags 		= STATUS_BIT_USEALIGNMENTMATRIX|STATUS_BIT_GLOW|STATUS_BIT_NOZWRITES|
									STATUS_BIT_NOFOG|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-1;	//turret->Slot-2;					// dont move until next frame
	gNewObjectDefinition.moveCall 	= MoveTurretBullet;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);


	newObj->Kind = WEAPON_TYPE_STUNPULSE;

	newObj->ColorFilter.a = .99;			// do this just to turn on transparency so it'll glow

	newObj->Delta.x = muzzleVector.x * 2000.0f;
	newObj->Delta.y = muzzleVector.y * 2000.0f;
	newObj->Delta.z = muzzleVector.z * 2000.0f;

	newObj->Health = 1.0f;

				/* COLLISION */

	newObj->CType = CTYPE_WEAPON;
	newObj->CBits = CBITS_TOUCHABLE;
	SetObjectCollisionBounds(newObj, 50,-50,-50,50,50,-50);
	newObj->Damage = .05f;

			/* SET THE ALIGNMENT MATRIX */

	SetAlignmentMatrix(&newObj->AlignmentMatrix, &muzzleVector);


	PlayEffect_Parms3D(EFFECT_STUNGUN, &newObj->Coord, NORMAL_CHANNEL_RATE * 2/3, 2.0);

}

/******************* MOVE TURRET BULLET ***********************/

static void MoveTurretBullet(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		i;
			/* SEE IF GONE */

	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}


	GetObjectInfo(theNode);

			/* MOVE IT */

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/***********************/
			/* SEE IF HIT ANYTHING */
			/***********************/

	if (gCoord.y <= GetTerrainY(gCoord.x, gCoord.z))					// see if hit ground
	{
		ExplodeStunPulse(theNode);
		return;
	}

	if (DoSimpleBoxCollision(gCoord.y + 40.0f, gCoord.y - 40.0f, gCoord.x - 40.0f, gCoord.x + 40.0f,
							gCoord.z + 40.0f, gCoord.z - 40.0f, CTYPE_MISC | CTYPE_PLAYER))

	{
		for (i = 0; i < gNumCollisions; i++)								// affect all hit objects
		{
			ObjNode *hitObj = gCollisionList[i].objectPtr;					// get hit ObjNode
			if (hitObj)
			{
				if (hitObj->CType & CTYPE_PLAYER)
				{
					PlayEffect3D(EFFECT_SAUCERHIT, &gCoord);
					ImpactPlayerSaucer(gCoord.x, gCoord.z, .03, hitObj, 0);
					break;
				}
			}
		}


		ExplodeStunPulse(theNode);
		return;
	}




	UpdateObject(theNode);
}



/******************* HURT TURRET *****************************/

static Boolean HurtTurret(ObjNode *base, float unused)
{
#pragma unused (unused)

	PlayEffect_Parms3D(EFFECT_SAUCERKABOOM, &base->Coord, NORMAL_CHANNEL_RATE, 2.0);
	MakeSparkExplosion(base->Coord.x, base->Coord.y, base->Coord.z, 400.0f, 1.0, PARTICLE_SObjType_RedSpark, 0);
	ExplodeGeometry(base, 700, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .5);
	DeleteObject(base);

	return(false);										// return value doesn't mean anything
}






