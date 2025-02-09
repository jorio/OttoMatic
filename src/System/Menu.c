// MENU.C
// (c)2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static ObjNode* MakeTextAtRowCol(const char* text, int row, int col);
static void LayOutMenu(const MenuItem* menu);
static ObjNode* LayOutCyclerValueText(int row);

#define SpecialRow					Special[0]
#define SpecialCol					Special[1]
#define SpecialPulsateTimer			SpecialF[0]
#define SpecialSweepTimer			SpecialF[1]

/****************************/
/*    CONSTANTS             */
/****************************/

#define MAX_MENU_ROWS	32
#define MAX_MENU_COLS	5

#define kSfxNavigate	EFFECT_WEAPONCLICK
#define kSfxMenuChange	EFFECT_MENUCHANGE
#define kSfxCycle		EFFECT_ATOMCHIME
#define kSfxError		EFFECT_NOJUMPJET
#define kSfxDelete		EFFECT_FLAREEXPLODE

const int16_t kJoystickDeadZone_BindingThreshold = (75 * 32767 / 100);

enum
{
	kMenuStateOff,
	kMenuStateFadeIn,
	kMenuStateReady,
	kMenuStateFadeOut,
	kMenuStateAwaitingKeyPress,
	kMenuStateAwaitingPadPress,
	kMenuStateAwaitingMouseClick,
};

const MenuStyle kDefaultMenuStyle =
{
	.darkenPane			= true,
	.darkenPaneScaleY	= 480,
	.darkenPaneOpacity	= .8f,
	.fadeInSpeed		= 3.0f,
	.asyncFadeOut		= true,
	.centeredText		= false,
	.titleColor			= {1.0f, 1.0f, 0.7f, 1.0f},
	.inactiveColor		= {0.3f, 0.5f, 0.2f, 1.0f},
	.inactiveColor2		= {0.8f, 0.0f, 0.5f, 0.5f},
	.standardScale		= .75f,
	.titleScale			= 1.25f,
	.subtitleScale		= .5f,
	.rowHeight			= 13*1.5f,
	.uniformXExtent		= 0,
	.playMenuChangeSounds	= true,
	.startButtonExits	= false,
	.isInteractive		= true,
};

/*********************/
/*    VARIABLES      */
/*********************/

static const MenuItem*		gMenu = nil;
static const MenuItem*		gRootMenu = nil;
static const MenuStyle*		gMenuStyle = nil;
static int					gNumMenuEntries;
static int					gMenuRow = 0;
static int					gLastRowOnRootMenu = -1;
static int					gKeyColumn = 0;
static int					gPadColumn = 0;
static float				gMenuColXs[MAX_MENU_COLS] = { 0, 190, 300, 430, 560 };
static float				gMenuRowYs[MAX_MENU_ROWS];
static float				gMenuFadeAlpha = 0;
static int					gMenuState = kMenuStateOff;
static int					gMenuPick = -1;
static ObjNode*				gMenuObjects[MAX_MENU_ROWS][MAX_MENU_COLS];

static bool					gMouseHoverValidRow = false;
static int					gMouseHoverColumn = -1;
static SDL_Cursor*			gHandCursor = NULL;
static SDL_Cursor*			gStandardCursor = NULL;

/****************************/
/*    MENU UTILITIES        */
/****************************/
#pragma mark - Utilities

static OGLColorRGBA TwinkleColor(void)
{
	float rf = .7f + RandomFloat() * .29f;
	return (OGLColorRGBA){rf, rf, rf, 1};
}

static OGLColorRGBA PulsateColor(float* time)
{
	*time += gFramesPerSecondFrac;
	float intensity = (1.0f + sinf(*time * 10.0f)) * 0.5f;
	return (OGLColorRGBA) {1,1,1,intensity};
}

static KeyBinding* GetBindingAtRow(int row)
{
	return &gGamePrefs.remappableKeys[gMenu[row].kb];
}

static const char* GetKeyBindingName(int row, int col)
{
	int16_t scancode = GetBindingAtRow(row)->key[col];

	switch (scancode)
	{
		case 0:
			return Localize(STR_UNBOUND_PLACEHOLDER);
		case SDL_SCANCODE_COMMA:				// on a US layout, it makes more sense to show "<" for camera left
			return "<";						
		case SDL_SCANCODE_PERIOD:				// on a US layout, it makes more sense to show ">" for camera right
			return ">";						
		default:
			return SDL_GetScancodeName(scancode);
	}
}

static const char* GetPadBindingName(int row, int col)
{
	KeyBinding* kb = GetBindingAtRow(row);

	switch (kb->gamepad[col].type)
	{
		case kInputTypeUnbound:
			return Localize(STR_UNBOUND_PLACEHOLDER);

		case kInputTypeButton:
			switch (kb->gamepad[col].id)
			{
				case SDL_GAMEPAD_BUTTON_INVALID:		return Localize(STR_UNBOUND_PLACEHOLDER);
				case SDL_GAMEPAD_BUTTON_SOUTH:			return "A";
				case SDL_GAMEPAD_BUTTON_EAST:			return "B";
				case SDL_GAMEPAD_BUTTON_WEST:			return "X";
				case SDL_GAMEPAD_BUTTON_NORTH:			return "Y";
				case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:	return "LB";
				case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:	return "RB";
				case SDL_GAMEPAD_BUTTON_LEFT_STICK:		return "Push LS";
				case SDL_GAMEPAD_BUTTON_RIGHT_STICK:	return "Push RS";
				default:
					return SDL_GetGamepadStringForButton(kb->gamepad[col].id);
			}
			break;

		case kInputTypeAxisPlus:
			switch (kb->gamepad[col].id)
			{
				case SDL_GAMEPAD_AXIS_LEFTX:			return "LS right";
				case SDL_GAMEPAD_AXIS_LEFTY:			return "LS down";
				case SDL_GAMEPAD_AXIS_RIGHTX:			return "RS right";
				case SDL_GAMEPAD_AXIS_RIGHTY:			return "RS down";
				case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:		return "LT";
				case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:	return "RT";
				default:
					return SDL_GetGamepadStringForAxis(kb->gamepad[col].id);
			}
			break;

		case kInputTypeAxisMinus:
			switch (kb->gamepad[col].id)
			{
				case SDL_GAMEPAD_AXIS_LEFTX:			return "LS left";
				case SDL_GAMEPAD_AXIS_LEFTY:			return "LS up";
				case SDL_GAMEPAD_AXIS_RIGHTX:			return "RS left";
				case SDL_GAMEPAD_AXIS_RIGHTY:			return "RS up";
				case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:		return "LT";
				case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:	return "RT";
				default:
					return SDL_GetGamepadStringForAxis(kb->gamepad[col].id);
			}
			break;

		default:
			return "???";
	}
}

