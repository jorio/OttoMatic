/****************************/
/*   		ITEMS2.C	    */
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

static void MoveZigZagSlats(ObjNode *theNode);
static void DrawCloudPlatform(ObjNode *theNode);
static void MoveCloudPlatform(ObjNode *theNode);
static void MoveNeuronStrand(ObjNode *theNode);
static void MoveRadarDish(ObjNode *base);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	CLOUD_RADIUS	300.0f

#define	LAVA_STONE_SCALE	2.5f

/*********************/
/*    VARIABLES      */
/*********************/

#define	ZigOrZag	Special[0]

static float	gZigZagIndex;


/************************ INIT ZIG ZAG SLATS ******************************/

void InitZigZagSlats(void)
{
	gZigZagIndex = 0;
}


/********************** UPDATE ZIGZAG SLATS ***************************/

void UpdateZigZagSlats(void)
{
	gZigZagIndex += gFramesPerSecondFrac;


}


/************************* ADD ZIGZAG SLATS *********************************/

Boolean AddZigZagSlats(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= CLOUD_ObjType_ZigZag_Blue + itemPtr->parm[0];	// blue or red?
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY2(x,z) + 10.0f;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 78;
	gNewObjectDefinition.moveCall 	= MoveZigZagSlats;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * PI/2;
	gNewObjectDefinition.scale 		= 3;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC | CTYPE_MPLATFORM | CTYPE_BLOCKCAMERA | CTYPE_BLOCKSHADOW;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Rotated(newObj, 	1, 1);
	newObj->BottomOff = -200;
	CalcObjectBoxFromNode(newObj);


	newObj->ZigOrZag = itemPtr->parm[1];

	return(true);													// item was added
}



/************************* MOVE ZIGZAG SLATS ************************/

static void MoveZigZagSlats(ObjNode *theNode)
{
float	off;

	GetObjectInfo(theNode);

	off = sin(gZigZagIndex) * 300.0f;


			/* DO HORIZ */

	if (theNode->Rot.y == 0.0f)
	{
		if (theNode->ZigOrZag)
			gCoord.x = theNode->InitCoord.x + off;
		else
			gCoord.x = theNode->InitCoord.x - off;

		gDelta.x = (gCoord.x - theNode->OldCoord.x) * gFramesPerSecond;
	}

			/* DO VERT */
	else
	{
		if (theNode->ZigOrZag)
			gCoord.z = theNode->InitCoord.z + off;
		else
			gCoord.z = theNode->InitCoord.z - off;

		gDelta.z = (gCoord.z - theNode->OldCoord.z) * gFramesPerSecond;
	}


	UpdateObject(theNode);
}


#pragma mark -

/************************* ADD CLOUD PLATFORM *********************************/

Boolean AddCloudPlatform(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

					/* MAIN OBJECT */

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY2(x,z) + 40.0f;
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_KEEPBACKFACES | STATUS_BIT_NOLIGHTING |
									 STATUS_BIT_NOZWRITES | STATUS_BIT_NOFOG | STATUS_BIT_NOTEXTUREWRAP;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-5;
	gNewObjectDefinition.moveCall 	= MoveCloudPlatform;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;									// keep ptr to item list

	newObj->CustomDrawFunction = DrawCloudPlatform;

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj, 10, -200, -CLOUD_RADIUS, CLOUD_RADIUS, CLOUD_RADIUS, -CLOUD_RADIUS);


	newObj->SpecialF[0] = RandomFloat()*PI2;
	newObj->SpecialF[1] = RandomFloat()*PI2;
	newObj->SpecialF[2] = RandomFloat()*PI2;
	newObj->SpecialF[3] = RandomFloat()*PI2;

	newObj->SpecialF[4] = RandomFloat2() * PI2;

	return(true);
}


/***************** MOVE CLOUD PLATFORM ******************/

static void MoveCloudPlatform(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;

		/* SEE IF OUT OF RANGE */

	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}


		/* SPIN THE LAYERS */

	theNode->SpecialF[0] += fps * .5f;
	theNode->SpecialF[1] -= fps * .6f;
	theNode->SpecialF[2] += fps * .1f;
	theNode->SpecialF[3] -= fps * .3f;


		/* WOBBLE */

	theNode->SpecialF[4] += fps * 1.1f;
	theNode->Coord.y = theNode->InitCoord.y + sin(theNode->SpecialF[4]) * 30.0f;
	CalcObjectBoxFromNode(theNode);


}





