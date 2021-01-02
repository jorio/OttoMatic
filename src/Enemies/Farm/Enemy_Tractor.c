/****************************/
/*   ENEMY: TRACTOR.C		*/
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	SplineDefType			**gSplineList;
extern	OGLPoint3D				gCoord;
extern	short					gNumEnemies;
extern	float					gFramesPerSecondFrac,gFramesPerSecond;
extern	OGLVector3D			gDelta;
extern	signed char			gNumEnemyOfKind[];
extern	u_long		gAutoFadeStatusBits;
extern	PlayerInfoType	gPlayerInfo;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void MoveTractor(ObjNode *theNode);
static void AlignTractorWheels(ObjNode *theNode);
static void MoveTractor_Chase(ObjNode *theNode);
static void MoveTractor_Ram(ObjNode *theNode);
static void MoveTractor_Wait(ObjNode *theNode);
static void DoTractorCollisionDetect(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	TRACTOR_SCALE	1.0f

#define TRACTOR_YOFF	150.0f

#define	TRACTOR_CHASE_DIST	4000.0f
#define	TRACTOR_CHASE_SPEED	440.0f

#define	TRACTOR_RAM_DIST	1000.0f
#define	TRACTOR_RAM_SPEED	1900.0f

#define TRACTOR_TURN_SPEED	1.6f
#define	RAM_DURATION		1.3f
#define	WAIT_DELAY			1.5f


/*********************/
/*    VARIABLES      */
/*********************/

#define	RamTimer	SpecialF[0]
#define	WaitDelay	SpecialF[1]


/************************* ADD TRACTOR *********************************/

Boolean AddTractor(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj,*frontLeftWheel, *backLeftWheel,*frontRightWheel,*backRightWheel;

				/******************/
				/* MAKE MAIN BODY */
				/******************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FARM_ObjType_Tractor;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
	gNewObjectDefinition.slot 		= 205;
	gNewObjectDefinition.moveCall 	= MoveTractor;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= TRACTOR_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->Mode = TRACTOR_MODE_WAIT;
	newObj->WaitDelay = 0;
	newObj->TractorIsDone = false;

	newObj->Damage = .4;


			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(newObj,.8,.8);
	newObj->Coord.y 		-= newObj->BottomOff;					// adjust y

	BG3D_SphereMapGeomteryMaterial(gNewObjectDefinition.group, gNewObjectDefinition.type,
								 	0, MULTI_TEXTURE_COMBINE_ADD, SPHEREMAP_SObjType_DarkDusk);	// set this model to be sphere mapped

				/***************/
				/* MAKE WHEELS */
				/***************/

				/* BACK LEFT */

	gNewObjectDefinition.type 		= FARM_ObjType_BackLeftWheel;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB;
	gNewObjectDefinition.moveCall 	= nil;
	backLeftWheel = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	newObj->ChainNode = backLeftWheel;

				/* BACK RIGHT */

	gNewObjectDefinition.type 		= FARM_ObjType_BackLeftWheel;
	backRightWheel = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	backLeftWheel->ChainNode = backRightWheel;

				/* FRONT LEFT */

	gNewObjectDefinition.type 		= FARM_ObjType_FrontLeftWheel;
	frontLeftWheel = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	backRightWheel->ChainNode = frontLeftWheel;

				/* FRONT RIGHT */

	gNewObjectDefinition.type 		= FARM_ObjType_FrontRightWheel;
	frontRightWheel = MakeNewDisplayGroupObject(&gNewObjectDefinition);
	frontLeftWheel->ChainNode = frontRightWheel;



				/* MAKE SHADOW */

	AttachShadowToObject(newObj, SHADOW_TYPE_CIRCULAR, 9, 14,false);



	return(true);													// item was added
}


/********************** MOVE TRACTOR ***********************/

static void MoveTractor(ObjNode *theNode)
{
		/* SEE IF MAKE GO AWAY */

	if (theNode->TractorIsDone)
	{
		if (TrackTerrainItem(theNode))
		{
			theNode->TerrainItemPtr = nil;					// never come back
			DeleteObject(theNode);
			return;
		}
	}


		/* MOVE IT */

	GetObjectInfo(theNode);

	switch(theNode->Mode)
	{
				/* WAITING MODE */

		case	TRACTOR_MODE_WAIT:
				MoveTractor_Wait(theNode);
				break;


				/* CHASE MODE */

		case	TRACTOR_MODE_CHASE:
				MoveTractor_Chase(theNode);
				break;


				/* RAM MODE */

		case	TRACTOR_MODE_RAM:
				MoveTractor_Ram(theNode);
				break;

	}


		/* COLLISION DETECT */

	DoTractorCollisionDetect(theNode);


		/**********/
		/* UPDATE */
		/**********/

		/* UPDATE SOUND */

	if (!theNode->TractorIsDone)
	{
		if (theNode->EffectChannel == -1)			// make sure playing sound
			theNode->EffectChannel = PlayEffect3D(EFFECT_TRACTOR, &gCoord);
		else
		{
			int	freq = NORMAL_CHANNEL_RATE + (theNode->Speed2D * 8.0f);
			ChangeChannelRate(theNode->EffectChannel, freq);
			Update3DSoundChannel(EFFECT_TRACTOR, &theNode->EffectChannel, &gCoord);
		}
	}

	UpdateObject(theNode);
	AlignTractorWheels(theNode);



	/*********************/
	/* SEE IF HIT PLAYER */
	/*********************/

	if (OGLPoint3D_Distance(&gCoord, &gPlayerInfo.coord) < (theNode->BBox.max.z * theNode->Scale.x + gPlayerInfo.objNode->BBox.max.x))
	{
		OGLVector2D	v1,v2;
		float		r,a;

				/* SEE IF PLAYER IN FRONT */

		r = theNode->Rot.y;							// calc tractor aim vec
		v1.x = -sin(r);
		v1.y = -cos(r);

		v2.x = gPlayerInfo.coord.x - gCoord.x;
		v2.y = gPlayerInfo.coord.z - gCoord.z;
		FastNormalizeVector2D(v2.x, v2.y, &v2, true);

		a = acos(OGLVector2D_Dot(&v1,&v2));			// calc angle between

		if (a < (PI/2))
		{
			PlayerGotHit(theNode, 0);
		}
	}
}


/******************** MOVE TRACTOR:  WAIT *****************************/

static void MoveTractor_Wait(ObjNode *theNode)
{
float	speed,r,fps = gFramesPerSecondFrac;

			/* DECELERATE */

	theNode->Speed2D -= 2300.0f * fps;
	if (theNode->Speed2D < 0.0f)
		theNode->Speed2D = 0;
	speed = theNode->Speed2D;

			/* MOVE TOWARD PLAYER */

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * speed;
	gDelta.z = -cos(r) * speed;

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y = GetTerrainY(gCoord.x, gCoord.z) + TRACTOR_YOFF;			// always stick to ground
	gDelta.y = (gCoord.y - theNode->OldCoord.y) * gFramesPerSecond;


			/* SEE IF CHASE */

	if (!theNode->TractorIsDone)
	{
		theNode->WaitDelay -= fps;							// see if ready to chase
		if (theNode->WaitDelay <= 0.0f)
		{
			if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < TRACTOR_CHASE_DIST)
			{
				theNode->Mode = TRACTOR_MODE_CHASE;
			}
		}
	}
}