static const char* GetMouseBindingName(int row)
{
	DECLARE_STATIC_WORKBUF(buf, bufSize);

	KeyBinding* kb = GetBindingAtRow(row);

	switch (kb->mouseButton)
	{
		case 0:							return Localize(STR_UNBOUND_PLACEHOLDER);
		case SDL_BUTTON_LEFT:			return Localize(STR_MOUSE_BUTTON_LEFT);
		case SDL_BUTTON_MIDDLE:			return Localize(STR_MOUSE_BUTTON_MIDDLE);
		case SDL_BUTTON_RIGHT:			return Localize(STR_MOUSE_BUTTON_RIGHT);
		case SDL_BUTTON_WHEELUP:		return Localize(STR_MOUSE_WHEEL_UP);
		case SDL_BUTTON_WHEELDOWN:		return Localize(STR_MOUSE_WHEEL_DOWN);
		default:
			SDL_snprintf(buf, bufSize, "%s %d", Localize(STR_BUTTON), kb->mouseButton);
			return buf;
	}
}

static bool IsMenuItemTypeSelectable(int type)
{
	switch (type)
	{
		case kMenuItem_Spacer:
		case kMenuItem_Label:
		case kMenuItem_Title:
		case kMenuItem_Subtitle:
			return false;

		default:
			return true;
	}
}

static void ReplaceMenuText(LocStrID originalTextInMenuDefinition, LocStrID newText)
{
	for (int i = 0; i < MAX_MENU_ROWS && gMenu[i].type != kMenuItem_END_SENTINEL; i++)
	{
		if (gMenu[i].text == originalTextInMenuDefinition)
		{
			MakeTextAtRowCol(Localize(newText), i, 0);
		}
	}
}

/****************************/
/*    MENU MOVE CALLS       */
/****************************/
#pragma mark - Move calls

static void MoveDarkenPane(ObjNode* node)
{
	node->ColorFilter.a = gMenuFadeAlpha * gMenuStyle->darkenPaneOpacity;
}

static void MoveGenericMenuItem(ObjNode* node)
{
	node->SpecialSweepTimer += gFramesPerSecondFrac * 5;

	if (node->SpecialSweepTimer < 0)
	{
		node->ColorFilter.a = 0;
	}
	else if (node->SpecialSweepTimer < 1)
	{
		node->ColorFilter.a *= node->SpecialSweepTimer;

		float xBackup = node->Coord.x;

		float p = (1.0f - node->SpecialSweepTimer);
		node->Coord.x -= p*p * 50.0f;
		UpdateObjectTransforms(node);

		node->Coord.x = xBackup;
	}
	else
	{
		UpdateObjectTransforms(node);
	}
}

static void MoveLabel(ObjNode* node)
{
	node->ColorFilter.a = gMenuFadeAlpha;
	MoveGenericMenuItem(node);
}

static void MoveAction(ObjNode* node)
{
	if (node->SpecialRow == gMenuRow)
		node->ColorFilter = TwinkleColor();
	else
		node->ColorFilter = gMenuStyle->inactiveColor;

	node->ColorFilter.a *= gMenuFadeAlpha;
	MoveGenericMenuItem(node);
}

static void MoveKeyBinding(ObjNode* node)
{
	if (node->SpecialRow == gMenuRow && node->SpecialCol == (gKeyColumn+1))
	{
		if (gMenuState == kMenuStateAwaitingKeyPress)
			node->ColorFilter = PulsateColor(&node->SpecialPulsateTimer);
		else
			node->ColorFilter = TwinkleColor();
	}
	else
		node->ColorFilter = gMenuStyle->inactiveColor;

	node->ColorFilter.a *= gMenuFadeAlpha;
	MoveGenericMenuItem(node);
}

static void MovePadBinding(ObjNode* node)
{
	if (node->SpecialRow == gMenuRow && node->SpecialCol == (gPadColumn+1))
	{
		if (gMenuState == kMenuStateAwaitingPadPress)
			node->ColorFilter = PulsateColor(&node->SpecialPulsateTimer);
		else
			node->ColorFilter = TwinkleColor();
	}
	else
		node->ColorFilter = gMenuStyle->inactiveColor;

	node->ColorFilter.a *= gMenuFadeAlpha;
	MoveGenericMenuItem(node);
}

static void MoveMouseBinding(ObjNode* node)
{
	if (node->SpecialRow == gMenuRow)
	{
		if (gMenuState == kMenuStateAwaitingMouseClick)
			node->ColorFilter = PulsateColor(&node->SpecialPulsateTimer);
		else
			node->ColorFilter = TwinkleColor();
	}
	else
		node->ColorFilter = gMenuStyle->inactiveColor;

	node->ColorFilter.a *= gMenuFadeAlpha;
	MoveGenericMenuItem(node);
}

