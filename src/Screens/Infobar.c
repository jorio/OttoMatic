/****************************/
/*   	INFOBAR.C		    */
/* (c)2001 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"
//#include <AGL/aglmacro.h> // srcport rm

extern	float					gCurrentAspectRatio,gGlobalTransparency,gFramesPerSecondFrac;
extern	PlayerInfoType			gPlayerInfo;
extern	int						gLevelNum;
extern	FSSpec					gDataSpec;
extern	long					gTerrainUnitWidth,gTerrainUnitDepth;
extern	OGLColorRGB				gGlobalColorFilter;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	PrefsType			gGamePrefs;
extern	u_long					gGlobalMaterialFlags;
extern	SpriteType	*gSpriteGroupList[];
extern	AGLContext		gAGLContext;
extern	short					gBeamModeSelected,gBeamMode;
extern	float					gBeamCharge;
extern	short	gNumHumansRescuedOfType[NUM_HUMAN_TYPES];

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawInfobarSprite_Rotated(float x, float y, float size, short texNum, float rot, const OGLSetupOutputType *setupInfo);
static void DrawInfobarSprite_Centered(float x, float y, float size, short texNum, const OGLSetupOutputType *setupInfo);
static void DrawInfobarSprite_Scaled(float x, float y, float scaleX, float scaleY, short texNum, const OGLSetupOutputType *setupInfo);
static void Infobar_DrawWeaponInventory(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawGirders(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawHealth(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawLives(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawFuel(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawJumpJet(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawBeams(const OGLSetupOutputType *setupInfo);
static void Infobar_DrawHumans(const OGLSetupOutputType *setupInfo);

static void UpdateHelpMessage(const OGLSetupOutputType *setupInfo);
static void MoveHelpBeacon(ObjNode *theNode);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	HEALTH_X		23.0f
#define	HEALTH_Y		19.0f
#define	HEALTH_SIZE		50.0f

#define FUEL_SIZE		HEALTH_SIZE
#define	FUEL_X			(640.0f-FUEL_SIZE-HEALTH_X)
#define	FUEL_Y			HEALTH_Y

#define JUMP_SIZE		HEALTH_SIZE
#define	JUMP_X			(HEALTH_X + HEALTH_SIZE +10)
#define	JUMP_Y			HEALTH_Y


#define	WEAPON_X			(JUMP_X+JUMP_SIZE + 10)
#define	WEAPON_Y			52
#define	WEAPON_HIDDEN_Y		-80
#define	WEAPON_INACTIVE_Y	-30
#define	WEAPON_SIZE			50
#define	WEAPON_FRAME_SIZE	(WEAPON_SIZE * 3/2)

#define	BEAM_CUP_X			190.0f
#define	BEAM_CUP_Y			20.0f
#define	BEAM_CUP_SCALE		20.0f
#define	BEAM_CUP_GLOW_SCALE (BEAM_CUP_SCALE * 3.0f)

#define	BEAM_SPARKLE_X		(BEAM_CUP_X-BEAM_CUP_GLOW_SCALE/2-10.0f)

#define	BEAM_SCALE			BEAM_CUP_SCALE
#define	BEAM_X				(BEAM_CUP_X + BEAM_CUP_SCALE/2)
#define	BEAM_Y				(BEAM_CUP_Y - BEAM_SCALE/2)

#define	HUMAN_SCALE			25.0f
#define	HUMAN_X				(630.0f -HUMAN_SCALE/2)
#define	HUMAN_Y				150.0f
#define	HUMAN_SPACING		(HUMAN_SCALE * 2.8f)

#define	LETTER_SPACING		9.0f

#define	HELP_Y				440.0f
#define	BLANK_LETTER_SPACER	.6f


/******************* INFOBAR SOBJTYPES *************************/

enum
{
	INFOBAR_SObjType_PulseGun,
	INFOBAR_SObjType_FreezeGun,
	INFOBAR_SObjType_FlameGun,
	INFOBAR_SObjType_Fist,
	INFOBAR_SObjType_SuperNova,
	INFOBAR_SObjType_GrowVial,
	INFOBAR_SObjType_FlareGun,
	INFOBAR_SObjType_DartPOW,

	INFOBAR_SObjType_PulseGunGlow,
	INFOBAR_SObjType_FreezeGlow,
	INFOBAR_SObjType_FlameGlow,
	INFOBAR_SObjType_FistGlow,
	INFOBAR_SObjType_SuperNovaGlow,
	INFOBAR_SObjType_GrowVialGlow,
	INFOBAR_SObjType_FlareGlow,
	INFOBAR_SObjType_DartGlow,

