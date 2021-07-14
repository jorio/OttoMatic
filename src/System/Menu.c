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

#define SpecialRow		Special[0]
#define SpecialCol		Special[1]

/****************************/
/*    CONSTANTS             */
/****************************/

#define MAX_MENU_ROWS	25
#define MAX_MENU_COLS	5

#define kSfxNavigate	EFFECT_WEAPONCLICK
#define kSfxMenuChange	EFFECT_MENUCHANGE
#define kSfxCycle		EFFECT_ATOMCHIME
#define kSfxError		EFFECT_NOJUMPJET
#define kSfxDelete		EFFECT_FLAREEXPLODE

const int16_t kJoystickDeadZone_BindingThreshold = (75 * 32767 / 100);

static const char* kUnboundCaption = "\xE1\xE1\xE1";

enum
{
	kMenuStateOff,
	kMenuStateFadeIn,
	kMenuStateReady,
	kMenuStateFadeOut,
	kMenuStateAwaitingKeyPress,
	kMenuStateAwaitingPadPress,
};

const MenuStyle kDefaultMenuStyle =
{
	.darkenPane			= true,
	.darkenPaneScaleY	= 480,
	.darkenPaneOpacity	= .7f,
	.fadeInSpeed		= 3.0f,
	.asyncFadeOut		= true,
	.centeredText		= false,
	.titleColor			= {1.0f, 1.0f, 0.7f, 1.0f},
	.inactiveColor		= {0.3f, 0.5f, 0.2f, 1.0f},
	.inactiveColor2		= {0.8f, 0.0f, 0.5f, 0.5f},
	.startPosition		= {-170, -160, 0},
	.standardScale		= .75f,
	.titleScale			= 1.25f,
	.rowHeight			= 13*1.5f,
	.playMenuChangeSounds	= true,
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
static float				gMenuColXs[MAX_MENU_COLS] = { 0, 170, 300, 330, 360 };
static float				gMenuRowYs[MAX_MENU_ROWS];
static float				gMenuFadeAlpha = 0;
static int					gMenuState = kMenuStateOff;
static int					gMenuPick = -1;
static ObjNode*				gMenuObjects[MAX_MENU_ROWS][MAX_MENU_COLS];

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
	return &gGamePrefs.keys[gMenu[row].kb];
}

static const char* GetKeyBindingName(int row, int col)
{
	int16_t scancode = GetBindingAtRow(row)->key[col];
	if (scancode == 0)
		return kUnboundCaption;
	else
		return SDL_GetScancodeName(scancode);
}

