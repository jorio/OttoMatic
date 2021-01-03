/****************************/
/*   	ANIM2.C    	    	*/
/* (c)1996-2000 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "3dmath.h"

extern	float				gFramesPerSecondFrac;
extern	OGLPoint3D	gCoord;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void InterpolateKeyFrames(const JointKeyframeType *kf1, const JointKeyframeType *kf2,JointKeyframeType *interpKf,float currentTime);
static void GetModelMorphPosition(const SkeletonObjDataType *skeleton,long jointNum, JointKeyframeType *interpKf);
static short GetNextAnimEventAtTime(const SkeletonObjDataType *skeleton, float time);
static float CalcMaxKeyFrameTime(const SkeletonObjDataType *skeleton);
inline float	AccelerationPercent(float percent);
static void SetSkeletonAnimGuts(SkeletonObjDataType *skeleton, long animNum);


/****************************/
/*    CONSTANTS             */
/****************************/

enum
{
	ANIM_DIRECTION_FORWARD,
	ANIM_DIRECTION_BACKWARD
};


/*********************/
/*    VARIABLES      */
/*********************/

float	gAccelerationCurve[CURVE_SIZE];
Boolean	gDisableAnimSounds = false;


/***************** SET SKELETON ANIM TIME ************************/

void SetSkeletonAnimTime(SkeletonObjDataType *skeleton, float timeRatio)
{
SkeletonDefType	*def = skeleton->skeletonDefinition;
int				numAnimEvents, i, time;
AnimEventType	*events;

	time = skeleton->CurrentAnimTime = skeleton->MaxAnimTime * timeRatio;			// set time index

			/* SET ANIM EVENT INDEX BASED ON TIME */

	numAnimEvents = def->NumAnimEvents[skeleton->AnimNum];					// get # anim events for this anim
	events = &def->AnimEventsList[skeleton->AnimNum][0];					// point to list of events

	for (i = 0; i < numAnimEvents; i++)										// scan for first anim event which is at or after our current time index
	{
		if (events[i].time >= time)
		{
			skeleton->AnimEventIndex = i;
			break;
		}
	}
}


/*************** SET SKELETON ANIM ****************/
//
// Causes immediate commencement of indicated anim
//

void SetSkeletonAnim(SkeletonObjDataType *skeleton, long animNum)
{
	SetSkeletonAnimGuts(skeleton, animNum);

	GetModelCurrentPosition(skeleton);			// update matrices
}


/*************** SET SKELETON ANIM GUTS ****************/

static void SetSkeletonAnimGuts(SkeletonObjDataType *skeleton, long animNum)
{
	if (skeleton == nil)
		return;

	if (animNum >= skeleton->skeletonDefinition->NumAnims)
	{
		DoFatalAlert("SetSkeletonAnim: illegal animNum");
	}

	skeleton->LoopBackTime = 0;								// assume no SetMarker & it loops or zigzags to time 0:00
	skeleton->AnimNum = animNum;
	skeleton->AnimDirection = ANIM_DIRECTION_FORWARD;
	skeleton->AnimEventIndex = 0;
	skeleton->CurrentAnimTime = 0;
	skeleton->PauseTimer = 0;
	skeleton->MaxAnimTime = CalcMaxKeyFrameTime(skeleton);	// calc ending time
	skeleton->AnimHasStopped = false;
	skeleton->IsMorphing = false;
	skeleton->AnimSpeed = 1.0;
}



/*************** MORPH TO SKELETON ANIM ****************/
//
// Causes object to morph to indicated anim and then process anim
//
// INPUT: speed = speed of morph (bigger is faster)
//

