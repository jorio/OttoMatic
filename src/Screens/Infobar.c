/****************************/
/*   	INFOBAR.C		    */
/* By Brian Greenstone      */
/* (c)2001 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawInfobarSprite_Rotated(float x, float y, float size, short texNum, float rot);
static void DrawInfobarSprite_Centered(float x, float y, float size, short texNum);
static void DrawInfobarSprite_Scaled(float x, float y, float scaleX, float scaleY, short texNum);
static void Infobar_DrawWeaponInventory(void);
static void Infobar_DrawGirders(void);
static void Infobar_DrawHealth(void);
static void Infobar_DrawLives(void);
static void Infobar_DrawFuel(void);
static void Infobar_DrawJumpJet(void);
static void Infobar_DrawBeams(void);
static void Infobar_DrawHumans(void);

static void UpdateHelpMessage(void);
static void MoveHelpBeacon(ObjNode *theNode);

static inline float AnchorLeft(float x);
static inline float AnchorRight(float x);
static inline float AnchorTop(float y);
static inline float AnchorBottom(float y);


/****************************/
/*    CONSTANTS             */
/****************************/

#define	HEALTH_X			AnchorLeft(23.0f)
#define	HEALTH_Y			AnchorTop(19.0f)
#define	HEALTH_SIZE			50.0f

#define JUMP_SIZE			HEALTH_SIZE
#define	JUMP_X				(HEALTH_X + HEALTH_SIZE +10)
#define	JUMP_Y				HEALTH_Y

#define	WEAPON_X			(JUMP_X+JUMP_SIZE + 10)
#define	WEAPON_Y			52
#define	WEAPON_HIDDEN_Y		-80
#define	WEAPON_INACTIVE_Y	-30
#define	WEAPON_SIZE			50
#define	WEAPON_FRAME_SIZE	(WEAPON_SIZE * 3/2)
#define	WEAPON_SPACING		(WEAPON_FRAME_SIZE - 5)

#define FUEL_SIZE			HEALTH_SIZE
#define	FUEL_X				AnchorRight(FUEL_SIZE+23.0f)
#define	FUEL_Y				HEALTH_Y

#define	BEAM_CUP_X			AnchorLeft(190.0f)
#define	BEAM_CUP_Y			AnchorTop(20.0f)
#define	BEAM_CUP_SCALE		20.0f
#define	BEAM_CUP_GLOW_SCALE (BEAM_CUP_SCALE * 3.0f)

#define	BEAM_SPARKLE_X		(BEAM_CUP_X-BEAM_CUP_GLOW_SCALE/2-10.0f)

#define	BEAM_SCALE			BEAM_CUP_SCALE
#define	BEAM_X				(BEAM_CUP_X + BEAM_CUP_SCALE/2)
#define	BEAM_Y				(BEAM_CUP_Y - BEAM_SCALE/2)

#define	HUMAN_SCALE			25.0f
#define HUMAN_XFROMRIGHT	(-10 - HUMAN_SCALE/2)
#define HUMAN_OFFSCREEN_OFFSET_X (HUMAN_XFROMRIGHT + 60.0f)
#define	HUMAN_X				AnchorRight(-HUMAN_XFROMRIGHT)
#define	HUMAN_Y				AnchorTop(150.0f)
#define	HUMAN_SPACING		(HUMAN_SCALE * 2.8f)

#define	ALTHUMAN_SPACING	(HUMAN_SCALE * 2.3f)
#define ALTHUMAN_X			AnchorRight(ALTHUMAN_SPACING*0.75f)
#define ALTHUMAN_Y			AnchorBottom(-3)

#define	HELP_Y				AnchorBottom(60)






/*********************/
/*    VARIABLES      */
/*********************/

static RectF	gUIAnchors;

static 	short			gHealthWarningChannel = -1;
static	float			gHealthWarningWobble = 0;

float			gJumpJetWarningCooldown = 0;

