/****************************/
/*   ENEMY: BLOB.C			*/
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

static ObjNode *MakeEnemy_Blob(long x, long z);
static void MoveBlob(ObjNode *theNode);
static void  MoveBlob_Walking(ObjNode *theNode);
static void UpdateBlob(ObjNode *theNode);
static void SetBlobColor(ObjNode *blob);

static void BlobHitByJumpJet(ObjNode *enemy);
static Boolean BlobEnemyHitByEnergy(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean BlobEnemyHitByFreezeGun(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void KillBlob(ObjNode *enemy);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_BLOBS					6

#define	BLOB_SCALE					2.4f

#define	BLOB_CHASE_DIST			2000.0f
#define	BLOB_DETACH_DIST		(BLOB_CHASE_DIST / 2.0f)
#define	BLOB_CEASE_DIST			(BLOB_CHASE_DIST + 500.0f)
#define	BLOB_ATTACK_DIST		1200.0f

#define	BLOB_TARGET_OFFSET		100.0f

#define BLOB_TURN_SPEED			2.4f
#define BLOB_WALK_SPEED			170.0f

#define	BLOB_DAMAGE				.25f


		/* ANIMS */


enum
{
	BLOB_ANIM_WALK
};


/*********************/
/*    VARIABLES      */
/*********************/

#define	ThawDelay SpecialF[0]

#define	BlobColorType		Special[0]			// 0 = red, 1= green, 2 = blue
#define	DelayUntilAttack	SpecialF[2]


/************************ ADD BLOB ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_Blob(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*newObj;

	if (gNumEnemies >= gMaxEnemies)								// keep from getting absurd
		return(false);

	if (!(itemPtr->parm[3] & 1))								// see if always add
	{
		if (gNumEnemyOfKind[ENEMY_KIND_BLOB] >= MAX_BLOBS)
			return(false);
	}


	newObj = MakeEnemy_Blob(x,z);
	newObj->TerrainItemPtr = itemPtr;
	newObj->EnemyRegenerate = itemPtr->parm[3] & (1<<1);

	newObj->BlobColorType = itemPtr->parm[0];					// save color type


	return(true);
}


/************************ MAKE BLOB ENEMY *************************/

static ObjNode *MakeEnemy_Blob(long x, long z)
{
ObjNode	*newObj;

			/****************************/
			/* MAKE NEW SKELETON OBJECT */
			/****************************/

	gNewObjectDefinition.type 		= SKELETON_TYPE_BLOB;
	gNewObjectDefinition.animNum 	= BLOB_ANIM_WALK;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_UVTRANSFORM;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= BLOB_SCALE;

	newObj = MakeNewSkeletonObject(&gNewObjectDefinition);



				/*******************/
				/* SET BETTER INFO */
				/*******************/

	newObj->ColorFilter.a = .85;

	newObj->MoveCall 	= MoveBlob;							// set move call
	newObj->Health 		= 1.0;
	newObj->Damage 		= .12;
	newObj->Kind 		= ENEMY_KIND_BLOB;


				/* SET COLLISION INFO */

	newObj->CType = CTYPE_ENEMY|CTYPE_BLOCKCAMERA|CTYPE_AUTOTARGETWEAPON;
	newObj->CBits = CBITS_TOUCHABLE;						// blobs are not solid

	CreateCollisionBoxFromBoundingBox(newObj, 1,.7);
	CalcNewTargetOffsets(newObj,BLOB_TARGET_OFFSET);


				/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= BlobEnemyHitByEnergy;
	newObj->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= BlobEnemyHitByEnergy;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] 		= BlobEnemyHitByEnergy;
	newObj->HitByWeaponHandler[WEAPON_TYPE_FREEZE] 		= BlobEnemyHitByFreezeGun;
	newObj->HitByJumpJetHandler = BlobHitByJumpJet;

	newObj->WeaponAutoTargetOff.y = newObj->BBox.max.y * .9f;	// shoot @ this point


				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 11, 11,false);

	SetBlobColor(newObj);


	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_BLOB]++;

	return(newObj);
}


