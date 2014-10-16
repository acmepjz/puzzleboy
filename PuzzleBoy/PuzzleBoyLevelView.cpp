#include "PuzzleBoyLevelView.h"
#include "PuzzleBoyLevelFile.h"
#include "PuzzleBoyLevel.h"
#include "PuzzleBoyApp.h"
#include "PushableBlock.h"
#include "RecordManager.h"
#include "VertexList.h"
#include "main.h"
#include "NetworkManager.h"

#include <string.h>

#include "include_sdl.h"
#include "include_gl.h"

//ad-hoc!!
extern bool m_bKeyDownProcessed;
extern GLuint adhoc3_tex;

static const int m_nDefaultKey[8]={
	SDLK_UP,
	SDLK_DOWN,
	SDLK_LEFT,
	SDLK_RIGHT,
	SDLK_u, //undo
	SDLK_r, //redo
	SDLK_SPACE, //switch
	0, //restart (Ctrl+R is always available)
};

class PuzzleBoyEditToolbar:public virtual MultiTouchView{
public:
	PuzzleBoyEditToolbar(){
		m_nCurrentTool=0;
		m_nHighlightTool=-1;
		m_nHighlightTimer=0;
	}
	bool OnTimer(){
		//register view
		MultiTouchViewStruct *view=theApp->touchMgr.FindView(this);
		int buttonSize=theApp->m_nButtonSize;
		bool ret=false;
		float f=float(buttonSize)/float(screenHeight);
		if(view){
			view->left=0.0f;
			view->top=1.0f-f*2.0f;
			view->right=screenAspectRatio-f*3.0f;
			view->bottom=1.0f-f;

			if(m_nHighlightTool<0){
				m_nHighlightTimer=0;
			}else if(view->m_nDraggingState==0){
				if((--m_nHighlightTimer)<0){
					m_nHighlightTimer=0;
					m_nHighlightTool=-1;
				}
			}else{
				if((++m_nHighlightTimer)>8) m_nHighlightTimer=8;
			}
			ret=(m_nHighlightTool>=0);
		}else{
			theApp->touchMgr.AddView(0.0f,
				1.0f-f*2.0f,
				screenAspectRatio-f*3.0f,
				1.0f-f,
				MultiTouchViewFlags::AcceptDragging,this,0);
		}

		//resize scroll view
		m_scrollView.m_screen.y=screenHeight-buttonSize*2;
		m_scrollView.m_screen.w=screenWidth-buttonSize*3;
		m_scrollView.m_screen.h=buttonSize;
		m_scrollView.m_virtual.w=buttonSize*10;
		m_scrollView.m_virtual.h=buttonSize;
		m_scrollView.m_flags=SimpleScrollViewFlags::Horizontal
			|SimpleScrollViewFlags::DrawScrollBar
			|SimpleScrollViewFlags::FlushLeft
			|SimpleScrollViewFlags::FlushTop;

		//update scroll view
		ret|=m_scrollView.OnTimer();

		return ret;
	}
	void Draw(){
		//draw background
		SetProjectionMatrix(0);
		m_scrollView.DisableScissorRect();
		int buttonSize=theApp->m_nButtonSize;
		{
			float f=float(buttonSize)/float(screenHeight);
			float x2=screenAspectRatio-f*3.0f;
			float y1=1.0f-f*2.0f,y2=1.0f-f;
			float vv[8]={
				0,y1,
				0,y2,
				x2,y1,
				x2,y2,
			};

			unsigned short ii[6]={0,1,3,0,3,2};

			glColor4f(0.25f, 0.25f, 0.25f, 0.5f);

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2,GL_FLOAT,0,vv);
			glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,ii);
			glDisableClientState(GL_VERTEX_ARRAY);
		}

		m_scrollView.SetProjectionMatrix();
		m_scrollView.EnableScissorRect();
		//draw selection
		{
			float x1=float(m_nCurrentTool*buttonSize);
			float x2=x1+float(buttonSize);
			float vv[8]={
				x1,0,
				x1,float(buttonSize),
				x2,0,
				x2,float(buttonSize),
			};

			unsigned short ii[6]={0,1,3,0,3,2};

			glColor4f(0.25f, 0.25f, 0.25f, 1.0f);

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2,GL_FLOAT,0,vv);
			glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,ii);
			glDisableClientState(GL_VERTEX_ARRAY);
		}
		//draw highlight
		if(m_nHighlightTool>=0 && m_nHighlightTimer>0){
			float x1=float(m_nHighlightTool*buttonSize);
			float x2=x1+float(buttonSize);
			float vv[8]={
				x1,0,
				x1,float(buttonSize),
				x2,0,
				x2,float(buttonSize),
			};

			unsigned short ii[6]={0,1,3,0,3,2};

			glColor4f(0.5f, 0.5f, 0.5f, float(m_nHighlightTimer)*0.125f);

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2,GL_FLOAT,0,vv);
			glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,ii);
			glDisableClientState(GL_VERTEX_ARRAY);
		}
		//draw icons
		{
			float vv[16]={
				0,0,0,0,
				0,float(buttonSize),0,1,
				float(buttonSize*16),0,1,0,
				float(buttonSize*16),float(buttonSize),1,1,
			};

			unsigned short ii[6]={0,1,3,0,3,2};

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, adhoc3_tex);

			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glVertexPointer(2,GL_FLOAT,4*sizeof(float),vv);
			glTexCoordPointer(2,GL_FLOAT,4*sizeof(float),vv+2);

			glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,ii);

			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);
		}
		//draw scroll bar
		m_scrollView.Draw();
		m_scrollView.DisableScissorRect();
	}
	void OnMultiGesture(float fx,float fy,float dx,float dy,float zoom) override{
		m_scrollView.OnMultiGesture(fx,fy,dx,0,1.0f);
	}
	void OnMouseEvent(int which,int state,int xMouse,int yMouse,int nFlags,int nType) override{
		switch(nType){
		case SDL_MOUSEBUTTONDOWN:
			m_scrollView.TranslateCoordinate(xMouse,yMouse,xMouse,yMouse);
			if(xMouse>=0 && yMouse>=0 && yMouse<theApp->m_nButtonSize){
				xMouse/=theApp->m_nButtonSize;
				if(xMouse>=0 && xMouse<10) m_nHighlightTool=xMouse;
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if(m_nHighlightTool>=0 && m_nHighlightTool<10) m_nCurrentTool=m_nHighlightTool;
			break;
		}
	}
	void OnMouseWheel(int which,int x,int y,int dx,int dy,int nFlags) override{
		m_scrollView.OnMultiGesture(0,0,float(theApp->m_nButtonSize*(dy>0?2:-2))/float(screenHeight),0,1.0f);
	}
public:
	SimpleScrollView m_scrollView;
	int m_nCurrentTool,m_nHighlightTool,m_nHighlightTimer;
};

