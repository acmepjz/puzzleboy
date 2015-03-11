#include "LevelRecordScreen.h"
#include "SimpleText.h"
#include "PuzzleBoyApp.h"
#include "SimpleTextBox.h"
#include "SimpleListScreen.h"
#include "PuzzleBoyLevelView.h"
#include "main.h"
#include "NetworkManager.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "include_sdl.h"
#include "include_gl.h"

//ad-hoc
extern SDL_Event event;
extern bool m_bKeyDownProcessed;

class RecordListScreen:public SimpleListScreen{
public:
	void OnDirty() override{
		ResetList();
		for(int i=0,m=theApp->m_view[0]->m_tCurrentBestRecord.size();i<m;i++){
			char s[16];
			sprintf(s,": %d",theApp->m_view[0]->m_tCurrentBestRecord[i].nStep);
			AddItem(toUTF8(theApp->m_view[0]->m_tCurrentBestRecord[i].sPlayerName)+s);
		}
	}
	int OnClick(int index) override{
		return index+1;
	}
	int DoModal() override{
		m_bDirtyOnResize=true;
		m_titleBar.m_sTitle=_("List of Records");
		return SimpleListScreen::DoModal();
	}
};

int LevelRecordScreen(const u8string& title,const u8string& prompt,u8string& record,bool readOnly){
	bool b=true;
	int ret=0;

	int buttonSize=theApp->m_nButtonSize;

	SimpleText *m_txtPrompt=new SimpleText;

	//create text
	m_txtPrompt->AddString(mainFont,prompt,64,float(buttonSize+32),0,0,
		1.0f,DrawTextFlags::Multiline);

	//create title bar
	SimpleTitleBar titleBar;
	{
		titleBar.m_sTitle=title;
		titleBar.m_LeftButtons.assign(1,SCREEN_KEYBOARD_LEFT);
		const int rightButtons[]={SCREEN_KEYBOARD_COPY,SCREEN_KEYBOARD_PASTE,SCREEN_KEYBOARD_SEARCH};
		titleBar.m_RightButtons.assign(rightButtons,rightButtons+(readOnly?1:3));
		titleBar.m_AdditionalButtons.push_back(
			SimpleTitleBarButton(0x1001,-1,_("Apply").c_str(),
			-224,buttonSize+32,-64,buttonSize+96,
			1.0f,0.0f,1.0f,0.0f));
		titleBar.m_AdditionalButtons.push_back(
			SimpleTitleBarButton(0x1002,-1,_("Animation Demo").c_str(),
			-224,buttonSize+128,-64,buttonSize+192,
			1.0f,0.0f,1.0f,0.0f));
	}

	MultiTouchManager mgr;
	SimpleTextBox txt;

	txt.m_scrollView.m_flags|=SimpleScrollViewFlags::AutoResize;
	txt.m_scrollView.m_fAutoResizeScale[2]=1.0f;
	txt.m_scrollView.m_fAutoResizeScale[3]=1.0f;
	txt.m_scrollView.m_nAutoResizeOffset[0]=64;
	txt.m_scrollView.m_nAutoResizeOffset[1]=buttonSize+224;
	txt.m_scrollView.m_nAutoResizeOffset[2]=-64;
	txt.m_scrollView.m_nAutoResizeOffset[3]=-32;
	txt.m_bLocked=readOnly;
	txt.SetMultiline(true,true);
	txt.SetText(record);

	while(m_bRun && b){
		//do network (????)
		netMgr->OnTimer();

		//create title bar buttons
		bool bDirty=titleBar.OnTimer();

		bDirty|=txt.OnTimer();
		txt.RegisterView(mgr);

		UpdateIdleTime(bDirty);

		if(NeedToDrawScreen()){
			//clear and draw
			ClearScreen();

			SetProjectionMatrix(1);

			glDisable(GL_LIGHTING);

			//draw title bar
			titleBar.Draw();

			//draw prompt text
			if(m_txtPrompt && !m_txtPrompt->empty()){
				mainFont->DrawString(*m_txtPrompt,SDL_MakeColor(255,255,255,255));
			}

			//draw textbox
			txt.Draw();

			//over
			ShowScreen();
		}

		while(SDL_PollEvent(&event)){
			if(mgr.OnEvent()) continue;
			if(txt.OnEvent()) continue;

			if(event.type==SDL_MOUSEBUTTONDOWN){
				txt.ClearFocus(); //???
			}

			int titleBarResult=titleBar.OnEvent();
			if(titleBarResult>=-1){
				m_nIdleTime=0;
				switch(titleBarResult){
				case SCREEN_KEYBOARD_LEFT:
					b=false;
					break;
				case SCREEN_KEYBOARD_COPY:
					txt.CopyToClipboard();
					break;
				case SCREEN_KEYBOARD_PASTE:
					txt.PasteFromClipboard();
					break;
				case SCREEN_KEYBOARD_SEARCH:
					{
						//view all records
						txt.ClearFocus();
						int ret=RecordListScreen().DoModal();
						if(ret>0){
							u8string s;
							RecordManager::ConvertRecordDataToString(theApp->m_view[0]->m_tCurrentBestRecord[ret-1].bSolution,s);
							txt.SetText(s);
						}
					}
					break;
				case 0x1001:
					//apply
					ret=1;
					b=false;
					break;
				case 0x1002:
					//animation demo
					ret=2;
					b=false;
					break;
				}
				continue;
			}

			switch(event.type){
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym){
				case SDLK_AC_BACK:
				case SDLK_ESCAPE:
					m_bKeyDownProcessed=true;
					b=false;
					break;
				}
				break;
			}
		}

		WaitForNextFrame();
	}

	SDL_StopTextInput();

	if(ret) txt.GetText(record);

	delete m_txtPrompt;

	m_nIdleTime=0;

	return ret;
}
