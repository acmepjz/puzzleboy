#include "main.h"
#include "FileSystem.h"
#include "PuzzleBoyApp.h"
#include "PuzzleBoyLevelFile.h"
#include "PuzzleBoyLevelView.h"
#include "SimpleBitmapFont.h"
#include "SimpleListScreen.h"
#include "ConfigScreen.h"
#include "MyFormat.h"
#include "TestSolver.h"
#include "TestRandomLevel.h"
#include "RandomMapScreen.h"
#include "SimpleFont.h"
#include "ChooseLevelScreen.h"
#include "LevelRecordScreen.h"
#include "SimpleMessageBox.h"
#include "SimpleMiscScreen.h"
#include "SimpleProgressScreen.h"

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

//================

//ad-hoc textures
GLuint adhoc_screenkb_tex=0;
GLuint adhoc3_tex=0;

SimpleFontFile *mainFontFile=NULL;
SimpleBaseFont *mainFont=NULL;
SimpleBaseFont *titleFont=NULL;

//================

float m_FPS=0.0f;
int m_nFPSLastCount=0;
unsigned int m_nFPSLastTime=0;

int m_nIdleTime=0;

//================

void WaitForNextFrame(){
	SDL_Delay(25);
}

void SetProjectionMatrix(int idx){
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	switch(idx){
	case 0:
		glOrtho(0.0f,screenAspectRatio,1.0f,0.0f,-1.0f,1.0f);
		break;
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

void ShowScreen(){
	bool bDirty=false;

	SetProjectionMatrix(1);

	if(theApp->m_bShowFPS){
		//update FPS
		m_nFPSLastCount++;
		unsigned int t=SDL_GetTicks();
		int dt=t-m_nFPSLastTime;
		if(dt>500){
			m_FPS=float(m_nFPSLastCount)*1000.0f/float(dt);
			m_nFPSLastTime=t;
			m_nFPSLastCount=0;
		}

		//draw FPS
		char s[64];
		SDL_snprintf(s,sizeof(s),"FPS: %0.1f",m_FPS);
		mainFont->DrawString(s,float(screenWidth-128),0,0,0,0.75f,0,SDL_MakeColor(255,255,255,255));
	}else{
		m_FPS=0.0f;
		m_nFPSLastCount=0;
		m_nFPSLastTime=0;
	}

	//show tooltip
	if(theApp->m_nToolTipTime>0){
		bDirty=true;

		int alpha=theApp->m_nToolTipTime;
		if(alpha>64) alpha=128-alpha;
		if(alpha>0){
			alpha<<=4;
			if(alpha>255) alpha=255;

			//create text object
			size_t m=theApp->m_sToolTipText.size();
			SimpleText txt;

			U8STRING_FOR_EACH_CHARACTER_DO_BEGIN((theApp->m_sToolTipText),i,m,c,'?');

			//we need to call some internal functions
			txt.AddChar(mainFont,c,0,0,1.0f,0);

			U8STRING_FOR_EACH_CHARACTER_DO_END();

			//get width of text
			float ww=txt.ww;
			float hh=mainFont->GetFontHeight();

			float scale=1.0f;
			if(ww>float(screenWidth-8)){
				scale=float(screenWidth-8)/ww;
				hh*=scale;
				ww=float(screenWidth-8);
			}

			float xx=(float(screenWidth)-ww)*0.5f;
			float yy=float(screenHeight)*0.75f-hh*0.5f;

			txt.AddChar(mainFont,-1,xx,float(screenWidth-8),1.0f,DrawTextFlags::AutoSize);
			txt.AdjustVerticalPosition(mainFont,0,float(screenHeight)*0.75f,0,/*scale*/1.0f,DrawTextFlags::VCenter);

			xx-=4.0f;ww+=8.0f;
			//yy-=4.0f;hh+=8.0f;

			//draw background
			float vv[8]={
				xx,yy,xx,yy+hh,
				xx+ww,yy,xx+ww,yy+hh,
			};

			unsigned short ii[6]={0,1,3,0,3,2};

			glColor4ub(64,64,64,alpha);

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(2,GL_FLOAT,0,vv);
			glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_SHORT,ii);
			glDisableClientState(GL_VERTEX_ARRAY);

			//draw text
			mainFont->DrawString(txt,SDL_MakeColor(255,255,255,alpha));
		}

		theApp->m_nToolTipTime-=2;
		if(theApp->m_nToolTipTime<=0){
			theApp->m_nToolTipTime=0;
			theApp->m_bToolTipIsExit=false;
		}
	}

	SDL_GL_SwapWindow(mainWindow);

	if(bDirty) m_nIdleTime=0;
}

unsigned int CreateGLTexture(int width,int height,int desiredFormat,int wrap,int minFilter,int magFilter,const char* srcBMPFile,SDL_Surface* srcSurface,const void* srcData,unsigned int colorKey){
	const int SDL_PIXELFORMAT_RGBA32=
		(SDL_BYTEORDER==SDL_BIG_ENDIAN)?SDL_PIXELFORMAT_RGBA8888:SDL_PIXELFORMAT_ABGR8888;

	SDL_Surface *tmp=NULL,*tmp2=NULL;
	int format=0,sdlFormat=0;
	GLuint tex=0;

	if(srcBMPFile){
		tmp=SDL_LoadBMP(srcBMPFile);
		if(tmp==NULL) return 0;
		srcSurface=tmp;
	}

	if(srcSurface){
		width=srcSurface->w;
		height=srcSurface->h;
		switch(srcSurface->format->BitsPerPixel){
		case 32:
			format=GL_RGBA;
			sdlFormat=SDL_PIXELFORMAT_RGBA32;
			break;
		case 24:
			if(desiredFormat==GL_RGBA){
				format=GL_RGBA;
				sdlFormat=SDL_PIXELFORMAT_RGBA32;
			}else{
				format=GL_RGB;
				sdlFormat=SDL_PIXELFORMAT_RGB24;
			}
			break;
		case 8:
			switch(desiredFormat){
#ifndef USE_OPENGLES
			case GL_RED:
			case GL_GREEN:
			case GL_BLUE:
#endif
			case GL_ALPHA:
			case GL_LUMINANCE:
				format=desiredFormat;
				break;
			case GL_RGBA:
				format=GL_RGBA;
				sdlFormat=SDL_PIXELFORMAT_RGBA32;
				break;
			default:
				format=GL_RGB;
				sdlFormat=SDL_PIXELFORMAT_RGB24;
				break;
			}
			break;
		default: //unsupported
			if(tmp) SDL_FreeSurface(tmp);
			return 0;
		}
		if(sdlFormat){
			tmp2=SDL_ConvertSurfaceFormat(srcSurface,sdlFormat,0);
			if(!tmp2){
				if(tmp) SDL_FreeSurface(tmp);
				return 0;
			}
			srcData=tmp2->pixels;
			if(format==GL_RGBA && colorKey!=0){
				//new: color key
				unsigned char r=colorKey,g=colorKey>>8,b=colorKey>>16;
				for(int i=0,m=width*height;i<m;i++){
					if(((unsigned char*)srcData)[i*4]==r
						&& ((unsigned char*)srcData)[i*4+1]==g
						&& ((unsigned char*)srcData)[i*4+2]==b)
					{
						((unsigned char*)srcData)[i*4]=0;
						((unsigned char*)srcData)[i*4+1]=0;
						((unsigned char*)srcData)[i*4+2]=0;
						((unsigned char*)srcData)[i*4+3]=0;
					}
				}
			}
		}else{
			srcData=srcSurface->pixels;
		}
	}else{
		format=desiredFormat;
	}

	if(srcData){
		glGenTextures(1,&tex);
		glBindTexture(GL_TEXTURE_2D, tex);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
#ifndef USE_OPENGLES
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#endif

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, srcData);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	if(tmp) SDL_FreeSurface(tmp);
	if(tmp2) SDL_FreeSurface(tmp2);

	return tex;
}

void AddScreenKeyboard(float x,float y,float w,float h,int index,std::vector<float>& v,std::vector<unsigned short>& idx){
	unsigned short m=(unsigned short)(v.size()>>2);

	float tx=float(index & 0xFF)*SCREENKB_W;
	float ty=float(index>>8)*SCREENKB_H;

	float vv[16]={
		x,y,tx,ty,
		x+w,y,tx+SCREENKB_W,ty,
		x+w,y+h,tx+SCREENKB_W,ty+SCREENKB_H,
		x,y+h,tx,ty+SCREENKB_H,
	};

	unsigned short ii[6]={
		m,m+1,m+2,m,m+2,m+3,
	};

	v.insert(v.end(),vv,vv+16);
	idx.insert(idx.end(),ii,ii+6);
}

void AddEmptyHorizontalButton(float left,float top,float right,float bottom,std::vector<float>& v,std::vector<unsigned short>& idx){
	unsigned short m=(unsigned short)(v.size()>>2);

	float halfButtonSize=(bottom-top)*0.5f;

	float vv[32]={
		left,top,SCREENKB_W*3.0f,SCREENKB_H*3.0f,
		left+halfButtonSize,top,SCREENKB_W*3.5f,SCREENKB_H*3.0f,
		right-halfButtonSize,top,SCREENKB_W*3.5f,SCREENKB_H*3.0f,
		right,top,SCREENKB_W*4.0f,SCREENKB_H*3.0f,
		left,bottom,SCREENKB_W*3.0f,SCREENKB_H*4.0f,
		left+halfButtonSize,bottom,SCREENKB_W*3.5f,SCREENKB_H*4.0f,
		right-halfButtonSize,bottom,SCREENKB_W*3.5f,SCREENKB_H*4.0f,
		right,bottom,SCREENKB_W*4.0f,SCREENKB_H*4.0f,
	};

	unsigned short ii[18]={
		m,m+1,m+5,m,m+5,m+4,
		m+1,m+2,m+6,m+1,m+6,m+5,
		m+2,m+3,m+7,m+2,m+7,m+6,
	};

	v.insert(v.end(),vv,vv+32);
	idx.insert(idx.end(),ii,ii+18);
}

void DrawScreenKeyboard(const std::vector<float>& v,const std::vector<unsigned short>& idx,unsigned int color){
	if(idx.empty()) return;

	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, adhoc_screenkb_tex);

	glColor4ub(color, color>>8, color>>16, color>>24);
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

static void OnVideoResize(int width, int height){
	m_nResizeTime++;

	screenWidth=width;
	screenHeight=height;
	screenAspectRatio=float(screenWidth)/float(screenHeight);

	glViewport(0,0,screenWidth,screenHeight);
}

static bool SaveUserLevelFile(const char* format,bool useFormat){
	if(theApp->m_pDocument){
		char s0[256];
		const char* lps=NULL;

		if(useFormat){
			time_t t=time(NULL);
			tm *timeinfo=localtime(&t);
			strftime(s0,sizeof(s0),format,timeinfo);
			lps=s0;
		}else{
			lps=format;
		}

		u8string s;
		for(;;){
			char c=*(lps++);
			if(c==0) break;
			switch(c){
			case '\\':
			case '/':
			case ':':
			case '*':
			case '?':
			case '\"':
			case '<':
			case '>':
			case '|':
				continue;
			default:
				s.push_back(c);
				break;
			}
		}

		int m=s.size();
		if(m<4 || SDL_strcasecmp(s.c_str()+(m-4),".lev")) s+=".lev";

		bool ret=theApp->SaveFile(externalStoragePath+"/levels/"+s);
		if(ret){
			theApp->ShowToolTip(str(MyFormat(_("File saved to %s"))<<s));
		}else{
			theApp->ShowToolTip(str(MyFormat(_("Failed to save file %s"))<<s));
		}
	}

	return false;
}

static void ReloadCurrentLevel(){
	int idx=theApp->m_nCurrentLevel;
	if(!theApp->m_sLastFile.empty() && theApp->LoadFile(theApp->m_sLastFile)){
		theApp->m_nCurrentLevel=(idx>=0 && idx<(int)theApp->m_pDocument->m_objLevels.size())?idx:0;
	}else{
		theApp->LoadFile("data/levels/PuzzleBoy.lev");
		theApp->m_nCurrentLevel=0;
	}
}

SimpleMessageBox* CreateLevelChangedMsgBox(){
	SimpleMessageBox* msgBox=new SimpleMessageBox();

	msgBox->m_prompt=_("Level has been edited.\nIf you continue, unsaved changes will be lost.\n\nContinue?");
	msgBox->m_buttons.push_back(_("Yes"));
	msgBox->m_buttons.push_back(_("No"));
	msgBox->Create();
	msgBox->m_nDefaultValue=0;
	msgBox->m_nCancelValue=1;

	return msgBox;
}

static int SolveLevelCallback(void* userData,const LevelSolverState& progress){
	SimpleProgressScreen *progressScreen=(SimpleProgressScreen*)userData;

	progressScreen->progress=float(progress.nOpenedNodeCount)/float(progress.nAllNodeCount);
	return progressScreen->DrawAndDoEvents()?0:1;
}

static int SolveCurrentLevel(bool bTestMode){
	if(!theApp->m_view.empty()
		&& theApp->m_view[0]->m_objPlayingLevel)
	{
		PuzzleBoyLevelData *dat=theApp->m_pDocument->GetLevel(theApp->m_nCurrentLevel);
		if(dat){
			//create progress screen
			SimpleProgressScreen progressScreen;
			progressScreen.Create();

			//start solver
			PuzzleBoyLevel *lev=new PuzzleBoyLevel(*dat);
			lev->StartGame();
			u8string s;
			int ret=lev->SolveIt(s,&progressScreen,SolveLevelCallback);

			delete lev;

			//over
			progressScreen.Destroy();

			//show solution
			switch(ret){
			case 1:
				ret=LevelRecordScreen(_("Level Solver"),_("The solution is:"),s,true);
				if(ret>0){
					theApp->ApplyRecord(s,ret==2,bTestMode);
					return 0;
				}
				break;
			case 0:
				theApp->ShowToolTip(_("No solution"));
				break;
			case -1:
				theApp->ShowToolTip(_("Aborted"));
				break;
			default:
				theApp->ShowToolTip(_("Can't solve this level"));
				break;
			}
		}
	}

	return -1;
}

class MainMenuScreen:public SimpleListScreen{
public:
	static const int TestFeatureStart=7;
public:
	void OnDirty() override{
		ResetList();

		AddItem(_("Choose Level"));
		AddItem(_("Choose Level File"));
		AddItem(_("Level Record"));
		AddItem(_("Edit Level"));
		AddItem(_("Config"));
		AddItem(_("Exit Game"));

		//test feature
		AddEmptyItem();

		AddItem(_("Level Solver"));
		AddItem(_("Random Map"));
		AddItem("Random Map x10");
		AddItem(_("Save Temp Level File"));
		AddItem(_("Start Multiplayer"));
	}

	int OnClick(int index) override{
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
			//level record
			if(!theApp->m_view.empty()
				&& theApp->m_view[0]->m_objPlayingLevel)
			{
				u8string s=theApp->m_view[0]->m_objPlayingLevel->GetRecord();
				int ret=LevelRecordScreen(_("Level Record"),
					_("Copy record here or paste record\nand click 'Apply' button"),
					s);
				if(ret>0){
					theApp->ApplyRecord(s,ret==2);
					return 0;
				}
			}
			break;
		case 3:
			//edit level
			return 3;
			break;
		case 4:
			//config
			ConfigScreen().DoModal();
			m_bDirty=true;
			//recreate header in case of button size changed
			RecreateTitleBar();
			//resize the game screen in case of button size changed
			m_nResizeTime++;
			break;
		case 5:
			//exit game
			m_bRun=false;
			return 0;
			break;
		case TestFeatureStart:
			//ad-hoc solver test
			return SolveCurrentLevel(false);
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
						theApp->m_nCurrentLevel=0;
						if(theApp->m_bAutoSaveRandomMap) SaveUserLevelFile("rnd-%Y%m%d%H%M%S.lev",true);
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
						theApp->m_nCurrentLevel=0;
						if(theApp->m_bAutoSaveRandomMap) SaveUserLevelFile("rnd-%Y%m%d%H%M%S.lev",true);
						return 1;
					}
				}
			}
			break;
		case TestFeatureStart+3:
			//save temp file
			SaveUserLevelFile("tmp-%Y%m%d%H%M%S.lev",true);
			return 0;
			break;
		case TestFeatureStart+4:
			//start multiplayer!!! TEST!!
			return 2;
			break;
		}

		return -1;
	}

	int DoModal() override{
		//show
		m_titleBar.m_sTitle=_("Main Menu");
		return SimpleListScreen::DoModal();
	}
};

