#include "main.h"
#include "FileSystem.h"
#include "PuzzleBoyApp.h"
#include "PuzzleBoyLevelFile.h"
#include "PuzzleBoyLevel.h"
#include "PuzzleBoyLevelView.h"
#include "VertexList.h"
#include "SimpleBitmapFont.h"
#include "SimpleListScreen.h"
#include "ConfigScreen.h"
#include "MyFormat.h"
#include "TestSolver.h"
#include "TestRandomLevel.h"
#include "RandomMapScreen.h"
#include "SimpleFont.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include "include_sdl.h"
#include "include_gl.h"

SDL_Window *mainWindow=NULL;
SDL_GLContext glContext=NULL;
int screenWidth=0;
int screenHeight=0;
int m_nResizeTime=0; ///< value increased when screen resized
float screenAspectRatio=1.0f; ///< width/height

SDL_Event event;

bool m_bRun=true;
bool m_bKeyDownProcessed=false;

//================touchscreen and multi-touch event

float m_fMultitouchOldX,m_fMultitouchOldY;
float m_fMultitouchOldDistSquared=-1.0f;

//test
#ifdef ANDROID
bool m_bTouchscreen=true;
#else
bool m_bTouchscreen=false;
#endif

bool m_bShowYesNoScreenKeyboard=false;

//0=not dragging or failed,1=mouse down,2=dragging,3=multi-touch is running or failed
int m_nDraggingState=0;

//================

//FIXME: ad-hoc
GLuint adhoc_screenkb_tex=0;

SimpleFontFile *mainFontFile=NULL;
SimpleBaseFont *mainFont=NULL;
SimpleBaseFont *titleFont=NULL;

//================

static void OnMouseWheel(int which,int x,int y,int dx,int dy,int nFlags);

void WaitForNextFrame(){
	SDL_Delay(25);
}

void SetProjectionMatrix(int idx){
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	switch(idx){
	case 1:
	default:
		glOrtho(0.0f,float(screenWidth),float(screenHeight),0.0f,-1.0f,1.0f);
		break;
	}

	glMatrixMode(GL_MODELVIEW);
}

