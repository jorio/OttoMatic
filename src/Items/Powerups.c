/****************************/
/*   	POWERUPS.C		    */
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

static Boolean PowerupPodGotPunched(ObjNode *weapon, ObjNode *pod, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta);
static void MovePowerupPod(ObjNode *theNode);
static void MoveOrbDebris(ObjNode *theNode);

static ObjNode *MakeAtom(float x, float y, float z, int atomType);
static void MoveAtom(ObjNode *theNode);
static void DrawAtom(ObjNode *theNode);
static void PlayerGotAtom(ObjNode *theNode);
static void CheckIfRegenerateAtom(ObjNode *theNode);
static void CheckIfRegeneratePOW(ObjNode *theNode);

static Boolean MakePowerupBalloon(TerrainItemEntryType *itemPtr, float x, float y, float z);
static void MovePowerupBalloon(ObjNode *string);
static Boolean PowerupBalloonGotHitByStunPulse(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	ATOM_RADIUS			60.0f

enum
{
	ATOM_TYPE_HEALTH,
	ATOM_TYPE_JUMPJET,
	ATOM_TYPE_FUEL
};

#define ATOM_INCREMENT_HEALTH	0.1f
#define ATOM_INCREMENT_JUMPJET	0.1f
#define ATOM_INCREMENT_FUEL		0.05f

#define	POD_SCALE	1.8f

#define	BALLOON_SCALE	1.2f


/*********************/
/*    VARIABLES      */
/*********************/


#define	AtomQuantity		Special[0]
#define	PowerupParm2		Special[1]
#define	IsBalloon			Flag[1]
#define	BalloonWobble		SpecialF[1]

#define	AtomDelayToActive	SpecialF[4]
#define	AtomRegenThreshold	SpecialF[5]
#define	AtomOnWater			Flag[0]



/********************** SPEW ATOMS ***************************/

void SpewAtoms(OGLPoint3D *where, short numRed, short numGreen, short numBlue, Boolean regenerate)
{
ObjNode	*atom;
int		i;

			/* DO RED */

	for (i = 0; i < numRed; i++)
	{
		atom = MakeAtom(where->x, where->y, where->z, ATOM_TYPE_HEALTH);

		atom->Delta.x = RandomFloat2() * 400.0f;
		atom->Delta.y = 500.0f + RandomFloat() * 500.0f;
		atom->Delta.z = RandomFloat2() * 400.0f;
		atom->POWRegenerate = regenerate;
		atom->AtomRegenThreshold = ((float) i) * ATOM_INCREMENT_HEALTH;
	}

			/* DO GREEN */

	for (i = 0; i < numGreen; i++)
	{
		atom = MakeAtom(where->x, where->y, where->z, ATOM_TYPE_JUMPJET);

		atom->Delta.x = RandomFloat2() * 400.0f;
		atom->Delta.y = 500.0f + RandomFloat() * 500.0f;
		atom->Delta.z = RandomFloat2() * 400.0f;
		atom->POWRegenerate = regenerate;
		atom->AtomRegenThreshold = ((float) i) * ATOM_INCREMENT_JUMPJET;
	}

			/* DO BLUE */

	for (i = 0; i < numBlue; i++)
	{
		atom = MakeAtom(where->x, where->y, where->z, ATOM_TYPE_FUEL);

		atom->Delta.x = RandomFloat2() * 400.0f;
		atom->Delta.y = 500.0f + RandomFloat() * 500.0f;
		atom->Delta.z = RandomFloat2() * 400.0f;
		atom->POWRegenerate = regenerate;
	}
}


/************************* ADD ATOM *********************************/

Boolean AddAtom(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
float	ty,y;
static const float	yoff = 100.0f;

	if (itemPtr->parm[3] & (1<<1))							// see if put on terrain only
		ty = GetTerrainY(x,z);
	else
		ty = FindHighestCollisionAtXZ(x, z, CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TERRAIN);

	newObj = MakeAtom(x,ty + yoff,z, itemPtr->parm[0]);
	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->POWRegenerate = itemPtr->parm[3] & 1;			// see if auto-regen

				/* SEE IF ATOM IS ABOVE WATER */

	if (GetWaterY(x, z, &y))
	{
		if (y > ty)
		{
			newObj->Coord.y = y + yoff;
			newObj->AtomOnWater = true;
		}
	}



	return(true);
}



/********************* MAKE ATOM ******************************/
//
// Atom Type:  	0 = red, health
//				1 = green, jumpjet
//				2 = blue, fuel
//

static ObjNode *MakeAtom(float x, float y, float z, int atomType)
{
ObjNode	*newObj;

			/* MAKE OBJECT */

	gNewObjectDefinition.type		= atomType;
	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.flags 		= STATUS_BIT_NOZWRITES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG|
									STATUS_BIT_GLOW|STATUS_BIT_KEEPBACKFACES|gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= WATER_SLOT+1;
	gNewObjectDefinition.moveCall 	= MoveAtom;
	gNewObjectDefinition.rot 		= RandomFloat() * 360.0f;
	gNewObjectDefinition.scale 		= 1.0;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->AtomDelayToActive = .8f;				// set delay before player can get it

	newObj->AtomOnWater = false;					// assume not


			/* SET COLLISION */

	newObj->BBox.min.x = -ATOM_RADIUS;						// build bbox for culling test
	newObj->BBox.min.y = -ATOM_RADIUS;
	newObj->BBox.min.z = -ATOM_RADIUS;
	newObj->BBox.max.x = ATOM_RADIUS;
	newObj->BBox.max.y = ATOM_RADIUS;
	newObj->BBox.max.z = ATOM_RADIUS;
	newObj->BBox.isEmpty = false;

	SetObjectCollisionBounds(newObj, ATOM_RADIUS, -ATOM_RADIUS, -ATOM_RADIUS, ATOM_RADIUS, ATOM_RADIUS, -ATOM_RADIUS);



			/* SET SPECIFICS */

	newObj->CustomDrawFunction = DrawAtom;

	newObj->Rot.x = RandomFloat() * 360.0f;
	newObj->SpecialF[0] = RandomFloat() * 360.0f;

	AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 3,3, false);

	return(newObj);
}


/*********************** MOVE ATOM *************************/

