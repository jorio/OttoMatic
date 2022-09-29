//
// windows.h
//

extern	float			gGammaFadeFrac;

//=================================


extern void	InitWindowStuff(void);
ObjNode* MakeFadeEvent(Boolean fadeIn, float fadeSpeed);

void OGL_FadeOutScene(void (*drawCall)(void), void (*moveCall)(void));

void Enter2D(void);
void Exit2D(void);

void SetFullscreenModeFromPrefs(void);
