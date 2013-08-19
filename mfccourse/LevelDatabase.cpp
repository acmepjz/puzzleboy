// LevelDatabase.cpp : 实现文件
//

#include "stdafx.h"
#include "mfccourse.h"
#include "LevelDatabase.h"
#include <map>

#define NEW_LEVEL 0x1
#define NEW_RECORD 0x2

// CLevelDatabase 对话框

IMPLEMENT_DYNAMIC(CLevelDatabase, CDialog)

CLevelDatabase::CLevelDatabase(CWnd* pParent /*=NULL*/)
	: CDialog(CLevelDatabase::IDD, pParent)
{

}

CLevelDatabase::~CLevelDatabase()
{
}

void CLevelDatabase::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_lstLevels);
	DDX_Control(pDX, IDC_LIST2, m_lstRecord);
}


BEGIN_MESSAGE_MAP(CLevelDatabase, CDialog)
	ON_BN_CLICKED(IDYES, &CLevelDatabase::OnBnClickedYes)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST1, &CLevelDatabase::OnLvnKeydownList1)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, &CLevelDatabase::OnLvnItemchangedList1)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST2, &CLevelDatabase::OnLvnKeydownList2)
	ON_BN_CLICKED(IDOK, &CLevelDatabase::OnBnClickedOk)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST1, &CLevelDatabase::OnNMCustomdrawList1)
	ON_BN_CLICKED(IDRETRY, &CLevelDatabase::OnBnClickedRetry)
END_MESSAGE_MAP()


// CLevelDatabase 消息处理程序

BOOL CLevelDatabase::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_lstLevels.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_lstRecord.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	m_lstLevels.InsertColumn(0,_T("关卡名称"),LVCFMT_LEFT,224);
	m_lstLevels.InsertColumn(1,_T("大小"),LVCFMT_LEFT,48);
	m_lstLevels.InsertColumn(2,_T("纪录"),LVCFMT_LEFT,128);
	m_lstLevels.InsertColumn(3,_T("校验和"),LVCFMT_LEFT,288);

	m_lstRecord.InsertColumn(0,_T("玩家名称"),LVCFMT_LEFT,80);
	m_lstRecord.InsertColumn(1,_T("纪录"),LVCFMT_LEFT,48);

	ShowLevels();

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CLevelDatabase::ShowLevels(){
	CString sKeyword;

	GetDlgItem(IDC_KEYWORD)->GetWindowText(sKeyword);
	sKeyword.Trim();
	sKeyword.MakeLower();

	m_lstLevels.DeleteAllItems();

	for(unsigned int i=0;i<m_tFile.objLevels.size();i++){
		RecordLevelItem& lev=m_tFile.objLevels[i];

		if(lev.bData.empty()) continue;

		if(!sKeyword.IsEmpty()){
			CString s=lev.sName;
			s.MakeLower();
			if(s.Find(sKeyword)<0) continue;
		}

		int j=m_lstLevels.GetItemCount();
		m_lstLevels.InsertItem(j,lev.sName);
		m_lstLevels.SetItemData(j,i);

		CString s;
		for(int k=0;k<CLevel::ChecksumSize;k++){
			s.AppendFormat(_T("%02X"),lev.objChecksum[k]);
		}
		m_lstLevels.SetItemText(j,3,s);

		s.Format(_T("%dx%d"),lev.nWidth,lev.nHeight);
		m_lstLevels.SetItemText(j,1,s);

		int st=lev.nBestRecord;
		if(st>0){
			s.Format(_T("%d ("),st);
			int j=0;
			for(unsigned int i=0;i<lev.objRecord.size();i++){
				if(lev.objRecord[i].nStep==st){
					if(j>0) s.Append(_T(", "));
					s.Append(lev.objRecord[i].sPlayerName);
					j++;
				}
			}
			s.Append(_T(")"));
		}else{
			s=_T("不存在");
		}
		m_lstLevels.SetItemText(j,2,s);
	}
}

