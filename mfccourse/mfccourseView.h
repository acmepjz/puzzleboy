// mfccourseView.h : CmfccourseView 类的接口
//


#pragma once

#include "Level.h"
#include "RecordManager.h"

class CPushableBlock;
class CLevelRecord;
class CLevel;
class CmfccourseDoc;

class CmfccourseView : public CView
{
protected: // 仅从序列化创建
	CmfccourseView();
	DECLARE_DYNCREATE(CmfccourseView)

// 属性
public:
	//level for recording video
	CLevel *m_objRecordingLevel;
	int m_nVideoWidth;
	int m_nVideoHeight;

	//level checksum and best record in database
	unsigned char m_bCurrentChecksum[CLevel::ChecksumSize];
	int m_nCurrentBestStep;
	CString m_sCurrentBestStepOwner;
	std::vector<RecordItem> m_tCurrentBestRecord;

private:
	int m_nCurrentLevel;
	bool m_bEditMode,m_bPlayFromRecord;
	int m_nCurrentTool;

	int m_nEditingBlockIndex;
	int m_nEditingBlockX,m_nEditingBlockY;
	int m_nEditingBlockDX,m_nEditingBlockDY;
	CPushableBlock *m_objBackupBlock;

	CDC m_dcMem; //double buffer device context
	CBitmap m_bmp;
	int m_bmpWidth,m_bmpHeight;
	CPrintInfo *m_objPrintInfo;

	//copy of current level for game play
	CLevel *m_objPlayingLevel;

	//currently replaying level replay
	CString m_sRecord;
	int m_nRecordIndex;

// 操作
public:
	CmfccourseDoc* GetDocument() const;
	bool StartGame();
	void FinishGame();
	void EnterBlockEdit(int nIndex,bool bBackup);
	void FinishBlockEdit();
	void CancelBlockEdit();

	int GetCurrentLevel() const {return m_nCurrentLevel;}

	bool BeginRecordingVideo(const CLevelRecord* frm);
	int DrawRecordingVideo(CDC* pDC);
	void EndRecordingVideo();

// 重写
public:
	virtual void OnDraw(CDC* pDC);  // 重写以绘制该视图
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// 实现
public:
	virtual ~CmfccourseView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void OnMouseEvent(UINT nFlags, CPoint point, bool IsMouseDown, bool IsMouseUp);

// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnUpdateEditEditmode(CCmdUI *pCmdUI);
	afx_msg void OnUpdateGameRestart(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditTools(CCmdUI *pCmdUI);
	afx_msg void OnEditEditmode();
	afx_msg void OnEditTools(UINT);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnGameRestart();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnEditProperties();
	afx_msg void OnLevelList(UINT nID);
	afx_msg void OnEditAddLevel();
	afx_msg void OnEditRemoveLevel();
	afx_msg void OnEditUndo();
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateEditUndo(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditRedo(CCmdUI *pCmdUI);
	afx_msg void OnUpdateEditMove(CCmdUI *pCmdUI);
	afx_msg void OnEditMove(UINT nID);
	afx_msg void OnEditClear();
	afx_msg void OnUpdateEditClear(CCmdUI *pCmdUI);
	afx_msg void OnToolbarDropdown(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnGameRecord();
	afx_msg void OnUpdateViewGrid(CCmdUI *pCmdUI);
	afx_msg void OnViewGrid();
	afx_msg void OnUpdateViewAnimationSpeed(CCmdUI *pCmdUI);
	afx_msg void OnViewAnimationSpeed(UINT nID);
	afx_msg void OnLevelDatabase();
	afx_msg void OnToolsImportSokoban();
};

#ifndef _DEBUG  // mfccourseView.cpp 中的调试版本
inline CmfccourseDoc* CmfccourseView::GetDocument() const
   { return reinterpret_cast<CmfccourseDoc*>(m_pDocument); }
#endif

