// mfccourse.h : mfccourse 应用程序的主头文件
//
#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"       // 主符号
#include "RecordManager.h"


// CmfccourseApp:
// 有关此类的实现，请参阅 mfccourse.cpp
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

// 重写
public:
	virtual BOOL InitInstance();

	void CreateMRUMenu(CMenu& mnu);
	void SavePlayerName();

// 实现
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CmfccourseApp theApp;