Boolean			gHideInfobar = false;
static float	gHealthOccilate = 0;
static float	gHealthMeterRot = 0, gFuelMeterRot = 0, gJumpJetMeterRot=0;

static float	gWeaponY[MAX_INVENTORY_SLOTS];

short	gDisplayedHelpMessage;
static float	gHelpMessageAlpha;
static float	gHelpMessageTimer;
static ObjNode	*gHelpMessageObject;

#define	MessageNum	Special[0]

Boolean	gHelpMessageDisabled[NUM_HELP_MESSAGES];

static	float	gHumanOffsetX[NUM_HUMAN_TYPES];


/*************** ASPECT RATIO-INDEPENDENT ANCHORS ******************/

static inline float GetUIScale(void)
{
	// During normal gameplay, Otto may fill up 5 different weapon slots in level 10.
	// The max scale to be able to fit 5 weapon slots comfortably in a 4:3 aspect ratio is 1.1x.

	static const float values[NUM_UI_SCALE_LEVELS] =
	{
		[0] = 0.5f,
		[1] = 0.6f,
		[2] = 0.7f,
		[3] = 0.8f,
		[4] = 0.9f,
		[5] = 1.0f,
		[6] = 1.05f,
		[7] = 1.1f,
	};

	return values[gGamePrefs.uiScaleLevel];
}

static inline float AnchorLeft(float x)
{
	return gUIAnchors.left + x;
}

static inline float AnchorRight(float x)
{
	return gUIAnchors.right - x;
}

static inline float AnchorTop(float y)
{
	return gUIAnchors.top + y;
}

static inline float AnchorBottom(float y)
{
	return gUIAnchors.bottom - y;
}

static inline float AnchorCenterX(float x)
{
	float cx = 0.5f * (gUIAnchors.right + gUIAnchors.left);
	return x + cx;
}

static inline float AnchorCenterY(float y)
{
	float cy = 0.5f * (gUIAnchors.bottom + gUIAnchors.top);
	return y + cy;
}


/********************* INIT INFOBAR ****************************/
//
// Called at beginning of level
//

void InitInfobar(void)
{
	gDisplayedHelpMessage = HELP_MESSAGE_NONE;

	for (int i = 0; i < NUM_HUMAN_TYPES; i++)
		gHumanOffsetX[i] = HUMAN_OFFSCREEN_OFFSET_X;

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


	for (int i = 0; i < MAX_INVENTORY_SLOTS; i++)
		gWeaponY[i] = WEAPON_HIDDEN_Y;
}


/****************** DISPOSE INFOBAR **********************/

void DisposeInfobar(void)
{
	gDisplayedHelpMessage = HELP_MESSAGE_NONE;
	if (gHelpMessageObject)
	{
		DeleteObject(gHelpMessageObject);
		gHelpMessageObject = NULL;
	}
}


/***************** SET INFOBAR SPRITE STATE *******************/