PuzzleBoyLevelView::PuzzleBoyLevelView()
:m_toolbar(NULL)
,m_nScreenKeyboardType(0)
,m_nScreenKeyboardPressedIndex(-1)
,m_pDocument(NULL)
,m_nCurrentLevel(0)
,m_nMode(PLAY_MODE)
,m_bPlayFromRecord(false)
,m_bSkipRecord(false)
,m_nEditingBlockIndex(-1)
,m_objBackupBlock(NULL)
,m_objPlayingLevel(NULL)
,m_nRecordIndex(-1)
,m_nKey(m_nDefaultKey)
{
}

#define m_bEditMode (m_nMode==EDIT_MODE)
#define m_bTestMode (m_nMode==TEST_MODE)

PuzzleBoyLevelView::~PuzzleBoyLevelView(){
	delete m_toolbar;
	delete m_objPlayingLevel;
	delete m_objBackupBlock;
}

void PuzzleBoyLevelView::Draw(){
	m_nScreenKeyboardType=0;

	if(m_objPlayingLevel){
		//draw level
		m_objPlayingLevel->Draw(m_bEditMode,m_nEditingBlockIndex);

		if(!m_bEditMode && m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<(int)m_objPlayingLevel->m_objBlocks.size()){
			//draw selected block in game mode
			PushableBlock *objBlock=m_objPlayingLevel->m_objBlocks[m_nEditingBlockIndex];
			if(objBlock->m_nType==ROTATE_BLOCK){
				std::vector<float> v;
				std::vector<unsigned short> idx;

				AddScreenKeyboard(float(m_nEditingBlockDX),
					float(m_nEditingBlockDY),
					1.0f,1.0f,SCREEN_KEYBOARD_NO,v,idx);

				if(m_nEditingBlockDX==objBlock->m_x){
					if(m_nEditingBlockX<=objBlock->m_x){
						//left
						AddScreenKeyboard(float(m_nEditingBlockDX-1),
							float(m_nEditingBlockDY),
							1.0f,1.0f,SCREEN_KEYBOARD_LEFT,v,idx);
					}
					if(m_nEditingBlockX>=objBlock->m_x){
						//right
						AddScreenKeyboard(float(m_nEditingBlockDX+1),
							float(m_nEditingBlockDY),
							1.0f,1.0f,SCREEN_KEYBOARD_RIGHT,v,idx);
					}
				}else{
					if(m_nEditingBlockY<=objBlock->m_y){
						//up
						AddScreenKeyboard(float(m_nEditingBlockDX),
							float(m_nEditingBlockDY-1),
							1.0f,1.0f,SCREEN_KEYBOARD_UP,v,idx);
					}
					if(m_nEditingBlockY>=objBlock->m_y){
						//down
						AddScreenKeyboard(float(m_nEditingBlockDX),
							float(m_nEditingBlockDY+1),
							1.0f,1.0f,SCREEN_KEYBOARD_DOWN,v,idx);
					}
				}

				DrawScreenKeyboard(v,idx);
			}else{
				//enable yes/no screen keyboard
				if(theApp->IsTouchscreen()) m_nScreenKeyboardType=1;

				//draw floating normal blocks
				glTranslatef(float(m_nEditingBlockX-objBlock->m_x),
					float(m_nEditingBlockY-objBlock->m_y),0.0f);

				if(objBlock->m_faces){
					glDisable(GL_LIGHTING);
					if(objBlock->m_nType==NORMAL_BLOCK) glColor4f(PUSH_BLOCK_COLOR_trans);
					else glColor4f(TARGET_BLOCK_COLOR_trans);
					objBlock->m_faces->Draw();
				}

				if(objBlock->m_shade){
					glEnable(GL_LIGHTING);
					glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
						(objBlock->m_nType==NORMAL_BLOCK)?PUSH_BLOCK_COLOR_trans_mat:TARGET_BLOCK_COLOR_trans_mat);
					objBlock->m_shade->Draw();
				}

				glDisable(GL_LIGHTING);
				if(theApp->m_bShowLines && objBlock->m_lines){
					glColor4f(0.0f,0.0f,0.0f,1.0f);
					objBlock->m_lines->Draw();
				}

				//reset world matrix!
				glLoadIdentity();
			}
		}

		//enable fast forward/no screen kwyboard regardless of touchscreen mode
		if(!m_sRecord.empty()) m_nScreenKeyboardType=2;

		//draw scroll bar
		m_scrollView.Draw();

		//draw toolbar
		if(m_bEditMode && m_toolbar){
			m_toolbar->Draw();
		}

		//draw screen keypad
		SetProjectionMatrix(1);
		{
			int x1=m_scrollView.m_screen.x+m_scrollView.m_screen.w;
			int y1=m_scrollView.m_screen.y+m_scrollView.m_screen.h;
			int buttonSize=theApp->m_nButtonSize;

			std::vector<float> v;
			std::vector<unsigned short> i;
			v.reserve(64);
			i.reserve(64);

			if(m_bEditMode){
				//add default screen keyboard
				float v0[16]={
					float(x1-3*buttonSize),float(y1+buttonSize),0.0f,SCREENKB_H,
					float(x1),float(y1+buttonSize),SCREENKB_W*3.0f,SCREENKB_H,
					float(x1),float(y1+2*buttonSize),SCREENKB_W*3.0f,SCREENKB_H*2.0f,
					float(x1-3*buttonSize),float(y1+2*buttonSize),0.0f,SCREENKB_H*2.0f,
				};

				const unsigned short i0[6]={
					0,1,2,0,2,3,
				};

				v.insert(v.end(),v0,v0+16);
				i.insert(i.end(),i0,i0+6);

				AddScreenKeyboard(float(x1-3*buttonSize),float(y1),
					float(buttonSize),float(buttonSize),SCREEN_KEYBOARD_ZOOM_IN,v,i);
				AddScreenKeyboard(float(x1-2*buttonSize),float(y1),
					float(buttonSize),float(buttonSize),SCREEN_KEYBOARD_UP,v,i);
				AddScreenKeyboard(float(x1-buttonSize),float(y1),
					float(buttonSize),float(buttonSize),SCREEN_KEYBOARD_ZOOM_OUT,v,i);

				//determine the position of additional screen keyboard
				int xxx=m_scrollView.m_screen.w/buttonSize;
				if(xxx>=6){
					xxx=m_scrollView.m_screen.w-7*buttonSize;
					if(xxx<0) xxx=0;
				}else{
					xxx=m_scrollView.m_screen.w-6*buttonSize;
				}
				m_nScreenKeyboardPos[0]=xxx;
				m_nScreenKeyboardPos[1]=m_scrollView.m_screen.h+buttonSize;

				if(m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<(int)m_objPlayingLevel->m_objBlocks.size()){
					m_nScreenKeyboardType=1;
					AddScreenKeyboard(float(m_scrollView.m_screen.x+xxx),float(y1+buttonSize),
						float(buttonSize),float(buttonSize),SCREEN_KEYBOARD_YES,v,i);
					AddScreenKeyboard(float(m_scrollView.m_screen.x+xxx+buttonSize),float(y1+buttonSize),
						float(buttonSize),float(buttonSize),SCREEN_KEYBOARD_NO,v,i);
					AddScreenKeyboard(float(m_scrollView.m_screen.x+xxx+buttonSize*2),float(y1+buttonSize),
						float(buttonSize),float(buttonSize),SCREEN_KEYBOARD_DELETE,v,i);
				}else{
					AddScreenKeyboard(float(x1-4*buttonSize),float(y1+buttonSize),
						float(buttonSize),float(buttonSize),SCREEN_KEYBOARD_PLAY,v,i);
				}
			}else{
				//add default screen keyboard
				float v0[16]={
					float(x1-4*buttonSize),float(y1),0.0f,0.0f,
					float(x1),float(y1),SCREENKB_W*4.0f,0.0f,
					float(x1),float(y1+2*buttonSize),SCREENKB_W*4.0f,SCREENKB_H*2.0f,
					float(x1-4*buttonSize),float(y1+2*buttonSize),0.0f,SCREENKB_H*2.0f,
				};

				const unsigned short i0[6]={
					0,1,2,0,2,3,
				};

				v.insert(v.end(),v0,v0+16);
				i.insert(i.end(),i0,i0+6);

				//determine the position of additional screen keyboard
				int xxx=m_scrollView.m_screen.w/buttonSize;
				if(xxx>=6){
					xxx=m_scrollView.m_screen.w-7*buttonSize;
					if(xxx<0) xxx=0;
					m_nScreenKeyboardPos[0]=xxx;
					m_nScreenKeyboardPos[1]=m_scrollView.m_screen.h+buttonSize;
					m_nScreenKeyboardPos[2]=xxx+buttonSize;
					m_nScreenKeyboardPos[3]=m_nScreenKeyboardPos[1];
				}else{
					xxx=m_scrollView.m_screen.w-5*buttonSize;
					if(xxx>0) xxx=0;
					m_nScreenKeyboardPos[0]=xxx;
					m_nScreenKeyboardPos[1]=m_scrollView.m_screen.h;
					m_nScreenKeyboardPos[2]=xxx;
					m_nScreenKeyboardPos[3]=m_scrollView.m_screen.h+buttonSize;
				}

				//add additional screen keyboard
				switch(m_nScreenKeyboardType){
				case 1: //yes/no
				case 2: //fast forward/no
					AddScreenKeyboard(float(m_scrollView.m_screen.x+m_nScreenKeyboardPos[0]),
						float(m_scrollView.m_screen.y+m_nScreenKeyboardPos[1]),
						float(buttonSize),
						float(buttonSize),
						m_nScreenKeyboardType==2?SCREEN_KEYBOARD_FAST_FORWARD:SCREEN_KEYBOARD_YES,v,i);
					AddScreenKeyboard(float(m_scrollView.m_screen.x+m_nScreenKeyboardPos[2]),
						float(m_scrollView.m_screen.y+m_nScreenKeyboardPos[3]),
						float(buttonSize),
						float(buttonSize),
						SCREEN_KEYBOARD_NO,v,i);
					break;
				}

				switch(m_scrollView.m_nOrientation){
				case 2:
					for(int i=0,m=v.size();i<m;i+=4){
						v[i]=float(m_scrollView.m_screen.x*2+m_scrollView.m_screen.w)-v[i];
						v[i+1]=float(m_scrollView.m_screen.y*2+m_scrollView.m_screen.h)-v[i+1];
					}
					break;
				}
			}

			DrawScreenKeyboard(v,i);
		}
	}
}

