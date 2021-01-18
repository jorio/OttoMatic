//
// mainmenu.h
//

#define	SCORE_TEXT_SPACING	13.0f
#define	SCORE_DIGITS		9

void DoMainMenuScreen(void);
void DrawScoreText(const char *s, float x, float y, OGLSetupOutputType *info);
void DrawMainMenuCallback(OGLSetupOutputType *info);
