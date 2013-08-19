#include "PuzzleBoyApp.h"
#include "PuzzleBoyLevelFile.h"
#include "PuzzleBoyLevel.h"
#include "PushableBlock.h"
#include "FileSystem.h"

#include <string.h>

#include "include_sdl.h"

extern SDL_Event event;

PuzzleBoyApp::PuzzleBoyApp()
:m_nAnimationSpeed(0)
,m_bShowGrid(false)
,m_bShowLines(false)
,m_pDocument(NULL)
,m_nCurrentLevel(0)
,m_bEditMode(false)
,m_bPlayFromRecord(false)
,m_nCurrentTool(-1)
,m_nEditingBlockIndex(-1)
,m_objBackupBlock(NULL)
,m_objPlayingLevel(NULL)
,m_nRecordIndex(-1)
{
}

PuzzleBoyApp::~PuzzleBoyApp(){
	delete m_pDocument;
	delete m_objPlayingLevel;
	delete m_objBackupBlock;
}

//TODO: load new file format
bool PuzzleBoyApp::LoadFile(const u8string& fileName){
	u8file *f=u8fopen(fileName.c_str(),"rb");
	if(f==NULL) return false;

	MFCSerializer ar;
	ar.SetFile(f,false,4096);

	PuzzleBoyLevelFile* pDoc=new PuzzleBoyLevelFile;
	if(pDoc->MFCSerialize(ar)){
		delete m_pDocument;
		m_pDocument=pDoc;
	}else{
		delete pDoc;
		pDoc=NULL;
	}

	u8fclose(f);

	if(pDoc==NULL) return false;

	delete m_objPlayingLevel;
	m_objPlayingLevel=NULL;

	delete m_objBackupBlock;
	m_objBackupBlock=NULL;

	m_nCurrentLevel=0;
	m_bEditMode=false;
	m_bPlayFromRecord=false;
	m_nCurrentTool=-1;
	m_nEditingBlockIndex=-1;
	m_nRecordIndex=-1;

	return true;
}

void PuzzleBoyApp::Draw(){
	if(m_objPlayingLevel) m_objPlayingLevel->Draw();
}

bool PuzzleBoyApp::StartGame(){
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
	PuzzleBoyLevelFile* pDoc = GetDocument();
	if(pDoc==NULL) return false;

	if(m_nCurrentLevel<0) m_nCurrentLevel=0;
	if(m_nCurrentLevel>=(int)pDoc->m_objLevels.size()) m_nCurrentLevel=pDoc->m_objLevels.size()-1;
	if(m_nCurrentLevel<0) return false;

	m_objPlayingLevel=new PuzzleBoyLevel(*(pDoc->GetLevel(m_nCurrentLevel)));
	m_objPlayingLevel->StartGame();

	//get the best record of current level
	int st=m_objRecordMgr.FindAllRecordsOfLevel(*(pDoc->GetLevel(m_nCurrentLevel)),m_tCurrentBestRecord,m_bCurrentChecksum);
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
					m_sCurrentBestStepOwner+=m_tCurrentBestRecord[i].sPlayerName;
				}
			}
		}
	}

	//create graphics
	m_objPlayingLevel->CreateGraphics();

	return true;
}

void PuzzleBoyApp::FinishGame(){
	memset(m_bCurrentChecksum,0,PuzzleBoyLevelData::ChecksumSize);

	PuzzleBoyLevelFile* pDoc = GetDocument();
	int m=0;
	if(pDoc) m=pDoc->m_objLevels.size();

	if (m_bEditMode || !pDoc || m_nCurrentLevel<0 || m_nCurrentLevel>=m) {
		//TODO: complain
		//AfxMessageBox(_T("恭喜你完成本关卡!"),MB_ICONINFORMATION);
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
	}*/

	//next level
	if(m_nCurrentLevel>=0 && m_nCurrentLevel<m-1){
		m_nCurrentLevel++;
	}else{
		m_nCurrentLevel=0;
	}

	StartGame();
}

void PuzzleBoyApp::OnTimer(){
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
					const int nKeys[]={
						SDL_SCANCODE_U,
						SDL_SCANCODE_R,
						SDL_SCANCODE_UP,
						SDL_SCANCODE_DOWN,
						SDL_SCANCODE_LEFT,
						SDL_SCANCODE_RIGHT,
						SDL_SCANCODE_SPACE,
					};
					const int nKey2[]={
						SDLK_u, //undo
						SDLK_r, //redo
						SDLK_UP,
						SDLK_DOWN,
						SDLK_LEFT,
						SDLK_RIGHT,
						SDLK_SPACE,
					};
					for(int i=0;i<sizeof(nKeys)/sizeof(nKeys[0]);i++){
						if(b[nKeys[i]]){
							OnKeyDown(nKey2[i],0);
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

void PuzzleBoyApp::Undo()
{
	if(m_bEditMode){
		//TODO: edit undo
	}else if(m_objPlayingLevel && !m_objPlayingLevel->IsAnimating() && m_sRecord.empty()){
		if(m_objPlayingLevel->Undo()){
			m_nEditingBlockIndex=-1;
		}else{
			//TODO: complain
			//AfxMessageBox(_T("出现错误"));
		}
	}
}

void PuzzleBoyApp::Redo()
{
	if(m_bEditMode){
		//TODO: edit redo
	}else if(m_objPlayingLevel && !m_objPlayingLevel->IsAnimating() && m_sRecord.empty()){
		if(m_objPlayingLevel->Redo()){
			m_nEditingBlockIndex=-1;
		}else{
			//TODO: complain
			//AfxMessageBox(_T("出现错误"));
		}
	}
}

void PuzzleBoyApp::OnKeyDown(int nChar,int nFlags){
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
			}
			return;
		}

		if(m_sRecord.empty()){
			if(!m_objPlayingLevel->IsAnimating() && !m_objPlayingLevel->IsWin()){
				bool b=false;
				switch(nChar){
				case SDLK_UP:
					m_nEditingBlockIndex=-1;
					b=m_objPlayingLevel->MovePlayer(0,-1,true);
					break;
				case SDLK_DOWN:
					m_nEditingBlockIndex=-1;
					b=m_objPlayingLevel->MovePlayer(0,1,true);
					break;
				case SDLK_LEFT:
					m_nEditingBlockIndex=-1;
					b=m_objPlayingLevel->MovePlayer(-1,0,true);
					break;
				case SDLK_RIGHT:
					m_nEditingBlockIndex=-1;
					b=m_objPlayingLevel->MovePlayer(1,0,true);
					break;
				case SDLK_SPACE:
					b=m_objPlayingLevel->SwitchPlayer(-1,-1,true);
					break;
				case SDLK_u:
					Undo();
					return;
				case SDLK_r:
					Redo();
					return;
				default:
					return;
				}
				//TODO: complain
				/*if(!b){
					MessageBeep(0);
				}*/
			}
		}else{
			switch(nChar){
			case SDLK_ESCAPE:
				//skip demo
				m_bPlayFromRecord=false;
				m_sRecord.clear();
				m_nRecordIndex=-1;
				break;
			default:
				//TODO: complain
				//MessageBeep(0);
				break;
			}
		}
	}
}
