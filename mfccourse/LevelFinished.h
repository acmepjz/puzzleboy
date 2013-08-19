#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "Level.h"

// CLevelFinished �Ի���

class CLevelFinished : public CDialog
{
	DECLARE_DYNAMIC(CLevelFinished)

public:
	CLevelFinished(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CLevelFinished();

// �Ի�������
	enum { IDD = IDD_LEVEL_FINISHED };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
public:
	CStatic m_picInformation;
	CString m_sInfo;
	CString m_sPlayerName;
	CListCtrl m_lstRecord;
	CLevel *m_objCurrentLevel;
	bool m_bPlayFromRecord;

	virtual BOOL OnInitDialog();
};
