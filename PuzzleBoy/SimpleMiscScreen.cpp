#include "SimpleMiscScreen.h"
#include "SimpleTitleBar.h"
#include "SimpleText.h"
#include "PuzzleBoyApp.h"
#include "SimpleTextBox.h"
#include "MyFormat.h"
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

	SimpleText *m_txtPrompt=new SimpleText;

	//create text
	m_txtPrompt->AddString(mainFont,prompt,64,float(buttonSize+32),0,0,
		1.0f,DrawTextFlags::Multiline);

	//create title bar
	SimpleTitleBar titleBar;
	{
		titleBar.m_sTitle=title;
		titleBar.m_LeftButtons.assign(1,SCREEN_KEYBOARD_LEFT);
		const int rightButtons[]={SCREEN_KEYBOARD_COPY,SCREEN_KEYBOARD_PASTE,SCREEN_KEYBOARD_YES};
		titleBar.m_RightButtons.assign(rightButtons,
			rightButtons+(sizeof(rightButtons)/sizeof(rightButtons[0])));
	}

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
		bool bDirty=titleBar.OnTimer();

		bDirty|=txt.OnTimer();

		txt.RegisterView(mgr);

		UpdateIdleTime(bDirty);

		if(NeedToDrawScreen()){
			//clear and draw
			ClearScreen();

			SetProjectionMatrix(1);

			glDisable(GL_LIGHTING);

			//title bar
			titleBar.Draw();

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
				case SCREEN_KEYBOARD_YES:
					b=false;
					ret=true;
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

	delete m_txtPrompt;

	m_nIdleTime=0;

	return ret;
}

