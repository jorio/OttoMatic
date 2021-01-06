//
// file.h
//

#ifndef FILE_H
#define FILE_H


		/***********************/
		/* RESOURCE STURCTURES */
		/***********************/

			/* Hedr */

typedef struct
{
	int16_t				version;			// 0xaa.bb
	int16_t				numAnims;			// gNumAnims
	int16_t				numJoints;			// gNumJoints
	int16_t				num3DMFLimbs;		// gNumLimb3DMFLimbs
}SkeletonFile_Header_Type;

			/* Bone resource */
			//
			// matches BoneDefinitionType except missing 
			// point and normals arrays which are stored in other resources.
			// Also missing other stuff since arent saved anyway.

typedef struct
{
	int32_t				parentBone;			 		// index to previous bone
	unsigned char		name[32];					// text string name for bone
	OGLPoint3D			coord;						// absolute coord (not relative to parent!) 
	uint16_t			numPointsAttachedToBone;	// # vertices/points that this bone has
	uint16_t			numNormalsAttachedToBone;	// # vertex normals this bone has
	uint32_t			reserved[8];				// reserved for future use
}File_BoneDefinitionType;




			/* AnHd */
			
typedef struct
{
	Str32				animName;
	int16_t				numAnimEvents;
}SkeletonFile_AnimHeader_Type;


#define	MAX_HTTP_NOTES	1000


		/* PREFERENCES */
		
typedef struct
{
	Byte	difficulty;
	Boolean	showScreenModeDialog;
	int16_t	depth;
	int32_t		screenWidth;
	int32_t		screenHeight;
	double	hz;
	Byte	monitorNum;
	Byte	language;
	Boolean	playerRelControls;

#if 0 // srcport rm
	DateTimeRec	lastVersCheckDate;
	Byte	didThisNote[MAX_HTTP_NOTES];
	Boolean customerHasRegistered;		// not the serial number, but the customer info registration

	int		numHTTPReadFails;			// counter which determines how many times an HTTP read has failed - possible pirate hacking
#endif

	Boolean	anaglyph;
	Boolean	anaglyphColor;
	int16_t	anaglyphCalibrationRed;
	int16_t	anaglyphCalibrationGreen;
	int16_t	anaglyphCalibrationBlue;
	Boolean doAnaglyphChannelBalancing;

	Boolean	dontUseHID;
	HIDControlSettingsType	controlSettings;		

	uint32_t	reserved[8];
}PrefsType;



		/* SAVE PLAYER */
		
typedef struct
{
	Byte		numAgesCompleted;		// encode # ages in lower 4 bits, and stage in upper 4 bits
	Str255		playerName;
}SavePlayerType;

extern	PrefsType			gGamePrefs;



//=================================================

SkeletonDefType *LoadSkeletonFile(short skeletonType, OGLSetupOutputType *setupInfo);
extern	OSErr LoadPrefs(PrefsType *prefBlock);
void SavePrefs(void);

void LoadPlayfield(FSSpec *specPtr, OGLSetupOutputType *setupInfo);
void LoadLevelArt(OGLSetupOutputType *setupInfo);
OSErr DrawPictureIntoGWorld(FSSpec *myFSSpec, GWorldPtr *theGWorld, short depth);
void SetDefaultDirectory(void);

Boolean SaveGame(void);
Boolean LoadSavedGame(void);


#if 0 // srcport rm
void GetLibVersion(ConstStrFileNameParam libName, NumVersion *version);
#endif

void VerifyApplicationFileID(void);



#endif











