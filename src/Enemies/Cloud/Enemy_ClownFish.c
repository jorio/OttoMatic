/****************************/
/*   ENEMY: CLOWNFISH.C			*/
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

static void MoveClownFishOnSpline(ObjNode *theNode);

static Boolean ClownFishHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);

static void SeeIfClownFishAttack(ObjNode *theNode);
static Boolean HurtClownFish(ObjNode *enemy, float damage);

static void LoadClownFishWithBomb(ObjNode *fish);
static void DropTheBomb(ObjNode *fish);
static void MoveFishBomb(ObjNode *theNode);
static void ExplodeFishBomb(ObjNode *theNode);
static void MoveShockwave(ObjNode *theNode);
static void MoveConeBlast(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_CLOWNFISHS					8

#define	CLOWNFISH_SCALE					2.0f

#define	CLOWNFISH_DAMAGE				.33f

#define	CLOWN_FISH_HOVERHEIGHT			700.0f

#define	CLOWNFISH_ATTACK_DIST			700.0f

#define	CLOWNFISH_HEALTH				.1f


		/* ANIMS */


enum
{
	CLOWNFISH_ANIM_SWIM
};




/*********************/
/*    VARIABLES      */
/*********************/

static const OGLPoint3D gBombOff = {0,-50,0};


/************************ PRIME CLOWNFISH ENEMY *************************/

Boolean PrimeEnemy_ClownFish(long splineNum, SplineItemType *itemPtr)
{
ObjNode			*newObj;
float			x,z,placement;

			/* GET SPLINE INFO */

	placement = itemPtr->placement;

	GetCoordOnSpline(&(*gSplineList)[splineNum], placement, &x, &z);

				/***********************/
				/* MAKE SKELETON ENEMY */
				/***********************/

	newObj = MakeEnemySkeleton(SKELETON_TYPE_CLOWNFISH,x,z, CLOWNFISH_SCALE, 0, nil);


	newObj->SplineItemPtr = itemPtr;
	newObj->SplineNum = splineNum;

	newObj->Skeleton->AnimSpeed = 2.0f;

				/* SET BETTER INFO */

	newObj->InitCoord 		= newObj->Coord;							// remember where started
	newObj->StatusBits		|= STATUS_BIT_ONSPLINE;
	newObj->SplinePlacement = placement;
	newObj->Coord.y 		+= CLOWN_FISH_HOVERHEIGHT;
	newObj->SplineMoveCall 	= MoveClownFishOnSpline;					// set move call
	newObj->Health 			= CLOWNFISH_HEALTH;
	newObj->Damage 			= CLOWNFISH_DAMAGE;
	newObj->Kind 			= ENEMY_KIND_CLOWNFISH;


				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);


				/* SET WEAPON HANDLERS */

	for (int i = 0; i < NUM_WEAPON_TYPES; i++)
		newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= ClownFishHitByWeapon;

	newObj->HurtCallback = HurtClownFish;							// set hurt callback function


				/* MAKE SHADOW & GLOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 14, 10, false);

				/* GIVE IT THE BOMB */

	LoadClownFishWithBomb(newObj);



			/* ADD SPLINE OBJECT TO SPLINE OBJECT LIST */

	DetachObject(newObj, true);										// detach this object from the linked list
	AddToSplineObjectList(newObj, true);



	return(true);
}


/******************** MOVE CLOWNFISH ON SPLINE ***************************/

static void MoveClownFishOnSpline(ObjNode *theNode)
{
Boolean isVisible;

	isVisible = IsSplineItemVisible(theNode);					// update its visibility

		/* MOVE ALONG THE SPLINE */

	IncreaseSplineIndex(theNode, 150);
	GetObjectCoordOnSpline(theNode);


			/* UPDATE STUFF IF VISIBLE */

	if (isVisible)
	{

		theNode->Rot.y = CalcYAngleFromPointToPoint(theNode->Rot.y, theNode->OldCoord.x, theNode->OldCoord.z,			// calc y rot aim
												theNode->Coord.x, theNode->Coord.z);

		UpdateObjectTransforms(theNode);											// update transforms
		UpdateShadow(theNode);

				/* DO SOME COLLISION CHECKING */

		GetObjectInfo(theNode);
		if (DoEnemyCollisionDetect(theNode,CTYPE_HURTENEMY, false))					// just do this to see if explosions hurt
			return;


			/* UPDATE THE BOMB */

		SeeIfClownFishAttack(theNode);									// see if attack first

		if (theNode->ChainNode != nil)
		{
			ObjNode	*bomb = theNode->ChainNode;

			FindCoordOnJoint(theNode, 0, &gBombOff, &bomb->Coord);
			bomb->Rot.y = theNode->Rot.y;
			UpdateObjectTransforms(bomb);
		}

				/* IF CULLED THEN GIVE BOMB */

		if (theNode->StatusBits & STATUS_BIT_ISCULLED)
		{
			if (theNode->ChainNode == nil)
			{
				if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) > (CLOWNFISH_ATTACK_DIST * 2))		// gotta be far away to reload
					LoadClownFishWithBomb(theNode);
			}
		}
	}
}



