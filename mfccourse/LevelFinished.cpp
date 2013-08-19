// LevelFinished.cpp : 实现文件
//

#include "stdafx.h"
#include "mfccourse.h"
#include "LevelFinished.h"
#include "RecordManager.h"


// CLevelFinished 对话框

IMPLEMENT_DYNAMIC(CLevelFinished, CDialog)

CLevelFinished::CLevelFinished(CWnd* pParent /*=NULL*/)
	: CDialog(CLevelFinished::IDD, pParent)
	, m_sInfo(_T(""))
	, m_sPlayerName(_T(""))
	, m_objCurrentLevel(NULL)
	, m_bPlayFromRecord(false)
{

}

CLevelFinished::~CLevelFinished()
{
}

void CLevelFinished::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ICON_INFORMATION, m_picInformation);
	DDX_Text(pDX, IDC_INFO, m_sInfo);
	DDX_Control(pDX, IDC_LIST1, m_lstRecord);
	DDX_Text(pDX, IDC_EDIT1, m_sPlayerName);
}


BEGIN_MESSAGE_MAP(CLevelFinished, CDialog)
END_MESSAGE_MAP()


// CLevelFinished 消息处理程序

BOOL CLevelFinished::OnInitDialog()
{
	CDialog::OnInitDialog();

	if(m_bPlayFromRecord) this->GetDlgItem(IDC_EDIT1)->EnableWindow(0);

	//load icon
	m_picInformation.SetIcon((HICON)LoadImage(NULL,MAKEINTRESOURCE(32516 /*OIC_NOTE*/),
		IMAGE_ICON,0,0,LR_DEFAULTSIZE|LR_SHARED));

	//init listview
	m_lstRecord.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	m_lstRecord.InsertColumn(0,_T("玩家名称"),LVCFMT_LEFT,80);
	m_lstRecord.InsertColumn(1,_T("纪录"),LVCFMT_LEFT,48);

	//load records
	if(m_objCurrentLevel){
		std::vector<RecordItem> ret;
		theApp.m_objRecordMgr.FindAllRecordsOfLevel(*m_objCurrentLevel,ret);

		for(unsigned int i=0;i<ret.size();i++){
			m_lstRecord.InsertItem(i,ret[i].sPlayerName);

			CString s;
			s.Format(_T("%d"),ret[i].nStep);
			m_lstRecord.SetItemText(i,1,s);
		}
	}

	MessageBeep(MB_ICONINFORMATION);

	return TRUE;
}