/************************ SET BLOB COLOR *************************/

static void SetBlobColor(ObjNode *blob)
{
float	h;

	h = blob->Health;

	switch(blob->BlobColorType)
	{
		case	0:									// RED
				blob->ColorFilter.g = 1.0f - h;
				blob->ColorFilter.b = 1.0f - h;
				blob->ColorFilter.r = 1.0f;
				break;

		case	1:									// GREEN
				blob->ColorFilter.r = 1.0f - h;
				blob->ColorFilter.b = 1.0f - h;
				blob->ColorFilter.g = 1.0f;
				break;

		case	2:									// AQUA
				blob->ColorFilter.r = 1.0f - h;
				blob->ColorFilter.g = 1.0f - (h * .4f);
				blob->ColorFilter.b = 1.0f;
				break;

		default:
				DoFatalAlert("SetBlobColor: undefined blob color type");
	}

}


/********************* MOVE BLOB **************************/

static void MoveBlob(ObjNode *theNode)
{
static	void(*myMoveTable[])(ObjNode *) =
				{
					MoveBlob_Walking
				};

	if (TrackTerrainItem(theNode))						// just check to see if it's gone
	{
		DeleteEnemy(theNode);
		return;
	}

	GetObjectInfo(theNode);
	myMoveTable[theNode->Skeleton->AnimNum](theNode);
}


/********************** MOVE BLOB: WALKING ******************************/

static void  MoveBlob_Walking(ObjNode *theNode)
{
float		r,fps,dist;
float		health = theNode->Health;
float		speed;


	if (gPlayerHasLanded && (theNode->Scale.x >= BLOB_SCALE))
		speed = BLOB_WALK_SPEED * health;			// speed is based on health
	else
		speed = 0;


	fps = gFramesPerSecondFrac;

			/* MOVE TOWARD PLAYER */

	TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, BLOB_TURN_SPEED * health, true);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * speed;
	gDelta.z = -cos(r) * speed;
	gDelta.y -= ENEMY_GRAVITY*fps;									// add gravity
	MoveEnemy(theNode);


	theNode->Skeleton->AnimSpeed = health;							// anim speed is based on health

				/* DO BASIC COLLISION */

	if (DoEnemyCollisionDetect(theNode,CTYPE_MISC|CTYPE_FENCE, false))		// only do on some basic things
		return;

				/***************************/
				/* SEE IF ENGULFING PLAYER */
				/***************************/

	if (!gPlayerIsDead)
	{
		if (DoSimpleBoxCollisionAgainstPlayer(gCoord.y + theNode->TopOff, gCoord.y + theNode->BottomOff,
											gCoord.x + theNode->LeftOff, gCoord.x + theNode->RightOff,
											gCoord.z + theNode->FrontOff, gCoord.z + theNode->BackOff))
		{
			ApplyFrictionToDeltas(gPlayerInfo.objNode->Speed3D * 3.5f, &gPlayerInfo.objNode->Delta);		// blob is viscous so slow player
			PlayerLoseHealth(theNode->Damage * fps, PLAYER_DEATH_TYPE_EXPLODE);								// lose health
			MakeSteam(theNode, gPlayerInfo.coord.x, gCoord.y + theNode->BottomOff, gPlayerInfo.coord.z);
		}
	}


			/* CHECK RANGE */

	dist = CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
	if (dist > BLOB_CEASE_DIST)										// see if stop chasing
	{
		gDelta.x = gDelta.z = 0;
	}

	UpdateBlob(theNode);
}



/***************** UPDATE BLOB ************************/

