//
// windows.h
//

#define	USE_DSP			1
#define	ALLOW_FADE		(1 && USE_DSP)



extern	Boolean			gPlayFullScreen;
extern	GDHandle 		gGDevice;
extern	GrafPtr			gGameWindowGrafPtr;
extern	CGrafPtr		gDisplayContextGrafPtr;
extern	float			gGammaFadePercent;

//=================================


extern void	InitWindowStuff(void);
extern void	DumpGWorld2(GWorldPtr, WindowPtr, Rect *);
extern void	DoLockPixels(GWorldPtr);
pascal void DoBold (DialogPtr dlogPtr, short item);
pascal void DoOutline (DialogPtr dlogPtr, short item);
void MakeFadeEvent(Boolean fadeIn, float fadeSpeed);
void DumpGWorld2(GWorldPtr thisWorld, WindowPtr thisWindow,Rect *destRect);

extern	void CleanupDisplay(void);
extern	void GammaFadeOut(void);
extern	void GammaFadeIn(void);
extern	void GammaOn(void);
void GammaOff(void);

extern	void GameScreenToBlack(void);

void DoLockPixels(GWorldPtr world);
void DoScreenModeDialog(void);

void DoScreenModeDialog(void);

void Wait(u_long ticks);

void Enter2D(void);
void Exit2D(void);

void BuildControlMenu(WindowRef window, SInt32 controlSig, SInt32 id, Str255 textList[], short numItems, short defaultSelection);
