// mfccourseView.cpp : CmfccourseView 类的实现
//

#include "stdafx.h"
#include "mfccourse.h"

#include "MainFrm.h"
#include "mfccourseDoc.h"
#include "mfccourseView.h"

#include "LevelProperties.h"
#include "LevelList.h"
#include "LevelRecord.h"
#include "LevelDatabase.h"
#include "LevelFinished.h"
#include "Level.h"
#include "PushableBlock.h"

#include <string.h>

#define TIMER_INTERVAL 20

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CmfccourseView

IMPLEMENT_DYNCREATE(CmfccourseView, CView)

BEGIN_MESSAGE_MAP(CmfccourseView, CView)
	// 标准打印命令
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
	ON_UPDATE_COMMAND_UI_RANGE(52701,ID_EDIT_EDITMODE,&CmfccourseView::OnUpdateEditEditmode)
	ON_UPDATE_COMMAND_UI(ID_GAME_RECORD, &CmfccourseView::OnUpdateGameRestart)
	ON_UPDATE_COMMAND_UI(ID_GAME_RESTART, &CmfccourseView::OnUpdateGameRestart)
	ON_UPDATE_COMMAND_UI_RANGE(52751,52999,&CmfccourseView::OnUpdateEditTools)
	ON_COMMAND(ID_EDIT_EDITMODE, &CmfccourseView::OnEditEditmode)
	ON_COMMAND_RANGE(ID_EDIT_SELECT,52999,&CmfccourseView::OnEditTools)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_KEYDOWN()
	ON_COMMAND(ID_GAME_RESTART, &CmfccourseView::OnGameRestart)
	ON_WM_TIMER()
	ON_COMMAND(ID_EDIT_PROPERTIES, &CmfccourseView::OnEditProperties)
	ON_COMMAND_RANGE(ID_FIRST_LEVEL,ID_LEVEL_LIST,&CmfccourseView::OnLevelList)
	ON_COMMAND(ID_EDIT_ADD_LEVEL, &CmfccourseView::OnEditAddLevel)
	ON_COMMAND(ID_EDIT_REMOVE_LEVEL, &CmfccourseView::OnEditRemoveLevel)
	ON_COMMAND(ID_EDIT_UNDO, &CmfccourseView::OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO, &CmfccourseView::OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, &CmfccourseView::OnUpdateEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, &CmfccourseView::OnUpdateEditRedo)
	ON_UPDATE_COMMAND_UI_RANGE(ID_EDIT_MOVE_UP,ID_EDIT_ROTATE_CW,&CmfccourseView::OnUpdateEditMove)
	ON_COMMAND_RANGE(ID_EDIT_MOVE_UP,ID_EDIT_ROTATE_CW,&CmfccourseView::OnEditMove)
	ON_COMMAND(ID_EDIT_CLEAR, &CmfccourseView::OnEditClear)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CLEAR, &CmfccourseView::OnUpdateEditClear)
	ON_NOTIFY(TBN_DROPDOWN, AFX_IDW_TOOLBAR, &CmfccourseView::OnToolbarDropdown)
	ON_COMMAND(ID_GAME_RECORD, &CmfccourseView::OnGameRecord)
	ON_UPDATE_COMMAND_UI(ID_VIEW_GRID, &CmfccourseView::OnUpdateViewGrid)
	ON_COMMAND(ID_VIEW_GRID, &CmfccourseView::OnViewGrid)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_ANIMATION_SPEED_0,ID_VIEW_ANIMATION_SPEED_999,&CmfccourseView::OnUpdateViewAnimationSpeed)
	ON_COMMAND_RANGE(ID_VIEW_ANIMATION_SPEED_0,ID_VIEW_ANIMATION_SPEED_999,&CmfccourseView::OnViewAnimationSpeed)
	ON_COMMAND(ID_LEVEL_DATABASE, &CmfccourseView::OnLevelDatabase)
	ON_COMMAND(ID_TOOLS_IMPORT_SOKOBAN, &CmfccourseView::OnToolsImportSokoban)
END_MESSAGE_MAP()

// CmfccourseView 构造/析构

CmfccourseView::CmfccourseView()
{
	// 在此处添加构造代码
	m_nCurrentLevel=0;
	m_bEditMode=false;
	m_bPlayFromRecord=false;
	m_nCurrentTool=-1;
	m_bmpWidth=0;
	m_bmpHeight=0;
	m_objPrintInfo=NULL;

	m_nEditingBlockIndex=-1;
	m_objBackupBlock=NULL;

	m_objPlayingLevel=NULL;
	m_nRecordIndex=-1;

	m_objRecordingLevel=NULL;
}

CmfccourseView::~CmfccourseView()
{
	if(m_dcMem.m_hDC) m_dcMem.DeleteDC();
	if(m_bmp.m_hObject) m_bmp.DeleteObject();
	if(m_objPlayingLevel) delete m_objPlayingLevel;
	if(m_objBackupBlock) delete m_objBackupBlock;
}

BOOL CmfccourseView::PreCreateWindow(CREATESTRUCT& cs)
{
	// 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	return CView::PreCreateWindow(cs);
}

// CmfccourseView 绘制

