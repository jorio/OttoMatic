#pragma once

#define MAX_MENU_CYCLER_CHOICES		8

typedef enum
{
	kMenuItem_END_SENTINEL,
	kMenuItem_Title,
	kMenuItem_Subtitle,
	kMenuItem_Label,
	kMenuItem_Action,
	kMenuItem_Submenu,
	kMenuItem_Spacer,
	kMenuItem_Cycler,
	kMenuItem_Pick,
	kMenuItem_KeyBinding,
	kMenuItem_PadBinding,
	kMenuItem_MouseBinding,
	kMenuItem_NUM_ITEM_TYPES
} MenuItemType;

typedef struct MenuItem
{
	MenuItemType			type;

	LocStrID				text;
	const char*				rawText;
	const char*				(*generateText)(void);

	union
	{
		struct
		{
			void			(*callback)(void);
		} action;

		struct
		{
			const struct MenuItem* menu;
		} submenu;

		struct
		{
			Byte*			valuePtr;

			void			(*callback)(void);
			bool			callbackSetsValue;

			uint8_t			numChoices;
			LocStrID		choices[MAX_MENU_CYCLER_CHOICES];	// localizable strings

			uint8_t			(*generateNumChoices)(void);
			const char*		(*generateChoiceString)(char* buf, int bufSize, Byte value);
		} cycler;

		int					pick;

		int 				kb;
	};
} MenuItem;

typedef struct MenuStyle
{
	bool			darkenPane;
	float			fadeInSpeed;
	bool			asyncFadeOut;
	bool			centeredText;
	OGLColorRGBA	titleColor;
	OGLColorRGBA	inactiveColor;
	OGLColorRGBA	inactiveColor2;
	float			standardScale;
	float			titleScale;
	float			subtitleScale;
	float			rowHeight;
	float			uniformXExtent;
	bool			playMenuChangeSounds;
	float			darkenPaneScaleY;
	float			darkenPaneOpacity;
	bool			startButtonExits;
	bool			isInteractive;
} MenuStyle;

int StartMenu(
		const MenuItem* menu,
		const MenuStyle* style,
		void (*updateRoutine)(void),
		void (*backgroundDrawRoutine)(void));
void LayoutCurrentMenuAgain(void);
void MenuCallback_Back(void);