	INFOBAR_SObjType_LeftGirder,
	INFOBAR_SObjType_RightGirder,
	INFOBAR_SObjType_OttoHead,
	INFOBAR_SObjType_HealthMeter,
	INFOBAR_SObjType_FuelMeter,
	INFOBAR_SObjType_JumpJetMeter,
	INFOBAR_SObjType_MeterBack,
	INFOBAR_SObjType_WeaponDisplay,
	INFOBAR_SObjType_RocketIcon,

	INFOBAR_SObjType_0,
	INFOBAR_SObjType_1,
	INFOBAR_SObjType_2,
	INFOBAR_SObjType_3,
	INFOBAR_SObjType_4,
	INFOBAR_SObjType_5,
	INFOBAR_SObjType_6,
	INFOBAR_SObjType_7,
	INFOBAR_SObjType_8,
	INFOBAR_SObjType_9,

	INFOBAR_SObjType_BeamCupLeft,
	INFOBAR_SObjType_BeamCupRight,
	INFOBAR_SObjType_TeleportBeam,
	INFOBAR_SObjType_DestructoBeam,

	INFOBAR_SObjType_HumanFrame,
	INFOBAR_SObjType_Farmer,
	INFOBAR_SObjType_BeeWoman,
	INFOBAR_SObjType_Scientist,
	INFOBAR_SObjType_SkirtLady
};




/*********************/
/*    VARIABLES      */
/*********************/

static 	short			gHealthWarningChannel = -1;
static	float			gHealthWarningWobble = 0;

Boolean			gHideInfobar = false;
static float	gHealthOccilate = 0;
static float	gHealthMeterRot = 0, gFuelMeterRot = 0, gJumpJetMeterRot=0;

static float	gWeaponY[MAX_INVENTORY_SLOTS];

short	gDisplayedHelpMessage;
static Str255	gHelpString;
static float	gHelpMessageAlpha,gHelpMessageX;
static float	gHelpMessageTimer;

#define	MessageNum	Special[0]

Boolean	gHelpMessageDisabled[NUM_HELP_MESSAGES];

static	float	gHumanFrameX[NUM_HUMAN_TYPES];


/********************* INIT INFOBAR ****************************/
//
// Called at beginning of level
//

void InitInfobar(OGLSetupOutputType *setupInfo)
{
int	i;

#pragma unused(setupInfo)

	gDisplayedHelpMessage = HELP_MESSAGE_NONE;


	for (i = 0; i < NUM_HUMAN_TYPES; i++)
		gHumanFrameX[i] = 700.0f;

	gHealthOccilate = 0;
	gHealthMeterRot = gFuelMeterRot = gJumpJetMeterRot = 0;
	gHealthWarningChannel = -1;

		/* SET GLOWING */

	BlendASprite(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_HealthMeter);
	BlendASprite(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_FuelMeter);
	BlendASprite(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_JumpJetMeter);

	BlendASprite(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_PulseGunGlow);
	BlendASprite(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_FreezeGlow);
	BlendASprite(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_FlameGlow);
	BlendASprite(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_FistGlow);
	BlendASprite(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_SuperNovaGlow);
	BlendASprite(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_GrowVialGlow);
	BlendASprite(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_FlareGlow);
	BlendASprite(SPRITE_GROUP_INFOBAR, INFOBAR_SObjType_DartGlow);


	for (i = 0; i< MAX_INVENTORY_SLOTS; i++)
		gWeaponY[i] = WEAPON_HIDDEN_Y;


}


/****************** DISPOSE INFOBAR **********************/

void DisposeInfobar(void)
{

}


/***************** SET INFOBAR SPRITE STATE *******************/

void SetInfobarSpriteState(void)
{
AGLContext agl_ctx = gAGLContext;

	OGL_DisableLighting();
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);								// no z-buffer

	gGlobalMaterialFlags = BG3D_MATERIALFLAG_CLAMP_V|BG3D_MATERIALFLAG_CLAMP_U;	// clamp all textures


			/* INIT MATRICES */

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 640, 480, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


/********************** DRAW INFOBAR ****************************/

void DrawInfobar(OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;

	if (gHideInfobar)
		return;

		/************/
		/* SET TAGS */
		/************/

	OGL_PushState();

	if (setupInfo->useFog)
		glDisable(GL_FOG);

	SetInfobarSpriteState();



		/***************/
		/* DRAW THINGS */
		/***************/


		/* DRAW STUFF */

	Infobar_DrawGirders(setupInfo);
	Infobar_DrawLives(setupInfo);
	Infobar_DrawHealth(setupInfo);
	Infobar_DrawFuel(setupInfo);
	Infobar_DrawHumans(setupInfo);

	if (gLevelNum != LEVEL_NUM_SAUCER)
	{
		Infobar_DrawWeaponInventory(setupInfo);
		Infobar_DrawJumpJet(setupInfo);
	}
	else
	{
		gGlobalTransparency = .3f;					// dim these in saucer mode
		Infobar_DrawJumpJet(setupInfo);
		gGlobalTransparency = 1.0f;

		Infobar_DrawBeams(setupInfo);
	}

	UpdateHelpMessage(setupInfo);

			/***********/
			/* CLEANUP */
			/***********/

	OGL_PopState();
	gGlobalMaterialFlags = 0;
}


