/****************************/
/*   ENEMY: BRAIN BOSS.C	*/
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

static void MoveBrainBoss(ObjNode *theNode);
static void UpdateBrainBoss(ObjNode *theNode);

static Boolean BrainBossHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static Boolean BrainBossHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);
static void KillBrainBoss(ObjNode *enemy);

static Boolean HurtBrainBoss(ObjNode *enemy, float damage);

static void SetBrainStaticLocation(ObjNode *stat);
static void MoveBrainStatic(ObjNode *theNode);
static void DrawBrainStatic(ObjNode *theNode);

static void MoveBrainAlienPort(ObjNode *theNode);
static void DrawPortalBeams(ObjNode *theNode);
static Boolean PortalHitByWeapon(ObjNode *weapon, ObjNode *portal, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta);

static void SeeIfBrainBossShoot(ObjNode *core);
static void MoveBrainBullet(ObjNode *theNode);
static void ExplodeBrainBullet(ObjNode *bullet);

static void MakeEliteBrainAlien(float x, float z, float rot);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	BRAINBOSS_SCALE					5.0f

#define	BRAINBOSS_HOVER_Y				110.0f

#define	BRAINBOSS_HEALTH				25.0f

enum
{
	BRAIN_MODE_HOVER,
	BRAIN_MODE_OPENING,
	BRAIN_MODE_ATTACK,
	BRAIN_MODE_CLOSING
};


#define	NUM_BRAIN_STATIC	12

#define	BRAIN_STATIC_WIDTH	(13.0f * BRAINBOSS_SCALE)

#define	OPEN_DIST			(35.0f * BRAINBOSS_SCALE)

#define	NUM_BRAIN_PORTALS	8

#define	ELITEBRAINALIEN_SCALE					2.1f

#define	BRAINBOSS_SPEED		140.0f

#define	BRAIN_BULLET_SPEED	700.0f


/*********************/
/*    VARIABLES      */
/*********************/

#define	HoverWobble		SpecialF[0]
#define	OpenDelay		SpecialF[1]
#define	OpenDist		SpecialF[2]
#define	AttackTimer		SpecialF[3]
#define	ShootTimer		SpecialF[4]

#define	PortalID		Special[0]

#define	DelayToSeek		SpecialF[0]									// timer for seeking flares


static ObjNode	*gBrainBoss = nil;

static	short	gBrainBossDeformation = -1;

static OGLPoint3D	gBrainPortalCoords[NUM_BRAIN_PORTALS];
static	Boolean		gPortalBlown[NUM_BRAIN_PORTALS];
static	short		gNumPortalsBlown;

static const 	OGLColorRGBA 	gTrailColor 		= {.5,1,.5,.5};
static const	OGLColorRGBA	gTrailColorStart	= {1,1,1,.7};

Boolean		gBrainBossDead;

/*********************** INIT BRAINBOSS *****************************/

void InitBrainBoss(void)
{
int						i;
TerrainItemEntryType	*itemPtr;
ObjNode					*newObj;

	gNumPortalsBlown = 0;
	gBrainBossDead = false;

	for (i = 0; i < NUM_BRAIN_PORTALS; i++)
		gPortalBlown[i] = false;


			/******************************/
			/* FIND COORDS OF ALL PORTALS */
			/******************************/


	itemPtr = *gMasterItemList; 									// get pointer to data inside the LOCKED handle

	for (i= 0; i < gNumTerrainItems; i++)
	{
		if (itemPtr[i].type == MAP_ITEM_BRAINPORTAL)				// see if it's what we're looking for
		{
			short	id;
			float	x,z;

			id = itemPtr[i].parm[0];								// get generator ID

			x = gBrainPortalCoords[id].x = itemPtr[i].x;			// get coords
			z = gBrainPortalCoords[id].z = itemPtr[i].y;
			gBrainPortalCoords[id].y = GetTerrainY_Undeformed(x,z) + 700.0f;
		}
	}


		/****************************************/
		/* CREATE DUMMY DRAW OBJECT FOR PORTALS */
		/****************************************/

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+2;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.scale 		= 1;
	gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOLIGHTING|STATUS_BIT_GLOW|STATUS_BIT_NOZWRITES;

	newObj = MakeNewObject(&gNewObjectDefinition);
	newObj->CustomDrawFunction = DrawPortalBeams;

}



/************************ ADD BRAINBOSS ENEMY *************************/
//
// A skeleton character
//