void CmfccourseView::OnDraw(CDC* pDC)
{
	CmfccourseDoc* pDoc = GetDocument();
	if (!pDoc) return;

	//get current level
	CLevel *lev=NULL;
	int idx=0;
	if(m_objPrintInfo){
		idx=m_objPrintInfo->m_nCurPage;
		lev=pDoc->GetLevel(idx-1);
	}else if(m_objRecordingLevel){
		idx=m_nCurrentLevel+1;
		lev=m_objRecordingLevel;
	}else if(!m_bEditMode){
		if(m_objPlayingLevel==NULL) StartGame();
		idx=m_nCurrentLevel+1;
		lev=m_objPlayingLevel;
	}
	if(lev==NULL){
		if(m_nCurrentLevel<0) m_nCurrentLevel=0;
		if(m_nCurrentLevel>=pDoc->m_objLevels.GetSize()) m_nCurrentLevel=pDoc->m_objLevels.GetSize()-1;

		idx=m_nCurrentLevel+1;
		lev=pDoc->GetLevel(m_nCurrentLevel);
		if(lev==NULL) return;
	}

	//init double buffer
	CDC* pOldDC=NULL;
	if(m_objPrintInfo==NULL && m_objRecordingLevel==NULL && m_dcMem.m_hDC && m_bmp.m_hObject){
		pOldDC=pDC;
		pDC=&m_dcMem;
		//clear screen
		pDC->BitBlt(0,0,m_bmpWidth,m_bmpHeight,NULL,0,0,WHITENESS);
	}

	// 在此处为本机数据添加绘制代码
	RECT r;
	int nFontHeight;
	if(m_objPrintInfo){
		r.left=pDC->GetDeviceCaps(LOGPIXELSX)*5/4;
		r.top=pDC->GetDeviceCaps(LOGPIXELSY);
		r.right=pDC->GetDeviceCaps(HORZRES)-r.left;
		r.bottom=pDC->GetDeviceCaps(VERTRES)-r.top;
		nFontHeight=r.top/8;
	}else if(m_objRecordingLevel){
		pDC->BitBlt(0,0,m_nVideoWidth,m_nVideoHeight,NULL,0,0,WHITENESS);
		r.left=0;
		r.top=0;
		r.right=m_nVideoWidth;
		r.bottom=m_nVideoHeight;
		nFontHeight=20;
	}else{
		GetClientRect(&r);
		nFontHeight=20;
	}

	pDC->SetTextColor(0);
	pDC->SetBkMode(TRANSPARENT);

	CString s=pDoc->m_sLevelPackName;
	if(m_objPrintInfo==NULL && m_bEditMode){
		s.AppendFormat(IDS_EDIT_MODE);
	}

	RECT r1={r.left,r.top,r.right,r.top+nFontHeight};
	pDC->DrawText(s,-1,&r1,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOCLIP);

	s.Format(IDS_LEVEL_INFO,
		idx,lev->m_sLevelName,lev->m_nWidth,lev->m_nHeight,pDoc->m_objLevels.GetSize());
	if(m_objPrintInfo==NULL && !m_bEditMode){
		if(lev->m_nMoves>0) s.AppendFormat(IDS_LEVEL_INFO_2,lev->m_nMoves);
		if(m_nCurrentBestStep>0){
			s.AppendFormat(IDS_LEVEL_INDO_3,m_nCurrentBestStep);
			if(!m_sCurrentBestStepOwner.IsEmpty()) s.AppendFormat(_T(" (%s)"),(LPCTSTR)m_sCurrentBestStepOwner);
		}
	}
	r1.top=r1.bottom;
	r1.bottom=r1.top+nFontHeight;
	pDC->DrawText(s,-1,&r1,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOCLIP);

	r1.top=r1.bottom;
	r1.bottom=r.bottom;
	lev->Draw(pDC,r1,m_bEditMode,true,0,m_nEditingBlockIndex,theApp.m_bShowGrid);

	//draw selected block in game mode
	if(m_objPrintInfo==NULL && !m_bEditMode && m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<lev->m_objBlocks.GetSize()){
		CPushableBlock *objBlock=(CPushableBlock*)lev->m_objBlocks[m_nEditingBlockIndex];

		int nBlockSize=lev->GetBlockSize(r1.right-r1.left,r1.bottom-r1.top);
		RECT r2=lev->GetDrawArea(r1,nBlockSize);
		
		if(objBlock->m_nType==ROTATE_BLOCK){
			RECT r3={
				r2.left+(m_nEditingBlockDX)*nBlockSize,
				r2.top+(m_nEditingBlockDY)*nBlockSize,
				r2.left+(m_nEditingBlockDX+1)*nBlockSize,
				r2.top+(m_nEditingBlockDY+1)*nBlockSize
			};

			CPen hpn(0,2,0xFF0000);
			CBrush hbr(HS_DIAGCROSS,0xFF0000);
			CPen *hpnOld=pDC->SelectObject(&hpn);
			CBrush *hbrOld=pDC->SelectObject(&hbr);
			pDC->Rectangle(&r3);

			r3.left+=nBlockSize/2;
			r3.top+=nBlockSize/2;
			pDC->MoveTo(r3.left,r3.top);

			if(m_nEditingBlockDX==objBlock->m_x){
				if(m_nEditingBlockX<objBlock->m_x){
					//left
					r3.left-=nBlockSize+8;
					pDC->LineTo(r3.left,r3.top);
					pDC->LineTo(r3.left+8,r3.top-4);
					pDC->MoveTo(r3.left,r3.top);
					pDC->LineTo(r3.left+8,r3.top+4);
				}else if(m_nEditingBlockX>objBlock->m_x){
					//right
					r3.left+=nBlockSize+8;
					pDC->LineTo(r3.left,r3.top);
					pDC->LineTo(r3.left-8,r3.top-4);
					pDC->MoveTo(r3.left,r3.top);
					pDC->LineTo(r3.left-8,r3.top+4);
				}
			}else{
				if(m_nEditingBlockY<objBlock->m_y){
					//up
					r3.top-=nBlockSize+8;
					pDC->LineTo(r3.left,r3.top);
					pDC->LineTo(r3.left-4,r3.top+8);
					pDC->MoveTo(r3.left,r3.top);
					pDC->LineTo(r3.left+4,r3.top+8);
				}else if(m_nEditingBlockY>objBlock->m_y){
					//down
					r3.top+=nBlockSize+8;
					pDC->LineTo(r3.left,r3.top);
					pDC->LineTo(r3.left-4,r3.top-8);
					pDC->MoveTo(r3.left,r3.top);
					pDC->LineTo(r3.left+4,r3.top-8);
				}
			}

			pDC->SelectObject(hpnOld);
			pDC->SelectObject(hbrOld);
		}else{
			HRGN rgn=NULL;

			for(int j=0;j<objBlock->m_h;j++){
				for(int i=0;i<objBlock->m_w;i++){
					if((*objBlock)(i,j)){
						HRGN rgn1=CreateRectRgn(
							r2.left+(m_nEditingBlockX+i)*nBlockSize,
							r2.top+(m_nEditingBlockY+j)*nBlockSize,
							r2.left+(m_nEditingBlockX+i+1)*nBlockSize,
							r2.top+(m_nEditingBlockY+j+1)*nBlockSize);
						if(rgn==NULL) rgn=rgn1;
						else{
							CombineRgn(rgn,rgn,rgn1,RGN_OR);
							DeleteObject(rgn1);
						}
					}
				}
			}

			if(rgn!=NULL){
				CBrush hbr1(0xFF0000);
				CBrush hbr2(HS_DIAGCROSS,0xFF0000);
				FillRgn(pDC->m_hDC,rgn,(HBRUSH)hbr2.m_hObject);
				FrameRgn(pDC->m_hDC,rgn,(HBRUSH)hbr1.m_hObject,2,2);
				DeleteObject(rgn);
			}
		}
	}

	//copy to screen
	if(pOldDC){
		pOldDC->BitBlt(0,0,m_bmpWidth,m_bmpHeight,pDC,0,0,SRCCOPY);
	}
}


// CmfccourseView 打印

BOOL CmfccourseView::OnPreparePrinting(CPrintInfo* pInfo)
{
	CmfccourseDoc* pDoc = GetDocument();
	if (!pDoc) return 0;

	pInfo->SetMaxPage(pDoc->m_objLevels.GetSize());

	return DoPreparePrinting(pInfo);
}

void CmfccourseView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo)
{
	// 添加额外的打印前进行的初始化过程
	m_objPrintInfo=pInfo;
}

void CmfccourseView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// 添加打印后进行的清理过程
	m_objPrintInfo=NULL;
}


// CmfccourseView 诊断

#ifdef _DEBUG
void CmfccourseView::AssertValid() const
{
	CView::AssertValid();
}

void CmfccourseView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CmfccourseDoc* CmfccourseView::GetDocument() const // 非调试版本是内联的
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CmfccourseDoc)));
	return (CmfccourseDoc*)m_pDocument;
}
#endif //_DEBUG


// CmfccourseView 消息处理程序

static void SetHourglassCursor(){
	SetCursor((HCURSOR)LoadImage(NULL,MAKEINTRESOURCE(32514),IMAGE_CURSOR,LR_DEFAULTSIZE,LR_DEFAULTSIZE,LR_SHARED));
}

void CmfccourseView::OnUpdateEditEditmode(CCmdUI *pCmdUI)
{
	pCmdUI->Enable();
	pCmdUI->SetCheck(m_bEditMode && pCmdUI->m_nID==ID_EDIT_EDITMODE);
}

void CmfccourseView::OnUpdateGameRestart(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!m_bEditMode);
}

void CmfccourseView::OnUpdateEditTools(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bEditMode);
	pCmdUI->SetCheck(pCmdUI->m_nID==52800+m_nCurrentTool);
}

void CmfccourseView::OnEditEditmode()
{
	//finish block edit
	if(m_bEditMode && m_nEditingBlockIndex>=0) FinishBlockEdit();
	m_nEditingBlockIndex=-1;

	//switch mode
	m_bEditMode=!m_bEditMode;

	//show toolbar
	if(m_bEditMode){
		CMainFrame* frm=(CMainFrame*)theApp.m_pMainWnd;
		frm->ShowControlBar(&frm->m_wndEditToolBar,1,1);
	}

	//start game if not in edit mode
	StartGame();

	//over
	Invalidate();
}

void CmfccourseView::OnEditTools(UINT nID)
{
	if(m_bEditMode && m_nEditingBlockIndex>=0) FinishBlockEdit();
	m_nCurrentTool=nID-52800;
}

void CmfccourseView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CView::OnLButtonDown(nFlags, point);

	OnMouseEvent(nFlags,point,true,false);
}

void CmfccourseView::OnMouseMove(UINT nFlags, CPoint point)
{
	CView::OnMouseMove(nFlags, point);

	OnMouseEvent(nFlags,point,false,false);
}

void CmfccourseView::OnLButtonUp(UINT nFlags, CPoint point)
{
	CView::OnLButtonUp(nFlags, point);

	OnMouseEvent(nFlags,point,false,true);
}

void CmfccourseView::OnRButtonDown(UINT nFlags, CPoint point)
{
	CView::OnRButtonDown(nFlags, point);

	OnMouseEvent(nFlags,point,true,false);
}