static void UpdateBlob(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/* UPDATE SCALE */

	if (theNode->Scale.x < BLOB_SCALE)
	{
		theNode->Scale.x = theNode->Scale.y = theNode->Scale.z += fps * .5f;
	}


	theNode->DelayUntilAttack -= fps;

	UpdateEnemy(theNode);
	RotateOnTerrain(theNode, 0, nil);							// set transform matrix

	theNode->BottomOff = theNode->BBox.min.y;					// use the bbox as the collision bottom


			/* UPDATE TEXTURE ANIM */

	theNode->TextureTransformU += fps * theNode->Health;		// animation speed is bassed on health of blob

	SetBlobColor(theNode);


				/* THAW */

	theNode->ThawDelay -= fps;
	if (theNode->ThawDelay <= 0.0f)
	{
		theNode->Health += fps * .1f;
		if (theNode->Health >= 1.0f)					// see if thawed
		{
			theNode->Health = 1.0;
			theNode->CBits = CBITS_ALLSOLID;		// not solid anymore
		}
	}


			/* UPDATE SOUND */

	if (theNode->EffectChannel == -1)
		theNode->EffectChannel = PlayEffect_Parms3D(EFFECT_BLOBMOVE, &gCoord, NORMAL_CHANNEL_RATE / 2, .5);
	else
		Update3DSoundChannel(EFFECT_BLOBMOVE, &theNode->EffectChannel, &gCoord);

}


#pragma mark -

/************ BLOB HIT BY ENERGY WEAPON *****************/
//
// Called when weapon is moved and this handler is called after a collision.
//

static Boolean BlobEnemyHitByEnergy(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon,	weaponCoord, weaponDelta)

			/* SHATTER IT IF HEALTH IS 0, THUS FROZEN */

	if (enemy->Health == 0.0f)
	{
		KillBlob(enemy);
	}

			/* NOT FROZEN, SO JUST HELP IT THAW */
	else
	{
		enemy->Health += .5f;
		if (enemy->Health > 1.0f)
			enemy->Health = 1.0;
	}

	return(true);
}


/************ BLOB HIT BY FREEZE GUN *****************/
//
// Called when weapon is moved and this handler is called after a collision.
//

static Boolean BlobEnemyHitByFreezeGun(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon,	weaponCoord, weaponDelta)


		/* SEE IF FROZEN */

	enemy->Health -= .55f;
	if (enemy->Health <= 0.0f)
	{
		enemy->Health = 0;
		enemy->ThawDelay = 4;				// # seconds to wait before starting to thaw
		enemy->CBits = CBITS_ALLSOLID;		// solid now that its frozen
	}

	return(true);
}



/************** BLOB GOT HIT BY JUMP JET *****************/

static void BlobHitByJumpJet(ObjNode *enemy)
{
	KillBlob(enemy);
}


/****************** KILL BLOB *********************/
//
// OUTPUT: true if ObjNode was deleted
//

