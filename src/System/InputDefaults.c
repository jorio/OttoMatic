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
#define MB(x)		{kButton, SDL_BUTTON_##x}
#define MWPLUS()	{kAxisPlus, 0}
#define MWMINUS()	{kAxisMinus, 0}
#define MBNULL()	{kUnbound, 0}

// Controller button/axis
#define CB(x)		{kButton, SDL_CONTROLLER_BUTTON_##x}
#define CAPLUS(x)	{kAxisPlus, SDL_CONTROLLER_AXIS_##x}
#define CAMINUS(x)	{kAxisMinus, SDL_CONTROLLER_AXIS_##x}
#define CBNULL()	{kUnbound, 0}

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
[kNeed_Jump				] = { { SC(SPACE)	, 0				}, MBNULL()		, { CB(A)				, CBNULL()				} },
[kNeed_CameraMode		] = { { SC(TAB)		, 0				}, MBNULL()		, { CB(RIGHTSTICK)		, CBNULL()				} },
[kNeed_CameraLeft		] = { { SC(COMMA)	, 0				}, MBNULL()		, { CBNULL()			, CBNULL()				} },
[kNeed_CameraRight		] = { { SC(PERIOD)	, 0				}, MBNULL()		, { CBNULL()			, CBNULL()				} },

//[kNeed_ToggleMusic		] = { { SC(M)		, 0				}, MBNULL()		, { CBNULL()			, CBNULL()				} },
//[kNeed_ToggleFullscreen	] = { { SC(F11)		, 0				}, MBNULL()		, { CBNULL()			, CBNULL()				} },
//[kNeed_RaiseVolume		] = { { SC(EQUALS)	, 0				}, MBNULL()		, { CBNULL()			, CBNULL()				} },
//[kNeed_LowerVolume		] = { { SC(MINUS)	, 0				}, MBNULL()		, { CBNULL()			, CBNULL()				} },
[kNeed_UIUp				] = { { SC(UP)		, 0				}, MBNULL()		, { CB(DPAD_UP)			, CAMINUS(LEFTY)		} },
[kNeed_UIDown			] = { { SC(DOWN)	, 0				}, MBNULL()		, { CB(DPAD_DOWN)		, CAPLUS(LEFTY)			} },
[kNeed_UILeft			] = { { SC(LEFT)	, 0				}, MBNULL()		, { CB(DPAD_LEFT)		, CAMINUS(LEFTX)		} },
[kNeed_UIRight			] = { { SC(RIGHT)	, 0				}, MBNULL()		, { CB(DPAD_RIGHT)		, CAPLUS(LEFTX)			} },
[kNeed_UIPrev			] = { { 0			, 0				}, MBNULL()		, { CB(LEFTSHOULDER)	, CBNULL()				} },
[kNeed_UINext			] = { { 0			, 0				}, MBNULL()		, { CB(RIGHTSHOULDER)	, CBNULL()				} },
[kNeed_UIConfirm		] = { { SC(RETURN)	, SC(SPACE)		}, MBNULL()		, { CB(START)			, CB(A)					} },
[kNeed_UIBack			] = { { SC(ESCAPE)	, 0				}, MBNULL()		, { CB(BACK)			, CB(B)					} },
[kNeed_UIPause			] = { { SC(ESCAPE)	, 0				}, MBNULL()		, { CB(START)			, CBNULL()				} },
};
