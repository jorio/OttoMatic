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
#define MB(x)		{kInputTypeButton, SDL_BUTTON_##x}
#define MWPLUS()	{kInputTypeAxisPlus, 0}
#define MWMINUS()	{kInputTypeAxisMinus, 0}
#define MBNULL()	{kInputTypeUnbound, 0}

// Controller button/axis
#define CB(x)		{kInputTypeButton, SDL_CONTROLLER_BUTTON_##x}
#define CAPLUS(x)	{kInputTypeAxisPlus, SDL_CONTROLLER_AXIS_##x}
#define CAMINUS(x)	{kInputTypeAxisMinus, SDL_CONTROLLER_AXIS_##x}
#define CBNULL()	{kInputTypeUnbound, 0}

const KeyBinding kDefaultKeyBindings[NUM_CONTROL_NEEDS] =
{
//Need-------------------     Keys---------------------------  Mouse--------  Gamepad----------------------------------------
[kNeed_Forward			] = { { SC(UP)		, SC(W)			}, MBNULL()		, { CB(DPAD_UP)			, CBNULL()				} },
[kNeed_Backward			] = { { SC(DOWN)	, SC(S)			}, MBNULL()		, { CB(DPAD_DOWN)		, CBNULL()				} },
[kNeed_TurnLeft			] = { { SC(LEFT)	, SC(A)			}, MBNULL()		, { CB(DPAD_LEFT)		, CBNULL()				} },
[kNeed_TurnRight		] = { { SC(RIGHT)	, SC(D)			}, MBNULL()		, { CB(DPAD_RIGHT)		, CBNULL()				} },
[kNeed_PrevWeapon		] = { { SC(RSHIFT)	, 0				}, MWPLUS()		, { CB(LEFTSHOULDER)	, CBNULL()				} },
[kNeed_NextWeapon		] = { { SC(LSHIFT)	, 0				}, MWMINUS()	, { CB(RIGHTSHOULDER)	, CBNULL()				} },
[kNeed_Shoot			] = { { SC_SHOOT1	, SC_SHOOT2		}, MB(LEFT)		, { CB(X)				, CAPLUS(TRIGGERRIGHT)	} },
[kNeed_PunchPickup		] = { { SC(LALT)	, SC(RALT)		}, MB(MIDDLE)	, { CB(B)				, CBNULL()				} },
[kNeed_Jump				] = { { SC(SPACE)	, 0				}, MB(RIGHT)	, { CB(A)				, CBNULL()				} },
[kNeed_CameraMode		] = { { SC(TAB)		, 0				}, MBNULL()		, { CB(RIGHTSTICK)		, CBNULL()				} },
[kNeed_CameraLeft		] = { { SC(COMMA)	, 0				}, MBNULL()		, { CBNULL()			, CBNULL()				} },
[kNeed_CameraRight		] = { { SC(PERIOD)	, 0				}, MBNULL()		, { CBNULL()			, CBNULL()				} },

[kNeed_UIUp				] = { { SC(UP)		, 0				}, MBNULL()		, { CB(DPAD_UP)			, CAMINUS(LEFTY)		} },
[kNeed_UIDown			] = { { SC(DOWN)	, 0				}, MBNULL()		, { CB(DPAD_DOWN)		, CAPLUS(LEFTY)			} },
[kNeed_UILeft			] = { { SC(LEFT)	, 0				}, MBNULL()		, { CB(DPAD_LEFT)		, CAMINUS(LEFTX)		} },
[kNeed_UIRight			] = { { SC(RIGHT)	, 0				}, MBNULL()		, { CB(DPAD_RIGHT)		, CAPLUS(LEFTX)			} },
[kNeed_UIPrev			] = { { 0			, 0				}, MBNULL()		, { CB(LEFTSHOULDER)	, CBNULL()				} },
[kNeed_UINext			] = { { 0			, 0				}, MBNULL()		, { CB(RIGHTSHOULDER)	, CBNULL()				} },
[kNeed_UIConfirm		] = { { SC(RETURN)	, SC(SPACE)		}, MBNULL()		, { CB(A)				, CBNULL()				} },
[kNeed_UIStart			] = { { 0			, 0				}, MBNULL()		, { CB(START)			, CBNULL()				} },
[kNeed_UIBack			] = { { SC(ESCAPE)	, 0				}, MB(X1)		, { CB(B)				, CB(BACK)				} },
[kNeed_UIPause			] = { { SC(ESCAPE)	, 0				}, MBNULL()		, { CB(START)			, CBNULL()				} },

[kNeed_TextEntry_Left	] = { { SC(LEFT)	, 0				}, MBNULL()		, { CB(DPAD_LEFT)		, CB(LEFTSHOULDER)		} },
[kNeed_TextEntry_Left2	] = { { 0			, 0				}, MBNULL()		, { CAMINUS(LEFTX)		, 						} },
[kNeed_TextEntry_Right	] = { { SC(RIGHT)	, 0				}, MBNULL()		, { CB(DPAD_RIGHT)		, CB(RIGHTSHOULDER)		} },
[kNeed_TextEntry_Right2	] = { { 0			, 0				}, MBNULL()		, { CAPLUS(LEFTX)		, CBNULL()				} },
[kNeed_TextEntry_Home	] = { { SC(HOME)	, 0				}, MBNULL()		, { CBNULL()			, CBNULL()				} },
[kNeed_TextEntry_End	] = { { SC(HOME)	, 0				}, MBNULL()		, { CBNULL()			, CBNULL()				} },
[kNeed_TextEntry_Bksp	] = { { SC(BACKSPACE),0				}, MBNULL()		, { CB(X)				, CBNULL()				} },
[kNeed_TextEntry_Del	] = { { SC(DELETE)	, 0				}, MBNULL()		, { CBNULL()			, CBNULL()				} },
[kNeed_TextEntry_Done	] = { { SC(RETURN)	, SC(KP_ENTER)	}, MBNULL()		, { CB(START)			, CBNULL()				} },
[kNeed_TextEntry_CharPP	] = { { 0			, 0				}, MBNULL()		, { CB(A)				, CB(DPAD_UP)			} },
[kNeed_TextEntry_CharMM	] = { { 0			, 0				}, MBNULL()		, { CB(B)				, CB(DPAD_DOWN)			} },
[kNeed_TextEntry_Space	] = { { 0			, 0				}, MBNULL()		, { CB(Y)				, CBNULL()				} },
};