class EditMenuScreen:public SimpleListScreen{
public:
	void OnDirty() override{
		ResetList();

		AddItem(_("Choose Level"));
		AddItem(_("Choose Level File"));
		AddItem(_("Test Level"));
		AddItem(_("New Level File"));
		AddItem(_("Save Level"));
		AddItem(_("Config"));
		AddItem(_("Exit Editor"));

		AddEmptyItem();

		AddItem(str(MyFormat(_("Level Pack Name"))(": %s")<<
			toUTF8(theApp->m_pDocument->m_sLevelPackName)));
		AddItem(str(MyFormat(_("Level Name"))(": %s")<<
			toUTF8(theApp->m_pDocument->m_objLevels[theApp->m_nCurrentLevel]->m_sLevelName)));
		AddItem(_("Change Level Size"));
		AddItem(_("Add Level"));
		AddItem(_("Move Level"));
		AddItem(_("Remove Level"));
	}

	int OnClick(int index) override{
		m_nPressedIndex=index;

		switch(index){
		case 0:
			//choose level
			if(ChooseLevelScreen().DoModal()) return 3;
			break;
		case 1:
			//choose level file
			if(ChooseLevelFileScreen().DoModal()){
				ChooseLevelScreen().DoModal();
				return 3;
			}
			break;
		case 2:
			//test level
			return 4;
			break;
		case 3:
			//new level pack
			if(theApp->m_pDocument->m_bModified){
				delete m_msgBox;
				m_msgBox=CreateLevelChangedMsgBox();
			}else{
				return 32;
			}
			break;
		case 4:
			//save level
			{
				//get default file name
				u8string fileName=theApp->m_sLastFile;
				size_t n=fileName.find_last_of("\\/");
				if(n!=fileName.npos) fileName=fileName.substr(n+1);

				//show dialog
				if(!SimpleInputScreen(_("Save Level"),
					_("Please input level file name"),fileName)) break;

				if(fileName.empty()) break;
				SaveUserLevelFile(fileName.c_str(),false);
				return 0;
			}
			break;
		case 5:
			//config
			ConfigScreen().DoModal();
			m_bDirty=true;
			//recreate header in case of button size changed
			RecreateTitleBar();
			//resize the game screen in case of button size changed
			m_nResizeTime++;
			break;
		case 6:
			//exit editor
			if(theApp->m_pDocument->m_bModified){
				delete m_msgBox;
				m_msgBox=CreateLevelChangedMsgBox();
			}else{
				return 1;
			}
			break;
		case 8:
			//level pack name
			{
				u8string s=toUTF8(theApp->m_pDocument->m_sLevelPackName);
				if(!SimpleInputScreen(_("Level Pack Name"),
					_("Please input level pack name"),s)) break;
				theApp->m_pDocument->m_sLevelPackName=toUTF16(s);
				theApp->m_pDocument->SetModifiedFlag();
				m_bDirty=true;
			}
			break;
		case 9:
			//level name
			{
				u8string s=toUTF8(theApp->m_pDocument->m_objLevels[theApp->m_nCurrentLevel]->m_sLevelName);
				if(!SimpleInputScreen(_("Level Name"),
					_("Please input level name"),s)) break;
				theApp->m_pDocument->m_objLevels[theApp->m_nCurrentLevel]->m_sLevelName
					=theApp->m_view[0]->m_objPlayingLevel->m_sLevelName
					=toUTF16(s);
				theApp->m_pDocument->SetModifiedFlag();
				m_bDirty=true;
			}
			break;
		case 10:
			//change size
			{
				PuzzleBoyLevelFile *pDoc=theApp->m_pDocument;
				PuzzleBoyLevelData *lev=pDoc->m_objLevels[theApp->m_nCurrentLevel];

				ChangeSizeScreen frm(lev->m_nWidth,lev->m_nHeight);

				if(frm.DoModal()){
					if((frm.m_nWidth!=lev->m_nWidth || frm.m_nHeight!=lev->m_nHeight
						|| (frm.m_bPreserve && (frm.m_nXOffset!=0 || frm.m_nYOffset!=0))))
					{
						lev->Create(frm.m_nWidth,frm.m_nHeight,frm.m_bPreserve,frm.m_nXOffset,frm.m_nYOffset);
						pDoc->SetModifiedFlag();
						return 3;
					}
					return 0;
				}
			}
			break;
		case 11:
			//add level
			{
				PuzzleBoyLevelFile *pDoc=theApp->m_pDocument;
				int idx=theApp->m_nCurrentLevel;

				PuzzleBoyLevelData *lev=new PuzzleBoyLevelData();
				lev->CreateDefault();

				if(idx>=(int)pDoc->m_objLevels.size()-1){
					pDoc->m_objLevels.push_back(lev);
					idx=pDoc->m_objLevels.size()-1;
				}else{
					if(idx<0) idx=0; else idx++;
					pDoc->m_objLevels.insert(pDoc->m_objLevels.begin()+idx,lev);
				}

				theApp->m_nCurrentLevel=idx;
				pDoc->SetModifiedFlag();
				return 3;
			}
			break;
		case 12:
			//move level
			{
				char s0[32];
				sprintf(s0,"%d",theApp->m_nCurrentLevel+1);
				u8string s=s0;
				if(!SimpleInputScreen(_("Move Level"),
					str(MyFormat(_("Please input the destination level number"))(" (1-%d)")
					<<int(theApp->m_pDocument->m_objLevels.size())),
					s,"01234\n56789")) break;
				int n;
				if(sscanf(s.c_str(),"%d",&n)!=1) break;
				n--;
				int m=theApp->m_pDocument->m_objLevels.size();
				if(n>=0 && n<m){
					if(n==theApp->m_nCurrentLevel) return 0;

					PuzzleBoyLevelFile *pDoc=theApp->m_pDocument;
					PuzzleBoyLevelData *lev=pDoc->m_objLevels[theApp->m_nCurrentLevel];

					if(n<theApp->m_nCurrentLevel){
						for(int i=theApp->m_nCurrentLevel;i>n;i--){
							pDoc->m_objLevels[i]=pDoc->m_objLevels[i-1];
						}
					}else{
						for(int i=theApp->m_nCurrentLevel;i<n;i++){
							pDoc->m_objLevels[i]=pDoc->m_objLevels[i+1];
						}
					}

					pDoc->m_objLevels[n]=lev;
					pDoc->SetModifiedFlag();
					theApp->m_nCurrentLevel=n;
					return 3;
				}
			}
			break;
		case 13:
			//remove level
			delete m_msgBox;
			m_msgBox=new SimpleMessageBox();

			m_msgBox->m_prompt=_("Are you sure?");
			m_msgBox->m_buttons.push_back(_("Yes"));
			m_msgBox->m_buttons.push_back(_("No"));
			m_msgBox->Create();
			m_msgBox->m_nDefaultValue=0;
			m_msgBox->m_nCancelValue=1;

			break;
		}

		return -1;
	}

