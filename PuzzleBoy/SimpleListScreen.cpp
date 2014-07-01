#include "SimpleListScreen.h"
#include "SimpleText.h"
#include "PuzzleBoyApp.h"
#include "main.h"

#include <string.h>
#include <math.h>

#include "include_sdl.h"
#include "include_gl.h"

//ad-hoc
extern SDL_Event event;
extern bool m_bKeyDownProcessed;

SimpleListScreen::SimpleListScreen()
:m_txtTitle(NULL)
,m_txtList(NULL)
,m_nListCount(0)
,m_nReturnValue(0)
,m_bDirty(true)
,m_bDirtyOnResize(false)
,m_nMyResizeTime(m_nResizeTime)
,m_nDraggingState(0)
,m_fOldY(0.0f)
{
}

SimpleListScreen::~SimpleListScreen(){
	delete m_txtTitle;
	delete m_txtList;
}

void SimpleListScreen::OnDirty(){
}

int SimpleListScreen::OnClick(int index){
	return m_nReturnValue;
}

int SimpleListScreen::OnTitleBarButtonClick(int index){
	return m_nReturnValue;
}

void SimpleListScreen::ResetList(){
	if(m_txtList) m_txtList->clear();
	else m_txtList=new SimpleText;

	m_nListCount=0;
}

void SimpleListScreen::AddEmptyItem(){
	m_txtList->NewStringIndex();
	m_nListCount++;
}

void SimpleListScreen::AddItem(const u8string& str,bool newIndex,float x,float w,int extraFlags){
	if(newIndex){
		m_txtList->NewStringIndex();
		m_nListCount++;
	}

	m_txtList->AddString(mainFont,str,
		x,float((m_nListCount-1)*theApp->m_nMenuHeight),
		w,float(theApp->m_nMenuHeight),theApp->m_fMenuTextScale,
		DrawTextFlags::VCenter | extraFlags);
}

void SimpleListScreen::CreateTitleBarText(const u8string& title){
	if(m_txtTitle) m_txtTitle->clear();
	else m_txtTitle=new SimpleText;

	m_txtTitle->AddString(titleFont?titleFont:mainFont,title,
		(float(m_LeftButtons.size())+0.25f)*float(theApp->m_nButtonSize),0,0,float(theApp->m_nButtonSize),
		(titleFont?1.0f:1.5f)*float(theApp->m_nButtonSize)/64.0f,DrawTextFlags::VCenter);
}

