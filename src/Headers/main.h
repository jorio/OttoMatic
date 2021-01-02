//
// main.h
//

#ifndef __MAIN
#define __MAIN
  
#define	GAME_FOV		1.2f
  
#define	TRACK_COMPLETE_COOLDOWN_TIME	5.0				// n seconds
  
  


#define	AGE_MASK_AGE	0x0f			// lower 4 bits are age
#define	AGE_MASK_STAGE	0xf0			// upper 4 bits are stage within age

//#include <ppc_intrinsics.h>


enum
{
	LEVEL_NUM_FARM = 0,
	LEVEL_NUM_BLOB,
	LEVEL_NUM_BLOBBOSS,
	LEVEL_NUM_APOCALYPSE,
	LEVEL_NUM_CLOUD,
	LEVEL_NUM_JUNGLE,
	LEVEL_NUM_JUNGLEBOSS,
	LEVEL_NUM_FIREICE,
	LEVEL_NUM_SAUCER,
	LEVEL_NUM_BRAINBOSS,
	
	NUM_LEVELS
};


enum
{
	LANGUAGE_ENGLISH = 0,
	LANGUAGE_FRENCH,
	LANGUAGE_GERMAN,
	LANGUAGE_SPANISH,
	LANGUAGE_ITALIAN,
	LANGUAGE_SWEDISH,
	MAX_LANGUAGES
};



  
//=================================================

#if 0 // srcport rm
extern	IBNibRef 			gNibs;
extern	CFBundleRef 		gBundle;
#endif

extern	int main(void);
extern	void ToolBoxInit(void);
void MoveEverything(void);
void InitDefaultPrefs(void);
void DrawArea(OGLSetupOutputType *setupInfo);

#endif