	int OnMsgBoxClick(int index) override{
		delete m_msgBox;
		m_msgBox=NULL;

		switch(m_nPressedIndex){
		case 3: //new file
			return index?-1:32;
			break;
		case 6: //exit
			if(index==0){
				ReloadCurrentLevel();
				return 1;
			}
			break;
		case 13: //remove level
			if(index==0){
				PuzzleBoyLevelFile *pDoc=theApp->m_pDocument;
				int idx=theApp->m_nCurrentLevel;

				if(pDoc->m_objLevels.size()==1){
					pDoc->GetLevel(0)->CreateDefault();
					idx=0;
				}else if(pDoc->m_objLevels.empty()){
					PuzzleBoyLevelData *lev=new PuzzleBoyLevelData();
					lev->CreateDefault();
					pDoc->m_objLevels.push_back(lev);
					idx=0;
				}else{
					delete pDoc->m_objLevels[idx];
					pDoc->m_objLevels.erase(pDoc->m_objLevels.begin()+idx);
					if(idx>=(int)pDoc->m_objLevels.size()) idx=pDoc->m_objLevels.size()-1;
				}

				theApp->m_nCurrentLevel=idx;
				pDoc->SetModifiedFlag();
				return 3;
			}
			break;
		}

		return -1;
	}

