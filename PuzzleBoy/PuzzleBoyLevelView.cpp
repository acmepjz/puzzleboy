#include "PuzzleBoyLevelView.h"
#include "PuzzleBoyLevelFile.h"
#include "PuzzleBoyLevel.h"
#include "PuzzleBoyApp.h"
#include "PushableBlock.h"
#include "RecordManager.h"
#include "VertexList.h"
#include "main.h"

#include <string.h>

#include "include_sdl.h"
#include "include_gl.h"

//ad-hoc!!
extern GLuint adhoc_screenkb_tex;

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

PuzzleBoyLevelView::PuzzleBoyLevelView()
:m_bShowYesNoScreenKeyboard(false)
,m_pDocument(NULL)
,m_nCurrentLevel(0)
,m_bEditMode(false)
,m_bPlayFromRecord(false)
,m_nCurrentTool(-1)
,m_nEditingBlockIndex(-1)
,m_objBackupBlock(NULL)
,m_objPlayingLevel(NULL)
,m_nRecordIndex(-1)
,m_nKey(m_nDefaultKey)
{
}

PuzzleBoyLevelView::~PuzzleBoyLevelView(){
	delete m_objPlayingLevel;
	delete m_objBackupBlock;
}

void PuzzleBoyLevelView::Draw(){
	m_bShowYesNoScreenKeyboard=false;

	if(m_objPlayingLevel){
		//draw level
		m_objPlayingLevel->Draw();

		//draw selected block in game mode
		if(!m_bEditMode && m_nEditingBlockIndex>=0 && m_nEditingBlockIndex<(int)m_objPlayingLevel->m_objBlocks.size()){
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
				//enable screen keyboard
				if(m_bTouchscreen) m_bShowYesNoScreenKeyboard=true;

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

		//draw scroll bar
		m_scrollView.DrawScrollBar();

		//FIXME: ad-hoc: draw screen keypad
		SetProjectionMatrix(1);
		{
			int x1=m_scrollView.m_screen.x+m_scrollView.m_screen.w;
			int y1=m_scrollView.m_screen.y+m_scrollView.m_screen.h;

			float v[]={
				float(x1-256),float(y1),0.0f,0.0f,
				float(x1),float(y1),1.0f,0.0f,
				float(x1),float(y1+128),1.0f,0.5f,
				float(x1-256),float(y1+128),0.0f,0.5f,

				float(x1-448),float(y1+64),0.0f,0.5f,
				float(x1-320),float(y1+64),0.5f,0.5f,
				float(x1-320),float(y1+128),0.5f,0.75f,
				float(x1-448),float(y1+128),0.0f,0.75f,
			};

			switch(m_scrollView.m_nOrientation){
			case 2:
				for(int i=0;i<8;i++){
					v[i*4]=float(m_scrollView.m_screen.x*2+m_scrollView.m_screen.w)-v[i*4];
					v[i*4+1]=float(m_scrollView.m_screen.y*2+m_scrollView.m_screen.h)-v[i*4+1];
				}
				break;
			}

			const unsigned short i[]={
				0,1,2,0,2,3,
				4,5,6,4,6,7,
			};

			glDisable(GL_LIGHTING);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, adhoc_screenkb_tex);

			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glVertexPointer(2,GL_FLOAT,4*sizeof(float),v);
			glTexCoordPointer(2,GL_FLOAT,4*sizeof(float),v+2);

			glDrawElements(GL_TRIANGLES,m_bShowYesNoScreenKeyboard?12:6,GL_UNSIGNED_SHORT,i);

			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);
		}
	}
}

bool PuzzleBoyLevelView::StartGame(){
	//delete old level
	if(m_objPlayingLevel){
		delete m_objPlayingLevel;
		m_objPlayingLevel=NULL;
	}

	m_bPlayFromRecord=false;
	m_sRecord.clear();
	m_nRecordIndex=-1;

	if(m_bEditMode) return true;

	m_nEditingBlockIndex=-1;

	//create a copy of current level
	PuzzleBoyLevelFile* pDoc = m_pDocument;
	if(pDoc==NULL) return false;

	if(m_nCurrentLevel<0) m_nCurrentLevel=0;
	if(m_nCurrentLevel>=(int)pDoc->m_objLevels.size()) m_nCurrentLevel=pDoc->m_objLevels.size()-1;
	if(m_nCurrentLevel<0) return false;

	m_objPlayingLevel=new PuzzleBoyLevel(*(pDoc->GetLevel(m_nCurrentLevel)));
	m_objPlayingLevel->m_view=this;
	m_objPlayingLevel->StartGame();

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

	//create graphics
	m_objPlayingLevel->CreateGraphics();

	//FIXME: ad-hoc one
	SDL_Rect r={0,0,m_objPlayingLevel->m_nWidth,m_objPlayingLevel->m_nHeight};
	m_scrollView.m_virtual=r;
	m_scrollView.CenterView();

	return true;
}