void MorphToSkeletonAnim(SkeletonObjDataType *skeleton, long animNum, float speed)
{
long	j;
SkeletonDefType	*skeletonDef;

			/* SET THE USUAL STUFF FIRST */

	if (skeleton == nil)
		return;

	if (animNum >= skeleton->skeletonDefinition->NumAnims)
		DoFatalAlert("MorphToSkeletonAnim: bad anim #");

	SetSkeletonAnimGuts(skeleton,animNum);

	skeletonDef = skeleton->skeletonDefinition;



		/* NOW SET MORPHING STUFF */

	skeleton->IsMorphing = true;
	skeleton->MorphPercent = 0;
	skeleton->MorphSpeed = speed;

	for (j=0; j < skeletonDef->NumBones; j++)
	{
		skeleton->MorphStart[j] = skeleton->JointCurrentPosition[j];		// copy current position into MorphStart keyframe
		if (skeletonDef->JointKeyframes[j].numKeyFrames[animNum] > 0)
			skeleton->MorphEnd[j] = skeletonDef->JointKeyframes[j].keyFrames[animNum][0];	// copy 1st keyframe of next anim into end kf
		else
			skeleton->MorphEnd[j] = skeleton->JointCurrentPosition[j];		// or if none, make end same as current
	}

	GetModelCurrentPosition(skeleton);			// update matrices
}



/************** UPDATE SKELETON ANIMATION *******************/

