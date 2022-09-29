/****************************/
/*   	SPRITES.C			*/
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/


/****************************/
/*    CONSTANTS             */
/****************************/

#define	FONT_WIDTH	.51f


/*********************/
/*    VARIABLES      */
/*********************/

SpriteType	*gSpriteGroupList[MAX_SPRITE_GROUPS];
long		gNumSpritesInGroupList[MAX_SPRITE_GROUPS];		// note:  this must be long's since that's what we read from the sprite file!



/****************** INIT SPRITE MANAGER ***********************/

void InitSpriteManager(void)
{
int	i;

	for (i = 0; i < MAX_SPRITE_GROUPS; i++)
	{
		gSpriteGroupList[i] = nil;
		gNumSpritesInGroupList[i] = 0;
	}
}


/******************* DISPOSE ALL SPRITE GROUPS ****************/

void DisposeAllSpriteGroups(void)
{
int	i;

	for (i = 0; i < MAX_SPRITE_GROUPS; i++)
	{
		if (gSpriteGroupList[i])
			DisposeSpriteGroup(i);
	}
}


/************************** DISPOSE BG3D *****************************/

void DisposeSpriteGroup(int groupNum)
{
int 		i,n;

	n = gNumSpritesInGroupList[groupNum];						// get # sprites in this group
	if ((n == 0) || (gSpriteGroupList[groupNum] == nil))
		return;


			/* DISPOSE OF ALL LOADED OPENGL TEXTURENAMES */

	for (i = 0; i < n; i++)
		MO_DisposeObjectReference(gSpriteGroupList[groupNum][i].materialObject);


		/* DISPOSE OF GROUP'S ARRAY */

	SafeDisposePtr((Ptr)gSpriteGroupList[groupNum]);
	gSpriteGroupList[groupNum] = nil;
	gNumSpritesInGroupList[groupNum] = 0;
}



/********************** LOAD SPRITE FILE **************************/
//
// NOTE:  	All sprite files must be imported AFTER the draw context has been created,
//			because all imported textures are named with OpenGL and loaded into OpenGL!
//

