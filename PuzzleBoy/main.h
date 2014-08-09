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
void OnVideoResize(int width, int height);

//draw FPS and other various overlay
//lpIdleTime: optional variable, if the overlay is dirty then it set idle time to 0
void ShowScreen(int* lpIdleTime=0);

unsigned int CreateGLTexture(int width,int height,int desiredFormat,int wrap,int minFilter,int magFilter,const char* srcBMPFile,SDL_Surface* srcSurface,const void* srcData);

void AddScreenKeyboard(float x,float y,float w,float h,int index,std::vector<float>& v,std::vector<unsigned short>& idx);
void AddEmptyHorizontalButton(float left,float top,float right,float bottom,std::vector<float>& v,std::vector<unsigned short>& idx);
void DrawScreenKeyboard(const std::vector<float>& v,const std::vector<unsigned short>& idx);

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

const int SCREEN_KEYBOARD_EMPTY=0x303;

extern int screenWidth;
extern int screenHeight;
extern int m_nResizeTime;
extern float screenAspectRatio;

extern bool m_bRun;

class SimpleFontFile;
class SimpleBaseFont;

extern SimpleFontFile *mainFontFile;
extern SimpleBaseFont *mainFont;
extern SimpleBaseFont *titleFont;

void WaitForNextFrame();

