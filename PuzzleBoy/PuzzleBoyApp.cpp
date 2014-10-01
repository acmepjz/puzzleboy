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
extern SDL_Event event;

//test
#ifdef ANDROID
#define TOUCHSCREEN_DEFAULT_VALUE true
#else
#define TOUCHSCREEN_DEFAULT_VALUE false
#endif

PuzzleBoyApp::PuzzleBoyApp()
:m_nAnimationSpeed(0)
,m_bShowGrid(false)
,m_bShowLines(true)
,m_bInternationalFont(true)
,m_bAutoSave(true)
,m_bContinuousKey(true)
,m_bShowFPS(false)
,m_bShowMainMenuButton(true)
,m_bAutoSaveRandomMap(true)
,m_pDocument(NULL)
,m_nCurrentLevel(0)
,m_nMyResizeTime(-1)
,m_nToolTipTime(0)
,m_bToolTipIsExit(false)
,m_bTouchscreen(TOUCHSCREEN_DEFAULT_VALUE)
,m_nTouchConfig(2)
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
	bool ret=false;
	u8file *f=u8fopen(fileName.c_str(),"rb");
	if(f){
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

		if(pDoc){
			DestroyGame();
			m_nCurrentLevel=0;
			m_sLastFile=fileName;
			ret=true;
		}
	}

	if(!ret){
		printf("[PuzzleBoyApp] Error: Failed to load file %s\n",fileName.c_str());
	}

	return ret;
}

bool PuzzleBoyApp::SaveFile(const u8string& fileName){
	if(m_pDocument==NULL) return false;

	bool ret=false;
	u8file *f=u8fopen(fileName.c_str(),"wb");
	if(f){
		MFCSerializer ar;
		ar.SetFile(f,true,4096);

		ret=m_pDocument->MFCSerialize(ar);
		ar.FileFlush();

		u8fclose(f);

		m_sLastFile=fileName;
	}

	if(ret){
		m_pDocument->m_bModified=false;
	}else{
		printf("[PuzzleBoyApp] Error: Failed to save file %s\n",fileName.c_str());
	}

	return ret;
}

static u8string GetConfig(const std::map<u8string,u8string>& cfg,const u8string& name,const u8string& defaultValue){
	std::map<u8string,u8string>::const_iterator it=cfg.find(name);
	if(it==cfg.end()) return defaultValue;
	else return it->second;
}

