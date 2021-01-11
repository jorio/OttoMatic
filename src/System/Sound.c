/****************************/
/*     SOUND ROUTINES       */
/* (c)2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/***************/
/* EXTERNALS   */
/***************/

#include "3dmath.h"

extern	short		gMainAppRezFile;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	FSSpec				gDataSpec;
extern	float		gFramesPerSecondFrac,gGameWindowShrink;
extern	PrefsType			gGamePrefs;
extern	Boolean				gOSX;


/****************************/
/*    PROTOTYPES            */
/****************************/

static short FindSilentChannel(void);
static void Calc3DEffectVolume(short effectNum, OGLPoint3D *where, float volAdjust, u_long *leftVolOut, u_long *rightVolOut);
static void UpdateGlobalVolume(void);
//static pascal void CallBackFn (SndChannelPtr chan, SndCommand *cmd);		// srcport rm


/****************************/
/*    CONSTANTS             */
/****************************/

#define	ANNOUNCER_VOLUME	(FULL_CHANNEL_VOLUME * 4)
#define	SONG_VOLUME		3.0f

#define FloatToFixed16(a)      ((Fixed)((float)(a) * 0x000100L))		// convert float to 16bit fixed pt


#define		MAX_CHANNELS			40

#define		MAX_EFFECTS				70

#define     kNumBogusConverters     200000

typedef struct
{
	Byte	bank,sound;
	long	refDistance;
}EffectType;



#define	VOLUME_DISTANCE_FACTOR	.004f		// bigger == sound decays FASTER with dist, smaller = louder far away

/**********************/
/*     VARIABLES      */
/**********************/

float						gMoviesTaskTimer = 0;

float						gGlobalVolume = .3;

static OGLPoint3D					gEarCoords;			// coord of camera plus a tad to get pt in front of camera
static	OGLVector3D			gEyeVector;


static	SndListHandle		gSndHandles[MAX_SOUND_BANKS][MAX_EFFECTS];		// handles to ALL sounds
static  long				gSndOffsets[MAX_SOUND_BANKS][MAX_EFFECTS];

static	SndChannelPtr		gSndChannel[MAX_CHANNELS];
ChannelInfoType		gChannelInfo[MAX_CHANNELS];

static short				gMaxChannels = 0;

static	SndChannelPtr		gMusicChannel = nil;


static short				gNumSndsInBank[MAX_SOUND_BANKS] = {0,0,0};


Boolean						gSongPlayingFlag = false;
Boolean						gLoopSongFlag = true;
Boolean						gAllowAudioKeys = true;


static short				gMusicFileRefNum = 0x0ded;
Boolean				gMuteMusicFlag = false;
short				gCurrentSong = -1;

//Movie				gSongMovie = nil;

static CGrafPtr		gQTDummyPort = nil;


		/*****************/
		/* EFFECTS TABLE */
		/*****************/

