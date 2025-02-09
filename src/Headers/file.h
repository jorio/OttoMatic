//
// file.h
//

#pragma once

#include "input.h"

#define NUM_SAVE_SLOTS 12

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
	Boolean	vsync;
	Boolean	uiCentering;
	Byte	uiScaleLevel;
	Byte	displayNumMinus1;
	Byte	antialiasingLevel;
	Boolean	music;
	Byte	language;
	Boolean	playerRelControls;
	Boolean	autoAlignCamera;
	Byte	mouseSensitivityLevel;
	Boolean	mouseControlsOtto;
	Boolean	snappyCameraControl;		// if false, vanilla momentum-y camera swinging
	Boolean	gamepadRumble;
	Byte	anaglyphMode;
	uint8_t	anaglyphCalibrationRed;
	uint8_t	anaglyphCalibrationGreen;
	uint8_t	anaglyphCalibrationBlue;
	Boolean doAnaglyphChannelBalancing;
	KeyBinding	remappableKeys[NUM_REMAPPABLE_NEEDS];
}PrefsType;

enum
{
	ANAGLYPH_OFF,
	ANAGLYPH_COLOR,
	ANAGLYPH_MONO,
};




		/* SAVE GAME */
		// READ IN FROM FILE!

typedef struct
{
	uint32_t	version;
	uint32_t	score;
	int16_t		realLevel;
	int16_t		numLives;
	float		health;
	float		jumpJet;
	uint64_t	timestamp;		// new in file version 0x0200
}SaveGameType;


//=================================================

SkeletonDefType *LoadSkeletonFile(short skeletonType);
OSErr LoadPrefs(void);
void SavePrefs(void);

void LoadPlayfield(FSSpec *specPtr);
void LoadLevelArt(void);

bool SaveGame(int saveSlot);
bool LoadSaveGameStruct(int saveSlot, SaveGameType* saveData);
bool LoadSavedGame(int saveSlot);

Ptr LoadDataFile(const char* path, long* outLength);
char* CSVIterator(char** csvCursor, bool* eolOut);
