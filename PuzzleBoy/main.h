#pragma once

#include <vector>

struct SDL_Surface;

const float SQRT_1_2=0.707106781f;

const float m_fBorderSize=0.125f;
const float m_fOverlaySize=0.166666667f;
const float m_fRotateBlockCenterSize=0.2f;
const float m_fPlayerSize=0.375f;

//ad-hoc
void SetProjectionMatrix(int idx=0);

void ClearScreen();

//draw FPS and other various overlay
//will update global idle time
void ShowScreen();

//width and height are used only when input data is srcData
//input data priority: srcBMPFile>srcSurface>srcData
//colorKey is used only when desiredFormat==GL_RGBA and colorKey!=0
//bit 0-23 of colorKey is red, green, blue, respectively
unsigned int CreateGLTexture(int width,int height,int desiredFormat,int wrap,int minFilter,int magFilter,const char* srcBMPFile,SDL_Surface* srcSurface,const void* srcData,unsigned int colorKey);

void AddScreenKeyboard(float x,float y,float w,float h,int index,std::vector<float>& v,std::vector<unsigned short>& idx);
void AddEmptyHorizontalButton(float left,float top,float right,float bottom,std::vector<float>& v,std::vector<unsigned short>& idx);
void DrawScreenKeyboard(const std::vector<float>& v,const std::vector<unsigned short>& idx,unsigned int color=-1);

const float SCREENKB_W=1.0f/8.0f;
const float SCREENKB_H=1.0f/4.0f;

const int SCREEN_KEYBOARD_UP=0x1;
const int SCREEN_KEYBOARD_DOWN=0x101;
const int SCREEN_KEYBOARD_LEFT=0x100;
const int SCREEN_KEYBOARD_RIGHT=0x102;
const int SCREEN_KEYBOARD_UNDO=0x0;
const int SCREEN_KEYBOARD_REDO=0x2;
const int SCREEN_KEYBOARD_SWITCH=0x103;
const int SCREEN_KEYBOARD_RESTART=0x3;
const int SCREEN_KEYBOARD_YES=0x200;
const int SCREEN_KEYBOARD_NO=0x201;
const int SCREEN_KEYBOARD_OPEN=0x202;
const int SCREEN_KEYBOARD_SEARCH=0x203;
const int SCREEN_KEYBOARD_MORE=0x300;
const int SCREEN_KEYBOARD_FAST_FORWARD=0x301;
const int SCREEN_KEYBOARD_PLAY=0x302;
const int SCREEN_KEYBOARD_EMPTY=0x303;
const int SCREEN_KEYBOARD_COPY=0x4;
const int SCREEN_KEYBOARD_PASTE=0x5;
const int SCREEN_KEYBOARD_ZOOM_IN=0x6;
const int SCREEN_KEYBOARD_ZOOM_OUT=0x7;
const int SCREEN_KEYBOARD_DELETE=0x104;
const int SCREEN_KEYBOARD_BACKSPACE=0x105;

extern int screenWidth;
extern int screenHeight;
extern int m_nResizeTime;
extern float screenAspectRatio;

extern bool m_bRun;

extern int m_nIdleTime;

inline void UpdateIdleTime(bool bDirty){
	if(bDirty) m_nIdleTime=0;
	else if((++m_nIdleTime)>=64) m_nIdleTime=32;
}

inline bool NeedToDrawScreen(){
	return m_nIdleTime<=32;
}

class SimpleFontFile;
class SimpleBaseFont;

extern SimpleFontFile *mainFontFile;
extern SimpleBaseFont *mainFont;
extern SimpleBaseFont *titleFont;

void WaitForNextFrame();

class SimpleMessageBox;

SimpleMessageBox* CreateLevelChangedMsgBox();
