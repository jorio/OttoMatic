/****************************/
/*   		SKY.C		    */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"
#include <AGL/aglmacro.h>

extern	float				gFramesPerSecondFrac,gFramesPerSecond;
extern	OGLPoint3D			gCoord;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLVector3D			gDelta;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	FSSpec		gDataSpec;
extern	SpriteType	*gSpriteGroupList[MAX_SPRITE_GROUPS];
extern	PlayerInfoType	gPlayerInfo;
extern	int				gLevelNum;
extern	u_long			gGlobalMaterialFlags;


/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawSky(ObjNode *theNode, const OGLSetupOutputType *setupInfo);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	SKY_GRID_SIZE		10			// # verts x / z

#define	SKY_GRID_SCALE		(setupInfo->yon / (SKY_GRID_SIZE / 2.0f))		// scale of each grid segment

#define	NUM_SKY_TRIANGLES	((SKY_GRID_SIZE-1) * (SKY_GRID_SIZE-1) * 2)

/*********************/
/*    VARIABLES      */
/*********************/

static Boolean				gIsSky = false;
static OGLPoint3D			gSkyPoints[SKY_GRID_SIZE][SKY_GRID_SIZE];
static OGLTextureCoord		gSkyUVs1[SKY_GRID_SIZE][SKY_GRID_SIZE];
static OGLColorRGBA			gSkyColors[SKY_GRID_SIZE][SKY_GRID_SIZE];
static MOTriangleIndecies	gSkyTriangles[NUM_SKY_TRIANGLES];
static float				gSkyScrollX,gSkyScrollZ;

const	Boolean gIsSkyTable[NUM_LEVELS] =
{
	true,				// farm
	true,				// slime
	false,				// slime boss
	true,				// apocalypse
	true,				// cloud
	true,				// jungle
	true,				// jungleboss
	true,				// fireice
	false,				// saucer
	true,				// brain boss
};


const float gSkyY[NUM_LEVELS] =
{
	4000.0f,		// farm
	3500.0f,		// slime
	4000.0f,		// slime boss
	4000.0f,		// apocalypse
	-1100.0f,		// cloud
	3500.0f,		// jungle
	3000.0f,		// jungle boss
	4000.0f,		// fire ice
	3000.0f,
	3000.0f,		// brain boss
};

const float gSkyDip[NUM_LEVELS] =
{
	.1,					// farm
	.4,					// slime
	.3,					// slime boss
	.3,					// apocalypse
	-.1,				// cloud
	0,					// jungle
	0,					// jungle boss
	.1,					// fire ice
	0,
	.2					// brain boss
};

const float gSkyWarp[NUM_LEVELS] =
{
	.15,			// farm
	0,				// slime
	0,				// slime boss
	.1,				// apocalypse
	.2,				// cloud
	0,
	0,
	0,
	0,
	0,				// brain boss
};


const float gFadeEdges[NUM_LEVELS] =
{
	true,			// farm
	true,				// slime
	true,				// slime boss
	true,				// apocalypse
	false,				// cloud
	true,
	true,
	true,
	true,
	true,				// brain boss
};




/************************* INIT SKY ***************************/
//
// Called at the beginning of each level to prime the sky
//

void InitSky(OGLSetupOutputType *setupInfo)
{
int					r,c;
float				cornerX,cornerZ,dist,alpha;
MOTriangleIndecies	*triPtr;
Boolean				fadeEdges;
ObjNode				*obj;

	gIsSky = gIsSkyTable[gLevelNum];								// sky on this level?
	if (!gIsSky)
		return;

	fadeEdges = gFadeEdges[gLevelNum];

			/* BUILD BASE GRID */

	cornerX = -(float)SKY_GRID_SIZE/2.0f * SKY_GRID_SCALE;
	cornerZ = -(float)SKY_GRID_SIZE/2.0f * SKY_GRID_SCALE;

	for (r = 0; r < SKY_GRID_SIZE; r++)
	{
		for (c = 0; c < SKY_GRID_SIZE; c++)
		{
			gSkyPoints[r][c].x = cornerX + c * SKY_GRID_SCALE + (RandomFloat2() * (SKY_GRID_SCALE * gSkyWarp[gLevelNum]));
			gSkyPoints[r][c].z = cornerZ + r * SKY_GRID_SCALE + (RandomFloat2() * (SKY_GRID_SCALE * gSkyWarp[gLevelNum]));
			gSkyPoints[r][c].y = RandomFloat2() * 100.0f;

			dist = CalcDistance3D(0,0,0, gSkyPoints[r][c].x,gSkyPoints[r][c].y, gSkyPoints[r][c].z);
			if (fadeEdges)
			{
				alpha = 1.0f - (dist / (setupInfo->yon * .7f));
				if (alpha < 0.0f)
					alpha = 0.0f;
				else
				if (alpha > 1.0f)
					alpha = 1.0f;
			}
			else
				alpha = 1.0;

			gSkyPoints[r][c].y -= dist * gSkyDip[gLevelNum];	// dip down as it goes away


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

	gIsSky = true;


			/**********************/
			/* CREATE DRAW OBJECT */
			/**********************/

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.slot 		= TERRAIN_SLOT+1;
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
	gIsSky = false;
}



/**************** DRAW SKY *****************/

static void DrawSky(ObjNode *theNode, const OGLSetupOutputType *setupInfo)
{
#pragma unused(theNode)

OGLMatrix4x4	m;
int					r,c;
float			u,v;
AGLContext agl_ctx = setupInfo->drawContext;

	if (!gIsSky)
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

	if (gFadeEdges[gLevelNum])
	{
		glColorPointer(4, GL_FLOAT, 0, (GLfloat *)&gSkyColors[0][0]);
		glEnableClientState(GL_COLOR_ARRAY);								// enable color arrays
	}
	else
		glDisableClientState(GL_COLOR_ARRAY);


			/* SET TRANSLATION TRANSFORM */

	OGLMatrix4x4_SetTranslate(&m, gPlayerInfo.camera.cameraLocation.x,
								gSkyY[gLevelNum],
								gPlayerInfo.camera.cameraLocation.z);
	glMultMatrixf((GLfloat *)&m);


			/* SUBMIT IT */

	MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_LEVELSPECIFIC][0].materialObject, setupInfo);

	glDrawElements(GL_TRIANGLES,NUM_SKY_TRIANGLES*3,GL_UNSIGNED_INT,&gSkyTriangles[0]);
	if (OGL_CheckError())
		DoFatalAlert("\pDrawSky: glDrawElements");


	gGlobalMaterialFlags &= ~BG3D_MATERIALFLAG_ALWAYSBLEND;			// make sure this is off
	OGL_PopState();
}