static void MoveAtom(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

			/*****************************************/
			/* SEE IF CHECK REGENERATION STUFF FIRST */
			/*****************************************/

	if (theNode->POWRegenerate)
	{
		if (theNode->StatusBits & STATUS_BIT_HIDDEN)			// if hidden then check if need to regenerate it
		{
			CheckIfRegenerateAtom(theNode);
			return;
		}
	}
	else														// regenerates stick around, so don't track them
	{
			/* SEE IF GONE */

		if (TrackTerrainItem(theNode))							// just check to see if it's gone
		{
			DeleteObject(theNode);
			return;
		}
	}

	GetObjectInfo(theNode);



		/**************/
		/* ANIMATE IT */
		/**************/

			/* SPIN */
			//
			// angles are in degrees, not radians!
			//

	theNode->Rot.y += 450.0f * fps;
	theNode->Rot.x += 500.0f * fps;
	theNode->Rot.z -= 550.0f * fps;
	theNode->SpecialF[0] -= 400.0f * fps;


			/***********/
			/* MOVE IT */
			/***********/

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// do friction if landed
		ApplyFrictionToDeltasXZ(1000.0,&gDelta);

	if (!theNode->AtomOnWater)
		gDelta.y -= 1200.0f * fps;							// gravity (if not on water)

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.x += theNode->DeltaRot.x * fps;
	theNode->Rot.y += theNode->DeltaRot.y * fps;
	theNode->Rot.z += theNode->DeltaRot.z * fps;


		/*********************/
		/* HANDLE COLLISIONS */
		/*********************/

	HandleCollisions(theNode, CTYPE_MISC|CTYPE_TERRAIN|CTYPE_FENCE, -.5);


	/* IF LAND IN WATER THEN RESET COORD */

	if (DoWaterCollisionDetect(theNode, gCoord.x, gCoord.y+theNode->BottomOff, gCoord.z, nil))
	{
		gCoord = theNode->InitCoord;
		gDelta.x =
		gDelta.y =
		gDelta.z = 0;
	}



			/*************************/
			/* SEE IF PLAYER TOUCHED */
			/*************************/

	theNode->AtomDelayToActive -= fps;
	if (theNode->AtomDelayToActive <= 0.0f)
	{
		if (DoSimpleBoxCollisionAgainstPlayer(theNode->Coord.y + 50.0f, theNode->Coord.y - 50.0f,
											theNode->Coord.x - 50.0f, theNode->Coord.x + 50.0f,
											theNode->Coord.z + 50.0f, theNode->Coord.z - 50.0f))
		{
			PlayerGotAtom(theNode);
			return;
		}
	}


			/* UPDATE */

	UpdateObject(theNode);


		/* SEE IF DISPLAY HELP */

	if (!gHelpMessageDisabled[HELP_MESSAGE_HEALTHATOM + theNode->Type])
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < HELP_BEACON_RANGE)
		{
			DisplayHelpMessage(HELP_MESSAGE_HEALTHATOM + theNode->Type, .5f, false);
		}
	}

}


/************* CHECK IF REGENERATE ATOM *********************/

static void CheckIfRegenerateAtom(ObjNode *theNode)
{
Boolean	reGen = false;			// assume no re-gen

	float regenThreshold = theNode->AtomRegenThreshold;

	switch(theNode->Type)
	{
		case	ATOM_TYPE_HEALTH:
				if (gPlayerInfo.health <= regenThreshold)									// if player is out of health
					if (!OGL_IsBBoxVisible(&theNode->BBox, &theNode->BaseTransformMatrix))	// and player won't see it pop back to life...
						reGen = true;														// ... then regenerate it
				break;

		case	ATOM_TYPE_JUMPJET:
				if (gPlayerInfo.jumpJet <= regenThreshold)									// if player is out of jumpjet
					if (!OGL_IsBBoxVisible(&theNode->BBox, &theNode->BaseTransformMatrix))	// and player won't see it pop back to life...
						reGen = true;														// ... then regenerate it
				break;

		case	ATOM_TYPE_FUEL:
				if (gPlayerInfo.fuel <= 0.0f)												// if player is out of fuel
					if (!OGL_IsBBoxVisible(&theNode->BBox, &theNode->BaseTransformMatrix))	// and player won't see it pop back to life...
						reGen = true;														// ... then regenerate it
				break;
	}


	if (reGen)
	{
		theNode->StatusBits &= ~STATUS_BIT_HIDDEN;				// unhide
		if (theNode->ShadowNode)								// and unhide shadow
			theNode->ShadowNode->StatusBits &= ~STATUS_BIT_HIDDEN;

	}
}



/******************** DRAW ATOM ****************************/

static void DrawAtom(ObjNode *theNode)
{
int			atomType = theNode->Type;
static const OGLVector3D 	up = {0,1,0};
OGLMatrix4x4				m;
OGLPoint3D					tc[4];
float						s;
static OGLPoint3D		nucleusCoords[4] =
{
	{-30,30,0},
	{30,30,0},
	{30,-30,0},
	{-30,-30,0},
};



				/****************/
				/* DRAW NUCLEUS */
				/****************/

			/* SUBMIT TEXTURE */

	MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_AtomicNucleus_Red+atomType].materialObject);


			/* CALC COORDS OF VERTICES */

	s = 70.0f + RandomFloat2() * 20.0f;
	nucleusCoords[0].x = -s;	nucleusCoords[0].y = s;
	nucleusCoords[1].x = s;		nucleusCoords[1].y = s;
	nucleusCoords[2].x = s;		nucleusCoords[2].y = -s;
	nucleusCoords[3].x = -s;	nucleusCoords[3].y = -s;

	SetLookAtMatrixAndTranslate(&m, &up, &theNode->Coord, &gGameViewInfoPtr->cameraPlacement.cameraLocation);		// aim at camera & translate
	OGLPoint3D_TransformArray(&nucleusCoords[0], &m, tc, 4);

			/* DRAW IT */


	SetColor4f(1,1,.7f + sinf(theNode->SpecialF[1]) * .3f, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);	glVertex3fv(&tc[0].x);
	glTexCoord2f(1,0);	glVertex3fv(&tc[1].x);
	glTexCoord2f(1,1);	glVertex3fv(&tc[2].x);
	glTexCoord2f(0,1);	glVertex3fv(&tc[3].x);
	glEnd();



				/*************/
				/* DRAW RING */
				/*************/

			/* CALC COORDS OF VERTICES */

	glPushMatrix();

	glTranslatef(theNode->Coord.x, theNode->Coord.y, theNode->Coord.z);
	glRotatef(theNode->Rot.y,0,1,0);
	glRotatef(theNode->Rot.x,1,0,0);


			/* DRAW IT */

	MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_GLOBAL][GLOBAL_SObjType_AtomicRing_Red+atomType].materialObject);
	SetColor4f(1,1,1, .5);
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);	glVertex3f(-60,0,-60);
	glTexCoord2f(1,0);	glVertex3f(60,0,-60);
	glTexCoord2f(1,1);	glVertex3f(60,0,60);
	glTexCoord2f(0,1);	glVertex3f(-60,0,60);
	glEnd();
	glPopMatrix();


			/* CALC COORDS OF VERTICES */

	glPushMatrix();

	glTranslatef(theNode->Coord.x, theNode->Coord.y, theNode->Coord.z);
	glRotatef(theNode->Rot.z,0,1,0);
	glRotatef(theNode->SpecialF[0],1,0,0);


			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,0);	glVertex3f(-ATOM_RADIUS,0,-ATOM_RADIUS);
	glTexCoord2f(1,0);	glVertex3f(ATOM_RADIUS,0,-ATOM_RADIUS);
	glTexCoord2f(1,1);	glVertex3f(ATOM_RADIUS,0,ATOM_RADIUS);
	glTexCoord2f(0,1);	glVertex3f(-ATOM_RADIUS,0,ATOM_RADIUS);
	glEnd();

	SetColor4f(1,1,1,1);
	glPopMatrix();
}