/******************** DRAW INFOBAR SPRITE **********************/

void DrawInfobarSprite(float x, float y, float size, short texNum, const OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;
MOMaterialObject	*mo;
float				aspect;

		/* ACTIVATE THE MATERIAL */

	mo = gSpriteGroupList[SPRITE_GROUP_INFOBAR][texNum].materialObject;
	MO_DrawMaterial(mo, setupInfo);

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(x, 		y);
	glTexCoord2f(1,1);	glVertex2f(x+size, 	y);
	glTexCoord2f(1,0);	glVertex2f(x+size,  y+(size*aspect));
	glTexCoord2f(0,0);	glVertex2f(x,		y+(size*aspect));
	glEnd();
}

/******************** DRAW INFOBAR SPRITE: CENTERED **********************/
//
// Coords are for center of sprite, not upper left
//

static void DrawInfobarSprite_Centered(float x, float y, float size, short texNum, const OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;
MOMaterialObject	*mo;
float				aspect;

		/* ACTIVATE THE MATERIAL */

	mo = gSpriteGroupList[SPRITE_GROUP_INFOBAR][texNum].materialObject;
	MO_DrawMaterial(mo, setupInfo);

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

	x -= size*.5f;
	y -= (size*aspect)*.5f;

			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(x, 		y);
	glTexCoord2f(1,1);	glVertex2f(x+size, 	y);
	glTexCoord2f(1,0);	glVertex2f(x+size,  y+(size*aspect));
	glTexCoord2f(0,0);	glVertex2f(x,		y+(size*aspect));
	glEnd();
}



/******************** DRAW INFOBAR SPRITE 2**********************/
//
// This version lets user pass in the sprite group
//

void DrawInfobarSprite2(float x, float y, float size, short group, short texNum, const OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;
MOMaterialObject	*mo;
float				aspect;

		/* ACTIVATE THE MATERIAL */

	mo = gSpriteGroupList[group][texNum].materialObject;
	MO_DrawMaterial(mo, setupInfo);

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(x, 		y);
	glTexCoord2f(1,1);	glVertex2f(x+size, 	y);
	glTexCoord2f(1,0);	glVertex2f(x+size,  y+(size*aspect));
	glTexCoord2f(0,0);	glVertex2f(x,		y+(size*aspect));
	glEnd();
}



/******************** DRAW INFOBAR SPRITE: ROTATED **********************/

static void DrawInfobarSprite_Rotated(float x, float y, float size, short texNum, float rot, const OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;
MOMaterialObject	*mo;
float				aspect, xoff, yoff;
OGLPoint2D			p[4];
OGLMatrix3x3		m;

		/* ACTIVATE THE MATERIAL */

	mo = gSpriteGroupList[SPRITE_GROUP_INFOBAR][texNum].materialObject;
	MO_DrawMaterial(mo, setupInfo);

				/* SET COORDS */

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

	xoff = size*.5f;
	yoff = (size*aspect)*.5f;

	p[0].x = -xoff;		p[0].y = -yoff;
	p[1].x = xoff;		p[1].y = -yoff;
	p[2].x = xoff;		p[2].y = yoff;
	p[3].x = -xoff;		p[3].y = yoff;

	OGLMatrix3x3_SetRotate(&m, rot);
	OGLPoint2D_TransformArray(p, &m, p, 4);

	xoff += x;
	yoff += y;

			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(p[0].x + xoff, p[0].y + yoff);
	glTexCoord2f(1,1);	glVertex2f(p[1].x + xoff, p[1].y + yoff);
	glTexCoord2f(1,0);	glVertex2f(p[2].x + xoff, p[2].y + yoff);
	glTexCoord2f(0,0);	glVertex2f(p[3].x + xoff, p[3].y + yoff);
	glEnd();
}


/******************** DRAW INFOBAR SPRITE: SCALED **********************/

