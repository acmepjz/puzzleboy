#include "SimpleProgressScreen.h"
#include "main.h"

#include <stdlib.h>
#include <string.h>
#include <SDL.h>

#include "include_gl.h"

//ad-hoc
extern SDL_Event event;
extern SDL_Window *mainWindow;
extern bool m_bKeyDownProcessed;

SimpleProgressScreen::SimpleProgressScreen()
:m_Tex(0)
,m_LastTime(0)
,m_x(0)
,progress(0.0f)
{
}

SimpleProgressScreen::~SimpleProgressScreen(){
	Destroy();
}

void SimpleProgressScreen::Create(){
	Destroy();

	glGenTextures(1,&m_Tex);
	glBindTexture(GL_TEXTURE_2D, m_Tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifndef ANDROID
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#endif

	//generate texture
	const int nWidth=32;
	unsigned char* pixels=(unsigned char*)malloc(nWidth*nWidth);

	for(int j=0;j<nWidth;j++){
		for(int i=0;i<nWidth;i++){
			pixels[j*nWidth+i]=((i+j)&(nWidth/2))?64:0;
		}
	}

	//fill texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, nWidth, nWidth, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);

	free(pixels);

	glBindTexture(GL_TEXTURE_2D, 0);

	//reset timer
	m_LastTime=0;
	m_x=0;
	progress=0.0f;
}

void SimpleProgressScreen::Destroy(){
	if(m_Tex){
		glDeleteTextures(1,&m_Tex);
		m_Tex=0;
	}
}

void SimpleProgressScreen::ResetTimer(){
	m_LastTime=0;
	m_x=0;
	progress=0.0f;
}

bool SimpleProgressScreen::DrawAndDoEvents(){
	//check if we should redraw
	unsigned int t=SDL_GetTicks();
	int dx=t-m_LastTime;
	if(m_LastTime>0 && dx<30) return true;

	//update time
	if(m_LastTime>0) m_x+=dx;
	m_x&=(512-1);
	m_LastTime=t;

	//clear and draw
	ClearScreen();

	SetProjectionMatrix(1);

	glDisable(GL_LIGHTING);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	//draw border
	float f1=float(screenWidth)*0.5f;
	float f2=float(screenWidth)/32.0f;

	float v[8]={
		float(screenWidth)*0.125f,float(screenHeight)*0.46875f,
		float(screenWidth)*0.875f,float(screenHeight)*0.46875f,
		float(screenWidth)*0.125f,float(screenHeight)*0.53125f,
		float(screenWidth)*0.875f,float(screenHeight)*0.53125f,
	};

	const unsigned short i0[]={0,1,1,3,3,2,2,0};
	const unsigned short idx[]={0,1,3,0,3,2};

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2,GL_FLOAT,0,v);
	glDrawElements(GL_LINES,8,GL_UNSIGNED_SHORT,i0);
	glDisableClientState(GL_VERTEX_ARRAY);

	//draw moving strips
	if(m_Tex){
		float f3=screenAspectRatio*12.0f;
		float f4=1.0f-m_x/512.0f;

		float tc[8]={
			f4,0.0f,
			f4+f3,0.0f,
			f4,1.0f,
			f4+f3,1.0f,
		};

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_Tex);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(2,GL_FLOAT,0,v);
		glTexCoordPointer(2,GL_FLOAT,0,tc);

		glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,idx);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	//draw progress
	v[2]=float(screenWidth)*(0.125f+0.75f*progress);
	v[6]=v[2];

	glColor4f(0.5f, 0.5f, 0.5f, 0.5f);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2,GL_FLOAT,0,v);
	glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,idx);
	glDisableClientState(GL_VERTEX_ARRAY);

	//over
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	SDL_GL_SwapWindow(mainWindow);

	//check events
	bool b=true;
	while(SDL_PollEvent(&event)){
		switch(event.type){
		case SDL_QUIT:
			m_bRun=false;
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
				b=false;
				break;
			}
			break;
		}
	}

	//???
	m_bKeyDownProcessed=true;

	return m_bRun && b;
}