/************** PLAYER GOT ATOM ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

static void PlayerGotAtom(ObjNode *theNode)
{
long	pg,i;
OGLVector3D	delta;
OGLPoint3D	pt;
NewParticleDefType		newParticleDef;
float		x,y,z;


	PlayEffect_Parms3D(EFFECT_ATOMCHIME, &theNode->Coord, NORMAL_CHANNEL_RATE + (RandomFloat2() * 0x3000), 1.5);


			/* SEE WHAT TO DO WITH IT */

	switch(theNode->Type)
	{
		case	ATOM_TYPE_HEALTH:
				gPlayerInfo.health += ATOM_INCREMENT_HEALTH;
				gPlayerInfo.health = MinFloat(1, gPlayerInfo.health);
				DisableHelpType(HELP_MESSAGE_HEALTHATOM);
				break;

		case	ATOM_TYPE_JUMPJET:
				gPlayerInfo.jumpJet += ATOM_INCREMENT_JUMPJET;
				gPlayerInfo.jumpJet = MinFloat(1, gPlayerInfo.jumpJet);
				DisableHelpType(HELP_MESSAGE_JUMPJETATOM);
				break;

		case	ATOM_TYPE_FUEL:
				gPlayerInfo.fuel += ATOM_INCREMENT_FUEL;
				gPlayerInfo.fuel = MinFloat(1, gPlayerInfo.fuel);
				DisableHelpType(HELP_MESSAGE_FUELATOM);
				break;

	}


			/**********************/
			/* MAKE PARTICLE POOF */
			/**********************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE;
	gNewParticleGroupDef.gravity				= 1000;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 15;
	gNewParticleGroupDef.decayRate				=  .3;
	gNewParticleGroupDef.fadeRate				= .8;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_BlueSpark;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;

	x = theNode->Coord.x;
	y = theNode->Coord.y;
	z = theNode->Coord.z;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < 10; i++)
		{
			pt.x = x;
			pt.y = y;
			pt.z = z;

			delta.x = RandomFloat2() * 200.0f;
			delta.y = 300.0f + RandomFloat2() * 80.0f;
			delta.z = RandomFloat2() * 200.0f;

			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= 1.0;
			newParticleDef.rotZ			= RandomFloat();
			newParticleDef.rotDZ		= 100;
			newParticleDef.alpha		= .5f + RandomFloat() * .5f;
			AddParticleToGroup(&newParticleDef);
		}
	}


			/*************/
			/* DELETE IT */
			/*************/

	if (theNode->POWRegenerate)									// if regeneratable, then just hide until needed
	{
		theNode->StatusBits |= STATUS_BIT_HIDDEN;
		if (theNode->ShadowNode)								// and hide shadow
			theNode->ShadowNode->StatusBits |= STATUS_BIT_HIDDEN;
	}
	else
	{
		theNode->TerrainItemPtr = nil;				// never come back
		DeleteObject(theNode);
	}
}

#pragma mark -


/************************* ADD POWERUP POD *********************************/

Boolean AddPowerupPod(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
short	i,j,n;
float	y;


	if (itemPtr->parm[3] & (1<<1))							// see if put on terrain only
		y = GetTerrainY(x,z);
	else
		y = FindHighestCollisionAtXZ(x, z, CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TERRAIN);


				/* SEE IF BALLOON CONTAINER ON CLOUD LEVEL */

	if (itemPtr->parm[3] & (1<<2))
	{
		return (MakePowerupBalloon(itemPtr, x,y,z));
	}

					/*********************/
					/* MAKE POD GEOMETRY */
					/*********************/

	gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
	gNewObjectDefinition.type 		= GLOBAL_ObjType_PowerupOrb;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= y + 30.0f;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
	gNewObjectDefinition.slot 		= 450;
	gNewObjectDefinition.moveCall 	= MovePowerupPod;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= POD_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	if (newObj == nil)
		return(false);

	newObj->TerrainItemPtr = itemPtr;					// keep ptr to item list

	newObj->POWType = itemPtr->parm[0];					// set powerup type

	newObj->AtomQuantity = itemPtr->parm[1];			// set # atoms (if atom type)
	if (newObj->AtomQuantity == 0)						// make sure it's at least 1
		newObj->AtomQuantity = 1;

	newObj->PowerupParm2 = itemPtr->parm[2];			// keep parm2 for special uses

	newObj->POWRegenerate = itemPtr->parm[3] & 1;		// remember if let this POW regenerate



			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_TRIGGER;
	newObj->CBits			= CBITS_ALLSOLID;
	newObj->TriggerSides 	= CBITS_ALLSOLID;							// side(s) to activate it
	SetObjectCollisionBounds(newObj, newObj->BBox.max.y * POD_SCALE, newObj->BBox.min.y * POD_SCALE - 30.0f,
							newObj->BBox.min.x * POD_SCALE, newObj->BBox.max.x * POD_SCALE,
							newObj->BBox.max.z * POD_SCALE, newObj->BBox.min.z * POD_SCALE);

	newObj->Kind = TRIGTYPE_POWERUPPOD;


			/* SET WEAPON HANDLERS */

	newObj->HitByWeaponHandler[WEAPON_TYPE_FIST] = PowerupPodGotPunched;			// set punch callback



			/* SHADOW & SHINE */


	AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 5,5, true);


			/*******************/
			/* ATTACH SPARKLES */
			/*******************/

	if ((gLevelNum == LEVEL_NUM_BRAINBOSS) || (!gG4))					// conserve sparkles on BB level
		n = 4;
	else
		n = 8;

	for (j = 0; j < n; j++)
	{
		i = newObj->Sparkles[j] = GetFreeSparkle(newObj);				// get free sparkle slot
		if (i != -1)
		{
			gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
			gSparkles[i].where.x = newObj->Coord.x;
			gSparkles[i].where.y = newObj->Coord.y;
			gSparkles[i].where.z = newObj->Coord.z;

			gSparkles[i].aim.x =
			gSparkles[i].aim.y =
			gSparkles[i].aim.z = 1;

			gSparkles[i].color.r = 1;
			gSparkles[i].color.g = .3;
			gSparkles[i].color.b = .3;
			gSparkles[i].color.a = 1;

			gSparkles[i].scale = 30.0f;

			gSparkles[i].separation = 10.0f;

			gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark;
		}
	}

	return(true);													// item was added
}



/********************* MOVE POWERUP POD **********************/