static int GetConfig(const std::map<u8string,u8string>& cfg,const u8string& name,int defaultValue){
	std::map<u8string,u8string>::const_iterator it=cfg.find(name);
	if(it==cfg.end()) return defaultValue;
	else{
		int i;
		if(sscanf(it->second.c_str(),"%d",&i)!=1) return defaultValue;
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
	if(m_nAnimationSpeed<0 || m_nAnimationSpeed>3) m_nAnimationSpeed=0;

	m_bShowGrid=GetConfig(cfg,"ShowGrid",0)!=0;
	m_bShowLines=GetConfig(cfg,"ShowLines",1)!=0;
	m_bInternationalFont=GetConfig(cfg,"InternationalFont",1)!=0;

	m_sPlayerName[0]=toUTF16(GetConfig(cfg,"PlayerName",_("Player")));
	m_sPlayerName[1]=toUTF16(GetConfig(cfg,"PlayerName2",_("Player2")));

	const int defaultKey[24]={
		SDLK_UP,
		SDLK_DOWN,
		SDLK_LEFT,
		SDLK_RIGHT,
		SDLK_u, //undo
		SDLK_r, //redo
		SDLK_SPACE, //switch
		0, //restart
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

	for(int i=0;i<24;i++){
		char s[8];
		sprintf(s,"Key%d",i+1);
		m_nKey[i]=GetConfig(cfg,s,defaultKey[i]);
	}

	m_sLocale=GetConfig(cfg,"Locale","");
	if(m_sLocale.find_first_of('?')!=u8string::npos) m_sLocale="?";

	m_nThreadCount=GetConfig(cfg,"ThreadCount",0);
	if(m_nThreadCount<0 || m_nThreadCount>8) m_nThreadCount=0;

	m_nOrientation=GetConfig(cfg,"Orientation",0);
	if(m_nOrientation<0 || m_nOrientation>2) m_nOrientation=0;

	m_bAutoSave=GetConfig(cfg,"AutoSave",1)!=0;
	m_sLastFile=GetConfig(cfg,"LastFile","");
	m_nCurrentLevel=GetConfig(cfg,"LastLevel",0);
	m_sLastRecord=GetConfig(cfg,"LastRecord","");

	m_bContinuousKey=GetConfig(cfg,"ContinuousKey",1)!=0;
	m_bShowFPS=GetConfig(cfg,"ShowFPS",0)!=0;
#ifdef __IPHONEOS__
	m_bShowMainMenuButton=true;
#else
	m_bShowMainMenuButton=GetConfig(cfg,"ShowMainMenuButton",1)!=0;
#endif
	m_bAutoSaveRandomMap=GetConfig(cfg,"AutoSaveRandomMap",1)!=0;

	m_nButtonSize=GetConfig(cfg,"ButtonSize",64);
	m_nMenuTextSize=GetConfig(cfg,"MenuTextSize",32);
	m_fMenuTextScale=m_nMenuTextSize/32.0f;
	m_nMenuHeightFactor=GetConfig(cfg,"MenuHeightFactor",4);
	m_nMenuHeight=(m_nMenuTextSize*m_nMenuHeightFactor)>>2;

	m_nTouchConfig=GetConfig(cfg,"IsTouchscreen",2);
	if(m_nTouchConfig<0 || m_nTouchConfig>2) m_nTouchConfig=2;
}

void PuzzleBoyApp::SaveConfig(const u8string& fileName){
	std::map<u8string,u8string> cfg;

	PutConfig(cfg,"AnimationSpeed",m_nAnimationSpeed);
	PutConfig(cfg,"ShowGrid",m_bShowGrid?1:0);
	PutConfig(cfg,"ShowLines",m_bShowLines?1:0);
	PutConfig(cfg,"InternationalFont",m_bInternationalFont?1:0);

	cfg["PlayerName"]=toUTF8(m_sPlayerName[0]);
	cfg["PlayerName2"]=toUTF8(m_sPlayerName[1]);

	for(int i=0;i<24;i++){
		char s[8];
		sprintf(s,"Key%d",i+1);
		PutConfig(cfg,s,m_nKey[i]);
	}

	cfg["Locale"]=m_sLocale;

	PutConfig(cfg,"ThreadCount",m_nThreadCount);
	PutConfig(cfg,"Orientation",m_nOrientation);

	PutConfig(cfg,"AutoSave",m_bAutoSave?1:0);
	if(m_bAutoSave){
		cfg["LastFile"]=m_sLastFile;
		PutConfig(cfg,"LastLevel",m_nCurrentLevel);
		cfg["LastRecord"]=m_sLastRecord;
	}

	PutConfig(cfg,"ContinuousKey",m_bContinuousKey?1:0);
	PutConfig(cfg,"ShowFPS",m_bShowFPS?1:0);
	PutConfig(cfg,"ShowMainMenuButton",m_bShowMainMenuButton?1:0);
	PutConfig(cfg,"AutoSaveRandomMap",m_bAutoSaveRandomMap?1:0);

	PutConfig(cfg,"ButtonSize",m_nButtonSize);
	PutConfig(cfg,"MenuTextSize",m_nMenuTextSize);
	PutConfig(cfg,"MenuHeightFactor",m_nMenuHeightFactor);

	PutConfig(cfg,"IsTouchscreen",m_nTouchConfig);

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

	for(int i=0,m=m_view.size();i<m;i++){
		m_view[i]->m_scrollView.EnableScissorRect();
		m_view[i]->m_scrollView.SetProjectionMatrix();
		m_view[i]->Draw();
	}
	SimpleScrollView::DisableScissorRect();

	if(m_bShowMainMenuButton){
		std::vector<float> v;
		std::vector<unsigned short> i;
		AddScreenKeyboard(float(screenWidth-m_nButtonSize),
			0.0f,
			float(m_nButtonSize),
			float(m_nButtonSize),
			SCREEN_KEYBOARD_MORE,v,i);

		SetProjectionMatrix(1);

		DrawScreenKeyboard(v,i,0x80FFFFFF);
	}
}

void PuzzleBoyApp::DestroyGame(){
	for(unsigned int i=0;i<m_view.size();i++){
		delete m_view[i];
	}
	m_view.clear();
}

bool PuzzleBoyApp::StartGame(int nPlayerCount,bool bEditMode,bool bTestMode,int currentLevel2){
	DestroyGame();

	m_nMyResizeTime=-1;
	touchMgr.clear();

	if(nPlayerCount==2){
		//multiplayer
		PuzzleBoyLevelView *view=new PuzzleBoyLevelView();

		view->m_sPlayerName=m_sPlayerName[0];
		view->m_nKey=m_nKey+8;
		view->m_scrollView.m_flags|=SimpleScrollViewFlags::AutoResize;
		switch(m_nOrientation){
		case 0:
			//normal
			view->m_scrollView.m_fAutoResizeScale[2]=0.5f;
			view->m_scrollView.m_fAutoResizeScale[3]=1.0f;
			view->m_scrollView.m_nAutoResizeOffset[2]=-4;
			view->m_scrollView.m_nAutoResizeOffset[3]=-2*m_nButtonSize;
			view->m_scrollView.m_nScissorOffset[3]=2*m_nButtonSize;
			break;
		case 1:
			//horizontal up-down
			view->m_scrollView.m_fAutoResizeScale[2]=0.5f;
			view->m_scrollView.m_fAutoResizeScale[3]=1.0f;
			view->m_scrollView.m_nAutoResizeOffset[2]=-4;
			view->m_scrollView.m_nAutoResizeOffset[1]=2*m_nButtonSize;
			view->m_scrollView.m_nScissorOffset[1]=-2*m_nButtonSize;
			view->m_scrollView.m_nOrientation=2;
			break;
		case 2:
			//vertical up-down
			view->m_scrollView.m_fAutoResizeScale[2]=1.0f;
			view->m_scrollView.m_fAutoResizeScale[3]=0.5f;
			view->m_scrollView.m_nAutoResizeOffset[1]=2*m_nButtonSize;
			view->m_scrollView.m_nAutoResizeOffset[3]=-4;
			view->m_scrollView.m_nScissorOffset[1]=-2*m_nButtonSize;
			view->m_scrollView.m_nOrientation=2;
			break;
		}
		view->m_scrollView.OnTimer();

		view->m_pDocument=m_pDocument;
		view->m_nCurrentLevel=m_nCurrentLevel;
		if(!view->StartGame()){
			delete view;
			return false;
		}
		m_view.push_back(view);

		touchMgr.AddView(0,0,0,0,MultiTouchViewFlags::AcceptDragging | MultiTouchViewFlags::AcceptZoom,view);

		//player 2
		view=new PuzzleBoyLevelView();

		view->m_sPlayerName=m_sPlayerName[1];
		view->m_nKey=m_nKey+16;
		view->m_scrollView.m_flags|=SimpleScrollViewFlags::AutoResize;
		view->m_scrollView.m_fAutoResizeScale[2]=1.0f;
		view->m_scrollView.m_fAutoResizeScale[3]=1.0f;
		view->m_scrollView.m_nAutoResizeOffset[3]=-2*m_nButtonSize;
		view->m_scrollView.m_nScissorOffset[3]=2*m_nButtonSize;
		switch(m_nOrientation){
		case 0:
		case 1:
			//normal and horizontal up-down
			view->m_scrollView.m_fAutoResizeScale[0]=0.5f;
			view->m_scrollView.m_nAutoResizeOffset[0]=4;
			break;
		case 2:
			//vertical up-down
			view->m_scrollView.m_fAutoResizeScale[1]=0.5f;
			view->m_scrollView.m_nAutoResizeOffset[1]=4;
			break;
		}
		view->m_scrollView.OnTimer();

		view->m_pDocument=m_pDocument;
		view->m_nCurrentLevel=(currentLevel2>=0
			&& currentLevel2<(int)m_pDocument->m_objLevels.size())?currentLevel2:m_nCurrentLevel;
		view->StartGame();

		m_view.push_back(view);

		touchMgr.AddView(0,0,0,0,MultiTouchViewFlags::AcceptDragging | MultiTouchViewFlags::AcceptZoom,view);
	}else{
		//single player (or edit mode)
		PuzzleBoyLevelView *view=new PuzzleBoyLevelView();

		view->m_bEditMode=bEditMode;
		view->m_bTestMode=bTestMode;
		view->m_sPlayerName=m_sPlayerName[0];
		view->m_nKey=m_nKey;
		view->m_scrollView.m_flags|=SimpleScrollViewFlags::AutoResize;
		view->m_scrollView.m_fAutoResizeScale[2]=1.0f;
		view->m_scrollView.m_fAutoResizeScale[3]=1.0f;
		view->m_scrollView.m_nAutoResizeOffset[3]=-2*m_nButtonSize;
		view->m_scrollView.m_nScissorOffset[3]=2*m_nButtonSize;
		view->m_scrollView.OnTimer();

		view->m_pDocument=m_pDocument;
		view->m_nCurrentLevel=m_nCurrentLevel;
		if(!view->StartGame()){
			delete view;
			return false;
		}
		m_view.push_back(view);

		touchMgr.AddView(0,0,0,0,MultiTouchViewFlags::AcceptDragging | MultiTouchViewFlags::AcceptZoom,view);
	}

	return true;
}

void PuzzleBoyApp::ApplyRecord(const u8string& record,bool animationDemo,bool testMode){
	StartGame(1,false,testMode);

	m_view[0]->m_bPlayFromRecord=true;
	if(animationDemo){
		m_view[0]->m_sRecord=record;
		m_view[0]->m_nRecordIndex=0;
	}else{
		m_view[0]->m_objPlayingLevel->ApplyRecord(record);
		m_view[0]->m_sRecord.clear();
		m_view[0]->m_nRecordIndex=-1;
	}
}

bool PuzzleBoyApp::OnTimer(){
	bool bDirty=false;

	if(m_nMyResizeTime!=m_nResizeTime){
		m_nMyResizeTime=m_nResizeTime;
		bDirty=true;

		for(unsigned int i=0;i<m_view.size();i++){
			MultiTouchViewStruct *view=touchMgr.FindView(m_view[i]);

			if(view){
				if(m_view.size()==1){
					//single player
					view->left=0.0f;
					view->top=0.0f;
					view->right=screenAspectRatio;
					view->bottom=1.0f;
					m_view[i]->m_scrollView.m_nAutoResizeOffset[3]=-2*m_nButtonSize;
					m_view[i]->m_scrollView.m_nScissorOffset[3]=2*m_nButtonSize;
				}else if(i==0){
					//player 1
					if(m_nOrientation==2){
						view->left=0.0f;
						view->top=0.0f;
						view->right=screenAspectRatio;
						view->bottom=0.5f;
					}else{
						view->left=0.0f;
						view->top=0.0f;
						view->right=screenAspectRatio*0.5f;
						view->bottom=1.0f;
					}
					if(m_nOrientation==0){
						m_view[i]->m_scrollView.m_nAutoResizeOffset[3]=-2*m_nButtonSize;
						m_view[i]->m_scrollView.m_nScissorOffset[3]=2*m_nButtonSize;
					}else{
						m_view[i]->m_scrollView.m_nAutoResizeOffset[1]=2*m_nButtonSize;
						m_view[i]->m_scrollView.m_nScissorOffset[1]=-2*m_nButtonSize;
					}
				}else{
					//player 2
					if(m_nOrientation==2){
						view->left=0.0f;
						view->top=0.5f;
						view->right=screenAspectRatio;
						view->bottom=1.0f;
					}else{
						view->left=screenAspectRatio*0.5f;
						view->top=0.0f;
						view->right=screenAspectRatio;
						view->bottom=1.0f;
					}
					m_view[i]->m_scrollView.m_nAutoResizeOffset[3]=-2*m_nButtonSize;
					m_view[i]->m_scrollView.m_nScissorOffset[3]=2*m_nButtonSize;
				}
			}
		}
	}

	for(unsigned int i=0;i<m_view.size();i++){
		bDirty=m_view[i]->OnTimer() || bDirty;
	}

	return bDirty;
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

void PuzzleBoyApp::ShowToolTip(const u8string& text,bool isExit){
	m_sToolTipText=text;
	m_bToolTipIsExit=isExit;

	if(m_nToolTipTime<16) m_nToolTipTime=127-m_nToolTipTime;
	else if(m_nToolTipTime<112) m_nToolTipTime=112;
}