static void initGL(){
	screenAspectRatio=float(screenWidth)/float(screenHeight);
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

void ClearScreen(){
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

void AddScreenKeyboard(float x,float y,float w,float h,int index,std::vector<float>& v,std::vector<unsigned short>& idx){
	unsigned short m=(unsigned short)(v.size()>>2);

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
	glBindTexture(GL_TEXTURE_2D, adhoc_screenkb_tex);

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

class ChooseLevelFileScreen:public SimpleListScreen{
public:
	virtual void OnDirty(){
		m_nListCount=m_files.size();

		if(m_txtList) m_txtList->clear();
		else m_txtList=new SimpleText;

		for(int i=0;i<m_nListCount;i++){
			m_txtList->NewStringIndex();
			m_txtList->AddString(mainFont,m_fileDisplayName[i],0,float(i*32),0,32,1,DrawTextFlags::VCenter);
		}
	}

	virtual int OnClick(int index){
		if(!theApp->LoadFile(m_files[index])){
			printf("[ChooseLevelFileScreen] Failed to load file %s\n",m_files[index].c_str());
			return -1;
		}
		return 1;
	}

	virtual int DoModal(){
		//enum internal files
		{
			u8string fn="data/levels/";
			std::vector<u8string> fs=enumAllFiles(fn,"lev");
			for(size_t i=0;i<fs.size();i++){
				m_files.push_back(fn+fs[i]);
				m_fileDisplayName.push_back(fs[i]);
			}
		}

		//enum external files
		{
			u8string fn=externalStoragePath+"/levels/";
			std::vector<u8string> fs=enumAllFiles(fn,"lev");
			for(size_t i=0;i<fs.size();i++){
				m_files.push_back(fn+fs[i]);
				m_fileDisplayName.push_back(_("User level: ")+fs[i]);
			}
		}

		//show
		m_LeftButtons.push_back(SCREEN_KEYBOARD_LEFT);
		CreateTitleBarText(_("Choose Level File"));
		return SimpleListScreen::DoModal();
	}
private:
	std::vector<u8string> m_files;
	std::vector<u8string> m_fileDisplayName;
};

class ChooseLevelScreen:public SimpleListScreen{
public:
	virtual void OnDirty(){
		m_nListCount=theApp->m_pDocument->m_objLevels.size();

		if(m_txtList) m_txtList->clear();
		else m_txtList=new SimpleText;

		for(int i=0;i<m_nListCount;i++){
			char s[32];
			m_txtList->NewStringIndex();
			sprintf(s,"%d",i+1);
			m_txtList->AddString(mainFont,s,0,float(i*32),0,32,1,DrawTextFlags::VCenter);
			m_txtList->AddString(mainFont,toUTF8(theApp->m_pDocument->m_objLevels[i]->m_sLevelName),96,float(i*32),0,32,1,DrawTextFlags::VCenter);
			sprintf(s,"%dx%d",theApp->m_pDocument->m_objLevels[i]->m_nWidth,
				theApp->m_pDocument->m_objLevels[i]->m_nHeight);
			m_txtList->AddString(mainFont,s,float(screenWidth-128),float(i*32),0,32,1,DrawTextFlags::VCenter);
		}
	}

	virtual int OnClick(int index){
		theApp->m_nCurrentLevel=index;
		return 1;
	}

	virtual int OnTitleBarButtonClick(int index){
		switch(index){
		case SCREEN_KEYBOARD_OPEN:
			if(ChooseLevelFileScreen().DoModal()){
				m_nReturnValue=1;
				m_bDirty=true;
			}
			return -1;
			break;
		}

		return m_nReturnValue;
	}

	virtual int DoModal(){
		m_bDirtyOnResize=true;
		m_LeftButtons.push_back(SCREEN_KEYBOARD_LEFT);
		m_RightButtons.push_back(SCREEN_KEYBOARD_OPEN);
		CreateTitleBarText(_("Choose Level"));
		return SimpleListScreen::DoModal();
	}
};

void OnVideoResize(int width, int height){
	m_nResizeTime++;

	screenWidth=width;
	screenHeight=height;
	screenAspectRatio=float(screenWidth)/float(screenHeight);

	glViewport(0,0,screenWidth,screenHeight);
}

static void OnMouseDown(int which,int button,int x,int y,int nFlags){
	//FIXME: ad-hoc screen keyboard
	bool b=false;
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
	case 0x204: theApp->OnKeyDown(SDLK_u,0); b=true; break;
	case 0x203: theApp->OnKeyDown(SDLK_UP,0); b=true; break;
	case 0x202: theApp->OnKeyDown(SDLK_r,0); b=true; break;
	case 0x201: theApp->OnKeyDown(SDLK_r,KMOD_CTRL); b=true; break;
	case 0x104: theApp->OnKeyDown(SDLK_LEFT,0); b=true; break;
	case 0x103: theApp->OnKeyDown(SDLK_DOWN,0); b=true; break;
	case 0x102: theApp->OnKeyDown(SDLK_RIGHT,0); b=true; break;
	case 0x101: theApp->OnKeyDown(SDLK_SPACE,0); b=true; break;
	}

	if(m_bShowYesNoScreenKeyboard){
		switch(idx){
		case 0x107: theApp->OnKeyDown(SDLK_RETURN,0); b=true; break;
		case 0x106: theApp->OnKeyDown(SDLK_ESCAPE,0); b=true; break;
		}
	}

	if(b){
		m_nDraggingState=3;
	}else{
		theApp->OnMouseEvent(SDL_BUTTON(button),x,y,SDL_MOUSEBUTTONDOWN);
	}
}

static void OnMouseUp(int which,int button,int x,int y,int nFlags){
	theApp->OnMouseEvent(SDL_BUTTON(button),x,y,SDL_MOUSEBUTTONUP);
}

//FIXME: ad-hoc disable mouse move event on screen keyboard
static void OnMouseMove(int which,int state,int x,int y,int nFlags){
	if(!m_bTouchscreen) theApp->OnMouseEvent(state,x,y,SDL_MOUSEMOTION);
}

//fx,fy: position of new center in normalized coordinate: (w,h)-->(screenAspectRatio,1.0f)
//dx,dy: relative motion in normalized coordinate
//zoom: relative zoom factor
static void OnMultiGesture(float fx,float fy,float dx,float dy,float zoom){
	//FIXME: ad-hoc
	int m=theApp->m_view.size();

	if(m<=0){
		return;
	}else if(m==1){
		m=0;
	}else if(m>=2){
		if(fx<screenAspectRatio*0.5f) m=0;
		else m=1;
	}

	theApp->m_view[m]->m_scrollView.OnMultiGesture(fx,fy,dx,dy,zoom);
}

static void OnMouseWheel(int which,int x,int y,int dx,int dy,int nFlags){
	//test
	if(dy){
		OnMultiGesture(float(x)/float(screenHeight),float(y)/float(screenHeight),0.0f,0.0f,dy>0?1.1f:1.0f/1.1f);
	}
}

class MainMenuScreen:public SimpleListScreen{
public:
	static const int TestFeatureStart=5;
public:
	virtual void OnDirty(){
		if(m_txtList) m_txtList->clear();
		else m_txtList=new SimpleText;

		m_nListCount=0;

		m_txtList->NewStringIndex();
		m_txtList->AddString(mainFont,_("Choose Level"),0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);

		m_txtList->NewStringIndex();
		m_txtList->AddString(mainFont,_("Choose Level File"),0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);

		m_txtList->NewStringIndex();
		m_txtList->AddString(mainFont,_("Config"),0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);

		m_txtList->NewStringIndex();
		m_txtList->AddString(mainFont,_("Exit Game"),0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);

		//test feature
		m_txtList->NewStringIndex();
		m_nListCount++;

		m_txtList->NewStringIndex();
		m_txtList->AddString(mainFont,"Test Solver",0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);

		m_txtList->NewStringIndex();
		m_txtList->AddString(mainFont,"Random Map",0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);

		m_txtList->NewStringIndex();
		m_txtList->AddString(mainFont,"Random Map x10",0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);

		m_txtList->NewStringIndex();
		m_txtList->AddString(mainFont,"Save Temp Level File",0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);

		m_txtList->NewStringIndex();
		m_txtList->AddString(mainFont,"Start Multiplayer",0,float((m_nListCount++)*32),0,32,1,DrawTextFlags::VCenter);
	}

	virtual int OnClick(int index){
		switch(index){
		case 0:
			//choose level
			if(ChooseLevelScreen().DoModal()) return 1;
			break;
		case 1:
			//choose level file
			if(ChooseLevelFileScreen().DoModal()){
				ChooseLevelScreen().DoModal();
				return 1;
			}
			break;
		case 2:
			//config
			ConfigScreen().DoModal();
			return 0;
			break;
		case 3:
			//exit game
			m_bRun=false;
			return 0;
			break;
		case TestFeatureStart:
			//ad-hoc solver test
			{
				PuzzleBoyLevelData *dat=theApp->m_pDocument->GetLevel(theApp->m_nCurrentLevel);
				if(dat){
					printf("--- Solver Test ---\n");

					Uint64 f=SDL_GetPerformanceFrequency(),t=SDL_GetPerformanceCounter();

					PuzzleBoyLevel *lev=new PuzzleBoyLevel(*dat);
					lev->StartGame();
					u8string s;
					int ret=lev->SolveIt(s,NULL,NULL);

					t=SDL_GetPerformanceCounter()-t;

					printf("SolveIt() returns %d, Time=%0.2fms\n",ret,double(t)/double(f)*1000.0);
					if(ret==1) printf("The solution is %s\n",s.c_str());

					delete lev;
				}
				return 0;
			}
			break;
		case TestFeatureStart+1:
			//ad-hoc random level TEST
			{
				int type=RandomMapScreen().DoModal();
				if(type>0){
					PuzzleBoyLevelFile *doc=RandomMapScreen::DoRandomLevels(type,1);
					if(doc){
						delete theApp->m_pDocument;
						theApp->m_pDocument=doc;
						return 1;
					}
				}
			}
			break;
		case TestFeatureStart+2:
			//batch TEST
			{
				int type=RandomMapScreen().DoModal();
				if(type>0){
					PuzzleBoyLevelFile *doc=RandomMapScreen::DoRandomLevels(type,10);
					if(doc){
						delete theApp->m_pDocument;
						theApp->m_pDocument=doc;
						return 1;
					}
				}
			}
			break;
		case TestFeatureStart+3:
			//save temp file
			if(theApp->m_pDocument){
				time_t t=time(NULL);
				tm *timeinfo=localtime(&t);
				char s[128];
				strftime(s,sizeof(s),"/levels/tmp-%Y%m%d%H%M%S.lev",timeinfo);
				theApp->SaveFile(externalStoragePath+s);
			}
			return 0;
			break;
		case TestFeatureStart+4:
			//start multiplayer!!! TEST!!
			return 2;
			break;
		}

		return -1;
	}

	virtual int DoModal(){
		//show
		m_LeftButtons.push_back(SCREEN_KEYBOARD_LEFT);
		CreateTitleBarText(_("Main Menu"));
		return SimpleListScreen::DoModal();
	}
};

static bool OnKeyDown(int nChar,int nFlags){
	//ad-hoc menu experiment
	if(nChar==SDLK_MENU || nChar==SDLK_APPLICATION
#ifndef ANDROID
		|| nChar==SDLK_ESCAPE
#endif
		)
	{
		m_nDraggingState=0;
		int ret=MainMenuScreen().DoModal();
		if(ret>=1){
			if(ret>=2) ret=2;
			theApp->StartGame(ret);
		}
		return false;
	}

	if(theApp->OnKeyDown(nChar,nFlags)) return true;
	return false;
}

static void OnKeyUp(int nChar,int nFlags){
	theApp->OnKeyUp(nChar,nFlags);
}

//experimental dragging support
//x,y: position of new center in normalized coordinate: (w,h)-->(screenAspectRatio,1.0f)
static void CheckDragging(float x,float y){
	float dx=x-m_fMultitouchOldX;
	float dy=y-m_fMultitouchOldY;
	if(m_nDraggingState==2 || dx*dx+dy*dy>0.003f){
		m_nDraggingState=2;
		m_fMultitouchOldX=x;
		m_fMultitouchOldY=y;
		OnMultiGesture(x,y,dx,dy,1.0f);
	}
}

int SDL_main(int argc,char** argv){
	//init file system
	initPaths();

	//init SDL
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK)==-1) abort();

	//init freetype
	if(!FreeType_Init()) abort();

	theApp=new PuzzleBoyApp;

	//init random number
	{
		unsigned int seed[4];
		unsigned long long t=time(NULL);
		seed[0]=(unsigned int)t;
		seed[1]=(unsigned int)(t>>32);

		t=SDL_GetPerformanceCounter();
		seed[2]=(unsigned int)t;
		seed[3]=(unsigned int)(t>>32);

		theApp->m_objMainRnd.Init(seed,sizeof(seed)/sizeof(unsigned int));
	}

	//load config
	theApp->LoadConfig(externalStoragePath+"/PuzzleBoy.cfg");

	//load locale
	theApp->LoadLocale();

	//load record file
	theApp->m_objRecordMgr.LoadFile((externalStoragePath+"/PuzzleBoyRecord.dat").c_str());

	//TEST: load default level file
	if(!theApp->LoadFile("data/levels/PuzzleBoy.lev")){
		printf("[main] Error: Failed to load default level file\n");
	}

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
		SDL_Surface *tmp=IMG_Load("data/gfx/adhoc.png");
		IMG_Quit();
#else
		SDL_Surface *tmp=SDL_LoadBMP("data/gfx/adhoc.bmp");
#endif

		glGenTextures(1,&adhoc_screenkb_tex);
		glBindTexture(GL_TEXTURE_2D, adhoc_screenkb_tex);

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

	//try to load FreeType font
	if(theApp->m_bInternationalFont){
		mainFontFile=new SimpleFontFile;
		if(mainFontFile->LoadFile("data/font/DroidSansFallback.ttf")){
			mainFont=new SimpleFont(*mainFontFile,20);
			titleFont=new SimpleFont(*mainFontFile,32);

			//test preload glyph
			mainFont->AddGlyph(0,true);
			titleFont->AddGlyph(0,true);
		}else{
			printf("[main] Error: Can't load font file, fallback to bitmap font\n");
			delete mainFontFile;
			mainFontFile=NULL;
		}
	}

	//ad-hoc bitmap font
	if(mainFont==NULL){
		SDL_Surface *tmp=SDL_LoadBMP("data/gfx/adhoc2.bmp");
		mainFont=new SimpleBitmapFont(tmp);
		SDL_FreeSurface(tmp);
	}

	//init OpenGL properties
	initGL();

	//init level and graphics
	theApp->StartGame(1);

	while(m_bRun){
		//game logic
		theApp->OnTimer();

		//clear and draw
		ClearScreen();

		theApp->Draw();

		SetProjectionMatrix(1);

		//FIXME: ad-hoc: draw screen keypad
		{
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

			const unsigned short i[]={
				0,1,2,0,2,3,
				4,5,6,4,6,7,
			};

			glDisable(GL_LIGHTING);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, adhoc_screenkb_tex);

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
		{
			MyFormat fmt(_("Pack: %s"));
			fmt<<toUTF8(theApp->m_pDocument->m_sLevelPackName);

			int m=theApp->m_view.size();

			if(m==1){
				PuzzleBoyLevelView *view=theApp->m_view[0];

				fmt("\n")(_("Level %d: %s"))("\n")(_("Moves: %d"))<<(view->m_nCurrentLevel+1)
					<<toUTF8(view->m_objPlayingLevel->m_sLevelName)
					<<view->m_objPlayingLevel->m_nMoves;

				if(view->m_nCurrentBestStep>0){
					fmt(" - ")(_("Best: %d (%s)"))<<view->m_nCurrentBestStep
						<<view->m_sCurrentBestStepOwner;
				}
			}else if(m>=2){
				fmt("\nLevel: %d vs %d\nMoves: %d vs %d")<<(theApp->m_view[0]->m_nCurrentLevel+1)
					<<(theApp->m_view[1]->m_nCurrentLevel+1)
					<<(theApp->m_view[0]->m_objPlayingLevel->m_nMoves)<<(theApp->m_view[1]->m_objPlayingLevel->m_nMoves);
			}

			mainFont->DrawString(str(fmt),0.0f,0.0f,(float)screenWidth,(float)screenHeight,
				1.0f,DrawTextFlags::Multiline | DrawTextFlags::Bottom,SDL_MakeColor(255,255,255,255));
		}

		SDL_GL_SwapWindow(mainWindow);

		while(SDL_PollEvent(&event)){
			switch(event.type){
			case SDL_QUIT:
				m_bRun=false;
				break;
			case SDL_MOUSEMOTION:
				m_bTouchscreen=(event.motion.which==SDL_TOUCH_MOUSEID);

				//experimental dragging support
				if(!m_bTouchscreen && event.motion.state && (m_nDraggingState==1 || m_nDraggingState==2)){
					CheckDragging(float(event.motion.x)/float(screenHeight),
						float(event.motion.y)/float(screenHeight));
				}

				if(m_nDraggingState<2) OnMouseMove(
					event.motion.which,
					event.motion.state,
					event.motion.x,
					event.motion.y,
					SDL_GetModState());
				break;
			case SDL_MOUSEBUTTONDOWN:
				m_bTouchscreen=(event.button.which==SDL_TOUCH_MOUSEID);

				//experimental dragging support
				if(m_nDraggingState==0){
					m_nDraggingState=1;
					m_fMultitouchOldX=float(event.button.x)/float(screenHeight);
					m_fMultitouchOldY=float(event.button.y)/float(screenHeight);
				}

				if(m_nDraggingState<2) OnMouseDown(
					event.button.which,
					event.button.button,
					event.button.x,
					event.button.y,
					SDL_GetModState());
				break;
			case SDL_MOUSEBUTTONUP:
				m_bTouchscreen=(event.button.which==SDL_TOUCH_MOUSEID);

				if(m_nDraggingState<2) OnMouseUp(
					event.button.which,
					event.button.button,
					event.button.x,
					event.button.y,
					SDL_GetModState());

				//experimental dragging support
				if(!m_bTouchscreen && m_nDraggingState && SDL_GetMouseState(NULL,NULL)==0) m_nDraggingState=0;

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
				{
					int m=SDL_GetNumTouchFingers(event.tfinger.touchId);
					SDL_Finger *f0=NULL,*f1=NULL;
					if(m==2){
						f0=SDL_GetTouchFinger(event.tfinger.touchId,0);
						f1=SDL_GetTouchFinger(event.tfinger.touchId,1);
					}
					if(f0 && f1){
						m_fMultitouchOldX=(f0->x+f1->x)*screenAspectRatio*0.5f;
						m_fMultitouchOldY=(f0->y+f1->y)*0.5f;
						float dx=(f0->x-f1->x)*screenAspectRatio;
						float dy=f0->y-f1->y;
						m_fMultitouchOldDistSquared=dx*dx+dy*dy;
					}else{
						m_fMultitouchOldDistSquared=-1.0f;
					}
					if(m>=2) m_nDraggingState=3;
				}
				break;
			case SDL_FINGERMOTION:
				m_bTouchscreen=true;
				{
					int m=SDL_GetNumTouchFingers(event.tfinger.touchId);
					SDL_Finger *f0=NULL,*f1=NULL;
					if(m==2 && m_fMultitouchOldDistSquared>0.0f){
						f0=SDL_GetTouchFinger(event.tfinger.touchId,0);
						f1=SDL_GetTouchFinger(event.tfinger.touchId,1);
					}
					if(f0 && f1){
						float x=(f0->x+f1->x)*screenAspectRatio*0.5f;
						float y=(f0->y+f1->y)*0.5f;

						float dx=(f0->x-f1->x)*screenAspectRatio;
						float dy=f0->y-f1->y;
						dx=dx*dx+dy*dy;
						if(dx<1E-30f) dx=1E-30f;

						float zoom=sqrt(dx/m_fMultitouchOldDistSquared);
						if(zoom<0.1f) zoom=0.1f;
						else if(zoom>10.0f) zoom=10.0f;
						m_fMultitouchOldDistSquared=dx;

						dx=x-m_fMultitouchOldX;
						dy=y-m_fMultitouchOldY;
						m_fMultitouchOldX=x;
						m_fMultitouchOldY=y;

						OnMultiGesture(x,y,dx,dy,zoom);
					}else{
						m_fMultitouchOldDistSquared=-1.0f;
					}

					//experimental dragging support
					if(m>=2) m_nDraggingState=3;
					if(m==1 && (m_nDraggingState==1 || m_nDraggingState==2)){
						CheckDragging(event.tfinger.x*screenAspectRatio,event.tfinger.y);
					}
				}
				break;
			case SDL_FINGERUP:
				m_bTouchscreen=true;
				m_fMultitouchOldDistSquared=-1.0f;
				if(SDL_GetNumTouchFingers(event.tfinger.touchId)<=0) m_nDraggingState=0;
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
			case SDL_KEYUP:
				switch(event.key.keysym.sym){
				case SDLK_AC_BACK: event.key.keysym.sym=SDLK_ESCAPE; break;
				}
				OnKeyUp(
					event.key.keysym.sym,
					event.key.keysym.mod);

				//chcek exit event
				if(m_bKeyDownProcessed) m_bKeyDownProcessed=false;
				else{
					switch(event.key.keysym.sym){
#ifdef ANDROID
					case SDLK_ESCAPE:
						{ //just press Esc
#else
					case SDLK_F4:
						if(event.key.keysym.mod & KMOD_ALT){ //Alt+F4
#endif
							m_bRun=false;
						}
						break;
					}
				}
				break;
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym){
				case SDLK_AC_BACK: event.key.keysym.sym=SDLK_ESCAPE; break;
				}
				if(OnKeyDown(
					event.key.keysym.sym,
					event.key.keysym.mod)) m_bKeyDownProcessed=true;
				break;
			}
		}

		WaitForNextFrame();
	}

	glDeleteTextures(1,&adhoc_screenkb_tex);

	delete mainFont;
	mainFont=NULL;
	delete titleFont;
	titleFont=NULL;
	delete mainFontFile;
	mainFontFile=NULL;

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(mainWindow);
	SDL_Quit();

	FreeType_Quit();

	delete theApp;
	theApp=NULL;

#ifdef ANDROID
	exit(0);
#endif
	return 0;
}

