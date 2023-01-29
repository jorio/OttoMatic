/****************************/
/*     SOUND ROUTINES       */
/* By Brian Greenstone      */
/* (c)2000 Pangea Software  */
/* (c)2021 Iliyas Jorio     */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static short FindSilentChannel(void);
static void Calc3DEffectVolume(short effectNum, OGLPoint3D *where, float volAdjust, uint32_t *leftVolOut, uint32_t *rightVolOut);
//static void UpdateGlobalVolume(void);


/****************************/
/*    CONSTANTS             */
/****************************/

#define		MAX_CHANNELS			40

typedef struct
{
	int				bank;
	const char*		filename;
	float			refDistance;
	int				flags;
} EffectDef;

typedef struct
{
	float			force;
	uint16_t		duration;
} AutoRumbleDef;

typedef struct
{
	SndListHandle	sndHandle;
	long			sndOffset;
	short			lastPlayedOnChannel;
	uint32_t			lastLoudness;
} LoadedEffect;

enum
{
	// At most one instance of the effect may be played back at once.
	// If the effect is started again, and it is louder than the previous instance,
	// the previous instance is stopped.
	kSoundFlag_Unique = 1 << 0,

	// Don't interpolate if the effect's sample rate differs from the target sample rate.
	kSoundFlag_NoInterp = 1 << 1,
};



#define	VOLUME_DISTANCE_FACTOR	.004f		// bigger == sound decays FASTER with dist, smaller = louder far away

const int kLevelSoundBanks[NUM_LEVELS] =
{
	SOUNDBANK_FARM,
	SOUNDBANK_SLIME,
	SOUNDBANK_SLIME,
	SOUNDBANK_APOC,
	SOUNDBANK_CLOUD,
	SOUNDBANK_JUNGLE,
	SOUNDBANK_JUNGLE,
	SOUNDBANK_FIREICE,
	SOUNDBANK_SAUCER,
	SOUNDBANK_BRAIN,
};

/**********************/
/*     VARIABLES      */
/**********************/

float						gGlobalVolume = .3;

static OGLPoint3D					gEarCoords;			// coord of camera plus a tad to get pt in front of camera
static	OGLVector3D			gEyeVector;

static	LoadedEffect		gLoadedEffects[NUM_EFFECTS];

static	SndChannelPtr		gSndChannel[MAX_CHANNELS];
ChannelInfoType		gChannelInfo[MAX_CHANNELS];

static short				gMaxChannels = 0;

static	SndChannelPtr		gMusicChannel = nil;


Boolean						gAllowAudioKeys = true;


static short				gCurrentSong = -1;
static Boolean				gSongPlayingFlag = false;


		/*****************/
		/* EFFECTS TABLE */
		/*****************/

static const char* kSoundBankNames[] =
{
	[SOUNDBANK_MAIN]      = "main",
	[SOUNDBANK_MENU]      = "menu",
	[SOUNDBANK_BONUS]     = "bonus",
	[SOUNDBANK_FARM]      = "farm",
	[SOUNDBANK_SLIME]     = "slime",
	[SOUNDBANK_APOC]      = "apocalypse",
	[SOUNDBANK_CLOUD]     = "cloud",
	[SOUNDBANK_JUNGLE]    = "jungle",
	[SOUNDBANK_FIREICE]   = "fireice",
	[SOUNDBANK_SAUCER]    = "saucer",
	[SOUNDBANK_BRAIN]     = "brainboss",
	[SOUNDBANK_LOSE]      = "lose",
};