void SetInfobarSpriteState(bool setOriginToCenterOfScreen)
{
	OGL_DisableLighting();
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);								// no z-buffer

	gGlobalMaterialFlags = BG3D_MATERIALFLAG_CLAMP_V|BG3D_MATERIALFLAG_CLAMP_U;	// clamp all textures


			/* INIT MATRICES */

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (setOriginToCenterOfScreen)
		glOrtho(-g2DLogicalWidth*.5f, g2DLogicalWidth*.5f, g2DLogicalHeight*.5f, -g2DLogicalHeight*.5f, 0, 1);
	else
		glOrtho(0, g2DLogicalWidth, g2DLogicalHeight, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


static void SetInfobarSpriteStateScaled(float s)
{
	OGL_DisableLighting();
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);								// no z-buffer

	gGlobalMaterialFlags = BG3D_MATERIALFLAG_CLAMP_V|BG3D_MATERIALFLAG_CLAMP_U;	// clamp all textures


			/* INIT MATRICES */

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, g2DLogicalWidth/s, g2DLogicalHeight/s, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


/********************** DRAW INFOBAR ****************************/

void DrawInfobar(void)
{
	if (gHideInfobar)
		return;

		/*********************/
		/* SET UP UI ANCHORS */
		/*********************/

	float uiScale = GetUIScale();

	gUIAnchors.left = 0;
	gUIAnchors.top = 0;
	gUIAnchors.right = g2DLogicalWidth / uiScale;
	gUIAnchors.bottom = g2DLogicalHeight / uiScale;
	if (gGamePrefs.uiCentering)
	{
		float currentAR = g2DLogicalWidth / g2DLogicalHeight;
		float targetAR = 640.0f / 480.0f;

		if (currentAR >= targetAR)		// current aspect ratio is wider than the target aspect ratio
		{
			float covered = (g2DLogicalWidth / uiScale) * targetAR / currentAR;
			float padding = (g2DLogicalWidth / uiScale) - covered;

			gUIAnchors.left = padding / 2.0f;
			gUIAnchors.right = gUIAnchors.left + covered;
		}
	}


		/************/
		/* SET TAGS */
		/************/

	OGL_PushState();

	if (gGameViewInfoPtr->useFog)
		glDisable(GL_FOG);


	SetInfobarSpriteStateScaled(uiScale);



		/***************/
		/* DRAW THINGS */
		/***************/


		/* DRAW STUFF */

	Infobar_DrawGirders();
	Infobar_DrawLives();
	Infobar_DrawHealth();
	Infobar_DrawFuel();
	Infobar_DrawHumans();

	if (gLevelNum != LEVEL_NUM_SAUCER)
	{
		Infobar_DrawWeaponInventory();
		Infobar_DrawJumpJet();
	}
	else
	{
		gGlobalTransparency = .3f;					// dim these in saucer mode
		Infobar_DrawJumpJet();
		gGlobalTransparency = 1.0f;

		Infobar_DrawBeams();
	}

	UpdateHelpMessage();

			/***********/
			/* CLEANUP */
			/***********/

	OGL_PopState();
	gGlobalMaterialFlags = 0;
}


/******************** DRAW INFOBAR SPRITE **********************/

void DrawInfobarSprite(float x, float y, float size, short texNum)
{
MOMaterialObject	*mo;
float				aspect;

		/* ACTIVATE THE MATERIAL */

	mo = gSpriteGroupList[SPRITE_GROUP_INFOBAR][texNum].materialObject;
	MO_DrawMaterial(mo);

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,0);	glVertex2f(x, 		y);
	glTexCoord2f(1,0);	glVertex2f(x+size, 	y);
	glTexCoord2f(1,1);	glVertex2f(x+size,  y+(size*aspect));
	glTexCoord2f(0,1);	glVertex2f(x,		y+(size*aspect));
	glEnd();
}

/******************** DRAW INFOBAR SPRITE: CENTERED **********************/
//
// Coords are for center of sprite, not upper left
//

static void DrawInfobarSprite_Centered(float x, float y, float size, short texNum)
{
MOMaterialObject	*mo;
float				aspect;

		/* ACTIVATE THE MATERIAL */

	mo = gSpriteGroupList[SPRITE_GROUP_INFOBAR][texNum].materialObject;
	MO_DrawMaterial(mo);

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

	x -= size*.5f;
	y -= (size*aspect)*.5f;

			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,0);	glVertex2f(x, 		y);
	glTexCoord2f(1,0);	glVertex2f(x+size, 	y);
	glTexCoord2f(1,1);	glVertex2f(x+size,  y+(size*aspect));
	glTexCoord2f(0,1);	glVertex2f(x,		y+(size*aspect));
	glEnd();
}



/******************** DRAW INFOBAR SPRITE 2**********************/
//
// This version lets user pass in the sprite group
//

void DrawInfobarSprite2(float x, float y, float size, short group, short texNum)
{
MOMaterialObject	*mo;
float				aspect;

		/* ACTIVATE THE MATERIAL */

	mo = gSpriteGroupList[group][texNum].materialObject;
	MO_DrawMaterial(mo);

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,0);	glVertex2f(x, 		y);
	glTexCoord2f(1,0);	glVertex2f(x+size, 	y);
	glTexCoord2f(1,1);	glVertex2f(x+size,  y+(size*aspect));
	glTexCoord2f(0,1);	glVertex2f(x,		y+(size*aspect));
	glEnd();
}



