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
	int				width,height;
	float			aspectRatio;			// h/w
	int				srcFormat, destFormat;
	MetaObjectPtr	materialObject;
}SpriteType;


void InitSpriteManager(void);
void DisposeAllSpriteGroups(void);
void DisposeSpriteGroup(int groupNum);
void LoadSpriteGroup(int groupNum, int numSprites, const char* basename);
ObjNode *MakeSpriteObject(NewObjectDefinitionType *newObjDef);
void BlendAllSpritesInGroup(short group);
void ModifySpriteObjectFrame(ObjNode *theNode, short type);
void DrawSprite(int	group, int type, float x, float y, float scale, float rot, uint32_t flags);
void BlendASprite(int group, int type);
