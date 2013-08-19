#include "main.h"
#include "FileSystem.h"
#include "PuzzleBoyApp.h"
#include "PuzzleBoyLevelFile.h"
#include "PuzzleBoyLevel.h"
#include "VertexList.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "include_sdl.h"
#include "include_gl.h"

using namespace std;

PuzzleBoyApp *theApp=NULL;
SDL_Window *mainWindow=NULL;
SDL_GLContext glContext=NULL;
int screenWidth=0;
int screenHeight=0;

SDL_Event event;

#define DATA_PATH "data"

//FIXME: ad-hoc zoom
float adhoc_zoom=48.0f;

void SetProjectionMatrix(int idx=0){
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	switch(idx){
	case 1:
		glOrtho(0.0f,float(screenWidth),float(screenHeight),0.0f,-1.0f,1.0f);
		break;
	default:
		glOrtho(0.0f,float(screenWidth)/adhoc_zoom,float(screenHeight)/adhoc_zoom,0.0f,-1.0f,1.0f);
		break;
	}

	glMatrixMode(GL_MODELVIEW);
}

void initGL(){
	glViewport(0,0,screenWidth,screenHeight);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	
	const GLfloat pos[4] = {-1.5f, -2.0f, 2.5f, 0.0f};
	//const GLfloat ambient[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	//const GLfloat diffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	//glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	//glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

	glShadeModel(GL_SMOOTH);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void clearScreen(){
	glClear(GL_COLOR_BUFFER_BIT);

	/*glDisable(GL_TEXTURE_2D);
	//glActiveTexture(GL_TEXTURE0);
	//glClientActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, your_texture_id);
	//glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_BLEND);
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnableClientState(GL_COLOR_ARRAY);*/
}

void reshape(int width, int height){
	screenWidth=width;
	screenHeight=height;

	glViewport(0,0,screenWidth,screenHeight);
}

void OnMouseDown(int which,int button,int x,int y){
	int idx=0;

	if(y<screenHeight){
		int i=2-((screenHeight-y-1)>>6);
		if(i>0) idx=i<<4;
	}

	if(x<screenWidth){
		int i=4-((screenWidth-x-1)>>6);
		if(i>0) idx|=i;
	}

	switch(idx){
	case 0x11: theApp->OnKeyDown(SDLK_u,0); break;
	case 0x12: theApp->OnKeyDown(SDLK_UP,0); break;
	case 0x13: theApp->OnKeyDown(SDLK_r,0); break;
	case 0x14: theApp->OnKeyDown(SDLK_r,KMOD_CTRL); break;
	case 0x21: theApp->OnKeyDown(SDLK_LEFT,0); break;
	case 0x22: theApp->OnKeyDown(SDLK_DOWN,0); break;
	case 0x23: theApp->OnKeyDown(SDLK_RIGHT,0); break;
	case 0x24: theApp->OnKeyDown(SDLK_SPACE,0); break;
	}
}

void OnMouseUp(int which,int button,int x,int y){
}

int main(int argc,char** argv){
	theApp=new PuzzleBoyApp;

	theApp->m_objGetText.LoadFileWithAutoLocale(DATA_PATH "/locale/*.mo");

	theApp->m_bShowGrid=true;
	theApp->m_bShowLines=true;

	//TEST
	//enum files and choose
	u8string fn=DATA_PATH "/levels/";
	//FIXME: ad-hoc
#ifdef ANDROID
	fn.append("PuzzleBoy.lev");
#else
	{
		vector<u8string> fs=enumAllFiles(fn,"lev");

		for(unsigned int i=0;i<fs.size();i++) cout<<i<<" "<<fs[i]<<endl;

		//cout<<"? ";
		int idx=0;
		//cin>>idx;

		fn.append(fs[idx]);
	}
#endif

	//load file and enum levels
	if(theApp->LoadFile(fn)){
		cout<<toUTF8(theApp->m_pDocument->m_sLevelPackName)<<endl;
		for(int i=0;i<(int)theApp->m_pDocument->m_objLevels.size();i++){
			cout<<i<<" "<<toUTF8(theApp->m_pDocument->m_objLevels[i]->m_sLevelName)<<endl;
		}
	}else{
		cout<<"Failed to load file"<<endl;
	}

	//choose level
	//cout<<"? ";
	int idx=0;
	//cin>>idx;

	//init level and graphics
	theApp->m_nCurrentLevel=idx;
	theApp->StartGame();

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK)==-1) abort();