/******************** DRAW INFOBAR SPRITE: ROTATED **********************/

static void DrawInfobarSprite_Rotated(float x, float y, float size, short texNum, float rot)
{
MOMaterialObject	*mo;
float				aspect, xoff, yoff;
OGLPoint2D			p[4];
OGLMatrix3x3		m;

		/* ACTIVATE THE MATERIAL */

	mo = gSpriteGroupList[SPRITE_GROUP_INFOBAR][texNum].materialObject;
	MO_DrawMaterial(mo);

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
	glTexCoord2f(0,0);	glVertex2f(p[0].x + xoff, p[0].y + yoff);
	glTexCoord2f(1,0);	glVertex2f(p[1].x + xoff, p[1].y + yoff);
	glTexCoord2f(1,1);	glVertex2f(p[2].x + xoff, p[2].y + yoff);
	glTexCoord2f(0,1);	glVertex2f(p[3].x + xoff, p[3].y + yoff);
	glEnd();
}


/******************** DRAW INFOBAR SPRITE: SCALED **********************/

static void DrawInfobarSprite_Scaled(float x, float y, float scaleX, float scaleY, short texNum)
{
MOMaterialObject	*mo;
float				aspect;

		/* ACTIVATE THE MATERIAL */

	mo = gSpriteGroupList[SPRITE_GROUP_INFOBAR][texNum].materialObject;
	MO_DrawMaterial(mo);

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

			/* DRAW IT */

	glBegin(GL_QUADS);
	glTexCoord2f(0,0);	glVertex2f(x, 			y);
	glTexCoord2f(1,0);	glVertex2f(x+scaleX, 	y);
	glTexCoord2f(1,1);	glVertex2f(x+scaleX, 	y+(scaleY*aspect));
	glTexCoord2f(0,1);	glVertex2f(x,			y+(scaleY*aspect));
	glEnd();
}


#pragma mark -


/********************** DRAW WEAPON INVENTORY *************************/

static void Infobar_DrawWeaponInventory(void)
{
float	x,y,fps = gFramesPerSecondFrac;
float	tx;
int		i,n;
short	type;
Str255	s;

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

			DrawInfobarSprite(x, y, WEAPON_FRAME_SIZE, INFOBAR_SObjType_WeaponDisplay);


			if (type != NO_INVENTORY_HERE)
			{

								/* DRAW GLOW */

				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				DrawInfobarSprite_Centered(x + WEAPON_FRAME_SIZE/2, y+WEAPON_Y, WEAPON_SIZE * (1.0f + RandomFloat()*.15f), INFOBAR_SObjType_PulseGunGlow + type);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

								/* DRAW ICON */

				DrawInfobarSprite_Centered(x + WEAPON_FRAME_SIZE/2, y+WEAPON_Y, WEAPON_SIZE, INFOBAR_SObjType_PulseGun + type);


					/* DRAW QUANTITY NUMBER */

				if (type != WEAPON_TYPE_FIST)									// dont draw fist quantity since unlimited
				{
					NumToString(gPlayerInfo.weaponInventory[i].quantity, s);

					tx = x + 50;

					for (n = 1; n <= s[0]; n++)
					{
						DrawInfobarSprite(tx, y+60, 8, INFOBAR_SObjType_0 + s[n]-'0');
						tx += 5.1f;
					}
				}
			}
		}

			/* NEXT SLOT */

		x += WEAPON_SPACING;
	}

}



/********************** DRAW GIRDERS *************************/