static const EffectDef kEffectsTable[] =
{
	[EFFECT_BADSELECT]        = {SOUNDBANK_MAIN   , "badselect"       , 2000, 0	},
	[EFFECT_SAUCER]           = {SOUNDBANK_MAIN   , "saucer"          , 1000, 0	},
	[EFFECT_STUNGUN]          = {SOUNDBANK_MAIN   , "stungun"         , 700 , 0	},
	[EFFECT_ZAP]              = {SOUNDBANK_MAIN   , "zap"             , 2000, 0	},
	[EFFECT_ROCKET]           = {SOUNDBANK_MAIN   , "rocket"          , 2500, 0	},
	[EFFECT_ROCKETLANDED]     = {SOUNDBANK_MAIN   , "landed"          , 3000, 0	},
	[EFFECT_JUMPJET]          = {SOUNDBANK_MAIN   , "jumpjet"         , 2000, 0	},
	[EFFECT_SHATTER]          = {SOUNDBANK_MAIN   , "shatter"         , 3000, 0	},
	[EFFECT_WEAPONCLICK]      = {SOUNDBANK_MAIN   , "weaponclick"     , 3000, kSoundFlag_Unique	},
	[EFFECT_WEAPONWHIR]       = {SOUNDBANK_MAIN   , "weaponwhir"      , 3000, kSoundFlag_NoInterp	},
	[EFFECT_NEWLIFE]          = {SOUNDBANK_MAIN   , "newlife"         , 3000, kSoundFlag_NoInterp	},
	[EFFECT_FREEZEGUN]        = {SOUNDBANK_MAIN   , "freezegun"       , 3000, 0	},
	[EFFECT_PUNCHHIT]         = {SOUNDBANK_MAIN   , "punchhit"        , 1000, 0	},
	[EFFECT_PLAYERCRASH]      = {SOUNDBANK_MAIN   , "playercrash"     , 1000, 0	},
	[EFFECT_NOJUMPJET]        = {SOUNDBANK_MAIN   , "nojumpjet"       , 1000, 0	},
	[EFFECT_PLAYERCRUSH]      = {SOUNDBANK_MAIN   , "playercrush"     , 1400, 0	},
	[EFFECT_HEADSWOOSH]       = {SOUNDBANK_MAIN   , "headswoosh"      , 1200, 0	},
	[EFFECT_HEADTHUD]         = {SOUNDBANK_MAIN   , "headthud"        , 1200, 0	},
	[EFFECT_POWPODHIT]        = {SOUNDBANK_MAIN   , "powpodhit"       , 400 , 0	},
	[EFFECT_METALLAND]        = {SOUNDBANK_MAIN   , "metalland"       , 50  , 0	},
	[EFFECT_SERVO]            = {SOUNDBANK_MAIN   , "servo"           , 300 , 0	},
	[EFFECT_LEFTFOOT]         = {SOUNDBANK_MAIN   , "leftfoot"        , 40  , 0	},
	[EFFECT_RIGHTFOOT]        = {SOUNDBANK_MAIN   , "rightfoot"       , 40  , 0	},
	[EFFECT_NOVACHARGE]       = {SOUNDBANK_MAIN   , "novacharge"      , 4000, 0	},
	[EFFECT_TELEPORTHUMAN]    = {SOUNDBANK_MAIN   , "teleporthuman"   , 3000, 0	},
	[EFFECT_CHECKPOINTHIT]    = {SOUNDBANK_MAIN   , "checkpointhit"   , 3000, 0	},
	[EFFECT_CHECKPOINTLOOP]   = {SOUNDBANK_MAIN   , "checkpointloop"  , 800 , 0	},
	[EFFECT_FLARESHOOT]       = {SOUNDBANK_MAIN   , "flareshoot"      , 3000, 0	},
	[EFFECT_FLAREEXPLODE]     = {SOUNDBANK_MAIN   , "flareexplode"    , 3000, 0	},
	[EFFECT_DARTWOOSH]        = {SOUNDBANK_MAIN   , "dartwoosh"       , 3000, 0	},
	[EFFECT_ATOMCHIME]        = {SOUNDBANK_MAIN   , "atomchime"       , 3000, 0	},
	[EFFECT_PLAYERCLANG]      = {SOUNDBANK_MAIN   , "playerclang"     , 2000, kSoundFlag_Unique		},
	[EFFECT_THROWNSWOOSH]     = {SOUNDBANK_MAIN   , "thrownswoosh"    , 4000, 0	},
	[EFFECT_BEAMHUM]          = {SOUNDBANK_MAIN   , "beamhum"         , 1500, 0	},
	[EFFECT_JUMP]             = {SOUNDBANK_MAIN   , "jump"            , 1000, 0	},
	[EFFECT_HEALTHWARNING]    = {SOUNDBANK_MAIN   , "healthwarning"   , 1000, 0	},
	[EFFECT_HATCH]            = {SOUNDBANK_MAIN   , "hatch"           , 2000, 0	},
	[EFFECT_BRAINWAVE]        = {SOUNDBANK_MAIN   , "brainwave"       , 10  , 0	},
	[EFFECT_WEAPONDEPOSIT]    = {SOUNDBANK_MAIN   , "weapondeposit"   , 2000, 0	},
	[EFFECT_BRAINDIE]         = {SOUNDBANK_MAIN   , "braindie"        , 2000, kSoundFlag_Unique	},
	[EFFECT_LASERHIT]         = {SOUNDBANK_MAIN   , "laserhit"        , 1000, 0	},
	[EFFECT_FLAREUP]          = {SOUNDBANK_MAIN   , "flareup"         , 1500, 0	},
	[EFFECT_FREEZEPOOF]       = {SOUNDBANK_MAIN   , "freezepoof"      , 300 , 0	},
	[EFFECT_CHANGEWEAPON]     = {SOUNDBANK_MAIN   , "changeweapon"    , 300 , 0	},
	[EFFECT_MENUCHANGE]       = {SOUNDBANK_MAIN   , "menuchange"      , 1200, 0	},
	[EFFECT_GIANTFOOTSTEP]    = {SOUNDBANK_MAIN   , "giantfootstep"   , 1500, 0	},

	[EFFECT_LOGOAMBIENCE]     = {SOUNDBANK_MENU   , "ambience"        , 1200, 0	},
	[EFFECT_ACCENTDRONE1]     = {SOUNDBANK_MENU   , "accentdrone1"    , 1200, 0	},
	[EFFECT_ACCENTDRONE2]     = {SOUNDBANK_MENU   , "accentdrone2"    , 1200, 0	},

	[EFFECT_BONUSTELEPORT]    = {SOUNDBANK_BONUS  , "teleporthuman"   , 5000, 0	},
	[EFFECT_POINTBEEP]        = {SOUNDBANK_BONUS  , "pointbeep"       , 5000, 0	},
	[EFFECT_BONUSTRACTORBEAM] = {SOUNDBANK_BONUS  , "tractorbeam"     , 5000, 0	},
	[EFFECT_BONUSROCKET]      = {SOUNDBANK_BONUS  , "rocket"          , 5000, 0	},

	[EFFECT_POPCORN]          = {SOUNDBANK_FARM   , "popcornpop"      , 2000, 0	},
	[EFFECT_SHOOTCORN]        = {SOUNDBANK_FARM   , "shootcorn"       , 2000, 0	},
	[EFFECT_METALGATEHIT]     = {SOUNDBANK_FARM   , "metalgatehit"    , 3000, 0	},
	[EFFECT_METALGATECRASH]   = {SOUNDBANK_FARM   , "metalgatecrash"  , 3000, 0	},
	[EFFECT_TRACTOR]          = {SOUNDBANK_FARM   , "tractor"         , 2000, 0	},
	[EFFECT_ONIONSWOOSH]      = {SOUNDBANK_FARM   , "onionswoosh"     , 2000, 0	},
	[EFFECT_WOODGATECRASH]    = {SOUNDBANK_FARM   , "woodgatesmash"   , 3000, 0	},
	[EFFECT_TOMATOJUMP]       = {SOUNDBANK_FARM   , "tomatojump"      , 3000, 0	},
	[EFFECT_TOMATOSPLAT]      = {SOUNDBANK_FARM   , "tomatosplat"     , 3000, 0	},
	[EFFECT_WOODDOORHIT]      = {SOUNDBANK_FARM   , "wooddoorhit"     , 2000, 0	},
	[EFFECT_ONIONSPLAT]       = {SOUNDBANK_FARM   , "onionsplat"      , 3000, 0	},
	[EFFECT_CORNCRUNCH]       = {SOUNDBANK_FARM   , "corncrunch"      , 3000, kSoundFlag_Unique	},

	[EFFECT_BUBBLEPOP]        = {SOUNDBANK_SLIME  , "bubblepop"       , 3000, 0	},
	[EFFECT_BLOBMOVE]         = {SOUNDBANK_SLIME  , "slimemonster"    , 400 , 0	},
	[EFFECT_SLIMEBOAT]        = {SOUNDBANK_SLIME  , "boat"            , 2000, 0	},
	[EFFECT_CRYSTALCRACK]     = {SOUNDBANK_SLIME  , "crystalcrack"    , 800 , 0	},
	[EFFECT_SLIMEPIPES]       = {SOUNDBANK_SLIME  , "pipes"           , 500 , 0	},
	[EFFECT_CRYSTALCRASH]     = {SOUNDBANK_SLIME  , "crystalcrash"    , 3000, 0	},
	[EFFECT_BUMPERBUBBLE]     = {SOUNDBANK_SLIME  , "bumperbubble"    , 5000, 0	},
	[EFFECT_SLIMEBOSSOPEN]    = {SOUNDBANK_SLIME  , "bossopen"        , 8000, 0	},
	[EFFECT_BLOBSHOOT]        = {SOUNDBANK_SLIME  , "blobshoot"       , 3000, 0	},
	[EFFECT_BLOBBEAMHUM]      = {SOUNDBANK_SLIME  , "beamhum"         , 3000, 0	},
	[EFFECT_SLIMEBOUNCE]      = {SOUNDBANK_SLIME  , "slimebounce"     , 3000, 0	},
	[EFFECT_SLIMEBOOM]        = {SOUNDBANK_SLIME  , "slimeboom"       , 6000, 0	},
	[EFFECT_BLOBBOSSBOOM]     = {SOUNDBANK_SLIME  , "blobbossboom"    , 4000, 0	},
	[EFFECT_AIRPUMP]          = {SOUNDBANK_SLIME  , "airpump"         , 4000, 0	},

	[EFFECT_PODBUZZ]          = {SOUNDBANK_APOC   , "podbuzz"         , 4000, 0	},
	[EFFECT_PODCRASH]         = {SOUNDBANK_APOC   , "podcrash"        , 3000, 0	},
	[EFFECT_MANHOLEBLAST]     = {SOUNDBANK_APOC   , "manholeblast"    , 2000, 0	},
	[EFFECT_PODWORM]          = {SOUNDBANK_APOC   , "podworm"         , 2000, 0	},
	[EFFECT_MANHOLEROLL]      = {SOUNDBANK_APOC   , "manholeroll"     , 900 , kSoundFlag_Unique		},
	[EFFECT_DOORCLANKOPEN]    = {SOUNDBANK_APOC   , "doorclankopen"   , 1000, 0	},
	[EFFECT_DOORCLANKCLOSE]   = {SOUNDBANK_APOC   , "doorclankclose"  , 1000, kSoundFlag_Unique		},
	[EFFECT_MINEEXPLODE]      = {SOUNDBANK_APOC   , "mineexplosion"   , 3000, kSoundFlag_Unique		},
	[EFFECT_MUTANTROBOTSHOOT] = {SOUNDBANK_APOC   , "mutantrobotshoot", 3000, 0	},
	[EFFECT_MUTANTGROWL]      = {SOUNDBANK_APOC   , "mutantgrowl"     , 2000, 0	},
	[EFFECT_PLAYERTELEPORT]   = {SOUNDBANK_APOC   , "teleport"        , 3000, 0	},
	[EFFECT_TELEPORTERDRONE]  = {SOUNDBANK_APOC   , "teleporterdrone" , 2000, 0	},
	[EFFECT_DEBRISSMASH]      = {SOUNDBANK_APOC   , "debrissmash"     , 4000, 0	},

	[EFFECT_CANNONFIRE]       = {SOUNDBANK_CLOUD  , "cannonfire"      , 4000, 0	},
	[EFFECT_BUMPERHIT]        = {SOUNDBANK_CLOUD  , "bumperhit"       , 900 , kSoundFlag_Unique		},
	[EFFECT_BUMPERHUM]        = {SOUNDBANK_CLOUD  , "bumpercarhum"    , 300 , 0	},
	[EFFECT_BUMPERPOLETAP]    = {SOUNDBANK_CLOUD  , "bumperpoletap"   , 1500, 0	},
	[EFFECT_BUMPERPOLEHUM]    = {SOUNDBANK_CLOUD  , "bumperpolehum"   , 100 , 0	},
	[EFFECT_BUMPERPOLEOFF]    = {SOUNDBANK_CLOUD  , "bumperpoleoff"   , 1500, 0	},
	[EFFECT_CLOWNBUBBLEPOP]   = {SOUNDBANK_CLOUD  , "clownbubblepop"  , 2500, 0	},
	[EFFECT_INFLATE]          = {SOUNDBANK_CLOUD  , "inflateballoon"  , 1500, 0	},
	[EFFECT_BOMBDROP]         = {SOUNDBANK_CLOUD  , "bombdrop"        , 200 , 0	},
	[EFFECT_BUMPERPOLEBREAK]  = {SOUNDBANK_CLOUD  , "bumperpolebreak" , 1500, 0	},
	[EFFECT_ROCKETSLED]       = {SOUNDBANK_CLOUD  , "rocketsled"      , 3000, 0	},
	[EFFECT_TRAPDOOR]         = {SOUNDBANK_CLOUD  , "trapdoor"        , 2000, 0	},
	[EFFECT_BALLOONPOP]       = {SOUNDBANK_CLOUD  , "balloonpop"      , 5000, 0	},
	[EFFECT_FALLYAA]          = {SOUNDBANK_CLOUD  , "yaaa"            , 5000, 0	},
	[EFFECT_BIRDBOMBBOOM]     = {SOUNDBANK_CLOUD  , "birdbombboom"    , 3000, 0	},
	[EFFECT_CONFETTIBOOM]     = {SOUNDBANK_CLOUD  , "confettiboom"    , 3000, 0	},
	[EFFECT_FISHBOOM]         = {SOUNDBANK_CLOUD  , "fishboom"        , 3000, 0	},

	[EFFECT_ACIDSIZZLE]       = {SOUNDBANK_JUNGLE , "acidsizzle"      , 40  , 0	},
	[EFFECT_LIZARDROAR]       = {SOUNDBANK_JUNGLE , "lizardroar"      , 3000, 0	},
	[EFFECT_FIREBREATH]       = {SOUNDBANK_JUNGLE , "firebreath"      , 2000, 0	},
	[EFFECT_LIZARDINHALE]     = {SOUNDBANK_JUNGLE , "inhale"          , 3000, 0	},
	[EFFECT_MANTISSPIT]       = {SOUNDBANK_JUNGLE , "spit"            , 2000, 0	},
	[EFFECT_BIGDOORSMASH]     = {SOUNDBANK_JUNGLE , "bigdoorsmash"    , 2000, 0	},
	[EFFECT_TRACTORBEAM]      = {SOUNDBANK_JUNGLE , "tractorbeam"     , 2000, 0	},
	[EFFECT_PITCHERPAIN]      = {SOUNDBANK_JUNGLE , "pitcherpain"     , 5000, 0	},
	[EFFECT_PITCHERPUKE]      = {SOUNDBANK_JUNGLE , "pitcherpuke"     , 5000, 0	},
	[EFFECT_FLYTRAP]          = {SOUNDBANK_JUNGLE , "flytrap"         , 600 , 0	},
	[EFFECT_PITCHERBOOM]      = {SOUNDBANK_JUNGLE , "pitcherboom"     , 5000, 0	},
	[EFFECT_PODSHOOT]         = {SOUNDBANK_JUNGLE , "podshoot"        , 2000, 0	},
	[EFFECT_PODBOOM]          = {SOUNDBANK_JUNGLE , "podboom"         , 5000, 0	},

	[EFFECT_VOLCANOBLOW]      = {SOUNDBANK_FIREICE, "volcanoblow"     , 5000, 0	},
	[EFFECT_METALHIT]         = {SOUNDBANK_FIREICE, "metalhit"        , 1000, kSoundFlag_Unique		},
	[EFFECT_SWINGERDRONE]     = {SOUNDBANK_FIREICE, "swingerdrone"    , 10  , 0	},
	[EFFECT_METALHIT2]        = {SOUNDBANK_FIREICE, "metalhit2"       , 2000, kSoundFlag_Unique		},
	[EFFECT_SAUCERHATCH]      = {SOUNDBANK_FIREICE, "saucerhatch"     , 4000, 0	},
	[EFFECT_ICECRACK]         = {SOUNDBANK_FIREICE, "icecrack"        , 4000, 0	},
	[EFFECT_ROBOTEXPLODE]     = {SOUNDBANK_FIREICE, "robotexplode"    , 4000, kSoundFlag_Unique		},
	[EFFECT_HAMMERSQUEAK]     = {SOUNDBANK_FIREICE, "hammersqueak"    , 50  , 0	},
	[EFFECT_DRILLBOTWHINE]    = {SOUNDBANK_FIREICE, "drillbotwhine"   , 500 , 0	},
	[EFFECT_DRILLBOTWHINEHI]  = {SOUNDBANK_FIREICE, "drillbotwhinehi" , 2000, 0	},
	[EFFECT_PILLARCRUNCH]     = {SOUNDBANK_FIREICE, "pillarcrunch"    , 2000, kSoundFlag_Unique		},
	[EFFECT_ROCKETSLED2]      = {SOUNDBANK_FIREICE, "rocketsled"      , 3000, 0	},
	[EFFECT_SPLATHIT]         = {SOUNDBANK_FIREICE, "splathit"        , 2000, 0	},
	[EFFECT_SQUOOSHYSHOOT]    = {SOUNDBANK_FIREICE, "squooshyshoot"   , 3000, 0	},
	[EFFECT_SLEDEXPLODE]      = {SOUNDBANK_FIREICE, "sledexplode"     , 3000, 0	},

	[EFFECT_SAUCERKABOOM]     = {SOUNDBANK_SAUCER , "mineexplosion"   , 3000, 0	},
	[EFFECT_SAUCERHIT]        = {SOUNDBANK_SAUCER , "saucerhit"       , 3000, 0	},

	[EFFECT_BRAINSTATIC]      = {SOUNDBANK_BRAIN  , "brainstatic"     , 1000, 0	},
	[EFFECT_BRAINBOSSSHOOT]   = {SOUNDBANK_BRAIN  , "brainbossshoot"  , 3000, 0	},
	[EFFECT_BRAINBOSSDIE]     = {SOUNDBANK_BRAIN  , "brainbossdie"    , 5000, 0	},
	[EFFECT_PORTALBOOM]       = {SOUNDBANK_BRAIN  , "portalboom"      , 3000, 0	},
	[EFFECT_BRAINPAIN]        = {SOUNDBANK_BRAIN  , "brainpain"       , 4000, 0	},

	[EFFECT_CONVEYORBELT]     = {SOUNDBANK_LOSE   , "conveyorbelt"    , 3000, 0	},
	[EFFECT_TRANSFORM]        = {SOUNDBANK_LOSE   , "transform"       , 3000, 0	},
};