void CmfccourseView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	if(cx>0 && cy>0 && (m_bmp.m_hObject==NULL || m_bmpWidth!=cx || m_bmpHeight!=cy)){
		if(m_dcMem.m_hDC) m_dcMem.DeleteDC();
		if(m_bmp.m_hObject) m_bmp.DeleteObject();

		CDC tmp;
		tmp.CreateDC(_T("DISPLAY"),NULL,NULL,NULL);

		m_dcMem.CreateCompatibleDC(&tmp);
		m_bmp.CreateCompatibleBitmap(&tmp,cx,cy);
		m_dcMem.SelectObject(&m_bmp);

		tmp.DeleteDC();

		m_bmpWidth=cx;
		m_bmpHeight=cy;
	}
}

BOOL CmfccourseView::OnEraseBkgnd(CDC* pDC)
{
	// 在此添加消息处理程序代码和/或调用默认值

	return 1;
}

void CmfccourseView::OnMouseEvent(UINT nFlags, CPoint point, bool IsMouseDown, bool IsMouseUp){
	//currently we don't process mouse up event
	if(IsMouseUp) return;

	//process mouse move event
	if((nFlags&(MK_LBUTTON|MK_RBUTTON))==0){
		if(!m_bEditMode && m_objPlayingLevel
			&& m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<m_objPlayingLevel->m_objBlocks.GetSize())
		{
			RECT r;
			GetClientRect(&r);
			r.top=40;

			int nBlockSize=m_objPlayingLevel->GetBlockSize(r.right,r.bottom-40);
			r=m_objPlayingLevel->GetDrawArea(r,nBlockSize);

			if(point.x>=r.left && point.y>=r.top){
				int x=(point.x-r.left)/nBlockSize;
				int y=(point.y-r.top)/nBlockSize;

				if(((CPushableBlock*)m_objPlayingLevel->m_objBlocks[m_nEditingBlockIndex])->m_nType!=ROTATE_BLOCK){
					x+=m_nEditingBlockDX;
					y+=m_nEditingBlockDY;
				}

				if(x!=m_nEditingBlockX || y!=m_nEditingBlockY){
					m_nEditingBlockX=x;
					m_nEditingBlockY=y;
					Invalidate();
				}
			}
		}
		return;
	}

	if(!m_bEditMode){
		//process in-game mouse events
		if(IsMouseDown && (nFlags&(MK_LBUTTON|MK_RBUTTON))!=0
			&& m_objPlayingLevel
			)
		{
			if(nFlags&MK_RBUTTON){
				if(m_nEditingBlockIndex>=0){
					//cancel moving blocks
					m_nEditingBlockIndex=-1;
					Invalidate();
				}else if(!m_sRecord.IsEmpty()){
					//skip demo
					m_bPlayFromRecord=false;
					m_sRecord.Empty();
					m_nRecordIndex=-1;
				}else if(!m_objPlayingLevel->IsAnimating()){
					//undo
					if(m_objPlayingLevel->CanUndo()) OnEditUndo();
					else MessageBeep(0);
				}
			}else if(
				!m_objPlayingLevel->IsAnimating()
				&& !m_objPlayingLevel->IsWin()
				&& m_sRecord.IsEmpty()
				)
			{
				RECT r;
				GetClientRect(&r);
				r.top=40;

				int nBlockSize=m_objPlayingLevel->GetBlockSize(r.right,r.bottom-40);
				r=m_objPlayingLevel->GetDrawArea(r,nBlockSize);

				if(point.x>=r.left && point.y>=r.top){

					int x=(point.x-r.left)/nBlockSize;
					int y=(point.y-r.top)/nBlockSize;
					
					if(x<m_objPlayingLevel->m_nWidth && y<m_objPlayingLevel->m_nHeight){
						if(m_objPlayingLevel->SwitchPlayer(x,y,true)){
							//it's switch player
							Invalidate();
							if(m_objPlayingLevel->IsAnimating()){
								SetTimer(ID_TIMER1,TIMER_INTERVAL,NULL);
							}
						}else if(m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<m_objPlayingLevel->m_objBlocks.GetSize()){
							//move a moveable block to new position
							CString s;
							int idx=m_nEditingBlockIndex;
							m_nEditingBlockIndex=-1;

							SetHourglassCursor();

							CPushableBlock *objBlock=(CPushableBlock*)m_objPlayingLevel->m_objBlocks[idx];

							if(objBlock->m_nType==ROTATE_BLOCK){
								int dx=0,dy=0;
								if(m_nEditingBlockDX==objBlock->m_x){
									if(m_nEditingBlockX<objBlock->m_x) dx=-1;
									else if(m_nEditingBlockX>objBlock->m_x) dx=1;
								}else{
									if(m_nEditingBlockY<objBlock->m_y) dy=-1;
									else if(m_nEditingBlockY>objBlock->m_y) dy=1;
								}

								int xx=m_nEditingBlockDX-dx;
								int yy=m_nEditingBlockDY-dy;

								if(m_objPlayingLevel->CheckIfCurrentPlayerCanMove(dx,dy,xx,yy)
									&& m_objPlayingLevel->FindPath(s,xx,yy))
								{
									s.AppendChar(dx<0?'A':dy<0?'W':dx>0?'D':'S');
								}
							}else{
								m_objPlayingLevel->FindPathForBlock(s,idx,m_nEditingBlockX,m_nEditingBlockY);
							}

							if(!s.IsEmpty()){
								m_sRecord=s;
								m_nRecordIndex=0;
								SetTimer(ID_TIMER1,TIMER_INTERVAL,NULL);
							}else{
								MessageBeep(0);
								Invalidate();
							}
						}else{
							int x0=m_objPlayingLevel->GetCurrentPlayerX();
							int y0=m_objPlayingLevel->GetCurrentPlayerY();

							//check if a block clicked
							int idx=m_objPlayingLevel->HitTestForPlacingBlocks(x,y,-1);
							int nType=-1;
							CPushableBlock *objBlock=NULL;
							if(idx>=0){
								objBlock=(CPushableBlock*)m_objPlayingLevel->m_objBlocks[idx];
								nType=objBlock->m_nType;
							}
							if(nType==NORMAL_BLOCK || nType==TARGET_BLOCK){
								//clicked a moveable block
								m_nEditingBlockIndex=idx;
								m_nEditingBlockX=objBlock->m_x;
								m_nEditingBlockY=objBlock->m_y;
								m_nEditingBlockDX=m_nEditingBlockX-x;
								m_nEditingBlockDY=m_nEditingBlockY-y;
								Invalidate();
							}else if(nType==ROTATE_BLOCK && (x!=objBlock->m_x || y!=objBlock->m_y)){
								//clicked a rotate block
								m_nEditingBlockIndex=idx;
								m_nEditingBlockX=x;
								m_nEditingBlockY=y;
								m_nEditingBlockDX=x;
								m_nEditingBlockDY=y;
								Invalidate();
							}else if(x==x0 && y==y0-1){ //check if the clicked position is adjacent to the player
								OnKeyDown(VK_UP,0,0);
							}else if(x==x0 && y==y0+1){
								OnKeyDown(VK_DOWN,0,0);
							}else if(x==x0-1 && y==y0){
								OnKeyDown(VK_LEFT,0,0);
							}else if(x==x0+1 && y==y0){
								OnKeyDown(VK_RIGHT,0,0);
							}else{
								CString s;

								SetHourglassCursor();

								if(m_objPlayingLevel->FindPath(s,x,y) && !s.IsEmpty()){
									//it's pathfinding
									m_sRecord=s;
									m_nRecordIndex=0;
									SetTimer(ID_TIMER1,TIMER_INTERVAL,NULL);
								}else{
									MessageBeep(0);
								}
							}
						}
					}
				}
			}
		}
		return;
	}

	CmfccourseDoc* pDoc = GetDocument();
	if (!pDoc) return;

	if(m_nCurrentLevel<0) m_nCurrentLevel=0;
	if(m_nCurrentLevel>=pDoc->m_objLevels.GetSize()) m_nCurrentLevel=pDoc->m_objLevels.GetSize()-1;

	CLevel *lev=pDoc->GetLevel(m_nCurrentLevel);
	if(lev==NULL) return;

	RECT r;
	GetClientRect(&r);
	r.top=40;

	int nBlockSize=lev->GetBlockSize(r.right,r.bottom-40);
	r=lev->GetDrawArea(r,nBlockSize);

	if(point.x<r.left || point.y<r.top) return;
	int x=(point.x-r.left)/nBlockSize;
	int y=(point.y-r.top)/nBlockSize;
	if(x>=lev->m_nWidth || y>=lev->m_nHeight) return;

	//check if we are editing some pushable block
	if(m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<lev->m_objBlocks.GetSize()){
		CPushableBlock *objBlock=(CPushableBlock*)lev->m_objBlocks[m_nEditingBlockIndex];
		if(objBlock->m_nType==ROTATE_BLOCK){
			int idx=-1,value=-1;

			if(x==objBlock->m_x){
				if(y==objBlock->m_y){
					if(nFlags&MK_LBUTTON){
						MessageBeep(0);
					}else{
						//delete this block
						OnEditClear();
					}
				}else if(y<objBlock->m_y){
					idx=0;
					value=objBlock->m_y-y;
				}else{
					idx=2;
					value=y-objBlock->m_y;
				}
			}else if(y==objBlock->m_y){
				if(x<objBlock->m_x){
					idx=1;
					value=objBlock->m_x-x;
				}else{
					idx=3;
					value=x-objBlock->m_x;
				}
			}else{
				MessageBeep(0);
			}

			if(idx>=0){
				if(nFlags&MK_RBUTTON) value--;
				if((*objBlock)[idx]!=value){
					(*objBlock)[idx]=value;
					Invalidate();
					pDoc->SetModifiedFlag();
				}
			}
		}else{
			if((nFlags&MK_LBUTTON)!=0 && lev->HitTestForPlacingBlocks(x,y,m_nEditingBlockIndex)!=-1){
				MessageBeep(0);
			}else{
				int n=(nFlags&MK_LBUTTON)?1:0;
				if((*objBlock)(x,y)!=n){
					(*objBlock)(x,y)=n;
					Invalidate();
					pDoc->SetModifiedFlag();
				}
			}
		}
		return;
	}

	switch(m_nCurrentTool){
	case -1:
		//TODO: other selection tool
		{
			if(nFlags&MK_LBUTTON){
				int idx=lev->HitTestForPlacingBlocks(x,y,m_nEditingBlockIndex);
				if(idx>=0){
					EnterBlockEdit(idx,true);
				}
			}
		}
		break;
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		{
			int idx=m_nCurrentTool;
			int idx0=(*lev)(x,y);

			if(nFlags&MK_RBUTTON) idx=0;
			else if(idx==TARGET_TILE && (idx0==PLAYER_TILE || idx0==PLAYER_AND_TARGET_TILE)) idx=PLAYER_AND_TARGET_TILE;
			else if(idx==PLAYER_TILE && (idx0==TARGET_TILE || idx0==PLAYER_AND_TARGET_TILE)) idx=PLAYER_AND_TARGET_TILE;

			if(idx0==idx) break;

			(*lev)(x,y)=idx;
			pDoc->SetModifiedFlag();
			Invalidate();
		}
		break;
	case 100:
	case 101:
	case 102:
		{
			if(nFlags&MK_LBUTTON){
				int idx=lev->HitTestForPlacingBlocks(x,y,m_nEditingBlockIndex);
				if(idx==-1){
					CPushableBlock *objBlock=new CPushableBlock;
					objBlock->CreateSingle(m_nCurrentTool-100,x,y);
					EnterBlockEdit(lev->m_objBlocks.Add(objBlock),false);
				}else if(idx>=0){
					EnterBlockEdit(idx,true);
				}else{
					MessageBeep(0);
				}
			}
		}
		break;
	default:
		break;
	}
}

