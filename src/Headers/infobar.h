//
// infobar.h
//

#define NUM_UI_SCALE_LEVELS	8
#define DEFAULT_UI_SCALE_LEVEL 5

#define	HELP_BEACON_RANGE	500.0f


enum
{
	HELP_MESSAGE_NONE = -1,
	HELP_MESSAGE_POWPOD = 0,
	HELP_MESSAGE_PICKUPPOW,
	HELP_MESSAGE_JUMPJET,
	HELP_MESSAGE_SMASH,
	HELP_MESSAGE_NEEDJUMPFUEL,
	HELP_MESSAGE_HEALTHATOM,
	HELP_MESSAGE_JUMPJETATOM,
	HELP_MESSAGE_FUELATOM,
	HELP_MESSAGE_ENTERSHIP,
	HELP_MESSAGE_SAVEHUMAN,
	HELP_MESSAGE_CHECKPOINT,
	HELP_MESSAGE_SAUCEREMPTY,
	HELP_MESSAGE_LETGOMAGNET,
	HELP_MESSAGE_BEAMHUMANS,
	HELP_MESSAGE_FUELFORHUMANS,
	HELP_MESSAGE_SAUCERFULL,
	HELP_MESSAGE_PRESSJUMPTOLEAVE,
	HELP_MESSAGE_NOTENOUGHFUELTOLEAVE,
	HELP_MESSAGE_SLIMEBALLS,
	HELP_MESSAGE_NEEDELECTRICITY,
	HELP_MESSAGE_DOORSIDE,
	HELP_MESSAGE_INTOCANNON,

	NUM_HELP_MESSAGES
};


void InitInfobar(void);
void DrawInfobar(void);
void DisposeInfobar(void);
void DrawInfobarSprite(float x, float y, float size, short texNum);

void InitHelpMessages(void);
void DisplayHelpMessage(short messNum, float timer, Boolean overrideCurrent);
Boolean AddHelpBeacon(TerrainItemEntryType *itemPtr, long  x, long z);
void DisableHelpType(short messNum);

void DrawInfobarSprite2(float x, float y, float size, short group, short texNum);

void SetInfobarSpriteState(bool setOriginToCenterOfScreen);
