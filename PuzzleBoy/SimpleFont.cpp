#include "SimpleFont.h"
#include "FileSystem.h"
#include "main.h"

#include <stdlib.h>
#include <string.h>

#include "include_sdl.h"
#include "include_gl.h"

#include <ft2build.h>
#include FT_FREETYPE_H

static FT_Library ft_lib=NULL;

bool FreeType_Init(){
	if(ft_lib) return true;
	else return FT_Init_FreeType(&ft_lib)==0;
}

void FreeType_Quit(){
	FT_Done_FreeType(ft_lib);
	ft_lib=NULL;
}

struct SimpleFontFileData{
	FT_Face face;
	FT_Open_Args args;
	FT_StreamRec stream;
	int lastSize;

	static unsigned long Read(FT_Stream stream, unsigned long offset, unsigned char* buffer, unsigned long count){
		u8file *f=(u8file*)stream->descriptor.pointer;

		if(u8fseek(f,offset,SEEK_SET)) return count?0:1;

		if(count) return u8fread(buffer,1,count,f);
		else return 0;
	}

	inline void SetSize(int size){
		if(size!=lastSize){
			FT_Set_Char_Size(face,0,size,96,96);
			lastSize=size;
		}
	}
};

SimpleFontFile::SimpleFontFile()
:data(NULL)
{
}

SimpleFontFile::~SimpleFontFile(){
	Destroy();
}

bool SimpleFontFile::LoadFile(const u8string& fileName){
	Destroy();

	u8file *f=u8fopen(fileName.c_str(),"rb");
	if(f==NULL) return false;

	data=new SimpleFontFileData;
	memset(data,0,sizeof(SimpleFontFileData));

	data->args.flags=FT_OPEN_STREAM;
	data->args.stream=&(data->stream);

	u8fseek(f,0,SEEK_END);
	data->stream.size=u8ftell(f);
	u8fseek(f,0,SEEK_SET);
	data->stream.descriptor.pointer=f;
	data->stream.read=SimpleFontFileData::Read;

	if(FT_Open_Face(ft_lib,&data->args,0,&data->face)){
		u8fclose(f);
		delete data;
		data=NULL;
		return false;
	}

	return true;
}

void SimpleFontFile::Destroy(){
	if(data){
		FT_Done_Face(data->face);
		u8fclose((u8file*)data->stream.descriptor.pointer);

		delete data;
		data=NULL;
	}
}

SimpleFont::SimpleFont(SimpleFontFile& font,float fontSize)
:data(font.data),fontSize(int(fontSize*64.0f+0.5f))
,bmWShift(0),bmHShift(0)
,bmWidth(0),bmHeight(0)
,bmCurrentX(0),bmCurrentY(0),bmRowHeight(0)
,bitmap(NULL)
,bitmapDirty(false)
{
	data->SetSize(this->fontSize);

	fontAscender=float(data->face->size->metrics.ascender)*(1.0f/64.0f);
	fontDescender=float(-data->face->size->metrics.descender)*(1.0f/64.0f);
	fontHeight=float(data->face->size->metrics.height)*(1.0f/64.0f);
	fontAdvance=float(data->face->size->metrics.max_advance)*(1.0f/64.0f);
}

SimpleFont::~SimpleFont(){
	Destroy();
}

void SimpleFont::Destroy(){
	SimpleBaseFont::Destroy();

	data=NULL;
	fontSize=0;

	if(bitmap){
		free(bitmap);
		bitmap=NULL;
	}

	bmWShift=0;
	bmHShift=0;
	bmWidth=0;
	bmHeight=0;
	bmCurrentX=0;
	bmCurrentY=0;
	bmRowHeight=0;

	bitmapDirty=false;

	glyphMap.clear();
}

void SimpleFont::Reset(){
	//TODO:
}

void SimpleFont::UpdateTexture(){
	if(bitmap && bitmapDirty){
		if(m_Tex){
			glBindTexture(GL_TEXTURE_2D, m_Tex);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bmWidth, bmHeight, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap);
			glBindTexture(GL_TEXTURE_2D, 0);
		}else{
			m_Tex=CreateGLTexture(bmWidth,bmHeight,GL_ALPHA,GL_CLAMP_TO_EDGE,GL_LINEAR,GL_LINEAR,NULL,NULL,bitmap,0);
		}

		bitmapDirty=false;
	}
}