void LoadSpriteFile(FSSpec *spec, int groupNum)
{
short			refNum;
int				i,w,h;
long			count;
MOMaterialData	matData;


		/* OPEN THE FILE */

	if (FSpOpenDF(spec, fsRdPerm, &refNum) != noErr)
		DoFatalAlert("LoadSpriteFile: FSpOpenDF failed");

		/* READ # SPRITES IN THIS FILE */

	gNumSpritesInGroupList[groupNum] = FSReadBELong(refNum);


		/* ALLOCATE MEMORY FOR SPRITE RECORDS */

	gSpriteGroupList[groupNum] = (SpriteType *)AllocPtr(sizeof(SpriteType) * gNumSpritesInGroupList[groupNum]);
	if (gSpriteGroupList[groupNum] == nil)
		DoFatalAlert("LoadSpriteFile: AllocPtr failed");


			/********************/
			/* READ EACH SPRITE */
			/********************/

	for (i = 0; i < gNumSpritesInGroupList[groupNum]; i++)
	{
		long		bufferSize;
		u_char *buffer;

			/* READ WIDTH/HEIGHT, ASPECT RATIO, SRC/DEST FORMATS */

		gSpriteGroupList[groupNum][i].width			= FSReadBELong(refNum);
		gSpriteGroupList[groupNum][i].height		= FSReadBELong(refNum);
		gSpriteGroupList[groupNum][i].aspectRatio	= FSReadBEFloat(refNum);
		gSpriteGroupList[groupNum][i].srcFormat		= FSReadBELong(refNum);
		gSpriteGroupList[groupNum][i].destFormat	= FSReadBELong(refNum);


			/* READ BUFFER SIZE */

		bufferSize = FSReadBELong(refNum);

		buffer = AllocPtr(bufferSize);							// alloc memory for buffer
		if (buffer == nil)
			DoFatalAlert("LoadSpriteFile: AllocPtr failed");


			/* READ THE SPRITE PIXEL BUFFER */

		count = bufferSize;
		FSRead(refNum, &count, (Ptr) buffer);

		if (gSpriteGroupList[groupNum][i].srcFormat == GL_UNSIGNED_SHORT_1_5_5_5_REV)
		{
			int		q;
			u_short *pix = (u_short *)buffer;
			for (q = 0; q < (count/2); q++)
			{
				pix[q] = SwizzleUShort(&pix[q]);
			}
		}


				/*****************************/
				/* CREATE NEW TEXTURE OBJECT */
				/*****************************/

		matData.setupInfo		= gGameViewInfoPtr;
		matData.flags			= BG3D_MATERIALFLAG_TEXTURED;
		matData.diffuseColor.r	= 1;
		matData.diffuseColor.g	= 1;
		matData.diffuseColor.b	= 1;
		matData.diffuseColor.a	= 1;

		matData.numMipmaps		= 1;
		w = matData.width		= gSpriteGroupList[groupNum][i].width;
		h = matData.height		= gSpriteGroupList[groupNum][i].height;

		matData.pixelSrcFormat	= gSpriteGroupList[groupNum][i].srcFormat;
		matData.pixelDstFormat	= gSpriteGroupList[groupNum][i].destFormat;

		matData.texturePixels[0]= nil;											// we're going to preload

					/* SPRITE IS 16-BIT PACKED PIXEL FORMAT */

		if (matData.pixelSrcFormat == GL_UNSIGNED_SHORT_1_5_5_5_REV)	// see if convert 24 to 16-bit
		{
			matData.textureName[0] = OGL_TextureMap_Load(buffer, matData.width, matData.height, GL_BGRA_EXT, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV); // load 16 as 16

		}

				/* CONVERT 24-BIT TO 16--BIT */

		else
		if ((matData.pixelSrcFormat == GL_RGB) && (matData.pixelDstFormat == GL_RGB5_A1))
		{
			u_short	*buff16 = (u_short *)AllocPtr(matData.width*matData.height*2);			// alloc buff for 16-bit texture

			ConvertTexture24To16(buffer, buff16, matData.width, matData.height);
			matData.textureName[0] = OGL_TextureMap_Load(buff16, matData.width, matData.height, GL_BGRA_EXT, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV); // load 16 as 16

			SafeDisposePtr((Ptr)buff16);							// dispose buff

		}

				/* USE INPUT FORMATS */
		else
		{
			matData.textureName[0] 	= OGL_TextureMap_Load(buffer,
													 matData.width,
													 matData.height,
													 matData.pixelSrcFormat,
													 matData.pixelDstFormat, GL_UNSIGNED_BYTE);
		}

		gSpriteGroupList[groupNum][i].materialObject = MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);

		if (gSpriteGroupList[groupNum][i].materialObject == nil)
			DoFatalAlert("LoadSpriteFile: MO_CreateNewObjectOfType failed");


		SafeDisposePtr((Ptr)buffer);														// free the buffer
	}



		/* CLOSE FILE */

	FSClose(refNum);


}


#pragma mark -

/************* MAKE NEW SRITE OBJECT *************/

ObjNode *MakeSpriteObject(NewObjectDefinitionType *newObjDef)
{
ObjNode				*newObj;
MOSpriteObject		*spriteMO;
MOSpriteSetupData	spriteData;

			/* ERROR CHECK */

	if (newObjDef->type >= gNumSpritesInGroupList[newObjDef->group])
		DoFatalAlert("MakeSpriteObject: illegal type");


			/* MAKE OBJNODE */

	newObjDef->genre = SPRITE_GENRE;
	newObjDef->flags |= STATUS_BIT_KEEPBACKFACES|STATUS_BIT_NOZBUFFER|STATUS_BIT_NOLIGHTING;

	newObj = MakeNewObject(newObjDef);
	if (newObj == nil)
		return(nil);

			/* MAKE SPRITE META-OBJECT */

	spriteData.loadFile = false;										// these sprites are already loaded into gSpriteList
	spriteData.group	= newObjDef->group;								// set group
	spriteData.type 	= newObjDef->type;								// set group subtype


	spriteMO = MO_CreateNewObjectOfType(MO_TYPE_SPRITE, 0, &spriteData);
	if (!spriteMO)
		DoFatalAlert("MakeSpriteObject: MO_CreateNewObjectOfType failed!");


			/* SET SPRITE MO INFO */

	spriteMO->objectData.scaleX =
	spriteMO->objectData.scaleY = newObj->Scale.x;
	spriteMO->objectData.coord = newObj->Coord;


			/* ATTACH META OBJECT TO OBJNODE */

	newObj->SpriteMO = spriteMO;

	return(newObj);
}


