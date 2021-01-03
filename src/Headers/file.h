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
	short	version;			// 0xaa.bb
	short	numAnims;			// gNumAnims
	short	numJoints;			// gNumJoints
	short	num3DMFLimbs;		// gNumLimb3DMFLimbs
}SkeletonFile_Header_Type;

			/* Bone resource */
			//
			// matches BoneDefinitionType except missing 
			// point and normals arrays which are stored in other resources.
			// Also missing other stuff since arent saved anyway.
			
typedef struct
{
	long 				parentBone;			 		// index to previous bone
	unsigned char		name[32];					// text string name for bone
	OGLPoint3D			coord;						// absolute coord (not relative to parent!) 
	u_short				numPointsAttachedToBone;	// # vertices/points that this bone has
	u_short				numNormalsAttachedToBone;	// # vertex normals this bone has
	u_long				reserved[8];				// reserved for future use
}File_BoneDefinitionType;



			/* Joit */
			
typedef struct
{
	OGLVector3D		maxRot;						// max rot values of joint
	OGLVector3D		minRot;						// min rot values of joint
	long 			parentBone; 		// index to previous link joint definition
	unsigned char	name[32];						// text string name for joint
	long			limbIndex;					// index into limb list
}Joit_Rez_Type;




			/* AnHd */
			
typedef struct
{
	Str32	animName;			
	short	numAnimEvents;	
}SkeletonFile_AnimHeader_Type;


#define	MAX_HTTP_NOTES	1000


		/* PREFERENCES */
		
typedef struct
{
	Byte	difficulty;
	Boolean	showScreenModeDialog;
	short	depth;
	int		screenWidth;
	int		screenHeight;
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
	short	anaglyphCalibrationRed;
	short	anaglyphCalibrationGreen;
	short	anaglyphCalibrationBlue;
	Boolean doAnaglyphChannelBalancing;

	Boolean	dontUseHID;
	HIDControlSettingsType	controlSettings;		

	int		reserved[8];
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
extern	void	OpenGameFile(Str255 filename,short *fRefNumPtr, Str255 errString);
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











