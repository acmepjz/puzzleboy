#include "main.h"
#include "FileSystem.h"
#include "PuzzleBoyApp.h"
#include "PuzzleBoyLevelFile.h"
#include "PuzzleBoyLevel.h"
#include "VertexList.h"
#include "SimpleBitmapFont.h"
#include "MyFormat.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
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

//ad-hoc touchscreen test
#ifdef ANDROID
bool m_bTouchscreen=true;
#else
bool m_bTouchscreen=false;
#endif

bool m_bShowYesNoScreenKeyboard=false;

//FIXME: ad-hoc
GLuint adhoc_tex=0;
float adhoc_zoom=48.0f;

u8string adhoc_debug;

static void SetProjectionMatrix(int idx=0){
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

static void initGL(){
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

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

static void clearScreen(){
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

void TranslateCoordinate(int x,int y,int& out_x,int& out_y){
	out_x=(int)floor(float(x)/adhoc_zoom);
	out_y=(int)floor(float(y)/adhoc_zoom);
}

void AddScreenKeyboard(float x,float y,float w,float h,int index,std::vector<float>& v,std::vector<unsigned short>& idx){
	int m=v.size()>>2;

	float tx=float(index & 0xFF)*0.25f;
	float ty=float(index>>8)*0.25f;

	float vv[16]={
		x,y,tx,ty,
		x+w,y,tx+0.25f,ty,
		x+w,y+h,tx+0.25f,ty+0.25f,
		x,y+h,tx,ty+0.25f,
	};

	unsigned short ii[6]={
		m,m+1,m+2,m,m+2,m+3,
	};

	v.insert(v.end(),vv,vv+16);
	idx.insert(idx.end(),ii,ii+6);
}

void DrawScreenKeyboard(const std::vector<float>& v,const std::vector<unsigned short>& idx){
	if(idx.empty()) return;

	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, adhoc_tex);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2,GL_FLOAT,4*sizeof(float),&(v[0]));
	glTexCoordPointer(2,GL_FLOAT,4*sizeof(float),&(v[2]));
	glDrawElements(GL_TRIANGLES,idx.size(),GL_UNSIGNED_SHORT,&(idx[0]));
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
}

static void OnResize(int width, int height){
	screenWidth=width;
	screenHeight=height;

	glViewport(0,0,screenWidth,screenHeight);
}

//FIXME: ad-hoc
static void OnMouseDown(int which,int button,int x,int y,int nFlags){
	int idx=0;

	if(y<screenHeight){
		int i=1+((screenHeight-y-1)>>6);
		idx=i<<8;
	}

	if(x<screenWidth){
		int i=1+((screenWidth-x-1)>>6);
		idx|=i;
	}

	switch(idx){
	case 0x204: theApp->OnKeyDown(SDLK_u,0); return; break;
	case 0x203: theApp->OnKeyDown(SDLK_UP,0); return; break;
	case 0x202: theApp->OnKeyDown(SDLK_r,0); return; break;
	case 0x201: theApp->OnKeyDown(SDLK_r,KMOD_CTRL); return; break;
	case 0x104: theApp->OnKeyDown(SDLK_LEFT,0); return; break;
	case 0x103: theApp->OnKeyDown(SDLK_DOWN,0); return; break;
	case 0x102: theApp->OnKeyDown(SDLK_RIGHT,0); return; break;
	case 0x101: theApp->OnKeyDown(SDLK_SPACE,0); return; break;
	}

	if(m_bShowYesNoScreenKeyboard){
		switch(idx){
		case 0x107: theApp->OnKeyDown(SDLK_RETURN,0); return; break;
		case 0x106: theApp->OnKeyDown(SDLK_ESCAPE,0); return; break;
		}
	}

	theApp->OnMouseEvent(SDL_BUTTON(button),x,y,SDL_MOUSEBUTTONDOWN);
}

static void OnMouseUp(int which,int button,int x,int y,int nFlags){
	theApp->OnMouseEvent(SDL_BUTTON(button),x,y,SDL_MOUSEBUTTONUP);
}

//FIXME: ad-hoc disable mouse move event on screen keyboard
static void OnMouseMove(int which,int state,int x,int y,int nFlags){
	if(!m_bTouchscreen) theApp->OnMouseEvent(state,x,y,SDL_MOUSEMOTION);
}

static void OnMouseWheel(int which,int x,int y,int dx,int dy,int nFlags){
}

static void OnMultiGesture(int which,int numFingers,float fx,float fy,float pinch,float rotate){
	adhoc_debug=str(MyFormat("OnMultiGesture numFingers=%d\nfx=%f fy=%f\npinch=%f rotate=%f")
		<<numFingers<<fx<<fy<<pinch<<rotate);
}

static bool OnKeyDown(int nChar,int nFlags){
	if(theApp->OnKeyDown(nChar,nFlags)) return true;
	return false;
}

static void OnKeyUp(int nChar,int nFlags){
	theApp->OnKeyUp(nChar,nFlags);
}

int main(int argc,char** argv){
	theApp=new PuzzleBoyApp;

	theApp->m_objGetText.LoadFileWithAutoLocale(DATA_PATH "/locale/*.mo");

	adhoc_debug="DEBUG!!1";
	theApp->m_nAnimationSpeed=1;
	theApp->m_bShowGrid=true;
	theApp->m_bShowLines=true;

	//TEST
	//enum files and choose
	u8string fn=DATA_PATH "/levels/";
	//FIXME: ad-hoc file name
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

	//FIXME: ad-hoc screen keypad
	{
#if 0
		IMG_Init(IMG_INIT_PNG);
		SDL_Surface *tmp=IMG_Load(DATA_PATH "/gfx/adhoc.png");
		IMG_Quit();
#else
		SDL_Surface *tmp=SDL_LoadBMP(DATA_PATH "/gfx/adhoc.bmp");
#endif

		glGenTextures(1,&adhoc_tex);
		glBindTexture(GL_TEXTURE_2D, adhoc_tex);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifndef ANDROID
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#endif

		//FIXME: Android doesn't support GL_BGRA
		int internalformat,format;
		if(tmp->format->BitsPerPixel==32){
			internalformat=GL_RGBA;
			format=GL_RGBA;
#ifndef ANDROID
			if(tmp->format->Rmask!=0xFF) format=GL_BGRA;
#endif
		}else{
			internalformat=GL_RGB;
			format=GL_RGB;
#ifndef ANDROID
			if(tmp->format->Rmask!=0xFF) format=GL_BGR;
#endif
		}

		glTexImage2D(GL_TEXTURE_2D, 0, internalformat, 256, 256, 0, format, GL_UNSIGNED_BYTE, tmp->pixels);

		glBindTexture(GL_TEXTURE_2D, 0);

		SDL_FreeSurface(tmp);
	}

	//FIXME: ad-hoc font
	SimpleBitmapFont fnt;
	{
		SDL_Surface *tmp=SDL_LoadBMP(DATA_PATH "/gfx/adhoc2.bmp");
		fnt.Create(tmp);
		SDL_FreeSurface(tmp);
	}

	//init OpenGL properties
	initGL();

	bool bRun=true,pKeyDownProcessed=false;
	while(bRun){
		theApp->OnTimer();

		clearScreen();

		SetProjectionMatrix();
		theApp->Draw();

		//FIXME: ad-hoc: draw screen keypad
		{
			SetProjectionMatrix(1);

			glDisable(GL_LIGHTING);
			glEnable(GL_TEXTURE_2D);
			//glActiveTexture(GL_TEXTURE0);
			//glClientActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, adhoc_tex);

			float v[]={
				float(screenWidth-256),float(screenHeight-128),0.0f,0.0f,
				float(screenWidth),float(screenHeight-128),1.0f,0.0f,
				float(screenWidth),float(screenHeight),1.0f,0.5f,
				float(screenWidth-256),float(screenHeight),0.0f,0.5f,

				float(screenWidth-448),float(screenHeight-64),0.0f,0.5f,
				float(screenWidth-320),float(screenHeight-64),0.5f,0.5f,
				float(screenWidth-320),float(screenHeight),0.5f,0.75f,
				float(screenWidth-448),float(screenHeight),0.0f,0.75f,
			};

			unsigned short i[]={
				0,1,2,0,2,3,
				4,5,6,4,6,7,
			};

			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glVertexPointer(2,GL_FLOAT,4*sizeof(float),v);
			glTexCoordPointer(2,GL_FLOAT,4*sizeof(float),v+2);

			glDrawElements(GL_TRIANGLES,m_bShowYesNoScreenKeyboard?12:6,GL_UNSIGNED_SHORT,i);

			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glBindTexture(GL_TEXTURE_2D, 0);
			glDisable(GL_TEXTURE_2D);
		}

		//TEST
		fnt.BeginDraw();
		fnt.DrawString(str(MyFormat("Pack: %s\nLevel %d: %s\nMoves: %d")
			<<toUTF8(theApp->m_pDocument->m_sLevelPackName)
			<<(theApp->m_nCurrentLevel+1)<<toUTF8(theApp->m_objPlayingLevel->m_sLevelName)
			<<theApp->m_objPlayingLevel->m_nMoves
			),0.0f,0.0f,(float)screenWidth,(float)screenHeight,
			32.0f,DrawTextFlags::Multiline | DrawTextFlags::Bottom,
			SDL_MakeColor(255,255,255,255));
		fnt.DrawString(adhoc_debug,0.0f,0.0f,(float)screenWidth,(float)screenHeight,
			32.0f,DrawTextFlags::Multiline,
			SDL_MakeColor(255,255,255,255));
		fnt.EndDraw();

		SDL_GL_SwapWindow(mainWindow);

		while(SDL_PollEvent(&event)){
			switch(event.type){
			case SDL_QUIT:
				bRun=false;
				break;
			case SDL_MOUSEMOTION:
				m_bTouchscreen=(event.motion.which==SDL_TOUCH_MOUSEID);
				OnMouseMove(
					event.motion.which,
					event.motion.state,
					event.motion.x,
					event.motion.y,
					SDL_GetModState());
				break;
			case SDL_MOUSEBUTTONDOWN:
				m_bTouchscreen=(event.button.which==SDL_TOUCH_MOUSEID);
				OnMouseDown(
					event.button.which,
					event.button.button,
					event.button.x,
					event.button.y,
					SDL_GetModState());
				break;
			case SDL_MOUSEBUTTONUP:
				m_bTouchscreen=(event.button.which==SDL_TOUCH_MOUSEID);
				OnMouseUp(
					event.button.which,
					event.button.button,
					event.button.x,
					event.button.y,
					SDL_GetModState());
				break;
			case SDL_MOUSEWHEEL:
				{
					int x,y;
					SDL_GetMouseState(&x,&y);
					OnMouseWheel(event.wheel.which,
						x,y,
						event.wheel.x,
						event.wheel.y,
						SDL_GetModState());
				}
				break;
			case SDL_FINGERDOWN:
				m_bTouchscreen=true;
				break;
			case SDL_FINGERUP:
				m_bTouchscreen=true;
				break;
			case SDL_MULTIGESTURE:
				m_bTouchscreen=true;
				OnMultiGesture(
					(int)event.mgesture.touchId,
					event.mgesture.numFingers,
					event.mgesture.x,
					event.mgesture.y,
					event.mgesture.dDist,
					event.mgesture.dTheta);
				break;
			case SDL_WINDOWEVENT:
				switch(event.window.event){
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					OnResize(
						event.window.data1,
						event.window.data2);
					break;
				}
				break;
			case SDL_KEYUP:
				switch(event.key.keysym.sym){
				case SDLK_AC_BACK:
					event.key.keysym.sym=SDLK_ESCAPE;
					break;
				}
				OnKeyUp(
					event.key.keysym.sym,
					event.key.keysym.mod);

				//chcek exit event
				if(pKeyDownProcessed) pKeyDownProcessed=false;
				else{
					switch(event.key.keysym.sym){
					case SDLK_ESCAPE:
						bRun=false;
						break;
					}
				}
				break;
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym){
				case SDLK_AC_BACK:
					event.key.keysym.sym=SDLK_ESCAPE;
					break;
				}
				if(OnKeyDown(
					event.key.keysym.sym,
					event.key.keysym.mod)) pKeyDownProcessed=true;
				break;
			}
		}

		//debug
		SDL_Delay(20);
	}

	glDeleteTextures(1,&adhoc_tex);

	fnt.Destroy();

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