static const AutoRumbleDef kAutoRumbleTable[] =
{
	[EFFECT_BADSELECT]        = {0,0},
	[EFFECT_SAUCER]           = {0,0},
	[EFFECT_STUNGUN]          = {0,0},//{0.2f, 150}, // --no auto rumble for this one because lv9 uses it all over the place
	[EFFECT_ZAP]              = {1.0f, 1000},
	[EFFECT_ROCKET]           = {0,0},
	[EFFECT_ROCKETLANDED]     = {0,0},
	[EFFECT_JUMPJET]          = {0.5f, 750},
	[EFFECT_SHATTER]          = {0,0},
	[EFFECT_WEAPONCLICK]      = {0,0},
	[EFFECT_WEAPONWHIR]       = {0,0},
	[EFFECT_NEWLIFE]          = {0,0},
	[EFFECT_FREEZEGUN]        = {0.2f, 150},
	[EFFECT_PUNCHHIT]         = {0.5f, 300},
	[EFFECT_PLAYERCRASH]      = {1.0f, 1000},
	[EFFECT_NOJUMPJET]        = {0,0},
	[EFFECT_PLAYERCRUSH]      = {1.0f, 350},
	[EFFECT_HEADSWOOSH]       = {0,0},
	[EFFECT_HEADTHUD]         = {1.0f, 250},
	[EFFECT_POWPODHIT]        = {0.5f, 300},
	[EFFECT_METALLAND]        = {0,0},
	[EFFECT_SERVO]            = {0,0},
	[EFFECT_LEFTFOOT]         = {0,0},
	[EFFECT_RIGHTFOOT]        = {0,0},
	[EFFECT_NOVACHARGE]       = {0,0},
	[EFFECT_TELEPORTHUMAN]    = {0.2f, 200},
	[EFFECT_CHECKPOINTHIT]    = {0,0},
	[EFFECT_CHECKPOINTLOOP]   = {0,0},
	[EFFECT_FLARESHOOT]       = {0.8f, 150},
	[EFFECT_FLAREEXPLODE]     = {0,0},
	[EFFECT_DARTWOOSH]        = {0.2f, 150},
	[EFFECT_ATOMCHIME]        = {0.2f, 100},
	[EFFECT_PLAYERCLANG]      = {1.0f, 250},
	[EFFECT_THROWNSWOOSH]     = {0,0},
	[EFFECT_BEAMHUM]          = {0,0},
	[EFFECT_JUMP]             = {0,0},
	[EFFECT_HEALTHWARNING]    = {0,0},
	[EFFECT_HATCH]            = {0,0},
	[EFFECT_BRAINWAVE]        = {0,0},
	[EFFECT_WEAPONDEPOSIT]    = {0,0},
	[EFFECT_BRAINDIE]         = {0,0},
	[EFFECT_LASERHIT]         = {0,0},
	[EFFECT_FLAREUP]          = {0,0},
	[EFFECT_FREEZEPOOF]       = {0,0},
	[EFFECT_CHANGEWEAPON]     = {0,0},
	[EFFECT_MENUCHANGE]       = {0,0},

	[EFFECT_LOGOAMBIENCE]     = {0,0},
	[EFFECT_ACCENTDRONE1]     = {0,0},
	[EFFECT_ACCENTDRONE2]     = {0,0},

	[EFFECT_BONUSTELEPORT]    = {0,0},
	[EFFECT_POINTBEEP]        = {0,0},
	[EFFECT_BONUSTRACTORBEAM] = {0,0},
	[EFFECT_BONUSROCKET]      = {0,0},

	[EFFECT_POPCORN]          = {0,0},
	[EFFECT_SHOOTCORN]        = {0,0},
	[EFFECT_METALGATEHIT]     = {0.6f, 300},
	[EFFECT_METALGATECRASH]   = {0.8f, 500},
	[EFFECT_TRACTOR]          = {0,0},
	[EFFECT_ONIONSWOOSH]      = {0,0},
	[EFFECT_WOODGATECRASH]    = {0.8f, 500},
	[EFFECT_TOMATOJUMP]       = {0,0},
	[EFFECT_TOMATOSPLAT]      = {0,0},
	[EFFECT_WOODDOORHIT]      = {0.6f, 300},
	[EFFECT_ONIONSPLAT]       = {0,0},
	[EFFECT_CORNCRUNCH]       = {0,0},

	[EFFECT_BUBBLEPOP]        = {0,0},
	[EFFECT_BLOBMOVE]         = {0,0},
	[EFFECT_SLIMEBOAT]        = {0,0},
	[EFFECT_CRYSTALCRACK]     = {0,0},
	[EFFECT_SLIMEPIPES]       = {0,0},
	[EFFECT_CRYSTALCRASH]     = {0,0},
	[EFFECT_BUMPERBUBBLE]     = {0,0},
	[EFFECT_SLIMEBOSSOPEN]    = {0,0},
	[EFFECT_BLOBSHOOT]        = {0,0},
	[EFFECT_BLOBBEAMHUM]      = {0,0},
	[EFFECT_SLIMEBOUNCE]      = {0.8f, 500},
	[EFFECT_SLIMEBOOM]        = {0.5f, 300},
	[EFFECT_BLOBBOSSBOOM]     = {0.6f, 200},
	[EFFECT_AIRPUMP]          = {0,0},

	[EFFECT_PODBUZZ]          = {0,0},
	[EFFECT_PODCRASH]         = {0,0},
	[EFFECT_MANHOLEBLAST]     = {0,0},
	[EFFECT_PODWORM]          = {0,0},
	[EFFECT_MANHOLEROLL]      = {0,0},
	[EFFECT_DOORCLANKOPEN]    = {0,0},
	[EFFECT_DOORCLANKCLOSE]   = {0,0},
	[EFFECT_MINEEXPLODE]      = {0.5f, 300},
	[EFFECT_MUTANTROBOTSHOOT] = {0,0},
	[EFFECT_MUTANTGROWL]      = {0,0},
	[EFFECT_PLAYERTELEPORT]   = {0,0},
	[EFFECT_TELEPORTERDRONE]  = {0,0},
	[EFFECT_DEBRISSMASH]      = {0.8f, 500},

	[EFFECT_CANNONFIRE]       = {0.8f, 500},
	[EFFECT_BUMPERHIT]        = {0,0},
	[EFFECT_BUMPERHUM]        = {0,0},
	[EFFECT_BUMPERPOLETAP]    = {0,0},
	[EFFECT_BUMPERPOLEHUM]    = {0,0},
	[EFFECT_BUMPERPOLEOFF]    = {0.8f, 300},
	[EFFECT_CLOWNBUBBLEPOP]   = {0,0},
	[EFFECT_INFLATE]          = {0,0},
	[EFFECT_BOMBDROP]         = {0,0},
	[EFFECT_BUMPERPOLEBREAK]  = {0.6f, 300},
	[EFFECT_ROCKETSLED]       = {0,0},
	[EFFECT_TRAPDOOR]         = {1.0f, 200},
	[EFFECT_BALLOONPOP]       = {0,0},
	[EFFECT_FALLYAA]          = {1.0f, 250},
	[EFFECT_BIRDBOMBBOOM]     = {0.5f, 300},
	[EFFECT_CONFETTIBOOM]     = {0,0},
	[EFFECT_FISHBOOM]         = {0,0},

	[EFFECT_ACIDSIZZLE]       = {0,0},
	[EFFECT_LIZARDROAR]       = {0,0},
	[EFFECT_FIREBREATH]       = {0,0},
	[EFFECT_LIZARDINHALE]     = {0,0},
	[EFFECT_MANTISSPIT]       = {0,0},
	[EFFECT_GIANTFOOTSTEP]    = {0.6f, 200},
	[EFFECT_BIGDOORSMASH]     = {0.8f, 600},
	[EFFECT_TRACTORBEAM]      = {0,0},
	[EFFECT_PITCHERPAIN]      = {0,0},
	[EFFECT_PITCHERPUKE]      = {0,0},
	[EFFECT_FLYTRAP]          = {0,0},
	[EFFECT_PITCHERBOOM]      = {1.0f, 1000},
	[EFFECT_PODSHOOT]         = {0,0},
	[EFFECT_PODBOOM]          = {0,0},

	[EFFECT_VOLCANOBLOW]      = {0,0},
	[EFFECT_METALHIT]         = {0,0},
	[EFFECT_SWINGERDRONE]     = {0,0},
	[EFFECT_METALHIT2]        = {1.0f, 250},
	[EFFECT_SAUCERHATCH]      = {0,0},
	[EFFECT_ICECRACK]         = {1.0f, 400},
	[EFFECT_ROBOTEXPLODE]     = {0,0},
	[EFFECT_HAMMERSQUEAK]     = {0,0},
	[EFFECT_DRILLBOTWHINE]    = {0,0},
	[EFFECT_DRILLBOTWHINEHI]  = {0.8f, 1500},
	[EFFECT_PILLARCRUNCH]     = {0.8f, 200},
	[EFFECT_ROCKETSLED2]      = {0,0},
	[EFFECT_SPLATHIT]         = {0,0},
	[EFFECT_SQUOOSHYSHOOT]    = {0,0},
	[EFFECT_SLEDEXPLODE]      = {1.0f, 500},

	[EFFECT_SAUCERKABOOM]     = {0.4f, 600},
	[EFFECT_SAUCERHIT]        = {0.6f, 200},

	[EFFECT_BRAINSTATIC]      = {0,0},
	[EFFECT_BRAINBOSSSHOOT]   = {0.5f, 300},
	[EFFECT_BRAINBOSSDIE]     = {0,0},
	[EFFECT_PORTALBOOM]       = {0.5f, 300},
	[EFFECT_BRAINPAIN]        = {0,0},

	[EFFECT_CONVEYORBELT]     = {0,0},
	[EFFECT_TRANSFORM]        = {0,0},
};