/********************** DRAW CLOUD PLATFORM *************************/

static void DrawCloudPlatform(ObjNode *theNode)
{
float	x,y,z;
OGLMatrix4x4	m;
int			i;
float	s;
static const float	scale[4] = {.5, .7, .9, .6};

	x = theNode->Coord.x;
	y = theNode->Coord.y - 30.0f;
	z = theNode->Coord.z;


	MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_LEVELSPECIFIC][CLOUD_SObjType_Cloud].materialObject);

			/*******************/
			/* DRAW ALL LAYERS */
			/*******************/

	for (i = 0; i < 4; i++)
	{
		s = CLOUD_RADIUS * scale[i];

		glPushMatrix();

		OGLMatrix4x4_SetRotate_Y(&m, theNode->SpecialF[i]);				// set rot
		m.value[M03] = x;												// translate
		m.value[M13] = y;
		m.value[M23] = z;
		glMultMatrixf(m.value);


		glBegin(GL_QUADS);

		glTexCoord2f(0,0);			glVertex3f(-s,0,-s);
		glTexCoord2f(0,1);			glVertex3f(s,0,-s);
		glTexCoord2f(1,1);			glVertex3f(s,0,s);
		glTexCoord2f(1,0);			glVertex3f(-s,0,s);

		glEnd();

		glPopMatrix();

		y += 30.0f;
	}


}


#pragma mark -

/************************* ADD LAVA STONE *********************************/

Boolean AddLavaStone(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;
short	type = itemPtr->parm[0];
float	rot = (float)itemPtr->parm[1] * (PI2/16);
float	e,r;
OGLMatrix3x3	m;
static const OGLPoint2D hengeOff = {-150.0f * LAVA_STONE_SCALE,0};
OGLPoint2D	hengePt;
CollisionBoxType *boxPtr;

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= FIREICE_ObjType_Stone_Blance + type;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
//	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.coord.y 	= GetMinTerrainY(x, z, gNewObjectDefinition.group, gNewObjectDefinition.type, LAVA_STONE_SCALE);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
	gNewObjectDefinition.slot 		= 70;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	gNewObjectDefinition.rot 		= rot;
	gNewObjectDefinition.scale 		= LAVA_STONE_SCALE;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC|CTYPE_BLOCKCAMERA;
	newObj->CBits			= CBITS_ALLSOLID;


	switch(type)
	{
				/* BALANCE, B2, DONUT, CLAW */

		case	0:
		case	3:
		case	4:
		case	5:
		case	6:
		case	9:
		case	10:
		case	11:
				e = 71.0f * newObj->Scale.x;
				SetObjectCollisionBounds(newObj, 1000,0,-e,e,e,-e);
				break;


				/* MONOLITH */

		case	1:
		case	7:
				CreateCollisionBoxFromBoundingBox(newObj,1,1);
				break;


				/* STONEHENGE */

		case	2:
		case	8:
				newObj->NumCollisionBoxes = 2;						// 2 collision boxes
				boxPtr = &newObj->CollisionBoxes[0];				// get ptr to box array

				OGLMatrix3x3_SetRotate(&m, -rot);					// calc pts of the columns
				OGLPoint2D_Transform(&hengeOff, &m, &hengePt);

				r = 85.0f * LAVA_STONE_SCALE;						// calc radius of each column

						/* FIRST COLUMN */

				boxPtr[0].top 		= newObj->Coord.y + 1000.0f;
				boxPtr[0].bottom 	= newObj->Coord.y;
				boxPtr[0].left 		= (newObj->Coord.x + hengePt.x) - r;
				boxPtr[0].right 	= (newObj->Coord.x + hengePt.x) + r;
				boxPtr[0].front 	= (newObj->Coord.z + hengePt.y) + r;
				boxPtr[0].back 		= (newObj->Coord.z + hengePt.y) - r;

						/* SECOND COLUMN */

				boxPtr[1].top 		= newObj->Coord.y + 1000.0f;
				boxPtr[1].bottom 	= newObj->Coord.y;
				boxPtr[1].left 		= (newObj->Coord.x - hengePt.x) - r;
				boxPtr[1].right 	= (newObj->Coord.x - hengePt.x) + r;
				boxPtr[1].front 	= (newObj->Coord.z - hengePt.y) + r;
				boxPtr[1].back 		= (newObj->Coord.z - hengePt.y) - r;

				KeepOldCollisionBoxes(newObj);
				break;
	}
	return(true);													// item was added
}


