#include "LevelRecordScreen.h"
#include "SimpleText.h"
#include "PuzzleBoyApp.h"
#include "SimpleTextBox.h"
#include "SimpleListScreen.h"
#include "PuzzleBoyLevelView.h"
#include "main.h"

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
			char s[32];
			AddItem(toUTF8(theApp->m_view[0]->m_tCurrentBestRecord[i].sPlayerName));
			sprintf(s,"%d",theApp->m_view[0]->m_tCurrentBestRecord[i].nStep);
			AddItem(s,false,float(screenWidth),0,DrawTextFlags::Right);
		}
	}
	int OnClick(int index) override{
		return index+1;
	}
	int DoModal() override{
		m_bDirtyOnResize=true;
		m_LeftButtons.push_back(SCREEN_KEYBOARD_LEFT);
		CreateTitleBarText(_("List of Records"));
		return SimpleListScreen::DoModal();
	}
};

int LevelRecordScreen(const u8string& title,const u8string& prompt,u8string& record,bool readOnly){
	bool b=true;
	int ret=0;

	int buttonSize=theApp->m_nButtonSize;

	SimpleText *m_txtTitle=new SimpleText,*m_txtPrompt=new SimpleText;
	int m_nMyResizeTime=-1;

	//create text
	m_txtTitle->AddString(titleFont?titleFont:mainFont,title,1.25f*float(buttonSize),0,0,float(buttonSize),
		(titleFont?1.0f:1.5f)*float(theApp->m_nButtonSize)/64.0f,DrawTextFlags::VCenter);

	//for title bar buttons
	std::vector<float> m_v;
	std::vector<unsigned short> m_idx;

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
		//create title bar buttons
		if(m_nMyResizeTime!=m_nResizeTime){
			m_nMyResizeTime=m_nResizeTime;

			m_v.clear();
			m_idx.clear();

			AddScreenKeyboard(0,0,float(buttonSize),float(buttonSize),SCREEN_KEYBOARD_LEFT,m_v,m_idx);

			const int left=buttonSize;
			const int right=screenWidth-buttonSize*(readOnly?1:3);

			AddScreenKeyboard(float(right),0,float(buttonSize),float(buttonSize),SCREEN_KEYBOARD_COPY,m_v,m_idx);
			if(!readOnly){
				AddScreenKeyboard(float(right+buttonSize),0,float(buttonSize),float(buttonSize),SCREEN_KEYBOARD_PASTE,m_v,m_idx);
				AddScreenKeyboard(float(right+buttonSize*2),0,float(buttonSize),float(buttonSize),SCREEN_KEYBOARD_SEARCH,m_v,m_idx);
			}

			AddEmptyHorizontalButton(float(left),0,float(right),float(buttonSize),m_v,m_idx);

			//add two buttons
			AddEmptyHorizontalButton(float(screenWidth-224),float(buttonSize+32),
				float(screenWidth-64),float(buttonSize+96),m_v,m_idx);
			AddEmptyHorizontalButton(float(screenWidth-224),float(buttonSize+128),
				float(screenWidth-64),float(buttonSize+192),m_v,m_idx);

			//create prompt text
			m_txtPrompt->clear();
			m_txtPrompt->AddString(mainFont,prompt,64,float(buttonSize+32),0,0,
				1.0f,DrawTextFlags::Multiline);
			m_txtPrompt->AddString(mainFont,_("Apply"),float(screenWidth-(224-4)),float(buttonSize+32),160-8,64,
				1.0f,DrawTextFlags::Center|DrawTextFlags::VCenter|DrawTextFlags::AutoSize);
			m_txtPrompt->AddString(mainFont,_("Animation Demo"),float(screenWidth-(224-4)),float(buttonSize+128),160-8,64,
				1.0f,DrawTextFlags::Center|DrawTextFlags::VCenter|DrawTextFlags::AutoSize);
		}

		txt.OnTimer();
		txt.RegisterView(mgr);

		//clear and draw
		ClearScreen();

		SetProjectionMatrix(1);

		glDisable(GL_LIGHTING);

		//ad-hoc title bar
		DrawScreenKeyboard(m_v,m_idx);

		//draw title text
		if(m_txtTitle && !m_txtTitle->empty()){
			SimpleBaseFont *fnt=titleFont?titleFont:mainFont;

			fnt->BeginDraw();
			m_txtTitle->Draw(SDL_MakeColor(255,255,255,255));
			fnt->EndDraw();
		}

		//draw prompt text
		if(m_txtPrompt && !m_txtPrompt->empty()){
			mainFont->BeginDraw();
			m_txtPrompt->Draw(SDL_MakeColor(255,255,255,255));
			mainFont->EndDraw();
		}

		//draw textbox
		txt.Draw();

		//over
		ShowScreen();

		while(SDL_PollEvent(&event)){
			if(mgr.OnEvent()) continue;
			if(txt.OnEvent()) continue;

			switch(event.type){
			case SDL_QUIT:
				m_bRun=false;
				break;
			case SDL_MOUSEBUTTONUP:
				if(event.button.y<buttonSize){
					bool bCopy=false;

					//check clicked title bar buttons
					if(event.button.x<0){
						//nothing
					}else if(event.button.x<buttonSize){
						//cancel
						b=false;
					}else if(readOnly){
						if(event.button.x<screenWidth){
							//copy
							bCopy=true;
						}
					}else{
						if(event.button.x<screenWidth-buttonSize*3){
							//nothing
						}else if(event.button.x<screenWidth-buttonSize*2){
							//copy
							bCopy=true;
						}else if(event.button.x<screenWidth-buttonSize){
							//paste
							txt.PasteFromClipboard();
						}else if(event.button.x<screenWidth){
							//view all records
							txt.ClearFocus();
							int ret=RecordListScreen().DoModal();
							if(ret>0){
								u8string s;
								RecordManager::ConvertRecordDataToString(theApp->m_view[0]->m_tCurrentBestRecord[ret-1].bSolution,s);
								txt.SetText(s);
							}
						}
					}

					if(bCopy) txt.CopyToClipboard();
				}else if(event.button.x>=screenWidth-224 && event.button.x<screenWidth-64){
					if(event.button.y>=buttonSize+32 && event.button.y<buttonSize+96){
						//apply
						ret=1;
						b=false;
					}else if(event.button.y>=buttonSize+128 && event.button.y<buttonSize+192){
						//animation demo
						ret=2;
						b=false;
					}
				}

				break;
			case SDL_WINDOWEVENT:
				switch(event.window.event){
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					OnVideoResize(
						event.window.data1,
						event.window.data2);
					break;
				}
				break;
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

	delete m_txtTitle;
	delete m_txtPrompt;

	return ret;
}