/*********************** MODIFY SPRITE OBJECT IMAGE ******************************/

void ModifySpriteObjectFrame(ObjNode *theNode, short type)
{
MOSpriteSetupData	spriteData;
MOSpriteObject		*spriteMO;


	if (type == theNode->Type)										// see if it is the same
		return;

		/* DISPOSE OF OLD TYPE */

	MO_DisposeObjectReference(theNode->SpriteMO);


		/* MAKE NEW SPRITE MO */

	spriteData.loadFile = false;									// these sprites are already loaded into gSpriteList
	spriteData.group	= theNode->Group;							// set group
	spriteData.type 	= type;										// set group subtype

	spriteMO = MO_CreateNewObjectOfType(MO_TYPE_SPRITE, 0, &spriteData);
	if (!spriteMO)
		DoFatalAlert("ModifySpriteObjectFrame: MO_CreateNewObjectOfType failed!");


			/* SET SPRITE MO INFO */

	spriteMO->objectData.scaleX =
	spriteMO->objectData.scaleY = theNode->Scale.x;
	spriteMO->objectData.coord = theNode->Coord;


			/* ATTACH META OBJECT TO OBJNODE */

	theNode->SpriteMO = spriteMO;
	theNode->Type = type;
}


#pragma mark -

/*********************** BLEND ALL SPRITES IN GROUP ********************************/
//
// Set the blending flag for all sprites in the group.
//

void BlendAllSpritesInGroup(short group)
{
int		i,n;
MOMaterialObject	*m;

	n = gNumSpritesInGroupList[group];								// get # sprites in this group
	if ((n == 0) || (gSpriteGroupList[group] == nil))
		DoFatalAlert("BlendAllSpritesInGroup: this group is empty");


			/* DISPOSE OF ALL LOADED OPENGL TEXTURENAMES */

	for (i = 0; i < n; i++)
	{
		m = gSpriteGroupList[group][i].materialObject; 				// get material object ptr
		if (m == nil)
			DoFatalAlert("BlendAllSpritesInGroup: material == nil");

		m->objectData.flags |= 	BG3D_MATERIALFLAG_ALWAYSBLEND;		// set flag
	}
}


/*********************** BLEND A SPRITE ********************************/
//
// Set the blending flag for 1 sprite in the group.
//

void BlendASprite(int group, int type)
{
MOMaterialObject	*m;

	if (type >= gNumSpritesInGroupList[group])
		DoFatalAlert("BlendASprite: illegal type");


			/* DISPOSE OF ALL LOADED OPENGL TEXTURENAMES */

	m = gSpriteGroupList[group][type].materialObject; 				// get material object ptr
	if (m == nil)
		DoFatalAlert("BlendASprite: material == nil");

	m->objectData.flags |= 	BG3D_MATERIALFLAG_ALWAYSBLEND;		// set flag
}


/************************** DRAW SPRITE ************************/

void DrawSprite(int	group, int type, float x, float y, float scale, float rot, u_long flags)
{
			/* SET STATE */

	OGL_PushState();								// keep state

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 640, 480, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gGlobalMaterialFlags = BG3D_MATERIALFLAG_CLAMP_V|BG3D_MATERIALFLAG_CLAMP_U;	// clamp all textures
	OGL_DisableLighting();
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);


	if (flags & SPRITE_FLAG_GLOW)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	if (rot != 0.0f)
		glRotatef(OGLMath_RadiansToDegrees(rot), 0, 0, 1);											// remember:  rotation is in degrees, not radians!


		/* ACTIVATE THE MATERIAL */

	MO_DrawMaterial(gSpriteGroupList[group][type].materialObject);


			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(x, y);
	glTexCoord2f(1,1);	glVertex2f(x+scale, y);
	glTexCoord2f(1,0);	glVertex2f(x+scale, y+scale);
	glTexCoord2f(0,0);	glVertex2f(x, y+scale);
	glEnd();


		/* CLEAN UP */

	OGL_PopState();									// restore state
	gGlobalMaterialFlags = 0;

	gPolysThisFrame += 2;						// 2 tris drawn
}
