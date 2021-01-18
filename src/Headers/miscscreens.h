//
// miscscreens.h
//

#define	NUM_SCORES		15
#define	MAX_NAME_LENGTH	15

typedef struct
{
	char			name[MAX_NAME_LENGTH+1];
	uint32_t		score;
}HighScoreType;

#define	DARKEN_PANE_Z	450.0f

enum
{
	LOSE_SObjType_GameOver,
	WIN_SObjType_TheEnd_Text,
	WIN_SObjType_TheEnd_Glow,
	WIN_SObjType_BlackOut,
	WIN_SObjType_QText,
	WIN_SObjType_QGlow
};



void DisplayPicture(FSSpec *spec);
void DoPaused(void);

void DoLegalScreen(void);
Boolean DoLevelCheatDialog(void);
void DoLevelIntro(void);
void DoBonusScreen(void);
void DoGameSettingsDialog(void);

void NewScore(void);
void LoadHighScores(void);
void ClearHighScores(void);

void DoLoseScreen(void);
void DoWinScreen(void);

void DrawDarkenPane(ObjNode *theNode, const OGLSetupOutputType *setupInfo);
void MoveCredits(ObjNode *text);

void DoSettingsOverlay(void);
