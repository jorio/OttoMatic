//
// sprites.h
//

#ifndef SPRITES_H
#define SPRITES_H


enum
{
	SPRITE_FLAG_GLOW = (1)
};


typedef struct
{
	long				width,height;
	float			aspectRatio;			// h/w
	long			srcFormat, destFormat;
	MetaObjectPtr	materialObject;
}SpriteType;


void InitSpriteManager(void);
void DisposeAllSpriteGroups(void);
void DisposeSpriteGroup(int groupNum);
void LoadSpriteFile(FSSpec *spec, int groupNum, OGLSetupOutputType *setupInfo);
ObjNode *MakeSpriteObject(NewObjectDefinitionType *newObjDef, OGLSetupOutputType *setupInfo);
void BlendAllSpritesInGroup(short group);
void ModifySpriteObjectFrame(ObjNode *theNode, short type, OGLSetupOutputType *setupInfo);
void DrawSprite(int	group, int type, float x, float y, float scale, float rot, u_long flags, const OGLSetupOutputType *setupInfo);
void BlendASprite(int group, int type);

ObjNode *MakeFontStringObject(const Str31 s, NewObjectDefinitionType *newObjDef, OGLSetupOutputType *setupInfo, Boolean center);


#endif