static void MovePowerupPod(ObjNode *theNode)
{
short	i,j;
float	r,c,fps = gFramesPerSecondFrac;
float	w,yoff,yoff2;
float	dx,dy,dz;

			/* SEE IF GONE */

	if (TrackTerrainItem_FromInit(theNode))			// track from init coord so stays on same supertile as any platforms
	{
		DeleteObject(theNode);
		return;
	}


	GetObjectInfo(theNode);



			/* MOVE */

	gDelta.y -= gGravity * fps;					// gravity

	dx = gDelta.x;
	dy = gDelta.y;
	dz = gDelta.z;

	if (theNode->MPlatform)						// see if factor in moving platform
	{
		ObjNode *plat = theNode->MPlatform;
		dx += plat->Delta.x;
		dy += plat->Delta.y;
		dz += plat->Delta.z;
	}

	gCoord.x += dx*fps;
	gCoord.y += dy*fps;
	gCoord.z += dz*fps;

	r = theNode->Rot.y += fps;						// spin


			/* DO COLLISION */

	HandleCollisions(theNode, CTYPE_TERRAIN|CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TRIGGER2, 0);



			/* UPDATE SPARKLES */

	c = theNode->SpecialF[0] += fps * 4.0f;

	w = 16.8f * POD_SCALE;
	yoff = 63.0f * POD_SCALE;
	yoff2 = 7.0f * POD_SCALE;

	for (j = 0; j < 8; j++)
	{
		i = theNode->Sparkles[j];
		if (i != -1)
		{
			gSparkles[i].where.x = theNode->Coord.x + sin(r) * w;
			if (j < 4)
				gSparkles[i].where.y = theNode->Coord.y + yoff;
			else
				gSparkles[i].where.y = theNode->Coord.y + yoff2;
			gSparkles[i].where.z = theNode->Coord.z + cos(r) * w;
			r += PI2 / 4.0f;

			gSparkles[i].color.g = (1.0f + sin(c)) * .5f;
			gSparkles[i].color.r = (1.0f + cos(c)) * .5f;
			c += .6f;
		}
	}


		/* UPDATE */

	UpdateObject(theNode);


	/* SEE IF DISPLAY HELP */

	if (!gHelpMessageDisabled[HELP_MESSAGE_POWPOD])
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < HELP_BEACON_RANGE)
		{
			DisplayHelpMessage(HELP_MESSAGE_POWPOD, .5f, false);
		}
	}
}


/************** DO TRIGGER - POWERUP POD ********************/
//
// OUTPUT: True = want to handle trigger as a solid object
//

Boolean DoTrig_PowerupPod(ObjNode *theNode, ObjNode *whoNode, Byte sideBits)
{
#pragma unused(sideBits)

	if (gPlayerInfo.scaleRatio > 1.0f)			// giant player can simply crush the powerup pods
	{
		PowerupPodGotPunched(whoNode, theNode, nil, nil);
	}

	return(true);
}

/**************** POWERUP POD GOT PUNCHED **********************/
//
// returns true if want to disable punch after this
//