static void Infobar_DrawGirders(void)
{
	// TODO: Scale g2DLogicalWidth
	// TODO: Hide fusée if not enough room for all weapon slots
	// Always draw girders in corners of screen (no AnchorLeft/AnchorRight) even if HUD is centered
	DrawInfobarSprite(0, 0, 100, INFOBAR_SObjType_LeftGirder);
	DrawInfobarSprite(g2DLogicalWidth/GetUIScale()-100, 0, 100, INFOBAR_SObjType_RightGirder);
}


/********************** DRAW LIVES *************************/

static void Infobar_DrawLives(void)
{
short	i;

	for (i = 0; i < gPlayerInfo.lives; i++)
		DrawInfobarSprite(AnchorLeft(i * 30), AnchorBottom(30), 40, INFOBAR_SObjType_OttoHead);

}


/********************** DRAW HUMANS *************************/

static void Infobar_DrawHumans(void)
{
static const float scales[NUM_HUMAN_TYPES] =
{
	HUMAN_SCALE * 2.0f,					// farmer
	HUMAN_SCALE,
	HUMAN_SCALE,
	HUMAN_SCALE
};


	Boolean useAltPlacement
		= gGamePrefs.uiCentering
		&& (fabsf(gUIAnchors.right - g2DLogicalWidth/GetUIScale()) > 15);


	for (int i = 0; i < NUM_HUMAN_TYPES; i++)
	{
		if (gNumHumansRescuedOfType[i] == 0)					// skip if none of these rescued
			continue;

				/* SCROLL INTO POSITION */

		if (gHumanOffsetX[i] > 0.0f)							// see if need to move it
		{
			gHumanOffsetX[i] -= gFramesPerSecondFrac * 150.0f;
			if (gHumanOffsetX[i] < 0)
				gHumanOffsetX[i] = 0;
		}


		float x;
		float y;
		float mugshotY;

		if (!useAltPlacement)
		{
			x = HUMAN_X + gHumanOffsetX[i];
			y = HUMAN_Y + HUMAN_SPACING * i;
			mugshotY = y;
			DrawInfobarSprite_Centered(x, y, HUMAN_SCALE * 3.0f, INFOBAR_SObjType_HumanFrame);
		}
		else
		{
			y = ALTHUMAN_Y + gHumanOffsetX[i];
			x = ALTHUMAN_X - ALTHUMAN_SPACING * i;
			mugshotY = y - HUMAN_SCALE * .65f;
			DrawInfobarSprite_Centered(x, y, HUMAN_SCALE * 3.0f, INFOBAR_SObjType_HumanFrame2);
		}

				/* DRAW HUMAN ICON */

		x -= HUMAN_SCALE * .4f;
		DrawInfobarSprite_Centered(x, mugshotY, scales[i], INFOBAR_SObjType_Farmer+i);

				/* DRAW QUANTITY */

		int n = gNumHumansRescuedOfType[i];

		float tx = x - (HUMAN_SCALE * .8f);
		if (n >= 100) tx += 2*HUMAN_SCALE/5;
		else if (n >= 10) tx += HUMAN_SCALE/5;

		for (; n != 0; n /= 10)
		{
			DrawInfobarSprite(tx, y-HUMAN_SCALE*0.9f, HUMAN_SCALE/3, INFOBAR_SObjType_0 + (n % 10));
			tx -= HUMAN_SCALE/5;
		}
	}
}


/********************** DRAW HEALTH *************************/

static void Infobar_DrawHealth(void)
{
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
		DrawInfobarSprite_Rotated(HEALTH_X + xoff, HEALTH_Y, HEALTH_SIZE, INFOBAR_SObjType_MeterBack, gHealthMeterRot);
	}
	else
		DrawInfobarSprite(HEALTH_X + xoff, HEALTH_Y, HEALTH_SIZE, INFOBAR_SObjType_MeterBack);


			/* DRAW METER */

	gHealthOccilate += fps * 10.0f;
	size = HEALTH_SIZE * (1.0f + sin(gHealthOccilate)*.04f) * n;
	x = HEALTH_X + (HEALTH_SIZE - size) * .5f;
	y = HEALTH_Y + (HEALTH_SIZE - size) * .5f;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);								// make glow
	DrawInfobarSprite(x+xoff, y, size, INFOBAR_SObjType_HealthMeter);
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

