//
// misc.h
//

void DoAlert(const char* format, ...);
POMME_NORETURN void DoFatalAlert(const char* format, ...);
POMME_NORETURN void CleanQuit(void);

#define AllocHandle(size) NewHandle((size))
#define AllocPtr(size) ((void*) NewPtr((size)))
#define AllocPtrClear(size) ((void*) NewPtrClear((size)))
#define SafeDisposePtr(ptr) DisposePtr((ptr))

void SetMyRandomSeed(uint32_t seed);
uint32_t MyRandomLong(void);
float RandomFloat(void);
uint16_t RandomRange(unsigned short min, unsigned short max);
float RandomFloat2(void);
float RandomFloatRange(float a, float b);

void CheckPreferencesFolder(void);

void CalcFramesPerSecond(void);

Boolean IsPowerOf2(int num);
int PositiveModulo(int value, unsigned int m);

void MyFlushEvents(void);

int16_t SwizzleShort(const int16_t *shortPtr);
int32_t SwizzleLong(const int32_t *longPtr);
float SwizzleFloat(const float *floatPtr);
uint64_t SwizzleULong64(const uint64_t *longPtr);
uint32_t SwizzleULong(const uint32_t *longPtr);
uint16_t SwizzleUShort(const uint16_t *shortPtr);
void SwizzlePoint3D(OGLPoint3D *pt);
void SwizzleVector3D(OGLVector3D *pt);
void SwizzleUV(OGLTextureCoord *pt);

int16_t FSReadBEShort(short refNum);
uint16_t FSReadBEUShort(short refNum);
int32_t FSReadBELong(short refNum);
uint32_t FSReadBEULong(short refNum);
float FSReadBEFloat(short refNum);

#define GAME_ASSERT(condition) \
	do { \
		if (!(condition)) \
			DoFatalAlert("Assertion failed! %s:%d: %s", __func__, __LINE__, #condition); \
	} while(0)

#define GAME_ASSERT_MESSAGE(condition, message) \
	do { \
		if (!(condition)) \
			DoFatalAlert("Assertion failed! %s:%d: %s", __func__, __LINE__, message); \
	} while(0)

#define DECLARE_WORKBUF(buf, bufSize) char (buf)[256]; const int (bufSize) = 256
#define DECLARE_STATIC_WORKBUF(buf, bufSize) static char (buf)[256]; static const int (bufSize) = 256