static Boolean PowerupPodGotPunched(ObjNode *weapon, ObjNode *pod, OGLPoint3D *fistCoord, OGLVector3D *weaponDelta)
{
ObjNode	*newObj,*top,*bot;
float	r;

#pragma unused(fistCoord,weapon,weaponDelta)

	DisableHelpType(HELP_MESSAGE_POWPOD);					// dont need to show any help now that they've done it

	switch(pod->POWType)
	{
					/**************/
					/* STUN PULSE */
					/**************/

		case	POW_TYPE_STUNPULSE:

					/* PUT DOWN THE POWERUP */

				gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
				gNewObjectDefinition.type 		= GLOBAL_ObjType_StunPulseGun;
				gNewObjectDefinition.coord	 	= pod->Coord;
				gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
				gNewObjectDefinition.slot 		= 70;
				gNewObjectDefinition.moveCall 	= MovePowerup;
				gNewObjectDefinition.rot 		= 0;
				gNewObjectDefinition.scale 		= 1.2;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->POWType = pod->POWType;
				newObj->POWRegenerate = pod->POWRegenerate;					// copy regeneration flag to the POW

				newObj->CType 			= CTYPE_POWERUP | CTYPE_MISC;
				newObj->CBits			= CBITS_ALLSOLID;
				CreateCollisionBoxFromBoundingBox_Maximized(newObj);

				AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 4, 4, true);
				break;

					/**************/
					/* FREEZE GUN */
					/**************/

		case	POW_TYPE_FREEZE:

					/* PUT DOWN THE POWERUP */

				gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
				gNewObjectDefinition.type 		= GLOBAL_ObjType_FreezeGun;
				gNewObjectDefinition.coord	 	= pod->Coord;
				gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
				gNewObjectDefinition.slot 		= 70;
				gNewObjectDefinition.moveCall 	= MovePowerup;
				gNewObjectDefinition.rot 		= 0;
				gNewObjectDefinition.scale 		= 1.2;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->POWType = pod->POWType;
				newObj->POWRegenerate = pod->POWRegenerate;					// copy regeneration flag to the POW


				newObj->CType 			= CTYPE_POWERUP | CTYPE_MISC;
				newObj->CBits			= CBITS_ALLSOLID;
				CreateCollisionBoxFromBoundingBox_Maximized(newObj);

				AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 4, 4, true);
				break;

					/*************/
					/* FLAME GUN */
					/*************/

		case	POW_TYPE_FLAME:

					/* PUT DOWN THE POWERUP */

				gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
				gNewObjectDefinition.type 		= GLOBAL_ObjType_FlameGun;
				gNewObjectDefinition.coord	 	= pod->Coord;
				gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
				gNewObjectDefinition.slot 		= 70;
				gNewObjectDefinition.moveCall 	= MovePowerup;
				gNewObjectDefinition.rot 		= 0;
				gNewObjectDefinition.scale 		= 1.2;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->POWType = pod->POWType;
				newObj->POWRegenerate = pod->POWRegenerate;					// copy regeneration flag to the POW


				newObj->CType 			= CTYPE_POWERUP | CTYPE_MISC;
				newObj->CBits			= CBITS_ALLSOLID;
				CreateCollisionBoxFromBoundingBox_Maximized(newObj);

				AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 4, 4, true);
				break;

					/**************/
					/* SUPERNOVA */
					/**************/

		case	POW_TYPE_SUPERNOVA:

					/* PUT DOWN THE POWERUP */

				gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
				gNewObjectDefinition.type 		= GLOBAL_ObjType_SuperNovaBattery;
				gNewObjectDefinition.coord	 	= pod->Coord;
				gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
				gNewObjectDefinition.slot 		= 70;
				gNewObjectDefinition.moveCall 	= MovePowerup;
				gNewObjectDefinition.rot 		= 0;
				gNewObjectDefinition.scale 		= 1.2;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->POWType = pod->POWType;
				newObj->POWRegenerate = pod->POWRegenerate;					// copy regeneration flag to the POW

						/* SET COLLISION STUFF */

				newObj->CType 			= CTYPE_POWERUP | CTYPE_MISC;
				newObj->CBits			= CBITS_ALLSOLID;
				CreateCollisionBoxFromBoundingBox_Maximized(newObj);

				AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 4, 4, true);
				break;


					/********/
					/* DART */
					/********/

		case	POW_TYPE_DART:

					/* PUT DOWN THE POWERUP */

				gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
				gNewObjectDefinition.type 		= GLOBAL_ObjType_DartPOW;
				gNewObjectDefinition.coord	 	= pod->Coord;
				gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
				gNewObjectDefinition.slot 		= 70;
				gNewObjectDefinition.moveCall 	= MovePowerup;
				gNewObjectDefinition.rot 		= 0;
				gNewObjectDefinition.scale 		= .4;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->POWType = pod->POWType;
				newObj->POWRegenerate = pod->POWRegenerate;					// copy regeneration flag to the POW

						/* SET COLLISION STUFF */

				newObj->CType 			= CTYPE_POWERUP | CTYPE_MISC;
				newObj->CBits			= CBITS_ALLSOLID;
				CreateCollisionBoxFromBoundingBox_Maximized(newObj);

				AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 2, 4, true);
				break;


					/*************/
					/* FLARE GUN */
					/*************/

		case	POW_TYPE_FLARE:

					/* PUT DOWN THE POWERUP */

				gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
				gNewObjectDefinition.type 		= GLOBAL_ObjType_FlareGun;
				gNewObjectDefinition.coord	 	= pod->Coord;
				gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
				gNewObjectDefinition.slot 		= 70;
				gNewObjectDefinition.moveCall 	= MovePowerup;
				gNewObjectDefinition.rot 		= 0;
				gNewObjectDefinition.scale 		= 1.2;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->POWType = pod->POWType;
				newObj->POWRegenerate = pod->POWRegenerate;					// copy regeneration flag to the POW


				newObj->CType 			= CTYPE_POWERUP | CTYPE_MISC;
				newObj->CBits			= CBITS_ALLSOLID;
				CreateCollisionBoxFromBoundingBox_Maximized(newObj);

				AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 4, 4, true);
				break;

				/*****************/
				/* ATOM POWERUPS */
				/*****************/

		case	POW_TYPE_HEALTH:
				SpewAtoms(&pod->Coord, pod->AtomQuantity, 0, 0,pod->POWRegenerate);
				break;

		case	POW_TYPE_JUMPJET:
				SpewAtoms(&pod->Coord, 0, pod->AtomQuantity, 0,pod->POWRegenerate);
				break;

		case	POW_TYPE_FUEL:
				SpewAtoms(&pod->Coord, 0, 0, pod->AtomQuantity,pod->POWRegenerate);
				break;


				/**********/
				/* MAGNET */
				/**********/

		case	POW_TYPE_MAGNET:
					/* PUT DOWN THE POWERUP */

				gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
				gNewObjectDefinition.type 		= SLIME_ObjType_Magnet;
				gNewObjectDefinition.coord	 	= pod->Coord;
				gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
				gNewObjectDefinition.slot 		= 70;
				gNewObjectDefinition.moveCall 	= MovePowerup;
				gNewObjectDefinition.rot 		= 0;
				gNewObjectDefinition.scale 		= .25;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->POWType = pod->POWType;
				newObj->POWRegenerate = false;							// magnet has a custom regen procedure
				newObj->MagnetMonsterID = pod->PowerupParm2;			// get monster ID from pod parm[2]


						/* SET COLLISION STUFF */

				newObj->CType 			= CTYPE_POWERUP | CTYPE_MISC;
				newObj->CBits			= CBITS_ALLSOLID;
				CreateCollisionBoxFromBoundingBox_Maximized(newObj);

				AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 4, 4, true);
				break;


				/**********/
				/* GROWTH */
				/**********/

		case	POW_TYPE_GROWTH:

				if (gLevelNum != LEVEL_NUM_JUNGLE)
					DoFatalAlert("PowerupPodGotPunched: growth POW only on jungle!");

					/* PUT DOWN THE POWERUP */

				gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
				gNewObjectDefinition.type 		= JUNGLE_ObjType_GrowthPOW;
				gNewObjectDefinition.coord	 	= pod->Coord;
				gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
				gNewObjectDefinition.slot 		= SLOT_OF_DUMB-1;
				gNewObjectDefinition.moveCall 	= MovePowerup;
				gNewObjectDefinition.rot 		= 0;
				gNewObjectDefinition.scale 		= .25;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->POWType = pod->POWType;
				newObj->POWRegenerate = pod->POWRegenerate;					// copy regeneration flag to the POW

				BG3D_SphereMapGeomteryMaterial(gNewObjectDefinition.group, gNewObjectDefinition.type,
										 	0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkYosemite);	// set this model to be sphere mapped


						/* SET COLLISION STUFF */

				newObj->CType 			= CTYPE_POWERUP | CTYPE_MISC;
				newObj->CBits			= CBITS_ALLSOLID;
				CreateCollisionBoxFromBoundingBox_Maximized(newObj);

				AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 2, 2, true);
				break;

					/*************/
					/* FREE LIFE */
					/*************/

		case	POW_TYPE_FREELIFE:

					/* PUT DOWN THE POWERUP */

				gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
				gNewObjectDefinition.type 		= GLOBAL_ObjType_FreeLifePOW;
				gNewObjectDefinition.coord	 	= pod->Coord;
				gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
				gNewObjectDefinition.slot 		= 70;
				gNewObjectDefinition.moveCall 	= MovePowerup;
				gNewObjectDefinition.rot 		= 0;
				gNewObjectDefinition.scale 		= PLAYER_DEFAULT_SCALE;
				newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

				newObj->POWType = pod->POWType;
																	// note: these don't ever regenerate, so don't set POWRegenerate
						/* SET COLLISION STUFF */

				newObj->CType 			= CTYPE_POWERUP | CTYPE_MISC;
				newObj->CBits			= CBITS_ALLSOLID;
				CreateCollisionBoxFromBoundingBox_Maximized(newObj);

				AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 4, 4, true);


				break;



		default:
				DoFatalAlert("PowerupPodGotPunched: unknown powerup ID");

	}




		/***********************/
		/* BLOW UP THE BALLOON */
		/***********************/

	if (pod->IsBalloon)
	{
		ObjNode	*string = pod->ChainHead;

		ExplodeGeometry(pod, 400, SHARD_MODE_FROMORIGIN, 1, 1.5);
		PlayEffect_Parms3D(EFFECT_BALLOONPOP, &pod->Coord, NORMAL_CHANNEL_RATE, 2.0);

		string->TerrainItemPtr = nil;				// dont come back
		DeleteObject(string);
	}


		/*******************/
		/* BLOW UP THE POD */
		/*******************/

	else
	{
		r = gPlayerInfo.objNode->Rot.y;

				/* MAKE TOP PART */

		gNewObjectDefinition.group 		= MODEL_GROUP_GLOBAL;
		gNewObjectDefinition.type 		= GLOBAL_ObjType_PowerupOrb_Top;
		gNewObjectDefinition.coord.x 	= pod->Coord.x;
		gNewObjectDefinition.coord.y 	= pod->Coord.y + 70.0f;
		gNewObjectDefinition.coord.z 	= pod->Coord.z;
		gNewObjectDefinition.flags 		= 0;
		gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
		gNewObjectDefinition.moveCall 	= MoveOrbDebris;
		gNewObjectDefinition.rot 		= pod->Rot.y;
		gNewObjectDefinition.scale 		= POD_SCALE;
		top = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		top->Health = 3.0f;
		top->Delta.x = -sin(r) * 300.0f;
		top->Delta.y = 400.0f;
		top->Delta.z = -cos(r) * 300.0f;
		top->DeltaRot.x = RandomFloat2() * PI2;
		top->DeltaRot.y = RandomFloat2() * PI2;
		top->DeltaRot.z = RandomFloat2() * PI2;

		CreateCollisionBoxFromBoundingBox(top, 1,1);

				/* MAKE BOTTOM PART */

		gNewObjectDefinition.type 		= GLOBAL_ObjType_PowerupOrb_Bottom;
		gNewObjectDefinition.coord.y 	= pod->Coord.y + 30.0f;
		bot = MakeNewDisplayGroupObject(&gNewObjectDefinition);

		bot->Health = 3.0f;
		bot->Delta.x = -sin(r) * 300.0f;
		bot->Delta.y = 200.0f;
		bot->Delta.z = -cos(r) * 300.0f;
		bot->DeltaRot.x = RandomFloat2() * PI2;
		bot->DeltaRot.y = RandomFloat2() * PI2;
		bot->DeltaRot.z = RandomFloat2() * PI2;

		CreateCollisionBoxFromBoundingBox(bot, 1,1);

		AttachShadowToObject(top, GLOBAL_SObjType_Shadow_Circular, 5,5, true);
		AttachShadowToObject(bot, GLOBAL_SObjType_Shadow_Circular, 5,5, true);


		PlayEffect3D(EFFECT_POWPODHIT, &pod->Coord);		// play effect here since letting punch continue

		pod->TerrainItemPtr = nil;							// dont ever come back
		DeleteObject(pod);
	}



	return(false);
}


