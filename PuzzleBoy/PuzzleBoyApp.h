#pragma once

#include "GNUGetText.h"
#include "RecordManager.h"
#include "PuzzleBoyLevelData.h"
#include "UTF8-16.h"

class PuzzleBoyLevel;
class PuzzleBoyLevelFile;
class PushableBlock;

class PuzzleBoyApp{
public:
	PuzzleBoyApp();
	~PuzzleBoyApp();

	PuzzleBoyLevelFile* GetDocument() const {
		return m_pDocument;
	}

	void Draw();

	bool LoadFile(const u8string& fileName);
	bool SaveFile(const u8string& fileName);

	bool StartGame();
	void FinishGame();

	void Undo();
	void Redo();

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
	mutable PuzzleBoyLevelFile* m_pDocument;

	//level checksum and best record in database
	unsigned char m_bCurrentChecksum[PuzzleBoyLevelData::ChecksumSize];
	int m_nCurrentBestStep;
	u8string m_sCurrentBestStepOwner;
	std::vector<RecordItem> m_tCurrentBestRecord;

	int m_nCurrentLevel;
	bool m_bEditMode,m_bPlayFromRecord;
	int m_nCurrentTool;

	int m_nEditingBlockIndex;
	int m_nEditingBlockX,m_nEditingBlockY;
	int m_nEditingBlockDX,m_nEditingBlockDY;
	PushableBlock *m_objBackupBlock;

	//copy of current level for game play
	PuzzleBoyLevel *m_objPlayingLevel;

	//currently replaying level replay
	u8string m_sRecord;
	int m_nRecordIndex;

};

extern PuzzleBoyApp *theApp;

#define _(X) (theApp->m_objGetText.GetText(X))
