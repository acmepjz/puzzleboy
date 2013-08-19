#pragma once
#include "afxwin.h"

// CLevelProperties �Ի���

class CLevelProperties : public CDialog
{
	DECLARE_DYNAMIC(CLevelProperties)

public:
	CLevelProperties(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CLevelProperties();

// �Ի�������
	enum { IDD = IDD_LEVEL_PROPERTIES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

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
