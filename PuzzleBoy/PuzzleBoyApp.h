#pragma once

#include "GNUGetText.h"
#include "RecordManager.h"
#include "PuzzleBoyLevelData.h"
#include "UTF8-16.h"
#include "MT19937.h"

class PuzzleBoyLevel;
class PuzzleBoyLevelFile;
class PuzzleBoyLevelView;
class PushableBlock;

class PuzzleBoyApp{
public:
	PuzzleBoyApp();
	~PuzzleBoyApp();

	void Draw();

	bool LoadFile(const u8string& fileName);
	bool SaveFile(const u8string& fileName);

	void LoadConfig(const u8string& fileName);
	void SaveConfig(const u8string& fileName);

	bool LoadLocale();

	void DestroyGame();
	bool StartGame(int nPlayerCount);

	void OnTimer();
	bool OnKeyDown(int nChar,int nFlags);
	void OnKeyUp(int nChar,int nFlags);

	//nType: SDL_MOUSEBUTTONDOWN or SDL_MOUSEBUTTONUP or SDL_MOUSEMOTION
	void OnMouseEvent(int nFlags, int xMouse, int yMouse, int nType);
public:
	GNUGetText m_objGetText;
	MT19937 m_objMainRnd;

	RecordManager m_objRecordMgr;

	//variables load from config file
	int m_nAnimationSpeed;
	bool m_bShowGrid;
	bool m_bShowLines;
	u16string m_sPlayerName[2];

	bool m_bInternationalFont;
	u8string m_sLocale; ///< empty means automatic, "?" means disabled

	int m_nThreadCount; ///< 0-8, 0 means automatic

	//level file object
	PuzzleBoyLevelFile* m_pDocument;

	//default level for new game view
	int m_nCurrentLevel;

	//level view (e.g  multiplayer)
	std::vector<PuzzleBoyLevelView*> m_view;
};

extern PuzzleBoyApp *theApp;

#define _(X) (theApp->m_objGetText.GetText(X))