#pragma mark -


/************************ ADD RADAR DISH *****************************/

Boolean AddRadarDish(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*base,*dish;

				/*************/
				/* MAKE BASE */
				/*************/

	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SAUCER_ObjType_DishBase;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits ;
	gNewObjectDefinition.slot 		= 70;
	gNewObjectDefinition.moveCall 	= MoveRadarDish;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 		= 1.0;
	base = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	base->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	base->CType 		= CTYPE_MISC;
	base->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox(base, 1,1);


				/*************/
				/* MAKE DISH */
				/*************/

	gNewObjectDefinition.type 		= SAUCER_ObjType_Dish;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	dish = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	base->ChainNode = dish;


	return(true);
}


/********************** MOVE RADAR DISH *******************************/

static void MoveRadarDish(ObjNode *base)
{
ObjNode	*dish = base->ChainNode;

	dish->Rot.y += gFramesPerSecondFrac * PI2;
	UpdateObjectTransforms(dish);

	MoveObjectUnderPlayerSaucer(base);
}



#pragma mark -


/************************ ADD BLOB ARROW *****************************/

Boolean AddBlobArrow(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= SLIME_ObjType_BlobArrow;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= FindHighestCollisionAtXZ(x,z, CTYPE_TERRAIN | CTYPE_WATER);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits;
	gNewObjectDefinition.slot 		= 70;
	gNewObjectDefinition.moveCall 	= MoveStaticObject2;
	gNewObjectDefinition.rot 		= (float)itemPtr->parm[0] * (PI2/8);
	gNewObjectDefinition.scale 		= 1.6;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_ALLSOLID;
	SetObjectCollisionBounds(newObj, 200, -300, -20,20,20,-20);

	return(true);
}



/************************ ADD NEURON STRAND *****************************/

Boolean AddNeuronStrand(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;


	gNewObjectDefinition.group 		= MODEL_GROUP_LEVELSPECIFIC;
	gNewObjectDefinition.type 		= BRAINBOSS_ObjType_NeuronStrand;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= gAutoFadeStatusBits | STATUS_BIT_KEEPBACKFACES | STATUS_BIT_NOLIGHTING;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB-1;
	gNewObjectDefinition.moveCall 	= MoveNeuronStrand;
	gNewObjectDefinition.rot 		= RandomFloat()*PI2;
	gNewObjectDefinition.scale 		= 9.0;
	newObj = MakeNewDisplayGroupObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

			/* SET COLLISION STUFF */

	newObj->CType 			= CTYPE_MISC;
	newObj->CBits			= CBITS_ALLSOLID;
	CreateCollisionBoxFromBoundingBox_Maximized(newObj);

	newObj->Timer = RandomFloat() * 2.0f;

	return(true);
}


/***************** MOVE NEURON STRAND **************************/

static void MoveNeuronStrand(ObjNode *theNode)
{
float	fps = gFramesPerSecondFrac;
short	i;


	if (TrackTerrainItem(theNode))							// just check to see if it's gone
	{
		DeleteObject(theNode);
		return;
	}

	TurnObjectTowardTarget(theNode, &theNode->Coord, gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z, 100, false);	// aim at camera
	UpdateObjectTransforms(theNode);


		/* SEE IF FIRE NEURON */

	if (theNode->Sparkles[0] == -1)
	{
		theNode->Timer -= fps;
		if (theNode->Timer <= 0.0f)
		{
			theNode->Timer = RandomFloat() * 2.0f;							// reset timer

			i = theNode->Sparkles[0] = GetFreeSparkle(theNode);				// get free sparkle slot
			if (i != -1)
			{
				gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_RANDOMSPIN|SPARKLE_FLAG_FLICKER;
				gSparkles[i].where = theNode->Coord;

				gSparkles[i].color.r = .9;
				gSparkles[i].color.g = .9;
				gSparkles[i].color.b = 1;
				gSparkles[i].color.a = 1;

				gSparkles[i].scale = 300.0f;

				gSparkles[i].separation = 20.0f;

				gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark4;
			}
		}
	}

		/* UPDATE EXISTING SPARKLE */

	else
	{
		i = theNode->Sparkles[0];
		gSparkles[i].where.y += RandomFloat() * 10000.0f * fps;
		if (gSparkles[i].where.y > (theNode->Coord.y + 6000.0f))
		{
			DeleteSparkle(i);
			theNode->Sparkles[0] = -1;
		}
	}

}