static void DrawInfobarSprite_Scaled(float x, float y, float scaleX, float scaleY, short texNum, const OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;
MOMaterialObject	*mo;
float				aspect;

		/* ACTIVATE THE MATERIAL */

	mo = gSpriteGroupList[SPRITE_GROUP_INFOBAR][texNum].materialObject;
	MO_DrawMaterial(mo, setupInfo);

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex2f(x, 			y);
	glTexCoord2f(1,1);	glVertex2f(x+scaleX, 	y);
	glTexCoord2f(1,0);	glVertex2f(x+scaleX, 	y+(scaleY*aspect));
	glTexCoord2f(0,0);	glVertex2f(x,			y+(scaleY*aspect));
	glEnd();
}


#pragma mark -

#if 0
/********************** DRAW ATOMS *************************/

static void Infobar_DrawAtoms(const OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;
Str32	s;
int		q,n;
float	x;

				/* DRAW ATOM ICON */

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);								// make glow
	DrawInfobarSprite(ATOM_X, ATOM_Y, ATOM_SIZE, INFOBAR_SObjType_Atom, setupInfo);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


		/* DRAW QUANTITY NUMBER */

	q = gPlayerInfo.numAtoms;
	NumToString(q, s);

	x = ATOM_X + ATOM_SIZE;
	for (n = 1; n <= s[0]; n++)
	{
		DrawSprite(SPRITE_GROUP_FONT, s[n]-'0',x, ATOM_Y + 10.0f, 20.0f, 0, 0, setupInfo);
		x += 17.0f;
	}

}
#endif


/********************** DRAW WEAPON INVENTORY *************************/

static void Infobar_DrawWeaponInventory(const OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;
float	x,y,fps = gFramesPerSecondFrac;
float	tx;
int		i,n;
short	type;
Str32	s;

	x = WEAPON_X;

	for (i = 0; i < MAX_INVENTORY_SLOTS; i++)
	{
		type = gPlayerInfo.weaponInventory[i].type;										// get weapon type

		if ((type != NO_INVENTORY_HERE)	|| (gWeaponY[i] > WEAPON_HIDDEN_Y))						// see if valid
		{
					/* CALC Y */

			if (type == NO_INVENTORY_HERE)
				y = WEAPON_HIDDEN_Y;
			else
			if (gPlayerInfo.currentWeaponType == type)
				y = 0;
			else
				y = WEAPON_INACTIVE_Y;

			if (gWeaponY[i] > y)
			{
				gWeaponY[i] -= fps * WEAPON_FRAME_SIZE * 3.0f;
				if (gWeaponY[i] < y)
					gWeaponY[i] = y;
			}
			else
			if (gWeaponY[i] < y)
			{
				gWeaponY[i] += fps * WEAPON_FRAME_SIZE * 3.0f;
				if (gWeaponY[i] > y)
					gWeaponY[i] = y;
			}

			y = gWeaponY[i];


					/* DRAW FRAME */

			DrawInfobarSprite(x, y, WEAPON_FRAME_SIZE, INFOBAR_SObjType_WeaponDisplay, setupInfo);


			if (type != NO_INVENTORY_HERE)
			{

								/* DRAW GLOW */

				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				DrawInfobarSprite_Centered(x + WEAPON_FRAME_SIZE/2, y+WEAPON_Y, WEAPON_SIZE * (1.0f + RandomFloat()*.15f), INFOBAR_SObjType_PulseGunGlow + type, setupInfo);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

								/* DRAW ICON */

				DrawInfobarSprite_Centered(x + WEAPON_FRAME_SIZE/2, y+WEAPON_Y, WEAPON_SIZE, INFOBAR_SObjType_PulseGun + type, setupInfo);


					/* DRAW QUANTITY NUMBER */

				if (type != WEAPON_TYPE_FIST)									// dont draw fist quantity since unlimited
				{
					NumToString(gPlayerInfo.weaponInventory[i].quantity, s);

					tx = x + 50;

					for (n = 1; n <= s[0]; n++)
					{
						DrawInfobarSprite(tx, y+60, 8, INFOBAR_SObjType_0 + s[n]-'0', setupInfo);
						tx += 5.1f;
					}
				}
			}
		}

			/* NEXT SLOT */

		x += WEAPON_FRAME_SIZE - 5;
	}

}



/********************** DRAW GIRDERS *************************/

static void Infobar_DrawGirders(const OGLSetupOutputType *setupInfo)
{

	DrawInfobarSprite(0,0, 100, INFOBAR_SObjType_LeftGirder, setupInfo);

	DrawInfobarSprite(640-100,0, 100, INFOBAR_SObjType_RightGirder, setupInfo);
}


/********************** DRAW LIVES *************************/

static void Infobar_DrawLives(const OGLSetupOutputType *setupInfo)
{
short	i;

	for (i = 0; i < gPlayerInfo.lives; i++)
		DrawInfobarSprite(i * 30, 450, 40, INFOBAR_SObjType_OttoHead, setupInfo);

}


