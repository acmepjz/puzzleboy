// LevelList.cpp : 实现文件
//

#include "stdafx.h"
#include "mfccourse.h"
#include "LevelList.h"

#pragma warning(disable:4996)


// CLevelList 对话框

IMPLEMENT_DYNAMIC(CLevelList, CDialog)

CLevelList::CLevelList(CWnd* pParent /*=NULL*/)
	: CDialog(CLevelList::IDD, pParent)
	, m_pDoc(NULL)
	, m_nMoveLevelSrc(0)
	, m_nLevelCount(100)
	, m_nMoveLevel(0)
{

}

CLevelList::~CLevelList()
{
}

void CLevelList::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_nMoveLevel);
	DDX_Control(pDX, IDC_LIST1, m_lstLevels);
}


BEGIN_MESSAGE_MAP(CLevelList, CDialog)
	ON_BN_CLICKED(IDC_MOVE_LEVEL, &CLevelList::OnBnClickedMoveLevel)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, &CLevelList::OnNMDblclkList1)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST1, &CLevelList::OnLvnEndlabeleditList1)
	ON_BN_CLICKED(IDYES, &CLevelList::OnBnClickedYes)
	ON_BN_CLICKED(IDNO, &CLevelList::OnBnClickedNo)
	ON_BN_CLICKED(IDC_SHOW_SPECIFIED_PLAYER, &CLevelList::OnBnClickedShowSpecifiedPlayer)
	ON_BN_CLICKED(IDOK, &CLevelList::OnBnClickedOk)
END_MESSAGE_MAP()

// CLevelList 消息处理程序

void CLevelList::OnBnClickedMoveLevel()
{
	if(!UpdateData()) return;

	int i=m_lstLevels.GetSelectionMark();
	if(i<0 || i>=m_lstLevels.GetItemCount()) return;
	i=m_lstLevels.GetItemData(i);
	if(i<0 || i>=m_nLevelCount) return;
	m_nMoveLevelSrc=i;

	EndDialog(IDC_MOVE_LEVEL);
}

BOOL CLevelList::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_nLevelCount=m_pDoc->m_objLevels.GetSize();

	m_lstLevels.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	m_lstLevels.InsertColumn(0,_T("关卡名称"),LVCFMT_LEFT,192);
	m_lstLevels.InsertColumn(1,_T("编号"),LVCFMT_LEFT,48);
	m_lstLevels.InsertColumn(2,_T("大小"),LVCFMT_LEFT,48);
	//m_lstLevels.InsertColumn(3,_T("校验和"),LVCFMT_LEFT,288);
	m_lstLevels.InsertColumn(3,_T("纪录"),LVCFMT_LEFT,128);

	ShowLevel();

	GetDlgItem(IDC_EDIT1)->EnableWindow(m_nMoveLevel>0?1:0);
	GetDlgItem(IDC_EDIT2)->SetWindowText(theApp.m_sPlayerName);
	GetDlgItem(IDC_MOVE_LEVEL)->EnableWindow(m_nMoveLevel>0?1:0);

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CLevelList::ShowLevel(){
	CString sKeyword;

	GetDlgItem(IDC_KEYWORD)->GetWindowText(sKeyword);
	sKeyword.Trim();
	sKeyword.MakeLower();

	m_lstLevels.DeleteAllItems();

	int j0=-1;

	for(int i=0;i<m_nLevelCount;i++){
		CString s;
		CLevel *lev=m_pDoc->GetLevel(i);

		if(!sKeyword.IsEmpty()){
			s=lev->m_sLevelName;
			s.MakeLower();
			if(s.Find(sKeyword)<0) continue;
		}

		int j=m_lstLevels.GetItemCount();
		m_lstLevels.InsertItem(j,lev->m_sLevelName);
		m_lstLevels.SetItemData(j,i);
		s.Format(_T("%d"),i+1);
		m_lstLevels.SetItemText(j,1,s);
		s.Format(_T("%dx%d"),lev->m_nWidth,lev->m_nHeight);
		m_lstLevels.SetItemText(j,2,s);

		if(m_nMoveLevelSrc==i) j0=j;
	}

	if(j0>=0){
		m_lstLevels.SetItemState(j0,LVIS_SELECTED,LVIS_SELECTED);
		m_lstLevels.SetSelectionMark(j0);
		m_lstLevels.EnsureVisible(j0,0);
	}
}

void CLevelList::OnNMDblclkList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	OnOK();
}

void CLevelList::OnOK()
{
	int i=m_lstLevels.GetSelectionMark();
	if(i<0 || i>=m_lstLevels.GetItemCount()) return;
	i=m_lstLevels.GetItemData(i);
	if(i<0 || i>=m_nLevelCount) return;
	m_nMoveLevelSrc=i;

	CDialog::OnOK();
}

void CLevelList::OnLvnEndlabeleditList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	*pResult = 0;

	if(GetDlgItem(IDC_MOVE_LEVEL)->IsWindowEnabled() && pDispInfo->item.iItem>=0){
		CLevel *lev=m_pDoc->GetLevel(m_lstLevels.GetItemData(pDispInfo->item.iItem));
		lev->m_sLevelName=pDispInfo->item.pszText;
		m_pDoc->SetModifiedFlag();
		*pResult = 1;
	}
}

void CLevelList::OnBnClickedYes()
{
	for(int i=0,m=m_lstLevels.GetItemCount();i<m;i++){
		CLevel *lev=m_pDoc->GetLevel(m_lstLevels.GetItemData(i));
		theApp.m_objRecordMgr.AddLevel(*lev);
	}

	CString s;
	s.Format(_T("完成。当前数据库共有 %d 关卡。"),theApp.m_objRecordMgr.GetLevelCount());

	MessageBox(s,NULL,MB_ICONINFORMATION);
}

void CLevelList::OnBnClickedNo()
{
	CString s,sName;
	int nCount=0;

	if(((CButton*)GetDlgItem(IDC_SHOW_SPECIFIED_PLAYER))->GetCheck()){
		GetDlgItem(IDC_EDIT2)->GetWindowText(sName);
	}

	for(int i=0,m=m_lstLevels.GetItemCount();i<m;i++){
		CLevel *lev=m_pDoc->GetLevel(m_lstLevels.GetItemData(i));
		CString returnName;

		int st=theApp.m_objRecordMgr.FindLevelAndRecord(*lev,
			sName.IsEmpty()?(const wchar_t*)NULL:(const wchar_t*)sName,NULL,NULL,&returnName);
		if(st>0){
			s.Format(_T("%d (%s)"),st,(LPCTSTR)returnName);
			nCount++;
		}else{
			s=_T("不存在");
		}
		m_lstLevels.SetItemText(i,3,s);
	}

	s.Format(_T("完成。共找到 %d 关卡有最佳纪录。"),nCount);

	MessageBox(s,NULL,MB_ICONINFORMATION);
}

void CLevelList::OnBnClickedShowSpecifiedPlayer()
{
	GetDlgItem(IDC_EDIT2)->EnableWindow(((CButton*)GetDlgItem(IDC_SHOW_SPECIFIED_PLAYER))->GetCheck());
}

void CLevelList::OnBnClickedOk()
{
	CWnd* focus=GetFocus();

	if(GetDlgItem(IDC_KEYWORD)==focus){
		ShowLevel();
	}else if(GetDlgItem(IDC_EDIT2)==focus){
		OnBnClickedNo();
	}else{
		OnOK();
	}
}