	int DoModal() override{
		//save changes
		if(!theApp->m_view.empty() && theApp->m_view[0] && theApp->m_view[0]->m_bEditMode){
			theApp->m_view[0]->SaveEdit();
		}

		//show
		m_titleBar.m_sTitle=_("Edit Level");
		return SimpleListScreen::DoModal();
	}
public:
	int m_nPressedIndex;
};

class TestMenuScreen:public SimpleListScreen{
public:
	static const int TestFeatureStart=4;
public:
	void OnDirty() override{
		ResetList();

		AddItem(_("Level Record"));
		AddItem(_("Config"));
		AddItem(_("Quit to Editor"));

		//test feature
		AddEmptyItem();

		AddItem(_("Level Solver"));
	}

	int OnClick(int index) override{
		switch(index){
		case 0:
			//level record
			if(!theApp->m_view.empty()
				&& theApp->m_view[0]->m_objPlayingLevel)
			{
				u8string s=theApp->m_view[0]->m_objPlayingLevel->GetRecord();
				int ret=LevelRecordScreen(_("Level Record"),
					_("Copy record here or paste record\nand click 'Apply' button"),
					s);
				if(ret>0){
					theApp->ApplyRecord(s,ret==2,true);
					return 0;
				}
			}
			break;
		case 1:
			//config
			ConfigScreen().DoModal();
			m_bDirty=true;
			//recreate header in case of button size changed
			RecreateTitleBar();
			//resize the game screen in case of button size changed
			m_nResizeTime++;
			break;
		case 2:
			//back to edit mode
			return 3;
			break;
		case TestFeatureStart:
			//ad-hoc solver test
			return SolveCurrentLevel(true);
			break;
		}

		return -1;
	}

