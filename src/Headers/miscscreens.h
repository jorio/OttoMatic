//
// miscscreens.h
//

#define	NUM_SCORES		15
#define	MAX_NAME_LENGTH	24

typedef struct
{
	char			name[MAX_NAME_LENGTH+1];
	uint32_t		score;
}HighScoreType;

#define	DARKEN_PANE_Z	450.0f

enum
{
	FILE_SCREEN_TYPE_LOAD,
	FILE_SCREEN_TYPE_SAVE,
};


void PausedUpdateCallback(void);
void DoPaused(void);

void DoLegalScreen(void);
void DoMainMenuScreen(void);
int DoLevelCheatDialog(void (*backgroundDrawRoutine)(void));
void DoLevelIntro(void);
void DoBonusScreen(void);

void NewScore(void);
ObjNode* SetupHighScoreTableObjNodes(ObjNode* chainTail, int returnNth);
void LoadHighScores(void);
void ClearHighScores(void);

void DoLoseScreen(void);
void DoWinScreen(void);

void DrawDarkenPane(ObjNode *theNode);
void MoveCredits(ObjNode *text);

void DoSettingsOverlay(void (*updateRoutine)(void),
					   void (*backgroundDrawRoutine)(void));

bool DoFileScreen(int fileScreenType, void (*backgroundDrawRoutine)(void));

void DoWarpCheat(void);

