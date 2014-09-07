#include "SimpleMiscScreen.h"
#include "SimpleText.h"
#include "PuzzleBoyApp.h"
#include "SimpleTextBox.h"
#include "main.h"

#include <string.h>
#include <math.h>

#include "include_sdl.h"
#include "include_gl.h"

//ad-hoc
extern SDL_Event event;
extern bool m_bKeyDownProcessed;

bool SimpleInputScreen(const u8string& title,const u8string& prompt,u8string& text,const char* allowedChars){
	bool b=true,ret=false;

	int buttonSize=theApp->m_nButtonSize;

	SimpleText *m_txtTitle=new SimpleText,*m_txtPrompt=new SimpleText;
	int m_nMyResizeTime=-1;

	//create text
	m_txtTitle->AddString(titleFont?titleFont:mainFont,title,1.25f*float(buttonSize),0,0,float(buttonSize),
		(titleFont?1.0f:1.5f)*float(theApp->m_nButtonSize)/64.0f,DrawTextFlags::VCenter);
	m_txtPrompt->AddString(mainFont,prompt,64,float(buttonSize+32),0,0,
		1.0f,DrawTextFlags::Multiline);

	//for title bar buttons
	std::vector<float> m_v;
	std::vector<unsigned short> m_idx;

	MultiTouchManager mgr;
	SimpleTextBox txt;

	txt.m_scrollView.m_flags|=SimpleScrollViewFlags::AutoResize;
	txt.m_scrollView.m_fAutoResizeScale[2]=1.0f;
	txt.m_scrollView.m_nAutoResizeOffset[0]=64;
#ifdef ANDROID
	txt.m_scrollView.m_nAutoResizeOffset[1]=buttonSize+136;
#else
	txt.m_scrollView.m_fAutoResizeScale[1]=0.5f;
	txt.m_scrollView.m_fAutoResizeScale[3]=0.5f;
	txt.m_scrollView.m_nAutoResizeOffset[1]=(buttonSize-64)/2;
#endif
	txt.m_scrollView.m_nAutoResizeOffset[2]=-64;
	txt.m_scrollView.m_nAutoResizeOffset[3]=txt.m_scrollView.m_nAutoResizeOffset[1]+40;
	if(allowedChars) txt.SetAllowedChars(allowedChars);
	txt.SetText(text);
	txt.SetFocus();

	while(m_bRun && b){
		bool bDirty=false;

		//create title bar buttons
		if(m_nMyResizeTime!=m_nResizeTime){
			m_nMyResizeTime=m_nResizeTime;

			m_v.clear();
			m_idx.clear();

			const int left=buttonSize;
			const int right=screenWidth-buttonSize*3;

			AddScreenKeyboard(0,0,float(buttonSize),float(buttonSize),SCREEN_KEYBOARD_LEFT,m_v,m_idx);

			AddScreenKeyboard(float(right),0,float(buttonSize),float(buttonSize),SCREEN_KEYBOARD_COPY,m_v,m_idx);
			AddScreenKeyboard(float(right+buttonSize),0,float(buttonSize),float(buttonSize),SCREEN_KEYBOARD_PASTE,m_v,m_idx);
			AddScreenKeyboard(float(right+buttonSize*2),0,float(buttonSize),float(buttonSize),SCREEN_KEYBOARD_YES,m_v,m_idx);

			AddEmptyHorizontalButton(float(left),0,float(right),float(buttonSize),m_v,m_idx);

			bDirty=true;
		}

		bDirty|=txt.OnTimer();
		txt.RegisterView(mgr);

		UpdateIdleTime(bDirty);

		if(NeedToDrawScreen()){
			//clear and draw
			ClearScreen();

			SetProjectionMatrix(1);

			glDisable(GL_LIGHTING);

			//ad-hoc title bar
			DrawScreenKeyboard(m_v,m_idx);

			//draw title text
			if(m_txtTitle && !m_txtTitle->empty()){
				SimpleBaseFont *fnt=titleFont?titleFont:mainFont;

				fnt->DrawString(*m_txtTitle,SDL_MakeColor(255,255,255,255));
			}

			//draw prompt text
			if(m_txtPrompt && !m_txtPrompt->empty()){
				mainFont->DrawString(*m_txtPrompt,SDL_MakeColor(255,255,255,255));
			}

			//draw textbox
			txt.Draw();

			//draw overlay
			txt.DrawOverlay();

			//over
			ShowScreen();
		}

		while(SDL_PollEvent(&event)){
			if(mgr.OnEvent()) continue;
			if(txt.OnEvent()) continue;

			switch(event.type){
			case SDL_MOUSEBUTTONDOWN:
				txt.ClearFocus(); //???
				break;
			case SDL_MOUSEBUTTONUP:
				if(event.button.y<buttonSize){
					//check clicked title bar buttons
					if(event.button.x<0){
						//nothing
					}else if(event.button.x<buttonSize){
						//cancel
						b=false;
					}else if(event.button.x<screenWidth-buttonSize*3){
						//nothing
					}else if(event.button.x<screenWidth-buttonSize*2){
						//copy
						txt.CopyToClipboard();
					}else if(event.button.x<screenWidth-buttonSize){
						//paste
						txt.PasteFromClipboard();
					}else if(event.button.x<screenWidth){
						//ok
						b=false;
						ret=true;
					}
				}

				break;
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym){
				case SDLK_AC_BACK:
				case SDLK_ESCAPE:
					m_bKeyDownProcessed=true;
					b=false;
					break;
				case SDLK_RETURN:
					m_bKeyDownProcessed=true;
					b=false;
					ret=true;
					break;
				}
				break;
			}
		}

		WaitForNextFrame();
	}

	SDL_StopTextInput();

	if(ret) txt.GetText(text);

	delete m_txtTitle;
	delete m_txtPrompt;

	return ret;
}

