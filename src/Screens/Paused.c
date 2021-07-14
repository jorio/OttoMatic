// PAUSED.C
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include "game.h"

Boolean			gGamePaused = false;

static const OGLColorRGBA gPausedMenuNoHiliteColor = {1,1,.3,1};

static void PausedUpdateCallback(void)
{
	// need to call this to keep supertiles active
	DoPlayerTerrainUpdate(gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z);
}

/********************** DO PAUSED **************************/

void DoPaused(void)
{
	GammaOn();

	gGamePaused = true;
	Pomme_PauseAllChannels(true);

	static const MenuItem pauseMenu[] =
	{
		{.type=kMenuItem_Pick,		.text=STR_RESUME_GAME,			.pick=0},
		{.type=kMenuItem_Pick,		.text=STR_RETIRE_GAME,			.pick=1},
		{.type=kMenuItem_Pick,		.text=STR_QUIT_APPLICATION,		.pick=2},
		{.type=kMenuItem_END_SENTINEL},
	};

	MenuStyle menuStyle = kDefaultMenuStyle;
	menuStyle.centeredText = true;
	menuStyle.asyncFadeOut = true;
	menuStyle.fadeInSpeed = 15;
	menuStyle.inactiveColor = gPausedMenuNoHiliteColor;
	menuStyle.startPosition = (OGLPoint3D) {0,-12,0};
	menuStyle.standardScale = 1.0f;
	menuStyle.rowHeight = 24;
	menuStyle.darkenPane = false;
	menuStyle.playMenuChangeSounds = false;

	int pick = StartMenu(pauseMenu, &menuStyle, PausedUpdateCallback, DrawArea);

	Pomme_PauseAllChannels(false);
	gGamePaused = false;

	if (pick == 1)
		gGameOver = true;
	else if (pick == 2)
		CleanQuit();
}