/********************** DRAW HUMANS *************************/

static void Infobar_DrawHumans(const OGLSetupOutputType *setupInfo)
{
Str15	s;
int		i,n;
float	tx,y,x;
static const float scales[NUM_HUMAN_TYPES] =
{
	HUMAN_SCALE * 2.0f,					// farmer
	HUMAN_SCALE,
	HUMAN_SCALE,
	HUMAN_SCALE
};

	for (i = 0; i < NUM_HUMAN_TYPES; i++)
	{
		if (gNumHumansRescuedOfType[i] == 0)					// skip if none of these rescued
			continue;

				/* SCROLL INTO POSITION */

		x = gHumanFrameX[i];									// get current scroll X
		if (x > HUMAN_X)										// see if need to move it
		{
			x -= gFramesPerSecondFrac * 150.0f;
			if (x < HUMAN_X)
				x = HUMAN_X;
			gHumanFrameX[i] = x;
		}

		y = HUMAN_Y + HUMAN_SPACING * i;


					/* DRAW FRAME */

		DrawInfobarSprite_Centered(x, y, HUMAN_SCALE * 3.0f, INFOBAR_SObjType_HumanFrame, setupInfo);



					/* DRAW HUMAN ICON */

		x -= HUMAN_SCALE * .4f;
		DrawInfobarSprite_Centered(x, y, scales[i], INFOBAR_SObjType_Farmer+i, setupInfo);


					/* DRAW QUANTITY */

		NumToString(gNumHumansRescuedOfType[i], s);

		tx = x - (HUMAN_SCALE * 3/4);

		for (n = 1; n <= s[0]; n++)
		{
			DrawInfobarSprite(tx, y-HUMAN_SCALE, HUMAN_SCALE/3, INFOBAR_SObjType_0 + s[n]-'0', setupInfo);
			tx += HUMAN_SCALE/5;
		}

	}
}




/********************** DRAW HEALTH *************************/

static void Infobar_DrawHealth(const OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;
float	x,y,size,xoff;
float	n, fps = gFramesPerSecondFrac;
Boolean	warningOn;

	n = gPlayerInfo.health;										// get health
	if ((n <= .2) && (n > 0.0f))								// see if low enough for warning
		warningOn = true;
	else
		warningOn = false;

			/* DRAW BACK */

	if (warningOn)												// see if wobble
	{
		xoff = sin(gHealthWarningWobble) * 4.0f;
		gHealthWarningWobble += fps * 25.0f;
	}
	else
		xoff = 0;

	if (n >= 1.0f)
	{
		gHealthMeterRot += fps * 2.0f;
		DrawInfobarSprite_Rotated(HEALTH_X + xoff, HEALTH_Y, HEALTH_SIZE, INFOBAR_SObjType_MeterBack, gHealthMeterRot, setupInfo);
	}
	else
		DrawInfobarSprite(HEALTH_X + xoff, HEALTH_Y, HEALTH_SIZE, INFOBAR_SObjType_MeterBack, setupInfo);


			/* DRAW METER */

	gHealthOccilate += fps * 10.0f;
	size = HEALTH_SIZE * (1.0f + sin(gHealthOccilate)*.04f) * n;
	x = HEALTH_X + (HEALTH_SIZE - size) * .5f;
	y = HEALTH_Y + (HEALTH_SIZE - size) * .5f;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);								// make glow
	DrawInfobarSprite(x+xoff, y, size, INFOBAR_SObjType_HealthMeter, setupInfo);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



		/* WHILE WE'RE HERE, UPDATE WARNING BEEP */


	if (warningOn)
	{
		if (gHealthWarningChannel == -1)
			gHealthWarningChannel = PlayEffect(EFFECT_HEALTHWARNING);
	}
	else									// make sure warning is OFF since not in danger
	{
		if (gHealthWarningChannel != -1)
			StopAChannel(&gHealthWarningChannel);
	}
}



/********************** DRAW FUEL *************************/

static void Infobar_DrawFuel(const OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;
float	x,y,size;
float	n, fps = gFramesPerSecondFrac;


			/* DRAW ROCKET ICON */

	DrawInfobarSprite(FUEL_X-34, 15, 30, INFOBAR_SObjType_RocketIcon, setupInfo);


	n = gPlayerInfo.fuel;										// get health

			/* DRAW BACK */

	if (n >= 1.0f)
	{
		gFuelMeterRot += fps * 2.0f;
		DrawInfobarSprite_Rotated(FUEL_X, FUEL_Y, FUEL_SIZE, INFOBAR_SObjType_MeterBack, gFuelMeterRot, setupInfo);
	}
	else
		DrawInfobarSprite(FUEL_X, FUEL_Y, FUEL_SIZE, INFOBAR_SObjType_MeterBack, setupInfo);


	size = FUEL_SIZE * (1.0f + cos(gHealthOccilate)*.04f) * n;
	x = FUEL_X + (FUEL_SIZE - size) * .5f;
	y = FUEL_Y + (FUEL_SIZE - size) * .5f;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);								// make glow
	DrawInfobarSprite(x, y, size, INFOBAR_SObjType_FuelMeter, setupInfo);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


