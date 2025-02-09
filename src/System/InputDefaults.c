#include "game.h"

#if __APPLE__
	#define SC_SHOOT1 SDL_SCANCODE_LGUI
	#define SC_SHOOT2 SDL_SCANCODE_RGUI
#else
	#define SC_SHOOT1 SDL_SCANCODE_LCTRL
	#define SC_SHOOT2 SDL_SCANCODE_RCTRL
#endif

// Scancode
#define SC(x)		SDL_SCANCODE_##x

// Mouse button/wheel
#define MB(x)		SDL_BUTTON_##x

// Controller button/axis
#define GB(x)		{kInputTypeButton, SDL_GAMEPAD_BUTTON_##x}
#define GAPLUS(x)	{kInputTypeAxisPlus, SDL_GAMEPAD_AXIS_##x}
#define GAMINUS(x)	{kInputTypeAxisMinus, SDL_GAMEPAD_AXIS_##x}
#define GBNULL()	{kInputTypeUnbound, 0}

const KeyBinding kDefaultKeyBindings[NUM_CONTROL_NEEDS] =
{
//Need-------------------     Keys---------------------------  Mouse--------  Gamepad----------------------------------------
[kNeed_Forward			] = { { SC(UP)		, SC(W)			}, 0			, { GB(DPAD_UP)			, GBNULL()				} },
[kNeed_Backward			] = { { SC(DOWN)	, SC(S)			}, 0			, { GB(DPAD_DOWN)		, GBNULL()				} },
[kNeed_TurnLeft			] = { { SC(LEFT)	, SC(A)			}, 0			, { GB(DPAD_LEFT)		, GBNULL()				} },
[kNeed_TurnRight		] = { { SC(RIGHT)	, SC(D)			}, 0			, { GB(DPAD_RIGHT)		, GBNULL()				} },
[kNeed_PrevWeapon		] = { { SC(RSHIFT),SC(LEFTBRACKET)	}, MB(WHEELUP)	, { GB(LEFT_SHOULDER)	, GBNULL()				} },
[kNeed_NextWeapon		] = { { SC(LSHIFT),SC(RIGHTBRACKET)	}, MB(WHEELDOWN), { GB(RIGHT_SHOULDER)	, GBNULL()				} },
[kNeed_Shoot			] = { { SC_SHOOT1	, SC_SHOOT2		}, MB(LEFT)		, { GB(WEST)			, GAPLUS(RIGHT_TRIGGER)	} },
[kNeed_PunchPickup		] = { { SC(LALT)	, SC(RALT)		}, MB(MIDDLE)	, { GB(EAST)			, GBNULL()				} },
[kNeed_Jump				] = { { SC(SPACE)	, 0				}, MB(RIGHT)	, { GB(SOUTH)			, GBNULL()				} },
[kNeed_CameraMode		] = { { SC(TAB)		, 0				}, 0			, { GB(RIGHT_STICK)		, GBNULL()				} },
[kNeed_CameraLeft		] = { { SC(COMMA)	, 0				}, 0			, { GBNULL()			, GBNULL()				} },
[kNeed_CameraRight		] = { { SC(PERIOD)	, 0				}, 0			, { GBNULL()			, GBNULL()				} },

[kNeed_UIUp				] = { { SC(UP)		, 0				}, 0			, { GB(DPAD_UP)			, GAMINUS(LEFTY)		} },
[kNeed_UIDown			] = { { SC(DOWN)	, 0				}, 0			, { GB(DPAD_DOWN)		, GAPLUS(LEFTY)			} },
[kNeed_UILeft			] = { { SC(LEFT)	, 0				}, 0			, { GB(DPAD_LEFT)		, GAMINUS(LEFTX)		} },
[kNeed_UIRight			] = { { SC(RIGHT)	, 0				}, 0			, { GB(DPAD_RIGHT)		, GAPLUS(LEFTX)			} },
[kNeed_UIPrev			] = { { 0			, 0				}, 0			, { GB(LEFT_SHOULDER)	, GBNULL()				} },
[kNeed_UINext			] = { { 0			, 0				}, 0			, { GB(RIGHT_SHOULDER)	, GBNULL()				} },
[kNeed_UIConfirm		] = { { SC(RETURN)	, SC(SPACE)		}, 0			, { GB(SOUTH)			, GBNULL()				} },
[kNeed_UIDelete			] = { { SC(DELETE)	, SC(X)			}, 0			, { GB(WEST)			, GBNULL()				} },
[kNeed_UIStart			] = { { 0			, 0				}, 0			, { GB(START)			, GBNULL()				} },
[kNeed_UIBack			] = { { SC(ESCAPE)	, SC(BACKSPACE)	}, MB(X1)		, { GB(EAST)			, GB(BACK)				} },
[kNeed_UIPause			] = { { SC(ESCAPE)	, 0				}, 0			, { GB(START)			, GBNULL()				} },

[kNeed_TextEntry_Left	] = { { SC(LEFT)	, 0				}, 0			, { GB(DPAD_LEFT)		, GB(LEFT_SHOULDER)		} },
[kNeed_TextEntry_Left2	] = { { 0			, 0				}, 0			, { GAMINUS(LEFTX)		, GBNULL()				} },
[kNeed_TextEntry_Right	] = { { SC(RIGHT)	, 0				}, 0			, { GB(DPAD_RIGHT)		, GB(RIGHT_SHOULDER)	} },
[kNeed_TextEntry_Right2	] = { { 0			, 0				}, 0			, { GAPLUS(LEFTX)		, GBNULL()				} },
[kNeed_TextEntry_Home	] = { { SC(HOME)	, 0				}, 0			, { GBNULL()			, GBNULL()				} },
[kNeed_TextEntry_End	] = { { SC(END)		, 0				}, 0			, { GBNULL()			, GBNULL()				} },
[kNeed_TextEntry_Bksp	] = { { SC(BACKSPACE),0				}, 0			, { GB(WEST)			, GBNULL()				} },
[kNeed_TextEntry_Del	] = { { SC(DELETE)	, 0				}, 0			, { GBNULL()			, GBNULL()				} },
[kNeed_TextEntry_Done	] = { { SC(RETURN)	, SC(KP_ENTER)	}, 0			, { GB(START)			, GBNULL()				} },
[kNeed_TextEntry_CharPP	] = { { 0			, 0				}, 0			, { GB(SOUTH)			, GB(DPAD_UP)			} },
[kNeed_TextEntry_CharMM	] = { { 0			, 0				}, 0			, { GB(EAST)			, GB(DPAD_DOWN)			} },
[kNeed_TextEntry_Space	] = { { 0			, 0				}, 0			, { GB(NORTH)			, GBNULL()				} },
};