void UpdateSkeletonAnimation(ObjNode *theNode)
{
Byte	eventType,animDirection,eventValue,loopCount;
short	animNum,animEventIndex;
float	currentTime,eventTime,loopbackTime;
SkeletonObjDataType	*skeleton;
SkeletonDefType	*skeletonDef;
float	fps;


	skeleton = theNode->Skeleton;								// get ptr to skeleton data
	if (skeleton == nil)
		return;
	skeletonDef = skeleton->skeletonDefinition;

	fps = gFramesPerSecondFrac;

				/* IF JUST GOT A MORPH POSITION, THEN UPDATE MORPH */

	if (skeleton->IsMorphing)
	{
		skeleton->MorphPercent += skeleton->MorphSpeed*fps;
		if (skeleton->MorphPercent >= 1.0f)								// see if done morphing
			skeleton->IsMorphing = false;
		goto  update_transforms;
	}

				/* GET SOME BASIC INFO */

	animNum = skeleton->AnimNum;
	animEventIndex = skeleton->AnimEventIndex;
	currentTime = skeleton->CurrentAnimTime;
	animDirection = skeleton->AnimDirection;
	loopbackTime = skeleton->LoopBackTime;


				/* INCREMENT TIME INDEX */

	if (skeleton->PauseTimer > 0.0f)													// actually, don't inc if paused
	{
		skeleton->PauseTimer -= gFramesPerSecondFrac;
	}
	else
	{
		if (animDirection == ANIM_DIRECTION_FORWARD)							// inc or dec time index
			currentTime += (30.0f*fps)*skeleton->AnimSpeed;						// next frame
		else																	// previous frame
		{
			currentTime -= (30.0f*fps)*skeleton->AnimSpeed;
			if (currentTime < loopbackTime)										// see if reached start
			{
				currentTime = loopbackTime+(loopbackTime-currentTime);
				switch(skeleton->EndMode)										// see how to handle this end
				{
					case	ANIMEVENT_TYPE_ZIGZAG:
							animDirection = ANIM_DIRECTION_FORWARD;
							if (loopbackTime == 0)
								animEventIndex = 0;
							else
								animEventIndex = GetNextAnimEventAtTime(skeleton,currentTime);
							break;

					default:
							skeleton->AnimHasStopped = true;					// anim has completed
				}
			}
		}
	}

			/* CHECK FOR ANIM EVENTS */

	loopCount = 0;														// we havnt hit a loop-based event yet

	while((animEventIndex < skeletonDef->NumAnimEvents[animNum]) &&
			(currentTime >= skeletonDef->AnimEventsList[animNum][animEventIndex].time))	// see if need to process one
	{
		eventTime = skeletonDef->AnimEventsList[animNum][animEventIndex].time;
		eventType = skeletonDef->AnimEventsList[animNum][animEventIndex].type;
		eventValue = skeletonDef->AnimEventsList[animNum][animEventIndex].value;

		switch(eventType)
		{
			case	ANIMEVENT_TYPE_STOP:
					skeleton->AnimHasStopped = true;						// anim has completed
					animEventIndex++;
					break;

			case	ANIMEVENT_TYPE_SETMARKER:
					animEventIndex++;
					loopbackTime = skeleton->LoopBackTime = eventTime;		// remember time to loop back to
					break;

			case	ANIMEVENT_TYPE_LOOP:
					loopCount++;
					if (loopbackTime != 0)
					{
						currentTime -= eventTime;
						currentTime += loopbackTime;
						animEventIndex = GetNextAnimEventAtTime(skeleton,currentTime);
					}
					else
					{
						if (currentTime != 0)						// error check for loops of 1 keyframe (same as stop)
						{
							currentTime -= eventTime;
							animEventIndex = 0;
						}
						else
						{
							skeleton->AnimHasStopped = true;		// loop is of duration 0 so treat it as a STOP
							animEventIndex++;
						}
					}
					break;


			case	ANIMEVENT_TYPE_ZIGZAG:
					loopCount++;
					animDirection = ANIM_DIRECTION_BACKWARD;
					currentTime -= (eventTime - currentTime);
					skeleton->EndMode = ANIMEVENT_TYPE_ZIGZAG;
					animEventIndex++;
					break;

			case	ANIMEVENT_TYPE_SETFLAG:
					if (eventValue >= MAX_FLAGS_IN_OBJNODE)
						DoFatalAlert("Error: ANIMEVENT_TYPE_SETFLAG > MAX_FLAGS_IN_OBJNODE!");
					theNode->Flag[eventValue] = true;
					animEventIndex++;
					break;

			case	ANIMEVENT_TYPE_CLEARFLAG:
					if (eventValue >= MAX_FLAGS_IN_OBJNODE)
						DoFatalAlert("Error: ANIMEVENT_TYPE_SETFLAG > MAX_FLAGS_IN_OBJNODE!");
					theNode->Flag[eventValue] = false;
					animEventIndex++;
					break;

			case	ANIMEVENT_TYPE_PLAYSOUND:
					if (!gDisableAnimSounds)
					{
						switch(eventValue)
						{
									/* ONION SWAP */
							case	0:
									PlayEffect_Parms3D(EFFECT_ONIONSWOOSH, &theNode->Coord, NORMAL_CHANNEL_RATE + (MyRandomLong()&0x3fff), 1.0);
									break;

									/* PLAYER FOOTSTEP */

							case	3:
							case	4:
									if (gPlayerInfo.scaleRatio > 1.0f)							// see if play giant footstep
									{
										DeformationType		defData;

										PlayEffect3D(EFFECT_GIANTFOOTSTEP, &gPlayerInfo.coord);


											/* MAKE DEFORMATION WAVE */

										defData.type 				= DEFORMATION_TYPE_RADIALWAVE;
										defData.amplitude 			= 30.0f;
										defData.radius 				= 0;
										defData.speed 				= 3000;
										defData.origin.x			= gPlayerInfo.coord.x;
										defData.origin.y			= gPlayerInfo.coord.z;
										defData.oneOverWaveLength 	= 1.0f / 400.0f;
										defData.radialWidth			= 900.0f;
										defData.decayRate			= 100.0f;
										NewSuperTileDeformation(&defData);
									}
									else														// normal footstep
									{
//										PlayEffect3D(EFFECT_LEFTFOOT + (eventValue-3), &gPlayerInfo.coord);
										PlayEffect_Parms3D(EFFECT_LEFTFOOT + (eventValue-3), &gPlayerInfo.coord, NORMAL_CHANNEL_RATE, .5);
									}

									break;

									/* PITCHER PLANT PUKE */

							case	5:
									PlayEffect_Parms3D(EFFECT_PITCHERPUKE, &theNode->Coord, NORMAL_CHANNEL_RATE, 2.0);
									break;

									/* FLYTRAP CHOMP */

							case	6:
									PlayEffect3D(EFFECT_FLYTRAP, &theNode->Coord);
									break;

						}
					}
					animEventIndex++;
					break;

			case	ANIMEVENT_TYPE_PAUSE:
					skeleton->PauseTimer = (float)eventValue / 30.0f;						// set timer
					currentTime = eventTime;												// set time to be exactly when we want it paused!
					animEventIndex++;
					break;


			default:
					animEventIndex++;

		}

		if (loopCount > 1)													// see if we may have entered an infinite loop due to single-frame loopback event occurences
			break;
	}


			/* UPDATE OBJ RECORD */

	skeleton->CurrentAnimTime = currentTime;
	skeleton->AnimDirection = animDirection;
	skeleton->AnimEventIndex = animEventIndex;


			/* UPDATE ALL OF THE TRANSFORMS & SUCH */
update_transforms:
	GetModelCurrentPosition(skeleton);

}


