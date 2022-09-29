/****************************/
/*   		SKY.C		    */
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

static void DrawSky(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	SKY_GRID_SIZE		10			// # verts x / z

#define	SKY_GRID_SCALE		(gGameViewInfoPtr->yon / (SKY_GRID_SIZE / 2.0f))		// scale of each grid segment

#define	NUM_SKY_TRIANGLES	((SKY_GRID_SIZE-1) * (SKY_GRID_SIZE-1) * 2)

/*********************/
/*    VARIABLES      */
/*********************/

static OGLPoint3D			gSkyPoints[SKY_GRID_SIZE][SKY_GRID_SIZE];
static OGLTextureCoord		gSkyUVs1[SKY_GRID_SIZE][SKY_GRID_SIZE];
static OGLColorRGBA			gSkyColors[SKY_GRID_SIZE][SKY_GRID_SIZE];
static MOTriangleIndecies	gSkyTriangles[NUM_SKY_TRIANGLES];
static float				gSkyScrollX,gSkyScrollZ;

float						gSkyAltitudeY;

typedef struct
{
	bool		hasSky;
	float		altitude;
	float		dip;
	float		warp;
	bool		fadeEdges;
	bool		drawFirst;		// draw before terrain?
} SkyStyle;

const SkyStyle kSkyTable[NUM_LEVELS] =
{
//	Has?		Alt			Dip			Warp		FadeEdge	Draw1st
	{ true,		4000,		.1f,		.15f,		true,		false,	}, // farm
	{ true,		3500,		.4f,		0,			true,		true,	}, // slime
	{ false,	0,			0,			0,			0,			0,		}, // slime boss
	{ true,		4000,		.3f,		.1f,		true,		false,	}, // apocalypse
	{ true,		-1100,		-.1f,		.2f,		false,		false,	}, // cloud
	{ true,		3500,		0,			0,			true,		false,	}, // jungle
	{ true,		3000,		0,			0,			true,		false,	}, // jungleboss
	{ true,		4000,		.1f,		0,			true,		false,	}, // fireice
	{ false,	0,			0,			0,			0,			0,		}, // saucer
	{ true,		3000,		.2f,		0,			true,		false,	}, // brain boss
};



/************************* INIT SKY ***************************/
//
// Called at the beginning of each level to prime the sky
//

void InitSky(void)
{
int					r,c;
float				cornerX,cornerZ,dist,alpha;
MOTriangleIndecies	*triPtr;
ObjNode				*obj;

	const SkyStyle* mySky = &kSkyTable[gLevelNum];

	gSkyAltitudeY = mySky->altitude;

	if (!mySky->hasSky)								// sky on this level?
		return;

			/* BUILD BASE GRID */

	cornerX = -(float)SKY_GRID_SIZE/2.0f * SKY_GRID_SCALE;
	cornerZ = -(float)SKY_GRID_SIZE/2.0f * SKY_GRID_SCALE;

	for (r = 0; r < SKY_GRID_SIZE; r++)
	{
		for (c = 0; c < SKY_GRID_SIZE; c++)
		{
			gSkyPoints[r][c].x = cornerX + c * SKY_GRID_SCALE + (RandomFloat2() * (SKY_GRID_SCALE * mySky->warp));
			gSkyPoints[r][c].z = cornerZ + r * SKY_GRID_SCALE + (RandomFloat2() * (SKY_GRID_SCALE * mySky->warp));
			gSkyPoints[r][c].y = RandomFloat2() * 100.0f;

			dist = CalcDistance3D(0,0,0, gSkyPoints[r][c].x,gSkyPoints[r][c].y, gSkyPoints[r][c].z);
			if (mySky->fadeEdges)
			{
				alpha = 1.0f - (dist / (gGameViewInfoPtr->yon * .7f));
				if (alpha < 0.0f)
					alpha = 0.0f;
				else
				if (alpha > 1.0f)
					alpha = 1.0f;
			}
			else
				alpha = 1.0;

			gSkyPoints[r][c].y -= dist * mySky->dip;	// dip down as it goes away


			gSkyColors[r][c].r = 1;
			gSkyColors[r][c].g = 1;
			gSkyColors[r][c].b = 1;
			gSkyColors[r][c].a = alpha;
		}
	}

			/* BUILD TRIANGLES */

	triPtr = &gSkyTriangles[0];

	for (r = 0; r < (SKY_GRID_SIZE-1); r++)
	{
		for (c = 0; c < (SKY_GRID_SIZE-1); c++)
		{
			triPtr->vertexIndices[0] = r * SKY_GRID_SIZE + c;
			triPtr->vertexIndices[1] = triPtr->vertexIndices[0] + 1;
			triPtr->vertexIndices[2] = triPtr->vertexIndices[0] + SKY_GRID_SIZE;
			triPtr++;

			triPtr->vertexIndices[0] = r * SKY_GRID_SIZE + c + 1;
			triPtr->vertexIndices[1] = triPtr->vertexIndices[0] + SKY_GRID_SIZE;
			triPtr->vertexIndices[2] = triPtr->vertexIndices[1] -1;
			triPtr++;
		}
	}

	gSkyScrollX = 0;
	gSkyScrollZ = 0;


			/**********************/
			/* CREATE DRAW OBJECT */
			/**********************/

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.slot 		= TERRAIN_SLOT + (mySky->drawFirst? -1: +1);
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.flags 		= STATUS_BIT_KEEPBACKFACES | STATUS_BIT_NOZWRITES | STATUS_BIT_NOLIGHTING;

	obj = MakeNewObject(&gNewObjectDefinition);
	obj->CustomDrawFunction = DrawSky;

}




/******************** DISPOSE SKY **********************/
//
// Called @ end of each level to release data for sky
//

void DisposeSky(void)
{
}



/**************** DRAW SKY *****************/

static void DrawSky(ObjNode *theNode)
{
#pragma unused(theNode)

OGLMatrix4x4	m;
int					r,c;
float			u,v;

	if (!kSkyTable->hasSky)
		return;


		/***************************/
		/* ANIMATE & CALCULATE UVS */
		/***************************/

	u = gSkyScrollX += gFramesPerSecondFrac * .03f;
	v = gSkyScrollZ += gFramesPerSecondFrac * .03f;

	u += gPlayerInfo.camera.cameraLocation.x * .0003f;
	v += gPlayerInfo.camera.cameraLocation.z * .0003f;

	for (r = 0; r < SKY_GRID_SIZE; r++)
	{
		for (c = 0; c < SKY_GRID_SIZE; c++)
		{
			gSkyUVs1[r][c].u = u + c * .8f;
			gSkyUVs1[r][c].v = v + r * .8f;
		}
	}


			/*****************/
			/* DRAW GEOMETRY */
			/*****************/

	OGL_PushState();

	glEnable(GL_TEXTURE_2D);											// enable textures
	glDisableClientState(GL_NORMAL_ARRAY);								// disable normal arrays

	if (gLevelNum == LEVEL_NUM_APOCALYPSE)					// see if glow
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);								// make glow
		gGlobalMaterialFlags |= BG3D_MATERIALFLAG_ALWAYSBLEND;
	}

			/* SETUP VERTEX ARRAY */

	glEnableClientState(GL_VERTEX_ARRAY);								// enable vertex arrays
	glVertexPointer(3, GL_FLOAT, 0, (GLfloat *)&gSkyPoints[0][0]);		// point to point array


			/* SETUP VERTEX UVS */

	glTexCoordPointer(2, GL_FLOAT, 0,(GLfloat *)&gSkyUVs1[0][0]);			// enable uv arrays
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);


			/* SETUP VERTEX COLORS */

	if (kSkyTable[gLevelNum].fadeEdges)
	{
		glColorPointer(4, GL_FLOAT, 0, (GLfloat *)&gSkyColors[0][0]);
		glEnableClientState(GL_COLOR_ARRAY);								// enable color arrays
	}
	else
		glDisableClientState(GL_COLOR_ARRAY);


			/* SET TRANSLATION TRANSFORM */

	OGLMatrix4x4_SetTranslate(&m, gPlayerInfo.camera.cameraLocation.x,
								kSkyTable[gLevelNum].altitude,
								gPlayerInfo.camera.cameraLocation.z);
	glMultMatrixf((GLfloat *)&m);


			/* SUBMIT IT */

	MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_LEVELSPECIFIC][0].materialObject);

	glDrawElements(GL_TRIANGLES,NUM_SKY_TRIANGLES*3,GL_UNSIGNED_INT,&gSkyTriangles[0]);
	if (OGL_CheckError())
		DoFatalAlert("DrawSky: glDrawElements");


	gGlobalMaterialFlags &= ~BG3D_MATERIALFLAG_ALWAYSBLEND;			// make sure this is off
	OGL_PopState();
}


