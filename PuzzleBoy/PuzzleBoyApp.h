#pragma once

#include "GNUGetText.h"
#include "RecordManager.h"
#include "PuzzleBoyLevelData.h"
#include "MultiTouchManager.h"
#include "UTF8-16.h"
#include "MT19937.h"

class PuzzleBoyLevel;
class PuzzleBoyLevelFile;
class PuzzleBoyLevelView;
class PushableBlock;
class MultiTouchManager;

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

	bool OnTimer();
	bool OnKeyDown(int nChar,int nFlags);
	void OnKeyUp(int nChar,int nFlags);
public:
	GNUGetText m_objGetText;
	MT19937 m_objMainRnd;

	RecordManager m_objRecordMgr;

	//variables load from config file
	int m_nAnimationSpeed;
	bool m_bShowGrid;
	bool m_bShowLines;
	bool m_bInternationalFont;
	bool m_bAutoSave;
	u16string m_sPlayerName[2];

	u8string m_sLastFile;
	int m_nLastLevel;
	u8string m_sLastRecord;

	u8string m_sLocale; ///< empty means automatic, "?" means disabled

	int m_nThreadCount; ///< 0-8, 0 means automatic

	//0=normal
	//1=horizontal up-down
	//2=vertical up-down
	int m_nOrientation;

	int m_nKey[24];

	//level file object
	PuzzleBoyLevelFile* m_pDocument;

	//default level for new game view
	int m_nCurrentLevel;

	//level view (e.g  multiplayer)
	std::vector<PuzzleBoyLevelView*> m_view;

	//touch manager
	MultiTouchManager touchMgr;

	//resize time
	int m_nMyResizeTime;
};

extern PuzzleBoyApp *theApp;

#define _(X) (theApp->m_objGetText.GetText(X))
