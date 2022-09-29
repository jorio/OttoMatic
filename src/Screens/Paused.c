// PAUSED.C
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include "game.h"

Boolean			gGamePaused = false;

static const OGLColorRGBA gPausedMenuNoHiliteColor = {1,1,.3,1};

void PausedUpdateCallback(void)
{
	// need to call this to keep supertiles active
	DoPlayerTerrainUpdate(gPlayerInfo.camera.cameraLocation.x, gPlayerInfo.camera.cameraLocation.z);
}

/********************** DO PAUSED **************************/

void DoPaused(void)
{
	int pick = 0;

	gGammaFadeFrac = 1;//GammaOn();

	gGamePaused = true;
	PauseAllChannels(true);

	static const MenuItem pauseMenu[] =
	{
		{.type=kMenuItem_Pick,		.text=STR_RESUME_GAME,			.pick=0},
		{.type=kMenuItem_Pick,		.text=STR_SETTINGS,				.pick=1},
		{.type=kMenuItem_Pick,		.text=STR_RETIRE_GAME,			.pick=2},
		{.type=kMenuItem_Pick,		.text=STR_QUIT_APPLICATION,		.pick=3},
		{.type=kMenuItem_END_SENTINEL},
	};

	MenuStyle menuStyle = kDefaultMenuStyle;
	menuStyle.centeredText = true;
	menuStyle.asyncFadeOut = true;
	menuStyle.fadeInSpeed = 15;
	menuStyle.inactiveColor = gPausedMenuNoHiliteColor;
	menuStyle.standardScale = 1.0f;
	menuStyle.rowHeight = 24;
	menuStyle.uniformXExtent = 100;
	menuStyle.darkenPane = true;
	menuStyle.darkenPaneScaleY = 64;
	menuStyle.darkenPaneOpacity = .3f;
	menuStyle.playMenuChangeSounds = false;
	menuStyle.startButtonExits = true;

again:
	pick = StartMenu(pauseMenu, &menuStyle, PausedUpdateCallback, DrawObjects);


	switch (pick)
	{
		case 1:
			DoSettingsOverlay(PausedUpdateCallback, DrawObjects);
			goto again;
			break;

		case 2:
			gGameOver = true;
			break;

		case 3:
			CleanQuit();
			break;

		default:
			break;
	}

	gGamePaused = false;

	PauseAllChannels(false);
	EnforceMusicPausePref();
}