	int DoModal() override{
		//show
		m_titleBar.m_sTitle=_("Test Level");
		return SimpleListScreen::DoModal();
	}
};

class MainMenuButton:public virtual MultiTouchView{
public:
	void OnTimer(){
		if(theApp->m_bShowMainMenuButton){
			MultiTouchViewStruct *view=theApp->touchMgr.FindView(this);
			float f=float(theApp->m_nButtonSize)/float(screenHeight);
			if(view){
				view->left=screenAspectRatio-f;
				view->top=0;
				view->right=screenAspectRatio;
				view->bottom=f;
			}else{
				theApp->touchMgr.AddView(screenAspectRatio-f,0,screenAspectRatio,f,
					MultiTouchViewFlags::AcceptDragging,this,0);
			}
		}else{
			theApp->touchMgr.RemoveView(this);
		}
	}
	void OnMouseEvent(int which,int state,int xMouse,int yMouse,int nFlags,int nType) override{
		if(nType==SDL_MOUSEBUTTONUP && state==SDL_BUTTON_LMASK && theApp->m_bShowMainMenuButton){
			SDL_Event evt=event;
			evt.type=SDL_KEYDOWN;
			evt.key.state=SDL_PRESSED;
			evt.key.repeat=0;
			evt.key.keysym.scancode=SDL_SCANCODE_MENU;
			evt.key.keysym.sym=SDLK_MENU;
			evt.key.keysym.mod=0;
			SDL_PushEvent(&evt);
		}
	}
} *adhoc_menu_btn=NULL;