static void MoveAsyncFadeOutAndDelete(ObjNode *theNode)
{
	theNode->ColorFilter.a -= gFramesPerSecondFrac * 3.0f;
	if (theNode->ColorFilter.a < 0.0f)
		DeleteObject(theNode);
}

/****************************/
/*    MENU CALLBACKS        */
/****************************/
#pragma mark - Callbacks

void MenuCallback_Back(void)
{
	MyFlushEvents();

	if (gMenu != gRootMenu)
	{
		LayOutMenu(gRootMenu);
	}
	else
	{
		gMenuState = kMenuStateFadeOut;
	}
}

/****************************/
/*    MENU NAVIGATION       */
/****************************/
#pragma mark - Menu navigation

static void NavigateSettingEntriesVertically(int delta)
{
	int browsed = 0;
	bool skipEntry = false;

	do
	{
		gMenuRow += delta;
		gMenuRow = PositiveModulo(gMenuRow, (unsigned int)gNumMenuEntries);

		skipEntry = !IsMenuItemTypeSelectable(gMenu[gMenuRow].type);

		if (browsed++ > gNumMenuEntries)
		{
			// no entries are selectable
			return;
		}
	} while (skipEntry);

	PlayEffect(kSfxNavigate);
	gMouseHoverValidRow = false;
}

static void NavigateSettingEntriesMouseHover(void)
{
	if (!gMouseMotionNow)
		return;

	float mxRaw, myRaw;
	SDL_GetMouseState(&mxRaw, &myRaw);

	// On macOS, the mouse position is relative to the window's "point size" on Retina screens.
	int windowW = 1;
	int windowH = 1;
	SDL_GetWindowSize(gSDLWindow, &windowW, &windowH);
	float dpiScaleX = (float) gGameWindowWidth / (float) windowW;		// gGameWindowWidth is in actual pixels
	float dpiScaleY = (float) gGameWindowHeight / (float) windowH;		// gGameWindowHeight is in actual pixels

	float mx = (mxRaw*dpiScaleX - gGameWindowWidth*0.5f) * g2DLogicalWidth / gGameWindowWidth;
	float my = (myRaw*dpiScaleY - gGameWindowHeight*0.5f) * g2DLogicalHeight / gGameWindowHeight;

	gMouseHoverValidRow = false;
	gMouseHoverColumn = -1;

	for (int row = 0; row < gNumMenuEntries; row++)
	{
		if (!IsMenuItemTypeSelectable(gMenu[row].type))
			continue;

		OGLRect fullExtents;
		fullExtents.top		= fullExtents.left	= 100000;
		fullExtents.bottom	= fullExtents.right	= -100000;

		for (int col = 0; col < MAX_MENU_COLS; col++)
		{
			ObjNode* textNode = gMenuObjects[row][col];
			if (!textNode)
				continue;

			OGLRect extents = TextMesh_GetExtents(textNode);
			if (extents.top		< fullExtents.top	) fullExtents.top		= extents.top;
			if (extents.left	< fullExtents.left	) fullExtents.left		= extents.left;
			if (extents.bottom	> fullExtents.bottom) fullExtents.bottom	= extents.bottom;
			if (extents.right	> fullExtents.right	) fullExtents.right		= extents.right;

			if (my >= extents.top
				&& my <= extents.bottom
				&& mx >= extents.left - 10
				&& mx <= extents.right + 10)
			{
				gMouseHoverColumn = col;
			}
		}

		if (my >= fullExtents.top &&
			my <= fullExtents.bottom &&
			mx >= fullExtents.left - 10 &&
			mx <= fullExtents.right + 10)
		{
			gMouseHoverValidRow = true;

			if (SDL_GetCursor() != gHandCursor)
			{
				SDL_SetCursor(gHandCursor);
			}

			if (gMenuRow != row)
			{
				gMenuRow = row;
//				PlayEffect(kSfxNavigate);
			}

			return;
		}
	}

	GAME_ASSERT(!gMouseHoverValidRow);		// if we got here, we're not hovering over anything

	if (SDL_GetCursor() != gStandardCursor)
		SDL_SetCursor(gStandardCursor);
}

static void NavigateAction(const MenuItem* entry)
{
	if (GetNewNeedState(kNeed_UIConfirm) || (gMouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_LEFT)))
	{
		if (entry->action.callback != MenuCallback_Back)
			PlayEffect(kSfxCycle);
		else if (gMenuStyle->playMenuChangeSounds)
			PlayEffect(kSfxMenuChange);

		if (entry->action.callback)
			entry->action.callback();
	}
}

static void NavigatePick(const MenuItem* entry)
{
	if (GetNewNeedState(kNeed_UIConfirm) || (gMouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_LEFT)))
	{
		gMenuPick = entry->pick;

		MenuCallback_Back();
	}
}

static void NavigateSubmenuButton(const MenuItem* entry)
{
	if (GetNewNeedState(kNeed_UIConfirm) || (gMouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_LEFT)))
	{
		if (gMenuStyle->playMenuChangeSounds)
			PlayEffect(kSfxMenuChange);

		MyFlushEvents();	// flush keypresses

		LayOutMenu(entry->submenu.menu);
	}
}