#pragma mark -

/************ CLOWNFISH HIT BY STUN PULSE *****************/
//
// Called when weapon is moved and this handler is called after a collision.
//
// Also used for Flame weapon!
//

static Boolean ClownFishHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord)


			/* HURT IT */

	HurtClownFish(enemy, weapon->Damage);


			/* GIVE MOMENTUM */

	enemy->Delta.x += weaponDelta->x * .4f;
	enemy->Delta.y += weaponDelta->y * .4f  + 200.0f;
	enemy->Delta.z += weaponDelta->z * .4f;

	return(true);			// stop weapon
}



/*********************** HURT CLOWNFISH ***************************/

static Boolean HurtClownFish(ObjNode *enemy, float damage)
{
#pragma unused(damage)

	PlayEffect3D(EFFECT_FISHBOOM, &enemy->Coord);
	ExplodeGeometry(enemy, 200, SHARD_MODE_FROMORIGIN, 1, 1.0);
	DeleteObject(enemy);

	return(true);
}




#pragma mark -

/************ SEE IF CLOWNFISH ATTACK *********************/

static void SeeIfClownFishAttack(ObjNode *theNode)
{
	if (theNode->ChainNode == nil)							// see if even has a bomb
		return;

	if (!gPlayerHasLanded)
		return;

	if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) > CLOWNFISH_ATTACK_DIST)
		return;



	DropTheBomb(theNode);
}



/******************* LOAD CLOWNFISH WITH BOMB ***********************/

static void LoadClownFishWithBomb(ObjNode *fish)
{
ObjNode *bomb;

	FindCoordOnJoint(fish, 0, &gBombOff, &gNewObjectDefinition.coord);


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOUD_ObjType_FishBomb;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= fish->Rot.y;
	gNewObjectDefinition.scale 		= fish->Scale.x;
	bomb = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	fish->ChainNode = bomb;

	CreateCollisionBoxFromBoundingBox_Rotated(bomb,.8,1);			// set collision box but no Ctype since only for checking collisions

}



/************************* DROP THE BOMB ****************************/

static void DropTheBomb(ObjNode *fish)
{
ObjNode	*bomb = fish->ChainNode;

	fish->ChainNode = nil;							// detach from fish
	bomb->MoveCall = MoveFishBomb;					// and assign a move call

	bomb->Delta.x = -sin(fish->Rot.y) * 200.0f;		// give it some delta
	bomb->Delta.z = -cos(fish->Rot.y) * 200.0f;

	bomb = AttachShadowToObject(bomb, SHADOW_TYPE_CIRCULAR, 8, 10, false);

}


/*********************** MOVE FISH BOMB *****************************/

static void MoveFishBomb(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

			/* DO GRAVITY AND MOTION */

	gDelta.y -= 1000.0f * fps;

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


			/* TILT DOWN */

	theNode->Rot.x -= 1.5f * fps;
	if (theNode->Rot.x < (-PI/2))
		theNode->Rot.x = -PI/2;


			/* SEE IF HIT */

	if (HandleCollisions(theNode, CTYPE_TERRAIN | CTYPE_MISC | CTYPE_PLAYER, 0))
	{
		ExplodeFishBomb(theNode);
		return;
	}


	UpdateObject(theNode);


		/* UPDATE SOUND */

	if (theNode->EffectChannel == -1)
		theNode->EffectChannel = PlayEffect3D(EFFECT_BOMBDROP, &gCoord);
	else
		Update3DSoundChannel(EFFECT_BOMBDROP, &theNode->EffectChannel, &gCoord);

}


/*************** EXPLODE FISH BOMB ******************************/