/******************* MOVE ORB DEBRIS ************************/

static void MoveOrbDebris(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;


		/* SEE IF OUT OF RANGE */

	if (TrackTerrainItem(theNode) || (theNode->StatusBits & STATUS_BIT_ISCULLED))		// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	GetObjectInfo(theNode);


			/* MOVE IT */

	if (theNode->StatusBits & STATUS_BIT_ONGROUND)			// do friction if landed
	{
		ApplyFrictionToDeltasXZ(400.0,&gDelta);
		ApplyFrictionToRotation(14.0,&theNode->DeltaRot);
	}

	gDelta.y -= 900.0f * fps;								// gravity
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	theNode->Rot.x += theNode->DeltaRot.x * fps;
	theNode->Rot.y += theNode->DeltaRot.y * fps;
	theNode->Rot.z += theNode->DeltaRot.z * fps;


		/* HANDLE COLLISIONS */

	HandleCollisions(theNode, CTYPE_MISC|CTYPE_TERRAIN|CTYPE_FENCE|CTYPE_PLAYER, -.5);

	UpdateObject(theNode);
}




/*************** MOVE POWERUP *******************/

void MovePowerup(ObjNode *theNode)
{
float	dx,dy,dz;
float	fps = gFramesPerSecondFrac;


			/*****************************************/
			/* SEE IF CHECK REGENERATION STUFF FIRST */
			/*****************************************/

	if (theNode->POWRegenerate)
	{
		if (theNode->StatusBits & STATUS_BIT_HIDDEN)			// if hidden then check if need to regenerate it
		{
			CheckIfRegeneratePOW(theNode);
			return;
		}
	}


	GetObjectInfo(theNode);

			/* MOVE */

	gDelta.y -= 1000.0f * fps;					// gravity

	dx = gDelta.x;
	dy = gDelta.y;
	dz = gDelta.z;

	if (theNode->MPlatform)						// see if factor in moving platform
	{
		ObjNode *plat = theNode->MPlatform;
		dx += plat->Delta.x;
		dy += plat->Delta.y;
		dz += plat->Delta.z;
	}

	gCoord.x += dx*fps;
	gCoord.y += dy*fps;
	gCoord.z += dz*fps;

			/* DO COLLISION */

	HandleCollisions(theNode, CTYPE_TERRAIN|CTYPE_MISC|CTYPE_MPLATFORM|CTYPE_TRIGGER2, .2f);


			/* UPDATE */

	theNode->Rot.y += gFramesPerSecondFrac * PI2;

	UpdateObject(theNode);


		/* SEE IF DISPLAY HELP */

	if (!gHelpMessageDisabled[HELP_MESSAGE_PICKUPPOW])
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < HELP_BEACON_RANGE)
		{
			DisplayHelpMessage(HELP_MESSAGE_PICKUPPOW, .5f, false);
		}
	}

}


/************* CHECK IF REGENERATE POW *********************/

static void CheckIfRegeneratePOW(ObjNode *theNode)
{
Boolean	reGen = false;			// assume no re-gen

	switch(theNode->POWType)
	{
		case	POW_TYPE_STUNPULSE:
				if (FindWeaponInventoryIndex(WEAPON_TYPE_STUNPULSE) == NO_INVENTORY_HERE)	// if player is out of this
					if (!OGL_IsBBoxVisible(&theNode->BBox, &theNode->BaseTransformMatrix))	// and player won't see it pop back to life...
						reGen = true;														// ... then regenerate it
				break;

		case	POW_TYPE_FREEZE:
				if (FindWeaponInventoryIndex(WEAPON_TYPE_FREEZE) == NO_INVENTORY_HERE)		// if player is out of this
					if (!OGL_IsBBoxVisible(&theNode->BBox, &theNode->BaseTransformMatrix))	// and player won't see it pop back to life...
						reGen = true;														// ... then regenerate it
				break;

		case	POW_TYPE_SUPERNOVA:
				if (FindWeaponInventoryIndex(WEAPON_TYPE_SUPERNOVA) == NO_INVENTORY_HERE)	// if player is out of this
					if (!OGL_IsBBoxVisible(&theNode->BBox, &theNode->BaseTransformMatrix))	// and player won't see it pop back to life...
						reGen = true;														// ... then regenerate it
				break;

		case	POW_TYPE_DART:
				if (FindWeaponInventoryIndex(WEAPON_TYPE_DART) == NO_INVENTORY_HERE)		// if player is out of this
					if (!OGL_IsBBoxVisible(&theNode->BBox, &theNode->BaseTransformMatrix))	// and player won't see it pop back to life...
						reGen = true;														// ... then regenerate it
				break;

		case	POW_TYPE_FLAME:
				if (FindWeaponInventoryIndex(WEAPON_TYPE_FLAME) == NO_INVENTORY_HERE)		// if player is out of this
					if (!OGL_IsBBoxVisible(&theNode->BBox, &theNode->BaseTransformMatrix))	// and player won't see it pop back to life...
						reGen = true;														// ... then regenerate it
				break;

		case	POW_TYPE_FLARE:
				if (FindWeaponInventoryIndex(WEAPON_TYPE_FLARE) == NO_INVENTORY_HERE)		// if player is out of this
					if (!OGL_IsBBoxVisible(&theNode->BBox, &theNode->BaseTransformMatrix))	// and player won't see it pop back to life...
						reGen = true;														// ... then regenerate it
				break;

		case	POW_TYPE_GROWTH:
				if (gPlayerInfo.giantTimer <= 0.0f)												// only reappear after giantism is done
					if (FindWeaponInventoryIndex(WEAPON_TYPE_GROWTH) == NO_INVENTORY_HERE)		// if player is out of this
						if (!OGL_IsBBoxVisible(&theNode->BBox, &theNode->BaseTransformMatrix))	// and player won't see it pop back to life...
							reGen = true;														// ... then regenerate it
				break;

				/* NOT A REGENERATABLE POW, SO DELETE IT */
		default:
				theNode->TerrainItemPtr = nil;
				DeleteObject(theNode);
	}


	if (reGen)
	{
		theNode->CType = CTYPE_POWERUP | CTYPE_MISC;			// re-activate collision

		theNode->StatusBits &= ~STATUS_BIT_HIDDEN;				// unhide
		if (theNode->ShadowNode)								// and unhide shadow
			theNode->ShadowNode->StatusBits &= ~STATUS_BIT_HIDDEN;

		theNode->Coord = theNode->InitCoord;					// make sure back where it started
		UpdateObjectTransforms(theNode);
	}

}