/********************** DRAW JUMPJET *************************/

static void Infobar_DrawJumpJet(const OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;
float	x,y,size;
float	n, fps = gFramesPerSecondFrac;

	n = gPlayerInfo.jumpJet;										// get jj

			/* DRAW BACK */

	if (n >= 1.0f)
	{
		gJumpJetMeterRot -= fps * 2.0f;
		DrawInfobarSprite_Rotated(JUMP_X, JUMP_Y, JUMP_SIZE, INFOBAR_SObjType_MeterBack, gJumpJetMeterRot, setupInfo);
	}
	else
		DrawInfobarSprite(JUMP_X, JUMP_Y, JUMP_SIZE, INFOBAR_SObjType_MeterBack, setupInfo);


	size = JUMP_SIZE * (1.0f + cos(gHealthOccilate)*.04f) * n;
	x = JUMP_X + (JUMP_SIZE - size) * .5f;
	y = JUMP_Y + (JUMP_SIZE - size) * .5f;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);								// make glow
	DrawInfobarSprite(x, y, size, INFOBAR_SObjType_JumpJetMeter, setupInfo);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}



/********************** DRAW BEAMS *************************/

static void Infobar_DrawBeams(const OGLSetupOutputType *setupInfo)
{
AGLContext agl_ctx = gAGLContext;
float	q,q2,y;


	glDisable(GL_TEXTURE_2D);

			/**********************/
			/* DRAW TELEPORT BEAM */
			/**********************/

				/* DRAW BEAM */

	if (gBeamMode == BEAM_MODE_TELEPORT)
		q = gBeamCharge * 250.0f;
	else
		q = 0;

	q += 5.0f;														// give a tad extra so something is always showing

	gGlobalTransparency = .8f + RandomFloat() * .2f;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);								// make glow
	DrawInfobarSprite_Scaled(BEAM_X, BEAM_Y, q, BEAM_SCALE, INFOBAR_SObjType_TeleportBeam, setupInfo);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


				/* DRAW CUPS */

	gGlobalTransparency = 1.0f;
	q += BEAM_CUP_SCALE;
	DrawInfobarSprite_Centered(BEAM_CUP_X, BEAM_CUP_Y, BEAM_CUP_SCALE, INFOBAR_SObjType_BeamCupLeft, setupInfo);
	DrawInfobarSprite_Centered(BEAM_CUP_X+q, BEAM_CUP_Y, BEAM_CUP_SCALE, INFOBAR_SObjType_BeamCupRight, setupInfo);


			/***********************/
			/* DRAW DESTRUCTO BEAM */
			/***********************/

				/* DRAW BEAM */

	if (gBeamMode == BEAM_MODE_DESTRUCTO)
		q2 = gBeamCharge * 250.0f;
	else
		q2 = 0;

	q2 += 5.0f;

	gGlobalTransparency = .8f + RandomFloat() * .2f;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);								// make glow
	DrawInfobarSprite_Scaled(BEAM_X, BEAM_Y+BEAM_CUP_SCALE, q2, BEAM_SCALE, INFOBAR_SObjType_DestructoBeam, setupInfo);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				/* DRAW CUPS */

	gGlobalTransparency = 1.0f;
	q2 += BEAM_CUP_SCALE;
	DrawInfobarSprite_Centered(BEAM_CUP_X, BEAM_CUP_Y + BEAM_CUP_SCALE, BEAM_CUP_SCALE, INFOBAR_SObjType_BeamCupLeft, setupInfo);
	DrawInfobarSprite_Centered(BEAM_CUP_X+q2,  BEAM_CUP_Y + BEAM_CUP_SCALE, BEAM_CUP_SCALE, INFOBAR_SObjType_BeamCupRight, setupInfo);


			/*******************************/
			/* DRAW GLOW FOR SELECTED BEAM */
			/*******************************/

	y = BEAM_CUP_Y - (BEAM_CUP_GLOW_SCALE/2);
	if (gBeamModeSelected == BEAM_MODE_DESTRUCTO)
	{
		y += BEAM_CUP_SCALE;
		q = q2;
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);								// make glow
	gGlobalTransparency = .8f + RandomFloat() * .2f;
	DrawInfobarSprite2(BEAM_SPARKLE_X, y, BEAM_CUP_SCALE*3, SPRITE_GROUP_PARTICLES, PARTICLE_SObjType_WhiteSpark4, setupInfo);
	gGlobalTransparency = .8f + RandomFloat() * .2f;
	DrawInfobarSprite2(BEAM_CUP_X+q-BEAM_CUP_GLOW_SCALE/2+10, y, BEAM_CUP_SCALE*3, SPRITE_GROUP_PARTICLES, PARTICLE_SObjType_WhiteSpark4, setupInfo);


	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gGlobalTransparency = 1.0f;
}


