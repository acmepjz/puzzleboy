#include "SimpleMessageBox.h"
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

SimpleMessageBox::SimpleMessageBox()
:m_nDefaultValue(-1)
,m_nCancelValue(-1)
,m_nValue(-1)
,m_txtPrompt(NULL)
,m_txtButtons(NULL)
,m_x(0)
,m_y(0)
,m_textWidth(0)
,m_textHeight(0)
,m_nPressedValue(-1)
{
}

void SimpleMessageBox::Create(){
	if(m_txtPrompt) m_txtPrompt->clear();
	else m_txtPrompt=new SimpleText;

	if(m_txtButtons) m_txtButtons->clear();
	else m_txtButtons=new SimpleText;

	m_textWidth=0.0f;

	size_t m=m_prompt.size();

	U8STRING_FOR_EACH_CHARACTER_DO_BEGIN(m_prompt,i,m,c,'?');

	//we need to call some internal functions
	if(m_txtPrompt->ww>m_textWidth) m_textWidth=m_txtPrompt->ww;
	m_txtPrompt->AddChar(mainFont,c,0,0,1.0f,DrawTextFlags::Multiline);

	U8STRING_FOR_EACH_CHARACTER_DO_END();

	//get size of text
	if(m_txtPrompt->ww>m_textWidth) m_textWidth=m_txtPrompt->ww;
	m_textHeight=m_txtPrompt->yy+mainFont->GetFontHeight(); //???

	m_txtPrompt->AddChar(mainFont,-1,0,0,1.0f,0);
	m_txtPrompt->AdjustVerticalPosition(mainFont,0,0,0,1.0f,0);

	//create button text and background
	m_v.clear();
	m_idx.clear();
	for(int i=0;i<(int)m_buttons.size();i++){
		m_txtButtons->AddString(mainFont,m_buttons[i],float(i*192+4),0,(176-8),64,1.0f,
			DrawTextFlags::Center|DrawTextFlags::VCenter|DrawTextFlags::AutoSize);
		AddEmptyHorizontalButton(float(i*192),0,float(i*192+176),64,m_v,m_idx);
	}
}

SimpleMessageBox::~SimpleMessageBox(){
	delete m_txtPrompt;
	delete m_txtButtons;
}

/*bool SimpleMessageBox::OnTimer(){
	return false;
}*/

bool SimpleMessageBox::OnEvent(){
	switch(event.type){
	case SDL_MOUSEBUTTONDOWN:
		m_nPressedValue=-1;
		if(event.button.x>=m_x && event.button.y>=m_y && event.button.y<m_y+64){
			int x=event.button.x-m_x;
			int idx=x/192;
			x-=idx*192;
			if(idx<(int)m_buttons.size() && x<176) m_nPressedValue=idx;
		}
		break;
	case SDL_MOUSEBUTTONUP:
		if(event.button.x>=m_x && event.button.y>=m_y && event.button.y<m_y+64){
			int x=event.button.x-m_x;
			int idx=x/192;
			x-=idx*192;
			if(idx==m_nPressedValue && x<176){
				m_nValue=idx;
				m_nIdleTime=0;
			}
		}
		m_nPressedValue=-1;
		break;
	case SDL_KEYDOWN:
		switch(event.key.keysym.sym){
		case SDLK_RETURN:
			if(m_nDefaultValue>=0 && m_nDefaultValue<(int)m_buttons.size()){
				m_nValue=m_nDefaultValue;
				m_nIdleTime=0;
				m_bKeyDownProcessed=true;
			}
			break;
		case SDLK_ESCAPE:
		case SDLK_AC_BACK:
			if(m_nCancelValue>=0 && m_nCancelValue<(int)m_buttons.size()){
				m_nValue=m_nCancelValue;
				m_nIdleTime=0;
				m_bKeyDownProcessed=true;
			}
			break;
		}
		break;
	}

	return true;
}

void SimpleMessageBox::Draw(){
	//calc dialog size
	int minWidth=m_buttons.size()*192-16;
	float maxWidth=float(screenWidth-128);
	float maxHeight=float(screenHeight-224);

	float f=1.0f;
	float w=m_textWidth;
	float h=m_textHeight;
	if(w>maxWidth || h>maxHeight){
		if(w*maxHeight>h*maxWidth){ // m_w/m_h>maxWidth/maxHeight
			f=maxWidth/w;
		}else{
			f=maxHeight/h;
		}
		w*=f;
		h*=f;
	}

	if(w<(float)minWidth) w=(float)minWidth;
	w+=64.0f;
	h+=160.0f;
	float x=(float(screenWidth)-w)*0.5f;
	float y=(float(screenHeight)-h)*0.5f;

	m_x=(screenWidth-minWidth)/2;
	m_y=int(floor(y+h+(0.5f-96.0f)));

	//draw background
	SetProjectionMatrix(1);
	{
		float vv[16]={
			x,y,
			x,y+h,
			x+w,y,
			x+w,y+h,
			0,0,
			0,float(screenHeight),
			float(screenWidth),0,
			float(screenWidth),float(screenHeight),
		};

		unsigned short ii[6]={0,1,3,0,3,2};

		glEnableClientState(GL_VERTEX_ARRAY);
		glColor4f(0, 0, 0, 0.5f);
		glVertexPointer(2,GL_FLOAT,0,vv+8);
		glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,ii);
		glColor4f(0.25f, 0.25f, 0.25f, 0.875f);
		glVertexPointer(2,GL_FLOAT,0,vv);
		glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,ii);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	//draw prompt
	if(m_txtPrompt){
		glLoadIdentity();
		glTranslatef(x+32.0f,y+32.0f,0);
		glScalef(f,f,1);
		mainFont->BeginDraw();
		m_txtPrompt->Draw(SDL_MakeColor(255,255,255,255));
		mainFont->EndDraw();
	}

	//draw button
	if(!m_buttons.empty()){
		glLoadIdentity();
		glTranslatef(float(m_x),float(m_y),0);
		if(!m_v.empty()) DrawScreenKeyboard(m_v,m_idx);
		if(m_txtButtons){
			mainFont->BeginDraw();
			m_txtButtons->Draw(SDL_MakeColor(255,255,255,255));
			mainFont->EndDraw();
		}
	}
	
	//over
	glLoadIdentity();
}
