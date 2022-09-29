//
// vaportrails.h
//


enum
{
	VAPORTRAIL_TYPE_COLORSTREAK,
	VAPORTRAIL_TYPE_SMOKECOLUMN
};

void InitVaporTrails(void);
void DisposeVaporTrails(void);
int CreateNewVaporTrail(ObjNode *owner, Byte ownerTrailNum, Byte trailType,
						const OGLPoint3D *startPoint, const OGLColorRGBA *color,
						float size, float decayRate, float segmentDist);
void DrawVaporTrails(void);
void MoveVaporTrails(void);
void AddToVaporTrail(short *trailNum, const OGLPoint3D *where, const OGLColorRGBA *color);
Boolean VerifyVaporTrail(short	trailNum, ObjNode *owner, short ownerIndex);
