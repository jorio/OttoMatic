//
// main.h
//

#pragma once
  
#define	GAME_FOV		1.2f


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

//=================================================

extern	void ToolBoxInit(void);
void MoveEverything(void);
void InitDefaultPrefs(void);
void DrawArea(void);
void GameMain(void);