void CmfccourseView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(m_bEditMode){
		if(m_nEditingBlockIndex>=0){
			switch(nChar){
			case VK_UP:
				OnEditMove(ID_EDIT_MOVE_UP);
				break;
			case VK_DOWN:
				OnEditMove(ID_EDIT_MOVE_DOWN);
				break;
			case VK_LEFT:
				OnEditMove(ID_EDIT_MOVE_LEFT);
				break;
			case VK_RIGHT:
				OnEditMove(ID_EDIT_MOVE_RIGHT);
				break;
			case VK_PRIOR:
				OnEditMove(ID_EDIT_ROTATE_CCW);
				break;
			case VK_NEXT:
				OnEditMove(ID_EDIT_ROTATE_CW);
				break;
			case VK_RETURN:
				FinishBlockEdit();
				break;
			case VK_ESCAPE:
				CancelBlockEdit();
				break;
			default:
				break;
			}
		}
	}else if(m_objPlayingLevel){
		if(m_sRecord.IsEmpty()){
			if(!m_objPlayingLevel->IsAnimating() && !m_objPlayingLevel->IsWin()){
				bool b=false;
				switch(nChar){
				case VK_UP:
					m_nEditingBlockIndex=-1;
					b=m_objPlayingLevel->MovePlayer(0,-1,true);
					break;
				case VK_DOWN:
					m_nEditingBlockIndex=-1;
					b=m_objPlayingLevel->MovePlayer(0,1,true);
					break;
				case VK_LEFT:
					m_nEditingBlockIndex=-1;
					b=m_objPlayingLevel->MovePlayer(-1,0,true);
					break;
				case VK_RIGHT:
					m_nEditingBlockIndex=-1;
					b=m_objPlayingLevel->MovePlayer(1,0,true);
					break;
				case VK_SPACE:
					b=m_objPlayingLevel->SwitchPlayer(-1,-1,true);
					break;
				default:
					return;
				}
				if(b){
					//ad-hoc workaround
					Sleep(TIMER_INTERVAL);

					Invalidate();
					if(m_objPlayingLevel->IsAnimating()){
						SetTimer(ID_TIMER1,TIMER_INTERVAL,NULL);
					}
				}else{
					MessageBeep(0);
				}
			}
		}else{
			switch(nChar){
			case VK_ESCAPE:
				//skip demo
				m_bPlayFromRecord=false;
				m_sRecord.Empty();
				m_nRecordIndex=-1;
				break;
			default:
				MessageBeep(0);
				break;
			}
		}
	}
}

bool CmfccourseView::StartGame()
{
	//delete old level
	if(m_objPlayingLevel){
		delete m_objPlayingLevel;
		m_objPlayingLevel=NULL;
	}

	m_bPlayFromRecord=false;
	m_sRecord.Empty();
	m_nRecordIndex=-1;

	KillTimer(ID_TIMER1);

	if(m_bEditMode) return true;

	m_nEditingBlockIndex=-1;

	//create a copy of current level
	CmfccourseDoc* pDoc = GetDocument();
	if(pDoc==NULL) return false;

	if(m_nCurrentLevel<0) m_nCurrentLevel=0;
	if(m_nCurrentLevel>=pDoc->m_objLevels.GetSize()) m_nCurrentLevel=pDoc->m_objLevels.GetSize()-1;
	if(m_nCurrentLevel<0) return false;

	m_objPlayingLevel=new CLevel(*(pDoc->GetLevel(m_nCurrentLevel)));
	m_objPlayingLevel->StartGame();

	//get the best record of current level
	int st=theApp.m_objRecordMgr.FindAllRecordsOfLevel(*(pDoc->GetLevel(m_nCurrentLevel)),m_tCurrentBestRecord,m_bCurrentChecksum);
	if(st!=-2){
		m_nCurrentBestStep=st;
		m_sCurrentBestStepOwner.Empty();
		if(st>0){
			for(unsigned int i=0;i<m_tCurrentBestRecord.size();i++){
				if(st==m_tCurrentBestRecord[i].nStep){
					if(!m_sCurrentBestStepOwner.IsEmpty()) m_sCurrentBestStepOwner+=_T(", ");
					m_sCurrentBestStepOwner+=m_tCurrentBestRecord[i].sPlayerName;
				}
			}
		}
	}

	return true;
}