static void NavigateCycler(const MenuItem* entry)
{
	int delta = 0;

	if (GetNewNeedState(kNeed_UILeft)
		|| GetNewNeedState(kNeed_UIPrev)
		|| (gMouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_RIGHT)))
	{
		delta = -1;
	}
	else if (GetNewNeedState(kNeed_UIRight)
		|| GetNewNeedState(kNeed_UINext)
		|| GetNewNeedState(kNeed_UIConfirm)
		|| (gMouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_LEFT)))
	{
		delta = 1;
	}

	if (delta != 0)
	{
		PlayEffect_Parms(kSfxCycle, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE + (RandomFloat2() * 0x3000));

		if (entry->cycler.valuePtr && !entry->cycler.callbackSetsValue)
		{
			unsigned int value = (unsigned int)*entry->cycler.valuePtr;
			value = PositiveModulo(value + delta, entry->cycler.generateNumChoices? entry->cycler.generateNumChoices(): entry->cycler.numChoices);
			*entry->cycler.valuePtr = value;
		}

		if (entry->cycler.callback)
			entry->cycler.callback();

		LayOutCyclerValueText(gMenuRow);
	}
}

static void NavigateKeyBinding(const MenuItem* entry)
{
	if (gMouseHoverValidRow && (gMouseHoverColumn == 1 || gMouseHoverColumn == 2))
		gKeyColumn = gMouseHoverColumn - 1;

	if (GetNewNeedState(kNeed_UILeft) || GetNewNeedState(kNeed_UIPrev))
	{
		gKeyColumn = PositiveModulo(gKeyColumn - 1, KEYBINDING_MAX_KEYS);
		PlayEffect(kSfxNavigate);
		gMouseHoverValidRow = false;
		return;
	}

	if (GetNewNeedState(kNeed_UIRight) || GetNewNeedState(kNeed_UINext))
	{
		gKeyColumn = PositiveModulo(gKeyColumn + 1, KEYBINDING_MAX_KEYS);
		PlayEffect(kSfxNavigate);
		gMouseHoverValidRow = false;
		return;
	}

	// Past this point we must have a valid column
	if (gKeyColumn < 0 || gKeyColumn >= KEYBINDING_MAX_KEYS)
	{
		return;
	}

	if (GetNewNeedState(kNeed_UIDelete)
		|| (gMouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_MIDDLE)))
	{
		gGamePrefs.remappableKeys[entry->kb].key[gKeyColumn] = 0;

		PlayEffect(kSfxDelete);
		MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), gMenuRow, gKeyColumn+1);
		return;
	}

	if (GetNewKeyState(SDL_SCANCODE_RETURN)
		|| (gMouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_LEFT)))
	{
		gMenuState = kMenuStateAwaitingKeyPress;
		MakeTextAtRowCol(Localize(STR_PRESS), gMenuRow, gKeyColumn+1);

		// Change subtitle to help message
		ReplaceMenuText(STR_CONFIGURE_KEYBOARD_HELP, STR_CONFIGURE_KEYBOARD_HELP_CANCEL);

		return;
	}
}

static void NavigatePadBinding(const MenuItem* entry)
{
	if (gMouseHoverValidRow && (gMouseHoverColumn == 1 || gMouseHoverColumn == 2))
		gPadColumn = gMouseHoverColumn - 1;

	if (GetNewNeedState(kNeed_UILeft) || GetNewNeedState(kNeed_UIPrev))
	{
		gPadColumn = PositiveModulo(gPadColumn - 1, KEYBINDING_MAX_GAMEPAD_BUTTONS);
		PlayEffect(kSfxNavigate);
		gMouseHoverValidRow = false;
		return;
	}

	if (GetNewNeedState(kNeed_UIRight) || GetNewNeedState(kNeed_UINext))
	{
		gPadColumn = PositiveModulo(gPadColumn + 1, KEYBINDING_MAX_GAMEPAD_BUTTONS);
		PlayEffect(kSfxNavigate);
		gMouseHoverValidRow = false;
		return;
	}

	if (GetNewNeedState(kNeed_UIDelete)
		|| (gMouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_MIDDLE)))
	{
		gGamePrefs.remappableKeys[entry->kb].gamepad[gPadColumn].type = kInputTypeUnbound;

		PlayEffect(kSfxDelete);
		MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), gMenuRow, gPadColumn+1);
		return;
	}

	if (GetNewNeedState(kNeed_UIConfirm)
		|| (gMouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_LEFT)))
	{
		while (GetNeedState(kNeed_UIConfirm))
		{
			UpdateInput();
			SDL_Delay(30);
		}

		gMenuState = kMenuStateAwaitingPadPress;
		MakeTextAtRowCol(Localize(STR_PRESS), gMenuRow, gPadColumn+1);

		// Change subtitle to help message
		ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP, STR_CONFIGURE_GAMEPAD_HELP_CANCEL);

		return;
	}
}

static void NavigateMouseBinding(const MenuItem* entry)
{
	if (GetNewNeedState(kNeed_UIDelete)
		|| (gMouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_MIDDLE)))
	{
		gGamePrefs.remappableKeys[entry->kb].mouseButton = 0;

		PlayEffect(kSfxDelete);
		MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), gMenuRow, 1);
		return;
	}

	if (GetNewNeedState(kNeed_UIConfirm)
		|| (gMouseHoverValidRow && FlushMouseButtonPress(SDL_BUTTON_LEFT)))
	{
		while (GetNeedState(kNeed_UIConfirm))
		{
			UpdateInput();
			SDL_Delay(30);
		}

		gMenuState = kMenuStateAwaitingMouseClick;
		MakeTextAtRowCol(Localize(STR_CLICK), gMenuRow, 1);
		return;
	}
}