Boolean AddEnemy_BrainBoss(TerrainItemEntryType *itemPtr, long x, long z)
{
ObjNode	*core, *left, *right;
int		i;
DeformationType		defData;

#pragma unused (itemPtr)

				/*******************/
				/* MAKE BRAIN CORE */
				/*******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BRAINBOSS_ObjType_BrainCore;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z) + BRAINBOSS_HOVER_Y;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
	gNewObjectDefinition.slot 		= 200;
	gNewObjectDefinition.moveCall 	= MoveBrainBoss;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= BRAINBOSS_SCALE;
	gBrainBoss = core = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	core->Health 		= 	BRAINBOSS_HEALTH;
	core->Mode			=	BRAIN_MODE_HOVER;
	core->OpenDelay 	=	5.0f;
	core->OpenDist 		= 	0;

				/* SET COLLISION INFO */

	core->CType = CTYPE_MISC | CTYPE_AUTOTARGETWEAPON | CTYPE_BLOCKCAMERA;
	core->CBits = CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(core, 1,1);


				/* SET WEAPON HANDLERS */

	core->HitByWeaponHandler[WEAPON_TYPE_STUNPULSE] 	= BrainBossHitByWeapon;
	core->HitByWeaponHandler[WEAPON_TYPE_SUPERNOVA] 	= BrainBossHitBySuperNova;
	core->HitByWeaponHandler[WEAPON_TYPE_FLAME] 		= BrainBossHitByWeapon;
	core->HitByWeaponHandler[WEAPON_TYPE_FLARE] 		= BrainBossHitByWeapon;

	core->HurtCallback = HurtBrainBoss;							// set hurt callback function

	core->WeaponAutoTargetOff.y = BRAINBOSS_SCALE * 83.0f;


				/* MAKE SHADOW */

	AttachShadowToObject(core, SHADOW_TYPE_CIRCULAR, 15, 20, false);

				/*******************/
				/* MAKE LEFT BRAIN */
				/*******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BRAINBOSS_ObjType_LeftBrain;
	gNewObjectDefinition.coord		= core->Coord;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
	gNewObjectDefinition.slot 		= core->Slot+1;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= core->Rot.y;
	gNewObjectDefinition.scale 		= BRAINBOSS_SCALE;
	left = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	left->CType = CTYPE_HURTME | CTYPE_BLOCKCAMERA;
	left->CBits = CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(left, 1,1);

	core->ChainNode = left;

				/* MAKE EYE SPARKLE */

	i = left->Sparkles[0] = GetFreeSparkle(left);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL | SPARKLE_FLAG_TRANSFORMWITHOWNER | SPARKLE_FLAG_FLICKER;
		gSparkles[i].where.x = -38;
		gSparkles[i].where.y = 110;
		gSparkles[i].where.z = -127;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 220.0f;
		gSparkles[i].separation = 30.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_RedGlint;
	}



				/********************/
				/* MAKE RIGHT BRAIN */
				/********************/

	gNewObjectDefinition.type 		= BRAINBOSS_ObjType_RightBrain;
	right = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	right->CType = CTYPE_HURTME | CTYPE_BLOCKCAMERA;
	right->CBits = CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(right, 1,1);

	left->ChainNode = right;

				/* MAKE EYE SPARKLE */

	i = right->Sparkles[0] = GetFreeSparkle(right);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL | SPARKLE_FLAG_TRANSFORMWITHOWNER | SPARKLE_FLAG_FLICKER;
		gSparkles[i].where.x = 38;
		gSparkles[i].where.y = 110;
		gSparkles[i].where.z = -127;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 190.0f;
		gSparkles[i].separation = 30.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_RedGlint;
	}


			/**************************/
			/* MAKE STATIC DISCHARGES */
			/**************************/

	for (i = 0; i < NUM_BRAIN_STATIC; i++)
	{
		ObjNode	*stat;

		gNewObjectDefinition.genre		= CUSTOM_GENRE;
		gNewObjectDefinition.flags 		= STATUS_BIT_NOZWRITES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOTEXTUREWRAP|
											STATUS_BIT_KEEPBACKFACES | STATUS_BIT_GLOW;
		gNewObjectDefinition.slot 		= SLOT_OF_DUMB + 10;
		gNewObjectDefinition.moveCall 	= MoveBrainStatic;
		stat = MakeNewObject(&gNewObjectDefinition);

		stat->CustomDrawFunction = DrawBrainStatic;

		SetBrainStaticLocation(stat);

	}

			/****************************/
			/* MAKE TERRAIN DEFORMATION */
			/****************************/

	if (gG4)
	{
		defData.type 				= DEFORMATION_TYPE_CONTINUOUSWAVE;
		defData.amplitude 			= 30;
		defData.radius 				= 0;
		defData.speed 				= -1100;
		defData.origin.x			= x;
		defData.origin.y			= z;
		defData.oneOverWaveLength 	= 1.0f / 100.0f;
		defData.radialWidth			= 1100.0f;
		defData.decayRate			= 20.0f;
		gBrainBossDeformation = NewSuperTileDeformation(&defData);
	}
	else
		gBrainBossDeformation = -1;

	return(true);
}

