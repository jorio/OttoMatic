#pragma once

typedef enum
{
	kMenuItem_END_SENTINEL,
	kMenuItem_Title,
	kMenuItem_Label,
	kMenuItem_Action,
	kMenuItem_Submenu,
	kMenuItem_Spacer,
	kMenuItem_Cycler,
	kMenuItem_Pick,
	kMenuItem_KeyBinding,
	kMenuItem_PadBinding,
} MenuItemType;

typedef struct MenuItem
{
	MenuItemType			type;
	int						text;
	const char*				rawText;

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
			void			(*callback)(void);
			Byte*			valuePtr;
			unsigned int	numChoices;
			int				choices[8];
			bool			callbackSetsValue;
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
	OGLPoint3D		startPosition;
	float			standardScale;
	float			titleScale;
	float			rowHeight;
	bool			playMenuChangeSounds;
	float			darkenPaneScaleY;
	float			darkenPaneOpacity;
} MenuStyle;

int StartMenu(
		const MenuItem* menu,
		const MenuStyle* style,
		void (*updateRoutine)(void),
		void (*backgroundDrawRoutine)(OGLSetupOutputType *));
void LayoutCurrentMenuAgain(void);
void MenuCallback_Back(void);