#pragma mark -

/****************** INIT HELP MESSAGES *************************/

void InitHelpMessages(void)
{
int	i;

	for (i = 0; i < NUM_HELP_MESSAGES; i++)					// no messages disabled
		gHelpMessageDisabled[i] = false;

}


/**************************** DISPLAY HELP MESSAGE *******************************/

void DisplayHelpMessage(short messNum, float timer, Boolean overrideCurrent)
{
short	i,n,s;
float	size;

	if (gHelpMessageDisabled[messNum])						// dont do it if this message is disabled
		return;

	if (messNum == gDisplayedHelpMessage)					// if already showing this one then dont need to do anything more
	{
		gHelpMessageTimer = timer;							// keep timer going
		return;
	}

	if ((!overrideCurrent) && (gDisplayedHelpMessage != HELP_MESSAGE_NONE))	// if can't override current then bail
		return;

	gDisplayedHelpMessage = messNum;
	gHelpMessageAlpha = 0;
	gHelpMessageTimer = timer;								// set timer


			/* GET THE STRING TEXT TO DISPLAY */

	GetIndString(gHelpString, 1000 + gGamePrefs.language, messNum+1);


			/* CALC STRING PARAMETERS */

	size = 0;
	n = gHelpString[0];
	for (i = 1; i <= n; i++)
	{
		s = CharToSprite(gHelpString[i]);
		if (s == -1)
			size += LETTER_SPACING * BLANK_LETTER_SPACER;		// blank spaces are shorter
		else
			size += LETTER_SPACING;		// some day do proportional spacing here
	}

	gHelpMessageX = 320.0f - (size * .5f);	// center the message
}


/******************** UPDATE HELP MESSAGE *****************************/

static void UpdateHelpMessage(const OGLSetupOutputType *setupInfo)
{
float	fps,x,y;
short	i,n,texNum;
AGLContext agl_ctx = gAGLContext;

	if (gDisplayedHelpMessage == HELP_MESSAGE_NONE)
		return;

	fps = gFramesPerSecondFrac;

		/* FADE THE TEXT */

	gHelpMessageTimer -= fps;
	if (gHelpMessageTimer > 0.0f)				// fade in
	{
		gHelpMessageAlpha += fps;
		if (gHelpMessageAlpha > .99f)
			gHelpMessageAlpha = .99f;
	}
	else
	{
		gHelpMessageAlpha -= fps;
		if (gHelpMessageAlpha <= 0.0f)
		{
			gDisplayedHelpMessage = HELP_MESSAGE_NONE;
			return;
		}
	}

			/*******************/
			/* DRAW THE BORDER */
			/*******************/

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	SetColor4f(.2,.2,.2,gHelpMessageAlpha * .5f);
	glBegin(GL_QUADS);
	glVertex2f(0, 	HELP_Y + 16);
	glVertex2f(640, HELP_Y + 16);
	glVertex2f(640, HELP_Y);
	glVertex2f(0,	HELP_Y);
	glEnd();
	SetColor4f(1,1,1,1);


			/*******************/
			/* DRAW THE STRING */
			/*******************/

	x = gHelpMessageX;
	y = HELP_Y;

	n = gHelpString[0];										// get str len

	gGlobalTransparency = gHelpMessageAlpha;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);						// make glow

	for (i = 1; i <= n; i++)
	{
				/* CONVERT LETTER INTO SPRITE # */

		texNum = CharToSprite(gHelpString[i]);
		if (texNum == -1)
		{
			x += LETTER_SPACING * BLANK_LETTER_SPACER;
			continue;
		}

			/* DRAW THE LETTER */

		DrawInfobarSprite2(x, y, LETTER_SPACING * 1.8f, SPRITE_GROUP_FONT, texNum, setupInfo);
		x += LETTER_SPACING;
	}


			/* CLEANUP */

	gGlobalTransparency = 1.0f;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


/***************** CHAR TO SPRITE **********************/

