#pragma once
#include "afxcmn.h"

#include "RecordManager.h"

// CLevelDatabase �Ի���

class CLevelDatabase : public CDialog
{
	DECLARE_DYNAMIC(CLevelDatabase)

public:
	CLevelDatabase(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CLevelDatabase();

// �Ի�������
	enum { IDD = IDD_LEVEL_DATABASE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_lstLevels;
	CListCtrl m_lstRecord;
	RecordFile m_tFile;
public:
	virtual BOOL OnInitDialog();
	void ShowLevels();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedYes();
	afx_msg void OnLvnKeydownList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnKeydownList2(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedRetry();
};
