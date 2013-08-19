// LevelRecord.cpp : 实现文件
//

#include "stdafx.h"
#include "mfccourse.h"
#include "mfccourseDoc.h"
#include "mfccourseView.h"
#include "LevelRecord.h"

#include <vfw.h>

// CLevelRecord 对话框

IMPLEMENT_DYNAMIC(CLevelRecord, CDialog)

CLevelRecord::CLevelRecord(CWnd* pParent /*=NULL*/)
	: CDialog(CLevelRecord::IDD, pParent)
	, m_sRecord(_T(""))
	, m_bAutoDemo(FALSE)
	, m_nVideoWidth(640)
	, m_nVideoHeight(480)
	, m_pView(NULL)
{

}

CLevelRecord::~CLevelRecord()
{
}

void CLevelRecord::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_sRecord);
	DDX_Check(pDX, IDC_CHECK1, m_bAutoDemo);
	DDX_Text(pDX, IDC_EDIT2, m_nVideoWidth);
	DDX_Text(pDX, IDC_EDIT3, m_nVideoHeight);
	DDX_Control(pDX, IDC_PROGRESS1, m_objPB);
	DDX_Control(pDX, IDC_BEST_RECORD, m_lblBestRecord);
	DDX_Control(pDX, IDC_VIEW_RECORD, m_cmdViewRecord);
	DDX_Control(pDX, IDC_LIST1, m_lstRecord);
}


BEGIN_MESSAGE_MAP(CLevelRecord, CDialog)
	ON_BN_CLICKED(IDYES, &CLevelRecord::OnBnClickedYes)
	ON_BN_CLICKED(ID_GAME_RECORD, &CLevelRecord::OnBnClickedGameRecord)
	ON_BN_CLICKED(IDC_VIEW_RECORD, &CLevelRecord::OnBnClickedViewRecord)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, &CLevelRecord::OnLvnItemchangedList1)
	ON_BN_CLICKED(IDC_OPTIMIZE, &CLevelRecord::OnBnClickedOptimize)
	ON_BN_CLICKED(IDC_SOLVE_IT, &CLevelRecord::OnBnClickedSolveIt)
END_MESSAGE_MAP()


// CLevelRecord 消息处理程序

void CLevelRecord::OnBnClickedYes()
{
	UpdateData();
	EndDialog(IDYES);
}

void CLevelRecord::OnBnClickedGameRecord()
{
	if(m_pView==NULL || !UpdateData() || m_sRecord.IsEmpty()) return;

	//choose save file name
	CFileDialog cd(FALSE,_T("avi"),NULL,
		OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_EXPLORER,
		_T("视频剪辑(*.avi)|*.avi||"),this);
	if(cd.DoModal()!=IDOK) return;

	//choose video compressor
	COMPVARS cv={sizeof(cv)};
	if(!ICCompressorChoose(m_hWnd,
		ICMF_CHOOSE_ALLCOMPRESSORS|ICMF_CHOOSE_DATARATE|ICMF_CHOOSE_KEYFRAME,
		NULL,NULL,&cv,NULL)) return;

	//now start recording
	bool ret=false;
	int nBytesPerScanline=(m_nVideoWidth*3+3)&~3;
	BITMAPINFO tBI={{sizeof(BITMAPINFOHEADER),m_nVideoWidth,m_nVideoHeight,1,24,0,nBytesPerScanline*m_nVideoHeight}};

	if(ICCompressQuery(cv.hic,&tBI,NULL)==ICERR_OK){
		PAVIFILE file;
		PAVISTREAM stream,streamCompressed;

		AVIFileInit();

		if(AVIFileOpen(&file,cd.GetPathName(),OF_WRITE|OF_CREATE,NULL)==ICERR_OK){
			AVISTREAMINFO tStreamInfo={};

			tStreamInfo.fccType=streamtypeVIDEO;
			tStreamInfo.fccHandler=0;
			tStreamInfo.dwScale=1;
			tStreamInfo.dwRate=30;
			tStreamInfo.dwSuggestedBufferSize=tBI.bmiHeader.biSizeImage;
			tStreamInfo.rcFrame.right=m_nVideoWidth;
			tStreamInfo.rcFrame.bottom=m_nVideoHeight;

			if(AVIFileCreateStream(file,&stream,&tStreamInfo)==ICERR_OK){
				AVICOMPRESSOPTIONS tOptions={};

				tOptions.fccType=streamtypeVIDEO;
				tOptions.fccHandler=cv.fccHandler;
				tOptions.lpParms=cv.lpState;
				tOptions.cbParms=cv.cbState;
				tOptions.dwKeyFrameEvery=cv.lKey;
				tOptions.dwQuality=cv.lQ;
				//tOptions.dwBytesPerSecond=cv.lDataRate;
				tOptions.dwFlags=AVICOMPRESSF_VALID|AVICOMPRESSF_KEYFRAMES;//cv.dwFlags;

				if(AVIMakeCompressedStream(&streamCompressed,stream,&tOptions,NULL)==ICERR_OK){
					if(AVIStreamSetFormat(streamCompressed,0,&tBI,tBI.bmiHeader.biSize)==ICERR_OK){
						//now create dib section
						CDC dc0,dc;

						dc0.CreateDC(_T("DISPLAY"),NULL,NULL,NULL);
						dc.CreateCompatibleDC(&dc0);
						dc0.DeleteDC();

						void *lp;
						HBITMAP hbm=CreateDIBSection(dc,&tBI,DIB_RGB_COLORS,&lp,NULL,0);
						HBITMAP hbmOld=(HBITMAP)SelectObject(dc,hbm);

						//start
						if(m_pView->BeginRecordingVideo(this)){
							m_objPB.SetRange32(0,m_sRecord.GetLength());
							m_objPB.SetPos(0);

							for(int i=0,j=0;;i++){
								int p=m_pView->DrawRecordingVideo(&dc);
								LONG n;

								if(i==0 || p<0){
									for(int k=0;k<30;k++){
										AVIStreamWrite(streamCompressed,j,1,lp,tBI.bmiHeader.biSizeImage,(j&31)==0?AVIIF_KEYFRAME:0,0,&n);
										j++;
									}
								}else{
									AVIStreamWrite(streamCompressed,j,1,lp,tBI.bmiHeader.biSizeImage,(j&31)==0?AVIIF_KEYFRAME:0,0,&n);
									j++;
								}

								if(p<0) break;

								m_objPB.SetPos(p);

								//discard all messages
								MSG msg;
								for(int k=0;k<8;k++){
									if(!PeekMessage(&msg,m_hWnd,0,0,PM_REMOVE)) break;
								}
							}

							m_pView->EndRecordingVideo();
						}

						//over
						SelectObject(dc,hbmOld);
						DeleteObject(hbm);
						dc.DeleteDC();

						ret=true;
					}else{
						MessageBox(_T("AVIStreamSetFormat 失败"));
					}
					AVIStreamRelease(streamCompressed);
				}else{
					MessageBox(_T("AVIMakeCompressedStream 失败"));
				}
				AVIStreamRelease(stream);
			}else{
				MessageBox(_T("AVIFileCreateStream 失败"));
			}
			AVIFileRelease(file);
		}else{
			MessageBox(_T("AVIFileOpen 失败"));
		}
		AVIFileExit();
	}else{
		MessageBox(_T("ICCompressQuery 失败。请尝试修改视频分辨率或者编码器。"));
	}

	//over
	ICCompressorFree(&cv);
	if(ret) MessageBox(_T("完成。"),NULL,MB_ICONINFORMATION);
}

