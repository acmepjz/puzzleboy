#pragma once
#include "afxwin.h"

// CLevelProperties 对话框

class CLevelProperties : public CDialog
{
	DECLARE_DYNAMIC(CLevelProperties)

public:
	CLevelProperties(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CLevelProperties();

// 对话框数据
	enum { IDD = IDD_LEVEL_PROPERTIES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CString m_sLevelPackName;
	CString m_sLevelName;
	int m_nWidth;
	int m_nHeight;

	afx_msg void OnBnClickedCheck1();
	CButton m_chkPreserve;
	int m_nXOffset;
	int m_nYOffset;
	CEdit m_txtXOffset;
	CEdit m_txtYOffset;
	BOOL m_bPreserve;
protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
};
