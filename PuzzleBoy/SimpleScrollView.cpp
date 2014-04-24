#include "SimpleScrollView.h"
#include "main.h"

#include <math.h>

#include "include_gl.h"

SimpleScrollView::SimpleScrollView()
:m_bAutoResize(false),m_fMinZoomPerScreen(1.0f)
,m_zoom(1.0f/48.0f),m_x(0.0f),m_y(0.0f)
,m_zoom2(1.0f/48.0f),m_x2(0.0f),m_y2(0.0f)
,m_nMyResizeTime(-1)
{
	m_virtual.x=0;
	m_virtual.y=0;
	m_virtual.w=0;
	m_virtual.h=0;

	m_screen.x=0;
	m_screen.y=0;
	m_screen.w=0;
	m_screen.h=0;
}

void SimpleScrollView::OnTimer(){
	//check screen resize
	if(m_bAutoResize){
		if(m_nMyResizeTime!=m_nResizeTime){
			m_nMyResizeTime=m_nResizeTime;

			SDL_Rect oldScreen=m_screen;

			m_screen.x=int(float(screenWidth)*m_fAutoResizeScale[0]+0.5f)+m_nAutoResizeOffset[0];
			m_screen.y=int(float(screenHeight)*m_fAutoResizeScale[1]+0.5f)+m_nAutoResizeOffset[1];
			m_screen.w=int(float(screenWidth)*m_fAutoResizeScale[2]+0.5f)+m_nAutoResizeOffset[2]-m_screen.x;
			m_screen.h=int(float(screenHeight)*m_fAutoResizeScale[3]+0.5f)+m_nAutoResizeOffset[3]-m_screen.y;

			if(m_virtual.w>0 && m_virtual.h>0 && m_screen.w>0 && m_screen.h>0 && oldScreen.w>0 && oldScreen.h>0){
				//TODO: update zoom pos
			}

			ConstraintView(true);
		}
	}else{
		m_nMyResizeTime=-1;
	}

	if(m_virtual.w>0 && m_virtual.h>0 && m_screen.w>0 && m_screen.h>0){
		//TODO: update zoom pos
	}

	//fake animation
	m_zoom2=(m_zoom+m_zoom2)*0.5f;
	m_x2=(m_x+m_x2)*0.5f;
	m_y2=(m_y+m_y2)*0.5f;
}

void SimpleScrollView::OnMultiGesture(float fx,float fy,float dx,float dy,float zoom){
	//test only

	//move
	float f=float(screenHeight)*m_zoom;
	m_x-=dx*f;
	m_y-=dy*f;

	//zoom
	float new_zoom=m_zoom/zoom;

	//ad-hoc range check

	//check minimal zoom factor (maximal zoom in)
	f=m_fMinZoomPerScreen/float(m_screen.w>m_screen.h?m_screen.w:m_screen.h);
	if(new_zoom<f) new_zoom=f;

	//check maximal zoom factor (maximal zoom out)
	if(m_virtual.w*m_screen.h>m_virtual.h*m_screen.w){ // w/h>m_screen.w/m_screen.h
		f=float(m_virtual.w)/float(m_screen.w);
	}else{
		f=float(m_virtual.h)/float(m_screen.h);
	}
	if(new_zoom>f) new_zoom=f;

	//apply zoom
	f=float(screenHeight)*(m_zoom-new_zoom);
	m_x+=fx*f;
	m_y+=fy*f;
	m_zoom=new_zoom;

	//move view if necessary
	ConstraintView(false);
}