#pragma mark -


/*************** SET BRAIN STATIC LOCATION ***************************/

static void SetBrainStaticLocation(ObjNode *stat)
{
	stat->Timer = .5f + RandomFloat();						// set duration @ this location

	stat->Coord.y = (8.0f * BRAINBOSS_SCALE) + RandomFloat() * (BRAINBOSS_SCALE * 110.0f);		// set random y OFFSET
	stat->Coord.z = RandomFloat2() * (BRAINBOSS_SCALE * 80.0f);				// set random z OFFSET
}


/********************** MOVE BRAIN STATIC ******************************/

static void MoveBrainStatic(ObjNode *theNode)
{
	if (gBrainBoss == nil)					// delete static if brain boss killed
	{
		DeleteObject(theNode);
		return;
	}

	theNode->Timer -= gFramesPerSecondFrac;
	if (theNode->Timer <= 0.0f)								// see if time to move to new location
		SetBrainStaticLocation(theNode);
}



/********************** DRAW BRAIN STATIC *****************************/

static void DrawBrainStatic(ObjNode *theNode)
{
OGLMatrix4x4	m;
OGLPoint3D		pts[4];

	if (!gBrainBoss)
		return;

			/***************/
			/* CALC POINTS */
			/***************/

	if (gBrainBoss->OpenDist == 0.0f)				// bail if nothing to draw
		return;

			/* SET OFFSET POINTS */

	pts[0].x = -gBrainBoss->OpenDist;				// left top offset
	pts[0].y = theNode->Coord.y;
	pts[0].z = theNode->Coord.z;

	pts[1].x = -gBrainBoss->OpenDist;				// left bottom offset
	pts[1].y = theNode->Coord.y - BRAIN_STATIC_WIDTH;
	pts[1].z = theNode->Coord.z;

	pts[2].x = gBrainBoss->OpenDist;				// right bottom offset
	pts[2].y = theNode->Coord.y - BRAIN_STATIC_WIDTH;
	pts[2].z = theNode->Coord.z;

	pts[3].x = gBrainBoss->OpenDist;				// right top offset
	pts[3].y = theNode->Coord.y;
	pts[3].z = theNode->Coord.z;


			/* BUILD TRANSFORM MATRIX & TRANSFORM */

	OGLMatrix4x4_SetRotate_Y(&m,gBrainBoss->Rot.y);				// set rotation
	m.value[M03] = gBrainBoss->Coord.x;							// insert translation
	m.value[M13] = gBrainBoss->Coord.y;
	m.value[M23] = gBrainBoss->Coord.z;

	OGLPoint3D_TransformArray(pts, &m, pts, 4);



			/* SUBMIT STATIC TEXTURE */

	gGlobalTransparency = .6;

	MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_LEVELSPECIFIC][BRAINBOSS_SObjType_Static1+(MyRandomLong()&0x3)].materialObject);


			/* DRAW QUAD */

	glBegin(GL_QUADS);

		if (MyRandomLong()&1)								// random flip
		{
			glTexCoord2f(0,1);	glVertex3fv((GLfloat *)&pts[0]);
			glTexCoord2f(0,0);	glVertex3fv((GLfloat *)&pts[1]);
			glTexCoord2f(1,0);	glVertex3fv((GLfloat *)&pts[2]);
			glTexCoord2f(1,1);	glVertex3fv((GLfloat *)&pts[3]);
		}
		else
		{
			glTexCoord2f(0,0);	glVertex3fv((GLfloat *)&pts[0]);
			glTexCoord2f(0,1);	glVertex3fv((GLfloat *)&pts[1]);
			glTexCoord2f(1,1);	glVertex3fv((GLfloat *)&pts[2]);
			glTexCoord2f(1,0);	glVertex3fv((GLfloat *)&pts[3]);
		}

	glEnd();

	gGlobalTransparency = 1.0;
}