void CLevelDatabase::OnBnClickedYes()
{
	//choose open file name
	CFileDialog cd(TRUE,_T("dat"),NULL,
		OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER,
		_T("关卡数据库文件(*.dat)|*.dat||"),this);
	if(cd.DoModal()!=IDOK) return;

	//open file
	FILE *f;
#if _UNICODE
	f=_wfopen(cd.GetPathName(),L"rb");
#else
	f=fopen(cd.GetPathName(),"rb");
#endif

	RecordFile lvs;
	int nNewLevel=0,nNewRecord=0;

	bool b=(f!=NULL && RecordManager::LoadWholeFile(f,lvs));

	if(b){
		//build a map for existing levels
		std::map<RecordLevelChecksum,unsigned int> mp;

		//add all existing levels to the map
		for(unsigned int i=0;i<m_tFile.objLevels.size();i++){
			if(m_tFile.objLevels[i].bData.empty()) continue;
			mp[m_tFile.objLevels[i].objChecksum]=i;
		}

		//check newly added levels
		for(unsigned int i=0;i<lvs.objLevels.size();i++){
			RecordLevelItem& lev=lvs.objLevels[i];

			if(lev.bData.empty()) continue;

			std::map<RecordLevelChecksum,unsigned int>::const_iterator
				it=mp.find(lev.objChecksum);

			if(it==mp.end()){
				//just add this level
				lev.nUserData=NEW_LEVEL;
				m_tFile.objLevels.push_back(lev);
				mp[lev.objChecksum]=m_tFile.objLevels.size()-1;

				nNewLevel++;
			}else{
				//check records
				std::map<CString,unsigned int> mapPlayerNames;

				//first get existing records
				RecordLevelItem& lev0=m_tFile.objLevels[it->second];
				for(unsigned int j=0,m=lev0.objRecord.size();j<m;j++){
					mapPlayerNames[lev0.objRecord[j].sPlayerName]=j;
				}

				//check new records
				for(unsigned int j=0,m=lev.objRecord.size();j<m;j++){
					std::map<CString,unsigned int>::const_iterator
						it=mapPlayerNames.find(lev.objRecord[j].sPlayerName);

					if(it==mapPlayerNames.end()){
						//no record, just add it
						mapPlayerNames[lev.objRecord[j].sPlayerName]=lev0.objRecord.size();
						lev0.nUserData|=NEW_RECORD;
						lev0.objRecord.push_back(lev.objRecord[j]);
						nNewRecord++;
					}else if(lev.objRecord[j].nStep<lev0.objRecord[it->second].nStep){
						//replace record
						lev0.nUserData|=NEW_RECORD;
						lev0.objRecord[it->second]=lev.objRecord[j];
						nNewRecord++;
					}
				}

				//update best record
				lev0.nBestRecord=0;
				for(unsigned int j=0,m=lev0.objRecord.size();j<m;j++){
					if(lev0.nBestRecord==0 || lev0.nBestRecord>lev0.objRecord[j].nStep){
						lev0.nBestRecord=lev0.objRecord[j].nStep;
					}
				}
			}
		}
	}

	if(f) fclose(f);

	if(b){
		CString s;
		s.Format(_T("完成。新增 %d 关卡, %d 最佳纪录。"),nNewLevel,nNewRecord);
		AfxMessageBox(s,MB_ICONINFORMATION);
		ShowLevels();
	}else{
		AfxMessageBox(_T("出现错误"));
	}
}

void CLevelDatabase::OnLvnKeydownList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

	int idx=m_lstLevels.GetSelectionMark();
	if(idx<0 || idx>=m_lstLevels.GetItemCount()) return;

	int idx2=m_lstLevels.GetItemData(idx);

	if(pLVKeyDown->wVKey==VK_DELETE
		&& idx2>=0 && idx2<(int)m_tFile.objLevels.size()
		&& AfxMessageBox(_T("确定删除"),MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2)==IDYES)
	{
		m_tFile.objLevels[idx2].bData.clear();
		memset(&(m_tFile.objLevels[idx2].objChecksum),0,sizeof(RecordLevelChecksum));

		m_lstLevels.DeleteItem(idx);
	}

	*pResult = 0;
}

void CLevelDatabase::OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	m_lstRecord.DeleteAllItems();
	
	int idx=pNMLV->iItem;
	if(idx>=0 && idx<m_lstLevels.GetItemCount()){
		idx=m_lstLevels.GetItemData(idx);
		if(idx>=0 && idx<(int)m_tFile.objLevels.size()){
			RecordLevelItem& lev=m_tFile.objLevels[idx];
			CString s;

			for(unsigned int i=0;i<lev.objRecord.size();i++){
				m_lstRecord.InsertItem(i,lev.objRecord[i].sPlayerName);
				s.Format(_T("%d"),lev.objRecord[i].nStep);
				m_lstRecord.SetItemText(i,1,s);
			}
		}
	}

	*pResult = 0;
}

void CLevelDatabase::OnLvnKeydownList2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

	int idx=m_lstLevels.GetSelectionMark();
	if(idx<0 || idx>=m_lstLevels.GetItemCount()) return;
	idx=m_lstLevels.GetItemData(idx);

	int idx2=m_lstRecord.GetSelectionMark();
	if(pLVKeyDown->wVKey==VK_DELETE
		&& idx>=0 && idx<(int)m_tFile.objLevels.size()
		&& idx2>=0 && idx2<(int)m_tFile.objLevels[idx].objRecord.size()
		&& AfxMessageBox(_T("确定删除"),MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2)==IDYES)
	{
		m_tFile.objLevels[idx].objRecord.erase(
			m_tFile.objLevels[idx].objRecord.begin()+idx2
			);
		m_lstRecord.DeleteItem(idx2);
	}

	*pResult = 0;
}

void CLevelDatabase::OnBnClickedOk()
{
	if(AfxMessageBox(_T("确定保存修改? 请做好备份工作"),MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2)!=IDYES){
		return;
	}

	OnOK();
}

void CLevelDatabase::OnNMCustomdrawList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVCUSTOMDRAW pNMCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pNMHDR);

	switch(pNMCD->nmcd.dwDrawStage){
	case CDDS_PREPAINT:
		*pResult = CDRF_NOTIFYITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT:
		{
			int idx=pNMCD->nmcd.dwItemSpec;

			if(idx>=0 && idx<m_lstLevels.GetItemCount()) idx=m_lstLevels.GetItemData(idx);
			else idx=-1;

			if(idx>=0 && idx<(int)m_tFile.objLevels.size()){
				int flags=m_tFile.objLevels[idx].nUserData;
				if(flags&NEW_RECORD) pNMCD->clrTextBk=0x00FFFF;
				else if(flags&NEW_LEVEL) pNMCD->clrTextBk=0x8080FF;
			}
		}
		*pResult = CDRF_DODEFAULT;
		break;
	default:
		*pResult = CDRF_DODEFAULT;
		break;
	}
}

void CLevelDatabase::OnBnClickedRetry()
{
	ShowLevels();
}
