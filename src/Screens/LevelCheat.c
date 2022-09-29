// OTTO MATIC LEVEL SELECT SCREEN
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include "game.h"

int DoLevelCheatDialog(void (*backgroundDrawRoutine)(void))
{
	static const MenuItem kLevelCheatMenu[] =
	{
		{.type=kMenuItem_Title,		.text=STR_LEVEL_CHEAT},
		{.type=kMenuItem_Spacer},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL_1,		.pick=0},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL_2,		.pick=1},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL_3,		.pick=2},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL_4,		.pick=3},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL_5,		.pick=4},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL_6,		.pick=5},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL_7,		.pick=6},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL_8,		.pick=7},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL_9,		.pick=8},
		{.type=kMenuItem_Pick,		.text=STR_LEVEL_10,		.pick=9},
		{.type=kMenuItem_END_SENTINEL},
	};

	return StartMenu(kLevelCheatMenu, nil, nil, backgroundDrawRoutine);
}