void CmfccourseView::FinishGame()
{
	KillTimer(ID_TIMER1);

	memset(m_bCurrentChecksum,0,CLevel::ChecksumSize);

	CmfccourseDoc* pDoc = GetDocument();
	int m=0;
	if(pDoc) m=pDoc->m_objLevels.GetSize();

	if (m_bEditMode || !pDoc || m_nCurrentLevel<0 || m_nCurrentLevel>=m) {
		AfxMessageBox(IDS_LEVEL_FINISHED,MB_ICONINFORMATION);
		return;
	}

	CLevelFinished frm;

	if(m_nCurrentLevel>=0 && m_nCurrentLevel<m-1){
		frm.m_sInfo.LoadString(IDS_LEVEL_FINISHED_2);
	}else{
		frm.m_sInfo.LoadString(IDS_LEVEL_FINISHED_3);
	}

	frm.m_sInfo.AppendFormat(IDS_LEVEL_FINISHED_4,m_objPlayingLevel->m_nMoves);

	if(m_nCurrentBestStep>0) frm.m_sInfo.AppendFormat(_T("%d"),m_nCurrentBestStep);
	else frm.m_sInfo.AppendFormat(IDS_DOES_NOT_EXIST);

	if(m_nCurrentBestStep<=0 || m_nCurrentBestStep>m_objPlayingLevel->m_nMoves){
		frm.m_sInfo.AppendFormat(IDS_LEVEL_FINISHED_5);
	}

	frm.m_objCurrentLevel=pDoc->GetLevel(m_nCurrentLevel);
	frm.m_sPlayerName=theApp.m_sPlayerName;
	frm.m_bPlayFromRecord=m_bPlayFromRecord;

	//show dialog
	frm.DoModal();

	//save player name
	if(frm.m_sPlayerName!=theApp.m_sPlayerName){
		theApp.m_sPlayerName=frm.m_sPlayerName;
		theApp.SavePlayerName();
	}

	//save record
	if(m_bPlayFromRecord){
#ifdef _DEBUG
		OutputDebugString(_T("Play from record, don't save it\n"));
#endif
	}else{
#ifdef _DEBUG
		OutputDebugString(_T("Not play from record, save it\n"));
#endif
		theApp.m_objRecordMgr.AddLevelAndRecord(
			*(pDoc->GetLevel(m_nCurrentLevel)),
			m_objPlayingLevel->m_nMoves,
			m_objPlayingLevel->GetRecord(),
			frm.m_sPlayerName.IsEmpty()?(const wchar_t*)NULL:frm.m_sPlayerName);
	}

	//next level
	if(m_nCurrentLevel>=0 && m_nCurrentLevel<m-1){
		m_nCurrentLevel++;
	}else{
		m_nCurrentLevel=0;
	}

	StartGame();
	Invalidate();
}

void CmfccourseView::OnGameRestart()
{
	StartGame();
	Invalidate();
}

void CmfccourseView::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent==ID_TIMER1 && !m_bEditMode && m_objPlayingLevel){
		if(m_objPlayingLevel->IsAnimating()){
			m_objPlayingLevel->OnTimer();
			Invalidate();
		}else{
			if(m_objPlayingLevel->IsWin()){
				FinishGame();
				return;
			}else if(m_sRecord.IsEmpty()){
				m_bPlayFromRecord=false;
				//get continuous key event
				if(theApp.m_nAnimationSpeed<=2){
					unsigned char b[256];
					const int nKeys[]={VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT,VK_SPACE};
					GetKeyboardState(b);
					//undo?
					if(b[2] & 0x80){
						if(m_objPlayingLevel->CanUndo()){
							OnEditUndo();
						}
					}else{
						for(int i=0;i<sizeof(nKeys)/sizeof(nKeys[0]);i++){
							if(b[nKeys[i]] & 0x80){
								OnKeyDown(nKeys[i],0,0);
								break;
							}
						}
					}
				}
			}else{
				//play from record
				if(m_nRecordIndex<0) m_nRecordIndex=0;
				int m=m_sRecord.GetLength();
				bool bSkip=(GetAsyncKeyState(VK_SHIFT) & 0x8000)!=0;

				if(bSkip) SetHourglassCursor();

				for(;m_nRecordIndex<m;){
					switch(m_sRecord[m_nRecordIndex]){
					case 'A':
					case 'a':
						m_objPlayingLevel->MovePlayer(-1,0,true);
						break;
					case 'W':
					case 'w':
						m_objPlayingLevel->MovePlayer(0,-1,true);
						break;
					case 'D':
					case 'd':
						m_objPlayingLevel->MovePlayer(1,0,true);
						break;
					case 'S':
					case 's':
						m_objPlayingLevel->MovePlayer(0,1,true);
						break;
					case '(':
						{
							int x=0,y=0;
							for(m_nRecordIndex++;m_nRecordIndex<m;m_nRecordIndex++){
								int ch=m_sRecord[m_nRecordIndex];
								if(ch>='0' && ch<='9'){
									x=x*10+ch-'0';
								}else if(ch==','){
									break;
								}
							}
							if(m_nRecordIndex>=m || x<=0 || x>m_objPlayingLevel->m_nWidth){
								m=-1;
								break;
							}
							for(m_nRecordIndex++;m_nRecordIndex<m;m_nRecordIndex++){
								int ch=m_sRecord[m_nRecordIndex];
								if(ch>='0' && ch<='9'){
									y=y*10+ch-'0';
								}else if(ch==')'){
									break;
								}
							}
							if(m_nRecordIndex>=m || y<=0 || y>m_objPlayingLevel->m_nHeight){
								m=-1;
								break;
							}
							m_objPlayingLevel->SwitchPlayer(x-1,y-1,true);
						}
						break;
					default:
						break;
					}
					m_nRecordIndex++;
					if(m_objPlayingLevel->IsAnimating()){
						if(bSkip){
							while(m_objPlayingLevel->IsAnimating()) m_objPlayingLevel->OnTimer();
							if(m_objPlayingLevel->IsWin()) break;
						}else{
							break;
						}
					}
				}

				if(m_nRecordIndex>=m){
					m_sRecord.Empty();
					m_nRecordIndex=-1;
				}

				m_nEditingBlockIndex=-1;

				//show
				Invalidate();
			}

			if(!m_objPlayingLevel->IsAnimating()) KillTimer(ID_TIMER1);
		}
	}

	CView::OnTimer(nIDEvent);
}

void CmfccourseView::EnterBlockEdit(int nIndex,bool bBackup){
	if(m_nEditingBlockIndex>=0) FinishBlockEdit();
	if(m_bEditMode){
		CmfccourseDoc* pDoc = GetDocument();
		if (pDoc) {
			if(m_nCurrentLevel<0) m_nCurrentLevel=0;
			if(m_nCurrentLevel>=pDoc->m_objLevels.GetSize()) m_nCurrentLevel=pDoc->m_objLevels.GetSize()-1;
			CLevel *lev=pDoc->GetLevel(m_nCurrentLevel);
			if(lev && nIndex>=0 && nIndex<lev->m_objBlocks.GetSize()){
				CPushableBlock *objBlock=(CPushableBlock*)(lev->m_objBlocks[nIndex]);
				if(m_objBackupBlock){
					delete m_objBackupBlock;
					m_objBackupBlock=NULL;
				}
				if(bBackup) m_objBackupBlock=new CPushableBlock(*objBlock);
				objBlock->UnoptimizeSize(lev->m_nWidth,lev->m_nHeight);
				m_nEditingBlockIndex=nIndex;
				pDoc->SetModifiedFlag();
			}
		}
	}
	Invalidate();
}

void CmfccourseView::FinishBlockEdit(){
	if(m_bEditMode){
		CmfccourseDoc* pDoc = GetDocument();
		if (pDoc) {
			if(m_nCurrentLevel<0) m_nCurrentLevel=0;
			if(m_nCurrentLevel>=pDoc->m_objLevels.GetSize()) m_nCurrentLevel=pDoc->m_objLevels.GetSize()-1;
			CLevel *lev=pDoc->GetLevel(m_nCurrentLevel);
			if(lev && m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<lev->m_objBlocks.GetSize()){
				CPushableBlock *objBlock=(CPushableBlock*)(lev->m_objBlocks[m_nEditingBlockIndex]);
				if(!objBlock->OptimizeSize()){
					lev->m_objBlocks.RemoveAt(m_nEditingBlockIndex);
					delete objBlock;
				}
				pDoc->SetModifiedFlag();
			}
		}
	}
	if(m_objBackupBlock){
		delete m_objBackupBlock;
		m_objBackupBlock=NULL;
	}
	m_nEditingBlockIndex=-1;
	Invalidate();
}