#pragma mark -


/********************* MOVE BRAINBOSS **************************/

static void MoveBrainBoss(ObjNode *core)
{
float	fps = gFramesPerSecondFrac;



	GetObjectInfo(core);


			/* MAKE IT HOVER-WOBBLE */

	core->HoverWobble += fps * 3.0f;
	gCoord.y = core->InitCoord.y + sin(core->HoverWobble) * 20.0f;




	switch(core->Mode)
	{
				/* HOVERING */

		case	BRAIN_MODE_HOVER:
				if (gNumPortalsBlown >= NUM_BRAIN_PORTALS)
				{
					core->OpenDelay -= fps;								// see if time to open
					if (core->OpenDelay <= 0.0f)
					{
						core->Mode = BRAIN_MODE_OPENING;				// start brain opening
					}
				}
				break;


				/* OPENING */

		case	BRAIN_MODE_OPENING:
				core->OpenDist += fps * 70.0f;						// open it wider
				if (core->OpenDist > OPEN_DIST)						// see if opened all the way
				{
					core->OpenDist = OPEN_DIST;
					core->AttackTimer = 2.5f;
					core->ShootTimer = 0;
					core->Mode = BRAIN_MODE_ATTACK;
				}
				break;

				/* ATTACK */

		case	BRAIN_MODE_ATTACK:
				SeeIfBrainBossShoot(core);

				core->AttackTimer -= fps;
				if (core->AttackTimer <= 0.0f)
					core->Mode = BRAIN_MODE_CLOSING;
				break;

				/* CLOSING */

		case	BRAIN_MODE_CLOSING:
				core->OpenDist -= fps * 70.0f;					// close it
				if (core->OpenDist < 0.0f)						// see if closed
				{
					core->OpenDist = 0.0f;
					core->OpenDelay = 1.0f + RandomFloat() * 2.0f;
					core->Mode = BRAIN_MODE_HOVER;
				}
				break;

	}


		/***********/
		/* MOVE IT */
		/***********/

	if (gNumPortalsBlown >= NUM_BRAIN_PORTALS)
	{
		float	angle,r;

				/* TURN AND MOVE */

		angle = TurnObjectTowardTarget(core, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, 1.0, false);

		r = core->Rot.y;
		gDelta.x = -sin(r) * BRAINBOSS_SPEED;
		gDelta.z = -cos(r) * BRAINBOSS_SPEED;

		gCoord.x += gDelta.x * fps;
		gCoord.z += gDelta.z * fps;

					/* DO ENEMY COLLISION */

		if (DoEnemyCollisionDetect(core,CTYPE_HURTENEMY|CTYPE_FENCE, false))
			return;


				/* UPDATE DEFORMATION COORDS */

		UpdateDeformationCoords(gBrainBossDeformation, gCoord.x, gCoord.z);

	}



		/**********/
		/* UPDATE */
		/**********/

	UpdateBrainBoss(core);
}




/***************** UPDATE BRAIN BOSS **********************/

static void UpdateBrainBoss(ObjNode *core)
{
ObjNode	*left, *right;
float	r,xoff,zoff;

	UpdateObject(core);


			/**********************/
			/* ALIGN BRAIN HALVES */
			/**********************/

	left = core->ChainNode;										// get brain halves
	right = left->ChainNode;

	r = core->Rot.y;
	left->Rot.y = right->Rot.y = r;

	r += PI/2;													// get 90 angle
	xoff = sin(r) * core->OpenDist;								// calc x/z offsets
	zoff = cos(r) * core->OpenDist;

	left->Coord.x = gCoord.x - xoff;
	left->Coord.z = gCoord.z - zoff;

	right->Coord.x = gCoord.x + xoff;
	right->Coord.z = gCoord.z + zoff;

	left->Coord.y = right->Coord.y = gCoord.y;


	UpdateObjectTransforms(left);
	CreateCollisionBoxFromBoundingBox_Rotated(left, 1, .7);

	UpdateObjectTransforms(right);
	CreateCollisionBoxFromBoundingBox_Rotated(right, 1, .7);


		/* UPDATE AUDIO */

	if (core->EffectChannel == -1)
		core->EffectChannel = PlayEffect_Parms3D(EFFECT_BRAINSTATIC, &gCoord, NORMAL_CHANNEL_RATE, core->OpenDist / OPEN_DIST);
	else
	{
		gChannelInfo[core->EffectChannel].volumeAdjust = core->OpenDist / OPEN_DIST;			// modify volume
		Update3DSoundChannel(EFFECT_BRAINSTATIC, &core->EffectChannel, &gCoord);
	}
}


