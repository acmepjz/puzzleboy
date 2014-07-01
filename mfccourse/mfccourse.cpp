// mfccourse.cpp : 定义应用程序的类行为。
//

#include "stdafx.h"
#include "afxadv.h"
#include "mfccourse.h"
#include "MainFrm.h"

#include "ChildFrm.h"
#include "mfccourseDoc.h"
#include "mfccourseView.h"
#include "afxcmn.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma warning(disable:4996)

// CmfccourseApp

BEGIN_MESSAGE_MAP(CmfccourseApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CmfccourseApp::OnAppAbout)
	// 基于文件的标准文档命令
	ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinApp::OnFileOpen)
	// 标准打印设置命令
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()


// CmfccourseApp 构造

CmfccourseApp::CmfccourseApp()
{
	m_nAnimationSpeed=0;
	m_bShowGrid=false;
	m_nLanguage=0;
}


// 唯一的一个 CmfccourseApp 对象

CmfccourseApp theApp;


// CmfccourseApp 初始化

BOOL CmfccourseApp::InitInstance()
{
	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControlsEx()。否则，将无法创建窗口。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
	SetRegistryKey(_T("VB and VBA Program Settings")); //lol
	LoadStdProfileSettings(8);  // 加载标准 INI 文件选项(包括 MRU)
	m_sPlayerName=GetProfileString(_T("Settings"),_T("PlayerName"));
	m_nAnimationSpeed=GetProfileInt(_T("Settings"),_T("AnimationSpeed"),0);
	m_bShowGrid=GetProfileInt(_T("Settings"),_T("ShowGrid"),0)!=0;

	m_nLanguage=GetThreadLocale();
	m_nLanguage=((m_nLanguage&0xFF)==0x04)?2052:1033;
	m_nLanguage=GetProfileInt(_T("Settings"),_T("Language"),m_nLanguage);
	SetThreadLocale(m_nLanguage);

	// 注册应用程序的文档模板。文档模板
	// 将用作文档、框架窗口和视图之间的连接
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_LevelTYPE,
		RUNTIME_CLASS(CmfccourseDoc),
		RUNTIME_CLASS(CChildFrame), // 自定义 MDI 子框架
		RUNTIME_CLASS(CmfccourseView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	// 创建主 MDI 框架窗口
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
	{
		delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;
	// 仅当具有后缀时才调用 DragAcceptFiles
	//  在 MDI 应用程序中，这应在设置 m_pMainWnd 之后立即发生
	// 启用拖/放
	m_pMainWnd->DragAcceptFiles();

	// 启用“DDE 执行”
	EnableShellOpen();
	//RegisterShellFileTypes(TRUE);

	// 分析标准外壳命令、DDE、打开文件操作的命令行
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// check if we should show about box
	bool bShowAboutBox=false;

	// load recent file or default file
	if(cmdInfo.m_nShellCommand==CCommandLineInfo::FileNew){
		CDocument *pDoc=NULL;

		//try to load recent file
		if(m_pRecentFileList){
			while(m_pRecentFileList->GetSize()>0){
				CString& fn=(*m_pRecentFileList)[0];
				if(fn.IsEmpty()) break;
				pDoc=OpenDocumentFile(fn);
				if(pDoc) break;
				m_pRecentFileList->Remove(0);
			}
		}

		//try to load default file
		if(pDoc==NULL){
			TCHAR s0[1024];
			int m=GetModuleFileName(NULL,s0,1024);
			while(m>0){
				if(s0[m-1]=='\\' || s0[m-1]=='/'){
					s0[m]=0;
					break;
				}
				m--;
			}

			CString s;
			s=s0;
			s+=_T("PuzzleBoy.lev");
			int attr=GetFileAttributes(s);
			if(attr!=-1 && (attr & FILE_ATTRIBUTE_DIRECTORY)==0){
				pDoc=OpenDocumentFile(s);
			}
			if(pDoc==NULL){
				s=s0;
				s+=_T("..\\PuzzleBoy.lev");
				int attr=GetFileAttributes(s);
				if(attr!=-1 && (attr & FILE_ATTRIBUTE_DIRECTORY)==0){
					pDoc=OpenDocumentFile(s);
				}
			}

			bShowAboutBox=true;
		}

		if(pDoc) cmdInfo.m_nShellCommand=CCommandLineInfo::FileNothing;
	}

	// load record file
	{
		char s[4096];
		int m=GetModuleFileNameA(NULL,s,sizeof(s));

		int i;
		for(i=0;i<m;i++){
			if(s[i]=='/') s[i]='\\';
		}
		for(i=m-1;i>=0;i--){
			if(s[i]=='\\'){
				s[i]=0;
				break;
			}
		}
		strcat(s,"\\PuzzleBoyRecord.dat");

		m_objRecordMgr.LoadFile(s);
	}

	// 调度在命令行中指定的命令。如果
	// 用 /RegServer、/Register、/Unregserver 或 /Unregister 启动应用程序，则返回 FALSE。
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// 主窗口已初始化，因此显示它并对其进行更新
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	// show about box
	if(bShowAboutBox) OnAppAbout();

	return TRUE;
}

void CmfccourseApp::CreateMRUMenu(CMenu& mnu)
{
	for(int i=0;i<m_pRecentFileList->GetSize();i++){
		CString& fn=(*m_pRecentFileList)[i];
		if(fn.IsEmpty()) continue;

		if(mnu.m_hMenu==0) mnu.CreatePopupMenu();

		/*CString s,s1;
		s.Format(_T("&%d %s"),i+1,fn);
		mnu.AppendMenu(0,0,s);*/
		mnu.AppendMenu(0,ID_FILE_MRU_FILE1+i);
	}
}

void CmfccourseApp::SavePlayerName(){
	WriteProfileString(_T("Settings"),_T("PlayerName"),m_sPlayerName);
}

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnNMClickGnuGpl(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickPuzzleBoyFlash(NMHDR *pNMHDR, LRESULT *pResult);
	CTabCtrl m_objTab;
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult);
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB1, m_objTab);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_NOTIFY(NM_CLICK, IDC_GNU_GPL, &CAboutDlg::OnNMClickGnuGpl)
	ON_NOTIFY(NM_CLICK, IDC_PUZZLE_BOY_FLASH, &CAboutDlg::OnNMClickPuzzleBoyFlash)
	ON_WM_CTLCOLOR()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CAboutDlg::OnTcnSelchangeTab1)
