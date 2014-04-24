#pragma once

#include "GNUGetText.h"
#include "RecordManager.h"
#include "PuzzleBoyLevelData.h"
#include "UTF8-16.h"

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

	void DestroyGame();
	bool StartGame(int nPlayerCount);

	void OnTimer();
	bool OnKeyDown(int nChar,int nFlags);
	void OnKeyUp(int nChar,int nFlags);

	//nType: SDL_MOUSEBUTTONDOWN or SDL_MOUSEBUTTONUP or SDL_MOUSEMOTION
	void OnMouseEvent(int nFlags, int xMouse, int yMouse, int nType);
public:
	GNUGetText m_objGetText;

	RecordManager m_objRecordMgr;
	int m_nAnimationSpeed;
	bool m_bShowGrid;
	bool m_bShowLines;
	u16string m_sPlayerName;

	//file name
	u8string m_sFileName;

	//level file object
	PuzzleBoyLevelFile* m_pDocument;

	//default level for new game view
	int m_nCurrentLevel;

	//level view (e.g  multiplayer)
	std::vector<PuzzleBoyLevelView*> m_view;
};

extern PuzzleBoyApp *theApp;

#define _(X) (theApp->m_objGetText.GetText(X))