void CLevelRecord::OnBnClickedViewRecord()
{
	if(m_pView!=NULL){
		std::vector<RecordItem>& records=m_pView->m_tCurrentBestRecord;

		int nBestIndex=-1,nBest=0;

		for(int i=0,m=records.size();i<m;i++){
			if(nBest==0 || nBest>records[i].nStep){
				nBestIndex=i;
				nBest=records[i].nStep;
			}
		}

		if(nBestIndex<0) return;

		UpdateData();
		RecordManager::ConvertRecordDataToString(records[nBestIndex].bSolution,m_sRecord);
		//m_sRecord.Delete(m_sRecord.GetLength()-1); //drop the last move
		UpdateData(0);
	}
}

BOOL CLevelRecord::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_lstRecord.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	m_lstRecord.InsertColumn(0,_T("玩家名称"),LVCFMT_LEFT,80);
	m_lstRecord.InsertColumn(1,_T("纪录"),LVCFMT_LEFT,48);

	if(m_pView!=NULL){
		std::vector<RecordItem>& records=m_pView->m_tCurrentBestRecord;

		for(int i=0,m=records.size();i<m;i++){
			m_lstRecord.InsertItem(i,records[i].sPlayerName);

			CString s;
			s.Format(_T("%d"),records[i].nStep);
			m_lstRecord.SetItemText(i,1,s);
		}
	}

	if(m_pView!=NULL && m_pView->m_nCurrentBestStep>0){
		CString s;
		s.Format(_T("最佳纪录: %d"),m_pView->m_nCurrentBestStep);
		m_lblBestRecord.SetWindowText(s);
	}else{
		m_lblBestRecord.SetWindowText(_T("最佳纪录不存在"));
		m_cmdViewRecord.EnableWindow(FALSE);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CLevelRecord::OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	int idx=pNMLV->iItem;

	if(m_pView!=NULL && idx>=0 && idx<(int)m_pView->m_tCurrentBestRecord.size()){
		UpdateData();
		RecordManager::ConvertRecordDataToString(m_pView->m_tCurrentBestRecord[idx].bSolution,m_sRecord);
		//m_sRecord.Delete(m_sRecord.GetLength()-1); //drop the last move
		UpdateData(0);
	}

	*pResult = 0;
}

void CLevelRecord::OnBnClickedOptimize()
{
	if(m_pView==NULL || !UpdateData() || m_sRecord.IsEmpty()) return;

	CmfccourseDoc* pDoc = m_pView->GetDocument();
	if(pDoc==NULL) return;

	CLevel *lev=new CLevel(*(pDoc->GetLevel(m_pView->GetCurrentLevel())));

	lev->StartGame();
	lev->OptimizeRecord(m_sRecord);

	//over
	delete lev;
	UpdateData(0);

	MessageBox(_T("完成。"),NULL,MB_ICONINFORMATION);
}

void CLevelRecord::OnBnClickedSolveIt()
{
	// TODO: 在此添加控件通知处理程序代码
}