/********************* INIT SOUND TOOLS ********************/

void InitSoundTools(void)
{
	gMaxChannels = 0;

			/* INIT BANK INFO */

	memset(gLoadedEffects, 0, sizeof(gLoadedEffects));

			/******************/
			/* ALLOC CHANNELS */
			/******************/

			/* MAKE MUSIC CHANNEL */

	SndNewChannel(&gMusicChannel,sampledSynth,initStereo,nil);


			/* ALL OTHER CHANNELS */

	for (gMaxChannels = 0; gMaxChannels < MAX_CHANNELS; gMaxChannels++)
	{
			/* NEW SOUND CHANNEL */

		OSErr iErr = SndNewChannel(&gSndChannel[gMaxChannels], sampledSynth, initStereo, nil);
		GAME_ASSERT(!iErr);			// if err, stop allocating channels
	}


		/* LOAD DEFAULT SOUNDS */

	LoadSoundBank(SOUNDBANK_MAIN);
}


/******************** SHUTDOWN SOUND ***************************/
//
// Called at Quit time
//

void ShutdownSound(void)
{
int	i;

			/* STOP ANY PLAYING AUDIO */

	StopAllEffectChannels();
	KillSong();


		/* DISPOSE OF CHANNELS */

	for (i = 0; i < gMaxChannels; i++)
		SndDisposeChannel(gSndChannel[i], true);
	gMaxChannels = 0;


}

