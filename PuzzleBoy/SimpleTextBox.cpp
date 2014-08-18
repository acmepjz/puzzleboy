#include "SimpleTextBox.h"
#include "SimpleText.h"
#include "PuzzleBoyApp.h"
#include "main.h"

#include <string.h>
#include <math.h>

#include "include_sdl.h"
#include "include_gl.h"

SimpleTextBox* SimpleTextBox::m_objFocus=NULL;
char SimpleTextBox::m_IMEText[SDL_TEXTEDITINGEVENT_TEXT_SIZE]={};
int SimpleTextBox::m_IMETextCaret=0;

//ad-hoc
extern SDL_Event event;

SimpleTextBox::SimpleTextBox()
:m_bLocked(false)
,m_caretPos(0)
,m_caretTimer(0)
,m_caretDirty(true)
{
	//some temporary values
	m_scrollView.m_virtual.w=8;
	m_scrollView.m_virtual.h=8;

	m_scrollView.m_flags=SimpleScrollViewFlags::Horizontal
		|SimpleScrollViewFlags::DrawScrollBar
		|SimpleScrollViewFlags::FlushLeft
		|SimpleScrollViewFlags::FlushTop;
}

SimpleTextBox::~SimpleTextBox(){
	if(HasFocus()) m_objFocus=NULL;
}

void SimpleTextBox::SetText(const u8string& text){
	m_chars.clear();

	size_t m=text.size();

	U8STRING_FOR_EACH_CHARACTER_DO_BEGIN(text,i,m,c,'?');

	if(c!='\r' && (c!='\n' || (m_scrollView.m_flags & SimpleScrollViewFlags::Vertical)!=0)){
		SimpleTextBoxCharacter ch={c,0,0};
		m_chars.push_back(ch);
	}

	U8STRING_FOR_EACH_CHARACTER_DO_END();

	m_caretPos=0;
	m_caretTimer=0;
	m_caretDirty=true;
}

void SimpleTextBox::SetMultiline(bool multiline,bool wrap){
	int flags=m_scrollView.m_flags&~SimpleScrollViewFlags::Both;

	if(multiline){
		if(wrap) flags|=SimpleScrollViewFlags::Vertical;
		else flags|=SimpleScrollViewFlags::Both;
	}else{
		flags|=SimpleScrollViewFlags::Horizontal;
	}

	m_scrollView.m_flags=flags;
	m_caretDirty=true;
}

void SimpleTextBox::SetFocus(){
	m_objFocus=this;
	if(m_bLocked) SDL_StopTextInput();
	else SDL_StartTextInput();
}

void SimpleTextBox::ClearFocus(){
	m_objFocus=NULL;
	SDL_StopTextInput();
}

void SimpleTextBox::GetText(u8string& text) const{
	text.clear();

	for(int i=0,m=m_chars.size();i<m;i++){
		int c=m_chars[i].c;
		U8_ENCODE(c,text.push_back);
	}
}

void SimpleTextBox::CopyToClipboard() const{
	u8string s;
	GetText(s);
	SDL_SetClipboardText(s.c_str());
	theApp->ShowToolTip(_("Copied to clipboard"));
}

void SimpleTextBox::PasteFromClipboard(){
	if(m_bLocked) return;

	m_caretDirty=true;
	char* s=SDL_GetClipboardText();
	if(s){
		if(m_caretPos<0) m_caretPos=0;
		else if(m_caretPos>(int)m_chars.size()) m_caretPos=m_chars.size();

		size_t m=strlen(s)+1;

		U8STRING_FOR_EACH_CHARACTER_DO_BEGIN(s,i,m,c,'?');

		if(c==0) break;
		if(c!='\r' && (c!='\n' || (m_scrollView.m_flags & SimpleScrollViewFlags::Vertical)!=0)){
			SimpleTextBoxCharacter ch={c,0,0};
			m_chars.insert(m_chars.begin()+(m_caretPos++),ch);
			m_caretTimer=0;
		}

		U8STRING_FOR_EACH_CHARACTER_DO_END();

		SDL_free(s);
	}
}

