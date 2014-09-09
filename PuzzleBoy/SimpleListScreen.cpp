#include "SimpleListScreen.h"
#include "SimpleText.h"
#include "SimpleMessageBox.h"
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
:m_txtList(NULL)
,m_nListCount(0)
,m_nListIndex(-1)
,m_nReturnValue(0)
,m_bDirty(true)
,m_bDirtyOnResize(false)
,m_msgBox(NULL)
,m_y0(0)
{
}

SimpleListScreen::~SimpleListScreen(){
	delete m_txtList;
	delete m_msgBox;
}

void SimpleListScreen::OnDirty(){
}

int SimpleListScreen::OnClick(int index){
	return m_nReturnValue;
}

int SimpleListScreen::OnTitleBarButtonClick(int index){
	return m_nReturnValue;
}

int SimpleListScreen::OnMsgBoxClick(int index){
	if(m_msgBox){
		delete m_msgBox;
		m_msgBox=NULL;
	}
	return -1;
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

const unsigned short i013032[]={0,1,3,0,3,2};

int SimpleListScreen::DoModal(){
	bool b=true;

	//???
	if(m_titleBar.m_LeftButtons.empty()){
		m_titleBar.m_LeftButtons.push_back(SCREEN_KEYBOARD_LEFT);
	}

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

	while(m_bRun && b){
		//update list
		if(m_bDirty){
			OnDirty();
			m_bDirty=false;
			m_nIdleTime=0;
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
			m_nIdleTime=0;
			y02=(m_y0+y02)>>1;
		}else{
			if(scrollBarIdleTime<32){
				scrollBarIdleTime++;
				m_nIdleTime=0;
			}
			y02=m_y0;
		}

		if(selected0>=0){
			selected2=selected0;
			if(selectedTime<8){
				selectedTime++;
				m_nIdleTime=0;
			}
		}else{
			if(selectedTime>0){
				selectedTime--;
				m_nIdleTime=0;
			}
		}

		//update title bar
		if(m_titleBar.OnTimer()) m_nIdleTime=0;

		//update idle time
		UpdateIdleTime(false);

		//clear and draw (if not idle, otherwise only draw after 32 frames)
		if(NeedToDrawScreen()){
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
				glTranslatef(0.0f,float(theApp->m_nButtonSize-y02),0.0f);
				mainFont->DrawString(*m_txtList,SDL_MakeColor(255,255,255,255),
					y02/theApp->m_nMenuHeight,screenHeight/theApp->m_nMenuHeight+1);
				glLoadIdentity();
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

			//draw title bar
			m_titleBar.Draw();

			//draw overlay
			if(m_msgBox) m_msgBox->Draw();

			ShowScreen();
		}

		while(SDL_PollEvent(&event)){
			if(m_msgBox){
				m_msgBox->OnEvent();
				if(m_msgBox->m_nValue>=0){
					int idx=OnMsgBoxClick(m_msgBox->m_nValue);
					if(idx>=0){
						if(m_msgBox){
							delete m_msgBox;
							m_msgBox=NULL;
						}
						m_nReturnValue=idx;
						b=false;
					}
				}
				continue;
			}

			int titleBarResult=m_titleBar.OnEvent();
			if(titleBarResult>=-1){
				if(titleBarResult>=0){
					int idx=OnTitleBarButtonClick(titleBarResult);
					if(idx>=0){
						m_nReturnValue=idx;
						b=false;
					}
				}
				continue;
			}

			switch(event.type){
			case SDL_MOUSEMOTION:
				//experimental dragging support
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
				if(m_nDraggingState<2 && event.button.y>=theApp->m_nButtonSize){
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

				//experimental dragging support
				if(m_nDraggingState && SDL_GetMouseState(NULL,NULL)==0){
					if(SDL_GetTicks()-lastSwipeTime>50) dy=0.0f;
					m_nDraggingState=0;
				}

				selected0=-1;

				break;
			case SDL_MOUSEWHEEL:
				if(event.wheel.y){
					m_nIdleTime=0;
					if(event.wheel.y>0) m_y0-=theApp->m_nMenuHeight*4;
					else m_y0+=theApp->m_nMenuHeight*4;
				}
				break;
			case SDL_KEYDOWN:
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
			if(m_bDirtyOnResize) m_bDirty=true;
			m_nIdleTime=0;
		}

		WaitForNextFrame();
	}

	m_nIdleTime=0;

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
