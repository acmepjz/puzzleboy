// LevelProperties.cpp : 实现文件
//

#include "stdafx.h"
#include "mfccourse.h"
#include "LevelProperties.h"

#pragma warning(disable:4996)

// CLevelProperties 对话框

IMPLEMENT_DYNAMIC(CLevelProperties, CDialog)

CLevelProperties::CLevelProperties(CWnd* pParent /*=NULL*/)
	: CDialog(CLevelProperties::IDD, pParent)
	, m_sLevelPackName(_T(""))
	, m_sLevelName(_T(""))
	, m_nWidth(0)
	, m_nHeight(0)
	, m_nXOffset(0)
	, m_nYOffset(0)
	, m_bPreserve(FALSE)
{

}

CLevelProperties::~CLevelProperties()
{
}

void CLevelProperties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_LEVEL_PACK_NAME, m_sLevelPackName);
	DDX_Text(pDX, IDC_LEVEL_NAME, m_sLevelName);
	DDX_Text(pDX, IDC_WIDTH, m_nWidth);
	DDV_MinMaxInt(pDX, m_nWidth, 1, 255);
	DDX_Text(pDX, IDC_HEIGHT, m_nHeight);
	DDV_MinMaxInt(pDX, m_nHeight, 1, 255);
	DDX_Control(pDX, IDC_CHECK1, m_chkPreserve);
	DDX_Text(pDX, IDC_X_OFFSET, m_nXOffset);
	DDX_Text(pDX, IDC_Y_OFFSET, m_nYOffset);
	DDX_Control(pDX, IDC_X_OFFSET, m_txtXOffset);
	DDX_Control(pDX, IDC_Y_OFFSET, m_txtYOffset);
	DDX_Check(pDX, IDC_CHECK1, m_bPreserve);
}


BEGIN_MESSAGE_MAP(CLevelProperties, CDialog)
	ON_BN_CLICKED(IDC_CHECK1, &CLevelProperties::OnBnClickedCheck1)
END_MESSAGE_MAP()


// CLevelProperties 消息处理程序

void CLevelProperties::OnBnClickedCheck1()
{
	int value=m_chkPreserve.GetCheck();

	m_txtXOffset.EnableWindow(value);
	m_txtYOffset.EnableWindow(value);

	for(int i=0;i<9;i++){
		GetDlgItem(IDC_ALIGN1+i)->EnableWindow(value);
	}
}

BOOL CLevelProperties::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int id=LOWORD(wParam);
	id-=IDC_ALIGN1;
	if(id>=0 && id<9){
		int w=0,h=0;
		CString s;

		GetDlgItem(IDC_WIDTH)->GetWindowText(s);
		swscanf(s,L"%d",&w);
		GetDlgItem(IDC_HEIGHT)->GetWindowText(s);
		swscanf(s,L"%d",&h);

		if(w>0 && h>0){
			int x,y;
			
			switch(id%3){
			case 0:
				x=0;
				break;
			case 1:
				x=(w-m_nWidth)/2;
				break;
			default:
				x=w-m_nWidth;
				break;
			}
			
			switch(id/3){
			case 0:
				y=0;
				break;
			case 1:
				y=(h-m_nHeight)/2;
				break;
			default:
				y=h-m_nHeight;
				break;
			}

			s.Format(_T("%d"),x);
			m_txtXOffset.SetWindowText(s);
			s.Format(_T("%d"),y);
			m_txtYOffset.SetWindowText(s);

			return 0;
		}
	}

	return CDialog::OnCommand(wParam, lParam);
}
