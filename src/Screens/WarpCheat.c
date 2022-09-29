// WARP CHEAT
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include "game.h"

#define kMaxItems		24
#define kCharsPerLine		64

void DoWarpCheat(void)
{
	MenuItem menu[kMaxItems + 1];
	char textBuf[kMaxItems * kCharsPerLine];
	char* textPtr = textBuf;
	int mi = 0;		// menu item ID

		/* BUILD MENU */
	
	menu[mi++] = (MenuItem){ .type = kMenuItem_Title, .text = STR_WARP_TO_ITEM };
	menu[mi++] = (MenuItem){ .type = kMenuItem_Spacer };

	for (int i = 0; i < gNumTerrainItems; i++)		// Look for checkpoints in map items
	{
		const TerrainItemEntryType* item = &(*gMasterItemList)[i];
		const char* itemName = nil;

		switch (item->type)
		{
			case MAP_ITEM_MYSTARTCOORD:		itemName = "Start"; break;
			case MAP_ITEM_EXITROCKET:		itemName = "Exit"; break;
			case MAP_ITEM_CHECKPOINT:		itemName = "Checkpoint"; break;
			case MAP_ITEM_TELEPORTER:		itemName = "Teleporter"; break;
			default:						continue;
		}

		GAME_ASSERT(mi < kMaxItems);

		snprintf(textPtr, kCharsPerLine, "#%03d   %05d,%05d   \t%s", i, item->x, item->y, itemName);
		menu[mi] = (MenuItem){ .type = kMenuItem_Pick, .text = -1, .rawText = textPtr, .pick = i };

		mi++;
		textPtr += kCharsPerLine;
	}

	menu[mi].type = kMenuItem_END_SENTINEL;

		/* RUN MENU */

	gGamePaused = true;

	int pick = StartMenu(menu, nil, PausedUpdateCallback, DrawObjects);

	gGamePaused = false;

		/* WARP PLAYER TO PICKED ITEM */

	if (pick >= 0)
	{
		float x = (*gMasterItemList)[pick].x;
		float z = (*gMasterItemList)[pick].y;

		gPlayerInfo.coord.x = gPlayerInfo.objNode->Coord.x = x + 150;
		gPlayerInfo.coord.z = gPlayerInfo.objNode->Coord.z = z;
		gPlayerInfo.coord.y = gPlayerInfo.objNode->Coord.y = FindHighestCollisionAtXZ(x, z, ~0u) + 500;

		InitCamera();
	}
}
