#pragma once

#include "UTF8-16.h"
#include <SDL.h>

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
