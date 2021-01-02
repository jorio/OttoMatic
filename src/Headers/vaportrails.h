//
// vaportrails.h
//


enum
{
	VAPORTRAIL_TYPE_COLORSTREAK,
	VAPORTRAIL_TYPE_SMOKECOLUMN
};

void InitVaporTrails(void);
int CreatetNewVaporTrail(ObjNode *owner, Byte ownerTrailNum, Byte trailType,
						const OGLPoint3D *startPoint, const OGLColorRGBA *color,
						float size, float decayRate, float segmentDist);
void DrawVaporTrails(OGLSetupOutputType *setupInfo);
void MoveVaporTrails(void);
void AddToVaporTrail(short *trailNum, const OGLPoint3D *where, const OGLColorRGBA *color);
Boolean VerifyVaporTrail(short	trailNum, ObjNode *owner, short ownerIndex);
