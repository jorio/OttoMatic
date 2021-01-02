
#pragma precompile_target "MyPCH_Normal.mch"


		/* MY BUILD OPTIONS */
		
#define	TARGET_API_MAC_CARBON				1
#define	CALL_IN_SPOCKETS_BUT_NOT_IN_CARBON	1

#define	DEMO	0


		/* HEADERS */
		
#include <stdlib.h>
#include <TextUtils.h>
#include <math.h>
#include <gl.h>
#include <textutils.h>
#include <Files.h>
#include <Resources.h>
#include <Sound.h>
#include <DrawSprocket.h>
#include <InputSprocket.h>
#include <CursorDevices.h>
#include <Traps.h>
#include <TextUtils.h>
#include <FixMath.h>

#include <Power.h>
#include <Script.h>
#include <Fonts.h>
#include <TextUtils.h>
#include <Gestalt.h>
#include	<Movies.h>
#include <Components.h>
#include <QuicktimeComponents.h>

#include "globals.h"
#include "structs.h"

#include "metaobjects.h"
#include "ogl_support.h"
#include "main.h"
#include "mobjtypes.h"
#include "misc.h"
#include "sound2.h"
#include "sobjtypes.h"
#include "sprites.h"
#include "sparkle.h"
#include "bg3d.h"
#include "camera.h"
#include "collision.h"
#include 	"input.h"
#include "file.h"
#include "windows.h"
#include "internet.h"
#include "player.h"
#include "terrain.h"
#include "humans.h"
#include "skeletonobj.h"
#include "skeletonanim.h"
#include "skeletonjoints.h"
#include	"infobar.h"
#include "triggers.h"
#include "effects.h"
#include "shards.h"
#include "bones.h"
#include "vaportrails.h"
#include "splineitems.h"
#include "mytraps.h"
#include "enemy.h"
#include "items.h"
#include "sky.h"
#include "water.h"
#include "fences.h"
#include "miscscreens.h"
#include "objects.h"
#include "mainmenu.h"
#include "lzss.h"


