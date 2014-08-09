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
,m_nListIndex(-1)
,m_nReturnValue(0)
,m_bDirty(true)
,m_bDirtyOnResize(false)
,m_y0(0)
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
	m_nListIndex=-1;
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

void SimpleListScreen::EnsureVisible(int index){
	if(index<0 || index>=m_nListCount) return;

	if(m_y0>index*theApp->m_nMenuHeight
		|| m_y0<(index+1)*theApp->m_nMenuHeight-screenHeight+theApp->m_nButtonSize)
	{
		m_y0=index*theApp->m_nMenuHeight-(screenHeight-theApp->m_nButtonSize-theApp->m_nMenuHeight)/2;
	}
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

	AddEmptyHorizontalButton(float(left),0,float(right),float(buttonSize),m_v,m_idx);
}

const unsigned short i013032[]={0,1,3,0,3,2};

int SimpleListScreen::DoModal(){
	bool b=true;

	CreateTitleBarButtons();

	m_bDirty=true;
	m_nReturnValue=0;

	int m_nMyResizeTime=m_nResizeTime;

	//0=not dragging or failed,1=mouse down,2=dragging,3=dragging scrollbar or failed
	int m_nDraggingState=0;
	float m_fOldY=0.0f;

	int y02=m_y0,ym=0;
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
			m_y0-=int(floor(dy*float(screenHeight)+0.5f));
		}

		if(m_y0>ym) m_y0=ym;
		if(m_y0<0) m_y0=0;

		if(m_nDraggingState==3 || m_y0<y02-1 || m_y0>y02+1){
			if(scrollBarIdleTime>0) scrollBarIdleTime-=4;
			nIdleTime=0;
			y02=(m_y0+y02)>>1;
		}else{
			if(scrollBarIdleTime<32){
				scrollBarIdleTime++;
				nIdleTime=0;
			}
			y02=m_y0;
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

			//draw list index
			if(m_nListIndex>=0 && m_nListIndex<m_nListCount){
				int y1=theApp->m_nButtonSize-y02+m_nListIndex*theApp->m_nMenuHeight;
				int y2=y1+theApp->m_nMenuHeight;

				float v[]={
					0.0f,float(y1),
					float(screenWidth),float(y1),
					0.0f,float(y2),
					float(screenWidth),float(y2),
				};

				glColor4f(1.0f,1.0f,1.0f,0.25f);
				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(2,GL_FLOAT,0,v);
				glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,i013032);
				glDisableClientState(GL_VERTEX_ARRAY);
			}

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

				float transparency=float(selectedTime)/16.0f;
				if(transparency>0.5f) transparency=0.5f;

				glColor4f(1.0f,1.0f,1.0f,transparency);
				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(2,GL_FLOAT,0,v);
				glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,i013032);
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

				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(2,GL_FLOAT,0,v);
				glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,i013032);
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

								m_y0-=int(floor(dy*float(screenHeight)+0.5f));
							}
						}
						break;
					case 3:
						//dragging scrollbar
						m_y0=int(float(event.motion.y-64)*float(ym+largeChange)/float(largeChange)+0.5f)-largeChange/2;
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
						m_y0=int(float(event.button.y-theApp->m_nButtonSize)*float(ym+largeChange)/float(largeChange)+0.5f)-largeChange/2;
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
					if(event.wheel.y>0) m_y0-=theApp->m_nMenuHeight*4;
					else m_y0+=theApp->m_nMenuHeight*4;
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
					m_y0-=theApp->m_nMenuHeight;
					break;
				case SDLK_DOWN:
					m_y0+=theApp->m_nMenuHeight;
					break;
				case SDLK_HOME:
					m_y0=0;
					break;
				case SDLK_END:
					m_y0=ym;
					break;
				case SDLK_PAGEUP:
					m_y0-=largeChange;
					break;
				case SDLK_PAGEDOWN:
					m_y0+=largeChange;
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
	int tmp=m_nListIndex;
	ResetList();
	m_nListIndex=tmp;

	for(int i=0,m=m_sList.size();i<m;i++){
		if(m_sList[i].empty()) AddEmptyItem();
		else AddItem(m_sList[i]);
	}

	EnsureVisible();
}

int SimpleStaticListScreen::OnClick(int index){
	return index+1;
}

int SimpleStaticListScreen::DoModal(){
	m_LeftButtons.push_back(SCREEN_KEYBOARD_LEFT);
	CreateTitleBarText(m_sTitle);
	return SimpleListScreen::DoModal();
}