void CmfccourseView::CancelBlockEdit(){
	if(m_bEditMode){
		CmfccourseDoc* pDoc = GetDocument();
		if (pDoc) {
			if(m_nCurrentLevel<0) m_nCurrentLevel=0;
			if(m_nCurrentLevel>=pDoc->m_objLevels.GetSize()) m_nCurrentLevel=pDoc->m_objLevels.GetSize()-1;
			CLevel *lev=pDoc->GetLevel(m_nCurrentLevel);
			if(lev && m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<lev->m_objBlocks.GetSize()){
				if(m_objBackupBlock){
					//restore to old block
					CPushableBlock *objBlock=(CPushableBlock*)(lev->m_objBlocks[m_nEditingBlockIndex]);
					delete objBlock;
					lev->m_objBlocks[m_nEditingBlockIndex]=m_objBackupBlock;
					m_objBackupBlock=NULL;
				}else{
					//delete current block
					CPushableBlock *objBlock=(CPushableBlock*)(lev->m_objBlocks[m_nEditingBlockIndex]);
					lev->m_objBlocks.RemoveAt(m_nEditingBlockIndex);
					delete objBlock;
				}
				pDoc->SetModifiedFlag();
			}
		}
	}
	if(m_objBackupBlock){
		delete m_objBackupBlock;
		m_objBackupBlock=NULL;
	}
	m_nEditingBlockIndex=-1;
	Invalidate();
}

void CmfccourseView::OnEditProperties()
{
	if(!m_bEditMode) return;
	if(m_nEditingBlockIndex>=0) FinishBlockEdit();

	CmfccourseDoc* pDoc = GetDocument();
	if (!pDoc) return;

	//get current level
	if(m_nCurrentLevel<0) m_nCurrentLevel=0;
	if(m_nCurrentLevel>=pDoc->m_objLevels.GetSize()) m_nCurrentLevel=pDoc->m_objLevels.GetSize()-1;

	CLevel *lev=pDoc->GetLevel(m_nCurrentLevel);
	if(lev==NULL) return;

	CLevelProperties frm;

	frm.m_sLevelPackName=pDoc->m_sLevelPackName;
	frm.m_sLevelName=lev->m_sLevelName;
	frm.m_nWidth=lev->m_nWidth;
	frm.m_nHeight=lev->m_nHeight;

	if(frm.DoModal()==IDOK){
		pDoc->m_sLevelPackName=frm.m_sLevelPackName;
		lev->m_sLevelName=frm.m_sLevelName;
		
		if(frm.m_nWidth!=lev->m_nWidth || frm.m_nHeight!=lev->m_nHeight
			|| (frm.m_bPreserve && (frm.m_nXOffset!=0 || frm.m_nYOffset!=0)))
		{
			lev->Create(frm.m_nWidth,frm.m_nHeight,frm.m_bPreserve,frm.m_nXOffset,frm.m_nYOffset);
		}

		pDoc->SetModifiedFlag();
		pDoc->UpdateAllViews(NULL);
	}
}

void CmfccourseView::OnLevelList(UINT nID)
{
	if(m_bEditMode && m_nEditingBlockIndex>=0) FinishBlockEdit();

	CmfccourseDoc* pDoc = GetDocument();
	if (!pDoc) return;

	//get current level
	int idx=m_nCurrentLevel;
	if(idx<0) idx=0;
	if(idx>=pDoc->m_objLevels.GetSize()) idx=pDoc->m_objLevels.GetSize()-1;

	switch(nID){
	case ID_FIRST_LEVEL:
		idx=0;
		break;
	case ID_PREV_LEVEL:
		if(idx>0) idx--;
		break;
	case ID_NEXT_LEVEL:
		if(idx<pDoc->m_objLevels.GetSize()-1) idx++;
		break;
	case ID_LAST_LEVEL:
		idx=pDoc->m_objLevels.GetSize()-1;
		break;
	case ID_LEVEL_LIST:
		{
			CLevelList frm;
			frm.m_pDoc=pDoc;
			frm.m_nMoveLevelSrc=idx;
			frm.m_nMoveLevel=m_bEditMode?(idx+1):0;

			switch(frm.DoModal()){
			case IDOK:
				idx=frm.m_nMoveLevelSrc;
				break;
			case IDC_MOVE_LEVEL:
				idx=frm.m_nMoveLevelSrc;
				frm.m_nMoveLevel--;
				if(frm.m_nMoveLevel<idx){
					if(frm.m_nMoveLevel<0) frm.m_nMoveLevel=0;
					CObject *lev=pDoc->m_objLevels[idx];
					for(int i=idx;i>frm.m_nMoveLevel;i--){
						pDoc->m_objLevels[i]=pDoc->m_objLevels[i-1];
					}
					pDoc->m_objLevels[frm.m_nMoveLevel]=lev;
					pDoc->SetModifiedFlag();
					pDoc->UpdateAllViews(NULL);
				}else if(frm.m_nMoveLevel>idx){
					if(frm.m_nMoveLevel>=frm.m_nLevelCount) frm.m_nMoveLevel=frm.m_nLevelCount-1;
					CObject *lev=pDoc->m_objLevels[idx];
					for(int i=idx;i<frm.m_nMoveLevel;i++){
						pDoc->m_objLevels[i]=pDoc->m_objLevels[i+1];
					}
					pDoc->m_objLevels[frm.m_nMoveLevel]=lev;
					pDoc->SetModifiedFlag();
					pDoc->UpdateAllViews(NULL);
				}
				idx=frm.m_nMoveLevel;
				break;
			default:
				return;
				break;
			}
		}
		break;
	default:
		if(nID>=0x100000) idx=nID-0x100000;
		break;
	}

	if(idx>=0 && idx<pDoc->m_objLevels.GetSize() && idx!=m_nCurrentLevel){
		m_nCurrentLevel=idx;
		StartGame();
		Invalidate();
	}
}

void CmfccourseView::OnEditAddLevel()
{
	if(!m_bEditMode) return;
	if(m_nEditingBlockIndex>=0) FinishBlockEdit();

	CmfccourseDoc* pDoc = GetDocument();
	if (!pDoc) return;

	//get current level
	int idx=m_nCurrentLevel;
	if(idx>=pDoc->m_objLevels.GetSize()) idx=pDoc->m_objLevels.GetSize()-1;
	if(idx<0) idx=0;

	CLevel *lev=new CLevel();
	lev->CreateDefault();

	if(idx>=pDoc->m_objLevels.GetSize()-1){
		pDoc->m_objLevels.Add(lev);
		idx=pDoc->m_objLevels.GetSize()-1;
	}else{
		if(idx<0) idx=0; else idx++;
		pDoc->m_objLevels.InsertAt(idx,lev);
	}

	m_nCurrentLevel=idx;
	pDoc->SetModifiedFlag();
	pDoc->UpdateAllViews(NULL);
}

void CmfccourseView::OnEditRemoveLevel()
{
	if(!m_bEditMode) return;
	if(m_nEditingBlockIndex>=0) FinishBlockEdit();

	CmfccourseDoc* pDoc = GetDocument();
	if (!pDoc) return;

	//get current level
	int idx=m_nCurrentLevel;
	if(idx>=pDoc->m_objLevels.GetSize()) idx=pDoc->m_objLevels.GetSize()-1;
	if(idx<0) idx=0;

	if(AfxMessageBox(IDS_CONFIRM_DELETE,MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2)==IDYES){
		if(pDoc->m_objLevels.GetSize()==1){
			pDoc->GetLevel(0)->CreateDefault();
			idx=0;
		}else if(pDoc->m_objLevels.GetSize()<=0){
			CLevel *lev=new CLevel();
			lev->CreateDefault();
			pDoc->m_objLevels.Add(lev);
			idx=0;
		}else{
			delete pDoc->m_objLevels[idx];
			pDoc->m_objLevels.RemoveAt(idx);
			if(idx>=pDoc->m_objLevels.GetSize()) idx=pDoc->m_objLevels.GetSize()-1;
		}

		m_nCurrentLevel=idx;
		pDoc->SetModifiedFlag();
		pDoc->UpdateAllViews(NULL);
	}
}

