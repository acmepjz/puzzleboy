#pragma once

#include "SimpleScrollView.h"
#include "PuzzleBoyLevelData.h"
#include "UTF8-16.h"

class PuzzleBoyLevel;
class PuzzleBoyLevelFile;
class PushableBlock;
struct RecordItem;

class PuzzleBoyLevelView{
public:
	PuzzleBoyLevelView();
	~PuzzleBoyLevelView();

	void Draw();

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
	SimpleScrollView m_scrollView;

	u16string m_sPlayerName;

	//level file object. Warning: weak reference!!! don't delete!!!
	PuzzleBoyLevelFile* m_pDocument;

	//level checksum and best record in database
	unsigned char m_bCurrentChecksum[PuzzleBoyLevelData::ChecksumSize];
	int m_nCurrentBestStep;
	u8string m_sCurrentBestStepOwner;
private:
	std::vector<RecordItem> m_tCurrentBestRecord;

public:
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

	//ad-hoc: key config
	const int* m_nKey;
};