void SimpleTextBox::RegisterView(MultiTouchManager& mgr){
	MultiTouchViewStruct *view=mgr.FindView(this);
	if(view){
		view->left=float(m_scrollView.m_screen.x)/float(screenHeight);
		view->top=float(m_scrollView.m_screen.y)/float(screenHeight);
		view->right=float(m_scrollView.m_screen.x+m_scrollView.m_screen.w)/float(screenHeight);
		view->bottom=float(m_scrollView.m_screen.y+m_scrollView.m_screen.h)/float(screenHeight);
	}else{
		mgr.AddView(float(m_scrollView.m_screen.x)/float(screenHeight),
			float(m_scrollView.m_screen.y)/float(screenHeight),
			float(m_scrollView.m_screen.x+m_scrollView.m_screen.w)/float(screenHeight),
			float(m_scrollView.m_screen.y+m_scrollView.m_screen.h)/float(screenHeight),
			MultiTouchViewFlags::AcceptDragging,this);
	}
}

void SimpleTextBox::OnMultiGesture(float fx,float fy,float dx,float dy,float zoom){
	m_scrollView.OnMultiGesture(fx,fy,dx,dy,1.0f);
}

void SimpleTextBox::OnMouseEvent(int which,int state,int xMouse,int yMouse,int nFlags,int nType){
	//TODO:other

	switch(nType){
	case SDL_MOUSEBUTTONUP:
		if(xMouse>=m_scrollView.m_screen.x
			&& xMouse<m_scrollView.m_screen.x+m_scrollView.m_screen.w
			&& yMouse>=m_scrollView.m_screen.y
			&& yMouse<m_scrollView.m_screen.y+m_scrollView.m_screen.h)
		{
			SetFocus();
			SDL_StartTextInput();

			//move caret
			float fx,fy;
			m_scrollView.TranslateCoordinate(float(xMouse),float(yMouse),fx,fy);
			fx-=4.0f;
			if(fx<1E-6f) fx=1E-6f;

			int i=0,j=m_chars.size()-1,k=0;
			if(m_scrollView.m_flags & SimpleScrollViewFlags::Vertical){
				//multiline
				int yy=(int)floor((fy-4.0f)/mainFont->GetFontHeight());
				if(yy<0) yy=0;
				else if(j>=0 && yy>m_chars[j].row) yy=m_chars[j].row;

				for(;i<=j;){
					k=(i+j)/2;
					float x=m_chars[k].x;
					if(k>=(int)m_chars.size()-1 || yy!=m_chars[k+1].row) x=1E+6f;
					if(yy>m_chars[k].row || (yy==m_chars[k].row && fx>x)){
						i=k+1;
					}else{
						if(k>0){
							x=m_chars[k-1].x;
							if(yy!=m_chars[k].row) x=1E+6f;
						}
						if(k>0 && (yy<m_chars[k-1].row || (yy==m_chars[k-1].row && fx<x))){
							j=k-1;
						}else{
							break;
						}
					}
				}
			}else{
				for(;i<=j;){
					k=(i+j)/2;
					if(fx>m_chars[k].x){
						i=k+1;
					}else if(k>0 && fx<m_chars[k-1].x){
						j=k-1;
					}else{
						break;
					}
				}
			}

			if(k<0){
				m_caretPos=0;
			}else if(k<(int)m_chars.size()){
				float x0=0.0f;
				if(k>0 && m_chars[k].row==m_chars[k-1].row) x0=m_chars[k-1].x;
				m_caretPos=k+((m_chars[k].x-fx<fx-x0)?1:0);
			}else{
				m_caretPos=m_chars.size();
			}

			m_caretTimer=0;
			m_caretDirty=true;
		}
		break;
	}
}

void SimpleTextBox::OnMouseWheel(int which,int x,int y,int dx,int dy,int nFlags){
	if(m_scrollView.m_flags & SimpleScrollViewFlags::Vertical){
		//vertical scroll
		m_scrollView.OnMultiGesture(0,0,0,mainFont->GetFontHeight()*(dy>0?4.0f:-4.0f)/float(screenHeight),1.0f);
	}else{
		//horizontal scroll
		m_scrollView.OnMultiGesture(0,0,mainFont->GetFontAdvance()*(dy>0?4.0f:-4.0f)/float(screenHeight),0,1.0f);
	}
}