void CmfccourseView::OnEditUndo()
{
	if(m_bEditMode){
		//TODO: edit undo
	}else if(m_objPlayingLevel && !m_objPlayingLevel->IsAnimating() && m_sRecord.IsEmpty()){
		if(m_objPlayingLevel->Undo()){
			m_nEditingBlockIndex=-1;

			//ad-hoc workaround
			Sleep(TIMER_INTERVAL);

			if(m_objPlayingLevel->IsAnimating()){
				SetTimer(ID_TIMER1,TIMER_INTERVAL,NULL);
			}
			Invalidate();
		}else{
			AfxMessageBox(IDS_ERROR_OCCURED);
		}
	}
}

void CmfccourseView::OnEditRedo()
{
	if(m_bEditMode){
		//TODO: edit redo
	}else if(m_objPlayingLevel && !m_objPlayingLevel->IsAnimating() && m_sRecord.IsEmpty()){
		if(m_objPlayingLevel->Redo()){
			m_nEditingBlockIndex=-1;

			//ad-hoc workaround
			Sleep(TIMER_INTERVAL);

			if(m_objPlayingLevel->IsAnimating()){
				SetTimer(ID_TIMER1,TIMER_INTERVAL,NULL);
			}
			Invalidate();
		}else{
			AfxMessageBox(IDS_ERROR_OCCURED);
		}
	}
}

void CmfccourseView::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
	if(m_bEditMode){
		//TODO: update edit undo
		pCmdUI->Enable(0);
	}else{
		if(m_objPlayingLevel){
			pCmdUI->Enable(m_objPlayingLevel->CanUndo());
		}else{
			pCmdUI->Enable(0);
		}
	}
}

void CmfccourseView::OnUpdateEditRedo(CCmdUI *pCmdUI)
{
	if(m_bEditMode){
		//TODO: update edit redo
		pCmdUI->Enable(0);
	}else{
		if(m_objPlayingLevel){
			pCmdUI->Enable(m_objPlayingLevel->CanRedo());
		}else{
			pCmdUI->Enable(0);
		}
	}
}

void CmfccourseView::OnUpdateEditMove(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(0);

	CPushableBlock *objBlock=NULL;

	CmfccourseDoc* pDoc = GetDocument();
	if (!m_bEditMode || pDoc==NULL) return;

	if(m_nCurrentLevel<0) m_nCurrentLevel=0;
	if(m_nCurrentLevel>=pDoc->m_objLevels.GetSize()) m_nCurrentLevel=pDoc->m_objLevels.GetSize()-1;

	CLevel *lev=pDoc->GetLevel(m_nCurrentLevel);
	if(lev && m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<lev->m_objBlocks.GetSize()){
		objBlock=(CPushableBlock*)(lev->m_objBlocks[m_nEditingBlockIndex]);
	}else{
		return;
	}

	switch(pCmdUI->m_nID){
	case ID_EDIT_ROTATE_CCW:
	case ID_EDIT_ROTATE_CW:
		pCmdUI->Enable(objBlock->m_nType==ROTATE_BLOCK);
		break;
	default:
		pCmdUI->Enable();
		break;
	}
}

void CmfccourseView::OnEditMove(UINT nID)
{
	CPushableBlock *objBlock=NULL;

	CmfccourseDoc* pDoc = GetDocument();
	if (!m_bEditMode || pDoc==NULL) return;

	if(m_nCurrentLevel<0) m_nCurrentLevel=0;
	if(m_nCurrentLevel>=pDoc->m_objLevels.GetSize()) m_nCurrentLevel=pDoc->m_objLevels.GetSize()-1;

	CLevel *lev=pDoc->GetLevel(m_nCurrentLevel);
	if(lev && m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<lev->m_objBlocks.GetSize()){
		objBlock=(CPushableBlock*)(lev->m_objBlocks[m_nEditingBlockIndex]);
	}else{
		return;
	}

	switch(nID){
	case ID_EDIT_MOVE_UP:
		objBlock->MoveBlockInEditMode(0,-1);
		break;
	case ID_EDIT_MOVE_DOWN:
		objBlock->MoveBlockInEditMode(0,1);
		break;
	case ID_EDIT_MOVE_LEFT:
		objBlock->MoveBlockInEditMode(-1,0);
		break;
	case ID_EDIT_MOVE_RIGHT:
		objBlock->MoveBlockInEditMode(1,0);
		break;
	case ID_EDIT_ROTATE_CCW:
		if(!objBlock->Rotate(0)) return;
		break;
	case ID_EDIT_ROTATE_CW:
		if(!objBlock->Rotate(1)) return;
		break;
	case ID_EDIT_APPLY_CHANGES:
		FinishBlockEdit();
		return;
		break;
	case ID_EDIT_DISCARD_CHANGES:
		CancelBlockEdit();
		return;
		break;
	default:
		return;
	}

	Invalidate();
	pDoc->SetModifiedFlag();
}

void CmfccourseView::OnEditClear()
{
	CPushableBlock *objBlock=NULL;

	CmfccourseDoc* pDoc = GetDocument();
	if (!m_bEditMode || pDoc==NULL) return;

	if(m_nCurrentLevel<0) m_nCurrentLevel=0;
	if(m_nCurrentLevel>=pDoc->m_objLevels.GetSize()) m_nCurrentLevel=pDoc->m_objLevels.GetSize()-1;

	CLevel *lev=pDoc->GetLevel(m_nCurrentLevel);
	if(lev && m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<lev->m_objBlocks.GetSize()){
		objBlock=(CPushableBlock*)(lev->m_objBlocks[m_nEditingBlockIndex]);
		lev->m_objBlocks.RemoveAt(m_nEditingBlockIndex);
		delete objBlock;

		pDoc->SetModifiedFlag();

		if(m_objBackupBlock){
			delete m_objBackupBlock;
			m_objBackupBlock=NULL;
		}
		m_nEditingBlockIndex=-1;
		Invalidate();
	}else{
		return;
	}
}

void CmfccourseView::OnUpdateEditClear(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(0);

	CmfccourseDoc* pDoc = GetDocument();
	if (!m_bEditMode || pDoc==NULL) return;

	if(m_nCurrentLevel<0) m_nCurrentLevel=0;
	if(m_nCurrentLevel>=pDoc->m_objLevels.GetSize()) m_nCurrentLevel=pDoc->m_objLevels.GetSize()-1;

	CLevel *lev=pDoc->GetLevel(m_nCurrentLevel);
	if(lev && m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<lev->m_objBlocks.GetSize()){
		pCmdUI->Enable();
	}
}

void CmfccourseView::OnToolbarDropdown(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMTOOLBAR *pnmtb = reinterpret_cast<NMTOOLBAR*>(pNMHDR);

	CMenu mnu;
	CWnd *wnd=NULL;

	switch(pnmtb->iItem){
	case ID_LEVEL_LIST:
		{
			CmfccourseDoc* pDoc = GetDocument();
			if (pDoc==NULL || pDoc->m_objLevels.GetSize()<=0) return;

			mnu.CreatePopupMenu();

			for(int i=0;i<pDoc->m_objLevels.GetSize();i++){
				CLevel *lev=pDoc->GetLevel(i);
				CString s;
				if(i<9){
					s.Format(_T("&%d %s"),i+1,lev->m_sLevelName);
				}else if(i==9){
					s.Format(_T("1&0 %s"),lev->m_sLevelName);
				}else{
					s.Format(_T("%d %s"),i+1,lev->m_sLevelName);
				}
				mnu.AppendMenu(i==m_nCurrentLevel?MF_CHECKED:0,i+1,s);
			}

			wnd=&(((CMainFrame*)(theApp.m_pMainWnd))->m_wndToolBar);

			*pResult = 0;

			RECT rc;
			wnd->SendMessage(TB_GETRECT,pnmtb->iItem,(LPARAM)&rc);
			wnd->ClientToScreen(&rc);
			int ret=mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL | TPM_RETURNCMD,
				rc.left,rc.bottom,this,&rc)-1;

			if(ret<0 || ret>=pDoc->m_objLevels.GetSize()) return;
			OnLevelList(ret+0x100000);
		}
		break;
	default:
		((CMainFrame*)(theApp.m_pMainWnd))->OnToolbarDropdown(pNMHDR,pResult);
		return;
	}
}