static const char* GetPadBindingName(int row, int col)
{
	KeyBinding* kb = GetBindingAtRow(row);

	switch (kb->gamepad[col].type)
	{
		case kInputTypeUnbound:
			return kUnboundCaption;

		case kInputTypeButton:
			switch (kb->gamepad[col].id)
			{
				case SDL_CONTROLLER_BUTTON_INVALID:			return kUnboundCaption;
				case SDL_CONTROLLER_BUTTON_A:				return "A";
				case SDL_CONTROLLER_BUTTON_B:				return "B";
				case SDL_CONTROLLER_BUTTON_X:				return "X";
				case SDL_CONTROLLER_BUTTON_Y:				return "Y";
				case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:	return "LB";
				case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:	return "RB";
				case SDL_CONTROLLER_BUTTON_LEFTSTICK:		return "Push LS";
				case SDL_CONTROLLER_BUTTON_RIGHTSTICK:		return "Push RS";
				default:
					return SDL_GameControllerGetStringForButton(kb->gamepad[col].id);
			}
			break;

		case kInputTypeAxisPlus:
			switch (kb->gamepad[col].id)
			{
				case SDL_CONTROLLER_AXIS_LEFTX:				return "LS right";
				case SDL_CONTROLLER_AXIS_LEFTY:				return "LS down";
				case SDL_CONTROLLER_AXIS_RIGHTX:			return "RS right";
				case SDL_CONTROLLER_AXIS_RIGHTY:			return "RS down";
				case SDL_CONTROLLER_AXIS_TRIGGERLEFT:		return "LT";
				case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:		return "RT";
				default:
					return SDL_GameControllerGetStringForAxis(kb->gamepad[col].id);
			}
			break;

		case kInputTypeAxisMinus:
			switch (kb->gamepad[col].id)
			{
				case SDL_CONTROLLER_AXIS_LEFTX:				return "LS left";
				case SDL_CONTROLLER_AXIS_LEFTY:				return "LS up";
				case SDL_CONTROLLER_AXIS_RIGHTX:			return "RS left";
				case SDL_CONTROLLER_AXIS_RIGHTY:			return "RS up";
				case SDL_CONTROLLER_AXIS_TRIGGERLEFT:		return "LT";
				case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:		return "RT";
				default:
					return SDL_GameControllerGetStringForAxis(kb->gamepad[col].id);
			}
			break;

		default:
			return "???";
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

static void MoveLabel(ObjNode* node)
{
	node->ColorFilter.a = gMenuFadeAlpha;
}

static void MoveAction(ObjNode* node)
{
	if (node->SpecialRow == gMenuRow)
		node->ColorFilter = TwinkleColor();
	else
		node->ColorFilter = gMenuStyle->inactiveColor;

	node->ColorFilter.a *= gMenuFadeAlpha;
}

static void MoveKeyBinding(ObjNode* node)
{
	if (node->SpecialRow == gMenuRow && node->SpecialCol == (gKeyColumn+1))
	{
		if (gMenuState == kMenuStateAwaitingKeyPress)
			node->ColorFilter = PulsateColor(&node->SpecialF[0]);
		else
			node->ColorFilter = TwinkleColor();
	}
	else
		node->ColorFilter = gMenuStyle->inactiveColor;

	node->ColorFilter.a *= gMenuFadeAlpha;
}

static void MovePadBinding(ObjNode* node)
{
	if (node->SpecialRow == gMenuRow && node->SpecialCol == (gPadColumn+1))
	{
		if (gMenuState == kMenuStateAwaitingPadPress)
			node->ColorFilter = PulsateColor(&node->SpecialF[0]);
		else
			node->ColorFilter = TwinkleColor();
	}
	else
		node->ColorFilter = gMenuStyle->inactiveColor;

	node->ColorFilter.a *= gMenuFadeAlpha;
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

		int kind = gMenu[gMenuRow].type;
		skipEntry = kind == kMenuItem_Spacer || kind == kMenuItem_Label || kind == kMenuItem_Title;

		if (browsed++ > gNumMenuEntries)
		{
			// no entries are selectable
			return;
		}
	} while (skipEntry);
	PlayEffect(kSfxNavigate);
}

static void NavigateAction(const MenuItem* entry)
{
	if (GetNewNeedState(kNeed_UIConfirm))
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
	if (GetNewNeedState(kNeed_UIConfirm))
	{
		gMenuPick = entry->pick;

		MenuCallback_Back();
	}
}

static void NavigateSubmenuButton(const MenuItem* entry)
{
	if (GetNewNeedState(kNeed_UIConfirm))
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

	if (GetNewNeedState(kNeed_UILeft) || GetNewNeedState(kNeed_UIPrev))
		delta = -1;
	else if (GetNewNeedState(kNeed_UIRight) || GetNewNeedState(kNeed_UINext) || GetNewNeedState(kNeed_UIConfirm))
		delta = 1;

	if (delta != 0)
	{
		PlayEffect_Parms(kSfxCycle, FULL_CHANNEL_VOLUME, FULL_CHANNEL_VOLUME, NORMAL_CHANNEL_RATE + (RandomFloat2() * 0x3000));

		if (entry->cycler.valuePtr && !entry->cycler.callbackSetsValue)
		{
			unsigned int value = (unsigned int)*entry->cycler.valuePtr;
			value = PositiveModulo(value + delta, entry->cycler.numChoices);
			*entry->cycler.valuePtr = value;
		}

		if (entry->cycler.callback)
			entry->cycler.callback();

		if (entry->cycler.numChoices > 0)
		{
			int newChoice = entry->cycler.choices[*entry->cycler.valuePtr];
			MakeTextAtRowCol(Localize(newChoice), gMenuRow, 1);
		}
	}
}

static void NavigateKeyBinding(const MenuItem* entry)
{
	if (GetNewNeedState(kNeed_UILeft) || GetNewNeedState(kNeed_UIPrev))
	{
		gKeyColumn = PositiveModulo(gKeyColumn - 1, KEYBINDING_MAX_KEYS);
		PlayEffect(kSfxNavigate);
		return;
	}

	if (GetNewNeedState(kNeed_UIRight) || GetNewNeedState(kNeed_UINext))
	{
		gKeyColumn = PositiveModulo(gKeyColumn + 1, KEYBINDING_MAX_KEYS);
		PlayEffect(kSfxNavigate);
		return;
	}

	if (GetNewKeyState(SDL_SCANCODE_DELETE) || GetNewKeyState(SDL_SCANCODE_BACKSPACE))
	{
		gGamePrefs.keys[entry->kb].key[gKeyColumn] = 0;
		PlayEffect(kSfxDelete);
		MakeTextAtRowCol(kUnboundCaption, gMenuRow, gKeyColumn+1);
		return;
	}

	if (GetNewKeyState(SDL_SCANCODE_RETURN))
	{
		gMenuState = kMenuStateAwaitingKeyPress;
		MakeTextAtRowCol(Localize(STR_PRESS), gMenuRow, gKeyColumn+1);
		return;
	}
}

static void NavigatePadBinding(const MenuItem* entry)
{
	if (GetNewNeedState(kNeed_UILeft))
	{
		gPadColumn = PositiveModulo(gPadColumn - 1, KEYBINDING_MAX_GAMEPAD_BUTTONS);
		PlayEffect(kSfxNavigate);
		return;
	}

	if (GetNewNeedState(kNeed_UIRight))
	{
		gPadColumn = PositiveModulo(gPadColumn + 1, KEYBINDING_MAX_GAMEPAD_BUTTONS);
		PlayEffect(kSfxNavigate);
		return;
	}

	if (GetNewKeyState(SDL_SCANCODE_DELETE) || GetNewKeyState(SDL_SCANCODE_BACKSPACE))
	{
		gGamePrefs.keys[entry->kb].gamepad[gPadColumn].type = kInputTypeUnbound;
		PlayEffect(kSfxDelete);
		MakeTextAtRowCol(kUnboundCaption, gMenuRow, gPadColumn+1);
		return;
	}

	if (GetNewNeedState(kNeed_UIConfirm))
	{
		while (GetNeedState(kNeed_UIConfirm))
		{
			UpdateInput();
			SDL_Delay(30);
		}

		gMenuState = kMenuStateAwaitingPadPress;
		MakeTextAtRowCol(Localize(STR_PRESS), gMenuRow, gPadColumn+1);
		return;
	}
}

static void NavigateMenu(void)
{
	if (GetNewNeedState(kNeed_UIBack))
		MenuCallback_Back();

	if (GetNewNeedState(kNeed_UIUp))
		NavigateSettingEntriesVertically(-1);

	if (GetNewNeedState(kNeed_UIDown))
		NavigateSettingEntriesVertically(1);

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
		const MenuItem* entry = &gMenu[row];

		if (entry->type != kMenuItem_KeyBinding)
			continue;

		KeyBinding* binding = GetBindingAtRow(row);

		for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
		{
			if (binding->key[j] == sdlScancode)
			{
				binding->key[j] = 0;
				MakeTextAtRowCol(kUnboundCaption, row, j+1);
			}
		}
	}
}

