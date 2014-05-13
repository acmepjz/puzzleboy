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
#include <stdio.h>

#include <map>

#include "include_sdl.h"
#include "include_gl.h"

PuzzleBoyApp *theApp=NULL;

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
,m_bShowLines(true)
,m_bInternationalFont(true)
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

static u8string GetConfig(const std::map<u8string,u8string>& cfg,const u8string& name,const u8string& default){
	std::map<u8string,u8string>::const_iterator it=cfg.find(name);
	if(it==cfg.end()) return default;
	else return it->second;
}

static int GetConfig(const std::map<u8string,u8string>& cfg,const u8string& name,int default){
	std::map<u8string,u8string>::const_iterator it=cfg.find(name);
	if(it==cfg.end()) return default;
	else{
		int i;
		if(sscanf(it->second.c_str(),"%d",&i)!=1) return default;
		else return i;
	}
}

static void PutConfig(std::map<u8string,u8string>& cfg,const u8string& name,int value){
	char s[16];
	sprintf(s,"%d",value);
	cfg[name]=s;
}

void PuzzleBoyApp::LoadConfig(const u8string& fileName){
	std::map<u8string,u8string> cfg;

	u8file *f=u8fopen(fileName.c_str(),"rb");
	if(f){
		u8string s;
		while(u8fgets2(s,f)){
			u8string::size_type lps=s.find_first_of('=');
			if(lps!=u8string::npos){
				u8string s2=s.substr(lps+1);
				s=s.substr(0,lps);

				lps=s.find_first_not_of(" \t\r\n");
				if(lps==u8string::npos) s="";
				else{
					s=s.substr(lps);
					lps=s.find_last_not_of(" \t\r\n");
					s=s.substr(0,lps+1);
				}

				lps=s2.find_first_not_of(" \t\r\n");
				if(lps==u8string::npos) s2="";
				else{
					s2=s2.substr(lps);
					lps=s2.find_last_not_of(" \t\r\n");
					s2=s2.substr(0,lps+1);
				}

				cfg[s]=s2;
			}
		}
		u8fclose(f);
	}

	m_nAnimationSpeed=GetConfig(cfg,"AnimationSpeed",0);
	if(m_nAnimationSpeed<0) m_nAnimationSpeed=0;
	else if(m_nAnimationSpeed>3) m_nAnimationSpeed=3;

	m_bShowGrid=GetConfig(cfg,"ShowGrid",0)!=0;
	m_bShowLines=GetConfig(cfg,"ShowLines",1)!=0;
	m_bInternationalFont=GetConfig(cfg,"InternationalFont",1)!=0;

	m_sPlayerName[0]=toUTF16(GetConfig(cfg,"PlayerName",_("Player")));
	m_sPlayerName[1]=toUTF16(GetConfig(cfg,"PlayerName2",_("Player2")));

	m_sLocale=GetConfig(cfg,"Locale","");
	if(m_sLocale.find_first_of('?')!=u8string::npos) m_sLocale="?";

	m_nThreadCount=GetConfig(cfg,"ThreadCount",0);
	if(m_nThreadCount<0) m_nThreadCount=0;
	else if(m_nThreadCount>8) m_nThreadCount=8;
}

void PuzzleBoyApp::SaveConfig(const u8string& fileName){
	std::map<u8string,u8string> cfg;

	PutConfig(cfg,"AnimationSpeed",m_nAnimationSpeed);
	PutConfig(cfg,"ShowGrid",m_bShowGrid?1:0);
	PutConfig(cfg,"ShowLines",m_bShowLines?1:0);
	PutConfig(cfg,"InternationalFont",m_bInternationalFont?1:0);

	cfg["PlayerName"]=toUTF8(m_sPlayerName[0]);
	cfg["PlayerName2"]=toUTF8(m_sPlayerName[1]);

	cfg["Locale"]=m_sLocale;

	PutConfig(cfg,"ThreadCount",m_nThreadCount);

	u8file *f=u8fopen(fileName.c_str(),"wb");
	if(f){
		for(std::map<u8string,u8string>::iterator it=cfg.begin();it!=cfg.end();++it){
			size_t m=it->first.size();
			if(m) u8fwrite(it->first.c_str(),m,1,f);

			u8fwrite("=",1,1,f);

			m=it->second.size();
			if(m) u8fwrite(it->second.c_str(),m,1,f);

			u8fwrite("\r\n",2,1,f);
		}
		u8fclose(f);
	}
}