/******************** MOVE TRACTOR:  CHASE *****************************/

static void MoveTractor_Chase(ObjNode *theNode)
{
float	speed,angle,r,fps = gFramesPerSecondFrac;

		/* ACCEL TO CHASE SPEED */

	theNode->Speed2D += TRACTOR_CHASE_SPEED * fps;
	if (theNode->Speed2D > TRACTOR_CHASE_SPEED)
		theNode->Speed2D = TRACTOR_CHASE_SPEED;
	speed = theNode->Speed2D;

			/* MOVE TOWARD PLAYER */

	angle = TurnObjectTowardTarget(theNode, &gCoord, gPlayerInfo.coord.x, gPlayerInfo.coord.z, TRACTOR_TURN_SPEED, false);

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * speed;
	gDelta.z = -cos(r) * speed;

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y = GetTerrainY(gCoord.x, gCoord.z) + TRACTOR_YOFF;			// always stick to ground
	gDelta.y = (gCoord.y - theNode->OldCoord.y) * gFramesPerSecond;


			/* SEE IF READY TO RAM */

	if (angle < PI/4)
	{
		if (CalcQuickDistance(gCoord.x, gCoord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z) < TRACTOR_RAM_DIST)
		{
			theNode->Mode = TRACTOR_MODE_RAM;
			theNode->RamTimer = RAM_DURATION;
		}
	}
}


