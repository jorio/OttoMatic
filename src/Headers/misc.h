//
// misc.h
//

#define SERIAL_LENGTH      12

extern	void ShowSystemErr(long err);
extern void	DoAlert(const char*);
extern void	DoFatalAlert(const char*);
void DoAssert(const char* msg, const char* file, int line);
extern unsigned char	*NumToHex(unsigned short);
extern unsigned char	*NumToHex2(unsigned long, short);
extern unsigned char	*NumToDec(unsigned long);
extern	void CleanQuit(void);
extern	void SetMyRandomSeed(unsigned long seed);
extern	unsigned long MyRandomLong(void);
extern	void FloatToString(float num, Str255 string);
extern	Handle	AllocHandle(long size);
extern	void *AllocPtr(long size);
void *AllocPtrClear(long size);
extern	void PStringToC(char *pString, char *cString);
float StringToFloat(Str255 textStr);
extern	void DrawCString(char *string);
extern	void InitMyRandomSeed(void);
extern	void VerifySystem(void);
extern	float RandomFloat(void);
u_short	RandomRange(unsigned short min, unsigned short max);
extern	void RegulateSpeed(short fps);
extern	void CopyPStr(ConstStr255Param	inSourceStr, StringPtr	outDestStr);
extern	void ShowSystemErr_NonFatal(long err);
void CalcFramesPerSecond(void);
Boolean IsPowerOf2(int num);
float RandomFloat2(void);

void CopyPString(Str255 from, Str255 to);

void SafeDisposePtr(Ptr ptr);
void MyFlushEvents(void);



int16_t SwizzleShort(int16_t *shortPtr);
int32_t SwizzleLong(int32_t *longPtr);
float SwizzleFloat(float *floatPtr);
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