static void NavigateMenu(void)
{
	GAME_ASSERT(gMenuStyle->isInteractive);

	if (GetNewNeedState(kNeed_UIBack))
		MenuCallback_Back();

	if (GetNewNeedState(kNeed_UIUp))
		NavigateSettingEntriesVertically(-1);

	if (GetNewNeedState(kNeed_UIDown))
		NavigateSettingEntriesVertically(1);

	NavigateSettingEntriesMouseHover();

	const MenuItem* entry = &gMenu[gMenuRow];

	switch (entry->type)
	{
		case kMenuItem_Action:
			NavigateAction(entry);
			break;

		case kMenuItem_Pick:
			NavigatePick(entry);
			break;

		case kMenuItem_Submenu:
			NavigateSubmenuButton(entry);
			break;

		case kMenuItem_Cycler:
			NavigateCycler(entry);
			break;

		case kMenuItem_KeyBinding:
			NavigateKeyBinding(entry);
			break;

		case kMenuItem_PadBinding:
			NavigatePadBinding(entry);
			break;

		case kMenuItem_MouseBinding:
			NavigateMouseBinding(entry);

		default:
			//DoFatalAlert("Not supposed to be hovering on this menu item!");
			break;
	}
}

/****************************/
/* INPUT BINDING CALLBACKS  */
/****************************/
#pragma mark - Input binding callbacks

static void UnbindScancodeFromAllRemappableInputNeeds(int16_t sdlScancode)
{
	for (int row = 0; row < gNumMenuEntries; row++)
	{
		if (gMenu[row].type != kMenuItem_KeyBinding)
			continue;

		KeyBinding* binding = GetBindingAtRow(row);

		for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
		{
			if (binding->key[j] == sdlScancode)
			{
				binding->key[j] = 0;
				MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), row, j+1);
			}
		}
	}
}

static void UnbindPadButtonFromAllRemappableInputNeeds(int8_t type, int8_t id)
{
	for (int row = 0; row < gNumMenuEntries; row++)
	{
		if (gMenu[row].type != kMenuItem_PadBinding)
			continue;

		KeyBinding* binding = GetBindingAtRow(row);

		for (int j = 0; j < KEYBINDING_MAX_GAMEPAD_BUTTONS; j++)
		{
			if (binding->gamepad[j].type == type && binding->gamepad[j].id == id)
			{
				binding->gamepad[j].type = kInputTypeUnbound;
				binding->gamepad[j].id = 0;
				MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), row, j+1);
			}
		}
	}
}

static void UnbindMouseButtonFromAllRemappableInputNeeds(int8_t id)
{
	for (int row = 0; row < gNumMenuEntries; row++)
	{
		if (gMenu[row].type != kMenuItem_MouseBinding)
			continue;

		KeyBinding* binding = GetBindingAtRow(row);

		if (binding->mouseButton == id)
		{
			binding->mouseButton = 0;
			MakeTextAtRowCol(Localize(STR_UNBOUND_PLACEHOLDER), row, 1);
		}
	}
}

static void AwaitKeyPress(void)
{
	if (GetNewKeyState(SDL_SCANCODE_ESCAPE))
	{
		MakeTextAtRowCol(GetKeyBindingName(gMenuRow, gKeyColumn), gMenuRow, 1 + gKeyColumn);
		gMenuState = kMenuStateReady;
		PlayEffect(kSfxError);
		ReplaceMenuText(STR_CONFIGURE_KEYBOARD_HELP, STR_CONFIGURE_KEYBOARD_HELP);
		return;
	}

	KeyBinding* kb = GetBindingAtRow(gMenuRow);

	for (int16_t scancode = 0; scancode < SDL_SCANCODE_COUNT; scancode++)
	{
		if (GetNewKeyState(scancode))
		{
			UnbindScancodeFromAllRemappableInputNeeds(scancode);
			kb->key[gKeyColumn] = scancode;

			MakeTextAtRowCol(GetKeyBindingName(gMenuRow, gKeyColumn), gMenuRow, gKeyColumn+1);
			gMenuState = kMenuStateReady;
			PlayEffect(kSfxCycle);
			ReplaceMenuText(STR_CONFIGURE_KEYBOARD_HELP, STR_CONFIGURE_KEYBOARD_HELP);
			return;
		}
	}
}

static void AwaitPadPress(void)
{
	if (GetNewKeyState(SDL_SCANCODE_ESCAPE)
		|| SDL_GetGamepadButton(gSDLGamepad, SDL_GAMEPAD_BUTTON_START))
	{
		MakeTextAtRowCol(GetPadBindingName(gMenuRow, gPadColumn), gMenuRow, 1 + gPadColumn);
		gMenuState = kMenuStateReady;
		PlayEffect(kSfxError);
		ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP, STR_CONFIGURE_GAMEPAD_HELP);
		return;
	}

	KeyBinding* kb = GetBindingAtRow(gMenuRow);

	if (!gSDLGamepad)
		return;

	for (int8_t button = 0; button < SDL_GAMEPAD_BUTTON_COUNT; button++)
	{
		switch (button)
		{
			case SDL_GAMEPAD_BUTTON_DPAD_UP:			// prevent binding those
			case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
			case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
			case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
				continue;
		}

		if (SDL_GetGamepadButton(gSDLGamepad, button))
		{
			UnbindPadButtonFromAllRemappableInputNeeds(kInputTypeButton, button);
			kb->gamepad[gPadColumn].type = kInputTypeButton;
			kb->gamepad[gPadColumn].id = button;

			MakeTextAtRowCol(GetPadBindingName(gMenuRow, gPadColumn), gMenuRow, gPadColumn+1);
			gMenuState = kMenuStateReady;
			PlayEffect(kSfxCycle);
			ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP, STR_CONFIGURE_GAMEPAD_HELP);
			return;
		}
	}

	for (int8_t axis = 0; axis < SDL_GAMEPAD_AXIS_COUNT; axis++)
	{
		switch (axis)
		{
			case SDL_GAMEPAD_AXIS_LEFTX:				// prevent binding those
			case SDL_GAMEPAD_AXIS_LEFTY:
			case SDL_GAMEPAD_AXIS_RIGHTX:
			case SDL_GAMEPAD_AXIS_RIGHTY:
				continue;
		}

		int16_t axisValue = SDL_GetGamepadAxis(gSDLGamepad, axis);
		if (SDL_abs(axisValue) > kJoystickDeadZone_BindingThreshold)
		{
			int axisType = axisValue < 0 ? kInputTypeAxisMinus : kInputTypeAxisPlus;
			UnbindPadButtonFromAllRemappableInputNeeds(axisType, axis);
			kb->gamepad[gPadColumn].type = axisType;
			kb->gamepad[gPadColumn].id = axis;

			MakeTextAtRowCol(GetPadBindingName(gMenuRow, gPadColumn), gMenuRow, gPadColumn+1);
			gMenuState = kMenuStateReady;
			PlayEffect(kSfxCycle);
			ReplaceMenuText(STR_CONFIGURE_GAMEPAD_HELP_CANCEL, STR_CONFIGURE_GAMEPAD_HELP);
			return;
		}
	}
}

