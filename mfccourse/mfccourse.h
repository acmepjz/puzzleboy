// mfccourse.h : mfccourse Ӧ�ó������ͷ�ļ�
//
#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"       // ������
#include "RecordManager.h"


// CmfccourseApp:
// �йش����ʵ�֣������ mfccourse.cpp
//

class CmfccourseApp : public CWinApp
{
public:
	CmfccourseApp();

public:
	RecordManager m_objRecordMgr;
	int m_nAnimationSpeed;
	bool m_bShowGrid;
	CString m_sPlayerName;
	int m_nLanguage;

// ��д
public:
	virtual BOOL InitInstance();

	void CreateMRUMenu(CMenu& mnu);
	void SavePlayerName();

// ʵ��
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CmfccourseApp theApp;