END_MESSAGE_MAP()

// 用于运行对话框的应用程序命令
void CmfccourseApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CmfccourseApp 消息处理程序


BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 在此添加额外的初始化
	GetDlgItem(IDC_GNU_GPL)->SetWindowText(_T("This program is free software: you can redistribute it and/or modify\nit under the terms of the GNU General Public License as published by\nthe Free Software Foundation, either version 3 of the License, or\n(at your option) any later version.\n\n")
		_T("This program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n\n")
		_T("You should have received a copy of the GNU General Public License\nalong with this program. If not, see <<a>http://www.gnu.org/licenses/</a>>."));

	CString s;
	s.LoadString(IDS_ABOUT);
	m_objTab.InsertItem(0,s);
	s.LoadString(IDS_GAME_INFO);
	m_objTab.InsertItem(1,s);
	s.LoadString(IDS_LEVEL_EDIT_INFO);
	m_objTab.InsertItem(2,s);

	OnTcnSelchangeTab1(NULL,NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CAboutDlg::OnNMClickGnuGpl(NMHDR *pNMHDR, LRESULT *pResult)
{
	ShellExecute(m_hWnd,_T("open"),_T("http://www.gnu.org/licenses/"),NULL,NULL,SW_SHOW);
	*pResult = 0;
}

void CAboutDlg::OnNMClickPuzzleBoyFlash(NMHDR *pNMHDR, LRESULT *pResult)
{
	ShellExecute(m_hWnd,_T("open"),_T("http://www.freegamesnews.com/en/games/2010/PuzzleBoyFlash.php"),NULL,NULL,SW_SHOW);
	*pResult = 0;
}

HBRUSH CAboutDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	// 在此更改 DC 的任何属性
	TCHAR sClassName[64];
	GetClassName(pWnd->m_hWnd,sClassName,64);
	if((_tcscmp(sClassName,_T("Static"))==0 && (pWnd->GetStyle() & SS_ICON)==0) ||
		_tcscmp(sClassName,_T("SysLink"))==0
		)
	{
		pDC->SetBkMode(TRANSPARENT);
		return (HBRUSH)GetStockObject(NULL_BRUSH);
	}

	// 如果默认的不是所需画笔，则返回另一个画笔
	return hbr;
}

void CAboutDlg::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	const int nCtrlID[]={
		IDC_MFC_ICON,IDC_INFO,IDC_PUZZLE_BOY_FLASH,IDC_GNU_GPL,0,
		IDC_INFO2,IDC_INFO3,IDC_INFO4,IDC_INFO5,IDC_INFO6,
		IDR_PLAYER,IDR_EXIT,IDR_WALL,IDR_BLOCK,IDR_ROTATE_BLOCK,IDR_TARGET_BLOCK,0,
		IDC_EDIT_INFO,
	};

	int i0=0,idx=m_objTab.GetCurSel();
	for(int i=0;i<sizeof(nCtrlID)/sizeof(nCtrlID[0]);i++){
		if(nCtrlID[i]){
			GetDlgItem(nCtrlID[i])->ShowWindow(i0==idx?SW_SHOW:SW_HIDE);
		}else{
			i0++;
		}
	}

	if(pResult) *pResult = 0;
}