int SimpleConfigKeyScreen(int key){
	bool b=true;

	int buttonSize=theApp->m_nButtonSize;

	SimpleText *m_txtPrompt=new SimpleText;

	//create text
	m_txtPrompt->AddString(mainFont,_("Press any key..."),64,float(buttonSize+32),0,0,1.0f,0);

	//create title bar
	SimpleTitleBar titleBar;
	{
		titleBar.m_sTitle=_("Config Key");
		titleBar.m_LeftButtons.assign(1,SCREEN_KEYBOARD_LEFT);
		titleBar.m_RightButtons.assign(1,SCREEN_KEYBOARD_YES);
		titleBar.m_AdditionalButtons.push_back(
			SimpleTitleBarButton(SCREEN_KEYBOARD_NO,SCREEN_KEYBOARD_NO,NULL,
			-128,(buttonSize-64)/2,-64,(buttonSize+64)/2,
			1.0f,0.5f,1.0f,0.5f));
	}

	while(m_bRun && b){
		//get position of text box
		SDL_Rect r={64,(screenHeight+buttonSize-64)/2,screenWidth-192,64};

		//create title bar buttons
		if(titleBar.OnTimer()) m_nIdleTime=0;

		//update idle time
		UpdateIdleTime(false);

		//clear and draw (if not idle, otherwise only draw after 32 frames)
		if(NeedToDrawScreen()){
			ClearScreen();

			SetProjectionMatrix(1);

			glDisable(GL_LIGHTING);

			//draw title bar
			titleBar.Draw();

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
			int titleBarResult=titleBar.OnEvent();
			if(titleBarResult>=-1){
				m_nIdleTime=0;
				switch(titleBarResult){
				case SCREEN_KEYBOARD_LEFT:
					b=false;
					key=-1;
					break;
				case SCREEN_KEYBOARD_YES:
					b=false;
					break;
				case SCREEN_KEYBOARD_NO:
					key=0;
					break;
				}
				continue;
			}

			switch(event.type){
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

	delete m_txtPrompt;

	m_nIdleTime=0;

	return key;
}

ChangeSizeScreen::ChangeSizeScreen(int w,int h)
:m_nOldWidth(w),m_nOldHeight(h)
,m_nXOffset(0),m_nYOffset(0)
,m_nWidth(w),m_nHeight(h)
,m_bPreserve(false)
{
}

bool ChangeSizeScreen::DoModal(){
	bool b=true,ret=false;

	int buttonSize=theApp->m_nButtonSize;

	SimpleText *m_txtPrompt=new SimpleText;

	SimpleTitleBar titleBar;
	MultiTouchManager mgr;
	SimpleTextBox txt[4];

	u8string yesno[2];
	yesno[0]=_("No");
	yesno[1]=_("Yes");

	//create title bar, text box, etc.
	{
		titleBar.m_sTitle=_("Change Level Size");
		titleBar.m_LeftButtons.assign(1,SCREEN_KEYBOARD_LEFT);
		titleBar.m_RightButtons.assign(1,SCREEN_KEYBOARD_YES);

		u8string prompt[6];
		prompt[0]=_("Level Width");
		prompt[1]=_("Level Height");
		prompt[2]=_("Horizontal Offset");
		prompt[3]=_("Vertical Offset");
		prompt[4]=_("Horizontal Align:");
		prompt[5]=_("Vertical Align:");

		u8string text[4];
		text[0]=str(MyFormat("%d")<<m_nWidth);
		text[1]=str(MyFormat("%d")<<m_nHeight);
		text[2]=str(MyFormat("%d")<<m_nXOffset);
		text[3]=str(MyFormat("%d")<<m_nYOffset);

		int y=buttonSize+32;
		int h1=48,h2=buttonSize;
		if(y+h1*4+h2*3+112>screenHeight){
			h1=40;
			h2=40;
		}

		for(int i=0;i<4;i++){
			if(i==2){
				//preserve
				titleBar.m_AdditionalButtons.push_back(
					SimpleTitleBarButton(0x1010,-1,
					(_("Preserve Level Contents")+": "+yesno[m_bPreserve?1:0]).c_str(),
					64,y,-64,y+h2,0.0f,0.0f,1.0f,0.0f));

				y+=h2+16;
			}

			//label
			m_txtPrompt->AddString(mainFont,prompt[i],64,float(y),176,float(h1),
				1.0f,DrawTextFlags::VCenter|DrawTextFlags::AutoSize);

			//textbox
			txt[i].m_scrollView.m_flags|=SimpleScrollViewFlags::AutoResize;
			txt[i].m_scrollView.m_fAutoResizeScale[2]=1.0f;
			txt[i].m_scrollView.m_nAutoResizeOffset[0]=256;
			txt[i].m_scrollView.m_nAutoResizeOffset[1]=y;
			txt[i].m_scrollView.m_nAutoResizeOffset[2]=-192;
			txt[i].m_scrollView.m_nAutoResizeOffset[3]=y+h1;
			txt[i].SetAllowedChars(i<2?"01234\n56789":"-01234\n56789");
			txt[i].SetText(text[i]);

			//two buttons
			titleBar.m_AdditionalButtons.push_back(
				SimpleTitleBarButton(0x1000+i*2,SCREEN_KEYBOARD_EMPTY,"-",
				-176,y,-128,y+h1,1.0f,0.0f,1.0f,0.0f));
			titleBar.m_AdditionalButtons.push_back(
				SimpleTitleBarButton(0x1001+i*2,SCREEN_KEYBOARD_EMPTY,"+",
				-112,y,-64,y+h1,1.0f,0.0f,1.0f,0.0f));

			y+=h1+16;
		}

		//align
		m_txtPrompt->AddString(mainFont,prompt[4],64,float(y),176,float(h2),
			1.0f,DrawTextFlags::VCenter|DrawTextFlags::AutoSize);
		titleBar.m_AdditionalButtons.push_back(
			SimpleTitleBarButton(0x1011,-1,
			_("Left").c_str(),
			256,y,144,y+h2,0.0f,0.0f,1.0f/3.0f,0.0f));
		titleBar.m_AdditionalButtons.push_back(
			SimpleTitleBarButton(0x1012,-1,
			_("Center").c_str(),
			152,y,40,y+h2,1.0f/3.0f,0.0f,2.0f/3.0f,0.0f));
		titleBar.m_AdditionalButtons.push_back(
			SimpleTitleBarButton(0x1013,-1,
			_("Right").c_str(),
			48,y,-64,y+h2,2.0f/3.0f,0.0f,1.0f,0.0f));

		y+=h2+16;

		m_txtPrompt->AddString(mainFont,prompt[5],64,float(y),176,float(h2),
			1.0f,DrawTextFlags::VCenter|DrawTextFlags::AutoSize);
		titleBar.m_AdditionalButtons.push_back(
			SimpleTitleBarButton(0x1014,-1,
			_("Top").c_str(),
			256,y,144,y+h2,0.0f,0.0f,1.0f/3.0f,0.0f));
		titleBar.m_AdditionalButtons.push_back(
			SimpleTitleBarButton(0x1015,-1,
			_("Vertical Center").c_str(),
			152,y,40,y+h2,1.0f/3.0f,0.0f,2.0f/3.0f,0.0f));
		titleBar.m_AdditionalButtons.push_back(
			SimpleTitleBarButton(0x1016,-1,
			_("Bottom").c_str(),
			48,y,-64,y+h2,2.0f/3.0f,0.0f,1.0f,0.0f));

		y+=h2+16;
	}

	while(m_bRun && b){
		bool bDirty=titleBar.OnTimer();

		for(int i=0;i<4;i++) bDirty|=txt[i].OnTimer();

		for(int i=0;i<4;i++) txt[i].RegisterView(mgr);

		UpdateIdleTime(bDirty);

		if(NeedToDrawScreen()){
			//clear and draw
			ClearScreen();

			SetProjectionMatrix(1);

			glDisable(GL_LIGHTING);

			//title bar
			titleBar.Draw();

			//draw prompt text
			if(m_txtPrompt && !m_txtPrompt->empty()){
				mainFont->DrawString(*m_txtPrompt,SDL_MakeColor(255,255,255,255));
			}

			//draw textbox
			for(int i=0;i<4;i++) txt[i].Draw();

			//draw overlay
			for(int i=0;i<4;i++) txt[i].DrawOverlay();

			//over
			ShowScreen();
		}

		while(SDL_PollEvent(&event)){
			if(mgr.OnEvent()) continue;
			{
				bool bProcessed=false;
				for(int i=0;i<4;i++){
					if(txt[i].OnEvent()){
						bProcessed=true;
						break;
					}
				}
				if(bProcessed) continue;
			}

			if(event.type==SDL_MOUSEBUTTONDOWN){
				SimpleTextBox::ClearFocus(); //???
			}

			int titleBarResult=titleBar.OnEvent();
			if(titleBarResult>=-1){
				m_nIdleTime=0;
				switch(titleBarResult){
				case SCREEN_KEYBOARD_LEFT:
					b=false;
					break;
				case SCREEN_KEYBOARD_YES:
					b=false;
					ret=true;
					break;
				case 0x1000: case 0x1001: case 0x1002: case 0x1003:
				case 0x1004: case 0x1005: case 0x1006: case 0x1007:
					{
						u8string s;
						int n;

						txt[(titleBarResult&0xF)>>1].GetText(s);
						if(sscanf(s.c_str(),"%d",&n)==1){
							if(titleBarResult&1) n++;
							else n--;

							if(n<1 && titleBarResult<0x1004) n=1;
							else if(n<-255) n=-255;
							else if(n>255) n=255;

							txt[(titleBarResult&0xF)>>1].SetText(str(MyFormat("%d")<<n));
						}
					}
					break;
				case 0x1010:
					m_bPreserve=!m_bPreserve;
					titleBar.m_AdditionalButtons[4].m_sCaption=
						_("Preserve Level Contents")+": "+yesno[m_bPreserve?1:0];
					titleBar.Create();
					m_nIdleTime=0;
					break;
				case 0x1011: case 0x1012: case 0x1013:
					{
						u8string s;
						int n;

						txt[0].GetText(s);
						if(sscanf(s.c_str(),"%d",&n)!=1) break;
						if(n<1) n=1; else if(n>255) n=255;
						m_nWidth=n;

						m_nXOffset=(titleBarResult==0x1011)?0:
							(titleBarResult==0x1012)?(m_nWidth-m_nOldWidth)/2:
							(m_nWidth-m_nOldWidth);
						txt[2].SetText(str(MyFormat("%d")<<m_nXOffset));
					}
					break;
				case 0x1014: case 0x1015: case 0x1016:
					{
						u8string s;
						int n;

						txt[1].GetText(s);
						if(sscanf(s.c_str(),"%d",&n)!=1) break;
						if(n<1) n=1; else if(n>255) n=255;
						m_nHeight=n;

						m_nYOffset=(titleBarResult==0x1014)?0:
							(titleBarResult==0x1015)?(m_nHeight-m_nOldHeight)/2:
							(m_nHeight-m_nOldHeight);
						txt[3].SetText(str(MyFormat("%d")<<m_nYOffset));
					}
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

	if(ret){
		u8string s;
		int n;

		txt[0].GetText(s);
		if(sscanf(s.c_str(),"%d",&n)!=1) ret=false;
		if(n<1) n=1; else if(n>255) n=255;
		m_nWidth=n;

		txt[1].GetText(s);
		if(sscanf(s.c_str(),"%d",&n)!=1) ret=false;
		if(n<1) n=1; else if(n>255) n=255;
		m_nHeight=n;

		txt[2].GetText(s);
		if(sscanf(s.c_str(),"%d",&n)!=1) ret=false;
		if(n<-255) n=-255; else if(n>255) n=255;
		m_nXOffset=n;

		txt[3].GetText(s);
		if(sscanf(s.c_str(),"%d",&n)!=1) ret=false;
		if(n<-255) n=-255; else if(n>255) n=255;
		m_nYOffset=n;
	}

	delete m_txtPrompt;

	m_nIdleTime=0;

	return ret;
}
