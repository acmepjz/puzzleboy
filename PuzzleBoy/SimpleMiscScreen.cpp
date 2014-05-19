#include "SimpleMiscScreen.h"
#include "SimpleText.h"
#include "PuzzleBoyApp.h"
#include "main.h"

#include <string.h>
#include <math.h>

#include "include_sdl.h"
#include "include_gl.h"

//ad-hoc
extern SDL_Event event;
extern SDL_Window *mainWindow;
extern bool m_bKeyDownProcessed;

bool SimpleInputScreen(const u8string& title,const u8string& prompt,u8string& text){
	bool b=true,ret=false;

	SimpleText *m_txtTitle=new SimpleText,*m_txtPrompt=new SimpleText;
	int m_nMyResizeTime=-1;

	//create text
	m_txtTitle->AddString(titleFont?titleFont:mainFont,title,80.0f,0,0,64,
		titleFont?1.0f:1.5f,DrawTextFlags::VCenter);
	m_txtPrompt->AddString(mainFont,prompt,64,96,0,0,
		1.0f,DrawTextFlags::Multiline);

	//for title bar buttons
	std::vector<float> m_v;
	std::vector<unsigned short> m_idx;

	//break text into Unicode characters
	std::vector<int> newText;
	{
		size_t m=text.size();

		U8STRING_FOR_EACH_CHARACTER_DO_BEGIN(text,i,m,c,'?');

		newText.push_back(c);

		U8STRING_FOR_EACH_CHARACTER_DO_END();
	}

	int caretPos=newText.size();
	int caretTimer=0; //0-31

	char IMEText[SDL_TEXTEDITINGEVENT_TEXT_SIZE]={};
	int IMETextCaret=0;

	//enable text input, once is enough!
	SDL_StartTextInput();

	while(m_bRun && b){
		//create title bar buttons
		if(m_nMyResizeTime!=m_nResizeTime){
			m_nMyResizeTime=m_nResizeTime;

			m_v.clear();
			m_idx.clear();

			const int left=64;
			const int right=screenWidth-64;

			AddScreenKeyboard(0,0,64,64,SCREEN_KEYBOARD_LEFT,m_v,m_idx);

			AddScreenKeyboard(float(right),0,64,64,SCREEN_KEYBOARD_YES,m_v,m_idx);

			float vv[32]={
				float(left),0,0.75f,0.75f,
				float(left+32),0,0.875f,0.75f,
				float(right-32),0,0.875f,0.75f,
				float(right),0,1.0f,0.75f,
				float(left),64,0.75f,1.0f,
				float(left+32),64,0.875f,1.0f,
				float(right-32),64,0.875f,1.0f,
				float(right),64,1.0f,1.0f,
			};

			unsigned short idxs=(unsigned short)(m_v.size()>>2);

			unsigned short ii[18]={
				idxs,idxs+1,idxs+5,idxs,idxs+5,idxs+4,
				idxs+1,idxs+2,idxs+6,idxs+1,idxs+6,idxs+5,
				idxs+2,idxs+3,idxs+7,idxs+2,idxs+7,idxs+6,
			};

			m_v.insert(m_v.end(),vv,vv+32);
			m_idx.insert(m_idx.end(),ii,ii+18);
		}

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

		//draw textbox border
#ifdef ANDROID
		SDL_Rect r={64,200,screenWidth-128,40};
#else
		SDL_Rect r={64,screenHeight/2,screenWidth-128,40};
#endif
		{
			float vv[8]={
				float(r.x),float(r.y),
				float(r.x),float(r.y+r.h),
				float(r.x+r.w),float(r.y+r.h),
				float(r.x+r.w),float(r.y),
			};

			unsigned short ii[8]={
				0,1,1,2,2,3,3,0,
			};

			glColor4f(0.5f, 0.5f, 0.5f, 1.0f);

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2,GL_FLOAT,0,vv);
			glDrawElements(GL_LINES,8,GL_UNSIGNED_SHORT,ii);
			glDisableClientState(GL_VERTEX_ARRAY);
		}

		glEnable(GL_SCISSOR_TEST);
		glScissor(r.x,screenHeight-r.y-r.h,r.w,r.h);
		{
			//TODO: horizontal scrolling
			float caretX=float(r.x+4);
			float imeX1,imeX2;

			//we need to call some internal functions
			SimpleText txt;
			for(int i=0,m=newText.size();;i++){
				if(i==caretPos){
					//get caret pos and draw IME text (if any)
					if(IMEText[0]){
						int ii=0;
						imeX1=float(r.x+4)+txt.xx;

						U8STRING_FOR_EACH_CHARACTER_DO_BEGIN(IMEText,iii,(sizeof(IMEText)),c,'?');

						if(c==0) break;
						if((ii++)==IMETextCaret) caretX+=txt.xx; //< get caret pos
						txt.AddChar(mainFont,c,0,0,1.0f,0);

						U8STRING_FOR_EACH_CHARACTER_DO_END();

						if(ii==IMETextCaret) caretX+=txt.xx; //< get caret pos (at the end)
						imeX2=float(r.x+4)+txt.xx;
					}else{
						//no IME now, just get current caret pos
						caretX+=txt.xx;
					}
				}

				if(i>=m) break;

				int c=newText[i];
				txt.AddChar(mainFont,c,0,0,1.0f,0);
			}
			txt.AddChar(mainFont,-1,float(r.x+4),0,1.0f,0);
			txt.AdjustVerticalPosition(mainFont,0,float(r.y),float(r.h),1.0f,DrawTextFlags::VCenter);

			//draw IME area
			if(IMEText[0]){
				float vv[8]={
					imeX1,float(r.y+4),
					imeX1,float(r.y+r.h-4),
					imeX2,float(r.y+4),
					imeX2,float(r.y+r.h-4),
				};

				unsigned short ii[6]={0,1,3,0,3,2};

				glColor4f(0.25f, 0.25f, 0.25f, 1.0f);

				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(2,GL_FLOAT,0,vv);
				glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,ii);
				glDisableClientState(GL_VERTEX_ARRAY);
			}

			//draw text
			mainFont->BeginDraw();
			txt.Draw(SDL_MakeColor(255,255,255,255));
			mainFont->EndDraw();

			//update caret animation
			caretTimer=(caretTimer+1)&0x1F;
			int opacity;
			if(caretTimer & 0x10){
				opacity=caretTimer-27;
				if(opacity<0) opacity=0;
			}else{
				opacity=16-caretTimer;
				if(opacity>5) opacity=5;
			}

			//draw caret
			if(opacity>0){
				float vv[8]={
					caretX-1.0f,float(r.y+4),
					caretX-1.0f,float(r.y+r.h-4),
					caretX+1.0f,float(r.y+4),
					caretX+1.0f,float(r.y+r.h-4),
				};

				unsigned short ii[6]={0,1,3,0,3,2};

				glColor4f(1.0f, 1.0f, 1.0f, float(opacity)*0.2f);

				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(2,GL_FLOAT,0,vv);
				glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,ii);
				glDisableClientState(GL_VERTEX_ARRAY);
			}
		}
		glDisable(GL_SCISSOR_TEST);

		//over
		SDL_GL_SwapWindow(mainWindow);

		while(SDL_PollEvent(&event)){
			switch(event.type){
			case SDL_QUIT:
				m_bRun=false;
				break;
			case SDL_MOUSEMOTION:
				//TODO:
				break;
			case SDL_MOUSEBUTTONDOWN:
				//check if we should re-enable text input
				if(event.button.x>=r.x && event.button.x<r.x+r.w
					&& event.button.y>=r.y && event.button.y<r.y+r.h)
				{
					SDL_StartTextInput();
				}
				//TODO: move caret
				break;
			case SDL_MOUSEBUTTONUP:
				if(event.button.y<64){
					//check clicked title bar buttons
					if(event.button.x>=0 && event.button.x<64){
						b=false;
					}else if(event.button.x>=screenWidth-64 && event.button.x<screenWidth){
						b=false;
						ret=true;
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
					m_bKeyDownProcessed=true;
					b=false;
					break;
				case SDLK_ESCAPE:
					if(IMEText[0]==0){
						m_bKeyDownProcessed=true;
						b=false;
					}
					break;
				case SDLK_LEFT:
					if(IMEText[0]==0 && caretPos>0){
						caretPos--;
						caretTimer=0;
					}
					break;
				case SDLK_RIGHT:
					if(IMEText[0]==0 && caretPos<(int)newText.size()){
						caretPos++;
						caretTimer=0;
					}
					break;
				case SDLK_HOME:
					if(IMEText[0]==0){
						caretPos=0;
						caretTimer=0;
					}
					break;
				case SDLK_END:
					if(IMEText[0]==0){
						caretPos=newText.size();
						caretTimer=0;
					}
					break;
				case SDLK_BACKSPACE:
					if(IMEText[0]==0 && caretPos>0){
						newText.erase(newText.begin()+(--caretPos));
						caretTimer=0;
					}
					break;
				case SDLK_DELETE:
					if(IMEText[0]==0 && caretPos<(int)newText.size()){
						newText.erase(newText.begin()+caretPos);
						caretTimer=0;
					}
					break;
				case SDLK_RETURN:
					if(IMEText[0]==0){
						m_bKeyDownProcessed=true;
						b=false;
						ret=true;
					}
					break;
				case SDLK_v:
					if(IMEText[0]==0 && (event.key.keysym.mod & KMOD_CTRL)!=0){
						//try clipboard
						char* s=SDL_GetClipboardText();
						if(s){
							if(caretPos<0) caretPos=0;
							else if(caretPos>(int)newText.size()) caretPos=newText.size();

							size_t m=strlen(s)+1;

							U8STRING_FOR_EACH_CHARACTER_DO_BEGIN(s,i,m,c,'?');

							if(c==0) break;
							newText.insert(newText.begin()+(caretPos++),c);
							caretTimer=0;

							U8STRING_FOR_EACH_CHARACTER_DO_END();

							SDL_free(s);
						}
					}
					break;
				}
				break;
			case SDL_TEXTEDITING:
				memcpy(IMEText,event.edit.text,sizeof(IMEText));
				IMETextCaret=event.edit.start;
				break;
			case SDL_TEXTINPUT:
				{
					if(caretPos<0) caretPos=0;
					else if(caretPos>(int)newText.size()) caretPos=newText.size();

					U8STRING_FOR_EACH_CHARACTER_DO_BEGIN(event.text.text,i,(sizeof(event.text.text)),c,'?');

					if(c==0) break;
					newText.insert(newText.begin()+(caretPos++),c);
					caretTimer=0;

					U8STRING_FOR_EACH_CHARACTER_DO_END();
				}
				break;
			}
		}

		WaitForNextFrame();
	}

	SDL_StopTextInput();

	if(ret){
		text.clear();

		for(int i=0,m=newText.size();i<m;i++){
			int c=newText[i];
			U8_ENCODE(c,text.push_back);
		}
	}

	delete m_txtTitle;
	delete m_txtPrompt;

	return ret;
}