bool PuzzleBoyLevelView::StartGame(){
	//delete old level
	if(m_objPlayingLevel){
		delete m_objPlayingLevel;
		m_objPlayingLevel=NULL;
	}
	if(m_toolbar){
		theApp->touchMgr.RemoveView(m_toolbar);
		delete m_toolbar;
		m_toolbar=NULL;
	}

	m_bPlayFromRecord=false;
	m_bSkipRecord=false;
	m_sRecord.clear();
	m_nRecordIndex=-1;

	m_nEditingBlockIndex=-1;

	m_nScreenKeyboardPressedIndex=-1;

	//create a copy of current level
	PuzzleBoyLevelFile* pDoc = m_pDocument;
	if(pDoc==NULL) return false;

	if(m_nCurrentLevel<0) m_nCurrentLevel=0;
	if(m_nCurrentLevel>=(int)pDoc->m_objLevels.size()) m_nCurrentLevel=pDoc->m_objLevels.size()-1;
	if(m_nCurrentLevel<0) return false;

	m_objPlayingLevel=new PuzzleBoyLevel(*(pDoc->GetLevel(m_nCurrentLevel)));
	m_objPlayingLevel->m_bSendNetworkMove=(m_nMode==PLAY_MODE && netMgr && netMgr->IsNetworkMultiplayer());
	m_objPlayingLevel->m_view=this;
	m_objPlayingLevel->StartGame();

	if(m_bEditMode){
		m_toolbar=new PuzzleBoyEditToolbar;
		m_toolbar->OnTimer();
		m_toolbar->m_scrollView.CenterView(0,0,0,0);
	}else{
		//get the best record of current level
		int st=theApp->m_objRecordMgr.FindAllRecordsOfLevel(*(pDoc->GetLevel(m_nCurrentLevel)),m_tCurrentBestRecord,m_bCurrentChecksum);
		if(st!=-2){
			m_nCurrentBestStep=st;
			m_sCurrentBestStepOwner.clear();
			if(st>0){
				for(unsigned int i=0;i<m_tCurrentBestRecord.size();i++){
					if(st==m_tCurrentBestRecord[i].nStep){
						if(!m_sCurrentBestStepOwner.empty()){
							m_sCurrentBestStepOwner.push_back(',');
							m_sCurrentBestStepOwner.push_back(' ');
						}
						m_sCurrentBestStepOwner+=toUTF8(m_tCurrentBestRecord[i].sPlayerName);
					}
				}
			}
		}
	}

	//create graphics
	m_objPlayingLevel->CreateGraphics();

	//FIXME: ad-hoc one
	SDL_Rect r={0,0,m_objPlayingLevel->m_nWidth,m_objPlayingLevel->m_nHeight};
	m_scrollView.m_virtual=r;
	m_scrollView.CenterView();

	//show a tooltip
	if(m_bEditMode){
		theApp->ShowToolTip(_("Start editing level"));
	}else if(m_bTestMode){
		theApp->ShowToolTip(_("Start testing level"));
	}

	return true;
}