static void AwaitMouseClick(void)
{
	if (GetNewKeyState(SDL_SCANCODE_ESCAPE))
	{
		MakeTextAtRowCol(GetMouseBindingName(gMenuRow), gMenuRow, 1);
		gMenuState = kMenuStateReady;
		PlayEffect(kSfxError);
		return;
	}

	KeyBinding* kb = GetBindingAtRow(gMenuRow);

	for (int8_t mouseButton = 0; mouseButton < NUM_SUPPORTED_MOUSE_BUTTONS; mouseButton++)
	{
		if (FlushMouseButtonPress(mouseButton))
		{
			UnbindMouseButtonFromAllRemappableInputNeeds(mouseButton);
			kb->mouseButton = mouseButton;

			MakeTextAtRowCol(GetMouseBindingName(gMenuRow), gMenuRow, 1);
			gMenuState = kMenuStateReady;
			PlayEffect(kSfxCycle);
			return;
		}
	}
}

/****************************/
/*    PAGE LAYOUT           */
/****************************/
#pragma mark - Page Layout

static ObjNode* MakeDarkenPane(void)
{
	ObjNode* pane;

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.flags		= STATUS_BIT_NOZWRITES|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOFOG|STATUS_BIT_NOTEXTUREWRAP|
										STATUS_BIT_KEEPBACKFACES|STATUS_BIT_MOVEINPAUSE;
	gNewObjectDefinition.slot		= MENU_SLOT-1;
	gNewObjectDefinition.scale		= 1;
	gNewObjectDefinition.moveCall 	= nil;

	pane = MakeNewObject(&gNewObjectDefinition);
	pane->CustomDrawFunction = DrawDarkenPane;
	pane->ColorFilter = (OGLColorRGBA) {0, 0, 0, 0};
	pane->Scale.y = gMenuStyle->darkenPaneScaleY;
	pane->MoveCall = MoveDarkenPane;

	return pane;
}

static void DeleteAllText(void)
{
	for (int row = 0; row < MAX_MENU_ROWS; row++)
	{
		for (int col = 0; col < MAX_MENU_COLS; col++)
		{
			if (gMenuObjects[row][col])
			{
				DeleteObject(gMenuObjects[row][col]);
				gMenuObjects[row][col] = nil;
			}
		}
	}
}

static ObjNode* MakeTextAtRowCol(const char* text, int row, int col)
{
	ObjNode* node = gMenuObjects[row][col];

	if (node)
	{
		// Recycling existing text lets us keep the move call, color and specials
		TextMesh_Update(text, 0, node);
	}
	else
	{
		float startX = gMenuStyle->centeredText ? 0 : -170;
		gNewObjectDefinition.coord = (OGLPoint3D) { startX + gMenuColXs[col], gMenuRowYs[row], 0 };
		int alignment = gMenuStyle->centeredText? kTextMeshAlignCenter: kTextMeshAlignLeft;
		node = TextMesh_New(text, alignment, &gNewObjectDefinition);
		node->SpecialRow = row;
		node->SpecialCol = col;
		node->StatusBits |= STATUS_BIT_MOVEINPAUSE;
		gMenuObjects[row][col] = node;
	}

	if (!gMenuStyle->centeredText)
	{
		int paddedRightOff = ((gMenuColXs[col+1]-170) - node->Coord.x) / node->Scale.x;
		if (paddedRightOff > node->RightOff)
			node->RightOff = paddedRightOff;
	}

	if (gMenuStyle->uniformXExtent)
	{
		if (-gMenuStyle->uniformXExtent < node->LeftOff)
			node->LeftOff = -gMenuStyle->uniformXExtent;
		if (gMenuStyle->uniformXExtent > node->RightOff)
			node->RightOff = gMenuStyle->uniformXExtent;
	}

	return node;
}

static const float kMenuItemHeightMultipliers[kMenuItem_NUM_ITEM_TYPES] =
{
	[kMenuItem_END_SENTINEL] = 0.0f,
	[kMenuItem_Title]        = 1.4f,
	[kMenuItem_Subtitle]     = 0.8f,
	[kMenuItem_Label]        = 1,
	[kMenuItem_Action]       = 1,
	[kMenuItem_Submenu]      = 1,
	[kMenuItem_Spacer]       = 0.5f,
	[kMenuItem_Cycler]       = 1,
	[kMenuItem_Pick]         = 1,
	[kMenuItem_KeyBinding]   = 1,
	[kMenuItem_PadBinding]   = 1,
	[kMenuItem_MouseBinding] = 1,
};

static const char* GetMenuItemLabel(const MenuItem* entry)
{
	if (entry->rawText)
		return entry->rawText;
	else if (entry->generateText)
		return entry->generateText();
	else
		return Localize(entry->text);
}