#ifdef ANDROID
	//experimental orientation aware
	int windowFlags=SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN;
	{
		SDL_DisplayMode mode;

		if(SDL_GetDesktopDisplayMode(0,&mode)<0) abort();
		screenWidth=mode.w;
		screenHeight=mode.h;
	}
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 6 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
#else
	int windowFlags=SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
	screenWidth=800;
	screenHeight=480;
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
#endif

	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 0 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 0 );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 0 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	//SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
	//SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 4 );

	SDL_GL_SetSwapInterval(1);

	if((mainWindow=SDL_CreateWindow(_("Puzzle Boy").c_str(),
		SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,
		screenWidth,screenHeight,windowFlags))==NULL) abort();

	glContext=SDL_GL_CreateContext(mainWindow);
	if(glContext==NULL) abort();

	//FIXME: ad-hoc
	GLuint adhoc_tex=0;
	{
		SDL_Surface *tmp=SDL_LoadBMP(DATA_PATH "/gfx/adhoc.bmp");

		glEnable(GL_TEXTURE_2D);

		glGenTextures(1,&adhoc_tex);
		glBindTexture(GL_TEXTURE_2D, adhoc_tex);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#ifndef ANDROID
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#endif

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, tmp->pixels);

		glBindTexture(GL_TEXTURE_2D, 0);

		SDL_FreeSurface(tmp);
	}

	initGL();

	bool b=true;
	while(b){
		theApp->OnTimer();

		clearScreen();

		SetProjectionMatrix();
		theApp->Draw();

		//FIXME: ad-hoc
		{
			SetProjectionMatrix(1);

			glDisable(GL_LIGHTING);
			glEnable(GL_TEXTURE_2D);
			//glActiveTexture(GL_TEXTURE0);
			//glClientActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, adhoc_tex);

			float v[8]={
				float(screenWidth-256),float(screenHeight-128),
				float(screenWidth),float(screenHeight-128),
				float(screenWidth),float(screenHeight),
				float(screenWidth-256),float(screenHeight),
			};

			float t[8]={
				0.0f,0.0f,
				1.0f,0.0f,
				1.0f,1.0f,
				0.0f,1.0f,
			};

			unsigned short i[6]={0,1,2,0,2,3};

			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glVertexPointer(2,GL_FLOAT,0,v);
			glTexCoordPointer(2,GL_FLOAT,0,t);
			glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,i);
			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);
		}

		SDL_GL_SwapWindow(mainWindow);

		while(SDL_PollEvent(&event)){
			switch(event.type){
			case SDL_QUIT:
				b=false;
				break;
			case SDL_MOUSEBUTTONDOWN:
				OnMouseDown(event.button.which,event.button.button,event.button.x,event.button.y);
				break;
			case SDL_MOUSEBUTTONUP:
				OnMouseUp(event.button.which,event.button.button,event.button.x,event.button.y);
				break;
			case SDL_FINGERDOWN:
				//???
				break;
			case SDL_FINGERUP:
				//???
				break;
			case SDL_WINDOWEVENT:
				switch(event.window.event){
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					reshape(event.window.data1,event.window.data2);
					break;
				}
				break;
			case SDL_KEYUP:
				//TODO: key up event
				switch(event.key.keysym.sym){
				case SDLK_ESCAPE:
				case SDLK_AC_BACK: //'back' in android
					b=false;
					break;
				}
				break;
			case SDL_KEYDOWN:
				theApp->OnKeyDown(
					event.key.keysym.sym,
					event.key.keysym.mod);
				break;
			}
		}

		//debug
		//SDL_Delay(20);
	}

	glDeleteTextures(1,&adhoc_tex);

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(mainWindow);
	SDL_Quit();

	delete theApp;
	theApp=NULL;

#ifdef ANDROID
	exit(0);
#endif
	return 0;
}