static void Infobar_DrawFuel(void)
{
float	x,y,size;
float	n, fps = gFramesPerSecondFrac;
//float	FUEL_X = g2DLogicalWidth + FUEL_XFROMRIGHT;

			/* DRAW ROCKET ICON */

			
	float rocketIconX = FUEL_X-34;
	Boolean roomForRocketIcon = true;


	for (int i = 0; i < MAX_INVENTORY_SLOTS; i++)
	{
		if (gPlayerInfo.weaponInventory[i].type != NO_INVENTORY_HERE)
		{
			if (WEAPON_X + (i+1) * WEAPON_SPACING > rocketIconX)
			{
				roomForRocketIcon = false;
				break;
			}
		}
	}

	if (roomForRocketIcon)
		DrawInfobarSprite(FUEL_X-34, 15, 30, INFOBAR_SObjType_RocketIcon);


	n = gPlayerInfo.fuel;										// get health

			/* DRAW BACK */

	if (n >= 1.0f)
	{
		gFuelMeterRot += fps * 2.0f;
		DrawInfobarSprite_Rotated(FUEL_X, FUEL_Y, FUEL_SIZE, INFOBAR_SObjType_MeterBack, gFuelMeterRot);
	}
	else
		DrawInfobarSprite(FUEL_X, FUEL_Y, FUEL_SIZE, INFOBAR_SObjType_MeterBack);


	size = FUEL_SIZE * (1.0f + cos(gHealthOccilate)*.04f) * n;
	x = FUEL_X + (FUEL_SIZE - size) * .5f;
	y = FUEL_Y + (FUEL_SIZE - size) * .5f;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);								// make glow
	DrawInfobarSprite(x, y, size, INFOBAR_SObjType_FuelMeter);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


/********************** DRAW JUMPJET *************************/

static void Infobar_DrawJumpJet(void)
{
float	fps = gFramesPerSecondFrac;
float	x = JUMP_X;
float	y = JUMP_Y;
float	n = gPlayerInfo.jumpJet;								// get jj
float	size;



			/* DRAW BACK */

	if (n >= 1.0f)
	{
		gJumpJetMeterRot -= fps * 2.0f;
		DrawInfobarSprite_Rotated(x, y, JUMP_SIZE, INFOBAR_SObjType_MeterBack, gJumpJetMeterRot);
	}
	else
	{
		if (gJumpJetWarningCooldown > 0)
		{
			float amp = 4.0f * gJumpJetWarningCooldown / 0.5f;
			x += sinf(gJumpJetWarningCooldown * 32.0f) * amp;
			gJumpJetWarningCooldown -= fps;
		}

		DrawInfobarSprite(x, y, JUMP_SIZE, INFOBAR_SObjType_MeterBack);
	}


	size = JUMP_SIZE * (1.0f + cos(gHealthOccilate)*.04f) * n;
	x += (JUMP_SIZE - size) * .5f;
	y += (JUMP_SIZE - size) * .5f;



	glBlendFunc(GL_SRC_ALPHA, GL_ONE);								// make glow
	DrawInfobarSprite(x, y, size, INFOBAR_SObjType_JumpJetMeter);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}



/********************** DRAW BEAMS *************************/