/***************** MOVE MAGNET AFTER DROP *********************/
//
// Used when player drops the magnet.
//

void MoveMagnetAfterDrop(ObjNode *theNode)
{
float fps = gFramesPerSecondFrac;

	GetObjectInfo(theNode);

	theNode->Rot.x += 10.0f * fps;
	theNode->Rot.y += 8.0f * fps;

	gDelta.y -= 1000.0f * fps;
	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;

	if ((gCoord.y + theNode->BBox.max.y) < GetTerrainY(gCoord.x, gCoord.z))		//  if underground, then reset as powerup
	{
		theNode->MoveCall = MovePowerup;
		gCoord = theNode->Coord = theNode->OldCoord = theNode->InitCoord;
		gDelta.x = gDelta.y = gDelta.z = 0;
		theNode->Rot.x = 0;
		theNode->CType 	= CTYPE_POWERUP | CTYPE_MISC;
	}

	UpdateObject(theNode);
}





#pragma mark -

/**************** ADD POWERUP TO INVENTORY *********************/
//
// Called from pickup anim when player grabs a POW
//

void AddPowerupToInventory(ObjNode *pow)
{
	DisableHelpType(HELP_MESSAGE_PICKUPPOW);					// dont need to show any help now that they've done it

	switch(pow->POWType)
	{
		case	POW_TYPE_STUNPULSE:
				IncWeaponQuantity(WEAPON_TYPE_STUNPULSE, 20);
				if (!gPlayerInfo.holdingGun)
				{
					gPlayerInfo.holdingGun = true;
					gPlayerInfo.currentWeaponType = WEAPON_TYPE_STUNPULSE;
				}
				break;

		case	POW_TYPE_FREEZE:
				IncWeaponQuantity(WEAPON_TYPE_FREEZE, 15);
				if (!gPlayerInfo.holdingGun)
				{
					gPlayerInfo.holdingGun = true;
					gPlayerInfo.currentWeaponType = WEAPON_TYPE_FREEZE;
				}
				break;

		case	POW_TYPE_FLAME:
				IncWeaponQuantity(WEAPON_TYPE_FLAME, 15);
				if (!gPlayerInfo.holdingGun)
				{
					gPlayerInfo.holdingGun = true;
					gPlayerInfo.currentWeaponType = WEAPON_TYPE_FLAME;
				}
				break;

		case	POW_TYPE_FLARE:
				IncWeaponQuantity(WEAPON_TYPE_FLARE, 5);
				if (!gPlayerInfo.holdingGun)
				{
					gPlayerInfo.holdingGun = true;
					gPlayerInfo.currentWeaponType = WEAPON_TYPE_FLARE;
				}
				break;

		case	POW_TYPE_SUPERNOVA:
				IncWeaponQuantity(WEAPON_TYPE_SUPERNOVA, 1);
				break;

		case	POW_TYPE_DART:
				IncWeaponQuantity(WEAPON_TYPE_DART, 5);
				break;

		case	POW_TYPE_HEALTH:
				gPlayerInfo.health += .1f;
				if (gPlayerInfo.health > 1.0f)
					gPlayerInfo.health = 1.0f;
				break;

		case	POW_TYPE_JUMPJET:
				gPlayerInfo.jumpJet += .1f;
				if (gPlayerInfo.jumpJet > 1.0f)
					gPlayerInfo.jumpJet = 1.0f;
				break;

		case	POW_TYPE_FUEL:
				gPlayerInfo.fuel += .02f;
				if (gPlayerInfo.fuel > 1.0f)
					gPlayerInfo.fuel = 1.0f;
				break;

		case	POW_TYPE_GROWTH:
				IncWeaponQuantity(WEAPON_TYPE_GROWTH, 1);
				if (!gPlayerInfo.holdingGun)
				{
					gPlayerInfo.holdingGun = true;
					gPlayerInfo.currentWeaponType = WEAPON_TYPE_GROWTH;
				}
				break;

		case	POW_TYPE_FREELIFE:
				gPlayerInfo.lives++;
				break;

	}
}



/******************* UPDATE PLAYER GROWTH ********************/

void UpdatePlayerGrowth(ObjNode *player)
{
OGLBoundingBox	*bBox;
ObjNode		*shadowObj;

	switch(gPlayerInfo.growMode )
	{
		case	GROWTH_MODE_NONE:											// if none, then bail
				return;

				/***********/
				/* GROWING */
				/***********/

		case	GROWTH_MODE_GROW:

				gPlayerInfo.scale += gFramesPerSecondFrac;
				if (gPlayerInfo.scale > PLAYER_GIANT_SCALE)					// keep pinned at max scale
				{
					gPlayerInfo.scale = PLAYER_GIANT_SCALE;

					gPlayerInfo.giantTimer -= gFramesPerSecondFrac;			// see if time to shrink
					if (gPlayerInfo.giantTimer <= 0.0f)
					{
						gPlayerInfo.giantTimer = 0;
						gPlayerInfo.growMode = GROWTH_MODE_SHRINK;
					}
				}
				break;

				/*************/
				/* SHRINKING */
				/*************/

		case	GROWTH_MODE_SHRINK:
				gPlayerInfo.scale -= gFramesPerSecondFrac;
				if (gPlayerInfo.scale <= PLAYER_DEFAULT_SCALE)				// see if back to normal
				{
					gPlayerInfo.scale = PLAYER_DEFAULT_SCALE;
					gPlayerInfo.growMode = GROWTH_MODE_NONE;
				}
				break;
	}



		/****************/
		/* UPDATE STUFF */
		/****************/

	player->Scale.x =
	player->Scale.y =
	player->Scale.z = gPlayerInfo.scale;											// update player


			/* UPDATE COLLISION BOX OFFSETS */

	bBox = &gObjectGroupBBoxList[MODEL_GROUP_SKELETONBASE+SKELETON_TYPE_OTTO][0];		// point to default bbox

	player->LeftOff 	= -40.0f * gPlayerInfo.scale;
	player->RightOff 	= 40.0f * gPlayerInfo.scale;
	player->FrontOff 	= 40.0f * gPlayerInfo.scale;
	player->BackOff 	= -40.0f * gPlayerInfo.scale;
	player->TopOff 		= bBox->max.y * gPlayerInfo.scale;
	gPlayerBottomOff 	= bBox->min.y * gPlayerInfo.scale;

		/* UPDATE CAMERA STUFF */

	gPlayerInfo.scaleRatio = gPlayerInfo.scale / PLAYER_DEFAULT_SCALE;					// calc 1..n for scale ratio

	gCameraDistFromMe = CAMERA_DEFAULT_DIST_FROM_ME + (gPlayerInfo.scaleRatio -1.0f) * 75.0f;	// move camera away as scales up
	gCameraLookAtYOff = DEFAULT_CAMERA_YOFF + (gPlayerInfo.scaleRatio -1.0f) * 190.0f;	// and look up as well


		/* UPDATE SHADOW */

	shadowObj = player->ShadowNode;
	if (shadowObj)
	{
		shadowObj->Scale.x =
		shadowObj->Scale.y =
		shadowObj->Scale.z =
		shadowObj->SpecialF[0] =
		shadowObj->SpecialF[1] = DEFAULT_PLAYER_SHADOW_SCALE * gPlayerInfo.scaleRatio;
	}
}