/******************* SEE IF BRAIN BOSS SHOOT *******************************/

static void SeeIfBrainBossShoot(ObjNode *core)
{
float	fps = gFramesPerSecondFrac;
ObjNode	*newObj;
int		i;
OGLVector3D	aim;
float		r;

	core->ShootTimer -= fps;
	if (core->ShootTimer > 0.0f)
		return;

	core->ShootTimer = .7f;											// reset timer

			/* CALC AIM VECTOR */

	r = core->Rot.y + RandomFloat2() * .1f;
	aim.x = -sin(r);
	aim.y = RandomFloat() * .1f;									// lob up a little
	aim.z = -cos(r);
	FastNormalizeVector(aim.x, aim.y, aim.z, &aim);


			/* WHERE FROM */

	gNewObjectDefinition.coord.x 		= gCoord.x + aim.x * 200.0f;
	gNewObjectDefinition.coord.y 		= gCoord.y + 300.0f;
	gNewObjectDefinition.coord.z 		= gCoord.z + aim.z * 200.0f;


				/*********************/
				/* MAKE BULLET EVENT */
				/*********************/

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+5;
	gNewObjectDefinition.moveCall 	= MoveBrainBullet;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->Kind = WEAPON_TYPE_FLARE;

	newObj->Delta.x = aim.x * BRAIN_BULLET_SPEED;
	newObj->Delta.y = aim.y * BRAIN_BULLET_SPEED;
	newObj->Delta.z = aim.z * BRAIN_BULLET_SPEED;

	newObj->Health = 5.0f;
	newObj->Damage = .25f;

	newObj->DelayToSeek = .5;						// set a delay before they start heat seeking

	newObj->ParticleTimer = 0;

	newObj->CType 			= CTYPE_WEAPON;
	newObj->CBits			= CBITS_TOUCHABLE;
	SetObjectCollisionBounds(newObj, 40,-40,-40,40,40,-40);


			/* MAKE SHADOW */

	AttachShadowToObject(newObj, GLOBAL_SObjType_Shadow_Circular, 4,4, false);


			/* MAKE SPARKLE */

	i = newObj->Sparkles[0] = GetFreeSparkle(newObj);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_RANDOMSPIN|SPARKLE_FLAG_FLICKER;
		gSparkles[i].where = newObj->Coord;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 100.0f;

		gSparkles[i].separation = 0.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_RedSpark;
	}


				/* VAPOR TRAIL */

	newObj->VaporTrails[0] = CreateNewVaporTrail(newObj, 0, VAPORTRAIL_TYPE_COLORSTREAK,
													&newObj->Coord, &gTrailColorStart, .4, .6, 30);


	PlayEffect_Parms3D(EFFECT_BRAINBOSSSHOOT, &newObj->Coord, NORMAL_CHANNEL_RATE, 1.6);
}


/******************* MOVE BRAIN BULLET ***********************/

static void MoveBrainBullet(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
int		i;
float	dist;


	GetObjectInfo(theNode);


			/* SEE IF GONE */

	theNode->Health -= fps;
	if (theNode->Health <= 0.0f)
	{
		ExplodeBrainBullet(theNode);				// note:  gCoord must be set!
		return;
	}



			/* SEE IF HEAT SEEK */

	theNode->DelayToSeek -= fps;
	if (theNode->DelayToSeek <= 0.0f)
	{
		OGLVector3D	v;

		v.x = gPlayerInfo.coord.x - gCoord.x;
		v.y = gPlayerInfo.coord.y - gCoord.y;
		v.z = gPlayerInfo.coord.z - gCoord.z;
		FastNormalizeVector(v.x,v.y, v.z, &v);

		dist = CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);
		dist = 3000.0f - dist;
		if (dist > 3000.0f)
			dist = 3000.0f;
		dist /= 3000.0f;
		dist *= fps;


		gDelta.x += v.x * (4000.0f * dist);
		gDelta.y += v.y * (800.0f * dist);
		gDelta.z += v.z * (4000.0f * dist);
		theNode->Speed3D = CalcVectorLength(&gDelta);					// calc speed

		if (theNode->Speed3D > BRAIN_BULLET_SPEED)
		{
			float	q = BRAIN_BULLET_SPEED / theNode->Speed3D;

			gDelta.x *= q;
			gDelta.y *= q;
			gDelta.z *= q;
		}
	}


			/* MOVE IT */

	gCoord.x += gDelta.x * fps;
	gCoord.y += gDelta.y * fps;
	gCoord.z += gDelta.z * fps;


		/*********************/
		/* SEE IF HIT PLAYER */
		/*********************/

	if (DoSimpleBoxCollisionAgainstPlayer(gCoord.y+40.0f, gCoord.y-40.0f, gCoord.x-40.0f, gCoord.x+40.0f, gCoord.z+40.0f, gCoord.z-40.0f))
	{
		PlayerGotHit(theNode, 0);
		ExplodeBrainBullet(theNode);
		return;
	}

	UpdateObject(theNode);


			/******************/
			/* UPDATE SPARKLE */
			/******************/

	i = theNode->Sparkles[0];
	if (i != -1)
	{
		gSparkles[i].where = gCoord;
	}

	if (VerifyVaporTrail(theNode->VaporTrails[0], theNode, 0))
		AddToVaporTrail(&theNode->VaporTrails[0], &gCoord, &gTrailColor);

}