void PuzzleBoyLevelView::SaveEdit(){
	if(m_bEditMode && m_pDocument && m_objPlayingLevel
		&& m_nCurrentLevel>=0 && m_nCurrentLevel<(int)m_pDocument->m_objLevels.size())
	{
		FinishBlockEdit();

		int m=m_objPlayingLevel->m_nWidth*m_objPlayingLevel->m_nHeight;

		for(int i=0;i<m;i++){
			if(m_objPlayingLevel->m_bTargetData[i]){
				switch(m_objPlayingLevel->m_bMapData[i]){
				case FLOOR_TILE: m_objPlayingLevel->m_bMapData[i]=TARGET_TILE; break;
				case PLAYER_TILE: m_objPlayingLevel->m_bMapData[i]=PLAYER_AND_TARGET_TILE; break;
				}
			}
		}

		delete m_pDocument->m_objLevels[m_nCurrentLevel];
		m_pDocument->m_objLevels[m_nCurrentLevel]=new PuzzleBoyLevelData(*m_objPlayingLevel);

		for(int i=0;i<m;i++){
			switch(m_objPlayingLevel->m_bMapData[i]){
			case TARGET_TILE: m_objPlayingLevel->m_bMapData[i]=FLOOR_TILE; break;
			case PLAYER_AND_TARGET_TILE: m_objPlayingLevel->m_bMapData[i]=PLAYER_TILE; break;
			}
		}
	}
}

void PuzzleBoyLevelView::FinishGame(){
	memset(m_bCurrentChecksum,0,PuzzleBoyLevelData::ChecksumSize);

	PuzzleBoyLevelFile* pDoc = m_pDocument;
	int m=0;
	if(pDoc) m=pDoc->m_objLevels.size();

	if (m_bEditMode || !pDoc || m_nCurrentLevel<0 || m_nCurrentLevel>=m) {
		//AfxMessageBox(_T("恭喜你完成本关卡!"),MB_ICONINFORMATION);
		return;
	}

	if (m_bTestMode) {
		m_nMode=EDIT_MODE;
		StartGame();
		return;
	}

	/*CLevelFinished frm;

	if(m_nCurrentLevel>=0 && m_nCurrentLevel<m-1){
		frm.m_sInfo=_T("恭喜你完成本关卡! 进入下一关");
	}else{
		frm.m_sInfo=_T("恭喜你完成本关卡! 回到第一关");
	}

	frm.m_sInfo.AppendFormat(_T("\n步数: %d - 最佳纪录: "),m_objPlayingLevel->m_nMoves);

	if(m_nCurrentBestStep>0) frm.m_sInfo.AppendFormat(_T("%d"),m_nCurrentBestStep);
	else frm.m_sInfo+=_T("不存在");

	if(m_nCurrentBestStep<=0 || m_nCurrentBestStep>m_objPlayingLevel->m_nMoves){
		frm.m_sInfo+=_T("\n恭喜你打破最佳纪录!");
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
	}*/

	//save record
	if(!m_bPlayFromRecord){
		theApp->m_objRecordMgr.AddLevelAndRecord(
			*(pDoc->GetLevel(m_nCurrentLevel)),
			m_objPlayingLevel->m_nMoves,
			m_objPlayingLevel->GetRecord(),
			m_sPlayerName.empty()?(const unsigned short*)NULL:m_sPlayerName.c_str());
	}

	//next level
	if(m_nCurrentLevel>=0 && m_nCurrentLevel<m-1){
		m_nCurrentLevel++;
	}else{
		m_nCurrentLevel=0;
	}

	//update theApp->m_nCurrentLevel
	if(!theApp->m_view.empty() && theApp->m_view[0]==this){
		theApp->m_nCurrentLevel=m_nCurrentLevel;
	}

	StartGame();
}

void PuzzleBoyLevelView::Undo(){
	if(m_bEditMode){
		//TODO: edit undo
	}else if(m_objPlayingLevel && !m_objPlayingLevel->IsAnimating() && m_sRecord.empty()){
		if(!m_objPlayingLevel->CanUndo()){
			theApp->ShowToolTip(_("Can't undo"));
		}else if(m_objPlayingLevel->Undo()){
			m_nEditingBlockIndex=-1;
		}else{
			theApp->ShowToolTip(_("Failed to undo"));
		}
	}
}

void PuzzleBoyLevelView::Redo(){
	if(m_bEditMode){
		//TODO: edit redo
	}else if(m_objPlayingLevel && !m_objPlayingLevel->IsAnimating() && m_sRecord.empty()){
		if(!m_objPlayingLevel->CanRedo()){
			theApp->ShowToolTip(_("Can't redo"));
		}else if(m_objPlayingLevel->Redo()){
			m_nEditingBlockIndex=-1;
		}else{
			theApp->ShowToolTip(_("Failed to redo"));
		}
	}
}

