//
// misc.h
//

extern	void ShowSystemErr(long err);
extern void	DoAlert(const char*);
void DoFatalAlert(const char*);
void DoAssert(const char* msg, const char* file, int line);
extern	void CleanQuit(void);
extern	void SetMyRandomSeed(unsigned long seed);
extern	unsigned long MyRandomLong(void);
extern	Handle	AllocHandle(long size);
extern	void *AllocPtr(long size);
void *AllocPtrClear(long size);
extern	void InitMyRandomSeed(void);
extern	void CheckPreferencesFolder(void);
extern	float RandomFloat(void);
u_short	RandomRange(unsigned short min, unsigned short max);
extern	void RegulateSpeed(short fps);
extern	void CopyPStr(ConstStr255Param	inSourceStr, StringPtr	outDestStr);
extern	void ShowSystemErr_NonFatal(long err);
void CalcFramesPerSecond(void);
Boolean IsPowerOf2(int num);
float RandomFloat2(void);
float RandomFloatRange(float a, float b);
int PositiveModulo(int value, unsigned int m);

void SafeDisposePtr(Ptr ptr);
void FlushPtrTracking(bool issueWarnings);

void MyFlushEvents(void);

int16_t SwizzleShort(int16_t *shortPtr);
int32_t SwizzleLong(int32_t *longPtr);
float SwizzleFloat(float *floatPtr);
uint64_t SwizzleULong64(uint64_t *longPtr);
uint32_t SwizzleULong(uint32_t *longPtr);
uint16_t SwizzleUShort(uint16_t *shortPtr);
void SwizzlePoint3D(OGLPoint3D *pt);
void SwizzleVector3D(OGLVector3D *pt);
void SwizzleUV(OGLTextureCoord *pt);


int16_t FSReadBEShort(short refNum);
uint16_t FSReadBEUShort(short refNum);
int32_t FSReadBELong(short refNum);
uint32_t FSReadBEULong(short refNum);
float FSReadBEFloat(short refNum);