void SimpleListScreen::CreateTitleBarButtons(){
	m_v.clear();
	m_idx.clear();

	int buttonSize=theApp->m_nButtonSize;

	int left=int(m_LeftButtons.size())*buttonSize;
	int right=screenWidth-int(m_RightButtons.size())*buttonSize;

	for(int i=0,m=m_LeftButtons.size();i<m;i++){
		AddScreenKeyboard(float(i*buttonSize),0,float(buttonSize),float(buttonSize),m_LeftButtons[i],m_v,m_idx);
	}

	for(int i=0,m=m_RightButtons.size();i<m;i++){
		AddScreenKeyboard(float(right+i*buttonSize),0,float(buttonSize),float(buttonSize),m_RightButtons[i],m_v,m_idx);
	}

	float vv[32]={
		float(left),0,0.75f,0.75f,
		float(left+buttonSize/2),0,0.875f,0.75f,
		float(right-buttonSize/2),0,0.875f,0.75f,
		float(right),0,1.0f,0.75f,
		float(left),float(buttonSize),0.75f,1.0f,
		float(left+buttonSize/2),float(buttonSize),0.875f,1.0f,
		float(right-buttonSize/2),float(buttonSize),0.875f,1.0f,
		float(right),float(buttonSize),1.0f,1.0f,
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

int SimpleListScreen::DoModal(){
	bool b=true;

	CreateTitleBarButtons();

	m_bDirty=true;
	m_nDraggingState=0;
	m_nReturnValue=0;

	int y0=0,y02=0,ym=0;
	int scrollBarIdleTime=0;
	int selected0=-1,selected2=-1;
	int selectedTime=0;

	Uint32 lastSwipeTime=SDL_GetTicks();
	float dy=0.0f;

	int nIdleTime=0;

	while(m_bRun && b){
		//update list
		if(m_bDirty){
			OnDirty();
			m_bDirty=false;
			nIdleTime=0;
		}

		//update animation
		int largeChange=screenHeight-theApp->m_nButtonSize;
		ym=m_nListCount*theApp->m_nMenuHeight-largeChange;
		if(ym<0) ym=0;

		if(m_nDraggingState==0){
			dy*=0.95f;
			y0-=int(floor(dy*float(screenHeight)+0.5f));
		}

		if(y0>ym) y0=ym;
		if(y0<0) y0=0;

		if(m_nDraggingState==3 || y0<y02-1 || y0>y02+1){
			if(scrollBarIdleTime>0) scrollBarIdleTime-=4;
			nIdleTime=0;
			y02=(y0+y02)>>1;
		}else{
			if(scrollBarIdleTime<32){
				scrollBarIdleTime++;
				nIdleTime=0;
			}
			y02=y0;
		}

		if(selected0>=0){
			selected2=selected0;
			if(selectedTime<8) selectedTime++;
			nIdleTime=0;
		}else{
			if(selectedTime>0){
				selectedTime--;
				nIdleTime=0;
			}
		}

		//update idle time
		if((++nIdleTime)>=64) nIdleTime=32;

		//clear and draw (if not idle, otherwise only draw after 32 frames)
		if(nIdleTime<=32){
			ClearScreen();

			SetProjectionMatrix(1);

			glDisable(GL_LIGHTING);

			//draw selected
			if(selected2>=0 && selectedTime>0){
				int y1=theApp->m_nButtonSize-y02+selected2*theApp->m_nMenuHeight;
				int y2=y1+theApp->m_nMenuHeight;

				float v[]={
					0.0f,float(y1),
					float(screenWidth),float(y1),
					0.0f,float(y2),
					float(screenWidth),float(y2),
				};

				const unsigned short i[]={0,1,3,0,3,2};

				float transparency=float(selectedTime)/8.0f;
				if(transparency>1.0f) transparency=1.0f;

				glColor4f(0.5f,0.5f,0.5f,transparency);
				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(2,GL_FLOAT,0,v);
				glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,i);
				glDisableClientState(GL_VERTEX_ARRAY);
			}

			//draw list box
			if(m_txtList && !m_txtList->empty()){
				mainFont->BeginDraw();
				glTranslatef(0.0f,float(theApp->m_nButtonSize-y02),0.0f);
				m_txtList->Draw(SDL_MakeColor(255,255,255,255),
					y02/theApp->m_nMenuHeight,screenHeight/theApp->m_nMenuHeight+1);
				glLoadIdentity();
				mainFont->EndDraw();
			}

			//draw scrollbar (experimental)
			if(scrollBarIdleTime<32){
				float ratio=float(largeChange)/float(ym+largeChange);

#if 1
				//new ugly scrollbar
				float transparency=float(32-scrollBarIdleTime)/32.0f;
				if(transparency>0.5f) transparency=0.5f;

				glColor4f(1.0f,1.0f,1.0f,transparency);

				const int x1=screenWidth-64;
				const int x2=screenWidth;
#else
				//old scrollbar
				float transparency=float(32-scrollBarIdleTime)/16.0f;
				if(transparency>1.0f) transparency=1.0f;

				glColor4f(0.5f,0.5f,0.5f,transparency);

				const int x1=screenWidth-16;
				const int x2=screenWidth-8;
#endif

				float v[]={
					float(x1),float(theApp->m_nButtonSize)+float(y02)*ratio,
					float(x2),float(theApp->m_nButtonSize)+float(y02)*ratio,
					float(x1),float(theApp->m_nButtonSize)+float(y02+largeChange)*ratio,
					float(x2),float(theApp->m_nButtonSize)+float(y02+largeChange)*ratio,
				};

				const unsigned short i[]={0,1,3,0,3,2};

				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(2,GL_FLOAT,0,v);
				glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,i);
				glDisableClientState(GL_VERTEX_ARRAY);
			}

			//ad-hoc title bar
			DrawScreenKeyboard(m_v,m_idx);

			//draw title text
			if(m_txtTitle && !m_txtTitle->empty()){
				SimpleBaseFont *fnt=titleFont?titleFont:mainFont;

				fnt->BeginDraw();
				m_txtTitle->Draw(SDL_MakeColor(255,255,255,255));
				fnt->EndDraw();
			}

			ShowScreen(&nIdleTime);
		}

		while(SDL_PollEvent(&event)){
			switch(event.type){
			case SDL_QUIT:
				m_bRun=false;
				break;
			case SDL_MOUSEMOTION:
				//experimental dragging support
				nIdleTime=0;
				if(event.motion.state){
					switch(m_nDraggingState){
					case 1:
					case 2:
						{
							float y=float(event.motion.y)/float(screenHeight);
							dy=y-m_fOldY;
							if(m_nDraggingState==2 || dy*dy>0.003f){
								m_nDraggingState=2;
								m_fOldY=y;
								lastSwipeTime=SDL_GetTicks();

								y0-=int(floor(dy*float(screenHeight)+0.5f));
							}
						}
						break;
					case 3:
						//dragging scrollbar
						y0=int(float(event.motion.y-64)*float(ym+largeChange)/float(largeChange)+0.5f)-largeChange/2;
						break;
					}
				}

				break;
			case SDL_MOUSEBUTTONDOWN:
				//experimental dragging support
				nIdleTime=0;
				if(m_nDraggingState==0){
					if(event.button.x>=screenWidth-64 && event.button.y>=theApp->m_nButtonSize){
						//dragging scrollbar
						m_nDraggingState=3;
						y0=int(float(event.button.y-theApp->m_nButtonSize)*float(ym+largeChange)/float(largeChange)+0.5f)-largeChange/2;
					}else{
						m_nDraggingState=1;
						dy=0.0f;
						m_fOldY=float(event.button.y)/float(screenHeight);
						lastSwipeTime=SDL_GetTicks();
					}
				}

				//update selection (display only)
				if(m_nDraggingState!=3){
					int idx=y02+event.button.y-theApp->m_nButtonSize;
					if(idx>=0){
						idx/=theApp->m_nMenuHeight;
						if(idx<m_nListCount) selected0=idx;
					}
				}

				break;
			case SDL_MOUSEBUTTONUP:
				nIdleTime=0;
				if(m_nDraggingState<2){
					if(event.button.y<theApp->m_nButtonSize){
						//check clicked title bar buttons
						int idx=-1;

						if(event.button.x>=0 && event.button.x<int(m_LeftButtons.size())*theApp->m_nButtonSize){
							idx=m_LeftButtons[event.button.x/theApp->m_nButtonSize];
						}else if(event.button.x>=screenWidth-int(m_RightButtons.size())*theApp->m_nButtonSize && event.button.x<screenWidth){
							idx=m_RightButtons[(event.button.x-screenWidth+int(m_RightButtons.size())*theApp->m_nButtonSize)/theApp->m_nButtonSize];
						}

						if(idx>=0){
							idx=OnTitleBarButtonClick(idx);
							if(idx>=0){
								m_nReturnValue=idx;
								b=false;
							}
						}
					}else{
						//check clicked list box
						int idx=y02+event.button.y-theApp->m_nButtonSize;
						if(idx>=0){
							idx/=theApp->m_nMenuHeight;
							if(idx<m_nListCount){
								idx=OnClick(idx);
								if(idx>=0){
									m_nReturnValue=idx;
									b=false;
								}
							}
						}
					}
				}

				//experimental dragging support
				if(m_nDraggingState && SDL_GetMouseState(NULL,NULL)==0){
					if(SDL_GetTicks()-lastSwipeTime>50) dy=0.0f;
					m_nDraggingState=0;
				}

				selected0=-1;

				break;
			case SDL_MOUSEWHEEL:
				if(event.wheel.y){
					nIdleTime=0;
					if(event.wheel.y>0) y0-=theApp->m_nMenuHeight*4;
					else y0+=theApp->m_nMenuHeight*4;
				}
				break;
			case SDL_WINDOWEVENT:
				switch(event.window.event){
				case SDL_WINDOWEVENT_EXPOSED:
					nIdleTime=0;
					break;
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					nIdleTime=0;
					OnVideoResize(
						event.window.data1,
						event.window.data2);
					break;
				}
				break;
			case SDL_KEYDOWN:
				nIdleTime=0;
				switch(event.key.keysym.sym){
				case SDLK_AC_BACK:
				case SDLK_ESCAPE:
					m_bKeyDownProcessed=true;
					{
						int idx=OnTitleBarButtonClick(SCREEN_KEYBOARD_LEFT);
						if(idx>=0){
							m_nReturnValue=idx;
							b=false;
						}
					}
					break;
				case SDLK_UP:
					y0-=theApp->m_nMenuHeight;
					break;
				case SDLK_DOWN:
					y0+=theApp->m_nMenuHeight;
					break;
				case SDLK_HOME:
					y0=0;
					break;
				case SDLK_END:
					y0=ym;
					break;
				case SDLK_PAGEUP:
					y0-=largeChange;
					break;
				case SDLK_PAGEDOWN:
					y0+=largeChange;
					break;
				}
				break;
			}
		}

		if(m_nMyResizeTime!=m_nResizeTime){
			m_nMyResizeTime=m_nResizeTime;
			CreateTitleBarButtons();
			if(m_bDirtyOnResize) m_bDirty=true;
			nIdleTime=0;
		}

		WaitForNextFrame();
	}

	return m_nReturnValue;
}

void SimpleStaticListScreen::OnDirty(){
	ResetList();

	for(int i=0,m=m_sList.size();i<m;i++){
		if(m_sList[i].empty()) AddEmptyItem();
		else AddItem(m_sList[i]);
	}
}

int SimpleStaticListScreen::OnClick(int index){
	return index+1;
}

int SimpleStaticListScreen::DoModal(){
	m_LeftButtons.push_back(SCREEN_KEYBOARD_LEFT);
	CreateTitleBarText(m_sTitle);
	return SimpleListScreen::DoModal();
}
