#include "PuzzleBoyApp.h"
#include "PuzzleBoyLevelFile.h"
#include "PuzzleBoyLevel.h"
#include "PuzzleBoyLevelView.h"
#include "PushableBlock.h"
#include "SimpleScrollView.h"
#include "FileSystem.h"
#include "VertexList.h"
#include "main.h"

#include <string.h>

#include "include_sdl.h"
#include "include_gl.h"

//ad-hoc!!
static const int m_nDefaultMultiplayerKey[16]={
	SDLK_UP,
	SDLK_DOWN,
	SDLK_LEFT,
	SDLK_RIGHT,
	SDLK_k,
	SDLK_l,
	SDLK_j,
	0,
	SDLK_w,
	SDLK_s,
	SDLK_a,
	SDLK_d,
	SDLK_g,
	SDLK_h,
	SDLK_f,
	0,
};

extern SDL_Event event;

PuzzleBoyApp::PuzzleBoyApp()
:m_nAnimationSpeed(0)
,m_bShowGrid(false)
,m_bShowLines(false)
,m_pDocument(NULL)
,m_nCurrentLevel(0)
{
}

PuzzleBoyApp::~PuzzleBoyApp(){
	delete m_pDocument;
	for(unsigned int i=0;i<m_view.size();i++){
		delete m_view[i];
	}
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

	DestroyGame();

	m_nCurrentLevel=0;

	return true;
}

bool PuzzleBoyApp::SaveFile(const u8string& fileName){
	if(m_pDocument==NULL) return false;

	u8file *f=u8fopen(fileName.c_str(),"wb");
	if(f==NULL) return false;

	MFCSerializer ar;
	ar.SetFile(f,true,4096);

	bool ret=m_pDocument->MFCSerialize(ar);
	ar.FileFlush();

	u8fclose(f);

	return ret;
}

void PuzzleBoyApp::Draw(){
	if(m_view.empty()) return;

	int m=m_view.size();
	if(m==1){
		m_view[0]->m_scrollView.SetProjectionMatrix();
		m_view[0]->Draw();
	}else{
		for(int i=0;i<m;i++){
			m_view[i]->m_scrollView.EnableScissorRect();
			m_view[i]->m_scrollView.SetProjectionMatrix();
			m_view[i]->Draw();
		}
		SimpleScrollView::DisableScissorRect();
	}
}

void PuzzleBoyApp::DestroyGame(){
	for(unsigned int i=0;i<m_view.size();i++){
		delete m_view[i];
	}
	m_view.clear();
}

bool PuzzleBoyApp::StartGame(int nPlayerCount){
	DestroyGame();

	if(nPlayerCount==2){
		//multiplayer
		PuzzleBoyLevelView *view=new PuzzleBoyLevelView();

		view->m_nKey=m_nDefaultMultiplayerKey;
		view->m_scrollView.m_bAutoResize=true;
		view->m_scrollView.m_fAutoResizeScale[0]=0.0f;
		view->m_scrollView.m_fAutoResizeScale[1]=0.0f;
		view->m_scrollView.m_fAutoResizeScale[2]=0.5f;
		view->m_scrollView.m_fAutoResizeScale[3]=1.0f;
		view->m_scrollView.m_nAutoResizeOffset[0]=0;
		view->m_scrollView.m_nAutoResizeOffset[1]=0;
		view->m_scrollView.m_nAutoResizeOffset[2]=-4;
		view->m_scrollView.m_nAutoResizeOffset[3]=0;
		view->m_scrollView.OnTimer();

		view->m_pDocument=m_pDocument;
		view->m_nCurrentLevel=m_nCurrentLevel;
		if(!view->StartGame()){
			delete view;
			return false;
		}
		m_view.push_back(view);

		//player 2
		view=new PuzzleBoyLevelView();

		view->m_nKey=m_nDefaultMultiplayerKey+8;
		view->m_scrollView.m_bAutoResize=true;
		view->m_scrollView.m_fAutoResizeScale[0]=0.5f;
		view->m_scrollView.m_fAutoResizeScale[1]=0.0f;
		view->m_scrollView.m_fAutoResizeScale[2]=1.0f;
		view->m_scrollView.m_fAutoResizeScale[3]=1.0f;
		view->m_scrollView.m_nAutoResizeOffset[0]=4;
		view->m_scrollView.m_nAutoResizeOffset[1]=0;
		view->m_scrollView.m_nAutoResizeOffset[2]=0;
		view->m_scrollView.m_nAutoResizeOffset[3]=0;
		view->m_scrollView.OnTimer();

		view->m_pDocument=m_pDocument;
		view->m_nCurrentLevel=m_nCurrentLevel;
		view->StartGame();

		m_view.push_back(view);
	}else{
		//single player
		PuzzleBoyLevelView *view=new PuzzleBoyLevelView();

		view->m_scrollView.m_bAutoResize=true;
		view->m_scrollView.m_fAutoResizeScale[0]=0.0f;
		view->m_scrollView.m_fAutoResizeScale[1]=0.0f;
		view->m_scrollView.m_fAutoResizeScale[2]=1.0f;
		view->m_scrollView.m_fAutoResizeScale[3]=1.0f;
		view->m_scrollView.m_nAutoResizeOffset[0]=0;
		view->m_scrollView.m_nAutoResizeOffset[1]=0;
		view->m_scrollView.m_nAutoResizeOffset[2]=0;
		view->m_scrollView.m_nAutoResizeOffset[3]=-128;
		view->m_scrollView.OnTimer();

		view->m_pDocument=m_pDocument;
		view->m_nCurrentLevel=m_nCurrentLevel;
		if(!view->StartGame()){
			delete view;
			return false;
		}
		m_view.push_back(view);
	}

	return true;
}

void PuzzleBoyApp::OnTimer(){
	for(unsigned int i=0;i<m_view.size();i++){
		m_view[i]->OnTimer();
	}
}

bool PuzzleBoyApp::OnKeyDown(int nChar,int nFlags){
	bool b=false;
	for(unsigned int i=0;i<m_view.size();i++){
		if(m_view[i]->OnKeyDown(nChar,nFlags)) b=true; //???
	}
	return b;
}

void PuzzleBoyApp::OnKeyUp(int nChar,int nFlags){
	for(unsigned int i=0;i<m_view.size();i++){
		m_view[i]->OnKeyDown(nChar,nFlags);
	}
}

void PuzzleBoyApp::OnMouseEvent(int nFlags, int xMouse, int yMouse, int nType){
	//TODO: OnMouseEvent
}
