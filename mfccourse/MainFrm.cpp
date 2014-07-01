// MainFrm.cpp : CMainFrame 类的实现
//

#include "stdafx.h"
#include "mfccourse.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	ON_WM_CREATE()
	ON_UPDATE_COMMAND_UI(IDR_EDIT_TOOLBAR, &CMainFrame::OnUpdateEditToolbar)
	ON_COMMAND(IDR_EDIT_TOOLBAR, &CMainFrame::OnEditToolbar)
	ON_NOTIFY(TBN_DROPDOWN, AFX_IDW_TOOLBAR, &CMainFrame::OnToolbarDropdown)
	ON_COMMAND_RANGE(ID_LANGUAGE_1,ID_LANGUAGE_99,&CMainFrame::OnLanguage)
	ON_UPDATE_COMMAND_UI_RANGE(ID_LANGUAGE_1,ID_LANGUAGE_99,&CMainFrame::OnUpdateLanguage)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	0,           // 状态行指示器
};

static int languages[ID_LANGUAGE_99-ID_LANGUAGE_1+1]={
	1033,2052,
};

// CMainFrame 构造/析构

CMainFrame::CMainFrame()
{

}

CMainFrame::~CMainFrame()
{
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	CString s;

	m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndToolBar.LoadToolBar(IDR_MAINFRAME);
	s.LoadString(IDD_ABOUTBOX);
	m_wndToolBar.SetWindowText(s);

	//add some drop-down menus
	{
		CToolBarCtrl &tb=m_wndToolBar.GetToolBarCtrl();

		TBBUTTONINFO tbbi;
		tbbi.cbSize=sizeof(tbbi);
		tbbi.dwMask=TBIF_STYLE;
		tbbi.fsStyle=BTNS_DROPDOWN;

		tb.SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);
		tb.SetButtonInfo(ID_FILE_OPEN,&tbbi);
		tb.SetButtonInfo(ID_FILE_SAVE,&tbbi);
		tb.SetButtonInfo(ID_LEVEL_LIST,&tbbi);
	}

	m_wndEditToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndEditToolBar.LoadToolBar(IDR_EDIT_TOOLBAR);
	s.LoadString(IDR_EDIT_TOOLBAR);
	m_wndEditToolBar.SetWindowText(s);

	m_wndStatusBar.Create(this);
	m_wndStatusBar.SetIndicators(indicators,
	  sizeof(indicators)/sizeof(UINT));

	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndEditToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
	DockControlBar(&m_wndEditToolBar);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	return TRUE;
}


// CMainFrame 诊断

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG


// CMainFrame 消息处理程序




void CMainFrame::OnUpdateEditToolbar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_wndEditToolBar.IsWindowVisible());
}

void CMainFrame::OnEditToolbar()
{
	ShowControlBar(&m_wndEditToolBar,!m_wndEditToolBar.IsWindowVisible(),1);
}

void CMainFrame::OnToolbarDropdown(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMTOOLBAR *pnmtb = reinterpret_cast<NMTOOLBAR*>(pNMHDR);

	CMenu mnu,*popup=NULL;
	CWnd *wnd=NULL;

	switch(pnmtb->iItem){
	case ID_FILE_SAVE:
		mnu.LoadMenu(IDR_POPUP_SAVE);
		popup=mnu.GetSubMenu(0);
		wnd=&m_wndToolBar;
		break;
	case ID_FILE_OPEN:
		theApp.CreateMRUMenu(mnu);
		if(mnu.m_hMenu==0) return;
		popup=&mnu;
		wnd=&m_wndToolBar;
		break;
	default:
		return;
	}

	*pResult = 0;

	RECT rc;
	wnd->SendMessage(TB_GETRECT,pnmtb->iItem,(LPARAM)&rc);
	wnd->ClientToScreen(&rc);
	popup->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, rc.left,rc.bottom,this,&rc);
}

void CMainFrame::OnUpdateLanguage(CCmdUI *pCmdUI)
{
	//pCmdUI->Enable(m_bEditMode);
	pCmdUI->SetCheck(languages[pCmdUI->m_nID-ID_LANGUAGE_1]==theApp.m_nLanguage);
}


void CMainFrame::OnLanguage(UINT nID)
{
	AfxMessageBox(_T("You need to restart the program to change the language."));
	theApp.m_nLanguage=languages[nID-ID_LANGUAGE_1];
	theApp.WriteProfileInt(_T("Settings"),_T("Language"),theApp.m_nLanguage);
	SetThreadLocale(theApp.m_nLanguage);
}