/****************** GET MODEL CURRENT POSITION ******************/
//
// This will calculate theNode->Skeleton->JointCurrentPosition for each joint based on theNode->Skeleton->CurrentAnimTime,
// and the keyframes in the model.
//

void GetModelCurrentPosition(SkeletonObjDataType *skeleton)
{
long			jointNum;
long			numKeyFrames;
long			keyFrameNum;
JointKeyframeType	*kfPtr;
long			animNum;
float			currentAnimTime;
SkeletonDefType	*skeletonDef;


	animNum = skeleton->AnimNum;								// get anim # currently running
	currentAnimTime = skeleton->CurrentAnimTime;				// get time index into currenly running anim
	skeletonDef = skeleton->skeletonDefinition;

	if (skeleton->JointsAreGlobal)								// dont bother if global
		return;


			/* GET INFO FOR EACH JOINT */

	for (jointNum = 0; jointNum < skeletonDef->NumBones; jointNum++)
	{
					/* SEE IF MORPHING */

		if (skeleton->IsMorphing)
		{
			GetModelMorphPosition(skeleton,jointNum,&skeleton->JointCurrentPosition[jointNum]);
		}
		else
		{
				/* SCAN KEYFRAMES FOR CURRENT TIME */

			numKeyFrames = skeletonDef->JointKeyframes[jointNum].numKeyFrames[animNum];
			if (numKeyFrames == 0)														// if 0 keyframes, then nothing should have a keyframe and there's nothing to get, so exit
				return;

			for (keyFrameNum = 0; keyFrameNum < numKeyFrames; keyFrameNum++)
			{
				kfPtr = &skeletonDef->JointKeyframes[jointNum].keyFrames[animNum][keyFrameNum];	//  point to this keyframe's data

					/* SEE IF FOUND EXACT KEYFRAME */

				if (kfPtr->tick == currentAnimTime)
				{
					skeleton->JointCurrentPosition[jointNum] = *kfPtr;			// set data & goto next joint
					goto update;
				}
					/* SEE IF GOT NEXT KEYFRAME */
				else
				if (kfPtr->tick > currentAnimTime)
				{
					if (keyFrameNum == 0)										// if it's the 1st keyframe, then just use it
						skeleton->JointCurrentPosition[jointNum] = *kfPtr;
					else
					{
										/* INTERPOLATE VALUES */

						InterpolateKeyFrames(&skeletonDef->JointKeyframes[jointNum].keyFrames[animNum][keyFrameNum-1],
											kfPtr,&skeleton->JointCurrentPosition[jointNum],currentAnimTime);
					}
					goto update;
				}
			}
					/* CURRENT TIME IS AFTER LAST KEYFRAME, SO USE LAST KEYFRAME */

			skeleton->JointCurrentPosition[jointNum] = skeletonDef->JointKeyframes[jointNum].keyFrames[skeleton->AnimNum][numKeyFrames-1];		// set data & goto next joint
		}

				/* UPDATE SKELETON VIEW */

update:
		UpdateJointTransforms(skeleton,jointNum);
	}
}


