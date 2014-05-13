#pragma once

#include "SimpleText.h"
#include <map>

bool FreeType_Init();
void FreeType_Quit();

struct SimpleFontFileData;
class SimpleFont;

class SimpleFontFile{
	friend class SimpleFont;
public:
	SimpleFontFile();
	~SimpleFontFile();

	bool LoadFile(const u8string& fileName);
	void Destroy();
private:
	SimpleFontFileData *data;
};

class SimpleFont:public SimpleBaseFont{
public:
	SimpleFont(SimpleFontFile& font,float fontSize);
	virtual ~SimpleFont();

	virtual void Destroy();
	virtual void Reset();

	virtual void UpdateTexture();

	virtual bool AddChar(int c);
	virtual bool AddGlyph(int glyphIndex,bool saveBitmap); //internal function

	virtual bool GetCharMetric(int c,SimpleFontGlyphMetric& metric);
private:
	SimpleFontFileData *data; //weak reference, don't delete
	int fontSize;

	std::map<int,SimpleFontGlyphMetric> glyphMap;

	int bmWShift,bmHShift;
	int bmWidth,bmHeight;
	int bmCurrentX,bmCurrentY,bmRowHeight; //FIXME: naive algorithm, use rectangle packer instead
	unsigned char* bitmap;
	bool bitmapDirty;

	bool AddBitmap(const void* /*FT_Bitmap* */ bm_,SimpleFontGlyphMetric& metric);
};