#pragma mark -

/************************* MAKE POWERUP BALLOON ******************************/

static Boolean MakePowerupBalloon(TerrainItemEntryType *itemPtr, float x, float y, float z)
{
ObjNode	*string, *balloon;
int		i;

	if (gLevelNum != LEVEL_NUM_CLOUD)
		DoFatalAlert("MakePowerupBalloon: not cloud level");


					/****************************/
					/* FIRST MAKE STRING OBJECT */
					/****************************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOUD_ObjType_BalloonString;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= y;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
	gNewObjectDefinition.slot 		= 456;
	gNewObjectDefinition.moveCall 	= MovePowerupBalloon;
	gNewObjectDefinition.rot 		= RandomFloat() * PI2;
	gNewObjectDefinition.scale 		= BALLOON_SCALE;
	string = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	string->TerrainItemPtr = itemPtr;					// keep ptr to item list

	string->BalloonWobble = RandomFloat2() * PI2;


				/*******************************/
				/* MAKE BALLOON TRIGGER OBJECT */
				/*******************************/

	gNewObjectDefinition.type 		= CLOUD_ObjType_Balloon;
	gNewObjectDefinition.slot++;
	gNewObjectDefinition.moveCall 	= nil;
	balloon = MakeNewDisplayGroupObject(&gNewObjectDefinition);


	string->ChainNode = balloon;
	balloon->ChainHead = string;

			/* SET POW VARS */

	balloon->POWType = itemPtr->parm[0];					// set powerup type

	balloon->AtomQuantity = itemPtr->parm[1];			// set # atoms (if atom type)
	if (balloon->AtomQuantity == 0)						// make sure it's at least 1
		balloon->AtomQuantity = 1;

	balloon->PowerupParm2 = itemPtr->parm[2];			// keep parm2 for special uses

	balloon->POWRegenerate = itemPtr->parm[3] & 1;		// remember if let this POW regenerate

	balloon->IsBalloon = true;


			/* SET COLLISION STUFF */

	balloon->CType 			= CTYPE_MISC|CTYPE_HEATSEEKATTACT|CTYPE_AUTOTARGETWEAPON;
	balloon->CBits			= CBITS_TOUCHABLE;
	CreateCollisionBoxFromBoundingBox(balloon,1,1);


			/* SET WEAPON HANDLERS */

	for (i = 0; i < NUM_WEAPON_TYPES; i++)
		balloon->HitByWeaponHandler[i] = PowerupPodGotPunched;								// set punch callback for all weapon types
	balloon->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] = PowerupBalloonGotHitByStunPulse;	// special for this


			/* SHADOW & SHINE */

	BG3D_SphereMapGeomteryMaterial(gNewObjectDefinition.group, gNewObjectDefinition.type,
								 	0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);	// set this model to be sphere mapped

	AttachShadowToObject(balloon, GLOBAL_SObjType_Shadow_Circular, 5,5, true);


	return(true);													// item was added
}


/*********************** MOVE POWERUP BALLOON ****************************/

static void MovePowerupBalloon(ObjNode *string)
{
ObjNode	*balloon = string->ChainNode;


			/* SEE IF GONE */

	if (TrackTerrainItem_FromInit(string))			// track from init coord so stays on same supertile as any platforms
	{
		DeleteObject(string);
		return;
	}


	string->BalloonWobble += gFramesPerSecondFrac;
	string->Rot.x = sin(string->BalloonWobble) * .3f;


			/* UPDATE */

	UpdateObjectTransforms(string);
	if (balloon)
	{
		static const OGLPoint3D	off = {0,300, 0};

		OGLPoint3D_Transform(&off, &string->BaseTransformMatrix, &balloon->Coord);		// calc coord of balloon

		balloon->Rot.x = string->Rot.x;
		UpdateObjectTransforms(balloon);
		CalcObjectBoxFromNode(balloon);
	}
}

/************* POWERUP BALLOON GOT HIT BY STUN PULSE ********************/
//
// Stun pulse weapon ricochets off of the balloons
//

static Boolean PowerupBalloonGotHitByStunPulse(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
OGLMatrix4x4	m;
float			speed;

#pragma unused(weaponCoord, enemy)

			/* REVERSE IT */

	weaponDelta->x = -weaponDelta->x;						// reverse vector
	weaponDelta->z = -weaponDelta->z;
	speed = CalcVectorLength(weaponDelta);
	speed *= .9f;											// slow a little

	OGLMatrix4x4_SetRotate_XYZ(&m, RandomFloat2() * .5f, RandomFloat2() * .5f, RandomFloat2() * .5f);	// random scatter
	OGLVector3D_Transform(weaponDelta, &m, weaponDelta);
	SetAlignmentMatrix(&weapon->AlignmentMatrix, weaponDelta);
	weaponDelta->x *= speed;
	weaponDelta->y *= speed;
	weaponDelta->z *= speed;



			/* MOVE IT TO CLEAR IT FROM THE THING HIT */

	weaponCoord->x += weaponDelta->x * (gFramesPerSecondFrac * 2.0f);
	weaponCoord->y += weaponDelta->y * (gFramesPerSecondFrac * 2.0f);
	weaponCoord->z += weaponDelta->z * (gFramesPerSecondFrac * 2.0f);

		/* SHRINK WEAPON A TAD */

	weapon->Scale.x =
	weapon->Scale.y =
	weapon->Scale.z *= .8f;


	return(false);			// dont stop weapon
}


#pragma mark -