static void ExplodeFishBomb(ObjNode *theNode)
{
long					pg,i;
OGLVector3D				d;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
float					x,y,z;
ObjNode					*newObj;
DeformationType		defData;

		/*********************/
		/* FIRST MAKE SPARKS */
		/*********************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
	gNewParticleGroupDef.gravity				= 100;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 10;
	gNewParticleGroupDef.decayRate				=  .5;
	gNewParticleGroupDef.fadeRate				= 1.0;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_BlueSpark;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;
	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		x = gCoord.x;
		y = gCoord.y;
		z = gCoord.z;

		for (i = 0; i < 20; i++)
		{
			d.y = RandomFloat2() * 300.0f;
			d.x = RandomFloat2() * 300.0f;
			d.z = RandomFloat2() * 300.0f;

			pt.x = x + d.x * 100.0f;
			pt.y = y + d.y * 100.0f;
			pt.z = z + d.z * 100.0f;



			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &d;
			newParticleDef.scale		= RandomFloat() + 1.5f;
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= 0;
			newParticleDef.alpha		= FULL_ALPHA + (RandomFloat() * .3f);
			AddParticleToGroup(&newParticleDef);
		}
	}

		/******************/
		/* MAKE SHOCKWAVE */
		/******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOUD_ObjType_Shockwave;
	gNewObjectDefinition.coord		= gCoord;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG|STATUS_BIT_GLOW|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOZWRITES|STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+30;
	gNewObjectDefinition.moveCall 	= MoveShockwave;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .2;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	newObj->ColorFilter.a = .99;

	newObj->Damage = .2f;

		/*******************/
		/* MAKE CONE BLAST */
		/*******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOUD_ObjType_ConeBlast;
	gNewObjectDefinition.coord.x 	= gCoord.x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(gCoord.x, gCoord.z);
	gNewObjectDefinition.coord.z 	= gCoord.z;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOFOG|STATUS_BIT_GLOW|STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOZWRITES;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+11;
	gNewObjectDefinition.moveCall 	= MoveConeBlast;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= .3;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	newObj->ColorFilter.a = .99;


		/*************************/
		/* MAKE DEFORMATION WAVE */
		/*************************/

	defData.type 				= DEFORMATION_TYPE_RADIALWAVE;
	defData.amplitude 			= 100;
	defData.radius 				= 20;
	defData.speed 				= 3000;
	defData.origin.x			= gCoord.x;
	defData.origin.y			= gCoord.z;
	defData.oneOverWaveLength 	= 1.0f / 500.0f;
	defData.radialWidth			= 800.0f;
	defData.decayRate			= 300.0f;
	NewSuperTileDeformation(&defData);

	ExplodeGeometry(theNode, 500, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, 1.0);
	PlayEffect3D(EFFECT_BIRDBOMBBOOM, &gCoord);

	DeleteObject(theNode);



}




/***************** MOVE SHOCKWAVE ***********************/

static void MoveShockwave(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
float	x,y,z,w;


	theNode->Scale.x = theNode->Scale.y = theNode->Scale.z += 11.0f * fps;

	theNode->ColorFilter.a -= fps * 3.0f;
	if (theNode->ColorFilter.a <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}

	UpdateObjectTransforms(theNode);


			/* SEE IF HIT PLAYER */

	x = theNode->Coord.x;
	y = theNode->Coord.y;
	z = theNode->Coord.z;
	w = theNode->Scale.x * theNode->BBox.max.x;

	if (DoSimpleBoxCollisionAgainstPlayer(y+10, y-10, x - w, x + w,	z + w, z - w))
	{
		ObjNode	*player = gPlayerInfo.objNode;

		player->Rot.y = CalcYAngleFromPointToPoint(player->Rot.y,x,z,player->Coord.x, player->Coord.z);		// aim player direction of blast
		PlayerGotHit(nil, theNode->Damage);																			// get hit
		player->Delta.x *= 2.0f;
		player->Delta.z *= 2.0f;
//		player->Delta.y *= 2.0f;
	}
}


/***************** MOVE CONEBLAST ***********************/

static void MoveConeBlast(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	theNode->Scale.x = theNode->Scale.y = theNode->Scale.z += fps * 5.0f;

	theNode->ColorFilter.a -= fps * 1.5f;
	if (theNode->ColorFilter.a <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}

	UpdateObjectTransforms(theNode);
}

