enum
{
	MACROMAN_AUML	= '\x80',
	MACROMAN_ARING	= '\x81',
	MACROMAN_CCEDIL	= '\x82',
	MACROMAN_EACUTE	= '\x83',
	MACROMAN_NTILDE	= '\x84',
	MACROMAN_OUML	= '\x85',
	MACROMAN_UUML	= '\x86',
	MACROMAN_AGRAVE	= '\xCB',
	MACROMAN_ACIRC	= '\xE5',
	MACROMAN_ECIRC	= '\xE6',
	MACROMAN_AACUTE	= '\xE7',
	MACROMAN_EUML	= '\xE8',
	MACROMAN_EGRAVE	= '\xE9',
	MACROMAN_IACUTE	= '\xEA',
	MACROMAN_ICIRC	= '\xEB',
	MACROMAN_IUML	= '\xEC',
	MACROMAN_IGRAVE	= '\xED',
	MACROMAN_OACUTE	= '\xEE',
	MACROMAN_OCIRC	= '\xEF',
};

short CharToSprite(char c)
{
short	s;

	if ((c >= 'A') && (c <= 'Z'))
		s = HELPTEXT_SObjType_A + (c - 'A');
	else
	if ((c >= '0') && (c <= '9'))
		s = HELPTEXT_SObjType_0 + (c - '0');
	else
	switch(c)
	{
		case	'.':
				s = HELPTEXT_SObjType_Period;
				break;

		case	',':
				s = HELPTEXT_SObjType_Comma;
				break;

		case	'-':
				s = HELPTEXT_SObjType_Dash;
				break;

		case	'?':
				s = HELPTEXT_SObjType_QuestionMark;
				break;

		case	'!':
				s = HELPTEXT_SObjType_ExclamationMark;
				break;

		case	MACROMAN_UUML:
				s = HELPTEXT_SObjType_UU;
				break;

		case	MACROMAN_OUML:
				s = HELPTEXT_SObjType_OO;
				break;

		case	MACROMAN_AUML:
				s = HELPTEXT_SObjType_AA;
				break;

		case	MACROMAN_ARING:
				s = HELPTEXT_SObjType_AO;
				break;

		case	MACROMAN_NTILDE:
				s = HELPTEXT_SObjType_NN;
				break;

		case	MACROMAN_EACUTE:
				s = HELPTEXT_SObjType_EE;
				break;

		case	MACROMAN_EGRAVE:
				s = HELPTEXT_SObjType_EE;
				break;

		case	MACROMAN_ECIRC:
				s = HELPTEXT_SObjType_E;
				break;

		case	MACROMAN_AGRAVE:
				s = HELPTEXT_SObjType_Ax;
				break;

		case	MACROMAN_OCIRC:
				s = HELPTEXT_SObjType_Ox;
				break;

		case	MACROMAN_OACUTE:
				s = HELPTEXT_SObjType_Oa;
				break;

		case	CHAR_APOSTROPHE:
				s = HELPTEXT_SObjType_Apostrophe;
				break;


		default:
				s = -1;

	}


	return(s);
}


/******************* ADD HELP BEACON ***********************/

Boolean AddHelpBeacon(TerrainItemEntryType *itemPtr, long  x, long z)
{
ObjNode	*newObj;

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+100;
	gNewObjectDefinition.moveCall 	= MoveHelpBeacon;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list

	newObj->MessageNum = itemPtr->parm[0];

	return(true);
}


/***************** MOVE HELP BEACON *******************/

static void MoveHelpBeacon(ObjNode *theNode)
{
float	d;
short	messNum;

	if (TrackTerrainItem(theNode))
	{
		DeleteObject(theNode);
		return;
	}

			/*********************************/
			/* SEE IF IN RANGE TO DO MESSAGE */
			/*********************************/

	d = CalcQuickDistance(theNode->Coord.x, theNode->Coord.z, gPlayerInfo.coord.x, gPlayerInfo.coord.z);

	if (d < HELP_BEACON_RANGE)
	{
		messNum = theNode->MessageNum;					// get message #

		switch(messNum)									// do any custom handling
		{
			case	HELP_MESSAGE_JUMPJET:
					if (gPlayerInfo.jumpJet <= 0.0f)	// if player has no fuel, then change message
						messNum = HELP_MESSAGE_NEEDJUMPFUEL;
					break;
		}

		if (!gHelpMessageDisabled[messNum])				// check to see if it was disabled
			DisplayHelpMessage(messNum, 1.0, true);
	}
}


/****************** DISABLE HELP TYPE ***********************/

void DisableHelpType(short messNum)
{
	if (messNum < NUM_HELP_MESSAGES)
		gHelpMessageDisabled[messNum] = true;


}