static EffectType	gEffectsTable[] =
{
	SOUND_BANK_MAIN,SOUND_DEFAULT_BADSELECT,2000,			// EFFECT_BADSELECT
	SOUND_BANK_MAIN,SOUND_DEFAULT_SAUCER,1000,				// EFFECT_SAUCER
	SOUND_BANK_MAIN,SOUND_DEFAULT_STUNGUN,700,				// EFFECT_STUNGUN
	SOUND_BANK_MAIN,SOUND_DEFAULT_ZAP,2000,					// EFFECT_ZAP
	SOUND_BANK_MAIN,SOUND_DEFAULT_ROCKET,2500,				// EFFECT_ROCKET
	SOUND_BANK_MAIN,SOUND_DEFAULT_ROCKETLANDED,3000,		// EFFECT_ROCKETLANDED
	SOUND_BANK_MAIN,SOUND_DEFAULT_JUMPJET,2000,				// EFFECT_JUMPJET
	SOUND_BANK_MAIN,SOUND_DEFAULT_SHATTER,3000,				// EFFECT_SHATTER
	SOUND_BANK_MAIN,SOUND_DEFAULT_WEAPONCLICK,3000,			// EFFECT_WEAPONCLICK
	SOUND_BANK_MAIN,SOUND_DEFAULT_WEAPONWHIR,3000,			// EFFECT_WEAPONWHIR
	SOUND_BANK_MAIN,SOUND_DEFAULT_NEWLIFE,3000,				// EFFECT_NEWLIFE
	SOUND_BANK_MAIN,SOUND_DEFAULT_FREEZEGUN,3000,			// EFFECT_FREEZEGUN
	SOUND_BANK_MAIN,SOUND_DEFAULT_PUNCHHIT,1000,			// EFFECT_PUNCHHIT
	SOUND_BANK_MAIN,SOUND_DEFAULT_CRASH,1000,				// EFFECT_PLAYERCRASH
	SOUND_BANK_MAIN,SOUND_DEFAULT_NOJUMPJET,1000,			// EFFECT_NOJUMPJET
	SOUND_BANK_MAIN,SOUND_DEFAULT_PLAYERCRUSH,1400,			// EFFECT_PLAYERCRUSH
	SOUND_BANK_MAIN,SOUND_DEFAULT_HEADSWOOSH,1200,			// EFFECT_HEADSWOOSH
	SOUND_BANK_MAIN,SOUND_DEFAULT_HEADTHUD,1200,			// EFFECT_HEADTHUD
	SOUND_BANK_MAIN,SOUND_DEFAULT_POWPODHIT,400,			// EFFECT_POWPODHIT
	SOUND_BANK_MAIN,SOUND_DEFAULT_METALLAND,50,				// EFFECT_METALLAND
	SOUND_BANK_MAIN,SOUND_DEFAULT_SERVO,300,				// EFFECT_SERVO
	SOUND_BANK_MAIN,SOUND_DEFAULT_LEFTFOOT,40,				// EFFECT_LEFTFOOT
	SOUND_BANK_MAIN,SOUND_DEFAULT_RIGHTFOOT,40,				// EFFECT_RIGHTFOOT
	SOUND_BANK_MAIN,SOUND_DEFAULT_NOVACHARGE,4000,			// EFFECT_NOVACHARGE
	SOUND_BANK_MAIN,SOUND_DEFAULT_TELEPORTHUMAN,3000,		// EFFECT_TELEPORTHUMAN
	SOUND_BANK_MAIN,SOUND_DEFAULT_CHECKPOINTHIT,3000,		// EFFECT_CHECKPOINTHIT
	SOUND_BANK_MAIN,SOUND_DEFAULT_CHECKPOINTLOOP,800,		// EFFECT_CHECKPOINTLOOP
	SOUND_BANK_MAIN,SOUND_DEFAULT_FLARESHOOT,3000,			// EFFECT_FLARESHOOT
	SOUND_BANK_MAIN,SOUND_DEFAULT_FLAREEXPLODE,3000,		// EFFECT_FLAREEXPLODE
	SOUND_BANK_MAIN,SOUND_DEFAULT_DARTWOOSH,3000,			// EFFECT_DARTWOOSH
	SOUND_BANK_MAIN,SOUND_DEFAULT_ATOMCHIME,3000,			// EFFECT_ATOMCHIME
	SOUND_BANK_MAIN,SOUND_DEFAULT_PLAYERCLANG,2000,			// EFFECT_PLAYERCLANG
	SOUND_BANK_MAIN,SOUND_DEFAULT_THROWNSWOOSH,4000,		// EFFECT_THROWNSWOOSH
	SOUND_BANK_MAIN,SOUND_DEFAULT_BEAMHUM,1500,				// EFFECT_BEAMHUM
	SOUND_BANK_MAIN,SOUND_DEFAULT_JUMP,1000,				// EFFECT_JUMP
	SOUND_BANK_MAIN,SOUND_DEFAULT_HEALTHWARNING,1000,		// EFFECT_HEALTHWARNING
	SOUND_BANK_MAIN,SOUND_DEFAULT_HATCH,2000,				// EFFECT_HATCH
	SOUND_BANK_MAIN,SOUND_DEFAULT_BRAINWAVE,10,				// EFFECT_BRAINWAVE
	SOUND_BANK_MAIN,SOUND_DEFAULT_WEAPONDEPOSIT,2000,		// EFFECT_WEAPONDEPOSIT
	SOUND_BANK_MAIN,SOUND_DEFAULT_BRAINDIE,2000,			// EFFECT_BRAINDIE
	SOUND_BANK_MAIN,SOUND_DEFAULT_LASERHIT,1000,			// EFFECT_LASERHIT
	SOUND_BANK_MAIN,SOUND_DEFAULT_FLAREUP,1500,				// EFFECT_FLAREUP
	SOUND_BANK_MAIN,SOUND_DEFAULT_FREEZEPOOF,300,			// EFFECT_FREEZEPOOF
	SOUND_BANK_MAIN,SOUND_DEFAULT_CHANGEWEAPON,300,			// EFFECT_CHANGEWEAPON

	SOUND_BANK_MENU,SOUND_MENU_MENUCHANGE,1200,				// EFFECT_MENUCHANGE
	SOUND_BANK_MENU,SOUND_MENU_LOGOAMBIENCE,1200,			// EFFECT_LOGOAMBIENCE
	SOUND_BANK_MENU,SOUND_MENU_ACCENTDRONE1,1200,			// EFFECT_ACCENTDRONE1
	SOUND_BANK_MENU,SOUND_MENU_ACCENTDRONE2,1200,			// EFFECT_ACCENTDRONE2

	SOUND_BANK_BONUS,SOUND_BONUS_TELEPORT,5000,				// EFFECT_BONUSTELEPORT
	SOUND_BANK_BONUS,SOUND_BONUS_POINTBEEP,5000,			// EFFECT_POINTBEEP
	SOUND_BANK_BONUS,SOUND_BONUS_TRACTORBEAM,5000,			// EFFECT_BONUSTRACTORBEAM
	SOUND_BANK_BONUS,SOUND_BONUS_ROCKET,5000,				// EFFECT_BONUSROCKET

	SOUND_BANK_LEVELSPECIFIC,SOUND_FARM_POPCORN,2000,		// EFFECT_POPCORN
	SOUND_BANK_LEVELSPECIFIC,SOUND_FARM_SHOOTCORN,2000,		// EFFECT_SHOOTCORN
	SOUND_BANK_LEVELSPECIFIC,SOUND_FARM_METALGATEHIT,3000,	// EFFECT_METALGATEHIT
	SOUND_BANK_LEVELSPECIFIC,SOUND_FARM_METALGATECRASH,3000,	// EFFECT_METALGATECRASH
	SOUND_BANK_LEVELSPECIFIC,SOUND_FARM_TRACTOR,2000,		// EFFECT_TRACTOR
	SOUND_BANK_LEVELSPECIFIC,SOUND_FARM_ONIONSWOOSH,2000,		// EFFECT_ONIONSWOOSH
	SOUND_BANK_LEVELSPECIFIC,SOUND_FARM_WOODGATECRASH,3000,		// EFFECT_WOODGATECRASH
	SOUND_BANK_LEVELSPECIFIC,SOUND_FARM_TOMATOJUMP,3000,		// EFFECT_TOMATOJUMP
	SOUND_BANK_LEVELSPECIFIC,SOUND_FARM_TOMATOSPLAT,3000,		// EFFECT_TOMATOSPLAT
	SOUND_BANK_LEVELSPECIFIC,SOUND_FARM_WOODDORRHIT,2000,		// EFFECT_WOODDOORHIT
	SOUND_BANK_LEVELSPECIFIC,SOUND_FARM_ONIONSPLAT,3000,		// EFFECT_ONIONSPLAT
	SOUND_BANK_LEVELSPECIFIC,SOUND_FARM_CORNCRUNCH,3000,		// EFFECT_CORNCRUNCH

	SOUND_BANK_LEVELSPECIFIC,SOUND_SLIME_BUBBLEPOP,3000,		// EFFECT_BUBBLEPOP
	SOUND_BANK_LEVELSPECIFIC,SOUND_SLIME_BLOBMOVE,400,			// EFFECT_BLOBMOVE
	SOUND_BANK_LEVELSPECIFIC,SOUND_SLIME_BOAT,2000,				// EFFECT_SLIMEBOAT
	SOUND_BANK_LEVELSPECIFIC,SOUND_SLIME_CRYSTALCRACK,800,		// EFFECT_CRYSTALCRACK
	SOUND_BANK_LEVELSPECIFIC,SOUND_SLIME_PIPES,500,				// EFFECT_SLIMEPIPES
	SOUND_BANK_LEVELSPECIFIC,SOUND_SLIME_CRYSTALCRASH,3000,		// EFFECT_CRYSTALCRASH
	SOUND_BANK_LEVELSPECIFIC,SOUND_SLIME_BUMPERBUBBLE,5000,		// EFFECT_BUMPERBUBBLE
	SOUND_BANK_LEVELSPECIFIC,SOUND_SLIME_BOSSOPEN,8000,			// EFFECT_SLIMEBOSSOPEN
	SOUND_BANK_LEVELSPECIFIC,SOUND_SLIME_BLOBSHOOT,3000,		// EFFECT_BLOBSHOOT
	SOUND_BANK_LEVELSPECIFIC,SOUND_SLIME_BEAMHUM,3000,			// EFFECT_BLOBBEAMHUM
	SOUND_BANK_LEVELSPECIFIC,SOUND_SLIME_SLIMEBOUNCE,3000,		// EFFECT_SLIMEBOUNCE
	SOUND_BANK_LEVELSPECIFIC,SOUND_SLIME_SLIMEBOOM,6000,		// EFFECT_SLIMEBOOM
	SOUND_BANK_LEVELSPECIFIC,SOUND_SLIME_BLOBBOSSBOOM,4000,		// EFFECT_BLOBBOSSBOOM
	SOUND_BANK_LEVELSPECIFIC,SOUND_SLIME_AIRPUMP,4000,			// EFFECT_AIRPUMP

	SOUND_BANK_LEVELSPECIFIC,SOUND_APOC_PODBUZZ,4000,			// EFFECT_PODBUZZ
	SOUND_BANK_LEVELSPECIFIC,SOUND_APOC_PODCRASH,3000,			// EFFECT_PODCRASH
	SOUND_BANK_LEVELSPECIFIC,SOUND_APOC_MANHOLEBLAST,4000,		// EFFECT_MANHOLEBLAST
	SOUND_BANK_LEVELSPECIFIC,SOUND_APOC_PODWORM,2000,			// EFFECT_PODWORM
	SOUND_BANK_LEVELSPECIFIC,SOUND_APOC_MANHOLEROLL,4000,		// EFFECT_MANHOLEROLL
	SOUND_BANK_LEVELSPECIFIC,SOUND_APOC_DOORCLANKOPEN,1000,		// EFFECT_DOORCLANKOPEN
	SOUND_BANK_LEVELSPECIFIC,SOUND_APOC_DOORCLANKCLOSE,1000,	// EFFECT_DOORCLANKCLOSE
	SOUND_BANK_LEVELSPECIFIC,SOUND_APOC_MINEXPLODE,3000,		// EFFECT_MINEEXPLODE
	SOUND_BANK_LEVELSPECIFIC,SOUND_APOC_ROBOTSHOOT,3000,		// EFFECT_MUTANTROBOTSHOOT
	SOUND_BANK_LEVELSPECIFIC,SOUND_APOC_MUTANTGROWL,2000,		// EFFECT_MUTANTGROWL
	SOUND_BANK_LEVELSPECIFIC,SOUND_APOC_TELEPORT,3000,			// EFFECT_PLAYERTELEPORT
	SOUND_BANK_LEVELSPECIFIC,SOUND_APOC_TELEPORTERDRONE,2000,	// EFFECT_TELEPORTERDRONE
	SOUND_BANK_LEVELSPECIFIC,SOUND_APOC_DEBRISSMASH,4000,		// EFFECT_DEBRISSMASH

	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOUD_CANNONFIRE,4000,		// EFFECT_CANNONFIRE
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOUD_BUMPERHIT,900,			// EFFECT_BUMPERHIT
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOUD_BUMPERHUM,300,			// EFFECT_BUMPERHUM
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOUD_BUMPERPOLETAP,1500,	// EFFECT_BUMPERPOLETAP
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOUD_BUMPERPOLEHUM,100,		// EFFECT_BUMPERPOLEHUM
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOUD_BUMPERPOLEOFF,1500,	// EFFECT_BUMPERPOLEOFF
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOUD_CLOWNBUBBLEPOP,2500,	// EFFECT_CLOWNBUBBLEPOP
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOUD_INFLATE,1500,			// EFFECT_INFLATE
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOUD_BOMBDROP,200,			// EFFECT_BOMBDROP
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOUD_PUMPERPOLEBREAK,1500,	// EFFECT_BUMPERPOLEBREAK
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOUD_ROCKETSLED,3000,		// EFFECT_ROCKETSLED
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOUD_TRAPDOOR,2000,			// EFFECT_TRAPDOOR
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOUD_BALLOONPOP,5000,		// EFFECT_BALLOONPOP
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOUD_FALLYAA,5000,			// EFFECT_FALLYAA
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOUD_BIRDBOMBBOOM,3000,		// EFFECT_BIRDBOMBBOOM
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOUD_CONFETTIBBOOM,3000,	// EFFECT_CONFETTIBOOM
	SOUND_BANK_LEVELSPECIFIC,SOUND_CLOUD_FISHBOOM,3000,			// EFFECT_FISHBOOM


	SOUND_BANK_LEVELSPECIFIC,SOUND_JUNGLE_ACIDSIZZLE,40,		// EFFECT_ACIDSIZZLE
	SOUND_BANK_LEVELSPECIFIC,SOUND_JUNGLE_LIZARDROAR,3000,		// EFFECT_LIZARDROAR
	SOUND_BANK_LEVELSPECIFIC,SOUND_JUNGLE_FIREBREATH,2000,		// EFFECT_FIREBREATH
	SOUND_BANK_LEVELSPECIFIC,SOUND_JUNGLE_LIZARDINHALE,3000,	// EFFECT_LIZARDINHALE
	SOUND_BANK_LEVELSPECIFIC,SOUND_JUNGLE_SPIT,2000,			// EFFECT_MANTISSPIT
	SOUND_BANK_LEVELSPECIFIC,SOUND_JUNGLE_GIANTFOOTSTEP,1500,	// EFFECT_GIANTFOOTSTEP
	SOUND_BANK_LEVELSPECIFIC,SOUND_JUNGLE_BIGDOORSMASH,2000,	// EFFECT_BIGDOORSMASH
	SOUND_BANK_LEVELSPECIFIC,SOUND_JUNGLE_TRACTORBEAM,2000,		// EFFECT_TRACTORBEAM
	SOUND_BANK_LEVELSPECIFIC,SOUND_JUNGLE_PITCHERPAIN,5000,		// EFFECT_PITCHERPAIN
	SOUND_BANK_LEVELSPECIFIC,SOUND_JUNGLE_PITCHERPUKE,5000,		// EFFECT_PITCHERPUKE
	SOUND_BANK_LEVELSPECIFIC,SOUND_JUNGLE_FLYTRAP,600,			// EFFECT_FLYTRAP
	SOUND_BANK_LEVELSPECIFIC,SOUND_JUNGLE_PITCHERBOOM,5000,		// EFFECT_PITCHERBOOM
	SOUND_BANK_LEVELSPECIFIC,SOUND_JUNGLE_PODSHOOT,2000,		// EFFECT_PODSHOOT
	SOUND_BANK_LEVELSPECIFIC,SOUND_JUNGLE_PODBOOM,5000,			// EFFECT_PODBOOM

	SOUND_BANK_LEVELSPECIFIC,SOUND_FIREICE_VOLCANOBLOW,5000,	// EFFECT_VOLCANOBLOW
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIREICE_METALHIT,1000,		// EFFECT_METALHIT
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIREICE_SWINGERDRONE,10,		// EFFECT_SWINGERDRONE
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIREICE_METALHIT2,2000,		// EFFECT_METALHIT2
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIREICE_SAUCERHATCH,4000,	// EFFECT_SAUCERHATCH
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIREICE_ICECRACK,4000,		// EFFECT_ICECRACK
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIREICE_ROBOTEXPLODE,4000,	// EFFECT_ROBOTEXPLODE
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIREICE_HAMMERSQUEAK,50,		// EFFECT_HAMMERSQUEAK
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIREICE_DRILLBOTWHINE,500,	// EFFECT_DRILLBOTWHINE
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIREICE_DRILLBOTWHINEHI,2000,// EFFECT_DRILLBOTWHINEHI
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIREICE_PILLARCRUNCH,2000,	// EFFECT_PILLARCRUNCH
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIREICE_ROCKETSLED,3000,		// EFFECT_ROCKETSLED2
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIREICE_SPLATHIT,2000,		// EFFECT_SPLATHIT
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIREICE_SQUOOSHYSHOOT,3000,	// EFFECT_SQUOOSHYSHOOT
	SOUND_BANK_LEVELSPECIFIC,SOUND_FIREICE_SLEDEXPLODE,3000,	// EFFECT_SLEDEXPLODE

	SOUND_BANK_LEVELSPECIFIC,SOUND_SAUCER_KABOOM,3000,			// EFFECT_SAUCERKABOOM
	SOUND_BANK_LEVELSPECIFIC,SOUND_SAUCER_HIT,3000,				// EFFECT_SAUCERHIT

	SOUND_BANK_LEVELSPECIFIC,SOUND_BRAINBOSS_STATIC,1000,		// EFFECT_BRAINSTATIC
	SOUND_BANK_LEVELSPECIFIC,SOUND_BRAINBOSS_SHOOT,3000,		// EFFECT_BRAINBOSSSHOOT
	SOUND_BANK_LEVELSPECIFIC,SOUND_BRAINBOSS_DIE,5000,			// EFFECT_BRAINBOSSDIE
	SOUND_BANK_LEVELSPECIFIC,SOUND_BRAINBOSS_PORTAL,3000,		// EFFECT_PORTALBOOM
	SOUND_BANK_LEVELSPECIFIC,SOUND_BRAINBOSS_BRAINPAIN,4000,	// EFFECT_BRAINPAIN

	SOUND_BANK_LOSE,SOUND_LOSE_CONVEYORBELT,3000,				// EFFECT_CONVEYORBELT
	SOUND_BANK_LOSE,SOUND_LOSE_TRANSFORM,3000,					// EFFECT_TRANSFORM

};


