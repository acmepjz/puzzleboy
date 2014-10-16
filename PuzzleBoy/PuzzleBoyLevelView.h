#pragma once

#include "SimpleScrollView.h"
#include "PuzzleBoyLevelData.h"
#include "MultiTouchManager.h"
#include "UTF8-16.h"

class PuzzleBoyEditToolbar;
class PuzzleBoyLevel;
class PuzzleBoyLevelFile;
class PushableBlock;
struct RecordItem;

class PuzzleBoyLevelView:public virtual MultiTouchView{
public:
	PuzzleBoyLevelView();
	~PuzzleBoyLevelView();

	void Draw();

	bool StartGame();
	void SaveEdit();
	void FinishGame();

	void Undo();
	void Redo();

	bool OnTimer();
	bool OnKeyDown(int nChar,int nFlags);
	void OnKeyUp(int nChar,int nFlags);

	void OnMultiGesture(float fx,float fy,float dx,float dy,float zoom) override;

	//nType: SDL_MOUSEBUTTONDOWN or SDL_MOUSEBUTTONUP or SDL_MOUSEMOTION
	void OnMouseEvent(int which,int state,int xMouse,int yMouse,int nFlags,int nType) override;

private:
	void EnterBlockEdit(int nIndex,bool bBackup);
	void FinishBlockEdit();
	void CancelBlockEdit();

public:
	SimpleScrollView m_scrollView;

private:
	PuzzleBoyEditToolbar *m_toolbar;

	//0=default, 1=with yes/no, 2=with fast forward/no
	int m_nScreenKeyboardType;

	int m_nScreenKeyboardPressedIndex;

	//position of additional screen keyboard (in client space)
	//0,1-yes 2,3-no
	int m_nScreenKeyboardPos[4];

public:
	u16string m_sPlayerName;

	//level file object. Warning: weak reference!!! don't delete!!!
	PuzzleBoyLevelFile* m_pDocument;

	//level checksum and best record in database
	unsigned char m_bCurrentChecksum[PuzzleBoyLevelData::ChecksumSize];
	int m_nCurrentBestStep;
	u8string m_sCurrentBestStepOwner;
	std::vector<RecordItem> m_tCurrentBestRecord;

private:
	//internal function, only used in game mode
	//0-7: up,down,left,right,undo,redo,switch,restart
	//64: apply new position
	//65: discard new position or skip demo
	bool InternalKeyDown(int keyIndex);

public:
	int m_nCurrentLevel;

#define PLAY_MODE 0
#define EDIT_MODE 1
#define TEST_MODE 2
#define READONLY_MODE 3
	short m_nMode;

	bool m_bPlayFromRecord;
	bool m_bSkipRecord;

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