void CmfccourseView::OnGameRecord()
{
	if(!m_bEditMode && m_objPlayingLevel && !m_objPlayingLevel->IsAnimating()){
		CLevelRecord frm;
		frm.m_pView=this;
		frm.m_sRecord=m_objPlayingLevel->GetRecord();

		int ret=frm.DoModal();

		switch(ret){
		case IDYES:
			//replay record
			{
				//create a copy of current level
				CmfccourseDoc* pDoc = GetDocument();
				if(pDoc==NULL) return;

				if(m_nCurrentLevel<0) m_nCurrentLevel=0;
				if(m_nCurrentLevel>=pDoc->m_objLevels.GetSize()) m_nCurrentLevel=pDoc->m_objLevels.GetSize()-1;
				if(m_nCurrentLevel<0) return;

				CLevel *lev=new CLevel(*(pDoc->GetLevel(m_nCurrentLevel)));
				lev->StartGame();

				//apply record
				if(frm.m_bAutoDemo){
					m_bPlayFromRecord=true;
					m_sRecord=frm.m_sRecord;
					m_nRecordIndex=0;
					SetTimer(ID_TIMER1,TIMER_INTERVAL,NULL);
				}else{
					SetHourglassCursor();
					if(!lev->ApplyRecord(frm.m_sRecord)){
						delete lev;
						AfxMessageBox(IDS_ERROR_OCCURED);
						return;
					}

					m_sRecord.Empty();
					m_nRecordIndex=-1;
				}

				//replace old copy
				delete m_objPlayingLevel;
				m_objPlayingLevel=lev;
				Invalidate();
			}
			break;
		default:
			return;
		}
	}
}

bool CmfccourseView::BeginRecordingVideo(const CLevelRecord* frm)
{
	delete m_objRecordingLevel;
	m_objRecordingLevel=NULL;

	m_nVideoWidth=frm->m_nVideoWidth;
	m_nVideoHeight=frm->m_nVideoHeight;

	//create a copy of current level
	CmfccourseDoc* pDoc = GetDocument();
	if(pDoc==NULL) return false;

	if(m_nCurrentLevel<0) m_nCurrentLevel=0;
	if(m_nCurrentLevel>=pDoc->m_objLevels.GetSize()) m_nCurrentLevel=pDoc->m_objLevels.GetSize()-1;
	if(m_nCurrentLevel<0) return false;

	CLevel *lev=new CLevel(*(pDoc->GetLevel(m_nCurrentLevel)));
	lev->StartGame();

	//apply record
	m_sRecord=frm->m_sRecord;
	m_nRecordIndex=0;

	//over
	m_objRecordingLevel=lev;
	return true;
}

int CmfccourseView::DrawRecordingVideo(CDC* pDC)
{
	if(m_objRecordingLevel==NULL) return -1;

	//draw current frame
	OnDraw(pDC);

	//advance to next frame
	if(m_objRecordingLevel->IsAnimating()){
		m_objRecordingLevel->OnTimer();
	}else if(m_objRecordingLevel->IsWin()){
		return -1;
	}else{
		//play from record
		if(m_nRecordIndex<0) m_nRecordIndex=0;
		int m=m_sRecord.GetLength();

		for(;m_nRecordIndex<m;){
			switch(m_sRecord[m_nRecordIndex]){
			case 'A':
			case 'a':
				m_objRecordingLevel->MovePlayer(-1,0,true);
				break;
			case 'W':
			case 'w':
				m_objRecordingLevel->MovePlayer(0,-1,true);
				break;
			case 'D':
			case 'd':
				m_objRecordingLevel->MovePlayer(1,0,true);
				break;
			case 'S':
			case 's':
				m_objRecordingLevel->MovePlayer(0,1,true);
				break;
			case '(':
				{
					int x=0,y=0;
					for(m_nRecordIndex++;m_nRecordIndex<m;m_nRecordIndex++){
						int ch=m_sRecord[m_nRecordIndex];
						if(ch>='0' && ch<='9'){
							x=x*10+ch-'0';
						}else if(ch==','){
							break;
						}
					}
					if(m_nRecordIndex>=m || x<=0 || x>m_objRecordingLevel->m_nWidth){
						m=-1;
						break;
					}
					for(m_nRecordIndex++;m_nRecordIndex<m;m_nRecordIndex++){
						int ch=m_sRecord[m_nRecordIndex];
						if(ch>='0' && ch<='9'){
							y=y*10+ch-'0';
						}else if(ch==')'){
							break;
						}
					}
					if(m_nRecordIndex>=m || y<=0 || y>m_objRecordingLevel->m_nHeight){
						m=-1;
						break;
					}
					m_objRecordingLevel->SwitchPlayer(x-1,y-1,true);
				}
				break;
			default:
				break;
			}
			m_nRecordIndex++;
			if(m_objRecordingLevel->IsAnimating()) break;
		}

		if(!m_objRecordingLevel->IsAnimating() && m_nRecordIndex>=m){
			m_sRecord.Empty();
			m_nRecordIndex=-1;
		}
	}

	//over
	return m_nRecordIndex;
}

void CmfccourseView::EndRecordingVideo()
{
	delete m_objRecordingLevel;
	m_objRecordingLevel=NULL;

	m_sRecord.Empty();
	m_nRecordIndex=-1;
}

void CmfccourseView::OnUpdateViewGrid(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(theApp.m_bShowGrid);
}

void CmfccourseView::OnViewGrid()
{
	theApp.m_bShowGrid=!theApp.m_bShowGrid;
	theApp.WriteProfileInt(_T("Settings"),_T("ShowGrid"),theApp.m_bShowGrid?1:0);
	Invalidate();
}

void CmfccourseView::OnUpdateViewAnimationSpeed(CCmdUI *pCmdUI){
	pCmdUI->Enable();
	pCmdUI->SetCheck(pCmdUI->m_nID==ID_VIEW_ANIMATION_SPEED+theApp.m_nAnimationSpeed);
}

void CmfccourseView::OnViewAnimationSpeed(UINT nID){
	if(nID>=ID_VIEW_ANIMATION_SPEED){
		theApp.m_nAnimationSpeed=nID-ID_VIEW_ANIMATION_SPEED;
	}else if(nID==ID_VIEW_ANIMATION_SPEED_0){
		if(theApp.m_nAnimationSpeed<3) theApp.m_nAnimationSpeed++;
	}else if(nID==ID_VIEW_ANIMATION_SPEED_00){
		if(theApp.m_nAnimationSpeed>0) theApp.m_nAnimationSpeed--;
	}
	theApp.WriteProfileInt(_T("Settings"),_T("AnimationSpeed"),theApp.m_nAnimationSpeed);
}

void CmfccourseView::OnLevelDatabase()
{
	CLevelDatabase dlg;

	theApp.m_objRecordMgr.LoadWholeFileAndClose(dlg.m_tFile);
	
	int ret=dlg.DoModal();
	if(ret==IDOK){
		theApp.m_objRecordMgr.SaveWholeFile(dlg.m_tFile);
	}

	theApp.m_objRecordMgr.LoadFile();
	memset(m_bCurrentChecksum,0,sizeof(m_bCurrentChecksum));
}

void CmfccourseView::OnToolsImportSokoban()
{
	if(!m_bEditMode) return;
	if(m_nEditingBlockIndex>=0) FinishBlockEdit();

	CmfccourseDoc* pDoc = GetDocument();
	if (!pDoc) return;

	//choose open file name
	CString s;
	s.LoadString(IDS_FILE_FORMAT_TXT);
	CFileDialog cd(TRUE,NULL,NULL,
		OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER,
		s,this);
	if(cd.DoModal()!=IDOK) return;

	//load file
	if(!pDoc->LoadSokobanLevel(cd.GetPathName(),pDoc->m_objLevels.GetSize()<=1)){
		AfxMessageBox(IDS_ERROR_OCCURED,MB_ICONEXCLAMATION);
	}else{
		s.Format(IDS_IMPORT_LEVEL_COMPLETED,pDoc->m_objLevels.GetSize());
		AfxMessageBox(s,MB_ICONINFORMATION);
	}

	if(m_nCurrentLevel>=pDoc->m_objLevels.GetSize()) m_nCurrentLevel=pDoc->m_objLevels.GetSize()-1;
	if(m_nCurrentLevel<0) m_nCurrentLevel=0;
	pDoc->UpdateAllViews(NULL);
}
