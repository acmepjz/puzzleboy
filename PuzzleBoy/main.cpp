#include "main.h"
#include "FileSystem.h"
#include "PuzzleBoyApp.h"
#include "PuzzleBoyLevelFile.h"
#include "PuzzleBoyLevel.h"
#include "VertexList.h"
#include "SimpleBitmapFont.h"
#include "SimpleListScreen.h"
#include "MyFormat.h"
#include "clsTiming.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "include_sdl.h"
#include "include_gl.h"

PuzzleBoyApp *theApp=NULL;
SDL_Window *mainWindow=NULL;
SDL_GLContext glContext=NULL;
int screenWidth=0;
int screenHeight=0;
int m_nResizeTime=0; ///< value increased when screen resized
float screenAspectRatio=1.0f;

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
float adhoc_zoom=1.0f/48.0f;
float adhoc_x=0.0f;
float adhoc_y=0.0f;
float adhoc2_zoom=1.0f/48.0f;
float adhoc2_x=0.0f;
float adhoc2_y=0.0f;
SimpleBitmapFont *adhoc_fnt=NULL;

u8string adhoc_debug;

static void OnMouseWheel(int which,int x,int y,int dx,int dy,int nFlags);

void WaitForNextFrame(){
	SDL_Delay(20);
}

void SetProjectionMatrix(int idx){
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	switch(idx){
	case 1:
		glOrtho(0.0f,float(screenWidth),float(screenHeight),0.0f,-1.0f,1.0f);
		break;
	default:
		glOrtho(adhoc2_x,adhoc2_x+float(screenWidth)*adhoc2_zoom,
			adhoc2_y+float(screenHeight)*adhoc2_zoom,adhoc2_y,-1.0f,1.0f);
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

void TranslateCoordinate(int x,int y,int& out_x,int& out_y){
	out_x=(int)floor(float(x)*adhoc2_zoom+adhoc2_x);
	out_y=(int)floor(float(y)*adhoc2_zoom+adhoc2_y);
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
		else m_txtList=new SimpleBitmapText;

		for(int i=0;i<m_nListCount;i++){
			m_txtList->NewStringIndex();
			m_txtList->AddString(m_fileDisplayName[i],0,float(i*32),0,0,32,0);
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
		m_fnt=adhoc_fnt;
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
		else m_txtList=new SimpleBitmapText;

		for(int i=0;i<m_nListCount;i++){
			char s[32];
			m_txtList->NewStringIndex();
			sprintf(s,"%d",i+1);
			m_txtList->AddString(s,0,float(i*32),0,0,32,0);
			m_txtList->AddString(toUTF8(theApp->m_pDocument->m_objLevels[i]->m_sLevelName),96,float(i*32),0,0,32,0);
			sprintf(s,"%dx%d",theApp->m_pDocument->m_objLevels[i]->m_nWidth,
				theApp->m_pDocument->m_objLevels[i]->m_nHeight);
			m_txtList->AddString(s,float(screenWidth-128),float(i*32),0,0,32,0);
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
		m_fnt=adhoc_fnt;
		m_bDirtyOnResize=true;
		m_LeftButtons.push_back(SCREEN_KEYBOARD_LEFT);
		m_RightButtons.push_back(SCREEN_KEYBOARD_OPEN);
		CreateTitleBarText(_("Choose Level"));
		return SimpleListScreen::DoModal();
	}
};

void GameScreenFit(int x,int y,int w,int h){
	//FIXME: ad-hoc workspace size
	SDL_Rect r={0,0,screenWidth,screenHeight-128};

	if(w*r.h>h*r.w){ // w/h>r.w/r.h
		adhoc_zoom=float(w)/float(r.w);
		adhoc_x=float(x)-float(r.x)*adhoc_zoom;
		adhoc_y=float(y)+float(h)*0.5f-(float(r.y)+float(r.h)*0.5f)*adhoc_zoom;
	}else{
		adhoc_zoom=float(h)/float(r.h);
		adhoc_x=float(x)+float(w)*0.5f-(float(r.x)+float(r.w)*0.5f)*adhoc_zoom;
		adhoc_y=float(y)-float(r.y)*adhoc_zoom;
	}
}

void GameScreenEnsureVisible(int x,int y){
	//FIXME: ad-hoc workspace size
	SDL_Rect r={0,0,screenWidth,screenHeight-128};

	if(float(r.x)*adhoc2_zoom+adhoc2_x>float(x)+0.5f
		|| float(r.y)*adhoc2_zoom+adhoc2_y>float(y)+0.5f
		|| float(r.x+r.w)*adhoc2_zoom+adhoc2_x<float(x)+0.5f
		|| float(r.y+r.h)*adhoc2_zoom+adhoc2_y<float(y)+0.5f)
	{
		adhoc_x=float(x)+0.5f-(float(r.x)+float(r.w)*0.5f)*adhoc_zoom;
		adhoc_y=float(y)+0.5f-(float(r.y)+float(r.h)*0.5f)*adhoc_zoom;
	}
}

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
	//test only

	//move
	float f=float(screenHeight)*adhoc_zoom;
	adhoc_x-=dx*f;
	adhoc_y-=dy*f;

	//zoom
	f*=(1.0f-1.0f/zoom);
	adhoc_x+=fx*f;
	adhoc_y+=fy*f;
	adhoc_zoom/=zoom;
}

static void OnMouseWheel(int which,int x,int y,int dx,int dy,int nFlags){
	//test
	if(dy){
		OnMultiGesture(float(x)/float(screenHeight),float(y)/float(screenHeight),0.0f,0.0f,dy>0?1.1f:1.0f/1.1f);
	}
}

static bool OnKeyDown(int nChar,int nFlags){
	//ad-hoc experiment
	if(nChar==SDLK_MENU || nChar==SDLK_APPLICATION){
		m_nDraggingState=0;
		if(ChooseLevelScreen().DoModal()) theApp->StartGame();
		return false;
	}

	//ad-hoc solver test
	if(nChar==SDLK_t && (nFlags & KMOD_CTRL)!=0){
		PuzzleBoyLevelData *dat=theApp->GetDocument()->GetLevel(theApp->m_nCurrentLevel);
		if(dat){
			printf("--- Solver Test ---\n");

			clsTiming t;
			t.Start();
			PuzzleBoyLevel *lev=new PuzzleBoyLevel(*dat);
			lev->StartGame();
			u8string s;
			int ret=lev->SolveIt(s,NULL,NULL);
			t.Stop();

			printf("SolveIt() returns %d, Time=%0.2fms\n",ret,t.GetMs());
			if(ret==1) printf("The solution is %s\n",s.c_str());

			delete lev;
		}
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

int main(int argc,char** argv){
	initPaths();

	theApp=new PuzzleBoyApp;

	theApp->m_objGetText.LoadFileWithAutoLocale("data/locale/*.mo");

	adhoc_debug="DEBUG!!1";

	theApp->m_nAnimationSpeed=1;
	theApp->m_bShowGrid=true;
	theApp->m_bShowLines=true;

	//FIXME: ad-hoc!!!1!!1!! player name
	{
		u8string s;
		u8file *f=u8fopen((externalStoragePath+"/adhoc_playername.txt").c_str(),"rb");
		bool b=false;
		if(f){
			if(u8fgets2(s,f)){
				u8string::size_type lps=s.find_first_of("\r\n");
				if(lps!=u8string::npos) s=s.substr(0,lps);
				b=(!s.empty());
			}
			u8fclose(f);
		}
		if(b){
			theApp->m_sPlayerName=toUTF16(s);
		}else{
			f=u8fopen((externalStoragePath+"/adhoc_playername.txt").c_str(),"wb");
			if(f){
				u8fwrite("acme_pjz",8,1,f);
				u8fclose(f);
			}
			theApp->m_sPlayerName=toUTF16("acme_pjz");
		}
	}

	//load record file
	theApp->m_objRecordMgr.LoadFile((externalStoragePath+"/PuzzleBoyRecord.dat").c_str());

	//TEST: load default level file
	if(!theApp->LoadFile("data/levels/PuzzleBoy.lev")){
		printf("[main] Failed to load default level file\n");
	}

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

	//ad-hoc font
	SimpleBitmapText txt;
	{
		SDL_Surface *tmp=SDL_LoadBMP("data/gfx/adhoc2.bmp");
		adhoc_fnt=new SimpleBitmapFont();
		adhoc_fnt->Create(tmp);
		SDL_FreeSurface(tmp);
	}

	//init OpenGL properties
	initGL();

	//init level and graphics
	theApp->StartGame();

	while(m_bRun){
		//fake animation
		adhoc2_zoom=(adhoc_zoom+adhoc2_zoom)*0.5f;
		adhoc2_x=(adhoc_x+adhoc2_x)*0.5f;
		adhoc2_y=(adhoc_y+adhoc2_y)*0.5f;
		//game logic
		theApp->OnTimer();

		//clear and draw
		ClearScreen();

		SetProjectionMatrix();
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
			MyFormat fmt("Pack: %s\nLevel %d: %s\nMoves: %d");
			fmt<<toUTF8(theApp->m_pDocument->m_sLevelPackName)
				<<(theApp->m_nCurrentLevel+1)<<toUTF8(theApp->m_objPlayingLevel->m_sLevelName)
				<<theApp->m_objPlayingLevel->m_nMoves;

			if(theApp->m_nCurrentBestStep>0){
				fmt(" - Best: %d (%s)")<<theApp->m_nCurrentBestStep
					<<theApp->m_sCurrentBestStepOwner;
			}

			adhoc_fnt->BeginDraw();
			txt.clear();

			txt.AddString(str(fmt),0.0f,0.0f,(float)screenWidth,(float)screenHeight,
				32.0f,DrawTextFlags::Multiline | DrawTextFlags::Bottom);
			txt.AddString(adhoc_debug,0.0f,0.0f,(float)screenWidth,(float)screenHeight,
				32.0f,DrawTextFlags::Multiline);
			txt.Draw(SDL_MakeColor(255,255,255,255));
			adhoc_fnt->EndDraw();
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
					case SDLK_ESCAPE:
						m_bRun=false;
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

	adhoc_fnt->Destroy();
	delete adhoc_fnt;
	adhoc_fnt=NULL;

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