bool PuzzleBoyApp::LoadLocale(){
	if(m_sLocale.empty()){
		return m_objGetText.LoadFileWithAutoLocale("data/locale/*.mo");
	}else if(m_sLocale.find_first_of('?')!=u8string::npos){
		//locale disabled, do nothing
		m_objGetText.Close();
		return true;
	}else{
		if(!m_objGetText.LoadFile(u8string("data/locale/")+m_sLocale+".mo")){
			printf("[PuzzleBoyApp] Error: Failed to load locale '%s'\n",m_sLocale.c_str());
			return false;
		}
		return true;
	}
}

void PuzzleBoyApp::Draw(){
	if(m_view.empty()) return;

	int m=m_view.size();
	if(m==1){
		m_view[0]->m_scrollView.SetProjectionMatrix();
		m_view[0]->Draw();
		m_view[0]->m_scrollView.DrawScrollBar();
	}else{
		for(int i=0;i<m;i++){
			m_view[i]->m_scrollView.EnableScissorRect();
			m_view[i]->m_scrollView.SetProjectionMatrix();
			m_view[i]->Draw();
			m_view[i]->m_scrollView.DrawScrollBar();
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

		view->m_sPlayerName=m_sPlayerName[0];
		view->m_nKey=m_nDefaultMultiplayerKey;
		view->m_scrollView.m_bAutoResize=true;
		view->m_scrollView.m_fAutoResizeScale[0]=0.0f;
		view->m_scrollView.m_fAutoResizeScale[1]=0.0f;
		view->m_scrollView.m_fAutoResizeScale[2]=0.5f;
		view->m_scrollView.m_fAutoResizeScale[3]=1.0f;
		view->m_scrollView.m_nAutoResizeOffset[0]=0;
		view->m_scrollView.m_nAutoResizeOffset[1]=0;
		view->m_scrollView.m_nAutoResizeOffset[2]=-4;
		view->m_scrollView.m_nAutoResizeOffset[3]=-128;
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

		view->m_sPlayerName=m_sPlayerName[1];
		view->m_nKey=m_nDefaultMultiplayerKey+8;
		view->m_scrollView.m_bAutoResize=true;
		view->m_scrollView.m_fAutoResizeScale[0]=0.5f;
		view->m_scrollView.m_fAutoResizeScale[1]=0.0f;
		view->m_scrollView.m_fAutoResizeScale[2]=1.0f;
		view->m_scrollView.m_fAutoResizeScale[3]=1.0f;
		view->m_scrollView.m_nAutoResizeOffset[0]=4;
		view->m_scrollView.m_nAutoResizeOffset[1]=0;
		view->m_scrollView.m_nAutoResizeOffset[2]=0;
		view->m_scrollView.m_nAutoResizeOffset[3]=-128;
		view->m_scrollView.OnTimer();

		view->m_pDocument=m_pDocument;
		view->m_nCurrentLevel=m_nCurrentLevel;
		view->StartGame();

		m_view.push_back(view);
	}else{
		//single player
		PuzzleBoyLevelView *view=new PuzzleBoyLevelView();

		view->m_sPlayerName=m_sPlayerName[0];
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
		m_view[i]->OnKeyUp(nChar,nFlags);
	}
}

void PuzzleBoyApp::OnMouseEvent(int nFlags, int xMouse, int yMouse, int nType){
	//TODO: OnMouseEvent
}
