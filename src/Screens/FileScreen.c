// OTTO MATIC FILE SELECT SCREEN
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include "game.h"
#include <time.h>

bool DoFileScreen(int fileScreenType, void (*backgroundDrawRoutine)(void))
{
	bool isSave = fileScreenType == FILE_SCREEN_TYPE_SAVE;

	const int maxLabelLength = 256;
	char* labelBuffer = AllocPtrClear(maxLabelLength * NUM_SAVE_SLOTS);

	MenuItem menu[NUM_SAVE_SLOTS+5];
	memset(menu, 0, sizeof(menu));

	int menuIndex = 0;

	menu[menuIndex++] = (MenuItem) { .type=kMenuItem_Title, .text=(isSave ? STR_SAVE_GAME : STR_LOAD_GAME) };
	menu[menuIndex++] = (MenuItem) { .type=kMenuItem_Spacer };

	for (int i = 0; i < NUM_SAVE_SLOTS; i++)
	{
		SaveGameType saveData;

		MenuItem* mi = &menu[menuIndex++];

		if (!LoadSaveGameStruct(i, &saveData))
		{
			mi->text = STR_EMPTY_SLOT;

			if (isSave)
			{
				mi->type = kMenuItem_Pick;
				mi->pick = i;
			}
			else
			{
				mi->type = kMenuItem_Label;
			}
		}
		else
		{
			time_t timestamp = (time_t) saveData.timestamp;
			struct tm *timeinfo = localtime(&timestamp);

			snprintf(labelBuffer+maxLabelLength*i, maxLabelLength,
					"%s %d\t%d %s %d",
					Localize(STR_LEVEL),
					saveData.realLevel+1,
					timeinfo->tm_mday,
					Localize(timeinfo->tm_mon + STR_JANUARY),
					1900 + timeinfo->tm_year);

			mi->type = kMenuItem_Pick;
			mi->pick = i;
			mi->text = -1;
			mi->rawText = labelBuffer+maxLabelLength*i;
		}
	}

	menu[menuIndex++].type = kMenuItem_Spacer;

	menu[menuIndex++] = (MenuItem) {.type=kMenuItem_Action, .action.callback = MenuCallback_Back,
									.text = isSave ? STR_CONTINUE_WITHOUT_SAVING : STR_BACK };

	menu[menuIndex++].type = kMenuItem_END_SENTINEL;

	GAME_ASSERT(menuIndex == sizeof(menu)/sizeof(menu[0]));

	//-----------------------------------

	int pickedSlot = StartMenu(menu, nil, nil, backgroundDrawRoutine);

	SafeDisposePtr(labelBuffer);

	if (pickedSlot >= 0)
	{
		if (isSave)
			SaveGame(pickedSlot);
		else
			LoadSavedGame(pickedSlot);
		return true;
	}

	return false;
}