static void UnbindPadButtonFromAllRemappableInputNeeds(int8_t type, int8_t id)
{
	for (int row = 0; row < gNumMenuEntries; row++)
	{
		const MenuItem* entry = &gMenu[row];

		if (entry->type != kMenuItem_PadBinding)
			continue;

		KeyBinding* binding = GetBindingAtRow(row);

		for (int j = 0; j < KEYBINDING_MAX_GAMEPAD_BUTTONS; j++)
		{
			if (binding->gamepad[j].type == type && binding->gamepad[j].id == id)
			{
				binding->gamepad[j].type = kInputTypeUnbound;
				binding->gamepad[j].id = 0;
				MakeTextAtRowCol(kUnboundCaption, row, j+1);
			}
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
		return;
	}

	KeyBinding* kb = GetBindingAtRow(gMenuRow);

	for (int16_t scancode = 0; scancode < SDL_NUM_SCANCODES; scancode++)
	{
		if (GetNewKeyState(scancode))
		{
			UnbindScancodeFromAllRemappableInputNeeds(scancode);
			kb->key[gKeyColumn] = scancode;
			MakeTextAtRowCol(GetKeyBindingName(gMenuRow, gKeyColumn), gMenuRow, gKeyColumn+1);
			gMenuState = kMenuStateReady;
			PlayEffect(kSfxCycle);
			return;
		}
	}
}

static void AwaitPadPress(void)
{
	if (GetNewKeyState(SDL_SCANCODE_ESCAPE))
	{
		MakeTextAtRowCol(GetPadBindingName(gMenuRow, gPadColumn), gMenuRow, 1 + gPadColumn);
		gMenuState = kMenuStateReady;
		PlayEffect(kSfxError);
		return;
	}

	KeyBinding* kb = GetBindingAtRow(gMenuRow);


	if (!gSDLController)
		return;

	if (SDL_GameControllerGetButton(gSDLController, SDL_CONTROLLER_BUTTON_START))
	{
		MenuCallback_Back();
		return;
	}

	for (int8_t button = 0; button < SDL_CONTROLLER_BUTTON_MAX; button++)
	{
		switch (button)
		{
			case SDL_CONTROLLER_BUTTON_DPAD_UP:			// prevent binding those
			case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
				continue;
		}

		if (SDL_GameControllerGetButton(gSDLController, button))
		{
			UnbindPadButtonFromAllRemappableInputNeeds(kInputTypeButton, button);
			kb->gamepad[gPadColumn].type = kInputTypeButton;
			kb->gamepad[gPadColumn].id = button;
			MakeTextAtRowCol(GetPadBindingName(gMenuRow, gPadColumn), gMenuRow, gPadColumn+1);
			gMenuState = kMenuStateReady;
			PlayEffect(kSfxCycle);
			return;
		}
	}

	for (int8_t axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; axis++)
	{
		switch (axis)
		{
			case SDL_CONTROLLER_AXIS_LEFTX:				// prevent binding those
			case SDL_CONTROLLER_AXIS_LEFTY:
			case SDL_CONTROLLER_AXIS_RIGHTX:
			case SDL_CONTROLLER_AXIS_RIGHTY:
				continue;
		}

		int16_t axisValue = SDL_GameControllerGetAxis(gSDLController, axis);
		if (abs(axisValue) > kJoystickDeadZone_BindingThreshold)
		{
			int axisType = axisValue < 0 ? kInputTypeAxisMinus : kInputTypeAxisPlus;
			UnbindPadButtonFromAllRemappableInputNeeds(axisType, axis);
			kb->gamepad[gPadColumn].type = axisType;
			kb->gamepad[gPadColumn].id = axis;
			MakeTextAtRowCol(GetPadBindingName(gMenuRow, gPadColumn), gMenuRow, gPadColumn+1);
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
	gNewObjectDefinition.slot		= SLOT_OF_DUMB+100-1;
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
		gNewObjectDefinition.coord = (OGLPoint3D) { gMenuStyle->startPosition.x + gMenuColXs[col], gMenuRowYs[row], 0 };
		int alignment = gMenuStyle->centeredText? kTextMeshAlignCenter: kTextMeshAlignLeft;
		node = TextMesh_New(text, alignment, &gNewObjectDefinition);
		node->SpecialRow = row;
		node->SpecialCol = col;
		node->StatusBits |= STATUS_BIT_MOVEINPAUSE;
		gMenuObjects[row][col] = node;
	}

	return node;
}

static void LayOutMenu(const MenuItem* menu)
{
	char buf[256];

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

	memset(&gNewObjectDefinition, 0, sizeof(gNewObjectDefinition));
	gNewObjectDefinition.scale		= gMenuStyle->standardScale;
	gNewObjectDefinition.slot		= SLOT_OF_DUMB+100;

	float y = gMenuStyle->startPosition.y;

	for (int row = 0; menu[row].type != kMenuItem_END_SENTINEL; row++)
	{
		gMenuRowYs[row] = y;

		const MenuItem* entry = &menu[row];

		const char* text = entry->rawText;
		if (entry->text >= 0)
			text = Localize(entry->text);

		gNewObjectDefinition.scale = gMenuStyle->standardScale;

		switch (entry->type)
		{
			case kMenuItem_Spacer:
				break;

			case kMenuItem_Title:
			{
				gNewObjectDefinition.scale = gMenuStyle->titleScale;
				ObjNode* label = MakeTextAtRowCol(text, row, 0);
				label->ColorFilter = gMenuStyle->titleColor;
				label->MoveCall = MoveLabel;
				y += gMenuStyle->rowHeight;
				break;
			}

			case kMenuItem_Label:
			{
				ObjNode* label = MakeTextAtRowCol(text, row, 0);
				label->ColorFilter = gMenuStyle->inactiveColor;
				label->MoveCall = MoveLabel;
				break;
			}

			case kMenuItem_Action:
			case kMenuItem_Pick:
			case kMenuItem_Submenu:
			{
				ObjNode* node = MakeTextAtRowCol(text, row, 0);
				node->MoveCall = MoveAction;
				break;
			}

			case kMenuItem_Cycler:
			{
				snprintf(buf, sizeof(buf), "%s:", text);

				ObjNode* node1 = MakeTextAtRowCol(buf, row, 0);
				node1->MoveCall = MoveAction;
				if (entry->cycler.numChoices > 0)
				{
					const char* choiceCaption = Localize(entry->cycler.choices[*entry->cycler.valuePtr]);
					ObjNode* node2 = MakeTextAtRowCol(choiceCaption, row, 1);
					node2->MoveCall = MoveAction;
				}
				break;
			}

			case kMenuItem_KeyBinding:
			{
				snprintf(buf, sizeof(buf), "%s:", Localize(STR_KEYBINDING_DESCRIPTION_0 + entry->kb));

				gNewObjectDefinition.scale = 0.6f;
				ObjNode* label = MakeTextAtRowCol(buf, row, 0);
				label->ColorFilter = gMenuStyle->inactiveColor2;

				for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
				{
					ObjNode* keyNode = MakeTextAtRowCol(GetKeyBindingName(row, j), row, j + 1);
					keyNode->MoveCall = MoveKeyBinding;
				}
				break;
			}

			case kMenuItem_PadBinding:
			{
				snprintf(buf, sizeof(buf), "%s:", Localize(STR_KEYBINDING_DESCRIPTION_0 + entry->kb));

				ObjNode* label = MakeTextAtRowCol(buf, row, 0);
				label->ColorFilter = gMenuStyle->inactiveColor2;

				for (int j = 0; j < KEYBINDING_MAX_KEYS; j++)
				{
					ObjNode* keyNode = MakeTextAtRowCol(GetPadBindingName(row, j), row, j + 1);
					keyNode->MoveCall = MovePadBinding;
				}
				break;
			}

			default:
				DoFatalAlert("Unsupported menu item type");
				break;
		}

		if (entry->type == kMenuItem_Spacer)
			y += gMenuStyle->rowHeight / 2;
		else
			y += gMenuStyle->rowHeight;

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
		void (*backgroundDrawRoutine)(OGLSetupOutputType *))
{
		/* INITIALIZE MENU STATE */

	memset(gMenuObjects, 0, sizeof(gMenuObjects));
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
				NavigateMenu();
				break;

			case kMenuStateAwaitingKeyPress:
				AwaitKeyPress();
				break;

			case kMenuStateAwaitingPadPress:
				AwaitPadPress();
				break;

			default:
				break;
		}

			/* DRAW STUFF */

		CalcFramesPerSecond();
		MoveObjects();
		if (updateRoutine)
			updateRoutine();
		OGL_DrawScene(gGameViewInfoPtr, backgroundDrawRoutine);
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

		memset(gMenuObjects, 0, sizeof(gMenuObjects));
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

	return gMenuPick;
}
