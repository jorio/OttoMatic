// LOCALIZATION.C
// (C) 2021 Iliyas Jorio
// This file is part of Otto Matic. https://github.com/jorio/ottomatic

#include "game.h"

#define MAX_STRINGS 256

static GameLanguageID	gCurrentStringsLanguage = LANGUAGE_ILLEGAL;
static Ptr				gStringsBuffer = nil;
static const char*		gStringsTable[MAX_STRINGS];

static const char kLanguageCodesISO639_1[MAX_LANGUAGES][3] =
{
	[LANGUAGE_ENGLISH	] = "en",
	[LANGUAGE_FRENCH	] = "fr",
	[LANGUAGE_GERMAN	] = "de",
	[LANGUAGE_SPANISH	] = "es",
	[LANGUAGE_ITALIAN	] = "it",
	[LANGUAGE_SWEDISH	] = "sv",
};

void LoadLocalizedStrings(GameLanguageID languageID)
{
	// Don't bother reloading strings if we've already loaded this language
	if (languageID == gCurrentStringsLanguage)
	{
		return;
	}

	// Free previous strings buffer
	if (gStringsBuffer != nil)
	{
		SafeDisposePtr(gStringsBuffer);
		gStringsBuffer = nil;
	}

	GAME_ASSERT(languageID >= 0);
	GAME_ASSERT(languageID < MAX_LANGUAGES);

	FSSpec spec;
	FSMakeFSSpec(gDataSpec.vRefNum, gDataSpec.parID, ":system:strings.tsv", &spec);

	short refNum;
	if (FSpOpenDF(&spec, fsRdPerm, &refNum) != noErr)
		DoFatalAlert("LoadTextFile: FSpOpenDF failed");

	long eof = 0;
	GetEOF(refNum, &eof);
	long count = eof;
	gStringsBuffer = AllocPtrClear(count + 1);		// +1 for final '\0'
	FSRead(refNum, &count, gStringsBuffer);
	FSClose(refNum);

	GAME_ASSERT(count == eof);

	for (int i = 0; i < MAX_STRINGS; i++)
		gStringsTable[i] = nil;

	int row = 0;
	int col = 0;

	char prevChar = '\0';
	char* currentString = gStringsBuffer;
	char* fallbackString = gStringsBuffer;

	for (int i = 0; i < count; i++)
	{
		char currChar = gStringsBuffer[i];

		if (currChar == '\t')
		{
			gStringsBuffer[i] = '\0';

			if (col == languageID)
			{
				gStringsTable[row] = currentString[0]? currentString: fallbackString;
			}

			currentString = &gStringsBuffer[i + 1];
			col++;
		}

		if (currChar == '\n' || currChar == '\r')
		{
			gStringsBuffer[i] = '\0';

			if (!(prevChar == '\r' && currChar == '\n'))	// Windows CR+LF
			{
				if (col == languageID)
				{
					gStringsTable[row] = currentString[0]? currentString: fallbackString;
				}

				row++;
				col = 0;
				GAME_ASSERT(row < MAX_STRINGS);
			}

			currentString = &gStringsBuffer[i + 1];
			fallbackString = currentString;
		}

		prevChar = currChar;
	}

//	for (int i = 0; i < MAX_STRINGS; i++)
//		printf("String #%d: %s\n", i, gStringsTable[i]);
}

const char* Localize(LocStrID stringID)
{
	if (!gStringsBuffer)
		return "STRINGS NOT LOADED!!";

	if (stringID < 0 || stringID >= MAX_STRINGS)
		return "ILLEGAL STRING ID!!";

	if (!gStringsTable[stringID])
		return "";

	return gStringsTable[stringID];
}

GameLanguageID GetBestLanguageIDFromSystemLocale(void)
{
	GameLanguageID languageID = LANGUAGE_ENGLISH;

#if !(SDL_VERSION_ATLEAST(2,0,14))
	#warning Please upgrade to SDL 2.0.14 or later for SDL_GetPreferredLocales. Will default to English for now.
#else
	SDL_Locale* localeList = SDL_GetPreferredLocales();
	if (!localeList)
		return languageID;

	for (SDL_Locale* locale = localeList; locale->language; locale++)
	{
		for (int i = 0; i < MAX_LANGUAGES; i++)
		{
			if (0 == strncmp(locale->language, kLanguageCodesISO639_1[i], 2))
			{
				languageID = i;
				goto foundLocale;
			}
		}
	}

foundLocale:
	SDL_free(localeList);
#endif

	return languageID;
}