bool SimpleTextBox::OnEvent(){
	if(!HasFocus()) return false;

	switch(event.type){
	case SDL_KEYDOWN:
		switch(event.key.keysym.sym){
		case SDLK_AC_BACK:
		case SDLK_ESCAPE:
			if(m_IMEText[0]){ //???
				m_IMEText[0]=0;
				return true;
			}
			break;
		case SDLK_LEFT:
			m_caretDirty=true;
			if(m_IMEText[0]==0 && m_caretPos>0){
				m_caretPos--;
				m_caretTimer=0;
			}
			m_caretDirty=true;
			return true;
			break;
		case SDLK_RIGHT:
			m_caretDirty=true;
			if(m_IMEText[0]==0 && m_caretPos<(int)m_chars.size()){
				m_caretPos++;
				m_caretTimer=0;
			}
			return true;
			break;
		case SDLK_HOME:
			m_caretDirty=true;
			if(m_IMEText[0]==0){
				if((m_scrollView.m_flags & SimpleScrollViewFlags::Vertical)==0 || (event.key.keysym.mod & KMOD_CTRL)!=0){
					m_caretPos=0;
				}else{
					//multiline
					if(m_caretPos>0 && m_caretPos<=(int)m_chars.size()){
						int i=m_caretPos-1;
						for(;i>=0;i--){
							if(m_chars[i].c=='\n') break;
						}
						m_caretPos=i+1;
					}
				}
				m_caretTimer=0;
			}
			return true;
			break;
		case SDLK_END:
			m_caretDirty=true;
			if(m_IMEText[0]==0){
				if((m_scrollView.m_flags & SimpleScrollViewFlags::Vertical)==0 || (event.key.keysym.mod & KMOD_CTRL)!=0){
					m_caretPos=m_chars.size();
				}else{
					//multiline
					if(m_caretPos>=0 && m_caretPos<(int)m_chars.size()){
						int i=m_caretPos,m=m_chars.size();
						for(;i<m;i++){
							if(m_chars[i].c=='\n') break;
						}
						m_caretPos=i;
					}
				}
				m_caretTimer=0;
			}
			return true;
			break;
		case SDLK_BACKSPACE:
			if(m_bLocked) break;
			m_caretDirty=true;
			if(m_IMEText[0]==0 && m_caretPos>0){
				m_chars.erase(m_chars.begin()+(--m_caretPos));
				m_caretTimer=0;
			}
			return true;
			break;
		case SDLK_DELETE:
			if(m_bLocked) break;
			m_caretDirty=true;
			if(m_IMEText[0]==0 && m_caretPos<(int)m_chars.size()){
				m_chars.erase(m_chars.begin()+m_caretPos);
				m_caretTimer=0;
			}
			return true;
			break;
		case SDLK_RETURN:
			m_caretDirty=true;
			if(m_IMEText[0]){ //???
				m_IMEText[0]=0;
				return true;
			}else if(m_scrollView.m_flags & SimpleScrollViewFlags::Vertical){
				if(m_bLocked) break;
				m_caretDirty=true;

				if(m_caretPos<0) m_caretPos=0;
				else if(m_caretPos>(int)m_chars.size()) m_caretPos=m_chars.size();

				SimpleTextBoxCharacter ch={'\n',0,0};
				m_chars.insert(m_chars.begin()+(m_caretPos++),ch);
				m_caretTimer=0;

				return true;
			}
			break;
		case SDLK_c:
			//FIXME: copy all text to clipboard
			if(event.key.keysym.mod & KMOD_CTRL){
				CopyToClipboard();
				return true;
			}
			break;
		case SDLK_v:
			if(m_bLocked) break;
			m_caretDirty=true;
			if(m_IMEText[0]==0 && (event.key.keysym.mod & KMOD_CTRL)!=0){
				PasteFromClipboard();
				return true;
			}
			break;
		}
		break;
	case SDL_TEXTEDITING:
		if(m_bLocked) break;
		m_caretDirty=true;
		memcpy(m_IMEText,event.edit.text,sizeof(m_IMEText));
		m_IMETextCaret=event.edit.start;
		return true;
		break;
	case SDL_TEXTINPUT:
		if(m_bLocked) break;
		m_caretDirty=true;
		{
			if(m_caretPos<0) m_caretPos=0;
			else if(m_caretPos>(int)m_chars.size()) m_caretPos=m_chars.size();

			U8STRING_FOR_EACH_CHARACTER_DO_BEGIN(event.text.text,i,(sizeof(event.text.text)),c,'?');

			if(c==0) break;
			SimpleTextBoxCharacter ch={c,0,0};
			m_chars.insert(m_chars.begin()+(m_caretPos++),ch);
			m_caretTimer=0;

			U8STRING_FOR_EACH_CHARACTER_DO_END();
		}
		return true;
		break;
	}

	return false;
}

