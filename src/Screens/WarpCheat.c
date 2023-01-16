// WARP CHEAT
// (C) 2023 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include "game.h"

#define kMaxItems			30
#define kCharsPerLine		128

static int CompareWarpMenuEntries(const void* ptrA, const void* ptrB)
{
	const MenuItem* menuItemA = ptrA;
	const MenuItem* menuItemB = ptrB;

	int itemIndexA = menuItemA->pick;
	int itemIndexB = menuItemB->pick;

	const TerrainItemEntryType* itemA = &(*gMasterItemList)[itemIndexA];
	const TerrainItemEntryType* itemB = &(*gMasterItemList)[itemIndexB];

		/* SORT BY TYPE FIRST */

	if (itemA->type != itemB->type)
		return itemA->type - itemB->type;

		/* THEN SORT BY SEQUENCE NUMBER (PARM 0) */

	if (itemA->parm[0] != itemB->parm[0])
		return ((int)itemA->parm[0]) - ((int)itemB->parm[0]);

		/* FINALLY SORT BY INDEX WITHIN .TER FILE */

	return gTerrainItemFileIDs[itemIndexA] - gTerrainItemFileIDs[itemIndexB];
}

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
		char name[kCharsPerLine/2];

		switch (item->type)
		{
			case MAP_ITEM_MYSTARTCOORD:
				snprintf(name, sizeof(name), "Start");
				break;

			case MAP_ITEM_EXITROCKET:
				snprintf(name, sizeof(name), "Exit");
				break;

			case MAP_ITEM_TELEPORTER:
				snprintf(name, sizeof(name), "Teleporter %d > %d", item->parm[0], item->parm[1]);
				break;

			case MAP_ITEM_CHECKPOINT:
				snprintf(name, sizeof(name), "Checkpoint %d", item->parm[0]);
				break;

			case MAP_ITEM_BUBBLEPUMP:
				snprintf(name, sizeof(name), "BubblePump");
				break;

			case MAP_ITEM_BUMPERCARGATE:
				snprintf(name, sizeof(name), "BumperCarGate");
				break;

			case MAP_ITEM_ZIPLINE:
				if (item->parm[1] != 0)		// only show posts at start of zipline
					continue;
				snprintf(name, sizeof(name), "ZipLine %d", item->parm[0]);
				break;

			case MAP_ITEM_ROCKETSLED:
				snprintf(name, sizeof(name), "RocketSled");
				break;

			default:
				continue;
		}

		GAME_ASSERT(mi < kMaxItems);

		snprintf(textPtr, kCharsPerLine, "#%03d   %05d,%05d   %X,%X,%X,%X   %s",
				 gTerrainItemFileIDs[i], item->x, item->y,
				 item->parm[0],
				 item->parm[1],
				 item->parm[2],
				 item->parm[3],
				 name);

		menu[mi] = (MenuItem){ .type = kMenuItem_Pick, .text = -1, .rawText = textPtr, .pick = i };

		mi++;
		textPtr += kCharsPerLine;
	}


		/* SORT PICKABLE ITEMS */

	SDL_qsort(&menu[2], mi-2, sizeof(menu[0]), CompareWarpMenuEntries);

		/* MARK END OF MENU */

	menu[mi].type = kMenuItem_END_SENTINEL;

		/* RUN MENU */

	gGamePaused = true;


	MenuStyle style = kDefaultMenuStyle;
	style.rowHeight = 14;
	style.standardScale = 0.5f;

	CaptureMouse(false);

	int pick = StartMenu(menu, &style, PausedUpdateCallback, DrawObjects);

	gGamePaused = false;

		/* WARP PLAYER TO PICKED ITEM */

	if (pick >= 0)
	{
		float x = (*gMasterItemList)[pick].x;
		float z = (*gMasterItemList)[pick].y;
		float y = FindHighestCollisionAtXZ(x, z, ~0u);

		if (fabsf(y - GetTerrainY(x, z)) < EPS)		// supertile may be out of bounds so object collisions may not have been loaded in yet. Move Otto really high above terrain to be sure
			y += 1000;
		else
			y += 250;

		x += 150;		// put Otto besides checkpoint

		gPlayerInfo.coord.x = gPlayerInfo.objNode->Coord.x = x;
		gPlayerInfo.coord.z = gPlayerInfo.objNode->Coord.z = z;
		gPlayerInfo.coord.y = gPlayerInfo.objNode->Coord.y = y;

		InitCamera();

		CaptureMouse(true);
	}
}