static ObjNode* LayOutCyclerValueText(int row)
{
	DECLARE_WORKBUF(buf, bufSize);
	const MenuItem* entry = &gMenu[row];

	int numChoices = entry->cycler.numChoices;
	if (entry->cycler.generateNumChoices)
		numChoices = entry->cycler.generateNumChoices();

	if (numChoices <= 0)
		return NULL;

	Byte value = *entry->cycler.valuePtr;
	const char* valueText = NULL;

	if (entry->cycler.generateChoiceString)
	{
		valueText = entry->cycler.generateChoiceString(buf, bufSize, value);
	}
	else
	{
		GAME_ASSERT(numChoices <= MAX_MENU_CYCLER_CHOICES);
		valueText = Localize(entry->cycler.choices[value]);
	}

	ObjNode* node2 = MakeTextAtRowCol(valueText, row, 1);
	node2->MoveCall = MoveAction;
	return node2;
}

static void LayOutCycler(
		int row,
		float sweepFactor)
{
	DECLARE_WORKBUF(buf, bufSize);

	const MenuItem* entry = &gMenu[row];

	SDL_snprintf(buf, bufSize, "%s:", GetMenuItemLabel(entry));

	ObjNode* node1 = MakeTextAtRowCol(buf, row, 0);
	node1->MoveCall = MoveAction;
	node1->SpecialSweepTimer = sweepFactor;

	ObjNode* node2 = LayOutCyclerValueText(row);
	node2->SpecialSweepTimer = sweepFactor;
}

static void LayOutMenu(const MenuItem* menu)
{
	DECLARE_WORKBUF(buf, bufSize);

	bool enteringNewMenu = menu != gMenu;

	if (gMenu == gRootMenu)				// save position in root menu
		gLastRowOnRootMenu = gMenuRow;

	gMenu			= menu;
	gNumMenuEntries	= 0;
	gMenuPick		= -1;

	if (enteringNewMenu)
	{
		gMenuRow		= -1;

		if (menu == gRootMenu && gLastRowOnRootMenu >= 0)				// restore position in root menu
			gMenuRow = gLastRowOnRootMenu;
	}

	DeleteAllText();

	SDL_memset(&gNewObjectDefinition, 0, sizeof(gNewObjectDefinition));
	gNewObjectDefinition.scale		= gMenuStyle->standardScale;
	gNewObjectDefinition.slot		= MENU_SLOT;

	float totalHeight = 0;
	for (int row = 0; menu[row].type != kMenuItem_END_SENTINEL; row++)
	{
		totalHeight += kMenuItemHeightMultipliers[menu[row].type] * gMenuStyle->rowHeight;
	}

	float y = -totalHeight/2.0f;

	float sweepFactor = 0.0f;

	for (int row = 0; menu[row].type != kMenuItem_END_SENTINEL; row++)
	{
		gMenuRowYs[row] = y;

		const MenuItem* entry = &menu[row];

		gNewObjectDefinition.scale = gMenuStyle->standardScale;

		switch (entry->type)
		{
			case kMenuItem_Spacer:
				break;

			case kMenuItem_Title:
			{
				gNewObjectDefinition.scale = gMenuStyle->titleScale;
				ObjNode* label = MakeTextAtRowCol(GetMenuItemLabel(entry), row, 0);
				label->ColorFilter = gMenuStyle->titleColor;
				label->MoveCall = MoveLabel;
				label->SpecialSweepTimer = .5f;		// Title appears sooner than the rest
				break;
			}

			case kMenuItem_Subtitle:
			{
				gNewObjectDefinition.scale = gMenuStyle->subtitleScale;
				ObjNode* label = MakeTextAtRowCol(GetMenuItemLabel(entry), row, 0);
				label->ColorFilter = gMenuStyle->titleColor;
				label->MoveCall = MoveLabel;
				label->SpecialSweepTimer = .5f;		// Title appears sooner than the rest
				break;
			}

			case kMenuItem_Label:
			{
				ObjNode* label = MakeTextAtRowCol(GetMenuItemLabel(entry), row, 0);
				label->ColorFilter = gMenuStyle->inactiveColor;
				label->MoveCall = MoveLabel;
				label->SpecialSweepTimer = sweepFactor;
				break;
			}

			case kMenuItem_Action:
			case kMenuItem_Pick:
			case kMenuItem_Submenu:
			{
				ObjNode* node = MakeTextAtRowCol(GetMenuItemLabel(entry), row, 0);
				node->MoveCall = MoveAction;
				node->SpecialSweepTimer = sweepFactor;
				break;
			}

			case kMenuItem_Cycler:
			{
				LayOutCycler(row, sweepFactor);
				break;
			}

			case kMenuItem_KeyBinding:
			{
				SDL_snprintf(buf, bufSize, "%s:", Localize(STR_KEYBINDING_DESCRIPTION_0 + entry->kb));

				gNewObjectDefinition.scale = 0.6f;
				ObjNode* label = MakeTextAtRowCol(buf, row, 0);
				label->ColorFilter = gMenuStyle->inactiveColor2;
				label->MoveCall = MoveLabel;
				label->SpecialSweepTimer = sweepFactor;

				for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
				{
					ObjNode* keyNode = MakeTextAtRowCol(GetKeyBindingName(row, j), row, j + 1);
					keyNode->MoveCall = MoveKeyBinding;
					keyNode->SpecialSweepTimer = sweepFactor;
				}
				break;
			}

			case kMenuItem_PadBinding:
			{
				SDL_snprintf(buf, bufSize, "%s:", Localize(STR_KEYBINDING_DESCRIPTION_0 + entry->kb));

				ObjNode* label = MakeTextAtRowCol(buf, row, 0);
				label->ColorFilter = gMenuStyle->inactiveColor2;
				label->MoveCall = MoveLabel;
				label->SpecialSweepTimer = sweepFactor;

				for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
				{
					ObjNode* keyNode = MakeTextAtRowCol(GetPadBindingName(row, j), row, j + 1);
					keyNode->MoveCall = MovePadBinding;
					keyNode->SpecialSweepTimer = sweepFactor;
				}
				break;
			}

			case kMenuItem_MouseBinding:
			{
				SDL_snprintf(buf, bufSize, "%s:", Localize(STR_KEYBINDING_DESCRIPTION_0 + entry->kb));

				ObjNode* label = MakeTextAtRowCol(buf, row, 0);
				label->ColorFilter = gMenuStyle->inactiveColor2;
				label->MoveCall = MoveAction;
				label->SpecialSweepTimer = sweepFactor;

				ObjNode* keyNode = MakeTextAtRowCol(GetMouseBindingName(row), row, 1);
				keyNode->MoveCall = MoveMouseBinding;
				keyNode->SpecialSweepTimer = sweepFactor;
				break;
			}

			default:
				DoFatalAlert("Unsupported menu item type");
				break;
		}

		y += kMenuItemHeightMultipliers[entry->type] * gMenuStyle->rowHeight;

		if (entry->type != kMenuItem_Spacer)
			sweepFactor -= .2f;

		gNumMenuEntries++;
		GAME_ASSERT(gNumMenuEntries < MAX_MENU_ROWS);
	}

	if (gMenuRow < 0)
	{
		// Scroll down to first pickable entry
		gMenuRow = -1;
		NavigateSettingEntriesVertically(1);
	}
}

