#pragma once

#include "SimpleText.h"
#include "UTF8-16.h"
#include <vector>
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
	int LoadFiles(const std::vector<u8string>& files);
	void Destroy();
private:
	std::vector<SimpleFontFileData*> data;
};

class SimpleFont:public SimpleBaseFont{
public:
	SimpleFont(SimpleFontFile& font,float fontSize);
	virtual ~SimpleFont();

	void Destroy() override;
	void Reset() override;

	void UpdateTexture() override;

	bool AddChar(int c) override;

	bool GetCharMetric(int c,SimpleFontGlyphMetric& metric) override;
private:
	//internal function
	bool AddGlyph(int c,int fontIndex,int glyphIndex);

	SimpleFontFile *fontFile; //weak reference, don't delete
	int fontSize;

	//((fontIndex<<26)|glyphIndex) ==> metric
	std::map<int,SimpleFontGlyphMetric> glyphMap;

	int bmWShift,bmHShift;
	int bmWidth,bmHeight;
	int bmCurrentX,bmCurrentY,bmRowHeight; //FIXME: naive algorithm, use rectangle packer instead
	unsigned char* bitmap;
	bool bitmapDirty;

	bool AddBitmap(const void* /*FT_Bitmap* */ bm_,SimpleFontGlyphMetric& metric);
};