/*************** GET MODEL MORPH POSITION ***********************/
//
// Called by GetModelCurrentPosition if IsMorphing is set, in which case
// the current position is interpolated based on the 2 keyframes set in
// the morphing parameters.
//
// NOTE: Morphing currently only does linear interpolation
//

static void GetModelMorphPosition(const SkeletonObjDataType *skeleton,long jointNum, JointKeyframeType *interpKf)
{
const JointKeyframeType *kf1,*kf2;
float	k2Percent,k1Percent;

	kf1 = &skeleton->MorphStart[jointNum];
	kf2 = &skeleton->MorphEnd[jointNum];
	k2Percent = skeleton->MorphPercent;
	k1Percent = 1.0f - k2Percent;


				/* CALC NEW INTERPOLATED DATA */

	interpKf->coord.x = (kf1->coord.x * k1Percent) + (kf2->coord.x * k2Percent);
	interpKf->coord.y = (kf1->coord.y * k1Percent) + (kf2->coord.y * k2Percent);
	interpKf->coord.z = (kf1->coord.z * k1Percent) + (kf2->coord.z * k2Percent);
	interpKf->rotation.x = (kf1->rotation.x * k1Percent) + (kf2->rotation.x * k2Percent);
	interpKf->rotation.y = (kf1->rotation.y * k1Percent) + (kf2->rotation.y * k2Percent);
	interpKf->rotation.z = (kf1->rotation.z * k1Percent) + (kf2->rotation.z * k2Percent);

	if ((kf1->scale.x != 1.0f) ||	// (kf1->scale.y != 1.0f) || (kf1->scale.z != 1.0f) ||				// see if bother with scale
		(kf2->scale.x != 1.0f))		// || (kf2->scale.y != 1.0f) || (kf2->scale.z != 1.0f))
	{
		interpKf->scale.x = //(kf1->scale.x * k1Percent) + (kf2->scale.x * k2Percent);
		interpKf->scale.y = //(kf1->scale.y * k1Percent) + (kf2->scale.y * k2Percent);
		interpKf->scale.z = (kf1->scale.z * k1Percent) + (kf2->scale.z * k2Percent);
	}
	else
	{
		interpKf->scale.x =
		interpKf->scale.y =
		interpKf->scale.z = 1.0f;
	}
}



/******************** INTERPOLATE KEYFRAMES *************************/
//
// Given 2 keyframes, it interpolates the values inbetween based on theNode->Skeleton->CurrentAnimTime.
// NOTE: kf1 is assumed to be before kf2 in terms of time!
//
// INPUT: kf1/kf2 = input keyframes
//		  currentTime = time index into current animation
//
// OUTPUT: interpKf = output keyframe
//