bool SimpleTextBox::OnTimer(){
	//update scroll view
	bool ret=m_scrollView.OnTimer();

	//update caret animation
	if(HasFocus()){
		m_caretTimer=(m_caretTimer+1)&0x1F;
		ret=true;
	}else{
		m_caretTimer=0;
	}

	return ret;
}

void SimpleTextBox::Draw(){
	//draw textbox border
	SetProjectionMatrix(1);
	{
		float vv[8]={
			float(m_scrollView.m_screen.x),float(m_scrollView.m_screen.y),
			float(m_scrollView.m_screen.x),float(m_scrollView.m_screen.y+m_scrollView.m_screen.h),
			float(m_scrollView.m_screen.x+m_scrollView.m_screen.w),float(m_scrollView.m_screen.y+m_scrollView.m_screen.h),
			float(m_scrollView.m_screen.x+m_scrollView.m_screen.w),float(m_scrollView.m_screen.y),
		};

		unsigned short ii[8]={
			0,1,1,2,2,3,3,0,
		};

		glColor4f(1.0f, 1.0f, 1.0f, 0.5f);

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2,GL_FLOAT,0,vv);
		glDrawElements(GL_LINES,8,GL_UNSIGNED_SHORT,ii);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	m_scrollView.SetProjectionMatrix();
	m_scrollView.EnableScissorRect();

	//draw text
	float caretX=4.0f,caretY=4.0f;
	float imeX1=-1.0f,imeX2=-1.0f;

	//we need to call some internal functions
	SimpleText txt;
	float xxx=0.0f;
	int row=0;
	for(int i=0,m=m_chars.size();;i++){
		if(i==m_caretPos){
			//get caret pos and draw IME text (if any)
			if(m_IMEText[0] && !m_bLocked && HasFocus()){
				int ii=0;
				imeX1=4.0f+txt.xx;

				U8STRING_FOR_EACH_CHARACTER_DO_BEGIN(m_IMEText,iii,(sizeof(m_IMEText)),c,'?');

				if(c==0) break;
				if((ii++)==m_IMETextCaret) caretX+=txt.xx; //< get caret pos
				txt.AddChar(mainFont,c,4,0,1.0f,0);

				U8STRING_FOR_EACH_CHARACTER_DO_END();

				if(ii==m_IMETextCaret) caretX+=txt.xx; //< get caret pos (at the end)
				imeX2=4.0f+txt.xx;
			}else{
				//no IME now, just get current caret pos
				caretX+=txt.xx;
			}
			caretY+=float(row)*mainFont->GetFontHeight();
		}

		if(i>=m) break;

		int c=m_chars[i].c;

		if(c=='\n' && (m_scrollView.m_flags & SimpleScrollViewFlags::Vertical)!=0){
			if(txt.xx>xxx) xxx=txt.xx;
		}

		txt.AddChar(mainFont,c,4,0,1.0f,
			(m_scrollView.m_flags & SimpleScrollViewFlags::Vertical)!=0?DrawTextFlags::Multiline:0);

		if(c=='\n' && (m_scrollView.m_flags & SimpleScrollViewFlags::Vertical)!=0){
			m_chars[i].x=0;
			m_chars[i].row=(++row);
		}else{
			m_chars[i].x=txt.xx;
			m_chars[i].row=row;

			//ad-hoc text wrap
			if((m_scrollView.m_flags & SimpleScrollViewFlags::Both)==SimpleScrollViewFlags::Vertical
				&& i<m-1 && m_chars[i+1].c!='\n'
				&& txt.xx>float(m_scrollView.m_screen.w-48))
			{
				//calc size of next character
				SimpleFontGlyphMetric metric;
				mainFont->AddChar(m_chars[i+1].c);
				mainFont->GetCharMetric(m_chars[i+1].c,metric);
				if(txt.xx+metric.advanceX>float(m_scrollView.m_screen.w-8)){
					if(txt.xx>xxx) xxx=txt.xx;
					txt.AddChar(mainFont,'\n',4,0,1.0f,DrawTextFlags::Multiline);
					row++;
				}
			}
		}
	}

	//update width of text box
	if(m_scrollView.m_flags & SimpleScrollViewFlags::Vertical){
		if(txt.xx>xxx) xxx=txt.xx;
		m_scrollView.m_virtual.w=int(xxx+8.5f);
		m_scrollView.m_virtual.h=int(float(row+1)*mainFont->GetFontHeight()+8.5f);
	}else{
		m_scrollView.m_virtual.w=int(txt.xx+8.5f);
		m_scrollView.m_virtual.h=8;
	}

	//make sure caret is visible
	if(m_caretDirty){
		if(m_scrollView.m_flags & SimpleScrollViewFlags::Vertical){
			m_scrollView.EnsureVisible(caretX,caretY,1,mainFont->GetFontHeight());
		}else{
			m_scrollView.EnsureVisible(caretX,0);
		}
		m_caretDirty=false;
	}

	txt.AddChar(mainFont,-1,4,0,1.0f,0);
	if(m_scrollView.m_flags & SimpleScrollViewFlags::Vertical){
		txt.AdjustVerticalPosition(mainFont,0,0,0,1.0f,DrawTextFlags::Multiline);
	}else{
		txt.AdjustVerticalPosition(mainFont,0,0,float(m_scrollView.m_screen.h),1.0f,DrawTextFlags::VCenter);
	}

	//draw IME area
	//FIXME: multiline
	if(m_IMEText[0] && HasFocus()){
		float vv[8]={
			imeX1,caretY,
			imeX1,float(m_scrollView.m_screen.h-4),
			imeX2,caretY,
			imeX2,float(m_scrollView.m_screen.h-4),
		};

		if(m_scrollView.m_flags & SimpleScrollViewFlags::Vertical){
			vv[3]=vv[7]=caretY+mainFont->GetFontHeight();
		}

		unsigned short ii[6]={0,1,3,0,3,2};

		glColor4f(1.0f, 1.0f, 1.0f, 0.25f);

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2,GL_FLOAT,0,vv);
		glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,ii);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	//draw text
	mainFont->BeginDraw();
	txt.Draw(SDL_MakeColor(255,255,255,255));
	mainFont->EndDraw();

	//draw caret
	int opacity;
	if(m_caretTimer & 0x10){
		opacity=m_caretTimer-27;
		if(opacity<0) opacity=0;
	}else{
		opacity=16-m_caretTimer;
		if(opacity>5) opacity=5;
	}

	if(opacity>0 && HasFocus()){
		float vv[8]={
			caretX-1.0f,caretY,
			caretX-1.0f,float(m_scrollView.m_screen.h-4),
			caretX+1.0f,caretY,
			caretX+1.0f,float(m_scrollView.m_screen.h-4),
		};

		if(m_scrollView.m_flags & SimpleScrollViewFlags::Vertical){
			vv[3]=vv[7]=caretY+mainFont->GetFontHeight();
		}

		unsigned short ii[6]={0,1,3,0,3,2};

		glColor4f(1.0f, 1.0f, 1.0f, float(opacity)*0.2f);

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2,GL_FLOAT,0,vv);
		glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,ii);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

	//draw scroll bar
	m_scrollView.Draw();

	m_scrollView.DisableScissorRect();
}
