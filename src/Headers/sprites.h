//
// sprites.h
//

#pragma once


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
void LoadSpriteFile(FSSpec *spec, int groupNum);
ObjNode *MakeSpriteObject(NewObjectDefinitionType *newObjDef);
void BlendAllSpritesInGroup(short group);
void ModifySpriteObjectFrame(ObjNode *theNode, short type);
void DrawSprite(int	group, int type, float x, float y, float scale, float rot, u_long flags);
void BlendASprite(int group, int type);
