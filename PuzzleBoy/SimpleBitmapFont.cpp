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

class DrawTextInternalData{
private:
	std::vector<float> v;
	std::vector<unsigned short> idx;
	int xx,yy,ww;
	int nCount,nStartIndex;
	int flags;

public:
	DrawTextInternalData(int flags)
		:xx(0)
		,yy(0)
		,ww(0)
		,nCount(0)
		,nStartIndex(0)
		,flags(flags)
	{
	}

	int Width() const{return ww;}
	int Height() const{return yy+1;}

	void AddChar(int c,float x,float w,float size){
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

			for(int i=nStartIndex;i<nCount;i++){
				v[i*4]+=x;
			}

			nStartIndex=nCount;
			xx=0;
			ww=0;
		}
	}

	void AdjustVerticalPosition(float y,float h,float size){
		if(flags & DrawTextFlags::VCenter){
			y+=(h-size*Height())*0.5f;
		}else if(flags & DrawTextFlags::Bottom){
			y+=(h-size*Height());
		}

		for(int i=0;i<nCount;i++){
			v[i*4+1]+=y;
		}
	}

	void Draw(SDL_Color color){
		glColor4ub(color.r,color.g,color.b,color.a);
		glVertexPointer(2,GL_FLOAT,4*sizeof(float),&(v[0]));
		glTexCoordPointer(2,GL_FLOAT,4*sizeof(float),&(v[2]));
		glDrawElements(GL_TRIANGLES,idx.size(),GL_UNSIGNED_SHORT,&(idx[0]));
	}

	bool empty() const{return v.empty();}
};

//TODO: word wrap
void SimpleBitmapFont::DrawString(const u8string& str,float x,float y,float w,float h,float size,int flags,SDL_Color color){
	if(str.empty()) return;

	//generate vertices
	DrawTextInternalData data(flags);
	size_t m=str.size();

	U8STRING_FOR_EACH_CHARACTER_DO_BEGIN(str,i,m,c,'?');

	data.AddChar(c,x,w,size);

	U8STRING_FOR_EACH_CHARACTER_DO_END();

	//calc size and alignment
	data.AddChar(-1,x,w,size);
	if(data.empty()) return;
	data.AdjustVerticalPosition(y,h,size);

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
