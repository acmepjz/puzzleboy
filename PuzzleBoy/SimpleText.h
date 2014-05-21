#pragma once

#include "UTF8-16.h"
#include <SDL.h>
#include <vector>

struct SimpleFontGlyphMetric{
	//in texture coords
	float srcX;
	float srcY;
	float srcWidth;
	float srcHeight;

	//in pixels
	float destX;
	float destY;
	float destWidth;
	float destHeight;
	float advanceX;
};

class SimpleBaseFont{
public:
	SimpleBaseFont();
	virtual ~SimpleBaseFont();

	unsigned int GetTexture() const {return m_Tex;} //test only
	float GetFontAscender() const {return fontAscender;}
	float GetFontDescender() const {return fontDescender;}
	float GetFontHeight() const {return fontHeight;}
	float GetFontAdvance() const {return fontAdvance;}

	virtual void Destroy();
	virtual void Reset();

	virtual void UpdateTexture();

	virtual bool AddChar(int c);
	virtual bool AddGlyph(int glyphIndex,bool saveBitmap); //internal function

	void BeginDraw();
	void DrawString(const u8string& str,float x,float y,float w,float h,float scale,int flags,SDL_Color color);
	void EndDraw();

	virtual bool GetCharMetric(int c,SimpleFontGlyphMetric& metric);
protected:
	unsigned int m_Tex;
	bool m_bDrawing;

	float fontAscender;
	float fontDescender;
	float fontHeight;
	float fontAdvance;
};

namespace DrawTextFlags{
	const int Left=0;
	const int Center=1;
	const int Right=2;

	const int Top=0;
	const int VCenter=4;
	const int Bottom=8;

	const int Multiline=16;

	const int WordWrap=32; //currently unsupported

	const int EndEllipsis=64; //currently unsupported
	const int WordEllipsis=128; //currently unsupported
	const int PathEllipsis=256; //currently unsupported

	const int AutoSize=512; //currently only horizontal autosize supported, and is buggy
}

class SimpleInputScreen;

//experimental
class SimpleText{
	friend class SimpleInputScreen;
public:
	SimpleText();

	//int Width() const{return ww;}
	//int Height() const{return yy+1;}

	int NewStringIndex();
	void AddString(SimpleBaseFont *font,const u8string& str,float x,float y,float w,float h,float scale,int flags);

	void Draw(SDL_Color color,int start=0,int count=-1);

	void clear();
	bool empty() const{return v.empty();}

private:
	std::vector<float> v;
	std::vector<unsigned short> idx;
	std::vector<int> stringIdx;
public:
	float xx,yy,ww; //internal use only
private:
	int nCount,nRowStartIndex;

public:
	//internal advanced function
	void AddChar(SimpleBaseFont *font,int c,float x,float w,float scale,int flags);

	//internal advanced function
	void AdjustVerticalPosition(SimpleBaseFont *font,int start,float y,float h,float scale,int flags);
};