int SimpleConfigKeyScreen(int key){
	bool b=true;

	SimpleText *m_txtTitle=new SimpleText,*m_txtPrompt=new SimpleText;
	int m_nMyResizeTime=-1;

	//create text
	m_txtTitle->AddString(titleFont?titleFont:mainFont,_("Config Key"),80.0f,0,0,64,
		titleFont?1.0f:1.5f,DrawTextFlags::VCenter);
	m_txtPrompt->AddString(mainFont,_("Press any key..."),64,96,0,0,
		1.0f,DrawTextFlags::Multiline);

	//for title bar buttons
	std::vector<float> m_v;
	std::vector<unsigned short> m_idx;

	while(m_bRun && b){
		//get position of text box
		SDL_Rect r={64,screenHeight/2,screenWidth-192,64};

		//create title bar buttons
		if(m_nMyResizeTime!=m_nResizeTime){
			m_nMyResizeTime=m_nResizeTime;

			m_v.clear();
			m_idx.clear();

			const int left=64;
			const int right=screenWidth-64;

			AddScreenKeyboard(0,0,64,64,SCREEN_KEYBOARD_LEFT,m_v,m_idx);

			AddScreenKeyboard(float(right),0,64,64,SCREEN_KEYBOARD_YES,m_v,m_idx);

			AddScreenKeyboard(float(r.x+r.w),float(r.y),64,64,SCREEN_KEYBOARD_NO,m_v,m_idx);

			float vv[32]={
				float(left),0,0.75f,0.75f,
				float(left+32),0,0.875f,0.75f,
				float(right-32),0,0.875f,0.75f,
				float(right),0,1.0f,0.75f,
				float(left),64,0.75f,1.0f,
				float(left+32),64,0.875f,1.0f,
				float(right-32),64,0.875f,1.0f,
				float(right),64,1.0f,1.0f,
			};

			unsigned short idxs=(unsigned short)(m_v.size()>>2);

			unsigned short ii[18]={
				idxs,idxs+1,idxs+5,idxs,idxs+5,idxs+4,
				idxs+1,idxs+2,idxs+6,idxs+1,idxs+6,idxs+5,
				idxs+2,idxs+3,idxs+7,idxs+2,idxs+7,idxs+6,
			};

			m_v.insert(m_v.end(),vv,vv+32);
			m_idx.insert(m_idx.end(),ii,ii+18);
		}

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
		SDL_GL_SwapWindow(mainWindow);

		while(SDL_PollEvent(&event)){
			switch(event.type){
			case SDL_QUIT:
				m_bRun=false;
				break;
			case SDL_MOUSEMOTION:
				//TODO:
				break;
			case SDL_MOUSEBUTTONDOWN:
				//TODO:
				break;
			case SDL_MOUSEBUTTONUP:
				if(event.button.y<64){
					//check clicked title bar buttons
					if(event.button.x>=0 && event.button.x<64){
						b=false;
						key=-1;
					}else if(event.button.x>=screenWidth-64 && event.button.x<screenWidth){
						b=false;
					}
				}else if(event.button.x>=r.x+r.w && event.button.x<r.x+r.w+64
					&& event.button.y>=r.y && event.button.y<r.y+r.h)
				{
					key=0;
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
