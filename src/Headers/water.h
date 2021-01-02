//
// water.h
//

#ifndef WATER_H
#define WATER_H

#define	MAX_WATER_POINTS	100			// note:  cannot change this without breaking data files!!

enum
{
	WATER_FLAG_FIXEDHEIGHT	= (1)
};

enum
{
	WATER_TYPE_BLUEWATER,
	WATER_TYPE_SOAP,
	WATER_TYPE_GREENWATER,
	WATER_TYPE_OIL,
	WATER_TYPE_JUNGLEWATER,
	WATER_TYPE_MUD,
	WATER_TYPE_RADIOACTIVE,
	WATER_TYPE_LAVA,
	
	NUM_WATER_TYPES
};


typedef struct
{
	u_short			type;							// type of water
	u_long			flags;							// flags
	long			height;							// height offset or hard-wired index
	short			numNubs;						// # nubs in water
	long			reserved;						// for future use
	OGLPoint2D		nubList[MAX_WATER_POINTS];		// nub list
	
	float			hotSpotX,hotSpotZ;				// hot spot coords
	Rect			bBox;							// bounding box of water area	
}WaterDefType;




//============================================

void PrimeWater(void);
void DisposeWater(void);
Boolean DoWaterCollisionDetect(ObjNode *theNode, float x, float y, float z, int *patchNum);
Boolean GetWaterY(float x, float z, float *y);


#endif