bool PuzzleBoyLevelView::OnTimer(){
	bool bDirty=m_scrollView.OnTimer();

	if(m_bEditMode){
		MultiTouchViewStruct *viewStruct=theApp->touchMgr.FindView(this);

		if(m_toolbar){
			bDirty|=m_toolbar->OnTimer();

			int t=m_toolbar->m_nHighlightTool;
			if(t>=1 && t<=6 && m_nEditingBlockIndex>=0) FinishBlockEdit();

			if(viewStruct){
				viewStruct->flags=(m_toolbar->m_nCurrentTool==0 && m_nEditingBlockIndex<0)?
					(MultiTouchViewFlags::AcceptDragging|MultiTouchViewFlags::AcceptZoom):0;
			}
		}

		//check screen keyboard
		if(theApp->m_bContinuousKey && m_nScreenKeyboardPressedIndex>=0
			&& viewStruct && viewStruct->m_nDraggingState==3)
		{
			InternalKeyDown(m_nScreenKeyboardPressedIndex);
			bDirty=true;
		}else{
			m_nScreenKeyboardPressedIndex=-1;
		}
	}

	if(!m_bEditMode && m_objPlayingLevel){
		if(m_objPlayingLevel->IsAnimating()){
			bDirty=true;
			m_objPlayingLevel->OnTimer();
		}else{
			if(m_objPlayingLevel->IsWin()){
				FinishGame();
				return true;
			}else if(m_sRecord.empty()){
				m_bPlayFromRecord=false;

				//get continuous key event
				if(m_nKey && theApp->m_bContinuousKey){
					const Uint8 *bKey=SDL_GetKeyboardState(NULL);
					bool b=false;

					//only arrow keys and undo/redo
					for(int i=0;i<6;i++){
						if(m_nKey[i] && bKey[SDL_GetScancodeFromKey(m_nKey[i])]){
							InternalKeyDown(i);
							bDirty=true;
							b=true;
							break;
						}
					}

					MultiTouchViewStruct *viewStruct=theApp->touchMgr.FindView(this);

					//check right button
					if(!b && m_nEditingBlockIndex<0
						&& viewStruct && viewStruct->m_nDraggingState
						&& (SDL_GetMouseState(NULL,NULL) & SDL_BUTTON_RMASK)!=0)
					{
						if(SDL_GetModState() & KMOD_SHIFT){
							//redo
							InternalKeyDown(5);
							bDirty=true;
							b=true;
						}else{
							//undo
							InternalKeyDown(4);
							bDirty=true;
							b=true;
						}
					}

					//check screen keyboard
					if(m_nScreenKeyboardPressedIndex>=0){
						if(viewStruct && viewStruct->m_nDraggingState==3){
							if(!b){
								InternalKeyDown(m_nScreenKeyboardPressedIndex);
								bDirty=true;
								b=true;
							}
						}else{
							m_nScreenKeyboardPressedIndex=-1;
						}
					}
				}

				//get network move
				if(m_nMode==READONLY_MODE && netMgr->IsNetworkMultiplayer()){
					NetworkMove move;

					if(netMgr->GetNextReceivedMove(move)){
						switch(move.type){
						case 0: case 1: case 2: case 3:
						case 4: case 5: case 7:
							InternalKeyDown(move.type);
							bDirty=true;
							break;
						case 6:
							m_objPlayingLevel->SwitchPlayer(move.x,move.y,true);
							bDirty=true;
							break;
						default:
							break;
						}
					}
				}
			}else{
				//play from record
				if(m_nRecordIndex<0) m_nRecordIndex=0;
				int m=m_sRecord.size();

				bool bSkip=m_bSkipRecord || (SDL_GetModState() & KMOD_SHIFT)!=0;

				//TODO: SetHourglassCursor()
				//if(bSkip) SetHourglassCursor();*/

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
					m_sRecord.clear();
					m_nRecordIndex=-1;
				}

				m_bSkipRecord=false;

				m_nEditingBlockIndex=-1;
				bDirty=true;
			}
		}
	}

	return bDirty;
}

bool PuzzleBoyLevelView::OnKeyDown(int nChar,int nFlags){
	if(m_bEditMode){
		//TODO: edit mode
		/*if(m_nEditingBlockIndex>=0){
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
		}*/
	}else if(m_nKey && m_objPlayingLevel){
		if(nFlags){
			if(nChar==SDLK_r && (nFlags & KMOD_CTRL)!=0){
				InternalKeyDown(7);
				return true;
			}
			return false;
		}

		int keyIndex=-1;

		if(nChar==SDLK_RETURN) keyIndex=64;
		else if(nChar==SDLK_ESCAPE) keyIndex=65;
		else{
			for(int i=0;i<8;i++){
				if(m_nKey[i] && nChar==m_nKey[i]){
					keyIndex=i;
					break;
				}
			}
		}

		return InternalKeyDown(keyIndex);
	}

	return false;
}

bool PuzzleBoyLevelView::InternalKeyDown(int keyIndex){
	if(m_bEditMode && m_objPlayingLevel){
		//edit mode
		switch(keyIndex){
		case 0: //up
			m_scrollView.OnMultiGesture(0,0,0,16.0f/float(screenHeight),1.0f);
			return true;
			break;
		case 1: //down
			m_scrollView.OnMultiGesture(0,0,0,-16.0f/float(screenHeight),1.0f);
			return true;
			break;
		case 2: //left
			m_scrollView.OnMultiGesture(0,0,16.0f/float(screenHeight),0,1.0f);
			return true;
			break;
		case 3: //right
			m_scrollView.OnMultiGesture(0,0,-16.0f/float(screenHeight),0,1.0f);
			return true;
			break;
		case 32: //zoom in
			m_scrollView.OnMultiGesture(0.5f*screenAspectRatio,
				0.5f*float(m_scrollView.m_screen.h)/float(screenHeight),
				0,0,1.05f);
			return true;
			break;
		case 33: //zoom out
			m_scrollView.OnMultiGesture(0.5f*screenAspectRatio,
				0.5f*float(m_scrollView.m_screen.h)/float(screenHeight),
				0,0,0.96f);
			return true;
			break;
		case 34: //test play
			SaveEdit();
			m_nMode=TEST_MODE;
			StartGame();
			return true;
			break;
		case 64: //apply
			FinishBlockEdit();
			return true;
			break;
		case 65: //discard
			CancelBlockEdit();
			return true;
			break;
		case 66: //delete
			if(m_bEditMode && m_pDocument && m_objPlayingLevel
				&& m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<(int)m_objPlayingLevel->m_objBlocks.size())
			{
				PushableBlock *objBlock=m_objPlayingLevel->m_objBlocks[m_nEditingBlockIndex];
				m_objPlayingLevel->m_objBlocks.erase(m_objPlayingLevel->m_objBlocks.begin()+m_nEditingBlockIndex);
				delete objBlock;

				m_objPlayingLevel->CreateGraphics();
				m_pDocument->SetModifiedFlag();

				if(m_objBackupBlock){
					delete m_objBackupBlock;
					m_objBackupBlock=NULL;
				}
				m_nEditingBlockIndex=-1;
			}
			return true;
			break;
		}
	}

	if(!m_bEditMode && m_objPlayingLevel){
		if(keyIndex==7){
			if(m_nMode==PLAY_MODE && netMgr->IsNetworkMultiplayer()){
				NetworkMove move={7,0,0};
				netMgr->SendPlayerMove(move);
			}

			StartGame();
			return true;
		}

		if(m_sRecord.empty()){
			if(!m_objPlayingLevel->IsAnimating() && !m_objPlayingLevel->IsWin()){
				bool b=false;

				switch(keyIndex){
				case 0: //up
					m_nEditingBlockIndex=-1;
					b=m_objPlayingLevel->MovePlayer(0,-1,true);
					break;
				case 1: //down
					m_nEditingBlockIndex=-1;
					b=m_objPlayingLevel->MovePlayer(0,1,true);
					break;
				case 2: //left
					m_nEditingBlockIndex=-1;
					b=m_objPlayingLevel->MovePlayer(-1,0,true);
					break;
				case 3: //right
					m_nEditingBlockIndex=-1;
					b=m_objPlayingLevel->MovePlayer(1,0,true);
					break;
				case 4: //undo
					Undo();
					return true;
					break;
				case 5: //redo
					Redo();
					return true;
					break;
				case 6: //switch
					b=m_objPlayingLevel->SwitchPlayer(-1,-1,true);
					break;
				case 64: //move a moveable block to new position
					if(m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<(int)m_objPlayingLevel->m_objBlocks.size()){
						//move a moveable block to new position
						u8string s;
						int idx=m_nEditingBlockIndex;
						m_nEditingBlockIndex=-1;

						PushableBlock *objBlock=m_objPlayingLevel->m_objBlocks[idx];

						if(objBlock->m_nType==ROTATE_BLOCK){
							int dx=0,dy=0;
							m_nEditingBlockX;m_nEditingBlockY;
							if(m_nEditingBlockDX==objBlock->m_x){
								if(m_nEditingBlockY==m_nEditingBlockDY){
									switch(m_nEditingBlockX-objBlock->m_x){
									case -1: dx=-1; break;
									case 0: dx=-42; break;
									case 1: dx=1; break;
									}
								}
							}else{
								if(m_nEditingBlockX==m_nEditingBlockDX){
									switch(m_nEditingBlockY-objBlock->m_y){
									case -1: dy=-1; break;
									case 0: dx=-42; break;
									case 1: dy=1; break;
									}
								}
							}

							if(dx==0 && dy==0){
								//clicked the wrong position
								m_nEditingBlockIndex=idx;
								return true;
							}else if(dx==-42){
								//cancelled
								return true;
							}

							int xx=m_nEditingBlockDX-dx;
							int yy=m_nEditingBlockDY-dy;

							if(m_objPlayingLevel->CheckIfCurrentPlayerCanMove(dx,dy,xx,yy)
								&& m_objPlayingLevel->FindPath(s,xx,yy))
							{
								s.push_back(dx<0?'A':dy<0?'W':dx>0?'D':'S');
							}
						}else{
							m_objPlayingLevel->FindPathForBlock(s,idx,m_nEditingBlockX,m_nEditingBlockY);
						}

						if(!s.empty()){
							m_sRecord=s;
							m_nRecordIndex=0;
						}else{
							theApp->ShowToolTip(_("Can't move to specified location"));
						}

						return true;
					}
					return false;

					break;
				case 65: //cancel moving blocks
					if(m_nEditingBlockIndex>=0){
						//cancel moving blocks
						m_nEditingBlockIndex=-1;
						return true;
					}
					return false;
					break;
				default:
					return false;
				}

				if(!b){
					//TODO: MessageBeep(0);
				}
				return true;
			}
		}else{
			switch(keyIndex){
			case 64:
				//fast forward
				if(!m_sRecord.empty()){
					m_bSkipRecord=true;
					return true;
				}
				break;
			case 65:
				//cancel demo
				m_bPlayFromRecord=false;
				m_sRecord.clear();
				m_nRecordIndex=-1;
				return true;
				break;
			default:
				//TODO: MessageBeep(0);
				break;
			}
		}
	}

	return false;
}