/******************** MOVE TRACTOR:  RAM *****************************/

static void MoveTractor_Ram(ObjNode *theNode)
{
float	speed,r,fps = gFramesPerSecondFrac;


		/* ACCEL TO CHASE SPEED */

	theNode->Speed2D += TRACTOR_RAM_SPEED * fps;
	if (theNode->Speed2D > TRACTOR_RAM_SPEED)
		theNode->Speed2D = TRACTOR_RAM_SPEED;
	speed = theNode->Speed2D;

			/* MOVE TOWARD PLAYER */

	r = theNode->Rot.y;
	gDelta.x = -sin(r) * speed;
	gDelta.z = -cos(r) * speed;

	gCoord.x += gDelta.x * fps;
	gCoord.z += gDelta.z * fps;
	gCoord.y = GetTerrainY(gCoord.x, gCoord.z) + TRACTOR_YOFF;			// always stick to ground
	gDelta.y = (gCoord.y - theNode->OldCoord.y) * gFramesPerSecond;


			/* SEE IF DONE */

	theNode->RamTimer -= fps;
	if (theNode->RamTimer <= 0.0f)
	{
		theNode->Mode = TRACTOR_MODE_WAIT;
		theNode->WaitDelay = WAIT_DELAY;
	}
}




/********************* ALIGN TRACTOR WHEELS ************************/

static void AlignTractorWheels(ObjNode *theNode)
{
ObjNode	*wheel[4];
int		i;
OGLMatrix4x4	m1,m2,m3;
float			dr;
static const OGLPoint3D	wheelOffsets[4] =
{
	-84,-51,135,					// back left
	88,-51,135,						// back right
	-72,-113,-135,					// front left
	72,-113,-135					// front right
};

			/* GET THE WHEELS */

	wheel[0] = theNode->ChainNode;
	wheel[1] = wheel[0]->ChainNode;
	wheel[2] = wheel[1]->ChainNode;
	wheel[3] = wheel[2]->ChainNode;

	dr = theNode->Speed2D * .01f * gFramesPerSecondFrac;
	wheel[0]->Rot.x -= dr;
	wheel[1]->Rot.x -= dr;

	dr = theNode->Speed2D * .02f * gFramesPerSecondFrac;
	wheel[2]->Rot.x -= dr;
	wheel[3]->Rot.x -= dr;

	for (i = 0; i < 4; i++)
	{
		OGLMatrix4x4_SetRotate_X(&m1, wheel[i]->Rot.x);						// set rotation matrix
		OGLMatrix4x4_SetTranslate(&m2, 	wheelOffsets[i].x * TRACTOR_SCALE,
										wheelOffsets[i].y * TRACTOR_SCALE,
										wheelOffsets[i].z * TRACTOR_SCALE);	// locate tire on axel
		OGLMatrix4x4_Multiply(&m1, &m2, &m3);
		OGLMatrix4x4_Multiply(&m3, &theNode->BaseTransformMatrix, &wheel[i]->BaseTransformMatrix);
		SetObjectTransformMatrix(wheel[i]);

		wheel[i]->Coord = theNode->Coord;			// set coord to the tractor just so we have it for AutoFade
	}
}


/***************** DO TRACTOR COLLISION DETECT **************************/

static void DoTractorCollisionDetect(ObjNode *theNode)
{
			/* HANDLE THE BASIC STUFF */

	HandleCollisions(theNode, CTYPE_MISC|CTYPE_TRIGGER2|CTYPE_PLAYER, 0);


				/* CHECK FENCE COLLISION */

	DoFenceCollision(theNode);

}