/********************* INIT SOUND TOOLS ********************/

void InitSoundTools(void)
{
	gMaxChannels = 0;

			/* INIT BANK INFO */

	for (int i = 0; i < MAX_SOUND_BANKS; i++)
		gNumSndsInBank[i] = 0;

			/******************/
			/* ALLOC CHANNELS */
			/******************/

			/* MAKE MUSIC CHANNEL */

	SndNewChannel(&gMusicChannel,sampledSynth,initStereo,nil);


			/* ALL OTHER CHANNELS */

	for (gMaxChannels = 0; gMaxChannels < MAX_CHANNELS; gMaxChannels++)
	{
			/* NEW SOUND CHANNEL */

		OSErr iErr = SndNewChannel(&gSndChannel[gMaxChannels],sampledSynth,initMono+initNoInterp,/*NewSndCallBackUPP(CallBackFn)*/nil);
		GAME_ASSERT(!iErr);			// if err, stop allocating channels
	}


		/* LOAD DEFAULT SOUNDS */

	FSSpec			spec;
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":Audio:Main.sounds", &spec);
	LoadSoundBank(&spec, SOUND_BANK_MAIN);
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

/******************* LOAD SOUND BANK ************************/

void LoadSoundBank(FSSpec *spec, long bankNum)
{
short			srcFile1,numSoundsInBank,i;
OSErr			iErr;

	StopAllEffectChannels();

	if (bankNum >= MAX_SOUND_BANKS)
		DoFatalAlert("LoadSoundBank: bankNum >= MAX_SOUND_BANKS");

			/* DISPOSE OF EXISTING BANK */

	DisposeSoundBank(bankNum);


			/* OPEN APPROPRIATE REZ FILE */

	srcFile1 = FSpOpenResFile(spec, fsRdPerm);
	if (srcFile1 == -1)
		DoFatalAlert("LoadSoundBank: OpenResFile failed!");

			/****************************/
			/* LOAD ALL EFFECTS IN BANK */
			/****************************/

	UseResFile( srcFile1 );												// open sound resource fork
	numSoundsInBank = Count1Resources('snd ');							// count # snd's in this bank
	if (numSoundsInBank > MAX_EFFECTS)
		DoFatalAlert("LoadSoundBank: numSoundsInBank > MAX_EFFECTS");

	for (i=0; i < numSoundsInBank; i++)
	{
				/* LOAD SND REZ */

		gSndHandles[bankNum][i] = (SndListResource **)GetResource('snd ',BASE_EFFECT_RESOURCE+i);
		if (gSndHandles[bankNum][i] == nil)
		{
			iErr = ResError();
			DoAlert("LoadSoundBank: GetResource failed!");
			if (iErr == memFullErr)
				DoFatalAlert("LoadSoundBank: Out of Memory");
			else
				ShowSystemErr(iErr);
		}
		DetachResource((Handle)gSndHandles[bankNum][i]);				// detach resource from rez file & make a normal Handle

		HNoPurge((Handle)gSndHandles[bankNum][i]);						// make non-purgeable
		HLockHi((Handle)gSndHandles[bankNum][i]);

				/* GET OFFSET INTO IT */

		GetSoundHeaderOffset(gSndHandles[bankNum][i], &gSndOffsets[bankNum][i]);

				/* PRE-DECOMPRESS IT (Source port addition) */

		Pomme_DecompressSoundResource(&gSndHandles[bankNum][i], &gSndOffsets[bankNum][i]);
	}

	UseResFile(gMainAppRezFile );								// go back to normal res file
	CloseResFile(srcFile1);

	gNumSndsInBank[bankNum] = numSoundsInBank;					// remember how many sounds we've got
}