/********************* EXPLODE BRAIN BULLET ***********************/

static void ExplodeBrainBullet(ObjNode *bullet)
{
long					pg,i;
OGLVector3D				delta;
OGLPoint3D				pt;
NewParticleDefType		newParticleDef;
float					x,y,z;


	x = gCoord.x;
	y = gCoord.y;
	z = gCoord.z;

			/******************/
			/* MAKE PARTICLES */
			/******************/

	gNewParticleGroupDef.magicNum				= 0;
	gNewParticleGroupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
	gNewParticleGroupDef.flags					= PARTICLE_FLAGS_BOUNCE | PARTICLE_FLAGS_ALLAIM;
	gNewParticleGroupDef.gravity				= 150;
	gNewParticleGroupDef.magnetism				= 0;
	gNewParticleGroupDef.baseScale				= 50;
	gNewParticleGroupDef.decayRate				= 0;
	gNewParticleGroupDef.fadeRate				= 1.1;
	gNewParticleGroupDef.particleTextureNum		= PARTICLE_SObjType_WhiteSpark3;
	gNewParticleGroupDef.srcBlend				= GL_SRC_ALPHA;
	gNewParticleGroupDef.dstBlend				= GL_ONE;

	pg = NewParticleGroup(&gNewParticleGroupDef);
	if (pg != -1)
	{
		for (i = 0; i < 20; i++)
		{
			pt.x = x + RandomFloat2() * 50.0f;
			pt.y = y;
			pt.z = z + RandomFloat2() * 50.0f;

			delta.x = RandomFloat2() * 500.0f;
			delta.z = RandomFloat2() * 500.0f;
			delta.y = RandomFloat2() * 250.0f;

			newParticleDef.groupNum		= pg;
			newParticleDef.where		= &pt;
			newParticleDef.delta		= &delta;
			newParticleDef.scale		= 1.0f + RandomFloat();
			newParticleDef.rotZ			= 0;
			newParticleDef.rotDZ		= RandomFloat2() * 7.0f;
			newParticleDef.alpha		= FULL_ALPHA;
			if (AddParticleToGroup(&newParticleDef))
				break;
		}
	}
	PlayEffect_Parms3D(EFFECT_FLAREEXPLODE, &gCoord, NORMAL_CHANNEL_RATE / 2, 1.9);


	DeleteObject(bullet);

}


#pragma mark -

/************ BRAIN BOSS HIT BY STUN PULSE *****************/

static Boolean BrainBossHitByWeapon(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weaponCoord, weaponDelta)

	HurtBrainBoss(enemy, weapon->Damage);

	switch(enemy->Mode)
	{
		case	BRAIN_MODE_OPENING:
				enemy->Mode = BRAIN_MODE_CLOSING;
				PlayEffect3D(EFFECT_BRAINPAIN, &enemy->Coord);
				break;

		case	BRAIN_MODE_ATTACK:
				enemy->AttackTimer = 0;
				PlayEffect3D(EFFECT_BRAINPAIN, &enemy->Coord);
				break;
	}



	enemy->AttackTimer = 0;			// start to close

	return(true);			// stop weapon
}



/************** BRAIN BOSS GOT HIT BY SUPERNOVA *****************/

static Boolean BrainBossHitBySuperNova(ObjNode *weapon, ObjNode *enemy, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)

	HurtBrainBoss(enemy, 1.0);

	return(false);
}




