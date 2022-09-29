//
// SkeletonObj.h
//

#pragma once

enum
{
	SKELETON_TYPE_OTTO = 0,
	SKELETON_TYPE_FARMER,
	SKELETON_TYPE_BRAINALIEN,
	SKELETON_TYPE_ONION,
	SKELETON_TYPE_CORN,
	SKELETON_TYPE_TOMATO,
	SKELETON_TYPE_BLOB,
	SKELETON_TYPE_SLIMETREE,
	SKELETON_TYPE_BEEWOMAN,
	SKELETON_TYPE_SQUOOSHY,
	SKELETON_TYPE_FLAMESTER,
	SKELETON_TYPE_GIANTLIZARD,
	SKELETON_TYPE_FLYTRAP,
	SKELETON_TYPE_MANTIS,
	SKELETON_TYPE_TURTLE,
	SKELETON_TYPE_PODWORM,
	SKELETON_TYPE_MUTANT,
	SKELETON_TYPE_MUTANTROBOT,
	SKELETON_TYPE_SCIENTIST,
	SKELETON_TYPE_PITCHERPLANT,
	SKELETON_TYPE_CLOWN,
	SKELETON_TYPE_CLOWNFISH,
	SKELETON_TYPE_STRONGMAN,
	SKELETON_TYPE_SKIRTLADY,
	SKELETON_TYPE_ICECUBE,
	SKELETON_TYPE_ELITEBRAINALIEN,
	
	MAX_SKELETON_TYPES				
};




//===============================

extern	ObjNode	*MakeNewSkeletonObject(NewObjectDefinitionType *newObjDef);
extern	void DisposeSkeletonObjectMemory(SkeletonDefType *skeleton);
extern	void AllocSkeletonDefinitionMemory(SkeletonDefType *skeleton);
extern	void InitSkeletonManager(void);
void LoadASkeleton(Byte num);
extern	void FreeSkeletonFile(Byte skeletonType);
extern	void FreeAllSkeletonFiles(short skipMe);
extern	void FreeSkeletonBaseData(SkeletonObjDataType *data);
void FinalizeSkeletonObjectScale(ObjNode* theNode, float scale, long collisionReferenceAnim);