/******************** DISPOSE SOUND BANK **************************/

void DisposeSoundBank(short bankNum)
{
short	i;


	if (bankNum > MAX_SOUND_BANKS)
		return;

	StopAllEffectChannels();									// make sure all sounds are stopped before nuking any banks

			/* FREE ALL SAMPLES */

	for (i=0; i < gNumSndsInBank[bankNum]; i++)
		DisposeHandle((Handle)gSndHandles[bankNum][i]);


	gNumSndsInBank[bankNum] = 0;
}



/********************* STOP A CHANNEL **********************/
//
// Stops the indicated sound channel from playing.
//

void StopAChannel(short *channelNum)
{
SndCommand 	mySndCmd;
OSErr 		myErr;
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
		myErr = SndDoImmediate(gSndChannel[c], &mySndCmd);

		mySndCmd.cmd = quietCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		myErr = SndDoImmediate(gSndChannel[c], &mySndCmd);
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
OSErr 		myErr;
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
		myErr = SndDoImmediate(gSndChannel[c], &mySndCmd);

		mySndCmd.cmd = quietCmd;
		mySndCmd.param1 = 0;
		mySndCmd.param2 = 0;
		myErr = SndDoImmediate(gSndChannel[c], &mySndCmd);
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


/****************** WAIT EFFECTS SILENT *********************/

void WaitEffectsSilent(void)
{
short	i;
Boolean	isBusy;
SCStatus				theStatus;

	do
	{
		isBusy = 0;
		for (i=0; i < gMaxChannels; i++)
		{
			SndChannelStatus(gSndChannel[i],sizeof(SCStatus),&theStatus);	// get channel info
			isBusy |= theStatus.scChannelBusy;
		}
	}while(isBusy);
}

#pragma mark -

/******************** PLAY SONG ***********************/
//
// if songNum == -1, then play existing open song
//
// INPUT: loopFlag = true if want song to loop
//

void PlaySong(short songNum, Boolean loopFlag)
{
const char*	songNames[] =
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

float	volumeTweaks[]=
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

	if (songNum == gCurrentSong)					// see if this is already playing
		return;


		/* ZAP ANY EXISTING SONG */

	gCurrentSong 	= songNum;
	gLoopSongFlag 	= loopFlag;
	KillSong();
	DoSoundMaintenance();


			/******************************/
			/* OPEN APPROPRIATE AIFF FILE */
			/******************************/

	{
		FSSpec spec;
		OSErr iErr;

		iErr = FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, songNames[songNum], &spec);
		GAME_ASSERT(!iErr);

		iErr = FSpOpenDF(&spec, fsRdPerm, &gMusicFileRefNum);
		GAME_ASSERT(!iErr);
	}

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

	OSErr iErr = SndStartFilePlay(
			gMusicChannel,
			gMusicFileRefNum,
			0,
			/*STREAM_BUFFER_SIZE*/0,
			/*gMusicBuffer*/nil,
			nil,
			/*SongCompletionProc*/nil,
			true);
	GAME_ASSERT(!iErr);
	gSongPlayingFlag = true;


			/* SET VOLUME ON STREAM */

	mySndCmd.cmd = volumeCmd;										// set sound playback volume
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (volumeTweakShort<<16) | volumeTweakShort;
	SndDoImmediate(gMusicChannel, &mySndCmd);


			/* SET LOOP FLAG ON STREAM */
			/* So we don't need to re-read the file over and over. */

	mySndCmd.cmd = pommeSetLoopCmd;
	mySndCmd.param1 = loopFlag ? 1 : 0;
	mySndCmd.param2 = 0;
	iErr = SndDoImmediate(gMusicChannel, &mySndCmd);
	GAME_ASSERT_MESSAGE(!iErr, "PlaySong: SndDoImmediate (pomme loop extension) failed!");


			/* SEE IF WANT TO MUTE THE MUSIC */

	if (gMuteMusicFlag)
	{
//		if (gSongMovie)
//			StopMovie(gSongMovie);

	}
}