/*********************** HURT BRAIN BOSS ***************************/

static Boolean HurtBrainBoss(ObjNode *enemy, float damage)
{
	if (enemy->Mode == BRAIN_MODE_HOVER)					// cannot be hurt when it's closed
		return(false);

	enemy->Health -= damage;
	if (enemy->Health <= 0.0f)
	{
		KillBrainBoss(enemy);
		return(true);
	}
	else
	{
		OGLColorRGBA	c;
		float	r = enemy->Health / BRAINBOSS_HEALTH;		// get ratio 0..1

		r = .3f + (r * .7f);								// calc color component value

		c.r = 1.0;
		c.g = r;
		c.b = r;
		c.a = 1.0;

		enemy->ColorFilter = c;								// set color of halves plus core
		enemy->ChainNode->ColorFilter = c;
		enemy->ChainNode->ChainNode->ColorFilter = c;

	}

	return(false);
}


/****************** KILL BRAIN BOSS ***********************/

static void KillBrainBoss(ObjNode *enemy)
{
	SpewAtoms(&enemy->Coord, 1,0, 10, false);

	PlayEffect_Parms3D(EFFECT_BRAINBOSSDIE, &enemy->Coord, NORMAL_CHANNEL_RATE, 2.0f);
	Rumble(1.0f, 2000);
	ExplodeGeometry(enemy, 200, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .2);

	DeleteObject(enemy);
	gBrainBoss = nil;

	DeleteTerrainDeformation(gBrainBossDeformation);			// stop the deformation
	gBrainBossDeformation = -1;

	gBrainBossDead = true;
}


#pragma mark -

/******************** ADD BRAIN PORT *************************/

Boolean AddBrainPort(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
int		i,id;

	id = itemPtr->parm[0];

			/* MAKE OBJECT */

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BRAINBOSS_ObjType_BrainPort;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= ENEMY_SLOT-1;						// need to draw before elite brain dudes so xparency works right
	gNewObjectDefinition.moveCall 	= MoveBrainAlienPort;
	gNewObjectDefinition.rot 		= (float)id * (PI2/8);
	gNewObjectDefinition.scale 		= 3.5;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->PortalID = id;
	newObj->Health = 2.0f;
	newObj->Timer = RandomFloat() * 5.0f;


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA|CTYPE_AUTOTARGETWEAPON;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj,1,1);

	newObj->WeaponAutoTargetOff.y = 200.0f;

	for (i = 0; i < NUM_WEAPON_TYPES; i++)							// set weapon handlers
		newObj->HitByWeaponHandler[i] 	= PortalHitByWeapon;


			/* MAKE SPARKLE */

	i = newObj->Sparkles[0] = GetFreeSparkle(newObj);				// get free sparkle slot
	if (i != -1)
	{
		gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_FLICKER;
		gSparkles[i].where.x = x;
		gSparkles[i].where.y = gNewObjectDefinition.coord.y + 192.0f * gNewObjectDefinition.scale;
		gSparkles[i].where.z = z;

		gSparkles[i].color.r = 1;
		gSparkles[i].color.g = 1;
		gSparkles[i].color.b = 1;
		gSparkles[i].color.a = 1;

		gSparkles[i].scale = 300.0f;
		gSparkles[i].separation = 40.0f;

		gSparkles[i].textureNum = PARTICLE_SObjType_RedGlint;
	}


	return(true);
}


/************************ MOVE BRAIN ALIEN PORT *****************************/

static void MoveBrainAlienPort(ObjNode *theNode)
{

			/* SEE IF TOO MANY ELITES ALREADY */

	if (gG4)
	{
		if (gNumEnemyOfKind[ENEMY_KIND_ELITEBRAINALIEN] > 4)
			return;
	}
	else
	{
		if (gNumEnemyOfKind[ENEMY_KIND_ELITEBRAINALIEN] > 3)
			return;
	}


				/* SEE IF MAKE NEW ELITE BRAIN ALIEN */

	if (CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) > 3000.0f)
		return;

	theNode->Timer -= gFramesPerSecondFrac;
	if (theNode->Timer > 0.0f)
		return;

	theNode->Timer = 2.0f + RandomFloat() * 3.0f;


	MakeEliteBrainAlien(theNode->Coord.x, theNode->Coord.z, theNode->Rot.y);
}


/******************* MAKE ELITE BRIAN ALIEN ***********************/

