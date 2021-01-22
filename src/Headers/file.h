//
// file.h
//

#ifndef FILE_H
#define FILE_H

#include "input.h"

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



		/* PREFERENCES */

typedef struct
{
	Boolean	fullscreen;
	Boolean	antialiasing;
	Byte	language;
	Boolean	playerRelControls;
	Boolean	anaglyph;
	Boolean	anaglyphColor;
	uint8_t	anaglyphCalibrationRed;
	uint8_t	anaglyphCalibrationGreen;
	uint8_t	anaglyphCalibrationBlue;
	Boolean doAnaglyphChannelBalancing;
	KeyBinding	keys[NUM_CONTROL_NEEDS];
	char	lastFileDialogPath[4096];
}PrefsType;

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


#endif











