#include "SimpleBitmapFont.h"
#include <vector>

#include "include_gl.h"

SimpleBitmapFont::SimpleBitmapFont()
:m_Tex(0)
,m_bDrawing(false)
{
}

SimpleBitmapFont::~SimpleBitmapFont(){
	Destroy();
}

void SimpleBitmapFont::Create(SDL_Surface* surf){
	Destroy();

	glGenTextures(1,&m_Tex);
	glBindTexture(GL_TEXTURE_2D, m_Tex);

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
	if(surf->format->BitsPerPixel==32){
		internalformat=GL_RGBA;
		format=GL_RGBA;
#ifndef ANDROID
		if(surf->format->Rmask!=0xFF) format=GL_BGRA;
#endif
	}else{
		internalformat=GL_RGB;
		format=GL_RGB;
#ifndef ANDROID
		if(surf->format->Rmask!=0xFF) format=GL_BGR;
#endif
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, surf->w, surf->h, 0, format, GL_UNSIGNED_BYTE, surf->pixels);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void SimpleBitmapFont::Destroy(){
	EndDraw();

	if(m_Tex){
		glDeleteTextures(1,&m_Tex);
		m_Tex=0;
	}
}

void SimpleBitmapFont::BeginDraw(){
	if(!m_bDrawing){
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_Tex);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		m_bDrawing=true;
	}
}

SimpleBitmapText::SimpleBitmapText()
	:xx(0)
	,yy(0)
	,ww(0)
	,nCount(0)
	,nRowStartIndex(0)
{
}

void SimpleBitmapText::clear(){
	xx=0;
	yy=0;
	ww=0;
	nCount=0;
	nRowStartIndex=0;

	v.clear();
	idx.clear();
	stringIdx.clear();
}

void SimpleBitmapText::AddChar(int c,float x,float w,float size,int flags){
	bool bNewLine=false;

	//add character
	switch(c){
	case -1:
		bNewLine=true;
		break;
	case ' ':
		xx++;
		break;
	case '\t':
		xx=(xx&(-4))+4;
		break;
	case '\b':
		if(xx>0) xx--;
		break;
	case '\n':
		if(flags & DrawTextFlags::Multiline) yy++;
		//fall-through
	case '\r':
		if(flags & DrawTextFlags::Multiline) bNewLine=true;
		break;
	default:
		{
			if(c<32 || c>=128) c=127;

			float tx=0.0625f*(c&0xF);
			float ty=0.125f*((c>>4)-2);

			float v2[16]={
				size*0.5f*xx,size*yy,tx,ty,
				size*0.5f*(xx+1),size*yy,tx+0.0625f,ty,
				size*0.5f*(xx+1),size*(yy+1),tx+0.0625f,ty+0.125f,
				size*0.5f*xx,size*(yy+1),tx,ty+0.125f,
			};

			unsigned short i2[6]={
				nCount,nCount+1,nCount+2,
				nCount,nCount+2,nCount+3,
			};

			v.insert(v.end(),v2,v2+16);
			idx.insert(idx.end(),i2,i2+6);

			nCount+=4;
			xx++;
		}
		break;
	}

	//calc width
	if(xx>ww) ww=xx;

	//adjust horizontal position
	if(bNewLine){
		if(flags & DrawTextFlags::Center){
			x+=(w-size*0.5f*Width())*0.5f;
		}else if(flags & DrawTextFlags::Right){
			x+=(w-size*0.5f*Width());
		}

		for(int i=nRowStartIndex;i<nCount;i++){
			v[i*4]+=x;
		}

		nRowStartIndex=nCount;
		xx=0;
		ww=0;
	}
}

void SimpleBitmapText::AdjustVerticalPosition(int start,float y,float h,float size,int flags){
	if(flags & DrawTextFlags::VCenter){
		y+=(h-size*Height())*0.5f;
	}else if(flags & DrawTextFlags::Bottom){
		y+=(h-size*Height());
	}

	for(int i=start;i<nCount;i++){
		v[i*4+1]+=y;
	}
}

void SimpleBitmapText::Draw(SDL_Color color,int start,int count){
	if(count==0) return;

	int st=0;
	int sz=idx.size();

	if(start>=0 && start<(int)stringIdx.size()){
		st=stringIdx[start];
		sz=((count<0 || start+count>=(int)stringIdx.size())?
			idx.size():stringIdx[start+count])-st;
	}

	glColor4ub(color.r,color.g,color.b,color.a);

	int base=(st/3)&(-0x08000);

	while(sz>0){
		int sz2=base*3+0x18000-st;
		if(sz2>sz) sz2=sz;

		glVertexPointer(2,GL_FLOAT,4*sizeof(float),&(v[base*8]));
		glTexCoordPointer(2,GL_FLOAT,4*sizeof(float),&(v[base*8+2]));
		glDrawElements(GL_TRIANGLES,sz2,GL_UNSIGNED_SHORT,&(idx[st]));

		base+=0x08000;
		sz-=sz2;
		st+=sz2;
	}
}

int SimpleBitmapText::NewStringIndex(){
	//add string to array
	stringIdx.push_back(idx.size());
	return stringIdx.size()-1;
}

//TODO: word wrap
void SimpleBitmapText::AddString(const u8string& str,float x,float y,float w,float h,float size,int flags){
	int o1=nCount;
	xx=0;
	yy=0;
	ww=0;

	//generate vertices
	size_t m=str.size();

	U8STRING_FOR_EACH_CHARACTER_DO_BEGIN(str,i,m,c,'?');

	AddChar(c,x,w,size,flags);

	U8STRING_FOR_EACH_CHARACTER_DO_END();

	//calc size and alignment
	AddChar(-1,x,w,size,flags);
	AdjustVerticalPosition(o1,y,h,size,flags);
}

void SimpleBitmapFont::DrawString(const u8string& str,float x,float y,float w,float h,float size,int flags,SDL_Color color){
	if(str.empty()) return;

	//generate vertices
	SimpleBitmapText data;
	data.AddString(str,x,y,w,h,size,flags);
	if(data.empty()) return;

	//draw
	bool b=m_bDrawing;

	if(!b) BeginDraw();
	data.Draw(color);
	if(!b) EndDraw();
}

void SimpleBitmapFont::EndDraw(){
	if(m_bDrawing){
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		m_bDrawing=false;
	}
}