/*********************** KILL SONG *********************/

void KillSong(void)
{

	gCurrentSong = -1;

	if (!gSongPlayingFlag)
		return;

	gSongPlayingFlag = false;											// tell callback to do nothing

	SndStopFilePlay(gMusicChannel, true);
	if (gMusicFileRefNum == 0x0ded)
		DoAlert("KillSong: gMusicFileRefNum == 0x0ded");
	else
	{
		OSErr iErr = FSClose(gMusicFileRefNum);							// close the file
		if (iErr)
		{
			DoAlert("KillSong: FSClose failed!");
			ShowSystemErr_NonFatal(iErr);
		}
	}

	gMusicFileRefNum = 0x0ded;
}

/******************** TOGGLE MUSIC *********************/

void ToggleMusic(void)
{
	gMuteMusicFlag = !gMuteMusicFlag;
	SndPauseFilePlay(gMusicChannel);			// pause it
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
Byte					bankNum,soundNum;
u_long					leftVol, rightVol;

			/* GET BANK & SOUND #'S FROM TABLE */

	bankNum 	= gEffectsTable[effectNum].bank;
	soundNum 	= gEffectsTable[effectNum].sound;

	if (soundNum >= gNumSndsInBank[bankNum])					// see if illegal sound #
	{
		DoAlert("Illegal sound number!");
		ShowSystemErr(effectNum);
	}

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

short PlayEffect_Parms3D(short effectNum, OGLPoint3D *where, u_long rateMultiplier, float volumeAdjust)
{
short			theChan;
Byte			bankNum,soundNum;
u_long			leftVol, rightVol;

			/* GET BANK & SOUND #'S FROM TABLE */

	bankNum 	= gEffectsTable[effectNum].bank;
	soundNum 	= gEffectsTable[effectNum].sound;

	if (soundNum >= gNumSndsInBank[bankNum])					// see if illegal sound #
	{
		DoAlert("Illegal sound number!");
		ShowSystemErr(effectNum);
	}

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
u_long			leftVol,rightVol;
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

static void Calc3DEffectVolume(short effectNum, OGLPoint3D *where, float volAdjust, u_long *leftVolOut, u_long *rightVolOut)
{
float	dist;
float	refDist,volumeFactor;
u_long	volume,left,right;
u_long	maxLeft,maxRight;

	dist 	= OGLPoint3D_Distance(where, &gEarCoords);		// calc dist to sound for pane 0

			/* DO VOLUME CALCS */

	refDist = gEffectsTable[effectNum].refDistance;			// get ref dist

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

void UpdateListenerLocation(OGLSetupOutputType *setupInfo)
{
OGLVector3D	v;

	v.x = setupInfo->cameraPlacement.pointOfInterest.x - setupInfo->cameraPlacement.cameraLocation.x;	// calc line of sight vector
	v.y = setupInfo->cameraPlacement.pointOfInterest.y - setupInfo->cameraPlacement.cameraLocation.y;
	v.z = setupInfo->cameraPlacement.pointOfInterest.z - setupInfo->cameraPlacement.cameraLocation.z;
	FastNormalizeVector(v.x, v.y, v.z, &v);

	gEarCoords.x = setupInfo->cameraPlacement.cameraLocation.x + (v.x * 300.0f);			// put ear coord in front of camera
	gEarCoords.y = setupInfo->cameraPlacement.cameraLocation.y + (v.y * 300.0f);
	gEarCoords.z = setupInfo->cameraPlacement.cameraLocation.z + (v.z * 300.0f);

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

short  PlayEffect_Parms(short effectNum, u_long leftVolume, u_long rightVolume, unsigned long rateMultiplier)
{
SndCommand 		mySndCmd;
SndChannelPtr	chanPtr;
short			theChan;
Byte			bankNum,soundNum;
OSErr			myErr;
u_long			lv2,rv2;
static UInt32          loopStart, loopEnd;
//SoundHeaderPtr   sndPtr;		// srcport rm


			/* GET BANK & SOUND #'S FROM TABLE */

	bankNum = gEffectsTable[effectNum].bank;
	soundNum = gEffectsTable[effectNum].sound;

	if (soundNum >= gNumSndsInBank[bankNum])					// see if illegal sound #
	{
		DoAlert("Illegal sound number!");
		ShowSystemErr(effectNum);
	}

			/* LOOK FOR FREE CHANNEL */

	theChan = FindSilentChannel();
	if (theChan == -1)
	{
		return(-1);
	}

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

	mySndCmd.cmd = bufferCmd;										// make it play
	mySndCmd.param1 = 0;
	mySndCmd.param2 = ((long)*gSndHandles[bankNum][soundNum])+gSndOffsets[bankNum][soundNum];	// pointer to SoundHeader
	myErr = SndDoImmediate(chanPtr, &mySndCmd);
	if (myErr)
		return(-1);

	mySndCmd.cmd = volumeCmd;										// set sound playback volume
	mySndCmd.param1 = 0;
	mySndCmd.param2 = (rv2<<16) | lv2;
	myErr = SndDoImmediate(chanPtr, &mySndCmd);


	mySndCmd.cmd 		= rateMultiplierCmd;						// modify the rate to change the frequency
	mySndCmd.param1 	= 0;
	mySndCmd.param2 	= rateMultiplier;
	SndDoImmediate(chanPtr, &mySndCmd);


			/* SET MY INFO */

	gChannelInfo[theChan].effectNum 	= effectNum;		// remember what effect is playing on this channel
	gChannelInfo[theChan].leftVolume 	= leftVolume;		// remember requested volume (not the adjusted volume!)
	gChannelInfo[theChan].rightVolume 	= rightVolume;
	return(theChan);										// return channel #
}


#pragma mark -


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

/*************** CHANGE CHANNEL VOLUME **************/
//
// Modifies the volume of a currently playing channel
//

void ChangeChannelVolume(short channel, u_long leftVol, u_long rightVol)
{
SndCommand 		mySndCmd;
SndChannelPtr	chanPtr;
u_long			lv2,rv2;

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



		/* ALSO CHECK OPTIONS */


	if (GetNewKeyState(SDL_SCANCODE_F1))
	{
		DoGameSettingsDialog();
	}



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

