bool SimpleFont::AddChar(int c){
	if(c=='\t' || c=='\b' || c=='\n' || c=='\r'){
		//do nothing
		return true;
	}

	//get glyph index
	int glyphIndex=FT_Get_Char_Index(data->face,c);

	return AddGlyph(glyphIndex,
		c!=' ' && c!=0x3000);
}

bool SimpleFont::AddGlyph(int glyphIndex,bool saveBitmap){
	//check if we already have this glyph
	if(glyphMap.find(glyphIndex)!=glyphMap.end()) return true;

	//render glyph
	data->SetSize(fontSize);
	if(FT_Load_Glyph(data->face,glyphIndex,FT_LOAD_RENDER)){
		return false;
	}

	FT_GlyphSlot slot=data->face->glyph;
	SimpleFontGlyphMetric metric={};

	if(saveBitmap){
		if(!AddBitmap(&(slot->bitmap),metric)){
			//FIXME: the bitmap is full
			return false;
		}

		metric.destX=float(slot->bitmap_left);
		metric.destY=float(-slot->bitmap_top);
	}
	metric.advanceX=float(slot->advance.x)*(1.0f/64.0f);

	//add it
	glyphMap[glyphIndex]=metric;

	//over
	return true;
}

bool SimpleFont::AddBitmap(const void* /*FT_Bitmap* */ bm_,SimpleFontGlyphMetric& metric){
	const FT_Bitmap *bm=(const FT_Bitmap*)bm_;

	int w=bm->width,h=bm->rows;

	if(w<=0 || h<=0){
		//no bitmap at all
		return true;
	}

	if(bitmap==NULL){
		//create bitmap;
		bmWShift=10;
		bmHShift=10;
		bmWidth=1L<<bmWShift;
		bmHeight=1L<<bmHShift;

		int m=1L<<(bmWShift+bmHShift);
		bitmap=(unsigned char*)malloc(m);
		memset(bitmap,0,m);

		bmCurrentX=0;
		bmCurrentY=0;
		bmRowHeight=0;
	}

	metric.srcWidth=float(w)/float(bmWidth);
	metric.srcHeight=float(h)/float(bmHeight);
	metric.destWidth=float(w);
	metric.destHeight=float(h);

	//calculate position
	if(w>=bmWidth-bmCurrentX){
		//start a new row
		bmCurrentX=0;
		bmCurrentY+=bmRowHeight;
		bmRowHeight=0;
	}
	if(w>=bmWidth-bmCurrentX || h>=bmHeight-bmCurrentY){
		//TODO: too big
		return false;
	}
	if(h>=bmRowHeight) bmRowHeight=h+1; //update row height, add 1 for black border

	//copy data
	metric.srcX=float(bmCurrentX)/float(bmWidth);
	metric.srcY=float(bmCurrentY)/float(bmHeight);
	unsigned char* lpSrc=bm->buffer+(bm->pitch>0?0:bm->pitch*(1-h));
	unsigned char* lpDest=bitmap+((bmCurrentY<<bmWShift)+bmCurrentX);
	for(int i=0;i<h;i++){
		memcpy(lpDest,lpSrc,w);
		lpSrc+=bm->pitch;
		lpDest+=bmWidth;
	}

	//over
	bmCurrentX+=w+1; //add 1 for black border
	bitmapDirty=true;
	return true;
}

bool SimpleFont::GetCharMetric(int c,SimpleFontGlyphMetric& metric){
	//get glyph index
	int glyphIndex=FT_Get_Char_Index(data->face,c);

	std::map<int,SimpleFontGlyphMetric>::const_iterator it=glyphMap.find(glyphIndex);
	if(it==glyphMap.end()) it=glyphMap.find(0);
	if(it==glyphMap.end()) return false;
	metric=it->second;
	return true;
}