#pragma mark -

/******************* LOAD A SOUND EFFECT ************************/

void LoadSoundEffect(int effectNum)
{
char path[256];
FSSpec spec;
short refNum;
OSErr err;

	GAME_ASSERT_MESSAGE(effectNum >= 0 && effectNum < NUM_EFFECTS, "illegal effect number");

	LoadedEffect* loadedSound = &gLoadedEffects[effectNum];
	const EffectDef* effectDef = &kEffectsTable[effectNum];

	if (loadedSound->sndHandle)
	{
		// already loaded
		return;
	}

	snprintf(path, sizeof(path), ":audio:%s.sounds:%s.aiff", kSoundBankNames[effectDef->bank], effectDef->filename);

	err = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, path, &spec);
	GAME_ASSERT_MESSAGE(err == noErr, path);

	err = FSpOpenDF(&spec, fsRdPerm, &refNum);
	GAME_ASSERT_MESSAGE(err == noErr, path);

	loadedSound->sndHandle = Pomme_SndLoadFileAsResource(refNum);
	GAME_ASSERT_MESSAGE(loadedSound->sndHandle, path);

			/* CLOSE FILE */

	FSClose(refNum);

			/* GET OFFSET INTO IT */

	GetSoundHeaderOffset(loadedSound->sndHandle, &loadedSound->sndOffset);

			/* PRE-DECOMPRESS IT (Source port addition) */

	Pomme_DecompressSoundResource(&loadedSound->sndHandle, &loadedSound->sndOffset);
}

/******************* DISPOSE OF A SOUND EFFECT ************************/

void DisposeSoundEffect(int effectNum)
{
	GAME_ASSERT_MESSAGE(effectNum >= 0 && effectNum < NUM_EFFECTS, "illegal effect number");

	LoadedEffect* loadedSound = &gLoadedEffects[effectNum];

	if (loadedSound->sndHandle)
	{
		DisposeHandle((Handle) loadedSound->sndHandle);
		memset(loadedSound, 0, sizeof(LoadedEffect));
	}
}

/******************* LOAD SOUND BANK ************************/

void LoadSoundBank(int bankNum)
{
	StopAllEffectChannels();

			/****************************/
			/* LOAD ALL EFFECTS IN BANK */
			/****************************/

	for (int i = 0; i < NUM_EFFECTS; i++)
	{
		if (kEffectsTable[i].bank == bankNum)
		{
			LoadSoundEffect(i);
		}
	}
}

/******************** DISPOSE SOUND BANK **************************/

void DisposeSoundBank(int bankNum)
{
	StopAllEffectChannels();									// make sure all sounds are stopped before nuking any banks

			/* FREE ALL SAMPLES */

	for (int i = 0; i < NUM_EFFECTS; i++)
	{
		if (kEffectsTable[i].bank == bankNum)
		{
			DisposeSoundEffect(i);
		}
	}
}



/********************* STOP A CHANNEL **********************/
//
// Stops the indicated sound channel from playing.
//

