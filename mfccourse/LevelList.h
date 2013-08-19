#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "mfccourseDoc.h"


// CLevelList 对话框

class CLevelList : public CDialog
{
	DECLARE_DYNAMIC(CLevelList)

public:
	CLevelList(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CLevelList();

// 对话框数据
	enum { IDD = IDD_LEVEL_LIST };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CmfccourseDoc *m_pDoc;
	int m_nMoveLevelSrc,m_nLevelCount;
	int m_nMoveLevel;
	CListCtrl m_lstLevels;

	afx_msg void OnBnClickedMoveLevel();
	virtual BOOL OnInitDialog();
	afx_msg void OnNMDblclkList1(NMHDR *pNMHDR, LRESULT *pResult);
	void ShowLevel();
protected:
	virtual void OnOK();
public:
	afx_msg void OnLvnEndlabeleditList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedYes();
	afx_msg void OnBnClickedNo();
	afx_msg void OnBnClickedShowSpecifiedPlayer();
	afx_msg void OnBnClickedOk();
};