static void MakeEliteBrainAlien(float x, float z, float rot)
{
ObjNode	*newObj;

	newObj = MakeEnemySkeleton(SKELETON_TYPE_ELITEBRAINALIEN,x, z,ELITEBRAINALIEN_SCALE, 1, MoveEliteBrainAlien_FromPortal);

	newObj->BoundingSphereRadius *= 1.3f;						// enlarge a little

	SetSkeletonAnim(newObj->Skeleton, 1);


				/* SET BETTER INFO */

	newObj->ColorFilter.r =
	newObj->ColorFilter.g =
	newObj->ColorFilter.b = .1;
	newObj->ColorFilter.a = 0;

	newObj->Rot.y = rot;
	newObj->Timer = 1.2f;

	newObj->Health 		= 1.5;
	newObj->Damage 		= .2;
	newObj->Kind 		= ENEMY_KIND_ELITEBRAINALIEN;

				/* SET COLLISION INFO */

	CreateCollisionBoxFromBoundingBox(newObj, 1,1);
	CalcNewTargetOffsets(newObj,20);





				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 8, 8,false);

	CreateBrainAlienGlow(newObj);

	gNumEnemies++;
	gNumEnemyOfKind[ENEMY_KIND_ELITEBRAINALIEN]++;

}




/******************** DRAW PORTAL BEAMS *************************/

static void DrawPortalBeams(ObjNode *theNode)
{
short	i,i2;
float	x,y,z,x2,y2,z2,u,u2,yo;

#pragma unused(theNode)


	gGlobalTransparency = .99;

	x = gBrainPortalCoords[0].x;									// get coords of 1st generator
	y = gBrainPortalCoords[0].y;
	z = gBrainPortalCoords[0].z;

	for (i = 0; i < NUM_BRAIN_PORTALS; i++)
	{
				/* GET COORDS OF NEXT GENERATOR */

		if (i == (NUM_BRAIN_PORTALS-1))							// see if loop back
			i2 = 0;
		else
			i2 = i+1;

		x2 = gBrainPortalCoords[i2].x;							// get next coords
		y2 = gBrainPortalCoords[i2].y;
		z2 = gBrainPortalCoords[i2].z;


		if ((gPortalBlown[i] == false) && (gPortalBlown[i2] == false))	// if either post is blown then skip
		{
				/* DRAW A QUAD */

			yo = 30.0f + RandomFloat() * 10.0f;

			u = RandomFloat() * 5.0f;
			u2 = u + 3.0f;

			MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_LEVELSPECIFIC][BRAINBOSS_SObjType_RedZap].materialObject);

			glBegin(GL_QUADS);
			glTexCoord2f(u,0);			glVertex3f(x,y+yo,z);
			glTexCoord2f(u,1);			glVertex3f(x,y-yo,z);
			glTexCoord2f(u2,1);			glVertex3f(x2,y2-yo, z2);
			glTexCoord2f(u2,0);			glVertex3f(x2,y2+yo,z2);
			glEnd();
		}

				/* NEXT ENDPOINT */
		x = x2;
		y = y2;
		z = z2;

	}

	gGlobalTransparency = 1.0;

}


/************ PORTAL HIT BY WEAPON *****************/

static Boolean PortalHitByWeapon(ObjNode *weapon, ObjNode *portal, OGLPoint3D *weaponCoord, OGLVector3D *weaponDelta)
{
#pragma unused (weapon, weaponCoord, weaponDelta)
float	damage;

	if (portal->Health > 2.0f)	//------------------- hack to fix some bug
		portal->Health = 0;

	if (weapon)
	{
		damage = weapon->Damage;
		if (damage <= 0.0f)
			damage = 1.0f;
	}
	else				// supernova probably
		damage = 1.0f;

	portal->Health -= damage;
	if (portal->Health <= 0.0f)
	{
		gPortalBlown[portal->PortalID] = true;
		gNumPortalsBlown++;

		ExplodeGeometry(portal, 500, SHARD_MODE_BOUNCE|SHARD_MODE_FROMORIGIN, 1, .5);
		PlayEffect_Parms3D(EFFECT_PORTALBOOM, &portal->Coord, NORMAL_CHANNEL_RATE, 1.8);

		MakeSparkExplosion(portal->Coord.x, portal->Coord.y + 50.0f, portal->Coord.z, 500.0f, 2.0, PARTICLE_SObjType_WhiteSpark,0);

		DeleteObject(portal);
	}

	return(true);			// stop weapon
}


