static bool OnKeyDown(int nChar,int nFlags){
	if(theApp->OnKeyDown(nChar,nFlags)) return true;

	//ad-hoc menu experiment
	if(nChar==SDLK_MENU || nChar==SDLK_APPLICATION
#ifndef ANDROID
		|| nChar==SDLK_ESCAPE
#endif
		)
	{
		theApp->touchMgr.ResetDraggingState();
		int ret;
		bool b=(!theApp->m_view.empty() && theApp->m_view[0]);
		if(b && theApp->m_view[0]->m_bEditMode){
			ret=EditMenuScreen().DoModal();
		}else if(b && theApp->m_view[0]->m_bTestMode){
			ret=TestMenuScreen().DoModal();
		}else{
			ret=MainMenuScreen().DoModal();
		}
		switch(ret){
		case 1: //single player
		case 2: //multiplayer
			theApp->StartGame(ret);
			break;
		case 3: //edit
			theApp->StartGame(1,true);
			break;
		case 4: //test
			theApp->StartGame(1,false,true);
			break;
		case 32: //new file
			theApp->m_pDocument->CreateNew();
			theApp->m_nCurrentLevel=0;
			theApp->m_sLastFile.clear();
			theApp->m_sLastRecord.clear();
			theApp->StartGame(1,true);
			break;
		}
		return false;
	}

	return false;
}

static void OnKeyUp(int nChar,int nFlags){
	theApp->OnKeyUp(nChar,nFlags);
}

static void OnAutoSave(){
	if(theApp && theApp->m_bAutoSave && theApp->m_view[0] && !theApp->m_sLastFile.empty()){
		theApp->m_nCurrentLevel=theApp->m_view[0]->m_nCurrentLevel;
		theApp->m_sLastRecord=theApp->m_view[0]->m_objPlayingLevel->GetRecord();
		theApp->SaveConfig(externalStoragePath+"/PuzzleBoy.cfg");
	}
}

