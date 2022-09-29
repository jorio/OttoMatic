//
// sparkle.h
//

#define	MAX_SPARKLES	600

enum
{
	SPARKLE_FLAG_OMNIDIRECTIONAL = 1,				// if is visible from any angle
	SPARKLE_FLAG_RANDOMSPIN = (1<<1),
	SPARKLE_FLAG_TRANSFORMWITHOWNER = (1<<2),		// if want to use owner's transform matrix
	SPARKLE_FLAG_FLICKER = (1<<3)		
};

typedef struct
{
	Boolean				isActive;								// true if this one is active
	ObjNode				*owner;									// node which owns this sparkle (or nil)
	u_long				flags;						
	OGLPoint3D			where;	
	OGLVector3D			aim;
	OGLColorRGBA		color;
	float				scale;
	float				separation;								// how far to move sparkle from base coord toward camera
	short				textureNum;
}SparkleType;


void InitSparkles(void);
short GetFreeSparkle(ObjNode *theNode);
void DeleteSparkle(short i);

void CreatePlayerSparkles(ObjNode *theNode);
void UpdatePlayerSparkles(ObjNode *theNode);
void CreatePlayerJumpJetSparkles(ObjNode *theNode);

Boolean AddRunwayLights(TerrainItemEntryType *itemPtr, long  x, long z);