void SimpleScrollView::ConstraintView(bool zoom){
	float f;

	//reset zoom if necessary
	if(zoom){
		float new_zoom=m_zoom;

		//check minimal zoom factor (maximal zoom in)
		f=m_fMinZoomPerScreen/float(m_screen.w>m_screen.h?m_screen.w:m_screen.h);
		if(new_zoom<f) new_zoom=f;

		//check maximal zoom factor (maximal zoom out)
		if(m_virtual.w*m_screen.h>m_virtual.h*m_screen.w){ // w/h>m_screen.w/m_screen.h
			f=float(m_virtual.w)/float(m_screen.w);
		}else{
			f=float(m_virtual.h)/float(m_screen.h);
		}
		if(new_zoom>f) new_zoom=f;

		float fx=float(m_screen.x)+float(m_screen.w)*0.5f;
		float fy=float(m_screen.y)+float(m_screen.h)*0.5f;
		f=m_zoom-new_zoom;
		m_x+=fx*f;
		m_y+=fy*f;
		m_zoom=new_zoom;
	}

	//move view if necessary
	if(float(m_screen.w)*m_zoom>float(m_virtual.w)){
		m_x=float(m_virtual.x)+float(m_virtual.w)*0.5f-(float(m_screen.x)+float(m_screen.w)*0.5f)*m_zoom;
	}else{
		f=float(m_virtual.x)-float(m_screen.x)*m_zoom;
		if(m_x<f) m_x=f;
		f+=float(m_virtual.w)-float(m_screen.w)*m_zoom;
		if(m_x>f) m_x=f;
	}
	if(float(m_screen.h)*m_zoom>float(m_virtual.h)){
		m_y=float(m_virtual.y)+float(m_virtual.h)*0.5f-(float(m_screen.y)+float(m_screen.h)*0.5f)*m_zoom;
	}else{
		f=float(m_virtual.y)-float(m_screen.y)*m_zoom;
		if(m_y<f) m_y=f;
		f+=float(m_virtual.h)-float(m_screen.h)*m_zoom;
		if(m_y>f) m_y=f;
	}
}

void SimpleScrollView::SetProjectionMatrix(){
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(m_x2,m_x2+float(screenWidth)*m_zoom2,
		m_y2+float(screenHeight)*m_zoom2,m_y2,-1.0f,1.0f);
	glMatrixMode(GL_MODELVIEW);
}

void SimpleScrollView::EnableScissorRect(){
	glEnable(GL_SCISSOR_TEST);
	if(m_screen.w>0 && m_screen.h>0){
		glScissor(m_screen.x,screenHeight-m_screen.y-m_screen.h,m_screen.w,m_screen.h);
	}else{
		glScissor(0,0,0,0);
	}
}

void SimpleScrollView::DisableScissorRect(){
	glDisable(GL_SCISSOR_TEST);
}

void SimpleScrollView::TranslateCoordinate(int x,int y,int& out_x,int& out_y){
	out_x=(int)floor(float(x)*m_zoom2+m_x2);
	out_y=(int)floor(float(y)*m_zoom2+m_y2);
}

void SimpleScrollView::CenterView(int x,int y,int w,int h){
	if(w<0 || h<0){
		x=m_virtual.x;
		y=m_virtual.y;
		w=m_virtual.w;
		h=m_virtual.h;
	}

	if(w<0 || h<0) return;

	if(w==0 && h==0){ //centering only
		m_x=float(x)-float(m_screen.x)*m_zoom;
		m_y=float(y)-float(m_screen.y)*m_zoom;
	}else if(w*m_screen.h>h*m_screen.w){ // w/h>m_screen.w/m_screen.h
		m_zoom=float(w)/float(m_screen.w);
		m_x=float(x)-float(m_screen.x)*m_zoom;
		m_y=float(y)+float(h)*0.5f-(float(m_screen.y)+float(m_screen.h)*0.5f)*m_zoom;
	}else{
		m_zoom=float(h)/float(m_screen.h);
		m_x=float(x)+float(w)*0.5f-(float(m_screen.x)+float(m_screen.w)*0.5f)*m_zoom;
		m_y=float(y)-float(m_screen.y)*m_zoom;
	}

	ConstraintView(true);
}

void SimpleScrollView::EnsureVisible(int x,int y,int w,int h){
	if(float(m_screen.x)*m_zoom2+m_x2>float(x)
		|| float(m_screen.y)*m_zoom2+m_y2>float(y)
		|| float(m_screen.x+m_screen.w)*m_zoom2+m_x2<float(x+w)
		|| float(m_screen.y+m_screen.h)*m_zoom2+m_y2<float(y+h))
	{
		m_x=float(x)+float(w)*0.5f-(float(m_screen.x)+float(m_screen.w)*0.5f)*m_zoom;
		m_y=float(y)+float(h)*0.5f-(float(m_screen.y)+float(m_screen.h)*0.5f)*m_zoom;

		ConstraintView(true);
	}
}