static int MyEventFilter(void *userdata, SDL_Event *evt){
	switch(evt->type){
	case SDL_QUIT:
		m_bRun=false;
		break;
	case SDL_APP_TERMINATING:
	case SDL_APP_WILLENTERBACKGROUND:
		OnAutoSave();
		break;
	case SDL_APP_LOWMEMORY:
		printf("[main] Fatal Error: Program received SDL_APP_LOWMEMORY! Program will abort\n");
		OnAutoSave();
		abort();
		break;
	case SDL_WINDOWEVENT:
		switch(evt->window.event){
		case SDL_WINDOWEVENT_EXPOSED:
			m_nIdleTime=0;
			break;
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			m_nIdleTime=0;
			OnVideoResize(
				evt->window.data1,
				evt->window.data2);
			break;
		}
		break;
	case SDL_KEYDOWN:
		//check Alt+F4 or Ctrl+Q exit event (for all platforms)
		if((evt->key.keysym.sym==SDLK_F4 && (evt->key.keysym.mod & KMOD_ALT)!=0)
			|| (evt->key.keysym.sym==SDLK_q && (evt->key.keysym.mod & KMOD_CTRL)!=0))
		{
			m_bRun=false;
		}
		break;
	}

	return 1;
}

int main(int argc,char** argv){
	//init SDL
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)<0){
		printf("[main] Fatal Error: Can't initialize SDL video or timer!\n");
		abort();
	}

	/*if(SDL_InitSubSystem(SDL_INIT_JOYSTICK)>0){
		printf("[main] Error: Can't initialize SDL joystick!\n");
	}*/

	//init file system
	initPaths();

	//init freetype
	if(!FreeType_Init()){
		printf("[main] Fatal Error: Can't initialize FreeType!\n");
		abort();
	}

	//create main object
	theApp=new PuzzleBoyApp;

	//register event callback
	SDL_SetEventFilter(MyEventFilter,NULL);

	//init random number
	{
#if 0
		//random number debug
		unsigned int seed[4]={0xD0CF11E,0xDEADBEEF,0xBADC0DE,0xCAFEBABE};
#else
		unsigned int seed[4];
		unsigned long long t=time(NULL);
		seed[0]=(unsigned int)t;
		seed[1]=(unsigned int)(t>>32);

		t=SDL_GetPerformanceCounter();
		seed[2]=(unsigned int)t;
		seed[3]=(unsigned int)(t>>32);
#endif

		theApp->m_objMainRnd.Init(seed,sizeof(seed)/sizeof(unsigned int));
	}

	//load config
	theApp->LoadConfig(externalStoragePath+"/PuzzleBoy.cfg");

	//load locale
	theApp->LoadLocale();

	//load record file
	theApp->m_objRecordMgr.LoadFile((externalStoragePath+"/PuzzleBoyRecord.dat").c_str());

	//load level file and progress
	do{
		if(theApp->m_bAutoSave){
			int tmp=theApp->m_nCurrentLevel;
			if(theApp->LoadFile(theApp->m_sLastFile)){
				if(tmp>=0 && tmp<(int)theApp->m_pDocument->m_objLevels.size()){
					theApp->m_nCurrentLevel=tmp;
				}else{
					printf("[main] Error: Level number specified by autosave out of range\n");
					theApp->m_sLastRecord.clear();
				}
				break;
			}else{
				printf("[main] Error: Failed to load level file specified by autosave\n");
				theApp->m_sLastRecord.clear();
			}
		}
		theApp->m_nCurrentLevel=0;
		if(!theApp->LoadFile("data/levels/PuzzleBoy.lev")){
			printf("[main] Error: Failed to load default level file\n");
		}
	}while(0);

#ifdef ANDROID
	//experimental orientation aware
	int windowFlags=SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN;
	{
		SDL_DisplayMode mode;

		if(SDL_GetDesktopDisplayMode(0,&mode)<0){
			printf("[main] Error: Can't get Android screen resolution!\n");
			//fallback
			screenWidth=800;
			screenHeight=480;
		}else{
			screenWidth=mode.w;
			screenHeight=mode.h;
		}
	}
#else
	int windowFlags=SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
	screenWidth=800;
	screenHeight=480;
#endif