static void InterpolateKeyFrames(const JointKeyframeType *kf1, const JointKeyframeType *kf2,
								 JointKeyframeType *interpKf,float currentTime)
{
float	time1,time2;
float	diffA,diffB;
float	k2Percent,k1Percent;
long	accMode;
const float	one = 1.0f;

				/* GET ACCELERATION MODE */

	accMode = kf1->accelerationMode;


				/* CALC K1/K2 RATIOS */

	time1 = kf1->tick;
	time2 = kf2->tick;

	diffA = time2-time1;							// calc time between kf1 & kf2
	diffB =	currentTime-time1;						// calc time from kf1 to current time
	k2Percent = diffB/diffA;						// calc % of k2 values
	k1Percent = one - k2Percent;					// calc % of k1 values


			/* HANDLE SPECIAL ACCELERATION MODES */

	switch(accMode)
	{
		case	ACCEL_MODE_EASEINOUT:
				k1Percent = AccelerationPercent(k1Percent);			// calc curve & adjust
				k2Percent = one - k1Percent;
				break;

		case	ACCEL_MODE_EASEIN:
				k1Percent = AccelerationPercent(.5f*k1Percent);			// calc curve & adjust
				k1Percent *= 2.0f;

				if (k1Percent > one)
					k1Percent = one;
				else
				if (k1Percent < 0.0f)
					k1Percent = 0.0f;

				k2Percent = 1.0f - k1Percent;
				break;

		case	ACCEL_MODE_EASEOUT:
				k2Percent = AccelerationPercent(.5f*k2Percent);			// calc curve & adjust
				k2Percent *= 2.0f;

				if (k2Percent > one)
					k2Percent = one;
				else
				if (k2Percent < 0.0f)
					k2Percent = 0.0f;

				k1Percent = one - k2Percent;
				break;
	}

				/* CALC NEW INTERPOLATED DATA */

	interpKf->coord.x = (kf1->coord.x * k1Percent) + (kf2->coord.x * k2Percent);
	interpKf->coord.y = (kf1->coord.y * k1Percent) + (kf2->coord.y * k2Percent);
	interpKf->coord.z = (kf1->coord.z * k1Percent) + (kf2->coord.z * k2Percent);
	interpKf->rotation.x = (kf1->rotation.x * k1Percent) + (kf2->rotation.x * k2Percent);
	interpKf->rotation.y = (kf1->rotation.y * k1Percent) + (kf2->rotation.y * k2Percent);
	interpKf->rotation.z = (kf1->rotation.z * k1Percent) + (kf2->rotation.z * k2Percent);

	if ((kf1->scale.x != one) || (kf2->scale.x != one))
	{
		interpKf->scale.x =
		interpKf->scale.y =
		interpKf->scale.z = (kf1->scale.z * k1Percent) + (kf2->scale.z * k2Percent);
	}
	else
	{
		interpKf->scale.x =
		interpKf->scale.y =
		interpKf->scale.z = one;
	}
}


/****************** CALC MAX KEYFRAME TIME *******************/
//
// returns the max tick/time value of a keyframe in current anim.
//

static float CalcMaxKeyFrameTime(const SkeletonObjDataType *skeleton)
{
Byte	jointNum,keyFrameNum;
long	time,maxTime;

	maxTime = 0;
	for (jointNum = 0; jointNum < skeleton->skeletonDefinition->NumBones; jointNum++)
	{
		for (keyFrameNum = 0; keyFrameNum < skeleton->skeletonDefinition->JointKeyframes[jointNum].numKeyFrames[skeleton->AnimNum]; keyFrameNum++)
		{
			time = skeleton->skeletonDefinition->JointKeyframes[jointNum].keyFrames[skeleton->AnimNum][keyFrameNum].tick;
			if (time > maxTime)
				maxTime = time;
		}
	}

	return(maxTime);
}


/******************** GET NEXT ANIM EVENT AT TIME ************************/
//
// Given a time, return the index to the next anim event from that time
//
// INPUT: time
// OUTPUT: index
//

static short GetNextAnimEventAtTime(const SkeletonObjDataType *skeleton, float time)
{
Byte	i,numEvents;

	numEvents = skeleton->skeletonDefinition->NumAnimEvents[skeleton->AnimNum];

	for (i = 0; i < numEvents; i++)
	{
		if (skeleton->skeletonDefinition->AnimEventsList[skeleton->AnimNum][i].time >= time)
			return(i);
	}
	return(0);
}

/*********************** CALC ACCELERATION SPLINE CURVE **********************/

void CalcAccelerationSplineCurve(void)
{
long	i;
float	x,n;

	for (i = 0; i < CURVE_SIZE; i++)
	{
		x = i;
	    x = (x - 0.0f)/(CURVE_SIZE - 0.0f); 		/* normalize to [0:1] */
		n = (x*x * (3.0f - 2.0f*x));

		gAccelerationCurve[i] = n;
	}
}