void StopAChannel(short *channelNum)
{
SndCommand 	mySndCmd;
//SCStatus	theStatus;
short		c = *channelNum;

	if ((c < 0) || (c >= gMaxChannels))		// make sure its a legal #
		return;

//	myErr = SndChannelStatus(gSndChannel[c],sizeof(SCStatus),&theStatus);	// get channel info
//	if (theStatus.scChannelBusy)					// if channel busy, then stop it
	{

		mySndCmd.cmd = flushCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		SndDoImmediate(gSndChannel[c], &mySndCmd);

		mySndCmd.cmd = quietCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		SndDoImmediate(gSndChannel[c], &mySndCmd);
	}

	*channelNum = -1;
}


/********************* STOP A CHANNEL IF EFFECT NUM **********************/
//
// Stops the indicated sound channel from playing if it is still this effect #
//

void StopAChannelIfEffectNum(short *channelNum, short effectNum)
{
SndCommand 	mySndCmd;
//SCStatus	theStatus;
short		c = *channelNum;

	if ((c < 0) || (c >= gMaxChannels))				// make sure its a legal #
		return;

	if (gChannelInfo[c].effectNum != effectNum)		// make sure its the right effect
		return;


//	myErr = SndChannelStatus(gSndChannel[c],sizeof(SCStatus),&theStatus);	// get channel info
//	if (theStatus.scChannelBusy)					// if channel busy, then stop it
	{

		mySndCmd.cmd = flushCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		SndDoImmediate(gSndChannel[c], &mySndCmd);

		mySndCmd.cmd = quietCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		SndDoImmediate(gSndChannel[c], &mySndCmd);
	}

	*channelNum = -1;
}



/********************* STOP ALL EFFECT CHANNELS **********************/

void StopAllEffectChannels(void)
{
short		i;

	for (i=0; i < gMaxChannels; i++)
	{
		short	c;

		c = i;
		StopAChannel(&c);
	}
}


#pragma mark -

/******************** PLAY SONG ***********************/

void PlaySong(short songNum, int flags)
{
static const char*	songNames[NUM_SONGS] =
{
	":Audio:ThemeSong.aif",
	":Audio:FarmSong.aif",
	":Audio:SlimeSong.aif",
	":Audio:SlimeBossSong.aif",
	":Audio:ApocalypseSong.aif",
	":Audio:CloudSong.aif",
	":Audio:JungleSong.aif",
	":Audio:JungleBoss.aif",
	":Audio:FireIceSong.aif",
	":Audio:SaucerSong.aif",
	":Audio:BonusSong.aif",
	":Audio:HighScoreSong.aif",
	":Audio:BrainBossSong.aif",
	":Audio:LoseSong.aif",
	":Audio:WinSong.aif",
};

static const float	volumeTweaks[NUM_SONGS] =
{
	1.0,				// theme
	.8,					// farm
	.8,					// slime
	.7,					// slime boss
	.8,					// apoc
	1.0,				// cloud
	.6,					// jungle
	1.0,				// jungle boss
	.8,					// fireice
	1.1,				// saucer
	1.1,				// bonus
	1.0,				// highscore
	1.0,				// brain boss
	1.0,				// lose song
	1.0,				// win song
};


	if (songNum == gCurrentSong)					// see if this is already loaded
	{
		if (!gSongPlayingFlag && gGamePrefs.music)
		{
			SndPauseFilePlay(gMusicChannel);
			gSongPlayingFlag = true;
		}

		return;										// bail -- don't restart the song
	}


		/* ZAP ANY EXISTING SONG */

	KillSong();
	DoSoundMaintenance();


			/******************************/
			/* OPEN APPROPRIATE AIFF FILE */
			/******************************/

	short musicFileRefNum = -1;
	FSSpec spec;
	OSErr iErr;

	iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, songNames[songNum], &spec);
	GAME_ASSERT(!iErr);

	iErr = FSpOpenDF(&spec, fsRdPerm, &musicFileRefNum);
	GAME_ASSERT(!iErr);

	gCurrentSong = songNum;


			/*******************/
			/* START STREAMING */
			/*******************/

	SndCommand 		mySndCmd;

			/* RESET CHANNEL TO DEFAULT VALUES */

	uint16_t volumeTweakShort = (uint16_t)(volumeTweaks[songNum] * kFullVolume);

	/*
	mySndCmd.cmd = volumeCmd;										// set sound playback volume
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (volumeTweakShort<<16) | volumeTweakShort;
	SndDoImmediate(gMusicChannel, &mySndCmd);

	mySndCmd.cmd 		= freqCmd;									// modify the rate to change the frequency
	mySndCmd.param1 	= 0;
	mySndCmd.param2 	= kMiddleC;
	SndDoImmediate(gMusicChannel, &mySndCmd);
	*/


			/* START PLAYING FROM FILE */

	iErr = SndStartFilePlay(
			gMusicChannel,
			musicFileRefNum,
			0,
			/*STREAM_BUFFER_SIZE*/0,
			/*gMusicBuffer*/nil,
			nil,
			/*SongCompletionProc*/nil,
			true);
	GAME_ASSERT(!iErr);
	gSongPlayingFlag = true;

	FSClose(musicFileRefNum);		// close the file (Pomme decompresses entire song into memory)


			/* SET VOLUME ON STREAM */

	mySndCmd.cmd = volumeCmd;										// set sound playback volume
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (volumeTweakShort<<16) | volumeTweakShort;
	SndDoImmediate(gMusicChannel, &mySndCmd);


			/* SET LOOP FLAG ON STREAM */
			/* So we don't need to re-read the file over and over. */

	mySndCmd.cmd = pommeSetLoopCmd;
	mySndCmd.param1 = (!(flags & kPlaySong_PlayOnceFlag)) ? 1 : 0;
	mySndCmd.param2 = 0;
	iErr = SndDoImmediate(gMusicChannel, &mySndCmd);
	GAME_ASSERT_MESSAGE(!iErr, "PlaySong: SndDoImmediate (pomme loop extension) failed!");


			/* SEE IF WANT TO MUTE THE MUSIC */

	if ((flags & kPlaySong_PreloadFlag)
		|| !gGamePrefs.music)
	{
		SndPauseFilePlay(gMusicChannel);			// pause it
		gSongPlayingFlag = false;
	}
}



/*********************** KILL SONG *********************/

void KillSong(void)
{
	if (gCurrentSong < 0)												// no song loaded
		return;

	gCurrentSong = -1;
	gSongPlayingFlag = false;

	SndStopFilePlay(gMusicChannel, true);
}

/******************** TOGGLE MUSIC *********************/

void EnforceMusicPausePref(void)
{
	if (gGamePaused)
		return;

	SCStatus	theStatus;

	SndChannelStatus(gMusicChannel, sizeof(SCStatus), &theStatus);	// get channel info

	if (gGamePrefs.music == theStatus.scChannelPaused)
		SndPauseFilePlay(gMusicChannel);
}


#pragma mark -

/***************************** PLAY EFFECT 3D ***************************/
//
// NO SSP
//
// OUTPUT: channel # used to play sound
//

short PlayEffect3D(short effectNum, OGLPoint3D *where)
{
short					theChan;
uint32_t					leftVol, rightVol;

				/* CALC VOLUME */

	Calc3DEffectVolume(effectNum, where, 1.0, &leftVol, &rightVol);
	if ((leftVol+rightVol) == 0)
		return(-1);


	theChan = PlayEffect_Parms(effectNum, leftVol, rightVol, NORMAL_CHANNEL_RATE);

	if (theChan != -1)
		gChannelInfo[theChan].volumeAdjust = 1.0;			// full volume adjust

	return(theChan);									// return channel #
}