void PuzzleBoyLevelView::FinishGame(){
	memset(m_bCurrentChecksum,0,PuzzleBoyLevelData::ChecksumSize);

	PuzzleBoyLevelFile* pDoc = m_pDocument;
	int m=0;
	if(pDoc) m=pDoc->m_objLevels.size();

	if (m_bEditMode || !pDoc || m_nCurrentLevel<0 || m_nCurrentLevel>=m) {
		//TODO: AfxMessageBox(_T("恭喜你完成本关卡!"),MB_ICONINFORMATION);
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
	if(m_bPlayFromRecord){
#ifdef _DEBUG
		printf("[PuzzleBoyApp] Play from record, don't save it\n");
#endif
	}else{
#ifdef _DEBUG
		printf("[PuzzleBoyApp] Not play from record, save it\n");
#endif
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

	StartGame();
}

void PuzzleBoyLevelView::Undo(){
	if(m_bEditMode){
		//TODO: edit undo
	}else if(m_objPlayingLevel && !m_objPlayingLevel->IsAnimating() && m_sRecord.empty()){
		if(!m_objPlayingLevel->CanUndo()){
			//TODO: MessageBeep(0);
		}else if(m_objPlayingLevel->Undo()){
			m_nEditingBlockIndex=-1;
		}else{
			//TODO: AfxMessageBox(_T("出现错误"));
		}
	}
}

void PuzzleBoyLevelView::Redo(){
	if(m_bEditMode){
		//TODO: edit redo
	}else if(m_objPlayingLevel && !m_objPlayingLevel->IsAnimating() && m_sRecord.empty()){
		if(!m_objPlayingLevel->CanRedo()){
			//TODO: MessageBeep(0);
		}else if(m_objPlayingLevel->Redo()){
			m_nEditingBlockIndex=-1;
		}else{
			//TODO: AfxMessageBox(_T("出现错误"));
		}
	}
}

void PuzzleBoyLevelView::OnTimer(){
	m_scrollView.OnTimer();

	if(!m_bEditMode && m_objPlayingLevel){
		if(m_objPlayingLevel->IsAnimating()){
			m_objPlayingLevel->OnTimer();
		}else{
			if(m_objPlayingLevel->IsWin()){
				FinishGame();
				return;
			}else if(m_sRecord.empty()){
				m_bPlayFromRecord=false;
				//get continuous key event
				if(theApp->m_nAnimationSpeed<=2){
					const Uint8 *b=SDL_GetKeyboardState(NULL);

					//only arrow keys and undo/redo, still buggy
					for(int i=0;i<6;i++){
						if(m_nKey[i] && b[SDL_GetScancodeFromKey(m_nKey[i])]){
							InternalKeyDown(i);
							break;
						}
					}
				}
			}else{
				//play from record
				if(m_nRecordIndex<0) m_nRecordIndex=0;
				int m=m_sRecord.size();
				//TODO: skip record
				bool bSkip=false; /*(GetAsyncKeyState(VK_SHIFT) & 0x8000)!=0;

				if(bSkip) SetHourglassCursor();*/

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

				m_nEditingBlockIndex=-1;
			}
		}
	}
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
	}else if(m_objPlayingLevel){
		if(nFlags){
			if(nChar==SDLK_r && (nFlags & KMOD_CTRL)!=0){
				StartGame();
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
	if(!m_bEditMode && m_objPlayingLevel){
		if(keyIndex==7){
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
								s.push_back(dx<0?'A':dy<0?'W':dx>0?'D':'S');
							}
						}else{
							m_objPlayingLevel->FindPathForBlock(s,idx,m_nEditingBlockX,m_nEditingBlockY);
						}

						if(!s.empty()){
							m_sRecord=s;
							m_nRecordIndex=0;
						}else{
							//TODO: MessageBeep(0);
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
			case 65:
				//skip demo
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
	//TODO: key up
}

void PuzzleBoyLevelView::OnMultiGesture(float fx,float fy,float dx,float dy,float zoom){
	m_scrollView.OnMultiGesture(fx,fy,dx,dy,zoom);
}

void PuzzleBoyLevelView::OnMouseEvent(int which,int state,int xMouse,int yMouse,int nFlags,int nType){
	//process mouse move event
	if(nType==SDL_MOUSEMOTION && (state&(SDL_BUTTON_LMASK|SDL_BUTTON_RMASK))==0){
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

	if(!m_bEditMode){
		//process ad-hoc screen keyboard event
		if(nType==SDL_MOUSEBUTTONDOWN && state==SDL_BUTTON_LMASK){
			bool b=false;
			int idx=0;
			int tmp;

			switch(m_scrollView.m_nOrientation){
			case 2:
				tmp=m_scrollView.m_screen.y-128;
				if(yMouse>=tmp){
					int i=1+((yMouse-tmp)>>6);
					idx=i<<8;
				}
				tmp=m_scrollView.m_screen.x;
				if(xMouse>=tmp){
					int i=1+((xMouse-tmp)>>6);
					idx|=i;
				}
				break;
			default:
				tmp=m_scrollView.m_screen.y+m_scrollView.m_screen.h+128;
				if(yMouse<tmp){
					int i=1+((tmp-yMouse-1)>>6);
					idx=i<<8;
				}
				tmp=m_scrollView.m_screen.x+m_scrollView.m_screen.w;
				if(xMouse<tmp){
					int i=1+((tmp-xMouse-1)>>6);
					idx|=i;
				}
				break;
			}

			switch(idx){
			case 0x204: InternalKeyDown(4); b=true; break;
			case 0x203: InternalKeyDown(0); b=true; break;
			case 0x202: InternalKeyDown(5); b=true; break;
			case 0x201: InternalKeyDown(7); b=true; break;
			case 0x104: InternalKeyDown(2); b=true; break;
			case 0x103: InternalKeyDown(1); b=true; break;
			case 0x102: InternalKeyDown(3); b=true; break;
			case 0x101: InternalKeyDown(6); b=true; break;
			}

			if(m_bShowYesNoScreenKeyboard){
				switch(idx){
				case 0x107: InternalKeyDown(64); b=true; break;
				case 0x106: InternalKeyDown(65); b=true; break;
				}
			}

			if(b){
				theApp->touchMgr.DisableTemporarily(this);
				return;
			}
		}

		//process in-game mouse events
		//FIXME: mouse up??
		if(nType==SDL_MOUSEBUTTONUP && (state&(SDL_BUTTON_LMASK|SDL_BUTTON_RMASK))!=0
			&& m_objPlayingLevel
			)
		{
			if(state&SDL_BUTTON_RMASK){
				if(m_nEditingBlockIndex>=0){
					//cancel moving blocks
					m_nEditingBlockIndex=-1;
				}else if(!m_sRecord.empty()){
					//skip demo
					m_bPlayFromRecord=false;
					m_sRecord.clear();
					m_nRecordIndex=-1;
				}else if(!m_objPlayingLevel->IsAnimating()){
					//undo
					if(m_objPlayingLevel->CanUndo()) Undo();
					else{
						//TODO: MessageBeep(0);
					}
				}
			}else if(
				!m_objPlayingLevel->IsAnimating()
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
						OnMouseEvent(0,0,xMouse,yMouse,0,SDL_MOUSEMOTION);

						//move a moveable block to new position
						if(!m_bTouchscreen
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
								//TODO: MessageBeep(0);
							}
						}
					}
				}
			}
		}
		return;
	}

	//TODO: edit mode
	/*CmfccourseDoc* pDoc = GetDocument();
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
		PushableBlock *objBlock=lev->m_objBlocks[m_nEditingBlockIndex];
		if(objBlock->m_nType==ROTATE_BLOCK){
			int idx=-1,value=-1;

			if(x==objBlock->m_x){
				if(y==objBlock->m_y){
					if(state&SDL_BUTTON_LMASK){
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
				if(state&SDL_BUTTON_RMASK) value--;
				if((*objBlock)[idx]!=value){
					(*objBlock)[idx]=value;
					Invalidate();
					pDoc->SetModifiedFlag();
				}
			}
		}else{
			if((state&SDL_BUTTON_LMASK)!=0 && lev->HitTestForPlacingBlocks(x,y,m_nEditingBlockIndex)!=-1){
				MessageBeep(0);
			}else{
				int n=(state&SDL_BUTTON_LMASK)?1:0;
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
			if(state&SDL_BUTTON_LMASK){
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

			if(state&SDL_BUTTON_RMASK) idx=0;
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
			if(state&SDL_BUTTON_LMASK){
				int idx=lev->HitTestForPlacingBlocks(x,y,m_nEditingBlockIndex);
				if(idx==-1){
					PushableBlock *objBlock=new PushableBlock;
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
	}*/
}
