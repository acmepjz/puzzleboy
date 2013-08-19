#pragma once
#include "afxcmn.h"
#include "afxwin.h"

class CmfccourseView;

// CLevelRecord 对话框

class CLevelRecord : public CDialog
{
	DECLARE_DYNAMIC(CLevelRecord)

public:
	CLevelRecord(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CLevelRecord();

// 对话框数据
	enum { IDD = IDD_RECORD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CmfccourseView *m_pView;
	CString m_sRecord;
	BOOL m_bAutoDemo;
	int m_nVideoWidth;
	int m_nVideoHeight;
	CProgressCtrl m_objPB;
	CStatic m_lblBestRecord;
	CButton m_cmdViewRecord;
	CListCtrl m_lstRecord;
public:
	afx_msg void OnBnClickedYes();
	afx_msg void OnBnClickedGameRecord();
	afx_msg void OnBnClickedViewRecord();
	virtual BOOL OnInitDialog();
	afx_msg void OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedOptimize();
	afx_msg void OnBnClickedSolveIt();
};