/***************************** PLAY EFFECT PARMS 3D ***************************/
//
// Plays an effect with parameters in 3D
//
// OUTPUT: channel # used to play sound
//

short PlayEffect_Parms3D(short effectNum, OGLPoint3D *where, uint32_t rateMultiplier, float volumeAdjust)
{
short			theChan;
uint32_t			leftVol, rightVol;

				/* CALC VOLUME */

	Calc3DEffectVolume(effectNum, where, volumeAdjust, &leftVol, &rightVol);
	if ((leftVol+rightVol) == 0)
		return(-1);


				/* PLAY EFFECT */

	theChan = PlayEffect_Parms(effectNum, leftVol, rightVol, rateMultiplier);

	if (theChan != -1)
		gChannelInfo[theChan].volumeAdjust = volumeAdjust;	// remember volume adjuster

	return(theChan);									// return channel #
}


/************************* UPDATE 3D SOUND CHANNEL ***********************/
//
// Returns TRUE if effectNum was a mismatch or something went wrong
//

Boolean Update3DSoundChannel(short effectNum, short *channel, OGLPoint3D *where)
{
//SCStatus		theStatus;
uint32_t			leftVol,rightVol;
short			c;

	c = *channel;

	if (c == -1)
		return(true);


			/* SEE IF SOUND HAS COMPLETED */

//	SndChannelStatus(gSndChannel[c],sizeof(SCStatus),&theStatus);	// get channel info
//	if (!theStatus.scChannelBusy)									// see if channel not busy
//	{
//		*channel = -1;												// this channel is now invalid
//		return(true);
//	}

			/* MAKE SURE THE SAME SOUND IS STILL ON THIS CHANNEL */

	if (effectNum != gChannelInfo[c].effectNum)
		goto gone;


			/* UPDATE THE THING */

	Calc3DEffectVolume(gChannelInfo[c].effectNum, where, gChannelInfo[c].volumeAdjust, &leftVol, &rightVol);
	if ((leftVol+rightVol) == 0)										// if volume goes to 0, then kill channel
	{
gone:
		StopAChannel(channel);
		return(false);
	}

	ChangeChannelVolume(c, leftVol, rightVol);

	return(false);
}

/******************** CALC 3D EFFECT VOLUME *********************/

static void Calc3DEffectVolume(short effectNum, OGLPoint3D *where, float volAdjust, uint32_t *leftVolOut, uint32_t *rightVolOut)
{
float	dist;
float	refDist,volumeFactor;
uint32_t	volume,left,right;
uint32_t	maxLeft,maxRight;

	dist 	= OGLPoint3D_Distance(where, &gEarCoords);		// calc dist to sound for pane 0

			/* DO VOLUME CALCS */

	refDist = kEffectsTable[effectNum].refDistance;			// get ref dist

	dist -= refDist;
	if (dist <= EPS)
		volumeFactor = 1.0f;
	else
	{
		volumeFactor = 1.0f / (dist * VOLUME_DISTANCE_FACTOR);
		if (volumeFactor > 1.0f)
			volumeFactor = 1.0f;
	}

	volume = (float)FULL_CHANNEL_VOLUME * volumeFactor * volAdjust;


	if (volume < 6)							// if really quiet, then just turn it off
	{
		*leftVolOut = *rightVolOut = 0;
		return;
	}

			/************************/
			/* DO STEREO SEPARATION */
			/************************/

	else
	{
		float		volF = (float)volume;
		OGLVector2D	earToSound,lookVec;
		float		dot,cross;

		maxLeft = maxRight = 0;

			/* CALC VECTOR TO SOUND */

		earToSound.x = where->x - gEarCoords.x;
		earToSound.y = where->z - gEarCoords.z;
		FastNormalizeVector2D(earToSound.x, earToSound.y, &earToSound, true);


			/* CALC EYE LOOK VECTOR */

		FastNormalizeVector2D(gEyeVector.x, gEyeVector.z, &lookVec, true);


			/* DOT PRODUCT  TELLS US HOW MUCH STEREO SHIFT */

		dot = 1.0f - fabs(OGLVector2D_Dot(&earToSound,  &lookVec));
		if (dot < 0.0f)
			dot = 0.0f;
		else
		if (dot > 1.0f)
			dot = 1.0f;


			/* CROSS PRODUCT TELLS US WHICH SIDE */

		cross = OGLVector2D_Cross(&earToSound,  &lookVec);


				/* DO LEFT/RIGHT CALC */

		if (cross > 0.0f)
		{
			left 	= volF + (volF * dot);
			right 	= volF - (volF * dot);
		}
		else
		{
			right 	= volF + (volF * dot);
			left 	= volF - (volF * dot);
		}


				/* KEEP MAX */

		if (left > maxLeft)
			maxLeft = left;
		if (right > maxRight)
			maxRight = right;

	}

	*leftVolOut = maxLeft;
	*rightVolOut = maxRight;
}



#pragma mark -

/******************* UPDATE LISTENER LOCATION ******************/
//
// Get ear coord for all local players
//

void UpdateListenerLocation(void)
{
OGLVector3D	v;

	v.x = gGameViewInfoPtr->cameraPlacement.pointOfInterest.x - gGameViewInfoPtr->cameraPlacement.cameraLocation.x;	// calc line of sight vector
	v.y = gGameViewInfoPtr->cameraPlacement.pointOfInterest.y - gGameViewInfoPtr->cameraPlacement.cameraLocation.y;
	v.z = gGameViewInfoPtr->cameraPlacement.pointOfInterest.z - gGameViewInfoPtr->cameraPlacement.cameraLocation.z;
	FastNormalizeVector(v.x, v.y, v.z, &v);

	gEarCoords.x = gGameViewInfoPtr->cameraPlacement.cameraLocation.x + (v.x * 300.0f);			// put ear coord in front of camera
	gEarCoords.y = gGameViewInfoPtr->cameraPlacement.cameraLocation.y + (v.y * 300.0f);
	gEarCoords.z = gGameViewInfoPtr->cameraPlacement.cameraLocation.z + (v.z * 300.0f);

	gEyeVector = v;
}


/***************************** PLAY EFFECT ***************************/
//
// OUTPUT: channel # used to play sound
//

short PlayEffect(short effectNum)
{
	return(PlayEffect_Parms(effectNum,FULL_CHANNEL_VOLUME,FULL_CHANNEL_VOLUME,NORMAL_CHANNEL_RATE));

}

/***************************** PLAY EFFECT PARMS ***************************/
//
// Plays an effect with parameters
//
// OUTPUT: channel # used to play sound
//