#ifdef USE_OPENGLES
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 6 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
#else
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
		screenWidth,screenHeight,windowFlags))==NULL)
	{
		printf("[main] Fatal Error: Can't create SDL window!\n");
		abort();
	}

	glContext=SDL_GL_CreateContext(mainWindow);
	if(glContext==NULL){
		printf("[main] Fatal Error: Can't create OpenGL context!\n");
		abort();
	}

	//load ad-hoc textures
	adhoc_screenkb_tex=CreateGLTexture(0,0,0,GL_CLAMP_TO_EDGE,GL_LINEAR,GL_LINEAR,"data/gfx/adhoc.bmp",NULL,NULL,0);
	adhoc3_tex=CreateGLTexture(0,0,GL_RGBA,GL_CLAMP_TO_EDGE,GL_LINEAR,GL_LINEAR,"data/gfx/adhoc3.bmp",NULL,NULL,0xFFFF00FF);

	if(adhoc_screenkb_tex==0 || adhoc3_tex==0){
		printf("[main] Fatal Error: Can't find necessary data! Make sure the working directory is correct!\n");
		abort();
	}

	//try to load FreeType font
	if(theApp->m_bInternationalFont){
		mainFontFile=new SimpleFontFile;
		if(mainFontFile->LoadFile("data/font/DroidSansFallback.ttf")
#ifdef ANDROID
			|| mainFontFile->LoadFile("/system/fonts/DroidSansFallback.ttf")
#endif
			)
		{
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

	//load autosave
	if(theApp->m_bAutoSave && !theApp->m_sLastRecord.empty()){
		theApp->m_view[0]->m_objPlayingLevel->ApplyRecord(theApp->m_sLastRecord);
	}

	//create ad-hoc main menu button
	adhoc_menu_btn=new MainMenuButton;

	//message box
	SimpleMessageBox *msgBox=NULL;

	while(m_bRun){
		//game logic
		UpdateIdleTime(theApp->OnTimer());

		adhoc_menu_btn->OnTimer();

		//clear and draw (if not idle, otherwise only draw after 32 frames)
		if(NeedToDrawScreen()){
			ClearScreen();

			theApp->Draw();

			SetProjectionMatrix(1);

			//TEST
			{
				MyFormat fmt(_("Pack: %s"));
				fmt<<toUTF8(theApp->m_pDocument->m_sLevelPackName);

				int m=theApp->m_view.size();

				if(m==1){
					PuzzleBoyLevelView *view=theApp->m_view[0];

					fmt("\n")(_("Level %d: %s"))<<(view->m_nCurrentLevel+1)
						<<toUTF8(view->m_objPlayingLevel->m_sLevelName);

					if(!view->m_bEditMode){
						fmt("\n")(_("Moves: %d"))<<view->m_objPlayingLevel->m_nMoves;

						if(view->m_nCurrentBestStep>0){
							fmt(" - ")(_("Best: %d (%s)"))<<view->m_nCurrentBestStep
								<<view->m_sCurrentBestStepOwner;
						}
					}
				}else if(m>=2){
					fmt("\nLevel: %d vs %d\nMoves: %d vs %d")<<(theApp->m_view[0]->m_nCurrentLevel+1)
						<<(theApp->m_view[1]->m_nCurrentLevel+1)
						<<(theApp->m_view[0]->m_objPlayingLevel->m_nMoves)<<(theApp->m_view[1]->m_objPlayingLevel->m_nMoves);
				}

				mainFont->DrawString(str(fmt),0.0f,0.0f,float(screenWidth-theApp->m_nButtonSize*4),float(screenHeight),1.0f,
					DrawTextFlags::Multiline | DrawTextFlags::Bottom | DrawTextFlags::AutoSize,
					SDL_MakeColor(255,255,255,255));
			}

			//draw message box
			if(msgBox) msgBox->Draw();

			ShowScreen();
		}

		while(SDL_PollEvent(&event)){
			//check msgbox event
			if(msgBox && msgBox->OnEvent()){
				int ret=msgBox->m_nValue;
				if(ret>=0){
					delete msgBox;
					msgBox=NULL;

					if(ret==0){
						ReloadCurrentLevel();
						theApp->StartGame(1);
					}
				}
				continue;
			}

			//check multi-touch event
			if(theApp->touchMgr.OnEvent()){
				m_nIdleTime=0;
				continue;
			}

			switch(event.type){
			case SDL_KEYUP:
				switch(event.key.keysym.sym){
				case SDLK_AC_BACK: event.key.keysym.sym=SDLK_ESCAPE; break;
				}
				m_nIdleTime=0;
				OnKeyUp(
					event.key.keysym.sym,
					event.key.keysym.mod);

				if(m_bKeyDownProcessed) m_bKeyDownProcessed=false;
#ifdef ANDROID
				//chcek exit event (Android only)
				else if(event.key.keysym.sym==SDLK_ESCAPE){
					bool b=(!theApp->m_view.empty() && theApp->m_view[0]);
					if(b && theApp->m_view[0]->m_bEditMode){
						//exit edit mode
						if(theApp->m_pDocument->m_bModified){
							delete msgBox;
							msgBox=CreateLevelChangedMsgBox();
						}else{
							theApp->StartGame(1);
						}
					}else if(b && theApp->m_view[0]->m_bTestMode){
						//exit test mode
						theApp->StartGame(1,true);
					}else if(theApp->m_bToolTipIsExit){
						m_bRun=false;
					}else{
						theApp->ShowToolTip(_("Press again to exit"),true);
					}
				}
#endif
				break;
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym){
				case SDLK_AC_BACK: event.key.keysym.sym=SDLK_ESCAPE; break;
				}
				m_nIdleTime=0;
				if(OnKeyDown(
					event.key.keysym.sym,
					event.key.keysym.mod)) m_bKeyDownProcessed=true;

				break;
			}
		}

		WaitForNextFrame();
	}

	//save progress at exit
	OnAutoSave();

	//destroy everything
	delete adhoc_menu_btn;
	adhoc_menu_btn=NULL;

	delete msgBox;
	msgBox=NULL;

	glDeleteTextures(1,&adhoc_screenkb_tex);
	glDeleteTextures(1,&adhoc3_tex);

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