void LayoutCurrentMenuAgain(void)
{
	GAME_ASSERT(gMenu);
	LayOutMenu(gMenu);
}

int StartMenu(
		const MenuItem* menu,
		const MenuStyle* style,
		void (*updateRoutine)(void),
		void (*backgroundDrawRoutine)(void))
{
	bool cursorStateBeforeMenu = SDL_CursorVisible();
	gStandardCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
	gHandCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
	SDL_ShowCursor();

		/* INITIALIZE MENU STATE */

	SDL_memset(gMenuObjects, 0, sizeof(gMenuObjects));
	gMenuStyle			= style? style: &kDefaultMenuStyle;
	gRootMenu			= menu;
	gMenuState			= kMenuStateFadeIn;
	gMenuFadeAlpha		= 0;
	gMenuRow			= -1;
	gLastRowOnRootMenu	= -1;

		/* LAY OUT MENU COMPONENTS */

	ObjNode* pane = nil;

	if (gMenuStyle->darkenPane)
		pane = MakeDarkenPane();

	LayOutMenu(menu);

		/* SHOW IN ANIMATED LOOP */

	while (gMenuState != kMenuStateOff)
	{
		UpdateInput();

		if (gMenuStyle->startButtonExits && GetNewNeedState(kNeed_UIStart))
			gMenuState = kMenuStateFadeOut;

		switch (gMenuState)
		{
			case kMenuStateFadeIn:
				gMenuFadeAlpha += gFramesPerSecondFrac * gMenuStyle->fadeInSpeed;
				if (gMenuFadeAlpha >= 1.0f)
				{
					gMenuFadeAlpha = 1.0f;
					gMenuState = kMenuStateReady;
				}
				break;

			case kMenuStateFadeOut:
				if (gMenuStyle->asyncFadeOut)
				{
					gMenuState = kMenuStateOff;		// exit loop
				}
				else
				{
					gMenuFadeAlpha -= gFramesPerSecondFrac * 2.0f;
					if (gMenuFadeAlpha <= 0.0f)
					{
						gMenuFadeAlpha = 0.0f;
						gMenuState = kMenuStateOff;
					}
				}
				break;

			case kMenuStateReady:
				if (gMenuStyle->isInteractive)
				{
					NavigateMenu();
				}
				else if (UserWantsOut())
				{
					MenuCallback_Back();
				}
				break;

			case kMenuStateAwaitingKeyPress:
				AwaitKeyPress();
				break;

			case kMenuStateAwaitingPadPress:
				AwaitPadPress();
				break;

			case kMenuStateAwaitingMouseClick:
				AwaitMouseClick();
				break;

			default:
				break;
		}

			/* DRAW STUFF */

		CalcFramesPerSecond();
		MoveObjects();
		if (updateRoutine)
			updateRoutine();
		OGL_DrawScene(backgroundDrawRoutine);
	}


		/* CLEANUP */

	if (gMenuStyle->asyncFadeOut)
	{
		if (pane)
			pane->MoveCall = MoveAsyncFadeOutAndDelete;

		for (int row = 0; row < MAX_MENU_ROWS; row++)
		{
			for (int col = 0; col < MAX_MENU_COLS; col++)
			{
				if (gMenuObjects[row][col])
					gMenuObjects[row][col]->MoveCall = MoveAsyncFadeOutAndDelete;
			}
		}

		SDL_memset(gMenuObjects, 0, sizeof(gMenuObjects));
	}
	else
	{
		DeleteAllText();

		if (pane)
			DeleteObject(pane);
	}

	UpdateInput();
	MyFlushEvents();

	gMenu = nil;

	SDL_SetCursor(gStandardCursor);
	SDL_DestroyCursor(gStandardCursor);
	SDL_DestroyCursor(gHandCursor);
	gStandardCursor = nil;
	gHandCursor = nil;

	if (cursorStateBeforeMenu)
		SDL_ShowCursor();
	else
		SDL_HideCursor();

	return gMenuPick;
}