short PlayEffect_Parms(int effectNum, uint32_t leftVolume, uint32_t rightVolume, unsigned long rateMultiplier)
{
SndCommand 		mySndCmd;
SndChannelPtr	chanPtr;
short			theChan;
OSErr			myErr;
uint32_t			lv2,rv2;


			/* GET BANK & SOUND #'S FROM TABLE */

	LoadedEffect* sound = &gLoadedEffects[effectNum];

	GAME_ASSERT_MESSAGE(effectNum >= 0 && effectNum < NUM_EFFECTS, "illegal effect number");
	GAME_ASSERT_MESSAGE(sound->sndHandle, "effect wasn't loaded!");


			/* DON'T PLAY EFFECT MULTIPLE TIMES AT ONCE IF EFFECTS TABLE PREVENTS IT */
			// (Source port addition)

	theChan = sound->lastPlayedOnChannel;
	Byte flags = kEffectsTable[effectNum].flags;

	if (theChan >= 0
		&& (kSoundFlag_Unique & flags)
		&& gChannelInfo[theChan].effectNum == effectNum)
	{
		SCStatus theStatus;
		myErr = SndChannelStatus(gSndChannel[theChan], sizeof(SCStatus), &theStatus);
		if (myErr == noErr && theStatus.scChannelBusy)
		{
			if (sound->lastLoudness > leftVolume + rightVolume)	// don't interrupt louder effect
				return -1;
			else												// otherwise interrupt current effect, force replay
				StopAChannel(&theChan);
		}
	}

			/* LOOK FOR FREE CHANNEL */

	theChan = FindSilentChannel();
	if (theChan == -1)
	{
		return(-1);
	}

	// Remember channel # on which we played this effect
	sound->lastPlayedOnChannel = theChan;
	sound->lastLoudness = leftVolume + rightVolume;

	lv2 = (float)leftVolume * gGlobalVolume;							// amplify by global volume
	rv2 = (float)rightVolume * gGlobalVolume;


					/* GET IT GOING */

	chanPtr = gSndChannel[theChan];

	mySndCmd.cmd = flushCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd = quietCmd;
	mySndCmd.param1 = 0;
	mySndCmd.param2 = 0;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd = reInitCmd;										// interpolation setting
	mySndCmd.param1 = 0;
	mySndCmd.param2 = initStereo | ((flags & kSoundFlag_NoInterp) ? initNoInterp : 0);
	SndDoImmediate(chanPtr, &mySndCmd);

	GAME_ASSERT(sound->sndHandle);
	mySndCmd.cmd = bufferCmd;										// make it play
	mySndCmd.param1 = 0;
	mySndCmd.ptr = ((Ptr)*sound->sndHandle) + sound->sndOffset;		// pointer to SoundHeader
	myErr = SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd = volumeCmd;										// set sound playback volume
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (rv2<<16) | lv2;
	SndDoImmediate(chanPtr, &mySndCmd);


	mySndCmd.cmd 		= rateMultiplierCmd;						// modify the rate to change the frequency
	mySndCmd.param1 	= 0;
	mySndCmd.param2 	= rateMultiplier;
	SndDoImmediate(chanPtr, &mySndCmd);


			/* SET MY INFO */

	gChannelInfo[theChan].effectNum 	= effectNum;		// remember what effect is playing on this channel
	gChannelInfo[theChan].leftVolume 	= leftVolume;		// remember requested volume (not the adjusted volume!)
	gChannelInfo[theChan].rightVolume 	= rightVolume;

			/* AUTO-RUMBLE */

	if (kAutoRumbleTable[effectNum].force > 0)
	{
		Rumble(kAutoRumbleTable[effectNum].force, kAutoRumbleTable[effectNum].duration);
	}

	return(theChan);										// return channel #
}


#pragma mark -

#if 0
/****************** UPDATE GLOBAL VOLUME ************************/
//
// Call this whenever gGlobalVolume is changed.  This will update
// all of the sounds with the correct volume.
//

static void UpdateGlobalVolume(void)
{
int		c;

			/* ADJUST VOLUMES OF ALL CHANNELS REGARDLESS IF THEY ARE PLAYING OR NOT */

	for (c = 0; c < gMaxChannels; c++)
	{
		ChangeChannelVolume(c, gChannelInfo[c].leftVolume, gChannelInfo[c].rightVolume);
	}


			/* UPDATE SONG VOLUME */

	if (gSongPlayingFlag)
		SOURCE_PORT_MINOR_PLACEHOLDER(); //SetMovieVolume(gSongMovie, FloatToFixed16(gGlobalVolume) * SONG_VOLUME);

}
#endif


/*************** PAUSE ALL SOUND CHANNELS **************/

void PauseAllChannels(Boolean pause)
{
	SndCommand cmd = { .cmd = pause ? pommePausePlaybackCmd : pommeResumePlaybackCmd };

	for (int c = 0; c < gMaxChannels; c++)
	{
		SndDoImmediate(gSndChannel[c], &cmd);
	}

	SndDoImmediate(gMusicChannel, &cmd);
}


/*************** CHANGE CHANNEL VOLUME **************/
//
// Modifies the volume of a currently playing channel
//

void ChangeChannelVolume(short channel, uint32_t leftVol, uint32_t rightVol)
{
SndCommand 		mySndCmd;
SndChannelPtr	chanPtr;
uint32_t			lv2,rv2;

	if (channel < 0)									// make sure it's valid
		return;

	lv2 = (float)leftVol * gGlobalVolume;				// amplify by global volume
	rv2 = (float)rightVol * gGlobalVolume;

	chanPtr = gSndChannel[channel];						// get the actual channel ptr

	mySndCmd.cmd = volumeCmd;							// set sound playback volume
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (rv2<<16) | lv2;			// set volume left & right
	SndDoImmediate(chanPtr, &mySndCmd);

	gChannelInfo[channel].leftVolume = leftVol;				// remember requested volume (not the adjusted volume!)
	gChannelInfo[channel].rightVolume = rightVol;
}



/*************** CHANGE CHANNEL RATE **************/
//
// Modifies the frequency of a currently playing channel
//
// The Input Freq is a fixed-point multiplier, not the static rate via rateCmd.
// This function uses rateMultiplierCmd, so a value of 0x00020000 is x2.0
//

void ChangeChannelRate(short channel, long rateMult)
{
static	SndCommand 		mySndCmd;
static	SndChannelPtr	chanPtr;

	if (channel < 0)									// make sure it's valid
		return;

	chanPtr = gSndChannel[channel];						// get the actual channel ptr

	mySndCmd.cmd 		= rateMultiplierCmd;						// modify the rate to change the frequency
	mySndCmd.param1 	= 0;
	mySndCmd.param2 	= rateMult;
	SndDoImmediate(chanPtr, &mySndCmd);
}




#pragma mark -


/******************** DO SOUND MAINTENANCE *************/
//
// 		UpdateInput() must have already been called
//

void DoSoundMaintenance(void)
{
#if 0	// disabled because we can turn off music in-game via settings now
	if (gAllowAudioKeys)
	{
					/* SEE IF TOGGLE MUSIC */

		if (GetNewKeyState(SDL_SCANCODE_M))
		{
			ToggleMusic();
		}


				/* SEE IF CHANGE VOLUME */

		if (GetNewKeyState(SDL_SCANCODE_EQUALS))
		{
			gGlobalVolume += .5f * gFramesPerSecondFrac;
			UpdateGlobalVolume();
		}
		else
		if (GetNewKeyState(SDL_SCANCODE_MINUS))
		{
			gGlobalVolume -= .5f * gFramesPerSecondFrac;
			if (gGlobalVolume < 0.0f)
				gGlobalVolume = 0.0f;
			UpdateGlobalVolume();
		}
	}
#endif
}



/******************** FIND SILENT CHANNEL *************************/

static short FindSilentChannel(void)
{
short		theChan;
OSErr		myErr;
SCStatus	theStatus;

	for (theChan=0; theChan < gMaxChannels; theChan++)
	{
		myErr = SndChannelStatus(gSndChannel[theChan],sizeof(SCStatus),&theStatus);	// get channel info
		if (myErr)
			continue;
		if (!theStatus.scChannelBusy)					// see if channel not busy
		{
			return(theChan);
		}
	}

			/* NO FREE CHANNELS */

	return(-1);
}


/********************** IS EFFECT CHANNEL PLAYING ********************/

Boolean IsEffectChannelPlaying(short chanNum)
{
SCStatus	theStatus;

	SndChannelStatus(gSndChannel[chanNum],sizeof(SCStatus),&theStatus);	// get channel info
	return (theStatus.scChannelBusy);
}