static void KillBlob(ObjNode *enemy)
{
ObjNode	*newObj;
int		i;
float	dx,dy,dz;

		/*****************/
		/* CREATE CHUNKS */
		/*****************/

	for (i = 0; i < 5; i++)
	{
		dx = RandomFloat2() * 500.0f;
		dz = RandomFloat2() * 500.0f;
		dy = 300.0f + RandomFloat() * 400.0f;

		gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
		gNewObjectDefinition.type 		= SLIME_ObjType_BlobChunk;
		gNewObjectDefinition.coord.x 	= enemy->Coord.x + dx * .4f;
		gNewObjectDefinition.coord.y 	= enemy->Coord.y + dy * .3f;
		gNewObjectDefinition.coord.z 	= enemy->Coord.z + dz * .4f;
		gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
		gNewObjectDefinition.slot 		= SLOT_OF_DUMB-1;
		gNewObjectDefinition.moveCall 	= MoveBlobChunk;
		gNewObjectDefinition.rot 		= 0;
		gNewObjectDefinition.scale 		= .8;
		newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		switch(enemy->BlobColorType)					// set powerup type based on blob color
		{
			case	0:									// red == health
					newObj->POWType = POW_TYPE_HEALTH;
					break;

			case	1:									// green == jump-jet
					newObj->POWType = POW_TYPE_JUMPJET;
					break;

			case	2:									// blue == fuel
					newObj->POWType = POW_TYPE_FUEL;
					break;

			default:
					DoFatalAlert("KillBlob: illegal BlobColorType");
		}


		newObj->Delta.x = dx;
		newObj->Delta.y = dy;
		newObj->Delta.z = dz;

		newObj->DeltaRot.x = RandomFloat2() * 10.0f;
		newObj->DeltaRot.z = RandomFloat2() * 10.0f;


				/* SET COLLISION STUFF */

		newObj->CType 			= CTYPE_MISC | CTYPE_POWERUP | CTYPE_BLOBPOW;
		newObj->CBits			= CBITS_ALLSOLID;

		CreateCollisionBoxFromBoundingBox(newObj, .9,.9);


		newObj->ThawDelay = 6.0f + RandomFloat() * 5.0f;
		newObj->BlobColorType = enemy->BlobColorType;
		newObj->Health = 0;

		AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 2,2, true);
	}


		/* DO FINAL STUFF */

	ExplodeGeometry(enemy, 400, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 2, .6);
	PlayEffect3D(EFFECT_SHATTER, &enemy->Coord);


	if (!enemy->EnemyRegenerate)
		enemy->TerrainItemPtr = nil;							// dont ever come back

	DeleteEnemy(enemy);
}


/************************* MOVE BLOB CHUNK ***************************/

void MoveBlobChunk(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);


			/* MOVE IT */

	gDelta.y -= 1500.0f * fps;								// gravity

	gCoord.x += gDelta.x * fps;								// move it
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.x += theNode->DeltaRot.x * fps;			// spin
	theNode->Rot.y += theNode->DeltaRot.y * fps;
	theNode->Rot.z += theNode->DeltaRot.z * fps;


			/* COLLISION */

	HandleCollisions(theNode, CTYPE_MISC|CTYPE_ENEMY|CTYPE_PLAYER|CTYPE_TERRAIN|CTYPE_FENCE, -.8);

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)
		theNode->DeltaRot.x = theNode->DeltaRot.y = theNode->DeltaRot.z = 0;


			/* SEE IF THAWING */

	if (theNode->ThawDelay <= 0.0f)
	{
		theNode->Health += fps * .5f;
		if (theNode->Health >= 1.0f)						// see if done thawing
		{
			ObjNode	*newBlob;

			theNode->Health = 1.0f;


				/* SEE IF MAKE NEW BLOB ENEMY */

			if (gNumEnemyOfKind[ENEMY_KIND_BLOB] < MAX_BLOBS)
			{
				newBlob = MakeEnemy_Blob(gCoord.x, gCoord.z);					// make new blob enemy
				newBlob->BlobColorType = theNode->BlobColorType;				// set correct color
				newBlob->Scale.x = newBlob->Scale.y = newBlob->Scale.z = BLOB_SCALE * .1f;	// start shrunken

				theNode->MoveCall = MoveBlobChunk_Thawed;						// change chunks move call
			}
		}

		SetBlobColor(theNode);
	}
	else
		theNode->ThawDelay -= fps;


			/* UPDATE */

	UpdateObject(theNode);


		/* SEE IF DISPLAY HELP */

	if (!gHelpMessageDisabled[HELP_MESSAGE_SLIMEBALLS])
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < HELP_BEACON_RANGE)
		{
			DisplayHelpMessage(HELP_MESSAGE_SLIMEBALLS, .5f, true);
		}
	}
}


/******************** MOVE BLOB CHUNK THAWED ***********************/

void MoveBlobChunk_Thawed(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

		/* FADE OUT */

	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		DeleteObject(theNode);
		return;
	}

	theNode->ColorFilter.a = theNode->Health;
}



#pragma mark -