int SimpleConfigKeyScreen(int key){
	bool b=true;

	int buttonSize=theApp->m_nButtonSize;

	SimpleText *m_txtTitle=new SimpleText,*m_txtPrompt=new SimpleText;
	int m_nMyResizeTime=-1;

	//create text
	m_txtTitle->AddString(titleFont?titleFont:mainFont,_("Config Key"),1.25f*float(buttonSize),0,0,float(buttonSize),
		(titleFont?1.0f:1.5f)*float(theApp->m_nButtonSize)/64.0f,DrawTextFlags::VCenter);
	m_txtPrompt->AddString(mainFont,_("Press any key..."),64,float(buttonSize+32),0,0,1.0f,0);

	//for title bar buttons
	std::vector<float> m_v;
	std::vector<unsigned short> m_idx;

	while(m_bRun && b){
		//get position of text box
		SDL_Rect r={64,(screenHeight+buttonSize-64)/2,screenWidth-192,64};

		//create title bar buttons
		if(m_nMyResizeTime!=m_nResizeTime){
			m_nMyResizeTime=m_nResizeTime;
			m_nIdleTime=0;

			m_v.clear();
			m_idx.clear();

			const int left=buttonSize;
			const int right=screenWidth-buttonSize;

			AddScreenKeyboard(0,0,float(buttonSize),float(buttonSize),SCREEN_KEYBOARD_LEFT,m_v,m_idx);

			AddScreenKeyboard(float(right),0,float(buttonSize),float(buttonSize),SCREEN_KEYBOARD_YES,m_v,m_idx);

			AddScreenKeyboard(float(r.x+r.w),float(r.y),64,64,SCREEN_KEYBOARD_NO,m_v,m_idx);

			AddEmptyHorizontalButton(float(left),0,float(right),float(buttonSize),m_v,m_idx);
		}

		//update idle time
		UpdateIdleTime(false);

		//clear and draw (if not idle, otherwise only draw after 32 frames)
		if(NeedToDrawScreen()){
			ClearScreen();

			SetProjectionMatrix(1);

			glDisable(GL_LIGHTING);

			//ad-hoc title bar
			DrawScreenKeyboard(m_v,m_idx);

			//draw title text
			if(m_txtTitle && !m_txtTitle->empty()){
				SimpleBaseFont *fnt=titleFont?titleFont:mainFont;

				fnt->DrawString(*m_txtTitle,SDL_MakeColor(255,255,255,255));
			}

			//draw prompt text
			if(m_txtPrompt && !m_txtPrompt->empty()){
				mainFont->DrawString(*m_txtPrompt,SDL_MakeColor(255,255,255,255));
			}

			//draw border
			{
				float vv[8]={
					float(r.x),float(r.y),
					float(r.x),float(r.y+r.h),
					float(r.x+r.w),float(r.y+r.h),
					float(r.x+r.w),float(r.y),
				};

				unsigned short ii[6]={
					3,0,0,1,1,2,
				};

				glColor4f(0.5f, 0.5f, 0.5f, 1.0f);

				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(2,GL_FLOAT,0,vv);
				glDrawElements(GL_LINES,6,GL_UNSIGNED_SHORT,ii);
				glDisableClientState(GL_VERTEX_ARRAY);

				//draw text
				u8string s;
				if(key) s=SDL_GetKeyName(key);
				else s=_("(None)");

				mainFont->DrawString(s,float(r.x),float(r.y),float(r.w),float(r.h),
					1.0f,DrawTextFlags::Center | DrawTextFlags::VCenter,SDL_MakeColor(255,255,255,255));
			}

			//over
			ShowScreen();
		}

		while(SDL_PollEvent(&event)){
			switch(event.type){
			case SDL_MOUSEMOTION:
				//TODO:
				break;
			case SDL_MOUSEBUTTONDOWN:
				//TODO:
				break;
			case SDL_MOUSEBUTTONUP:
				m_nIdleTime=0;
				if(event.button.y<buttonSize){
					//check clicked title bar buttons
					if(event.button.x>=0 && event.button.x<buttonSize){
						b=false;
						key=-1;
					}else if(event.button.x>=screenWidth-buttonSize && event.button.x<screenWidth){
						b=false;
					}
				}else if(event.button.x>=r.x+r.w && event.button.x<r.x+r.w+64
					&& event.button.y>=r.y && event.button.y<r.y+r.h)
				{
					key=0;
				}

				break;
			case SDL_KEYDOWN:
				m_nIdleTime=0;
				switch(event.key.keysym.sym){
				case SDLK_AC_BACK:
				case SDLK_ESCAPE:
					m_bKeyDownProcessed=true;
					b=false;
					key=-1;
					break;
				default:
					key=event.key.keysym.sym;
					break;
				}
				break;
			}
		}

		WaitForNextFrame();
	}

	delete m_txtTitle;
	delete m_txtPrompt;

	return key;
}
