#include "SimpleTitleBar.h"
#include "SimpleText.h"
#include "PuzzleBoyApp.h"
#include "main.h"

#include <string.h>
#include <math.h>

#include "include_sdl.h"
#include "include_gl.h"

//ad-hoc
extern SDL_Event event;

SimpleTitleBarButton::SimpleTitleBarButton(int index,int showIndex,const char* caption,
	int left,int top,int right,int bottom,
	float left2,float top2,float right2,float bottom2,bool visible)
	:m_index(index/*<0?showIndex:index*/)
	,m_showIndex(showIndex)
	,m_visible(visible)
{
	if(caption) m_sCaption=caption;

	m_position[0].first=left;
	m_position[0].second=left2;
	m_position[1].first=top;
	m_position[1].second=top2;
	m_position[2].first=right;
	m_position[2].second=right2;
	m_position[3].first=bottom;
	m_position[3].second=bottom2;
}

void SimpleTitleBarButton::Create(std::vector<float>& v,std::vector<unsigned short>& idx,SimpleText*& txt){
	if(!m_visible) return;

	m_x=int(float(screenWidth)*m_position[0].second+0.5f)+m_position[0].first;
	m_y=int(float(screenHeight)*m_position[1].second+0.5f)+m_position[1].first;

	int right=int(float(screenWidth)*m_position[2].second+0.5f)+m_position[2].first;
	int bottom=int(float(screenHeight)*m_position[3].second+0.5f)+m_position[3].first;

	m_w=right-m_x;
	m_h=bottom-m_y;

	if(m_showIndex==-1) AddEmptyHorizontalButton(float(m_x),float(m_y),float(right),float(bottom),v,idx);
	else AddScreenKeyboard(float(m_x),float(m_y),float(m_w),float(m_h),m_showIndex,v,idx);

	if(!m_sCaption.empty()){
		if(txt==NULL) txt=new SimpleText;

		txt->AddString(mainFont,m_sCaption,
			float(m_x)+float(m_h)*0.0625f,float(m_y),
			float(m_w)-float(m_h)*0.125f,float(m_h),
			1.0f,DrawTextFlags::Center|DrawTextFlags::VCenter|DrawTextFlags::AutoSize);
	}
}

SimpleTitleBar::SimpleTitleBar()
:m_txtTitle(NULL)
,m_txtAdditional(NULL)
,m_nMyResizeTime(-1)
,m_pressedKey(-2)
{
}

SimpleTitleBar::~SimpleTitleBar(){
	delete m_txtTitle;
	delete m_txtAdditional;
}

void SimpleTitleBar::Create(){
	//add buttons
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

	//add text
	if(!m_sTitle.empty()){
		if(m_txtTitle) m_txtTitle->clear();
		else m_txtTitle=new SimpleText;

		m_txtTitle->AddString(titleFont?titleFont:mainFont,m_sTitle,
			float(left)+float(theApp->m_nButtonSize)*0.125f,
			0,
			float(right-left)-float(theApp->m_nButtonSize)*0.25f,
			float(theApp->m_nButtonSize),
			(titleFont?1.0f:1.5f)*float(theApp->m_nButtonSize)/64.0f,
			DrawTextFlags::VCenter|DrawTextFlags::AutoSize);
	}

	//add additional text
	if(m_txtAdditional) m_txtAdditional->clear();

	for(int i=0,m=m_AdditionalButtons.size();i<m;i++){
		m_AdditionalButtons[i].Create(m_v,m_idx,m_txtAdditional);
	}
}

bool SimpleTitleBar::OnTimer(){
	//check resize
	if(m_nMyResizeTime!=m_nResizeTime){
		m_nMyResizeTime=m_nResizeTime;
		Create();
		return true;
	}

	return false;
}

int SimpleTitleBar::OnEvent(){
	switch(event.type){
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		{
			int idx=-2;

			if(event.button.x>=0 && event.button.y>=0
				&& event.button.x<screenWidth
				&& event.button.y<theApp->m_nButtonSize)
			{
				idx=-1;

				if(event.button.x>=0 && event.button.x<int(m_LeftButtons.size())*theApp->m_nButtonSize){
					idx=m_LeftButtons[event.button.x/theApp->m_nButtonSize];
				}else if(event.button.x>=screenWidth-int(m_RightButtons.size())*theApp->m_nButtonSize && event.button.x<screenWidth){
					idx=m_RightButtons[(event.button.x-screenWidth+int(m_RightButtons.size())*theApp->m_nButtonSize)/theApp->m_nButtonSize];
				}
			}

			for(int i=0,m=m_AdditionalButtons.size();i<m;i++){
				const SimpleTitleBarButton& btn=m_AdditionalButtons[i];
				if(btn.m_visible){
					if(event.button.x>=btn.m_x && event.button.y>=btn.m_y
						&& event.button.x<btn.m_x+btn.m_w
						&& event.button.y<btn.m_y+btn.m_h)
					{
						idx=btn.m_index;
						break;
					}
				}
			}

			if(event.type==SDL_MOUSEBUTTONDOWN){
				m_pressedKey=idx;
				if(idx>=-1) return -1;
			}else{
				int tmp=m_pressedKey;
				m_pressedKey=-2;
				return (tmp==-2)?-2:((idx==tmp)?idx:-1);
			}
		}
		break;
	case SDL_MOUSEMOTION:
		if(m_pressedKey>=-1) return -1;
		break;
	}

	return -2;
}

void SimpleTitleBar::Draw(){
	//draw title bar
	DrawScreenKeyboard(m_v,m_idx);

	//draw title text
	if(m_txtTitle && !m_txtTitle->empty()){
		SimpleBaseFont *fnt=titleFont?titleFont:mainFont;

		fnt->DrawString(*m_txtTitle,SDL_MakeColor(255,255,255,255));
	}

	//draw additional text
	if(m_txtAdditional && !m_txtAdditional->empty()){
		mainFont->DrawString(*m_txtAdditional,SDL_MakeColor(255,255,255,255));
	}
}
