//
// windows.h
//

extern	float			gGammaFadePercent;

//=================================


extern void	InitWindowStuff(void);
void MakeFadeEvent(Boolean fadeIn, float fadeSpeed);

extern	void GammaFadeOut(void);
extern	void GammaOn(void);

void Enter2D(void);
void Exit2D(void);

void SetFullscreenModeFromPrefs(void);
