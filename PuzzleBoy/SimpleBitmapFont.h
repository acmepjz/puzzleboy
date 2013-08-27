#pragma once

#include "UTF8-16.h"
#include <SDL.h>
#include <vector>

namespace DrawTextFlags{
	const int Left=0;
	const int Center=1;
	const int Right=2;

	const int Top=0;
	const int VCenter=4;
	const int Bottom=8;

	const int Multiline=16;

	const int WordWrap=32;
}

//experimental
class SimpleBitmapText{
public:
	SimpleBitmapText();

	int Width() const{return ww;}
	int Height() const{return yy+1;}

	int NewStringIndex();
	void AddString(const u8string& str,float x,float y,float w,float h,float size,int flags);

	void Draw(SDL_Color color,int start=0,int count=-1);

	void clear();
	bool empty() const{return v.empty();}

private:
	std::vector<float> v;
	std::vector<unsigned short> idx;
	std::vector<int> stringIdx;
	int xx,yy,ww;
	int nCount,nRowStartIndex;

	void AddChar(int c,float x,float w,float size,int flags);
	void AdjustVerticalPosition(int start,float y,float h,float size,int flags);
};

class SimpleBitmapFont{
public:
	SimpleBitmapFont();
	~SimpleBitmapFont();

	void Create(SDL_Surface* surf);
	void Destroy();

	void BeginDraw();
	void DrawString(const u8string& str,float x,float y,float w,float h,float size,int flags,SDL_Color color);
	void EndDraw();
private:
	unsigned int m_Tex;
	bool m_bDrawing;
};
