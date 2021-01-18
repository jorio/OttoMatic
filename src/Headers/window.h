//
// windows.h
//

#define	USE_DSP			1
#define	ALLOW_FADE		(1 && USE_DSP)



extern	Boolean			gPlayFullScreen;
extern	float			gGammaFadePercent;

//=================================


extern void	InitWindowStuff(void);
extern void	DumpGWorld2(GWorldPtr, WindowPtr, Rect *);
extern void	DoLockPixels(GWorldPtr);
void MakeFadeEvent(Boolean fadeIn, float fadeSpeed);
void DumpGWorld2(GWorldPtr thisWorld, WindowPtr thisWindow,Rect *destRect);

extern	void GammaFadeOut(void);
extern	void GammaOn(void);

extern	void GameScreenToBlack(void);

void DoLockPixels(GWorldPtr world);

void Enter2D(void);
void Exit2D(void);

void SetFullscreenModeFromPrefs(void);