/******************** ACCELERATION PERCENT ***************************/

inline float	AccelerationPercent(float percent)
{
long	i;

	i = (long)((CURVE_SIZE-1)*percent);
	if (gAccelerationCurve[i] > 1.0f)
		DoFatalAlert(" gAccelerationCurve > 1.0");
	else
	if (gAccelerationCurve[i] < 0.0f)
		DoFatalAlert(" gAccelerationCurve < 0");

	if (percent > 1.0f)
		DoFatalAlert(" AccelerationPercent > 1.0");
	else
	if (percent < 0.0f)
		DoFatalAlert(" AccelerationPercent < 0");

	return(gAccelerationCurve[i]);
}

#pragma mark -


/********************* BURN SKELETON **************************/

void BurnSkeleton(ObjNode *theNode, float flameScale)
{
float	fps = gFramesPerSecondFrac;
NewParticleGroupDefType	groupDef;
NewParticleDefType	newParticleDef;
int		particleGroup,magicNum;
OGLVector3D			d;
OGLPoint3D			jointCoord[MAX_JOINTS],p;
int	numJoints,i;


		/* CALC COORDS OF EACH JOINT */

	numJoints = theNode->Skeleton->skeletonDefinition->NumBones;
	for (i = 0; i < numJoints; i++)
		FindCoordOfJoint(theNode, i, &jointCoord[i]);


			/* CREATE PARTICLE GROUP */

	theNode->ParticleTimer -= fps;
	if (theNode->ParticleTimer <= 0.0f)										// see if time to make particles
	{
		theNode->ParticleTimer += .05f;

		particleGroup 	= theNode->ParticleGroup;
		magicNum 		= theNode->ParticleMagicNum;

		if ((particleGroup == -1) || (!VerifyParticleGroupMagicNum(particleGroup, magicNum)))
		{
			theNode->ParticleMagicNum = magicNum = MyRandomLong();			// generate a random magic num

			groupDef.magicNum				= magicNum;
			groupDef.type					= PARTICLE_TYPE_FALLINGSPARKS;
			groupDef.flags					= PARTICLE_FLAGS_DONTCHECKGROUND;
			groupDef.gravity				= -200;
			groupDef.magnetism				= 0;
			groupDef.baseScale				= flameScale;
			groupDef.decayRate				=  .6;
			groupDef.fadeRate				= 1.2;
			groupDef.particleTextureNum		= PARTICLE_SObjType_Fire;
			groupDef.srcBlend				= GL_SRC_ALPHA;
			groupDef.dstBlend				= GL_ONE;
			theNode->ParticleGroup = particleGroup = NewParticleGroup(&groupDef);
		}

			/* ADD TO PARTICLE GROUP */

		if (particleGroup != -1)
		{
			for (i = 0; i < numJoints; i++)
			{
				p.x = jointCoord[i].x + RandomFloat2() * 30.0f;
				p.y = jointCoord[i].y + RandomFloat2() * 30.0f;
				p.z = jointCoord[i].z + RandomFloat2() * 30.0f;

				d.x = 0;
				d.y = 300.0f + RandomFloat() * 100.0f;
				d.z = 0;

				newParticleDef.groupNum		= particleGroup;
				newParticleDef.where		= &p;
				newParticleDef.delta		= &d;
				newParticleDef.scale		= 1.0f + RandomFloat() * .5f;
				newParticleDef.rotZ			= RandomFloat() * PI2;
				newParticleDef.rotDZ		= RandomFloat2() * 8.0f;
				newParticleDef.alpha		= .8f;
				if (AddParticleToGroup(&newParticleDef))
				{
					theNode->ParticleGroup = -1;
					break;
				}
			}
		}
	}
}
