static void Infobar_DrawBeams(void)
{
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
	DrawInfobarSprite_Scaled(BEAM_X, BEAM_Y, q, BEAM_SCALE, INFOBAR_SObjType_TeleportBeam);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


				/* DRAW CUPS */

	gGlobalTransparency = 1.0f;
	q += BEAM_CUP_SCALE;
	DrawInfobarSprite_Centered(BEAM_CUP_X, BEAM_CUP_Y, BEAM_CUP_SCALE, INFOBAR_SObjType_BeamCupLeft);
	DrawInfobarSprite_Centered(BEAM_CUP_X+q, BEAM_CUP_Y, BEAM_CUP_SCALE, INFOBAR_SObjType_BeamCupRight);


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
	DrawInfobarSprite_Scaled(BEAM_X, BEAM_Y+BEAM_CUP_SCALE, q2, BEAM_SCALE, INFOBAR_SObjType_DestructoBeam);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				/* DRAW CUPS */

	gGlobalTransparency = 1.0f;
	q2 += BEAM_CUP_SCALE;
	DrawInfobarSprite_Centered(BEAM_CUP_X, BEAM_CUP_Y + BEAM_CUP_SCALE, BEAM_CUP_SCALE, INFOBAR_SObjType_BeamCupLeft);
	DrawInfobarSprite_Centered(BEAM_CUP_X+q2,  BEAM_CUP_Y + BEAM_CUP_SCALE, BEAM_CUP_SCALE, INFOBAR_SObjType_BeamCupRight);


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
	DrawInfobarSprite2(BEAM_SPARKLE_X, y, BEAM_CUP_SCALE*3, SPRITE_GROUP_PARTICLES, PARTICLE_SObjType_WhiteSpark4);
	gGlobalTransparency = .8f + RandomFloat() * .2f;
	DrawInfobarSprite2(BEAM_CUP_X+q-BEAM_CUP_GLOW_SCALE/2+10, y, BEAM_CUP_SCALE*3, SPRITE_GROUP_PARTICLES, PARTICLE_SObjType_WhiteSpark4);


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


/************** GET KEYBINDING NAME FOR USE IN HELP MESSAGES ***************/

static const char* GetShortNameForInputNeed(int need)
{
	GAME_ASSERT(need >= 0);
	GAME_ASSERT(need < NUM_REMAPPABLE_NEEDS);

	if (gUserPrefersGamepad)
	{
		int8_t type	= gGamePrefs.remappableKeys[need].gamepad[0].type;
		int8_t id	= gGamePrefs.remappableKeys[need].gamepad[0].id;
		switch (type)
		{
			case kInputTypeButton:
				return SDL_GameControllerGetStringForButton(id);

			case kInputTypeAxisMinus:
			case kInputTypeAxisPlus:
				return SDL_GameControllerGetStringForAxis(id);
		}
	}
	else
	{
		int16_t key = gGamePrefs.remappableKeys[need].key[0];
		switch (key)
		{
#if __APPLE__
			case SDL_SCANCODE_LALT:
			case SDL_SCANCODE_RALT:
				return "Option";
#else
			case SDL_SCANCODE_LALT:
				return "Alt";

			case SDL_SCANCODE_RALT:
				return "AltGr";
#endif

			case SDL_SCANCODE_LCTRL:
			case SDL_SCANCODE_RCTRL:
				return "Ctrl";

			case SDL_SCANCODE_LGUI:
			case SDL_SCANCODE_RGUI:
				return "Command";

			case SDL_SCANCODE_LSHIFT:
			case SDL_SCANCODE_RSHIFT:
				return "Shift";

			default:
				return SDL_GetScancodeName(key);
		}
	}

	return "???";
}

/****************** FORMAT HELP MESSAGE WITH INPUT NEED ********************/

static const char* FormatHelpMessage(const char* localized, int need)
{
#define N 256
	static char buf[N];
	size_t i;

	const char* input = localized;

	const char* keyName = GetShortNameForInputNeed(need);

	i = 0;
	while (i < N-1)
	{
		if (!*input)
			break;

		if (*input == '#')
		{
			const char* k = keyName;
			while (*k && i < N-1)
			{
				if (*k >= 'a' && *k <= 'z')
					buf[i] = *k + ('A'-'a');
				else
					buf[i] = *k;
				i++;
				k++;
			}
			//i += snprintf(buf+i, N-i, "%s", keyDesc);
		}
		else
		{
			buf[i++] = *input;
		}

		input++;
	}

	buf[i] = '\0';

	return buf;
#undef N
}

/**************************** DISPLAY HELP MESSAGE *******************************/

void DisplayHelpMessage(short messNum, float timer, Boolean overrideCurrent)
{
	if (gHelpMessageDisabled[messNum])						// dont do it if this message is disabled
		return;

	if (messNum == gDisplayedHelpMessage)					// if already showing this one then dont need to do anything more
	{
		gHelpMessageTimer = timer;							// keep timer going
		return;
	}

	if ((!overrideCurrent) && (gDisplayedHelpMessage != HELP_MESSAGE_NONE))	// if can't override current then bail
		return;

	if (gDisplayedHelpMessage != HELP_MESSAGE_NONE && gHelpMessageObject)
	{
		DeleteObject(gHelpMessageObject);
		gHelpMessageObject = NULL;
	}

	gDisplayedHelpMessage = messNum;
	gHelpMessageAlpha = 0;
	gHelpMessageTimer = timer;								// set timer


			/* GET THE STRING TEXT TO DISPLAY */

	const char* helpString = Localize(messNum + STR_IN_GAME_HELP_0);

	switch (messNum)
	{
		case HELP_MESSAGE_POWPOD:				helpString = FormatHelpMessage(helpString, kNeed_PunchPickup); break;
		case HELP_MESSAGE_PICKUPPOW:			helpString = FormatHelpMessage(helpString, kNeed_PunchPickup); break;
		case HELP_MESSAGE_JUMPJET:				helpString = FormatHelpMessage(helpString, kNeed_Jump); break;
		case HELP_MESSAGE_LETGOMAGNET:			helpString = FormatHelpMessage(helpString, kNeed_Jump); break;
		case HELP_MESSAGE_PRESSJUMPTOLEAVE:		helpString = FormatHelpMessage(helpString, kNeed_Jump); break;
		case HELP_MESSAGE_INTOCANNON:			helpString = FormatHelpMessage(helpString, kNeed_Jump); break;
	}


			/* CALC STRING PARAMETERS */

	gNewObjectDefinition.coord		= (OGLPoint3D) {g2DLogicalWidth * .5f, HELP_Y, 0};
	gNewObjectDefinition.flags		= STATUS_BIT_HIDDEN;		// hidden because we'll draw it ourselves
	gNewObjectDefinition.moveCall	= nil;
	gNewObjectDefinition.rot		= 0;
	gNewObjectDefinition.scale		= 1;
	gNewObjectDefinition.slot		= SPRITE_SLOT;
	gHelpMessageObject = TextMesh_New(helpString, 1, &gNewObjectDefinition);
	gHelpMessageObject->ColorFilter.a = 0;
}


/******************** UPDATE HELP MESSAGE *****************************/

static void UpdateHelpMessage(void)
{
const float	fps = gFramesPerSecondFrac;

	if (gDisplayedHelpMessage == HELP_MESSAGE_NONE)
		return;

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
			DeleteObject(gHelpMessageObject);
			gHelpMessageObject = NULL;
			return;
		}
	}

			/*******************/
			/* DRAW THE BORDER */
			/*******************/

	float uiScale = GetUIScale();

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	SetColor4f(.2,.2,.2,gHelpMessageAlpha * .5f);
	glBegin(GL_QUADS);
	glVertex2f(0, 				HELP_Y + 24);
	glVertex2f(g2DLogicalWidth/uiScale, HELP_Y + 24);
	glVertex2f(g2DLogicalWidth/uiScale, HELP_Y - 2);
	glVertex2f(0,				HELP_Y - 2);
	glEnd();
	SetColor4f(1,1,1,1);


			/*******************/
			/* DRAW THE STRING */
			/*******************/

	gHelpMessageObject->ColorFilter.a = gHelpMessageAlpha;
	gHelpMessageObject->Coord.x = AnchorCenterX(0);
	gHelpMessageObject->Coord.y = HELP_Y;
	UpdateObjectTransforms(gHelpMessageObject);
	gGlobalTransparency = gHelpMessageAlpha;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	MO_DrawObject(gHelpMessageObject->BaseGroup);


			/* CLEANUP */

	gGlobalTransparency = 1.0f;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