void PuzzleBoyLevelView::OnKeyUp(int nChar,int nFlags){
	//key up. currently nothing to do
}

void PuzzleBoyLevelView::OnMultiGesture(float fx,float fy,float dx,float dy,float zoom){
	m_scrollView.OnMultiGesture(fx,fy,dx,dy,zoom);
}

void PuzzleBoyLevelView::OnMouseEvent(int which,int state,int xMouse,int yMouse,int nFlags,int nType){
	//in read-only mode no mouse event are processed (multi-touch event isn't processed here)
	if(m_nMode==READONLY_MODE) return;

	//process mouse move event
	if(nType==SDL_MOUSEMOTION &&
		(state==-42 ||
		((state&(SDL_BUTTON_LMASK|SDL_BUTTON_RMASK))==0 && !theApp->IsTouchscreen())))
	{
		if(!m_bEditMode && m_objPlayingLevel
			&& m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<(int)m_objPlayingLevel->m_objBlocks.size())
		{
			int x,y;
			m_scrollView.TranslateCoordinate(xMouse,yMouse,x,y);

			if((m_objPlayingLevel->m_objBlocks[m_nEditingBlockIndex])->m_nType!=ROTATE_BLOCK){
				x+=m_nEditingBlockDX;
				y+=m_nEditingBlockDY;
			}

			if(x!=m_nEditingBlockX || y!=m_nEditingBlockY){
				m_nEditingBlockX=x;
				m_nEditingBlockY=y;
			}
		}
		return;
	}

	if(m_bEditMode){
		int x,y;

		//process ad-hoc screen keyboard event (edit mode)
		if(nType==SDL_MOUSEBUTTONDOWN && state==SDL_BUTTON_LMASK){
			bool b=false;
			int idx=0,keyIndex=-1;
			int tmp;
			int buttonSize=theApp->m_nButtonSize;

			m_scrollView.TranslateScreenCoordinateToClient(xMouse,yMouse,x,y);

			tmp=m_scrollView.m_screen.h+2*buttonSize;
			if(y<tmp){
				int i=1+((tmp-y-1)/buttonSize);
				idx=i<<8;
			}
			tmp=m_scrollView.m_screen.w;
			if(x<tmp){
				int i=1+((tmp-x-1)/buttonSize);
				idx|=i;
			}

			switch(idx){
			case 0x203: keyIndex=32; b=true; break; //zoom in
			case 0x202: keyIndex=0; b=true; break;
			case 0x201: keyIndex=33; b=true; break; //zoom out
			case 0x104: if(m_nScreenKeyboardType==0) keyIndex=34; break;
			case 0x103: keyIndex=2; b=true; break;
			case 0x102: keyIndex=1; b=true; break;
			case 0x101: keyIndex=3; b=true; break;
			}

			if(m_nScreenKeyboardType==1){
				if(x>=m_nScreenKeyboardPos[0] && y>=m_nScreenKeyboardPos[1] && y<m_nScreenKeyboardPos[1]+buttonSize){
					switch((x-m_nScreenKeyboardPos[0])/buttonSize){
					case 0: keyIndex=64; break;
					case 1: keyIndex=65; break;
					case 2: keyIndex=66; break;
					}
				}
			}

			if(keyIndex>=0){
				InternalKeyDown(keyIndex);
				m_nScreenKeyboardPressedIndex=(b && theApp->m_bContinuousKey)?keyIndex:-1;

				//ad-hoc: disable drag event temporarily
				MultiTouchViewStruct *viewStruct=theApp->touchMgr.FindView(this);
				if(viewStruct){
					viewStruct->m_nDraggingState=3;
					viewStruct->m_fMultitouchOldDistSquared=-1.0f;
				}
				return;
			}
		}

		if(m_toolbar==NULL || m_objPlayingLevel==NULL) return;

		//check edit event
		m_scrollView.TranslateCoordinate(xMouse,yMouse,x,y);
		if(x<0 || y<0 || x>=m_objPlayingLevel->m_nWidth || y>=m_objPlayingLevel->m_nHeight) return;

		int idx=y*m_objPlayingLevel->m_nWidth+x;
		int t=m_toolbar->m_nCurrentTool,t2=0;

		if((nType==SDL_MOUSEBUTTONDOWN || nType==SDL_MOUSEMOTION) && state){
			//check if we are editing some pushable block
			if(m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<(int)m_objPlayingLevel->m_objBlocks.size()){
				PushableBlock *objBlock=m_objPlayingLevel->m_objBlocks[m_nEditingBlockIndex];
				if(objBlock->m_nType==ROTATE_BLOCK){
					int idx=-1,value=-1;

					if(x==objBlock->m_x){
						if(y==objBlock->m_y){
							if(state==SDL_BUTTON_LMASK){
								//check sub-block
								float fx,fy;
								m_scrollView.TranslateCoordinate(float(xMouse),float(yMouse),fx,fy);
								fx-=(float)x;
								fy-=(float)y;
								if(fx+fy<1.0f){
									idx=(fx<fy)?1:0;
								}else{
									idx=(fx<fy)?2:3;
								}
								value=0;
							}else if(state==SDL_BUTTON_RMASK){
								//delete this block
								InternalKeyDown(66);
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
						theApp->ShowToolTip(_("Can't place block here"));
					}

					if(idx>=0){
						if(state==SDL_BUTTON_RMASK) value--;
						if((*objBlock)[idx]!=value){
							(*objBlock)[idx]=value;
							m_objPlayingLevel->CreateGraphics();
							m_pDocument->SetModifiedFlag();
							m_nIdleTime=0;
						}
					}
				}else{
					if(state!=SDL_BUTTON_RMASK && m_objPlayingLevel->HitTestForPlacingBlocks(x,y,m_nEditingBlockIndex)!=-1){
						theApp->ShowToolTip(_("Can't place block here"));
					}else{
						int n=(state==SDL_BUTTON_RMASK)?0:1;
						if((*objBlock)(x,y)!=n){
							(*objBlock)(x,y)=n;
							m_objPlayingLevel->CreateGraphics();
							m_pDocument->SetModifiedFlag();
							m_nIdleTime=0;
						}
					}
				}
				return;
			}else if(state==SDL_BUTTON_LMASK){
				switch(t){
				case 0: //selection
					if(nType==SDL_MOUSEBUTTONDOWN){
						int idx=m_objPlayingLevel->HitTestForPlacingBlocks(x,y,m_nEditingBlockIndex);
						if(idx>=0){
							EnterBlockEdit(idx,true);
						}
					}
					t=-1;
					break;
				case 1: //ground
				case 2: //wall
				case 3: //empty
				case 5: //exit
					t--;
					break;
				case 4: //target
					t=m_objPlayingLevel->m_bMapData[idx];
					if(t!=0 && t!=PLAYER_TILE) t=0;
					t2=1;
					break;
				case 6: //player
					t=PLAYER_TILE;
					t2=m_objPlayingLevel->m_bTargetData[idx];
					break;
				case 7:
				case 8:
				case 9:
					if(nType==SDL_MOUSEBUTTONDOWN){
						int idx=m_objPlayingLevel->HitTestForPlacingBlocks(x,y,m_nEditingBlockIndex);
						if(idx==-1){
							PushableBlock *objBlock=new PushableBlock;
							objBlock->CreateSingle(t-7,x,y);
							m_objPlayingLevel->m_objBlocks.push_back(objBlock);
							EnterBlockEdit(m_objPlayingLevel->m_objBlocks.size()-1,false);
						}else if(idx>=0){
							EnterBlockEdit(idx,true);
						}else{
							theApp->ShowToolTip(_("Can't place block here"));
						}
					}
					t=-1;
					break;
				default:
					t=-1;
					break;
				}

				if(t>=0 && (m_objPlayingLevel->m_bMapData[idx]!=t
					|| m_objPlayingLevel->m_bTargetData[idx]!=t2))
				{
					m_objPlayingLevel->m_bMapData[idx]=t;
					m_objPlayingLevel->m_bTargetData[idx]=t2;
					m_objPlayingLevel->CreateGraphics();
					m_pDocument->SetModifiedFlag();
					m_nIdleTime=0;
				}
			}else if(state==SDL_BUTTON_RMASK){
				if(t>=1 && t<=6 && (m_objPlayingLevel->m_bMapData[idx]!=0
					|| m_objPlayingLevel->m_bTargetData[idx]!=0))
				{
					m_objPlayingLevel->m_bMapData[idx]=0;
					m_objPlayingLevel->m_bTargetData[idx]=0;
					m_objPlayingLevel->CreateGraphics();
					m_pDocument->SetModifiedFlag();
					m_nIdleTime=0;
				}
			}
		}
	}else{
		//process ad-hoc screen keyboard event
		if(nType==SDL_MOUSEBUTTONDOWN && state==SDL_BUTTON_LMASK){
			bool b=false;
			int idx=0,keyIndex=-1;
			int x,y,tmp;
			int buttonSize=theApp->m_nButtonSize;

			m_scrollView.TranslateScreenCoordinateToClient(xMouse,yMouse,x,y);

			tmp=m_scrollView.m_screen.h+2*buttonSize;
			if(y<tmp){
				int i=1+((tmp-y-1)/buttonSize);
				idx=i<<8;
			}
			tmp=m_scrollView.m_screen.w;
			if(x<tmp){
				int i=1+((tmp-x-1)/buttonSize);
				idx|=i;
			}

			switch(idx){
			case 0x204: keyIndex=4; b=true; break;
			case 0x203: keyIndex=0; b=true; break;
			case 0x202: keyIndex=5; b=true; break;
			case 0x201: keyIndex=7; break;
			case 0x104: keyIndex=2; b=true; break;
			case 0x103: keyIndex=1; b=true; break;
			case 0x102: keyIndex=3; b=true; break;
			case 0x101: keyIndex=6; break;
			}

			switch(m_nScreenKeyboardType){
			case 1:
			case 2:
				if(x>=m_nScreenKeyboardPos[0] && y>=m_nScreenKeyboardPos[1] &&
					x<m_nScreenKeyboardPos[0]+buttonSize && y<m_nScreenKeyboardPos[1]+buttonSize)
				{
					keyIndex=64;
				}else if(x>=m_nScreenKeyboardPos[2] && y>=m_nScreenKeyboardPos[3] &&
					x<m_nScreenKeyboardPos[2]+buttonSize && y<m_nScreenKeyboardPos[3]+buttonSize)
				{
					keyIndex=65;
				}
				break;
			}

			if(keyIndex>=0){
				InternalKeyDown(keyIndex);
				m_nScreenKeyboardPressedIndex=(b && theApp->m_bContinuousKey)?keyIndex:-1;

				//ad-hoc: disable drag event temporarily
				MultiTouchViewStruct *viewStruct=theApp->touchMgr.FindView(this);
				if(viewStruct){
					viewStruct->m_nDraggingState=3;
					viewStruct->m_fMultitouchOldDistSquared=-1.0f;
				}
				return;
			}
		}

		//process in-game mouse events
		//mouse up for right key
		if(nType==SDL_MOUSEBUTTONUP && (state & SDL_BUTTON_RMASK)!=0
			&& m_objPlayingLevel)
		{
			if(m_nEditingBlockIndex>=0){
				//cancel moving blocks
				m_nEditingBlockIndex=-1;
			}else if(!m_sRecord.empty()){
				//skip demo
				m_bPlayFromRecord=false;
				m_sRecord.clear();
				m_nRecordIndex=-1;
			}
		}

		//mouse up for left key
		if(nType==SDL_MOUSEBUTTONUP && (state & SDL_BUTTON_LMASK)!=0
			&& m_objPlayingLevel && !m_objPlayingLevel->IsAnimating()
			&& !m_objPlayingLevel->IsWin()
			&& m_sRecord.empty()
			)
		{
			int x,y;
			m_scrollView.TranslateCoordinate(xMouse,yMouse,x,y);

			if(x>=0 && y>=0 && x<m_objPlayingLevel->m_nWidth && y<m_objPlayingLevel->m_nHeight){
				if(m_objPlayingLevel->SwitchPlayer(x,y,true)){
					//it's switch player
					//do nothing in SDL version
				}else if(m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<(int)m_objPlayingLevel->m_objBlocks.size()){
					//workaround on Android and other devices without mouse move event
					OnMouseEvent(0,-42,xMouse,yMouse,0,SDL_MOUSEMOTION);

					//move a moveable block to new position
					if(!theApp->IsTouchscreen()
						|| m_objPlayingLevel->m_objBlocks[m_nEditingBlockIndex]->m_nType==ROTATE_BLOCK)
					{
						InternalKeyDown(64);
					}
				}else{
					int x0=m_objPlayingLevel->GetCurrentPlayerX();
					int y0=m_objPlayingLevel->GetCurrentPlayerY();

					//check if a block clicked
					int idx=m_objPlayingLevel->HitTestForPlacingBlocks(x,y,-1);
					int nType=-1;
					PushableBlock *objBlock=NULL;
					if(idx>=0){
						objBlock=m_objPlayingLevel->m_objBlocks[idx];
						nType=objBlock->m_nType;
					}
					if(nType==NORMAL_BLOCK || nType==TARGET_BLOCK){
						//clicked a moveable block
						m_nEditingBlockIndex=idx;
						m_nEditingBlockX=objBlock->m_x;
						m_nEditingBlockY=objBlock->m_y;
						m_nEditingBlockDX=m_nEditingBlockX-x;
						m_nEditingBlockDY=m_nEditingBlockY-y;
						//show a notification
						if(theApp->IsTouchscreen()){
							theApp->ShowToolTip(_("Start moving box"));
						}
					}else if(nType==ROTATE_BLOCK && (x!=objBlock->m_x || y!=objBlock->m_y)){
						//clicked a rotate block
						m_nEditingBlockIndex=idx;
						m_nEditingBlockX=x;
						m_nEditingBlockY=y;
						m_nEditingBlockDX=x;
						m_nEditingBlockDY=y;
					}else if(x==x0 && y==y0-1){ //check if the clicked position is adjacent to the player
						InternalKeyDown(0);
					}else if(x==x0 && y==y0+1){
						InternalKeyDown(1);
					}else if(x==x0-1 && y==y0){
						InternalKeyDown(2);
					}else if(x==x0+1 && y==y0){
						InternalKeyDown(3);
					}else{
						u8string s;

						if(m_objPlayingLevel->FindPath(s,x,y) && !s.empty()){
							//it's pathfinding
							m_sRecord=s;
							m_nRecordIndex=0;
						}else{
							theApp->ShowToolTip(_("Can't move to specified location"));
						}
					}
				}
			}
		}
		return;
	}
}

void PuzzleBoyLevelView::EnterBlockEdit(int nIndex,bool bBackup){
	if(m_nEditingBlockIndex>=0) FinishBlockEdit();
	if(m_bEditMode && m_pDocument && m_objPlayingLevel
		&& nIndex>=0 && nIndex<(int)m_objPlayingLevel->m_objBlocks.size())
	{
		PushableBlock *objBlock=m_objPlayingLevel->m_objBlocks[nIndex];
		if(m_objBackupBlock){
			delete m_objBackupBlock;
			m_objBackupBlock=NULL;
		}
		if(bBackup) m_objBackupBlock=new PushableBlock(*objBlock);
		objBlock->UnoptimizeSize(m_objPlayingLevel->m_nWidth,m_objPlayingLevel->m_nHeight);
		m_nEditingBlockIndex=nIndex;
		m_objPlayingLevel->CreateGraphics();
		m_pDocument->SetModifiedFlag();
	}
}

void PuzzleBoyLevelView::FinishBlockEdit(){
	if(m_bEditMode && m_pDocument && m_objPlayingLevel
		&& m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<(int)m_objPlayingLevel->m_objBlocks.size())
	{
		PushableBlock *objBlock=m_objPlayingLevel->m_objBlocks[m_nEditingBlockIndex];
		if(!objBlock->OptimizeSize()){
			m_objPlayingLevel->m_objBlocks.erase(m_objPlayingLevel->m_objBlocks.begin()+m_nEditingBlockIndex);
			delete objBlock;
		}
		m_objPlayingLevel->CreateGraphics();
		m_pDocument->SetModifiedFlag();
	}
	if(m_objBackupBlock){
		delete m_objBackupBlock;
		m_objBackupBlock=NULL;
	}
	m_nEditingBlockIndex=-1;
}

void PuzzleBoyLevelView::CancelBlockEdit(){
	if(m_bEditMode && m_pDocument && m_objPlayingLevel
		&& m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<(int)m_objPlayingLevel->m_objBlocks.size())
	{
		if(m_objBackupBlock){
			//restore to old block
			PushableBlock *objBlock=m_objPlayingLevel->m_objBlocks[m_nEditingBlockIndex];
			delete objBlock;
			m_objPlayingLevel->m_objBlocks[m_nEditingBlockIndex]=m_objBackupBlock;
			m_objBackupBlock=NULL;
		}else{
			//delete current block
			PushableBlock *objBlock=m_objPlayingLevel->m_objBlocks[m_nEditingBlockIndex];
			m_objPlayingLevel->m_objBlocks.erase(m_objPlayingLevel->m_objBlocks.begin()+m_nEditingBlockIndex);
			delete objBlock;
		}
		m_objPlayingLevel->CreateGraphics();
		m_pDocument->SetModifiedFlag();
	}
	if(m_objBackupBlock){
		delete m_objBackupBlock;
		m_objBackupBlock=NULL;
	}
	m_nEditingBlockIndex=-1;
}
